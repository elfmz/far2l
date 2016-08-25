/*
 * dialog.c - a reasonably platform-independent mechanism for
 * describing dialog boxes.
 */

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>

#define DEFINE_INTORPTR_FNS

#include "putty.h"
#include "dialog.h"

int ctrl_path_elements(char *path)
{
    int i = 1;
    while (*path) {
	if (*path == '/') i++;
	path++;
    }
    return i;
}

/* Return the number of matching path elements at the starts of p1 and p2,
 * or INT_MAX if the paths are identical. */
int ctrl_path_compare(char *p1, char *p2)
{
    int i = 0;
    while (*p1 || *p2) {
	if ((*p1 == '/' || *p1 == '\0') &&
	    (*p2 == '/' || *p2 == '\0'))
	    i++;		       /* a whole element matches, ooh */
	if (*p1 != *p2)
	    return i;		       /* mismatch */
	p1++, p2++;
    }
    return INT_MAX;		       /* exact match */
}

struct controlbox *ctrl_new_box(void)
{
    struct controlbox *ret = snew(struct controlbox);

    ret->nctrlsets = ret->ctrlsetsize = 0;
    ret->ctrlsets = NULL;
    ret->nfrees = ret->freesize = 0;
    ret->frees = NULL;
    ret->freefuncs = NULL;

    return ret;
}

void ctrl_free_box(struct controlbox *b)
{
    int i;

    for (i = 0; i < b->nctrlsets; i++) {
	ctrl_free_set(b->ctrlsets[i]);
    }
    for (i = 0; i < b->nfrees; i++)
	b->freefuncs[i](b->frees[i]);
    sfree(b->ctrlsets);
    sfree(b->frees);
    sfree(b->freefuncs);
    sfree(b);
}

void ctrl_free_set(struct controlset *s)
{
    int i;

    sfree(s->pathname);
    sfree(s->boxname);
    sfree(s->boxtitle);
    for (i = 0; i < s->ncontrols; i++) {
	ctrl_free(s->ctrls[i]);
    }
    sfree(s->ctrls);
    sfree(s);
}

/*
 * Find the index of first controlset in a controlbox for a given
 * path. If that path doesn't exist, return the index where it
 * should be inserted.
 */
static int ctrl_find_set(struct controlbox *b, char *path, int start)
{
    int i, last, thisone;

    last = 0;
    for (i = 0; i < b->nctrlsets; i++) {
	thisone = ctrl_path_compare(path, b->ctrlsets[i]->pathname);
	/*
	 * If `start' is true and there exists a controlset with
	 * exactly the path we've been given, we should return the
	 * index of the first such controlset we find. Otherwise,
	 * we should return the index of the first entry in which
	 * _fewer_ path elements match than they did last time.
	 */
	if ((start && thisone == INT_MAX) || thisone < last)
	    return i;
	last = thisone;
    }
    return b->nctrlsets;	       /* insert at end */
}

/*
 * Find the index of next controlset in a controlbox for a given
 * path, or -1 if no such controlset exists. If -1 is passed as
 * input, finds the first.
 */
int ctrl_find_path(struct controlbox *b, char *path, int index)
{
    if (index < 0)
	index = ctrl_find_set(b, path, 1);
    else
	index++;

    if (index < b->nctrlsets && !strcmp(path, b->ctrlsets[index]->pathname))
	return index;
    else
	return -1;
}

/* Set up a panel title. */
struct controlset *ctrl_settitle(struct controlbox *b,
				 char *path, char *title)
{
    
    struct controlset *s = snew(struct controlset);
    int index = ctrl_find_set(b, path, 1);
    s->pathname = dupstr(path);
    s->boxname = NULL;
    s->boxtitle = dupstr(title);
    s->ncontrols = s->ctrlsize = 0;
    s->ncolumns = 0;		       /* this is a title! */
    s->ctrls = NULL;
    if (b->nctrlsets >= b->ctrlsetsize) {
	b->ctrlsetsize = b->nctrlsets + 32;
	b->ctrlsets = sresize(b->ctrlsets, b->ctrlsetsize,struct controlset *);
    }
    if (index < b->nctrlsets)
	memmove(&b->ctrlsets[index+1], &b->ctrlsets[index],
		(b->nctrlsets-index) * sizeof(*b->ctrlsets));
    b->ctrlsets[index] = s;
    b->nctrlsets++;
    return s;
}

