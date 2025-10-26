#ifndef MAIN_PAGE_H_
#define MAIN_PAGE_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "Pages/Sidebar.h"
#include "Pages/Topbar.h"
#include <gtk/gtk.h>

typedef struct {
  GtkWidget *m_MainPage;
  GtkWidget *list_box;
  TopBarWidget *top_bar;
  SideBarWidget *side_bar;
  gchar *current_search_pattern; // Store current search pattern
} MainPageWidget;

/**
 * Widget Constructor
 * @param top_bar TopBarWidget instance to connect signals
 * @param side_bar SideBarWidget instance
 * @return New MainPageWidget instance
 */
extern MainPageWidget *MainPage_new(TopBarWidget *top_bar,
                                    SideBarWidget *side_bar);

/**
 * Destroy MainPage widget and free resources
 * @param mp MainPageWidget to destroy
 */
extern void MainPage_destroy(MainPageWidget *mp);

/**
 * Update the file listing for a directory
 * @param mp MainPageWidget instance
 * @param directory Directory path to display
 */
extern void MainPage_update_state(MainPageWidget *mp, const char *directory);

#ifdef __cplusplus
}
#endif
#endif // MAIN_PAGE_H_
