#ifndef SIDE_BAR_H_
#define SIDE_BAR_H_
#include <gtk/gtk.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  GtkWidget *m_SideBar;
  GtkWidget *list_box;
  GtkWidget *recent_list_box;
  GtkWidget *main_box;
  GHashTable *recent_paths; // Track frequency of visited paths
  gchar *config_file_path;  // Path to persistent storage file
} SideBarWidget;

/**
 * Create a SideBar widget
 * @return New SideBarWidget instance
 */
extern SideBarWidget *SideBar_new(void);

/**
 * Destroy SideBar widget and free resources
 * @param side_bar SideBarWidget to destroy
 */
extern void SideBar_destroy(SideBarWidget *side_bar);

/**
 * Connect a callback to the sidebar navigation signal
 * @param side_bar SideBarWidget instance
 * @param callback Callback function to invoke on navigation
 * @param user_data User data to pass to callback
 */
extern void SideBar_connect_navigation(SideBarWidget *side_bar,
                                       GCallback callback, gpointer user_data);

/**
 * Add a directory to recent/frequently used list
 * @param side_bar SideBarWidget instance
 * @param path Directory path to track
 */
extern void SideBar_add_recent_directory(SideBarWidget *side_bar,
                                         const char *path);

/**
 * Save recent directories to disk (call before closing program)
 * @param side_bar SideBarWidget instance
 * @return TRUE on success, FALSE on failure
 */
extern gboolean SideBar_save_recent_directories(SideBarWidget *side_bar);

/**
 * Load recent directories from disk (called automatically in SideBar_new)
 * @param side_bar SideBarWidget instance
 * @return TRUE on success, FALSE on failure
 */
extern gboolean SideBar_load_recent_directories(SideBarWidget *side_bar);

#ifdef __cplusplus
}
#endif
#endif // SIDE_BAR_H_
