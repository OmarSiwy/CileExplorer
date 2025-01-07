#include "search_kernel.cuh"
#include <cuda_runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 256
#define PRIME 101
#define MAX_PATTERN_LENGTH 1024

// Macro for CUDA error checking
#define CHECK_CUDA_CALL(call)                                                  \
  do {                                                                         \
    cudaError_t err = call;                                                    \
    if (err != cudaSuccess) {                                                  \
      fprintf(stderr, "CUDA error at %s:%d: error code %d\n", __FILE__, __LINE__, err); \
      return 0;                                                                \
    }                                                                          \
  } while (0)

__device__ __constant__ char d_pattern[MAX_PATTERN_LENGTH];
__device__ __constant__ unsigned long d_pattern_len;

// Device function to calculate a hash
__device__ unsigned long calculate_hash(const char *str, unsigned long len) {
    unsigned long hash = 0;
    for (unsigned long i = 0; i < len; i++) {
        hash = (hash * PRIME + str[i]);
    }
    return hash;
}

// CUDA kernel for Rabin-Karp algorithm
__global__ void rabin_karp_kernel(const char *text, unsigned long text_len, int *results) {
    const unsigned long idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= text_len - d_pattern_len + 1)
        return;

    unsigned long pattern_hash = calculate_hash(d_pattern, d_pattern_len);
    unsigned long text_hash = calculate_hash(&text[idx], d_pattern_len);

    if (pattern_hash == text_hash) {
        int match = 1;
        for (unsigned long i = 0; i < d_pattern_len; i++) {
            if (text[idx + i] != d_pattern[i]) {
                match = 0;
                break;
            }
        }
        if (match) {
            results[idx] = 1;
        }
    }
}

extern "C" int cuda_rabin_karp_search(const char *pattern, const char *text, unsigned long pattern_len, unsigned long text_len) {
    if (!pattern || !text || pattern_len > MAX_PATTERN_LENGTH)
        return 0;

    char *d_text = NULL;
    int *d_results = NULL;
    int *h_results = NULL;
    int found = 0;

    // Copy pattern to constant memory
    CHECK_CUDA_CALL(cudaMemcpyToSymbol(d_pattern, pattern, pattern_len));
    CHECK_CUDA_CALL(cudaMemcpyToSymbol(d_pattern_len, &pattern_len, sizeof(unsigned long)));

    // Allocate device memory
    CHECK_CUDA_CALL(cudaMalloc(&d_text, text_len));
    CHECK_CUDA_CALL(cudaMalloc(&d_results, (text_len - pattern_len + 1) * sizeof(int)));

    // Copy text to device memory
    CHECK_CUDA_CALL(cudaMemcpy(d_text, text, text_len, cudaMemcpyHostToDevice));
    CHECK_CUDA_CALL(cudaMemset(d_results, 0, (text_len - pattern_len + 1) * sizeof(int)));

    // Launch the kernel
    const unsigned long grid_size = (text_len - pattern_len + BLOCK_SIZE - 1) / BLOCK_SIZE;
    rabin_karp_kernel<<<grid_size, BLOCK_SIZE>>>(d_text, text_len, d_results);

    // Check for kernel launch errors
    if (cudaGetLastError() != cudaSuccess) {
        fprintf(stderr, "Kernel launch error\n");
        cudaFree(d_text);
        cudaFree(d_results);
        return 0;
    }

    // Copy results back to host memory
    h_results = (int *)malloc((text_len - pattern_len + 1) * sizeof(int));
    if (!h_results) {
        fprintf(stderr, "Host memory allocation failed\n");
        cudaFree(d_text);
        cudaFree(d_results);
        return 0;
    }

    CHECK_CUDA_CALL(cudaMemcpy(h_results, d_results, (text_len - pattern_len + 1) * sizeof(int), cudaMemcpyDeviceToHost));

    // Check if any match was found
    for (unsigned long i = 0; i < text_len - pattern_len + 1; i++) {
        if (h_results[i]) {
            found = 1;
            break;
        }
    }

    // Cleanup
    free(h_results);
    cudaFree(d_text);
    cudaFree(d_results);

    return found;
}

extern "C" bool cuda_batch_search(const char *pattern, char **file_contents, int file_count, size_t *file_sizes) {
    if (!pattern || !file_contents || !file_sizes || file_count <= 0) {
        return false;
    }

    unsigned long pattern_len = strlen(pattern);
    if (pattern_len > MAX_PATTERN_LENGTH) {
        return false;
    }

    // Copy pattern to constant memory
    CHECK_CUDA_CALL(cudaMemcpyToSymbol(d_pattern, pattern, pattern_len));
    CHECK_CUDA_CALL(cudaMemcpyToSymbol(d_pattern_len, &pattern_len, sizeof(unsigned long)));

    char **d_contents = (char **)malloc(file_count * sizeof(char *));
    int **d_results = (int **)malloc(file_count * sizeof(int *));
    if (!d_contents || !d_results) {
        free(d_contents);
        free(d_results);
        return false;
    }

    bool found_match = false;

    for (int i = 0; i < file_count; i++) {
        size_t text_len = file_sizes[i];
        if (text_len < pattern_len)
            continue;

        CHECK_CUDA_CALL(cudaMalloc(&d_contents[i], text_len));
        CHECK_CUDA_CALL(cudaMalloc(&d_results[i], (text_len - pattern_len + 1) * sizeof(int)));

        CHECK_CUDA_CALL(cudaMemcpy(d_contents[i], file_contents[i], text_len, cudaMemcpyHostToDevice));
        CHECK_CUDA_CALL(cudaMemset(d_results[i], 0, (text_len - pattern_len + 1) * sizeof(int)));

        const unsigned long grid_size = (text_len - pattern_len + BLOCK_SIZE - 1) / BLOCK_SIZE;
        rabin_karp_kernel<<<grid_size, BLOCK_SIZE>>>(d_contents[i], text_len, d_results[i]);

        if (cudaGetLastError() != cudaSuccess) {
            fprintf(stderr, "Kernel launch error\n");
            for (int j = 0; j <= i; j++) {
                cudaFree(d_contents[j]);
                cudaFree(d_results[j]);
            }
            free(d_contents);
            free(d_results);
            return false;
        }
    }

    for (int i = 0; i < file_count; i++) {
        size_t result_size = file_sizes[i] - pattern_len + 1;
        int *h_results = (int *)malloc(result_size * sizeof(int));
        if (!h_results) {
            for (int j = 0; j <= i; j++) {
                cudaFree(d_contents[j]);
                cudaFree(d_results[j]);
            }
            free(d_contents);
            free(d_results);
            return false;
        }

        CHECK_CUDA_CALL(cudaMemcpy(h_results, d_results[i], result_size * sizeof(int), cudaMemcpyDeviceToHost));

        for (size_t j = 0; j < result_size; j++) {
            if (h_results[j]) {
                found_match = true;
                break;
            }
        }

        free(h_results);
        cudaFree(d_contents[i]);
        cudaFree(d_results[i]);

        if (found_match) {
            break;
        }
    }

    free(d_contents);
    free(d_results);

    return found_match;
}
