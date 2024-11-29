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

/*********************************** Typedefs *********************************/
typedef int status_t;


typedef enum {
    ESC_ONESHOT_125 = 0,
    ESC_ONESHOT_42,
    ESC_MULTISHOT
} esc_protocol_t;




/********************************** Functions *********************************/


status_t ESC_Init(pwm_spec_dt *pwmSpec, esc_protocol_t protocol);
void ESC_DeInit(pwm_spec_dt *pwmSpec);


void ESC_SetThrottle(pwm_spec_dt *pwmSpec, uint32_t throttle);



#endif