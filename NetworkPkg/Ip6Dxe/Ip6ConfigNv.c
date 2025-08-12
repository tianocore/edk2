/** @file
  Helper functions for configuring or obtaining the parameters relating to IP6.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ip6Impl.h"

CHAR16  mIp6ConfigStorageName[] = L"IP6_CONFIG_IFR_NVDATA";

/**
  The notify function of create event when performing a manual configuration.

  @param[in]    Event        The pointer of Event.
  @param[in]    Context      The pointer of Context.

**/
VOID
EFIAPI
Ip6ConfigManualAddressNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  *((BOOLEAN *)Context) = TRUE;
}

/**
  Get the configuration data for the EFI IPv6 network stack running on the
  communication. It is a help function to the call EfiIp6ConfigGetData().

  @param[in]      Ip6Config      The pointer to the EFI_IP6_CONFIG_PROTOCOL instance.
  @param[in]      DataType       The type of data to get.
  @param[out]     DataSize       The size of buffer required in bytes.
  @param[out]     Data           The data buffer in which the configuration data is returned. The
                                 type of the data buffer associated with the DataType.
                                 It is the caller's responsibility to free the resource.

  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.
  @retval EFI_INVALID_PARAMETER One or more of the followings are TRUE:
                                - Ip6Config is NULL or invalid.
                                - DataSize is NULL.
                                - Data is NULL.
  @retval EFI_OUT_OF_RESOURCES  Fail to perform the operation due to lack of resources.
  @retval EFI_NOT_READY         The specified configuration data is not ready due to an
                                asynchronous configuration process already in progress.
  @retval EFI_NOT_FOUND         The specified configuration data was not found.

**/
EFI_STATUS
Ip6ConfigNvGetData (
  IN  EFI_IP6_CONFIG_PROTOCOL   *Ip6Config,
  IN  EFI_IP6_CONFIG_DATA_TYPE  DataType,
  OUT UINTN                     *DataSize,
  OUT VOID                      **Data
  )
{
  UINTN       BufferSize;
  VOID        *Buffer;
  EFI_STATUS  Status;

  if ((Ip6Config == NULL) || (Data == NULL) || (DataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  BufferSize = 0;
  Status     = Ip6Config->GetData (
                            Ip6Config,
                            DataType,
                            &BufferSize,
                            NULL
                            );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  Buffer = AllocateZeroPool (BufferSize);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = Ip6Config->GetData (
                        Ip6Config,
                        DataType,
                        &BufferSize,
                        Buffer
                        );
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return Status;
  }

  *DataSize = BufferSize;
  *Data     = Buffer;

  return EFI_SUCCESS;
}

/**
  Free all nodes in IP6_ADDRESS_INFO_ENTRY in the list array specified
  with ListHead.

  @param[in]      ListHead  The head of the list array in IP6_ADDRESS_INFO_ENTRY.

**/
VOID
Ip6FreeAddressInfoList (
  IN LIST_ENTRY  *ListHead
  )
{
  IP6_ADDRESS_INFO_ENTRY  *Node;
  LIST_ENTRY              *Entry;
  LIST_ENTRY              *NextEntry;

  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, ListHead) {
    Node = NET_LIST_USER_STRUCT (Entry, IP6_ADDRESS_INFO_ENTRY, Link);
    RemoveEntryList (&Node->Link);
    FreePool (Node);
  }
}

/**
  Convert the IPv6 address into a formatted string.

  @param[in]  Ip6       The IPv6 address.
  @param[out] Str       The formatted IP string.

**/
VOID
Ip6ToStr (
  IN  EFI_IPv6_ADDRESS  *Ip6,
  OUT CHAR16            *Str
  )
{
  UINTN    Index;
  BOOLEAN  Short;
  UINTN    Number;
  CHAR16   FormatString[8];

  Short = FALSE;

  for (Index = 0; Index < 15; Index = Index + 2) {
    if (!Short &&
        (Index % 2 == 0) &&
        (Ip6->Addr[Index] == 0) &&
        (Ip6->Addr[Index + 1] == 0)
        )
    {
      //
      // Deal with the case of ::.
      //
      if (Index == 0) {
        *Str       = L':';
        *(Str + 1) = L':';
        Str        = Str + 2;
      } else {
        *Str = L':';
        Str  = Str + 1;
      }

      while ((Index < 15) && (Ip6->Addr[Index] == 0) && (Ip6->Addr[Index + 1] == 0)) {
        Index = Index + 2;
      }

      Short = TRUE;

      if (Index == 16) {
        //
        // :: is at the end of the address.
        //
        *Str = L'\0';
        break;
      }
    }

    ASSERT (Index < 15);

    if (Ip6->Addr[Index] == 0) {
      Number = UnicodeSPrint (Str, 2 * IP6_STR_MAX_SIZE, L"%x:", (UINTN)Ip6->Addr[Index + 1]);
    } else {
      if (Ip6->Addr[Index + 1] < 0x10) {
        CopyMem (FormatString, L"%x0%x:", StrSize (L"%x0%x:"));
      } else {
        CopyMem (FormatString, L"%x%x:", StrSize (L"%x%x:"));
      }

      Number = UnicodeSPrint (
                 Str,
                 2 * IP6_STR_MAX_SIZE,
                 (CONST CHAR16 *)FormatString,
                 (UINTN)Ip6->Addr[Index],
                 (UINTN)Ip6->Addr[Index + 1]
                 );
    }

    Str = Str + Number;

    if (Index + 2 == 16) {
      *Str = L'\0';
      if (*(Str - 1) == L':') {
        *(Str - 1) = L'\0';
      }
    }
  }
}

