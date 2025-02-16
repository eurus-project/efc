/*
 * This file is part of the efc project <https://github.com/eurus-project/efc/>.
 * Copyright (c) (2025 - Present), The efc developers.
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

/**
 * @brief Initializes the log by creating the log file and adding initial
 * metadata and entering the definitions phase.
 *
 * @param log A pointer to the log instance
 * @param cfg Configuration struct pointer
 *
 * @retval ULOG_SUCCESS - Operation finished successfully
 * @retval ULOG_INVALID_PARAM - Log or configuration struct pointers are not set
 * @retval ULOG_FILESYSTEM_ERROR - An error occurred either while opening the
 * file or while writing to it
 */
ULOG_Error_Type ULOG_Init(ULOG_Inst_Type *log, const ULOG_Config_Type *cfg);

/**
 * @brief Adds a key-value mapping with arbitrary type and size to the metadata.
 *
 * This is usually used for software or hardware metadata. Some common usecases
 * include system name, hardware and software version, toolchain info etc.
 *
 * @param log     A pointer to the log instance
 * @param key     A pointer to the key string, has to be of the following
 *                 format: "<type> <name>" or "<type>[<array_size>] <name>"
 * @param key_len Length of the key string without 0 termination
 * @param val     A pointer to the value variable
 * @param val_len Size of the value in bytes
 *
 * @retval ULOG_SUCCESS - Operation finished successfully
 * @retval ULOG_INVALID_PARAM - Log, key or val pointers are not set
 * @retval ULOG_WRONG_PHASE - The log is not in the definitions phase
 * @retval ULOG_FILESYSTEM_ERROR - An error occurred while writing to the file
 */
ULOG_Error_Type ULOG_AddInfo(ULOG_Inst_Type *log, const char *key,
                             uint8_t key_len, const void *val, size_t val_len);

/**
 * @brief Adds a key-value mapping with a numeric type
 *
 * This is commonly used for logging configurable parameters which can change
 * from flight to flight or during the flight.
 *
 * @note Can be used in the data phase as well.
 *
 * @param log     A pointer to the log instance
 * @param key     A pointer to the key string, has to be of the following
 *                format: "<type> <name>" or "<type>[<array_size>] <name>"
 * @param key_len Length of the key string without 0 termination
 * @param val     A pointer to the value variable of the int32_t or float type
 *
 * @retval ULOG_SUCCESS - Operation finished successfully
 * @retval ULOG_INVALID_PARAM - Log, key or val pointers are not set
 * @retval ULOG_FILESYSTEM_ERROR - An error occurred while writing to the file
 */
ULOG_Error_Type ULOG_AddParameter(ULOG_Inst_Type *log, const char *key,
                                  uint8_t key_len, const void *val);

/**
 * @brief Starts the log data phase
 *
 * @param log A pointer to the log instance
 *
 * @retval ULOG_SUCCESS - Operation finished successfully
 * @retval ULOG_WRONG_PHASE - Log is not in the definitions phase
 */
ULOG_Error_Type ULOG_StartDataPhase(ULOG_Inst_Type *log);

/**
 * @brief Logs a string with a specified severity level
 *
 * @param log     A pointer to the log instance
 * @param string The string to log
 * @param len    Length of the string without 0 termination
 * @param level  Linux-compatible severity level.
 *
 * @retval ULOG_SUCCESS - Operation finished successfully
 * @retval ULOG_INVALID_PARAM - Log or string pointers are not set
 * @retval ULOG_WRONG_PHASE - The log is not in the data phase
 * @retval ULOG_FILESYSTEM_ERROR - An error occurred while writing to the file
 */
ULOG_Error_Type ULOG_LogString(ULOG_Inst_Type *log, const char *string,
                               size_t len, ULOG_Log_Level_Type level);

/**
 * @brief Logs a string with a specified severity level and a numeric tag
 *
 * @param log    A pointer to the log instance
 * @param string The string to log
 * @param len    Length of the string without 0 termination
 * @param level  A linux-compatible severity level
 * @param tag    An integer value corresponding to a tag
 *
 * @retval ULOG_SUCCESS - Operation finished successfully
 * @retval ULOG_INVALID_PARAM - Log or string pointers are not set
 * @retval ULOG_WRONG_PHASE - The log is not in the data phase
 * @retval ULOG_FILESYSTEM_ERROR - An error occurred while writing to the file
 */
ULOG_Error_Type ULOG_LogTaggedString(ULOG_Inst_Type *log, const char *string,
                                     size_t len, ULOG_Log_Level_Type level,
                                     uint16_t tag);

/**
 * @brief Logs a period during which logging was impossible or data was missed
 *
 * @param log         A pointer to the log instance
 * @param duration_ms Time elapsed during which data was lost in milliseconds
 *
 * @retval ULOG_SUCCESS - Operation finished successfully
 * @retval ULOG_INVALID_PARAM - Log pointer is not set
 * @retval ULOG_WRONG_PHASE - The log is not in the data phase
 * @retval ULOG_FILESYSTEM_ERROR - An error occurred while writing to the file
 */
ULOG_Error_Type ULOG_LogDropout(ULOG_Inst_Type *log, uint16_t duration_ms);

/**
 * @brief Adds a sync marker message and flushes data in flight to disk
 *
 * @param log A pointer to the log instance
 *
 * @retval ULOG_SUCCESS - Operation finished successfully
 * @retval ULOG_INVALID_PARAM - Log pointer is not set
 * @retval ULOG_WRONG_PHASE - The log is not in the data phase
 * @retval ULOG_FILESYSTEM_ERROR - An error occurred while writing to the file
 */
ULOG_Error_Type ULOG_Sync(ULOG_Inst_Type *log);

/**
 * @brief Flushes data in flight to disk and closes the log file
 *
 * @param log A pointer to the log instance
 *
 * @retval ULOG_SUCCESS - Operation finished successfully
 * @retval ULOG_INVALID_PARAM - Log pointer is not set
 * @retval ULOG_WRONG_PHASE - The log is not in the data phase
 * @retval ULOG_FILESYSTEM_ERROR - An error occurred while writing to the file
 */
ULOG_Error_Type ULOG_Close(ULOG_Inst_Type *log);

/**
 * @brief Gets current log phase
 *
 * @param log A pointer to the log instance
 *
 * @retval ULOG_PHASE_NONE - The log was not initialized
 * @retval ULOG_PHASE_DEFINITIONS - The log is in the definitions phase
 * @retval ULOG_PHASE_DATA - The log is in the data phase
 */
ULOG_Phase_Type ULOG_GetPhase(ULOG_Inst_Type *log);

#ifdef __cplusplus
}
#endif

#endif // ULOG_H
