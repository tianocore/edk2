/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  IpfVariable.c

Abstract:

Revision History

--*/

#include "Variable.h"

//
// Don't use module globals after the SetVirtualAddress map is signaled
//

STATIC
SAL_RETURN_REGS
EsalVariableCommonEntry (
  IN  UINT64                                      FunctionId,
  IN  UINT64                                      Arg2,
  IN  UINT64                                      Arg3,
  IN  UINT64                                      Arg4,
  IN  UINT64                                      Arg5,
  IN  UINT64                                      Arg6,
  IN  UINT64                                      Arg7,
  IN  UINT64                                      Arg8,
  IN  SAL_EXTENDED_SAL_PROC                       ExtendedSalProc,
  IN  BOOLEAN                                     VirtualMode,
  IN  ESAL_VARIABLE_GLOBAL                        *Global
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  SAL_RETURN_REGS ReturnVal;

  switch (FunctionId) {
  case EsalGetVariableFunctionId:
    ReturnVal.Status = GetVariable (
                        (CHAR16 *) Arg2,
                        (EFI_GUID *) Arg3,
                        (UINT32 *) Arg4,
                        (UINTN *) Arg5,
                        (VOID *) Arg6,
                        &Global->VariableGlobal[VirtualMode],
                        Global->FvbInstance
                        );
    return ReturnVal;

  case EsalGetNextVariableNameFunctionId:
    ReturnVal.Status = GetNextVariableName (
                        (UINTN *) Arg2,
                        (CHAR16 *) Arg3,
                        (EFI_GUID *) Arg4,
                        &Global->VariableGlobal[VirtualMode],
                        Global->FvbInstance
                        );
    return ReturnVal;

  case EsalSetVariableFunctionId:
    ReturnVal.Status = SetVariable (
                        (CHAR16 *) Arg2,
                        (EFI_GUID *) Arg3,
                        (UINT32) Arg4,
                        (UINTN) Arg5,
                        (VOID *) Arg6,
                        &Global->VariableGlobal[VirtualMode],
                        (UINTN *) &Global->VolatileLastVariableOffset,
                        (UINTN *) &Global->NonVolatileLastVariableOffset,
                        Global->FvbInstance
                        );
    return ReturnVal;

  case EsalQueryVariableInfoFunctionId:
    ReturnVal.Status = QueryVariableInfo (
                        (UINT32) Arg2,
                        (UINT64 *) Arg3,
                        (UINT64 *) Arg4,
                        (UINT64 *) Arg5,
                        &Global->VariableGlobal[VirtualMode],
                        Global->FvbInstance
                        );
    return ReturnVal;

  default:
    ReturnVal.Status = EFI_SAL_INVALID_ARGUMENT;
    return ReturnVal;
  }
}


VOID
VariableClassAddressChangeEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  CopyMem (
    &mVariableModuleGlobal->VariableGlobal[Virtual],
    &mVariableModuleGlobal->VariableGlobal[Physical],
    sizeof (VARIABLE_GLOBAL)
    );

  EfiConvertPointer (
    0x0,
    (VOID **) &mVariableModuleGlobal->VariableGlobal[Virtual].NonVolatileVariableBase
    );
  EfiConvertPointer (
    0x0,
    (VOID **) &mVariableModuleGlobal->VariableGlobal[Virtual].VolatileVariableBase
    );
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal);
}

EFI_STATUS
VariableServiceInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  EFI_STATUS  Status;

  Status = VariableCommonInitialize (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);

  //
  //  Register All the Functions with Extended Sal.
  //
  RegisterEsalClass (
    &gEfiExtendedSalVariableServicesProtocolGuid,
    mVariableModuleGlobal,
    EsalVariableCommonEntry,
    EsalGetVariableFunctionId,
    EsalVariableCommonEntry,
    EsalGetNextVariableNameFunctionId,
    EsalVariableCommonEntry,
    EsalSetVariableFunctionId,
    EsalVariableCommonEntry,
    EsalQueryVariableInfoFunctionId,
    NULL
    );

  return EFI_SUCCESS;
}
