#include "bgdl.h"

#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <stdio.h>
#include <string.h>

#include "debugScreen.h"

#define printf psvDebugScreenPrintf
#define URL_PATH "ux0:data/LAUpdate/url.txt"

static void trim_line(char *text)
{
    size_t length = strlen(text);
    while (length > 0 && (text[length - 1] == '\r' || text[length - 1] == '\n' ||
                           text[length - 1] == ' ' || text[length - 1] == '\t'))
        text[--length] = '\0';
}

static void load_url(char *url, size_t capacity)
{
    SceUID fd;
    int read_count;
    snprintf(url, capacity, "%s", LAU_DEFAULT_URL);
    fd = sceIoOpen(URL_PATH, SCE_O_RDONLY, 0);
    if (fd < 0)
        return;
    read_count = sceIoRead(fd, url, capacity - 1);
    sceIoClose(fd);
    if (read_count <= 0) {
        snprintf(url, capacity, "%s", LAU_DEFAULT_URL);
        return;
    }
    url[read_count] = '\0';
    trim_line(url);
}

int main(void)
{
    SceCtrlData pad;
    char url[1024];
    int queued = 0;

    psvDebugScreenInit();
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    load_url(url, sizeof(url));

    printf("LiveArea Update - BGDL hardware probe\n\n");
    printf("URL:\n%s\n\n", url);
    printf("X: queue in native Notifications\n");
    printf("START: exit\n\n");
    printf("Phase 1 only tests the system queue.\n");
    printf("An arbitrary payload may fail install after download.\n");

    for (;;) {
        memset(&pad, 0, sizeof(pad));
        sceCtrlPeekBufferPositive(0, &pad, 1);
        if ((pad.buttons & SCE_CTRL_CROSS) && !queued) {
            lau_bgdl_result detail;
            int result;
            queued = 1;
            printf("\nQueuing...\n");
            sceIoMkdir("ux0:bgdl", 0777);
            result = lau_bgdl_enqueue(LAU_BGDL_GAME,
                                      "LiveArea Update Test Payload", url,
                                      "app0:sce_sys/icon0.png", &detail);
            printf("result=0x%08X api=0x%08X service=0x%08X task=%d\n",
                   (unsigned int)result, (unsigned int)detail.api_result,
                   (unsigned int)detail.service_result, detail.task_id);
            printf("Open Notifications to inspect the task.\n");
        }
        if (pad.buttons & SCE_CTRL_START)
            break;
        sceKernelDelayThread(16 * 1000);
    }
    sceKernelExitProcess(0);
    return 0;
}
