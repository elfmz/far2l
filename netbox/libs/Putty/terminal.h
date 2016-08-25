/*
 * Internals of the Terminal structure, for those other modules
 * which need to look inside it. It would be nice if this could be
 * folded back into terminal.c in future, with an abstraction layer
 * to handle everything that other modules need to know about it;
 * but for the moment, this will do.
 */

#ifndef PUTTY_TERMINAL_H
#define PUTTY_TERMINAL_H

#include "tree234.h"

struct beeptime {
    struct beeptime *next;
    unsigned long ticks;
};

typedef struct {
    int y, x;
} pos;

#ifdef OPTIMISE_SCROLL
struct scrollregion {
    struct scrollregion *next;
    int topline; /* Top line of scroll region. */
    int botline; /* Bottom line of scroll region. */
    int lines; /* Number of lines to scroll by - +ve is forwards. */
};
#endif /* OPTIMISE_SCROLL */

typedef struct termchar termchar;
typedef struct termline termline;

struct termchar {
    /*
     * Any code in terminal.c which definitely needs to be changed
     * when extra fields are added here is labelled with a comment
     * saying FULL-TERMCHAR.
     */
    unsigned long chr;
    unsigned long attr;

    /*
     * The cc_next field is used to link multiple termchars
     * together into a list, so as to fit more than one character
     * into a character cell (Unicode combining characters).
     * 
     * cc_next is a relative offset into the current array of
     * termchars. I.e. to advance to the next character in a list,
     * one does `tc += tc->next'.
     * 
     * Zero means end of list.
     */
    int cc_next;
};

struct termline {
    unsigned short lattr;
    int cols;			       /* number of real columns on the line */
    int size;			       /* number of allocated termchars
					* (cc-lists may make this > cols) */
    int temporary;		       /* TRUE if decompressed from scrollback */
    int cc_free;		       /* offset to first cc in free list */
    struct termchar *chars;
};

struct bidi_cache_entry {
    int width;
    struct termchar *chars;
    int *forward, *backward;	       /* the permutations of line positions */
};

struct terminal_tag {

    int compatibility_level;

    tree234 *scrollback;	       /* lines scrolled off top of screen */
    tree234 *screen;		       /* lines on primary screen */
    tree234 *alt_screen;	       /* lines on alternate screen */
    int disptop;		       /* distance scrolled back (0 or -ve) */
    int tempsblines;		       /* number of lines of .scrollback that
					  can be retrieved onto the terminal
					  ("temporary scrollback") */

    termline **disptext;	       /* buffer of text on real screen */
    int dispcursx, dispcursy;	       /* location of cursor on real screen */
    int curstype;		       /* type of cursor on real screen */

#define VBELL_TIMEOUT (TICKSPERSEC/10) /* visual bell lasts 1/10 sec */

    struct beeptime *beephead, *beeptail;
    int nbeeps;
    int beep_overloaded;
    long lastbeep;

#define TTYPE termchar
#define TSIZE (sizeof(TTYPE))

#ifdef OPTIMISE_SCROLL
    struct scrollregion *scrollhead, *scrolltail;
#endif /* OPTIMISE_SCROLL */

    int default_attr, curr_attr, save_attr;
    termchar basic_erase_char, erase_char;

    bufchain inbuf;		       /* terminal input buffer */
    pos curs;			       /* cursor */
    pos savecurs;		       /* saved cursor position */
    int marg_t, marg_b;		       /* scroll margins */
    int dec_om;			       /* DEC origin mode flag */
    int wrap, wrapnext;		       /* wrap flags */
    int insert;			       /* insert-mode flag */
    int cset;			       /* 0 or 1: which char set */
    int save_cset, save_csattr;	       /* saved with cursor position */
    int save_utf, save_wnext;	       /* saved with cursor position */
    int rvideo;			       /* global reverse video flag */
    unsigned long rvbell_startpoint;   /* for ESC[?5hESC[?5l vbell */
    int cursor_on;		       /* cursor enabled flag */
    int reset_132;		       /* Flag ESC c resets to 80 cols */
    int use_bce;		       /* Use Background coloured erase */
    int cblinker;		       /* When blinking is the cursor on ? */
    int tblinker;		       /* When the blinking text is on */
    int blink_is_real;		       /* Actually blink blinking text */
    int term_echoing;		       /* Does terminal want local echo? */
    int term_editing;		       /* Does terminal want local edit? */
    int sco_acs, save_sco_acs;	       /* CSI 10,11,12m -> OEM charset */
    int vt52_bold;		       /* Force bold on non-bold colours */
    int utf;			       /* Are we in toggleable UTF-8 mode? */
    int utf_state;		       /* Is there a pending UTF-8 character */
    int utf_char;		       /* and what is it so far. */
    int utf_size;		       /* The size of the UTF character. */
    int printing, only_printing;       /* Are we doing ANSI printing? */
    int print_state;		       /* state of print-end-sequence scan */
    bufchain printer_buf;	       /* buffered data for printer */
    printer_job *print_job;

    /* ESC 7 saved state for the alternate screen */
    pos alt_savecurs;
    int alt_save_attr;
    int alt_save_cset, alt_save_csattr;
    int alt_save_utf, alt_save_wnext;
    int alt_save_sco_acs;

