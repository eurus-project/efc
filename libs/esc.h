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

//TODO: Check this!
// ESC Protocol Period in us
#define T_PWM_US      4000
#define T_ONESHOT_125 500
#define T_ONESHOT_42  168
#define T_MULTISHOT   50

/*********************************** Typedefs *********************************/
typedef int status_t;
typedef uint8_t esc_number_t;
typedef uint32_t esc_period_t;
typedef uint32_t esc_dc_min_t, esc_dc_max_t;

typedef enum {
    ESC_PWM = 0,
    ESC_ONESHOT_125,
    ESC_ONESHOT_42,
    ESC_MULTISHOT
} esc_protocol_t;

typedef enum {
    ESC_UINITIALIZED = 0,
    ESC_INITIALIZED
} esc_init_flag_t;

typedef struct {
    esc_number_t instance_num;
    const struct device *pwm_dev;
    uint32_t pwm_channel; 
} esc_instance_t;

typedef struct {
    esc_instance_t   instance;
    esc_init_flag_t  flag;
    esc_period_t     period;
    esc_dc_min_t     dc_min;
    esc_dc_max_t     dc_max;
} esc_t;

/********************************** Functions *********************************/
status_t ESC_Init(const struct device *pwm_dev,
                  uint32_t pwm_channel,
                  esc_protocol_t protocol, 
                  esc_t *esc_out);


/* TODO: This function should call pwm_set API to set PWM beneath esc layer, but
         to make this possible, channel should be passed as argument to pwm_set,
         so how to make correlation with esc instance? */

status_t ESC_SetSpeed(esc_t *esc, uint8_t speed);


#endif