#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include "esc.h"



int main(void)
{
    status_t status;

    esc_t esc1, esc2, esc3, esc4;

    const struct device *pwm_dev = DEVICE_DT_GET(DT_NODELABEL(pwm1));

    if (!device_is_ready(pwm_dev))
    {
        return;
    }

    status = ESC_Init(pwm_dev, 1, ESC_PWM, &esc1);
    if (status < 0)
        return;
     

    status = ESC_SetSpeed(&esc1, 50);


    while (1)
    {
        /*
        for (int i = 0; i < 100; i++)
        {
            ESC_SetSpeed(&esc1, i);
            ESC_SetSpeed(&esc2, i);
            ESC_SetSpeed(&esc3, i);
            k_msleep(20);
        }
        for (int i = 100; i > 0; i--)
        {
            ESC_SetSpeed(&esc1, i);
            ESC_SetSpeed(&esc2, i);
            ESC_SetSpeed(&esc3, i);
            k_msleep(20);
        }
        */
    }
}