#include "Pages/Sidebar.h"

GtkWidget *side_bar_new(void) {
  GtkWidget *list_box = gtk_list_box_new();

  // Add main destinations
  gtk_list_box_insert(GTK_LIST_BOX(list_box), gtk_label_new("Home"), -1);
  gtk_list_box_insert(GTK_LIST_BOX(list_box), gtk_label_new("Documents"), -1);
  gtk_list_box_insert(GTK_LIST_BOX(list_box), gtk_label_new("Downloads"), -1);

  return list_box;
}
