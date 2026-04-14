#include <rtthread.h>
#include <rtdevice.h>
#include "hal_base.h"

extern uint32_t SystemCoreClock;

/**
 * @brief Dump the current RK3562 CPU clock related frequencies.
 *
 * This helper prints the runtime ARM core clock reported by CRU, the APLL
 * source clock, and the current SystemCoreClock software view so we can see
 * whether the delay base and the hardware clock tree agree.
 */
void rk3562_dump_cpu_clock(void)
{
    uint32_t cpu_id;
    uint32_t armclk_hz;
    uint32_t apll_hz;

    cpu_id = HAL_CPU_TOPOLOGY_GetCurrentCpuId();
    armclk_hz = HAL_CRU_ClkGetFreq(ARMCLK);
    apll_hz = HAL_CRU_ClkGetFreq(PLL_APLL);

    rt_kprintf("[cpuclk] cpu_id=%u\n", cpu_id);
    rt_kprintf("[cpuclk] ARMCLK=%u Hz (%.3f MHz)\n",
               armclk_hz,
               armclk_hz / 1000000.0);
    rt_kprintf("[cpuclk] PLL_APLL=%u Hz (%.3f MHz)\n",
               apll_hz,
               apll_hz / 1000000.0);
    rt_kprintf("[cpuclk] SystemCoreClock=%u Hz (%.3f MHz)\n",
               SystemCoreClock,
               SystemCoreClock / 1000000.0);

    if (armclk_hz != SystemCoreClock)
    {
        rt_kprintf("[cpuclk] warning: ARMCLK and SystemCoreClock differ.\n");
    }
}

static void cpuclk_probe(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    rk3562_dump_cpu_clock();
}
MSH_CMD_EXPORT(cpuclk_probe, dump current rk3562 cpu clock);
