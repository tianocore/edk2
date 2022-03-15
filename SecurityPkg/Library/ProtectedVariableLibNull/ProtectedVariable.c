/** @file
  NULL version of ProtectedVariableLib used to disable protected variable services.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ProtectedVariableLib.h>

/**

  Initialization for protected varibale services.

  @param[in]  ContextIn   Pointer to variable service context needed by
                          protected variable.

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibInitialize (
  IN  PROTECTED_VARIABLE_CONTEXT_IN  *ContextIn
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Get a verified copy of NV variable storage.

  @param[out]     VariableFvHeader      Pointer to the header of whole NV firmware volume.
  @param[out]     VariableStoreHeader   Pointer to the header of variable storage.

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetStore (
  OUT EFI_FIRMWARE_VOLUME_HEADER  **VariableFvHeader,
  OUT VARIABLE_STORE_HEADER       **VariableStoreHeader
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Prepare for variable update.

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibWriteInit (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Update a variable with protection provided by this library.

  @param[in,out]  CurrVariable        Variable to be updated. It's NULL if
                                      adding a new variable.
  @param[in]      CurrVariableInDel   In-delete-transiion copy of updating variable.
  @param[in,out]  NewVariable         Buffer of new variable data.
                                      Buffer of "MetaDataHmacVar" and new
                                      variable (encrypted).
  @param[in,out]  NewVariableSize     Size of NewVariable.
                                      Size of (encrypted) NewVariable and
                                      "MetaDataHmacVar".

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibUpdate (
  IN  OUT VARIABLE_HEADER  *CurrVariable,
  IN      VARIABLE_HEADER  *CurrVariableInDel,
  IN  OUT VARIABLE_HEADER  *NewVariable,
  IN  OUT UINTN            *NewVariableSize
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Finalize a variable updating after it's written to NV variable storage
  successfully.

  @param[in]      NewVariable       Buffer of new variables and MetaDataHmacVar.
  @param[in]      VariableSize      Size of buffer pointed by NewVariable.
  @param[in]      Offset            Offset to NV variable storage from where the new
                                    variable and MetaDataHmacVar have been written.

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibWriteFinal (
  IN  VARIABLE_HEADER  *NewVariable,
  IN  UINTN            VariableSize,
  IN  UINT64           StoreIndex
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Perform garbage collection against the cached copy of NV variable storage.

  @param[in]      VariableStoreBuffer         Buffer used to do the reclaim.
  @param[out]     LastVariableOffset          New free space start point.
  @param[in]      CurrVariableOffset          Offset of existing variable.
  @param[in]      CurrVariableInDelOffset     Offset of old copy of existing variable.
  @param[in,out]  NewVariable                 Buffer of new variable data.
  @param[in]      NewVariableSize             Size of new variable data.
  @param[in,out]  HwErrVariableTotalSize      Total size of variables with HR attribute.
  @param[in,out]  CommonVariableTotalSize     Total size of common variables.
  @param[in,out]  CommonUserVariableTotalSize Total size of user variables.

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibReclaim (
  IN      VARIABLE_STORE_HEADER  *VariableStoreBuffer,
  OUT     UINTN                  *LastVariableOffset,
  IN      UINTN                  CurrVariableOffset,
  IN      UINTN                  CurrVariableInDelOffset,
  IN  OUT VARIABLE_HEADER        **NewVariable,
  IN      UINTN                  NewVariableSize,
  IN  OUT UINTN                  *HwErrVariableTotalSize,
  IN  OUT UINTN                  *CommonVariableTotalSize,
  IN  OUT UINTN                  *CommonUserVariableTotalSize
  )
{
  return EFI_UNSUPPORTED;
}

/**

  An alternative version of ProtectedVariableLibGetData to get plain data, if
  encrypted, from given variable, for different use cases.

  @param[in,out]      VarInfo     Pointer to structure containing variable information.

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetDataInfo (
  IN  OUT PROTECTED_VARIABLE_INFO  *VarInfo
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Retrieve plain data, if encrypted, of given variable.

  @param[in]      Variable           Pointer to header of a Variable.
  @param[in,out]  Data               Pointer to plain data of the given variable.
  @param[in,out]  DataSize           Size of data returned or data buffer needed.
  @param[in]      AuthFlag           Auth-variable indicator.

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetData (
  IN      VARIABLE_HEADER  *Variable,
  IN  OUT VOID             *Data,
  IN  OUT UINT32           *DataSize,
  IN      BOOLEAN          AuthFlag
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Retrieve next protected variable stub.

  @param[in,out]      VarInfo     Pointer to structure containing variable information.

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetNextEx (
  IN  OUT PROTECTED_VARIABLE_INFO  *VarInfo
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Retrieve next protected variable.

  @param[in,out]      VariableNameSize    Pointer to size of variable name.
  @param[in,out]      VariableName        Pointer to variable name.
  @param[in,out]      VariableGuid        Pointer to vairable GUID.

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetNext (
  IN OUT UINTN     *VariableNameSize,
  IN OUT CHAR16    *VariableName,
  IN OUT EFI_GUID  *VariableGuid
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Get the specified protected variable.

  @param[in]      VariableName      Pointer to variable name.
  @param[in]      VariableGuid      Pointer to vairable GUID.
  @param[out]     Attributes        Pointer to attributes.
  @param[in,out]  DataSize          Pointer to data size.
  @param[out]     Data              Pointer to data.

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGet (
  IN      CONST  CHAR16    *VariableName,
  IN      CONST  EFI_GUID  *VariableGuid,
  OUT UINT32               *Attributes,
  IN  OUT UINTN            *DataSize,
  OUT VOID                 *Data OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Find the protected variable.

  @param[in,out]      VarInfo     Pointer to structure containing variable information.

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibFind (
  IN  OUT PROTECTED_VARIABLE_INFO  *VarInfo
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Find next protected variable.

  @param[in,out]      VariableNameSize    Pointer to size of variable name.
  @param[in,out]      VariableName        Pointer to variable name.
  @param[in,out]      VariableGuid        Pointer to vairable GUID.

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibFindNext (
  IN OUT UINTN     *VariableNameSize,
  IN OUT CHAR16    *VariableName,
  IN OUT EFI_GUID  *VariableGuid
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Find next protected variable stub.

  @param[in,out]      VarInfo     Pointer to structure containing variable information.

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibFindNextEx (
  IN  OUT PROTECTED_VARIABLE_INFO  *VarInfo
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Get protected variable by information.

  @param[in,out]      VarInfo     Pointer to structure containing variable information.

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetByInfo (
  IN  OUT PROTECTED_VARIABLE_INFO  *VarInfo
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Get protected variable by name.

  @param[in]      VariableName      Pointer to variable name.
  @param[in]      VariableGuid      Pointer to vairable GUID.
  @param[out]     Attributes        Pointer to attributes.
  @param[in,out]  DataSize          Pointer to data size.
  @param[out]     Data              Pointer to data.

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetByName (
  IN      CONST  CHAR16    *VariableName,
  IN      CONST  EFI_GUID  *VariableGuid,
  OUT UINT32               *Attributes,
  IN  OUT UINTN            *DataSize,
  OUT VOID                 *Data OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Get protected variable by name.

  @param[in]      Variable      Pointer to variable name.
  @param[in,out]  Data          Pointer to variable data.
  @param[in,out]  DataSize      Pointer to data size.
  @param[out]     AuthFlag      Authenticate flag.

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetByBuffer (
  IN      VARIABLE_HEADER  *Variable,
  IN  OUT VOID             *Data,
  IN  OUT UINT32           *DataSize,
  IN      BOOLEAN          AuthFlag
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Refresh variable information changed by variable service.

  @param Variable         Pointer to buffer of the updated variable.
  @param VariableSize     Size of variable pointed by Variable.
  @param StoreIndex       New index of the variable in store.
  @param RefreshData      Flag to indicate if the variable has been updated.

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.
**/
EFI_STATUS
EFIAPI
ProtectedVariableLibRefresh (
  IN  VARIABLE_HEADER  *Variable,
  IN  UINTN            VariableSize,
  IN  UINT64           StoreIndex,
  IN  BOOLEAN          RefreshData
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Get sorted protected variable list.

  @param[in,out]  Buffer        Pointer to buffer.
  @param[in,out]  NumElements   Pointer to number of elements.

  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetSortedList (
  IN  OUT  EFI_PHYSICAL_ADDRESS  **Buffer,
  IN  OUT  UINTN                 *NumElements
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Determine if the variable is the HMAC variable.

  @param[in]  VariableName   Pointer to variable name.

  @return FALSE     Variable is not HMAC variable

**/
BOOLEAN
ProtectedVariableLibIsHmac (
  IN CHAR16  *VariableName
  )
{
  return FALSE;
}
