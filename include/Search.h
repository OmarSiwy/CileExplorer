#ifndef SEARCH_H
#define SEARCH_H

#include <gio/gio.h>
#include <glib.h>
#include <stdbool.h>

typedef struct {
  char *filename;
  unsigned char *icon_data; // Pointer to image data
  int icon_width;
  int icon_height;
  int icon_channels;
} FileEntry;

typedef struct {
  char **file_contents;
  size_t *file_sizes;
  int file_count;
  int capacity;
} FileBatch;

/*
 * Given a directory path, the function will return a list of the names of all
 * the files in the directory
 *
 * @param dirpath the path to the directory
 */
extern inline FileEntry **ListFilesInDir(const char *dirpath, int *file_count);

/*
 * Given a file path, the function will check if the files exists or not
 *
 * @param filepath the path to the file that you want to check
 */
extern inline bool DoesFileExist(const char *filepath);

extern void FreeFileEntries(FileEntry **entries, int file_count);

extern bool cuda_search_files(const char *pattern, const char *directory);

#endif // SEARCH_H
