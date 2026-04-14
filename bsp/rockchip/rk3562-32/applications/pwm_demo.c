#include <rtthread.h>
#include <rtdevice.h>
#include "hal_base.h"


void pwm_start(int argc, char *argv[])
{
    struct rt_device_pwm *pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm0");
    
    if (pwm_dev == RT_NULL) {
        rt_kprintf("未找到PWM设备: pwm0\n");
        return;
    }
    
    // 设置通道0的周期和占空比
    rt_pwm_set(pwm_dev, 0, 1000000000, 500000000);  // 设置通道0的周期和占空比
    rt_pwm_enable(pwm_dev, 0);                      // 启用通道0
    
    rt_kprintf("PWM已启动: 周期1秒，占空比50%%\n");
}
MSH_CMD_EXPORT(pwm_start, "PWM测试");


void pwm_stop(int argc, char *argv[])
{
    struct rt_device_pwm *pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm0");

    if (pwm_dev == RT_NULL) {
        rt_kprintf("未找到PWM设备: pwm0\n");
        return;
    }

    rt_pwm_disable(pwm_dev, 0);
    rt_kprintf("PWM已停止\n");
}
MSH_CMD_EXPORT(pwm_stop, "PWM停止");