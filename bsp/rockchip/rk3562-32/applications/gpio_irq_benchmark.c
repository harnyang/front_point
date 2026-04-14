#include <rtthread.h>
#include <rtdevice.h>
#include "hal_base.h"
#include "board_base.h"
#include <string.h>

#define GPIO_IRQ_BENCH_NAME          "girqbm"
#define GPIO_IRQ_BENCH_THREAD_NAME   "girqbm_w"
#define GPIO_IRQ_BENCH_EVENT_IRQ     (1UL << 0)
#define GPIO_IRQ_BENCH_DEFAULT_PIN   50
#define GPIO_IRQ_BENCH_BURST_IDLE_MS 100
#define GPIO_IRQ_BENCH_STACK_SIZE    1024
#define GPIO_IRQ_BENCH_PRIORITY      1
#define GPIO_IRQ_BENCH_TICK          5

#define RK_FORCE_INLINE static inline __attribute__((always_inline))
#define RK_U64_MAX ((rt_uint64_t)(~0ULL))

typedef struct
{
    rt_uint32_t irq_count;
    rt_uint32_t wake_count;
    rt_uint32_t coalesced_count;
    rt_uint64_t latency_sum_cycles;
    rt_uint64_t latency_min_cycles;
    rt_uint64_t latency_max_cycles;
    rt_uint64_t irq_interval_sum_cycles;
    rt_uint64_t irq_interval_min_cycles;
    rt_uint64_t irq_interval_max_cycles;
} rk_gpio_irq_bench_stats_t;

typedef struct
{
    rt_base_t pin;
    rt_bool_t running;
    rt_event_t evt;
    rt_thread_t worker;

    volatile rt_uint32_t irq_seq;
    volatile rt_uint64_t last_irq_cycles;
    volatile rt_uint64_t prev_irq_cycles;

    rk_gpio_irq_bench_stats_t burst;
    rk_gpio_irq_bench_stats_t total;
} rk_gpio_irq_bench_ctx_t;

static rk_gpio_irq_bench_ctx_t g_irq_bench =
{
    .pin = GPIO_IRQ_BENCH_DEFAULT_PIN,
    .running = RT_FALSE,
    .evt = RT_NULL,
    .worker = RT_NULL,
    .irq_seq = 0,
    .last_irq_cycles = 0,
    .prev_irq_cycles = 0,
    .burst =
    {
        .latency_min_cycles = RK_U64_MAX,
        .irq_interval_min_cycles = RK_U64_MAX,
    },
    .total =
    {
        .latency_min_cycles = RK_U64_MAX,
        .irq_interval_min_cycles = RK_U64_MAX,
    },
};

RK_FORCE_INLINE rt_uint32_t rk_gpio_irq_bench_cycles_to_us(rt_uint64_t cycles)
{
    return (rt_uint32_t)((cycles + (PLL_INPUT_OSC_RATE / 2000000U)) / (PLL_INPUT_OSC_RATE / 1000000U));
}

static void rk_gpio_irq_bench_reset_stats(rk_gpio_irq_bench_stats_t *stats)
{
    memset(stats, 0, sizeof(*stats));
    stats->latency_min_cycles = RK_U64_MAX;
    stats->irq_interval_min_cycles = RK_U64_MAX;
}

static void rk_gpio_irq_bench_merge_stats(rk_gpio_irq_bench_stats_t *dst,
                                          const rk_gpio_irq_bench_stats_t *src)
{
    dst->irq_count += src->irq_count;
    dst->wake_count += src->wake_count;
    dst->coalesced_count += src->coalesced_count;
    dst->latency_sum_cycles += src->latency_sum_cycles;
    dst->irq_interval_sum_cycles += src->irq_interval_sum_cycles;

    if (src->latency_min_cycles < dst->latency_min_cycles)
    {
        dst->latency_min_cycles = src->latency_min_cycles;
    }
    if (src->latency_max_cycles > dst->latency_max_cycles)
    {
        dst->latency_max_cycles = src->latency_max_cycles;
    }
    if (src->irq_interval_min_cycles < dst->irq_interval_min_cycles)
    {
        dst->irq_interval_min_cycles = src->irq_interval_min_cycles;
    }
    if (src->irq_interval_max_cycles > dst->irq_interval_max_cycles)
    {
        dst->irq_interval_max_cycles = src->irq_interval_max_cycles;
    }
}

