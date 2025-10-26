#include "Pages/Sidebar.h"
#include <glib.h>
#include <stdio.h>
#include <string.h>

#define MAX_RECENT_DIRS 5
#define RECENT_DIRS_FILE "recent_directories.conf"

typedef struct {
  char *path;
  int visit_count;
} RecentDirInfo;

static GtkWidget *create_sidebar_row(const char *label, const char *path);
static void update_recent_section(SideBarWidget *side_bar);
static gint compare_recent_dirs(gconstpointer a, gconstpointer b);
static gchar *get_config_dir(void);
static gboolean on_list_motion(GtkWidget *widget, GdkEventMotion *event,
                               gpointer user_data);

SideBarWidget *SideBar_new(void) {
  SideBarWidget *side_bar = g_new0(SideBarWidget, 1);

  gchar *config_dir = get_config_dir();
  side_bar->config_file_path =
      g_build_filename(config_dir, RECENT_DIRS_FILE, NULL);
  g_free(config_dir);

  side_bar->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  side_bar->m_SideBar = side_bar->main_box;

  side_bar->list_box = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(side_bar->list_box),
                                  GTK_SELECTION_SINGLE);
  gtk_widget_set_can_focus(side_bar->list_box, FALSE);
  gtk_widget_add_events(side_bar->list_box,
                        GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK);

  GtkWidget *main_label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(main_label), "<b>Places</b>");
  gtk_widget_set_halign(main_label, GTK_ALIGN_START);
  gtk_widget_set_margin_start(main_label, 10);
  gtk_widget_set_margin_top(main_label, 10);
  gtk_widget_set_margin_bottom(main_label, 5);

  gtk_box_pack_start(GTK_BOX(side_bar->main_box), main_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(side_bar->main_box), side_bar->list_box, FALSE,
                     FALSE, 0);

  const char *home_dir = g_get_home_dir();
  gtk_list_box_insert(GTK_LIST_BOX(side_bar->list_box),
                      create_sidebar_row("Home", home_dir), -1);

  gchar *documents_path = g_build_filename(home_dir, "Documents", NULL);
  gtk_list_box_insert(GTK_LIST_BOX(side_bar->list_box),
                      create_sidebar_row("Documents", documents_path), -1);
  g_free(documents_path);

  gchar *downloads_path = g_build_filename(home_dir, "Downloads", NULL);
  gtk_list_box_insert(GTK_LIST_BOX(side_bar->list_box),
                      create_sidebar_row("Downloads", downloads_path), -1);
  g_free(downloads_path);

  GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_margin_top(separator, 10);
  gtk_widget_set_margin_bottom(separator, 5);
  gtk_box_pack_start(GTK_BOX(side_bar->main_box), separator, FALSE, FALSE, 0);

  GtkWidget *recent_label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(recent_label), "<b>Recent</b>");
  gtk_widget_set_halign(recent_label, GTK_ALIGN_START);
  gtk_widget_set_margin_start(recent_label, 10);
  gtk_widget_set_margin_bottom(recent_label, 5);
  gtk_box_pack_start(GTK_BOX(side_bar->main_box), recent_label, FALSE, FALSE,
                     0);

  side_bar->recent_list_box = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(side_bar->recent_list_box),
                                  GTK_SELECTION_SINGLE);
  gtk_widget_set_can_focus(side_bar->recent_list_box, FALSE);
  gtk_widget_add_events(side_bar->recent_list_box,
                        GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK);

  gtk_box_pack_start(GTK_BOX(side_bar->main_box), side_bar->recent_list_box,
                     FALSE, FALSE, 0);

  side_bar->recent_paths =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

  g_signal_connect(side_bar->list_box, "motion-notify-event",
                   G_CALLBACK(on_list_motion), side_bar);

  g_signal_connect(side_bar->recent_list_box, "motion-notify-event",
                   G_CALLBACK(on_list_motion), side_bar);

  SideBar_load_recent_directories(side_bar);

  if (g_hash_table_size(side_bar->recent_paths) == 0) {
    gtk_list_box_insert(GTK_LIST_BOX(side_bar->recent_list_box),
                        gtk_label_new("No recent directories"), -1);
  }

  gtk_widget_show_all(side_bar->main_box);

  return side_bar;
}

void SideBar_destroy(SideBarWidget *side_bar) {
  if (side_bar) {
    SideBar_save_recent_directories(side_bar);
    if (side_bar->recent_paths) {
      g_hash_table_destroy(side_bar->recent_paths);
    }
    if (side_bar->config_file_path) {
      g_free(side_bar->config_file_path);
    }
    g_free(side_bar);
  }
}

void SideBar_connect_navigation(SideBarWidget *side_bar, GCallback callback,
                                gpointer user_data) {
  if (!side_bar || !callback)
    return;

  g_signal_connect(side_bar->list_box, "row-activated", callback, user_data);
  g_signal_connect(side_bar->recent_list_box, "row-activated", callback,
                   user_data);
}

