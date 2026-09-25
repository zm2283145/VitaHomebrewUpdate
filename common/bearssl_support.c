#include "bearssl_support.h"

#include <bearssl.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/rng.h>
#include <psp2/rtc.h>
#include <stdint.h>

#include "bearssl_trust_anchors.generated.c"

#define VHBU_HTTPS_RESPONSE_CAPACITY (96u * 1024u)
#define VHBU_HTTPS_REDIRECT_LIMIT 5
#define VHBU_HTTPS_TIMEOUT_US (15 * 1000 * 1000)

static br_ssl_client_context tls_client;
static br_x509_minimal_context tls_x509;
static br_sslio_context tls_io;
static unsigned char tls_buffer[BR_SSL_BUFSIZE_BIDI];
static unsigned char response_buffer[VHBU_HTTPS_RESPONSE_CAPACITY];
static const vhbu_net_ops *active_network;

static int ascii_equal_nocase(char left, char right)
{
    if (left >= 'A' && left <= 'Z')
        left = (char)(left + ('a' - 'A'));
    if (right >= 'A' && right <= 'Z')
        right = (char)(right + ('a' - 'A'));
    return left == right;
}

static int starts_with_nocase(const char *value, const char *prefix)
{
    while (*prefix != '\0') {
        if (*value == '\0' || !ascii_equal_nocase(*value, *prefix))
            return 0;
        ++value;
        ++prefix;
    }
    return 1;
}

static uint32_t days_before_year(unsigned int year)
{
    uint32_t y = year;
    return 365u * y + (y + 3u) / 4u - (y + 99u) / 100u +
           (y + 399u) / 400u;
}

static uint32_t days_before_month(unsigned int year, unsigned int month)
{
    static const uint16_t offsets[12] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    uint32_t days = offsets[month - 1u];
    if (month > 2u && ((year % 4u == 0u && year % 100u != 0u) ||
                      year % 400u == 0u))
        ++days;
    return days;
}

static void set_validation_time(br_x509_minimal_context *x509)
{
    SceDateTime now;
    uint32_t days;
    uint32_t seconds;

    if (sceRtcGetCurrentClock(&now, 0) < 0 || now.year < 2020 ||
        now.month < 1 || now.month > 12 || now.day < 1 || now.day > 31) {
        now.year = 2026;
        now.month = 9;
        now.day = 24;
        now.hour = 12;
        now.minute = 0;
        now.second = 0;
    }
    days = days_before_year(now.year) +
           days_before_month(now.year, now.month) + now.day - 1u;
    seconds = (uint32_t)now.hour * 3600u + (uint32_t)now.minute * 60u +
              now.second;
    br_x509_minimal_set_time(x509, days, seconds);
}

static int tls_read(void *context, unsigned char *buffer, size_t length)
{
    int socket = *(int *)context;
    int result = active_network->receive(socket, buffer,
                                         (unsigned int)length, 0);
    return result > 0 ? result : -1;
}

static int tls_write(void *context, const unsigned char *buffer,
                     size_t length)
{
    int socket = *(int *)context;
    int result = active_network->send_data(socket, buffer,
                                           (unsigned int)length, 0);
    return result > 0 ? result : -1;
}

static int parse_https_url(const char *url, char *host, size_t host_capacity,
                           char *path, size_t path_capacity)
{
    const char *cursor;
    size_t host_length = 0;
    size_t path_length;

    if (url == NULL || sceClibStrncmp(url, "https://", 8) != 0)
        return -1;
    cursor = url + 8;
    while (cursor[host_length] != '\0' && cursor[host_length] != '/')
        ++host_length;
    if (host_length == 0 || host_length >= host_capacity)
        return -2;
    sceClibMemcpy(host, cursor, host_length);
    host[host_length] = '\0';
    cursor += host_length;
    if (*cursor == '\0')
        cursor = "/";
    path_length = sceClibStrnlen(cursor, path_capacity);
    if (path_length >= path_capacity)
        return -3;
    sceClibMemcpy(path, cursor, path_length + 1u);
    return 0;
}

