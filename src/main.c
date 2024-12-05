#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include "esc.h"

int main(void)
{
    status_t status;

    esc_t esc1;

    const struct device *pwm_dev = DEVICE_DT_GET(DT_NODELABEL(pwm1));

    if (!device_is_ready(pwm_dev))
    {
        return;
    }

    status = ESC_Init(pwm_dev, 2, ESC_ONESHOT_125, &esc1);

    if (status < 0)
        return;
    
    status = ESC_SetSpeed(&esc1, 0);
    if (status < 0)
        return;

    while (1)
    {
        for (int i = 0; i < 100; i++)
        {
            ESC_SetSpeed(&esc1, i);
            k_msleep(50);
        }
    }
}