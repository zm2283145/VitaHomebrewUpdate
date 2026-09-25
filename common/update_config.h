#ifndef VHBU_UPDATE_CONFIG_H
#define VHBU_UPDATE_CONFIG_H

#include <stddef.h>
#include <stdint.h>

typedef struct VhbuUpdateConfig {
    char title_id[12];
    char name[128];
    char update_url[512];
    char installed_version[16];
    char available_version[16];
    uint64_t package_size;
    char package_sha1[41];
    char package_url[1024];
    char changeinfo_url[1024];
    char content_id[64];
    char package_name[160];
    char icon_path[192];
    char pending_path[192];
    char stage_path[192];
} VhbuUpdateConfig;

void vhbu_update_config_clear(VhbuUpdateConfig *config);
int vhbu_update_config_parse_ini(VhbuUpdateConfig *config,
                                 const char *text, size_t size);
int vhbu_update_config_parse_xml(VhbuUpdateConfig *config,
                                 const char *text, size_t size);
int vhbu_update_config_build_paths(VhbuUpdateConfig *config,
                                   const char *data_directory);
int vhbu_compare_versions(const char *left, const char *right);

#endif
