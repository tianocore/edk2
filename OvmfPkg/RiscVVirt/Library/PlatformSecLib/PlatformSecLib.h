/** @file

Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_SEC_LIB_
#define PLATFORM_SEC_LIB_

#include <PiPei.h>
#include <Ppi/SecHobData.h>
#include <Ppi/SecPlatformInformation.h>
#include <Guid/FdtHob.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/RiscVSecHobData.h>
#include <Register/RiscV64/RiscVImpl.h>
#include <Library/CpuLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/DebugLib.h>
#include <Library/FdtLib.h>
#include <Library/HobLib.h>
#include <Library/PlatformSecLib.h>
#include <Library/PrePiHobListPointerLib.h>
#include <Library/PcdLib.h>
#include <Library/PrePiLib.h>
#include <Library/SerialPortLib.h>
#include <Library/BaseRiscVSbiLib.h>
#include <Library/BaseMemoryLib.h>

//
// Size temporary region to store SEC handoff data for PEI
//
#define SEC_HANDOFF_DATA_RESERVE_SIZE  SIZE_4KB

/**
  Entry point to the C language phase of SEC for this platform. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.

  @param BootHartId           The hardware thread (Hart) ID of the current CPU.
  @param DeviceTreeAddress    Address of the device tree provided to the SEC phase.
  @param TempRamBase          Base address of the temporary memory.
  @param TempRamSize          Size of the temporary memory region.
**/
VOID
NORETURN
EFIAPI
SecStartupPlatform (
  IN  UINTN   BootHartId,
  IN  VOID    *DeviceTreeAddress,
  IN  UINT32  TempRamBase,
  IN  UINT32  TempRamSize
  );

/**
  Perform Platform initialization.

  @param  FdtPointer      The pointer to the device tree.

  @return EFI_SUCCESS     The platform initialized successfully.
  @retval  Others        - As the error code indicates

**/
EFI_STATUS
EFIAPI
PlatformInitialization (
  VOID  *FdtPointer
  );

/**
  Perform Memory initialization.

  @param  FdtPointer      The pointer to the device tree.

  @return EFI_SUCCESS     Memory initialized successfully.
  @retval  Others        - As the error code indicates

**/
EFI_STATUS
EFIAPI
MemoryInitialization (
  VOID  *FdtPointer
  );

/**
  Perform CPU initialization.

  @param  FdtPointer      The pointer to the device tree.

  @return EFI_SUCCESS     CPU initialized successfully.
  @retval  Others        - As the error code indicates

**/
EFI_STATUS
EFIAPI
CpuInitialization (
  VOID  *FdtPointer
  );

#endif /* PLATFORM_SEC_LIB_ */
