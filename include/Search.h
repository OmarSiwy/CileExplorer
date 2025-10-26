#ifndef SEARCH_H
#define SEARCH_H
#include <gio/gio.h>
#include <glib.h>
#include <stdbool.h>

typedef struct {
  char *filename;
  void *icon_data; // GIcon Pointer
} FileEntry;

typedef struct {
  char **file_contents;
  size_t *file_sizes;
  int file_count;
  int capacity;
} FileBatch;

/**
 * List all files in a directory
 * @param dirpath Directory path to scan
 * @param file_count Output parameter for number of files found
 * @return Array of FileEntry pointers, or NULL on error
 */
extern FileEntry **ListFilesInDir(const char *dirpath, int *file_count);

/**
 * Check if a file exists
 * @param filepath Path to check
 * @return true if file exists, false otherwise
 */
extern bool DoesFileExist(const char *filepath);

/**
 * Free array of FileEntry structures
 * @param entries Array to free
 * @param file_count Number of entries in array
 */
extern void FreeFileEntries(FileEntry **entries, int file_count);

/**
 * Search files in directory using CUDA
 * @param pattern Search pattern
 * @param directory Directory to search
 * @return true if pattern found, false otherwise
 */
extern bool cuda_search_files(const char *pattern, const char *directory);

#endif // SEARCH_H
