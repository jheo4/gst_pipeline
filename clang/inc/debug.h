#include <stdio.h>
#define __DEBUG__ 1
#define debug_print(...) do { if (__DEBUG__) \
                              fprintf(stderr, "\t[DEBUG] [%s] ", __PRETTY_FUNCTION__); \
                              fprintf(stderr, __VA_ARGS__); \
                              fprintf(stderr, "\n"); } while (0)

#define DEBUG(...) debug_print(__VA_ARGS__)
