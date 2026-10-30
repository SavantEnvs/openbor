#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "packfile.h"
#include "sdlport.h"

int pak_init(void);
void pak_term(void);
void makefilenamecache(void);
void freefilenamecache(void);

#define MAX_NAMES 8
#define MAX_NAME_LEN 96

static int write_temp_pak(const uint8_t *data, size_t size, char *path, size_t path_size)
{
    const char *tmp = getenv("TMPDIR");
    int fd;
    size_t off = 0;

    if(!tmp || !*tmp) {
        tmp = "/tmp";
    }
    snprintf(path, path_size, "%s/openbor-pak-XXXXXX", tmp);
    fd = mkstemp(path);
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

static size_t collect_names(const uint8_t *data, size_t size, char names[MAX_NAMES][MAX_NAME_LEN])
{
    size_t count = 0, i = 0;

    while(i < size && count < MAX_NAMES) {
        size_t start = i;
        while(i < size && data[i] >= 0x20 && data[i] < 0x7f) {
            i++;
        }
        if(i - start >= 3 && i - start < MAX_NAME_LEN) {
            memcpy(names[count], data + start, i - start);
            names[count][i - start] = '\0';
            count++;
        }
        i++;
    }
    return count;
}

static void touch_entry(const char *name, const char *pakpath)
{
    unsigned char buf[512];
    int h = openpackfile((char *)name, (char *)pakpath);
    int n, total = 0;

    if(h < 0) {
        return;
    }
    seekpackfile(h, 0, SEEK_END);
    seekpackfile(h, 0, SEEK_SET);
    while(total < 4096 && (n = readpackfile(h, buf, sizeof(buf))) > 0) {
        total += n;
    }
    seekpackfile(h, 1, SEEK_SET);
    seekpackfile(h, 0, SEEK_CUR);
    packfileeof(h);
    closepackfile(h);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char path[512];
    char names[MAX_NAMES][MAX_NAME_LEN];
    size_t n, i;
    int rc;

    if(write_temp_pak(data, size, path, sizeof(path)) != 0) {
        return -1;
    }
    n = collect_names(data, size, names);

    packfile_mode(0);
    for(i = 0; i < n; i++) {
        touch_entry(names[i], path);
    }

    snprintf(packfile, sizeof(packfile), "%s", path);
    rc = pak_init();
    if(rc > 0) {
        makefilenamecache();
        for(i = 0; i < n; i++) {
            touch_entry(names[i], path);
        }
        freefilenamecache();
    }
    pak_term();

    unlink(path);
    return rc > 0 ? 0 : -1;
}
