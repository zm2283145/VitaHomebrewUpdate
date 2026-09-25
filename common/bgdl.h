#ifndef LAU_BGDL_H
#define LAU_BGDL_H

#include <stdint.h>

enum lau_bgdl_type {
    LAU_BGDL_PSP = 0x00,
    /* HomebrewUpdate uses the generic task class for VPK payloads. */
    LAU_BGDL_VPK = 0x01,
    LAU_BGDL_PSM = 0x06,
    LAU_BGDL_THEME = 0x0c,
    LAU_BGDL_GAME = 0x16,
    LAU_BGDL_DLC = 0x17
};

typedef struct lau_bgdl_result {
    int api_result;
    int service_result;
    int task_id;
} lau_bgdl_result;

/*
 * Queue one task in the native SceDownload service. This prototype intentionally
 * accepts no license: phase one only establishes that a PC-hosted object reaches
 * the system notification queue. A completed arbitrary payload is expected to
 * fail installation until the package handoff is implemented.
 */
int lau_bgdl_enqueue(int type, const char *title, const char *url,
                     const char *icon_path, lau_bgdl_result *out);

#endif