/* Retrieve a pointer to a controlset, creating it if absent. */
struct controlset *ctrl_getset(struct controlbox *b,
			       char *path, char *name, char *boxtitle)
{
    struct controlset *s;
    int index = ctrl_find_set(b, path, 1);
    while (index < b->nctrlsets &&
	   !strcmp(b->ctrlsets[index]->pathname, path)) {
	if (b->ctrlsets[index]->boxname &&
	    !strcmp(b->ctrlsets[index]->boxname, name))
	    return b->ctrlsets[index];
	index++;
    }
    s = snew(struct controlset);
    s->pathname = dupstr(path);
    s->boxname = dupstr(name);
    s->boxtitle = boxtitle ? dupstr(boxtitle) : NULL;
    s->ncolumns = 1;
    s->ncontrols = s->ctrlsize = 0;
    s->ctrls = NULL;
    if (b->nctrlsets >= b->ctrlsetsize) {
	b->ctrlsetsize = b->nctrlsets + 32;
	b->ctrlsets = sresize(b->ctrlsets, b->ctrlsetsize,struct controlset *);
    }
    if (index < b->nctrlsets)
	memmove(&b->ctrlsets[index+1], &b->ctrlsets[index],
		(b->nctrlsets-index) * sizeof(*b->ctrlsets));
    b->ctrlsets[index] = s;
    b->nctrlsets++;
    return s;
}

/* Allocate some private data in a controlbox. */
void *ctrl_alloc_with_free(struct controlbox *b, size_t size,
                           ctrl_freefn_t freefunc)
{
    void *p;
    /*
     * This is an internal allocation routine, so it's allowed to
     * use smalloc directly.
     */
    p = smalloc(size);
    if (b->nfrees >= b->freesize) {
	b->freesize = b->nfrees + 32;
	b->frees = sresize(b->frees, b->freesize, void *);
	b->freefuncs = sresize(b->freefuncs, b->freesize, ctrl_freefn_t);
    }
    b->frees[b->nfrees] = p;
    b->freefuncs[b->nfrees] = freefunc;
    b->nfrees++;
    return p;
}

static void ctrl_default_free(void *p)
{
    sfree(p);
}

void *ctrl_alloc(struct controlbox *b, size_t size)
{
    return ctrl_alloc_with_free(b, size, ctrl_default_free);
}

static union control *ctrl_new(struct controlset *s, int type,
			       intorptr helpctx, handler_fn handler,
			       intorptr context)
{
    union control *c = snew(union control);
    if (s->ncontrols >= s->ctrlsize) {
	s->ctrlsize = s->ncontrols + 32;
	s->ctrls = sresize(s->ctrls, s->ctrlsize, union control *);
    }
    s->ctrls[s->ncontrols++] = c;
    /*
     * Fill in the standard fields.
     */
    c->generic.type = type;
    c->generic.tabdelay = 0;
    c->generic.column = COLUMN_FIELD(0, s->ncolumns);
    c->generic.helpctx = helpctx;
    c->generic.handler = handler;
    c->generic.context = context;
    c->generic.label = NULL;
    return c;
}

/* `ncolumns' is followed by that many percentages, as integers. */
union control *ctrl_columns(struct controlset *s, int ncolumns, ...)
{
    union control *c = ctrl_new(s, CTRL_COLUMNS, P(NULL), NULL, P(NULL));
    assert(s->ncolumns == 1 || ncolumns == 1);
    c->columns.ncols = ncolumns;
    s->ncolumns = ncolumns;
    if (ncolumns == 1) {
	c->columns.percentages = NULL;
    } else {
	va_list ap;
	int i;
	c->columns.percentages = snewn(ncolumns, int);
	va_start(ap, ncolumns);
	for (i = 0; i < ncolumns; i++)
	    c->columns.percentages[i] = va_arg(ap, int);
	va_end(ap);
    }
    return c;
}

