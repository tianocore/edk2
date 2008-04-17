/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  IpfReset.c

Abstract:

--*/

#include "Cf9Reset.h"

SAL_RETURN_REGS
ResetEsalServicesClassCommonEntry (
  IN  UINT64                     FunctionId,
  IN  UINT64                     Arg2,
  IN  UINT64                     Arg3,
  IN  UINT64                     Arg4,
  IN  UINT64                     Arg5,
  IN  UINT64                     Arg6,
  IN  UINT64                     Arg7,
  IN  UINT64                     Arg8,
  IN  SAL_EXTENDED_SAL_PROC      ExtendedSalProc,
  IN   BOOLEAN                   VirtualMode,
  IN  VOID                       *Global
  )
/*++

Routine Description:

  Main entry for Extended SAL Reset Services

Arguments:

  FunctionId    Function Id which needed to be called.
  Arg2          EFI_RESET_TYPE, whether WARM of COLD reset
  Arg3          Last EFI_STATUS 
  Arg4          Data Size of UNICODE STRING passed in ARG5
  Arg5          Unicode String which CHAR16*

Returns:

  SAL_RETURN_REGS

--*/
// TODO:    Arg6 - add argument and description to function comment
// TODO:    Arg7 - add argument and description to function comment
// TODO:    Arg8 - add argument and description to function comment
// TODO:    ExtendedSalProc - add argument and description to function comment
// TODO:    VirtualMode - add argument and description to function comment
// TODO:    Global - add argument and description to function comment
{
  SAL_RETURN_REGS ReturnVal;

  switch (FunctionId) {
  case ResetSystem:
    KbcResetSystem (Arg2, Arg3, (UINTN) Arg4, (VOID *) Arg5);
    ReturnVal.Status = EFI_SUCCESS;
    break;

  default:
    ReturnVal.Status = EFI_SAL_INVALID_ARGUMENT;
    break;
  }

  return ReturnVal;
}

EFI_STATUS
EFIAPI
InitializeReset (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

  Initialize the state information for the Reset Architectural Protocol

Arguments:

  ImageHandle of the loaded driver
  Pointer to the System Table

Returns:

  Status

  EFI_SUCCESS           - thread can be successfully created
  EFI_OUT_OF_RESOURCES  - cannot allocate protocol data structure
  EFI_DEVICE_ERROR      - cannot create the timer service

--*/
// TODO:    SystemTable - add argument and description to function comment
{
  EfiInitializeRuntimeDriverLib (ImageHandle, SystemTable, NULL);

  RegisterEsalClass (
    &gEfiExtendedSalResetServicesProtocolGuid,
    NULL,
    ResetEsalServicesClassCommonEntry,
    ResetSystem,
    NULL
    );

  return EFI_SUCCESS;
}

