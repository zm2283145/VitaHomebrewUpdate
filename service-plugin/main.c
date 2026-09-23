#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/net/net.h>
#include <stdio.h>
#include <string.h>

#define VHBU_STATUS_PORT 13379
#define VHBU_REQUEST_CAPACITY 512

static volatile int service_running;
static volatile int published_status = 1;
static SceUID service_thread = -1;
static int listen_socket = -1;

static int send_all(int socket, const char *data, int length)
{
    int sent = 0;
    while (sent < length) {
        int result = sceNetSend(socket, data + sent,
                                (unsigned int)(length - sent), 0);
        if (result <= 0)
            return -1;
        sent += result;
    }
    return 0;
}

static void serve_client(int client)
{
    static const char not_found[] =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n\r\n";
    char request[VHBU_REQUEST_CAPACITY];
    char response[192];
    int received;
    int length;
    int timeout = 250000;

    (void)sceNetSetsockopt(client, SCE_NET_SOL_SOCKET, SCE_NET_SO_RCVTIMEO,
                           &timeout, sizeof(timeout));
    (void)sceNetSetsockopt(client, SCE_NET_SOL_SOCKET, SCE_NET_SO_SNDTIMEO,
                           &timeout, sizeof(timeout));
    received = sceNetRecv(client, request, sizeof(request) - 1u, 0);
    if (received <= 0)
        return;
    request[received] = '\0';
    if (strncmp(request, "GET /status ", 12) != 0) {
        (void)send_all(client, not_found, (int)sizeof(not_found) - 1);
        return;
    }

    length = snprintf(response, sizeof(response),
                      "HTTP/1.1 200 OK\r\n"
                      "Content-Type: application/json\r\n"
                      "Cache-Control: no-store\r\n"
                      "Connection: close\r\n"
                      "Content-Length: 12\r\n\r\n"
                      "{\"status\":%d}",
                      (int)published_status);
    if (length > 0 && length < (int)sizeof(response))
        (void)send_all(client, response, length);
}

static int status_server_thread(SceSize argument_size, void *arguments)
{
    SceNetSockaddrIn address;
    int reuse = 1;
    (void)argument_size;
    (void)arguments;

    listen_socket = sceNetSocket("vhbu_status_server", SCE_NET_AF_INET,
                                 SCE_NET_SOCK_STREAM, 0);
    if (listen_socket < 0)
        return 0;
    (void)sceNetSetsockopt(listen_socket, SCE_NET_SOL_SOCKET,
                           SCE_NET_SO_REUSEADDR, &reuse, sizeof(reuse));
    memset(&address, 0, sizeof(address));
    address.sin_len = sizeof(address);
    address.sin_family = SCE_NET_AF_INET;
    address.sin_port = sceNetHtons(VHBU_STATUS_PORT);
    address.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_LOOPBACK);
    if (sceNetBind(listen_socket, (SceNetSockaddr *)&address,
                   sizeof(address)) < 0 ||
        sceNetListen(listen_socket, 2) < 0) {
        sceNetSocketClose(listen_socket);
        listen_socket = -1;
        return 0;
    }

    while (service_running) {
        int client = sceNetAccept(listen_socket, NULL, NULL);
        if (client < 0) {
            if (!service_running)
                break;
            sceKernelDelayThread(10u * 1000u);
            continue;
        }
        serve_client(client);
        (void)sceNetShutdown(client, SCE_NET_SHUT_RDWR);
        (void)sceNetSocketClose(client);
    }
    if (listen_socket >= 0) {
        sceNetSocketClose(listen_socket);
        listen_socket = -1;
    }
    return 0;
}

int module_start(SceSize argc, const void *args)
{
    int result;
    (void)argc;
    (void)args;

    service_running = 1;
    /* Report hook error until the firmware-gated LiveArea hook is proven. */
    published_status = 1;
    service_thread = sceKernelCreateThread("vhbu_status_server",
                                           status_server_thread,
                                           0x10000100, 0x10000, 0, 0, NULL);
    if (service_thread < 0)
        return SCE_KERNEL_START_FAILED;
    result = sceKernelStartThread(service_thread, 0, NULL);
    if (result < 0) {
        sceKernelDeleteThread(service_thread);
        service_thread = -1;
        return SCE_KERNEL_START_FAILED;
    }
    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
    (void)argc;
    (void)args;
    service_running = 0;
    if (listen_socket >= 0)
        (void)sceNetSocketAbort(listen_socket, 0);
    if (service_thread >= 0) {
        (void)sceKernelWaitThreadEnd(service_thread, NULL, NULL);
        (void)sceKernelDeleteThread(service_thread);
        service_thread = -1;
    }
    return SCE_KERNEL_STOP_SUCCESS;
}

int main(void)
{
    return 0;
}
