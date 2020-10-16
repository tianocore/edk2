/** @file
  ACPI Sdt Protocol Driver

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// Includes
//
#include "AcpiTable.h"

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ACPI_SDT_PROTOCOL  mAcpiSdtProtocolTemplate = {
  EFI_ACPI_TABLE_VERSION_NONE,
  GetAcpiTable2,
  RegisterNotify,
  Open,
  OpenSdt,
  Close,
  GetChild,
  GetOption,
  SetOption,
  FindPath
};

/**
  This function returns ACPI Table instance.

  @return AcpiTableInstance
**/
EFI_ACPI_TABLE_INSTANCE *
SdtGetAcpiTableInstance (
  VOID
  )
{
  return mPrivateData;
}

/**
  This function finds the table specified by the buffer.

  @param[in]  Buffer      Table buffer to find.

  @return ACPI table list.
**/
EFI_ACPI_TABLE_LIST *
FindTableByBuffer (
  IN VOID  *Buffer
  )
{
  EFI_ACPI_TABLE_INSTANCE   *AcpiTableInstance;
  LIST_ENTRY                *CurrentLink;
  EFI_ACPI_TABLE_LIST       *CurrentTableList;
  LIST_ENTRY                *StartLink;

  //
  // Get the instance of the ACPI Table
  //
  AcpiTableInstance = SdtGetAcpiTableInstance ();

  //
  // Find the notify
  //
  StartLink   = &AcpiTableInstance->TableList;
  CurrentLink = StartLink->ForwardLink;

  while (CurrentLink != StartLink) {
    CurrentTableList = EFI_ACPI_TABLE_LIST_FROM_LINK (CurrentLink);
    if (((UINTN)CurrentTableList->Table <= (UINTN)Buffer) &&
        ((UINTN)CurrentTableList->Table + CurrentTableList->TableSize > (UINTN)Buffer)) {
      //
      // Good! Found Table.
      //
      return CurrentTableList;
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  return NULL;
}

/**
  This function updates AML table checksum.
  It will search the ACPI table installed by ACPI_TABLE protocol.

  @param[in]  Buffer        A piece of AML code buffer pointer.

  @retval EFI_SUCCESS       The table holds the AML buffer is found, and checksum is updated.
  @retval EFI_NOT_FOUND     The table holds the AML buffer is not found.
**/
EFI_STATUS
SdtUpdateAmlChecksum (
  IN VOID  *Buffer
  )
{
  EFI_ACPI_TABLE_LIST       *CurrentTableList;

  CurrentTableList = FindTableByBuffer (Buffer);
  if (CurrentTableList == NULL) {
    return EFI_NOT_FOUND;
  }

  AcpiPlatformChecksum (
    (VOID *)CurrentTableList->Table,
    CurrentTableList->Table->Length,
    OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, Checksum)
    );
  return EFI_SUCCESS;
}

/**
  This function finds MAX AML buffer size.
  It will search the ACPI table installed by ACPI_TABLE protocol.

  @param[in]  Buffer        A piece of AML code buffer pointer.
  @param[out] MaxSize       On return it holds the MAX size of buffer.

  @retval EFI_SUCCESS       The table holds the AML buffer is found, and MAX size if returned.
  @retval EFI_NOT_FOUND     The table holds the AML buffer is not found.
**/
EFI_STATUS
SdtGetMaxAmlBufferSize (
  IN  VOID  *Buffer,
  OUT UINTN *MaxSize
  )
{
  EFI_ACPI_TABLE_LIST       *CurrentTableList;

  CurrentTableList = FindTableByBuffer (Buffer);
  if (CurrentTableList == NULL) {
    return EFI_NOT_FOUND;
  }

  *MaxSize = (UINTN)CurrentTableList->Table + CurrentTableList->Table->Length - (UINTN)Buffer;
  return EFI_SUCCESS;
}

/**
  This function invokes ACPI notification.

  @param[in]  AcpiTableInstance          Instance to AcpiTable
  @param[in]  Version                    Version(s) to set.
  @param[in]  Handle                     Handle of the table.
**/
VOID
SdtNotifyAcpiList (
  IN EFI_ACPI_TABLE_INSTANCE   *AcpiTableInstance,
  IN EFI_ACPI_TABLE_VERSION    Version,
  IN UINTN                     Handle
  )
{
  EFI_ACPI_NOTIFY_LIST      *CurrentNotifyList;
  LIST_ENTRY                *CurrentLink;
  LIST_ENTRY                *StartLink;
  EFI_ACPI_TABLE_LIST       *Table;
  EFI_STATUS                Status;

  //
  // We should not use Table buffer, because it is user input buffer.
  //
  Status = FindTableByHandle (
             Handle,
             &AcpiTableInstance->TableList,
             &Table
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Find the notify
  //
  StartLink   = &AcpiTableInstance->NotifyList;
  CurrentLink = StartLink->ForwardLink;

  while (CurrentLink != StartLink) {
    CurrentNotifyList = EFI_ACPI_NOTIFY_LIST_FROM_LINK (CurrentLink);

    //
    // Inovke notification
    //
    CurrentNotifyList->Notification ((EFI_ACPI_SDT_HEADER *)Table->Table, Version, Handle);

    CurrentLink = CurrentLink->ForwardLink;
  }

  return ;
}

/**
  Returns a requested ACPI table.

  The GetAcpiTable() function returns a pointer to a buffer containing the ACPI table associated
  with the Index that was input. The following structures are not considered elements in the list of
  ACPI tables:
  - Root System Description Pointer (RSD_PTR)
  - Root System Description Table (RSDT)
  - Extended System Description Table (XSDT)
  Version is updated with a bit map containing all the versions of ACPI of which the table is a
  member. For tables installed via the EFI_ACPI_TABLE_PROTOCOL.InstallAcpiTable() interface,
  the function returns the value of EFI_ACPI_STD_PROTOCOL.AcpiVersion.

  @param[in]    Index       The zero-based index of the table to retrieve.
  @param[out]   Table       Pointer for returning the table buffer.
  @param[out]   Version     On return, updated with the ACPI versions to which this table belongs. Type
                            EFI_ACPI_TABLE_VERSION is defined in "Related Definitions" in the
                            EFI_ACPI_SDT_PROTOCOL.
  @param[out]   TableKey    On return, points to the table key for the specified ACPI system definition table.
                            This is identical to the table key used in the EFI_ACPI_TABLE_PROTOCOL.
                            The TableKey can be passed to EFI_ACPI_TABLE_PROTOCOL.UninstallAcpiTable()
                            to uninstall the table.
  @retval EFI_SUCCESS       The function completed successfully.
  @retval EFI_NOT_FOUND     The requested index is too large and a table was not found.
**/
EFI_STATUS
EFIAPI
GetAcpiTable2 (
  IN  UINTN                               Index,
  OUT EFI_ACPI_SDT_HEADER                 **Table,
  OUT EFI_ACPI_TABLE_VERSION              *Version,
  OUT UINTN                               *TableKey
  )
{
  EFI_ACPI_TABLE_INSTANCE   *AcpiTableInstance;
  UINTN                     TableIndex;
  LIST_ENTRY                *CurrentLink;
  LIST_ENTRY                *StartLink;
  EFI_ACPI_TABLE_LIST       *CurrentTable;

  ASSERT (Table != NULL);
  ASSERT (Version != NULL);
  ASSERT (TableKey != NULL);

  //
  // Get the instance of the ACPI Table
  //
  AcpiTableInstance = SdtGetAcpiTableInstance ();

  //
  // Find the table
  //
  StartLink   = &AcpiTableInstance->TableList;
  CurrentLink = StartLink->ForwardLink;
  TableIndex = 0;

  while (CurrentLink != StartLink) {
    if (TableIndex == Index) {
      break;
    }
    //
    // Next one
    //
    CurrentLink = CurrentLink->ForwardLink;
    TableIndex ++;
  }

  if ((TableIndex != Index) || (CurrentLink == StartLink)) {
    return EFI_NOT_FOUND;
  }

  //
  // Get handle and version
  //
  CurrentTable  = EFI_ACPI_TABLE_LIST_FROM_LINK (CurrentLink);
  *TableKey     = CurrentTable->Handle;
  *Version      = CurrentTable->Version;
  *Table        = (EFI_ACPI_SDT_HEADER *)CurrentTable->Table;

  return EFI_SUCCESS;
}

/**
  Register a callback when an ACPI table is installed.

  This function registers a function which will be called whenever a new ACPI table is installed.

  @param[in]  Notification               Points to the callback function to be registered
**/
VOID
SdtRegisterNotify (
  IN EFI_ACPI_NOTIFICATION_FN   Notification
  )
{
  EFI_ACPI_TABLE_INSTANCE   *AcpiTableInstance;
  EFI_ACPI_NOTIFY_LIST      *CurrentNotifyList;

  //
  // Get the instance of the ACPI Table
  //
  AcpiTableInstance = SdtGetAcpiTableInstance ();

  //
  // Create a new list entry
  //
  CurrentNotifyList = AllocatePool (sizeof (EFI_ACPI_NOTIFY_LIST));
  ASSERT (CurrentNotifyList != NULL);

  //
  // Initialize the table contents
  //
  CurrentNotifyList->Signature    = EFI_ACPI_NOTIFY_LIST_SIGNATURE;
  CurrentNotifyList->Notification = Notification;

  //
  // Add the table to the current list of tables
  //
  InsertTailList (&AcpiTableInstance->NotifyList, &CurrentNotifyList->Link);

  return ;
}

/**
  Unregister a callback when an ACPI table is installed.

  This function unregisters a function which will be called whenever a new ACPI table is installed.

  @param[in]  Notification               Points to the callback function to be unregistered.

  @retval EFI_SUCCESS           Callback successfully unregistered.
  @retval EFI_INVALID_PARAMETER Notification does not match a known registration function.
**/
EFI_STATUS
SdtUnregisterNotify (
  IN EFI_ACPI_NOTIFICATION_FN   Notification
  )
{
  EFI_ACPI_TABLE_INSTANCE   *AcpiTableInstance;
  EFI_ACPI_NOTIFY_LIST      *CurrentNotifyList;
  LIST_ENTRY                *CurrentLink;
  LIST_ENTRY                *StartLink;

  //
  // Get the instance of the ACPI Table
  //
  AcpiTableInstance = SdtGetAcpiTableInstance ();

  //
  // Find the notify
  //
  StartLink   = &AcpiTableInstance->NotifyList;
  CurrentLink = StartLink->ForwardLink;

  while (CurrentLink != StartLink) {
    CurrentNotifyList = EFI_ACPI_NOTIFY_LIST_FROM_LINK (CurrentLink);
    if (CurrentNotifyList->Notification == Notification) {
      //
      // Good! Found notification.
      //
      // Remove it from list and free the node.
      //
      RemoveEntryList (&(CurrentNotifyList->Link));
      FreePool (CurrentNotifyList);
      return EFI_SUCCESS;
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  //
  // Not found!
  //
  return EFI_INVALID_PARAMETER;
}

/**
  Register or unregister a callback when an ACPI table is installed.

  This function registers or unregisters a function which will be called whenever a new ACPI table is
  installed.

  @param[in]    Register        If TRUE, then the specified function will be registered. If FALSE, then the specified
                                function will be unregistered.
  @param[in]    Notification    Points to the callback function to be registered or unregistered.

  @retval EFI_SUCCESS           Callback successfully registered or unregistered.
  @retval EFI_INVALID_PARAMETER Notification is NULL
  @retval EFI_INVALID_PARAMETER Register is FALSE and Notification does not match a known registration function.
**/
EFI_STATUS
EFIAPI
RegisterNotify (
  IN BOOLEAN                    Register,
  IN EFI_ACPI_NOTIFICATION_FN   Notification
  )
{
  //
  // Check for invalid input parameters
  //
  if (Notification == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Register) {
    //
    // Register a new notify
    //
    SdtRegisterNotify (Notification);
    return EFI_SUCCESS;
  } else {
    //
    // Unregister an old notify
    //
    return SdtUnregisterNotify (Notification);
  }
}

/**
  Create a handle for the first ACPI opcode in an ACPI system description table.

  @param[in]    TableKey    The table key for the ACPI table, as returned by GetTable().
  @param[out]   Handle      On return, points to the newly created ACPI handle.

  @retval EFI_SUCCESS       Handle created successfully.
  @retval EFI_NOT_FOUND     TableKey does not refer to a valid ACPI table.
**/
EFI_STATUS
SdtOpenSdtTable (
  IN    UINTN           TableKey,
  OUT   EFI_ACPI_HANDLE *Handle
  )
{
  EFI_ACPI_TABLE_INSTANCE   *AcpiTableInstance;
  EFI_STATUS                Status;
  EFI_ACPI_TABLE_LIST       *Table;
  EFI_AML_HANDLE            *AmlHandle;

  //
  // Get the instance of the ACPI Table
  //
  AcpiTableInstance = SdtGetAcpiTableInstance ();

  //
  // Find the table
  //
  Status = FindTableByHandle (
             TableKey,
             &AcpiTableInstance->TableList,
             &Table
             );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  AmlHandle = AllocatePool (sizeof(*AmlHandle));
  ASSERT (AmlHandle != NULL);
  AmlHandle->Signature       = EFI_AML_ROOT_HANDLE_SIGNATURE;
  AmlHandle->Buffer          = (VOID *)((UINTN)Table->Table + sizeof(EFI_ACPI_SDT_HEADER));
  AmlHandle->Size            = Table->Table->Length - sizeof(EFI_ACPI_SDT_HEADER);
  AmlHandle->AmlByteEncoding = NULL;
  AmlHandle->Modified        = FALSE;

  //
  // return the ACPI handle
  //
  *Handle = (EFI_ACPI_HANDLE)AmlHandle;

  return EFI_SUCCESS;
}

/**
  Create a handle for the first ACPI opcode in an ACPI system description table.

  @param[in]    TableKey    The table key for the ACPI table, as returned by GetTable().
  @param[out]   Handle      On return, points to the newly created ACPI handle.

  @retval EFI_SUCCESS       Handle created successfully.
  @retval EFI_NOT_FOUND     TableKey does not refer to a valid ACPI table.
**/
EFI_STATUS
EFIAPI
OpenSdt (
  IN    UINTN           TableKey,
  OUT   EFI_ACPI_HANDLE *Handle
  )
{
  if (Handle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return SdtOpenSdtTable (TableKey, Handle);
}

/**
  Create a handle from an ACPI opcode

  @param[in]  Buffer                 Points to the ACPI opcode.
  @param[in]  BufferSize             Max buffer size.
  @param[out] Handle                 Upon return, holds the handle.

  @retval   EFI_SUCCESS             Success
  @retval   EFI_INVALID_PARAMETER   Buffer is NULL or Handle is NULL or Buffer points to an
                                    invalid opcode.

**/
EFI_STATUS
SdtOpenEx (
  IN    VOID            *Buffer,
  IN    UINTN           BufferSize,
  OUT   EFI_ACPI_HANDLE *Handle
  )
{
  AML_BYTE_ENCODING   *AmlByteEncoding;
  EFI_AML_HANDLE      *AmlHandle;

  AmlByteEncoding = AmlSearchByOpByte (Buffer);
  if (AmlByteEncoding == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Do not open NameString as handle
  //
  if ((AmlByteEncoding->Attribute & AML_IS_NAME_CHAR) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Good, find it
  //
  AmlHandle = AllocatePool (sizeof(*AmlHandle));
  ASSERT (AmlHandle != NULL);

  AmlHandle->Signature       = EFI_AML_HANDLE_SIGNATURE;
  AmlHandle->Buffer          = Buffer;
  AmlHandle->AmlByteEncoding = AmlByteEncoding;
  AmlHandle->Modified        = FALSE;

  AmlHandle->Size = AmlGetObjectSize (AmlByteEncoding, Buffer, BufferSize);
  if (AmlHandle->Size == 0) {
    FreePool (AmlHandle);
    return EFI_INVALID_PARAMETER;
  }

  *Handle = (EFI_ACPI_HANDLE)AmlHandle;

  return EFI_SUCCESS;
}

/**
  Create a handle from an ACPI opcode

  @param[in]  Buffer                 Points to the ACPI opcode.
  @param[out] Handle                 Upon return, holds the handle.

  @retval   EFI_SUCCESS             Success
  @retval   EFI_INVALID_PARAMETER   Buffer is NULL or Handle is NULL or Buffer points to an
                                    invalid opcode.

**/
EFI_STATUS
EFIAPI
Open (
  IN    VOID            *Buffer,
  OUT   EFI_ACPI_HANDLE *Handle
  )
{
  EFI_STATUS          Status;
  UINTN               MaxSize;

  MaxSize = 0;

  //
  // Check for invalid input parameters
  //
  if (Buffer == NULL || Handle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = SdtGetMaxAmlBufferSize (Buffer, &MaxSize);
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  return SdtOpenEx (Buffer, MaxSize, Handle);
}

/**
  Close an ACPI handle.

  @param[in] Handle Returns the handle.

  @retval EFI_SUCCESS           Success
  @retval EFI_INVALID_PARAMETER Handle is NULL or does not refer to a valid ACPI object.
**/
EFI_STATUS
EFIAPI
Close (
  IN EFI_ACPI_HANDLE Handle
  )
{
  EFI_AML_HANDLE      *AmlHandle;
  EFI_STATUS          Status;

  //
  // Check for invalid input parameters
  //
  if (Handle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AmlHandle = (EFI_AML_HANDLE *)Handle;
  if ((AmlHandle->Signature != EFI_AML_ROOT_HANDLE_SIGNATURE) &&
      (AmlHandle->Signature != EFI_AML_HANDLE_SIGNATURE)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Update Checksum only if modified
  //
  if (AmlHandle->Modified) {
    Status = SdtUpdateAmlChecksum (AmlHandle->Buffer);
    if (EFI_ERROR (Status)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  FreePool (AmlHandle);

  return EFI_SUCCESS;
}

/**
  Retrieve information about an ACPI object.

  @param[in]    Handle      ACPI object handle.
  @param[in]    Index       Index of the data to retrieve from the object. In general, indexes read from left-to-right
                            in the ACPI encoding, with index 0 always being the ACPI opcode.
  @param[out]   DataType    Points to the returned data type or EFI_ACPI_DATA_TYPE_NONE if no data exists
                            for the specified index.
  @param[out]   Data        Upon return, points to the pointer to the data.
  @param[out]   DataSize    Upon return, points to the size of Data.

  @retval       EFI_SUCCESS           Success.
  @retval       EFI_INVALID_PARAMETER Handle is NULL or does not refer to a valid ACPI object.
**/
EFI_STATUS
EFIAPI
GetOption (
  IN        EFI_ACPI_HANDLE     Handle,
  IN        UINTN               Index,
  OUT       EFI_ACPI_DATA_TYPE  *DataType,
  OUT CONST VOID                **Data,
  OUT       UINTN               *DataSize
  )
{
  EFI_AML_HANDLE      *AmlHandle;
  AML_BYTE_ENCODING   *AmlByteEncoding;
  EFI_STATUS          Status;

  ASSERT (DataType != NULL);
  ASSERT (Data != NULL);
  ASSERT (DataSize != NULL);

  //
  // Check for invalid input parameters
  //
  if (Handle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AmlHandle = (EFI_AML_HANDLE *)Handle;
  //
  // Do not check EFI_AML_ROOT_HANDLE_SIGNATURE because there is no option for Root handle
  //
  if (AmlHandle->Signature != EFI_AML_HANDLE_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  AmlByteEncoding = AmlHandle->AmlByteEncoding;
  if (Index > AmlByteEncoding->MaxIndex) {
    *DataType = EFI_ACPI_DATA_TYPE_NONE;
    return EFI_SUCCESS;
  }

  //
  // Parse option
  //
  Status = AmlParseOptionHandleCommon (AmlHandle, (AML_OP_PARSE_INDEX)Index, DataType, (VOID **)Data, DataSize);
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Change information about an ACPI object.

  @param[in]  Handle    ACPI object handle.
  @param[in]  Index     Index of the data to retrieve from the object. In general, indexes read from left-to-right
                        in the ACPI encoding, with index 0 always being the ACPI opcode.
  @param[in]  Data      Points to the data.
  @param[in]  DataSize  The size of the Data.

  @retval EFI_SUCCESS           Success
  @retval EFI_INVALID_PARAMETER Handle is NULL or does not refer to a valid ACPI object.
  @retval EFI_BAD_BUFFER_SIZE   Data cannot be accommodated in the space occupied by
                                the option.

**/
EFI_STATUS
EFIAPI
SetOption (
  IN        EFI_ACPI_HANDLE Handle,
  IN        UINTN           Index,
  IN CONST  VOID            *Data,
  IN        UINTN           DataSize
  )
{
  EFI_AML_HANDLE      *AmlHandle;
  AML_BYTE_ENCODING   *AmlByteEncoding;
  EFI_STATUS          Status;
  EFI_ACPI_DATA_TYPE  DataType;
  VOID                *OrgData;
  UINTN               OrgDataSize;

  ASSERT (Data != NULL);

  //
  // Check for invalid input parameters
  //
  if (Handle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AmlHandle = (EFI_AML_HANDLE *)Handle;
  //
  // Do not check EFI_AML_ROOT_HANDLE_SIGNATURE because there is no option for Root handle
  //
  if (AmlHandle->Signature != EFI_AML_HANDLE_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }
  AmlByteEncoding = AmlHandle->AmlByteEncoding;

  if (Index > AmlByteEncoding->MaxIndex) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Parse option
  //
  Status = AmlParseOptionHandleCommon (AmlHandle, (AML_OP_PARSE_INDEX)Index, &DataType, &OrgData, &OrgDataSize);
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }
  if (DataType == EFI_ACPI_DATA_TYPE_NONE) {
    return EFI_INVALID_PARAMETER;
  }

  if (DataSize > OrgDataSize) {
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // Update
  //
  CopyMem (OrgData, Data, DataSize);
  AmlHandle->Modified = TRUE;

  return EFI_SUCCESS;
}

/**
  Return the child ACPI objects.

  @param[in]        ParentHandle    Parent handle.
  @param[in, out]   Handle          On entry, points to the previously returned handle or NULL to start with the first
                                    handle. On return, points to the next returned ACPI handle or NULL if there are no
                                    child objects.

  @retval EFI_SUCCESS               Success
  @retval EFI_INVALID_PARAMETER     ParentHandle is NULL or does not refer to a valid ACPI object.
**/
EFI_STATUS
EFIAPI
GetChild (
  IN EFI_ACPI_HANDLE        ParentHandle,
  IN OUT EFI_ACPI_HANDLE    *Handle
  )
{
  EFI_AML_HANDLE      *AmlParentHandle;
  EFI_AML_HANDLE      *AmlHandle;
  VOID                *Buffer;
  EFI_STATUS          Status;

  ASSERT (Handle != NULL);

  //
  // Check for invalid input parameters
  //
  if (ParentHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AmlHandle       = *Handle;
  if ((AmlHandle != NULL) && (AmlHandle->Signature != EFI_AML_HANDLE_SIGNATURE)) {
    return EFI_INVALID_PARAMETER;
  }

  AmlParentHandle = (EFI_AML_HANDLE *)ParentHandle;
  if (AmlParentHandle->Signature == EFI_AML_ROOT_HANDLE_SIGNATURE) {
    //
    // Root handle
    //
    Status = AmlGetChildFromRoot (AmlParentHandle, AmlHandle, &Buffer);
  } else if (AmlParentHandle->Signature == EFI_AML_HANDLE_SIGNATURE) {
    //
    // Non-root handle
    //
    Status = AmlGetChildFromNonRoot (AmlParentHandle, AmlHandle, &Buffer);
  } else {
    //
    // Invalid
    //
    return EFI_INVALID_PARAMETER;
  }

  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }
  if (Buffer == NULL) {
    *Handle = NULL;
    return EFI_SUCCESS;
  }
  return SdtOpenEx (Buffer, (UINTN)AmlParentHandle->Buffer + AmlParentHandle->Size - (UINTN)Buffer, Handle);
}

/**
  Returns the handle of the ACPI object representing the specified ACPI path

  @param[in]    HandleIn    Points to the handle of the object representing the starting point for the path search.
  @param[in]    AmlPath     Points to the AML path.
  @param[out]   HandleOut   On return, points to the ACPI object which represents AcpiPath, relative to
                            HandleIn.

  @retval EFI_SUCCESS           Success
  @retval EFI_INVALID_PARAMETER HandleIn is NULL or does not refer to a valid ACPI object.
**/
EFI_STATUS
SdtFindPathFromNonRoot (
  IN    EFI_ACPI_HANDLE HandleIn,
  IN    UINT8           *AmlPath,
  OUT   EFI_ACPI_HANDLE *HandleOut
  )
{
  EFI_AML_HANDLE      *AmlHandle;
  VOID                *Buffer;
  EFI_STATUS          Status;

  Buffer = NULL;
  AmlHandle = (EFI_AML_HANDLE *)HandleIn;

  //
  // For non-root handle, we need search from THIS node instead of ROOT.
  //
  Status = AmlFindPath (AmlHandle, AmlPath, &Buffer, FALSE);
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }
  if (Buffer == NULL) {
    *HandleOut = NULL;
    return EFI_SUCCESS;
  }
  return SdtOpenEx (Buffer, (UINTN)AmlHandle->Buffer + AmlHandle->Size - (UINTN)Buffer, HandleOut);
}

/**
  Duplicate AML handle.

  @param[in]    AmlHandle   Handle to be duplicated.

  @return Duplicated AML handle.
**/
EFI_AML_HANDLE *
SdtDuplicateHandle (
  IN EFI_AML_HANDLE      *AmlHandle
  )
{
  EFI_AML_HANDLE  *DstAmlHandle;

  DstAmlHandle = AllocatePool (sizeof(*DstAmlHandle));
  ASSERT (DstAmlHandle != NULL);
  CopyMem (DstAmlHandle, (VOID *)AmlHandle, sizeof(*DstAmlHandle));

  return DstAmlHandle;
}

/**
  Returns the handle of the ACPI object representing the specified ACPI path

  @param[in]    HandleIn    Points to the handle of the object representing the starting point for the path search.
  @param[in]    AmlPath     Points to the AML path.
  @param[out]   HandleOut   On return, points to the ACPI object which represents AcpiPath, relative to
                            HandleIn.

  @retval EFI_SUCCESS           Success
  @retval EFI_INVALID_PARAMETER HandleIn is NULL or does not refer to a valid ACPI object.
**/
EFI_STATUS
SdtFindPathFromRoot (
  IN    EFI_ACPI_HANDLE HandleIn,
  IN    UINT8           *AmlPath,
  OUT   EFI_ACPI_HANDLE *HandleOut
  )
{
  EFI_ACPI_HANDLE     ChildHandle;
  EFI_AML_HANDLE      *AmlHandle;
  EFI_STATUS          Status;
  VOID                *Buffer;

  Buffer = NULL;
  AmlHandle = (EFI_AML_HANDLE *)HandleIn;

  //
  // Handle case that AcpiPath is Root
  //
  if (AmlIsRootPath (AmlPath)) {
    //
    // Duplicate RootHandle
    //
    *HandleOut = (EFI_ACPI_HANDLE)SdtDuplicateHandle (AmlHandle);
    return EFI_SUCCESS;
  }

  //
  // Let children find it.
  //
  ChildHandle = NULL;
  while (TRUE) {
    Status = GetChild (HandleIn, &ChildHandle);
    if (EFI_ERROR (Status)) {
      return EFI_INVALID_PARAMETER;
    }

    if (ChildHandle == NULL) {
      //
      // Not found
      //
      *HandleOut = NULL;
      return EFI_SUCCESS;
    }

    //
    // More child
    //
    AmlHandle = (EFI_AML_HANDLE *)ChildHandle;
    Status = AmlFindPath (AmlHandle, AmlPath, &Buffer, TRUE);
    if (EFI_ERROR (Status)) {
      return EFI_INVALID_PARAMETER;
    }

    if (Buffer != NULL) {
      //
      // Great! Find it, open
      //
      Status = SdtOpenEx (Buffer, (UINTN)AmlHandle->Buffer + AmlHandle->Size - (UINTN)Buffer, HandleOut);
      if (!EFI_ERROR (Status))  {
        return EFI_SUCCESS;
      }
      //
      // Not success, try next one
      //
    }
  }

  //
  // Should not run here
  //
}

/**
  Returns the handle of the ACPI object representing the specified ACPI path

  @param[in]    HandleIn    Points to the handle of the object representing the starting point for the path search.
  @param[in]    AcpiPath    Points to the ACPI path, which conforms to the ACPI encoded path format.
  @param[out]   HandleOut   On return, points to the ACPI object which represents AcpiPath, relative to
                            HandleIn.

  @retval EFI_SUCCESS           Success
  @retval EFI_INVALID_PARAMETER HandleIn is NULL or does not refer to a valid ACPI object.
**/
EFI_STATUS
EFIAPI
FindPath (
  IN    EFI_ACPI_HANDLE HandleIn,
  IN    VOID            *AcpiPath,
  OUT   EFI_ACPI_HANDLE *HandleOut
  )
{
  EFI_AML_HANDLE      *AmlHandle;
  EFI_STATUS          Status;
  UINT8               *AmlPath;

  //
  // Check for invalid input parameters
  //
  if (HandleIn == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AmlHandle = (EFI_AML_HANDLE *)HandleIn;

  //
  // Convert ASL path to AML path
  //
  AmlPath = AmlNameFromAslName (AcpiPath);
  if (AmlPath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG_CODE_BEGIN ();
  DEBUG ((EFI_D_ERROR, "AcpiSdt: FindPath - "));
  AmlPrintNameString (AmlPath);
  DEBUG ((EFI_D_ERROR, "\n"));
  DEBUG_CODE_END ();

  if (AmlHandle->Signature == EFI_AML_ROOT_HANDLE_SIGNATURE) {
    //
    // Root Handle
    //
    Status = SdtFindPathFromRoot (HandleIn, AmlPath, HandleOut);
  } else if (AmlHandle->Signature == EFI_AML_HANDLE_SIGNATURE) {
    //
    // Non-Root handle
    //
    Status = SdtFindPathFromNonRoot (HandleIn, AmlPath, HandleOut);
  } else {
    Status = EFI_INVALID_PARAMETER;
  }

  FreePool (AmlPath);

  return Status;
}

/**
  This function initializes AcpiSdt protocol in ACPI table instance.

  @param[in]  AcpiTableInstance       Instance to construct
**/
VOID
SdtAcpiTableAcpiSdtConstructor (
  IN EFI_ACPI_TABLE_INSTANCE   *AcpiTableInstance
  )
{

  InitializeListHead (&AcpiTableInstance->NotifyList);
  CopyMem (&AcpiTableInstance->AcpiSdtProtocol, &mAcpiSdtProtocolTemplate, sizeof(mAcpiSdtProtocolTemplate));
  AcpiTableInstance->AcpiSdtProtocol.AcpiVersion = (EFI_ACPI_TABLE_VERSION)PcdGet32 (PcdAcpiExposedTableVersions);

  return ;
}
