#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/modulemgr.h>
#include <stdio.h>
#include <string.h>
#include <taihen.h>

#define LOG_DIR "ux0:data/LAUpdate"
#define LOG_PATH LOG_DIR "/shell-probe.log"

static void append_log(const char *event)
{
    tai_module_info_t info;
    SceUID fd;
    char line[256];
    int length;
    int result;

    sceIoMkdir(LOG_DIR, 0777);
    memset(&info, 0, sizeof(info));
    info.size = sizeof(info);
    result = taiGetModuleInfo("SceShell", &info);
    length = snprintf(line, sizeof(line),
                      "%s tai=%08X modid=%08X nid=%08X name=%s\n",
                      event, (unsigned int)result, (unsigned int)info.modid,
                      (unsigned int)info.module_nid,
                      result >= 0 ? info.name : "unavailable");
    if (length <= 0 || (size_t)length >= sizeof(line))
        return;
    fd = sceIoOpen(LOG_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0666);
    if (fd >= 0) {
        sceIoWrite(fd, line, (SceSize)length);
        sceIoClose(fd);
    }
}

int module_start(SceSize argc, const void *args)
{
    (void)argc;
    (void)args;
    append_log("start");
    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
    (void)argc;
    (void)args;
    append_log("stop");
    return SCE_KERNEL_STOP_SUCCESS;
}

/* Kept only to satisfy VitaSDK's normal executable link step. The SUPRX export
 * configuration selects module_start/module_stop, so SceShell never calls it. */
int main(void)
{
    return 0;
}
