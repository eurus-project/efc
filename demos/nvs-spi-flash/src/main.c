/*
 * This file is part of the efc project <https://github.com/eurus-project/efc/>.
 * Copyright (c) (2024 - Present), The efc developers.
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

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(main);

static struct nvs_fs nvs;

#define NVS_PARTITION spiflash_partition0
#define NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(NVS_PARTITION)
#define NVS_SECTOR_COUNT 3

#define BOOT_COUNT_ID 1
#define DEMO_SLEEP_TIME_MS 100

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

static void nvs_update_boot_count(struct nvs_fs *nvs) {
    int32_t boot_cnt = 0;

    int ret = nvs_read(nvs, BOOT_COUNT_ID, &boot_cnt, sizeof(boot_cnt));
    if (ret > 0) {  // Item found
        boot_cnt++; // Increment boot count if found, if not write zero
    }

    LOG_INF("Boot count: ID = %d, Value = %d\n", BOOT_COUNT_ID, boot_cnt);

    ret = nvs_write(nvs, BOOT_COUNT_ID, &boot_cnt, sizeof(boot_cnt));
    if (ret < 0) {
        LOG_ERR("Error in writting to nvs!");
    }
}

int main() {
    /* Initialize USB for logging */
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

    int ret = 0;
    struct flash_pages_info nvs_info;

    /* Configure NVS */
    nvs.flash_device = NVS_PARTITION_DEVICE;
    if (!device_is_ready(nvs.flash_device)) {
        LOG_ERR("Flash device not ready!");
        return 0;
    }

    nvs.offset = NVS_PARTITION_OFFSET;
    ret = flash_get_page_info_by_offs(nvs.flash_device, nvs.offset, &nvs_info);
    if (ret) {
        LOG_ERR("Unable to get page info, error: %d\n", ret);
        return 0;
    }

    nvs.sector_size = nvs_info.size;
    nvs.sector_count = NVS_SECTOR_COUNT;

    ret = nvs_mount(&nvs);
    if (ret) {
        LOG_ERR("Flash init failed, error: %d\n", ret);
        return 0;
    }

    // Update boot count
    nvs_update_boot_count(&nvs);

    while (1) {
        k_msleep(DEMO_SLEEP_TIME_MS);
    }

    return 0;
}