union control *ctrl_editbox(struct controlset *s, char *label, char shortcut,
			    int percentage,
			    intorptr helpctx, handler_fn handler,
			    intorptr context, intorptr context2)
{
    union control *c = ctrl_new(s, CTRL_EDITBOX, helpctx, handler, context);
    c->editbox.label = label ? dupstr(label) : NULL;
    c->editbox.shortcut = shortcut;
    c->editbox.percentwidth = percentage;
    c->editbox.password = 0;
    c->editbox.has_list = 0;
    c->editbox.context2 = context2;
    return c;
}

union control *ctrl_combobox(struct controlset *s, char *label, char shortcut,
			     int percentage,
			     intorptr helpctx, handler_fn handler,
			     intorptr context, intorptr context2)
{
    union control *c = ctrl_new(s, CTRL_EDITBOX, helpctx, handler, context);
    c->editbox.label = label ? dupstr(label) : NULL;
    c->editbox.shortcut = shortcut;
    c->editbox.percentwidth = percentage;
    c->editbox.password = 0;
    c->editbox.has_list = 1;
    c->editbox.context2 = context2;
    return c;
}

/*
 * `ncolumns' is followed by (alternately) radio button titles and
 * intorptrs, until a NULL in place of a title string is seen. Each
 * title is expected to be followed by a shortcut _iff_ `shortcut'
 * is NO_SHORTCUT.
 */
union control *ctrl_radiobuttons(struct controlset *s, char *label,
				 char shortcut, int ncolumns, intorptr helpctx,
				 handler_fn handler, intorptr context, ...)
{
    va_list ap;
    int i;
    union control *c = ctrl_new(s, CTRL_RADIO, helpctx, handler, context);
    c->radio.label = label ? dupstr(label) : NULL;
    c->radio.shortcut = shortcut;
    c->radio.ncolumns = ncolumns;
    /*
     * Initial pass along variable argument list to count the
     * buttons.
     */
    va_start(ap, context);
    i = 0;
    while (va_arg(ap, char *) != NULL) {
	i++;
	if (c->radio.shortcut == NO_SHORTCUT)
	    (void)va_arg(ap, int);     /* char promotes to int in arg lists */
	(void)va_arg(ap, intorptr);
    }
    va_end(ap);
    c->radio.nbuttons = i;
    if (c->radio.shortcut == NO_SHORTCUT)
	c->radio.shortcuts = snewn(c->radio.nbuttons, char);
    else
	c->radio.shortcuts = NULL;
    c->radio.buttons = snewn(c->radio.nbuttons, char *);
    c->radio.buttondata = snewn(c->radio.nbuttons, intorptr);
    /*
     * Second pass along variable argument list to actually fill in
     * the structure.
     */
    va_start(ap, context);
    for (i = 0; i < c->radio.nbuttons; i++) {
	c->radio.buttons[i] = dupstr(va_arg(ap, char *));
	if (c->radio.shortcut == NO_SHORTCUT)
	    c->radio.shortcuts[i] = va_arg(ap, int);
				       /* char promotes to int in arg lists */
	c->radio.buttondata[i] = va_arg(ap, intorptr);
    }
    va_end(ap);
    return c;
}

union control *ctrl_pushbutton(struct controlset *s,char *label,char shortcut,
			       intorptr helpctx, handler_fn handler,
			       intorptr context)
{
    union control *c = ctrl_new(s, CTRL_BUTTON, helpctx, handler, context);
    c->button.label = label ? dupstr(label) : NULL;
    c->button.shortcut = shortcut;
    c->button.isdefault = 0;
    c->button.iscancel = 0;
    return c;
}

union control *ctrl_listbox(struct controlset *s,char *label,char shortcut,
			    intorptr helpctx, handler_fn handler,
			    intorptr context)
{
    union control *c = ctrl_new(s, CTRL_LISTBOX, helpctx, handler, context);
    c->listbox.label = label ? dupstr(label) : NULL;
    c->listbox.shortcut = shortcut;
    c->listbox.height = 5;	       /* *shrug* a plausible default */
    c->listbox.draglist = 0;
    c->listbox.multisel = 0;
    c->listbox.percentwidth = 100;
    c->listbox.ncols = 0;
    c->listbox.percentages = NULL;
    c->listbox.hscroll = TRUE;
    return c;
}

