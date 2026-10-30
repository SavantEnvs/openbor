/* Calls the same entry point as fuzz_png.c, but without its seal_png() chunk-walk filter. */
#include <stdio.h>
#include "globals.h"
#include "types.h"
#include "bitmap.h"
#include "screen.h"

int loadscreen32(char *filename, char *packfile, s_screen **screen);

int main(int argc, char **argv)
{
    s_screen *screen = NULL;
    int ok;

    if(argc != 2) {
        fprintf(stderr, "usage: %s file.png\n", argv[0]);
        return 2;
    }
    ok = loadscreen32(argv[1], NULL, &screen);
    fprintf(stderr, "loadscreen32: %s\n", ok ? "ok" : "rejected");
    return 0;
}
