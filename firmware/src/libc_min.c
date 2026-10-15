/*
 * The firmware links with -nostdlib, but GCC may still emit calls to these
 * for struct copies, array initialisation or zeroing loops.
 */
#include <stddef.h>

void *memset(void *dst, int c, size_t n)
{
    unsigned char *d = dst;

    while (n-- > 0) {
        *d++ = (unsigned char)c;
    }
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;

    while (n-- > 0) {
        *d++ = *s++;
    }
    return dst;
}
