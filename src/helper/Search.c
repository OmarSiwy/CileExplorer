#define _DEFAULT_SOURCE
#define STB_IMAGE_IMPLEMENTATION
#define MAX_FILES 1024
#define MAX_PATH_LENGTH 4096

#include "Search.h"
#include "cuda/search_kernel.cuh"

// File Icons
#include "stb_image.h"
#include <gio/gio.h>

// Print Statements for debugging
#include <dirent.h> // Include for working with directories
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

inline FileEntry **ListFilesInDir(const char *dirpath, int *file_count) {
  DIR *dir = opendir(dirpath);
  if (!dir) {
    perror("Error opening directory");
    return NULL;
  }

  struct dirent *entry;
  FileEntry **file_entries =
      (FileEntry **)malloc(MAX_FILES * sizeof(FileEntry *));
  int count = 0;

  while ((entry = readdir(dir)) != NULL && count < MAX_FILES) {
    // Skip .. entries
    if (strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0) {
      continue;
    }

    FileEntry *file_entry = (FileEntry *)malloc(sizeof(FileEntry));
    char full_path[MAX_PATH_LENGTH];
    snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", dirpath, entry->d_name);

    // Store file name
    file_entry->filename = strdup(entry->d_name);

    // Get file icon using GIO
    GFile *file = g_file_new_for_path(full_path);
    GFileInfo *info = g_file_query_info(file, "standard::icon",
                                        G_FILE_QUERY_INFO_NONE, NULL, NULL);

    if (info) {
      GIcon *gicon = g_file_info_get_icon(info);
      if (gicon) {
        file_entry->icon_data =
            g_object_ref(gicon); // Increment reference count
        g_print("Loaded icon for file: %s\n", file_entry->filename);
      } else {
        file_entry->icon_data = NULL; // No icon available
        g_print("No icon available for file: %s\n", file_entry->filename);
      }
      g_object_unref(info);
    } else {
      file_entry->icon_data = NULL; // Failed to query info
      g_print("Failed to query info for file: %s\n", file_entry->filename);
    }
    g_object_unref(file);

    file_entries[count++] = file_entry;
  }

  closedir(dir);

  *file_count = count;
  return file_entries;
}
// Helper function to free the FileEntry array
void FreeFileEntries(FileEntry **entries, int file_count) {
  if (!entries)
    return;

  for (int i = 0; i < file_count; i++) {
    free(entries[i]->filename);
    if (entries[i]->icon_data) {
      g_object_unref((GIcon *)entries[i]->icon_data); // Release GIcon reference
      g_print("Freed icon for file entry %d\n", i);
    }
    free(entries[i]);
  }

  free(entries);
}

inline bool DoesFileExist(const char *filepath) {
  struct stat path_stat;
  return (stat(filepath, &path_stat) == 0);
}

bool cuda_search_files(const char *pattern, const char *directory) {
  DIR *dir;
  struct dirent *entry;
  char **file_contents = NULL;
  size_t *file_sizes = NULL;
  int file_count = 0;
  char full_path[MAX_PATH_LENGTH];

  dir = opendir(directory);
  if (!dir)
    return false;

  // Allocate arrays for file contents and sizes
  file_contents = (char **)malloc(MAX_FILES * sizeof(char *));
  file_sizes = (size_t *)malloc(MAX_FILES * sizeof(size_t));

  if (!file_contents || !file_sizes) {
    if (file_contents)
      free(file_contents);
    if (file_sizes)
      free(file_sizes);
    closedir(dir);
    return false;
  }

  // Read all files in directory
  while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
    struct stat path_stat;
    snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", directory, entry->d_name);

    if (stat(full_path, &path_stat) == 0 && S_ISREG(path_stat.st_mode)) {
      FILE *file = fopen(full_path, "r");
      if (!file)
        continue;

      fseek(file, 0, SEEK_END);
      size_t file_size = ftell(file);
      fseek(file, 0, SEEK_SET);

      file_contents[file_count] = (char *)malloc(file_size + 1);
      if (!file_contents[file_count]) {
        fclose(file);
        continue;
      }

      size_t bytes_read = fread(file_contents[file_count], 1, file_size, file);
      file_contents[file_count][bytes_read] = '\0';
      file_sizes[file_count] = bytes_read;

      fclose(file);
      file_count++;
    }
  }
  closedir(dir);

  // Perform CUDA batch search
  bool found = false;
  if (file_count > 0) {
    found = cuda_batch_search(pattern, file_contents, file_count, file_sizes);
  }

  // Cleanup
  for (int i = 0; i < file_count; i++) {
    free(file_contents[i]);
  }
  free(file_contents);
  free(file_sizes);

  return found;
}
