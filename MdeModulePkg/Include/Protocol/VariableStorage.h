/** @file
  Fvb Variable Storage Lib for Smm.

Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Guid/VariableFormat.h>
#include <Protocol/VarCheck.h>

#define EDKII_VARIABLE_STORAGE_PROTOCOL_GUID \
  { \
    0xf9aff246, 0x81f3, 0x11f1, { 0x99, 0x1c, 0xaf, 0xf9, 0xcc, 0x08, 0x41, 0x35 } \
  }

typedef
VOID
(*WRITE_READY_NOTIFY_FN)(
  VOID
  );

/**
  Check whether Variable Storage use Auth Format.

  @return TRUE    Use Auth format.
  @return FALSE   Not use Auth format.

**/
typedef
BOOLEAN
(EFIAPI *EDKII_VARIABLE_STORAGE_PROTOCOL_CHECK_AUTH_FORMAT)(
  VOID
  );

/**
  Get maximum variable size, covering both non-volatile and volatile variables.

  @return Maximum variable size.

**/
typedef
UINTN
(EFIAPI *EDKII_VARIABLE_STORAGE_PROTOCOL_GET_MAX_VARIABLE_SIZE)(
  VOID
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_PROTOCOL_GET_VARIABLE)(
  IN      CHAR16    *VariableName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINT32    *Attributes OPTIONAL,
  IN OUT  UINTN     *DataSize,
  OUT     VOID      *Data OPTIONAL
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_PROTOCOL_GET_NEXT_VARIABLE_NAME)(
  IN OUT  UINTN     *VariableNameSize,
  IN OUT  CHAR16    *VariableName,
  IN OUT  EFI_GUID  *VendorGuid
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_PROTOCOL_SET_VARIABLE)(
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data,
  IN BOOLEAN   FromTrusted
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_PROTOCOL_QUERY_VARIABLE_INFO)(
  IN  UINT32  Attributes,
  OUT UINT64  *MaximumVariableStorageSize,
  OUT UINT64  *RemainingVariableStorageSize,
  OUT UINT64  *MaximumVariableSize
  );

