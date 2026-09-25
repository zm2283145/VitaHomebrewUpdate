#ifndef VHBU_NOTIFICATION_DB_H
#define VHBU_NOTIFICATION_DB_H

#include <stdint.h>

#define VHBU_NOTIFICATION_NOT_FOUND (-1012)

int vhbu_notification_max_rowid(int64_t *rowid);

int vhbu_notification_suppress_native_task(int task_id, int *rows_deleted);

int vhbu_notification_publish_checking(const char *title_id,
                                       int64_t after_rowid,
                                       int64_t *event_rowid);

int vhbu_notification_publish_waiting(const char *title_id,
                                      int task_id,
                                      const char *display_title,
                                      int64_t after_rowid,
                                      const char *icon_path,
                                      int64_t *event_rowid);

int vhbu_notification_publish_install_complete(const char *title_id,
                                               int64_t event_rowid);

int vhbu_notification_publish_failure(const char *title_id,
                                      int64_t event_rowid,
                                      const char *description);

int vhbu_notification_delete_event(const char *title_id,
                                   int64_t event_rowid);

#endif
