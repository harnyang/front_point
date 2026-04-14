#include <rtthread.h>
#include <rtdevice.h>
#include <stdlib.h>

#define GPIO1_C1_PIN 49
#define GPIO1_C2_PIN 50
#define GPIO1C_WATCH_THREAD_NAME "g1c_watch"
#define GPIO1C_WATCH_STACK_SIZE  1024
#define GPIO1C_WATCH_PRIORITY    18
#define GPIO1C_WATCH_TICK        10
#define GPIO1C_WATCH_PERIOD_MS   1

typedef struct
{
    rt_base_t scl_pin;
    rt_base_t sda_pin;
    rt_thread_t thread;
    rt_bool_t running;
    int last_scl;
    int last_sda;
} rk_gpio1c_watch_ctx_t;

static rk_gpio1c_watch_ctx_t g_gpio1c_watch =
{
    .scl_pin = GPIO1_C1_PIN,
    .sda_pin = GPIO1_C2_PIN,
    .thread = RT_NULL,
    .running = RT_FALSE,
    .last_scl = -1,
    .last_sda = -1,
};

static void rk_gpio1c_level_watch_thread(void *parameter)
{
    RT_UNUSED(parameter);

    rt_pin_mode(g_gpio1c_watch.scl_pin, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(g_gpio1c_watch.sda_pin, PIN_MODE_INPUT_PULLUP);

    g_gpio1c_watch.last_scl = rt_pin_read(g_gpio1c_watch.scl_pin);
    g_gpio1c_watch.last_sda = rt_pin_read(g_gpio1c_watch.sda_pin);

    rt_kprintf("[g1c] start scl(pin=%d)=%d sda(pin=%d)=%d\n",
               (int)g_gpio1c_watch.scl_pin,
               g_gpio1c_watch.last_scl,
               (int)g_gpio1c_watch.sda_pin,
               g_gpio1c_watch.last_sda);

    while (g_gpio1c_watch.running)
    {
        int scl_now = rt_pin_read(g_gpio1c_watch.scl_pin);
        int sda_now = rt_pin_read(g_gpio1c_watch.sda_pin);

        if (scl_now != g_gpio1c_watch.last_scl)
        {
            g_gpio1c_watch.last_scl = scl_now;
            rt_kprintf("[g1c] scl=%d\n", scl_now);
        }

        if (sda_now != g_gpio1c_watch.last_sda)
        {
            g_gpio1c_watch.last_sda = sda_now;
            rt_kprintf("[g1c] sda=%d\n", sda_now);
        }

        rt_thread_mdelay(GPIO1C_WATCH_PERIOD_MS);
    }
}

rt_err_t rk_gpio1c_level_watch_start(void)
{
    if (g_gpio1c_watch.running)
    {
        return -RT_EBUSY;
    }

    g_gpio1c_watch.thread = rt_thread_create(GPIO1C_WATCH_THREAD_NAME,
                                             rk_gpio1c_level_watch_thread,
                                             RT_NULL,
                                             GPIO1C_WATCH_STACK_SIZE,
                                             GPIO1C_WATCH_PRIORITY,
                                             GPIO1C_WATCH_TICK);
    if (g_gpio1c_watch.thread == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    g_gpio1c_watch.running = RT_TRUE;
    rt_thread_startup(g_gpio1c_watch.thread);
    return RT_EOK;
}

void rk_gpio1c_level_watch_stop(void)
{
    if (!g_gpio1c_watch.running)
    {
        return;
    }

    g_gpio1c_watch.running = RT_FALSE;
    rt_thread_mdelay(2);

    if (g_gpio1c_watch.thread != RT_NULL)
    {
        rt_thread_delete(g_gpio1c_watch.thread);
        g_gpio1c_watch.thread = RT_NULL;
    }

    g_gpio1c_watch.last_scl = -1;
    g_gpio1c_watch.last_sda = -1;
}

static void gpio1c_watch_start(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (rk_gpio1c_level_watch_start() != RT_EOK)
    {
        rt_kprintf("[g1c] start failed or already running\n");
    }
}
MSH_CMD_EXPORT(gpio1c_watch_start, start GPIO1_C1/C2 level watch);

static void gpio1c_watch_stop(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    rk_gpio1c_level_watch_stop();
}
MSH_CMD_EXPORT(gpio1c_watch_stop, stop GPIO1_C1/C2 level watch);
