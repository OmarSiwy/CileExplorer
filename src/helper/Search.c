#include "Search.h"

inline const char** ListFilesInDir(const char* dirpath) {
    GDir *dir;
    GError *error = NULL;
    const gchar *filename;
    GList *files = NULL;

    dir = g_dir_open(dirpath, 0, &error);
    if (!dir) {
        g_print("Error opening directory: %s\n", error->message);
        g_error_free(error);
        return NULL;
    }

    while ((filename = g_dir_read_name(dir)) != NULL) {
        files = g_list_prepend(files, g_strdup(filename));
    }
    g_dir_close(dir);

    // Convert GList to array
    int length = g_list_length(files);
    const char **fileArray = g_new(const char*, length + 1);
    GList *l;
    int i;
    for (i = 0, l = files; l != NULL; l = l->next, i++) {
        fileArray[i] = (const char*)l->data;
    }
    fileArray[i] = NULL;  // NULL-terminated array

    g_list_free_full(files, g_free);  // Clean up list and data

    return fileArray;
}

inline _Bool DoesFileExist(const char* filepath) {
    GFile *file = g_file_new_for_path(filepath);
    GFileInfo *info = g_file_query_info(file, "standard::*", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);
    _Bool exists = info != NULL;
    if (info) g_object_unref(info);
    g_object_unref(file);
    return exists;
}

inline GtkWidget* GetFileIconAsWidget(const char* filepath) {
  GError *error = NULL;
  GFile *file = g_file_new_for_path(filepath);
  GFileInfo *info = g_file_query_info(file, "standard::icon", G_FILE_QUERY_INFO_NONE, NULL, &error);

  if (error != NULL) {
      g_print("Error retrieving file info: %s\n", error->message);
      g_error_free(error);
      return NULL;
  }

  GIcon *icon = g_file_info_get_icon(info);
  GtkWidget *image = gtk_image_new_from_gicon(icon, GTK_ICON_SIZE_DIALOG);
  g_object_unref(info);
  g_object_unref(file);
  
  return image;
}

void AhoCorasick(const char* text, const char* pattern, int* matches, int text_length, int pat_length) {

}
