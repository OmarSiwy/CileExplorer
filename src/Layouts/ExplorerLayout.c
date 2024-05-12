#include "ExplorerLayout.h"
#include "gtk/gtk.h"

void ApplyGridLayout(GtkGrid *grid, int WidgetNumber, ...) {
  va_list args;
  va_start(args, WidgetNumber);

  WidgetPosition* widget;
  for (int i = 0; i < WidgetNumber; i++) {
    widget = va_arg(args, WidgetPosition*);
    gtk_grid_attach(GTK_GRID(grid), widget->widget, widget->left, widget->top, widget->width, widget->height);
  }

  va_end(args);
  gtk_widget_show_all(GTK_WIDGET(grid));
}
