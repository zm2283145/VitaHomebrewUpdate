#include <stddef.h>
#include <psp2/kernel/clib.h>

void *
memmove(void *destination, const void *source, size_t size)
{
    unsigned char *dst = (unsigned char *)destination;
    const unsigned char *src = (const unsigned char *)source;

    if (dst < src) {
        while (size-- != 0)
            *dst++ = *src++;
    } else if (dst > src) {
        dst += size;
        src += size;
        while (size-- != 0)
            *--dst = *--src;
    }
    return destination;
}

int
memcmp(const void *left, const void *right, size_t size)
{
    const unsigned char *a = (const unsigned char *)left;
    const unsigned char *b = (const unsigned char *)right;

    while (size-- != 0) {
        if (*a != *b)
            return *a < *b ? -1 : 1;
        ++a;
        ++b;
    }
    return 0;
}

size_t
strlen(const char *value)
{
    const char *end = value;

    while (*end != '\0')
        ++end;
    return (size_t)(end - value);
}
