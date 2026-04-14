#include <stdint.h>

#include <rthw.h>
#include <rtthread.h>


#define RT_USING_RPMSG_CDEV_THREAD_TEST
#ifdef RT_USING_RPMSG_CDEV_THREAD_TEST

#include "hal_base.h"
#include "rpmsg_lite.h"
#include "rpmsg_queue.h"
#include "rpmsg_ns.h"

// test is CPU0 as master and CPU3 as remote.
#define MASTER_ID   ((uint32_t)0)
#define REMOTE_ID_3 ((uint32_t)3)

// define endpoint id for test
#ifdef HAL_AP_CORE
#define RPMSG_RTT_REMOTE_TEST3_EPT_ID 0x3003U
#define RPMSG_RTT_REMOTE_TEST_EPT3_NAME "rpmsg-ap3-ch0"
#else
#define RPMSG_RTT_REMOTE_TEST3_EPT_ID 0x3004U
#define RPMSG_RTT_REMOTE_TEST_EPT3_NAME "rpmsg-mcu0-test"
#endif

#define RPMSG_RTT_TEST_MSG "Rockchip rpmsg linux test!"

#define RPMSG_NAME  "[rpmsg]"

/* TODO: These are defined in the linked script gcc_arm.ld.S */
extern uint32_t __linux_share_rpmsg_start__[];
extern uint32_t __linux_share_rpmsg_end__[];

#define RPMSG_LINUX_MEM_BASE ((uint32_t)&__linux_share_rpmsg_start__)
#define RPMSG_LINUX_MEM_END  ((uint32_t)&__linux_share_rpmsg_end__)
#define RPMSG_LINUX_MEM_SIZE (2UL * RL_VRING_OVERHEAD)

struct rpmsg_block_t
{
    uint32_t len;
    uint8_t buffer[496 - 4];
};

struct rpmsg_info_t
{
    struct rpmsg_lite_instance *instance;
    struct rpmsg_lite_endpoint *ept;
    uint32_t cb_sta;    // callback status flags
    void *private;
};

static struct rpmsg_info_t *g_info = RT_NULL;
static rpmsg_queue_handle g_remote_queue = RT_NULL;
static rt_thread_t g_rpmsg_thread = RT_NULL;
static rt_bool_t g_rpmsg_thread_exit = RT_FALSE;

static void rpmsg_share_mem_check(void)
{
    if ((RPMSG_LINUX_MEM_BASE + RPMSG_LINUX_MEM_SIZE) > RPMSG_LINUX_MEM_END)
    {
        rt_kprintf("share memory size error!\n");
        while (1)
        {
            ;
        }
    }
}

rpmsg_ns_new_ept_cb rpmsg_ns_cdev_cb(uint32_t new_ept, const char *new_ept_name, uint32_t flags, void *user_data)
{
    uint32_t cpu_id;
    char ept_name[RL_NS_NAME_SIZE];

#ifdef HAL_AP_CORE
    cpu_id = HAL_CPU_TOPOLOGY_GetCurrentCpuId();
    printf("%s rpmsg remote: name service callback cpu_id-%ld\n", RPMSG_NAME, cpu_id);
#endif
    strncpy(ept_name, new_ept_name, RL_NS_NAME_SIZE);
    printf("%s remote: new_ept-0x%lx name-%s\n", RPMSG_NAME, new_ept, ept_name);
}

static void rpmsg_thread_entry(void *parameter)
{
    int j;
    uint32_t master_ept_id;
    char *rx_msg = (char *)rt_malloc(RL_BUFFER_PAYLOAD_SIZE);
    
    if (rx_msg == NULL)
    {
        rt_kprintf("[rpmsg] rx_msg malloc error!\n");
        return;
    }
    
    rt_kprintf("[rpmsg] rpmsg start, recv msg from Linux\n");

    for (j = 0; j < 100 && !g_rpmsg_thread_exit; j++)
    {
        /* 需要先获取主ept_id，知道是谁发的然后才能给主发送. */
        rpmsg_queue_recv(g_info->instance, g_remote_queue, (uint32_t *)&master_ept_id, 
                         rx_msg, RL_BUFFER_PAYLOAD_SIZE, RL_NULL, RL_BLOCK);
        rt_kprintf("[rpmsg] remote: master_ept_id-0x%lx rx_msg: %s\n", master_ept_id, rx_msg);
        rpmsg_lite_send(g_info->instance, g_info->ept, master_ept_id, 
                        RPMSG_RTT_TEST_MSG, strlen(RPMSG_RTT_TEST_MSG), RL_BLOCK);
    }
    
    rt_free(rx_msg);
    g_rpmsg_thread = RT_NULL;
    rt_kprintf("[rpmsg] thread exited\n");
}

