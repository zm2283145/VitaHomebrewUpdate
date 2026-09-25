/*
 * Native Vita background-download adapter.
 *
 * Derived from PKGj src/bgdl.cpp, reverse engineered by dots_tb with help from
 * CelesteBlue123, SilicaDevs, possvkey, and the NPS team.
 * PKGj is distributed under the BSD-2-Clause license; see THIRD_PARTY.md.
 */
#include "bgdl.h"

#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <stdint.h>
#include <taihen.h>

typedef struct lau_ipmi_download_param {
    int type[2];
    char reserved_08[0x68];
    char url[0x800];
    char icon_path[0x100];
    char title[0x33a];
    char license_path[0x100];
    char reserved_daa[0x16];
} lau_ipmi_download_param;

typedef struct lau_sce_download_param {
    union {
        struct {
            uint32_t *ptr_to_dc0_ptr;
            uint32_t *ptr_to_2e0_ptr;
            uint32_t unk_1;
            uint32_t unk_2;
            uint32_t unk_3;
            lau_ipmi_download_param *addr_dc0;
            uint32_t size_dc0;
        } init;
        struct {
            int32_t *result;
            uint32_t unk_2;
            uint32_t unk_3;
            uint32_t unk_4;
            uint32_t unk_5;
            uint32_t unk_6;
            uint32_t unk_7;
        } state;
    } u;
    void *addr_2e0;
    uint32_t size_2e0;
    uint32_t unk_4;
    uint32_t *task_id;
    uint32_t unk_5;
    int32_t *result;
    uint32_t unk_4_2;
    uint32_t shell_func_8;
} lau_sce_download_param;

typedef struct lau_shellsvc_init {
    uint32_t unk_0;
    char name[0x10];
    void *unk_ptr;
    uint32_t unk_1;
    uint32_t size_1;
    uint32_t size_2;
    uint32_t unk_2;
    uint32_t unk_3;
    uint32_t unk_4;
    uint32_t unk_5;
    char padding[0x84];
    uint32_t unk_7;
    uint32_t unk_8;
    void *unk_ptr_2;
    char padding_2[0x88];
} lau_shellsvc_init;

typedef struct lau_download_class_header {
    uint32_t unk_0;
    uint32_t unk_1;
    uint32_t unk_2;
    uint32_t **functions;
    uint32_t unk_3;
    uint32_t *buffer_c4;
    uint32_t *buffer_1000;
} lau_download_class_header;

typedef int (*lau_download_init_fn)(uint32_t **, void *, int,
                                    lau_shellsvc_init *, int);
typedef int (*lau_download_change_state_fn)(uint32_t **, int, void *, int,
                                            lau_sce_download_param);
typedef int (*lau_ipmi_create_fn)(const char *, int);
typedef int (*lau_ipmi_init_fn)(uint32_t ***, const char *,
                                lau_download_class_header *, uint32_t *);

static int copy_text(char *dst, size_t capacity, const char *src)
{
    size_t length;
    if (dst == NULL || src == NULL || capacity == 0)
        return -1;
    length = sceClibStrnlen(src, capacity);
    if (length >= capacity)
        return -2;
    sceClibMemcpy(dst, src, length + 1);
    return 0;
}

