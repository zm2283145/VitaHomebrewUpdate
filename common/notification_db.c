#include "notification_db.h"

#include <psp2/io/fcntl.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/threadmgr.h>
#include <stdint.h>
#include <taihen.h>

typedef struct sqlite3 sqlite3;
typedef int (*sqlite_open_v2_fn)(const char *, sqlite3 **, int, const char *);
typedef int (*sqlite_close_fn)(sqlite3 *);
typedef int (*sqlite_busy_timeout_fn)(sqlite3 *, int);
typedef int (*sqlite_exec_fn)(sqlite3 *, const char *,
                              int (*)(void *, int, char **, char **),
                              void *, char **);
typedef void (*sqlite_free_fn)(void *);

#define SQLITE_OK 0
#define SQLITE_NOTFOUND 12
#define SQLITE_OPEN_READWRITE 2
#define VHBU_SHELL_DB "ur0:shell/db/app.db"
#define VHBU_NOTIFICATION_EXEC_TITLE "NPXS10004"
#define VHBU_NOTIFICATION_URI_PREFIX "photo:browse?category=ALL&hbu="
#define VHBU_NOTIFICATION_ACT_TYPE 60129542146LL
#define VHBU_NOTIFICATION_ICON_MAX 12288u

/* Private SceSqliteVsh surface used by HomebrewUpdate 1.6. */
#define SQLITE_VSH_LIBRARY_NID 0x4943BF26u
#define SQLITE_VSH_CLOSE_NID 0x3E5BEB88u
#define SQLITE_VSH_BUSY_TIMEOUT_NID 0x1BEBC7B3u
#define SQLITE_VSH_EXEC_NID 0x3E7EF391u
#define SQLITE_VSH_FREE_NID 0x14D27083u
#define SQLITE_VSH_OPEN_V2_NID 0xF67857ACu

static sqlite_open_v2_fn db_open_v2;
static sqlite_close_fn db_close;
static sqlite_busy_timeout_fn db_busy_timeout;
static sqlite_exec_fn db_exec;
static sqlite_free_fn db_free;
static volatile int db_guard;
static unsigned char icon_buffer[VHBU_NOTIFICATION_ICON_MAX];
static char icon_hex[VHBU_NOTIFICATION_ICON_MAX * 2u + 1u];
static char update_sql[VHBU_NOTIFICATION_ICON_MAX * 2u + 2048u];

static int lock_db_guard(void)
{
    unsigned int attempt;
    for (attempt = 0; attempt < 3000u; ++attempt) {
        if (__sync_lock_test_and_set(&db_guard, 1) == 0) {
            __sync_synchronize();
            return 0;
        }
        sceKernelDelayThread(1000u);
    }
    return -2000;
}

static void unlock_db_guard(void)
{
    __sync_synchronize();
    __sync_lock_release(&db_guard);
}

static int db_error(int result)
{
    return result == SQLITE_OK ? 0 : -1000 - result;
}

static int resolve_sqlite_vsh(void)
{
    static const char * const modules[] = {"SceSqliteVsh", "SceSqlite"};
    unsigned int index;
    int result = -1;

    if (db_open_v2 != NULL && db_close != NULL && db_busy_timeout != NULL &&
        db_exec != NULL && db_free != NULL)
        return 0;
    for (index = 0; index < sizeof(modules) / sizeof(modules[0]); ++index) {
        sqlite_open_v2_fn open_fn = NULL;
        sqlite_close_fn close_fn = NULL;
        sqlite_busy_timeout_fn busy_fn = NULL;
        sqlite_exec_fn exec_fn = NULL;
        sqlite_free_fn free_fn = NULL;
        int current;

        current = taiGetModuleExportFunc(modules[index],
            SQLITE_VSH_LIBRARY_NID, SQLITE_VSH_CLOSE_NID,
            (uintptr_t *)&close_fn);
        if (current < 0) { result = current; continue; }
        current = taiGetModuleExportFunc(modules[index],
            SQLITE_VSH_LIBRARY_NID, SQLITE_VSH_BUSY_TIMEOUT_NID,
            (uintptr_t *)&busy_fn);
        if (current < 0) { result = current; continue; }
        current = taiGetModuleExportFunc(modules[index],
            SQLITE_VSH_LIBRARY_NID, SQLITE_VSH_EXEC_NID,
            (uintptr_t *)&exec_fn);
        if (current < 0) { result = current; continue; }
        current = taiGetModuleExportFunc(modules[index],
            SQLITE_VSH_LIBRARY_NID, SQLITE_VSH_FREE_NID,
            (uintptr_t *)&free_fn);
        if (current < 0) { result = current; continue; }
        current = taiGetModuleExportFunc(modules[index],
            SQLITE_VSH_LIBRARY_NID, SQLITE_VSH_OPEN_V2_NID,
            (uintptr_t *)&open_fn);
        if (current < 0) { result = current; continue; }
        db_open_v2 = open_fn;
        db_close = close_fn;
        db_busy_timeout = busy_fn;
        db_exec = exec_fn;
        db_free = free_fn;
        return 0;
    }
    return result;
}

