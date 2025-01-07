#include "Pages/Topbar.h"

typedef struct {
  GtkWidget *address_entry;
  GtkWidget *search_entry;
} TopBarPrivate;

// Callback when the address entry is activated
static void on_address_entry_activate(GtkEntry *entry, gpointer user_data) {
  GtkWidget *top_bar = GTK_WIDGET(user_data);
  const char *new_address = gtk_entry_get_text(entry);

  // Emit the custom signal
  g_signal_emit_by_name(top_bar, "address-changed", new_address);
}

// Callback when the search entry is activated
static void on_search_entry_activate(GtkEntry *entry, gpointer user_data) {
  GtkWidget *top_bar = GTK_WIDGET(user_data);
  const char *search_text = gtk_entry_get_text(entry);

  // Emit a custom "search" signal with the entered search text
  g_signal_emit_by_name(top_bar, "search-triggered", search_text);
}

GtkWidget *top_bar_new(const char *initial_address) {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

  // Address entry
  GtkWidget *address_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(address_entry),
                     initial_address ? initial_address : "Address");
  gtk_box_pack_start(GTK_BOX(box), address_entry, TRUE, TRUE, 5);

  // Search box
  GtkWidget *search_entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Search...");
  gtk_box_pack_end(GTK_BOX(box), search_entry, FALSE, FALSE, 5);

  // Store references
  TopBarPrivate *private = g_new(TopBarPrivate, 1);
  private->address_entry = address_entry;
  private->search_entry = search_entry;
  g_object_set_data_full(G_OBJECT(box), "top-bar-private", private, g_free);

  // Connect signals
  g_signal_connect(address_entry, "activate",
                   G_CALLBACK(on_address_entry_activate), box);
  g_signal_connect(search_entry, "activate",
                   G_CALLBACK(on_search_entry_activate), box);

  // Register custom signals
  g_signal_new("address-changed", G_TYPE_FROM_INSTANCE(box), G_SIGNAL_RUN_LAST,
               0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING);
  g_signal_new("search-triggered", G_TYPE_FROM_INSTANCE(box), G_SIGNAL_RUN_LAST,
               0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING);

  return box;
}

void top_bar_set_address(GtkWidget *top_bar, const char *address) {
  TopBarPrivate *private =
      g_object_get_data(G_OBJECT(top_bar), "top-bar-private");
  if (private && private->address_entry) {
    gtk_entry_set_text(GTK_ENTRY(private->address_entry), address);
  }
}

const char *top_bar_get_address(GtkWidget *top_bar) {
  TopBarPrivate *private =
      g_object_get_data(G_OBJECT(top_bar), "top-bar-private");
  return private ? gtk_entry_get_text(GTK_ENTRY(private->address_entry)) : NULL;
}

GtkWidget *top_bar_get_search_entry(GtkWidget *top_bar) {
  TopBarPrivate *private =
      g_object_get_data(G_OBJECT(top_bar), "top-bar-private");
  return private ? private->search_entry : NULL;
}

void top_bar_connect_address_change(GtkWidget *top_bar, GCallback callback,
                                    gpointer user_data) {
  g_signal_connect(top_bar, "address-changed", callback, user_data);
}

void top_bar_connect_search(GtkWidget *top_bar, GCallback callback,
                            gpointer user_data) {
  g_signal_connect(top_bar, "search-triggered", callback, user_data);
}
