/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-04-12     Steven Liu   the first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include "hal_base.h"
extern rt_err_t rk_gpio1c_level_watch_start(void);
extern rt_err_t rk_gpio1c_irq_watch_start(void); 
extern rt_err_t rk_gpio_irq_benchmark_start(rt_base_t pin);

extern rt_err_t rk_soft_i2c_slave_start_default(void);
int main(int argc, char **argv)
{
    rt_kprintf("Hi, this is RT-Thread!!\n");
    rt_kprintf("ok \n");
    // rk_gpio1c_level_watch_start();
    // rk_gpio1c_irq_watch_start();

    if (rk_soft_i2c_slave_start_default() != RT_EOK)
    {
        rt_kprintf("soft i2c slave start failed\n");
    }
    rt_kprintf("ok \n");

    // rk_gpio_irq_benchmark_start(50); 
    // while (1) {
    //     gpio_off();
    //     rt_thread_mdelay(2000);
    //     rt_kprintf("Hi, this is RT-Thread!!\n");
    //     gpio_on();
    //     rt_thread_mdelay(2000);

    // }
    while (1)
    {
        rt_thread_mdelay(100000);
        // rt_kprintf("is running ...!!\n");
        //  rt_thread_mdelay(1000);
    }
    
    return 0;
}


