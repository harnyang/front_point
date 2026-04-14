#include <rtthread.h>
#include <rtdevice.h>

#define GPIO1_C1_PIN 49
#define GPIO1_C2_PIN 50
#define GPIO1C_IRQ_LOG_BURST 4

typedef struct
{
    rt_base_t scl_pin;
    rt_base_t sda_pin;
    rt_bool_t running;
    volatile rt_uint32_t scl_irq_count;
    volatile rt_uint32_t sda_irq_count;
    volatile rt_uint8_t scl_log_left;
    volatile rt_uint8_t sda_log_left;
    volatile int scl_level;
    volatile int sda_level;
} rk_gpio1c_irq_watch_ctx_t;

static rk_gpio1c_irq_watch_ctx_t g_gpio1c_irq_watch =
{
    .scl_pin = GPIO1_C1_PIN,
    .sda_pin = GPIO1_C2_PIN,
    .running = RT_FALSE,
    .scl_irq_count = 0,
    .sda_irq_count = 0,
    .scl_log_left = 0,
    .sda_log_left = 0,
    .scl_level = -1,
    .sda_level = -1,
};

static void rk_gpio1c_irq_watch_scl(void *args)
{
    RT_UNUSED(args);

    g_gpio1c_irq_watch.scl_irq_count++;
    g_gpio1c_irq_watch.scl_level = rt_pin_read(g_gpio1c_irq_watch.scl_pin);

    if (g_gpio1c_irq_watch.scl_log_left > 0)
    {
        g_gpio1c_irq_watch.scl_log_left--;
        rt_kprintf("[g1c-irq] scl irq=%u level=%d\n",
                   (unsigned int)g_gpio1c_irq_watch.scl_irq_count,
                   g_gpio1c_irq_watch.scl_level);
    }
}

static void rk_gpio1c_irq_watch_sda(void *args)
{
    RT_UNUSED(args);

    g_gpio1c_irq_watch.sda_irq_count++;
    g_gpio1c_irq_watch.sda_level = rt_pin_read(g_gpio1c_irq_watch.sda_pin);

    if (g_gpio1c_irq_watch.sda_log_left > 0)
    {
        g_gpio1c_irq_watch.sda_log_left--;
        rt_kprintf("[g1c-irq] sda irq=%u level=%d\n",
                   (unsigned int)g_gpio1c_irq_watch.sda_irq_count,
                   g_gpio1c_irq_watch.sda_level);
    }
}

rt_err_t rk_gpio1c_irq_watch_start(void)
{
    rt_err_t ret;

    if (g_gpio1c_irq_watch.running)
    {
        return -RT_EBUSY;
    }

    g_gpio1c_irq_watch.scl_irq_count = 0;
    g_gpio1c_irq_watch.sda_irq_count = 0;
    g_gpio1c_irq_watch.scl_log_left = GPIO1C_IRQ_LOG_BURST;
    g_gpio1c_irq_watch.sda_log_left = GPIO1C_IRQ_LOG_BURST;

    rt_pin_mode(g_gpio1c_irq_watch.scl_pin, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(g_gpio1c_irq_watch.sda_pin, PIN_MODE_INPUT_PULLUP);

    g_gpio1c_irq_watch.scl_level = rt_pin_read(g_gpio1c_irq_watch.scl_pin);
    g_gpio1c_irq_watch.sda_level = rt_pin_read(g_gpio1c_irq_watch.sda_pin);

    ret = rt_pin_attach_irq(g_gpio1c_irq_watch.scl_pin,
                            PIN_IRQ_MODE_RISING_FALLING,
                            rk_gpio1c_irq_watch_scl,
                            RT_NULL);
    if (ret != RT_EOK)
    {
        rt_kprintf("[g1c-irq] attach scl failed: %d\n", ret);
        return ret;
    }

    ret = rt_pin_attach_irq(g_gpio1c_irq_watch.sda_pin,
                            PIN_IRQ_MODE_RISING_FALLING,
                            rk_gpio1c_irq_watch_sda,
                            RT_NULL);
    if (ret != RT_EOK)
    {
        rt_pin_detach_irq(g_gpio1c_irq_watch.scl_pin);
        rt_kprintf("[g1c-irq] attach sda failed: %d\n", ret);
        return ret;
    }

    ret = rt_pin_irq_enable(g_gpio1c_irq_watch.scl_pin, PIN_IRQ_ENABLE);
    if (ret != RT_EOK)
    {
        rt_pin_detach_irq(g_gpio1c_irq_watch.sda_pin);
        rt_pin_detach_irq(g_gpio1c_irq_watch.scl_pin);
        rt_kprintf("[g1c-irq] enable scl failed: %d\n", ret);
        return ret;
    }

    ret = rt_pin_irq_enable(g_gpio1c_irq_watch.sda_pin, PIN_IRQ_ENABLE);
    if (ret != RT_EOK)
    {
        rt_pin_irq_enable(g_gpio1c_irq_watch.scl_pin, PIN_IRQ_DISABLE);
        rt_pin_detach_irq(g_gpio1c_irq_watch.sda_pin);
        rt_pin_detach_irq(g_gpio1c_irq_watch.scl_pin);
        rt_kprintf("[g1c-irq] enable sda failed: %d\n", ret);
        return ret;
    }

    g_gpio1c_irq_watch.running = RT_TRUE;
    rt_kprintf("[g1c-irq] started scl(pin=%d level=%d) sda(pin=%d level=%d)\n",
               (int)g_gpio1c_irq_watch.scl_pin,
               g_gpio1c_irq_watch.scl_level,
               (int)g_gpio1c_irq_watch.sda_pin,
               g_gpio1c_irq_watch.sda_level);
    return RT_EOK;
}

void rk_gpio1c_irq_watch_stop(void)
{
    if (!g_gpio1c_irq_watch.running)
    {
        return;
    }

    rt_pin_irq_enable(g_gpio1c_irq_watch.scl_pin, PIN_IRQ_DISABLE);
    rt_pin_irq_enable(g_gpio1c_irq_watch.sda_pin, PIN_IRQ_DISABLE);
    rt_pin_detach_irq(g_gpio1c_irq_watch.scl_pin);
    rt_pin_detach_irq(g_gpio1c_irq_watch.sda_pin);
    g_gpio1c_irq_watch.running = RT_FALSE;
}

static void rk_gpio1c_irq_watch_dump(void)
{
    rt_kprintf("[g1c-irq] running=%d scl_irq=%u sda_irq=%u scl_level=%d sda_level=%d\n",
               g_gpio1c_irq_watch.running,
               (unsigned int)g_gpio1c_irq_watch.scl_irq_count,
               (unsigned int)g_gpio1c_irq_watch.sda_irq_count,
               g_gpio1c_irq_watch.scl_level,
               g_gpio1c_irq_watch.sda_level);
}

static void gpio1c_irq_watch_start(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (rk_gpio1c_irq_watch_start() != RT_EOK)
    {
        rt_kprintf("[g1c-irq] start failed\n");
    }
}
MSH_CMD_EXPORT(gpio1c_irq_watch_start, start GPIO1_C1/C2 irq watch);

static void gpio1c_irq_watch_stop(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    rk_gpio1c_irq_watch_stop();
}
MSH_CMD_EXPORT(gpio1c_irq_watch_stop, stop GPIO1_C1/C2 irq watch);

static void gpio1c_irq_watch_stat(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    rk_gpio1c_irq_watch_dump();
}
MSH_CMD_EXPORT(gpio1c_irq_watch_stat, stat GPIO1_C1/C2 irq watch);