static void rk_gpio_irq_bench_dump_stats(const char *tag,
                                         const rk_gpio_irq_bench_stats_t *stats)
{
    rt_uint32_t latency_avg_us = 0;
    rt_uint32_t latency_min_us = 0;
    rt_uint32_t latency_max_us = 0;
    rt_uint32_t interval_avg_us = 0;
    rt_uint32_t interval_min_us = 0;
    rt_uint32_t interval_max_us = 0;
    rt_uint32_t interval_samples;

    if (stats->irq_count == 0)
    {
        rt_kprintf("[girqbm] %s: no irq captured\n", tag);
        return;
    }

    if (stats->wake_count > 0)
    {
        latency_avg_us = rk_gpio_irq_bench_cycles_to_us(stats->latency_sum_cycles / stats->wake_count);
        latency_min_us = rk_gpio_irq_bench_cycles_to_us(stats->latency_min_cycles);
        latency_max_us = rk_gpio_irq_bench_cycles_to_us(stats->latency_max_cycles);
    }

    interval_samples = (stats->irq_count > 1U) ? (stats->irq_count - 1U) : 0U;
    if (interval_samples > 0U)
    {
        interval_avg_us = rk_gpio_irq_bench_cycles_to_us(stats->irq_interval_sum_cycles / interval_samples);
        interval_min_us = rk_gpio_irq_bench_cycles_to_us(stats->irq_interval_min_cycles);
        interval_max_us = rk_gpio_irq_bench_cycles_to_us(stats->irq_interval_max_cycles);
    }

    rt_kprintf("[girqbm] %s pin=%d irq=%u wake=%u lost=%u lat(us): min=%u avg=%u max=%u gap(us): min=%u avg=%u max=%u\n",
               tag,
               (int)g_irq_bench.pin,
               (unsigned int)stats->irq_count,
               (unsigned int)stats->wake_count,
               (unsigned int)stats->coalesced_count,
               (unsigned int)latency_min_us,
               (unsigned int)latency_avg_us,
               (unsigned int)latency_max_us,
               (unsigned int)interval_min_us,
               (unsigned int)interval_avg_us,
               (unsigned int)interval_max_us);
}

RK_FORCE_INLINE void rk_gpio_irq_bench_record_irq(rt_uint64_t now_cycles)
{
    rt_uint64_t prev_cycles = g_irq_bench.prev_irq_cycles;

    g_irq_bench.irq_seq++;
    g_irq_bench.last_irq_cycles = now_cycles;
    g_irq_bench.burst.irq_count++;

    if (prev_cycles != 0)
    {
        rt_uint64_t delta = now_cycles - prev_cycles;
        g_irq_bench.burst.irq_interval_sum_cycles += delta;
        if (delta < g_irq_bench.burst.irq_interval_min_cycles)
        {
            g_irq_bench.burst.irq_interval_min_cycles = delta;
        }
        if (delta > g_irq_bench.burst.irq_interval_max_cycles)
        {
            g_irq_bench.burst.irq_interval_max_cycles = delta;
        }
    }

    g_irq_bench.prev_irq_cycles = now_cycles;
}

static void rk_gpio_irq_bench_isr(void *args)
{
    RT_UNUSED(args);

    rk_gpio_irq_bench_record_irq(HAL_GetSysTimerCount());
    rt_event_send(g_irq_bench.evt, GPIO_IRQ_BENCH_EVENT_IRQ);
}