static int connect_host(const char *host, const vhbu_net_ops *network)
{
    SceNetSockaddrIn address;
    SceNetInAddr resolved;
    int resolver;
    int socket;
    int timeout = VHBU_HTTPS_TIMEOUT_US;
    int result;

    resolver = network->resolver_create("hbu_bearssl", NULL, 0);
    if (resolver < 0)
        return resolver;
    result = network->resolver_start_ntoa(resolver, host, &resolved,
                                          VHBU_HTTPS_TIMEOUT_US, 3, 0);
    (void)network->resolver_destroy(resolver);
    if (result < 0)
        return result;
    socket = network->socket_create("hbu_bearssl", SCE_NET_AF_INET,
                                    SCE_NET_SOCK_STREAM, 0);
    if (socket < 0)
        return socket;
    (void)network->set_option(socket, SCE_NET_SOL_SOCKET,
                              SCE_NET_SO_RCVTIMEO, &timeout,
                              sizeof(timeout));
    (void)network->set_option(socket, SCE_NET_SOL_SOCKET,
                              SCE_NET_SO_SNDTIMEO, &timeout,
                              sizeof(timeout));
    sceClibMemset(&address, 0, sizeof(address));
    address.sin_len = sizeof(address);
    address.sin_family = SCE_NET_AF_INET;
    address.sin_port = (uint16_t)((443u << 8) | (443u >> 8));
    address.sin_addr = resolved;
    result = network->connect_socket(socket, (SceNetSockaddr *)&address,
                                     sizeof(address));
    if (result < 0) {
        (void)network->close_socket(socket);
        return result;
    }
    return socket;
}

static int find_header_end(const unsigned char *response, size_t length,
                           size_t *header_size)
{
    size_t index;
    for (index = 3; index < length; ++index) {
        if (response[index - 3] == '\r' && response[index - 2] == '\n' &&
            response[index - 1] == '\r' && response[index] == '\n') {
            *header_size = index + 1u;
            return 0;
        }
    }
    return -1;
}

static int find_location(const unsigned char *response, size_t header_size,
                         char *location, size_t capacity)
{
    const char *cursor = (const char *)response;
    const char *end = cursor + header_size;
    while (cursor < end) {
        const char *line_end = cursor;
        size_t value_length;
        while (line_end + 1 < end &&
               !(line_end[0] == '\r' && line_end[1] == '\n'))
            ++line_end;
        if (starts_with_nocase(cursor, "Location:")) {
            cursor += 9;
            while (cursor < line_end && (*cursor == ' ' || *cursor == '\t'))
                ++cursor;
            value_length = (size_t)(line_end - cursor);
            if (value_length == 0 || value_length >= capacity)
                return -1;
            sceClibMemcpy(location, cursor, value_length);
            location[value_length] = '\0';
            return 0;
        }
        cursor = line_end + 2;
    }
    return -1;
}

