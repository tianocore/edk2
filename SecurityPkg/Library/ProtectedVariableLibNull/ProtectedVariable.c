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

  If this initialization failed upon any error, the whole variable services
  should not be used.  A system reset might be needed to re-construct NV
  variable storage to be the default state.

  @param[in]  ContextIn   Pointer to variable service context needed by
                          protected variable.

  @retval EFI_SUCCESS               Protected variable services are ready.
  @retval EFI_INVALID_PARAMETER     If ContextIn == NULL or something missing or
                                    mismatching in the content in ContextIn.
  @retval EFI_COMPROMISED_DATA      If failed to check integrity of protected variables.
  @retval EFI_OUT_OF_RESOURCES      Fail to allocate enough resource.
  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibInitialize (
  IN  PROTECTED_VARIABLE_CONTEXT_IN   *ContextIn
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Get a verified copy of NV variable storage.

  @param[out]     VariableFvHeader      Pointer to the header of whole NV firmware volume.
  @param[out]     VariableStoreHeader   Pointer to the header of variable storage.

  @retval EFI_SUCCESS             A copy of NV variable storage is returned
                                  successfully.
  @retval EFI_NOT_FOUND           The NV variable storage is not found or cached.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetStore (
  OUT EFI_FIRMWARE_VOLUME_HEADER            **VariableFvHeader,
  OUT VARIABLE_STORE_HEADER                 **VariableStoreHeader
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Prepare for variable update.

  This is needed only once during current boot to mitigate replay attack. Its
  major job is to advance RPMC (Replay Protected Monotonic Counter).

  @retval EFI_SUCCESS             Variable is ready to update hereafter.
  @retval EFI_UNSUPPORTED         Updating variable is not supported.
  @retval EFI_DEVICE_ERROR        Error in advancing RPMC.

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

  If variable encryption is employed, the new variable data will be encrypted
  before being written to NV variable storage.

  A special variable, called "MetaDataHmacVar", will always be updated along
  with variable being updated to reflect the changes (HMAC value) of all
  protected valid variables. The only exceptions, currently, are variable
  "MetaDataHmacVar" itself and variable "VarErrorLog".

  The buffer passed by NewVariable must be double of maximum variable size,
  which allows to pass the "MetaDataHmacVar" back to caller along with encrypted
  new variable data, if any. This can make sure the new variable data and
  "MetaDataHmacVar" can be written at almost the same time to reduce the changce
  of compromising the integrity.

  If *NewVariableSize is zero, it means to delete variable passed by CurrVariable
  and/or CurrVariableIndel. "MetaDataHmacVar" will be updated as well in such
  case because of less variables in storage. NewVariable should be always passed
  in to convey new "MetaDataHmacVar" back.

  @param[in,out]  CurrVariable        Variable to be updated. It's NULL if
                                      adding a new variable.
  @param[in,out]  CurrVariableInDel   In-delete-transiion copy of updating variable.
  @param[in]      NewVariable         Buffer of new variable data.
  @param[out]     NewVariable         Buffer of "MetaDataHmacVar" and new
                                      variable (encrypted).
  @param[in]      NewVariableSize     Size of NewVariable.
  @param[out]     NewVariableSize     Size of (encrypted) NewVariable and
                                      "MetaDataHmacVar".

  @retval EFI_SUCCESS             The variable is updated with protection successfully.
  @retval EFI_INVALID_PARAMETER   NewVariable is NULL.
  @retval EFI_NOT_FOUND           Information missing to finish the operation.
  @retval EFI_ABORTED             Failed to encypt variable or calculate HMAC.
  @retval EFI_NOT_READY           The RPMC device is not yet initialized.
  @retval EFI_DEVICE_ERROR        The RPMC device has error in updating.
  @retval EFI_ACCESS_DENIED       The given variable is not allowed to update.
                                  Currently this only happens on updating
                                  "MetaDataHmacVar" from code outside of this
                                  library.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibUpdate (
  IN  OUT VARIABLE_HEADER             *CurrVariable,
  IN      VARIABLE_HEADER             *CurrVariableInDel,
  IN  OUT VARIABLE_HEADER             *NewVariable,
  IN  OUT UINTN                       *NewVariableSize
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Finalize a variable updating after it's written to NV variable storage
  successfully.

  This usually includes works like increasing RPMC, synchronizing local cache,
  updating new position of "MetaDataHmacVar", deleting old copy of "MetaDataHmacVar"
  completely, etc.

  @param[in]      NewVariable       Buffer of new variables and MetaDataHmacVar.
  @param[in]      VariableSize      Size of buffer pointed by NewVariable.
  @param[in]      Offset            Offset to NV variable storage from where the new
                                    variable and MetaDataHmacVar have been written.

  @retval EFI_SUCCESS         No problem in winding up the varible write operation.
  @retval Others              Failed to updating state of old copy of updated
                              variable, or failed to increase RPMC, etc.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibWriteFinal (
  IN  VARIABLE_HEADER         *NewVariable,
  IN  UINTN                   VariableSize,
  IN  UINT64                  StoreIndex
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Perform garbage collection against the cached copy of NV variable storage.

  @param[in,out]  VariableStoreBuffer         Buffer used to do the reclaim.
  @param[out]     LastVariableOffset          New free space start point.
  @param[in]      CurrVariableOffset          Offset of existing variable.
  @param[in]      CurrVariableInDelOffset     Offset of old copy of existing variable.
  @param[in,out]  NewVariable                 Buffer of new variable data.
  @param[in]      NewVariableSize             Size of new variable data.
  @param[out]     HwErrVariableTotalSize      Total size of variables with HR attribute.
  @param[out]     CommonVariableTotalSize     Total size of common variables.
  @param[out]     CommonUserVariableTotalSize Total size of user variables.

  @retval EFI_SUCCESS               Reclaim is performed successfully.
  @retval EFI_INVALID_PARAMETER     Reclaim buffer is not given or big enough.
  @retval EFI_OUT_OF_RESOURCES      Not enought buffer to complete the reclaim.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibReclaim (
  IN      VARIABLE_STORE_HEADER       *VariableStoreBuffer,
  OUT     UINTN                       *LastVariableOffset,
  IN      UINTN                       CurrVariableOffset,
  IN      UINTN                       CurrVariableInDelOffset,
  IN  OUT VARIABLE_HEADER             **NewVariable,
  IN      UINTN                       NewVariableSize,
  IN  OUT UINTN                       *HwErrVariableTotalSize,
  IN  OUT UINTN                       *CommonVariableTotalSize,
  IN  OUT UINTN                       *CommonUserVariableTotalSize
  )
{
  return EFI_UNSUPPORTED;
}

/**

  An alternative version of ProtectedVariableLibGetData to get plain data, if
  encrypted, from given variable, for different use cases.

  @param[in,out]      VarInfo     Pointer to structure containing variable information.

  @retval EFI_SUCCESS               Found the specified variable.
  @retval EFI_INVALID_PARAMETER     VarInfo is NULL or both VarInfo->Address and
                                    VarInfo->Offset are invalid.
  @retval EFI_NOT_FOUND             The specified variable could not be found.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetDataInfo (
  IN  OUT PROTECTED_VARIABLE_INFO       *VarInfo
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Retrieve plain data, if encrypted, of given variable.

  If variable encryption is employed, this function will initiate a SMM request
  to get the plain data. Due to security consideration, the decrytpion can only
  be done in SMM environment.

  @param[in]      Variable           Pointer to header of a Variable.
  @param[out]     Data               Pointer to plain data of the given variable.
  @param[in, out] DataSize           Size of data returned or data buffer needed.
  @param[in]      AuthFlag           Auth-variable indicator.

  @retval EFI_SUCCESS                Found the specified variable.
  @retval EFI_INVALID_PARAMETER      Invalid parameter.
  @retval EFI_NOT_FOUND              The specified variable could not be found.
  @retval EFI_BUFFER_TOO_SMALL       If *DataSize is smaller than needed.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetData (
  IN      VARIABLE_HEADER                   *Variable,
  IN  OUT VOID                              *Data,
  IN  OUT UINT32                            *DataSize,
  IN      BOOLEAN                           AuthFlag
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
ProtectedVariableLibGetNextEx (
  IN  OUT PROTECTED_VARIABLE_INFO       *VarInfo
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
ProtectedVariableLibGetNext (
  IN OUT UINTN                              *VariableNameSize,
  IN OUT CHAR16                             *VariableName,
  IN OUT EFI_GUID                           *VariableGuid
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
ProtectedVariableLibGet (
  IN      CONST  CHAR16                   *VariableName,
  IN      CONST  EFI_GUID                 *VariableGuid,
      OUT UINT32                          *Attributes,
  IN  OUT UINTN                           *DataSize,
      OUT VOID                            *Data OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
ProtectedVariableLibFind (
  IN  OUT PROTECTED_VARIABLE_INFO       *VarInfo
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
ProtectedVariableLibFindNext (
  IN OUT UINTN                              *VariableNameSize,
  IN OUT CHAR16                             *VariableName,
  IN OUT EFI_GUID                           *VariableGuid
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
ProtectedVariableLibFindNextEx (
  IN  OUT PROTECTED_VARIABLE_INFO       *VarInfo
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
ProtectedVariableLibGetByInfo (
  IN  OUT PROTECTED_VARIABLE_INFO       *VarInfo
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
ProtectedVariableLibGetByName (
  IN      CONST  CHAR16                   *VariableName,
  IN      CONST  EFI_GUID                 *VariableGuid,
      OUT UINT32                          *Attributes,
  IN  OUT UINTN                           *DataSize,
      OUT VOID                            *Data OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
ProtectedVariableLibGetByBuffer (
  IN      VARIABLE_HEADER                   *Variable,
  IN  OUT VOID                              *Data,
  IN  OUT UINT32                            *DataSize,
  IN      BOOLEAN                           AuthFlag
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

  @return EFI_SUCCESS     No error occurred in updating.
  @return EFI_NOT_FOUND   The given variable was not found in
                          ProtectedVariableLib.
**/
EFI_STATUS
EFIAPI
ProtectedVariableLibRefresh (
  IN  VARIABLE_HEADER         *Variable,
  IN  UINTN                   VariableSize,
  IN  UINT64                  StoreIndex,
  IN  BOOLEAN                 RefreshData
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
ProtectedVariableLibGetSortedList (
  IN  OUT  EFI_PHYSICAL_ADDRESS     **Buffer,
  IN  OUT  UINTN                    *NumElements
  )
{
  return EFI_UNSUPPORTED;
}

/**

  Determine if the variable is the HMAC variable

  @param VariableName   Pointer to variable name.

  @return TRUE      Variable is HMAC variable
  @return FALSE     Variable is not HMAC variable

**/
BOOLEAN
ProtectedVariableLibIsHmac (
  IN CHAR16             *VariableName
  )
{
  return FALSE;
}