/**
  Callback function at End of DXE phase notification.

  @return EFI_SUCCESS
  @return Others                   ERROR.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_PROTOCOL_END_OF_DXE)(
  IN VOID
  );

/**
  Callback function at READY_TO_BOOT phase notification.

  @return EFI_SUCCESS
  @return Others                   ERROR.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_PROTOCOL_READY_TO_BOOT)(
  IN VOID
  );

/**
  Callback function for EXIT_BOOT phase notification.

  @return EFI_SUCCESS
  @return Others                   ERROR.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_PROTOCOL_EXIT_BOOT_SERVICE)(
  IN VOID
  );

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
  @retval EFI_UNSUPPORTED       No variable information exists in variable driver.
  @retval EFI_BUFFER_TOO_SMALL  The buffer is too small to hold the next variable information.
  @retval EFI_INVALID_PARAMETER Input parameter is invalid.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_PROTOCOL_GET_STATICS)(
  IN OUT VARIABLE_INFO_ENTRY  *InfoEntry,
  IN OUT UINTN                *InfoSize
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_PROTOCOL_REQUEST_TO_LOCK)(
  IN       CHAR16    *VariableName,
  IN       EFI_GUID  *VendorGuid
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_PROTOCOL_PROPERTY_SET)(
  IN CHAR16                       *Name,
  IN EFI_GUID                     *Guid,
  IN VAR_CHECK_VARIABLE_PROPERTY  *VariableProperty
  );

/**
  Variable property get.

  @param[in]  Name              Pointer to the variable name.
  @param[in]  Guid              Pointer to the vendor GUID.
  @param[out] VariableProperty  Pointer to the output variable property.

  @retval EFI_SUCCESS           The property of variable specified by the Name and Guid was got successfully.
  @retval EFI_INVALID_PARAMETER Name, Guid or VariableProperty is NULL, or Name is an empty string.
  @retval EFI_NOT_FOUND         The property of variable specified by the Name and Guid was not found.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_PROTOCOL_PROPERTY_GET)(
  IN CHAR16                        *Name,
  IN EFI_GUID                      *Guid,
  OUT VAR_CHECK_VARIABLE_PROPERTY  *VariableProperty
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_PROTOCOL_INIT_CACHE)(
  IN  VARIABLE_STORE_HEADER  *HobCache,
  IN  VARIABLE_STORE_HEADER  *VolatileCache,
  IN  VARIABLE_STORE_HEADER  *NvCache,
  IN  BOOLEAN                *PendingUpdate,
  IN  BOOLEAN                *ReadLock,
  IN  BOOLEAN                *HobFlushComplete
  );

/**
  Copies any pending updates to runtime variable caches.

  @retval EFI_SUCCESS             The cache store was updated successfully.
  @retval EFI_UNSUPPORTED         The cache store to be updated is not initialized properly.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_PROTOCOL_SYNC_CACHE)(
  VOID
  );

/**
  Get runtime variable caches info.

  @param[out]    HobCacheSize                  Cache size of variables in Hob list.
  @param[out]    VolatileCacheSize             Cache size of variables in volatile memory.
  @param[out]    NvCacheSize                   Cache size of variables in non-volatile memory.

  @retval EFI_SUCCESS
  @retval EFI_UNSUPPORTED

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_PROTOCOL_GET_CACHE_INFO)(
  OUT UINTN  *HobCacheSize,
  OUT UINTN  *VolatileCacheSize,
  OUT UINTN  *NvCacheSize
  );

/**
  Initialise Variable Write Service.
  This function should install gSmmVariableWriteGuid or register notifier to install

  @param[in] WriteReadyNotify   Notifier to be called when variables are
                                writable.

  @return EFI_SUCCESS
  @return Others                Failed to initialise write service.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_VARIABLE_STORAGE_PROTOCOL_INIT_WRITE_SERVICE)(
  IN WRITE_READY_NOTIFY_FN  WriteReadyNotify
  );

typedef struct _EDKII_VARIABLE_STORAGE_PROTOCOL {
  EDKII_VARIABLE_STORAGE_PROTOCOL_CHECK_AUTH_FORMAT         CheckAuthFormat;
  EDKII_VARIABLE_STORAGE_PROTOCOL_GET_MAX_VARIABLE_SIZE     GetMaxVariableSize;
  EDKII_VARIABLE_STORAGE_PROTOCOL_GET_VARIABLE              GetVariable;
  EDKII_VARIABLE_STORAGE_PROTOCOL_GET_NEXT_VARIABLE_NAME    GetNextVariableName;
  EDKII_VARIABLE_STORAGE_PROTOCOL_SET_VARIABLE              SetVariable;
  EDKII_VARIABLE_STORAGE_PROTOCOL_QUERY_VARIABLE_INFO       QueryVariableInfo;
  EDKII_VARIABLE_STORAGE_PROTOCOL_END_OF_DXE                EndOfDxe;
  EDKII_VARIABLE_STORAGE_PROTOCOL_READY_TO_BOOT             ReadyToBoot;
  EDKII_VARIABLE_STORAGE_PROTOCOL_EXIT_BOOT_SERVICE         ExitBootService;
  EDKII_VARIABLE_STORAGE_PROTOCOL_GET_STATICS               GetStatics;
  EDKII_VARIABLE_STORAGE_PROTOCOL_REQUEST_TO_LOCK           RequestToLock;
  EDKII_VARIABLE_STORAGE_PROTOCOL_PROPERTY_SET              PropertySet;
  EDKII_VARIABLE_STORAGE_PROTOCOL_PROPERTY_GET              PropertyGet;
  EDKII_VARIABLE_STORAGE_PROTOCOL_INIT_CACHE                InitCache;
  EDKII_VARIABLE_STORAGE_PROTOCOL_SYNC_CACHE                SyncCache;
  EDKII_VARIABLE_STORAGE_PROTOCOL_GET_CACHE_INFO            GetCacheInfo;
  EDKII_VARIABLE_STORAGE_PROTOCOL_INIT_WRITE_SERVICE        InitWriteService;
} EDKII_VARIABLE_STORAGE_PROTOCOL;

extern EFI_GUID  gEdkiiVariableStorageProtocolGuid;
