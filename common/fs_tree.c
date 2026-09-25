#include "fs_tree.h"

#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/clib.h>

#define VHBU_STAGE_PREFIX "ux0:data/VitaHomebrewUpdate/stage-"
#define VHBU_TREE_PATH_CAPACITY 384u
#define VHBU_TREE_DEPTH_LIMIT 16u

static int remove_tree_inner(const char *path, unsigned int depth)
{
    SceIoDirent entry;
    SceUID directory;
    int result;

    if (depth > VHBU_TREE_DEPTH_LIMIT)
        return -10;
    directory = sceIoDopen(path);
    if (directory < 0)
        return directory;
    for (;;) {
        char child[VHBU_TREE_PATH_CAPACITY];
        sceClibMemset(&entry, 0, sizeof(entry));
        result = sceIoDread(directory, &entry);
        if (result <= 0)
            break;
        if ((entry.d_name[0] == '.' && entry.d_name[1] == '\0') ||
            (entry.d_name[0] == '.' && entry.d_name[1] == '.' &&
             entry.d_name[2] == '\0'))
            continue;
        result = sceClibSnprintf(child, sizeof(child), "%s/%s",
                                 path, entry.d_name);
        if (result <= 0 || result >= (int)sizeof(child)) {
            result = -11;
            break;
        }
        result = SCE_S_ISDIR(entry.d_stat.st_mode) ?
            remove_tree_inner(child, depth + 1u) : sceIoRemove(child);
        if (result < 0)
            break;
    }
    (void)sceIoDclose(directory);
    if (result < 0)
        return result;
    return sceIoRmdir(path);
}

int vhbu_remove_stage_tree(const char *path)
{
    SceIoStat status;
    unsigned int prefix_length = sizeof(VHBU_STAGE_PREFIX) - 1u;
    unsigned int length;

    if (path == NULL ||
        sceClibStrncmp(path, VHBU_STAGE_PREFIX, prefix_length) != 0)
        return -1;
    length = (unsigned int)sceClibStrnlen(path, VHBU_TREE_PATH_CAPACITY);
    if (length <= prefix_length || length >= VHBU_TREE_PATH_CAPACITY ||
        path[length - 1u] == '/')
        return -2;
    if (sceIoGetstat(path, &status) < 0)
        return 0;
    if (!SCE_S_ISDIR(status.st_mode))
        return -3;
    return remove_tree_inner(path, 0);
}

int vhbu_remove_bgdl_task_tree(int task_id)
{
    char path[32];
    SceIoStat status;
    int result;

    if (task_id <= 0)
        return -1;
    result = sceClibSnprintf(path, sizeof(path),
                             "ux0:bgdl/t/%08x", task_id);
    if (result <= 0 || result >= (int)sizeof(path))
        return -2;
    if (sceIoGetstat(path, &status) < 0)
        return 0;
    if (!SCE_S_ISDIR(status.st_mode))
        return -3;
    return remove_tree_inner(path, 0);
}
