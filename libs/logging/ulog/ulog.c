/*
 * This file is part of the efc project <https://github.com/eurus-project/efc/>.
 * Copyright (c) (2024 - Present), The efc developers.
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

#include "ulog.h"

#include <stdint.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>

typedef struct __attribute__((packed)) {
    uint16_t msg_size;
    uint8_t msg_type;
} Message_Header_Type;

static int WriteHeader(struct fs_file_t *file_h) {
    const uint8_t magic[] = {0x55, 0x4c, 0x6f, 0x67, 0x01, 0x12, 0x35};

    int ret = fs_write(file_h, magic, sizeof(magic));
    if (ret < 0) {
        return ret;
    }

    const uint8_t protocol_version = ULOG_PROTOCOL_VERSION;
    ret = fs_write(file_h, &protocol_version, sizeof(protocol_version));
    if (ret < 0) {
        return ret;
    }

    const uint64_t uptime_us = k_uptime_get() * 1000;
    ret = fs_write(file_h, &uptime_us, sizeof(uptime_us));
    if (ret < 0) {
        return ret;
    }

    return 0;
}

static int WriteFlagBits(struct fs_file_t *file_h, bool has_default_params,
                         bool has_appended_data, uint64_t *appended_offsets) {
    uint8_t compat_flags[8] = {0};
    uint8_t incompat_flags[8] = {0};

    Message_Header_Type header = {
        .msg_type = 'B',
        .msg_size = sizeof(compat_flags) + sizeof(incompat_flags) +
                    sizeof(uint64_t) * 3,
    };

    int ret = fs_write(file_h, &header, sizeof(header));
    if (ret < 0) {
        return ret;
    }

    compat_flags[0] |= has_default_params << 0;
    ret = fs_write(file_h, compat_flags, sizeof(compat_flags));
    if (ret < 0) {
        return ret;
    }

    incompat_flags[0] |= has_appended_data << 0;
    ret = fs_write(file_h, incompat_flags, sizeof(incompat_flags));
    if (ret < 0) {
        return ret;
    }

    ret = fs_write(file_h, appended_offsets, sizeof(uint64_t) * 3);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

static int WriteSync(struct fs_file_t *file_h) {
    const uint8_t sync_magic[] = {0x2f, 0x73, 0x13, 0x20,
                                  0x25, 0x0c, 0xbb, 0x12};

    Message_Header_Type header = {
        .msg_type = 'S',
        .msg_size = sizeof(sync_magic),
    };

    int ret = fs_write(file_h, &header, sizeof(header));
    if (ret < 0) {
        return ret;
    }

    ret = fs_write(file_h, sync_magic, sizeof(sync_magic));
    if (ret < 0) {
        return ret;
    }

    return 0;
}

ULOG_Error_Type ULOG_Init(ULOG_Inst_Type *log, const ULOG_Config_Type *cfg) {
    if (log == NULL || cfg == NULL) {
        return ULOG_INVALID_PARAM;
    }

    fs_file_t_init(&log->file);

    const fs_mode_t flags = FS_O_CREATE | FS_O_WRITE;
    int ret = fs_open(&log->file, cfg->filename, flags);
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    ret = WriteHeader(&log->file);
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    // Currently, default parameters and appended offsets are unsupported
    uint64_t appended_offsets[3] = {0};
    ret = WriteFlagBits(&log->file, false, false, appended_offsets);
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    log->phase = ULOG_PHASE_DEFINITIONS;
    log->next_msg_id = 0;

    return ULOG_SUCCESS;
}

ULOG_Error_Type ULOG_AddInfo(ULOG_Inst_Type *log, const char *key,
                             const uint8_t key_len, const void *val,
                             const size_t val_len) {
    if (log == NULL || key == NULL || val == NULL) {
        return ULOG_INVALID_PARAM;
    }

    if (log->phase != ULOG_PHASE_DEFINITIONS) {
        return ULOG_WRONG_PHASE;
    }

    Message_Header_Type header = {
        .msg_type = 'I', .msg_size = sizeof(uint8_t) + key_len + val_len};

    int ret = fs_write(&log->file, &header, sizeof(header));
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    ret = fs_write(&log->file, &key_len, sizeof(key_len));
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    ret = fs_write(&log->file, key, key_len);
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    ret = fs_write(&log->file, val, val_len);
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    return ULOG_SUCCESS;
}

ULOG_Error_Type ULOG_AddParameter(ULOG_Inst_Type *log, const char *key,
                                  const uint8_t key_len, const void *val) {
    if (log == NULL || key == NULL || val == NULL) {
        return ULOG_INVALID_PARAM;
    }

    Message_Header_Type header = {
        .msg_type = 'P',
        .msg_size =
            sizeof(uint8_t) + key_len + 4 // Size of either int32_t or float
    };

    int ret = fs_write(&log->file, &header, sizeof(header));
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    ret = fs_write(&log->file, &key_len, sizeof(key_len));
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    ret = fs_write(&log->file, key, key_len);
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    ret = fs_write(&log->file, val, 4);
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    return ULOG_SUCCESS;
}

ULOG_Error_Type ULOG_StartDataPhase(ULOG_Inst_Type *log) {
    if (log->phase != ULOG_PHASE_DEFINITIONS) {
        return ULOG_WRONG_PHASE;
    }

    log->phase = ULOG_PHASE_DATA;

    return ULOG_SUCCESS;
}

ULOG_Error_Type ULOG_LogString(ULOG_Inst_Type *log, const char *string,
                               const size_t len,
                               const ULOG_Log_Level_Type level) {
    if (log == NULL || string == NULL) {
        return ULOG_INVALID_PARAM;
    }

    if (log->phase != ULOG_PHASE_DATA) {
        return ULOG_WRONG_PHASE;
    }

    Message_Header_Type header = {
        .msg_type = 'L',
        .msg_size = sizeof(uint8_t) + sizeof(uint64_t) + len,
    };

    int ret = fs_write(&log->file, &header, sizeof(header));
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    const uint8_t log_level = level;
    ret = fs_write(&log->file, &log_level, sizeof(log_level));
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    const uint64_t uptime_us = k_uptime_get() * 1000;
    ret = fs_write(&log->file, &uptime_us, sizeof(uptime_us));
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    ret = fs_write(&log->file, string, len);
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    return ULOG_SUCCESS;
}

ULOG_Error_Type ULOG_LogTaggedString(ULOG_Inst_Type *log, const char *string,
                                     const size_t len,
                                     const ULOG_Log_Level_Type level,
                                     const uint16_t tag) {
    if (log == NULL || string == NULL) {
        return ULOG_INVALID_PARAM;
    }

    if (log->phase != ULOG_PHASE_DATA) {
        return ULOG_WRONG_PHASE;
    }

    Message_Header_Type header = {
        .msg_type = 'C',
        .msg_size = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint64_t) + len,
    };

    int ret = fs_write(&log->file, &header, sizeof(header));
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    const uint8_t log_level = level;
    ret = fs_write(&log->file, &log_level, sizeof(log_level));
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    ret = fs_write(&log->file, &tag, sizeof(tag));
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    const uint64_t uptime_us = k_uptime_get() * 1000;
    ret = fs_write(&log->file, &uptime_us, sizeof(uptime_us));
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    ret = fs_write(&log->file, string, len);
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    return ULOG_SUCCESS;
}

ULOG_Error_Type ULOG_LogDropout(ULOG_Inst_Type *log,
                                const uint16_t duration_ms) {
    if (log == NULL) {
        return ULOG_INVALID_PARAM;
    }

    if (log->phase != ULOG_PHASE_DATA) {
        return ULOG_WRONG_PHASE;
    }

    Message_Header_Type header = {
        .msg_type = 'O',
        .msg_size = sizeof(uint16_t),
    };

    int ret = fs_write(&log->file, &header, sizeof(header));
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    ret = fs_write(&log->file, &duration_ms, sizeof(duration_ms));
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    return ULOG_SUCCESS;
}

ULOG_Error_Type ULOG_Sync(ULOG_Inst_Type *log) {
    if (log == NULL) {
        return ULOG_INVALID_PARAM;
    }

    if (log->phase != ULOG_PHASE_DATA) {
        return ULOG_WRONG_PHASE;
    }

    int ret = WriteSync(&log->file);
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    ret = fs_sync(&log->file);
    return ret == 0 ? ULOG_SUCCESS : ULOG_FILESYSTEM_ERROR;
}

ULOG_Error_Type ULOG_Close(ULOG_Inst_Type *log) {
    if (log == NULL) {
        return ULOG_INVALID_PARAM;
    }

    int ret = WriteSync(&log->file);
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    ret = fs_close(&log->file);
    if (ret < 0) {
        return ULOG_FILESYSTEM_ERROR;
    }

    log->phase = ULOG_PHASE_NONE;

    return ULOG_SUCCESS;
}

ULOG_Phase_Type ULOG_GetPhase(ULOG_Inst_Type *log) { return log->phase; }
