#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/net/net.h>
#include <psp2/sysmodule.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debugScreen.h"
#include "homebrew_update_client.h"

#define printf psvDebugScreenPrintf
#define NET_MEMORY_SIZE (1024 * 1024)

static const char *status_name(int status)
{
    switch (status) {
    case HOMEBREW_UPDATE_DISABLED:
        return "installed, disabled";
    case HOMEBREW_UPDATE_HOOK_ERROR:
        return "installed, hooks unavailable (use fallback)";
    case HOMEBREW_UPDATE_READY:
        return "ready (use plugin update path)";
    case HOMEBREW_UPDATE_NOT_DETECTED:
    default:
        return "not detected (use fallback)";
    }
}

int main(void)
{
    SceCtrlData pad;
    SceNetInitParam net_init;
    void *net_memory = NULL;
    int net_module_loaded = 0;
    int net_initialized = 0;
    int update_status = HOMEBREW_UPDATE_NOT_DETECTED;

    psvDebugScreenInit();
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

    if (sceSysmoduleLoadModule(SCE_SYSMODULE_NET) >= 0) {
        net_module_loaded = 1;
        net_memory = malloc(NET_MEMORY_SIZE);
        if (net_memory != NULL) {
            memset(&net_init, 0, sizeof(net_init));
            net_init.memory = net_memory;
            net_init.size = NET_MEMORY_SIZE;
            if (sceNetInit(&net_init) >= 0) {
                net_initialized = 1;
                update_status = HomebrewUpdateClientGetStatus();
            }
        }
    }

    printf("LiveArea Update disposable target\n\n");
    printf("Installed version: %s\n\n", LAU_TEST_APP_VERSION);
    printf("Update service: %d\n", update_status);
    printf("%s\n\n", status_name(update_status));
    printf("Build 01.00 and 01.01 to prove replacement.\n");
    printf("Press START to exit.\n");
    for (;;) {
        memset(&pad, 0, sizeof(pad));
        sceCtrlPeekBufferPositive(0, &pad, 1);
        if (pad.buttons & SCE_CTRL_START)
            break;
        sceKernelDelayThread(16 * 1000);
    }
    if (net_initialized)
        sceNetTerm();
    free(net_memory);
    if (net_module_loaded)
        sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
    sceKernelExitProcess(0);
    return 0;
}
