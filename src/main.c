#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/sensor/icm42688.h>

LOG_MODULE_REGISTER(icm42688p_test, LOG_LEVEL_INF);


void main()
{
    const struct device *icm_imu = DEVICE_DT_GET(DT_NODELABEL(imu_icm42688));

    int ret;
    float accX, accY, accZ;
    float gyroX, gyroY, gyroZ;
    struct sensor_value icm_accel[3];
    struct sensor_value icm_gyro[3];

    if (!device_is_ready(icm_imu))
    {
        LOG_ERR("ICM-42688P device is not ready!\n");
        return;
    }

    LOG_INF("ICM-42688P device initialized succesfully on SPI2!\n");

    while (1)
    {
        /* Fetch a sample from the sensor */
        if (sensor_sample_fetch(icm_imu) < 0)
        {
            LOG_ERR("Failed to fetch sensor sample!");
        }

        /* Get accelerometer data */
        ret = sensor_channel_get(icm_imu, SENSOR_CHAN_ACCEL_XYZ, icm_accel);
        if (ret < 0)
        {
            LOG_ERR("Could not read icm accel data!");
        } else {
            accX = sensor_value_to_float(&icm_accel[0]);
            accY = sensor_value_to_float(&icm_accel[1]);
            accZ = sensor_value_to_float(&icm_accel[2]);

            LOG_INF("AccZ: %f", (double)accZ);
            LOG_INF("AccX: %f", (double)accX);
            LOG_INF("AccY: %f", (double)accY);
        }
        
        ret = sensor_channel_get(icm_imu, SENSOR_CHAN_GYRO_XYZ, icm_gyro);
        if (ret < 0)
        {
            LOG_ERR("Could not read icm gyro data!");
        } else {
            gyroX = sensor_value_to_float(&icm_gyro[0]);
            gyroY = sensor_value_to_float(&icm_gyro[1]);
            gyroZ = sensor_value_to_float(&icm_gyro[2]);

            LOG_INF("GyroX: %f", (double)gyroX);
            LOG_INF("GyroY: %f", (double)gyroY);
            LOG_INF("GyroZ: %f", (double)gyroZ);
        }

        k_sleep(K_MSEC(1000));
    }   
}