/**
 * @file   esc.h
 * @brief  Zephyr Electronic Speed Controller ESC driver for BLDC motors
 * @author Eurus organization
 */

#ifndef ESC_H
#define ESC_H


/********************************** Includes **********************************/
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>

/*********************************** Defines **********************************/
// ESC Protocol Duty Cycle range in us
#define DC_MIN_PWM_US     1000
#define DC_MAX_PWM_US     2000

#define DC_MIN_ONESHOT_125_US 125
#define DC_MAX_ONESHOT_125_US 250

#define DC_MIN_ONESHOT_42_US  42
#define DC_MAX_ONESHOT_42_US  84

#define DC_MIN_MULTISHOT_US   5
#define DC_MAX_MULTISHOT_US   25


// ESC Protocol Period in us
#define T_PWM_US 4000

/*********************************** Typedefs *********************************/
typedef int status_t;


typedef enum {
    ESC_PWM = 0,
    ESC_ONESHOT_125,
    ESC_ONESHOT_42,
    ESC_MULTISHOT
} esc_protocol_t;




/********************************** Functions *********************************/


status_t ESC_Init(const struct device *pwm_dev, esc_protocol_t protocol);


#endif