#include <rtthread.h>
#include <rtdevice.h>
#include "hal_base.h"


#define GPIO_PIN     137  // GPIO4_B1 


static void gpio_on(int argc, char **argv)
{
    rt_pin_mode(GPIO_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(GPIO_PIN, PIN_HIGH);
    rt_kprintf("GPIO4_B1 set to HIGH\n");
}
MSH_CMD_EXPORT(gpio_on, "设置 GPIO4_B1 为高电平");


static void gpio_off(int argc, char **argv)
{
    rt_pin_mode(GPIO_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(GPIO_PIN, PIN_LOW);
    rt_kprintf("GPIO4_B1 set to LOW\n");
}
MSH_CMD_EXPORT(gpio_off, "设置 GPIO4_B1 为低电平");