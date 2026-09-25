#include "sha1.h"

static uint32_t rotate_left(uint32_t value, unsigned int bits)
{
    return (value << bits) | (value >> (32u - bits));
}

static void transform(VhbuSha1Context *context,
                      const unsigned char block[64])
{
    uint32_t words[80];
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;
    unsigned int index;

    for (index = 0; index < 16u; ++index) {
        words[index] = ((uint32_t)block[index * 4u] << 24) |
                       ((uint32_t)block[index * 4u + 1u] << 16) |
                       ((uint32_t)block[index * 4u + 2u] << 8) |
                       (uint32_t)block[index * 4u + 3u];
    }
    for (; index < 80u; ++index)
        words[index] = rotate_left(words[index - 3u] ^ words[index - 8u] ^
                                   words[index - 14u] ^ words[index - 16u], 1u);

    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    for (index = 0; index < 80u; ++index) {
        uint32_t function;
        uint32_t constant;
        uint32_t temporary;
        if (index < 20u) {
            function = (b & c) | ((~b) & d);
            constant = 0x5a827999u;
        } else if (index < 40u) {
            function = b ^ c ^ d;
            constant = 0x6ed9eba1u;
        } else if (index < 60u) {
            function = (b & c) | (b & d) | (c & d);
            constant = 0x8f1bbcdcu;
        } else {
            function = b ^ c ^ d;
            constant = 0xca62c1d6u;
        }
        temporary = rotate_left(a, 5u) + function + e + constant +
                    words[index];
        e = d;
        d = c;
        c = rotate_left(b, 30u);
        b = a;
        a = temporary;
    }
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
}

void vhbu_sha1_begin(VhbuSha1Context *context)
{
    context->state[0] = 0x67452301u;
    context->state[1] = 0xefcdab89u;
    context->state[2] = 0x98badcfeu;
    context->state[3] = 0x10325476u;
    context->state[4] = 0xc3d2e1f0u;
    context->bytes = 0;
    context->used = 0;
}

void vhbu_sha1_add(VhbuSha1Context *context, const void *input,
                   unsigned int size)
{
    const unsigned char *data = (const unsigned char *)input;
    context->bytes += size;
    while (size > 0u) {
        unsigned int available = 64u - context->used;
        unsigned int amount = size < available ? size : available;
        unsigned int index;
        for (index = 0; index < amount; ++index)
            context->block[context->used + index] = data[index];
        context->used += amount;
        data += amount;
        size -= amount;
        if (context->used == 64u) {
            transform(context, context->block);
            context->used = 0;
        }
    }
}

void vhbu_sha1_end(VhbuSha1Context *context, unsigned char digest[20])
{
    uint64_t bits = context->bytes * 8u;
    unsigned int index;

    context->block[context->used++] = 0x80u;
    if (context->used > 56u) {
        while (context->used < 64u)
            context->block[context->used++] = 0;
        transform(context, context->block);
        context->used = 0;
    }
    while (context->used < 56u)
        context->block[context->used++] = 0;
    for (index = 0; index < 8u; ++index)
        context->block[63u - index] = (unsigned char)(bits >> (index * 8u));
    transform(context, context->block);
    for (index = 0; index < 5u; ++index) {
        digest[index * 4u] = (unsigned char)(context->state[index] >> 24);
        digest[index * 4u + 1u] =
            (unsigned char)(context->state[index] >> 16);
        digest[index * 4u + 2u] =
            (unsigned char)(context->state[index] >> 8);
        digest[index * 4u + 3u] = (unsigned char)context->state[index];
    }
}
