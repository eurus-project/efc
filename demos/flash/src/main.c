/*
 * This file is part of the efc project <https://github.com/eurus-project/efc/>.
 * Copyright (c) 2024 - 2025 The efc developers.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// This LED simply blinks at an interval, indicating visually that the firmware
// is running If anything causes the whole firmware to abort, it will be
// apparent without looking at log output or hooking up a debugger.
static const struct gpio_dt_spec fw_running_led =
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static struct fs_littlefs main_fs;

struct fs_mount_t main_fs_mount = {
    .type = FS_LITTLEFS,
    .fs_data = &main_fs,
    // .flags = FS_MOUNT_FLAG_USE_DISK_ACCESS,
    .storage_dev = (void *)FIXED_PARTITION_ID(lfs_partition),
    .mnt_point = "/SPI_FLASH:",
};

int32_t boot_count = -1;

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

static int update_boot_count(void) {
    struct fs_file_t boot_count_file;
    fs_file_t_init(&boot_count_file);

    char filename[LFS_NAME_MAX];
    snprintf(filename, sizeof(filename), "%s/boot_count",
             main_fs_mount.mnt_point);
    int ret = fs_open(&boot_count_file, filename, FS_O_RDWR | FS_O_CREATE);
    if (ret < 0) {
        LOG_ERR("Could not open file!\n");
        return ret;
    }

    ret = fs_read(&boot_count_file, &boot_count, sizeof(boot_count));
    if (ret < 0) {
        LOG_ERR("Could not read from file!\n");
        return ret;
    }

    LOG_INF("Boot count: %d\n", (int)boot_count);

    ret = fs_seek(&boot_count_file, 0, FS_SEEK_SET);
    if (ret < 0) {
        LOG_ERR("Could not seek to beggining of the file!\n");
        return ret;
    }

    boot_count += 1;
    ret = fs_write(&boot_count_file, &boot_count, sizeof(boot_count));
    if (ret < 0) {
        LOG_ERR("Could not write to file!\n");
        return ret;
    }

    ret = fs_close(&boot_count_file);
    if (ret < 0) {
        LOG_ERR("Could not close file!\n");
        return ret;
    }

    return 0;
}

static int create_demo_file(void) {
    struct fs_file_t demo_file;
    const char demo_data[] = "This is a demo test message from EFC";

    fs_file_t_init(&demo_file);

    char filename[LFS_NAME_MAX];
    snprintf(filename, sizeof(filename), "%s/demo_file.txt",
             main_fs_mount.mnt_point);

    int ret = fs_open(&demo_file, filename, FS_O_RDWR | FS_O_CREATE);
    if (ret < 0) {
        LOG_ERR("Could not open file!\n");
        return ret;
    }

    ret = fs_write(&demo_file, &demo_data, sizeof(demo_data));
    if (ret < 0) {
        LOG_ERR("Could not write to file!\n");
        return ret;
    }

    ret = fs_close(&demo_file);
    if (ret < 0) {
        LOG_ERR("Could not close file!\n");
        return ret;
    }

    return 0;
}

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

    ret = fs_mount(&main_fs_mount);
    if (ret < 0) {
        LOG_ERR("Could not mount filesystem!\n");
    }

    ret = update_boot_count();
    if (ret < 0) {
        LOG_ERR("Could not update boot count!");
    }

    ret = create_demo_file();
    if (ret < 0) {
        LOG_ERR("Could not create demo file!");
    }

    while (1) {
        ret = gpio_pin_toggle_dt(&fw_running_led);
        if (ret < 0) {
            LOG_ERR("Could not toggle the firmware running LED");
            return 0;
        }

        k_msleep(1000);
    }

    return 0;
}
