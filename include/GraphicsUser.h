#ifndef GRAPHICS_USER_H
#define GRAPHICS_USER_H

/*
* This File Deals with the Graphics Processing Behind the Scenes of the Search, Caching, and etc..
*/

#define CL_TARGET_OPENCL_VERSION 120
#include <OpenCl/cl.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PROGRAM_FILE "src/helper/RabinKarp.cl"
#define KERNEL_FUNC "RabinKarp"
#define ARRAY_SIZE 64

extern cl_device_id CreateDevice(void);
extern cl_program BuildProgram(cl_context ctx, cl_device_id dev, const char* filename);

/*
* Uses OpenCL to search for a pattern in a haystack, powered by gpu
*
* @param pattern The filename to search for
* @param haystack the list of files to search through
* @return the filepaths that matches the pattern
*/
extern const char** StartMatch(const char* pattern, const char** haystack, int num_strings, int* match_count);

#endif // GRAPHICS_USER_H