static int fetch_once(const char *url, unsigned char *body, size_t capacity,
                      size_t *body_size, char *location,
                      size_t location_capacity, const vhbu_net_ops *network)
{
    char host[256];
    char path[1024];
    char request[1536];
    unsigned char entropy[32];
    size_t response_size = 0;
    size_t header_size;
    size_t payload_size;
    int socket;
    int length;
    int status;
    int result;

    result = parse_https_url(url, host, sizeof(host), path, sizeof(path));
    if (result < 0)
        return -100 + result;
    socket = connect_host(host, network);
    if (socket < 0)
        return socket;
    active_network = network;
    br_ssl_client_init_full(&tls_client, &tls_x509, TAs, TAs_NUM);
    set_validation_time(&tls_x509);
    br_ssl_engine_set_buffer(&tls_client.eng, tls_buffer,
                             sizeof(tls_buffer), 1);
    if (sceKernelGetRandomNumber(entropy, sizeof(entropy)) < 0) {
        (void)network->close_socket(socket);
        return -110;
    }
    br_ssl_engine_inject_entropy(&tls_client.eng, entropy, sizeof(entropy));
    if (!br_ssl_client_reset(&tls_client, host, 0)) {
        (void)network->close_socket(socket);
        return -111;
    }
    br_sslio_init(&tls_io, &tls_client.eng, tls_read, &socket,
                  tls_write, &socket);
    length = sceClibSnprintf(request, sizeof(request),
        "GET %s HTTP/1.0\r\nHost: %s\r\n"
        "User-Agent: VitaHomebrewUpdate/0.1 BearSSL/0.6\r\n"
        "Accept: application/xml,text/xml,*/*\r\nConnection: close\r\n\r\n",
        path, host);
    if (length <= 0 || length >= (int)sizeof(request) ||
        br_sslio_write_all(&tls_io, request, (size_t)length) < 0 ||
        br_sslio_flush(&tls_io) < 0) {
        (void)network->close_socket(socket);
        return -112;
    }
    while (response_size < sizeof(response_buffer)) {
        int received = br_sslio_read(&tls_io,
            response_buffer + response_size,
            sizeof(response_buffer) - response_size);
        if (received <= 0)
            break;
        response_size += (size_t)received;
    }
    (void)network->close_socket(socket);
    if (response_size < 12 ||
        sceClibStrncmp((const char *)response_buffer, "HTTP/1.", 7) != 0)
        return -113;
    status = (response_buffer[9] - '0') * 100 +
             (response_buffer[10] - '0') * 10 +
             (response_buffer[11] - '0');
    if (find_header_end(response_buffer, response_size, &header_size) < 0)
        return -114;
    if (status >= 300 && status < 400) {
        if (find_location(response_buffer, header_size, location,
                          location_capacity) < 0)
            return -115;
        return status;
    }
    if (status != 200)
        return -status;
    payload_size = response_size - header_size;
    if (payload_size == 0 || payload_size > capacity)
        return -116;
    sceClibMemcpy(body, response_buffer + header_size, payload_size);
    *body_size = payload_size;
    return 0;
}

int vhbu_https_get(const char *url, unsigned char *body, size_t capacity,
                   size_t *body_size, const vhbu_net_ops *network)
{
    char current[1024];
    char location[1024];
    size_t length;
    int redirect;
    int result;

    if (url == NULL || body == NULL || body_size == NULL || network == NULL)
        return -1;
    length = sceClibStrnlen(url, sizeof(current));
    if (length >= sizeof(current))
        return -2;
    sceClibMemcpy(current, url, length + 1u);
    for (redirect = 0; redirect <= VHBU_HTTPS_REDIRECT_LIMIT; ++redirect) {
        location[0] = '\0';
        result = fetch_once(current, body, capacity, body_size, location,
                            sizeof(location), network);
        if (result >= 300 && result < 400) {
            length = sceClibStrnlen(location, sizeof(current));
            if (length >= sizeof(current) ||
                sceClibStrncmp(location, "https://", 8) != 0)
                return -3;
            sceClibMemcpy(current, location, length + 1u);
            continue;
        }
        return result;
    }
    return -4;
}

