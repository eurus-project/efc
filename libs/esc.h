/**
 * @file   esc.h
 * @brief  Zephyr Electronic Speed Controller ESC driver for BLDC motors
 * @author Eurus-project
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

// ESC Arming 
#define ARM_THROTTLE  8    //NOTE: Arm throttle is around 5-10 percent
#define ARM_DURATION  3000 //NOTE: Arm duration is around 2000-5000 ms

/*********************************** Macros ***********************************/

/*********************************** Typedefs *********************************/
typedef int status_t;
typedef uint8_t esc_number_t;
typedef uint32_t esc_period_t;
typedef uint32_t esc_dc_min_t, esc_dc_max_t;

/**
 * @brief ESC Protocol type
 */
typedef enum {
    ESC_PWM = 0,
    ESC_ONESHOT_125,
    ESC_ONESHOT_42,
    ESC_MULTISHOT
} esc_protocol_t;

/**
 * @brief ESC Initialization flag
 */
typedef enum {
    ESC_UNINITIALIZED = 0,
    ESC_INITIALIZED
} esc_init_flag_t;

/**
 * @brief ESC Instance struct
 */
typedef struct {
    esc_number_t instance_num;
    const struct device *pwm_dev;
    uint32_t pwm_channel; 
} esc_instance_t;

/**
 * @brief ESC configuration struct
 */
typedef struct {
    esc_instance_t   instance;
    esc_init_flag_t  flag;
    esc_period_t     period;
    esc_dc_min_t     dc_min;
    esc_dc_max_t     dc_max;
} esc_t;

/********************************** Functions *********************************/
/**
 * @brief ESC Initialization function
 * @param[in] pwm_dev     Pointer to the device struct - pwm device
 * @param[in] pwm_channel PWM channel which will be used by ESC device
 * @param[in] protocol    ESC Protocol
 * @param[out] esc_out    Pointer to the ESC out struct, it needs to be passed 
 *                        to the ESC_SetSpeed function
 * @retval ::status_t
 */
status_t ESC_Init(const struct device *pwm_dev,
                  uint32_t pwm_channel,
                  esc_protocol_t protocol, 
                  esc_t *esc_out);


/**
 * @brief ESC Set Speed by percentage from 0 to 100
 * @param[in] esc   Pointer to the preinitialized ESC out struct from ESC_Init
 * @param[in] speed ESC speed, in perentage
 * 
 * @retval ::status_t
 */
status_t ESC_SetSpeed(esc_t *esc, uint8_t speed);


/**
 * @brief ESC Stop - Stop ESC signal
 * @param[in] esc Pointer to the preinitialized ESC out struct from ESC_Init
 * 
 * @retval ::status_t
 */
status_t ESC_Stop(esc_t *esc);


/**
 * @brief ESC Deinitialize device
 * @param[in] esc Pointer to the preinitialized ESC out struct from ESC_Init
 * 
 * @retval ::status_t
 */
status_t ESC_DeInit(esc_t *esc);

/**
 * @brief ESC Arming procedure
 * @param[in] esc Pointer to the preinitialized ESC out struct from ESC_Init
 * 
 * @retval ::status_t
 */
status_t ESC_Arm(esc_t *esc);
#endif