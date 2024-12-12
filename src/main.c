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

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

struct fs_littlefs lfsfs;

static struct fs_mount_t __mp = {
    .type = FS_LITTLEFS,
    .fs_data = &lfsfs,
    .flags = FS_MOUNT_FLAG_USE_DISK_ACCESS,
};

struct fs_mount_t *mountpoint = &__mp;

#if defined(CONFIG_DISK_DRIVER_SDMMC)
  #define CONFIG_SDMMC_VOLUME_NAME "SD"
#elif defined(CONFIG_DISK_DRIVER_MMC)
  #define CONFIG_SDMMC_VOLUME_NAME "SD2"
#else
  #error "No disk device defined, is your board supported?"
#endif


static const char *disk_mount_pt = "/" CONFIG_SDMMC_VOLUME_NAME ":";
static const char *disk_pdrv = CONFIG_SDMMC_VOLUME_NAME;

char fname1[LFS_NAME_MAX];

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

    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }

    int ret;
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return 0;
    }

    const struct device *const main_imu = DEVICE_DT_GET(DT_NODELABEL(main_imu));

    if (!device_is_ready(main_imu)) {
        LOG_ERR("Device %s is not ready\n", main_imu->name);
        return 0;
    }

    mountpoint->storage_dev = (void *)disk_pdrv;
    mountpoint->mnt_point = disk_mount_pt;

    ret = fs_mount(mountpoint);
    if (ret < 0) {
        LOG_ERR("Could not mount filesystem!\n");
        return 0;
    }

    // read current count
    uint32_t boot_count = 0;
    struct fs_file_t file;
    fs_file_t_init(&file);

    snprintf(fname1, sizeof(fname1), "%s/boot_count", mountpoint->mnt_point);
    ret = fs_open(&file, fname1, FS_O_RDWR | FS_O_CREATE);
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

    bool led_state = true;

    while (1) {
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0) {
            return 0;
        }

        led_state = !led_state;
        printf("LED state: %s\n", led_state ? "ON" : "OFF");

        process_imu(main_imu);

        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}
