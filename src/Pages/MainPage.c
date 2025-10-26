#include "Pages/MainPage.h"
#include "Search.h"
#include <libgen.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PATH_LENGTH 4096

// Forward declarations
static GtkWidget *create_file_row(const char *filename, const char *full_path,
                                  GIcon *icon);
static void on_navigation_event(GtkWidget *widget, const char *path,
                                gpointer user_data);
static void on_search_triggered(GtkWidget *widget, const char *search_text,
                                gpointer user_data);
static gboolean filename_matches_pattern(const char *filename,
                                         const char *pattern);
static gboolean on_list_motion(GtkWidget *widget, GdkEventMotion *event,
                               gpointer user_data);

MainPageWidget *MainPage_new(TopBarWidget *top_bar, SideBarWidget *side_bar) {
  // Allocate the struct
  MainPageWidget *mp = g_new0(MainPageWidget, 1);

  // Create the list box
  mp->list_box = gtk_list_box_new();
  mp->m_MainPage = mp->list_box;
  mp->top_bar = top_bar;
  mp->side_bar = side_bar;
  mp->current_search_pattern = NULL;

  gtk_list_box_set_selection_mode(GTK_LIST_BOX(mp->list_box),
                                  GTK_SELECTION_SINGLE);
  gtk_widget_set_can_focus(mp->list_box, FALSE);
  gtk_widget_add_events(mp->list_box,
                        GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK);

  // Connect address changed signal
  TopBar_connect_address_changed(top_bar, G_CALLBACK(on_navigation_event), mp);

  // Connect search signal
  TopBar_connect_search(top_bar, G_CALLBACK(on_search_triggered), mp);

  // Connect main page row activation
  g_signal_connect(mp->list_box, "row-activated",
                   G_CALLBACK(on_navigation_event), mp);

  // Connect sidebar navigation
  SideBar_connect_navigation(side_bar, G_CALLBACK(on_navigation_event), mp);

  g_signal_connect(mp->list_box, "motion-notify-event",
                   G_CALLBACK(on_list_motion), mp);

  // Initial population
  const char *home_dir = g_get_home_dir();
  TopBar_set_address(mp->top_bar, home_dir);
  MainPage_update_state(mp, home_dir);

  return mp;
}

void MainPage_destroy(MainPageWidget *mp) {
  if (mp) {
    g_free(mp->current_search_pattern);
    // Widget will be destroyed by GTK when parent is destroyed
    g_free(mp);
  }
}

void MainPage_update_state(MainPageWidget *mp, const char *directory) {
  if (!mp || !directory)
    return;

  // TopBar holds the canonical current directory
  // Update it if we're navigating from row activation
  const char *current = TopBar_get_address(mp->top_bar);
  if (g_strcmp0(current, directory) != 0) {
    TopBar_set_address(mp->top_bar, directory);
  }

  // Clear existing children
  GList *children = gtk_container_get_children(GTK_CONTAINER(mp->list_box));
  for (GList *child = children; child != NULL; child = child->next) {
    gtk_widget_destroy(GTK_WIDGET(child->data));
  }
  g_list_free(children);

  // Add "Back" button if not at root directory (always show, no filtering)
  char parent_directory[MAX_PATH_LENGTH];
  if (g_strcmp0(directory, "/") != 0) {
    if (realpath(directory, parent_directory)) {
      char *parent = dirname(parent_directory);
      GtkWidget *back_row = create_file_row(".. (Back)", parent, NULL);
      gtk_list_box_insert(GTK_LIST_BOX(mp->list_box), back_row, -1);
    } else {
      g_printerr("Failed to get parent directory of: %s\n", directory);
    }
  }

  // Populate with files
  int file_count = 0;
  FileEntry **files = ListFilesInDir(directory, &file_count);

  if (!files) {
    gtk_list_box_insert(GTK_LIST_BOX(mp->list_box),
                        gtk_label_new("Failed to load files."), -1);
    return;
  }

  if (file_count == 0) {
    gtk_list_box_insert(GTK_LIST_BOX(mp->list_box),
                        gtk_label_new("No files found."), -1);
  } else {
    int visible_count = 0;

    for (int i = 0; i < file_count; i++) {
      // Apply search filter if pattern exists
      if (mp->current_search_pattern &&
          strlen(mp->current_search_pattern) > 0) {
        if (!filename_matches_pattern(files[i]->filename,
                                      mp->current_search_pattern)) {
          continue; // Skip this file
        }
      }

      char full_path[MAX_PATH_LENGTH];
      snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", directory,
               files[i]->filename);

      GtkWidget *row = create_file_row(files[i]->filename, full_path,
                                       (GIcon *)files[i]->icon_data);
      gtk_list_box_insert(GTK_LIST_BOX(mp->list_box), row, -1);
      visible_count++;
    }

    // Show message if search filtered everything out
    if (visible_count == 0 && mp->current_search_pattern &&
        strlen(mp->current_search_pattern) > 0) {
      gchar *msg =
          g_strdup_printf("No files match '%s'", mp->current_search_pattern);
      gtk_list_box_insert(GTK_LIST_BOX(mp->list_box), gtk_label_new(msg), -1);
      g_free(msg);
    }
  }

  FreeFileEntries(files, file_count);
  // gtk_widget_show_all(mp->m_MainPage);
}

