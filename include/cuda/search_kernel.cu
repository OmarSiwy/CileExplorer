#include "search_kernel.cuh"
#include <cuda_runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 256
#define WARP_SIZE 32
#define PRIME 101
#define MAX_PATTERN_LENGTH 1024

// Macro for CUDA error checking
#define CHECK_CUDA_CALL(call)                                                  \
  do {                                                                         \
    cudaError_t err = call;                                                    \
    if (err != cudaSuccess) {                                                  \
      fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__,       \
              cudaGetErrorString(err));                                        \
      return 0;                                                                \
    }                                                                          \
  } while (0)

#define CHECK_CUDA_CALL_BOOL(call)                                             \
  do {                                                                         \
    cudaError_t err = call;                                                    \
    if (err != cudaSuccess) {                                                  \
      fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__,       \
              cudaGetErrorString(err));                                        \
      return false;                                                            \
    }                                                                          \
  } while (0)

__device__ __constant__ char d_pattern[MAX_PATTERN_LENGTH];
__device__ __constant__ unsigned long d_pattern_len;
__device__ __constant__ unsigned long d_pattern_hash;

// Precompute pattern hash on host
static unsigned long host_calculate_hash(const char *str, unsigned long len) {
    unsigned long hash = 0;
    for (unsigned long i = 0; i < len; i++) {
        hash = (hash * PRIME + str[i]);
    }
    return hash;
}

// Device function to calculate rolling hash
__device__ unsigned long calculate_hash(const char *str, unsigned long len) {
    unsigned long hash = 0;
    #pragma unroll 8
    for (unsigned long i = 0; i < len; i++) {
        hash = (hash * PRIME + str[i]);
    }
    return hash;
}

// Optimized kernel with warp-level primitives and early exit
__global__ void rabin_karp_kernel(const char *text,
                                  unsigned long text_len,
                                  int *found) {
    const unsigned long idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx >= text_len - d_pattern_len + 1)
        return;

    // Early exit if already found
    if (*found)
        return;

    // Calculate hash for this position
    unsigned long text_hash = calculate_hash(&text[idx], d_pattern_len);

    // Quick hash comparison first
    if (text_hash == d_pattern_hash) {
        // Verify character-by-character
        bool match = true;
        #pragma unroll 4
        for (unsigned long i = 0; i < d_pattern_len; i++) {
            if (text[idx + i] != d_pattern[i]) {
                match = false;
                break;
            }
        }

        if (match) {
            atomicExch(found, 1);  // Set found flag
        }
    }
}

__global__ void batch_rabin_karp_kernel(char **texts,
                                        unsigned long *text_lens,
                                        int file_count,
                                        int *found) {
    const int file_idx = blockIdx.y;
    const unsigned long idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (file_idx >= file_count)
        return;

    if (*found)
        return;

    const char *text = texts[file_idx];
    const unsigned long text_len = text_lens[file_idx];

    if (idx >= text_len - d_pattern_len + 1)
        return;

    unsigned long text_hash = calculate_hash(&text[idx], d_pattern_len);

    if (text_hash == d_pattern_hash) {
        bool match = true;
        #pragma unroll 4
        for (unsigned long i = 0; i < d_pattern_len; i++) {
            if (text[idx + i] != d_pattern[i]) {
                match = false;
                break;
            }
        }

        if (match) {
            atomicExch(found, 1);
        }
    }
}

extern "C" int cuda_rabin_karp_search(const char *pattern,
                                      const char *text,
                                      unsigned long pattern_len,
                                      unsigned long text_len) {
    if (!pattern || !text || pattern_len > MAX_PATTERN_LENGTH || pattern_len > text_len)
        return 0;

    // Precompute pattern hash on host
    unsigned long pattern_hash = host_calculate_hash(pattern, pattern_len);

    char *d_text = NULL;
    int *d_found = NULL;
    int h_found = 0;

    // Copy pattern and metadata to constant memory
    CHECK_CUDA_CALL(cudaMemcpyToSymbol(d_pattern,       // symbol
                                       pattern,          // src
                                       pattern_len));    // count
    CHECK_CUDA_CALL(cudaMemcpyToSymbol(d_pattern_len,   // symbol
                                       &pattern_len,     // src
                                       sizeof(unsigned long)));  // count
    CHECK_CUDA_CALL(cudaMemcpyToSymbol(d_pattern_hash,  // symbol
                                       &pattern_hash,    // src
                                       sizeof(unsigned long)));  // count

    // Allocate device memory
    CHECK_CUDA_CALL(cudaMalloc(&d_text, text_len));             // devPtr, size
    CHECK_CUDA_CALL(cudaMalloc(&d_found, sizeof(int)));         // devPtr, size

    // Copy data to device
    CHECK_CUDA_CALL(cudaMemcpy(d_text,                          // dst
                               text,                            // src
                               text_len,                        // count
                               cudaMemcpyHostToDevice));        // kind
    CHECK_CUDA_CALL(cudaMemset(d_found, 0, sizeof(int)));       // devPtr, value, count

    // Launch kernel with optimal grid size
    const unsigned long num_positions = text_len - pattern_len + 1;
    const unsigned long grid_size = (num_positions + BLOCK_SIZE - 1) / BLOCK_SIZE;

    rabin_karp_kernel<<<grid_size, BLOCK_SIZE>>>(d_text,       // text
                                                  text_len,     // text_len
                                                  d_found);     // found

    // Check for kernel errors
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        fprintf(stderr, "Kernel launch error: %s\n", cudaGetErrorString(err));
        cudaFree(d_text);
        cudaFree(d_found);
        return 0;
    }

    // Copy result back
    CHECK_CUDA_CALL(cudaMemcpy(&h_found,                        // dst
                               d_found,                         // src
                               sizeof(int),                     // count
                               cudaMemcpyDeviceToHost));        // kind

    // Cleanup
    cudaFree(d_text);
    cudaFree(d_found);

    return h_found;
}

