/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022 Rockchip Electronics Co., Ltd.
 */
/* Automatically generated file; DO NOT EDIT. */
/* RT-Thread Project Configuration */
/* RT-Thread Kernel */
/* kservice optimization */
/* Inter-Thread communication */
/* Memory Management */
/* Kernel Device Object */
/* RT-Thread Components */
/* Device Drivers */
/* Using USB */
/* C/C++ and POSIX layer */
/* POSIX (Portable Operating System Interface) layer */
/* Interprocess Communication (IPC) */
/* Socket is in the 'Network' category */
/* Network */
/* Utilities */
/* RT-Thread Benchmarks */
/* System */
/* RT-Thread Utestcases */
/* RT-Thread third party package */
/* Bluetooth */
/* examples bluetooth */
/* Bluetooth examlpes */
/* Example 'BT API TEST' Config */
/* Example 'BT DISCOVERY' Config */
/* Example 'A2DP SINK' Config */
/* Example 'A2DP SOURCE' Config  */
/* Example 'HFP CLIENT' Config */
/* RT-Thread board config */
/* RT-Thread rockchip common drivers */
/* Enable Fault Dump Hook */
/* RT-Thread rockchip inno-mipi-dphy driver */
/* RT-Thread rockchip jpeg enc driver */
/* RT-Thread rockchip rga driver */
/* RT-Thread rockchip pm drivers */
/* RT-Thread rockchip mipi-dphy driver */
/* RT-Thread rockchip isp driver */
/* RT-Thread rockchip vcm driver */
/* RT-Thread rockchip vicap driver */
/* RT-Thread rockchip camera driver */
/* RT-Thread rockchip vicap_lite driver */
/* RT-Thread rockchip csi2host driver */
/* RT-Thread rockchip buffer_manage driver */
/* RT-Thread rockchip coredump driver */
/* Enable PSTORE */
/* RT-Thread rockchip RPMSG driver */
/* Enable FTL */
/* RT-Thread rockchip RK3562 drivers */
/* Enable I2C */
/* Enable PWM */
/* Enable GMAC */
/* Enable UART */
/* RT-Thread bsp test case */
/* RT-Thread Common Test case */
/* RT-Thread third party package */
/* Bluetooth */
/* examples bluetooth */
/* Bluetooth examlpes */
/* Example 'BT API TEST' Config */
/* Example 'BT DISCOVERY' Config */
/* Example 'A2DP SINK' Config */
/* Example 'A2DP SOURCE' Config  */
/* Example 'HFP CLIENT' Config */
MEMORY
{
    SRAM (rxw) : ORIGIN = 0xfe480000, LENGTH = 64K /* SYSTEM SRAM */
    DRAM (rxw) : ORIGIN = 0x01800000, LENGTH = 0x00800000 /* DRAM */
    SHMEM (rxw) : ORIGIN = 0x07800000, LENGTH = 0x00400000 /* shared memory for all cpu */
    LINUX_RPMSG (rxw) : ORIGIN = 0x07c00000, LENGTH = 0x00500000
}
__SPINLOCK_MEM_SIZE = 0x100;
__SHARE_RPMSG_SIZE = 0x80000;
SECTIONS
{
    .text :
    {
        _stext = .;
        KEEP(*(.text.entrypoint)) /* The entry point */
        *(.vectors)
        *(.text) /* remaining code */
        *(.text.*) /* remaining code */
        *(.rodata) /* read-only data (constants) */
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)
        *(.gnu.linkonce.t*)
        *(COMMON)
        /* section information for finsh shell */
        . = ALIGN(16);
        __fsymtab_start = .;
        KEEP(*(FSymTab))
        __fsymtab_end = .;
        . = ALIGN(16);
        __vsymtab_start = .;
        KEEP(*(VSymTab))
        __vsymtab_end = .;
        . = ALIGN(16);
        /* section information for initial. */
        . = ALIGN(16);
        __rt_init_start = .;
        KEEP(*(SORT(.rti_fn*)))
        __rt_init_end = .;
        . = ALIGN(16);
        . = ALIGN(16);
        _etext = .;
    } > DRAM
    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > DRAM
    __exidx_end = .;
    __rodata_start = .;
    .rodata :
    {
        *(.rodata) *(.rodata.*)
    } > DRAM
    __rodata_end = .;
    .zero.table :
    {
        . = ALIGN(4);
        __zero_table_start__ = .;
        LONG (ADDR(.bss))
        LONG (SIZEOF(.bss) / 4)
        __zero_table_end__ = .;
    } > DRAM
    .copy.table :
    {
        . = ALIGN(4);
        __copy_table_start__ = .;
        __copy_table_end__ = .;
    } > DRAM
    . = ALIGN(16);
    .data :
    {
        *(.data)
        *(.data.*)
        . = ALIGN(16);
        _gp = ABSOLUTE(.); /* Base of small data */
    } > DRAM
    . = ALIGN(16);
    .ctors :
    {
        PROVIDE(__ctors_start__ = .);
        KEEP(*(SORT(.ctors.*)))
        KEEP(*(.ctors))
        PROVIDE(__ctors_end__ = .);
    } > DRAM
    .dtors :
    {
        PROVIDE(__dtors_start__ = .);
        KEEP(*(SORT(.dtors.*)))
        KEEP(*(.dtors))
        PROVIDE(__dtors_end__ = .);
    } > DRAM
    .linux_share_rpmsg (NOLOAD):
    {
        PROVIDE(__linux_share_rpmsg_start__ = .);
        . += 0x00500000;
        PROVIDE(__linux_share_rpmsg_end__ = .);
    } > LINUX_RPMSG
    .share_rpmsg (NOLOAD):
    {
        . = ALIGN(0x1000);
        PROVIDE(__share_rpmsg_start__ = .);
        . += __SHARE_RPMSG_SIZE;
        PROVIDE(__share_rpmsg_end__ = .);
        . = ALIGN(0x1000);
    } > SHMEM
    .share_lock (NOLOAD) :
    {
        . = ALIGN(64);
        PROVIDE(__spinlock_mem_start__ = .);
        . += __SPINLOCK_MEM_SIZE;
        PROVIDE(__spinlock_mem_end__ = .);
        . = ALIGN(64);
    } > SHMEM
    .share_data :
    {
        . = ALIGN(64);
        PROVIDE(__share_data_start__ = .);
        KEEP(*(.share_data))
        PROVIDE(__share_data_end__ = .);
        . = ALIGN(64);
    } > SHMEM AT > DRAM
    . = ALIGN(16);
    .bss (NOLOAD) :
    {
        PROVIDE(__bss_start = .);
        *(.bss)
        *(.bss.*)
        *(.dynbss)
        *(COMMON)
        . = ALIGN(16);
        PROVIDE(__bss_end = .);
    } > DRAM
    .heap :
    {
        . = ALIGN(16);
        PROVIDE(__heap_begin = .);
        . = ORIGIN(DRAM) + LENGTH(DRAM);
        PROVIDE(__heap_end = .);
    } > DRAM
}
