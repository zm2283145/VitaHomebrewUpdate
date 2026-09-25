#include <psp2/appmgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/cpu.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/net/net.h>
#include <psp2/promoterutil.h>
#include <psp2/shellutil.h>
#include <psp2/sysmodule.h>
#include <stdint.h>
#include <taihen.h>

#include "bgdl.h"
#include "bearssl_support.h"
#include "changeinfo.h"
#include "file_verify.h"
#include "fs_tree.h"
#include "pending_update.h"
#include "update_config.h"
#include "vpk_extract.h"

typedef struct vhbu_paf_wstring {
    uint16_t *data;
    uint32_t length;
} vhbu_paf_wstring;

typedef struct vhbu_paf_string {
    const char *data;
    uint32_t length;
    uint32_t allocator;
} vhbu_paf_string;

typedef struct vhbu_paf_id_param {
    vhbu_paf_string id;
    uint32_t hash;
} vhbu_paf_id_param;

typedef struct vhbu_paf_intrusive_ptr {
    void *object;
    void *control;
} vhbu_paf_intrusive_ptr;

typedef struct vhbu_paf_shared_ptr {
    void *object;
    void *counter;
} vhbu_paf_shared_ptr;

typedef struct vhbu_paf_plugin_init_param {
    uint8_t opaque[0x94];
} vhbu_paf_plugin_init_param;

typedef struct vhbu_paf_page_open_param {
    uint8_t opaque[0x2c];
} vhbu_paf_page_open_param;

typedef struct vhbu_paf_page_close_param {
    uint8_t opaque[0x10];
} vhbu_paf_page_close_param;

typedef struct vhbu_paf_template_open_param {
    uint8_t opaque[0x08];
} vhbu_paf_template_open_param;

typedef void (*vhbu_paf_callback)(int instance_slot, int button, void *data);

extern void *vhbu_paf_plugin_find(const char *name)
    __asm__("_ZN3paf6Plugin4FindEPKc");
extern void vhbu_paf_plugin_init_param_construct(
    vhbu_paf_plugin_init_param *param)
    __asm__("_ZN3paf6Plugin9InitParamC1Ev");
extern void vhbu_paf_plugin_load_sync(
    const vhbu_paf_plugin_init_param *param, void (*callback)(void *),
    int option)
    __asm__("_ZN3paf6Plugin8LoadSyncERKNS0_9InitParamEPFvPS0_Ei");
extern void vhbu_paf_plugin_unload_async(
    void *plugin, void (*callback)(const char *), int option)
    __asm__("_ZN3paf6Plugin11UnloadAsyncEPS0_PFvPKcEi");
extern void vhbu_paf_page_open_param_construct(
    vhbu_paf_page_open_param *param)
    __asm__("_ZN3paf6Plugin13PageOpenParamC1Ev");
extern void vhbu_paf_page_close_param_construct(
    vhbu_paf_page_close_param *param)
    __asm__("_ZN3paf6Plugin14PageCloseParamC1Ev");
extern void vhbu_paf_template_open_param_construct(
    vhbu_paf_template_open_param *param)
    __asm__("_ZN3paf6Plugin17TemplateOpenParamC1Ev");
extern void *vhbu_paf_plugin_page_open(
    void *plugin, const vhbu_paf_id_param *id,
    const vhbu_paf_page_open_param *param)
    __asm__("_ZN3paf6Plugin8PageOpenERKNS_7IDParamERKNS0_13PageOpenParamE");
extern void vhbu_paf_plugin_page_close(
    void *plugin, const vhbu_paf_id_param *id,
    const vhbu_paf_page_close_param *param)
    __asm__("_ZN3paf6Plugin9PageCloseERKNS_7IDParamERKNS0_14PageCloseParamE");
extern void vhbu_paf_plugin_get_texture(
    vhbu_paf_intrusive_ptr *result, void *plugin,
    const vhbu_paf_id_param *id)
    __asm__("_ZN3paf6Plugin10GetTextureERKNS_7IDParamE");
extern int vhbu_paf_plugin_template_open(
    void *plugin, void *parent, const vhbu_paf_id_param *id,
    const vhbu_paf_template_open_param *param)
    __asm__("_ZN3paf6Plugin12TemplateOpenEPNS_2ui6WidgetERKNS_7IDParamERKNS0_17TemplateOpenParamE");
extern uint32_t vhbu_paf_id_to_hash(
    vhbu_paf_id_param *id, const vhbu_paf_string *text)
    __asm__("_ZN3paf7IDParam9ID2IDHashERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEE");
extern void *vhbu_paf_widget_find_child(
    void *widget, const vhbu_paf_id_param *id, int option)
    __asm__("_ZN3paf2ui6Widget9FindChildERKNS_7IDParamEi");
extern void vhbu_paf_widget_show(void *widget, int transition, float time,
                                 void *callback, void *data)
    __asm__("_ZN3paf2ui6Widget4ShowENS_6common10transition4TypeEfPFviPNS0_7HandlerEPNS0_5EventEPvES9_");
extern void vhbu_paf_widget_hide(void *widget, int transition, float time,
                                 void *callback, void *data)
    __asm__("_ZN3paf2ui6Widget4HideENS_6common10transition4TypeEfPFviPNS0_7HandlerEPNS0_5EventEPvES9_");
extern void vhbu_paf_surface_release(void *surface)
    __asm__("_ZN3paf5graph7Surface7ReleaseEv");
extern void vhbu_paf_surface_load(
    vhbu_paf_intrusive_ptr *result, void *pool,
    const vhbu_paf_shared_ptr *file, void *option)
    __asm__("_ZN3paf5graph7Surface4LoadEPNS0_11SurfacePoolENS_6common9SharedPtrINS_4FileEEEPNS1_10LoadOptionE");
extern void *vhbu_paf_get_default_surface_pool(void)
    __asm__("_ZN3paf5gutil21GetDefaultSurfacePoolEv");
extern void vhbu_paf_local_file_open(
    vhbu_paf_shared_ptr *result, const char *path,
    uint32_t flags, uint32_t mode, int *error)
    __asm__("_ZN3paf9LocalFile4OpenEPKcjjPi");
extern void vhbu_paf_free(void *memory) __asm__("sce_paf_free");
extern void vhbu_paf_main_thread_register(void (*callback)(void *), void *data)
    __asm__("_ZN3paf6common18MainThreadCallList8RegisterEPFvPvES2_");
extern void vhbu_paf_main_thread_unregister(void (*callback)(void *), void *data)
    __asm__("_ZN3paf6common18MainThreadCallList10UnregisterEPFvPvES2_");
extern int vhbu_paf_dialog_show(void *plugin, vhbu_paf_wstring *title,
                                vhbu_paf_wstring *message, void *parameters,
                                vhbu_paf_callback callback, void *data)
    __asm__("_ZN3sce15CommonGuiDialog6Dialog4ShowEPN3paf6PluginEPNS2_12basic_stringIwNS2_11char_traitsIwEENS2_9allocatorIwEEEESB_PNS0_5ParamEPFviNS0_9DIALOG_CBEPvESF_");
extern int vhbu_paf_dialog_close(int instance_slot)
    __asm__("_ZN3sce15CommonGuiDialog6Dialog5CloseEi");
extern void *vhbu_paf_dialog_get_widget(int instance_slot, int register_id)
    __asm__("_ZN3sce15CommonGuiDialog6Dialog9GetWidgetEiNS0_11REGISTER_IDE");
extern unsigned char vhbu_paf_dialog_yes_no
    __asm__("_ZN3sce15CommonGuiDialog5Param13s_dialogYesNoE");
extern unsigned char vhbu_paf_dialog_text_small_busy
    __asm__("_ZN3sce15CommonGuiDialog5Param21s_dialogTextSmallBusyE");
extern int scePromoterUtilityUpdateUpgradableStatus(const char *title_id,
                                                    int value);

#define VHBU_STATUS_PORT 13379
#define VHBU_UPDATE_XML_CAPACITY (64u * 1024u)
#define VHBU_CHANGEINFO_CAPACITY (16u * 1024u)
#define VHBU_MAX_CONFIGURED_APPS 8u
#define VHBU_NOOP_ISOLATION 0
#define VHBU_ENABLE_STATUS_SERVER 1
#define VHBU_ENABLE_DELAYED_HTTP_HOOK 1
#define VHBU_ENABLE_EXPLORATORY_HOOKS 0
#define VHBU_ENABLE_START_NAME_HOOKS 1
#define VHBU_START_GATE_PROBE 1
#define VHBU_START_URI_PROBE 0
#define VHBU_START_CONTROLLER_PROBE 1
/*
 * Keep the private PAF dialog experiment disabled until the reference
 * plugin's LiveArea state handoff and matching cleanup path are reproduced.
 * Showing a second modal while SceShell owns the native update action leaves
 * the shell controller locked after either dialog is dismissed.
 */
#ifndef VHBU_ENABLE_CUSTOM_DIALOG
#define VHBU_ENABLE_CUSTOM_DIALOG 0
#endif
#define VHBU_REQUEST_CAPACITY 512
#define VHBU_LOG_DIRECTORY "ux0:data/VitaHomebrewUpdate"
#define VHBU_LOG_PATH VHBU_LOG_DIRECTORY "/service.log"
#define VHBU_SESSION_LOG_PATH VHBU_LOG_DIRECTORY "/service-session.log"
#define VHBU_PREVIOUS_SESSION_LOG_PATH \
    VHBU_LOG_DIRECTORY "/service-session.previous.log"
#define VHBU_NET_RETRY_COUNT 120
#define VHBU_NET_RETRY_DELAY_US (500u * 1000u)
#define SCE_NET_LIBRARY_NID 0x6BF8B2A2u
#define SCE_HTTP_LIBRARY_NID 0xE8F15CDEu
#define SCE_HTTP_CREATE_CONNECTION_WITH_URL_NID 0xC616C200u
#define SCE_HTTP_CREATE_REQUEST_WITH_URL_NID 0xBD5DA1D0u
#define SCE_HTTP_SEND_REQUEST_NID 0x9CA58B99u
#define SCE_APPMGR_USER_LIBRARY_NID 0xA6605D6Fu
#define SCE_LSDB_LIBRARY_NID 0x6BC25E17u
#define SCE_LSDB_START_LOOKUP_NID 0x7D7F6809u
#define SCE_LSDB_SEND_NOTIFICATION_NID 0x315B9FD6u
#define SCE_APPMGR_GET_STATUS_BY_NAME_NID 0x656E0ABDu
#define SCE_APPMGR_GET_STATUS_BY_APP_ID_NID 0x47958DE5u
#define SCE_APPMGR_LAUNCH_APP_BY_URI_NID 0x003C634Fu
#define SCE_APPMGR_LAUNCH_APP_BY_NAME_NID 0xC9C77E21u
#define SCE_APPMGR_LAUNCH_APP_BY_NAME2_NID 0x42945D67u
#define VHBU_NOTIFICATION_URI_PREFIX "photo:browse?category=ALL&hbu="
#define SCE_SHELL_365_RETAIL_NID 0x5549BF1Fu
#define SCE_SHELL_PATCH_IDENTITY_RETURN_OFFSET 0x1C1332u
#define SCE_SHELL_GAME_UPDATE_JOB_OFFSET 0x2B19D6u
#define SCE_SHELL_PATCH_CHECK_CACHE_CTOR_OFFSET 0x1C4898u
#define SCE_SHELL_PATCH_CHECK_CALLBACK_OFFSET 0x1C4A06u
#define SCE_SHELL_PATCH_PLUGIN_REQUEST_OFFSET 0x299C50u
#define SCE_SHELL_LIVEAREA_UPDATE_OFFSET 0x2AD898u
#define SCE_SHELL_LIVEAREA_EVENT_WRAPPER_OFFSET 0x2ABB4Cu
#define SCE_SHELL_LIVEAREA_OBJECT_SLOT_OFFSET 0x545B50u
#define SCE_SHELL_START_COMMAND_HANDLER_OFFSET 0x1CF8E2u
#define SCE_SHELL_START_STATUS_ROUTINE_OFFSET 0x29BFC6u
#define SCE_SHELL_START_GATE_OFFSET 0x29C744u
#define SCE_SHELL_STRING_C_STR_OFFSET 0x45A358u
#define SCE_SHELL_NOTICE_INIT_OFFSET 0x429754u
#define SCE_SHELL_STRING_SET_UTF8_OFFSET 0x40925Cu
#define SCE_SHELL_NOTICE_CLEAN_OFFSET 0x416830u
#define SCE_SHELL_BGDL_TOAST_OFFSET 0xFD94Cu
#define SCE_SHELL_HBU_AUX_A_OFFSET 0x11644Eu
#define SCE_SHELL_HBU_AUX_B_OFFSET 0x11B63Cu
#define SCE_SHELL_HBU_METHOD_OFFSET 0x50A9E8u
#define VHBU_NOTIFICATION_WORK_TITLE_ID "NPXS10004"

typedef int (*net_socket_fn)(const char *, int, int, int);
typedef int (*net_accept_fn)(int, SceNetSockaddr *, unsigned int *);
typedef int (*net_bind_fn)(int, const SceNetSockaddr *, unsigned int);
typedef int (*net_connect_fn)(int, const SceNetSockaddr *, unsigned int);
typedef int (*net_listen_fn)(int, int);
typedef int (*net_recv_fn)(int, void *, unsigned int, int);
typedef int (*net_send_fn)(int, const void *, unsigned int, int);
typedef int (*net_setsockopt_fn)(int, int, int, const void *, unsigned int);
typedef int (*net_shutdown_fn)(int, int);
typedef int (*net_socket_abort_fn)(int, int);
typedef int (*net_socket_close_fn)(int);
typedef int (*net_resolver_create_fn)(const char *, SceNetResolverParam *, int);
typedef int (*net_resolver_start_ntoa_fn)(int, const char *, SceNetInAddr *,
                                          int, int, int);
typedef int (*net_resolver_destroy_fn)(int);

static net_socket_fn net_socket;
static net_accept_fn net_accept;
static net_bind_fn net_bind;
static net_connect_fn net_connect;
static net_listen_fn net_listen;
static net_recv_fn net_recv;
static net_send_fn net_send;
static net_setsockopt_fn net_setsockopt;
static net_shutdown_fn net_shutdown;
static net_socket_abort_fn net_socket_abort;
static net_socket_close_fn net_socket_close;
static net_resolver_create_fn net_resolver_create;
static net_resolver_start_ntoa_fn net_resolver_start_ntoa;
static net_resolver_destroy_fn net_resolver_destroy;

static volatile int service_running;
static volatile int published_status = 1;
static volatile unsigned int game_update_call_count;
static volatile uintptr_t game_update_last_self;
static volatile int game_update_last_result;
static volatile unsigned int patch_check_ctor_call_count;
static volatile uintptr_t patch_check_ctor_last_self;
static volatile uintptr_t patch_check_ctor_last_argument;
static volatile uintptr_t patch_check_ctor_last_context;
static volatile uintptr_t patch_check_ctor_last_result;
#define VHBU_CTOR_SNAPSHOT_COUNT 4u
#define VHBU_CTOR_SNAPSHOT_WORDS 32u
static volatile uintptr_t patch_check_ctor_snapshot_self[VHBU_CTOR_SNAPSHOT_COUNT];
static volatile uintptr_t patch_check_ctor_snapshot_result[VHBU_CTOR_SNAPSHOT_COUNT];
static volatile uint32_t patch_check_ctor_snapshot_words[VHBU_CTOR_SNAPSHOT_COUNT]
                                                        [VHBU_CTOR_SNAPSHOT_WORDS];
static volatile unsigned int patch_check_callback_call_count;
static volatile uintptr_t patch_check_callback_last_self;
static volatile uint32_t patch_check_callback_fields[10];
static volatile int patch_check_callback_last_result;
static volatile unsigned int patch_plugin_request_call_count;
static volatile uintptr_t patch_plugin_request_last_object;
static volatile int patch_plugin_request_last_result;
static volatile unsigned int livearea_update_call_count;
static volatile uintptr_t livearea_update_last_self;
static volatile int livearea_update_last_action;
static volatile int livearea_update_last_result;
static volatile uint32_t livearea_update_last_mode;
static volatile uint32_t livearea_update_last_state;
static volatile uint32_t livearea_update_last_error;
static volatile uintptr_t livearea_update_last_completion;
static volatile unsigned int livearea_update_reference_handoff_count;
static volatile unsigned int livearea_event_call_count;
static volatile int livearea_event_last_value;
static volatile uintptr_t livearea_event_last_object;
static volatile uint32_t livearea_event_last_state;
static volatile unsigned int hbu_aux_a_call_count;
static volatile uintptr_t hbu_aux_a_last_args[4];
static volatile int hbu_aux_a_last_result;
static volatile int hbu_aux_a_last_task_id = -1;
static volatile unsigned int hbu_aux_a_override_count;
static volatile int test_download_completion_seen;
static volatile int test_download_completion_notice_sent;
static volatile int test_preexport_move_result;
static volatile unsigned int hbu_aux_b_call_count;
static volatile uintptr_t hbu_aux_b_last_args[4];
static volatile int hbu_aux_b_last_result;
static volatile unsigned int hbu_aux_b_override_count;
static volatile unsigned int http_connection_call_count;
static volatile int http_connection_last_template;
static volatile int http_connection_last_keepalive;
static volatile int http_connection_last_result;
static volatile int http_connection_hook_attempted;
static volatile int http_connection_last_redirected;
static volatile int test_download_state;
static volatile int test_download_task_id = -1;
static volatile int test_update_dialog_pending;
static volatile int test_update_dialog_active;
static volatile int test_update_dialog_confirmed;
static volatile int test_update_dialog_init_result;
static volatile int test_update_dialog_button;
static volatile int test_update_dialog_slot = -1;
static volatile int test_update_dialog_kind;
static volatile int test_download_queue_pending;
static volatile int test_download_queue_result;
static volatile int test_download_queue_service_result;
static volatile int test_download_preserve_result;
static volatile int test_download_preserved_size;
static volatile int test_verification_failure_kind;
static volatile unsigned int test_download_preserve_attempts;
static volatile unsigned int test_download_completion_baseline;
static int64_t test_notification_event_rowid;
static volatile unsigned int test_helper_launch_count;
static volatile int test_helper_launch_result;
static volatile int test_install_pending;
static volatile int test_install_result;
static volatile int test_install_phase;
static volatile int test_install_from_start_gate;
static volatile int test_close_target_pending;
static volatile int test_launch_target_pending;
static volatile int test_busy_dialog_pending;
static volatile int test_busy_dialog_close_pending;
static volatile int test_progress_ui_pending;
static volatile int test_progress_ui_close_pending;
static volatile int test_progress_ui_state;
static volatile int test_progress_ui_result;
static volatile int test_progress_ui_fallback_allowed = 1;
static volatile int test_progress_icon_result;
static volatile int test_progress_value_pending;
static volatile int test_progress_value_target;
#if VHBU_ENABLE_CUSTOM_DIALOG
static void *test_progress_plugin;
static void *test_progress_scene;
static void *test_progress_base;
static void *test_progress_bar;
#endif
static char http_connection_last_url[384];
static char http_connection_last_effective_url[384];
static volatile unsigned int http_request_call_count;
static volatile int http_request_last_connection;
static volatile int http_request_last_method;
static volatile int http_request_last_result;
static char http_request_last_url[384];
static volatile unsigned int http_send_call_count;
static volatile int http_send_last_request;
static volatile unsigned int http_send_last_size;
static volatile int http_send_last_result;
static volatile unsigned int identity_call_count;
static volatile unsigned int identity_candidate_count;
static volatile unsigned int identity_substitution_count;
static volatile int identity_substitution_armed;
static volatile uintptr_t identity_last_return_offset;
static volatile int identity_last_app_id;
static volatile int identity_last_result;
static volatile int identity_last_candidate;
static uintptr_t identity_shell_text_base;
static volatile unsigned int start_getid_call_count;
static volatile uintptr_t start_getid_last_return_offset;
static volatile int start_getid_last_result;
static volatile uintptr_t start_getid_last_title_ptr;
static volatile SceUID start_getid_last_app_id;
static volatile int start_getid_last_status_2c;
static char start_getid_last_title[32];
static volatile unsigned int start_command_call_count;
static volatile unsigned int start_command_staged_install_count;
static volatile uintptr_t start_command_last_return_offset;
static volatile int start_command_last_result;
static volatile uintptr_t start_command_last_context;
static volatile unsigned int start_command_last_word0;
static volatile unsigned int start_command_last_word1;
static volatile unsigned int start_command_last_payload_type;
static volatile unsigned int start_command_last_payload_size;
static char start_command_last_title[32];
static volatile unsigned int start_status_routine_call_count;
static volatile uintptr_t start_status_routine_last_return_offset;
static volatile int start_status_routine_last_result;
static volatile unsigned int start_status_routine_last_mode;
static volatile uint32_t start_status_routine_last_request_words[8];
static volatile uintptr_t start_status_routine_last_request_ptr;
static volatile unsigned int start_status_routine_install_count;
static volatile int start_status_routine_active_title_seen;
static volatile unsigned int start_gate_call_count;
static volatile unsigned int start_gate_defer_count;
static volatile int start_gate_last_result;
static char start_gate_last_title[32];
static volatile unsigned int start_lsdb_call_count;
static volatile int start_lsdb_last_result;
static volatile int start_lsdb_last_status;
static SceUID service_thread = -1;
static SceUID download_thread = -1;
static int listen_socket = -1;
static unsigned char update_xml[VHBU_UPDATE_XML_CAPACITY];
static unsigned char remote_update_xml[VHBU_UPDATE_XML_CAPACITY];
static size_t update_xml_size;
static volatile int update_xml_ready;
static volatile unsigned int update_refresh_ticks;
static unsigned char changeinfo_xml[VHBU_CHANGEINFO_CAPACITY];
static size_t changeinfo_xml_size;
static volatile int changeinfo_xml_ready;
static VhbuUpdateConfig active_update;
static char local_update_url[256];
static char local_package_url[384];
static char local_changeinfo_url[256];
typedef struct VhbuConfiguredUpdate {
    VhbuUpdateConfig config;
    unsigned char changeinfo[VHBU_CHANGEINFO_CAPACITY];
    size_t changeinfo_size;
    int update_available;
} VhbuConfiguredUpdate;
static VhbuConfiguredUpdate configured_updates[VHBU_MAX_CONFIGURED_APPS];
static unsigned int configured_update_count;
static int active_update_index = -1;
#define ACTIVE_TITLE_ID active_update.title_id
#define ACTIVE_NAME active_update.name
#define ACTIVE_INSTALLED_VERSION active_update.installed_version
#define ACTIVE_UPDATE_VERSION active_update.available_version
#define ACTIVE_PACKAGE_SIZE active_update.package_size
#define ACTIVE_PACKAGE_SHA1 active_update.package_sha1
#define ACTIVE_PACKAGE_URL active_update.package_url
#define ACTIVE_CHANGEINFO_URL active_update.changeinfo_url
#define ACTIVE_CONTENT_ID active_update.content_id
#define ACTIVE_PACKAGE_NAME active_update.package_name
#define ACTIVE_PENDING_PATH active_update.pending_path
#define ACTIVE_STAGE_PATH active_update.stage_path
#define ACTIVE_ICON_PATH active_update.icon_path
static SceUID game_update_hook_id = -1;
static tai_hook_ref_t game_update_hook_ref;
static SceUID launch_uri_hook_id = -1;
static tai_hook_ref_t launch_uri_hook_ref;
#if VHBU_ENABLE_START_NAME_HOOKS
static SceUID launch_name_hook_id = -1;
static tai_hook_ref_t launch_name_hook_ref;
static SceUID launch_name2_hook_id = -1;
static tai_hook_ref_t launch_name2_hook_ref;
#endif
static SceUID patch_check_ctor_hook_id = -1;
static tai_hook_ref_t patch_check_ctor_hook_ref;
static SceUID patch_check_callback_hook_id = -1;
static tai_hook_ref_t patch_check_callback_hook_ref;
static SceUID patch_plugin_request_hook_id = -1;
static tai_hook_ref_t patch_plugin_request_hook_ref;
static SceUID livearea_update_hook_id = -1;
static tai_hook_ref_t livearea_update_hook_ref;
static SceUID livearea_event_hook_id = -1;
static tai_hook_ref_t livearea_event_hook_ref;
static SceUID start_getid_hook_id = -1;
static tai_hook_ref_t start_getid_hook_ref;
static SceUID start_command_hook_id = -1;
static tai_hook_ref_t start_command_hook_ref;
static SceUID start_status_routine_hook_id = -1;
static tai_hook_ref_t start_status_routine_hook_ref;
static SceUID start_gate_hook_id = -1;
static tai_hook_ref_t start_gate_hook_ref;
static SceUID start_lsdb_hook_id = -1;
static tai_hook_ref_t start_lsdb_hook_ref;
static SceUID hbu_aux_a_hook_id = -1;
static tai_hook_ref_t hbu_aux_a_hook_ref;
static SceUID hbu_aux_b_hook_id = -1;
static tai_hook_ref_t hbu_aux_b_hook_ref;
static SceUID bgdl_toast_hook_id = -1;
static tai_hook_ref_t bgdl_toast_hook_ref;
static SceUID http_connection_hook_id = -1;
static tai_hook_ref_t http_connection_hook_ref;
static SceUID http_request_hook_id = -1;
static tai_hook_ref_t http_request_hook_ref;
static SceUID http_send_hook_id = -1;
static tai_hook_ref_t http_send_hook_ref;