extern "C" bool cuda_batch_search(const char *pattern,
                                  char **file_contents,
                                  int file_count,
                                  size_t *file_sizes) {
    if (!pattern || !file_contents || !file_sizes || file_count <= 0)
        return false;

    unsigned long pattern_len = strlen(pattern);
    if (pattern_len > MAX_PATTERN_LENGTH)
        return false;

    // Precompute pattern hash
    unsigned long pattern_hash = host_calculate_hash(pattern, pattern_len);

    // Copy pattern to constant memory
    CHECK_CUDA_CALL_BOOL(cudaMemcpyToSymbol(d_pattern,         // symbol
                                            pattern,            // src
                                            pattern_len));      // count
    CHECK_CUDA_CALL_BOOL(cudaMemcpyToSymbol(d_pattern_len,     // symbol
                                            &pattern_len,       // src
                                            sizeof(unsigned long)));  // count
    CHECK_CUDA_CALL_BOOL(cudaMemcpyToSymbol(d_pattern_hash,    // symbol
                                            &pattern_hash,      // src
                                            sizeof(unsigned long)));  // count

    // Allocate unified arrays for all files
    char **d_file_ptrs = NULL;
    unsigned long *d_file_lens = NULL;
    int *d_found = NULL;

    CHECK_CUDA_CALL_BOOL(cudaMalloc(&d_file_ptrs,               // devPtr
                                    file_count * sizeof(char*))); // size
    CHECK_CUDA_CALL_BOOL(cudaMalloc(&d_file_lens,               // devPtr
                                    file_count * sizeof(unsigned long))); // size
    CHECK_CUDA_CALL_BOOL(cudaMalloc(&d_found, sizeof(int)));    // devPtr, size
    CHECK_CUDA_CALL_BOOL(cudaMemset(d_found, 0, sizeof(int)));  // devPtr, value, count

    // Allocate device memory for each file and copy data
    char **h_d_file_ptrs = (char **)malloc(file_count * sizeof(char*));
    unsigned long *h_file_lens = (unsigned long *)malloc(file_count * sizeof(unsigned long));

    if (!h_d_file_ptrs || !h_file_lens) {
        free(h_d_file_ptrs);
        free(h_file_lens);
        cudaFree(d_file_ptrs);
        cudaFree(d_file_lens);
        cudaFree(d_found);
        return false;
    }

    // Single batch allocation and copy
    for (int i = 0; i < file_count; i++) {
        h_file_lens[i] = file_sizes[i];

        if (file_sizes[i] < pattern_len) {
            h_d_file_ptrs[i] = NULL;
            continue;
        }

        CHECK_CUDA_CALL_BOOL(cudaMalloc(&h_d_file_ptrs[i],              // devPtr
                                        file_sizes[i]));                 // size
        CHECK_CUDA_CALL_BOOL(cudaMemcpy(h_d_file_ptrs[i],               // dst
                                        file_contents[i],                // src
                                        file_sizes[i],                   // count
                                        cudaMemcpyHostToDevice));        // kind
    }

    // Copy pointer arrays to device
    CHECK_CUDA_CALL_BOOL(cudaMemcpy(d_file_ptrs,                        // dst
                                    h_d_file_ptrs,                       // src
                                    file_count * sizeof(char*),          // count
                                    cudaMemcpyHostToDevice));            // kind
    CHECK_CUDA_CALL_BOOL(cudaMemcpy(d_file_lens,                        // dst
                                    h_file_lens,                         // src
                                    file_count * sizeof(unsigned long),  // count
                                    cudaMemcpyHostToDevice));            // kind

    // Find max file size for grid sizing
    size_t max_len = 0;
    for (int i = 0; i < file_count; i++) {
        if (file_sizes[i] > max_len)
            max_len = file_sizes[i];
    }

    // Launch batch kernel with 2D grid
    const unsigned long max_positions = max_len - pattern_len + 1;
    dim3 grid((max_positions + BLOCK_SIZE - 1) / BLOCK_SIZE,    // x
              file_count);                                       // y
    dim3 block(BLOCK_SIZE);                                      // x

    batch_rabin_karp_kernel<<<grid, block>>>(d_file_ptrs,       // texts
                                             d_file_lens,        // text_lens
                                             file_count,         // file_count
                                             d_found);           // found

    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        fprintf(stderr, "Kernel launch error: %s\n", cudaGetErrorString(err));
        for (int i = 0; i < file_count; i++) {
            if (h_d_file_ptrs[i])
                cudaFree(h_d_file_ptrs[i]);
        }
        free(h_d_file_ptrs);
        free(h_file_lens);
        cudaFree(d_file_ptrs);
        cudaFree(d_file_lens);
        cudaFree(d_found);
        return false;
    }

    // Copy result
    int h_found = 0;
    CHECK_CUDA_CALL_BOOL(cudaMemcpy(&h_found,                           // dst
                                    d_found,                            // src
                                    sizeof(int),                        // count
                                    cudaMemcpyDeviceToHost));           // kind

    // Cleanup
    for (int i = 0; i < file_count; i++) {
        if (h_d_file_ptrs[i])
            cudaFree(h_d_file_ptrs[i]);
    }
    free(h_d_file_ptrs);
    free(h_file_lens);
    cudaFree(d_file_ptrs);
    cudaFree(d_file_lens);
    cudaFree(d_found);

    return h_found != 0;
}
