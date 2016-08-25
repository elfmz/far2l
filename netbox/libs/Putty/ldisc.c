/*
 * ldisc.c: PuTTY line discipline. Sits between the input coming
 * from keypresses in the window, and the output channel leading to
 * the back end. Implements echo and/or local line editing,
 * depending on what's currently configured.
 */

#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "putty.h"
#include "terminal.h"
#include "ldisc.h"

#define ECHOING (ldisc->localecho == FORCE_ON || \
                 (ldisc->localecho == AUTO && \
                      (ldisc->back->ldisc(ldisc->backhandle, LD_ECHO) || \
			   term_ldisc(ldisc->term, LD_ECHO))))
#define EDITING (ldisc->localedit == FORCE_ON || \
                 (ldisc->localedit == AUTO && \
                      (ldisc->back->ldisc(ldisc->backhandle, LD_EDIT) || \
			   term_ldisc(ldisc->term, LD_EDIT))))

static void c_write(Ldisc ldisc, char *buf, int len)
{
    from_backend(ldisc->frontend, 0, buf, len);
}

static int plen(Ldisc ldisc, unsigned char c)
{
    if ((c >= 32 && c <= 126) || (c >= 160 && !in_utf(ldisc->term)))
	return 1;
    else if (c < 128)
	return 2;		       /* ^x for some x */
    else if (in_utf(ldisc->term) && c >= 0xC0)
	return 1;		       /* UTF-8 introducer character
					* (FIXME: combining / wide chars) */
    else if (in_utf(ldisc->term) && c >= 0x80 && c < 0xC0)
	return 0;		       /* UTF-8 followup character */
    else
	return 4;		       /* <XY> hex representation */
}

static void pwrite(Ldisc ldisc, unsigned char c)
{
    if ((c >= 32 && c <= 126) ||
	(!in_utf(ldisc->term) && c >= 0xA0) ||
	(in_utf(ldisc->term) && c >= 0x80)) {
	c_write(ldisc, (char *)&c, 1);
    } else if (c < 128) {
	char cc[2];
	cc[1] = (c == 127 ? '?' : c + 0x40);
	cc[0] = '^';
	c_write(ldisc, cc, 2);
    } else {
	char cc[5];
	sprintf(cc, "<%02X>", c);
	c_write(ldisc, cc, 4);
    }
}

static int char_start(Ldisc ldisc, unsigned char c)
{
    if (in_utf(ldisc->term))
	return (c < 0x80 || c >= 0xC0);
    else
	return 1;
}

static void bsb(Ldisc ldisc, int n)
{
    while (n--)
	c_write(ldisc, "\010 \010", 3);
}

#define CTRL(x) (x^'@')
#define KCTRL(x) ((x^'@') | 0x100)

void *ldisc_create(Conf *conf, Terminal *term,
		   Backend *back, void *backhandle,
		   void *frontend)
{
    Ldisc ldisc = snew(struct ldisc_tag);

    ldisc->buf = NULL;
    ldisc->buflen = 0;
    ldisc->bufsiz = 0;
    ldisc->quotenext = 0;

    ldisc->back = back;
    ldisc->backhandle = backhandle;
    ldisc->term = term;
    ldisc->frontend = frontend;

    ldisc_configure(ldisc, conf);

    /* Link ourselves into the backend and the terminal */
    if (term)
	term->ldisc = ldisc;
    if (back)
	back->provide_ldisc(backhandle, ldisc);

    return ldisc;
}

void ldisc_configure(void *handle, Conf *conf)
{
    Ldisc ldisc = (Ldisc) handle;

    ldisc->telnet_keyboard = conf_get_int(conf, CONF_telnet_keyboard);
    ldisc->telnet_newline = conf_get_int(conf, CONF_telnet_newline);
    ldisc->protocol = conf_get_int(conf, CONF_protocol);
    ldisc->localecho = conf_get_int(conf, CONF_localecho);
    ldisc->localedit = conf_get_int(conf, CONF_localedit);
}

void ldisc_free(void *handle)
{
    Ldisc ldisc = (Ldisc) handle;

    if (ldisc->term)
	ldisc->term->ldisc = NULL;
    if (ldisc->back)
	ldisc->back->provide_ldisc(ldisc->backhandle, NULL);
    if (ldisc->buf)
	sfree(ldisc->buf);
    sfree(ldisc);
}

