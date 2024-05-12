#ifndef SEARCH_H
#define SEARCH_H

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <glib.h>

/*
 * Given a directory path, the function will return a list of the names of all the files in the directory
 *
 * @param dirpath the path to the directory
 */
extern inline const char** ListFilesInDir(const char* dirpath);

/*
 * Given a file path, the function will check if the files exists or not
 *
 * @param filepath the path to the file that you want to check
 */
extern inline _Bool DoesFileExist(const char* filepath);

/*
 * Given a file path, the function will return the file's icon as a GtkWidget
 *
 * @param filepath the path to the file that you want to get an icon for
 */
extern inline GtkWidget* GetFileIconAsWidget(const char* filepath);

#endif // SEARCH_H