static void on_navigation_event(GtkWidget *widget, const char *path,
                                gpointer user_data) {
  MainPageWidget *mp = (MainPageWidget *)user_data;
  const char *target_path = NULL;

  // Determine source and extract path
  if (GTK_IS_LIST_BOX(widget)) {
    GtkListBoxRow *row = GTK_LIST_BOX_ROW(path); // path is actually the row

    // Check if it's from MainPage or SideBar
    const char *main_path = g_object_get_data(G_OBJECT(row), "full-path");
    const char *sidebar_path =
        g_object_get_data(G_OBJECT(row), "destination-path");

    target_path = main_path ? main_path : sidebar_path;
  } else {
    // Called from address-changed signal (TopBar holds the new path already)
    target_path = path;
  }

  if (!target_path)
    return;

  // Validate and navigate
  if (g_file_test(target_path, G_FILE_TEST_IS_DIR)) {
    // Clear search when navigating to a new directory
    g_free(mp->current_search_pattern);
    mp->current_search_pattern = NULL;

    SideBar_add_recent_directory(mp->side_bar, target_path);

    MainPage_update_state(mp, target_path);
  } else if (g_file_test(target_path, G_FILE_TEST_EXISTS)) {
    g_print("File selected: %s\n", target_path);
  } else {
    g_printerr("Invalid path: %s\n", target_path);
    // TopBar already has the old valid path, just refresh the view
    const char *current = TopBar_get_address(mp->top_bar);
    MainPage_update_state(mp, current);
  }
}

static void on_search_triggered(GtkWidget *widget, const char *search_text,
                                gpointer user_data) {
  MainPageWidget *mp = (MainPageWidget *)user_data;

  if (!mp)
    return;

  // Update search pattern
  g_free(mp->current_search_pattern);

  if (search_text && strlen(search_text) > 0) {
    mp->current_search_pattern = g_strdup(search_text);
    g_print("Search triggered: '%s'\n", search_text);
  } else {
    mp->current_search_pattern = NULL;
    g_print("Search cleared\n");
  }

  // Refresh the current directory with the new filter
  const char *current_dir = TopBar_get_address(mp->top_bar);
  if (current_dir) {
    MainPage_update_state(mp, current_dir);
  }
}

// Simple case-insensitive substring match
static gboolean filename_matches_pattern(const char *filename,
                                         const char *pattern) {
  if (!filename || !pattern)
    return TRUE;

  if (strlen(pattern) == 0)
    return TRUE;

  // Convert both to lowercase for case-insensitive comparison
  gchar *filename_lower = g_utf8_strdown(filename, -1);
  gchar *pattern_lower = g_utf8_strdown(pattern, -1);

  gboolean matches = (strstr(filename_lower, pattern_lower) != NULL);

  g_free(filename_lower);
  g_free(pattern_lower);

  return matches;
}

// ==========================================
// Internal Functions
// ==========================================

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
  gtk_widget_show_all(row);

  return row;
}

static gboolean on_list_motion(GtkWidget *widget, GdkEventMotion *event,
                               gpointer user_data) {
  gtk_widget_queue_draw(widget);
  return FALSE;
}
