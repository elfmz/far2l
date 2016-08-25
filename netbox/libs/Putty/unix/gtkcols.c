/*
 * gtkcols.c - implementation of the `Columns' GTK layout container.
 */

#include "gtkcols.h"
#include <gtk/gtk.h>

static void columns_init(Columns *cols);
static void columns_class_init(ColumnsClass *klass);
static void columns_map(GtkWidget *widget);
static void columns_unmap(GtkWidget *widget);
#if !GTK_CHECK_VERSION(2,0,0)
static void columns_draw(GtkWidget *widget, GdkRectangle *area);
static gint columns_expose(GtkWidget *widget, GdkEventExpose *event);
#endif
static void columns_base_add(GtkContainer *container, GtkWidget *widget);
static void columns_remove(GtkContainer *container, GtkWidget *widget);
static void columns_forall(GtkContainer *container, gboolean include_internals,
                           GtkCallback callback, gpointer callback_data);
#if !GTK_CHECK_VERSION(2,0,0)
static gint columns_focus(GtkContainer *container, GtkDirectionType dir);
#endif
static GtkType columns_child_type(GtkContainer *container);
static void columns_size_request(GtkWidget *widget, GtkRequisition *req);
static void columns_size_allocate(GtkWidget *widget, GtkAllocation *alloc);

static GtkContainerClass *parent_class = NULL;

#if !GTK_CHECK_VERSION(2,0,0)
GtkType columns_get_type(void)
{
    static GtkType columns_type = 0;

    if (!columns_type) {
        static const GtkTypeInfo columns_info = {
            "Columns",
            sizeof(Columns),
            sizeof(ColumnsClass),
            (GtkClassInitFunc) columns_class_init,
            (GtkObjectInitFunc) columns_init,
            /* reserved_1 */ NULL,
            /* reserved_2 */ NULL,
            (GtkClassInitFunc) NULL,
        };

        columns_type = gtk_type_unique(GTK_TYPE_CONTAINER, &columns_info);
    }

    return columns_type;
}
#else
GType columns_get_type(void)
{
    static GType columns_type = 0;

    if (!columns_type) {
        static const GTypeInfo columns_info = {
            sizeof(ColumnsClass),
	    NULL,
	    NULL,
            (GClassInitFunc) columns_class_init,
	    NULL,
	    NULL,
            sizeof(Columns),
	    0,
            (GInstanceInitFunc)columns_init,
        };

        columns_type = g_type_register_static(GTK_TYPE_CONTAINER, "Columns",
					      &columns_info, 0);
    }

    return columns_type;
}
#endif

#if !GTK_CHECK_VERSION(2,0,0)
static gint (*columns_inherited_focus)(GtkContainer *container,
				       GtkDirectionType direction);
#endif

static void columns_class_init(ColumnsClass *klass)
{
#if !GTK_CHECK_VERSION(2,0,0)
    /* GtkObjectClass *object_class = (GtkObjectClass *)klass; */
    GtkWidgetClass *widget_class = (GtkWidgetClass *)klass;
    GtkContainerClass *container_class = (GtkContainerClass *)klass;
#else
    /* GObjectClass *object_class = G_OBJECT_CLASS(klass); */
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    GtkContainerClass *container_class = GTK_CONTAINER_CLASS(klass);
#endif

#if !GTK_CHECK_VERSION(2,0,0)
    parent_class = gtk_type_class(GTK_TYPE_CONTAINER);
#else
    parent_class = g_type_class_peek_parent(klass);
#endif

    widget_class->map = columns_map;
    widget_class->unmap = columns_unmap;
#if !GTK_CHECK_VERSION(2,0,0)
    widget_class->draw = columns_draw;
    widget_class->expose_event = columns_expose;
#endif
    widget_class->size_request = columns_size_request;
    widget_class->size_allocate = columns_size_allocate;

    container_class->add = columns_base_add;
    container_class->remove = columns_remove;
    container_class->forall = columns_forall;
    container_class->child_type = columns_child_type;
#if !GTK_CHECK_VERSION(2,0,0)
    /* Save the previous value of this method. */
    if (!columns_inherited_focus)
	columns_inherited_focus = container_class->focus;
    container_class->focus = columns_focus;
#endif
}

static void columns_init(Columns *cols)
{
    GTK_WIDGET_SET_FLAGS(cols, GTK_NO_WINDOW);

    cols->children = NULL;
    cols->spacing = 0;
}

/*
 * These appear to be thoroughly tedious functions; the only reason
 * we have to reimplement them at all is because we defined our own
 * format for our GList of children...
 */
