#include "Pages/MainPage.h"
#include "Pages/Topbar.h"
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

// Callback for TopBar address change
static void on_address_changed(GtkWidget *top_bar, const char *new_address,
                               GtkWidget *main_page) {
  g_print("Address changed to: %s\n", new_address);

  // Update the MainPage to display files in the new address
  main_page_set_files(main_page, new_address);
}

// Callback for row activation in the MainPage
static void on_file_row_activated(GtkListBox *list_box, GtkListBoxRow *row,
                                  gpointer user_data) {
  const char *full_path = g_object_get_data(G_OBJECT(row), "full-path");
  if (!full_path)
    return;

  // Check if it's a directory
  if (g_file_test(full_path, G_FILE_TEST_IS_DIR)) {
    g_print("Navigating to directory: %s\n", full_path);

    // Update the MainPage and TopBar with the new directory
    GtkWidget *main_page = GTK_WIDGET(user_data);
    main_page_set_files(main_page, full_path);

    GtkWidget *top_bar = gtk_widget_get_parent(GTK_WIDGET(list_box));
    top_bar_set_address(top_bar, full_path);
  } else {
    // Open the file with the default application
    g_print("Opening file: %s\n", full_path);
    GError *error = NULL;
    if (!g_app_info_launch_default_for_uri(full_path, NULL, &error)) {
      g_printerr("Error opening file: %s\n", error->message);
      g_error_free(error);
    }
  }
}

static void activate(GtkApplication *app, gpointer user_data) {
  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Explorer");
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

  // Create a vertical box to hold the top bar and main content
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

  // Create the TopBar
  GtkWidget *top_bar = top_bar_new("/app");
  gtk_box_pack_start(GTK_BOX(vbox), top_bar, FALSE, FALSE, 0);

  // Create the MainPage
  GtkWidget *main_page = main_page_new();
  gtk_box_pack_start(GTK_BOX(vbox), main_page, TRUE, TRUE, 0);

  // Connect the TopBar's address change signal to the MainPage
  top_bar_connect_address_change(top_bar, G_CALLBACK(on_address_changed),
                                 main_page);

  // Connect row activation signal
  main_page_connect_row_activated(main_page, G_CALLBACK(on_file_row_activated),
                                  main_page);

  gtk_container_add(GTK_CONTAINER(window), vbox);
  gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
  GtkApplication *app =
      gtk_application_new("com.example.explorer", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}
