#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const struct adc_dt_spec adc_channel =
    ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

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

int main(void) {
    int err;
    int32_t buf = 0;
    int32_t value_mV;
    uint16_t sample;

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

    if (!adc_is_ready_dt(&adc_channel)) {
        printk("ADC device not ready\n");
        return 0;
    }

    err = adc_channel_setup_dt(&adc_channel);
    if (err < 0) {
        printk("Could not setup channel!\n");
        return 0;
    }

    struct adc_sequence sequence = {
        .buffer = &buf,
        // buffer size in bytes, not number of samples
        .buffer_size = 2,
        // Optional
        .calibrate = true,
    };

    err = adc_sequence_init_dt(&adc_channel, &sequence);
    if (err < 0) {
        printk("Could not initalize sequnce");
        return 0;
    }

    while (1) {
        err = adc_read(adc_channel.dev, &sequence);
        if (err < 0) {
            printk("Could not read (%d)", err);
            continue;
        } else {
            printk("Raw ADC value: %d", buf);
            err = adc_raw_to_millivolts_dt(&adc_channel, &buf);
            printk("Value in millivolts: %d", buf);
        }

        k_msleep(500);
    }

    return 0;
}
