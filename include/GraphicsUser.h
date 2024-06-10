#ifndef GRAPHICS_USER_H
#define GRAPHICS_USER_H

/*
* This File Deals with the Graphics Processing Behind the Scenes of the Search, Caching, and etc..
*/

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h> // Use standard OpenCL include path

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

cl_device_id CreateDevice(void);
cl_program BuildProgram(cl_context ctx, cl_device_id dev, const char* filename);

/*
* Uses OpenCL to search for a pattern in a haystack, powered by GPU
*
* @param pattern The pattern to search for
* @param haystack The list of strings to search through
* @param num_strings The number of strings in the haystack
* @param match_count Output parameter for the number of matches found
* @return The filepaths that match the pattern
*/
const char** StartMatch(const char* pattern, const char** haystack, int num_strings, int* match_count);

#ifdef __cplusplus
}
#endif

#endif // GRAPHICS_USER_H
