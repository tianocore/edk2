/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Ia32EfiRuntimeDriverLib.h

Abstract:

  Light weight lib to support IA32 EFI Libraries.

--*/

#ifndef _IA32_EFI_RUNTIME_LIB_H_
#define _IA32_EFI_RUNTIME_LIB_H_

#include "Tiano.h"
#include "EfiRuntimeLib.h"
#include EFI_PROTOCOL_DEFINITION (ExtendedSalGuid)

typedef
EFI_STATUS
(EFIAPI *COMMON_PROC_ENTRY) (
  IN  UINTN                      FunctionId,
  IN  UINTN                      Arg2,
  IN  UINTN                      Arg3,
  IN  UINTN                      Arg4,
  IN  UINTN                      Arg5,
  IN  UINTN                      Arg6,
  IN  UINTN                      Arg7,
  IN  UINTN                      Arg8
  );

typedef struct {
  COMMON_PROC_ENTRY CommonProcEntry;
} COMMON_PROC_ENTRY_STRUCT;

EFI_STATUS
InstallPlatformRuntimeLib (
  IN  EFI_GUID                      *Guid,
  IN  COMMON_PROC_ENTRY_STRUCT      *CommonEntry
  )
/*++

Routine Description:

  Install platform runtime lib.

Arguments:

  Guid                  - Guid for runtime lib
  CommonEntry           - Common entry

Returns: 

  Status code

--*/
;

EFI_STATUS
GetPlatformRuntimeLib (
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:

  Get platform runtime lib.

Arguments:

  SystemTable           - Pointer to system table

Returns: 

  Status code

--*/
;

EFI_STATUS
ConvertPlatformRuntimeLibPtr (
  IN EFI_RUNTIME_SERVICES  *mRT
  )
/*++

Routine Description:

  Convert platform runtime lib pointer.  

Arguments:

  mRT                   - Pointer to runtime service table.

Returns: 

  Status code

--*/
;

#endif
