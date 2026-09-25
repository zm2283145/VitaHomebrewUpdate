#ifndef VHBU_FILE_VERIFY_H
#define VHBU_FILE_VERIFY_H

#include <stdint.h>

int vhbu_verify_file_size_sha1(const char *path, uint64_t expected_size,
                               const char *expected_sha1);

#endif
