#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

//LOG_MODULE_REGISTER(icm42688p_test, LOG_LEVEL_INF);


void main()
{
    
    const struct device *icm_imu = DEVICE_DT_GET(DT_NODELABEL(icm42688p));

    struct sensor_value icm_accel[3];
    struct sensor_value icm_gyro[3];

    if (!device_is_ready(icm_imu))
    {
        //LOG_ERR("ICM-42688P device is not ready!\n");
        return;
    }

    //LOG_INF("ICM-42688P device initialized succesfully on SPI2!\n");

    while (1)
    {
        /* Fetch a sample from the sensor */
        if (sensor_sample_fetch(icm_imu) < 0)
        {
            //LOG_ERR("Failed to fetch sensor sample!");
        }

        /* Get accelerometer data */
        sensor_channel_get(icm_imu, SENSOR_CHAN_ACCEL_XYZ, icm_accel);
        sensor_channel_get(icm_imu, SENSOR_CHAN_GYRO_XYZ, icm_gyro);

        /* Log the data */
        /*
        LOG_INF("Accel: X=%d.%06d Y=%d.%06d Z=%d.%06d",
                accel[0].val1, accel[0].val2,
                accel[1].val1, accel[1].val2,
                accel[2].val1, accel[2].val2);

        LOG_INF("Gyro: X=%d.%06d Y=%d.%06d Z=%d.%06d",
                gyro[0].val1, gyro[0].val2,
                gyro[1].val1, gyro[1].val2,
                gyro[2].val1, gyro[2].val2);
        */
        k_sleep(K_MSEC(1000));
    }

    
}