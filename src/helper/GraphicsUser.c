#include "GraphicsUser.h"

cl_device_id CreateDevice(void) {
  cl_platform_id platform;
  cl_device_id dev;
  int err;

  /* Identify a platform */
  err = clGetPlatformIDs(1, &platform, NULL);
  if(err < 0) {
    perror("Couldn't identify a platform");
    exit(1);
  } 

  // Access a device
  // GPU
  err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
  if(err == CL_DEVICE_NOT_FOUND) {
    // CPU
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
  }
  if(err < 0) {
    perror("Couldn't access any devices");
    exit(1);   
  }

  return dev;
}

cl_program BuildProgram(cl_context ctx, cl_device_id dev, const char* filename) {
  cl_program program;
  FILE *program_handle;
  char *program_buffer, *program_log;
  size_t program_size, log_size;
  int err;

  /* Read program file and place content into buffer */
  program_handle = fopen(filename, "r");
  if(program_handle == NULL) {
    perror("Couldn't find the program file");
    exit(1);
  }
  fseek(program_handle, 0, SEEK_END);
  program_size = ftell(program_handle);
  rewind(program_handle);
  program_buffer = (char*)malloc(program_size + 1);
  program_buffer[program_size] = '\0';
  fread(program_buffer, sizeof(char), program_size, program_handle);
  fclose(program_handle);

  /* Create program from file 

  Creates a program from the source code in the RabinKarp.cl file. 
  Specifically, the code reads the file's content into a char array 
  called program_buffer, and then calls clCreateProgramWithSource.
  */
  program = clCreateProgramWithSource(ctx, 1, 
    (const char**)&program_buffer, &program_size, &err);
  if(err < 0) {
    perror("Couldn't create the program");
    exit(1);
  }
  free(program_buffer);

  /* Build program 

  The fourth parameter accepts options that configure the compilation. 
  These are similar to the flags used by gcc. For example, you can 
  define a macro with the option -DMACRO=VALUE and turn off optimization 
  with -cl-opt-disable.
  */
  err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
  if(err < 0) {

    /* Find size of log and print to std output */
    clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 
          0, NULL, &log_size);
    program_log = (char*) malloc(log_size + 1);
    program_log[log_size] = '\0';
    clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 
          log_size + 1, program_log, NULL);
    printf("%s\n", program_log);
    free(program_log);
    exit(1);
  }

  return program;
}


const char** StartMatch(const char* pattern, const char** haystack, int num_strings, int* match_count) {
  /* Initialization and context setup */
  cl_int err;
  cl_device_id device = CreateDevice();
  cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
  if(err < 0) {
    perror("Couldn't create a context");
    exit(1);   
  }

  /* Build program */
  cl_program program = BuildProgram(context, device, PROGRAM_FILE);

  /* Prepare data */
  int pat_length = strlen(pattern);
  size_t total_text_length = 0;
  int* text_lengths = (int*)malloc(num_strings * sizeof(int));

  // Compute total length of all strings and store individual lengths
  for (int i = 0; i < num_strings; i++) {
    text_lengths[i] = strlen(haystack[i]);
    total_text_length += text_lengths[i];
  }

  // Create a big buffer to hold all text concatenated
  char* all_texts = (char*)malloc(total_text_length + 1);
  int* matches = (int*)calloc(total_text_length, sizeof(int)); // For all individual character matches
  int* match_indices = (int*)malloc(num_strings * sizeof(int)); // Store indices of matches
  int current_pos = 0, local_match_count = 0;

  for (int i = 0; i < num_strings; i++) {
    strcpy(all_texts + current_pos, haystack[i]);
    current_pos += text_lengths[i];
  }

  /* Create data buffer 

  • `global_size`: total number of work items that will be 
    executed on the GPU (e.g. total size of your array)
  • `local_size`: size of local workgroup. Each workgroup contains 
    several work items and goes to a compute unit 

   Notes: 
  • Intel recommends workgroup size of 64-128. Often 128 is minimum to 
  get good performance on GPU
  • On NVIDIA Fermi, workgroup size must be at least 192 for full 
  utilization of cores
  • Optimal workgroup size differs across applications
  */

  size_t global_size = total_text_length; // WHY ONLY 8?
  size_t local_size = 128; 

  cl_mem text_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                                        total_text_length, all_texts, &err);
  cl_mem pattern_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                         pat_length, (void*) pattern, &err);
  cl_mem match_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
                                       total_text_length * sizeof(int), NULL, &err);
  if(err < 0) {
    perror("Couldn't create a buffer");
    exit(1);   
  };

  /* Create a command queue 

  Does not support profiling or out-of-order-execution
  */
  cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);
  if(err < 0) {
    perror("Couldn't create a command queue");
    exit(1);   
  };

  /* Set kernel arguments */
  cl_kernel kernel = clCreateKernel(program, KERNEL_FUNC, &err);
  clSetKernelArg(kernel, 0, sizeof(cl_mem), &text_buffer); // <=====INPUT
  clSetKernelArg(kernel, 1, sizeof(cl_mem), &pattern_buffer); // <=====INPUT
  clSetKernelArg(kernel, 2, sizeof(cl_mem), &match_buffer); // <=====OUTPUT
  clSetKernelArg(kernel, 3, sizeof(int), &total_text_length);
  clSetKernelArg(kernel, 4, sizeof(int), &pat_length);
  if(err < 0) {
    perror("Couldn't create a kernel argument");
    exit(1);
  }

  /* Execute kernel */
  clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size,
                        &local_size, 0, NULL, NULL);
  if(err < 0) {
    perror("Couldn't create a kernel argument");
    exit(1);
  }

  /* Read output */
  clEnqueueReadBuffer(queue, match_buffer, CL_TRUE, 0, 
                      total_text_length * sizeof(int), matches, 0, NULL, NULL);
  if(err < 0) {
    perror("Couldn't read the buffer");
    exit(1);
  }

  /* Process matches and collect indices of matched strings */
  for (int i = 0; i < num_strings; i++) {
    if (matches[current_pos] == 1) { // If match found at the start of this string
      match_indices[local_match_count++] = i;
    }
    current_pos += text_lengths[i]; // Move to start of next string
  }

  // Allocate space for matched string pointers
  const char** matched_strings = (const char**)malloc(local_match_count * sizeof(char*));
  for (int i = 0; i < local_match_count; i++) {
    matched_strings[i] = haystack[match_indices[i]];
  }

  /* Cleanup */
  free(matches);
  free(match_indices);
  free(text_lengths);
  free(all_texts);
  clReleaseMemObject(text_buffer);
  clReleaseMemObject(pattern_buffer);
  clReleaseMemObject(match_buffer);
  clReleaseKernel(kernel);
  clReleaseCommandQueue(queue);
  clReleaseProgram(program);
  clReleaseContext(context);

  *match_count = local_match_count;

  return matched_strings;
}
