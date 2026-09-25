#include "file_verify.h"
#include "sha1.h"

#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <stdint.h>

static int hex_value(char value)
{
    if (value >= '0' && value <= '9')
        return value - '0';
    if (value >= 'a' && value <= 'f')
        return value - 'a' + 10;
    if (value >= 'A' && value <= 'F')
        return value - 'A' + 10;
    return -1;
}

int vhbu_verify_file_size_sha1(const char *path, uint64_t expected_size,
                               const char *expected_sha1)
{
    unsigned char buffer[16u * 1024u];
    unsigned char digest[20];
    VhbuSha1Context context;
    SceIoStat status;
    SceUID file;
    int result;
    unsigned int index;

    if (path == NULL || expected_sha1 == NULL)
        return -1;
    for (index = 0; index < 40u; ++index) {
        if (expected_sha1[index] == '\0' || hex_value(expected_sha1[index]) < 0)
            return -2;
    }
    if (expected_sha1[40] != '\0')
        return -2;
    result = sceIoGetstat(path, &status);
    if (result < 0)
        return result;
    if ((uint64_t)status.st_size != expected_size)
        return -3;

    file = sceIoOpen(path, SCE_O_RDONLY, 0);
    if (file < 0)
        return file;
    vhbu_sha1_begin(&context);
    result = 0;
    while (result == 0) {
        int received = sceIoRead(file, buffer, sizeof(buffer));
        if (received < 0) {
            result = received;
            break;
        }
        if (received == 0)
            break;
        vhbu_sha1_add(&context, buffer, (unsigned int)received);
    }
    (void)sceIoClose(file);
    if (result == 0)
        vhbu_sha1_end(&context, digest);
    if (result != 0)
        return result;
    for (index = 0; index < sizeof(digest); ++index) {
        if ((unsigned char)((hex_value(expected_sha1[index * 2u]) << 4) |
                            hex_value(expected_sha1[index * 2u + 1u])) !=
            digest[index])
            return -4;
    }
    return 0;
}
