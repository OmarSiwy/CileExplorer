#include "Pages/MainPage.h"
#include "Pages/Topbar.h" // Include TopBar for top_bar_set_address
#include "Search.h"
#include <libgen.h> // For dirname
#include <stdio.h>
#include <stdlib.h> // For realpath
#include <string.h>

#define MAX_PATH_LENGTH 4096 // Set a suitable max path length for your system

static void on_file_row_activated(GtkListBox *list_box, GtkListBoxRow *row,
                                  gpointer user_data) {
  const char *full_path = g_object_get_data(G_OBJECT(row), "full-path");
  if (!full_path) {
    g_printerr("Error: full_path is NULL\n");
    return;
  }

  // Check if it's the "Back" button
  const char *label =
      gtk_label_get_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(row))));
  if (g_strcmp0(label, ".. (Back)") == 0) {
    g_print("Navigating to parent directory: %s\n", full_path);

    // Update the MainPage and TopBar with the parent directory
    GtkWidget *main_page = GTK_WIDGET(user_data);
    main_page_set_files(main_page, full_path);

    GtkWidget *top_bar = gtk_widget_get_parent(
        GTK_WIDGET(list_box)); // Assuming direct parent is TopBar
    top_bar_set_address(top_bar, full_path);
    return;
  }

  // Check if it's a directory
  if (g_file_test(full_path, G_FILE_TEST_IS_DIR)) {
    g_print("Navigating to directory: %s\n", full_path);

    // Update the MainPage and TopBar with the new directory
    GtkWidget *main_page = GTK_WIDGET(user_data);
    main_page_set_files(main_page, full_path);

    GtkWidget *top_bar = gtk_widget_get_parent(GTK_WIDGET(list_box));
    top_bar_set_address(top_bar, full_path);
  } else {
    // Open the file using xdg-open
    g_print("Opening file: %s\n", full_path);

    char command[MAX_PATH_LENGTH];
    snprintf(command, MAX_PATH_LENGTH, "xdg-open \"%s\" &", full_path);
    int result = system(command);
    if (result != 0) {
      g_printerr("Failed to open file with system command: %s\n", command);
    }
  }
}

static GtkWidget *create_file_row(const char *filename, const char *full_path,
                                  GIcon *icon) {
  GtkWidget *row = gtk_list_box_row_new();
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

  // Add file icon
  GtkWidget *icon_widget;
  if (icon) {
    icon_widget = gtk_image_new_from_gicon(icon, GTK_ICON_SIZE_LARGE_TOOLBAR);
  } else {
    icon_widget = gtk_image_new_from_icon_name("text-x-generic",
                                               GTK_ICON_SIZE_LARGE_TOOLBAR);
  }
  gtk_box_pack_start(GTK_BOX(box), icon_widget, FALSE, FALSE, 5);

  // Add file name
  GtkWidget *label = gtk_label_new(filename);
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
  gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 5);

  gtk_container_add(GTK_CONTAINER(row), box);

  // Store the full path in the row for later use
  g_object_set_data_full(G_OBJECT(row), "full-path", g_strdup(full_path),
                         g_free);

  return row;
}

GtkWidget *main_page_new(void) {
  GtkWidget *list_box = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_SINGLE);
  return list_box;
}

void main_page_set_files(GtkWidget *main_page, const char *directory) {
  // Clear existing children
  GList *children = gtk_container_get_children(GTK_CONTAINER(main_page));
  for (GList *child = children; child != NULL; child = child->next) {
    gtk_widget_destroy(GTK_WIDGET(child->data));
  }
  g_list_free(children);

  // Add "Back" button if not at root directory
  char parent_directory[MAX_PATH_LENGTH];
  if (g_strcmp0(directory, "/") != 0) {
    if (realpath(directory,
                 parent_directory)) { // Check return value of realpath
      char *parent = dirname(parent_directory); // Get parent directory
      GtkWidget *back_row = create_file_row(".. (Back)", parent, NULL);
      gtk_list_box_insert(GTK_LIST_BOX(main_page), back_row, -1);
    } else {
      g_printerr("Failed to get parent directory of: %s\n", directory);
    }
  }

  // Populate with files
  int file_count = 0;
  FileEntry **files = ListFilesInDir(directory, &file_count);

  if (!files) {
    gtk_list_box_insert(GTK_LIST_BOX(main_page),
                        gtk_label_new("Failed to load files."), -1);
    return;
  }

  if (file_count == 0) {
    gtk_list_box_insert(GTK_LIST_BOX(main_page),
                        gtk_label_new("No files found."), -1);
  } else {
    for (int i = 0; i < file_count; i++) {
      char full_path[MAX_PATH_LENGTH];
      snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", directory,
               files[i]->filename);

      GtkWidget *row = create_file_row(files[i]->filename, full_path,
                                       (GIcon *)files[i]->icon_data);
      gtk_list_box_insert(GTK_LIST_BOX(main_page), row, -1);
    }
  }

  FreeFileEntries(files, file_count);
  gtk_widget_show_all(main_page);
}

void main_page_connect_row_activated(GtkWidget *main_page, GCallback callback,
                                     gpointer user_data) {
  g_signal_connect(main_page, "row-activated", callback, user_data);
}