void SideBar_add_recent_directory(SideBarWidget *side_bar, const char *path) {
  if (!side_bar || !path)
    return;

  const char *home_dir = g_get_home_dir();
  gchar *documents_path = g_build_filename(home_dir, "Documents", NULL);
  gchar *downloads_path = g_build_filename(home_dir, "Downloads", NULL);

  gboolean is_main_dest =
      (g_strcmp0(path, home_dir) == 0 || g_strcmp0(path, documents_path) == 0 ||
       g_strcmp0(path, downloads_path) == 0);

  g_free(documents_path);
  g_free(downloads_path);

  if (is_main_dest)
    return;

  RecentDirInfo *info = g_hash_table_lookup(side_bar->recent_paths, path);
  if (info) {
    info->visit_count++;
  } else {
    info = g_new(RecentDirInfo, 1);
    info->path = g_strdup(path);
    info->visit_count = 1;
    g_hash_table_insert(side_bar->recent_paths, g_strdup(path), info);
  }

  update_recent_section(side_bar);
}

gboolean SideBar_save_recent_directories(SideBarWidget *side_bar) {
  if (!side_bar || !side_bar->config_file_path)
    return FALSE;

  gchar *config_dir = g_path_get_dirname(side_bar->config_file_path);
  g_mkdir_with_parents(config_dir, 0755);
  g_free(config_dir);

  FILE *file = fopen(side_bar->config_file_path, "w");
  if (!file) {
    g_warning("Failed to open config file for writing: %s",
              side_bar->config_file_path);
    return FALSE;
  }

  fprintf(file, "# Recent Directories Configuration\n");
  fprintf(file, "# Format: path|visit_count\n\n");

  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init(&iter, side_bar->recent_paths);

  while (g_hash_table_iter_next(&iter, &key, &value)) {
    RecentDirInfo *info = (RecentDirInfo *)value;
    fprintf(file, "%s|%d\n", info->path, info->visit_count);
  }

  fclose(file);
  return TRUE;
}

gboolean SideBar_load_recent_directories(SideBarWidget *side_bar) {
  if (!side_bar || !side_bar->config_file_path)
    return FALSE;

  FILE *file = fopen(side_bar->config_file_path, "r");
  if (!file) {
    return FALSE;
  }

  char line[4096];
  while (fgets(line, sizeof(line), file)) {
    if (line[0] == '#' || line[0] == '\n')
      continue;

    line[strcspn(line, "\n")] = 0;

    char *separator = strchr(line, '|');
    if (!separator)
      continue;

    *separator = '\0';
    char *path = line;
    int visit_count = atoi(separator + 1);

    if (!g_file_test(path, G_FILE_TEST_IS_DIR))
      continue;

    RecentDirInfo *info = g_new(RecentDirInfo, 1);
    info->path = g_strdup(path);
    info->visit_count = visit_count;
    g_hash_table_insert(side_bar->recent_paths, g_strdup(path), info);
  }

  fclose(file);

  if (g_hash_table_size(side_bar->recent_paths) > 0) {
    update_recent_section(side_bar);
  }

  return TRUE;
}

static GtkWidget *create_sidebar_row(const char *label, const char *path) {
  GtkWidget *row = gtk_list_box_row_new();
  GtkWidget *label_widget = gtk_label_new(label);
  gtk_widget_set_halign(label_widget, GTK_ALIGN_START);
  gtk_widget_set_margin_start(label_widget, 10);
  gtk_widget_set_margin_end(label_widget, 10);
  gtk_widget_set_margin_top(label_widget, 5);
  gtk_widget_set_margin_bottom(label_widget, 5);
  gtk_container_add(GTK_CONTAINER(row), label_widget);

  g_object_set_data_full(G_OBJECT(row), "destination-path", g_strdup(path),
                         g_free);
  gtk_widget_show_all(row);
  return row;
}

static gint compare_recent_dirs(gconstpointer a, gconstpointer b) {
  const RecentDirInfo *info_a = *(const RecentDirInfo **)a;
  const RecentDirInfo *info_b = *(const RecentDirInfo **)b;
  return info_b->visit_count - info_a->visit_count;
}

static void update_recent_section(SideBarWidget *side_bar) {
  if (!side_bar)
    return;

  GList *children =
      gtk_container_get_children(GTK_CONTAINER(side_bar->recent_list_box));
  for (GList *child = children; child != NULL; child = child->next) {
    gtk_widget_destroy(GTK_WIDGET(child->data));
  }
  g_list_free(children);

  GList *values = g_hash_table_get_values(side_bar->recent_paths);

  if (values == NULL) {
    gtk_list_box_insert(GTK_LIST_BOX(side_bar->recent_list_box),
                        gtk_label_new("No recent directories"), -1);
    return;
  }

  if (g_list_length(values) == 0) {
    gtk_list_box_insert(GTK_LIST_BOX(side_bar->recent_list_box),
                        gtk_label_new("No recent directories"), -1);
  } else {
    GList *sorted = g_list_sort(values, compare_recent_dirs);

    int count = 0;
    for (GList *iter = sorted; iter != NULL && count < MAX_RECENT_DIRS;
         iter = g_list_next(iter)) {
      RecentDirInfo *info = (RecentDirInfo *)iter->data;

      gchar *basename = g_path_get_basename(info->path);
      GtkWidget *row = create_sidebar_row(basename, info->path);
      g_free(basename);

      gtk_list_box_insert(GTK_LIST_BOX(side_bar->recent_list_box), row, -1);
      count++;
    }

    g_list_free(sorted);
  }
}

static gboolean on_list_motion(GtkWidget *widget, GdkEventMotion *event,
                               gpointer user_data) {
  gtk_widget_queue_draw(widget);
  return FALSE;
}

static gchar *get_config_dir(void) {
  const gchar *config_home = g_get_user_config_dir();
  gchar *app_config_dir = g_build_filename(config_home, "your-app-name", NULL);
  return app_config_dir;
}
