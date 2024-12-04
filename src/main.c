#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>


/* PWM configuration */
#define FREQ_2_PERIOD_S(freq) (1/freq)
#define PERIOD_S_2_PERIOD_US (period_s) (period_s/1000000)

#define FREQUENCY         500

#define PWM_PERIOD_US     500/* Period in microseconds */
#define PWM_DUTY_CYCLE_US 125  /* DT in microseconds */



int main(void)
{
    /* Get the PWM device */
    const struct device *pwm_dev_1 = DEVICE_DT_GET(DT_NODELABEL(pwm1));

    /* Check if device is ready */
    if (!device_is_ready(pwm_dev_1))
    {
        printk("Error: PWM device is not ready\n");
        return;
    }

    printk("PWM device is ready!\n");
    
    /* Setting the PWM signal */
    int ret1, ret2, ret3, ret4, dt_inc;
    dt_inc = 0;

    ret1 = pwm_set(pwm_dev_1,
                  1, 
                  PWM_USEC(PWM_PERIOD_US),
                  PWM_USEC(PWM_DUTY_CYCLE_US),
                  0);
    ret1 = pwm_set(pwm_dev_1,
                  2, 
                  PWM_USEC(PWM_PERIOD_US),
                  PWM_USEC(PWM_DUTY_CYCLE_US),
                  0);
    ret3 = pwm_set(pwm_dev_1,
                  3, 
                  PWM_USEC(PWM_PERIOD_US),
                  PWM_USEC(PWM_DUTY_CYCLE_US),
                  0);
    ret4 = pwm_set(pwm_dev_1,
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
        pwm_set(pwm_dev_1,
                1,
                PWM_USEC(PWM_PERIOD_US),
                PWM_USEC(dt_inc),
                0);
        if (dt_inc >= PWM_PERIOD_US)
            dt_inc = 0;
        else 
            dt_inc++;
        k_msleep(5);
    }
}