static int open_db(sqlite3 **database)
{
    int result;
    if (database == NULL)
        return -1;
    *database = NULL;
    result = resolve_sqlite_vsh();
    if (result < 0)
        return result;
    result = db_open_v2(VHBU_SHELL_DB, database, SQLITE_OPEN_READWRITE, NULL);
    if (result != SQLITE_OK) {
        if (*database != NULL)
            (void)db_close(*database);
        *database = NULL;
        return db_error(result);
    }
    /* HomebrewUpdate 1.5 uses a 3000 ms timeout and serializes every app.db
     * operation through one updater-owned guard. */
    (void)db_busy_timeout(*database, 3000);
    return 0;
}

static int exec_sql_callback(sqlite3 *database, const char *sql,
    int (*callback)(void *, int, char **, char **), void *context)
{
    char *message = NULL;
    int result = db_exec(database, sql, callback, context, &message);
    if (message != NULL)
        db_free(message);
    return db_error(result);
}

static int exec_sql(sqlite3 *database, const char *sql)
{
    return exec_sql_callback(database, sql, NULL, NULL);
}

static int safe_sql_text(const char *text)
{
    unsigned int index;
    if (text == NULL)
        return 0;
    for (index = 0; text[index] != '\0'; ++index) {
        unsigned char value = (unsigned char)text[index];
        if (index >= 127u || value < 0x20u || value == '\'')
            return 0;
    }
    return index != 0;
}

static int parse_rowid(const char *text, int64_t *value)
{
    int64_t parsed = 0;
    unsigned int digits = 0;
    if (text == NULL || value == NULL)
        return -1;
    while (*text >= '0' && *text <= '9') {
        if (parsed > 922337203685477580LL)
            return -1;
        parsed = parsed * 10 + (*text - '0');
        ++text;
        ++digits;
    }
    if (digits == 0 || *text != '\0')
        return -1;
    *value = parsed;
    return 0;
}

static int rowid_callback(void *context, int columns, char **values,
                          char **names)
{
    (void)names;
    if (context != NULL && columns > 0 && values != NULL && values[0] != NULL)
        (void)parse_rowid(values[0], (int64_t *)context);
    return 0;
}

static int integer_callback(void *context, int columns, char **values,
                            char **names)
{
    int value = 0;
    const char *text;
    (void)names;
    if (context == NULL || columns <= 0 || values == NULL || values[0] == NULL)
        return 0;
    text = values[0];
    while (*text >= '0' && *text <= '9') {
        value = value * 10 + (*text - '0');
        ++text;
    }
    *(int *)context = value;
    return 0;
}

static int find_plain_event_rowid(sqlite3 *database,
                                  const char *title_id,
                                  int64_t after_rowid,
                                  int64_t *event_rowid)
{
    char sql[768];
    int length;
    int result;
    if (!safe_sql_text(title_id) || event_rowid == NULL)
        return -1;
    *event_rowid = 0;
    length = sceClibSnprintf(sql, sizeof(sql),
        "SELECT rowid FROM tbl_newevent WHERE "
        "rowid>%lld AND id_title_id='%s' AND id_item_id='g00' "
        "AND msg_type=258 AND act_type=%lld AND exec_mode=131072 "
        "AND exec_title_id='%s' AND COALESCE(\"desc\",'')="
        "'Waiting to Install' "
        "ORDER BY rowid DESC LIMIT 1;",
        (long long)after_rowid, title_id,
        (long long)VHBU_NOTIFICATION_ACT_TYPE, title_id);
    if (length <= 0 || length >= (int)sizeof(sql))
        return -2;
    result = exec_sql_callback(database, sql, rowid_callback, event_rowid);
    if (result < 0)
        return result;
    return *event_rowid > 0 ? 0 : db_error(SQLITE_NOTFOUND);
}

