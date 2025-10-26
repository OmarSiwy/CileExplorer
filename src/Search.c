#define _DEFAULT_SOURCE
#define STB_IMAGE_IMPLEMENTATION
#define MAX_FILES 1024
#define MAX_PATH_LENGTH 4096
#define INITIAL_CAPACITY 64

#include "Search.h"
#include "cuda/search_kernel.cuh"

#include "stb_image.h"
#include <gio/gio.h>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct {
  char **filenames;
  GIcon **icons;
  int count;
  int capacity;
} FileEntryCache;

static FileEntryCache *create_file_cache(int initial_capacity) {
  FileEntryCache *cache = malloc(sizeof(FileEntryCache));
  cache->filenames = malloc(initial_capacity * sizeof(char *)); // filenames
  cache->icons = malloc(initial_capacity * sizeof(GIcon *));    // icons
  cache->count = 0;
  cache->capacity = initial_capacity;
  return cache;
}

static void resize_file_cache(FileEntryCache *cache) {
  cache->capacity *= 2;
  cache->filenames = realloc(cache->filenames,                  // ptr
                             cache->capacity * sizeof(char *)); // size
  cache->icons = realloc(cache->icons,                          // ptr
                         cache->capacity * sizeof(GIcon *));    // size
}

static void free_file_cache(FileEntryCache *cache) {
  if (!cache)
    return;

  for (int i = 0; i < cache->count; i++) {
    free(cache->filenames[i]);
    if (cache->icons[i]) {
      g_object_unref(cache->icons[i]);
    }
  }

  free(cache->filenames);
  free(cache->icons);
  free(cache);
}

FileEntry **ListFilesInDir(const char *dirpath, int *file_count) {
  DIR *dir = opendir(dirpath);
  if (!dir) {
    perror("Error opening directory");
    return NULL;
  }

  FileEntryCache *cache = create_file_cache(INITIAL_CAPACITY);
  struct dirent *entry;
  char full_path[MAX_PATH_LENGTH];

  // First pass: collect all filenames (hot loop, minimal work)
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0) {
      continue;
    }

    if (cache->count >= cache->capacity) {
      resize_file_cache(cache);
    }

    cache->filenames[cache->count] = strdup(entry->d_name);
    cache->icons[cache->count] = NULL; // Initialize to NULL
    cache->count++;
  }
  closedir(dir);

  // Second pass: load icons in batch
  for (int i = 0; i < cache->count; i++) {
    snprintf(full_path,            // str
             MAX_PATH_LENGTH,      // size
             "%s/%s",              // format
             dirpath,              // ...
             cache->filenames[i]); // ...

    GFile *file = g_file_new_for_path(full_path);
    GFileInfo *info = g_file_query_info(file,                   // file
                                        "standard::icon",       // attributes
                                        G_FILE_QUERY_INFO_NONE, // flags
                                        NULL,                   // cancellable
                                        NULL);                  // error

    if (info) {
      GIcon *gicon = g_file_info_get_icon(info);
      if (gicon) {
        cache->icons[i] = g_object_ref(gicon);
      }
      g_object_unref(info);
    }
    g_object_unref(file);
  }

  // Convert SoA back to AoS for compatibility with existing API
  FileEntry **file_entries = malloc(cache->count * sizeof(FileEntry *));
  for (int i = 0; i < cache->count; i++) {
    file_entries[i] = malloc(sizeof(FileEntry));
    file_entries[i]->filename = cache->filenames[i];
    file_entries[i]->icon_data = cache->icons[i];
  }

  *file_count = cache->count;

  // Free only the cache structure, not the data (transferred to file_entries)
  free(cache->filenames);
  free(cache->icons);
  free(cache);

  return file_entries;
}

void FreeFileEntries(FileEntry **entries, int file_count) {
  if (!entries)
    return;

  for (int i = 0; i < file_count; i++) {
    free(entries[i]->filename);
    if (entries[i]->icon_data) {
      g_object_unref((GIcon *)entries[i]->icon_data);
    }
    free(entries[i]);
  }

  free(entries);
}

bool DoesFileExist(const char *filepath) {
  struct stat path_stat;
  return (stat(filepath, &path_stat) == 0);
}

// =============================
// Cuda-Specific Operations
// =============================

typedef struct {
  char **contents;
  size_t *sizes;
  char **paths;
  int count;
  int capacity;
} FileContentBatch;

static FileContentBatch *create_content_batch(int initial_capacity) {
  FileContentBatch *batch = malloc(sizeof(FileContentBatch));
  batch->contents = malloc(initial_capacity * sizeof(char *)); // contents
  batch->sizes = malloc(initial_capacity * sizeof(size_t));    // sizes
  batch->paths = malloc(initial_capacity * sizeof(char *));    // paths
  batch->count = 0;
  batch->capacity = initial_capacity;
  return batch;
}

static void resize_content_batch(FileContentBatch *batch) {
  batch->capacity *= 2;
  batch->contents = realloc(batch->contents,                   // ptr
                            batch->capacity * sizeof(char *)); // size
  batch->sizes = realloc(batch->sizes,                         // ptr
                         batch->capacity * sizeof(size_t));    // size
  batch->paths = realloc(batch->paths,                         // ptr
                         batch->capacity * sizeof(char *));    // size
}

static void free_content_batch(FileContentBatch *batch) {
  if (!batch)
    return;

  for (int i = 0; i < batch->count; i++) {
    free(batch->contents[i]);
    free(batch->paths[i]);
  }

  free(batch->contents);
  free(batch->sizes);
  free(batch->paths);
  free(batch);
}

bool cuda_search_files(const char *pattern, const char *directory) {
  DIR *dir = opendir(directory);
  if (!dir)
    return false;

  FileContentBatch *batch = create_content_batch(INITIAL_CAPACITY);
  struct dirent *entry;
  char full_path[MAX_PATH_LENGTH];
  struct stat path_stat;

  // Single pass with stat batching
  while ((entry = readdir(dir)) != NULL) {
    snprintf(full_path,       // str
             MAX_PATH_LENGTH, // size
             "%s/%s",         // format
             directory,       // ...
             entry->d_name);  // ...

    if (stat(full_path, &path_stat) != 0 || !S_ISREG(path_stat.st_mode)) {
      continue;
    }

    // Pre-allocate based on file size
    size_t file_size = path_stat.st_size;
    if (file_size == 0)
      continue;

    FILE *file = fopen(full_path, "rb"); // Binary mode is faster
    if (!file)
      continue;

    if (batch->count >= batch->capacity) {
      resize_content_batch(batch);
    }

    // Allocate exact size needed
    batch->contents[batch->count] = malloc(file_size + 1); // size
    if (!batch->contents[batch->count]) {
      fclose(file);
      continue;
    }

    size_t bytes_read = fread(batch->contents[batch->count], // ptr
                              1,                             // size
                              file_size,                     // nmemb
                              file);                         // stream
    batch->contents[batch->count][bytes_read] = '\0';
    batch->sizes[batch->count] = bytes_read;
    batch->paths[batch->count] = strdup(full_path);

    fclose(file);
    batch->count++;
  }
  closedir(dir);

  // Perform CUDA batch search
  bool found = false;
  if (batch->count > 0) {
    found = cuda_batch_search(pattern,         // pattern
                              batch->contents, // file_contents
                              batch->count,    // file_count
                              batch->sizes);   // file_sizes
  }

  free_content_batch(batch);
  return found;
}
