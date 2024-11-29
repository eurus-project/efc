#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>


/* PWM configuration */
#define PWM_PERIOD_US     100 /* Period in microseconds */
#define PWM_DUTY_CYCLE_US 50  /* DT in microseconds */

int main(void)
{
    /* Get the PWM device */
    const struct device *pwm_dev_1 = DEVICE_DT_GET(DT_NODELABEL(pwm1));
    const struct device *pwm_dev_2 = DEVICE_DT_GET(DT_NODELABEL(pwm2));
    const struct device *pwm_dev_3 = DEVICE_DT_GET(DT_NODELABEL(pwm3));
    const struct device *pwm_dev_4 = DEVICE_DT_GET(DT_NODELABEL(pwm4));

    if (!device_is_ready(pwm_dev_1) ||
        !device_is_ready(pwm_dev_2) ||
        !device_is_ready(pwm_dev_3) ||
        !device_is_ready(pwm_dev_4))
    {
        printk("Error: PWM device is not ready\n");
        return;
    }

    printk("PWM device is ready!\n");
    
    /* Setting the PWM signal */
    int ret1, ret2, ret3, ret4;

    ret1 = pwm_set(pwm_dev_1,
                  1, 
                  PWM_USEC(PWM_PERIOD_US),
                  PWM_USEC(PWM_DUTY_CYCLE_US),
                  0);
    ret2 = pwm_set(pwm_dev_2,
                  2, 
                  PWM_USEC(PWM_PERIOD_US),
                  PWM_USEC(PWM_DUTY_CYCLE_US),
                  0);
    ret3 = pwm_set(pwm_dev_3,
                  3, 
                  PWM_USEC(PWM_PERIOD_US),
                  PWM_USEC(PWM_DUTY_CYCLE_US),
                  0);
    ret4 = pwm_set(pwm_dev_4,
                  4, 
                  PWM_USEC(PWM_PERIOD_US),
                  PWM_USEC(PWM_DUTY_CYCLE_US),
                  0);
    
    if (ret1 || ret2 || ret3 || ret4)
    {
        printk("Error: Failed to set PWM\n");
        return;
    }

    while (1)
    {
        //k_msleep(1000);
    }
}