static int verify_test_stage(void);
static void discard_staged_test_update(int remove_notification);
static int install_bgdl_toast_hook(void);
static int read_sfo_text(const char *path, const char *field_name,
                         char *output, unsigned int output_size);
static int activate_configured_update(unsigned int index);
static SceUID identity_hook_id = -1;
static tai_hook_ref_t identity_hook_ref;

typedef int (*vhbu_shell_notice_init_fn)(void *);
typedef int (*vhbu_shell_string_set_utf8_fn)(void *, const char *, SceSize);
typedef int (*vhbu_shell_notice_clean_fn)(void *);
typedef int (*vhbu_lsdb_send_notification_fn)(void *, int);

static vhbu_shell_notice_init_fn shell_notice_init;
static vhbu_shell_string_set_utf8_fn shell_string_set_utf8;
static vhbu_shell_notice_clean_fn shell_notice_clean;
static vhbu_lsdb_send_notification_fn lsdb_send_notification;

static void log_event(const char *event, int result);
#if VHBU_ENABLE_CUSTOM_DIALOG
static void queue_progress_value(int value);
#endif
static SceUID download_method_injection_id = -1;

static void disarm_active_download_hooks(void)
{
    if (download_method_injection_id >= 0) {
        (void)taiInjectRelease(download_method_injection_id);
        download_method_injection_id = -1;
    }
    if (http_send_hook_id >= 0) {
        (void)taiHookRelease(http_send_hook_id, http_send_hook_ref);
        http_send_hook_id = -1;
    }
    if (http_request_hook_id >= 0) {
        (void)taiHookRelease(http_request_hook_id, http_request_hook_ref);
        http_request_hook_id = -1;
    }
}

static void release_all_late_update_hooks(void)
{
    disarm_active_download_hooks();
    if (http_connection_hook_id >= 0) {
        (void)taiHookRelease(http_connection_hook_id,
                             http_connection_hook_ref);
        http_connection_hook_id = -1;
    }
    http_connection_hook_attempted = 0;
}

/*
 * SceShell asks SceAppMgr for the status of its NPXS19999 host while it builds
 * a patch-check request.  HomeBrewUpdate substitutes the selected title in
 * this one temporary result.  Gate on the exact 3.65 return site and original
 * title so unrelated AppMgr callers are never modified.
 */
static __attribute__((noinline)) int appmgr_get_status_by_app_id_hook(
    int app_id, void *status)
{
    uintptr_t return_address = (uintptr_t)__builtin_return_address(0);
    uintptr_t return_offset = (return_address & ~(uintptr_t)1u) -
                              identity_shell_text_base;
    int result = TAI_CONTINUE(int, identity_hook_ref, app_id, status);
    unsigned int index;
    int candidate = 0;

    if (identity_substitution_armed &&
        result >= 0 && status != NULL &&
        identity_shell_text_base != 0 &&
        return_offset == SCE_SHELL_PATCH_IDENTITY_RETURN_OFFSET) {
        unsigned char *bytes = (unsigned char *)status;
        static const char shell_title[] = "NPXS19999";

        identity_call_count++;
        identity_last_app_id = app_id;
        identity_last_result = result;
        identity_last_return_offset = return_offset;
        candidate = 1;
        for (index = 0; index < sizeof(shell_title) - 1u; ++index) {
            if (bytes[0x0Cu + index] !=
                (unsigned char)shell_title[index]) {
                candidate = 0;
                break;
            }
        }
        if (candidate) {
            identity_candidate_count++;
            for (index = 0; index < 31u; ++index) {
                bytes[0x0Cu + index] =
                    index < 9u ?
                    (unsigned char)ACTIVE_TITLE_ID[index] : 0;
            }
            bytes[0x2Bu] = 0;
            identity_substitution_count++;
        }
        identity_last_candidate = candidate;
    }
    return result;
}

static __attribute__((unused)) int install_patch_identity_hook(void)
{
    static const uint8_t expected_callsite[] = {
        0x96, 0xf2, 0x4c, 0xee, 0x00, 0x28
    };
    tai_module_info_t shell;
    SceKernelModuleInfo kernel_info;
    const volatile uint8_t *callsite;
    unsigned int index;
    int result;

    sceClibMemset(&shell, 0, sizeof(shell));
    shell.size = sizeof(shell);
    result = taiGetModuleInfo("SceShell", &shell);
    if (result < 0 || shell.module_nid != SCE_SHELL_365_RETAIL_NID)
        return -1;
    sceClibMemset(&kernel_info, 0, sizeof(kernel_info));
    kernel_info.size = sizeof(kernel_info);
    result = sceKernelGetModuleInfo(shell.modid, &kernel_info);
    if (result < 0)
        return result;
    identity_shell_text_base =
        (uintptr_t)kernel_info.segments[0].vaddr;
    callsite = (const volatile uint8_t *)(identity_shell_text_base +
        SCE_SHELL_PATCH_IDENTITY_RETURN_OFFSET - 4u);
    for (index = 0; index < sizeof(expected_callsite); ++index) {
        if (callsite[index] != expected_callsite[index]) {
            identity_shell_text_base = 0;
            log_event("identity_callsite_mismatch", (int)index);
            return -1;
        }

    }
    identity_hook_id = taiHookFunctionImport(
        &identity_hook_ref, "SceShell", SCE_APPMGR_USER_LIBRARY_NID,
        SCE_APPMGR_GET_STATUS_BY_APP_ID_NID,
        appmgr_get_status_by_app_id_hook);
    log_event("identity_hook_install", identity_hook_id);
    return identity_hook_id < 0 ? identity_hook_id : 0;
}

