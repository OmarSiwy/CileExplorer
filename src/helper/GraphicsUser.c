#include "GraphicsUser.h"

inline const char* SearchFolders(const char* Directory) {
  GDir* dir;
  const gchar* entry;
  GError* error = NULL;

  dir = g_dir_open(Directory, 0, &error);

  if (!dir) {
    g_printerr("Error opening directory: %s\n", error->message);
    g_error_free(error);
    return "";
  }

  // Read each entry in the directory
  while ((entry = g_dir_read_name(dir)) != NULL) {
      g_print("%s\n", entry);
  }

  // Clean up
  g_dir_close(dir);
  return "";
}
