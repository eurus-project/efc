#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

static const struct adc_dt_spec adc_channel =
    ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

void main(void) {
    int err;
    int16_t buf;
    int32_t value_mV;
    uint16_t sample;

    if (DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart)) {
#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
        if (enable_usb_device_next()) {
            return;
        }
#else
        if (usb_enable(NULL)) {
            return;
        }
#endif
    }

    if (!adc_is_ready_dt(&adc_channel)) {
        printk("ADC device not ready\n");
        return;
    }

    err = adc_channel_setup_dt(&adc_channel);
    if (err < 0) {
        printk("Could not setup channel!\n");
        return;
    }

    struct adc_sequence sequence = {
        .buffer = &buf,
        /* buffer size in bytes, not number of samples */
        .buffer_size = sizeof(buf),
        // Optional
        //.calibrate = true,
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
            sample = sequence.buffer;
            printk("Raw ADC value: %d", sample);
            err = adc_raw_to_millivolts_dt(&adc_channel, &value_mV);
            printk("Value in millivolts: %d", value_mV);
        }

        k_msleep(500);
    }
}
