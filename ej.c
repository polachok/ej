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
	GtkWidget *docarea;
	GtkWidget *scrwin;
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
djvu_page_render (int num, int rot) {
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
	prect.w = page_width;
	prect.h = page_height;
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
	Document.curpage = 0;
	ddjvu_format_set_row_order(Document.fmt, 1);
}

void page(int n) {
	Page *page;
	if(n < 0) 
		n = 0;
	if(n >= Document.npages)
		n = Document.npages - 1 ;
	page = djvu_page_render(n, 0);
	gtk_image_set_from_pixbuf(GTK_IMAGE(UI.docarea), page->pixbuf); 
	if (Document.pages[PageCur]) {
		g_object_unref(Document.pages[PageCur]->pixbuf);
		free(Document.pages[PageCur]->pixels);
		free(Document.pages[PageCur]);
	}
	Document.pages[PageCur] = page;
	Document.curpage = n;
}

gboolean
keypress(GtkWidget *widget, GdkEventKey *event, gpointer data) {
	if(event->keyval == GDK_j || event->keyval == GDK_Page_Down)
		page(Document.curpage+1);
	if(event->keyval == GDK_k || event->keyval == GDK_Page_Up)
		page(Document.curpage-1);
	if(event->keyval == GDK_q)
		gtk_main_quit ();
	return TRUE;
}

int main(int argc, char *argv[]) {
	gtk_init(&argc, &argv);
	if (argc == 2)
		opendjvu(argv[1]);
	else 
		eprint("usage: ej [file]\n");

	UI.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	UI.scrwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (UI.scrwin),
			                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	UI.docarea = gtk_image_new();
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(UI.scrwin), UI.docarea);
	gtk_container_add(GTK_CONTAINER(UI.window), UI.scrwin);
	g_signal_connect (G_OBJECT(UI.window), "destroy",
			G_CALLBACK (gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(UI.window), "key-press-event", G_CALLBACK(keypress), NULL);
	gtk_widget_show_all(UI.window);
	page(0);
	gtk_main();
	return 0;
}
