/*++ @file
  Some definitions for reset.

Copyright (c) 2006 - 2010, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

--*/

#ifndef _ACPI_RESET_H
#define _ACPI_RESET_H

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/Reset.h>
#include <Guid/AcpiDescription.h>

/**
  Initialize the state information for the Reset Architectural Protocol.

  @param[in] ImageHandle  Image handle of the loaded driver
  @param[in] SystemTable  Pointer to the System Table

  @retval EFI_SUCCESS           Thread can be successfully created
  @retval EFI_UNSUPPORTED       Cannot find the info to reset system

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

  @param[in] ResetType       Warm or cold
  @param[in] ResetStatus     Possible cause of reset
  @param[in] DataSize        Size of ResetData in bytes
  @param[in] ResetData       Optional Unicode string

**/
VOID
EFIAPI
AcpiResetSystem (
  IN EFI_RESET_TYPE   ResetType,
  IN EFI_STATUS       ResetStatus,
  IN UINTN            DataSize,
  IN CHAR16           *ResetData, OPTIONAL
  IN EFI_ACPI_DESCRIPTION *AcpiDescription
  )
;

BOOLEAN
GetAcpiDescription (
  IN EFI_ACPI_DESCRIPTION *AcpiDescription
  );

#endif
