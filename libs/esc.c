/**
 * @file   esc.c
 * @brief  Zephyr Electronic Speed Controller ESC driver for BLDC motors
 * @author Eurus organization
 */



/******************************** Includes ************************************/
#include "esc.h"
#include <stdlib.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>


/******************************** Functions ***********************************/
status_t ESC_Init(pwm_spec_dt *pwmSpec, esc_protocol_t protocol)
{
    if (!pwm_is_ready_dt(pwmSpec))
        return -1;
    
    switch (protocol)
    {
        case ESC_ONESHOT_125:

            break;
        
        case ESC_ONESHOT_42:

            break;

        case ESC_MULTISHOT:

            break;

        default:
            return -1;
    }
}       

void ESC_DeInit(pwm_spec_dt *pwmSpec)
{

}


void ESC_SetThrottle(pwm_spec_dt *pwmSpec, uint32_t throttle)
{

}