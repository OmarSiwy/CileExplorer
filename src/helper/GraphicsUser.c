#include "GraphicsUser.h"

cl_device_id CreateDevice(void) {
    cl_platform_id platform;
    cl_device_id dev;
    int err;

    // Identify a platform
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err < 0) {
        perror("Couldn't identify a platform");
        exit(1);
    }

    // Access a device (GPU)
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
    if (err == CL_DEVICE_NOT_FOUND) {
        // Fallback to CPU
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
    }
    if (err < 0) {
        perror("Couldn't access any devices");
        exit(1);
    }

    return dev;
}

char* readSource(const char* kernelPath) {
    FILE* fp = fopen(kernelPath, "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(EXIT_FAILURE);
    }
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    rewind(fp);
    char* source = (char*)malloc(length + 1);
    source[length] = '\0';
    fread(source, sizeof(char), length, fp);
    fclose(fp);
    return source;
}

cl_program BuildProgram(cl_context ctx, cl_device_id dev, const char* filename) {
    cl_program program;
    char *program_buffer, *program_log;
    size_t program_size, log_size;
    int err;

    // Read program file and place content into buffer
    program_buffer = readSource(filename);

    // Create program from file
    program = clCreateProgramWithSource(ctx, 1, (const char**)&program_buffer, NULL, &err);
    if (err < 0) {
        perror("Couldn't create the program");
        exit(1);
    }
    free(program_buffer);

    // Build program
    err = clBuildProgram(program, 1, &dev, NULL, NULL, NULL);
    if (err < 0) {
        // Find size of log and print to std output
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        program_log = (char*)malloc(log_size + 1);
        program_log[log_size] = '\0';
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, log_size + 1, program_log, NULL);
        printf("%s\n", program_log);
        free(program_log);
        exit(1);
    }

    return program;
}

const char** StartMatch(const char* pattern, const char** haystack, int num_strings, int* match_count) {
    cl_int err;
    cl_device_id device = CreateDevice();
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err < 0) {
        perror("Couldn't create a context");
        exit(1);
    }

    cl_program program = BuildProgram(context, device, "kernel.cl");

    int pat_length = strlen(pattern);
    size_t total_text_length = 0;
    int* text_lengths = (int*)malloc(num_strings * sizeof(int));

    for (int i = 0; i < num_strings; i++) {
        text_lengths[i] = strlen(haystack[i]);
        total_text_length += text_lengths[i];
    }

    char* all_texts = (char*)malloc(total_text_length);
    int* matches = (int*)calloc(total_text_length, sizeof(int));
    int current_pos = 0;

    for (int i = 0; i < num_strings; i++) {
        strcpy(all_texts + current_pos, haystack[i]);
        current_pos += text_lengths[i];
    }

    size_t global_size = total_text_length;
    size_t local_size = 128;

    cl_mem text_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, total_text_length * sizeof(char), all_texts, &err);
    if (err < 0) {
        perror("Couldn't create a buffer");
        exit(1);
    }
    cl_mem pattern_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, pat_length * sizeof(char), (void*)pattern, &err);
    if (err < 0) {
        perror("Couldn't create a buffer");
        exit(1);
    }
    cl_mem match_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, total_text_length * sizeof(int), NULL, &err);
    if (err < 0) {
        perror("Couldn't create a buffer");
        exit(1);
    }

    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);
    if (err < 0) {
        perror("Couldn't create a command queue");
        exit(1);
    }

    cl_kernel kernel = clCreateKernel(program, "Match", &err);
    if (err < 0) {
        perror("Couldn't create a kernel");
        exit(1);
    }

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &pattern_buffer);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &text_buffer);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &match_buffer);
    clSetKernelArg(kernel, 3, sizeof(int), &pat_length);
    clSetKernelArg(kernel, 4, sizeof(int), &total_text_length);

    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
    if (err < 0) {
        perror("Couldn't enqueue the kernel");
        exit(1);
    }

    err = clEnqueueReadBuffer(queue, match_buffer, CL_TRUE, 0, total_text_length * sizeof(int), matches, 0, NULL, NULL);
    if (err < 0) {
        perror("Couldn't read the buffer");
        exit(1);
    }

    current_pos = 0;
    int local_match_count = 0;
    int* match_indices = (int*)malloc(num_strings * sizeof(int));

    for (int i = 0; i < num_strings; i++) {
        int string_matched = 0;
        for (int j = 0; j < text_lengths[i]; j++) {
            if (matches[current_pos + j] != -1) {
                string_matched = 1;
                break;
            }
        }
        if (string_matched) {
            match_indices[local_match_count++] = i;
        }
        current_pos += text_lengths[i];
    }

    const char** matched_strings = (const char**)malloc(local_match_count * sizeof(char*));
    for (int i = 0; i < local_match_count; i++) {
        matched_strings[i] = haystack[match_indices[i]];
    }

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