union control *ctrl_droplist(struct controlset *s, char *label, char shortcut,
			     int percentage, intorptr helpctx,
			     handler_fn handler, intorptr context)
{
    union control *c = ctrl_new(s, CTRL_LISTBOX, helpctx, handler, context);
    c->listbox.label = label ? dupstr(label) : NULL;
    c->listbox.shortcut = shortcut;
    c->listbox.height = 0;	       /* means it's a drop-down list */
    c->listbox.draglist = 0;
    c->listbox.multisel = 0;
    c->listbox.percentwidth = percentage;
    c->listbox.ncols = 0;
    c->listbox.percentages = NULL;
    c->listbox.hscroll = FALSE;
    return c;
}

union control *ctrl_draglist(struct controlset *s,char *label,char shortcut,
			     intorptr helpctx, handler_fn handler,
			     intorptr context)
{
    union control *c = ctrl_new(s, CTRL_LISTBOX, helpctx, handler, context);
    c->listbox.label = label ? dupstr(label) : NULL;
    c->listbox.shortcut = shortcut;
    c->listbox.height = 5;	       /* *shrug* a plausible default */
    c->listbox.draglist = 1;
    c->listbox.multisel = 0;
    c->listbox.percentwidth = 100;
    c->listbox.ncols = 0;
    c->listbox.percentages = NULL;
    c->listbox.hscroll = FALSE;
    return c;
}

union control *ctrl_filesel(struct controlset *s,char *label,char shortcut,
			    char const *filter, int write, char *title,
			    intorptr helpctx, handler_fn handler,
			    intorptr context)
{
    union control *c = ctrl_new(s, CTRL_FILESELECT, helpctx, handler, context);
    c->fileselect.label = label ? dupstr(label) : NULL;
    c->fileselect.shortcut = shortcut;
    c->fileselect.filter = filter;
    c->fileselect.for_writing = write;
    c->fileselect.title = dupstr(title);
    return c;
}

union control *ctrl_fontsel(struct controlset *s,char *label,char shortcut,
			    intorptr helpctx, handler_fn handler,
			    intorptr context)
{
    union control *c = ctrl_new(s, CTRL_FONTSELECT, helpctx, handler, context);
    c->fontselect.label = label ? dupstr(label) : NULL;
    c->fontselect.shortcut = shortcut;
    return c;
}

union control *ctrl_tabdelay(struct controlset *s, union control *ctrl)
{
    union control *c = ctrl_new(s, CTRL_TABDELAY, P(NULL), NULL, P(NULL));
    c->tabdelay.ctrl = ctrl;
    return c;
}

union control *ctrl_text(struct controlset *s, char *text, intorptr helpctx)
{
    union control *c = ctrl_new(s, CTRL_TEXT, helpctx, NULL, P(NULL));
    c->text.label = dupstr(text);
    return c;
}

union control *ctrl_checkbox(struct controlset *s, char *label, char shortcut,
			     intorptr helpctx, handler_fn handler,
			     intorptr context)
{
    union control *c = ctrl_new(s, CTRL_CHECKBOX, helpctx, handler, context);
    c->checkbox.label = label ? dupstr(label) : NULL;
    c->checkbox.shortcut = shortcut;
    return c;
}

void ctrl_free(union control *ctrl)
{
    int i;

    sfree(ctrl->generic.label);
    switch (ctrl->generic.type) {
      case CTRL_RADIO:
	for (i = 0; i < ctrl->radio.nbuttons; i++)
	    sfree(ctrl->radio.buttons[i]);
	sfree(ctrl->radio.buttons);
	sfree(ctrl->radio.shortcuts);
	sfree(ctrl->radio.buttondata);
	break;
      case CTRL_COLUMNS:
	sfree(ctrl->columns.percentages);
	break;
      case CTRL_LISTBOX:
	sfree(ctrl->listbox.percentages);
	break;
      case CTRL_FILESELECT:
	sfree(ctrl->fileselect.title);
	break;
    }
    sfree(ctrl);
}
