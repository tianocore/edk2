/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DxeDriverLib.c

Abstract:

  Light weight lib to support EFI drivers.

--*/

#include "Tiano.h"
#include "EfiDriverLib.h"

EFI_STATUS
DxeInitializeDriverLib (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:

  Intialize Driver Lib if it has not yet been initialized. 

Arguments:

  ImageHandle     - Standard EFI Image entry parameter
  
  SystemTable     - Standard EFI Image entry parameter

Returns: 

  EFI_STATUS always returns EFI_SUCCESS

--*/
{
  EFI_STATUS  Status;

  Status = EfiInitializeDriverLib (ImageHandle, SystemTable);
  if (!EFI_ERROR (Status)) {
    Status = EfiLibGetSystemConfigurationTable (&gEfiDxeServicesTableGuid, (VOID **) &gDS);
  }

  return Status;
}
