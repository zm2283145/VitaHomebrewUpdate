#ifndef VHBU_CHANGEINFO_H
#define VHBU_CHANGEINFO_H

#include <stddef.h>

#define VHBU_CHANGEINFO_OK 0
#define VHBU_CHANGEINFO_TRUNCATED 1

int vhbu_changeinfo_extract(const unsigned char *xml, size_t xml_size,
                            char *output, size_t output_capacity);
int vhbu_changeinfo_extract_or_fallback(
    const unsigned char *xml, size_t xml_size, char *output,
    size_t output_capacity, const char *fallback);

#endif
