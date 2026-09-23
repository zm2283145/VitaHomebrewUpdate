#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/net/net.h>
#include <stdint.h>
#include <taihen.h>

#define VHBU_STATUS_PORT 13379
#define VHBU_REQUEST_CAPACITY 512
#define VHBU_LOG_DIRECTORY "ux0:data/VitaHomebrewUpdate"
#define VHBU_LOG_PATH VHBU_LOG_DIRECTORY "/service.log"
#define VHBU_NET_RETRY_COUNT 120
#define VHBU_NET_RETRY_DELAY_US (500u * 1000u)
#define SCE_NET_LIBRARY_NID 0x6BF8B2A2u

typedef int (*net_socket_fn)(const char *, int, int, int);
typedef int (*net_accept_fn)(int, SceNetSockaddr *, unsigned int *);
typedef int (*net_bind_fn)(int, const SceNetSockaddr *, unsigned int);
typedef int (*net_listen_fn)(int, int);
typedef int (*net_recv_fn)(int, void *, unsigned int, int);
typedef int (*net_send_fn)(int, const void *, unsigned int, int);
typedef int (*net_setsockopt_fn)(int, int, int, const void *, unsigned int);
typedef int (*net_shutdown_fn)(int, int);
typedef int (*net_socket_abort_fn)(int, int);
typedef int (*net_socket_close_fn)(int);

static net_socket_fn net_socket;
static net_accept_fn net_accept;
static net_bind_fn net_bind;
static net_listen_fn net_listen;
static net_recv_fn net_recv;
static net_send_fn net_send;
static net_setsockopt_fn net_setsockopt;
static net_shutdown_fn net_shutdown;
static net_socket_abort_fn net_socket_abort;
static net_socket_close_fn net_socket_close;

static volatile int service_running;
static volatile int published_status = 1;
static SceUID service_thread = -1;
static int listen_socket = -1;

static uint16_t host_to_network16(uint16_t value)
{
    return (uint16_t)((value << 8) | (value >> 8));
}

static uint32_t host_to_network32(uint32_t value)
{
    return ((value & 0x000000ffu) << 24) |
           ((value & 0x0000ff00u) << 8) |
           ((value & 0x00ff0000u) >> 8) |
           ((value & 0xff000000u) >> 24);
}

static int resolve_network(void)
{
#define RESOLVE_NET(target, nid)                                                \
    do {                                                                        \
        int resolve_result = taiGetModuleExportFunc(                            \
            "SceNet", SCE_NET_LIBRARY_NID, (nid), (uintptr_t *)&(target));      \
        if (resolve_result < 0)                                                  \
            return resolve_result;                                              \
    } while (0)

    RESOLVE_NET(net_accept, 0x1ADF9BB1u);
    RESOLVE_NET(net_bind, 0x1296A94Bu);
    RESOLVE_NET(net_listen, 0x7A8DA094u);
    RESOLVE_NET(net_recv, 0x023643B7u);
    RESOLVE_NET(net_send, 0xE3DD8CD9u);
    RESOLVE_NET(net_setsockopt, 0x065505CAu);
    RESOLVE_NET(net_shutdown, 0x69E50BB5u);
    RESOLVE_NET(net_socket, 0xF084FCE3u);
    RESOLVE_NET(net_socket_abort, 0x891C1B9Bu);
    RESOLVE_NET(net_socket_close, 0x29822B4Du);
#undef RESOLVE_NET
    return 0;
}

static void log_event(const char *event, int result)
{
    char line[160];
    SceUID file;
    int length;

    (void)sceIoMkdir(VHBU_LOG_DIRECTORY, 0777);
    length = sceClibSnprintf(line, sizeof(line),
                             "%s result=%d hex=0x%08X\n", event, result,
                             (unsigned int)result);
    if (length <= 0 || length >= (int)sizeof(line))
        return;
    file = sceIoOpen(VHBU_LOG_PATH,
                     SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0666);
    if (file >= 0) {
        (void)sceIoWrite(file, line, (SceSize)length);
        (void)sceIoClose(file);
    }
}

