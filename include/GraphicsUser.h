#ifndef GRAPHICS_USER_H
#define GRAPHICS_USER_H

/*
* This File Deals with the Graphics Processing Behind the Scenes of the Search, Caching, and etc..
*/
#include "Search.h"
#include "OpenCL/cl.h"
#include <glib.h>
#include <glib/gstdio.h>

/*
* This Function Takes in a Directory and Returns all the Files/Folders in it
* @params: Directory - the Directory to Search
*/
extern inline const char* SearchFolders(const char* Directory);

#endif // GRAPHICS_USER_H
