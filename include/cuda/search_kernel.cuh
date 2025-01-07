#ifndef SEARCH_KERNEL_CUH
#define SEARCH_KERNEL_CUH

#ifdef __cplusplus
extern "C" {
#endif

int cuda_rabin_karp_search(const char *pattern, const char *text,
                           unsigned long pattern_len, unsigned long text_len);
bool cuda_batch_search(const char *pattern, char **file_contents,
                       int file_count, size_t *file_sizes);

#ifdef __cplusplus
}
#endif

#endif // SEARCH_KERNEL_CUH
