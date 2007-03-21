/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  InitVariable.c

Abstract:

Revision History

--*/

#include "Variable.h"

//
// Don't use module globals after the SetVirtualAddress map is signaled
//
extern ESAL_VARIABLE_GLOBAL *mVariableModuleGlobal;

EFI_STATUS
EFIAPI
RuntimeServiceGetVariable (
  IN CHAR16        *VariableName,
  IN EFI_GUID      * VendorGuid,
  OUT UINT32       *Attributes OPTIONAL,
  IN OUT UINTN     *DataSize,
  OUT VOID         *Data
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  return GetVariable (
          VariableName,
          VendorGuid,
          Attributes OPTIONAL,
          DataSize,
          Data,
          &mVariableModuleGlobal->VariableGlobal[Physical],
          mVariableModuleGlobal->FvbInstance
          );
}

EFI_STATUS
EFIAPI
RuntimeServiceGetNextVariableName (
  IN OUT UINTN     *VariableNameSize,
  IN OUT CHAR16    *VariableName,
  IN OUT EFI_GUID  *VendorGuid
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  return GetNextVariableName (
          VariableNameSize,
          VariableName,
          VendorGuid,
          &mVariableModuleGlobal->VariableGlobal[Physical],
          mVariableModuleGlobal->FvbInstance
          );
}

EFI_STATUS
EFIAPI
RuntimeServiceSetVariable (
  IN CHAR16        *VariableName,
  IN EFI_GUID      *VendorGuid,
  IN UINT32        Attributes,
  IN UINTN         DataSize,
  IN VOID          *Data
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  return SetVariable (
          VariableName,
          VendorGuid,
          Attributes,
          DataSize,
          Data,
          &mVariableModuleGlobal->VariableGlobal[Physical],
          &mVariableModuleGlobal->VolatileLastVariableOffset,
          &mVariableModuleGlobal->NonVolatileLastVariableOffset,
          mVariableModuleGlobal->FvbInstance
          );
}

EFI_STATUS
EFIAPI
RuntimeServiceQueryVariableInfo (
  IN  UINT32                 Attributes,
  OUT UINT64                 *MaximumVariableStorageSize,
  OUT UINT64                 *RemainingVariableStorageSize,
  OUT UINT64                 *MaximumVariableSize
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  return QueryVariableInfo (
          Attributes,
          MaximumVariableStorageSize,
          RemainingVariableStorageSize,
          MaximumVariableSize,
          &mVariableModuleGlobal->VariableGlobal[Physical],
          mVariableModuleGlobal->FvbInstance
          );
}

VOID
EFIAPI
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
  EfiConvertPointer (
    0x0,
    (VOID **) &mVariableModuleGlobal->VariableGlobal[Physical].NonVolatileVariableBase
    );
  EfiConvertPointer (
    0x0,
    (VOID **) &mVariableModuleGlobal->VariableGlobal[Physical].VolatileVariableBase
    );
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal);
}

EFI_STATUS
EFIAPI
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
  EFI_HANDLE  NewHandle;
  EFI_STATUS  Status;

  Status = VariableCommonInitialize (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);

  SystemTable->RuntimeServices->GetVariable         = RuntimeServiceGetVariable;
  SystemTable->RuntimeServices->GetNextVariableName = RuntimeServiceGetNextVariableName;
  SystemTable->RuntimeServices->SetVariable         = RuntimeServiceSetVariable;
  SystemTable->RuntimeServices->QueryVariableInfo   = RuntimeServiceQueryVariableInfo;

  //
  // Now install the Variable Runtime Architectural Protocol on a new handle
  //
  NewHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &NewHandle,
                  &gEfiVariableArchProtocolGuid,
                  NULL,
                  &gEfiVariableWriteArchProtocolGuid,
                  NULL,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
