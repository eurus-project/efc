#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>


/* PWM configuration */
#define PWM_PERIOD_US     100 /* Period in microseconds */
#define PWM_DUTY_CYCLE_US 50  /* DT in microseconds */

int main(void)
{
    /* Get the PWM device */
    const struct device *pwm_dev = DEVICE_DT_GET(DT_NODELABEL(pwm1));

    if (!device_is_ready(pwm_dev))
    {
        printk("Error: PWM device is not ready\n");
        return;
    }

    printk("PWM device is ready!\n");
    
    /* Setting the PWM signal */
    int ret = pwm_set(pwm_dev,
                      1, 
                      PWM_USEC(PWM_PERIOD_US),
                      PWM_USEC(PWM_DUTY_CYCLE_US),
                      0);
    
    if (ret)
    {
        printk("Error %d: Failed to set PWM\n", ret);
        return;
    }

    while (1)
    {
        //k_msleep(1000);
    }
}