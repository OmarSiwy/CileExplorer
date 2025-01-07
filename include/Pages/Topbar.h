#ifndef TOP_BAR_H_
#define TOP_BAR_H_

#include <gtk/gtk.h>

// Create a TopBar widget
GtkWidget *top_bar_new(const char *initial_address);

// Update the address display
void top_bar_set_address(GtkWidget *top_bar, const char *address);

// Get the current address from the address entry
const char *top_bar_get_address(GtkWidget *top_bar);

// Get the search box entry for connecting signals
GtkWidget *top_bar_get_search_entry(GtkWidget *top_bar);

// Connect a callback for when the address changes
void top_bar_connect_address_change(GtkWidget *top_bar, GCallback callback,
                                    gpointer user_data);

#endif // TOP_BAR_H_
