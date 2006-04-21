/** @file
  UEFI Runtime Services Table Library.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  UefiRuntimeServicesTableLib.c

**/

//
// Cached copy of the EFI Runtime Services Table
//
EFI_RUNTIME_SERVICES  *gRT = NULL;

/**
**/
EFI_STATUS
UefiRuntimeServicesTableLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // Cache pointer to the EFI Runtime Services Table
  //
  gRT = SystemTable->RuntimeServices;
  ASSERT (gRT != NULL);
  return EFI_SUCCESS;
}
