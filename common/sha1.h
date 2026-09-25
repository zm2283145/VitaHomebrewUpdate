#ifndef VHBU_SHA1_H
#define VHBU_SHA1_H

#include <stdint.h>

typedef struct VhbuSha1Context {
    uint32_t state[5];
    uint64_t bytes;
    unsigned char block[64];
    unsigned int used;
} VhbuSha1Context;

void vhbu_sha1_begin(VhbuSha1Context *context);
void vhbu_sha1_add(VhbuSha1Context *context, const void *data,
                   unsigned int size);
void vhbu_sha1_end(VhbuSha1Context *context, unsigned char digest[20]);

#endif
