#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

#include "globals.h"
#include "types.h"
#include "bitmap.h"
#include "screen.h"

int loadscreen32(char *filename, char *packfile, s_screen **screen);
s_bitmap *loadbitmap(char *filename, char *packfile, int format);
int loadimagepalette(char *filename, char *packfile, unsigned char *pal);

static int write_temp_png(const uint8_t *data, size_t size, char *path, size_t path_size)
{
    const char *tmp = getenv("TMPDIR");
    int fd;
    size_t off = 0;

    if(!tmp || !*tmp) {
        tmp = "/tmp";
    }
    snprintf(path, path_size, "%s/openbor-png-XXXXXX.png", tmp);
    fd = mkstemps(path, 4);
    if(fd < 0) {
        return -1;
    }
    while(off < size) {
        ssize_t n = write(fd, data + off, size - off);
        if(n <= 0) {
            close(fd);
            unlink(path);
            return -1;
        }
        off += (size_t)n;
    }
    close(fd);
    return 0;
}

/*
 * pngToScreen() takes a bare pointer with no length, and png_read_fn() memcpy()s whatever libpng asks
 * for, so any file whose chunks run past its end is an unconditional heap overread (see
 * known-findings/png-truncated-overread). Only complete chunk streams (signature .. IEND, every chunk in
 * bounds) reach the loader; CRCs are recomputed so mutations get past libpng's integrity check.
 */
static int seal_png(uint8_t *buf, size_t size)
{
    static const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a };
    size_t pos = 8;

    if(size < 8 + 12 || memcmp(buf, sig, 8) != 0) {
        return 0;
    }
    while(pos + 12 <= size) {
        uint32_t len = ((uint32_t)buf[pos] << 24) | ((uint32_t)buf[pos + 1] << 16) |
                       ((uint32_t)buf[pos + 2] << 8) | buf[pos + 3];
        size_t end;
        uint32_t crc;
        int is_iend;

        if(len > size - pos - 12) {
            return 0;
        }
        end = pos + 8 + len;
        crc = (uint32_t)crc32(crc32(0L, Z_NULL, 0), buf + pos + 4, (uInt)(len + 4));
        buf[end] = (uint8_t)(crc >> 24);
        buf[end + 1] = (uint8_t)(crc >> 16);
        buf[end + 2] = (uint8_t)(crc >> 8);
        buf[end + 3] = (uint8_t)crc;
        is_iend = memcmp(buf + pos + 4, "IEND", 4) == 0;
        pos = end + 4;
        if(is_iend) {
            return 1;
        }
    }
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    uint8_t *copy;
    char path[512];
    unsigned char pal[1024];
    s_bitmap *bitmap;
    s_screen *screen = NULL;
    int ok = 0;

    copy = malloc(size ? size : 1);
    if(!copy) {
        return -1;
    }
    memcpy(copy, data, size);
    if(!seal_png(copy, size)) {
        free(copy);
        return -1;
    }
    if(write_temp_png(copy, size, path, sizeof(path)) != 0) {
        free(copy);
        return -1;
    }
    free(copy);

    bitmap = loadbitmap(path, NULL, PIXEL_8);
    if(bitmap) {
        ok = 1;
        freebitmap(bitmap);
    }
    if(loadimagepalette(path, NULL, pal)) {
        ok = 1;
    }
    if(loadscreen32(path, NULL, &screen)) {
        ok = 1;
        freescreen(&screen);
    }

    unlink(path);
    return ok ? 0 : -1;
}