static void rk_gpio_irq_bench_worker(void *parameter)
{
    rt_uint32_t recv_set;
    rt_uint32_t handled_seq = 0;

    RT_UNUSED(parameter);

    while (g_irq_bench.running)
    {
        if (rt_event_recv(g_irq_bench.evt,
                          GPIO_IRQ_BENCH_EVENT_IRQ,
                          RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                          rt_tick_from_millisecond(GPIO_IRQ_BENCH_BURST_IDLE_MS),
                          &recv_set) == RT_EOK)
        {
            rt_uint32_t irq_seq;
            rt_uint64_t irq_cycles;
            rt_uint64_t now_cycles;
            rt_uint64_t latency_cycles;

            irq_seq = g_irq_bench.irq_seq;
            irq_cycles = g_irq_bench.last_irq_cycles;
            now_cycles = HAL_GetSysTimerCount();
            latency_cycles = now_cycles - irq_cycles;

            if (irq_seq > handled_seq + 1U)
            {
                g_irq_bench.burst.coalesced_count += (irq_seq - handled_seq - 1U);
            }

            g_irq_bench.burst.wake_count++;
            g_irq_bench.burst.latency_sum_cycles += latency_cycles;
            if (latency_cycles < g_irq_bench.burst.latency_min_cycles)
            {
                g_irq_bench.burst.latency_min_cycles = latency_cycles;
            }
            if (latency_cycles > g_irq_bench.burst.latency_max_cycles)
            {
                g_irq_bench.burst.latency_max_cycles = latency_cycles;
            }

            handled_seq = irq_seq;
        }
        else if (g_irq_bench.burst.irq_count > 0U)
        {
            rk_gpio_irq_bench_dump_stats("burst", &g_irq_bench.burst);
            rk_gpio_irq_bench_merge_stats(&g_irq_bench.total, &g_irq_bench.burst);
            rk_gpio_irq_bench_reset_stats(&g_irq_bench.burst);
            g_irq_bench.prev_irq_cycles = 0;
        }
    }
}

