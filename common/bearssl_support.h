#ifndef VHBU_BEARSSL_SUPPORT_H
#define VHBU_BEARSSL_SUPPORT_H

#include <psp2/net/net.h>
#include <stddef.h>

typedef struct vhbu_net_ops {
    int (*socket_create)(const char *, int, int, int);
    int (*connect_socket)(int, const SceNetSockaddr *, unsigned int);
    int (*receive)(int, void *, unsigned int, int);
    int (*send_data)(int, const void *, unsigned int, int);
    int (*set_option)(int, int, int, const void *, unsigned int);
    int (*close_socket)(int);
    int (*resolver_create)(const char *, SceNetResolverParam *, int);
    int (*resolver_start_ntoa)(int, const char *, SceNetInAddr *, int, int,
                               int);
    int (*resolver_destroy)(int);
} vhbu_net_ops;

int vhbu_https_get(const char *url, unsigned char *body, size_t capacity,
                   size_t *body_size, const vhbu_net_ops *network);

typedef int (*vhbu_https_write_fn)(void *context,
                                   const unsigned char *data, size_t size);

int vhbu_https_stream(const char *url, unsigned long long skip_bytes,
                      vhbu_https_write_fn write_callback, void *context,
                      unsigned long long *written_size,
                      const vhbu_net_ops *network);

#endif
