/* Simple dejavu viewer */

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

struct Window {
	GtkWidget *window;
} Window;

struct Document {
	char *filename;
	ddjvu_context_t *ctx;
	ddjvu_document_t *doc;
} Document;

int main(int argc, char *argv[])
{
	return 0;
}
