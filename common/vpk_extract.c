#include "vpk_extract.h"

#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/sysmem.h>
#include <stddef.h>
#include <stdint.h>
#include <zlib.h>

#define VHBU_ZIP_LOCAL_FILE 0x04034B50u
#define VHBU_ZIP_CENTRAL_FILE 0x02014B50u
#define VHBU_ZIP_END 0x06054B50u
#define VHBU_IO_CHUNK (8u * 1024u)
#define VHBU_PATH_CAPACITY 384u
#define VHBU_ZALLOC_MAGIC 0x56485A41u

typedef struct VhbuZAllocation {
    uint32_t magic;
    SceUID uid;
    uint32_t reserved[2];
} VhbuZAllocation;

static uint16_t read_u16(const unsigned char *data)
{
    return (uint16_t)data[0] | (uint16_t)((uint16_t)data[1] << 8);
}

static uint32_t read_u32(const unsigned char *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static int read_exact(SceUID file, void *buffer, unsigned int size)
{
    unsigned int offset = 0;
    while (offset < size) {
        int result = sceIoRead(file, (unsigned char *)buffer + offset,
                               size - offset);
        if (result <= 0)
            return result < 0 ? result : -1;
        offset += (unsigned int)result;
    }
    return 0;
}

static int write_exact(SceUID file, const void *buffer, unsigned int size)
{
    unsigned int offset = 0;
    while (offset < size) {
        int result = sceIoWrite(file, (const unsigned char *)buffer + offset,
                                size - offset);
        if (result <= 0)
            return result < 0 ? result : -1;
        offset += (unsigned int)result;
    }
    return 0;
}

static int safe_relative_path(const char *path)
{
    unsigned int component = 0;
    unsigned int index;

    if (path == NULL || path[0] == '\0' || path[0] == '/' ||
        path[0] == '\\')
        return 0;
    for (index = 0; path[index] != '\0'; ++index) {
        char value = path[index];
        if (value == '\\' || value == ':')
            return 0;
        if (value == '/') {
            if (component == 0)
                return 0;
            component = 0;
            continue;
        }
        if (component == 0 && value == '.' &&
            (path[index + 1] == '/' || path[index + 1] == '\0'))
            return 0;
        if (component == 0 && value == '.' && path[index + 1] == '.' &&
            (path[index + 2] == '/' || path[index + 2] == '\0'))
            return 0;
        ++component;
    }
    return component != 0 || (index > 0 && path[index - 1] == '/');
}

static int make_parent_directories(char *path)
{
    unsigned int index;
    for (index = 5; path[index] != '\0'; ++index) {
        if (path[index] == '/') {
            path[index] = '\0';
            (void)sceIoMkdir(path, 0777);
            path[index] = '/';
        }
    }
    return 0;
}

static voidpf vhbu_zalloc(voidpf opaque, uInt items, uInt size)
{
    VhbuZAllocation *header;
    void *base = NULL;
    uint64_t requested = (uint64_t)items * (uint64_t)size;
    uint32_t allocation_size;
    SceUID uid;
    (void)opaque;

    if (requested > 0x7FFFFFFFu - sizeof(*header))
        return NULL;
    allocation_size = (uint32_t)requested + sizeof(*header);
    allocation_size = (allocation_size + 0xFFFu) & ~0xFFFu;
    uid = sceKernelAllocMemBlock("vhbu_zlib",
        SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, allocation_size, NULL);
    if (uid < 0 || sceKernelGetMemBlockBase(uid, &base) < 0 || base == NULL) {
        if (uid >= 0)
            (void)sceKernelFreeMemBlock(uid);
        return NULL;
    }
    header = (VhbuZAllocation *)base;
    header->magic = VHBU_ZALLOC_MAGIC;
    header->uid = uid;
    return header + 1;
}

static void vhbu_zfree(voidpf opaque, voidpf address)
{
    VhbuZAllocation *header;
    (void)opaque;
    if (address == NULL)
        return;
    header = ((VhbuZAllocation *)address) - 1;
    if (header->magic == VHBU_ZALLOC_MAGIC) {
        SceUID uid = header->uid;
        header->magic = 0;
        (void)sceKernelFreeMemBlock(uid);
    }
}

/* VitaSDK's static zlib archive retains its default allocator and C-runtime
 * references even though this extractor supplies zalloc/zfree.  Keep the
 * plugin freestanding by satisfying those references with kernel-backed
 * implementations. */
#ifndef VHBU_USE_SYSTEM_LIBC
void *memcpy(void *destination, const void *source, size_t size)
{
    return sceClibMemcpy(destination, source, size);
}

void *memset(void *destination, int value, size_t size)
{
    return sceClibMemset(destination, value, size);
}

void *malloc(size_t size)
{
    return vhbu_zalloc(NULL, 1u, (uInt)size);
}

void free(void *address)
{
    vhbu_zfree(NULL, address);
}
#endif

static int extract_stored(SceUID source, SceUID destination,
                          uint32_t compressed_size, uint32_t expected_crc,
                          uint32_t expected_size)
{
    unsigned char buffer[VHBU_IO_CHUNK];
    uint32_t remaining = compressed_size;
    uint32_t written = 0;
    uLong crc = crc32(0L, Z_NULL, 0);

    while (remaining != 0) {
        unsigned int chunk = remaining < sizeof(buffer) ?
            remaining : (unsigned int)sizeof(buffer);
        int result = read_exact(source, buffer, chunk);
        if (result < 0)
            return result;
        result = write_exact(destination, buffer, chunk);
        if (result < 0)
            return result;
        crc = crc32(crc, buffer, chunk);
        written += chunk;
        remaining -= chunk;
    }
    return written == expected_size && (uint32_t)crc == expected_crc ? 0 : -20;
}

static int extract_deflated(SceUID source, SceUID destination,
                            uint32_t compressed_size, uint32_t expected_crc,
                            uint32_t expected_size)
{
    unsigned char input[VHBU_IO_CHUNK];
    unsigned char output[VHBU_IO_CHUNK];
    uint32_t remaining = compressed_size;
    uint32_t written = 0;
    uLong crc = crc32(0L, Z_NULL, 0);
    z_stream stream;
    int stream_end = 0;
    int result;

    sceClibMemset(&stream, 0, sizeof(stream));
    stream.zalloc = vhbu_zalloc;
    stream.zfree = vhbu_zfree;
    result = inflateInit2(&stream, -MAX_WBITS);
    if (result != Z_OK)
        return -21;
    while (remaining != 0 && !stream_end) {
        unsigned int chunk = remaining < sizeof(input) ?
            remaining : (unsigned int)sizeof(input);
        if (read_exact(source, input, chunk) < 0) {
            result = -22;
            goto done;
        }
        remaining -= chunk;
        stream.next_in = input;
        stream.avail_in = chunk;
        while (stream.avail_in != 0 && !stream_end) {
            unsigned int produced;
            stream.next_out = output;
            stream.avail_out = sizeof(output);
            result = inflate(&stream, Z_NO_FLUSH);
            if (result != Z_OK && result != Z_STREAM_END) {
                result = -23;
                goto done;
            }
            produced = (unsigned int)sizeof(output) - stream.avail_out;
            if (produced != 0) {
                if (write_exact(destination, output, produced) < 0) {
                    result = -24;
                    goto done;
                }
                crc = crc32(crc, output, produced);
                written += produced;
            }
            if (result == Z_STREAM_END)
                stream_end = 1;
        }
    }
    result = stream_end && remaining == 0 && written == expected_size &&
             (uint32_t)crc == expected_crc ? 0 : -25;
done:
    (void)inflateEnd(&stream);
    return result;
}

int vhbu_extract_vpk(const char *source_path, const char *destination_root)
{
    unsigned char header[30];
    char relative[256];
    char output_path[VHBU_PATH_CAPACITY];
    unsigned int entry_count = 0;
    SceUID source;
    int result = 0;

    if (source_path == NULL || destination_root == NULL)
        return -1;
    (void)sceIoMkdir(destination_root, 0777);
    source = sceIoOpen(source_path, SCE_O_RDONLY, 0);
    if (source < 0)
        return source;
    for (;;) {
        uint32_t signature;
        uint16_t flags;
        uint16_t method;
        uint16_t name_length;
        uint16_t extra_length;
        uint32_t expected_crc;
        uint32_t compressed_size;
        uint32_t uncompressed_size;
        unsigned int root_length;
        unsigned int index;
        SceUID destination;

        result = read_exact(source, header, sizeof(header));
        if (result < 0)
            break;
        signature = read_u32(header);
        if (signature == VHBU_ZIP_CENTRAL_FILE || signature == VHBU_ZIP_END) {
            result = entry_count != 0 ? 0 : -2;
            break;
        }
        if (signature != VHBU_ZIP_LOCAL_FILE) {
            result = -3;
            break;
        }
        flags = read_u16(header + 6);
        method = read_u16(header + 8);
        expected_crc = read_u32(header + 14);
        compressed_size = read_u32(header + 18);
        uncompressed_size = read_u32(header + 22);
        name_length = read_u16(header + 26);
        extra_length = read_u16(header + 28);
        if ((flags & 0x0009u) != 0 || (method != 0 && method != 8) ||
            name_length == 0 || name_length >= sizeof(relative)) {
            result = -4;
            break;
        }
        result = read_exact(source, relative, name_length);
        if (result < 0)
            break;
        relative[name_length] = '\0';
        if (!safe_relative_path(relative)) {
            result = -5;
            break;
        }
        if (extra_length != 0 && sceIoLseek(source, extra_length,
                                             SCE_SEEK_CUR) < 0) {
            result = -6;
            break;
        }
        root_length = (unsigned int)sceClibStrnlen(destination_root,
                                                   sizeof(output_path));
        if (root_length + 1u + name_length + 1u > sizeof(output_path)) {
            result = -7;
            break;
        }
        for (index = 0; index < root_length; ++index)
            output_path[index] = destination_root[index];
        output_path[root_length] = '/';
        for (index = 0; index <= name_length; ++index)
            output_path[root_length + 1u + index] = relative[index];
        make_parent_directories(output_path);
        if (relative[name_length - 1u] == '/') {
            (void)sceIoMkdir(output_path, 0777);
            ++entry_count;
            continue;
        }
        destination = sceIoOpen(output_path,
            SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
        if (destination < 0) {
            result = destination;
            break;
        }
        result = method == 0 ?
            extract_stored(source, destination, compressed_size,
                           expected_crc, uncompressed_size) :
            extract_deflated(source, destination, compressed_size,
                             expected_crc, uncompressed_size);
        (void)sceIoClose(destination);
        if (result < 0)
            break;
        ++entry_count;
        if (entry_count > 4096u) {
            result = -8;
            break;
        }
    }
    (void)sceIoClose(source);
    return result;
}
