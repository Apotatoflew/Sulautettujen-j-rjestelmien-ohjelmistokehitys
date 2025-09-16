// *******************************
// This example declares two tasks
// and runs them in parallel

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>


//Semaphores testi

//struct k_sem sem_red;
//struct k_sem sem_green;
//struct k_sem sem_yellow;

int valo = 0;
int entinen = 0;
bool seis = false;


void task1(void *, void *, void*);
void task2(void *, void *, void*);
void task3(void *, void *, void*);

#define	STACKSIZE	500
#define	PRIORITY	5
K_THREAD_DEFINE(tid1,STACKSIZE,task1,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(tid2,STACKSIZE,task2,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(tid3,STACKSIZE,task3,NULL,NULL,NULL,PRIORITY,0,0);


static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
//static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);


int init_led() {
    int ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
    //int ret2 = gpio_pin_configure_dt(&blue, GPIO_OUTPUT_ACTIVE);
    int ret3 = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
    if (ret < 0 ||  ret3 < 0) {
        printk("Error: Led configure failed\n");
        return -1;
    }
    return 0;
}

int main(void) {
    init_led();
    printk("LEDs initialized\n");
    //k_sem_init(&sem_red, 1, 1);    // Start with red allowed
    //k_sem_init(&sem_yellow, 0, 1);   // Blue waits
    //k_sem_init(&sem_green, 0, 1);  // Green waits
    return 0;
}

// Task1 function (Red)
void task1(void *, void *, void*) {
if (valo == 0) {   
    while (true) {
        //k_sem_take(&sem_red, K_FOREVER);
        gpio_pin_set_dt(&red, 1);
        //gpio_pin_set_dt(&blue, 0);
        gpio_pin_set_dt(&green, 0);
        printk("Red on\n");
        k_msleep(1000);
        gpio_pin_set_dt(&red, 0);
        printk("Red off\n");
        k_msleep(500);
        entinen = 1;
        valo = 1;
        //k_sem_give(&sem_yellow); // Yellow
    }
}}

// Task2 function (Blue)
void task2(void *, void *, void*) {
    if (valo == 1) {   

    while (true) {
        //k_sem_take(&sem_yellow, K_FOREVER);
        // Yellow: red + green ON, blue OFF
        gpio_pin_set_dt(&red, 1);
        //gpio_pin_set_dt(&blue, 0);
        gpio_pin_set_dt(&green, 1);
        printk("Yellow on\n");
        k_msleep(1000);
        gpio_pin_set_dt(&red, 0);
        gpio_pin_set_dt(&green, 0);
        printk("Yellow off\n");
        k_msleep(500);
        entinen = 2;
        valo = 2;
        
        //k_sem_give(&sem_green); // Green
    }
}}

// Task3 function (Green)
void task3(void *, void *, void*) {
    if (valo == 2) {   

    while (true) {
    
        //k_sem_take(&sem_green, K_FOREVER);
        gpio_pin_set_dt(&red, 0);
        //gpio_pin_set_dt(&blue, 0);
        gpio_pin_set_dt(&green, 1);
        printk("Green on\n");
        k_msleep(1000);
        gpio_pin_set_dt(&green, 0);
        printk("Green off\n");
        k_msleep(500);
        entinen = 0;
        valo = 0;
        //k_sem_give(&sem_red); // Red
    }
}}
