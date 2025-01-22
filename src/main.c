#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/sensor/icm42688.h>

LOG_MODULE_REGISTER(icm42688p_test, LOG_LEVEL_INF);

double accX, accY, accZ;
double gyroX, gyroY, gyroZ;
double temp;

static struct sensor_trigger trigger;

static int process_icm42688(const struct device *dev)
{
	struct sensor_value temperature;
	struct sensor_value accel[3];
	struct sensor_value gyro[3];
	int rc = sensor_sample_fetch(dev);

	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ,
					accel);
	}
	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ,
					gyro);
	}
	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP,
					&temperature);
	}
	if (rc == 0) {
        accX = sensor_value_to_double(&accel[0]);
        accY = sensor_value_to_double(&accel[1]);
        accZ = sensor_value_to_double(&accel[2]);

        gyroX = sensor_value_to_double(&gyro[0]);
        gyroY = sensor_value_to_double(&gyro[1]);
        gyroZ = sensor_value_to_double(&gyro[2]);

        temp = sensor_value_to_double(&temperature);

        LOG_INF("%g Cel\n", temp);
        LOG_INF("accel %f %f %f m/s/s\n", accX, accY, accZ);
        LOG_INF("gyro  %f %f %f rad/s\n", gyroX, gyroY, gyroZ);
	} else {
        LOG_ERR("sample fetch/get failed.\n");
	}

	return rc;
}

static void handle_icm42688_drdy(const struct device *dev,
				const struct sensor_trigger *trig)
{
	int rc = process_icm42688(dev);

	if (rc != 0) {
        LOG_ERR("cancelling trigger due to failure.\n");
		(void)sensor_trigger_set(dev, trig, NULL);
		return;
	}
}

int main(void)
{
    const struct device *icm_imu = DEVICE_DT_GET(DT_NODELABEL(imu_icm42688));

    if (!device_is_ready(icm_imu))
    {
        LOG_ERR("ICM-42688P device is not ready!\n");
        return 0;
    }

    trigger = (struct sensor_trigger) {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};
	if (sensor_trigger_set(icm_imu, &trigger, handle_icm42688_drdy) < 0) {
        LOG_ERR("Cannot configure trigger.\n");
		return 0;
	}
    LOG_INF("Configured for triggered sampling.\n");

    while (1) {
    }
    
    return 0;
}