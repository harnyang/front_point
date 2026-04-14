#include <rtthread.h>
#include <rtdevice.h>
#include "hal_base.h"


static void cmd_thread_entry(void *parameter)
{
    rt_uint8_t count = 0;
    
    while (1) {
        rt_kprintf("命令线程运行中，计数: %d\n", count++);
        rt_thread_mdelay(1000); 
        
        if (count > 5) { 
            rt_kprintf("命令线程执行完成\n");
            break;
        }
    }
}

static int printf_demo(int argc, char **argv)
{
    rt_thread_t tid;
    
    rt_kprintf("开始执行用户命令\n");
    
    // 创建命令线程
    tid = rt_thread_create("cmd_thread",
                          cmd_thread_entry,
                          RT_NULL,
                          1024,
                          10, 
                          10);
    
    if (tid != RT_NULL) {
        rt_thread_startup(tid);
        return RT_EOK;
    } else {
        rt_kprintf("创建命令线程失败!\n");
        return -RT_ERROR;
    }
}

MSH_CMD_EXPORT(printf_demo, "线程打印测试");