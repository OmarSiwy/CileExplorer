#ifndef SEARCH_KERNEL_CUH
#define SEARCH_KERNEL_CUH

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Search for pattern in text using CUDA-accelerated Rabin-Karp
 * @param pattern Search pattern string
 * @param text Text to search in
 * @param pattern_len Length of pattern
 * @param text_len Length of text
 * @return 1 if found, 0 otherwise
 */
int cuda_rabin_karp_search(const char *pattern,
                           const char *text,
                           unsigned long pattern_len,
                           unsigned long text_len);

/**
 * Batch search across multiple files using CUDA
 * @param pattern Search pattern string
 * @param file_contents Array of file content strings
 * @param file_count Number of files
 * @param file_sizes Array of file sizes
 * @return true if pattern found in any file, false otherwise
 */
bool cuda_batch_search(const char *pattern,
                       char **file_contents,
                       int file_count,
                       size_t *file_sizes);

#ifdef __cplusplus
}
#endif

#endif // SEARCH_KERNEL_CUH
