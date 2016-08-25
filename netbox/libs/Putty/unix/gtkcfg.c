/*
 * gtkcfg.c - the GTK-specific parts of the PuTTY configuration
 * box.
 */

#include <assert.h>
#include <stdlib.h>

#include "putty.h"
#include "dialog.h"
#include "storage.h"

static void about_handler(union control *ctrl, void *dlg,
			  void *data, int event)
{
    if (event == EVENT_ACTION) {
	about_box(ctrl->generic.context.p);
    }
}

void gtk_setup_config_box(struct controlbox *b, int midsession, void *win)
{
    struct controlset *s, *s2;
    union control *c;
    int i;

    if (!midsession) {
	/*
	 * Add the About button to the standard panel.
	 */
	s = ctrl_getset(b, "", "", "");
	c = ctrl_pushbutton(s, "About", 'a', HELPCTX(no_help),
			    about_handler, P(win));
	c->generic.column = 0;
    }

    /*
     * GTK makes it rather easier to put the scrollbar on the left
     * than Windows does!
     */
    s = ctrl_getset(b, "Window", "scrollback",
		    "Control the scrollback in the window");
    ctrl_checkbox(s, "Scrollbar on left", 'l',
		  HELPCTX(no_help),
		  conf_checkbox_handler,
                  I(CONF_scrollbar_on_left));
    /*
     * Really this wants to go just after `Display scrollbar'. See
     * if we can find that control, and do some shuffling.
     */
    for (i = 0; i < s->ncontrols; i++) {
        c = s->ctrls[i];
        if (c->generic.type == CTRL_CHECKBOX &&
            c->generic.context.i == CONF_scrollbar) {
            /*
             * Control i is the scrollbar checkbox.
             * Control s->ncontrols-1 is the scrollbar-on-left one.
             */
            if (i < s->ncontrols-2) {
                c = s->ctrls[s->ncontrols-1];
                memmove(s->ctrls+i+2, s->ctrls+i+1,
                        (s->ncontrols-i-2)*sizeof(union control *));
                s->ctrls[i+1] = c;
            }
            break;
        }
    }

    /*
     * X requires three more fonts: bold, wide, and wide-bold; also
     * we need the fiddly shadow-bold-offset control. This would
     * make the Window/Appearance panel rather unwieldy and large,
     * so I think the sensible thing here is to _move_ this
     * controlset into a separate Window/Fonts panel!
     */
    s2 = ctrl_getset(b, "Window/Appearance", "font",
                     "Font settings");
    /* Remove this controlset from b. */
    for (i = 0; i < b->nctrlsets; i++) {
        if (b->ctrlsets[i] == s2) {
            memmove(b->ctrlsets+i, b->ctrlsets+i+1,
                    (b->nctrlsets-i-1) * sizeof(*b->ctrlsets));
            b->nctrlsets--;
            ctrl_free_set(s2);
            break;
        }
    }
    ctrl_settitle(b, "Window/Fonts", "Options controlling font usage");
    s = ctrl_getset(b, "Window/Fonts", "font",
                    "Fonts for displaying non-bold text");
    ctrl_fontsel(s, "Font used for ordinary text", 'f',
		 HELPCTX(no_help),
		 conf_fontsel_handler, I(CONF_font));
    ctrl_fontsel(s, "Font used for wide (CJK) text", 'w',
		 HELPCTX(no_help),
		 conf_fontsel_handler, I(CONF_widefont));
    s = ctrl_getset(b, "Window/Fonts", "fontbold",
                    "Fonts for displaying bolded text");
    ctrl_fontsel(s, "Font used for bolded text", 'b',
		 HELPCTX(no_help),
		 conf_fontsel_handler, I(CONF_boldfont));
    ctrl_fontsel(s, "Font used for bold wide text", 'i',
		 HELPCTX(no_help),
		 conf_fontsel_handler, I(CONF_wideboldfont));
    ctrl_checkbox(s, "Use shadow bold instead of bold fonts", 'u',
		  HELPCTX(no_help),
		  conf_checkbox_handler,
		  I(CONF_shadowbold));
    ctrl_text(s, "(Note that bold fonts or shadow bolding are only"
	      " used if you have not requested bolding to be done by"
	      " changing the text colour.)",
              HELPCTX(no_help));
    ctrl_editbox(s, "Horizontal offset for shadow bold:", 'z', 20,
		 HELPCTX(no_help), conf_editbox_handler,
                 I(CONF_shadowboldoffset), I(-1));

    /*
     * Markus Kuhn feels, not totally unreasonably, that it's good
     * for all applications to shift into UTF-8 mode if they notice
     * that they've been started with a LANG setting dictating it,
     * so that people don't have to keep remembering a separate
     * UTF-8 option for every application they use. Therefore,
     * here's an override option in the Translation panel.
     */
    s = ctrl_getset(b, "Window/Translation", "trans",
		    "Character set translation on received data");
    ctrl_checkbox(s, "Override with UTF-8 if locale says so", 'l',
		  HELPCTX(translation_utf8_override),
		  conf_checkbox_handler,
		  I(CONF_utf8_override));

    if (!midsession) {
        /*
         * Allow the user to specify the window class as part of the saved
         * configuration, so that they can have their window manager treat
         * different kinds of PuTTY and pterm differently if they want to.
         */
        s = ctrl_getset(b, "Window/Behaviour", "x11",
                        "X Window System settings");
        ctrl_editbox(s, "Window class name:", 'z', 50,
                     HELPCTX(no_help), conf_editbox_handler,
                     I(CONF_winclass), I(1));
    }
}
