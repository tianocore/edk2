/** @file
  Include file for SMM Access2 Protocol on SMM Access Protocol Thunk driver.
  
  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef  _SMM_ACCESS2_ON_SMM_ACCESS_THUNK_H_
#define  _SMM_ACCESS2_ON_SMM_ACCESS_THUNK_H_

#include <PiDxe.h>
#include <FrameworkSmm.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Protocol/SmmAccess2.h>
#include <Protocol/SmmAccess.h>

/**
  Opens the SMRAM area to be accessible by a boot-service driver.

  This function "opens" SMRAM so that it is visible while not inside of SMM. The function should 
  return EFI_UNSUPPORTED if the hardware does not support hiding of SMRAM. The function 
  should return EFI_DEVICE_ERROR if the SMRAM configuration is locked.

  @param[in] This           The EFI_SMM_ACCESS2_PROTOCOL instance.

  @retval EFI_SUCCESS       The operation was successful.
  @retval EFI_UNSUPPORTED   The system does not support opening and closing of SMRAM.
  @retval EFI_DEVICE_ERROR  SMRAM cannot be opened, perhaps because it is locked.
**/
EFI_STATUS
EFIAPI
SmmAccess2Open (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  );

/**
  Inhibits access to the SMRAM.

  This function "closes" SMRAM so that it is not visible while outside of SMM. The function should 
  return EFI_UNSUPPORTED if the hardware does not support hiding of SMRAM.

  @param[in] This           The EFI_SMM_ACCESS2_PROTOCOL instance.

  @retval EFI_SUCCESS       The operation was successful.
  @retval EFI_UNSUPPORTED   The system does not support opening and closing of SMRAM.
  @retval EFI_DEVICE_ERROR  SMRAM cannot be closed.
**/
EFI_STATUS
EFIAPI
SmmAccess2Close (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  );

/**
  Inhibits access to the SMRAM.

  This function prohibits access to the SMRAM region.  This function is usually implemented such 
  that it is a write-once operation. 

  @param[in] This          The EFI_SMM_ACCESS2_PROTOCOL instance.

  @retval EFI_SUCCESS      The device was successfully locked.
  @retval EFI_UNSUPPORTED  The system does not support locking of SMRAM.
**/
EFI_STATUS
EFIAPI
SmmAccess2Lock (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  );

/**
  Queries the memory controller for the possible regions that will support SMRAM.

  @param[in]     This           The EFI_SMM_ACCESS2_PROTOCOL instance.
  @param[in, out] SmramMapSize   A pointer to the size, in bytes, of the SmramMemoryMap buffer.
  @param[in, out] SmramMap       A pointer to the buffer in which firmware places the current memory map.

  @retval EFI_SUCCESS           The chipset supported the given resource.
  @retval EFI_BUFFER_TOO_SMALL  The SmramMap parameter was too small.  The current buffer size 
                                needed to hold the memory map is returned in SmramMapSize.
**/
EFI_STATUS
EFIAPI
SmmAccess2GetCapabilities (
  IN CONST EFI_SMM_ACCESS2_PROTOCOL  *This,
  IN OUT UINTN                       *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR        *SmramMap
  );

#endif  
