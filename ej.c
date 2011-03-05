/* Simple dejavu viewer */

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

enum { PagePrev, PageCur, PageNext };

typedef struct {
	GdkPixbuf *pixbuf;
	gchar *pixels;
} Page;

struct UI {
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *docarea;
	GtkWidget *scrwin;
	GtkWidget *label;
	GtkWidget *status;
	GtkWidget *pgentry;
	float scale;
} UI;

struct Document {
	ddjvu_context_t *ctx;
	ddjvu_document_t *doc;
	ddjvu_format_t *fmt;
	char *filename;
	int curpage;
	int npages;
	Page *pages[3];
} Document;

void
eprint(const char *errstr, ...) {
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

void djvu_msg_handle(gboolean wait) {
	const ddjvu_message_t *msg;

	if (wait)
		ddjvu_message_wait(Document.ctx);
	while ((msg = ddjvu_message_peek(Document.ctx))) {
		switch(msg->m_any.tag) { 
			case DDJVU_ERROR:
				fprintf(stderr,"Error while decoding document\n");
				break;
#if 0
			case DDJVU_INFO:
			 	break;
			case DDJVU_NEWSTREAM:  .... ; break;
					       ....
#endif
			default: break;
		}
		ddjvu_message_pop(Document.ctx);
	}
}

Page *
djvu_page_render (int num, int rot, float scale) {
	Page *page;
    	ddjvu_rect_t rrect;
	ddjvu_rect_t prect;
	ddjvu_page_t *d_page;
	ddjvu_page_rotation_t rotation;
	int rowsize;
	double page_width, page_height, tmp;

	d_page = ddjvu_page_create_by_pageno (Document.doc, num);
	
	while (!ddjvu_page_decoding_done (d_page))
		djvu_msg_handle(TRUE);

	page = malloc(sizeof(Page));

	page_width = ddjvu_page_get_width (d_page);
	page_height = ddjvu_page_get_height (d_page);
	
	switch (rot) {
	        case 90:
			rotation = DDJVU_ROTATE_90;
			tmp = page_height;
			page_height = page_width;
			page_width = tmp;
			
			break;
	        case 180:
			rotation = DDJVU_ROTATE_180;
			
			break;
	        case 270:
			rotation = DDJVU_ROTATE_270;
			tmp = page_height;
			page_height = page_width;
			page_width = tmp;
			
			break;
	        default:
			rotation = DDJVU_ROTATE_0;
	}

	prect.x = 0;
	prect.y = 0;
	prect.w = page_width * scale;
	prect.h = page_height * scale;
	rrect = prect;
	rowsize = rrect.w*3; /* 24bpp */

	if(!(page->pixels = malloc(rowsize * rrect.h)))
		eprint("ENOMEM");

	ddjvu_page_set_rotation (d_page, rotation);
	
	ddjvu_page_render (d_page, DDJVU_RENDER_COLOR,
			   &prect,
			   &rrect,
			   Document.fmt,
			   rowsize,
			   page->pixels);

	page->pixbuf = gdk_pixbuf_new_from_data(page->pixels, GDK_COLORSPACE_RGB,
		       	FALSE, 8, rrect.w, rrect.h, rrect.w*3, NULL, NULL);
	return page;
}

void opendjvu(char *filename) {
	Document.filename = filename;

	if(!(Document.ctx = ddjvu_context_create("ej")))
		eprint("cannot create djvu context.\n");
	if(!(Document.doc = ddjvu_document_create_by_filename(Document.ctx, Document.filename, TRUE)))
		eprint("cannot open djvu document\n", Document.filename);
	if(!(Document.fmt = ddjvu_format_create(DDJVU_FORMAT_RGB24, 0, 0)))
		eprint("cannot set format\n");
	while (!ddjvu_document_decoding_done(Document.doc))
		djvu_msg_handle(TRUE);
	if(!(Document.npages = ddjvu_document_get_pagenum(Document.doc)))
		eprint("cannot get number of pages\n");
	ddjvu_format_set_row_order(Document.fmt, 1);
	Document.curpage = 0;
}

void page(int n, float scale) {
	Page *page;
	char status[20];

	if(n < 0) 
		n = 0;
	if(n >= Document.npages)
		n = Document.npages - 1;
	page = djvu_page_render(n, 0, scale);
	gtk_image_set_from_pixbuf(GTK_IMAGE(UI.docarea), page->pixbuf); 
	if (Document.pages[PageCur]) {
		g_object_unref(Document.pages[PageCur]->pixbuf);
		free(Document.pages[PageCur]->pixels);
		free(Document.pages[PageCur]);
	}
	Document.pages[PageCur] = page;
	Document.curpage = n;
	UI.scale = scale;
	snprintf(status, sizeof(status), "%d/%d %3.0f%%", Document.curpage+1,
		       	Document.npages, 100*(Document.curpage+1)/(float)Document.npages);
	gtk_label_set_text(GTK_LABEL(UI.status), status);
	snprintf(status, sizeof(status), "%d", Document.curpage+1);
	gtk_entry_set_text(GTK_ENTRY(UI.pgentry), status);
}

gboolean
keypress(GtkWidget *widget, GdkEventKey *event, gpointer data) {
	int n;
	gboolean dummy;
	
	if(widget == UI.pgentry) {
		if(event->keyval == GDK_Return) {
			n = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
			page(n - 1, UI.scale);
			gtk_widget_grab_focus(UI.scrwin);
			gtk_widget_hide(UI.hbox);
			return TRUE;
		}
		if(event->keyval == GDK_Escape) {
			gtk_widget_grab_focus(UI.scrwin);
			gtk_widget_hide(UI.hbox);
			return TRUE;
		}
		return FALSE;
	}
	if(widget == UI.scrwin) {
		if(event->keyval == GDK_Up)
			g_signal_emit_by_name(UI.scrwin, "scroll-child",
				      	GTK_SCROLL_STEP_BACKWARD, FALSE, &dummy);
		else if(event->keyval == GDK_Down)
			g_signal_emit_by_name(UI.scrwin, "scroll-child",
				      	GTK_SCROLL_STEP_FORWARD, FALSE, &dummy);
		else if(event->keyval == GDK_j || event->keyval == GDK_Page_Down)
			page(Document.curpage+1, UI.scale);
		else if(event->keyval == GDK_k || event->keyval == GDK_Page_Up)
			page(Document.curpage-1, UI.scale);
		else if(event->keyval == GDK_minus)
			page(Document.curpage, UI.scale-0.1);
		else if(event->keyval == GDK_equal || event->keyval == GDK_plus)
			page(Document.curpage, UI.scale+0.1);
		else if(event->keyval == GDK_q)
			gtk_main_quit ();
		else if(event->keyval == GDK_g) {
			if(gtk_widget_get_visible(UI.hbox)) {
				gtk_widget_grab_focus(UI.scrwin);
				gtk_widget_hide(UI.hbox);
			} else {
				gtk_widget_show(UI.hbox);
				gtk_widget_grab_focus(UI.pgentry);
			}
		} else 
			return FALSE;
		return TRUE;
	}
	return FALSE;
}

int main(int argc, char *argv[]) {
	gtk_init(&argc, &argv);
	if (argc == 2)
		if(!strcmp(argv[1], "-v"))
			eprint("ej-" VERSION "\n");
		else
			opendjvu(argv[1]);
	else 
		eprint("usage: ej [file]\n");

	UI.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	UI.vbox = gtk_vbox_new(FALSE, 0);
	UI.hbox = gtk_hbox_new(FALSE, 0);
	UI.label = gtk_label_new(" Page: ");
	UI.status = gtk_label_new("0/0 ");
	UI.scrwin = gtk_scrolled_window_new(NULL, NULL);
	UI.pgentry = gtk_entry_new();
	gtk_misc_set_alignment(GTK_MISC(UI.status), 1, 0.5);
	gtk_entry_set_text(GTK_ENTRY(UI.pgentry), "Goto page");
	gtk_entry_set_editable(GTK_ENTRY(UI.pgentry), TRUE);
	gtk_container_add(GTK_CONTAINER(UI.window), UI.vbox);
	gtk_box_pack_start (GTK_BOX (UI.vbox), UI.scrwin, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX(UI.vbox), UI.hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(UI.hbox), UI.label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(UI.hbox), UI.pgentry, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(UI.hbox), UI.status, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (UI.scrwin),
			                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	UI.docarea = gtk_image_new();
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(UI.scrwin), UI.docarea);
	g_signal_connect (G_OBJECT(UI.window), "destroy",
			G_CALLBACK (gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(UI.scrwin), "key-press-event", G_CALLBACK(keypress), NULL);
	g_signal_connect(G_OBJECT(UI.pgentry), "key-press-event", G_CALLBACK(keypress), NULL);
	gtk_widget_show_all(UI.window);
	gtk_widget_hide(UI.hbox);
	page(0, 1);
	gtk_main();
	return 0;
}