    int rows, cols, savelines;
    int has_focus;
    int in_vbell;
    long vbell_end;
    int app_cursor_keys, app_keypad_keys, vt52_mode;
    int repeat_off, cr_lf_return;
    int seen_disp_event;
    int big_cursor;

    int xterm_mouse;		       /* send mouse messages to host */
    int xterm_extended_mouse;
    int urxvt_extended_mouse;
    int mouse_is_down;		       /* used while tracking mouse buttons */

    int bracketed_paste;

    int cset_attr[2];

/*
 * Saved settings on the alternate screen.
 */
    int alt_x, alt_y, alt_om, alt_wrap, alt_wnext, alt_ins;
    int alt_cset, alt_sco_acs, alt_utf;
    int alt_t, alt_b;
    int alt_which;
    int alt_sblines; /* # of lines on alternate screen that should be used for scrollback. */

#define ARGS_MAX 32		       /* max # of esc sequence arguments */
#define ARG_DEFAULT 0		       /* if an arg isn't specified */
#define def(a,d) ( (a) == ARG_DEFAULT ? (d) : (a) )
    unsigned esc_args[ARGS_MAX];
    int esc_nargs;
    int esc_query;
#define ANSI(x,y)	((x)+((y)<<8))
#define ANSI_QUE(x)	ANSI(x,TRUE)

#define OSC_STR_MAX 2048
    int osc_strlen;
    char osc_string[OSC_STR_MAX + 1];
    int osc_w;

    char id_string[1024];

    unsigned char *tabs;

    enum {
	TOPLEVEL,
	SEEN_ESC,
	SEEN_CSI,
	SEEN_OSC,
	SEEN_OSC_W,

	DO_CTRLS,

	SEEN_OSC_P,
	OSC_STRING, OSC_MAYBE_ST,
	VT52_ESC,
	VT52_Y1,
	VT52_Y2,
	VT52_FG,
	VT52_BG
    } termstate;

    enum {
	NO_SELECTION, ABOUT_TO, DRAGGING, SELECTED
    } selstate;
    enum {
	LEXICOGRAPHIC, RECTANGULAR
    } seltype;
    enum {
	SM_CHAR, SM_WORD, SM_LINE
    } selmode;
    pos selstart, selend, selanchor;

    short wordness[256];

    /* Mask of attributes to pay attention to when painting. */
    int attr_mask;

    wchar_t *paste_buffer;
    int paste_len, paste_pos;

    void (*resize_fn)(void *, int, int);
    void *resize_ctx;

    void *ldisc;

    void *frontend;

    void *logctx;

    struct unicode_data *ucsdata;

    /*
     * We maintain a full copy of a Conf here, not merely a pointer
     * to it. That way, when we're passed a new one for
     * reconfiguration, we can check the differences and adjust the
     * _current_ setting of (e.g.) auto wrap mode rather than only
     * the default.
     */
    Conf *conf;

    /*
     * from_backend calls term_out, but it can also be called from
     * the ldisc if the ldisc is called _within_ term_out. So we
     * have to guard against re-entrancy - if from_backend is
     * called recursively like this, it will simply add data to the
     * end of the buffer term_out is in the process of working
     * through.
     */
    int in_term_out;

    /*
     * We schedule a window update shortly after receiving terminal
     * data. This tracks whether one is currently pending.
     */
    int window_update_pending;
    long next_update;

    /*
     * Track pending blinks and tblinks.
     */
    int tblink_pending, cblink_pending;
    long next_tblink, next_cblink;

    /*
     * These are buffers used by the bidi and Arabic shaping code.
     */
    termchar *ltemp;
    int ltemp_size;
    bidi_char *wcFrom, *wcTo;
    int wcFromTo_size;
    struct bidi_cache_entry *pre_bidi_cache, *post_bidi_cache;
    int bidi_cache_size;

    /*
     * We copy a bunch of stuff out of the Conf structure into local
     * fields in the Terminal structure, to avoid the repeated
     * tree234 lookups which would be involved in fetching them from
     * the former every time.
     */
    int ansi_colour;
    char *answerback;
    int answerbacklen;
    int arabicshaping;
    int beep;
    int bellovl;
    int bellovl_n;
    int bellovl_s;
    int bellovl_t;
    int bidi;
    int bksp_is_delete;
    int blink_cur;
    int blinktext;
    int cjk_ambig_wide;
    int conf_height;
    int conf_width;
    int crhaslf;
    int erase_to_scrollback;
    int funky_type;
    int lfhascr;
    int logflush;
    int logtype;
    int mouse_override;
    int nethack_keypad;
    int no_alt_screen;
    int no_applic_c;
    int no_applic_k;
    int no_dbackspace;
    int no_mouse_rep;
    int no_remote_charset;
    int no_remote_resize;
    int no_remote_wintitle;
    int rawcnp;
    int rect_select;
    int remote_qtitle_action;
    int rxvt_homeend;
    int scroll_on_disp;
    int scroll_on_key;
    int xterm_256_colour;
};

#define in_utf(term) ((term)->utf || (term)->ucsdata->line_codepage==CP_UTF8)

#endif
