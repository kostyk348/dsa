#define _GNU_SOURCE
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

typedef void *(*malloc_t)(size_t);
typedef void *(*memcpy_t)(void *, const void *, size_t);

static int g_dumped = 0;

static void maybe_dump(const void *dst, const void *src, size_t n) {
    (void)dst;
    if (n < 100000 || n > 16*1024*1024) return;   // big buffers only
    if (g_dumped >= 25) return;
    char path[256];
    snprintf(path, sizeof(path), "/tmp/ojd_dump_%d_%zu.bin", g_dumped, n);
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return;
    size_t w = n < 4 * 1024 * 1024 ? n : 4 * 1024 * 1024;
    ssize_t r = write(fd, src, w);
    (void)r;
    close(fd);
    g_dumped++;
}

void *memcpy(void *dst, const void *src, size_t n) {
    static memcpy_t real = NULL;
    if (!real) real = (memcpy_t)dlsym(RTLD_NEXT, "memcpy");
    if (real) maybe_dump(dst, src, n);
    return real(dst, src, n);
}
