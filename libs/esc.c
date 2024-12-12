/**
 * @file   esc.c
 * @brief  Zephyr Electronic Speed Controller ESC driver for BLDC motors
 * @author Eurus-project
 */

/******************************** Includes ************************************/
#include "esc.h"
#include <stdlib.h>
#include <stdint.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/pinctrl.h>

/****************************** Static Functions ******************************/
static status_t prvESC_ChannelCheck(const struct device *pwm_dev, 
                                    uint32_t pwm_channel);

/******************************** Functions ***********************************/

/* TODO: Retreive some info from device tree about pwm channels
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
        case ESC_PWM:
            esc_out->period = T_PWM_US;
            esc_out->dc_min = DC_MIN_PWM_US;
            esc_out->dc_max = DC_MAX_PWM_US;
            break;

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
            esc_out->flag = ESC_UNINITIALIZED;
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
    {
        printk("[ESC]: Error: Uninitialized ESC cannot be started!");
        return -1;
    }

    if (speed > 100)
        speed = 100;
    
    dc_speed = (esc->dc_max-esc->dc_min)*speed/100 + esc->dc_min;

    ret = pwm_set(esc->instance.pwm_dev, 
                  esc->instance.pwm_channel,
                  PWM_USEC(esc->period),
                  PWM_USEC(dc_speed),
                  0);
    if (ret)
    {
        printk("[ESC]: Error: Selected PWM device cannot generate specified\
        ESC signal, timer value could be overflowed due to the low prescaler\
        value.");
        return -1;
    }
}

status_t ESC_Stop(esc_t *esc)
{
    int ret;

    if (esc->flag != ESC_INITIALIZED)
    {
        printk("[ESC]: Error: Uninitialized ESC cannot be stopped!");
        return -1;
    }

    ret = pwm_set(esc->instance.pwm_dev,
                  esc->instance.pwm_channel,
                  0,
                  0,
                  0);
    if (ret)
    {
        printk("[ESC]: Error: Selected PWM device cannot be stopped!\n");
        return -1;
    }

    return 0;
}

status_t ESC_DeInit(esc_t *esc)
{
    if (esc->flag != ESC_INITIALIZED)
    {
        printk("[ESC]: Error: Specified ESC is already deinitialized\n");
        return -1;
    }

    ESC_Stop(esc);
    esc->flag = ESC_UNINITIALIZED;

    return 0;
}

/************************** Static Functions **********************************/
static int prvESC_ChannelCheck(const struct device *pwm_dev, uint32_t pwm_channel)
{
    if (!device_is_ready(pwm_dev)) {
        printk("[ESC]: Error: PWM device is not ready.\n");
        return -1;
    }

    // Check if the channel exists
    if (!ESC_PWM_CHANNEL_EXISTS(pwm_dev, pwm_channel)) {
        printk("[ESC]: Error: PWM channel %d does not exist for device %s.\n", pwm_channel, pwm_dev->name);
        return -1;
    }

    return 0; // Channel exists
}



