/** @file

  Definitions for reset

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _KBC_RESET_H
#define _KBC_RESET_H

#include <PiDxe.h>

#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/Reset.h>

/**
  Initialize the state information for the Reset Architectural Protocol

  @param ImageHandle     Handle of the loaded driver 
  @param SystemTable     Pointer to the System Table

  @retval EFI_SUCCESS           Thread can be successfully created
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Cannot create the timer service

**/
EFI_STATUS
EFIAPI
InitializeReset (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
;

/**
  Reset the system.

  @param ResetType       warm or cold
  @param ResetStatus     possible cause of reset
  @param DataSize        Size of ResetData in bytes
  @param ResetData       Optional Unicode string

**/
VOID
EFIAPI
KbcResetSystem (
  IN EFI_RESET_TYPE   ResetType,
  IN EFI_STATUS       ResetStatus,
  IN UINTN            DataSize,
  IN VOID             *ResetData OPTIONAL
  )
;

#endif