static void columns_map(GtkWidget *widget)
{
    Columns *cols;
    ColumnsChild *child;
    GList *children;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(IS_COLUMNS(widget));

    cols = COLUMNS(widget);
    GTK_WIDGET_SET_FLAGS(cols, GTK_MAPPED);

    for (children = cols->children;
         children && (child = children->data);
         children = children->next) {
        if (child->widget &&
	    GTK_WIDGET_VISIBLE(child->widget) &&
            !GTK_WIDGET_MAPPED(child->widget))
            gtk_widget_map(child->widget);
    }
}
static void columns_unmap(GtkWidget *widget)
{
    Columns *cols;
    ColumnsChild *child;
    GList *children;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(IS_COLUMNS(widget));

    cols = COLUMNS(widget);
    GTK_WIDGET_UNSET_FLAGS(cols, GTK_MAPPED);

    for (children = cols->children;
         children && (child = children->data);
         children = children->next) {
        if (child->widget &&
	    GTK_WIDGET_VISIBLE(child->widget) &&
            GTK_WIDGET_MAPPED(child->widget))
            gtk_widget_unmap(child->widget);
    }
}
#if !GTK_CHECK_VERSION(2,0,0)
static void columns_draw(GtkWidget *widget, GdkRectangle *area)
{
    Columns *cols;
    ColumnsChild *child;
    GList *children;
    GdkRectangle child_area;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(IS_COLUMNS(widget));

    if (GTK_WIDGET_DRAWABLE(widget)) {
        cols = COLUMNS(widget);

        for (children = cols->children;
             children && (child = children->data);
             children = children->next) {
            if (child->widget &&
		GTK_WIDGET_DRAWABLE(child->widget) &&
                gtk_widget_intersect(child->widget, area, &child_area))
                gtk_widget_draw(child->widget, &child_area);
        }
    }
}
static gint columns_expose(GtkWidget *widget, GdkEventExpose *event)
{
    Columns *cols;
    ColumnsChild *child;
    GList *children;
    GdkEventExpose child_event;

    g_return_val_if_fail(widget != NULL, FALSE);
    g_return_val_if_fail(IS_COLUMNS(widget), FALSE);
    g_return_val_if_fail(event != NULL, FALSE);

    if (GTK_WIDGET_DRAWABLE(widget)) {
        cols = COLUMNS(widget);
        child_event = *event;

        for (children = cols->children;
             children && (child = children->data);
             children = children->next) {
            if (child->widget &&
		GTK_WIDGET_DRAWABLE(child->widget) &&
                GTK_WIDGET_NO_WINDOW(child->widget) &&
                gtk_widget_intersect(child->widget, &event->area,
                                     &child_event.area))
                gtk_widget_event(child->widget, (GdkEvent *)&child_event);
        }
    }
    return FALSE;
}
#endif

static void columns_base_add(GtkContainer *container, GtkWidget *widget)
{
    Columns *cols;

    g_return_if_fail(container != NULL);
    g_return_if_fail(IS_COLUMNS(container));
    g_return_if_fail(widget != NULL);

    cols = COLUMNS(container);

    /*
     * Default is to add a new widget spanning all columns.
     */
    columns_add(cols, widget, 0, 0);   /* 0 means ncols */
}

static void columns_remove(GtkContainer *container, GtkWidget *widget)
{
    Columns *cols;
    ColumnsChild *child;
    GtkWidget *childw;
    GList *children;
    gboolean was_visible;

    g_return_if_fail(container != NULL);
    g_return_if_fail(IS_COLUMNS(container));
    g_return_if_fail(widget != NULL);

    cols = COLUMNS(container);

    for (children = cols->children;
         children && (child = children->data);
         children = children->next) {
        if (child->widget != widget)
            continue;

        was_visible = GTK_WIDGET_VISIBLE(widget);
        gtk_widget_unparent(widget);
        cols->children = g_list_remove_link(cols->children, children);
        g_list_free(children);
        g_free(child);
        if (was_visible)
            gtk_widget_queue_resize(GTK_WIDGET(container));
        break;
    }

    for (children = cols->taborder;
         children && (childw = children->data);
         children = children->next) {
        if (childw != widget)
            continue;

        cols->taborder = g_list_remove_link(cols->taborder, children);
        g_list_free(children);
#if GTK_CHECK_VERSION(2,0,0)
	gtk_container_set_focus_chain(container, cols->taborder);
#endif
        break;
    }
}