static int preserve_test_download(void)
{
    char source_path[160];
    unsigned char buffer[4096];
    SceUID source;
    SceUID destination;
    int total = 0;
    int read_result;

    if (test_download_task_id <= 0)
        return -1;
    log_event("test_vpk_preserve_begin", test_download_task_id);
    /* The reference moves the VPK out of ux0:bgdl in its completion hook,
     * before Sony's generic package path can consume it.  If that move has
     * already succeeded, only measure the preserved destination here. */
    source = sceIoOpen(ACTIVE_PENDING_PATH, SCE_O_RDONLY, 0);
    if (source >= 0) {
        while ((read_result = sceIoRead(source, buffer,
                                        sizeof(buffer))) > 0)
            total += read_result;
        (void)sceIoClose(source);
        test_download_preserved_size = total;
        if (read_result < 0)
            return read_result;
        if ((uint64_t)total == ACTIVE_PACKAGE_SIZE)
            return 0;
        total = 0;
    }
    (void)sceClibSnprintf(source_path, sizeof(source_path),
                          "ux0:bgdl/t/%08x/%s",
                          test_download_task_id, ACTIVE_PACKAGE_NAME);
    source = sceIoOpen(source_path, SCE_O_RDONLY, 0);
    if (source < 0) {
        log_event("test_vpk_source_open", source);
        return source;
    }
    log_event("test_vpk_source_open", 0);
    destination = sceIoOpen(ACTIVE_PENDING_PATH,
                            SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
    if (destination < 0) {
        log_event("test_vpk_dest_open", destination);
        (void)sceIoClose(source);
        return destination;
    }
    log_event("test_vpk_dest_open", 0);
    while ((read_result = sceIoRead(source, buffer, sizeof(buffer))) > 0) {
        int offset = 0;
        while (offset < read_result) {
            int write_result = sceIoWrite(destination, buffer + offset,
                                          (SceSize)(read_result - offset));
            if (write_result <= 0) {
                (void)sceIoClose(destination);
                (void)sceIoClose(source);
                return write_result < 0 ? write_result : -2;
            }
            offset += write_result;
        }
        total += read_result;
    }
    (void)sceIoClose(destination);
    (void)sceIoClose(source);
    test_download_preserved_size = total;
    if (read_result < 0) {
        log_event("test_vpk_read_result", read_result);
        return read_result;
    }
    log_event("test_vpk_read_result", total);
    return (uint64_t)total == ACTIVE_PACKAGE_SIZE ? 0 : -3;
}

static int publish_pending_test_update(void)
{
    static const char temporary_path[] = VHBU_PENDING_PATH ".tmp";
    VhbuPendingUpdate pending;
    SceUID file;
    int result;

    sceClibMemset(&pending, 0, sizeof(pending));
    pending.magic = VHBU_PENDING_MAGIC;
    pending.format = VHBU_PENDING_FORMAT;
    (void)sceClibSnprintf(pending.title_id, sizeof(pending.title_id),
                          "%s", ACTIVE_TITLE_ID);
    (void)sceClibSnprintf(pending.version, sizeof(pending.version),
                          "%s", ACTIVE_UPDATE_VERSION);
    pending.package_size = ACTIVE_PACKAGE_SIZE;
    pending.notification_rowid = test_notification_event_rowid;
    (void)sceClibSnprintf(pending.package_sha1,
                          sizeof(pending.package_sha1), "%s",
                          ACTIVE_PACKAGE_SHA1);
    (void)sceClibSnprintf(pending.content_id, sizeof(pending.content_id),
                          "%s", ACTIVE_CONTENT_ID);
    (void)sceClibSnprintf(pending.vpk_path, sizeof(pending.vpk_path),
                          "%s", ACTIVE_PENDING_PATH);
    (void)sceClibSnprintf(pending.stage_path, sizeof(pending.stage_path),
                          "%s", ACTIVE_STAGE_PATH);
    file = sceIoOpen(temporary_path,
        SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
    if (file < 0)
        return file;
    result = sceIoWrite(file, &pending, sizeof(pending));
    (void)sceIoClose(file);
    if (result != (int)sizeof(pending)) {
        (void)sceIoRemove(temporary_path);
        return result < 0 ? result : -1;
    }
    (void)sceIoRemove(VHBU_PENDING_PATH);
    result = sceIoRename(temporary_path, VHBU_PENDING_PATH);
    if (result < 0)
        (void)sceIoRemove(temporary_path);
    return result;
}

static int fixed_text_equals(const char *value, unsigned int capacity,
                             const char *expected)
{
    unsigned int expected_length =
        (unsigned int)sceClibStrnlen(expected, capacity);
    return expected_length < capacity &&
           value[expected_length] == '\0' &&
           sceClibStrncmp(value, expected, expected_length) == 0;
}

static int restore_pending_test_update(void)
{
    VhbuPendingUpdate pending;
    SceIoStat status;
    SceUID file;
    int received;
    int result;

    result = sceIoGetstat(VHBU_PENDING_PATH, &status);
    if (result < 0)
        return 1;
    if ((uint64_t)status.st_size != sizeof(pending))
        goto invalid;
    file = sceIoOpen(VHBU_PENDING_PATH, SCE_O_RDONLY, 0);
    if (file < 0)
        return file;
    received = sceIoRead(file, &pending, sizeof(pending));
    (void)sceIoClose(file);
    if (received != (int)sizeof(pending))
        goto invalid;
    if (pending.magic != VHBU_PENDING_MAGIC ||
        pending.format != VHBU_PENDING_FORMAT ||
        pending.package_size != ACTIVE_PACKAGE_SIZE ||
        pending.notification_rowid < 0 ||
        !fixed_text_equals(pending.title_id, sizeof(pending.title_id),
                           ACTIVE_TITLE_ID) ||
        !fixed_text_equals(pending.version, sizeof(pending.version),
                           ACTIVE_UPDATE_VERSION) ||
        !fixed_text_equals(pending.package_sha1,
                           sizeof(pending.package_sha1),
                           ACTIVE_PACKAGE_SHA1) ||
        !fixed_text_equals(pending.content_id, sizeof(pending.content_id),
                           ACTIVE_CONTENT_ID) ||
        !fixed_text_equals(pending.vpk_path, sizeof(pending.vpk_path),
                           ACTIVE_PENDING_PATH) ||
        !fixed_text_equals(pending.stage_path, sizeof(pending.stage_path),
                           ACTIVE_STAGE_PATH))
        goto invalid;
    result = vhbu_verify_file_size_sha1(
        ACTIVE_PENDING_PATH, pending.package_size,
        pending.package_sha1);
    if (result < 0)
        goto invalid;
    result = verify_test_stage();
    if (result < 0)
        goto invalid;
    test_notification_event_rowid = pending.notification_rowid;
    test_download_state = 7;
    return 0;

invalid:
    discard_staged_test_update(0);
    return -50;
}

typedef struct __attribute__((packed)) vhbu_sfo_header {
    uint32_t magic;
    uint32_t version;
    uint32_t key_offset;
    uint32_t value_offset;
    uint32_t count;
} vhbu_sfo_header;

typedef struct __attribute__((packed)) vhbu_sfo_entry {
    uint16_t name_offset;
    uint8_t alignment;
    uint8_t type;
    uint32_t value_size;
    uint32_t total_size;
    uint32_t data_offset;
} vhbu_sfo_entry;

static int sfo_text_equals(const unsigned char *data, unsigned int size,
                           unsigned int offset, unsigned int field_size,
                           const char *expected)
{
    unsigned int expected_size;
    if (offset >= size || field_size == 0 || field_size > size - offset)
        return 0;
    expected_size = (unsigned int)sceClibStrnlen(expected, field_size);
    return expected_size + 1u <= field_size &&
           data[offset + expected_size] == 0 &&
           sceClibStrncmp((const char *)data + offset, expected,
                          expected_size) == 0;
}

static int read_sfo_text(const char *path, const char *field_name,
                         char *output, unsigned int output_size)
{
    static unsigned char data[8192];
    const vhbu_sfo_header *header;
    const vhbu_sfo_entry *entries;
    unsigned int index;
    int received;
    SceUID file;

    if (path == NULL || field_name == NULL || output == NULL ||
        output_size == 0)
        return -1;
    output[0] = '\0';
    file = sceIoOpen(path, SCE_O_RDONLY, 0);
    if (file < 0)
        return file;
    received = sceIoRead(file, data, sizeof(data));
    (void)sceIoClose(file);
    if (received < (int)sizeof(*header) || received == (int)sizeof(data))
        return -2;
    header = (const vhbu_sfo_header *)data;
    if (header->magic != 0x46535000u || header->count > 128u ||
        sizeof(*header) + header->count * sizeof(*entries) >
            (unsigned int)received)
        return -3;
    entries = (const vhbu_sfo_entry *)(data + sizeof(*header));
    for (index = 0; index < header->count; ++index) {
        uint64_t key64 = (uint64_t)header->key_offset +
                         entries[index].name_offset;
        uint64_t value64 = (uint64_t)header->value_offset +
                           entries[index].data_offset;
        unsigned int key;
        unsigned int value;
        unsigned int available;
        unsigned int length;

        if (key64 >= (unsigned int)received ||
            value64 >= (unsigned int)received)
            continue;
        key = (unsigned int)key64;
        value = (unsigned int)value64;
        if (!sfo_text_equals(data, (unsigned int)received, key,
                             (unsigned int)received - key, field_name))
            continue;
        available = entries[index].value_size;
        if (available > (unsigned int)received - value)
            available = (unsigned int)received - value;
        length = 0;
        while (length < available && data[value + length] != 0)
            ++length;
        if (length >= output_size)
            length = output_size - 1u;
        sceClibMemcpy(output, data + value, length);
        output[length] = '\0';
        return length > 0 ? 0 : -4;
    }
    return -5;
}

static int verify_test_sfo(const char *path)
{
    unsigned char data[8192];
    const vhbu_sfo_header *header;
    const vhbu_sfo_entry *entries;
    unsigned int index;
    int have_title = 0;
    int have_version = 0;
    int have_content_id = 0;
    int received;
    SceUID file = sceIoOpen(path, SCE_O_RDONLY, 0);

    if (file < 0)
        return file;
    received = sceIoRead(file, data, sizeof(data));
    (void)sceIoClose(file);
    if (received < (int)sizeof(*header) || received == (int)sizeof(data))
        return -30;
    header = (const vhbu_sfo_header *)data;
    if (header->magic != 0x46535000u || header->count > 128u ||
        sizeof(*header) + header->count * sizeof(*entries) >
            (unsigned int)received)
        return -31;
    entries = (const vhbu_sfo_entry *)(data + sizeof(*header));
    for (index = 0; index < header->count; ++index) {
        uint64_t key64 = (uint64_t)header->key_offset +
                         entries[index].name_offset;
        uint64_t value64 = (uint64_t)header->value_offset +
                           entries[index].data_offset;
        unsigned int key;
        unsigned int value;
        if (key64 >= (unsigned int)received ||
            value64 >= (unsigned int)received)
            return -32;
        key = (unsigned int)key64;
        value = (unsigned int)value64;
        if (sfo_text_equals(data, (unsigned int)received, key,
                            (unsigned int)received - key, "TITLE_ID")) {
            if (!sfo_text_equals(data, (unsigned int)received, value,
                                 entries[index].value_size,
                                 ACTIVE_TITLE_ID))
                return -33;
            have_title = 1;
        } else if (sfo_text_equals(data, (unsigned int)received, key,
                                   (unsigned int)received - key,
                                   "APP_VER")) {
            if (!sfo_text_equals(data, (unsigned int)received, value,
                                 entries[index].value_size,
                                 ACTIVE_UPDATE_VERSION))
                return -34;
            have_version = 1;
        } else if (sfo_text_equals(data, (unsigned int)received, key,
                                   (unsigned int)received - key,
                                   "CONTENT_ID")) {
            if (!sfo_text_equals(data, (unsigned int)received, value,
                                 entries[index].value_size,
                                 ACTIVE_CONTENT_ID))
                return -36;
            have_content_id = 1;
        }
    }
    return have_title && have_version && have_content_id ? 0 : -35;
}

static int verify_regular_file(const char *path)
{
    SceIoStat status;
    int result = sceIoGetstat(path, &status);
    if (result < 0)
        return result;
    return SCE_S_ISREG(status.st_mode) && status.st_size > 0 ? 0 : -40;
}

static int verify_test_stage(void)
{
    char staged_sfo[256];
    char staged_eboot[256];
    char staged_head[256];
    (void)sceClibSnprintf(staged_sfo, sizeof(staged_sfo),
        "%s/sce_sys/param.sfo", ACTIVE_STAGE_PATH);
    (void)sceClibSnprintf(staged_eboot, sizeof(staged_eboot),
        "%s/eboot.bin", ACTIVE_STAGE_PATH);
    (void)sceClibSnprintf(staged_head, sizeof(staged_head),
        "%s/sce_sys/package/head.bin", ACTIVE_STAGE_PATH);
    int result = verify_regular_file(staged_eboot);
    if (result < 0)
        return result;
    result = verify_regular_file(staged_head);
    if (result < 0)
        return result;
    return verify_test_sfo(staged_sfo);
}

static int prepare_staged_test_update(void)
{
    int result;

    test_verification_failure_kind = 1;
    if (ACTIVE_PACKAGE_SIZE == 0u)
        return -41;
    result = vhbu_verify_file_size_sha1(
        ACTIVE_PENDING_PATH, ACTIVE_PACKAGE_SIZE,
        ACTIVE_PACKAGE_SHA1);
    log_event("test_vpk_integrity_result", result);
    if (result < 0)
        return result;
    test_verification_failure_kind = 2;
    result = vhbu_remove_stage_tree(ACTIVE_STAGE_PATH);
    log_event("test_stage_cleanup_result", result);
    if (result < 0)
        return result;
    result = vhbu_extract_vpk(ACTIVE_PENDING_PATH, ACTIVE_STAGE_PATH);
    log_event("test_vpk_extract_result", result);
    if (result < 0)
        goto failed;
    result = verify_test_stage();
    log_event("test_vpk_metadata_result", result);
    if (result < 0) {
        if (result == -34)
            test_verification_failure_kind = 3;
        goto failed;
    }
    test_verification_failure_kind = 0;
    return 0;

failed:
    (void)vhbu_remove_stage_tree(ACTIVE_STAGE_PATH);
    return result;
}

static int resolve_direct_notification(void)
{
    static const uint8_t init_signature[] = {
        0x2d, 0xe9, 0xf0, 0x41
    };
    static const uint8_t set_signature[] = {
        0x2d, 0xe9, 0xf0, 0x41
    };
    static const uint8_t clean_signature[] = {
        0x2d, 0xe9, 0xf0, 0x41
    };
    tai_module_info_t shell;
    SceKernelModuleInfo kernel_info;
    const volatile uint8_t *target;
    uintptr_t text_base;
    unsigned int index;
    int result;

    if (shell_notice_init != NULL && shell_string_set_utf8 != NULL &&
        shell_notice_clean != NULL && lsdb_send_notification != NULL)
        return 0;
    sceClibMemset(&shell, 0, sizeof(shell));
    shell.size = sizeof(shell);
    result = taiGetModuleInfo("SceShell", &shell);
    if (result < 0)
        return result;
    if (shell.module_nid != SCE_SHELL_365_RETAIL_NID)
        return -1;
    sceClibMemset(&kernel_info, 0, sizeof(kernel_info));
    kernel_info.size = sizeof(kernel_info);
    result = sceKernelGetModuleInfo(shell.modid, &kernel_info);
    if (result < 0)
        return result;
    text_base = (uintptr_t)kernel_info.segments[0].vaddr;

#define VERIFY_NOTICE_HELPER(offset, signature, error_code)                    \
    do {                                                                        \
        target = (const volatile uint8_t *)(text_base + (offset));              \
        for (index = 0; index < sizeof(signature); ++index) {                   \
            if (target[index] != (signature)[index])                            \
                return (error_code) - (int)index;                               \
        }                                                                       \
    } while (0)
    VERIFY_NOTICE_HELPER(SCE_SHELL_NOTICE_INIT_OFFSET,
                         init_signature, -100);
    VERIFY_NOTICE_HELPER(SCE_SHELL_STRING_SET_UTF8_OFFSET,
                         set_signature, -120);
    VERIFY_NOTICE_HELPER(SCE_SHELL_NOTICE_CLEAN_OFFSET,
                         clean_signature, -140);
#undef VERIFY_NOTICE_HELPER

    result = taiGetModuleExportFunc(
        "SceLsdb", SCE_LSDB_LIBRARY_NID,
        SCE_LSDB_SEND_NOTIFICATION_NID,
        (uintptr_t *)&lsdb_send_notification);
    if (result < 0)
        return result;
    shell_notice_init = (vhbu_shell_notice_init_fn)(
        text_base + SCE_SHELL_NOTICE_INIT_OFFSET + 1u);
    shell_string_set_utf8 = (vhbu_shell_string_set_utf8_fn)(
        text_base + SCE_SHELL_STRING_SET_UTF8_OFFSET + 1u);
    shell_notice_clean = (vhbu_shell_notice_clean_fn)(
        text_base + SCE_SHELL_NOTICE_CLEAN_OFFSET + 1u);
    return 0;
}

static int set_notification_text(void *field, const char *text,
                                 unsigned int maximum)
{
    SceSize length;

    if (field == NULL || text == NULL || shell_string_set_utf8 == NULL)
        return -1;
    length = (SceSize)sceClibStrnlen(text, maximum);
    if (length >= maximum)
        return -2;
    (void)shell_string_set_utf8(field, text, length);
    return 0;
}

enum vhbu_test_notification_state {
    VHBU_TEST_NOTIFICATION_DOWNLOAD_COMPLETE = 0,
    VHBU_TEST_NOTIFICATION_CHECKING = 1,
    VHBU_TEST_NOTIFICATION_WAITING = 2,
    VHBU_TEST_NOTIFICATION_INSTALLED = 3
};

static int publish_test_notification(enum vhbu_test_notification_state state)
{
    uint32_t notification_words[0x100u / sizeof(uint32_t)];
    unsigned char *notification = (unsigned char *)notification_words;
    char item_id[16];
    char description[160];
    char uri[128];
    const char *status = NULL;
    int actionable = state == VHBU_TEST_NOTIFICATION_WAITING;
    int result;
    int clean_result;

    result = resolve_direct_notification();
    log_event("direct_notification_resolve", result);
    if (result < 0)
        return result;
    if (test_download_task_id <= 0)
        return -3;
    result = sceClibSnprintf(item_id, sizeof(item_id), "%d",
                             test_download_task_id);
    if (result <= 0 || result >= (int)sizeof(item_id))
        return -4;
    if (state == VHBU_TEST_NOTIFICATION_CHECKING)
        status = "Checking File...";
    else if (state == VHBU_TEST_NOTIFICATION_WAITING)
        status = "Waiting to Install";
    else if (state == VHBU_TEST_NOTIFICATION_INSTALLED)
        status = "Install completed";
    description[0] = '\0';
    if (status != NULL) {
        result = sceClibSnprintf(
            description, sizeof(description), "%s: %s", ACTIVE_NAME, status);
        if (result <= 0 || result >= (int)sizeof(description))
            return -5;
    }
    uri[0] = '\0';
    if (actionable != 0) {
        result = sceClibSnprintf(
            uri, sizeof(uri), VHBU_NOTIFICATION_URI_PREFIX "%s:%d",
            ACTIVE_TITLE_ID, test_download_task_id);
        if (result <= 0 || result >= (int)sizeof(uri))
            return -6;
    }
    sceClibMemset(notification, 0, sizeof(notification_words));
    (void)shell_notice_init(notification);
    result = set_notification_text(notification + 0x00u,
                                   "LOGSTATUS0", 12u);
    if (result >= 0)
        result = set_notification_text(notification + 0x0Cu,
                                       item_id, 16u);
    if (result >= 0)
        result = set_notification_text(notification + 0x30u,
                                       ACTIVE_ICON_PATH, 0x10000u);
    if (result >= 0 &&
        state == VHBU_TEST_NOTIFICATION_DOWNLOAD_COMPLETE)
        result = set_notification_text(notification + 0xB0u,
                                       ACTIVE_NAME, 0x10000u);
    if (result >= 0 && status != NULL)
        result = set_notification_text(notification + 0xBCu,
                                       description, 0x10000u);
    if (result >= 0 && actionable != 0)
        result = set_notification_text(notification + 0xCCu,
                                       "NPXS10004", 16u);
    if (result >= 0 && actionable != 0)
        result = set_notification_text(notification + 0xD8u,
                                       uri, 0x10000u);
    if (result >= 0) {
        *(uint32_t *)(notification + 0x20u) =
            state == VHBU_TEST_NOTIFICATION_DOWNLOAD_COMPLETE ?
                0x51u : 0x100u;
        *(uint32_t *)(notification + 0x28u) =
            actionable != 0 ? 2u : 0u;
        notification[0x2Cu] = 1u;
        notification[0x2Du] = 1u;
        *(uint32_t *)(notification + 0xC8u) =
            actionable != 0 ? 0x70000u : 0u;
        *(uint32_t *)(notification + 0xFCu) = 0x10u;
        result = lsdb_send_notification(notification, 1);
    }
    clean_result = shell_notice_clean(notification);
    log_event("direct_notification_clean", clean_result);
    test_notification_event_rowid = 0;
    return result;
}

static int publish_download_complete_test_notification(void)
{
    return publish_test_notification(VHBU_TEST_NOTIFICATION_DOWNLOAD_COMPLETE);
}

static int publish_checking_test_notification(void)
{
    return publish_test_notification(VHBU_TEST_NOTIFICATION_CHECKING);
}

static int publish_waiting_test_notification(void)
{
    return publish_test_notification(VHBU_TEST_NOTIFICATION_WAITING);
}

static int publish_installed_test_notification(void)
{
    return publish_test_notification(VHBU_TEST_NOTIFICATION_INSTALLED);
}

static void discard_staged_test_update(int remove_notification)
{
    (void)remove_notification;
    test_notification_event_rowid = 0;
    (void)sceIoRemove(VHBU_PENDING_PATH ".tmp");
    (void)sceIoRemove(VHBU_PENDING_PATH);
    (void)sceIoRemove(ACTIVE_PENDING_PATH);
    (void)vhbu_remove_stage_tree(ACTIVE_STAGE_PATH);
}

static __attribute__((unused)) int install_staged_test_update(void)
{
    char installed_sfo[192];
    int result;
    int exit_result;
    int unload_result;
    int operation_result = 0;
    int promoter_state = 0;
    unsigned int attempt;

    (void)sceClibSnprintf(installed_sfo, sizeof(installed_sfo),
        "ux0:app/%s/sce_sys/param.sfo", ACTIVE_TITLE_ID);
    result = verify_test_stage();
    log_event("test_stage_reverify_result", result);
    if (result < 0)
        return result;
#if VHBU_ENABLE_CUSTOM_DIALOG
    queue_progress_value(10);
#endif

    result = sceSysmoduleLoadModuleInternal(
        SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
    log_event("test_promoter_module_load", result);
    if (result < 0 && (unsigned int)result != 0x805A1001u)
        return result;
#if VHBU_ENABLE_CUSTOM_DIALOG
    queue_progress_value(20);
#endif

    result = scePromoterUtilityInit();
    log_event("test_promoter_init", result);
    if (result >= 0) {
#if VHBU_ENABLE_CUSTOM_DIALOG
        queue_progress_value(30);
#endif
        result = scePromoterUtilityPromotePkgWithRif(
            ACTIVE_STAGE_PATH, 1);
        log_event("test_promoter_promote", result);
        if (result >= 0) {
#if VHBU_ENABLE_CUSTOM_DIALOG
            queue_progress_value(45);
#endif
            for (attempt = 0; attempt < 250u; ++attempt) {
                result = scePromoterUtilityGetState(&promoter_state);
                if (result < 0 || promoter_state == 0)
                    break;
#if VHBU_ENABLE_CUSTOM_DIALOG
                if ((attempt % 10u) == 0u)
                    queue_progress_value(
                        45 + (int)(attempt * 45u / 250u));
#endif
                sceKernelDelayThread(20u * 1000u);
            }
#if VHBU_ENABLE_CUSTOM_DIALOG
            if (result >= 0 && promoter_state == 0)
                queue_progress_value(90);
#endif
            log_event("test_promoter_state", promoter_state);
            if (result >= 0 && promoter_state != 0)
                result = -60;
        }
        if (result >= 0) {
            result = scePromoterUtilityGetResult(&operation_result);
            log_event("test_promoter_operation_result", operation_result);
            if (result >= 0 && operation_result < 0)
                result = operation_result;
#if VHBU_ENABLE_CUSTOM_DIALOG
            if (result >= 0)
                queue_progress_value(95);
#endif
        }
        if (result >= 0) {
            int status_result =
                scePromoterUtilityUpdateUpgradableStatus(
                    ACTIVE_TITLE_ID, 0);
            log_event("test_upgradable_status_result", status_result);
            if (status_result < 0)
                result = status_result;
        }
        exit_result = scePromoterUtilityExit();
        log_event("test_promoter_exit", exit_result);
        if (result >= 0 && exit_result < 0)
            result = exit_result;
#if VHBU_ENABLE_CUSTOM_DIALOG
        if (result >= 0)
            queue_progress_value(98);
#endif
    }

    unload_result = sceSysmoduleUnloadModuleInternal(
        SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
    log_event("test_promoter_module_unload", unload_result);
    if (result >= 0 && unload_result < 0)
        result = unload_result;
    if (result >= 0) {
        result = verify_test_sfo(installed_sfo);
        log_event("test_install_verify_result", result);
        if (result >= 0) {
            int task_cleanup_result;
#if VHBU_ENABLE_CUSTOM_DIALOG
            queue_progress_value(100);
#endif
            (void)sceIoRemove(VHBU_PENDING_PATH);
            (void)sceIoRemove(ACTIVE_PENDING_PATH);
            (void)vhbu_remove_stage_tree(ACTIVE_STAGE_PATH);
            task_cleanup_result =
                vhbu_remove_bgdl_task_tree(test_download_task_id);
            log_event("test_bgdl_task_cleanup_result",
                      task_cleanup_result);
            /* Direct app.db writes can block SceShell's database owner.
             * Completion UI remains plugin-owned until the reference's
             * in-process notification path is reproduced. */
        }
    }
    return result;
}

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
    RESOLVE_NET(net_connect, 0x11E5B6F6u);
    RESOLVE_NET(net_listen, 0x7A8DA094u);
    RESOLVE_NET(net_recv, 0x023643B7u);
    RESOLVE_NET(net_send, 0xE3DD8CD9u);
    RESOLVE_NET(net_setsockopt, 0x065505CAu);
    RESOLVE_NET(net_shutdown, 0x69E50BB5u);
    RESOLVE_NET(net_socket, 0xF084FCE3u);
    RESOLVE_NET(net_socket_abort, 0x891C1B9Bu);
    RESOLVE_NET(net_socket_close, 0x29822B4Du);
    RESOLVE_NET(net_resolver_create, 0x6DA29319u);
    RESOLVE_NET(net_resolver_start_ntoa, 0x1EB11857u);
    RESOLVE_NET(net_resolver_destroy, 0x3559F098u);
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
    file = sceIoOpen(VHBU_SESSION_LOG_PATH,
                     SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0666);
    if (file >= 0) {
        (void)sceIoWrite(file, line, (SceSize)length);
        (void)sceIoClose(file);
    }
}

static void log_text_event(const char *event, const char *value)
{
    char line[512];
    SceUID file;
    int length;

    (void)sceIoMkdir(VHBU_LOG_DIRECTORY, 0777);
    length = sceClibSnprintf(line, sizeof(line), "%s value=%s\n", event,
                             value != NULL ? value : "(null)");
    if (length <= 0 || length >= (int)sizeof(line))
        return;
    file = sceIoOpen(VHBU_SESSION_LOG_PATH,
                     SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0666);
    if (file >= 0) {
        (void)sceIoWrite(file, line, (SceSize)length);
        (void)sceIoClose(file);
    }
}

static int is_test_update_url(const char *url)
{
    char prefix[128];
    char suffix[64];
    unsigned int url_length;
    unsigned int prefix_length;
    unsigned int suffix_length;
    unsigned int configured_index;
    int result;

    if (url == NULL)
        return 0;
    url_length = (unsigned int)sceClibStrnlen(url, 1024u);
    if (url_length >= 1024u)
        return 0;
    for (configured_index = 0;
         configured_index < configured_update_count;
         ++configured_index) {
        const VhbuConfiguredUpdate *entry =
            &configured_updates[configured_index];
        result = sceClibSnprintf(prefix, sizeof(prefix),
            "https://gs-sec.ww.np.dl.playstation.net/pl/np/%s/",
            entry->config.title_id);
        if (result <= 0 || result >= (int)sizeof(prefix))
            continue;
        prefix_length = (unsigned int)result;
        result = sceClibSnprintf(suffix, sizeof(suffix),
            "/%s-ver.xml", entry->config.title_id);
        if (result <= 0 || result >= (int)sizeof(suffix))
            continue;
        suffix_length = (unsigned int)result;
        if (url_length >= suffix_length &&
            sceClibStrncmp(url, prefix, prefix_length) == 0 &&
            sceClibStrncmp(url + url_length - suffix_length,
                           suffix, suffix_length) == 0) {
            if (active_update_index != (int)configured_index) {
                if (test_download_state != 0)
                    return 0;
                if (activate_configured_update(configured_index) < 0)
                    return 0;
            }
            return 1;
        }
    }
    return 0;
}

static int http_create_connection_with_url_hook(int template_id,
                                                 const char *url,
                                                 int enable_keepalive)
{
    const char *effective_url = url;
    unsigned int index = 0;
    int result;

    http_connection_last_template = template_id;
    http_connection_last_keepalive = enable_keepalive;
    http_connection_last_redirected = 0;
    if (is_test_update_url(url))
        http_connection_last_redirected = 1;
    if (url != NULL) {
        while (index + 1u < sizeof(http_connection_last_url) && url[index]) {
            http_connection_last_url[index] = url[index];
            ++index;
        }
    }
    http_connection_last_url[index] = '\0';
    if (http_connection_last_redirected) {
        unsigned int original_length =
            (unsigned int)sceClibStrnlen(url, 1024u);
        unsigned int replacement_length =
            (unsigned int)sceClibStrnlen(local_update_url,
                                         sizeof(local_update_url));
        if (original_length >= replacement_length) {
            char *writable_url = (char *)url;
            for (index = 0; index <= replacement_length; ++index)
                writable_url[index] = local_update_url[index];
            effective_url = url;
        } else {
            http_connection_last_redirected = 0;
        }
        /* Merely opening or refreshing an up-to-date title must not arm the
         * download state.  Doing so prevents the idle periodic discovery
         * loop from observing a release published later in this session. */
        if (http_connection_last_redirected && test_download_state == 0 &&
            vhbu_compare_versions(ACTIVE_INSTALLED_VERSION,
                                  ACTIVE_UPDATE_VERSION) < 0)
            test_download_state = 1;
    }
    index = 0;
    if (effective_url != NULL) {
        while (index + 1u < sizeof(http_connection_last_effective_url) &&
               effective_url[index]) {
            http_connection_last_effective_url[index] = effective_url[index];
            ++index;
        }
    }
    http_connection_last_effective_url[index] = '\0';
    result = TAI_CONTINUE(int, http_connection_hook_ref, template_id,
                          effective_url, enable_keepalive);
    http_connection_last_result = result;
    http_connection_call_count++;
    return result;
}

static int install_passive_http_connection_hook(void)
{
    http_connection_hook_id = taiHookFunctionImport(
        &http_connection_hook_ref, "SceShell", SCE_HTTP_LIBRARY_NID,
        SCE_HTTP_CREATE_CONNECTION_WITH_URL_NID,
        http_create_connection_with_url_hook);
    log_event("http_connection_hook_install", http_connection_hook_id);
    return http_connection_hook_id < 0 ? http_connection_hook_id : 0;
}

static int http_create_request_with_url_hook(int connection_id, int method,
                                              const char *url,
                                              unsigned long long content_length)
{
    const char *effective_url = url;
    unsigned int index = 0;
    int result;

    http_request_last_connection = connection_id;
    http_request_last_method = method;
    if (url != NULL) {
        while (index + 1u < sizeof(http_request_last_url) && url[index]) {
            http_request_last_url[index] = url[index];
            ++index;
        }
    }
    http_request_last_url[index] = '\0';
    if (is_test_update_url(url))
        effective_url = local_update_url;
    result = TAI_CONTINUE(int, http_request_hook_ref, connection_id, method,
                          effective_url, content_length);
    http_request_last_result = result;
    http_request_call_count++;
    return result;
}

static int http_send_request_hook(int request_id, const void *post_data,
                                  unsigned int size)
{
    int result = TAI_CONTINUE(int, http_send_hook_ref, request_id,
                              post_data, size);
    http_send_last_request = request_id;
    http_send_last_size = size;
    http_send_last_result = result;
    http_send_call_count++;
    return result;
}

static int install_passive_http_followup_hooks(void)
{
    http_request_hook_id = taiHookFunctionImport(
        &http_request_hook_ref, "SceShell", SCE_HTTP_LIBRARY_NID,
        SCE_HTTP_CREATE_REQUEST_WITH_URL_NID,
        http_create_request_with_url_hook);
    log_event("http_request_hook_install", http_request_hook_id);
    http_send_hook_id = taiHookFunctionImport(
        &http_send_hook_ref, "SceShell", SCE_HTTP_LIBRARY_NID,
        SCE_HTTP_SEND_REQUEST_NID, http_send_request_hook);
    log_event("http_send_hook_install", http_send_hook_id);
    return (http_request_hook_id < 0 || http_send_hook_id < 0) ? -1 : 0;
}

static int install_download_get_injection(void)
{
    static const uint8_t expected[] = {'H', 'E', 'A', 'D'};
    static const uint8_t replacement[] = {'G', 'E', 'T', '\0'};
    tai_module_info_t shell;
    SceKernelModuleInfo kernel_info;
    const volatile uint8_t *target;
    unsigned int index;
    int result;

    if (download_method_injection_id >= 0)
        return 0;
    sceClibMemset(&shell, 0, sizeof(shell));
    shell.size = sizeof(shell);
    result = taiGetModuleInfo("SceShell", &shell);
    if (result < 0 || shell.module_nid != SCE_SHELL_365_RETAIL_NID)
        return -1;
    sceClibMemset(&kernel_info, 0, sizeof(kernel_info));
    kernel_info.size = sizeof(kernel_info);
    result = sceKernelGetModuleInfo(shell.modid, &kernel_info);
    if (result < 0)
        return result;
    if (SCE_SHELL_HBU_METHOD_OFFSET + sizeof(expected) >
        kernel_info.segments[0].memsz)
        return -1;
    target = (const volatile uint8_t *)(
        (uintptr_t)kernel_info.segments[0].vaddr +
        SCE_SHELL_HBU_METHOD_OFFSET);
    for (index = 0; index < sizeof(expected); ++index) {
        if (target[index] != expected[index]) {
            log_event("download_method_signature_mismatch", (int)index);
            return -1;
        }
    }
    download_method_injection_id = taiInjectData(
        shell.modid, 0, SCE_SHELL_HBU_METHOD_OFFSET,
        replacement, sizeof(replacement));
    log_event("download_method_get_injection",
              download_method_injection_id);
    return download_method_injection_id < 0 ?
        download_method_injection_id : 0;
}

static int game_update_job_hook(void *self)
{
    int result;

    game_update_last_self = (uintptr_t)self;
    game_update_call_count++;
    result = TAI_CONTINUE(int, game_update_hook_ref, self);
    game_update_last_result = result;
    return result;
}

static void *patch_check_cache_ctor_hook(void *self, void *argument,
                                         void *context)
{
    unsigned int call_index;
    unsigned int field_index;
    void *result;

    patch_check_ctor_last_self = (uintptr_t)self;
    patch_check_ctor_last_argument = (uintptr_t)argument;
    patch_check_ctor_last_context = (uintptr_t)context;
    result = TAI_CONTINUE(void *, patch_check_ctor_hook_ref,
                          self, argument, context);
    patch_check_ctor_last_result = (uintptr_t)result;
    call_index = patch_check_ctor_call_count % VHBU_CTOR_SNAPSHOT_COUNT;
    patch_check_ctor_snapshot_self[call_index] = (uintptr_t)self;
    patch_check_ctor_snapshot_result[call_index] = (uintptr_t)result;
    if (result != NULL) {
        const volatile uint32_t *words =
            (const volatile uint32_t *)result;
        for (field_index = 0; field_index < VHBU_CTOR_SNAPSHOT_WORDS;
             ++field_index)
            patch_check_ctor_snapshot_words[call_index][field_index] =
                words[field_index];
    }
    patch_check_ctor_call_count++;
    return result;
}

static __attribute__((unused)) int install_passive_patch_check_ctor_hook(void)
{
    static const uint8_t expected_prologue[] = {
        0xf0, 0xb5, 0x83, 0xb0
    };
    static const uint8_t expected_body_a[] = {
        0x08, 0x30, 0x28, 0x60
    };
    static const uint8_t expected_body_b[] = {
        0x28, 0x1c, 0x01, 0x9a, 0x39, 0x68, 0x91, 0x42
    };
    tai_module_info_t shell;
    SceKernelModuleInfo kernel_info;
    const volatile uint8_t *target;
    unsigned int index;
    int result;

    sceClibMemset(&shell, 0, sizeof(shell));
    shell.size = sizeof(shell);
    result = taiGetModuleInfo("SceShell", &shell);
    if (result < 0) {
        log_event("patch_ctor_shell_info_failed", result);
        return result;
    }
    if (shell.module_nid != SCE_SHELL_365_RETAIL_NID) {
        log_event("patch_ctor_shell_unsupported", (int)shell.module_nid);
        return -1;
    }
    sceClibMemset(&kernel_info, 0, sizeof(kernel_info));
    kernel_info.size = sizeof(kernel_info);
    result = sceKernelGetModuleInfo(shell.modid, &kernel_info);
    if (result < 0) {
        log_event("patch_ctor_module_info_failed", result);
        return result;
    }
    if (SCE_SHELL_PATCH_CHECK_CACHE_CTOR_OFFSET + 0x60u >
        kernel_info.segments[0].memsz) {
        log_event("patch_ctor_offset_out_of_range", -1);
        return -1;
    }
    target = (const volatile uint8_t *)(
        (uintptr_t)kernel_info.segments[0].vaddr +
        SCE_SHELL_PATCH_CHECK_CACHE_CTOR_OFFSET);
    for (index = 0; index < sizeof(expected_prologue); ++index) {
        if (target[index] != expected_prologue[index]) {
            log_event("patch_ctor_prologue_mismatch", (int)index);
            return -1;
        }
    }
    for (index = 0; index < sizeof(expected_body_a); ++index) {
        if (target[0x34u + index] != expected_body_a[index]) {
            log_event("patch_ctor_body_a_mismatch", (int)index);
            return -1;
        }
    }
    for (index = 0; index < sizeof(expected_body_b); ++index) {
        if (target[0x58u + index] != expected_body_b[index]) {
            log_event("patch_ctor_body_b_mismatch", (int)index);
            return -1;
        }
    }
    patch_check_ctor_hook_id = taiHookFunctionOffset(
        &patch_check_ctor_hook_ref, shell.modid, 0,
        SCE_SHELL_PATCH_CHECK_CACHE_CTOR_OFFSET, 1,
        patch_check_cache_ctor_hook);
    log_event("patch_check_ctor_hook_install", patch_check_ctor_hook_id);
    return patch_check_ctor_hook_id < 0 ? patch_check_ctor_hook_id : 0;
}

static int patch_check_callback_hook(void *self)
{
    const volatile uint32_t *words = (const volatile uint32_t *)self;
    unsigned int index;
    int result;

    patch_check_callback_last_self = (uintptr_t)self;
    for (index = 0; index < 10; ++index)
        patch_check_callback_fields[index] = words[7u + index];
    patch_check_callback_call_count++;
    result = TAI_CONTINUE(int, patch_check_callback_hook_ref, self);
    patch_check_callback_last_result = result;
    return result;
}

static __attribute__((unused)) int install_passive_patch_check_callback_hook(void)
{
    static const uint8_t expected_prologue[] = {
        0xf0, 0xb5, 0x87, 0xb0
    };
    static const uint8_t expected_body[] = {
        0x00, 0x20, 0x00, 0x21, 0xcd, 0xe9, 0x02, 0x01,
        0x00, 0x21, 0xcd, 0xe9, 0x04, 0x11
    };
    tai_module_info_t shell;
    SceKernelModuleInfo kernel_info;
    const volatile uint8_t *target;
    unsigned int index;
    int result;

    sceClibMemset(&shell, 0, sizeof(shell));
    shell.size = sizeof(shell);
    result = taiGetModuleInfo("SceShell", &shell);
    if (result < 0)
        return result;
    if (shell.module_nid != SCE_SHELL_365_RETAIL_NID)
        return -1;
    sceClibMemset(&kernel_info, 0, sizeof(kernel_info));
    kernel_info.size = sizeof(kernel_info);
    result = sceKernelGetModuleInfo(shell.modid, &kernel_info);
    if (result < 0)
        return result;
    if (SCE_SHELL_PATCH_CHECK_CALLBACK_OFFSET + 0x30u >
        kernel_info.segments[0].memsz)
        return -1;
    target = (const volatile uint8_t *)(
        (uintptr_t)kernel_info.segments[0].vaddr +
        SCE_SHELL_PATCH_CHECK_CALLBACK_OFFSET);
    for (index = 0; index < sizeof(expected_prologue); ++index) {
        if (target[index] != expected_prologue[index]) {
            log_event("patch_callback_prologue_mismatch", (int)index);
            return -1;
        }
    }
    for (index = 0; index < sizeof(expected_body); ++index) {
        if (target[0x12u + index] != expected_body[index]) {
            log_event("patch_callback_body_mismatch", (int)index);
            return -1;
        }
    }
    patch_check_callback_hook_id = taiHookFunctionOffset(
        &patch_check_callback_hook_ref, shell.modid, 0,
        SCE_SHELL_PATCH_CHECK_CALLBACK_OFFSET, 1,
        patch_check_callback_hook);
    log_event("patch_check_callback_hook_install",
              patch_check_callback_hook_id);
    return patch_check_callback_hook_id < 0 ?
        patch_check_callback_hook_id : 0;
}

static int patch_plugin_request_hook(void *object)
{
    int result;

    patch_plugin_request_last_object = (uintptr_t)object;
    patch_plugin_request_call_count++;
    result = TAI_CONTINUE(int, patch_plugin_request_hook_ref, object);
    patch_plugin_request_last_result = result;
    return result;
}

static __attribute__((unused)) int install_passive_patch_plugin_request_hook(void)
{
    static const uint8_t expected_prologue[] = {
        0x2d, 0xe9, 0xf0, 0x47, 0xb4, 0xb0
    };
    static const uint8_t expected_body[] = {
        0xda, 0xf8, 0x00, 0x10, 0x32, 0x91
    };
    tai_module_info_t shell;
    SceKernelModuleInfo kernel_info;
    const volatile uint8_t *target;
    unsigned int index;
    int result;

    sceClibMemset(&shell, 0, sizeof(shell));
    shell.size = sizeof(shell);
    result = taiGetModuleInfo("SceShell", &shell);
    if (result < 0)
        return result;
    if (shell.module_nid != SCE_SHELL_365_RETAIL_NID)
        return -1;
    sceClibMemset(&kernel_info, 0, sizeof(kernel_info));
    kernel_info.size = sizeof(kernel_info);
    result = sceKernelGetModuleInfo(shell.modid, &kernel_info);
    if (result < 0)
        return result;
    target = (const volatile uint8_t *)(
        (uintptr_t)kernel_info.segments[0].vaddr +
        SCE_SHELL_PATCH_PLUGIN_REQUEST_OFFSET);
    for (index = 0; index < sizeof(expected_prologue); ++index) {
        if (target[index] != expected_prologue[index]) {
            log_event("patch_request_prologue_mismatch", (int)index);
            return -1;
        }
    }
    for (index = 0; index < sizeof(expected_body); ++index) {
        if (target[0x0eu + index] != expected_body[index]) {
            log_event("patch_request_body_mismatch", (int)index);
            return -1;
        }
    }
    patch_plugin_request_hook_id = taiHookFunctionOffset(
        &patch_plugin_request_hook_ref, shell.modid, 0,
        SCE_SHELL_PATCH_PLUGIN_REQUEST_OFFSET, 1,
        patch_plugin_request_hook);
    log_event("patch_plugin_request_hook_install",
              patch_plugin_request_hook_id);
    return patch_plugin_request_hook_id < 0 ?
        patch_plugin_request_hook_id : 0;
}

#if VHBU_ENABLE_CUSTOM_DIALOG
static uint32_t utf8_to_paf_wstring(const char *source, uint16_t *destination,
                                    uint32_t capacity)
{
    uint32_t input = 0;
    uint32_t output = 0;

    if (source == NULL || destination == NULL || capacity == 0)
        return 0;
    while (output + 1u < capacity && source[input] != '\0') {
        uint32_t codepoint;
        unsigned char first = (unsigned char)source[input++];

        if (first < 0x80u) {
            codepoint = first;
        } else if ((first & 0xE0u) == 0xC0u && source[input] != '\0') {
            codepoint = ((uint32_t)(first & 0x1Fu) << 6) |
                        ((uint32_t)(unsigned char)source[input++] & 0x3Fu);
        } else if ((first & 0xF0u) == 0xE0u && source[input] != '\0' &&
                   source[input + 1u] != '\0') {
            codepoint = ((uint32_t)(first & 0x0Fu) << 12) |
                        (((uint32_t)(unsigned char)source[input] & 0x3Fu) << 6) |
                        ((uint32_t)(unsigned char)source[input + 1u] & 0x3Fu);
            input += 2u;
        } else {
            codepoint = '?';
            while (((unsigned char)source[input] & 0xC0u) == 0x80u)
                ++input;
        }
        if (codepoint > 0xFFFFu ||
            (codepoint >= 0xD800u && codepoint <= 0xDFFFu))
            codepoint = '?';
        destination[output++] = (uint16_t)codepoint;
    }
    destination[output] = 0;
    return output;
}

static uint32_t ascii_length(const char *text)
{
    uint32_t length = 0;

    if (text == NULL)
        return 0;
    while (text[length] != '\0')
        ++length;
    return length;
}

static void init_paf_id(vhbu_paf_id_param *id, const char *name)
{
    id->id.data = name;
    id->id.length = ascii_length(name);
    id->id.allocator = 0;
    id->hash = vhbu_paf_id_to_hash(id, &id->id);
}

static void init_paf_hash_id(vhbu_paf_id_param *id, uint32_t hash)
{
    id->id.data = NULL;
    id->id.length = 0;
    id->id.allocator = 0;
    id->hash = hash;
}

static void set_widget_text_ascii(void *widget, const char *label)
{
    typedef void (*set_text_fn)(void *, const vhbu_paf_wstring *);
    uint16_t label_buffer[128];
    vhbu_paf_wstring text;
    void **vtable;
    set_text_fn set_text;

    if (widget == NULL)
        return;
    text.data = label_buffer;
    text.length = utf8_to_paf_wstring(
        label, label_buffer,
        sizeof(label_buffer) / sizeof(label_buffer[0]));
    vtable = *(void ***)widget;
    if (vtable == NULL)
        return;
    set_text = (set_text_fn)vtable[0x11Cu / sizeof(void *)];
    if (set_text != NULL)
        set_text(widget, &text);
}

static void *find_progress_widget(void *parent, void *scene,
                                  const char *name)
{
    vhbu_paf_id_param id;
    void *widget;

    init_paf_id(&id, name);
    widget = parent == NULL ? NULL :
        vhbu_paf_widget_find_child(parent, &id, 0);
    if (widget == NULL && scene != NULL && scene != parent)
        widget = vhbu_paf_widget_find_child(scene, &id, 0);
    return widget;
}

static void set_progress_value(void *widget, int value)
{
    typedef int (*set_limit_fn)(void *, int);
    typedef int (*set_value_fn)(void *, int, int, int);
    void **vtable;

    if (widget == NULL)
        return;
    vtable = *(void ***)widget;
    if (vtable == NULL)
        return;
    ((set_limit_fn)vtable[0x184u / sizeof(void *)])(widget, 100);
    ((set_limit_fn)vtable[0x198u / sizeof(void *)])(widget, 0);
    ((set_value_fn)vtable[0x1ACu / sizeof(void *)])(
        widget, value, 0, 0);
}

static void test_progress_value_main_thread(void *data)
{
    int value;

    (void)data;
    (void)vhbu_paf_main_thread_unregister(
        test_progress_value_main_thread, NULL);
    value = test_progress_value_target;
    test_progress_value_pending = 0;
    if (test_progress_ui_state == 2 && test_progress_bar != NULL)
        set_progress_value(test_progress_bar, value);
}

static void queue_progress_value(int value)
{
    if (value < 0)
        value = 0;
    else if (value > 100)
        value = 100;
    test_progress_value_target = value;
    if (test_progress_value_pending == 0 &&
        test_progress_ui_state == 2) {
        test_progress_value_pending = 1;
        vhbu_paf_main_thread_register(
            test_progress_value_main_thread, NULL);
    }
}

static void set_widget_texture(void *widget,
                               const vhbu_paf_intrusive_ptr *texture)
{
    typedef int (*set_texture_fn)(
        void *, const vhbu_paf_intrusive_ptr *);
    void **vtable;

    if (widget == NULL || texture == NULL || texture->object == NULL)
        return;
    vtable = *(void ***)widget;
    if (vtable != NULL && vtable[0x100u / sizeof(void *)] != NULL)
        ((set_texture_fn)vtable[0x100u / sizeof(void *)])(
            widget, texture);
}

static void release_paf_shared_ptr(vhbu_paf_shared_ptr *pointer)
{
    typedef void (*deleter_fn)(void *);
    typedef struct vhbu_paf_ref_counter {
        void *object;
        int32_t strong_count;
        int32_t weak_count;
        deleter_fn deleter;
    } vhbu_paf_ref_counter;
    vhbu_paf_ref_counter *counter;

    if (pointer == NULL || pointer->object == NULL ||
        pointer->counter == NULL)
        return;
    counter = (vhbu_paf_ref_counter *)pointer->counter;
    if (counter->strong_count > 0) {
        int32_t previous = sceKernelAtomicGetAndAdd32(
            &counter->strong_count, -1);
        if (previous == 1) {
            if (counter->deleter != NULL)
                counter->deleter(counter->object);
            previous = sceKernelAtomicGetAndAdd32(
                &counter->weak_count, -1);
            if (previous == 1)
                vhbu_paf_free(counter);
        }
    }
    pointer->object = NULL;
    pointer->counter = NULL;
}

static int load_progress_app_icon(void *image_widget)
{
    vhbu_paf_shared_ptr file;
    vhbu_paf_intrusive_ptr surface;
    void *pool;
    int open_result = -1;

    file.object = NULL;
    file.counter = NULL;
    surface.object = NULL;
    surface.control = NULL;
    vhbu_paf_local_file_open(
        &file, ACTIVE_ICON_PATH, 1u, 0u, &open_result);
    if (open_result != 0 || file.object == NULL) {
        release_paf_shared_ptr(&file);
        return open_result != 0 ? open_result : -2;
    }
    pool = vhbu_paf_get_default_surface_pool();
    if (pool == NULL) {
        release_paf_shared_ptr(&file);
        return -3;
    }
    vhbu_paf_surface_load(&surface, pool, &file, NULL);
    release_paf_shared_ptr(&file);
    if (surface.object == NULL)
        return -4;
    set_widget_texture(image_widget, &surface);
    vhbu_paf_surface_release(surface.object);
    return 0;
}

static void test_progress_ui_open_main_thread(void *data)
{
    char installed_sfo[192];
    char application_title[128];
    vhbu_paf_plugin_init_param init_param;
    vhbu_paf_page_open_param page_param;
    vhbu_paf_template_open_param template_param;
    vhbu_paf_page_close_param close_param;
    vhbu_paf_id_param page_id;
    vhbu_paf_id_param template_id;
    vhbu_paf_id_param texture_id;
    vhbu_paf_intrusive_ptr texture;
    vhbu_paf_string *field;
    void *title_widget;
    void *text_widget;
    void *image_widget;

    (void)data;
    (void)vhbu_paf_main_thread_unregister(
        test_progress_ui_open_main_thread, NULL);
    test_progress_ui_result = -1;
    test_progress_plugin = NULL;
    test_progress_scene = NULL;
    test_progress_base = NULL;
    test_progress_bar = NULL;

    vhbu_paf_plugin_init_param_construct(&init_param);
    test_progress_ui_result = -10;
    field = (vhbu_paf_string *)&init_param.opaque[0x00];
    field->data = "hbu_install_progress_plugin";
    field->length = sizeof("hbu_install_progress_plugin") - 1u;
    field->allocator = 0;
    field = (vhbu_paf_string *)&init_param.opaque[0x0C];
    field->data = "__main__";
    field->length = sizeof("__main__") - 1u;
    field->allocator = 0;
    field = (vhbu_paf_string *)&init_param.opaque[0x2C];
    field->data = "vs0:/vsh/game/gamecard_installer_plugin.rco";
    field->length =
        sizeof("vs0:/vsh/game/gamecard_installer_plugin.rco") - 1u;
    field->allocator = 0;
    vhbu_paf_plugin_load_sync(&init_param, NULL, 0);
    test_progress_ui_result = -11;
    test_progress_plugin = vhbu_paf_plugin_find(
        "hbu_install_progress_plugin");
    if (test_progress_plugin == NULL)
        goto failed;

    vhbu_paf_page_open_param_construct(&page_param);
    test_progress_ui_result = -20;
    *(int32_t *)&page_param.opaque[8] = 1;
    init_paf_id(&page_id, "page_bg");
    test_progress_scene = vhbu_paf_plugin_page_open(
        test_progress_plugin, &page_id, &page_param);
    if (test_progress_scene == NULL)
        goto failed;

    vhbu_paf_template_open_param_construct(&template_param);
    test_progress_ui_result = -30;
    /* gamecard_installer_plugin.rco identifies its dialog template by the
     * resource hash used by HomebrewUpdate.  dialog_base is the widget that
     * becomes available after this template has been instantiated. */
    init_paf_hash_id(&template_id, 0x9AC337E5u);
    if (vhbu_paf_plugin_template_open(
            test_progress_plugin, test_progress_scene,
            &template_id, &template_param) < 0)
        goto failed;
    test_progress_base = find_progress_widget(
        test_progress_scene, test_progress_scene, "dialog_base");
    test_progress_ui_result = -40;
    if (test_progress_base == NULL)
        goto failed;

    /* The outer template creates dialog_base.  A second template attached
     * to that base creates dialog_title, dialog_text1, the progress bar, and
     * dialog_image.  This nested open is present in the reference binary. */
    init_paf_hash_id(&template_id, 0x388D5CD6u);
    test_progress_ui_result = -41;
    if (vhbu_paf_plugin_template_open(
            test_progress_plugin, test_progress_base,
            &template_id, &template_param) < 0)
        goto failed;

    title_widget = find_progress_widget(
        test_progress_base, test_progress_scene, "dialog_title");
    text_widget = find_progress_widget(
        test_progress_base, test_progress_scene, "dialog_text1");
    test_progress_bar = find_progress_widget(
        test_progress_base, test_progress_scene, "dialog_progressbar1");
    image_widget = find_progress_widget(
        test_progress_base, test_progress_scene, "dialog_image");
    test_progress_ui_result = -50;
    if (title_widget == NULL)
        test_progress_ui_result = -51;
    else if (text_widget == NULL)
        test_progress_ui_result = -52;
    else if (test_progress_bar == NULL)
        test_progress_ui_result = -53;
    else if (image_widget == NULL)
        test_progress_ui_result = -54;
    if (title_widget == NULL || text_widget == NULL ||
        test_progress_bar == NULL || image_widget == NULL)
        goto failed;

    (void)sceClibSnprintf(installed_sfo, sizeof(installed_sfo),
        "ux0:app/%s/sce_sys/param.sfo", ACTIVE_TITLE_ID);
    if (read_sfo_text(installed_sfo, "TITLE", application_title,
                      sizeof(application_title)) < 0)
        sceClibSnprintf(application_title, sizeof(application_title),
                        "Installing application");
    set_widget_text_ascii(title_widget, application_title);
    set_widget_text_ascii(text_widget, "Installing...");
    set_progress_value(test_progress_bar, 0);

    texture.object = NULL;
    texture.control = NULL;
    init_paf_hash_id(&texture_id, 0x264C5084u);
    vhbu_paf_plugin_get_texture(
        &texture, test_progress_plugin, &texture_id);
    if (texture.object != NULL) {
        set_widget_texture(image_widget, &texture);
        vhbu_paf_surface_release(texture.object);
    }
    test_progress_icon_result = load_progress_app_icon(image_widget);

    vhbu_paf_widget_show(test_progress_base, 1, 0.0f, NULL, NULL);
    test_progress_ui_result = 0;
    test_progress_ui_state = 2;
    return;

failed:
    if (test_progress_base != NULL)
        (void)vhbu_paf_widget_hide(
            test_progress_base, 1, 0.0f, NULL, NULL);
    if (test_progress_plugin != NULL && test_progress_scene != NULL) {
        vhbu_paf_page_close_param_construct(&close_param);
        init_paf_id(&page_id, "page_bg");
        vhbu_paf_plugin_page_close(
            test_progress_plugin, &page_id, &close_param);
    }
    if (test_progress_plugin != NULL)
        vhbu_paf_plugin_unload_async(test_progress_plugin, NULL, 0);
    test_progress_plugin = NULL;
    test_progress_scene = NULL;
    test_progress_base = NULL;
    test_progress_bar = NULL;
    test_progress_ui_state = -1;
}

static void test_progress_ui_close_main_thread(void *data)
{
    vhbu_paf_page_close_param close_param;
    vhbu_paf_id_param page_id;

    (void)data;
    (void)vhbu_paf_main_thread_unregister(
        test_progress_ui_close_main_thread, NULL);
    if (test_progress_base != NULL)
        (void)vhbu_paf_widget_hide(
            test_progress_base, 1, 0.0f, NULL, NULL);
    if (test_progress_plugin != NULL && test_progress_scene != NULL) {
        vhbu_paf_page_close_param_construct(&close_param);
        init_paf_id(&page_id, "page_bg");
        vhbu_paf_plugin_page_close(
            test_progress_plugin, &page_id, &close_param);
    }
    if (test_progress_plugin != NULL)
        vhbu_paf_plugin_unload_async(test_progress_plugin, NULL, 0);
    test_progress_plugin = NULL;
    test_progress_scene = NULL;
    test_progress_base = NULL;
    test_progress_bar = NULL;
    test_progress_ui_state = 0;
}

static void set_dialog_button_label(int instance_slot, int register_id,
                                    const char *label)
{
    void *widget;

    widget = vhbu_paf_dialog_get_widget(instance_slot, register_id);
    set_widget_text_ascii(widget, label);
}

static void test_update_dialog_button_callback(int instance_slot, int button,
                                               void *data)
{
    (void)data;
    test_update_dialog_button = button;
    if (button == 4) {
        test_update_dialog_confirmed = 1;
        if (test_update_dialog_kind == 2)
            test_install_pending = 1;
        else if (test_update_dialog_kind == 4)
            test_close_target_pending = 1;
        else if (test_update_dialog_kind == 6)
            test_launch_target_pending = 1;
        else if (test_update_dialog_kind == 1) {
            /* Match HomebrewUpdate's handoff: dismiss the choice dialog,
             * present its modal busy dialog, and only let the BGDL worker
             * register the task after that dialog is visible. */
            test_download_queue_pending = 1;
            test_busy_dialog_pending = 1;
        }
    } else if (test_update_dialog_kind == 4) {
        test_install_from_start_gate = 0;
    }
    (void)vhbu_paf_dialog_close(instance_slot);
    test_update_dialog_active = 3;
}

static void test_update_dialog_main_thread(void *data)
{
    static const char * const plugin_names[] = {
        "livearea_plugin", "topmenu_plugin", "game_plugin",
        "patch_chk_plugin"
    };
    static const char title_text[] = "Homebrew Update";
    static const char installing_title_text[] = "Installing application";
    static const char install_message_text[] =
        "This update has already been downloaded.\n\n"
        "Do you want to install it now?";
    static const char close_message_text[] =
        "This application is currently running.\n\n"
        "Do you want to close it and install the update now?";
    static const char launch_message_text[] =
        "Install complete.\n\n"
        "Do you want to launch this application now?";
    static const char wait_message_text[] = "Please wait...";
    static const char installing_message_text[] = "Installing...";
    static const char changeinfo_fallback_text[] =
        "Release notes are unavailable for this update.";
    char download_message_text[512];
    char changeinfo_text[288];
    const char *selected_title;
    const char *selected_message;
    int busy;
    static uint16_t title_buffer[64];
    static uint16_t message_buffer[512];
    vhbu_paf_wstring title;
    vhbu_paf_wstring message;
    void *plugin = NULL;
    unsigned int index;

    (void)data;
    (void)vhbu_paf_main_thread_unregister(test_update_dialog_main_thread,
                                          NULL);
    for (index = 0;
         index < sizeof(plugin_names) / sizeof(plugin_names[0]); ++index) {
        plugin = vhbu_paf_plugin_find(plugin_names[index]);
        if (plugin != NULL)
            break;
    }
    if (plugin == NULL) {
        test_update_dialog_init_result = -1;
        test_update_dialog_active = 3;
        return;
    }
    (void)vhbu_changeinfo_extract_or_fallback(
        changeinfo_xml, changeinfo_xml_size, changeinfo_text,
        sizeof(changeinfo_text), changeinfo_fallback_text);
    (void)sceClibSnprintf(
        download_message_text, sizeof(download_message_text),
        "A new version of the application is available\n\n"
        "Installed Version: %s\n"
        "Available Version: %s (%u.%02u KB)\n\n"
        "What's new:\n%s\n\n"
        "Do you want to download now?",
        ACTIVE_INSTALLED_VERSION, ACTIVE_UPDATE_VERSION,
        (unsigned int)(ACTIVE_PACKAGE_SIZE / 1024u),
        (unsigned int)((ACTIVE_PACKAGE_SIZE % 1024u) *
                       100u / 1024u),
        changeinfo_text);
    busy = test_update_dialog_kind == 3 || test_update_dialog_kind == 5;
    selected_title = test_update_dialog_kind == 5 ?
        installing_title_text : title_text;
    if (test_update_dialog_kind == 2)
        selected_message = install_message_text;
    else if (test_update_dialog_kind == 3)
        selected_message = wait_message_text;
    else if (test_update_dialog_kind == 4)
        selected_message = close_message_text;
    else if (test_update_dialog_kind == 5)
        selected_message = installing_message_text;
    else if (test_update_dialog_kind == 6)
        selected_message = launch_message_text;
    else
        selected_message = download_message_text;
    title.data = title_buffer;
    title.length = utf8_to_paf_wstring(selected_title, title_buffer,
                                         sizeof(title_buffer) /
                                         sizeof(title_buffer[0]));
    message.data = message_buffer;
    message.length = utf8_to_paf_wstring(selected_message,
                                           message_buffer,
                                           sizeof(message_buffer) /
                                           sizeof(message_buffer[0]));
    test_update_dialog_init_result = vhbu_paf_dialog_show(
        plugin, &title, &message,
        busy ?
            (void *)&vhbu_paf_dialog_text_small_busy :
            (void *)&vhbu_paf_dialog_yes_no,
        busy ? NULL :
            test_update_dialog_button_callback,
        NULL);
    test_update_dialog_slot = test_update_dialog_init_result;
    if (test_update_dialog_init_result < 0)
        test_update_dialog_active = 3;
    else {
        if (!busy) {
            const char *positive_label = "OK";
            if (test_update_dialog_kind == 1)
                positive_label = "Download";
            else if (test_update_dialog_kind == 2 ||
                     test_update_dialog_kind == 4)
                positive_label = "Install";
            else if (test_update_dialog_kind == 6)
                positive_label = "Launch";
            /* Match HomebrewUpdate: widget 12 is the cancel action and
             * widget 13 is the context-specific positive action. */
            set_dialog_button_label(test_update_dialog_init_result,
                                    12, "Cancel");
            set_dialog_button_label(test_update_dialog_init_result,
                                    13, positive_label);
        }
        test_update_dialog_active = 2;
    }
}

static void test_update_dialog_close_main_thread(void *data)
{
    (void)data;
    (void)vhbu_paf_main_thread_unregister(
        test_update_dialog_close_main_thread, NULL);
    if (test_update_dialog_slot >= 0)
        (void)vhbu_paf_dialog_close(test_update_dialog_slot);
    test_update_dialog_active = 3;
}
#endif

#if VHBU_ENABLE_CUSTOM_DIALOG
static __attribute__((unused)) int is_target_start_uri(const char *uri)
{
    static const char prefix[] = "psgm:play?titleid=";
    unsigned int prefix_length = sizeof(prefix) - 1u;
    unsigned int title_length =
        (unsigned int)sceClibStrnlen(ACTIVE_TITLE_ID, 12u);
    char suffix;

    if (uri == NULL || title_length == 0u || title_length >= 12u)
        return 0;
    if (sceClibStrncmp(uri, prefix, prefix_length) != 0 ||
        sceClibStrncmp(uri + prefix_length, ACTIVE_TITLE_ID,
                       title_length) != 0)
        return 0;
    suffix = uri[prefix_length + title_length];
    return suffix == '\0' || suffix == '&';
}

static __attribute__((unused)) int queue_start_gate_for_title(
    const char *title_id, int source)
{
    unsigned int title_length =
        (unsigned int)sceClibStrnlen(ACTIVE_TITLE_ID, 12u);

    if (test_download_state != 7 || title_id == NULL ||
        title_length == 0u || title_length >= 12u ||
        sceClibStrncmp(title_id, ACTIVE_TITLE_ID, title_length) != 0 ||
        title_id[title_length] != '\0' ||
        test_update_dialog_active != 0 ||
        test_update_dialog_pending != 0 || test_install_phase != 0)
        return 0;
    test_update_dialog_kind = 2;
    test_update_dialog_pending = 1;
    log_event("start_gate_install_action", source);
    return 1;
}

static int is_notification_install_uri(const char *uri)
{
    unsigned int prefix_length =
        sizeof(VHBU_NOTIFICATION_URI_PREFIX) - 1u;
    unsigned int title_length =
        (unsigned int)sceClibStrnlen(ACTIVE_TITLE_ID, 12u);
    const char *task;
    unsigned int digits = 0;

    if (uri == NULL || title_length == 0u || title_length >= 12u ||
        sceClibStrncmp(uri, VHBU_NOTIFICATION_URI_PREFIX,
                       prefix_length) != 0 ||
        sceClibStrncmp(uri + prefix_length, ACTIVE_TITLE_ID,
                       title_length) != 0 ||
        uri[prefix_length + title_length] != ':')
        return 0;
    task = uri + prefix_length + title_length + 1u;
    while (*task >= '0' && *task <= '9') {
        ++task;
        ++digits;
    }
    return digits != 0u && *task == '\0';
}

static int launch_app_by_uri_hook(int flags, const char *uri)
{
#if VHBU_START_URI_PROBE
    int result = TAI_CONTINUE(int, launch_uri_hook_ref, flags, uri);
    log_event("start_uri_probe_result", result);
    if (uri != NULL)
        log_text_event("start_uri_probe_value", uri);
    return result;
#else
    if (test_download_state == 7 &&
        is_notification_install_uri(uri) &&
        test_update_dialog_active == 0 &&
        test_update_dialog_pending == 0 &&
        test_install_phase == 0) {
        test_install_from_start_gate = 0;
        test_update_dialog_kind = 2;
        test_update_dialog_pending = 1;
        log_event("notification_install_action", flags);
        return 0;
    }

    return TAI_CONTINUE(int, launch_uri_hook_ref, flags, uri);
#endif
}

#if VHBU_ENABLE_START_NAME_HOOKS
static int launch_app_by_name_hook(int flags, const char *title_id,
                                   const char *parameter)
{
    if (test_download_state == 7 && title_id != NULL &&
        sceClibStrncmp(title_id,
                       VHBU_NOTIFICATION_WORK_TITLE_ID, 9) == 0 &&
        title_id[9] == '\0' &&
        test_update_dialog_active == 0 &&
        test_update_dialog_pending == 0 &&
        test_install_phase == 0) {
        test_install_from_start_gate = 0;
        test_update_dialog_kind = 2;
        test_update_dialog_pending = 1;
        log_event("notification_name_action", flags);
        if (parameter != NULL)
            log_text_event("notification_name_parameter", parameter);
        return 0;
    }
#if VHBU_START_GATE_PROBE
    int result = TAI_CONTINUE(int, launch_name_hook_ref,
                              flags, title_id, parameter);
    log_event("start_name_probe_result", result);
    if (title_id != NULL)
        log_text_event("start_name_probe_title", title_id);
    return result;
#else
    if (queue_start_gate_for_title(title_id, 2))
        return 0;
    return TAI_CONTINUE(int, launch_name_hook_ref,
                        flags, title_id, parameter);
#endif
}

static int launch_app_by_name2_hook(const char *title_id,
                                    const char *parameter,
                                    void *option)
{
#if VHBU_START_GATE_PROBE
    int result = TAI_CONTINUE(int, launch_name2_hook_ref,
                              title_id, parameter, option);
    log_event("start_name2_probe_result", result);
    if (title_id != NULL)
        log_text_event("start_name2_probe_title", title_id);
    return result;
#else
    if (queue_start_gate_for_title(title_id, 3))
        return 0;
    return TAI_CONTINUE(int, launch_name2_hook_ref,
                        title_id, parameter, option);
#endif
}
#endif

static int install_notification_uri_hook(void)
{
    int result = 0;

    launch_uri_hook_id = taiHookFunctionImport(
        &launch_uri_hook_ref, "SceShell", SCE_APPMGR_USER_LIBRARY_NID,
        SCE_APPMGR_LAUNCH_APP_BY_URI_NID, launch_app_by_uri_hook);
    log_event("notification_uri_hook_install", launch_uri_hook_id);
    if (launch_uri_hook_id < 0)
        result = launch_uri_hook_id;
#if VHBU_ENABLE_START_NAME_HOOKS
    launch_name_hook_id = taiHookFunctionImport(
        &launch_name_hook_ref, "SceShell", SCE_APPMGR_USER_LIBRARY_NID,
        SCE_APPMGR_LAUNCH_APP_BY_NAME_NID, launch_app_by_name_hook);
    log_event("start_name_hook_install", launch_name_hook_id);
    if (result >= 0 && launch_name_hook_id < 0)
        result = launch_name_hook_id;
    launch_name2_hook_id = taiHookFunctionImport(
        &launch_name2_hook_ref, "SceShell", SCE_APPMGR_USER_LIBRARY_NID,
        SCE_APPMGR_LAUNCH_APP_BY_NAME2_NID, launch_app_by_name2_hook);
    log_event("start_name2_hook_install", launch_name2_hook_id);
    if (result >= 0 && launch_name2_hook_id < 0)
        result = launch_name2_hook_id;
#endif
    return result;
}
#endif

#if VHBU_ENABLE_CUSTOM_DIALOG
static int livearea_title_matches_active(const void *self)
{
    const volatile unsigned char *title;
    unsigned int index;

    if (self == NULL || ACTIVE_TITLE_ID[9] != '\0')
        return 0;
    title = (const volatile unsigned char *)self + 0x92u;
    for (index = 0; index < 9; ++index) {
        if (ACTIVE_TITLE_ID[index] == '\0' ||
            title[index] != (unsigned char)ACTIVE_TITLE_ID[index])
            return 0;
    }
    return 1;
}
#endif

static int livearea_update_hook(void *self, int action)
{
    int result;
    volatile uint32_t *controller = (volatile uint32_t *)self;
#if VHBU_ENABLE_CUSTOM_DIALOG
    int matching_update = livearea_title_matches_active(self);
#endif

    livearea_update_last_self = (uintptr_t)self;
    livearea_update_last_action = action;
    if (controller != NULL) {
        livearea_update_last_mode = controller[0x104u / 4u];
        livearea_update_last_state = controller[0x108u / 4u];
        livearea_update_last_error = controller[0x10cu / 4u];
        livearea_update_last_completion = controller[0x88u / 4u];
    }
    livearea_update_call_count++;

#if VHBU_ENABLE_CUSTOM_DIALOG
    if (matching_update && action == 2 && test_download_state == 7 &&
        test_update_dialog_active == 0 &&
        test_update_dialog_pending == 0) {
        /* The reference hands a matching action 2 to Shell's action 1
         * cleanup before presenting its own install dialog. */
        livearea_update_reference_handoff_count++;
        (void)TAI_CONTINUE(int, livearea_update_hook_ref, self, 1);
        test_install_from_start_gate = 0;
        test_update_dialog_kind = 2;
        test_update_dialog_pending = 1;
        result = 0;
    } else if (matching_update && action == 2 &&
        test_download_state != 7 &&
        update_xml_ready != 0 && changeinfo_xml_ready != 0 &&
        test_update_dialog_active == 0 &&
        test_update_dialog_pending == 0) {
        /* HomebrewUpdate takes over after Shell's action 1 cleanup, rather
         * than starting the native Sony metadata request for action 2. */
        livearea_update_reference_handoff_count++;
        (void)TAI_CONTINUE(int, livearea_update_hook_ref, self, 1);
        test_download_state = 3;
        test_update_dialog_kind = 1;
        test_update_dialog_pending = 1;
        result = 0;
    } else if (matching_update && action == 0 &&
        test_download_state == 7) {
        uint32_t completion = 0;

        /* An action-0 activation follows Shell's own cleanup path, rather
         * than the action-2 handoff used to take over update lookup. */
        if (controller != NULL) {
            completion = controller[0x88u / 4u];
            controller[0x10cu / 4u] = 0;
            controller[0x88u / 4u] = 0;
        }
        (void)TAI_CONTINUE(int, livearea_update_hook_ref, self, 0);
        if (controller != NULL)
            controller[0x88u / 4u] = completion;
        test_install_from_start_gate = 0;
        test_update_dialog_kind = 2;
        test_update_dialog_pending = 1;
        result = 0;
    } else if (matching_update && action == 0 &&
        test_download_state == 3 &&
        test_update_dialog_active == 0) {
        uint32_t completion = 0;

        /*
         * Reset the native action without manufacturing Sony's action-1
         * server error, then queue custom UI after this hook has returned.
         */
        if (controller != NULL) {
            completion = controller[0x88u / 4u];
            controller[0x10cu / 4u] = 0;
            controller[0x88u / 4u] = 0;
        }
        (void)TAI_CONTINUE(int, livearea_update_hook_ref, self, 0);
        if (controller != NULL)
            controller[0x88u / 4u] = completion;
        test_update_dialog_kind = 1;
        test_update_dialog_pending = 1;
        result = 0;
    } else
#endif
    {
        result = TAI_CONTINUE(int, livearea_update_hook_ref, self, action);
    }
    livearea_update_last_result = result;
    return result;
}

/* Observe the Shell wrapper which forwards LiveArea widget events to the
 * update controller.  A later build can use this exact UI-thread path to
 * publish the custom update control without constructing private PAF types. */
static int livearea_event_hook(int event)
{
    void *object = NULL;
    int result;

    if (identity_shell_text_base != 0) {
        void * const volatile *slot = (void * const volatile *)(
            identity_shell_text_base +
            SCE_SHELL_LIVEAREA_OBJECT_SLOT_OFFSET);
        object = *slot;
    }
    livearea_event_last_value = event;
    livearea_event_last_object = (uintptr_t)object;
    if (object != NULL)
        livearea_event_last_state =
            *(const volatile uint32_t *)((uintptr_t)object + 0x108u);
    livearea_event_call_count++;
    result = TAI_CONTINUE(int, livearea_event_hook_ref, event);
    return result;
}

static __attribute__((unused)) int install_passive_livearea_event_hook(void)
{
    tai_module_info_t shell;
    SceKernelModuleInfo kernel_info;
    const volatile uint8_t *target;
    int result;

    sceClibMemset(&shell, 0, sizeof(shell));
    shell.size = sizeof(shell);
    result = taiGetModuleInfo("SceShell", &shell);
    if (result < 0)
        return result;
    if (shell.module_nid != SCE_SHELL_365_RETAIL_NID)
        return -1;
    sceClibMemset(&kernel_info, 0, sizeof(kernel_info));
    kernel_info.size = sizeof(kernel_info);
    result = sceKernelGetModuleInfo(shell.modid, &kernel_info);
    if (result < 0)
        return result;
    identity_shell_text_base = (uintptr_t)kernel_info.segments[0].vaddr;
    target = (const volatile uint8_t *)(identity_shell_text_base +
        SCE_SHELL_LIVEAREA_EVENT_WRAPPER_OFFSET);
    /* The movw/movt immediate is relocated with SceShell's data segment.
     * Validate only the stable instructions around it. */
    if (target[0] != 0x10 || target[1] != 0xb5 ||
        target[10] != 0x0a || target[11] != 0x68 ||
        target[12] != 0x01 || target[13] != 0x1c ||
        target[14] != 0x10 || target[15] != 0x1c) {
        log_event("livearea_event_signature_mismatch", -1);
        return -1;
    }
    livearea_event_hook_id = taiHookFunctionOffset(
        &livearea_event_hook_ref, shell.modid, 0,
        SCE_SHELL_LIVEAREA_EVENT_WRAPPER_OFFSET, 1,
        livearea_event_hook);
    log_event("livearea_event_hook_install", livearea_event_hook_id);
    return livearea_event_hook_id < 0 ? livearea_event_hook_id : 0;
}

static int appmgr_get_status_by_name_probe(const char *title_id, void *status)
{
    uintptr_t return_address = (uintptr_t)__builtin_return_address(0);
    uintptr_t return_offset =
        (return_address & ~(uintptr_t)1u) - identity_shell_text_base;
    unsigned int index = 0;
    int result = TAI_CONTINUE(int, start_getid_hook_ref, title_id, status);

    start_getid_last_return_offset = return_offset;
    start_getid_last_result = result;
    start_getid_last_title_ptr = (uintptr_t)title_id;
    start_getid_last_status_2c = result >= 0 && status != NULL ?
        *(const int *)((const unsigned char *)status + 0x2c) : -1;
    start_getid_last_app_id = result >= 0 && status != NULL ?
        *(const SceUID *)((const unsigned char *)status + 0x30) : -1;
    if (title_id != NULL) {
        while (index + 1u < sizeof(start_getid_last_title) &&
               title_id[index] != '\0') {
            start_getid_last_title[index] = title_id[index];
            index++;
        }
    }
    start_getid_last_title[index] = '\0';
#if VHBU_ENABLE_CUSTOM_DIALOG
    if (return_offset == 0x29c060u && title_id != NULL &&
        sceClibStrncmp(title_id, ACTIVE_TITLE_ID, 9) == 0 &&
        title_id[9] == '\0')
        start_status_routine_active_title_seen = 1;
#endif
    start_getid_call_count++;
    return result;
}

static int install_start_getid_probe(void)
{
    start_getid_hook_id = taiHookFunctionImport(
        &start_getid_hook_ref, "SceShell", SCE_APPMGR_USER_LIBRARY_NID,
        SCE_APPMGR_GET_STATUS_BY_NAME_NID, appmgr_get_status_by_name_probe);
    log_event("start_status_name_hook_install", start_getid_hook_id);
    return start_getid_hook_id < 0 ? start_getid_hook_id : 0;
}

static int start_command_handler_probe(void *context, const void *command)
{
    uintptr_t return_address = (uintptr_t)__builtin_return_address(0);
    uintptr_t return_offset =
        (return_address & ~(uintptr_t)1u) - identity_shell_text_base;
    const unsigned char *payload = NULL;
    unsigned int payload_size = 0;
    unsigned int index = 0;
    int result;

    if (command != NULL) {
        const unsigned char *bytes = (const unsigned char *)command;

        start_command_last_word0 = *(const unsigned int *)bytes;
        start_command_last_word1 = *(const unsigned int *)(bytes + 4);
        payload = *(const unsigned char * const *)(bytes + 16);
        payload_size = *(const unsigned int *)(bytes + 20);
    } else {
        start_command_last_word0 = 0;
        start_command_last_word1 = 0;
    }
    start_command_last_payload_size = payload_size;
    start_command_last_payload_type = payload != NULL && payload_size >= 4 ?
        *(const unsigned int *)payload : 0;
    if (payload != NULL && payload_size > 4) {
        payload += 4;
        payload_size -= 4;
        while (index + 1u < sizeof(start_command_last_title) &&
               index < payload_size && payload[index] != '\0') {
            start_command_last_title[index] = (char)payload[index];
            index++;
        }
    }
    start_command_last_title[index] = '\0';
    result = TAI_CONTINUE(int, start_command_hook_ref, context, command);
    start_command_last_return_offset = return_offset;
    start_command_last_result = result;
    start_command_last_context = (uintptr_t)context;
    start_command_call_count++;
    return result;
}

static int install_start_command_probe(void)
{
    static const uint8_t expected_signature[] = {
        0x2d, 0xe9, 0xf0, 0x4f, 0xd7, 0xb0,
        0x47, 0xf6, 0x14, 0x22, 0xc8, 0xf2, 0x46, 0x12,
        0x12, 0x68, 0x56, 0x92, 0x29, 0xae, 0x00, 0x27,
        0x52, 0x97, 0x0c, 0x1c, 0x05, 0x1c
    };
    tai_module_info_t shell;
    SceKernelModuleInfo kernel_info;
    const volatile uint8_t *target;
    unsigned int index;
    int result;

    sceClibMemset(&shell, 0, sizeof(shell));
    shell.size = sizeof(shell);
    result = taiGetModuleInfo("SceShell", &shell);
    if (result < 0)
        return result;
    if (shell.module_nid != SCE_SHELL_365_RETAIL_NID)
        return -1;
    sceClibMemset(&kernel_info, 0, sizeof(kernel_info));
    kernel_info.size = sizeof(kernel_info);
    result = sceKernelGetModuleInfo(shell.modid, &kernel_info);
    if (result < 0)
        return result;
    identity_shell_text_base = (uintptr_t)kernel_info.segments[0].vaddr;
    target = (const volatile uint8_t *)(identity_shell_text_base +
        SCE_SHELL_START_COMMAND_HANDLER_OFFSET);
    for (index = 0; index < sizeof(expected_signature); ++index) {
        /* movw/movt at +6..+13 embed a relocated Shell data address. */
        if (index >= 6 && index < 14)
            continue;
        if (target[index] != expected_signature[index]) {
            log_event("start_command_signature_mismatch", (int)index);
            return -1;
        }
    }
    start_command_hook_id = taiHookFunctionOffset(
        &start_command_hook_ref, shell.modid, 0,
        SCE_SHELL_START_COMMAND_HANDLER_OFFSET, 1,
        start_command_handler_probe);
    log_event("start_command_hook_install", start_command_hook_id);
    return start_command_hook_id < 0 ? start_command_hook_id : 0;
}

static int start_status_routine_probe(void *output, void *request,
                                      unsigned int app_id, unsigned int mode,
                                      void *arg4, void *arg5)
{
    uintptr_t return_address = (uintptr_t)__builtin_return_address(0);
    unsigned int index;
    int result;

    start_status_routine_active_title_seen = 0;
    if (request != NULL) {
        const uint32_t *words = (const uint32_t *)request;
        start_status_routine_last_request_ptr = (uintptr_t)request;
        for (index = 0; index < 8; ++index)
            start_status_routine_last_request_words[index] = words[index];
    }
    result = TAI_CONTINUE(int, start_status_routine_hook_ref,
                          output, request, app_id, mode, arg4, arg5);
    start_status_routine_last_return_offset =
        (return_address & ~(uintptr_t)1u) - identity_shell_text_base;
    start_status_routine_last_result = result;
    start_status_routine_last_mode = mode;
    start_status_routine_call_count++;
    return result;
}

static int install_start_status_routine_probe(void)
{
    static const uint8_t expected_signature[] = {
        0x2d, 0xe9, 0xf0, 0x4f, 0xad, 0xf6, 0x5c, 0x7d,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x24, 0x68, 0xcd, 0xf8, 0x58, 0x4f
    };
    tai_module_info_t shell;
    SceKernelModuleInfo kernel_info;
    const volatile uint8_t *target;
    unsigned int index;
    int result;

    sceClibMemset(&shell, 0, sizeof(shell));
    shell.size = sizeof(shell);
    result = taiGetModuleInfo("SceShell", &shell);
    if (result < 0)
        return result;
    if (shell.module_nid != SCE_SHELL_365_RETAIL_NID)
        return -1;
    sceClibMemset(&kernel_info, 0, sizeof(kernel_info));
    kernel_info.size = sizeof(kernel_info);
    result = sceKernelGetModuleInfo(shell.modid, &kernel_info);
    if (result < 0)
        return result;
    identity_shell_text_base = (uintptr_t)kernel_info.segments[0].vaddr;
    target = (const volatile uint8_t *)(identity_shell_text_base +
        SCE_SHELL_START_STATUS_ROUTINE_OFFSET);
    for (index = 0; index < sizeof(expected_signature); ++index) {
        if (index >= 8 && index < 16)
            continue;
        if (target[index] != expected_signature[index]) {
            log_event("start_status_routine_signature_mismatch", (int)index);
            return -1;
        }
    }
    start_status_routine_hook_id = taiHookFunctionOffset(
        &start_status_routine_hook_ref, shell.modid, 0,
        SCE_SHELL_START_STATUS_ROUTINE_OFFSET, 1,
        start_status_routine_probe);
    log_event("start_status_routine_hook_install",
              start_status_routine_hook_id);
    return start_status_routine_hook_id < 0 ?
        start_status_routine_hook_id : 0;
}

static int start_gate_hook(void *request)
{
    typedef const char *(*shell_string_c_str_fn)(const void *);
    const char *title_id = NULL;
    unsigned int index = 0;
    int result;

    if (request != NULL && identity_shell_text_base != 0) {
        shell_string_c_str_fn get_c_str = (shell_string_c_str_fn)(
            identity_shell_text_base + SCE_SHELL_STRING_C_STR_OFFSET);
        title_id = get_c_str((const unsigned char *)request + 0x20u);
    }
    if (title_id != NULL) {
        while (index + 1u < sizeof(start_gate_last_title) &&
               title_id[index] != '\0') {
            start_gate_last_title[index] = title_id[index];
            index++;
        }
    }
    start_gate_last_title[index] = '\0';

#if VHBU_ENABLE_CUSTOM_DIALOG
    if (title_id != NULL &&
        sceClibStrncmp(title_id,
                       VHBU_NOTIFICATION_WORK_TITLE_ID, 9) == 0 &&
        title_id[9] == '\0' &&
        test_download_state == 7) {
        if (test_update_dialog_active == 0 &&
            test_update_dialog_pending == 0 &&
            test_install_phase == 0) {
            test_install_from_start_gate = 0;
            test_update_dialog_kind = 2;
            test_update_dialog_pending = 1;
            start_gate_defer_count++;
            log_event("notification_work_title_action", 1);
        }
        result = 0;
    } else if (title_id != NULL &&
        sceClibStrncmp(title_id, ACTIVE_TITLE_ID, 9) == 0 &&
        title_id[9] == '\0' &&
        test_download_state == 7) {
        if (test_update_dialog_active == 0 &&
            test_update_dialog_pending == 0 &&
            test_install_phase == 0 &&
            test_install_pending == 0) {
            test_install_from_start_gate = 1;
            test_install_pending = 1;
            start_gate_defer_count++;
        }
        result = 0;
    } else
#endif
        result = TAI_CONTINUE(int, start_gate_hook_ref, request);

    start_gate_last_result = result;
    start_gate_call_count++;
    return result;
}

static int install_start_gate_hook(void)
{
    static const uint8_t expected_signature[] = {
        0x2d, 0xe9, 0xf0, 0x4f, 0x8d, 0xb0,
        0x47, 0xf6, 0x14, 0x2b, 0xc8, 0xf2, 0x46, 0x1b,
        0xdb, 0xf8, 0x00, 0x10, 0x0c, 0x91, 0x80, 0x46
    };
    tai_module_info_t shell;
    SceKernelModuleInfo kernel_info;
    const volatile uint8_t *target;
    unsigned int index;
    int result;

    sceClibMemset(&shell, 0, sizeof(shell));
    shell.size = sizeof(shell);
    result = taiGetModuleInfo("SceShell", &shell);
    if (result < 0)
        return result;
    if (shell.module_nid != SCE_SHELL_365_RETAIL_NID)
        return -1;
    sceClibMemset(&kernel_info, 0, sizeof(kernel_info));
    kernel_info.size = sizeof(kernel_info);
    result = sceKernelGetModuleInfo(shell.modid, &kernel_info);
    if (result < 0)
        return result;
    identity_shell_text_base = (uintptr_t)kernel_info.segments[0].vaddr;
    target = (const volatile uint8_t *)(identity_shell_text_base +
        SCE_SHELL_START_GATE_OFFSET);
    for (index = 0; index < sizeof(expected_signature); ++index) {
        if (index >= 6 && index < 14)
            continue;
        if (target[index] != expected_signature[index]) {
            log_event("start_gate_signature_mismatch", (int)index);
            return -1;
        }
    }
    start_gate_hook_id = taiHookFunctionOffset(
        &start_gate_hook_ref, shell.modid, 0,
        SCE_SHELL_START_GATE_OFFSET, 1, start_gate_hook);
    log_event("start_gate_hook_install", start_gate_hook_id);
    return start_gate_hook_id < 0 ? start_gate_hook_id : 0;
}

static int start_lsdb_probe(void *database, void *request,
                            unsigned int key, int *status,
                            unsigned int flags_lo, unsigned int flags_hi)
{
    uintptr_t return_address = (uintptr_t)__builtin_return_address(0);
    int result = TAI_CONTINUE(int, start_lsdb_hook_ref,
                              database, request, key, status,
                              flags_lo, flags_hi);

    if (((return_address & ~(uintptr_t)1u) -
         identity_shell_text_base) == 0x29C8E6u) {
        start_lsdb_last_result = result;
        start_lsdb_last_status = status != NULL ? *status : -1;
        start_lsdb_call_count++;
    }
    return result;
}

static int install_start_lsdb_probe(void)
{
    start_lsdb_hook_id = taiHookFunctionImport(
        &start_lsdb_hook_ref, "SceShell", SCE_LSDB_LIBRARY_NID,
        SCE_LSDB_START_LOOKUP_NID, start_lsdb_probe);
    log_event("start_lsdb_hook_install", start_lsdb_hook_id);
    return start_lsdb_hook_id < 0 ? start_lsdb_hook_id : 0;
}

static int install_passive_livearea_update_hook(void)
{
    static const uint8_t expected_signature[] = {
        0x10, 0xb5, 0x04, 0x1c, 0x94,
        0xf8, 0x18, 0x01, 0x00, 0x28
    };
    tai_module_info_t shell;
    SceKernelModuleInfo kernel_info;
    const volatile uint8_t *target;
    unsigned int index;
    int result;

    sceClibMemset(&shell, 0, sizeof(shell));
    shell.size = sizeof(shell);
    result = taiGetModuleInfo("SceShell", &shell);
    if (result < 0) {
        log_event("livearea_shell_info_failed", result);
        return result;
    }
    if (shell.module_nid != SCE_SHELL_365_RETAIL_NID) {
        log_event("livearea_shell_unsupported", (int)shell.module_nid);
        return -1;
    }
    sceClibMemset(&kernel_info, 0, sizeof(kernel_info));
    kernel_info.size = sizeof(kernel_info);
    result = sceKernelGetModuleInfo(shell.modid, &kernel_info);
    if (result < 0)
        return result;
    target = (const volatile uint8_t *)(
        (uintptr_t)kernel_info.segments[0].vaddr +
        SCE_SHELL_LIVEAREA_UPDATE_OFFSET);
    for (index = 0; index < sizeof(expected_signature); ++index) {
        if (target[index] != expected_signature[index]) {
            log_event("livearea_signature_mismatch", (int)index);
            return -1;
        }
    }
    livearea_update_hook_id = taiHookFunctionOffset(
        &livearea_update_hook_ref, shell.modid, 0,
        SCE_SHELL_LIVEAREA_UPDATE_OFFSET, 1, livearea_update_hook);
    log_event("livearea_update_hook_install", livearea_update_hook_id);
    return livearea_update_hook_id < 0 ? livearea_update_hook_id : 0;
}

static int hbu_aux_a_hook(void *arg0, void *arg1, void *arg2, void *arg3)
{
    char marker_path[160];
    char source_path[160];
    SceIoStat source_stat;
    int completed_task_id = -1;
    int result;
    hbu_aux_a_last_args[0] = (uintptr_t)arg0;
    hbu_aux_a_last_args[1] = (uintptr_t)arg1;
    hbu_aux_a_last_args[2] = (uintptr_t)arg2;
    hbu_aux_a_last_args[3] = (uintptr_t)arg3;
    hbu_aux_a_call_count++;

    /*
     * This is the BGDL export callback used by the reference plugin.  Sony's
     * handler must run first: while it is active SceShell still owns app.db
     * and the task directory.  Entering either one before TAI_CONTINUE
     * deadlocks the shell at the end of a download.
     */
    /* Preserve the complete register contract observed on this firmware.
     * The reference only consumes arg0 after the call, but SceShell's
     * original function also receives the live r1-r3 values. */
    result = TAI_CONTINUE(int, hbu_aux_a_hook_ref,
                          arg0, arg1, arg2, arg3);
    hbu_aux_a_last_result = result;

    if (arg0 != NULL) {
        const volatile int *export_event =
            *(const volatile int * const volatile *)arg0;
        if (export_event != NULL)
            completed_task_id = export_event[0];
    }
    hbu_aux_a_last_task_id = completed_task_id;

    if ((test_download_state == 3 || test_download_state == 4) &&
        completed_task_id > 0 &&
        (test_download_task_id <= 0 ||
         completed_task_id == test_download_task_id)) {
        (void)sceClibSnprintf(source_path, sizeof(source_path),
                              "ux0:bgdl/t/%08x/%s",
                              completed_task_id, ACTIVE_PACKAGE_NAME);
        sceClibMemset(&source_stat, 0, sizeof(source_stat));

        /* The reference additionally parses d0.pdb to recover the filename.
         * Our filename is fixed by the release manifest, so an exact path and
         * byte-size match provide the same guard without parsing private PDB
         * records in SceShell's callback. */
        if (sceIoGetstat(source_path, &source_stat) >= 0 &&
            (uint64_t)source_stat.st_size == ACTIVE_PACKAGE_SIZE) {
            test_download_task_id = completed_task_id;
            test_preexport_move_result =
                sceIoRename(source_path, ACTIVE_PENDING_PATH);
            if (test_preexport_move_result == 0) {
                SceUID marker;
                (void)sceClibSnprintf(marker_path, sizeof(marker_path),
                                      "ux0:bgdl/t/%08x/.installed",
                                      completed_task_id);
                marker = sceIoOpen(marker_path,
                                   SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC,
                                   0666);
                if (marker >= 0)
                    (void)sceIoClose(marker);
                if (bgdl_toast_hook_id < 0) {
                    int toast_result = install_bgdl_toast_hook();
                    log_event("bgdl_toast_active_arm", toast_result);
                }
                test_download_completion_seen = 1;
                test_download_preserved_size = (int)source_stat.st_size;
                /* The service thread verifies, stages, and publishes the
                 * actionable notification after this callback returns. */
                test_download_state = 5;
            }
        }
    }
    return result;
}

static int bgdl_toast_hook(void *task, unsigned int argument)
{
    int active_task_id = test_download_task_id;
    int state = test_download_state;

    if (argument == 0u && task != NULL && active_task_id > 0 &&
        state >= 3 && state <= 6) {
        char marker_path[160];
        SceIoStat status;
        int task_id = *(const int *)task;

        if (task_id == active_task_id) {
            (void)sceClibSnprintf(marker_path, sizeof(marker_path),
                                  "ux0:bgdl/t/%08x/.installed", task_id);
            sceClibMemset(&status, 0, sizeof(status));
            if (sceIoGetstat(marker_path, &status) >= 0)
                return 0;
        }
    }
    return TAI_CONTINUE(int, bgdl_toast_hook_ref, task, argument);
}

static int install_bgdl_toast_hook(void)
{
    static const uint8_t expected_signature[] = {
        0x2d, 0xe9, 0xf0, 0x41, 0x8c, 0xb0
    };
    tai_module_info_t shell;
    SceKernelModuleInfo kernel_info;
    const volatile uint8_t *target;
    unsigned int index;
    int result;

    sceClibMemset(&shell, 0, sizeof(shell));
    shell.size = sizeof(shell);
    result = taiGetModuleInfo("SceShell", &shell);
    if (result < 0 || shell.module_nid != SCE_SHELL_365_RETAIL_NID)
        return -1;
    sceClibMemset(&kernel_info, 0, sizeof(kernel_info));
    kernel_info.size = sizeof(kernel_info);
    result = sceKernelGetModuleInfo(shell.modid, &kernel_info);
    if (result < 0)
        return result;
    target = (const volatile uint8_t *)(
        (uintptr_t)kernel_info.segments[0].vaddr +
        SCE_SHELL_BGDL_TOAST_OFFSET);
    for (index = 0; index < sizeof(expected_signature); ++index) {
        if (target[index] != expected_signature[index]) {
            log_event("bgdl_toast_signature_mismatch", (int)index);
            return -1;
        }
    }
    bgdl_toast_hook_id = taiHookFunctionOffset(
        &bgdl_toast_hook_ref, shell.modid, 0,
        SCE_SHELL_BGDL_TOAST_OFFSET, 1, bgdl_toast_hook);
    log_event("bgdl_toast_hook_install", bgdl_toast_hook_id);
    return bgdl_toast_hook_id < 0 ? bgdl_toast_hook_id : 0;
}

static void release_bgdl_toast_hook(void)
{
    if (bgdl_toast_hook_id >= 0) {
        (void)taiHookRelease(bgdl_toast_hook_id, bgdl_toast_hook_ref);
        bgdl_toast_hook_id = -1;
        log_event("bgdl_toast_hook_release", 0);
    }
}

static __attribute__((unused)) int hbu_aux_b_hook(
    void *arg0, void *arg1, void *arg2, void *arg3)
{
    int result;
    hbu_aux_b_last_args[0] = (uintptr_t)arg0;
    hbu_aux_b_last_args[1] = (uintptr_t)arg1;
    hbu_aux_b_last_args[2] = (uintptr_t)arg2;
    hbu_aux_b_last_args[3] = (uintptr_t)arg3;
    hbu_aux_b_call_count++;
    result = TAI_CONTINUE(int, hbu_aux_b_hook_ref,
                          arg0, arg1, arg2, arg3);
    /*
     * HomebrewUpdate accepts this one SceDownload registration response and
     * supplies the first generic-task identity.  Restrict the behavior to the
     * exact interval in which our disposable-title worker is queuing a VPK.
     */
    if ((test_download_state == 3 || test_download_state == 4) &&
        arg1 != NULL &&
        (unsigned int)result == 0x80103A21u) {
        *(volatile int *)arg1 = 1;
        test_download_task_id = 1;
        test_download_state = 4;
        result = 0;
        hbu_aux_b_override_count++;
    }
    hbu_aux_b_last_result = result;
    return result;
}

static __attribute__((unused)) int install_passive_hbu_aux_hooks(void)
{
    static const uint8_t signature_a[] = {
        0x2d, 0xe9, 0xf0, 0x4f, 0xad, 0xf5, 0x47, 0x7d, 0xad, 0xf5
    };
    static const uint8_t signature_b[] = {
        0x2d, 0xe9, 0xf0, 0x4f, 0x83, 0xb0, 0x00, 0x24, 0x12, 0x68
    };
    tai_module_info_t shell;
    SceKernelModuleInfo kernel_info;
    const volatile uint8_t *target_a;
    const volatile uint8_t *target_b;
    unsigned int index;
    int result;

    sceClibMemset(&shell, 0, sizeof(shell));
    shell.size = sizeof(shell);
    result = taiGetModuleInfo("SceShell", &shell);
    if (result < 0 || shell.module_nid != SCE_SHELL_365_RETAIL_NID)
        return -1;
    sceClibMemset(&kernel_info, 0, sizeof(kernel_info));
    kernel_info.size = sizeof(kernel_info);
    result = sceKernelGetModuleInfo(shell.modid, &kernel_info);
    if (result < 0)
        return result;
    target_a = (const volatile uint8_t *)(
        (uintptr_t)kernel_info.segments[0].vaddr + SCE_SHELL_HBU_AUX_A_OFFSET);
    target_b = (const volatile uint8_t *)(
        (uintptr_t)kernel_info.segments[0].vaddr + SCE_SHELL_HBU_AUX_B_OFFSET);
    for (index = 0; index < sizeof(signature_a); ++index) {
        if (target_a[index] != signature_a[index]) {
            log_event("hbu_aux_a_signature_mismatch", (int)index);
            return -1;
        }
    }
    for (index = 0; index < sizeof(signature_b); ++index) {
        if (target_b[index] != signature_b[index]) {
            log_event("hbu_aux_b_signature_mismatch", (int)index);
            return -1;
        }
    }
    hbu_aux_a_hook_id = taiHookFunctionOffset(
        &hbu_aux_a_hook_ref, shell.modid, 0,
        SCE_SHELL_HBU_AUX_A_OFFSET, 1, hbu_aux_a_hook);
    log_event("hbu_aux_a_hook_install", hbu_aux_a_hook_id);
#if VHBU_ENABLE_EXPLORATORY_HOOKS || VHBU_ENABLE_CUSTOM_DIALOG
    hbu_aux_b_hook_id = taiHookFunctionOffset(
        &hbu_aux_b_hook_ref, shell.modid, 0,
        SCE_SHELL_HBU_AUX_B_OFFSET, 1, hbu_aux_b_hook);
    log_event("hbu_aux_b_hook_install", hbu_aux_b_hook_id);
    return (hbu_aux_a_hook_id < 0 || hbu_aux_b_hook_id < 0) ? -1 : 0;
#else
    return hbu_aux_a_hook_id < 0 ? hbu_aux_a_hook_id : 0;
#endif
}

static __attribute__((unused)) int install_passive_game_update_hook(void)
{
    static const uint8_t expected_prologue[] = {
        0x2d, 0xe9, 0xf0, 0x47, 0xd2, 0xb0
    };
    static const uint8_t expected_body[] = {
        0xda, 0xf8, 0x00, 0x10, 0x50, 0x91, 0x07, 0x1c,
        0x17, 0xf1, 0x24, 0x04
    };
    tai_module_info_t shell;
    SceKernelModuleInfo kernel_info;
    const volatile uint8_t *target;
    unsigned int index;
    int result;

    sceClibMemset(&shell, 0, sizeof(shell));
    shell.size = sizeof(shell);
    result = taiGetModuleInfo("SceShell", &shell);
    if (result < 0) {
        log_event("shell_info_failed", result);
        return result;
    }
    log_event("shell_module_nid", (int)shell.module_nid);
    if (shell.module_nid != SCE_SHELL_365_RETAIL_NID) {
        log_event("shell_unsupported", (int)shell.module_nid);
        return -1;
    }
    sceClibMemset(&kernel_info, 0, sizeof(kernel_info));
    kernel_info.size = sizeof(kernel_info);
    result = sceKernelGetModuleInfo(shell.modid, &kernel_info);
    if (result < 0) {
        log_event("kernel_module_info_failed", result);
        return result;
    }
    if (SCE_SHELL_GAME_UPDATE_JOB_OFFSET + 0x0Eu +
        sizeof(expected_body) >
        kernel_info.segments[0].memsz) {
        log_event("hook_offset_out_of_range", -1);
        return -1;
    }
    target = (const volatile uint8_t *)(
        (uintptr_t)kernel_info.segments[0].vaddr +
        SCE_SHELL_GAME_UPDATE_JOB_OFFSET);
    for (index = 0; index < sizeof(expected_prologue); ++index) {
        if (target[index] != expected_prologue[index]) {
            log_event("hook_signature_mismatch", (int)index);
            return -1;
        }
    }
    for (index = 0; index < sizeof(expected_body); ++index) {
        if (target[0x0Eu + index] != expected_body[index]) {
            log_event("hook_body_signature_mismatch", (int)index);
            return -1;
        }
    }
    game_update_hook_id = taiHookFunctionOffset(
        &game_update_hook_ref, shell.modid, 0,
        SCE_SHELL_GAME_UPDATE_JOB_OFFSET, 1, game_update_job_hook);
    log_event("game_update_hook_install", game_update_hook_id);
    return game_update_hook_id < 0 ? game_update_hook_id : 0;
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

static void fill_network_ops(vhbu_net_ops *network)
{
    sceClibMemset(network, 0, sizeof(*network));
    network->socket_create = net_socket;
    network->connect_socket = net_connect;
    network->receive = net_recv;
    network->send_data = net_send;
    network->set_option = net_setsockopt;
    network->close_socket = net_socket_close;
    network->resolver_create = net_resolver_create;
    network->resolver_start_ntoa = net_resolver_start_ntoa;
    network->resolver_destroy = net_resolver_destroy;
}

static void initialize_active_update(void)
{
    vhbu_update_config_clear(&active_update);
}

static int load_update_ini(const char *directory_title,
                           VhbuUpdateConfig *output)
{
    static char ini_text[4096];
    VhbuUpdateConfig parsed;
    char path[192];
    char sfo_path[192];
    SceUID file;
    int received;
    int result;

    if (directory_title == NULL || output == NULL)
        return -1;
    vhbu_update_config_clear(&parsed);
    result = sceClibSnprintf(path, sizeof(path),
        "ux0:app/%s/sce_sys/homebrew_update.ini", directory_title);
    if (result <= 0 || result >= (int)sizeof(path))
        return -1;
    file = sceIoOpen(path, SCE_O_RDONLY, 0);
    if (file < 0)
        return file;
    received = sceIoRead(file, ini_text, sizeof(ini_text) - 1u);
    (void)sceIoClose(file);
    if (received <= 0 || received >= (int)sizeof(ini_text))
        return -2;
    ini_text[received] = '\0';
    result = vhbu_update_config_parse_ini(
        &parsed, ini_text, (size_t)received);
    if (result < 0)
        return result;
    if (sceClibStrncmp(parsed.title_id, directory_title, 9u) != 0 ||
        parsed.title_id[9] != '\0')
        return -7;
    result = sceClibSnprintf(sfo_path, sizeof(sfo_path),
        "ux0:app/%s/sce_sys/param.sfo", parsed.title_id);
    if (result <= 0 || result >= (int)sizeof(sfo_path))
        return -3;
    result = read_sfo_text(sfo_path, "APP_VER",
        parsed.installed_version, sizeof(parsed.installed_version));
    if (result < 0)
        return result;
    *output = parsed;
    return 0;
}

static int build_active_local_urls(void)
{
    int result;
    result = sceClibSnprintf(local_update_url, sizeof(local_update_url),
        "http://127.0.0.1:%d/%s-ver.xml", VHBU_STATUS_PORT,
        active_update.title_id);
    if (result <= 0 || result >= (int)sizeof(local_update_url))
        return -1;
    result = sceClibSnprintf(local_package_url, sizeof(local_package_url),
        "http://127.0.0.1:%d/%s", VHBU_STATUS_PORT,
        active_update.package_name);
    if (result <= 0 || result >= (int)sizeof(local_package_url))
        return -2;
    result = sceClibSnprintf(local_changeinfo_url,
        sizeof(local_changeinfo_url),
        "http://127.0.0.1:%d/%s-changeinfo.xml", VHBU_STATUS_PORT,
        active_update.title_id);
    if (result <= 0 || result >= (int)sizeof(local_changeinfo_url))
        return -3;
    return 0;
}

static int activate_configured_update(unsigned int index)
{
    VhbuConfiguredUpdate *entry;
    int length;

    if (index >= configured_update_count)
        return -1;
    entry = &configured_updates[index];
    active_update = entry->config;
    if (build_active_local_urls() < 0)
        return -2;
    length = sceClibSnprintf((char *)update_xml, sizeof(update_xml),
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<titlepatch status=\"alive\" titleid=\"%s\">\n"
        "  <tag name=\"%s_T0\" signoff=\"true\">\n"
        "    <package version=\"%s\" size=\"%llu\" sha1sum=\"%s\" "
        "url=\"%s\" psp2_system_ver=\"56623104\" "
        "content_id=\"%s\">\n"
        "      <paramsfo><title>%s</title></paramsfo>\n"
        "      <changeinfo url=\"%s\"/>\n"
        "    </package>\n"
        "  </tag>\n"
        "</titlepatch>\n",
        ACTIVE_TITLE_ID, ACTIVE_TITLE_ID, ACTIVE_UPDATE_VERSION,
        (unsigned long long)ACTIVE_PACKAGE_SIZE, ACTIVE_PACKAGE_SHA1,
        local_package_url, ACTIVE_CONTENT_ID, ACTIVE_TITLE_ID,
        local_changeinfo_url);
    if (length <= 0 || length >= (int)sizeof(update_xml))
        return -3;
    if (entry->changeinfo_size > sizeof(changeinfo_xml))
        return -4;
    if (entry->changeinfo_size != 0) {
        sceClibMemcpy(changeinfo_xml, entry->changeinfo,
                      entry->changeinfo_size);
        changeinfo_xml_size = entry->changeinfo_size;
    } else {
        static const char empty_changeinfo[] =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<changeinfo><changes><![CDATA[]]></changes></changeinfo>\n";
        sceClibMemcpy(changeinfo_xml, empty_changeinfo,
                      sizeof(empty_changeinfo) - 1u);
        changeinfo_xml_size = sizeof(empty_changeinfo) - 1u;
    }
    update_xml_size = (size_t)length;
    update_xml_ready = 1;
    changeinfo_xml_ready = 1;
    active_update_index = (int)index;
    log_text_event("active_title_id", ACTIVE_TITLE_ID);
    log_text_event("active_update_version", ACTIVE_UPDATE_VERSION);
    return 0;
}

static int discover_configured_updates(void)
{
    vhbu_net_ops network;
    SceIoDirent directory_entry;
    SceUID directory;
    int read_result;
    int first_available = -1;
    int first_configured = -1;

    configured_update_count = 0;
    active_update_index = -1;
    fill_network_ops(&network);
    directory = sceIoDopen("ux0:app");
    if (directory < 0)
        return directory;
    while (configured_update_count < VHBU_MAX_CONFIGURED_APPS) {
        VhbuConfiguredUpdate *entry;
        size_t received_size = 0;
        int result;

        sceClibMemset(&directory_entry, 0, sizeof(directory_entry));
        read_result = sceIoDread(directory, &directory_entry);
        if (read_result <= 0)
            break;
        if (sceClibStrnlen(directory_entry.d_name, 16u) != 9u)
            continue;
        entry = &configured_updates[configured_update_count];
        sceClibMemset(entry, 0, sizeof(*entry));
        result = load_update_ini(directory_entry.d_name, &entry->config);
        if (result < 0)
            continue;
        log_text_event("configured_title", entry->config.title_id);
        result = vhbu_https_get(entry->config.update_url, remote_update_xml,
            sizeof(remote_update_xml) - 1u, &received_size, &network);
        if (result < 0)
            continue;
        remote_update_xml[received_size] = '\0';
        result = vhbu_update_config_parse_xml(&entry->config,
            (const char *)remote_update_xml, received_size);
        if (result < 0 || vhbu_update_config_build_paths(
                &entry->config, VHBU_LOG_DIRECTORY) < 0)
            continue;
        if (first_configured < 0)
            first_configured = (int)configured_update_count;
        if (vhbu_compare_versions(entry->config.installed_version,
                                  entry->config.available_version) < 0) {
            received_size = 0;
            result = vhbu_https_get(entry->config.changeinfo_url,
                entry->changeinfo, sizeof(entry->changeinfo),
                &received_size, &network);
            if (result == 0 && received_size > 0) {
                entry->changeinfo_size = received_size;
                entry->update_available = 1;
                if (first_available < 0)
                    first_available = (int)configured_update_count;
            }
        }
        configured_update_count++;
    }
    (void)sceIoDclose(directory);
    log_event("configured_update_count", (int)configured_update_count);
    if (first_available < 0) {
        if (first_configured < 0) {
            update_xml_ready = 0;
            changeinfo_xml_ready = 0;
            return 1;
        }
        if (activate_configured_update((unsigned int)first_configured) < 0)
            return -1;
        return 1;
    }
    return activate_configured_update((unsigned int)first_available);
}

static int fetch_update_xml(void)
{
    int result;

    initialize_active_update();
    result = discover_configured_updates();
    log_event("update_discovery_result", result);
    if (result == 0) {
        log_event("update_xml_size", (int)update_xml_size);
        log_event("changeinfo_xml_size", (int)changeinfo_xml_size);
    }
    return result;
}

static int serve_update_xml(int client, const char *request)
{
    char expected[96];
    char header[256];
    int expected_length;
    int header_length;

    expected_length = sceClibSnprintf(expected, sizeof(expected),
                                      "GET /%s-ver.xml ",
                                      ACTIVE_TITLE_ID);
    if (expected_length <= 0 || expected_length >= (int)sizeof(expected) ||
        sceClibStrncmp(request, expected, (size_t)expected_length) != 0)
        return 0;
    if (!update_xml_ready || update_xml_size == 0)
        return -1;
    header_length = sceClibSnprintf(
        header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/xml\r\n"
        "Cache-Control: no-store\r\n"
        "Connection: close\r\n"
        "Content-Length: %u\r\n\r\n",
        (unsigned int)update_xml_size);
    if (header_length <= 0 || header_length >= (int)sizeof(header))
        return -1;
    if (send_all(client, header, header_length) < 0 ||
        send_all(client, (const char *)update_xml,
                 (int)update_xml_size) < 0)
        return -1;
    return 1;
}

static int serve_changeinfo_xml(int client, const char *request)
{
    char expected[96];
    char header[256];
    int expected_length;
    int header_length;

    expected_length = sceClibSnprintf(expected, sizeof(expected),
                                      "GET /%s-changeinfo.xml ",
                                      ACTIVE_TITLE_ID);
    if (expected_length <= 0 || expected_length >= (int)sizeof(expected) ||
        sceClibStrncmp(request, expected, (size_t)expected_length) != 0)
        return 0;
    if (!changeinfo_xml_ready || changeinfo_xml_size == 0)
        return -1;
    header_length = sceClibSnprintf(
        header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/xml\r\n"
        "Cache-Control: no-store\r\n"
        "Connection: close\r\n"
        "Content-Length: %u\r\n\r\n",
        (unsigned int)changeinfo_xml_size);
    if (header_length <= 0 || header_length >= (int)sizeof(header))
        return -1;
    if (send_all(client, header, header_length) < 0 ||
        send_all(client, (const char *)changeinfo_xml,
                 (int)changeinfo_xml_size) < 0)
        return -1;
    return 1;
}

typedef struct vhbu_vpk_stream_context {
    int client;
    unsigned long long start;
    unsigned long long total;
    int partial;
    int header_sent;
} vhbu_vpk_stream_context;

static int send_vpk_response_header(vhbu_vpk_stream_context *stream)
{
    char header[384];
    unsigned long long remaining = stream->total - stream->start;
    int length;

    if (stream->partial) {
        length = sceClibSnprintf(
            header, sizeof(header),
            "HTTP/1.1 206 Partial Content\r\n"
            "Content-Type: application/octet-stream\r\n"
            "Accept-Ranges: bytes\r\n"
            "Content-Range: bytes %llu-%llu/%llu\r\n"
            "Content-Length: %llu\r\n"
            "Connection: close\r\n\r\n",
            stream->start, stream->total - 1u, stream->total, remaining);
    } else {
        length = sceClibSnprintf(
            header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/octet-stream\r\n"
            "Accept-Ranges: bytes\r\n"
            "Content-Length: %llu\r\n"
            "Connection: close\r\n\r\n",
            remaining);
    }
    if (length <= 0 || length >= (int)sizeof(header) ||
        send_all(stream->client, header, length) < 0)
        return -1;
    stream->header_sent = 1;
    return 0;
}

static int stream_vpk_chunk(void *context, const unsigned char *data,
                            size_t size)
{
    vhbu_vpk_stream_context *stream =
        (vhbu_vpk_stream_context *)context;
    if (!stream->header_sent && send_vpk_response_header(stream) < 0)
        return -1;
    return send_all(stream->client, (const char *)data, (int)size);
}

static int parse_range_start(const char *request,
                             unsigned long long *range_start)
{
    const char *range = sceClibStrstr(request, "Range: bytes=");
    unsigned long long value = 0;
    int digits = 0;

    if (range == NULL)
        return 0;
    range += sizeof("Range: bytes=") - 1u;
    while (*range >= '0' && *range <= '9') {
        unsigned int digit = (unsigned int)(*range - '0');
        if (value > 1844674407370955161ull)
            return -1;
        value = value * 10u + digit;
        ++range;
        ++digits;
    }
    if (digits == 0 || *range != '-')
        return -1;
    *range_start = value;
    return 1;
}

static int serve_vpk_proxy(int client, const char *request)
{
    char get_request[128];
    char head_request[128];
    vhbu_vpk_stream_context stream;
    vhbu_net_ops network;
    unsigned long long written = 0;
    int get_length;
    int head_length;
    int is_head;
    int range_result;
    int result;

    get_length = sceClibSnprintf(get_request, sizeof(get_request),
                                 "GET /%s ", ACTIVE_PACKAGE_NAME);
    head_length = sceClibSnprintf(head_request, sizeof(head_request),
                                  "HEAD /%s ", ACTIVE_PACKAGE_NAME);
    if (get_length <= 0 || head_length <= 0 ||
        get_length >= (int)sizeof(get_request) ||
        head_length >= (int)sizeof(head_request))
        return -1;
    is_head = sceClibStrncmp(request, head_request,
                             (size_t)head_length) == 0;
    if (!is_head && sceClibStrncmp(request, get_request,
                                   (size_t)get_length) != 0)
        return 0;
    sceClibMemset(&stream, 0, sizeof(stream));
    stream.client = client;
    stream.total = ACTIVE_PACKAGE_SIZE;
    range_result = parse_range_start(request, &stream.start);
    if (range_result < 0 || stream.total == 0 || stream.start >= stream.total)
        return -1;
    stream.partial = range_result > 0;
    if (is_head)
        return send_vpk_response_header(&stream) < 0 ? -1 : 1;
    fill_network_ops(&network);
    result = vhbu_https_stream(ACTIVE_PACKAGE_URL, stream.start,
                               stream_vpk_chunk, &stream, &written,
                               &network);
    log_event("vpk_proxy_stream_result", result);
    log_event("vpk_proxy_stream_size", (int)written);
    if (result < 0 || written != stream.total - stream.start)
        return -1;
    return 1;
}

static int serve_progress_debug(int client, const char *request)
{
    char response[256];
    char body[96];
    int body_length;
    int response_length;

    if (sceClibStrncmp(request, "GET /debug/progress/close ", 26) == 0) {
        if (test_progress_ui_state == 2)
            test_progress_ui_close_pending = 1;
    } else if (sceClibStrncmp(
                   request, "GET /debug/progress/fill ", 25) == 0) {
#if VHBU_ENABLE_CUSTOM_DIALOG
        queue_progress_value(100);
#endif
    } else if (sceClibStrncmp(request, "GET /debug/progress ", 20) == 0) {
        if (test_progress_ui_state == 0 &&
            test_progress_ui_pending == 0 &&
            test_update_dialog_active == 0) {
            test_progress_ui_fallback_allowed = 0;
            test_progress_ui_pending = 1;
        }
    } else {
        return 0;
    }

    body_length = sceClibSnprintf(
        body, sizeof(body),
        "{\"state\":%d,\"result\":%d,\"pending\":%d,\"icon\":%d}",
        (int)test_progress_ui_state,
        (int)test_progress_ui_result,
        (int)test_progress_ui_pending,
        (int)test_progress_icon_result);
    if (body_length <= 0 || body_length >= (int)sizeof(body))
        return -1;
    response_length = sceClibSnprintf(
        response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Cache-Control: no-store\r\n"
        "Connection: close\r\n"
        "Content-Length: %d\r\n\r\n%s",
        body_length, body);
    if (response_length <= 0 || response_length >= (int)sizeof(response))
        return -1;
    return send_all(client, response, response_length) < 0 ? -1 : 1;
}

static int parse_configured_status_title(const char *request)
{
    static const char prefix[] = "GET /status/";
    char title_id[10];
    unsigned int configured_index;
    unsigned int index;

    if (sceClibStrncmp(request, prefix, sizeof(prefix) - 1u) != 0)
        return 0;
    for (index = 0; index < 9u; ++index) {
        char character = request[sizeof(prefix) - 1u + index];
        if (character == '\0' || character == ' ' || character == '/')
            return -1;
        title_id[index] = character;
    }
    if (request[sizeof(prefix) - 1u + 9u] != ' ')
        return -1;
    title_id[9] = '\0';
    for (configured_index = 0;
         configured_index < configured_update_count;
         ++configured_index) {
        if (sceClibStrncmp(
                configured_updates[configured_index].config.title_id,
                title_id, sizeof(title_id)) == 0)
            return 1;
    }
    return -1;
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
    int result;
    int timeout = 250000;

    (void)net_setsockopt(client, SCE_NET_SOL_SOCKET, SCE_NET_SO_RCVTIMEO,
                         &timeout, sizeof(timeout));
    (void)net_setsockopt(client, SCE_NET_SOL_SOCKET, SCE_NET_SO_SNDTIMEO,
                         &timeout, sizeof(timeout));
    received = net_recv(client, request, sizeof(request) - 1u, 0);
    if (received <= 0)
        return;
    request[received] = '\0';
    result = serve_update_xml(client, request);
    if (result != 0) {
        log_event("update_xml_serve_result", result);
        return;
    }
    result = serve_changeinfo_xml(client, request);
    if (result != 0) {
        log_event("changeinfo_xml_serve_result", result);
        return;
    }
    result = serve_vpk_proxy(client, request);
    if (result != 0) {
        log_event("vpk_proxy_serve_result", result);
        return;
    }
    result = serve_progress_debug(client, request);
    if (result != 0) {
        log_event("progress_debug_serve_result", result);
        return;
    }
    result = parse_configured_status_title(request);
    if (sceClibStrncmp(request, "GET /status ", 12) != 0 && result != 1) {
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

static int download_queue_thread(SceSize argument_size, void *arguments)
{
    (void)argument_size;
    (void)arguments;

    log_event("download_thread_started", 0);
    while (service_running) {
#if VHBU_ENABLE_CUSTOM_DIALOG
        if (test_download_queue_pending != 0 &&
            test_update_dialog_kind == 3 &&
            test_update_dialog_active == 2) {
            lau_bgdl_result detail;
            char download_title[192];
            int queue_result;

            /* HomeBrewUpdate keeps its local VPK proxy alive on a different
             * thread while SceDownload registers the task.  Registration can
             * synchronously connect to the loopback URL, so running both on
             * one worker deadlocks SceShell. */
            test_download_queue_pending = 0;
            /* Do not open Shell's notification database while its update
             * controller is dismissing the prompt.  The preserved crash log
             * proved this read can wait forever on Shell's database lock.
             * Native completion gives us the task ID; notification rewriting
             * selects the newest matching event when the baseline is zero. */
            log_event("test_notification_baseline_skipped", 0);
            detail.api_result = -1;
            detail.service_result = 0;
            detail.task_id = -1;
            (void)sceClibSnprintf(download_title, sizeof(download_title),
                "%s %s", ACTIVE_NAME, ACTIVE_UPDATE_VERSION);
            queue_result = lau_bgdl_enqueue(
                LAU_BGDL_VPK, download_title,
                local_package_url, ACTIVE_ICON_PATH, &detail);
            test_download_queue_result = queue_result;
            test_download_queue_service_result = detail.service_result;
            test_download_task_id = detail.task_id;
            log_event("test_vpk_queue_result", queue_result);
            log_event("test_vpk_queue_api_result", detail.api_result);
            log_event("test_vpk_queue_service_result",
                      detail.service_result);
            log_event("test_vpk_queue_task_id", detail.task_id);
            if (queue_result >= 0 && detail.task_id > 0) {
                int toast_result = install_bgdl_toast_hook();
                log_event("bgdl_toast_active_arm", toast_result);
                if (toast_result < 0)
                    queue_result = toast_result;
            }
            test_download_state = queue_result < 0 ? 8 : 4;
            /* change_state(0x12340007) has committed the task by the time
             * lau_bgdl_enqueue returns.  Close "Please wait..." only after
             * that native registration step, so the notification entry is
             * ready when control returns to LiveArea. */
            test_busy_dialog_close_pending = 1;
        }
#endif
        sceKernelDelayThread(10u * 1000u);
    }
    log_event("download_thread_stopped", 0);
    return 0;
}

static int status_server_thread(SceSize argument_size, void *arguments)
{
    SceNetSockaddrIn address;
    int reuse = 1;
    int nonblocking = 1;
    int attempt;
    int result;
    int all_hooks_ready = 1;
    unsigned int logged_game_update_count = 0;
    unsigned int logged_patch_check_ctor_count = 0;
    unsigned int logged_patch_check_callback_count = 0;
    unsigned int logged_patch_plugin_request_count = 0;
    unsigned int logged_livearea_update_count = 0;
    unsigned int logged_livearea_event_count = 0;
    unsigned int logged_start_getid_count = 0;
    unsigned int logged_start_command_count = 0;
    unsigned int logged_start_status_routine_count = 0;
    unsigned int logged_start_gate_count = 0;
    unsigned int logged_start_lsdb_count = 0;
    unsigned int logged_hbu_aux_a_count = 0;
    unsigned int logged_hbu_aux_b_count = 0;
    unsigned int logged_http_connection_count = 0;
    unsigned int logged_http_request_count = 0;
    unsigned int logged_http_send_count = 0;
    unsigned int logged_identity_call_count = 0;
    unsigned int logged_identity_substitution_count = 0;
    unsigned int logged_helper_launch_count = 0;
    (void)argument_size;
    (void)arguments;

    log_event("server_thread_started", 0);
    if (VHBU_ENABLE_STATUS_SERVER) {
        for (attempt = 0;
             service_running && attempt < VHBU_NET_RETRY_COUNT;
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
        (void)net_setsockopt(listen_socket, SCE_NET_SOL_SOCKET,
                             SCE_NET_SO_NBIO, &nonblocking,
                             sizeof(nonblocking));
        sceClibMemset(&address, 0, sizeof(address));
        address.sin_len = sizeof(address);
        address.sin_family = SCE_NET_AF_INET;
        address.sin_port = host_to_network16(VHBU_STATUS_PORT);
        /* The update proxy continues to advertise its loopback URL, while
         * binding the diagnostic listener on all local interfaces also lets
         * the development PC trigger the read-only UI probe. */
        address.sin_addr.s_addr = host_to_network32(SCE_NET_INADDR_ANY);
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
    }
    {
        int hook_result;
        if (!VHBU_ENABLE_STATUS_SERVER)
            log_event("status_listener_disabled", 0);
        /*
         * Wait for SceShell's LiveArea/PAF setup to finish before installing
         * the hooks that HomeBrewUpdate installs from its worker.  Installing
         * these from module_start can succeed and then be displaced by later
         * Shell initialization.
         */
        sceKernelDelayThread(8u * 1000u * 1000u);
        hook_result = fetch_update_xml();
        if (hook_result < 0)
            all_hooks_ready = 0;
        hook_result = livearea_update_hook_id >= 0 ? 0 :
            install_passive_livearea_update_hook();
        log_event("livearea_update_delayed_arm", hook_result);
        if (hook_result < 0)
            all_hooks_ready = 0;
#if VHBU_ENABLE_EXPLORATORY_HOOKS || VHBU_START_CONTROLLER_PROBE
        hook_result = install_passive_livearea_event_hook();
        log_event("livearea_event_delayed_arm", hook_result);
#endif
#if VHBU_START_CONTROLLER_PROBE
        hook_result = install_start_getid_probe();
        log_event("start_status_name_delayed_arm", hook_result);
        if (hook_result < 0)
            all_hooks_ready = 0;
        hook_result = install_start_command_probe();
        log_event("start_command_delayed_arm", hook_result);
        if (hook_result < 0)
            all_hooks_ready = 0;
        hook_result = install_start_status_routine_probe();
        log_event("start_status_routine_delayed_arm", hook_result);
        if (hook_result < 0)
            all_hooks_ready = 0;
        hook_result = install_start_gate_hook();
        log_event("start_gate_delayed_arm", hook_result);
        if (hook_result < 0)
            all_hooks_ready = 0;
        hook_result = install_start_lsdb_probe();
        log_event("start_lsdb_delayed_arm", hook_result);
        if (hook_result < 0)
            all_hooks_ready = 0;
#endif
#if VHBU_ENABLE_EXPLORATORY_HOOKS
        hook_result = install_passive_hbu_aux_hooks();
        log_event("hbu_aux_delayed_arm", hook_result);
        hook_result = install_patch_identity_hook();
        log_event("identity_delayed_arm", hook_result);
#endif
#if VHBU_ENABLE_CUSTOM_DIALOG && !VHBU_ENABLE_EXPLORATORY_HOOKS
        hook_result = install_passive_hbu_aux_hooks();
        log_event("hbu_aux_a_dialog_arm", hook_result);
        if (hook_result < 0)
            all_hooks_ready = 0;
#endif
#if VHBU_ENABLE_CUSTOM_DIALOG
        hook_result = install_notification_uri_hook();
        log_event("notification_uri_delayed_arm", hook_result);
        if (hook_result < 0)
            all_hooks_ready = 0;
#endif
        if (VHBU_ENABLE_DELAYED_HTTP_HOOK) {
            http_connection_hook_attempted = 1;
            hook_result = install_passive_http_connection_hook();
            log_event("http_connection_delayed_arm", hook_result);
            if (hook_result < 0)
                all_hooks_ready = 0;
            hook_result = install_passive_http_followup_hooks();
            log_event("http_followup_delayed_arm", hook_result);
            if (hook_result < 0)
                all_hooks_ready = 0;
            /*
             * HomeBrewUpdate applies this firmware-gated HEAD -> GET data
             * injection before a LiveArea check.  SceShell probes the package
             * URL while deciding whether to publish the download control, so
             * waiting until a later queue request is too late for the icon.
             */
            hook_result = install_download_get_injection();
            log_event("download_method_delayed_arm", hook_result);
            if (hook_result < 0)
                all_hooks_ready = 0;
        } else {
            hook_result = 0;
            log_event("http_connection_delayed_arm_disabled", hook_result);
        }
    }

    published_status = all_hooks_ready ? 2 : 1;
    log_event("service_hook_status", published_status);

    result = restore_pending_test_update();
    log_event("pending_restore_result", result);

    while (service_running) {
        int client = VHBU_ENABLE_STATUS_SERVER ?
            net_accept(listen_socket, NULL, NULL) : -1;
        if (test_download_state == 0 &&
            test_update_dialog_active == 0 &&
            ++update_refresh_ticks >= 3000u) {
            int refresh_result;
            update_refresh_ticks = 0;
            refresh_result = discover_configured_updates();
            log_event("periodic_update_discovery_result", refresh_result);
        }
        if (test_download_state == 1) {
            int injection_result;

            /* Arm VPK handling, then wait for the user to tap the update icon. */
            test_download_state = 2;
            test_download_preserve_attempts = 0;
            test_download_task_id = -1;
            test_download_completion_seen = 0;
            test_download_completion_notice_sent = 0;
            test_download_completion_baseline = hbu_aux_a_call_count;
            /* A version-specific destination must not already exist when the
             * export callback performs the same atomic rename as the
             * reference plugin.  This runs on our worker, never in SceShell's
             * callback. */
            (void)sceIoRemove(ACTIVE_PENDING_PATH);
            injection_result = install_download_get_injection();
            log_event("test_vpk_method_injection", injection_result);
            if (injection_result < 0) {
                test_download_state = 8;
                continue;
            }
            (void)sceIoMkdir("ux0:bgdl", 0777);
            (void)sceIoMkdir("ux0:bgdl/t", 0777);
            log_event("test_vpk_waiting_for_icon", 0);
            test_download_state = 3;
        }
#if VHBU_ENABLE_CUSTOM_DIALOG
        if (test_update_dialog_pending != 0 &&
            test_update_dialog_active == 0) {
            test_update_dialog_pending = 0;
            test_update_dialog_button = 0;
            test_update_dialog_confirmed = 0;
            test_update_dialog_init_result = -2;
            test_update_dialog_active = 1;
            vhbu_paf_main_thread_register(test_update_dialog_main_thread,
                                          NULL);
            log_event("test_update_dialog_register", 0);
        }
        if (test_update_dialog_active == 2 &&
            test_update_dialog_init_result != -2) {
            log_event("test_update_dialog_init",
                      test_update_dialog_init_result);
            log_event("test_update_dialog_slot", test_update_dialog_slot);
            test_update_dialog_init_result = -2;
        }
        if (test_update_dialog_active == 3) {
            log_event("test_update_dialog_init",
                      test_update_dialog_init_result);
            log_event("test_update_dialog_button",
                      test_update_dialog_button);
            if (test_update_dialog_confirmed != 0)
                log_event("test_update_dialog_confirmed", 1);
            test_update_dialog_active = 0;
        }
        if (test_busy_dialog_pending != 0 &&
            test_update_dialog_active == 0) {
            test_busy_dialog_pending = 0;
            test_update_dialog_kind = 3;
            test_update_dialog_init_result = -2;
            test_update_dialog_active = 1;
            vhbu_paf_main_thread_register(test_update_dialog_main_thread,
                                          NULL);
            log_event("test_busy_dialog_register", 0);
        }
        if (test_busy_dialog_close_pending != 0) {
            if (test_update_dialog_active == 0) {
                test_busy_dialog_close_pending = 0;
            } else if (test_update_dialog_active == 2 &&
                       (test_update_dialog_kind == 3 ||
                        test_update_dialog_kind == 5)) {
                vhbu_paf_main_thread_register(
                    test_update_dialog_close_main_thread, NULL);
                log_event("test_busy_dialog_close_register", 0);
                test_busy_dialog_close_pending = 0;
            }
        }
        if (test_progress_ui_pending != 0 &&
            test_progress_ui_state == 0) {
            test_progress_ui_pending = 0;
            test_progress_ui_state = 1;
            test_progress_ui_result = -2;
            vhbu_paf_main_thread_register(
                test_progress_ui_open_main_thread, NULL);
            log_event("test_progress_ui_open_register", 0);
        }
        if (test_progress_ui_state == -1) {
            log_event("test_progress_ui_open_result",
                      test_progress_ui_result);
            test_progress_ui_state = 0;
            if (test_progress_ui_fallback_allowed != 0) {
                test_update_dialog_kind = 5;
                test_update_dialog_pending = 1;
                log_event("test_progress_ui_fallback", 1);
            } else {
                log_event("test_progress_ui_fallback", 0);
            }
        }
        if (test_progress_ui_close_pending != 0 &&
            test_progress_ui_state == 2) {
            test_progress_ui_close_pending = 0;
            test_progress_ui_state = 3;
            vhbu_paf_main_thread_register(
                test_progress_ui_close_main_thread, NULL);
            log_event("test_progress_ui_close_register", 0);
        }
#endif
        if ((test_download_state == 3 || test_download_state == 4) &&
            test_download_task_id > 0 &&
            ((test_download_completion_seen != 0) ||
             (hbu_aux_a_call_count > test_download_completion_baseline &&
              hbu_aux_a_last_task_id == test_download_task_id))) {
            log_event("test_preexport_move_result",
                      test_preexport_move_result);
            log_event("test_vpk_capture_scheduled", test_download_task_id);
            test_download_state = 5;
            /* Let SceDownload finish closing and publishing the task files. */
            sceKernelDelayThread(500u * 1000u);
        }
        if (test_download_state == 5) {
            /* Claim completion before doing filesystem work. */
            test_download_state = 6;
            log_event("test_preexport_move_result",
                      test_preexport_move_result);
            if (test_download_completion_notice_sent == 0) {
                int notification_result =
                    publish_download_complete_test_notification();
                log_event("test_download_complete_notification_result",
                          notification_result);
                test_download_completion_notice_sent = 1;
                /* Match the reference's visible completion popup before the
                 * same notification line advances to file verification. */
                sceKernelDelayThread(500u * 1000u);
            }
            test_download_preserve_result = preserve_test_download();
            if (test_download_preserve_result == 0) {
                log_event("test_vpk_preserve_result", 0);
                log_event("test_vpk_preserved_size",
                          test_download_preserved_size);
                disarm_active_download_hooks();
                log_event("late_update_hooks_disarmed", 0);
                {
                    int notification_result =
                        publish_checking_test_notification();
                    log_event("test_checking_notification_result",
                              notification_result);
                }
                test_download_preserve_result =
                    prepare_staged_test_update();
                log_event("test_vpk_prepare_result",
                          test_download_preserve_result);
                if (test_download_preserve_result >= 0) {
                    int notification_result =
                        publish_waiting_test_notification();
                    log_event("test_waiting_notification_result",
                              notification_result);
                    test_download_preserve_result =
                        publish_pending_test_update();
                }
                log_event("test_vpk_pending_result",
                          test_download_preserve_result);
                if (test_download_preserve_result < 0) {
                    discard_staged_test_update(0);
                }
                test_download_state =
                    test_download_preserve_result < 0 ? 8 : 7;
                release_bgdl_toast_hook();
            } else if (++test_download_preserve_attempts < 300u) {
                test_download_state = 5;
                sceKernelDelayThread(100u * 1000u);
            } else {
                log_event("test_vpk_preserve_result",
                          test_download_preserve_result);
                log_event("test_vpk_preserved_size",
                          test_download_preserved_size);
                disarm_active_download_hooks();
                log_event("late_update_hooks_disarmed", 0);
                discard_staged_test_update(0);
                release_bgdl_toast_hook();
                test_download_state = 8;
            }
        }
#if VHBU_ENABLE_CUSTOM_DIALOG
        if (test_install_pending != 0 &&
            test_update_dialog_active == 0) {
            SceUID target_id = -1;
            int running_result;

            test_install_pending = 0;
            running_result = sceAppMgrGetIdByName(
                &target_id, ACTIVE_TITLE_ID);
            log_event("test_target_running_result", running_result);
            log_event("test_target_running_id", target_id);
            if (running_result >= 0 && target_id >= 0) {
                test_update_dialog_kind = 4;
                test_update_dialog_pending = 1;
            } else {
                test_install_phase = 1;
            }
        }
        if (test_close_target_pending != 0 &&
            test_update_dialog_active == 0) {
            unsigned int close_attempt;
            int close_result;

            test_close_target_pending = 0;
            close_result = sceAppMgrDestroyAppByName(ACTIVE_TITLE_ID);
            log_event("test_target_close_result", close_result);
            if (close_result >= 0) {
                for (close_attempt = 0; close_attempt < 150u;
                     ++close_attempt) {
                    SceUID target_id = -1;
                    if (sceAppMgrGetIdByName(
                            &target_id, ACTIVE_TITLE_ID) < 0 ||
                        target_id < 0)
                        break;
                    sceKernelDelayThread(20u * 1000u);
                }
                log_event("test_target_close_wait",
                          (int)close_attempt);
                if (close_attempt < 150u) {
                    sceKernelDelayThread(1500u * 1000u);
                    test_install_phase = 1;
                }
            }
        }
        if (test_install_phase == 1 &&
            test_update_dialog_active == 0 &&
            test_update_dialog_pending == 0) {
            test_progress_ui_fallback_allowed = 1;
            test_progress_ui_pending = 1;
            test_install_phase = 2;
        }
        if (test_install_phase == 2 &&
            ((test_progress_ui_state == 2) ||
             (test_update_dialog_active == 2 &&
              test_update_dialog_kind == 5))) {
            test_install_result = install_staged_test_update();
            log_event("test_install_result", test_install_result);
            if (test_progress_ui_state == 2) {
                if (test_install_result >= 0) {
                    queue_progress_value(100);
                    sceKernelDelayThread(500u * 1000u);
                }
                test_progress_ui_close_pending = 1;
            } else {
                test_busy_dialog_close_pending = 1;
            }
            test_install_phase = 3;
        }
        if (test_install_phase == 3 &&
            test_update_dialog_active == 0 &&
            test_progress_ui_state == 0) {
            if (test_install_result < 0) {
                test_download_state = 7;
                test_install_phase = 0;
                test_install_from_start_gate = 0;
            } else {
                int notification_result =
                    publish_installed_test_notification();
                log_event("test_installed_notification_result",
                          notification_result);
                test_download_state = 9;
                if (test_install_from_start_gate != 0)
                    test_launch_target_pending = 1;
                else {
                    test_update_dialog_kind = 6;
                    test_update_dialog_pending = 1;
                }
                test_install_phase = 4;
            }
        }
        if (test_install_phase == 4 &&
            test_update_dialog_active == 0 &&
            test_update_dialog_pending == 0) {
            if (test_launch_target_pending != 0) {
                char launch_uri[64];
                int launch_result;
                test_launch_target_pending = 0;
                (void)sceClibSnprintf(
                    launch_uri, sizeof(launch_uri),
                    "psgm:play?titleid=%s", ACTIVE_TITLE_ID);
                launch_result = sceAppMgrLaunchAppByUri(
                    0xFFFFF, launch_uri);
                log_event("test_launch_result", launch_result);
            }
            test_download_state = 0;
            /* Keep serving the just-installed version until the immediate
             * rescan replaces it.  This avoids a failed Refresh request in
             * the short post-install window. */
            update_refresh_ticks = 3000u;
            test_install_phase = 0;
            test_install_from_start_gate = 0;
        }
#endif
        if (game_update_call_count != logged_game_update_count) {
            logged_game_update_count = game_update_call_count;
            log_event("game_update_call_count",
                      (int)logged_game_update_count);
            log_event("game_update_last_self",
                      (int)game_update_last_self);
            log_event("game_update_last_result",
                      game_update_last_result);
        }
        if (test_helper_launch_count != logged_helper_launch_count) {
            logged_helper_launch_count = test_helper_launch_count;
            log_event("test_helper_launch_result",
                      test_helper_launch_result);
        }
        while (logged_patch_check_ctor_count < patch_check_ctor_call_count) {
            unsigned int field_index;
            unsigned int snapshot_index =
                logged_patch_check_ctor_count % VHBU_CTOR_SNAPSHOT_COUNT;
            char field_name[48];
            logged_patch_check_ctor_count++;
            log_event("patch_ctor_snapshot_call",
                      (int)logged_patch_check_ctor_count);
            log_event("patch_ctor_snapshot_self",
                      (int)patch_check_ctor_snapshot_self[snapshot_index]);
            log_event("patch_ctor_snapshot_result",
                      (int)patch_check_ctor_snapshot_result[snapshot_index]);
            for (field_index = 0; field_index < VHBU_CTOR_SNAPSHOT_WORDS;
                 ++field_index) {
                (void)sceClibSnprintf(field_name, sizeof(field_name),
                                      "patch_ctor_field_%02X",
                                      field_index * 4u);
                log_event(field_name, (int)patch_check_ctor_snapshot_words
                    [snapshot_index][field_index]);
            }
        }
        if (patch_check_callback_call_count !=
            logged_patch_check_callback_count) {
            unsigned int field_index;
            char field_name[48];
            logged_patch_check_callback_count =
                patch_check_callback_call_count;
            log_event("patch_check_callback_call_count",
                      (int)logged_patch_check_callback_count);
            log_event("patch_check_callback_last_self",
                      (int)patch_check_callback_last_self);
            for (field_index = 0; field_index < 10; ++field_index) {
                (void)sceClibSnprintf(field_name, sizeof(field_name),
                                      "patch_callback_field_%02X",
                                      0x1cu + field_index * 4u);
                log_event(field_name,
                          (int)patch_check_callback_fields[field_index]);
            }
            log_event("patch_check_callback_last_result",
                      patch_check_callback_last_result);
        }
        if (patch_plugin_request_call_count !=
            logged_patch_plugin_request_count) {
            logged_patch_plugin_request_count =
                patch_plugin_request_call_count;
            log_event("patch_plugin_request_call_count",
                      (int)logged_patch_plugin_request_count);
            log_event("patch_plugin_request_last_object",
                      (int)patch_plugin_request_last_object);
            log_event("patch_plugin_request_last_result",
                      patch_plugin_request_last_result);
        }
        if (livearea_update_call_count != logged_livearea_update_count) {
            logged_livearea_update_count = livearea_update_call_count;
            log_event("livearea_update_call_count",
                      (int)logged_livearea_update_count);
            log_event("livearea_update_last_self",
                      (int)livearea_update_last_self);
            log_event("livearea_update_last_action",
                      livearea_update_last_action);
            log_event("livearea_update_last_mode",
                      (int)livearea_update_last_mode);
            log_event("livearea_update_last_state",
                      (int)livearea_update_last_state);
            log_event("livearea_update_last_error",
                      (int)livearea_update_last_error);
            log_event("livearea_update_last_completion",
                      (int)livearea_update_last_completion);
            log_event("livearea_update_last_result",
                      livearea_update_last_result);
            log_event("livearea_update_reference_handoff_count",
                      (int)livearea_update_reference_handoff_count);
        }
        if (livearea_event_call_count != logged_livearea_event_count) {
            logged_livearea_event_count = livearea_event_call_count;
            log_event("livearea_event_call_count",
                      (int)logged_livearea_event_count);
            log_event("livearea_event_last_value",
                      livearea_event_last_value);
            log_event("livearea_event_last_object",
                      (int)livearea_event_last_object);
            log_event("livearea_event_last_state",
                      (int)livearea_event_last_state);
        }
        if (start_getid_call_count != logged_start_getid_count) {
            logged_start_getid_count = start_getid_call_count;
            log_event("start_status_name_call_count",
                      (int)logged_start_getid_count);
            log_event("start_status_name_return_offset",
                      (int)start_getid_last_return_offset);
            log_event("start_status_name_result", start_getid_last_result);
            log_event("start_status_name_title_ptr",
                      (int)start_getid_last_title_ptr);
            log_event("start_status_name_status_2c",
                      start_getid_last_status_2c);
            log_event("start_status_name_app_id", start_getid_last_app_id);
            log_text_event("start_status_name_title",
                           start_getid_last_title);
        }
        if (start_command_call_count != logged_start_command_count) {
            logged_start_command_count = start_command_call_count;
            log_event("start_command_call_count",
                      (int)logged_start_command_count);
            log_event("start_command_staged_install_count",
                      (int)start_command_staged_install_count);
            log_event("start_command_return_offset",
                      (int)start_command_last_return_offset);
            log_event("start_command_result", start_command_last_result);
            log_event("start_command_context",
                      (int)start_command_last_context);
            log_event("start_command_word0",
                      (int)start_command_last_word0);
            log_event("start_command_word1",
                      (int)start_command_last_word1);
            log_event("start_command_payload_type",
                      (int)start_command_last_payload_type);
            log_event("start_command_payload_size",
                      (int)start_command_last_payload_size);
            log_text_event("start_command_title",
                           start_command_last_title);
        }
        if (start_status_routine_call_count !=
            logged_start_status_routine_count) {
            logged_start_status_routine_count =
                start_status_routine_call_count;
            log_event("start_status_routine_call_count",
                      (int)logged_start_status_routine_count);
            log_event("start_status_routine_return_offset",
                      (int)start_status_routine_last_return_offset);
            log_event("start_status_routine_result",
                      start_status_routine_last_result);
            log_event("start_status_routine_mode",
                      (int)start_status_routine_last_mode);
            log_event("start_status_routine_install_count",
                      (int)start_status_routine_install_count);
            log_event("start_status_request_ptr",
                      (int)start_status_routine_last_request_ptr);
            {
                unsigned int word_index;
                for (word_index = 0; word_index < 8; ++word_index) {
                    char event_name[40];
                    (void)sceClibSnprintf(
                        event_name, sizeof(event_name),
                        "start_status_request_word_%u", word_index);
                    log_event(event_name,
                              (int)start_status_routine_last_request_words[
                                  word_index]);
                }
            }
        }
        if (start_gate_call_count != logged_start_gate_count) {
            logged_start_gate_count = start_gate_call_count;
            log_event("start_gate_call_count",
                      (int)logged_start_gate_count);
            log_event("start_gate_defer_count",
                      (int)start_gate_defer_count);
            log_event("start_gate_last_result", start_gate_last_result);
            log_text_event("start_gate_last_title",
                           start_gate_last_title);
        }
        if (start_lsdb_call_count != logged_start_lsdb_count) {
            logged_start_lsdb_count = start_lsdb_call_count;
            log_event("start_lsdb_call_count",
                      (int)logged_start_lsdb_count);
            log_event("start_lsdb_result", start_lsdb_last_result);
            log_event("start_lsdb_status", start_lsdb_last_status);
        }
        if (hbu_aux_a_call_count != logged_hbu_aux_a_count) {
            logged_hbu_aux_a_count = hbu_aux_a_call_count;
            log_event("hbu_aux_a_call_count", (int)logged_hbu_aux_a_count);
            log_event("hbu_aux_a_arg0", (int)hbu_aux_a_last_args[0]);
            log_event("hbu_aux_a_arg1", (int)hbu_aux_a_last_args[1]);
            log_event("hbu_aux_a_arg2", (int)hbu_aux_a_last_args[2]);
            log_event("hbu_aux_a_arg3", (int)hbu_aux_a_last_args[3]);
            log_event("hbu_aux_a_result", hbu_aux_a_last_result);
            log_event("hbu_aux_a_task_id", hbu_aux_a_last_task_id);
        }
        if (hbu_aux_b_call_count != logged_hbu_aux_b_count) {
            logged_hbu_aux_b_count = hbu_aux_b_call_count;
            log_event("hbu_aux_b_call_count", (int)logged_hbu_aux_b_count);
            log_event("hbu_aux_b_arg0", (int)hbu_aux_b_last_args[0]);
            log_event("hbu_aux_b_arg1", (int)hbu_aux_b_last_args[1]);
            log_event("hbu_aux_b_arg2", (int)hbu_aux_b_last_args[2]);
            log_event("hbu_aux_b_arg3", (int)hbu_aux_b_last_args[3]);
            log_event("hbu_aux_b_result", hbu_aux_b_last_result);
            log_event("hbu_aux_b_override_count",
                      (int)hbu_aux_b_override_count);
        }
        if (http_connection_call_count != logged_http_connection_count) {
            logged_http_connection_count = http_connection_call_count;
            log_event("http_connection_call_count",
                      (int)logged_http_connection_count);
            log_event("http_connection_template",
                      http_connection_last_template);
            log_event("http_connection_keepalive",
                      http_connection_last_keepalive);
            log_event("http_connection_result",
                      http_connection_last_result);
            log_event("http_connection_redirected",
                      http_connection_last_redirected);
            log_text_event("http_connection_url",
                           http_connection_last_url);
            log_text_event("http_connection_effective_url",
                           http_connection_last_effective_url);
        }
        if (http_request_call_count != logged_http_request_count) {
            logged_http_request_count = http_request_call_count;
            log_event("http_request_call_count",
                      (int)logged_http_request_count);
            log_event("http_request_connection",
                      http_request_last_connection);
            log_event("http_request_method", http_request_last_method);
            log_event("http_request_result", http_request_last_result);
            log_text_event("http_request_url", http_request_last_url);
        }
        if (http_send_call_count != logged_http_send_count) {
            logged_http_send_count = http_send_call_count;
            log_event("http_send_call_count", (int)logged_http_send_count);
            log_event("http_send_request", http_send_last_request);
            log_event("http_send_size", (int)http_send_last_size);
            log_event("http_send_result", http_send_last_result);
        }
        if (identity_call_count != logged_identity_call_count ||
            identity_substitution_count !=
                logged_identity_substitution_count) {
            logged_identity_call_count = identity_call_count;
            logged_identity_substitution_count =
                identity_substitution_count;
            log_event("identity_call_count",
                      (int)logged_identity_call_count);
            log_event("identity_candidate_count",
                      (int)identity_candidate_count);
            log_event("identity_substitution_count",
                      (int)logged_identity_substitution_count);
            log_event("identity_last_return_offset",
                      (int)identity_last_return_offset);
            log_event("identity_last_app_id", identity_last_app_id);
            log_event("identity_last_result", identity_last_result);
            log_event("identity_last_candidate",
                      identity_last_candidate);
        }
        if (client < 0) {
            if (!service_running)
                break;
            sceKernelDelayThread(10u * 1000u);
            continue;
        }
        if (VHBU_ENABLE_STATUS_SERVER) {
            serve_client(client);
            (void)net_shutdown(client, SCE_NET_SHUT_RDWR);
            (void)net_socket_close(client);
        }
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
    SceUID session_log;
    (void)argc;
    (void)args;

    (void)sceIoMkdir(VHBU_LOG_DIRECTORY, 0777);
    (void)sceIoRemove(VHBU_PREVIOUS_SESSION_LOG_PATH);
    (void)sceIoRename(VHBU_SESSION_LOG_PATH,
                      VHBU_PREVIOUS_SESSION_LOG_PATH);
    session_log = sceIoOpen(VHBU_SESSION_LOG_PATH,
                            SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
    if (session_log >= 0)
        (void)sceIoClose(session_log);

    if (VHBU_NOOP_ISOLATION) {
        log_event("noop_isolation_start", 0);
        return SCE_KERNEL_START_SUCCESS;
    }
    service_running = 1;
    log_event("module_start", 0);
    /* Report hook error until the firmware-gated LiveArea hook is proven. */
    published_status = 1;
#if VHBU_ENABLE_EXPLORATORY_HOOKS
    (void)install_passive_game_update_hook();
    (void)install_passive_patch_check_ctor_hook();
    (void)install_passive_patch_check_callback_hook();
    (void)install_passive_patch_plugin_request_hook();
#endif
    /* SceShell caches this controller callback while LiveArea initializes.
     * Install it at startup, matching HomeBrewUpdate's proven ordering. */
    (void)install_passive_livearea_update_hook();
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
    download_thread = sceKernelCreateThread("vhbu_download_worker",
                                            download_queue_thread,
                                            0x10000100, 0x10000, 0, 0, NULL);
    if (download_thread < 0) {
        log_event("download_thread_create_failed", download_thread);
        service_running = 0;
        if (listen_socket >= 0 && net_socket_abort != NULL)
            (void)net_socket_abort(listen_socket, 0);
        (void)sceKernelWaitThreadEnd(service_thread, NULL, NULL);
        (void)sceKernelDeleteThread(service_thread);
        service_thread = -1;
        return SCE_KERNEL_START_FAILED;
    }
    result = sceKernelStartThread(download_thread, 0, NULL);
    if (result < 0) {
        log_event("download_thread_start_failed", result);
        (void)sceKernelDeleteThread(download_thread);
        download_thread = -1;
        service_running = 0;
        if (listen_socket >= 0 && net_socket_abort != NULL)
            (void)net_socket_abort(listen_socket, 0);
        (void)sceKernelWaitThreadEnd(service_thread, NULL, NULL);
        (void)sceKernelDeleteThread(service_thread);
        service_thread = -1;
        return SCE_KERNEL_START_FAILED;
    }
    log_event("download_thread_created", download_thread);
    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
    (void)argc;
    (void)args;
    if (VHBU_NOOP_ISOLATION)
        return SCE_KERNEL_STOP_SUCCESS;
    service_running = 0;
    log_event("module_stop", 0);
    if (game_update_hook_id >= 0) {
        (void)taiHookRelease(game_update_hook_id, game_update_hook_ref);
        game_update_hook_id = -1;
    }
    if (patch_check_ctor_hook_id >= 0) {
        (void)taiHookRelease(patch_check_ctor_hook_id,
                             patch_check_ctor_hook_ref);
        patch_check_ctor_hook_id = -1;
    }
    if (patch_check_callback_hook_id >= 0) {
        (void)taiHookRelease(patch_check_callback_hook_id,
                             patch_check_callback_hook_ref);
        patch_check_callback_hook_id = -1;
    }
    if (patch_plugin_request_hook_id >= 0) {
        (void)taiHookRelease(patch_plugin_request_hook_id,
                             patch_plugin_request_hook_ref);
        patch_plugin_request_hook_id = -1;
    }
    if (livearea_update_hook_id >= 0) {
        (void)taiHookRelease(livearea_update_hook_id,
                             livearea_update_hook_ref);
        livearea_update_hook_id = -1;
    }
    if (launch_uri_hook_id >= 0) {
        (void)taiHookRelease(launch_uri_hook_id, launch_uri_hook_ref);
        launch_uri_hook_id = -1;
    }
#if VHBU_ENABLE_START_NAME_HOOKS
    if (launch_name_hook_id >= 0) {
        (void)taiHookRelease(launch_name_hook_id, launch_name_hook_ref);
        launch_name_hook_id = -1;
    }
    if (launch_name2_hook_id >= 0) {
        (void)taiHookRelease(launch_name2_hook_id,
                             launch_name2_hook_ref);
        launch_name2_hook_id = -1;
    }
#endif
    if (start_getid_hook_id >= 0) {
        (void)taiHookRelease(start_getid_hook_id, start_getid_hook_ref);
        start_getid_hook_id = -1;
    }
    if (start_command_hook_id >= 0) {
        (void)taiHookRelease(start_command_hook_id,
                             start_command_hook_ref);
        start_command_hook_id = -1;
    }
    if (start_status_routine_hook_id >= 0) {
        (void)taiHookRelease(start_status_routine_hook_id,
                             start_status_routine_hook_ref);
        start_status_routine_hook_id = -1;
    }
    if (start_gate_hook_id >= 0) {
        (void)taiHookRelease(start_gate_hook_id, start_gate_hook_ref);
        start_gate_hook_id = -1;
    }
    if (start_lsdb_hook_id >= 0) {
        (void)taiHookRelease(start_lsdb_hook_id, start_lsdb_hook_ref);
        start_lsdb_hook_id = -1;
    }
    if (livearea_event_hook_id >= 0) {
        (void)taiHookRelease(livearea_event_hook_id,
                             livearea_event_hook_ref);
        livearea_event_hook_id = -1;
    }
    if (hbu_aux_a_hook_id >= 0) {
        (void)taiHookRelease(hbu_aux_a_hook_id, hbu_aux_a_hook_ref);
        hbu_aux_a_hook_id = -1;
    }
    if (hbu_aux_b_hook_id >= 0) {
        (void)taiHookRelease(hbu_aux_b_hook_id, hbu_aux_b_hook_ref);
        hbu_aux_b_hook_id = -1;
    }
    release_bgdl_toast_hook();
    if (identity_hook_id >= 0) {
        (void)taiHookRelease(identity_hook_id, identity_hook_ref);
        identity_hook_id = -1;
        identity_shell_text_base = 0;
    }
    release_all_late_update_hooks();
    if (listen_socket >= 0 && net_socket_abort != NULL)
        (void)net_socket_abort(listen_socket, 0);
    if (service_thread >= 0) {
        (void)sceKernelWaitThreadEnd(service_thread, NULL, NULL);
        (void)sceKernelDeleteThread(service_thread);
        service_thread = -1;
    }
    if (download_thread >= 0) {
        (void)sceKernelWaitThreadEnd(download_thread, NULL, NULL);
        (void)sceKernelDeleteThread(download_thread);
        download_thread = -1;
    }
    return SCE_KERNEL_STOP_SUCCESS;
}
