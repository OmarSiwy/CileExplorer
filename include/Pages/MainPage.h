#ifndef MAIN_PAGE_H_
#define MAIN_PAGE_H_

#include <gtk/gtk.h>

// Create a MainPage widget
GtkWidget *main_page_new(void);

// Populate files in the MainPage
void main_page_set_files(GtkWidget *main_page, const char *directory);

// Connect a callback for row activation (double-click or Enter key)
void main_page_connect_row_activated(GtkWidget *main_page, GCallback callback,
                                     gpointer user_data);

#endif // MAIN_PAGE_H_
