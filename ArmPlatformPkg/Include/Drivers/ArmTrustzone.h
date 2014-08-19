/** @file
*
*  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
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

#ifndef __ARM_TRUSTZONE_H__
#define __ARM_TRUSTZONE_H__

#include <Uefi.h>

// Setup TZ Protection Controller
#define TZPC_DECPROT_0  0
#define TZPC_DECPROT_1  1
#define TZPC_DECPROT_2  2
#define TZPC_DECPROT_MAX  2

/**
    FIXME: Need documentation
**/
EFI_STATUS
TZPCSetDecProtBits (
  IN  UINTN TzpcBase,
  IN  UINTN TzpcId,
  IN  UINTN Bits
  );

/**
    FIXME: Need documentation
**/
EFI_STATUS
TZPCClearDecProtBits (
  IN  UINTN TzpcBase,
  IN  UINTN TzpcId,
  IN  UINTN Bits
  );

// Setup TZ Address Space Controller
#define TZASC_REGION_ENABLED        1
#define TZASC_REGION_DISABLED       0
#define TZASC_REGION_SIZE_32KB      0xE
#define TZASC_REGION_SIZE_64KB      0xF
#define TZASC_REGION_SIZE_128KB     0x10
#define TZASC_REGION_SIZE_256KB     0x11
#define TZASC_REGION_SIZE_512KB     0x12
#define TZASC_REGION_SIZE_1MB       0x13
#define TZASC_REGION_SIZE_2MB       0x14
#define TZASC_REGION_SIZE_4MB       0x15
#define TZASC_REGION_SIZE_8MB       0x16
#define TZASC_REGION_SIZE_16MB      0x17
#define TZASC_REGION_SIZE_32MB      0x18
#define TZASC_REGION_SIZE_64MB      0x19
#define TZASC_REGION_SIZE_128MB     0x1A
#define TZASC_REGION_SIZE_256MB     0x1B
#define TZASC_REGION_SIZE_512MB     0x1C
#define TZASC_REGION_SIZE_1GB       0x1D
#define TZASC_REGION_SIZE_2GB       0x1E
#define TZASC_REGION_SIZE_4GB       0x1F
#define TZASC_REGION_SECURITY_SR    (1 << 3)
#define TZASC_REGION_SECURITY_SW    (1 << 2)
#define TZASC_REGION_SECURITY_SRW   (TZASC_REGION_SECURITY_SR|TZASC_REGION_SECURITY_SW)
#define TZASC_REGION_SECURITY_NSR   (1 << 1)
#define TZASC_REGION_SECURITY_NSW   1
#define TZASC_REGION_SECURITY_NSRW  (TZASC_REGION_SECURITY_NSR|TZASC_REGION_SECURITY_NSW)

/**
    FIXME: Need documentation
**/
EFI_STATUS
TZASCSetRegion (
  IN  INTN  TzascBase,
  IN  UINTN RegionId,
  IN  UINTN Enabled,
  IN  UINTN LowAddress,
  IN  UINTN HighAddress,
  IN  UINTN Size,
  IN  UINTN Security
  );

#endif
