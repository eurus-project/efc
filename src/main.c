/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/uart.h>

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart),
	     "Console device is not ACM CDC UART device");

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
static struct usbd_context *sample_usbd;

static int enable_usb_device_next(void)
{
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

static const char *disk_mount_pt = "/"CONFIG_SDMMC_VOLUME_NAME":";
static const char *disk_pdrv = CONFIG_SDMMC_VOLUME_NAME;

char fname1[LFS_NAME_MAX];

int main(void) {
#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
	if (enable_usb_device_next()) {
		return 0;
	}
#else
	if (usb_enable(NULL)) {
		return 0;
	}
#endif

    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }

    int ret;
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return 0;
    }

	mountpoint->storage_dev = (void *)disk_pdrv;
	mountpoint->mnt_point = disk_mount_pt;

	ret = fs_mount(mountpoint);
    if (ret < 0) {
        printf("Could not mount filesystem!\n");
		return 0;
	}

    // read current count
    uint32_t boot_count = 0;
    struct fs_file_t file;
    fs_file_t_init(&file);
	
	snprintf(fname1, sizeof(fname1), "%s/boot_count", mountpoint->mnt_point);
    ret = fs_open(&file, fname1, FS_O_RDWR | FS_O_CREATE);
    if (ret < 0) {
        printf("Could not open file!\n");
        return 0;
    }

    ret = fs_read(&file, &boot_count, sizeof(boot_count));
	if (ret < 0) {
		printf("Could not read from file!\n");
        return 0;
    }

	ret = fs_seek(&file, 0, FS_SEEK_SET);
	if (ret < 0) {
		printf("Could not seek to beggining of the file!\n");
		return 0;
	}

	boot_count += 1;
	ret = fs_write(&file, &boot_count, sizeof(boot_count));
	if (ret < 0) {
		printf("Could not write to file!\n");
		return 0;
	}

	ret = fs_close(&file);
	if (ret < 0) {
		printf("Could not close file!\n");
		return 0;
	}

    printf("Boot count: %d\n", (int)boot_count);

    bool led_state = true;

    while (1) {
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0) {
            return 0;
        }

        led_state = !led_state;
        printf("LED state: %s\n", led_state ? "ON" : "OFF");
        k_msleep(SLEEP_TIME_MS);
    }
    
    return 0;
}
