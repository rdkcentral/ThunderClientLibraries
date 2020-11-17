#include <ctype.h>
#include <playerinfo.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define Trace(fmt, ...)                                 \
    do {                                                \
        fprintf(stdout, "<< " fmt "\n", ##__VA_ARGS__); \
        fflush(stdout);                                 \
    } while (0)


int main(int argc, char* argv[])
{
    fprintf(stderr, "OK\n");
}