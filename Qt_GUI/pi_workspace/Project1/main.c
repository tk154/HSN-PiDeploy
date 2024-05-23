#include <stdio.h>

int main(int argc, char* argv[]) {
    // stdout is fully buffered when redirecting to a file
    // The following call enables line buffering so that it will be flushed on every newline character
    // If it is removed or uncommented every output has to be flushed explicitly with fflush(stdout)
    setvbuf(stdout, NULL, _IOLBF, 0);

    if (argc > 1) {
        printf("Given arguments:");
        for (int i = 1; i < argc; i++)
            printf(" %s", argv[i]);
        printf("\n");
    }
    else
        printf("No arguments given.\n");
}
