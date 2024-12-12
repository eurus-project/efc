#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(main);

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
static struct usbd_context *sample_usbd;

static int enable_usb_device_next(void) {
    int err;

    sample_usbd = sample_usbd_init_device(NULL);
    if (sample_usbd == NULL) {
        return -ENODEV;
    }

    err = usbd_enable(sample_usbd);
    if (err) {
        return err;
    }

    return 0;
}
#endif /* defined(CONFIG_USB_DEVICE_STACK_NEXT) */

static int process_imu(const struct device *dev) {
    struct sensor_value temperature;
    struct sensor_value accel[3];
    struct sensor_value gyro[3];
    int ret = sensor_sample_fetch(dev);
    if (ret < 0) {
        LOG_ERR("Could not fetch data from IMU!");
        return ret;
    }

    ret = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
    if (ret < 0) {
        LOG_ERR("Could not get accelerometer data!");
        return ret;
    }

    ret = sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyro);
    if (ret < 0) {
        LOG_ERR("Could not get gyroscope data!");
        return ret;
    }

    ret = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &temperature);
    if (ret < 0) {
        LOG_WRN("Could not get IMU die temperature data!");
    }

    printf(
        "Temperature:%g Cel\n"
        "Accelerometer: %f %f %f m/s/s\n"
        "Gyroscope:  %f %f %f rad/s\n",
        sensor_value_to_double(&temperature), sensor_value_to_double(&accel[0]),
        sensor_value_to_double(&accel[1]), sensor_value_to_double(&accel[2]),
        sensor_value_to_double(&gyro[0]), sensor_value_to_double(&gyro[1]),
        sensor_value_to_double(&gyro[2]));

    return 0;
}

// This LED simply blinks at an interval, indicating visually that the firmware is running
// If anything causes the whole firmware to abort, it will be apparent without looking at
// log output or hooking up a debugger.
static const struct gpio_dt_spec fw_running_led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static struct fs_littlefs log_fs;

static struct fs_mount_t log_fs_mount = {
    .type = FS_LITTLEFS,
    .fs_data = &log_fs,
    .flags = FS_MOUNT_FLAG_USE_DISK_ACCESS,
    .storage_dev = "SD",
    .mnt_point = "/SD:",
};

int main(void) {
    if (DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart)) {
#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
        if (enable_usb_device_next()) {
            return 0;
        }
#else
        if (usb_enable(NULL)) {
            return 0;
        }
#endif
    }

    if (!gpio_is_ready_dt(&fw_running_led)) {
        LOG_ERR("The firmware running LED is not ready");
        return 0;
    }

    int ret;
    ret = gpio_pin_configure_dt(&fw_running_led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Could not configure the firmware running LED as output");
        return 0;
    }

    const struct device *const main_imu = DEVICE_DT_GET(DT_NODELABEL(main_imu));

    if (!device_is_ready(main_imu)) {
        LOG_ERR("Device %s is not ready\n", main_imu->name);
        return 0;
    }

    ret = fs_mount(&log_fs_mount);
    if (ret < 0) {
        LOG_ERR("Could not mount filesystem!\n");
        return 0;
    }

    uint32_t boot_count = 0;
    struct fs_file_t file;
    fs_file_t_init(&file);

    char filename[LFS_NAME_MAX];
    snprintf(filename, sizeof(filename), "%s/boot_count", log_fs_mount.mnt_point);
    ret = fs_open(&file, filename, FS_O_RDWR | FS_O_CREATE);
    if (ret < 0) {
        LOG_ERR("Could not open file!\n");
        return 0;
    }

    ret = fs_read(&file, &boot_count, sizeof(boot_count));
    if (ret < 0) {
        LOG_ERR("Could not read from file!\n");
        return 0;
    }

    ret = fs_seek(&file, 0, FS_SEEK_SET);
    if (ret < 0) {
        LOG_ERR("Could not seek to beggining of the file!\n");
        return 0;
    }

    boot_count += 1;
    ret = fs_write(&file, &boot_count, sizeof(boot_count));
    if (ret < 0) {
        LOG_ERR("Could not write to file!\n");
        return 0;
    }

    ret = fs_close(&file);
    if (ret < 0) {
        LOG_ERR("Could not close file!\n");
        return 0;
    }

    LOG_INF("Boot count: %d\n", (int)boot_count);

    while (1) {
        ret = gpio_pin_toggle_dt(&fw_running_led);
        if (ret < 0) {
            LOG_ERR("Could not toggle the firmware running LED");
            return 0;
        }

        process_imu(main_imu);

        k_msleep(1000);
    }

    return 0;
}
