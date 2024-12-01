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
status_t ESC_Init(const struct device *pwm_dev, esc_protocol_t protocol)
{
    if (!device_is_ready(pwm_dev))
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