void ldisc_send(void *handle, char *buf, int len, int interactive)
{
    Ldisc ldisc = (Ldisc) handle;
    int keyflag = 0;
    /*
     * Called with len=0 when the options change. We must inform
     * the front end in case it needs to know.
     */
    if (len == 0) {
	ldisc_update(ldisc->frontend, ECHOING, EDITING);
	return;
    }

    /*
     * If that wasn't true, then we expect ldisc->term to be non-NULL
     * hereafter. (The only front ends which have an ldisc but no term
     * are those which do networking but no terminal emulation, in
     * which case they need the above if statement to handle
     * ldisc_updates passed from the back ends, but should never send
     * any actual input through this function.)
     */
    assert(ldisc->term);

    /*
     * Notify the front end that something was pressed, in case
     * it's depending on finding out (e.g. keypress termination for
     * Close On Exit). 
     */
    frontend_keypress(ldisc->frontend);

    if (interactive) {
        /*
         * Interrupt a paste from the clipboard, if one was in
         * progress when the user pressed a key. This is easier than
         * buffering the current piece of data and saving it until the
         * terminal has finished pasting, and has the potential side
         * benefit of permitting a user to cancel an accidental huge
         * paste.
         */
        term_nopaste(ldisc->term);
    }

    /*
     * Less than zero means null terminated special string.
     */
    if (len < 0) {
	len = strlen(buf);
	keyflag = KCTRL('@');
    }
    /*
     * Either perform local editing, or just send characters.
     */
    if (EDITING) {
	while (len--) {
	    int c;
	    c = (unsigned char)(*buf++) + keyflag;
	    if (!interactive && c == '\r')
		c += KCTRL('@');
	    switch (ldisc->quotenext ? ' ' : c) {
		/*
		 * ^h/^?: delete, and output BSBs, to return to
		 * last character boundary (in UTF-8 mode this may
		 * be more than one byte)
		 * ^w: delete, and output BSBs, to return to last
		 * space/nonspace boundary
		 * ^u: delete, and output BSBs, to return to BOL
		 * ^c: Do a ^u then send a telnet IP
		 * ^z: Do a ^u then send a telnet SUSP
		 * ^\: Do a ^u then send a telnet ABORT
		 * ^r: echo "^R\n" and redraw line
		 * ^v: quote next char
		 * ^d: if at BOL, end of file and close connection,
		 * else send line and reset to BOL
		 * ^m: send line-plus-\r\n and reset to BOL
		 */
	      case KCTRL('H'):
	      case KCTRL('?'):	       /* backspace/delete */
		if (ldisc->buflen > 0) {
		    do {
			if (ECHOING)
			    bsb(ldisc, plen(ldisc, ldisc->buf[ldisc->buflen - 1]));
			ldisc->buflen--;
		    } while (!char_start(ldisc, ldisc->buf[ldisc->buflen]));
		}
		break;
	      case CTRL('W'):	       /* delete word */
		while (ldisc->buflen > 0) {
		    if (ECHOING)
			bsb(ldisc, plen(ldisc, ldisc->buf[ldisc->buflen - 1]));
		    ldisc->buflen--;
		    if (ldisc->buflen > 0 &&
			isspace((unsigned char)ldisc->buf[ldisc->buflen-1]) &&
			!isspace((unsigned char)ldisc->buf[ldisc->buflen]))
			break;
		}
		break;
	      case CTRL('U'):	       /* delete line */
	      case CTRL('C'):	       /* Send IP */
	      case CTRL('\\'):	       /* Quit */
	      case CTRL('Z'):	       /* Suspend */
		while (ldisc->buflen > 0) {
		    if (ECHOING)
			bsb(ldisc, plen(ldisc, ldisc->buf[ldisc->buflen - 1]));
		    ldisc->buflen--;
		}
		ldisc->back->special(ldisc->backhandle, TS_EL);
                /*
                 * We don't send IP, SUSP or ABORT if the user has
                 * configured telnet specials off! This breaks
                 * talkers otherwise.
                 */
                if (!ldisc->telnet_keyboard)
                    goto default_case;
		if (c == CTRL('C'))
		    ldisc->back->special(ldisc->backhandle, TS_IP);
		if (c == CTRL('Z'))
		    ldisc->back->special(ldisc->backhandle, TS_SUSP);
		if (c == CTRL('\\'))
		    ldisc->back->special(ldisc->backhandle, TS_ABORT);
		break;
	      case CTRL('R'):	       /* redraw line */
		if (ECHOING) {
		    int i;
		    c_write(ldisc, "^R\r\n", 4);
		    for (i = 0; i < ldisc->buflen; i++)
			pwrite(ldisc, ldisc->buf[i]);
		}
		break;
	      case CTRL('V'):	       /* quote next char */
		ldisc->quotenext = TRUE;
		break;
	      case CTRL('D'):	       /* logout or send */
		if (ldisc->buflen == 0) {
		    ldisc->back->special(ldisc->backhandle, TS_EOF);
		} else {
		    ldisc->back->send(ldisc->backhandle, ldisc->buf, ldisc->buflen);
		    ldisc->buflen = 0;
		}
		break;
		/*
		 * This particularly hideous bit of code from RDB
		 * allows ordinary ^M^J to do the same thing as
		 * magic-^M when in Raw protocol. The line `case
		 * KCTRL('M'):' is _inside_ the if block. Thus:
		 * 
		 *  - receiving regular ^M goes straight to the
		 *    default clause and inserts as a literal ^M.
		 *  - receiving regular ^J _not_ directly after a
		 *    literal ^M (or not in Raw protocol) fails the
		 *    if condition, leaps to the bottom of the if,
		 *    and falls through into the default clause
		 *    again.
		 *  - receiving regular ^J just after a literal ^M
		 *    in Raw protocol passes the if condition,
		 *    deletes the literal ^M, and falls through
		 *    into the magic-^M code
		 *  - receiving a magic-^M empties the line buffer,
		 *    signals end-of-line in one of the various
		 *    entertaining ways, and _doesn't_ fall out of
		 *    the bottom of the if and through to the
		 *    default clause because of the break.
		 */
	      case CTRL('J'):
		if (ldisc->protocol == PROT_RAW &&
		    ldisc->buflen > 0 && ldisc->buf[ldisc->buflen - 1] == '\r') {
		    if (ECHOING)
			bsb(ldisc, plen(ldisc, ldisc->buf[ldisc->buflen - 1]));
		    ldisc->buflen--;
		    /* FALLTHROUGH */
	      case KCTRL('M'):	       /* send with newline */
		    if (ldisc->buflen > 0)
			ldisc->back->send(ldisc->backhandle, ldisc->buf, ldisc->buflen);
		    if (ldisc->protocol == PROT_RAW)
			ldisc->back->send(ldisc->backhandle, "\r\n", 2);
		    else if (ldisc->protocol == PROT_TELNET && ldisc->telnet_newline)
			ldisc->back->special(ldisc->backhandle, TS_EOL);
		    else
			ldisc->back->send(ldisc->backhandle, "\r", 1);
		    if (ECHOING)
			c_write(ldisc, "\r\n", 2);
		    ldisc->buflen = 0;
		    break;
		}
		/* FALLTHROUGH */
	      default:		       /* get to this label from ^V handler */
                default_case:
		if (ldisc->buflen >= ldisc->bufsiz) {
		    ldisc->bufsiz = ldisc->buflen + 256;
		    ldisc->buf = sresize(ldisc->buf, ldisc->bufsiz, char);
		}
		ldisc->buf[ldisc->buflen++] = c;
		if (ECHOING)
		    pwrite(ldisc, (unsigned char) c);
		ldisc->quotenext = FALSE;
		break;
	    }
	}
    } else {
	if (ldisc->buflen != 0) {
	    ldisc->back->send(ldisc->backhandle, ldisc->buf, ldisc->buflen);
	    while (ldisc->buflen > 0) {
		bsb(ldisc, plen(ldisc, ldisc->buf[ldisc->buflen - 1]));
		ldisc->buflen--;
	    }
	}
	if (len > 0) {
	    if (ECHOING)
		c_write(ldisc, buf, len);
	    if (keyflag && ldisc->protocol == PROT_TELNET && len == 1) {
		switch (buf[0]) {
		  case CTRL('M'):
		    if (ldisc->protocol == PROT_TELNET && ldisc->telnet_newline)
			ldisc->back->special(ldisc->backhandle, TS_EOL);
		    else
			ldisc->back->send(ldisc->backhandle, "\r", 1);
		    break;
		  case CTRL('?'):
		  case CTRL('H'):
		    if (ldisc->telnet_keyboard) {
			ldisc->back->special(ldisc->backhandle, TS_EC);
			break;
		    }
		  case CTRL('C'):
		    if (ldisc->telnet_keyboard) {
			ldisc->back->special(ldisc->backhandle, TS_IP);
			break;
		    }
		  case CTRL('Z'):
		    if (ldisc->telnet_keyboard) {
			ldisc->back->special(ldisc->backhandle, TS_SUSP);
			break;
		    }

		  default:
		    ldisc->back->send(ldisc->backhandle, buf, len);
		    break;
		}
	    } else
		ldisc->back->send(ldisc->backhandle, buf, len);
	}
    }
}
