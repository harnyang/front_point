#include <rtthread.h>
#include <rtdevice.h>
#include "hal_base.h"

#define GPIO_IRQ_PIN     143   // GPIO4_B7 的硬件编号
static rt_event_t irq_event;   // 中断事件对象
static rt_thread_t irq_thread; // 中断监控线程

/* 中断回调 */
static rt_err_t gpio_irq_handler(void *args)
{
    rt_event_send(irq_event, 0x01); // 发送中断事件
    return RT_EOK;
}

/* 中断监控线程 */
static void irq_monitor_thread(void *param)
{
    while (1)
    {
        /* 等待中断事件（阻塞） */
        if (rt_event_recv(irq_event, 0x01, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR,
                          RT_WAITING_FOREVER, RT_NULL) == RT_EOK)
        {
            rt_kprintf("[IRQ] GPIO4_B7 检测到下降沿!\n");
        }
    }
}

/* 初始化中断和线程 */
static void gpio_irq_init(void)
{
    /* 创建事件对象 */
    irq_event = rt_event_create("irq_event", RT_IPC_FLAG_FIFO);
    RT_ASSERT(irq_event != RT_NULL);

    /* 创建监控线程 */
    irq_thread = rt_thread_create("irq_monitor", irq_monitor_thread, RT_NULL,
                                  512, 20, 10);
    RT_ASSERT(irq_thread != RT_NULL);
    rt_thread_startup(irq_thread);

    /* 配置 GPIO 中断 */
    rt_pin_mode(GPIO_IRQ_PIN, PIN_MODE_INPUT_PULLUP); // 带上拉的输入模式
    rt_pin_attach_irq(GPIO_IRQ_PIN, PIN_IRQ_MODE_FALLING, gpio_irq_handler, RT_NULL);
    rt_pin_irq_enable(GPIO_IRQ_PIN, PIN_IRQ_ENABLE);   // 使能中断
    rt_kprintf("GPIO4_B7 interrupt enabled (falling edge).\n");
}


static void gpio_irq_start(int argc, char **argv)
{
    if (rt_thread_find("irq_monitor") != RT_NULL)
    {
        rt_kprintf("Interrupt test is already running!\n");
        return;
    }
    gpio_irq_init(); // 初始化中断和线程
}

MSH_CMD_EXPORT(gpio_irq_start, "使能 GPIO4_B7 下降沿中断");


static void gpio_irq_stop(int argc, char **argv)
{
    rt_thread_t thread;
    
    // 检查线程是否存在
    thread = rt_thread_find("irq_monitor");
    if (thread == RT_NULL)
    {
        rt_kprintf("中断测试未运行！\n");
        return;
    }
    
    // 禁用中断
    rt_pin_irq_enable(GPIO_IRQ_PIN, PIN_IRQ_DISABLE);
    
    // 分离中断
    rt_pin_detach_irq(GPIO_IRQ_PIN);
    
    // 删除线程（先等待一下确保线程可以安全退出）
    rt_thread_mdelay(10);
    rt_thread_delete(thread);
    
    // 删除事件对象
    if (irq_event != RT_NULL)
    {
        rt_event_delete(irq_event);
        irq_event = RT_NULL;
    }
    
    rt_kprintf("GPIO4_B7 interrupt disabled\n");
}

MSH_CMD_EXPORT(gpio_irq_stop, "禁用 GPIO4_B7 下降沿中断");