rt_err_t rk_gpio_irq_benchmark_start(rt_base_t pin)
{
    rt_err_t ret;

    if (g_irq_bench.running)
    {
        return -RT_EBUSY;
    }

    g_irq_bench.pin = pin;
    g_irq_bench.irq_seq = 0;
    g_irq_bench.last_irq_cycles = 0;
    g_irq_bench.prev_irq_cycles = 0;
    rk_gpio_irq_bench_reset_stats(&g_irq_bench.burst);
    rk_gpio_irq_bench_reset_stats(&g_irq_bench.total);

    g_irq_bench.evt = rt_event_create(GPIO_IRQ_BENCH_NAME, RT_IPC_FLAG_FIFO);
    if (g_irq_bench.evt == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    rt_pin_mode(g_irq_bench.pin, PIN_MODE_INPUT_PULLUP);

    ret = rt_pin_attach_irq(g_irq_bench.pin,
                            PIN_IRQ_MODE_RISING_FALLING,
                            rk_gpio_irq_bench_isr,
                            RT_NULL);
    if (ret != RT_EOK)
    {
        rt_event_delete(g_irq_bench.evt);
        g_irq_bench.evt = RT_NULL;
        return ret;
    }

    g_irq_bench.worker = rt_thread_create(GPIO_IRQ_BENCH_THREAD_NAME,
                                          rk_gpio_irq_bench_worker,
                                          RT_NULL,
                                          GPIO_IRQ_BENCH_STACK_SIZE,
                                          GPIO_IRQ_BENCH_PRIORITY,
                                          GPIO_IRQ_BENCH_TICK);
    if (g_irq_bench.worker == RT_NULL)
    {
        rt_pin_detach_irq(g_irq_bench.pin);
        rt_event_delete(g_irq_bench.evt);
        g_irq_bench.evt = RT_NULL;
        return -RT_ENOMEM;
    }

    g_irq_bench.running = RT_TRUE;
    rt_thread_startup(g_irq_bench.worker);

    ret = rt_pin_irq_enable(g_irq_bench.pin, PIN_IRQ_ENABLE);
    if (ret != RT_EOK)
    {
        g_irq_bench.running = RT_FALSE;
        rt_event_send(g_irq_bench.evt, GPIO_IRQ_BENCH_EVENT_IRQ);
        rt_thread_mdelay(1);
        rt_thread_delete(g_irq_bench.worker);
        g_irq_bench.worker = RT_NULL;
        rt_pin_detach_irq(g_irq_bench.pin);
        rt_event_delete(g_irq_bench.evt);
        g_irq_bench.evt = RT_NULL;
        return ret;
    }

    rt_kprintf("[girqbm] started pin=%d timer=%uMHz idle_dump=%ums\n",
               (int)g_irq_bench.pin,
               (unsigned int)(PLL_INPUT_OSC_RATE / 1000000U),
               (unsigned int)GPIO_IRQ_BENCH_BURST_IDLE_MS);
    return RT_EOK;
}

void rk_gpio_irq_benchmark_stop(void)
{
    rt_event_t evt;
    rt_thread_t worker;
    rt_base_t pin;

    if (!g_irq_bench.running && g_irq_bench.evt == RT_NULL)
    {
        return;
    }

    pin = g_irq_bench.pin;
    evt = g_irq_bench.evt;
    worker = g_irq_bench.worker;
    g_irq_bench.running = RT_FALSE;

    if (pin >= 0)
    {
        rt_pin_irq_enable(pin, PIN_IRQ_DISABLE);
        rt_pin_detach_irq(pin);
    }

    if (evt)
    {
        rt_event_send(evt, GPIO_IRQ_BENCH_EVENT_IRQ);
    }

    rt_thread_mdelay(1);

    if (worker)
    {
        rt_thread_delete(worker);
    }
    if (evt)
    {
        rt_event_delete(evt);
    }

    if (g_irq_bench.burst.irq_count > 0U)
    {
        rk_gpio_irq_bench_dump_stats("burst", &g_irq_bench.burst);
        rk_gpio_irq_bench_merge_stats(&g_irq_bench.total, &g_irq_bench.burst);
    }
    rk_gpio_irq_bench_dump_stats("total", &g_irq_bench.total);

    g_irq_bench.evt = RT_NULL;
    g_irq_bench.worker = RT_NULL;
    g_irq_bench.pin = GPIO_IRQ_BENCH_DEFAULT_PIN;
    g_irq_bench.irq_seq = 0;
    g_irq_bench.last_irq_cycles = 0;
    g_irq_bench.prev_irq_cycles = 0;
    rk_gpio_irq_bench_reset_stats(&g_irq_bench.burst);
    rk_gpio_irq_bench_reset_stats(&g_irq_bench.total);
}

void rk_gpio_irq_benchmark_dump(void)
{
    rk_gpio_irq_bench_dump_stats("burst", &g_irq_bench.burst);
    rk_gpio_irq_bench_dump_stats("total", &g_irq_bench.total);
}

static void gpio_irq_bench_start(int argc, char **argv)
{
    rt_base_t pin = GPIO_IRQ_BENCH_DEFAULT_PIN;

    if (argc >= 2)
    {
        pin = (rt_base_t)atoi(argv[1]);
    }

    if (rk_gpio_irq_benchmark_start(pin) != RT_EOK)
    {
        rt_kprintf("[girqbm] start failed\n");
    }
}
MSH_CMD_EXPORT(gpio_irq_bench_start, start gpio irq benchmark [pin]);

static void gpio_irq_bench_stop(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    rk_gpio_irq_benchmark_stop();
}
MSH_CMD_EXPORT(gpio_irq_bench_stop, stop gpio irq benchmark);

static void gpio_irq_bench_dump(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    rk_gpio_irq_benchmark_dump();
}
MSH_CMD_EXPORT(gpio_irq_bench_dump, dump gpio irq benchmark stats);