static int send_all(int socket, const char *data, int length)
{
    int sent = 0;
    while (sent < length) {
        int result = net_send(socket, data + sent,
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

    (void)net_setsockopt(client, SCE_NET_SOL_SOCKET, SCE_NET_SO_RCVTIMEO,
                         &timeout, sizeof(timeout));
    (void)net_setsockopt(client, SCE_NET_SOL_SOCKET, SCE_NET_SO_SNDTIMEO,
                         &timeout, sizeof(timeout));
    received = net_recv(client, request, sizeof(request) - 1u, 0);
    if (received <= 0)
        return;
    request[received] = '\0';
    if (sceClibStrncmp(request, "GET /status ", 12) != 0) {
        (void)send_all(client, not_found, (int)sizeof(not_found) - 1);
        return;
    }

    length = sceClibSnprintf(response, sizeof(response),
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
    int attempt;
    int result;
    (void)argument_size;
    (void)arguments;

    log_event("server_thread_started", 0);
    for (attempt = 0; service_running && attempt < VHBU_NET_RETRY_COUNT;
         ++attempt) {
        result = resolve_network();
        if (result < 0) {
            if (attempt == 0 || attempt == VHBU_NET_RETRY_COUNT - 1)
                log_event("net_resolve_retry", result);
            sceKernelDelayThread(VHBU_NET_RETRY_DELAY_US);
            continue;
        }
        listen_socket = net_socket("vhbu_status_server", SCE_NET_AF_INET,
                                   SCE_NET_SOCK_STREAM, 0);
        if (listen_socket >= 0)
            break;
        if (attempt == 0 || attempt == VHBU_NET_RETRY_COUNT - 1)
            log_event("socket_retry", listen_socket);
        sceKernelDelayThread(VHBU_NET_RETRY_DELAY_US);
    }
    if (listen_socket < 0) {
        log_event("socket_failed", listen_socket);
        return 0;
    }
    log_event("socket_ready", attempt);
    (void)net_setsockopt(listen_socket, SCE_NET_SOL_SOCKET,
                         SCE_NET_SO_REUSEADDR, &reuse, sizeof(reuse));
    sceClibMemset(&address, 0, sizeof(address));
    address.sin_len = sizeof(address);
    address.sin_family = SCE_NET_AF_INET;
    address.sin_port = host_to_network16(VHBU_STATUS_PORT);
    address.sin_addr.s_addr = host_to_network32(SCE_NET_INADDR_LOOPBACK);
    result = net_bind(listen_socket, (SceNetSockaddr *)&address,
                      sizeof(address));
    if (result < 0) {
        log_event("bind_failed", result);
        net_socket_close(listen_socket);
        listen_socket = -1;
        return 0;
    }
    result = net_listen(listen_socket, 2);
    if (result < 0) {
        log_event("listen_failed", result);
        net_socket_close(listen_socket);
        listen_socket = -1;
        return 0;
    }
    log_event("service_ready", published_status);

    while (service_running) {
        int client = net_accept(listen_socket, NULL, NULL);
        if (client < 0) {
            if (!service_running)
                break;
            sceKernelDelayThread(10u * 1000u);
            continue;
        }
        serve_client(client);
        (void)net_shutdown(client, SCE_NET_SHUT_RDWR);
        (void)net_socket_close(client);
    }
    if (listen_socket >= 0) {
        net_socket_close(listen_socket);
        listen_socket = -1;
    }
    log_event("server_thread_stopped", 0);
    return 0;
}

int module_start(SceSize argc, const void *args)
{
    int result;
    (void)argc;
    (void)args;

    service_running = 1;
    log_event("module_start", 0);
    /* Report hook error until the firmware-gated LiveArea hook is proven. */
    published_status = 1;
    service_thread = sceKernelCreateThread("vhbu_status_server",
                                           status_server_thread,
                                           0x10000100, 0x10000, 0, 0, NULL);
    if (service_thread < 0) {
        log_event("thread_create_failed", service_thread);
        return SCE_KERNEL_START_FAILED;
    }
    result = sceKernelStartThread(service_thread, 0, NULL);
    if (result < 0) {
        log_event("thread_start_failed", result);
        sceKernelDeleteThread(service_thread);
        service_thread = -1;
        return SCE_KERNEL_START_FAILED;
    }
    log_event("thread_started", service_thread);
    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
    (void)argc;
    (void)args;
    service_running = 0;
    log_event("module_stop", 0);
    if (listen_socket >= 0 && net_socket_abort != NULL)
        (void)net_socket_abort(listen_socket, 0);
    if (service_thread >= 0) {
        (void)sceKernelWaitThreadEnd(service_thread, NULL, NULL);
        (void)sceKernelDeleteThread(service_thread);
        service_thread = -1;
    }
    return SCE_KERNEL_STOP_SUCCESS;
}
