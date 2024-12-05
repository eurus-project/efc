/**
 * @file   esc.c
 * @brief  Zephyr Electronic Speed Controller ESC driver for BLDC motors
 * @author Eurus organization
 */

/******************************** Includes ************************************/
#include "esc.h"
#include <stdlib.h>
#include <stdint.h>
#include <zephyr/drivers/pwm.h>


/******************************** Functions ***********************************/

/* TODO: Retreive some info from device tree about pwm channels, make correlation
         with esc instance, should it be direct channel, or not? How to solve this?
*/ 
status_t ESC_Init(const struct device *pwm_dev,
                  uint32_t pwm_channel,
                  esc_protocol_t protocol, 
                  esc_t *esc_out)
{
    static esc_number_t esc_instance_num = 0;

    if (!device_is_ready(pwm_dev))
        return -1;
    
    switch (protocol)
    {
        case ESC_ONESHOT_125:
            esc_out->period = T_ONESHOT_125;
            esc_out->dc_min = DC_MIN_ONESHOT_125_US;
            esc_out->dc_max = DC_MAX_ONESHOT_125_US;
            break;
        
        case ESC_ONESHOT_42:
            esc_out->period = T_ONESHOT_42;
            esc_out->dc_min = DC_MIN_ONESHOT_42_US;
            esc_out->dc_max = DC_MAX_ONESHOT_42_US;
            break;

        case ESC_MULTISHOT:
            esc_out->period = T_MULTISHOT;
            esc_out->dc_min = DC_MIN_MULTISHOT_US;
            esc_out->dc_max = DC_MAX_MULTISHOT_US;
            break;

        default:
            esc_out->flag = ESC_UINITIALIZED;
            return -1;
    }
    esc_out->instance.pwm_dev      = pwm_dev;
    esc_out->instance.pwm_channel  = pwm_channel;
    esc_out->instance.instance_num = esc_instance_num;
    esc_out->flag                  = ESC_INITIALIZED;
    esc_instance_num++;
}       

status_t ESC_SetSpeed(esc_t *esc, uint8_t speed)
{
    int ret;
    uint32_t dc_speed;

    if (esc->flag != ESC_INITIALIZED)
        //TODO: Maybe print the error message that the esc isn't initialized
        return -1;
    
    /* NOTE: Here should be done mapping according to protocol chossen */
    dc_speed = (esc->dc_max-esc->dc_min)*speed/100 + esc->dc_min;

    ret = pwm_set(esc->instance.pwm_dev, 
                  esc->instance.pwm_channel,
                  PWM_USEC(esc->period),
                  PWM_USEC(dc_speed),
                  0); //TODO: Check here if some flags should be considered..
    if (ret)
        //TODO: Check if printing some error log is needed
        for(;;);
}
