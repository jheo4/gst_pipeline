#include <stdio.h>
#define __DEBUG__ 1
#define __ERROR__ 1
#define debug_print(...) do { if (__DEBUG__) \
                              fprintf(stderr, "\t[DEBUG] [%s] ", __PRETTY_FUNCTION__); \
                              fprintf(stderr, __VA_ARGS__); \
                              fprintf(stderr, "\n"); } while (0)

#define error_print(...) do { if (__ERROR__) \
                              fprintf(stderr, "\033[0;31m"); \
                              fprintf(stderr, "[ERROR] [%s] ", __PRETTY_FUNCTION__); \
                              fprintf(stderr, __VA_ARGS__); \
                              fprintf(stderr, "\033[0m\n"); } while (0)

#define DEBUG(...) debug_print(__VA_ARGS__)
#define ERROR(...) error_print(__VA_ARGS__)
