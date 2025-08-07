/*++

Copyright (c) 2013-2017, ARM Ltd. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

--*/

#ifndef ARM_GIC_DXE_H_
#define ARM_GIC_DXE_H_

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

extern EFI_CPU_ARCH_PROTOCOL  *gCpuArch;

// GicV2 API
EFI_STATUS
GicV2DxeInitialize (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  );

// GicV3 API
EFI_STATUS
GicV3DxeInitialize (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  );

// GicV5 API
EFI_STATUS
GicV5DxeInitialize (
  VOID
  );

// Shared code

EFI_STATUS
EFIAPI
GicCommonRegisterInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL  *This,
  IN HARDWARE_INTERRUPT_SOURCE        Source,
  IN HARDWARE_INTERRUPT_HANDLER       Handler,
  IN HARDWARE_INTERRUPT_HANDLER       *HandlerDest
  );

EFI_STATUS
EFIAPI
GicCommonInstallAndRegisterInterruptService (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL   *InterruptProtocol,
  IN EFI_HARDWARE_INTERRUPT2_PROTOCOL  *Interrupt2Protocol,
  IN EFI_CPU_INTERRUPT_HANDLER         InterruptHandler,
  IN EFI_EVENT_NOTIFY                  ExitBootServicesEvent
  );

/**
 *
 * Return whether the Source interrupt index refers to an extended shared
 * interrupt.
 *
 * @param Source  The source intid to test
 *
 * @return True if Source is an extended SPI intid, false otherwise.
 */
BOOLEAN
GicCommonSourceIsExtSpi (
  IN UINTN  Source
  );

/**
 * Return whether this is a special interrupt.
 *
 * @param Source  The source intid to test
 *
 * @return True if Source is a special interrupt intid, false otherwise.
 */
BOOLEAN
GicCommonSourceIsSpecialInterrupt (
  IN UINTN  Source
  );

/**
 *
 * Return whether the Source interrupt index refers to a shared interrupt (SPI)
 *
 * @param Source  The source intid to test
 *
 * @return True if Source is a SPI intid, false otherwise.
 */
BOOLEAN
GicCommonSourceIsSpi (
  IN UINTN  Source
  );

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
GicCommonGetDistributorIcfgBaseAndBit (
  IN HARDWARE_INTERRUPT_SOURCE  Source,
  OUT UINTN                     *RegAddress,
  OUT UINTN                     *Config1Bit
  );

UINT32
EFIAPI
ArmGicGetInterfaceIdentification (
  IN  UINTN  GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicDisableDistributor (
  IN  UINTN  GicDistributorBase
  );

UINTN
EFIAPI
ArmGicGetMaxNumInterrupts (
  IN  UINTN  GicDistributorBase
  );

UINT32
EFIAPI
ArmGicV3GetControlSystemRegisterEnable (
  VOID
  );

VOID
EFIAPI
ArmGicV3SetControlSystemRegisterEnable (
  IN UINT32  ControlSystemRegisterEnable
  );

VOID
EFIAPI
ArmGicV3EnableInterruptInterface (
  VOID
  );

VOID
EFIAPI
ArmGicV3DisableInterruptInterface (
  VOID
  );

UINTN
EFIAPI
ArmGicV3AcknowledgeInterrupt (
  VOID
  );

VOID
EFIAPI
ArmGicV3EndOfInterrupt (
  IN UINTN  Source
  );

VOID
ArmGicV3SetBinaryPointer (
  IN UINTN  BinaryPoint
  );

VOID
ArmGicV3SetPriorityMask (
  IN UINTN  Priority
  );

UINTN
ArmGicV3GetControlRegister (
  VOID
  );

VOID
ArmGicV3SetControlRegister (
  IN UINTN  Value
  );

#endif // ARM_GIC_DXE_H_
