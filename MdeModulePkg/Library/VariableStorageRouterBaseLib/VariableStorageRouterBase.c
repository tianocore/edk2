/** @file
  Variable Storage Router Base Library

Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/VariableStorageRouterLib.h>

#include <Protocol/VariableStorage.h>

EDKII_VARIABLE_STORAGE_PROTOCOL  *mVariableStorage;

/**
  Check whether Variable Storage use Auth Formet.

  @return TRUE    Use Auth format.
  @return FALSE   Not use Auth format.

**/
BOOLEAN
EFIAPI
VariableStorageRouterLibCheckAuthFormat (
  VOID
  )
{
  return mVariableStorage->CheckAuthFormat ();
}

/**
  Get maximum variable size, covering both non-volatile and volatile variables.

  @return Maximum variable size.

**/
UINTN
EFIAPI
VariableStorageRouterLibGetMaxVariableSize (
  VOID
  )
{
  return mVariableStorage->GetMaxVariableSize ();
}

/**

  This code finds variable in storage blocks (Volatile or Non-Volatile).

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize is external input.
  This function will do basic validation, before parse the data.

  @param VariableName               Name of Variable to be found.
  @param VendorGuid                 Variable vendor GUID.
  @param Attributes                 Attribute value of the variable found.
  @param DataSize                   Size of Data found. If size is less than the
                                    data, this value contains the required size.
  @param Data                       The buffer to return the contents of the variable. May be NULL
                                    with a zero DataSize in order to determine the size buffer needed.

  @retval EFI_SUCCESS               The function completed successfully.
  @retval EFI_NOT_FOUND             The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL      The DataSize is too small for the result.
  @retval EFI_INVALID_PARAMETER     VariableName is NULL.
  @retval EFI_INVALID_PARAMETER     VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER     DataSize is NULL.
  @retval EFI_INVALID_PARAMETER     The DataSize is not too small and Data is NULL.
  @retval EFI_DEVICE_ERROR          The variable could not be retrieved due to a hardware error.
  @retval EFI_SECURITY_VIOLATION    The variable could not be retrieved due to an authentication failure.
  @retval EFI_UNSUPPORTED           After ExitBootServices() has been called, this return code may be returned
                                    if no variable storage is supported. The platform should describe this
                                    runtime service as unsupported at runtime via an EFI_RT_PROPERTIES_TABLE
                                    configuration table.

**/
EFI_STATUS
EFIAPI
VariableStorageRouterLibGetVariable (
  IN      CHAR16    *VariableName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINT32    *Attributes OPTIONAL,
  IN OUT  UINTN     *DataSize,
  OUT     VOID      *Data OPTIONAL
  )
{
  return mVariableStorage->GetVariable (
                             VariableName,
                             VendorGuid,
                             Attributes,
                             DataSize,
                             Data
                             );
}

/**

  This code Finds the Next available variable.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode. This function will do basic validation, before parse the data.

  @param VariableNameSize           The size of the VariableName buffer. The size must be large
                                    enough to fit input string supplied in VariableName buffer.
  @param VariableName               Pointer to variable name.
  @param VendorGuid                 Variable Vendor Guid.

  @retval EFI_SUCCESS               The function completed successfully.
  @retval EFI_NOT_FOUND             The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL      The VariableNameSize is too small for the result.
                                    VariableNameSize has been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER     VariableNameSize is NULL.
  @retval EFI_INVALID_PARAMETER     VariableName is NULL.
  @retval EFI_INVALID_PARAMETER     VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER     The input values of VariableName and VendorGuid are not a name and
                                    GUID of an existing variable.
  @retval EFI_INVALID_PARAMETER     Null-terminator is not found in the first VariableNameSize bytes of
                                    the input VariableName buffer.
  @retval EFI_DEVICE_ERROR          The variable could not be retrieved due to a hardware error.
  @retval EFI_UNSUPPORTED           After ExitBootServices() has been called, this return code may be returned
                                    if no variable storage is supported. The platform should describe this
                                    runtime service as unsupported at runtime via an EFI_RT_PROPERTIES_TABLE
                                    configuration table.

**/
EFI_STATUS
EFIAPI
VariableStorageRouterLibGetNextVariableName (
  IN OUT  UINTN     *VariableNameSize,
  IN OUT  CHAR16    *VariableName,
  IN OUT  EFI_GUID  *VendorGuid
  )
{
  return mVariableStorage->GetNextVariableName (
                             VariableNameSize,
                             VariableName,
                             VendorGuid
                             );
}

