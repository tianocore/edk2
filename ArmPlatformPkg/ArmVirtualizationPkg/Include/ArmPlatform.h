/** @file
*  Header defining platform constants (Base addresses, sizes, flags)
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*  Copyright (c) 2014, Linaro Limited
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

//
// We don't care about this value, but the PL031 driver depends on the macro
// to exist: it will pass it on to our ArmPlatformSysConfigLib:ConfigGet()
// function, which just returns EFI_UNSUPPORTED.
//
#define SYS_CFG_RTC       0x0

#define QEMU_NOR_BLOCK_SIZE    SIZE_256KB
#define QEMU_NOR0_BASE         0x0
#define QEMU_NOR0_SIZE         SIZE_64MB
#define QEMU_NOR1_BASE         0x04000000
#define QEMU_NOR1_SIZE         SIZE_64MB

#endif
