/*++

Copyright (c) 2013-2017, ARM Ltd. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

--*/

#ifndef __ARM_GIC_DXE_H__
#define __ARM_GIC_DXE_H__

#include <Library/ArmGicLib.h>
#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/Cpu.h>
#include <Protocol/HardwareInterrupt.h>
#include <Protocol/HardwareInterrupt2.h>

extern UINTN                        mGicNumInterrupts;
extern HARDWARE_INTERRUPT_HANDLER  *gRegisteredInterruptHandlers;

// Common API
EFI_STATUS
InstallAndRegisterInterruptService (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL   *InterruptProtocol,
  IN EFI_HARDWARE_INTERRUPT2_PROTOCOL  *Interrupt2Protocol,
  IN EFI_CPU_INTERRUPT_HANDLER          InterruptHandler,
  IN EFI_EVENT_NOTIFY                   ExitBootServicesEvent
  );

EFI_STATUS
EFIAPI
RegisterInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source,
  IN HARDWARE_INTERRUPT_HANDLER         Handler
  );

// GicV2 API
EFI_STATUS
GicV2DxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );

// GicV3 API
EFI_STATUS
GicV3DxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );


// Shared code

/**
  Calculate GICD_ICFGRn base address and corresponding bit
  field Int_config[1] of the GIC distributor register.

  @param Source       Hardware source of the interrupt.
  @param RegAddress   Corresponding GICD_ICFGRn base address.
  @param Config1Bit   Bit number of F Int_config[1] bit in the register.

  @retval EFI_SUCCESS       Source interrupt supported.
  @retval EFI_UNSUPPORTED   Source interrupt is not supported.
**/
EFI_STATUS
GicGetDistributorIcfgBaseAndBit (
  IN HARDWARE_INTERRUPT_SOURCE             Source,
  OUT UINTN                               *RegAddress,
  OUT UINTN                               *Config1Bit
  );

#endif
