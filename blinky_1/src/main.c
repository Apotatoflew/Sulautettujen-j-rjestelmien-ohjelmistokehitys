/****************************
 Yhden pisteen yritys, valo syttyy ja sammuu kun konsoliin lähettää esim "R", ja nyt oikeasti signaaleilla.
 ****************************/

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdlib.h>

/****************************
 * Add to prj.conf:
 * CONFIG_HEAP_MEM_POOL_SIZE=1024
 ****************************/

#define STACKSIZE 500
#define PRIORITY 5

// UART
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

// FIFO
K_FIFO_DEFINE(dispatcher_fifo);

struct data_t {
    void *fifo_reserved;
    char msg[20];
};

// LEDs
static const struct gpio_dt_spec red   = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

// condvarit
K_MUTEX_DEFINE(thread_mutex);
K_CONDVAR_DEFINE(red_signal);
K_CONDVAR_DEFINE(yellow_signal);
K_CONDVAR_DEFINE(green_signal);

void red_task(void *, void *, void *);
void yellow_task(void *, void *, void *);
void green_task(void *, void *, void *);


int init_leds(void) {
    if (!device_is_ready(red.port) || !device_is_ready(green.port)) {
        printk("Error: LED device not ready\n");
        return -1;
    }
    if (gpio_pin_configure_dt(&red, GPIO_OUTPUT_INACTIVE) < 0) return -1;
    if (gpio_pin_configure_dt(&green, GPIO_OUTPUT_INACTIVE) < 0) return -1;
    return 0;
}



int main(void) {
    if (init_uart() != 0) {
        printk("UART initialization failed!\n");
        return -1;
    }
    if (init_leds() != 0) {
        printk("LED init failed!\n");
        return -1;
    }
    printk("System ready\n");
    return 0;
}

int init_uart(void) {
    if (!device_is_ready(uart_dev)) {
        return 1;
    }
    return 0;
}

/********************
 * UART task
 */
static void uart_task(void *, void *, void *)
{
    char rc = 0;
    char uart_msg[20];
    memset(uart_msg, 0, 20);
    int uart_msg_cnt = 0;

    while (true) {
        if (uart_poll_in(uart_dev, &rc) == 0) {
            if (rc != '\r' && rc != '\n') {
                uart_msg[uart_msg_cnt++] = rc;
            } else {
                if (uart_msg_cnt > 0) {
                    struct data_t *buf = k_malloc(sizeof(struct data_t));
                    if (buf == NULL) {
                        printk("Malloc failed!\n");
                        return;
                    }
                    snprintf(buf->msg, sizeof(buf->msg), "%s", uart_msg);
                    k_fifo_put(&dispatcher_fifo, buf);
                    printk("UART msg queued: %s\n", uart_msg);

                    // Reset buffer
                    uart_msg_cnt = 0;
                    memset(uart_msg, 0, sizeof(uart_msg));
                }
            }
        }
        k_msleep(10);
    }
}

/********************
 * Dispatcher task
 */
static void dispatcher_task(void *, void *, void *)
{
    while (true) {
        struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
        char sequence[20];
        memcpy(sequence, rec_item->msg, sizeof(sequence));
        k_free(rec_item);

        printk("Dispatcher got: %s\n", sequence);

        char color = sequence[0]; 
        printk("Parsed -> Color:%c\n", color);

        // Signaali ledeille
        k_mutex_lock(&thread_mutex, K_FOREVER);
        if (color == 'R') {
            k_condvar_broadcast(&red_signal);
        } else if (color == 'Y') {
            k_condvar_broadcast(&yellow_signal);
        } else if (color == 'G') {
            k_condvar_broadcast(&green_signal);
        }
        k_mutex_unlock(&thread_mutex);
    }
}

/********************
 * LED task
 */
void red_task(void *, void *, void *)
{
    while (true) {
        k_condvar_wait(&red_signal, &thread_mutex, K_FOREVER);
        gpio_pin_set_dt(&red, 1);
        printk("RED ON\n");
        k_msleep(500);
        gpio_pin_set_dt(&red, 0);
        printk("RED OFF\n");
    }
}

void yellow_task(void *, void *, void *)
{
    while (true) {
        k_condvar_wait(&yellow_signal, &thread_mutex, K_FOREVER);
        gpio_pin_set_dt(&red, 1);
        gpio_pin_set_dt(&green, 1);
        printk("YELLOW ON\n");
        k_msleep(500);
        gpio_pin_set_dt(&red, 0);
        gpio_pin_set_dt(&green, 0);
        printk("YELLOW OFF\n");
    }
}

void green_task(void *, void *, void *)
{
    while (true) {
        k_condvar_wait(&green_signal, &thread_mutex, K_FOREVER);
        gpio_pin_set_dt(&green, 1);
        printk("GREEN ON\n");
        k_msleep(500);
        gpio_pin_set_dt(&green, 0);
        printk("GREEN OFF\n");
    }
}

K_THREAD_DEFINE(red_thread, STACKSIZE, red_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(yel_thread, STACKSIZE, yellow_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(gre_thread, STACKSIZE, green_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(uart_thread, STACKSIZE, uart_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(dis_thread,  STACKSIZE, dispatcher_task, NULL, NULL, NULL, PRIORITY, 0, 0);