int vhbu_notification_max_rowid(int64_t *rowid)
{
    sqlite3 *database = NULL;
    int result;
    if (rowid == NULL)
        return -1;
    *rowid = 0;
    result = lock_db_guard();
    if (result < 0)
        return result;
    result = open_db(&database);
    if (result < 0)
        goto done;
    result = exec_sql_callback(database,
        "SELECT COALESCE(MAX(rowid),0) FROM tbl_newevent;",
        rowid_callback, rowid);
    (void)db_close(database);
done:
    unlock_db_guard();
    return result;
}

int vhbu_notification_suppress_native_task(int task_id, int *rows_deleted)
{
    char sql[192];
    sqlite3 *database = NULL;
    int result;

    if (task_id <= 0)
        return -1;
    if (rows_deleted != NULL)
        *rows_deleted = 0;
    result = lock_db_guard();
    if (result < 0)
        return result;
    result = open_db(&database);
    if (result < 0)
        goto done;
    (void)sceClibSnprintf(sql, sizeof(sql),
        "DELETE FROM tbl_newevent WHERE id_title_id='LOGSTATUS0' "
        "AND id_item_id='%d';", task_id);
    result = exec_sql(database, sql);
    if (result < 0)
        goto close;
    if (rows_deleted != NULL) {
        result = exec_sql_callback(database, "SELECT changes();",
                                   integer_callback, rows_deleted);
        if (result < 0)
            goto close;
    }
close:
    (void)db_close(database);
done:
    unlock_db_guard();
    return result;
}

int vhbu_notification_publish_checking(const char *title_id,
                                       int64_t after_rowid,
                                       int64_t *event_rowid)
{
    (void)title_id;
    (void)after_rowid;
    if (event_rowid != NULL)
        *event_rowid = 0;
    return VHBU_NOTIFICATION_NOT_FOUND;
}

int vhbu_notification_publish_waiting(const char *title_id,
                                      int task_id,
                                      const char *display_title,
                                      int64_t after_rowid,
                                      const char *icon_path,
                                      int64_t *event_rowid)
{
    char uri[96];
    sqlite3 *database = NULL;
    SceUID icon_file = -1;
    int icon_size = 0;
    int length;
    int result;
    int rows_changed = 0;
    unsigned int index;
    static const char hex[] = "0123456789ABCDEF";
    if (!safe_sql_text(title_id) || !safe_sql_text(display_title) ||
        icon_path == NULL || task_id <= 0 || event_rowid == NULL)
        return -1;
    length = sceClibSnprintf(uri, sizeof(uri), "%s%s:%d",
                             VHBU_NOTIFICATION_URI_PREFIX, title_id,
                             task_id);
    if (length <= 0 || length >= (int)sizeof(uri))
        return -2;
    icon_file = sceIoOpen(icon_path, SCE_O_RDONLY, 0);
    if (icon_file < 0)
        return icon_file;
    while (icon_size < (int)sizeof(icon_buffer)) {
        int read_result = sceIoRead(icon_file, icon_buffer + icon_size,
                                   sizeof(icon_buffer) - icon_size);
        if (read_result < 0) {
            (void)sceIoClose(icon_file);
            return read_result;
        }
        if (read_result == 0)
            break;
        icon_size += read_result;
    }
    (void)sceIoClose(icon_file);
    if (icon_size <= 0 || icon_size >= (int)sizeof(icon_buffer))
        return -3;
    for (index = 0; index < (unsigned int)icon_size; ++index) {
        icon_hex[index * 2u] = hex[icon_buffer[index] >> 4];
        icon_hex[index * 2u + 1u] = hex[icon_buffer[index] & 0x0fu];
    }
    icon_hex[(unsigned int)icon_size * 2u] = '\0';
    result = lock_db_guard();
    if (result < 0)
        return result;
    result = open_db(&database);
    if (result < 0)
        goto done;
    result = find_plain_event_rowid(database, title_id, after_rowid,
                                    event_rowid);
    if (result < 0)
        goto close;
    length = sceClibSnprintf(update_sql, sizeof(update_sql),
        "UPDATE tbl_newevent SET id_title_id='%s',id_item_id='g00',"
        "del_flag=0,msg_type=258,act_type=%lld,"
        "new_flag=1,exec_mode=458752,exec_title_id='%s',exec_arg='%s',"
        "title=NULL,\"desc\"='%s: Waiting to Install',se_id=0,"
        "icon_data=X'%s' "
        "WHERE rowid=%lld AND id_title_id='%s' AND id_item_id='g00' "
        "AND msg_type=258 AND act_type=%lld AND exec_mode=131072 "
        "AND exec_title_id='%s' AND COALESCE(\"desc\",'')="
        "'Waiting to Install';",
        title_id, (long long)VHBU_NOTIFICATION_ACT_TYPE,
        VHBU_NOTIFICATION_EXEC_TITLE, uri, display_title, icon_hex,
        (long long)*event_rowid, title_id,
        (long long)VHBU_NOTIFICATION_ACT_TYPE, title_id);
    result = length > 0 && length < (int)sizeof(update_sql) ?
        0 : -2;
    if (result < 0)
        goto close;
    result = exec_sql(database, "BEGIN IMMEDIATE;");
    if (result < 0)
        goto close;
    result = exec_sql(database, update_sql);
    if (result < 0)
        goto rollback;
    result = exec_sql_callback(database, "SELECT changes();",
                               integer_callback, &rows_changed);
    if (result < 0)
        goto rollback;
    if (rows_changed != 1) {
        result = -4;
        goto rollback;
    }
    result = exec_sql(database, "COMMIT;");
    if (result < 0)
        (void)exec_sql(database, "ROLLBACK;");
    goto close;
rollback:
    (void)exec_sql(database, "ROLLBACK;");
close:
    (void)db_close(database);
done:
    unlock_db_guard();
    return result;
}