static void rpmsg_cdev_init(void)
{
    uint32_t master_id, remote_id;
    void *ns_cb_data;
    
    rt_kprintf("%s rpmsg cdev init\n", RPMSG_NAME);
    
    rpmsg_share_mem_check();
    master_id = MASTER_ID;
#ifdef HAL_AP_CORE
    remote_id = HAL_CPU_TOPOLOGY_GetCurrentCpuId();
    rt_kprintf("rpmsg remote: remote core cpu_id-%ld\n", remote_id);
#else
    remote_id = 4;
#endif

    g_info = rt_malloc(sizeof(struct rpmsg_info_t));
    if (g_info == NULL)
    {
        rt_kprintf("%s info malloc error!\n", RPMSG_NAME);
        return;
    }
    
    g_info->private = rt_malloc(sizeof(struct rpmsg_block_t));
    if (g_info->private == NULL)
    {
        rt_kprintf("%s private malloc error!\n", RPMSG_NAME);
        rt_free(g_info);
        g_info = RT_NULL;
        return;
    }

    rt_kprintf("%s remote: shmem_base-0x%lx shmem_end-%lx\n", RPMSG_NAME, RPMSG_LINUX_MEM_BASE, RPMSG_LINUX_MEM_END);

    g_info->instance = rpmsg_lite_remote_init((void *)RPMSG_LINUX_MEM_BASE, 
                                            RL_PLATFORM_SET_LINK_ID(master_id, remote_id), 
                                            RL_NO_FLAGS);
    rpmsg_lite_wait_for_link_up(g_info->instance, 10U);
    rt_kprintf("%s remote: link up! link_id-0x%lx\n", RPMSG_NAME, g_info->instance->link_id);


    rpmsg_ns_bind(g_info->instance, rpmsg_ns_cdev_cb, &ns_cb_data);
    g_remote_queue = rpmsg_queue_create(g_info->instance);
    g_info->ept = rpmsg_lite_create_ept(g_info->instance, RPMSG_RTT_REMOTE_TEST3_EPT_ID, 
                                       rpmsg_queue_rx_cb, g_remote_queue);
    uint32_t ept_flags = RL_NS_CREATE;
    rpmsg_ns_announce(g_info->instance, g_info->ept, RPMSG_RTT_REMOTE_TEST_EPT3_NAME, ept_flags);
    
    rt_kprintf("%s initialization completed\n", RPMSG_NAME);
}

static void rpmsg_cdev_start(void)
{
    if (g_info == NULL || g_remote_queue == NULL) {
        rt_kprintf("%s not initialized! Please run rpmsg_cdev_init first.\n", RPMSG_NAME);
        return;
    }
    
    if (g_rpmsg_thread != RT_NULL) {
        rt_kprintf("%s already running!\n", RPMSG_NAME);
        return;
    }
    
    g_rpmsg_thread_exit = RT_FALSE;
    g_rpmsg_thread = rt_thread_create("rpmsg_test", rpmsg_thread_entry, RT_NULL, 
                                     2048, RT_THREAD_PRIORITY_MAX/3, 20);
    if (g_rpmsg_thread != RT_NULL) {
        rt_thread_startup(g_rpmsg_thread);
    }
    else {
        rt_kprintf("%s Failed to create rpmsg test thread!\n", RPMSG_NAME);
    }
}

static void rpmsg_cdev_stop(void)
{
    if (g_rpmsg_thread != RT_NULL)
    {
        g_rpmsg_thread_exit = RT_TRUE;
        rt_thread_mdelay(100);  // 给线程一些退出的时间
        rt_kprintf("%s Request to stop rpmsg\n", RPMSG_NAME);
    }
    else
    {
        rt_kprintf("%s No rpmsg test thread running\n", RPMSG_NAME);
    }
}

// 导出到初始化组件
// INIT_APP_EXPORT(rpmsg_cdev_init);

// 导出到 MSH 命令
MSH_CMD_EXPORT(rpmsg_cdev_start, start rpmsg test in a separate thread);
MSH_CMD_EXPORT(rpmsg_cdev_stop, stop running rpmsg test thread);
#endif  //RT_USING_RPMSG_CDEV_THREAD_TEST