static void columns_forall(GtkContainer *container, gboolean include_internals,
                           GtkCallback callback, gpointer callback_data)
{
    Columns *cols;
    ColumnsChild *child;
    GList *children, *next;

    g_return_if_fail(container != NULL);
    g_return_if_fail(IS_COLUMNS(container));
    g_return_if_fail(callback != NULL);

    cols = COLUMNS(container);

    for (children = cols->children;
         children && (child = children->data);
         children = next) {
        /*
         * We can't wait until after the callback to assign
         * `children = children->next', because the callback might
         * be gtk_widget_destroy, which would remove the link
         * `children' from the list! So instead we must get our
         * hands on the value of the `next' pointer _before_ the
         * callback.
         */
        next = children->next;
	if (child->widget)
	    callback(child->widget, callback_data);
    }
}

static GtkType columns_child_type(GtkContainer *container)
{
    return GTK_TYPE_WIDGET;
}

GtkWidget *columns_new(gint spacing)
{
    Columns *cols;

#if !GTK_CHECK_VERSION(2,0,0)
    cols = gtk_type_new(columns_get_type());
#else
    cols = g_object_new(TYPE_COLUMNS, NULL);
#endif

    cols->spacing = spacing;

    return GTK_WIDGET(cols);
}

void columns_set_cols(Columns *cols, gint ncols, const gint *percentages)
{
    ColumnsChild *childdata;
    gint i;

    g_return_if_fail(cols != NULL);
    g_return_if_fail(IS_COLUMNS(cols));
    g_return_if_fail(ncols > 0);
    g_return_if_fail(percentages != NULL);

    childdata = g_new(ColumnsChild, 1);
    childdata->widget = NULL;
    childdata->ncols = ncols;
    childdata->percentages = g_new(gint, ncols);
    childdata->force_left = FALSE;
    for (i = 0; i < ncols; i++)
        childdata->percentages[i] = percentages[i];

    cols->children = g_list_append(cols->children, childdata);
}

void columns_add(Columns *cols, GtkWidget *child,
                 gint colstart, gint colspan)
{
    ColumnsChild *childdata;

    g_return_if_fail(cols != NULL);
    g_return_if_fail(IS_COLUMNS(cols));
    g_return_if_fail(child != NULL);
    g_return_if_fail(child->parent == NULL);

    childdata = g_new(ColumnsChild, 1);
    childdata->widget = child;
    childdata->colstart = colstart;
    childdata->colspan = colspan;
    childdata->force_left = FALSE;

    cols->children = g_list_append(cols->children, childdata);
    cols->taborder = g_list_append(cols->taborder, child);

    gtk_widget_set_parent(child, GTK_WIDGET(cols));

#if GTK_CHECK_VERSION(2,0,0)
    gtk_container_set_focus_chain(GTK_CONTAINER(cols), cols->taborder);
#endif

    if (GTK_WIDGET_REALIZED(cols))
        gtk_widget_realize(child);

    if (GTK_WIDGET_VISIBLE(cols) && GTK_WIDGET_VISIBLE(child)) {
        if (GTK_WIDGET_MAPPED(cols))
            gtk_widget_map(child);
        gtk_widget_queue_resize(child);
    }
}

void columns_force_left_align(Columns *cols, GtkWidget *widget)
{
    ColumnsChild *child;
    GList *children;

    g_return_if_fail(cols != NULL);
    g_return_if_fail(IS_COLUMNS(cols));
    g_return_if_fail(widget != NULL);

    for (children = cols->children;
         children && (child = children->data);
         children = children->next) {
        if (child->widget != widget)
            continue;

	child->force_left = TRUE;
        if (GTK_WIDGET_VISIBLE(widget))
            gtk_widget_queue_resize(GTK_WIDGET(cols));
        break;
    }
}

void columns_taborder_last(Columns *cols, GtkWidget *widget)
{
    GtkWidget *childw;
    GList *children;

    g_return_if_fail(cols != NULL);
    g_return_if_fail(IS_COLUMNS(cols));
    g_return_if_fail(widget != NULL);

    for (children = cols->taborder;
         children && (childw = children->data);
         children = children->next) {
        if (childw != widget)
            continue;

        cols->taborder = g_list_remove_link(cols->taborder, children);
        g_list_free(children);
	cols->taborder = g_list_append(cols->taborder, widget);
#if GTK_CHECK_VERSION(2,0,0)
	gtk_container_set_focus_chain(GTK_CONTAINER(cols), cols->taborder);
#endif
        break;
    }
}

#if !GTK_CHECK_VERSION(2,0,0)
/*
 * Override GtkContainer's focus movement so the user can
 * explicitly specify the tab order.
 */