static int lau_bgdl_enqueue_locked(int type, const char *title,
                                   const char *url, const char *icon_path,
                                   lau_bgdl_result *out)
{
    static const char shellsvc_path[] = "vs0:sys/external/libshellsvc.suprx";
    static lau_ipmi_create_fn ipmi_create;
    static lau_ipmi_init_fn ipmi_init;
    static lau_download_class_header header;
    static lau_shellsvc_init init_header;
    static lau_sce_download_param params;
    static lau_ipmi_download_param dc0;
    static uint8_t buffer_2e0[0x2e0];
    static uint32_t buffer_c4[0xc4 / sizeof(uint32_t)];
    static uint32_t buffer_1000[0x1000 / sizeof(uint32_t)];
    static lau_download_init_fn download_init;
    static lau_download_change_state_fn change_state;
    static int32_t service_result;
    /* Match PKGj: SceDownload expects this field seeded with its first ID. */
    static int32_t task_id;
    static int initialized;
    int result;

    if (out != NULL) {
        out->api_result = -1;
        out->service_result = 0;
        out->task_id = -1;
    }
    if (title == NULL || url == NULL)
        return -1;

    if (!initialized) {
        /* HomebrewUpdate and PKGj both retain one initialized SceDownload
         * class for their full process lifetime.  Its function table points
         * into these backing buffers, so they must never live on the stack or
         * be cleared between registrations. */
        sceKernelLoadStartModule(shellsvc_path, 0, NULL, 0, NULL, NULL);
        result = taiGetModuleExportFunc("SceShellSvc", 0xf4e34edb,
                                        0x4e255c31,
                                        (uintptr_t *)&ipmi_create);
        if (result < 0)
            return result;
        result = taiGetModuleExportFunc("SceShellSvc", 0xf4e34edb,
                                        0xb282b430,
                                        (uintptr_t *)&ipmi_init);
        if (result < 0)
            return result;

        sceClibMemset(&header, 0, sizeof(header));
        sceClibMemset(&init_header, 0, sizeof(init_header));
        sceClibMemset(buffer_c4, 0, sizeof(buffer_c4));
        sceClibMemset(buffer_1000, 0, sizeof(buffer_1000));

        copy_text(init_header.name, sizeof(init_header.name), "SceDownload");
        init_header.unk_1 = 1;
        init_header.size_1 = 0x1e00;
        init_header.size_2 = 0x1e00;
        init_header.unk_2 = 1;
        init_header.unk_3 = 0x0f00;
        init_header.unk_4 = 0x0f00;
        init_header.unk_5 = 1;
        init_header.unk_7 = 2;
        init_header.unk_8 = (uint32_t)-1;

        result = ipmi_create(init_header.name, 0x1e00);
        if (result != 0xc4)
            return result < 0 ? result : -3;

        header.buffer_c4 = buffer_c4;
        header.buffer_1000 = buffer_1000;
        result = ipmi_init(&header.functions, init_header.name, &header,
                           header.buffer_1000);
        if (result < 0)
            return result;
        if (header.functions == NULL || *header.functions == NULL)
            return -4;

        download_init = (lau_download_init_fn)(*header.functions)[1];
        change_state = (lau_download_change_state_fn)(*header.functions)[5];
        if (download_init == NULL || change_state == NULL)
            return -5;
        result = download_init(header.functions, *header.functions, 0x14,
                               &init_header, 2);
        if (result < 0)
            return result;
        initialized = 1;
    }

    sceClibMemset(&params, 0, sizeof(params));
    sceClibMemset(&dc0, 0, sizeof(dc0));
    sceClibMemset(buffer_2e0, 0, sizeof(buffer_2e0));
    service_result = 0;
    task_id = 1;

    if (copy_text(dc0.url, sizeof(dc0.url), url) < 0 ||
        copy_text(dc0.title, sizeof(dc0.title), title) < 0 ||
        (icon_path != NULL &&
         copy_text(dc0.icon_path, sizeof(dc0.icon_path), icon_path) < 0))
        return -2;
    dc0.type[0] = type;
    dc0.type[1] = type;

    params.u.init.ptr_to_dc0_ptr = (uint32_t *)&params.u.init.addr_dc0;
    params.u.init.ptr_to_2e0_ptr = (uint32_t *)&params.addr_2e0;
    params.u.init.unk_1 = 2;
    params.u.init.unk_2 = (uint32_t)-1;
    params.u.init.addr_dc0 = &dc0;
    params.u.init.size_dc0 = sizeof(dc0);
    params.addr_2e0 = buffer_2e0;
    params.size_2e0 = sizeof(buffer_2e0);
    params.task_id = (uint32_t *)&task_id;
    params.unk_5 = 4;
    params.result = &service_result;
    params.shell_func_8 = (*header.functions)[8];

    result = change_state(header.functions, 0x12340012,
                          params.u.init.ptr_to_dc0_ptr, 1, params);
    if (out != NULL) {
        out->api_result = result;
        out->service_result = service_result;
        out->task_id = task_id;
    }
    if (result < 0)
        return result;
    if (service_result < 0)
        return service_result;
    /* PKGj accepts zero here; only a negative ID means registration failed. */
    if (task_id < 0)
        return -6;

    sceClibMemset(&params, 0, sizeof(params));
    service_result = 0;
    params.u.state.result = &service_result;
    params.u.state.unk_4 = 1;
    params.u.state.unk_7 = 0x00000a0a;
    result = change_state(header.functions, 0x12340007, NULL, 0, params);
    if (out != NULL) {
        out->api_result = result;
        out->service_result = service_result;
        out->task_id = task_id;
    }
    if (result < 0)
        return result;
    if (service_result < 0)
        return service_result;
    return 0;
}

int lau_bgdl_enqueue(int type, const char *title, const char *url,
                     const char *icon_path, lau_bgdl_result *out)
{
    static volatile int busy;
    int result;

    if (__sync_lock_test_and_set(&busy, 1) != 0)
        return -7;
    result = lau_bgdl_enqueue_locked(type, title, url, icon_path, out);
    __sync_lock_release(&busy);
    return result;
}
