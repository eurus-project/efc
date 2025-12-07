#include <math.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(main);

// SBUS UART device (USART2)
#define SBUS_UART_NODE DT_NODELABEL(usart2)
static const struct device *sbus_uart = DEVICE_DT_GET(SBUS_UART_NODE);

// UART receive buffer
#define UART_BUF_SIZE 128
static uint8_t uart_rx_buf[UART_BUF_SIZE];
static size_t uart_rx_pos = 0;

// This LED simply blinks at an interval, indicating visually that the firmware
// is running If anything causes the whole firmware to abort, it will be
// apparent without looking at log output or hooking up a debugger.
static const struct gpio_dt_spec fw_running_led =
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

// UART interrupt handler for received data
static void uart_isr(const struct device *dev, void *user_data) {
    uint8_t c;

    if (!uart_irq_update(dev)) {
        return;
    }

    // Check if data is available to read
    while (uart_irq_rx_ready(dev)) {
        uart_fifo_read(dev, &c, 1);

        // Store in buffer
        if (uart_rx_pos < UART_BUF_SIZE - 1) {
            uart_rx_buf[uart_rx_pos++] = c;
        }

        // Check for newline or carriage return to process complete message
        if (c == '\n' || c == '\r') {
            if (uart_rx_pos > 1) {
                uart_rx_buf[uart_rx_pos - 1] = '\0';
                LOG_INF("UART RX (%d bytes): %s", uart_rx_pos - 1, uart_rx_buf);
                LOG_HEXDUMP_INF(uart_rx_buf, uart_rx_pos - 1, "RX Data:");
            }
            uart_rx_pos = 0;
        }
    }
}

static int sbus_uart_init(void) {
    if (!device_is_ready(sbus_uart)) {
        LOG_ERR("SBUS UART device not ready");
        return -ENODEV;
    }

    // Get and log UART configuration
    struct uart_config uart_cfg;
    int ret = uart_config_get(sbus_uart, &uart_cfg);
    if (ret == 0) {
        LOG_INF("USART2 Configuration:");
        LOG_INF("  Baudrate: %d", uart_cfg.baudrate);
        LOG_INF("  Data bits: %d", uart_cfg.data_bits);
        LOG_INF("  Stop bits: %d", uart_cfg.stop_bits);
        LOG_INF("  Parity: %d", uart_cfg.parity);
        LOG_INF("  Flow control: %d", uart_cfg.flow_ctrl);
    } else {
        LOG_WRN("Failed to get UART config: %d", ret);
    }

    // Configure UART interrupt
    uart_irq_callback_user_data_set(sbus_uart, uart_isr, NULL);

    // Enable RX interrupt
    uart_irq_rx_enable(sbus_uart);

    LOG_INF("SBUS UART initialized successfully on PA3 RX (interrupt-driven)");
    return 0;
}

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

    // Initialize SBUS UART
    int ret = sbus_uart_init();
    if (ret) {
        LOG_ERR("Failed to initialize SBUS UART: %d", ret);
        return 0;
    }

    if (!gpio_is_ready_dt(&fw_running_led)) {
        LOG_ERR("The firmware running LED is not ready");
        return 0;
    }

    ret = gpio_pin_configure_dt(&fw_running_led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Could not configure the firmware running LED as output");
        return 0;
    }

    LOG_INF("SBUS UART demo running. Waiting for data on PA3 RX...");

    while (1) {
        gpio_pin_toggle_dt(&fw_running_led);
        k_msleep(500);
    }
}