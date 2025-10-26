#ifndef TOP_BAR_H_
#define TOP_BAR_H_
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  GtkWidget *m_TopBar;
  GtkWidget *address_entry;
  GtkWidget *search_entry;
  gchar *current_address;
} TopBarWidget;

/**
 * Create a TopBar widget
 * @param initial_address Starting address text, or NULL for default
 * @return New TopBarWidget instance
 */
extern TopBarWidget *TopBar_new(const char *initial_address);

/**
 * Destroy TopBar widget and free resources
 * @param top_bar TopBarWidget to destroy
 */
extern void TopBar_destroy(TopBarWidget *top_bar);

/**
 * Update the address display (emits signal if changed)
 * @param top_bar TopBarWidget instance
 * @param address New address text
 */
extern void TopBar_set_address(TopBarWidget *top_bar, const char *address);

/**
 * Get the current address from the address entry
 * @param top_bar TopBarWidget instance
 * @return Current address string, or NULL
 */
extern const char *TopBar_get_address(const TopBarWidget *top_bar);

/**
 * Connect to address-changed signal
 * @param top_bar TopBarWidget instance
 * @param callback Function to call when address changes
 * @param user_data Data to pass to callback
 */
extern void TopBar_connect_address_changed(TopBarWidget *top_bar,
                                           GCallback callback,
                                           gpointer user_data);

/**
 * Connect to search-triggered signal
 * @param top_bar TopBarWidget instance
 * @param callback Function to call when search is triggered
 * @param user_data Data to pass to callback
 */
extern void TopBar_connect_search(TopBarWidget *top_bar, GCallback callback,
                                  gpointer user_data);

#ifdef __cplusplus
}
#endif
#endif // TOP_BAR_H_
