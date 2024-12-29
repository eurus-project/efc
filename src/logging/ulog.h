/*
 * This file is part of the efc project <https://github.com/eurus-project/efc/>.
 * Copyright (c) 2024 The efc developers.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ULOG_H
#define ULOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <zephyr/fs/fs_interface.h>

#define ULOG_PROTOCOL_VERSION 1

typedef enum {
    ULOG_SUCCESS = 0,
    ULOG_INVALID_PARAM,
    ULOG_FILESYSTEM_ERROR,
    ULOG_WRONG_PHASE,
} ULOG_Error_Type;

typedef enum {
    ULOG_LOG_LEVEL_EMERG = '0',   // System is unusable
    ULOG_LOG_LEVEL_ALERT = '1',   // Action must be taken immediately
    ULOG_LOG_LEVEL_CRIT = '2',    // Critical conditions
    ULOG_LOG_LEVEL_ERR = '3',     // Error conditions
    ULOG_LOG_LEVEL_WARNING = '4', // Warning conditions
    ULOG_LOG_LEVEL_NOTICE = '5',  // Normal but significant condition
    ULOG_LOG_LEVEL_INFO = '6',    // Informational
    ULOG_LOG_LEVEL_DEBUG = '7',   // Debug-level messages
} ULOG_Log_Level_Type;

typedef enum {
    ULOG_PHASE_NONE = 0,
    ULOG_PHASE_DEFINITIONS,
    ULOG_PHASE_DATA,
} ULOG_Phase_Type;

typedef struct {
    char *filename;
} ULOG_Config_Type;

typedef struct {
    ULOG_Phase_Type phase;
    struct fs_file_t file;
    uint16_t next_msg_id;
} ULOG_Inst_Type;

ULOG_Error_Type ULOG_Init(ULOG_Inst_Type *log, const ULOG_Config_Type *cfg);

ULOG_Error_Type ULOG_AddInfo(ULOG_Inst_Type *log, const char *key,
                             uint8_t key_len, const void *val, size_t val_len);

ULOG_Error_Type ULOG_AddParameter(ULOG_Inst_Type *log, const char *key,
                                  uint8_t key_len, const void *val);

void ULOG_StartDataPhase(ULOG_Inst_Type *log);

ULOG_Error_Type ULOG_LogString(ULOG_Inst_Type *log, const char *string,
                               size_t len, ULOG_Log_Level_Type level);

ULOG_Error_Type ULOG_LogTaggedString(ULOG_Inst_Type *log, const char *string,
                                     size_t len, ULOG_Log_Level_Type level,
                                     uint16_t tag);

ULOG_Error_Type ULOG_LogDropout(ULOG_Inst_Type *log, uint16_t duration_ms);

ULOG_Error_Type ULOG_Sync(ULOG_Inst_Type *log);

ULOG_Error_Type ULOG_Close(ULOG_Inst_Type *log);

ULOG_Phase_Type ULOG_GetPhase(ULOG_Inst_Type *log);

#ifdef __cplusplus
}
#endif

#endif // ULOG_H