int vhbu_https_stream(const char *url, unsigned long long skip_bytes,
                      vhbu_https_write_fn write_callback, void *context,
                      unsigned long long *written_size,
                      const vhbu_net_ops *network)
{
    char current[1024];
    char location[1024];
    char host[256];
    char path[1024];
    char request[1664];
    unsigned char entropy[32];
    unsigned char chunk[8192];
    size_t length;
    int redirect;

    if (url == NULL || write_callback == NULL || written_size == NULL ||
        network == NULL)
        return -1;
    length = sceClibStrnlen(url, sizeof(current));
    if (length >= sizeof(current))
        return -2;
    sceClibMemcpy(current, url, length + 1u);
    *written_size = 0;
    for (redirect = 0; redirect <= VHBU_HTTPS_REDIRECT_LIMIT; ++redirect) {
        size_t response_size = 0;
        size_t header_size;
        unsigned long long remaining_skip = skip_bytes;
        int socket;
        int request_length;
        int status;
        int result;

        result = parse_https_url(current, host, sizeof(host), path,
                                 sizeof(path));
        if (result < 0)
            return -120 + result;
        socket = connect_host(host, network);
        if (socket < 0)
            return socket;
        active_network = network;
        br_ssl_client_init_full(&tls_client, &tls_x509, TAs, TAs_NUM);
        set_validation_time(&tls_x509);
        br_ssl_engine_set_buffer(&tls_client.eng, tls_buffer,
                                 sizeof(tls_buffer), 1);
        if (sceKernelGetRandomNumber(entropy, sizeof(entropy)) < 0) {
            (void)network->close_socket(socket);
            return -130;
        }
        br_ssl_engine_inject_entropy(&tls_client.eng, entropy,
                                     sizeof(entropy));
        if (!br_ssl_client_reset(&tls_client, host, 0)) {
            (void)network->close_socket(socket);
            return -131;
        }
        br_sslio_init(&tls_io, &tls_client.eng, tls_read, &socket,
                      tls_write, &socket);
        if (skip_bytes != 0) {
            request_length = sceClibSnprintf(
                request, sizeof(request),
                "GET %s HTTP/1.0\r\nHost: %s\r\n"
                "User-Agent: VitaHomebrewUpdate/0.1 BearSSL/0.6\r\n"
                "Accept: */*\r\nRange: bytes=%llu-\r\n"
                "Connection: close\r\n\r\n",
                path, host, skip_bytes);
        } else {
            request_length = sceClibSnprintf(
                request, sizeof(request),
                "GET %s HTTP/1.0\r\nHost: %s\r\n"
                "User-Agent: VitaHomebrewUpdate/0.1 BearSSL/0.6\r\n"
                "Accept: */*\r\nConnection: close\r\n\r\n",
                path, host);
        }
        if (request_length <= 0 || request_length >= (int)sizeof(request) ||
            br_sslio_write_all(&tls_io, request,
                               (size_t)request_length) < 0 ||
            br_sslio_flush(&tls_io) < 0) {
            (void)network->close_socket(socket);
            return -132;
        }
        while (response_size < sizeof(response_buffer)) {
            int received = br_sslio_read(&tls_io,
                response_buffer + response_size,
                sizeof(response_buffer) - response_size);
            if (received <= 0)
                break;
            response_size += (size_t)received;
            if (find_header_end(response_buffer, response_size,
                                &header_size) == 0)
                break;
        }
        if (response_size < 12 ||
            find_header_end(response_buffer, response_size,
                            &header_size) < 0) {
            (void)network->close_socket(socket);
            return -133;
        }
        status = (response_buffer[9] - '0') * 100 +
                 (response_buffer[10] - '0') * 10 +
                 (response_buffer[11] - '0');
        if (status >= 300 && status < 400) {
            result = find_location(response_buffer, header_size, location,
                                   sizeof(location));
            (void)network->close_socket(socket);
            if (result < 0 ||
                sceClibStrncmp(location, "https://", 8) != 0)
                return -134;
            length = sceClibStrnlen(location, sizeof(current));
            if (length >= sizeof(current))
                return -135;
            sceClibMemcpy(current, location, length + 1u);
            continue;
        }
        if (status != 200 && status != 206) {
            (void)network->close_socket(socket);
            return -status;
        }
        if (status == 206)
            remaining_skip = 0;
        if (response_size > header_size) {
            const unsigned char *data = response_buffer + header_size;
            size_t size = response_size - header_size;
            if (remaining_skip >= size) {
                remaining_skip -= size;
            } else {
                data += (size_t)remaining_skip;
                size -= (size_t)remaining_skip;
                remaining_skip = 0;
                if (size != 0 && write_callback(context, data, size) < 0) {
                    (void)network->close_socket(socket);
                    return -136;
                }
                *written_size += size;
            }
        }
        for (;;) {
            int received = br_sslio_read(&tls_io, chunk, sizeof(chunk));
            const unsigned char *data = chunk;
            size_t size;
            if (received <= 0)
                break;
            size = (size_t)received;
            if (remaining_skip >= size) {
                remaining_skip -= size;
                continue;
            }
            data += (size_t)remaining_skip;
            size -= (size_t)remaining_skip;
            remaining_skip = 0;
            if (size != 0 && write_callback(context, data, size) < 0) {
                (void)network->close_socket(socket);
                return -137;
            }
            *written_size += size;
        }
        (void)network->close_socket(socket);
        return remaining_skip == 0 ? 0 : -138;
    }
    return -139;
}
