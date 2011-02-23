/* Simple dejavu viewer */

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

struct UI {
	GtkWidget *window;
	GtkWidget *docarea;
} UI;

struct Document {
	char *filename;
	ddjvu_context_t *ctx;
	ddjvu_document_t *doc;
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

int main(int argc, char *argv[]) {
	gtk_init(&argc, &argv);
	if (argc == 2)
		Document.filename = argv[1];
	else 
		eprint("usage: ej [file]\n");

	UI.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	UI.docarea = gtk_image_new();
	g_signal_connect (G_OBJECT (UI.window), "destroy",
			G_CALLBACK (gtk_main_quit), NULL);
	gtk_widget_show_all(UI.window);
	gtk_main();
	return 0;
}