static gint columns_focus(GtkContainer *container, GtkDirectionType dir)
{
    Columns *cols;
    GList *pos;
    GtkWidget *focuschild;

    g_return_val_if_fail(container != NULL, FALSE);
    g_return_val_if_fail(IS_COLUMNS(container), FALSE);

    cols = COLUMNS(container);

    if (!GTK_WIDGET_DRAWABLE(cols) ||
	!GTK_WIDGET_IS_SENSITIVE(cols))
	return FALSE;

    if (!GTK_WIDGET_CAN_FOCUS(container) &&
	(dir == GTK_DIR_TAB_FORWARD || dir == GTK_DIR_TAB_BACKWARD)) {

	focuschild = container->focus_child;
	gtk_container_set_focus_child(container, NULL);

	if (dir == GTK_DIR_TAB_FORWARD)
	    pos = cols->taborder;
	else
	    pos = g_list_last(cols->taborder);

	while (pos) {
	    GtkWidget *child = pos->data;

	    if (focuschild) {
		if (focuschild == child) {
		    focuschild = NULL; /* now we can start looking in here */
		    if (GTK_WIDGET_DRAWABLE(child) &&
			GTK_IS_CONTAINER(child) &&
			!GTK_WIDGET_HAS_FOCUS(child)) {
			if (gtk_container_focus(GTK_CONTAINER(child), dir))
			    return TRUE;
		    }
		}
	    } else if (GTK_WIDGET_DRAWABLE(child)) {
		if (GTK_IS_CONTAINER(child)) {
		    if (gtk_container_focus(GTK_CONTAINER(child), dir))
			return TRUE;
		} else if (GTK_WIDGET_CAN_FOCUS(child)) {
		    gtk_widget_grab_focus(child);
		    return TRUE;
		}
	    }

	    if (dir == GTK_DIR_TAB_FORWARD)
		pos = pos->next;
	    else
		pos = pos->prev;
	}

	return FALSE;
    } else
	return columns_inherited_focus(container, dir);
}
#endif

/*
 * Now here comes the interesting bit. The actual layout part is
 * done in the following two functions:
 * 
 * columns_size_request() examines the list of widgets held in the
 * Columns, and returns a requisition stating the absolute minimum
 * size it can bear to be.
 * 
 * columns_size_allocate() is given an allocation telling it what
 * size the whole container is going to be, and it calls
 * gtk_widget_size_allocate() on all of its (visible) children to
 * set their size and position relative to the top left of the
 * container.
 */

static void columns_size_request(GtkWidget *widget, GtkRequisition *req)
{
    Columns *cols;
    ColumnsChild *child;
    GList *children;
    gint i, ncols, colspan, *colypos;
    const gint *percentages;
    static const gint onecol[] = { 100 };

    g_return_if_fail(widget != NULL);
    g_return_if_fail(IS_COLUMNS(widget));
    g_return_if_fail(req != NULL);

    cols = COLUMNS(widget);

    req->width = 0;
    req->height = cols->spacing;

    ncols = 1;
    colypos = g_new(gint, 1);
    colypos[0] = 0;
    percentages = onecol;

    for (children = cols->children;
         children && (child = children->data);
         children = children->next) {
        GtkRequisition creq;

	if (!child->widget) {
	    /* Column reconfiguration. */
	    for (i = 1; i < ncols; i++) {
		if (colypos[0] < colypos[i])
		    colypos[0] = colypos[i];
	    }
	    ncols = child->ncols;
	    percentages = child->percentages;
	    colypos = g_renew(gint, colypos, ncols);
	    for (i = 1; i < ncols; i++)
		colypos[i] = colypos[0];
	    continue;
	}

        /* Only take visible widgets into account. */
        if (!GTK_WIDGET_VISIBLE(child->widget))
            continue;

        gtk_widget_size_request(child->widget, &creq);
	colspan = child->colspan ? child->colspan : ncols-child->colstart;

        /*
         * To compute width: we know that creq.width plus
         * cols->spacing needs to equal a certain percentage of the
         * full width of the container. So we work this value out,
         * figure out how wide the container will need to be to
         * make that percentage of it equal to that width, and
         * ensure our returned width is at least that much. Very
         * simple really.
         */
        {
            int percent, thiswid, fullwid;

            percent = 0;
            for (i = 0; i < colspan; i++)
                percent += percentages[child->colstart+i];

            thiswid = creq.width + cols->spacing;
            /*
             * Since creq is the _minimum_ size the child needs, we
             * must ensure that it gets _at least_ that size.
             * Hence, when scaling thiswid up to fullwid, we must
             * round up, which means adding percent-1 before
             * dividing by percent.
             */
            fullwid = (thiswid * 100 + percent - 1) / percent;

            /*
             * The above calculation assumes every widget gets
             * cols->spacing on the right. So we subtract
             * cols->spacing here to account for the extra load of
             * spacing on the right.
             */
            if (req->width < fullwid - cols->spacing)
                req->width = fullwid - cols->spacing;
        }

        /*
         * To compute height: the widget's top will be positioned
         * at the largest y value so far reached in any of the
         * columns it crosses. Then it will go down by creq.height
         * plus padding; and the point it reaches at the bottom is
         * the new y value in all those columns, and minus the
         * padding it is also a lower bound on our own size
         * request.
         */
        {
            int topy, boty;

            topy = 0;
            for (i = 0; i < colspan; i++) {
                if (topy < colypos[child->colstart+i])
                    topy = colypos[child->colstart+i];
            }
            boty = topy + creq.height + cols->spacing;
            for (i = 0; i < colspan; i++) {
                colypos[child->colstart+i] = boty;
            }

            if (req->height < boty - cols->spacing)
                req->height = boty - cols->spacing;
        }
    }

    req->width += 2*GTK_CONTAINER(cols)->border_width;
    req->height += 2*GTK_CONTAINER(cols)->border_width;

    g_free(colypos);
}

