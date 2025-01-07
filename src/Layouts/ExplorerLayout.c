#include "Layouts/ExplorerLayout.h"
#include "Pages/MainPage.h"
#include "Pages/Sidebar.h"
#include "Pages/Topbar.h"

GtkWidget *explorer_layout_new(void) {
  GtkWidget *grid = gtk_grid_new();
  gtk_grid_set_column_homogeneous(GTK_GRID(grid), FALSE);
  gtk_grid_set_row_homogeneous(GTK_GRID(grid), FALSE);

  // Top bar
  GtkWidget *top_bar = top_bar_new("/home/username");

  // Side bar
  GtkWidget *side_bar = side_bar_new();

  // Main page
  GtkWidget *main_page = main_page_new();

  // Attach widgets
  gtk_grid_attach(GTK_GRID(grid), top_bar, 0, 0, 2,
                  1); // Top bar spans 2 columns
  gtk_grid_attach(GTK_GRID(grid), side_bar, 0, 1, 1, 1); // Side bar on the left
  gtk_grid_attach(GTK_GRID(grid), main_page, 1, 1, 1,
                  1); // Main page on the right

  return grid;
}
