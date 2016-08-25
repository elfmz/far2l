/*
 * Header file for gtkfont.c. Has to be separate from unix.h
 * because it depends on GTK data types, hence can't be included
 * from cross-platform code (which doesn't go near GTK).
 */

#ifndef PUTTY_GTKFONT_H
#define PUTTY_GTKFONT_H

/*
 * Exports from gtkfont.c.
 */
struct unifont_vtable;		       /* contents internal to gtkfont.c */
typedef struct unifont {
    const struct unifont_vtable *vt;
    /*
     * `Non-static data members' of the `class', accessible to
     * external code.
     */

    /*
     * public_charset is the charset used when the user asks for
     * `Use font encoding'.
     */
    int public_charset;

    /*
     * Font dimensions needed by clients.
     */
    int width, height, ascent, descent;

    /*
     * Indicates whether this font is capable of handling all glyphs
     * (Pango fonts can do this because Pango automatically supplies
     * missing glyphs from other fonts), or whether it would like a
     * fallback font to cope with missing glyphs.
     */
    int want_fallback;
} unifont;

unifont *unifont_create(GtkWidget *widget, const char *name,
			int wide, int bold,
			int shadowoffset, int shadowalways);
void unifont_destroy(unifont *font);
void unifont_draw_text(GdkDrawable *target, GdkGC *gc, unifont *font,
		       int x, int y, const wchar_t *string, int len,
		       int wide, int bold, int cellwidth);

/*
 * This function behaves exactly like the low-level unifont_create,
 * except that as well as the requested font it also allocates (if
 * necessary) a fallback font for filling in replacement glyphs.
 *
 * Return value is usable with unifont_destroy and unifont_draw_text
 * as if it were an ordinary unifont.
 */
unifont *multifont_create(GtkWidget *widget, const char *name,
                          int wide, int bold,
                          int shadowoffset, int shadowalways);

/*
 * Unified font selector dialog. I can't be bothered to do a
 * proper GTK subclassing today, so this will just be an ordinary
 * data structure with some useful members.
 * 
 * (Of course, these aren't the only members; this structure is
 * contained within a bigger one which holds data visible only to
 * the implementation.)
 */
typedef struct unifontsel {
    void *user_data;		       /* settable by the user */
    GtkWindow *window;
    GtkWidget *ok_button, *cancel_button;
} unifontsel;

unifontsel *unifontsel_new(const char *wintitle);
void unifontsel_destroy(unifontsel *fontsel);
void unifontsel_set_name(unifontsel *fontsel, const char *fontname);
char *unifontsel_get_name(unifontsel *fontsel);

#endif /* PUTTY_GTKFONT_H */