/**
  Convert EFI_IP6_CONFIG_INTERFACE_ID to string format.

  @param[out]      String  The buffer to store the converted string.
  @param[in]       IfId    The pointer of EFI_IP6_CONFIG_INTERFACE_ID.

  @retval EFI_SUCCESS              The string converted successfully.
  @retval EFI_INVALID_PARAMETER    Any input parameter is invalid.

**/
EFI_STATUS
Ip6ConvertInterfaceIdToString (
  OUT CHAR16                       *String,
  IN  EFI_IP6_CONFIG_INTERFACE_ID  *IfId
  )
{
  UINT8  Index;
  UINTN  Number;

  if ((String == NULL) || (IfId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < 8; Index++) {
    Number = UnicodeSPrint (
               String,
               2 * INTERFACE_ID_STR_STORAGE,
               L"%x:",
               (UINTN)IfId->Id[Index]
               );
    String = String + Number;
  }

  *(String - 1) = '\0';

  return EFI_SUCCESS;
}

/**
  Parse InterfaceId in string format and convert it to EFI_IP6_CONFIG_INTERFACE_ID.

  @param[in]        String  The buffer of the string to be parsed.
  @param[out]       IfId    The pointer of EFI_IP6_CONFIG_INTERFACE_ID.

  @retval EFI_SUCCESS              The operation finished successfully.
  @retval EFI_INVALID_PARAMETER    Any input parameter is invalid.

**/
EFI_STATUS
Ip6ParseInterfaceIdFromString (
  IN CONST CHAR16                  *String,
  OUT EFI_IP6_CONFIG_INTERFACE_ID  *IfId
  )
{
  UINT8   Index;
  CHAR16  *IfIdStr;
  CHAR16  *TempStr;
  UINTN   NodeVal;

  if ((String == NULL) || (IfId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  IfIdStr = (CHAR16 *)String;

  ZeroMem (IfId, sizeof (EFI_IP6_CONFIG_INTERFACE_ID));

  for (Index = 0; Index < 8; Index++) {
    TempStr = IfIdStr;

    while ((*IfIdStr != L'\0') && (*IfIdStr != L':')) {
      IfIdStr++;
    }

    //
    // The InterfaceId format is X:X:X:X, the number of X should not exceed 8.
    // If the number of X is less than 8, zero is appended to the InterfaceId.
    //
    if ((*IfIdStr == ':') && (Index == 7)) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Convert the string to interface id. AsciiStrHexToUintn stops at the
    // first character that is not a valid hex character, ':' or '\0' here.
    //
    NodeVal = StrHexToUintn (TempStr);
    if (NodeVal > 0xFF) {
      return EFI_INVALID_PARAMETER;
    }

    IfId->Id[Index] = (UINT8)NodeVal;

    IfIdStr++;
  }

  return EFI_SUCCESS;
}

/**
  Create Hii Extend Label OpCode as the start opcode and end opcode. It is
  a help function.

  @param[in]  StartLabelNumber   The number of start label.
  @param[out] StartOpCodeHandle  Points to the start opcode handle.
  @param[out] StartLabel         Points to the created start opcode.
  @param[out] EndOpCodeHandle    Points to the end opcode handle.
  @param[out] EndLabel           Points to the created end opcode.

  @retval EFI_OUT_OF_RESOURCES   Does not have sufficient resources to finish this
                                 operation.
  @retval EFI_INVALID_PARAMETER  Any input parameter is invalid.
  @retval EFI_SUCCESS            The operation completed successfully.

**/
EFI_STATUS
Ip6CreateOpCode (
  IN  UINT16              StartLabelNumber,
  OUT VOID                **StartOpCodeHandle,
  OUT EFI_IFR_GUID_LABEL  **StartLabel,
  OUT VOID                **EndOpCodeHandle,
  OUT EFI_IFR_GUID_LABEL  **EndLabel
  )
{
  EFI_STATUS          Status;
  EFI_IFR_GUID_LABEL  *InternalStartLabel;
  EFI_IFR_GUID_LABEL  *InternalEndLabel;

  if ((StartOpCodeHandle == NULL) || (StartLabel == NULL) || (EndOpCodeHandle == NULL) || (EndLabel == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *StartOpCodeHandle = NULL;
  *EndOpCodeHandle   = NULL;
  Status             = EFI_OUT_OF_RESOURCES;

  //
  // Initialize the container for dynamic opcodes.
  //
  *StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (*StartOpCodeHandle == NULL) {
    return Status;
  }

  *EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (*EndOpCodeHandle == NULL) {
    goto Exit;
  }

  //
  // Create Hii Extend Label OpCode as the start opcode.
  //
  InternalStartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                               *StartOpCodeHandle,
                                               &gEfiIfrTianoGuid,
                                               NULL,
                                               sizeof (EFI_IFR_GUID_LABEL)
                                               );
  if (InternalStartLabel == NULL) {
    goto Exit;
  }

  InternalStartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  InternalStartLabel->Number       = StartLabelNumber;

  //
  // Create Hii Extend Label OpCode as the end opcode.
  //
  InternalEndLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                             *EndOpCodeHandle,
                                             &gEfiIfrTianoGuid,
                                             NULL,
                                             sizeof (EFI_IFR_GUID_LABEL)
                                             );
  if (InternalEndLabel == NULL) {
    goto Exit;
  }

  InternalEndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  InternalEndLabel->Number       = LABEL_END;

  *StartLabel = InternalStartLabel;
  *EndLabel   = InternalEndLabel;

  return EFI_SUCCESS;

Exit:

  if (*StartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (*StartOpCodeHandle);
  }

  if (*EndOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (*EndOpCodeHandle);
  }

  return Status;
}

/**
  This function converts the different format of address list to string format and
  then generates the corresponding text opcode to illustrate the address info in
  IP6 configuration page. Currently, the following formats are supported:
  EFI_IP6_ADDRESS_INFO AddressType: Ip6ConfigNvHostAddress;
  EFI_IPv6_ADDRESS     AddressType: Ip6ConfigNvGatewayAddress and Ip6ConfigNvDnsAddress;
  EFI_IP6_ROUTE_TABLE  AddressType: Ip6ConfigNvRouteTable.

  @param[in, out] String           The pointer to the buffer to store the converted
                                   string.
  @param[in]      HiiHandle        A handle that was previously registered in the
                                   HII Database.
  @param[in]      AddressType      The address type.
  @param[in]      AddressInfo      Pointer to the address list.
  @param[in]      AddressCount     The address count of the address list.

  @retval EFI_SUCCESS              The operation finished successfully.
  @retval EFI_INVALID_PARAMETER    Any input parameter is invalid.
  @retval EFI_UNSUPPORTED          The AddressType is not supported.


**/
EFI_STATUS
Ip6ConvertAddressListToString (
  IN OUT CHAR16                      *String,
  IN     EFI_HII_HANDLE              HiiHandle,
  IN     IP6_CONFIG_NV_ADDRESS_TYPE  AddressType,
  IN     VOID                        *AddressInfo,
  IN     UINTN                       AddressCount
  )
{
  UINTN               Index;
  UINTN               Number;
  CHAR16              *TempStr;
  EFI_STATUS          Status;
  VOID                *StartOpCodeHandle;
  EFI_IFR_GUID_LABEL  *StartLabel;
  VOID                *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL  *EndLabel;
  UINT16              StartLabelNumber;
  EFI_STRING_ID       TextTwo;
  UINT8               *AddressHead;
  UINT8               PrefixLength;
  EFI_IPv6_ADDRESS    *Address;

  if ((String == NULL) || (HiiHandle == NULL) || (AddressInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (AddressType == Ip6ConfigNvHostAddress) {
    StartLabelNumber = HOST_ADDRESS_LABEL;
  } else if (AddressType == Ip6ConfigNvGatewayAddress) {
    StartLabelNumber = GATEWAY_ADDRESS_LABEL;
  } else if (AddressType == Ip6ConfigNvDnsAddress) {
    StartLabelNumber = DNS_ADDRESS_LABEL;
  } else if (AddressType == Ip6ConfigNvRouteTable) {
    StartLabelNumber = ROUTE_TABLE_LABEL;
  } else {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  Status = Ip6CreateOpCode (
             StartLabelNumber,
             &StartOpCodeHandle,
             &StartLabel,
             &EndOpCodeHandle,
             &EndLabel
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  AddressHead = (UINT8 *)AddressInfo;

  for (Index = 0; Index < AddressCount; Index++) {
    if (AddressType == Ip6ConfigNvHostAddress) {
      AddressInfo = AddressHead + sizeof (EFI_IP6_ADDRESS_INFO) * Index;
      Address     = &((EFI_IP6_ADDRESS_INFO *)AddressInfo)->Address;
    } else if (AddressType == Ip6ConfigNvRouteTable) {
      AddressInfo = AddressHead + sizeof (EFI_IP6_ROUTE_TABLE) * Index;
      Address     = &((EFI_IP6_ROUTE_TABLE *)AddressInfo)->Destination;
    } else {
      AddressInfo = AddressHead + sizeof (EFI_IPv6_ADDRESS) * Index;
      Address     = AddressInfo;
    }

    //
    // Convert the IP address info to string.
    //
    Ip6ToStr (Address, String);
    TempStr = String + StrLen (String);

    if ((AddressType == Ip6ConfigNvHostAddress) || (AddressType == Ip6ConfigNvRouteTable)) {
      if (AddressType == Ip6ConfigNvHostAddress) {
        PrefixLength = ((EFI_IP6_ADDRESS_INFO *)AddressInfo)->PrefixLength;
      } else {
        PrefixLength = ((EFI_IP6_ROUTE_TABLE *)AddressInfo)->PrefixLength;
      }

      //
      // Append the prefix length to the string.
      //
      *TempStr = L'/';
      TempStr++;
      Number  = UnicodeSPrint (TempStr, 6, L"%d", PrefixLength);
      TempStr = TempStr + Number;
    }

    if (AddressType == Ip6ConfigNvRouteTable) {
      //
      // Append " >> " to the string.
      //
      Number  = UnicodeSPrint (TempStr, 8, L" >>  ");
      TempStr = TempStr + Number;

      //
      // Append the gateway address to the string.
      //
      Ip6ToStr (&((EFI_IP6_ROUTE_TABLE *)AddressInfo)->Gateway, TempStr);
      TempStr = TempStr + StrLen (TempStr);
    }

    //
    // Generate a text opcode and update the UI.
    //
    TextTwo = HiiSetString (HiiHandle, 0, String, NULL);
    if (TextTwo == 0) {
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    HiiCreateTextOpCode (StartOpCodeHandle, STR_NULL, STR_NULL, TextTwo);

    String  = TempStr;
    *String = IP6_ADDRESS_DELIMITER;
    String++;
  }

  *(String - 1) = '\0';

  Status = HiiUpdateForm (
             HiiHandle,                       // HII handle
             &gIp6ConfigNvDataGuid,           // Formset GUID
             FORMID_MAIN_FORM,                // Form ID
             StartOpCodeHandle,               // Label for where to insert opcodes
             EndOpCodeHandle                  // Replace data
             );

Exit:
  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  return Status;
}

/**
  Parse address list in string format and convert it to a list array of node in
  IP6_ADDRESS_INFO_ENTRY.

  @param[in]        String         The buffer to string to be parsed.
  @param[out]       ListHead       The list head of array.
  @param[out]       AddressCount   The number of list nodes in the array.

  @retval EFI_SUCCESS              The operation finished successfully.
  @retval EFI_INVALID_PARAMETER    Any input parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES     Failed to perform the operation due to lack of resource.

**/
EFI_STATUS
Ip6ParseAddressListFromString (
  IN CONST CHAR16  *String,
  OUT LIST_ENTRY   *ListHead,
  OUT UINT32       *AddressCount
  )
{
  EFI_STATUS              Status;
  CHAR16                  *LocalString;
  CHAR16                  *Temp;
  CHAR16                  *TempStr;
  EFI_IP6_ADDRESS_INFO    AddressInfo;
  IP6_ADDRESS_INFO_ENTRY  *Node;
  BOOLEAN                 Last;
  UINT32                  Count;

  if ((String == NULL) || (ListHead == NULL) || (AddressCount == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&AddressInfo, sizeof (EFI_IP6_ADDRESS_INFO));
  LocalString = (CHAR16 *)AllocateCopyPool (StrSize (String), String);
  if (LocalString == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Clean the original address list.
  //
  Ip6FreeAddressInfoList (ListHead);

  Temp  = LocalString;
  Last  = FALSE;
  Count = 0;

  while (*LocalString != L'\0') {
    TempStr = LocalString;
    while ((*LocalString != L'\0') && (*LocalString != IP6_ADDRESS_DELIMITER)) {
      LocalString++;
    }

    if (*LocalString == L'\0') {
      Last = TRUE;
    }

    *LocalString = L'\0';

    Status = NetLibStrToIp6andPrefix (TempStr, &AddressInfo.Address, &AddressInfo.PrefixLength);
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    if (AddressInfo.PrefixLength == 0xFF) {
      AddressInfo.PrefixLength = 0;
    }

    if (!NetIp6IsValidUnicast (&AddressInfo.Address)) {
      Status = EFI_INVALID_PARAMETER;
      goto Error;
    }

    Node = AllocatePool (sizeof (IP6_ADDRESS_INFO_ENTRY));
    if (Node == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Error;
    }

    CopyMem (&Node->AddrInfo, &AddressInfo, sizeof (EFI_IP6_ADDRESS_INFO));
    InsertTailList (ListHead, &Node->Link);
    Count++;

    if (Last) {
      break;
    }

    LocalString++;
  }

  FreePool (Temp);
  *AddressCount = Count;
  return EFI_SUCCESS;

Error:
  Ip6FreeAddressInfoList (ListHead);
  FreePool (Temp);
  return Status;
}

/**
  This function converts the interface info to string and draws it to the IP6 UI.
  The interface information includes interface name, interface type, hardware
  address and route table information.

  @param[in]       IfInfo          The pointer of EFI_IP6_CONFIG_INTERFACE_INFO.
  @param[in]       HiiHandle       The handle that was previously registered in the
                                   HII Database.
  @param[in, out]  IfrNvData       Points to IP6_CONFIG_IFR_NVDATA.

  @retval EFI_SUCCESS              The operation finished successfully.
  @retval EFI_INVALID_PARAMETER    Any input parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES     The operation failed due to lack of resources.

**/
EFI_STATUS
Ip6ConvertInterfaceInfoToString (
  IN     EFI_IP6_CONFIG_INTERFACE_INFO  *IfInfo,
  IN     EFI_HII_HANDLE                 HiiHandle,
  IN OUT IP6_CONFIG_IFR_NVDATA          *IfrNvData
  )
{
  UINT32         Index;
  UINTN          Number;
  CHAR16         *String;
  CHAR16         PortString[ADDRESS_STR_MAX_SIZE];
  CHAR16         FormatString[8];
  EFI_STRING_ID  StringId;

  if ((IfInfo == NULL) || (HiiHandle == NULL) || (IfrNvData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Print the interface name.
  //
  StringId = HiiSetString (
               HiiHandle,
               STRING_TOKEN (STR_IP6_INTERFACE_NAME_CONTENT),
               IfInfo->Name,
               NULL
               );
  if (StringId == 0) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Print the interface type.
  //
  if (IfInfo->IfType == Ip6InterfaceTypeEthernet) {
    CopyMem (PortString, IP6_ETHERNET, sizeof (IP6_ETHERNET));
  } else if (IfInfo->IfType == Ip6InterfaceTypeExperimentalEthernet) {
    CopyMem (PortString, IP6_EXPERIMENTAL_ETHERNET, sizeof (IP6_EXPERIMENTAL_ETHERNET));
  } else {
    //
    // Refer to RFC1700, chapter Number Hardware Type.
    //
    UnicodeSPrint (PortString, 6, L"%d", IfInfo->IfType);
  }

  StringId = HiiSetString (
               HiiHandle,
               STRING_TOKEN (STR_IP6_INTERFACE_TYPE_CONTENT),
               PortString,
               NULL
               );
  if (StringId == 0) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Convert the hardware address.
  //
  String = PortString;
  ASSERT (IfInfo->HwAddressSize <= 32);

  for (Index = 0; Index < IfInfo->HwAddressSize; Index++) {
    if (IfInfo->HwAddress.Addr[Index] < 0x10) {
      CopyMem (FormatString, L"0%x-", sizeof (L"0%x-"));
    } else {
      CopyMem (FormatString, L"%x-", sizeof (L"%x-"));
    }

    Number = UnicodeSPrint (
               String,
               8,
               (CONST CHAR16 *)FormatString,
               (UINTN)IfInfo->HwAddress.Addr[Index]
               );
    String = String + Number;
  }

  if (Index != 0) {
    ASSERT (String > PortString);
    String--;
    *String = '\0';
  }

  //
  // Print the hardware address.
  //
  StringId = HiiSetString (
               HiiHandle,
               STRING_TOKEN (STR_IP6_MAC_ADDRESS_CONTENT),
               PortString,
               NULL
               );
  if (StringId == 0) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Build the address info list from list array of node in IP6_ADDRESS_INFO_ENTRY.

  @param[in]      Instance         Points to IP6 config instance data.
  @param[in]      AddressType      The address type.
  @param[out]     AddressInfo      The pointer to the buffer to store the address list.
  @param[out]     AddressSize      The address size of the address list.

  @retval EFI_SUCCESS              The operation finished successfully.
  @retval EFI_INVALID_PARAMETER    Any input parameter is invalid.
  @retval EFI_UNSUPPORTED          The AddressType is not supported.

**/
EFI_STATUS
Ip6BuildNvAddressInfo (
  IN  IP6_CONFIG_INSTANCE         *Instance,
  IN  IP6_CONFIG_NV_ADDRESS_TYPE  AddressType,
  OUT VOID                        **AddressInfo,
  OUT UINTN                       *AddressSize
  )
{
  IP6_CONFIG_NVDATA              *Ip6NvData;
  LIST_ENTRY                     *Entry;
  LIST_ENTRY                     *ListHead;
  IP6_ADDRESS_INFO_ENTRY         *Node;
  VOID                           *AddressList;
  VOID                           *TmpStr;
  UINTN                          DataSize;
  EFI_IPv6_ADDRESS               *Ip6Address;
  EFI_IP6_CONFIG_MANUAL_ADDRESS  *ManualAddress;

  if ((Instance == NULL) || (AddressInfo == NULL) || (AddressSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  NET_CHECK_SIGNATURE (Instance, IP6_CONFIG_INSTANCE_SIGNATURE);

  Ip6NvData = &Instance->Ip6NvData;

  if (AddressType == Ip6ConfigNvHostAddress) {
    ListHead = &Ip6NvData->ManualAddress;
    DataSize = sizeof (EFI_IP6_CONFIG_MANUAL_ADDRESS) * Ip6NvData->ManualAddressCount;
  } else if (AddressType == Ip6ConfigNvGatewayAddress) {
    ListHead = &Ip6NvData->GatewayAddress;
    DataSize = sizeof (EFI_IPv6_ADDRESS) * Ip6NvData->GatewayAddressCount;
  } else if (AddressType == Ip6ConfigNvDnsAddress) {
    ListHead = &Ip6NvData->DnsAddress;
    DataSize = sizeof (EFI_IPv6_ADDRESS) * Ip6NvData->DnsAddressCount;
  } else {
    return EFI_UNSUPPORTED;
  }

  AddressList = AllocateZeroPool (DataSize);
  if (AddressList  == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TmpStr = AddressList;

  NET_LIST_FOR_EACH (Entry, ListHead) {
    Node = NET_LIST_USER_STRUCT (Entry, IP6_ADDRESS_INFO_ENTRY, Link);
    if (AddressType == Ip6ConfigNvHostAddress) {
      ManualAddress = (EFI_IP6_CONFIG_MANUAL_ADDRESS *)AddressList;
      IP6_COPY_ADDRESS (&ManualAddress->Address, &Node->AddrInfo.Address);
      ManualAddress->PrefixLength = Node->AddrInfo.PrefixLength;
      AddressList                 = (UINT8 *)AddressList + sizeof (EFI_IP6_CONFIG_MANUAL_ADDRESS);
    } else {
      Ip6Address = (EFI_IPv6_ADDRESS *)AddressList;
      IP6_COPY_ADDRESS (Ip6Address, &Node->AddrInfo.Address);
      AddressList = (UINT8 *)AddressList + sizeof (EFI_IPv6_ADDRESS);
    }
  }

  *AddressInfo = TmpStr;
  *AddressSize = DataSize;
  return EFI_SUCCESS;
}

/**
  Convert the IP6 configuration data into the IFR data.

  @param[in, out]  IfrNvData       The IFR NV data.
  @param[in]       Instance        The IP6 config instance data.

  @retval EFI_SUCCESS              The operation finished successfully.
  @retval EFI_INVALID_PARAMETER    Any input parameter is invalid.
  @retval EFI_UNSUPPORTED          The policy is not supported in the current implementation.
  @retval Others                   Other errors as indicated.

**/
EFI_STATUS
Ip6ConvertConfigNvDataToIfrNvData (
  IN OUT IP6_CONFIG_IFR_NVDATA  *IfrNvData,
  IN     IP6_CONFIG_INSTANCE    *Instance
  )
{
  IP6_CONFIG_NVDATA                         *Ip6NvData;
  EFI_IP6_CONFIG_PROTOCOL                   *Ip6Config;
  UINTN                                     DataSize;
  VOID                                      *Data;
  EFI_STATUS                                Status;
  EFI_IP6_CONFIG_POLICY                     Policy;
  EFI_IP6_CONFIG_DUP_ADDR_DETECT_TRANSMITS  DadXmits;
  EFI_HII_HANDLE                            HiiHandle;

  if ((IfrNvData == NULL) || (Instance == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  NET_CHECK_SIGNATURE (Instance, IP6_CONFIG_INSTANCE_SIGNATURE);

  Ip6Config = &Instance->Ip6Config;
  Ip6NvData = &Instance->Ip6NvData;
  Data      = NULL;
  DataSize  = 0;
  HiiHandle = Instance->CallbackInfo.RegisteredHandle;

  //
  // Get the current interface info.
  //
  Status = Ip6ConfigNvGetData (
             Ip6Config,
             Ip6ConfigDataTypeInterfaceInfo,
             &DataSize,
             (VOID **)&Data
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Convert the interface info to string and print.
  //
  Status = Ip6ConvertInterfaceInfoToString (
             (EFI_IP6_CONFIG_INTERFACE_INFO *)Data,
             HiiHandle,
             IfrNvData
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Get the interface id.
  //
  DataSize = sizeof (EFI_IP6_CONFIG_INTERFACE_ID);
  ZeroMem (&Ip6NvData->InterfaceId, DataSize);
  Status = Ip6Config->GetData (
                        Ip6Config,
                        Ip6ConfigDataTypeAltInterfaceId,
                        &DataSize,
                        &Ip6NvData->InterfaceId
                        );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Ip6ConvertInterfaceIdToString (IfrNvData->InterfaceId, &Ip6NvData->InterfaceId);

  //
  // Get current policy.
  //
  DataSize = sizeof (EFI_IP6_CONFIG_POLICY);
  Status   = Ip6Config->GetData (
                          Ip6Config,
                          Ip6ConfigDataTypePolicy,
                          &DataSize,
                          &Policy
                          );

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  if (Policy == Ip6ConfigPolicyManual) {
    IfrNvData->Policy = IP6_POLICY_MANUAL;
  } else if (Policy == Ip6ConfigPolicyAutomatic) {
    IfrNvData->Policy = IP6_POLICY_AUTO;
  } else {
    ASSERT (FALSE);
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  //
  // Get Duplicate Address Detection Transmits count.
  //
  DataSize = sizeof (EFI_IP6_CONFIG_DUP_ADDR_DETECT_TRANSMITS);
  Status   = Ip6Config->GetData (
                          Ip6Config,
                          Ip6ConfigDataTypeDupAddrDetectTransmits,
                          &DataSize,
                          &DadXmits
                          );

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  IfrNvData->DadTransmitCount = DadXmits.DupAddrDetectTransmits;

Exit:
  if (Data != NULL) {
    FreePool (Data);
  }

  return Status;
}

/**
  Convert IFR data into IP6 configuration data. The policy, alternative interface
  ID, and DAD transmit counts, and will be saved.

  @param[in]       IfrNvData       The IFR NV data.
  @param[in, out]  Instance        The IP6 config instance data.

  @retval EFI_SUCCESS              The operation finished successfully.
  @retval EFI_INVALID_PARAMETER    Any input parameter is invalid.
  @retval Others                   Other errors as indicated.

**/
EFI_STATUS
Ip6ConvertIfrNvDataToConfigNvDataGeneral (
  IN     IP6_CONFIG_IFR_NVDATA  *IfrNvData,
  IN OUT IP6_CONFIG_INSTANCE    *Instance
  )
{
  IP6_CONFIG_NVDATA        *Ip6NvData;
  EFI_IP6_CONFIG_PROTOCOL  *Ip6Config;
  EFI_STATUS               Status;

  if ((IfrNvData == NULL) || (Instance == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  NET_CHECK_SIGNATURE (Instance, IP6_CONFIG_INSTANCE_SIGNATURE);
  Ip6NvData = &Instance->Ip6NvData;
  Ip6Config = &Instance->Ip6Config;

  //
  // Update those fields which don't have INTERACTIVE attribute.
  //
  if (IfrNvData->Policy == IP6_POLICY_AUTO) {
    Ip6NvData->Policy = Ip6ConfigPolicyAutomatic;
  } else if (IfrNvData->Policy == IP6_POLICY_MANUAL) {
    Ip6NvData->Policy = Ip6ConfigPolicyManual;
  }

  Ip6NvData->DadTransmitCount.DupAddrDetectTransmits = IfrNvData->DadTransmitCount;

  //
  // Set the configured policy.
  //
  Status = Ip6Config->SetData (
                        Ip6Config,
                        Ip6ConfigDataTypePolicy,
                        sizeof (EFI_IP6_CONFIG_POLICY),
                        &Ip6NvData->Policy
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set the duplicate address detection transmits count.
  //
  Status = Ip6Config->SetData (
                        Ip6Config,
                        Ip6ConfigDataTypeDupAddrDetectTransmits,
                        sizeof (EFI_IP6_CONFIG_DUP_ADDR_DETECT_TRANSMITS),
                        &Ip6NvData->DadTransmitCount
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set the alternative interface ID
  //
  Status = Ip6Config->SetData (
                        Ip6Config,
                        Ip6ConfigDataTypeAltInterfaceId,
                        sizeof (EFI_IP6_CONFIG_INTERFACE_ID),
                        &Ip6NvData->InterfaceId
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Convert IFR data into IP6 configuration data. The policy, configured
  manual address, gateway address, and DNS server address will be saved.

  @param[in]       IfrNvData       The IFR NV data.
  @param[in, out]  Instance        The IP6 config instance data.

  @retval EFI_SUCCESS              The operation finished successfully.
  @retval EFI_INVALID_PARAMETER    Any input parameter is invalid.
  @retval Others                   Other errors as indicated.

**/
EFI_STATUS
Ip6ConvertIfrNvDataToConfigNvDataAdvanced (
  IN     IP6_CONFIG_IFR_NVDATA  *IfrNvData,
  IN OUT IP6_CONFIG_INSTANCE    *Instance
  )
{
  IP6_CONFIG_NVDATA              *Ip6NvData;
  EFI_IP6_CONFIG_PROTOCOL        *Ip6Config;
  EFI_STATUS                     Status;
  EFI_IP6_CONFIG_MANUAL_ADDRESS  *ManualAddress;
  EFI_IPv6_ADDRESS               *Address;
  BOOLEAN                        IsAddressOk;
  EFI_EVENT                      SetAddressEvent;
  EFI_EVENT                      TimeoutEvent;
  UINTN                          DataSize;

  if ((IfrNvData == NULL) || (Instance == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (IfrNvData->Policy == IP6_POLICY_AUTO) {
    return EFI_SUCCESS;
  }

  NET_CHECK_SIGNATURE (Instance, IP6_CONFIG_INSTANCE_SIGNATURE);
  Ip6NvData = &Instance->Ip6NvData;
  Ip6Config = &Instance->Ip6Config;

  //
  // Update those fields which don't have INTERACTIVE attribute.
  //
  Ip6NvData->Policy = Ip6ConfigPolicyManual;

  //
  // Set the configured policy.
  //
  Status = Ip6Config->SetData (
                        Ip6Config,
                        Ip6ConfigDataTypePolicy,
                        sizeof (EFI_IP6_CONFIG_POLICY),
                        &Ip6NvData->Policy
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create events & timers for asynchronous settings.
  //
  SetAddressEvent = NULL;
  TimeoutEvent    = NULL;
  ManualAddress   = NULL;
  Address         = NULL;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  Ip6ConfigManualAddressNotify,
                  &IsAddressOk,
                  &SetAddressEvent
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &TimeoutEvent
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Set the manual address list. This is an asynchronous process.
  //
  if (!IsListEmpty (&Ip6NvData->ManualAddress) && (Ip6NvData->ManualAddressCount != 0)) {
    Status = Ip6BuildNvAddressInfo (
               Instance,
               Ip6ConfigNvHostAddress,
               (VOID **)&ManualAddress,
               &DataSize
               );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    IsAddressOk = FALSE;

    Status = Ip6Config->RegisterDataNotify (
                          Ip6Config,
                          Ip6ConfigDataTypeManualAddress,
                          SetAddressEvent
                          );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    Status = Ip6Config->SetData (
                          Ip6Config,
                          Ip6ConfigDataTypeManualAddress,
                          DataSize,
                          (VOID *)ManualAddress
                          );
    if (Status == EFI_NOT_READY) {
      gBS->SetTimer (TimeoutEvent, TimerRelative, 50000000);
      while (EFI_ERROR (gBS->CheckEvent (TimeoutEvent))) {
        if (IsAddressOk) {
          Status = EFI_SUCCESS;
        }

        break;
      }
    }

    Status = Ip6Config->UnregisterDataNotify (
                          Ip6Config,
                          Ip6ConfigDataTypeManualAddress,
                          SetAddressEvent
                          );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }
  }

  //
  // Set gateway address list.
  //
  if (!IsListEmpty (&Ip6NvData->GatewayAddress) && (Ip6NvData->GatewayAddressCount != 0)) {
    Status = Ip6BuildNvAddressInfo (
               Instance,
               Ip6ConfigNvGatewayAddress,
               (VOID **)&Address,
               &DataSize
               );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    Status = Ip6Config->SetData (
                          Ip6Config,
                          Ip6ConfigDataTypeGateway,
                          DataSize,
                          (VOID *)Address
                          );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    FreePool (Address);
    Address = NULL;
  }

  //
  // Set DNS server address list.
  //
  if (!IsListEmpty (&Ip6NvData->DnsAddress) && (Ip6NvData->DnsAddressCount != 0)) {
    Status = Ip6BuildNvAddressInfo (
               Instance,
               Ip6ConfigNvDnsAddress,
               (VOID **)&Address,
               &DataSize
               );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    Status = Ip6Config->SetData (
                          Ip6Config,
                          Ip6ConfigDataTypeDnsServer,
                          DataSize,
                          (VOID *)Address
                          );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }
  }

  Status = EFI_SUCCESS;

Exit:
  if (SetAddressEvent != NULL) {
    gBS->CloseEvent (SetAddressEvent);
  }

  if (TimeoutEvent != NULL) {
    gBS->CloseEvent (TimeoutEvent);
  }

  if (ManualAddress != NULL) {
    FreePool (ManualAddress);
  }

  if (Address != NULL) {
    FreePool (Address);
  }

  return Status;
}

/**
  This function allows the caller to request the current
  configuration for one or more named elements. The resulting
  string is in <ConfigAltResp> format. Any and all alternative
  configuration strings shall also be appended to the end of the
  current configuration string. If they are, they must appear
  after the current configuration. They must contain the same
  routing (GUID, NAME, PATH) as the current configuration string.
  They must have an additional description indicating the type of
  alternative configuration the string represents,
  "ALTCFG=<StringToken>". That <StringToken> (when
  converted from Hex UNICODE to binary) is a reference to a
  string in the associated string pack.

  @param[in] This       Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in] Request    A null-terminated Unicode string in
                        <ConfigRequest> format. Note that this
                        includes the routing information as well as
                        the configurable name / value pairs. It is
                        invalid for this string to be in
                        <MultiConfigRequest> format.
  @param[out] Progress  On return, points to a character in the
                        Request string. Points to the string's null
                        terminator if request was successful. Points
                        to the most recent "&" before the first
                        failing name / value pair (or the beginning
                        of the string if the failure is in the first
                        name / value pair) if the request was not
                        successful.
  @param[out] Results   A null-terminated Unicode string in
                        <ConfigAltResp> format which has all values
                        filled in for the names in the Request string.
                        String to be allocated by the called function.

  @retval EFI_SUCCESS             The Results string is filled with the
                                  values corresponding to all requested
                                  names.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the
                                  parts of the results that must be
                                  stored awaiting possible future
                                  protocols.
  @retval EFI_INVALID_PARAMETER   For example, passing in a NULL
                                  for the Request parameter
                                  would result in this type of
                                  error. In this case, the
                                  Progress parameter would be
                                  set to NULL.
  @retval EFI_NOT_FOUND           Routing data doesn't match any
                                  known driver. Progress set to the
                                  first character in the routing header.
                                  Note: There is no requirement that the
                                  driver validate the routing data. It
                                  must skip the <ConfigHdr> in order to
                                  process the names.
  @retval EFI_INVALID_PARAMETER   Illegal syntax. Progress set
                                  to most recent & before the
                                  error or the beginning of the
                                  string.
  @retval EFI_INVALID_PARAMETER   Unknown name. Progress points
                                  to the & before the name in
                                  question. Currently not implemented.
**/
EFI_STATUS
EFIAPI
Ip6FormExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT EFI_STRING                            *Progress,
  OUT EFI_STRING                            *Results
  )
{
  EFI_STATUS              Status;
  IP6_FORM_CALLBACK_INFO  *Private;
  IP6_CONFIG_INSTANCE     *Ip6ConfigInstance;
  IP6_CONFIG_IFR_NVDATA   *IfrNvData;
  EFI_STRING              ConfigRequestHdr;
  EFI_STRING              ConfigRequest;
  BOOLEAN                 AllocatedRequest;
  UINTN                   Size;
  UINTN                   BufferSize;

  if ((This == NULL) || (Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  if ((Request != NULL) &&
      !HiiIsConfigHdrMatch (Request, &gIp6ConfigNvDataGuid, mIp6ConfigStorageName))
  {
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  AllocatedRequest = FALSE;
  Size             = 0;

  Private           = IP6_FORM_CALLBACK_INFO_FROM_CONFIG_ACCESS (This);
  Ip6ConfigInstance = IP6_CONFIG_INSTANCE_FROM_FORM_CALLBACK (Private);
  BufferSize        = sizeof (IP6_CONFIG_IFR_NVDATA);

  IfrNvData = (IP6_CONFIG_IFR_NVDATA *)AllocateZeroPool (BufferSize);
  if (IfrNvData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = Ip6ConvertConfigNvDataToIfrNvData (IfrNvData, Ip6ConfigInstance);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator.
    //
    ConfigRequestHdr = HiiConstructConfigHdr (
                         &gIp6ConfigNvDataGuid,
                         mIp6ConfigStorageName,
                         Private->ChildHandle
                         );
    if (ConfigRequestHdr == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    Size          = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    if (ConfigRequest == NULL) {
      ASSERT (ConfigRequest != NULL);
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    AllocatedRequest = TRUE;
    UnicodeSPrint (
      ConfigRequest,
      Size,
      L"%s&OFFSET=0&WIDTH=%016LX",
      ConfigRequestHdr,
      (UINT64)BufferSize
      );
    FreePool (ConfigRequestHdr);
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *)IfrNvData,
                                BufferSize,
                                Results,
                                Progress
                                );

Exit:
  FreePool (IfrNvData);
  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
  }

  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}

/**
  This function applies changes in a driver's configuration.
  Input is a Configuration, which has the routing data for this
  driver followed by name / value configuration pairs. The driver
  must apply those pairs to its configurable storage. If the
  driver's configuration is stored in a linear block of data
  and the driver's name / value pairs are in <BlockConfig>
  format, it may use the ConfigToBlock helper function (above) to
  simplify the job. Currently not implemented.

  @param[in]  This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration  A null-terminated Unicode string in
                             <ConfigString> format.
  @param[out] Progress       A pointer to a string filled in with the
                             offset of the most recent '&' before the
                             first failing name / value pair (or the
                             beginning of the string if the failure
                             is in the first name / value pair) or
                             the terminating NULL if all was
                             successful.

  @retval EFI_SUCCESS             The results have been distributed or are
                                  awaiting distribution.
  @retval EFI_OUT_OF_MEMORY       Not enough memory to store the
                                  parts of the results that must be
                                  stored awaiting possible future
                                  protocols.
  @retval EFI_INVALID_PARAMETERS  Passing in a NULL for the
                                  Results parameter would result
                                  in this type of error.
  @retval EFI_NOT_FOUND           Target for the specified routing data
                                  was not found.
**/
EFI_STATUS
EFIAPI
Ip6FormRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                            *Progress
  )
{
  if ((This == NULL) || (Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check routing data in <ConfigHdr>.
  // Note: if only one Storage is used, then this checking could be skipped.
  //
  if (!HiiIsConfigHdrMatch (Configuration, &gIp6ConfigNvDataGuid, mIp6ConfigStorageName)) {
    *Progress = Configuration;
    return EFI_NOT_FOUND;
  }

  *Progress = Configuration + StrLen (Configuration);

  return EFI_SUCCESS;
}

/**
  Display host addresses, route table, DNS addresses and gateway addresses in
  "IPv6 Current Setting" page.

  @param[in]       Instance        The IP6 config instance data.

  @retval EFI_SUCCESS              The operation finished successfully.
  @retval Others                   Other errors as indicated.

**/
EFI_STATUS
Ip6GetCurrentSetting (
  IN IP6_CONFIG_INSTANCE  *Instance
  )
{
  EFI_IP6_CONFIG_PROTOCOL        *Ip6Config;
  EFI_HII_HANDLE                 HiiHandle;
  EFI_IP6_CONFIG_INTERFACE_INFO  *Data;
  UINTN                          DataSize;
  EFI_STATUS                     Status;
  CHAR16                         PortString[ADDRESS_STR_MAX_SIZE];
  EFI_IP6_CONFIG_INTERFACE_INFO  *IfInfo;

  Ip6Config = &Instance->Ip6Config;
  HiiHandle = Instance->CallbackInfo.RegisteredHandle;
  Data      = NULL;

  //
  // Get current interface info.
  //
  Status = Ip6ConfigNvGetData (
             Ip6Config,
             Ip6ConfigDataTypeInterfaceInfo,
             &DataSize,
             (VOID **)&Data
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Generate dynamic text opcode for host address and draw it.
  //
  IfInfo = (EFI_IP6_CONFIG_INTERFACE_INFO *)Data;
  Status = Ip6ConvertAddressListToString (
             PortString,
             HiiHandle,
             Ip6ConfigNvHostAddress,
             IfInfo->AddressInfo,
             IfInfo->AddressInfoCount
             );
  if (EFI_ERROR (Status)) {
    FreePool (Data);
    return Status;
  }

  //
  // Generate the dynamic text opcode for route table and draw it.
  //
  Status = Ip6ConvertAddressListToString (
             PortString,
             HiiHandle,
             Ip6ConfigNvRouteTable,
             IfInfo->RouteTable,
             IfInfo->RouteCount
             );
  if (EFI_ERROR (Status)) {
    FreePool (Data);
    return Status;
  }

  //
  // Get DNS server list.
  //
  FreePool (Data);
  DataSize = 0;
  Data     = NULL;
  Status   = Ip6ConfigNvGetData (
               Ip6Config,
               Ip6ConfigDataTypeDnsServer,
               &DataSize,
               (VOID **)&Data
               );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    if (Data != NULL) {
      FreePool (Data);
    }

    return Status;
  }

  if (DataSize > 0) {
    //
    // Generate the dynamic text opcode for DNS server and draw it.
    //
    Status = Ip6ConvertAddressListToString (
               PortString,
               HiiHandle,
               Ip6ConfigNvDnsAddress,
               Data,
               DataSize / sizeof (EFI_IPv6_ADDRESS)
               );
    if (EFI_ERROR (Status)) {
      FreePool (Data);
      return Status;
    }
  }

  //
  // Get gateway address list.
  //
  if (Data != NULL) {
    FreePool (Data);
  }

  DataSize = 0;
  Data     = NULL;
  Status   = Ip6ConfigNvGetData (
               Ip6Config,
               Ip6ConfigDataTypeGateway,
               &DataSize,
               (VOID **)&Data
               );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    if (Data != NULL) {
      FreePool (Data);
    }

    return Status;
  }

  if (DataSize > 0) {
    //
    // Generate the dynamic text opcode for gateway and draw it.
    //
    Status = Ip6ConvertAddressListToString (
               PortString,
               HiiHandle,
               Ip6ConfigNvGatewayAddress,
               Data,
               DataSize / sizeof (EFI_IPv6_ADDRESS)
               );
    if (EFI_ERROR (Status)) {
      FreePool (Data);
      return Status;
    }
  }

  if (Data != NULL) {
    FreePool (Data);
  }

  return EFI_SUCCESS;
}

/**
  This function is called to provide results data to the driver.
  This data consists of a unique key that is used to identify
  which data is either being passed back or being asked for.

  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Action             Specifies the type of action taken by the browser.
  @param[in]  QuestionId         A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect. The format of the data tends to
                                 vary based on the opcode that generated the callback.
  @param[in]  Type               The type of value for the question.
  @param[in]  Value              A pointer to the data being sent to the original
                                 exporting driver.
  @param[out]  ActionRequest     On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback. Currently not implemented.
  @retval EFI_INVALID_PARAMETER  Passed in the wrong parameter.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
EFIAPI
Ip6FormCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  EFI_BROWSER_ACTION                    Action,
  IN  EFI_QUESTION_ID                       QuestionId,
  IN  UINT8                                 Type,
  IN  EFI_IFR_TYPE_VALUE                    *Value,
  OUT EFI_BROWSER_ACTION_REQUEST            *ActionRequest
  )
{
  IP6_FORM_CALLBACK_INFO  *Private;
  UINTN                   BufferSize;
  IP6_CONFIG_IFR_NVDATA   *IfrNvData;
  EFI_STATUS              Status;
  EFI_INPUT_KEY           Key;
  IP6_CONFIG_INSTANCE     *Instance;
  IP6_CONFIG_NVDATA       *Ip6NvData;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private   = IP6_FORM_CALLBACK_INFO_FROM_CONFIG_ACCESS (This);
  Instance  = IP6_CONFIG_INSTANCE_FROM_FORM_CALLBACK (Private);
  Ip6NvData = &Instance->Ip6NvData;

  if ((Action == EFI_BROWSER_ACTION_FORM_OPEN) || (Action == EFI_BROWSER_ACTION_FORM_CLOSE)) {
    return EFI_SUCCESS;
  }

  if ((Action != EFI_BROWSER_ACTION_CHANGING) && (Action != EFI_BROWSER_ACTION_CHANGED)) {
    return EFI_UNSUPPORTED;
  }

  if ((Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Retrieve uncommitted data from Browser
  //

  BufferSize = sizeof (IP6_CONFIG_IFR_NVDATA);
  IfrNvData  = AllocateZeroPool (BufferSize);
  if (IfrNvData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EFI_SUCCESS;

  HiiGetBrowserData (NULL, NULL, BufferSize, (UINT8 *)IfrNvData);

  if (Action == EFI_BROWSER_ACTION_CHANGING) {
    switch (QuestionId) {
      case KEY_GET_CURRENT_SETTING:
        Status = Ip6GetCurrentSetting (Instance);
        break;

      default:
        break;
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGED) {
    switch (QuestionId) {
      case KEY_SAVE_CONFIG_CHANGES:
        Status = Ip6ConvertIfrNvDataToConfigNvDataAdvanced (IfrNvData, Instance);
        if (EFI_ERROR (Status)) {
          break;
        }

        Status = Ip6GetCurrentSetting (Instance);

        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT;
        break;

      case KEY_IGNORE_CONFIG_CHANGES:
        Ip6FreeAddressInfoList (&Ip6NvData->ManualAddress);
        Ip6FreeAddressInfoList (&Ip6NvData->GatewayAddress);
        Ip6FreeAddressInfoList (&Ip6NvData->DnsAddress);

        Ip6NvData->ManualAddressCount  = 0;
        Ip6NvData->GatewayAddressCount = 0;
        Ip6NvData->DnsAddressCount     = 0;

        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD_EXIT;
        break;

      case KEY_SAVE_CHANGES:
        Status = Ip6ConvertIfrNvDataToConfigNvDataGeneral (IfrNvData, Instance);
        if (EFI_ERROR (Status)) {
          break;
        }

        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_SUBMIT;
        break;

      case KEY_INTERFACE_ID:
        Status = Ip6ParseInterfaceIdFromString (IfrNvData->InterfaceId, &Ip6NvData->InterfaceId);
        if (EFI_ERROR (Status)) {
          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"Invalid Interface ID!",
            NULL
            );
        }

        break;

      case KEY_MANUAL_ADDRESS:
        Status = Ip6ParseAddressListFromString (
                   IfrNvData->ManualAddress,
                   &Ip6NvData->ManualAddress,
                   &Ip6NvData->ManualAddressCount
                   );
        if (EFI_ERROR (Status)) {
          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"Invalid Host Addresses!",
            NULL
            );
        }

        break;

      case KEY_GATEWAY_ADDRESS:
        Status = Ip6ParseAddressListFromString (
                   IfrNvData->GatewayAddress,
                   &Ip6NvData->GatewayAddress,
                   &Ip6NvData->GatewayAddressCount
                   );
        if (EFI_ERROR (Status)) {
          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"Invalid Gateway Addresses!",
            NULL
            );
        }

        break;

      case KEY_DNS_ADDRESS:
        Status = Ip6ParseAddressListFromString (
                   IfrNvData->DnsAddress,
                   &Ip6NvData->DnsAddress,
                   &Ip6NvData->DnsAddressCount
                   );
        if (EFI_ERROR (Status)) {
          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"Invalid DNS Addresses!",
            NULL
            );
        }

        break;

      default:
        break;
    }
  }

  if (!EFI_ERROR (Status)) {
    //
    // Pass changed uncommitted data back to Form Browser.
    //
    BufferSize = sizeof (IP6_CONFIG_IFR_NVDATA);
    HiiSetBrowserData (NULL, NULL, BufferSize, (UINT8 *)IfrNvData, NULL);
  }

  FreePool (IfrNvData);
  return Status;
}

/**
  Install HII Config Access protocol for network device and allocate resources.

  @param[in, out]  Instance      The IP6_CONFIG_INSTANCE to create a form.

  @retval EFI_SUCCESS            The HII Config Access protocol is installed.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
Ip6ConfigFormInit (
  IN OUT IP6_CONFIG_INSTANCE  *Instance
  )
{
  EFI_STATUS                      Status;
  IP6_SERVICE                     *IpSb;
  IP6_FORM_CALLBACK_INFO          *CallbackInfo;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;
  VENDOR_DEVICE_PATH              VendorDeviceNode;
  EFI_SERVICE_BINDING_PROTOCOL    *MnpSb;
  CHAR16                          *MacString;
  CHAR16                          MenuString[128];
  CHAR16                          PortString[128];
  CHAR16                          *OldMenuString;
  EFI_DEVICE_PATH_PROTOCOL        *ParentDevicePath;

  IpSb = IP6_SERVICE_FROM_IP6_CONFIG_INSTANCE (Instance);
  ASSERT (IpSb != NULL);

  Status = gBS->HandleProtocol (
                  IpSb->Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CallbackInfo            = &Instance->CallbackInfo;
  CallbackInfo->Signature = IP6_FORM_CALLBACK_INFO_SIGNATURE;

  //
  // Construct device path node for EFI HII Config Access protocol,
  // which consists of controller physical device path and one hardware
  // vendor guid node.
  //
  ZeroMem (&VendorDeviceNode, sizeof (VENDOR_DEVICE_PATH));
  VendorDeviceNode.Header.Type    = HARDWARE_DEVICE_PATH;
  VendorDeviceNode.Header.SubType = HW_VENDOR_DP;

  CopyGuid (&VendorDeviceNode.Guid, &gEfiCallerIdGuid);

  SetDevicePathNodeLength (&VendorDeviceNode.Header, sizeof (VENDOR_DEVICE_PATH));
  CallbackInfo->HiiVendorDevicePath = AppendDevicePathNode (
                                        ParentDevicePath,
                                        (EFI_DEVICE_PATH_PROTOCOL *)&VendorDeviceNode
                                        );
  if (CallbackInfo->HiiVendorDevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  ConfigAccess                = &CallbackInfo->HiiConfigAccess;
  ConfigAccess->ExtractConfig = Ip6FormExtractConfig;
  ConfigAccess->RouteConfig   = Ip6FormRouteConfig;
  ConfigAccess->Callback      = Ip6FormCallback;

  //
  // Install Device Path Protocol and Config Access protocol on new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &CallbackInfo->ChildHandle,
                  &gEfiDevicePathProtocolGuid,
                  CallbackInfo->HiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  ConfigAccess,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Open the Parent Handle for the child
    //
    Status = gBS->OpenProtocol (
                    IpSb->Controller,
                    &gEfiManagedNetworkServiceBindingProtocolGuid,
                    (VOID **)&MnpSb,
                    IpSb->Image,
                    CallbackInfo->ChildHandle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
  }

  if (EFI_ERROR (Status)) {
    goto Error;
  }

  //
  // Publish our HII data
  //
  CallbackInfo->RegisteredHandle = HiiAddPackages (
                                     &gIp6ConfigNvDataGuid,
                                     CallbackInfo->ChildHandle,
                                     Ip6DxeStrings,
                                     Ip6ConfigBin,
                                     NULL
                                     );
  if (CallbackInfo->RegisteredHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  //
  // Append MAC string in the menu help string and tile help string
  //
  Status = NetLibGetMacString (IpSb->Controller, IpSb->Image, &MacString);
  if (!EFI_ERROR (Status)) {
    OldMenuString = HiiGetString (
                      CallbackInfo->RegisteredHandle,
                      STRING_TOKEN (STR_IP6_CONFIG_FORM_HELP),
                      NULL
                      );
    if (OldMenuString != NULL) {
      UnicodeSPrint (MenuString, 128, L"%s (MAC:%s)", OldMenuString, MacString);
      HiiSetString (
        CallbackInfo->RegisteredHandle,
        STRING_TOKEN (STR_IP6_CONFIG_FORM_HELP),
        MenuString,
        NULL
        );
      UnicodeSPrint (PortString, 128, L"MAC:%s", MacString);
      HiiSetString (
        CallbackInfo->RegisteredHandle,
        STRING_TOKEN (STR_IP6_DEVICE_FORM_HELP),
        PortString,
        NULL
        );

      FreePool (OldMenuString);
    }

    FreePool (MacString);

    InitializeListHead (&Instance->Ip6NvData.ManualAddress);
    InitializeListHead (&Instance->Ip6NvData.GatewayAddress);
    InitializeListHead (&Instance->Ip6NvData.DnsAddress);

    return EFI_SUCCESS;
  }

Error:
  Ip6ConfigFormUnload (Instance);
  return Status;
}

/**
  Uninstall the HII Config Access protocol for network devices and free up the resources.

  @param[in, out]  Instance      The IP6_CONFIG_INSTANCE to unload a form.

**/
VOID
Ip6ConfigFormUnload (
  IN OUT IP6_CONFIG_INSTANCE  *Instance
  )
{
  IP6_SERVICE             *IpSb;
  IP6_FORM_CALLBACK_INFO  *CallbackInfo;
  IP6_CONFIG_NVDATA       *Ip6NvData;

  IpSb = IP6_SERVICE_FROM_IP6_CONFIG_INSTANCE (Instance);
  ASSERT (IpSb != NULL);

  CallbackInfo = &Instance->CallbackInfo;

  if (CallbackInfo->ChildHandle != NULL) {
    //
    // Close the child handle
    //
    gBS->CloseProtocol (
           IpSb->Controller,
           &gEfiManagedNetworkServiceBindingProtocolGuid,
           IpSb->Image,
           CallbackInfo->ChildHandle
           );
    //
    // Uninstall EFI_HII_CONFIG_ACCESS_PROTOCOL
    //
    gBS->UninstallMultipleProtocolInterfaces (
           CallbackInfo->ChildHandle,
           &gEfiDevicePathProtocolGuid,
           CallbackInfo->HiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &CallbackInfo->HiiConfigAccess,
           NULL
           );
  }

  if (CallbackInfo->HiiVendorDevicePath != NULL) {
    FreePool (CallbackInfo->HiiVendorDevicePath);
  }

  if (CallbackInfo->RegisteredHandle != NULL) {
    //
    // Remove HII package list
    //
    HiiRemovePackages (CallbackInfo->RegisteredHandle);
  }

  Ip6NvData = &Instance->Ip6NvData;

  Ip6FreeAddressInfoList (&Ip6NvData->ManualAddress);
  Ip6FreeAddressInfoList (&Ip6NvData->GatewayAddress);
  Ip6FreeAddressInfoList (&Ip6NvData->DnsAddress);

  Ip6NvData->ManualAddressCount  = 0;
  Ip6NvData->GatewayAddressCount = 0;
  Ip6NvData->DnsAddressCount     = 0;
}
