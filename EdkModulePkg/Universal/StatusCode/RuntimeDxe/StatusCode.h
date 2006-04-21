/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  StatusCode.h

Abstract:

  EFI DXE/RT Status Code include file.

--*/

#ifndef _EFI_RUNTIME_STATUS_CODE_H_
#define _EFI_RUNTIME_STATUS_CODE_H_

//
// Function prototypes
//

EFI_STATUS
EFIAPI
StatusCodeReportStatusCode (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId,
  IN EFI_STATUS_CODE_DATA     * Data OPTIONAL
  )
;

EFI_STATUS
InitializeStatusCode (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
;

EFI_STATUS
InstallStatusCodeDebugAssert (
  VOID
  )
;

//
// Driver entry point
//
EFI_STATUS
EFIAPI
InstallStatusCode (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
;

#endif