int vhbu_notification_publish_install_complete(const char *title_id,
                                               int64_t event_rowid)
{
    char sql[640];
    sqlite3 *database = NULL;
    int length;
    int result;
    if (!safe_sql_text(title_id) || event_rowid <= 0)
        return -1;
    result = open_db(&database);
    if (result < 0)
        return result;
    length = sceClibSnprintf(sql, sizeof(sql),
        "UPDATE tbl_newevent SET del_flag=0,msg_type=258,act_type=%lld,"
        "new_flag=1,exec_mode=131072,exec_title_id='%s',exec_arg=NULL,"
        "title=NULL,\"desc\"='Install complete',se_id=0,icon_data=NULL "
        "WHERE rowid=%lld AND id_title_id='%s' AND id_item_id='g00';",
        (long long)VHBU_NOTIFICATION_ACT_TYPE, title_id,
        (long long)event_rowid, title_id);
    result = length > 0 && length < (int)sizeof(sql) ?
        exec_sql(database, sql) : -2;
    (void)db_close(database);
    return result;
}

int vhbu_notification_publish_failure(const char *title_id,
                                      int64_t event_rowid,
                                      const char *description)
{
    char sql[768];
    sqlite3 *database = NULL;
    int length;
    int result;
    if (!safe_sql_text(title_id) || !safe_sql_text(description) ||
        event_rowid <= 0)
        return -1;
    result = open_db(&database);
    if (result < 0)
        return result;
    length = sceClibSnprintf(sql, sizeof(sql),
        "UPDATE tbl_newevent SET del_flag=0,msg_type=258,act_type=%lld,"
        "new_flag=1,exec_mode=131072,exec_title_id='%s',exec_arg=NULL,"
        "title=NULL,\"desc\"='%s',se_id=0,icon_data=NULL "
        "WHERE rowid=%lld AND id_title_id='%s' AND id_item_id='g00';",
        (long long)VHBU_NOTIFICATION_ACT_TYPE, title_id, description,
        (long long)event_rowid, title_id);
    result = length > 0 && length < (int)sizeof(sql) ?
        exec_sql(database, sql) : -2;
    (void)db_close(database);
    return result;
}

int vhbu_notification_delete_event(const char *title_id,
                                   int64_t event_rowid)
{
    char sql[320];
    sqlite3 *database = NULL;
    int length;
    int result;
    if (!safe_sql_text(title_id) || event_rowid <= 0)
        return -1;
    result = open_db(&database);
    if (result < 0)
        return result;
    length = sceClibSnprintf(sql, sizeof(sql),
        "DELETE FROM tbl_newevent WHERE rowid=%lld AND id_title_id='%s';",
        (long long)event_rowid, title_id);
    result = length > 0 && length < (int)sizeof(sql) ?
        exec_sql(database, sql) : -2;
    (void)db_close(database);
    return result;
}