/**

  This code sets variable in storage blocks (Volatile or Non-Volatile).

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize and data are external input.
  This function will do basic validation, before parse the data.
  This function will parse the authentication carefully to avoid security issues, like
  buffer overflow, integer overflow.
  This function will check attribute carefully to avoid authentication bypass.

  @param VariableName                     Name of Variable to be found.
  @param VendorGuid                       Variable vendor GUID.
  @param Attributes                       Attribute value of the variable found
  @param DataSize                         Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param Data                             Data pointer.
  @param FromTrusted                      Whether request comes from trusted.

  @retval EFI_SUCCESS                     The function completed successfully.
  @retval EFI_NOT_FOUND                   The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL            The DataSize is too small for the result.
  @retval EFI_INVALID_PARAMETER           VariableName is NULL.
  @retval EFI_INVALID_PARAMETER           VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER           DataSize is NULL.
  @retval EFI_INVALID_PARAMETER           The DataSize is not too small and Data is NULL.
  @retval EFI_DEVICE_ERROR                The variable could not be retrieved due to a hardware error.
  @retval EFI_SECURITY_VIOLATION          The variable could not be retrieved due to an authentication failure.
  @retval EFI_UNSUPPORTED                 After ExitBootServices() has been called, this return code may be returned
                                          if no variable storage is supported. The platform should describe this
                                          runtime service as unsupported at runtime via an EFI_RT_PROPERTIES_TABLE
                                          configuration table.

**/
EFI_STATUS
EFIAPI
VariableStorageRouterLibSetVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data,
  IN BOOLEAN   FromTrusted
  )
{
  return mVariableStorage->SetVariable (
                             VariableName,
                             VendorGuid,
                             Attributes,
                             DataSize,
                             Data,
                             FromTrusted
                             );
}

/**
  This code returns information about the EFI variables.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode. This function will do basic validation, before parse the data.

  @param Attributes                     Attributes bitmask to specify the type of variables
                                        on which to return information.
  @param MaximumVariableStorageSize     Pointer to the maximum size of the storage space available
                                        for the EFI variables associated with the attributes specified.
  @param RemainingVariableStorageSize   Pointer to the remaining size of the storage space available
                                        for EFI variables associated with the attributes specified.
  @param MaximumVariableSize            Pointer to the maximum size of an individual EFI variables
                                        associated with the attributes specified.

  @return EFI_INVALID_PARAMETER         An invalid combination of attribute bits was supplied.
  @return EFI_SUCCESS                   Query successfully.
  @return EFI_UNSUPPORTED               The attribute is not supported on this platform.

**/
EFI_STATUS
EFIAPI
VariableStorageRouterLibQueryVariableInfo (
  IN  UINT32  Attributes,
  OUT UINT64  *MaximumVariableStorageSize,
  OUT UINT64  *RemainingVariableStorageSize,
  OUT UINT64  *MaximumVariableSize
  )
{
  return mVariableStorage->QueryVariableInfo (
                             Attributes,
                             MaximumVariableStorageSize,
                             RemainingVariableStorageSize,
                             MaximumVariableSize
                             );
}

/**
  Callback function at End of DXE phase notification.

  @return EFI_SUCCESS
  @return Others                   ERROR.

**/
EFI_STATUS
EFIAPI
VariableStorageRouterLibEndOfDxe (
  IN VOID
  )
{
  return mVariableStorage->EndOfDxe ();
}

/**
  Callback function at READY_TO_BOOT phase notification.

  @return EFI_SUCCESS
  @return Others                   ERROR.

**/
EFI_STATUS
EFIAPI
VariableStorageRouterLibReadyToBoot (
  IN VOID
  )
{
  return mVariableStorage->ReadyToBoot ();
}

/**
  Callback function for EXIT_BOOT phase notification.

  @return EFI_SUCCESS
  @return Others                   ERROR.

**/
EFI_STATUS
EFIAPI
VariableStorageRouterLibExitBootService (
  IN VOID
  )
{
  return mVariableStorage->ExitBootService ();
}

/**
  Get the variable statistics information from the information buffer pointed by gVariableInfo.

  Caution: This function may be invoked at SMM runtime.
  InfoEntry and InfoSize are external input. Care must be taken to make sure not security issue at runtime.

  @param[in, out]  InfoEntry    A pointer to the buffer of variable information entry.
                                On input, point to the variable information returned last time. if
                                InfoEntry->VendorGuid is zero, return the first information.
                                On output, point to the next variable information.
  @param[in, out]  InfoSize     On input, the size of the variable information buffer.
                                On output, the returned variable information size.

  @retval EFI_SUCCESS           The variable information is found and returned successfully.
  @retval EFI_UNSUPPORTED       No variable inoformation exists in variable driver.
  @retval EFI_BUFFER_TOO_SMALL  The buffer is too small to hold the next variable information.
  @retval EFI_INVALID_PARAMETER Input parameter is invalid.

**/
EFI_STATUS
EFIAPI
VariableStorageRouterLibGetStatics (
  IN OUT VARIABLE_INFO_ENTRY  *InfoEntry,
  IN OUT UINTN                *InfoSize
  )
{
  return mVariableStorage->GetStatics (
                             InfoEntry,
                             InfoSize
                             );
}

/**
  Mark a variable that will become read-only after leaving the DXE phase of execution.

  @param[in] VariableName  A pointer to the variable name that will be made read-only subsequently.
  @param[in] VendorGuid    A pointer to the vendor GUID that will be made read-only subsequently.

  @retval EFI_SUCCESS           The variable specified by the VariableName and the VendorGuid was marked
                                as pending to be read-only.
  @retval EFI_INVALID_PARAMETER VariableName or VendorGuid is NULL.
                                Or VariableName is an empty string.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource to hold the lock request.
**/
EFI_STATUS
EFIAPI
VariableStorageRouterLibRequestToLock (
  IN       CHAR16    *VariableName,
  IN       EFI_GUID  *VendorGuid
  )
{
  return mVariableStorage->RequestToLock (
                             VariableName,
                             VendorGuid
                             );
}

