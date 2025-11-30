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

#ifndef ESC_H
#define ESC_H

#include <stdio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>

typedef enum {
    ESC_SUCCESS = 0,
    ESC_DEVICE_PWM_NOT_READY,
    ESC_INVALID_PROTOCOL,
    ESC_INVALID_SPEED_SET,
    ESC_TIMER_OVERFLOW_ERROR,
    ESC_NOT_INITIALIZED,
    ESC_INVALID_STOP,
    ESC_INVALID_ARMING,
} ESC_Error_Type;

typedef enum {
    ESC_PWM = 0,
    ESC_ONESHOT_125,
    ESC_ONESHOT_42,
    ESC_MULTISHOT
} ESC_Protocol_Type;

typedef struct {
    const struct device *pwm_dev;
    uint32_t pwm_channel;
    uint32_t period_us;
    float min_pulse_duration_us;
    float max_pulse_duration_us;
    ESC_Protocol_Type protocol;
    bool initialized;
} ESC_Inst_Type;

/**
 * @brief ESC Initialization function
 * @param[in] pwm_dev     Pointer to the device struct - pwm device
 * @param[in] pwm_channel PWM channel which will be used by ESC device
 * @param[in] protocol    ESC Protocol
 * @param[out] esc_out    Pointer to the ESC out struct, it needs to be passed
 *                        to the ESC_SetSpeed function
 *
 * @retval ESC_SUCCESS - Operation finished successfully
 * @retval ESC_DEVICE_PWM_NOT_READY - PWM device is not initialized correctly
 * @retval ESC_NOT_INITIALIZED - Specified esc is not initialized correctly
 */
ESC_Error_Type ESC_Init(ESC_Inst_Type *esc_out, const struct device *pwm_dev,
                        const uint32_t pwm_channel,
                        const ESC_Protocol_Type protocol);

/**
 * @brief ESC Set Speed by percentage from 0 to 1
 * @param[in] esc   Pointer to the preinitialized ESC out struct from ESC_Init
 * @param[in] speed ESC speed value [0.0f,1.0f]
 *
 * @retval ESC_SUCCESS - Operation finished successfully
 * @retval ESC_INVALID_SPEED_SET - Speed is out of valid range [0-100]
 * @retval ESC_TIMER_OVERFLOW_ERROR - Timer value is overflowed, check prescaler
 * @retval ESC_NOT_INITIALIZED - Specified esc is not initialized correctly
 */
ESC_Error_Type ESC_SetSpeed(const ESC_Inst_Type *esc, const float speed);

/**
 * @brief ESC Stop - Stop ESC signal
 * @param[in] esc Pointer to the preinitialized ESC out struct from ESC_Init
 *
 * @retval ESC_SUCCESS - Operation finished successfully
 * @retval ESC_NOT_INITIALIZED - Specified esc is not initialized correctly
 * @retval ESC_INVALID_STOP - Error in ESC stop command
 */
ESC_Error_Type ESC_Stop(const ESC_Inst_Type *esc);

/**
 * @brief ESC Deinitialize device
 * @param[in] esc Pointer to the preinitialized ESC out struct from ESC_Init
 *
 * @retval ESC_SUCCESS - Operation finished successfully
 * @retval ESC_NOT_INITIALIZED - Specified esc is not initialized correctly
 */
ESC_Error_Type ESC_DeInit(ESC_Inst_Type *esc);

/**
 * @brief ESC Arming procedure
 * @param[in] esc Pointer to the preinitialized ESC out struct from ESC_Init
 *
 * @note This function blocks the thread it is called in because of use of
 * k_msleep!
 *
 * @retval ESC_SUCCESS - Operation finished successfully
 * @retval ESC_INVALID_PROTOCOL - Specified ESC protocol is not valid
 * @retval ESC_NOT_INITIALIZED - Specified esc is not initialized correctly
 * @retval ESC_INVALID_ARMING - Error in arming procedure
 */
ESC_Error_Type ESC_Arm(const ESC_Inst_Type *esc);

#endif
