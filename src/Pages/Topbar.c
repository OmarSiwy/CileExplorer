#include "Pages/Topbar.h"
#include <string.h>

// Signal indices
enum { SIGNAL_ADDRESS_CHANGED, SIGNAL_SEARCH_TRIGGERED, LAST_SIGNAL };

static guint top_bar_signals[LAST_SIGNAL] = {0};

static gboolean on_topbar_motion(GtkWidget *widget, GdkEventMotion *event,
                                 gpointer user_data);

// Initialize signals once
static void ensure_signals_registered(GtkWidget *widget) {
  if (top_bar_signals[SIGNAL_ADDRESS_CHANGED] == 0) {
    top_bar_signals[SIGNAL_ADDRESS_CHANGED] =
        g_signal_new("address-changed",            // signal_name
                     G_TYPE_FROM_INSTANCE(widget), // itype
                     G_SIGNAL_RUN_LAST,            // signal_flags
                     0,                            // class_offset
                     NULL,                         // accumulator
                     NULL,                         // accu_data
                     NULL,                         // c_marshaller
                     G_TYPE_NONE,                  // return_type
                     1,                            // n_params
                     G_TYPE_STRING                 // param_types
        );

    top_bar_signals[SIGNAL_SEARCH_TRIGGERED] =
        g_signal_new("search-triggered",           // signal_name
                     G_TYPE_FROM_INSTANCE(widget), // itype
                     G_SIGNAL_RUN_LAST,            // signal_flags
                     0,                            // class_offset
                     NULL,                         // accumulator
                     NULL,                         // accu_data
                     NULL,                         // c_marshaller
                     G_TYPE_NONE,                  // return_type
                     1,                            // n_params
                     G_TYPE_STRING                 // param_types
        );
  }
}

// Callback when the address entry is activated
static void on_address_entry_activate(GtkEntry *entry, gpointer user_data) {
  TopBarWidget *top_bar = (TopBarWidget *)user_data;
  const char *new_address = gtk_entry_get_text(entry);

  // Only emit if address actually changed
  if (!top_bar->current_address ||
      strcmp(top_bar->current_address, new_address) != 0) {
    // Update cached address
    g_free(top_bar->current_address);
    top_bar->current_address = g_strdup(new_address);

    // Emit the address-changed signal
    g_signal_emit(top_bar->m_TopBar,                       // instance
                  top_bar_signals[SIGNAL_ADDRESS_CHANGED], // signal_id
                  0,                                       // detail
                  new_address);                            // ...
  }
}

// Callback when the search entry is activated
static void on_search_entry_activate(GtkEntry *entry, gpointer user_data) {
  TopBarWidget *top_bar = (TopBarWidget *)user_data;
  const char *search_text = gtk_entry_get_text(entry);

  // Emit the search-triggered signal
  g_signal_emit(top_bar->m_TopBar,                        // instance
                top_bar_signals[SIGNAL_SEARCH_TRIGGERED], // signal_id
                0,                                        // detail
                search_text);                             // ...
}

TopBarWidget *TopBar_new(const char *initial_address) {
  // Allocate the struct
  TopBarWidget *top_bar = g_new0(TopBarWidget, 1);

  // Create the main container
  top_bar->m_TopBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

  // Address entry
  top_bar->address_entry = gtk_entry_new();
  const char *addr = initial_address ? initial_address : "Address";
  gtk_entry_set_text(GTK_ENTRY(top_bar->address_entry), addr);
  top_bar->current_address = g_strdup(addr);
  gtk_box_pack_start(GTK_BOX(top_bar->m_TopBar), // box
                     top_bar->address_entry,     // child
                     TRUE,                       // expand
                     TRUE,                       // fill
                     5);                         // padding

  // Search box
  top_bar->search_entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(top_bar->search_entry), "Search...");

  // Fix hover issue on search
  gtk_widget_add_events(top_bar->search_entry,
                        GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK);
  g_signal_connect(top_bar->search_entry, "motion-notify-event",
                   G_CALLBACK(on_topbar_motion), top_bar);
  gtk_box_pack_end(GTK_BOX(top_bar->m_TopBar), // box
                   top_bar->search_entry,      // child
                   FALSE,                      // expand
                   FALSE,                      // fill
                   5);                         // padding

  // Register signals once
  ensure_signals_registered(top_bar->m_TopBar);

  // Connect internal callbacks
  g_signal_connect(top_bar->address_entry,                // instance
                   "activate",                            // detailed_signal
                   G_CALLBACK(on_address_entry_activate), // c_handler
                   top_bar);                              // data
  g_signal_connect(top_bar->search_entry,                 // instance
                   "activate",                            // detailed_signal
                   G_CALLBACK(on_search_entry_activate),  // c_handler
                   top_bar);                              // data

  // Store TopBarWidget pointer in the GtkWidget for cleanup
  g_object_set_data_full(G_OBJECT(top_bar->m_TopBar), // object
                         "top-bar-widget",            // key
                         top_bar,                     // data
                         NULL);                       // destroy

  return top_bar;
}

void TopBar_destroy(TopBarWidget *top_bar) {
  if (top_bar) {
    g_free(top_bar->current_address);
    // Widget will be destroyed by GTK when parent is destroyed
    g_free(top_bar);
  }
}

void TopBar_set_address(TopBarWidget *top_bar, const char *address) {
  if (!top_bar || !top_bar->address_entry)
    return;

  const char *addr = address ? address : "";

  // Only update if changed
  if (!top_bar->current_address ||
      strcmp(top_bar->current_address, addr) != 0) {
    gtk_entry_set_text(GTK_ENTRY(top_bar->address_entry), addr);

    g_free(top_bar->current_address);
    top_bar->current_address = g_strdup(addr);

    // Emit signal
    g_signal_emit(top_bar->m_TopBar,                       // instance
                  top_bar_signals[SIGNAL_ADDRESS_CHANGED], // signal_id
                  0,                                       // detail
                  addr);                                   // ...
  }
}

const char *TopBar_get_address(const TopBarWidget *top_bar) {
  return (top_bar && top_bar->current_address) ? top_bar->current_address
                                               : NULL;
}

void TopBar_connect_address_changed(TopBarWidget *top_bar, GCallback callback,
                                    gpointer user_data) {
  if (top_bar && top_bar->m_TopBar) {
    g_signal_connect(top_bar->m_TopBar, // instance
                     "address-changed", // detailed_signal
                     callback,          // c_handler
                     user_data);        // data
  }
}

void TopBar_connect_search(TopBarWidget *top_bar, GCallback callback,
                           gpointer user_data) {
  if (top_bar && top_bar->m_TopBar) {
    g_signal_connect(top_bar->m_TopBar,  // instance
                     "search-triggered", // detailed_signal
                     callback,           // c_handler
                     user_data);         // data
  }
}

static gboolean on_topbar_motion(GtkWidget *widget, GdkEventMotion *event,
                                 gpointer user_data) {
  gtk_widget_queue_draw(widget);
  return FALSE;
}
