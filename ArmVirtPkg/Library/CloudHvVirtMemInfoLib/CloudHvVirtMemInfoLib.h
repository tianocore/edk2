/** @file

  Copyright (c) 2022, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CLOUDHV_VIRT_MEM_INFO_LIB_H_
#define CLOUDHV_VIRT_MEM_INFO_LIB_H_

//
// Cloud Hypervisor may have more than one memory nodes. Even there is no limit for that,
// I think 10 is enough in general.
//
#define CLOUDHV_MAX_MEM_NODE_NUM  10

// Record memory node info (base address and size)
typedef struct {
  UINT64    Base;
  UINT64    Size;
} CLOUDHV_MEM_NODE_INFO;

// Number of Virtual Memory Map Descriptors
#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS  (4 + CLOUDHV_MAX_MEM_NODE_NUM)

//
// Core peripherals such as the UART, the GIC and the RTC are
// all mapped in the 'miscellaneous device I/O' region, which we just map
// in its entirety rather than device by device. Note that it does not
// cover any of the NOR flash banks or PCI resource windows.
//
#define MACH_VIRT_PERIPH_BASE  0x00400000
#define MACH_VIRT_PERIPH_SIZE  0x0FC00000

//
// The top of the 64M memory region under 4GB reserved for device
//
#define TOP_32BIT_DEVICE_BASE  0xFC000000
#define TOP_32BIT_DEVICE_SIZE  0x04000000

#endif // CLOUDHV_VIRT_MEM_INFO_LIB_H_