/**
  Variable property set.

  @param[in] Name               Pointer to the variable name.
  @param[in] Guid               Pointer to the vendor GUID.
  @param[in] VariableProperty   Pointer to the input variable property.

  @retval EFI_SUCCESS           The property of variable specified by the Name and Guid was set successfully.
  @retval EFI_INVALID_PARAMETER Name, Guid or VariableProperty is NULL, or Name is an empty string,
                                or the fields of VariableProperty are not valid.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource for the variable property set request.

**/
EFI_STATUS
EFIAPI
VariableStorageRouterLibPropertySet (
  IN CHAR16                       *Name,
  IN EFI_GUID                     *Guid,
  IN VAR_CHECK_VARIABLE_PROPERTY  *VariableProperty
  )
{
  return mVariableStorage->PropertySet (
                             Name,
                             Guid,
                             VariableProperty
                             );
}

/**
  Variable property get.

  @param[in]  Name              Pointer to the variable name.
  @param[in]  Guid              Pointer to the vendor GUID.
  @param[out] VariableProperty  Pointer to the output variable property.

  @retval EFI_SUCCESS           The property of variable specified by the Name and Guid was got successfully.
  @retval EFI_INVALID_PARAMETER Name, Guid or VariableProperty is NULL, or Name is an empty string.
  @retval EFI_NOT_FOUND         The property of variable specified by the Name and Guid was not found.

**/
EFI_STATUS
EFIAPI
VariableStorageRouterLibPropertyGet (
  IN CHAR16                        *Name,
  IN EFI_GUID                      *Guid,
  OUT VAR_CHECK_VARIABLE_PROPERTY  *VariableProperty
  )
{
  return mVariableStorage->PropertyGet (
                             Name,
                             Guid,
                             VariableProperty
                             );
}

/**
  Initialise Variable Runtime Cache.

  @param[in]   HobCache             Address of cache for variables in HobList.
  @param[in]   VolatileCache        Address of cache for variables in  volatile memory.
  @param[in]   NvCache              Address of cache for variables in  in non-volatile memory.
  @param[in]   PendingUpdate        Address to bool to check pending update.
  @param[in]   ReadLock             Address to bool to check read operation is locked.
  @param[in]   HobFlushComplete     Address to bool to check flush hob is completed.

  @return      EFI_SUCCESS
  @return      EFI_UNSUPPORTED      Runtime variable cache isn't supported
  @return      Others               Errors

**/
EFI_STATUS
EFIAPI
VariableStorageRouterLibInitCache (
  IN  VARIABLE_STORE_HEADER  *HobCache,
  IN  VARIABLE_STORE_HEADER  *VolatileCache,
  IN  VARIABLE_STORE_HEADER  *NvCache,
  IN  BOOLEAN                *PendingUpdate,
  IN  BOOLEAN                *ReadLock,
  IN  BOOLEAN                *HobFlushComplete
  )
{
  return mVariableStorage->InitCache (
                             HobCache,
                             VolatileCache,
                             NvCache,
                             PendingUpdate,
                             ReadLock,
                             HobFlushComplete
                             );
}

/**
  Copies any pending updates to runtime variable caches.

  @retval EFI_SUCCESS             The cache store was updated successfully.
  @retval EFI_UNSUPPORTED         The cache store to be updated is not initialized properly.

**/
EFI_STATUS
EFIAPI
VariableStorageRouterLibSyncCache (
  VOID
  )
{
  return mVariableStorage->SyncCache ();
}

/**
  Get runtime variable caches info.

  @param[out]    HobCacheSize                  Cache size of variables in Hob list.
  @param[out]    VolatileCacheSize             Cache size of variables in volatile memory.
  @param[out]    NvCacheSize                   Cache size of variables in non-volatile memory.

  @retval EFI_SUCCESS
  @retval EFI_UNSUPPORTED

**/
EFI_STATUS
EFIAPI
VariableStorageRouterLibGetCacheInfo (
  OUT UINTN  *HobCacheSize,
  OUT UINTN  *VolatileCacheSize,
  OUT UINTN  *NvCacheSize
  )
{
  return mVariableStorage->GetCacheInfo (
                             HobCacheSize,
                             VolatileCacheSize,
                             NvCacheSize
                             );
}

/**
  Initialise Variable Write Service.
  This function should install gSmmVariableWriteGuid or register notifier to install

  @param[in] WriteReadyNotify   Notifier to be called when varaibles are
                                writable.

  @return EFI_SUCCESS
  @return Others                Failed to initialise write service.

**/
EFI_STATUS
EFIAPI
VariableStorageRouterLibInitWriteService (
  IN WRITE_READY_NOTIFY_FN  WriteReadyNotify
  )
{
  return mVariableStorage->InitWriteService (
                             WriteReadyNotify
                             );
}
