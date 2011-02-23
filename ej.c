/* Simple dejavu viewer */

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

struct UI {
	GtkWidget *window;
	GtkWidget *docarea;
	GtkWidget *scrwin;
} UI;

struct Document {
	char *filename;
	ddjvu_context_t *ctx;
	ddjvu_document_t *doc;
	ddjvu_format_t *fmt;
} Document;

void
eprint(const char *errstr, ...)
{
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

void djvu_msg_handle(gboolean wait)
{
	const ddjvu_message_t *msg;

	if (wait)
		ddjvu_message_wait(Document.ctx);
	while ((msg = ddjvu_message_peek(Document.ctx)))
	{
		switch(msg->m_any.tag)
		{ 
			case DDJVU_ERROR:
				fprintf(stderr,"Error while decoding document\n");
				break;
//			case DDJVU_INFO:       .... ; break;
//			case DDJVU_NEWSTREAM:  .... ; break;
//					       ....
			default: break;
		}
		ddjvu_message_pop(Document.ctx);
	}
}

GdkPixbuf *
djvu_page_render (int page, int rot) {
	GdkPixbuf *pixbuf;
	gchar *pixels;
    	ddjvu_rect_t rrect;
	ddjvu_rect_t prect;
	ddjvu_page_t *d_page;
	ddjvu_page_rotation_t rotation;
	int rowsize;
	double page_width, page_height, tmp;

	d_page = ddjvu_page_create_by_pageno (Document.doc, page);
	
	while (!ddjvu_page_decoding_done (d_page))
		djvu_msg_handle(TRUE);

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
	rowsize = rrect.w*3;

	if(!(pixels = malloc(rowsize * rrect.h)))
		eprint("ENOMEM");

	ddjvu_page_set_rotation (d_page, rotation);
	
	ddjvu_page_render (d_page, DDJVU_RENDER_COLOR,
			   &prect,
			   &rrect,
			   Document.fmt,
			   rowsize,
			   pixels);

	pixbuf = gdk_pixbuf_new_from_data(pixels, GDK_COLORSPACE_RGB, FALSE, 8, rrect.w, rrect.h, rrect.w*3, NULL, NULL);
	return pixbuf;
}

void opendjvu(char *filename) {
	Document.filename = filename;

	if(!(Document.ctx = ddjvu_context_create("ej")))
		eprint("cannot create djvu context.\n");
	if(!(Document.doc = ddjvu_document_create_by_filename(Document.ctx, Document.filename, TRUE)))
		eprint("cannot open djvu document\n", Document.filename);
	if(!(Document.fmt = ddjvu_format_create(DDJVU_FORMAT_RGB24, 0, 0)))
		eprint("cannot set format\n");
	ddjvu_format_set_row_order(Document.fmt, 1);
}

void show() {
	GdkPixbuf *pixbuf;

	pixbuf = djvu_page_render(0, 0);
	gtk_image_set_from_pixbuf(GTK_IMAGE(UI.docarea), pixbuf); 
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
	g_signal_connect (G_OBJECT (UI.window), "destroy",
			G_CALLBACK (gtk_main_quit), NULL);
	gtk_widget_show_all(UI.window);
	show();
	gtk_main();
	return 0;
}