static void columns_size_allocate(GtkWidget *widget, GtkAllocation *alloc)
{
    Columns *cols;
    ColumnsChild *child;
    GList *children;
    gint i, ncols, colspan, border, *colxpos, *colypos;
    const gint *percentages;
    static const gint onecol[] = { 100 };

    g_return_if_fail(widget != NULL);
    g_return_if_fail(IS_COLUMNS(widget));
    g_return_if_fail(alloc != NULL);

    cols = COLUMNS(widget);
    widget->allocation = *alloc;
    border = GTK_CONTAINER(cols)->border_width;

    ncols = 1;
    percentages = onecol;
    /* colxpos gives the starting x position of each column.
     * We supply n+1 of them, so that we can find the RH edge easily.
     * All ending x positions are expected to be adjusted afterwards by
     * subtracting the spacing. */
    colxpos = g_new(gint, 2);
    colxpos[0] = 0;
    colxpos[1] = alloc->width - 2*border + cols->spacing;
    /* As in size_request, colypos is the lowest y reached in each column. */
    colypos = g_new(gint, 1);
    colypos[0] = 0;

    for (children = cols->children;
         children && (child = children->data);
         children = children->next) {
        GtkRequisition creq;
        GtkAllocation call;

	if (!child->widget) {
	    gint percent;

	    /* Column reconfiguration. */
	    for (i = 1; i < ncols; i++) {
		if (colypos[0] < colypos[i])
		    colypos[0] = colypos[i];
	    }
	    ncols = child->ncols;
	    percentages = child->percentages;
	    colypos = g_renew(gint, colypos, ncols);
	    for (i = 1; i < ncols; i++)
		colypos[i] = colypos[0];
	    colxpos = g_renew(gint, colxpos, ncols + 1);
	    colxpos[0] = 0;
	    percent = 0;
	    for (i = 0; i < ncols; i++) {
		percent += percentages[i];
		colxpos[i+1] = (((alloc->width - 2*border) + cols->spacing)
				* percent / 100);
	    }
	    continue;
	}

        /* Only take visible widgets into account. */
        if (!GTK_WIDGET_VISIBLE(child->widget))
            continue;

        gtk_widget_get_child_requisition(child->widget, &creq);
	colspan = child->colspan ? child->colspan : ncols-child->colstart;

        /*
         * Starting x position is cols[colstart].
         * Ending x position is cols[colstart+colspan] - spacing.
	 * 
	 * Unless we're forcing left, in which case the width is
	 * exactly the requisition width.
         */
        call.x = alloc->x + border + colxpos[child->colstart];
	if (child->force_left)
	    call.width = creq.width;
	else
	    call.width = (colxpos[child->colstart+colspan] -
			  colxpos[child->colstart] - cols->spacing);

        /*
         * To compute height: the widget's top will be positioned
         * at the largest y value so far reached in any of the
         * columns it crosses. Then it will go down by creq.height
         * plus padding; and the point it reaches at the bottom is
         * the new y value in all those columns.
         */
        {
            int topy, boty;

            topy = 0;
            for (i = 0; i < colspan; i++) {
                if (topy < colypos[child->colstart+i])
                    topy = colypos[child->colstart+i];
            }
            call.y = alloc->y + border + topy;
            call.height = creq.height;
            boty = topy + creq.height + cols->spacing;
            for (i = 0; i < colspan; i++) {
                colypos[child->colstart+i] = boty;
            }
        }

        gtk_widget_size_allocate(child->widget, &call);
    }

    g_free(colxpos);
    g_free(colypos);    
}
