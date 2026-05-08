/** @file
The device manager reference implementation

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DeviceManager.h"

DEVICE_MANAGER_CALLBACK_DATA  gDeviceManagerPrivate = {
  DEVICE_MANAGER_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  {
    DeviceManagerExtractConfig,
    DeviceManagerRouteConfig,
    DeviceManagerCallback
  }
};

#define  MAX_MAC_ADDRESS_NODE_LIST_LEN  10

EFI_GUID  mDeviceManagerGuid = DEVICE_MANAGER_FORMSET_GUID;

//
// Which Mac Address string is select
// it will decide what menu need to show in the NETWORK_DEVICE_FORM_ID form.
//
EFI_STRING  mSelectedMacAddrString;

//
// The Mac Address show in the NETWORK_DEVICE_LIST_FORM_ID
//
MAC_ADDRESS_NODE_LIST  mMacDeviceList;

HII_VENDOR_DEVICE_PATH  mDeviceManagerHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    //
    // {102579A0-3686-466e-ACD8-80C087044F4A}
    //
    { 0x102579a0, 0x3686, 0x466e, { 0xac, 0xd8, 0x80, 0xc0, 0x87, 0x4, 0x4f, 0x4a }
    }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(END_DEVICE_PATH_LENGTH),
      (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

/**
  Extract device path for given HII handle and class guid.

  @param Handle          The HII handle.

  @retval  NULL          Fail to get the device path string.
  @return  PathString    Get the device path string.

**/
CHAR16 *
DmExtractDevicePathFromHiiHandle (
  IN      EFI_HII_HANDLE  Handle
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  DriverHandle;

  ASSERT (Handle != NULL);

  if (Handle == NULL) {
    return NULL;
  }

  Status = gHiiDatabase->GetPackageListHandle (gHiiDatabase, Handle, &DriverHandle);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Get device path string.
  //
  return ConvertDevicePathToText (DevicePathFromHandle (DriverHandle), FALSE, FALSE);
}

/**
  Get the mac address string from the device path.
  if the device path has the vlan, get the vanid also.

  @param MacAddressNode              Device path begin with mac address
  @param PBuffer                     Output string buffer contain mac address.

**/
BOOLEAN
GetMacAddressString (
  IN  MAC_ADDR_DEVICE_PATH  *MacAddressNode,
  OUT CHAR16                **PBuffer
  )
{
  UINTN                     HwAddressSize;
  UINTN                     Index;
  UINT8                     *HwAddress;
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  UINT16                    VlanId;
  CHAR16                    *String;
  UINTN                     BufferLen;

  VlanId = 0;
  String = NULL;
  ASSERT (MacAddressNode != NULL);

  HwAddressSize = sizeof (EFI_MAC_ADDRESS);
  if ((MacAddressNode->IfType == 0x01) || (MacAddressNode->IfType == 0x00)) {
    HwAddressSize = 6;
  }

  //
  // The output format is MAC:XX:XX:XX:...\XXXX
  // The size is the Number size + ":" size + Vlan size(\XXXX) + End
  //
  BufferLen = (4 + 2 * HwAddressSize + (HwAddressSize - 1) + 5 + 1) * sizeof (CHAR16);
  String    = AllocateZeroPool (BufferLen);
  if (String == NULL) {
    return FALSE;
  }

  *PBuffer = String;
  StrCpyS (String, BufferLen / sizeof (CHAR16), L"MAC:");
  String += 4;

  //
  // Convert the MAC address into a unicode string.
  //
  HwAddress = &MacAddressNode->MacAddress.Addr[0];
  for (Index = 0; Index < HwAddressSize; Index++) {
    UnicodeValueToStringS (
      String,
      BufferLen - ((UINTN)String - (UINTN)*PBuffer),
      PREFIX_ZERO | RADIX_HEX,
      *(HwAddress++),
      2
      );
    String += StrnLenS (String, (BufferLen - ((UINTN)String - (UINTN)*PBuffer)) / sizeof (CHAR16));
    if (Index < HwAddressSize - 1) {
      *String++ = L':';
    }
  }

  //
  // If VLAN is configured, it will need extra 5 characters like "\0005".
  // Plus one unicode character for the null-terminator.
  //
  Node = (EFI_DEVICE_PATH_PROTOCOL  *)MacAddressNode;
  while (!IsDevicePathEnd (Node)) {
    if ((Node->Type == MESSAGING_DEVICE_PATH) && (Node->SubType == MSG_VLAN_DP)) {
      VlanId = ((VLAN_DEVICE_PATH *)Node)->VlanId;
    }

    Node = NextDevicePathNode (Node);
  }

  if (VlanId != 0) {
    *String++ = L'\\';
    UnicodeValueToStringS (
      String,
      BufferLen - ((UINTN)String - (UINTN)*PBuffer),
      PREFIX_ZERO | RADIX_HEX,
      VlanId,
      4
      );
    String += StrnLenS (String, (BufferLen - ((UINTN)String - (UINTN)*PBuffer)) / sizeof (CHAR16));
  }

  //
  // Null terminate the Unicode string
  //
  *String = L'\0';

  return TRUE;
}

/**
  Save question id and prompt id to the mac device list.
  If the same mac address has saved yet, no need to add more.

  @param MacAddrString               Mac address string.

  @retval  EFI_SUCCESS               Add the item is successful.
  @return  Other values if failed to Add the item.
**/
BOOLEAN
AddIdToMacDeviceList (
  IN  EFI_STRING  MacAddrString
  )
{
  MENU_INFO_ITEM  *TempDeviceList;
  UINTN           Index;
  EFI_STRING      StoredString;
  EFI_STRING_ID   PromptId;
  EFI_HII_HANDLE  HiiHandle;

  HiiHandle      =   gDeviceManagerPrivate.HiiHandle;
  TempDeviceList = NULL;

  for (Index = 0; Index < mMacDeviceList.CurListLen; Index++) {
    StoredString = HiiGetString (HiiHandle, mMacDeviceList.NodeList[Index].PromptId, NULL);
    if (StoredString == NULL) {
      return FALSE;
    }

    //
    // Already has save the same mac address to the list.
    //
    if (StrCmp (MacAddrString, StoredString) == 0) {
      return FALSE;
    }
  }

  PromptId = HiiSetString (HiiHandle, 0, MacAddrString, NULL);
  //
  // If not in the list, save it.
  //
  if (mMacDeviceList.MaxListLen > mMacDeviceList.CurListLen + 1) {
    mMacDeviceList.NodeList[mMacDeviceList.CurListLen].PromptId   = PromptId;
    mMacDeviceList.NodeList[mMacDeviceList.CurListLen].QuestionId = (EFI_QUESTION_ID)(mMacDeviceList.CurListLen + NETWORK_DEVICE_LIST_KEY_OFFSET);
  } else {
    mMacDeviceList.MaxListLen += MAX_MAC_ADDRESS_NODE_LIST_LEN;
    if (mMacDeviceList.CurListLen != 0) {
      TempDeviceList = ReallocatePool (
                         sizeof (MENU_INFO_ITEM) * mMacDeviceList.CurListLen,
                         sizeof (MENU_INFO_ITEM) * mMacDeviceList.MaxListLen,
                         mMacDeviceList.NodeList
                         );
    } else {
      TempDeviceList = (MENU_INFO_ITEM *)AllocatePool (sizeof (MENU_INFO_ITEM) * mMacDeviceList.MaxListLen);
    }

    if (TempDeviceList == NULL) {
      return FALSE;
    }

    TempDeviceList[mMacDeviceList.CurListLen].PromptId   = PromptId;
    TempDeviceList[mMacDeviceList.CurListLen].QuestionId = (EFI_QUESTION_ID)(mMacDeviceList.CurListLen + NETWORK_DEVICE_LIST_KEY_OFFSET);

    mMacDeviceList.NodeList = TempDeviceList;
  }

  mMacDeviceList.CurListLen++;

  return TRUE;
}

/**
  Check the devcie path, try to find whether it has mac address path.

  In this function, first need to check whether this path has mac address path.
  second, when the mac address device path has find, also need to deicide whether
  need to add this mac address relate info to the menu.

  @param    *Node            Input device which need to be check.
  @param    NextShowFormId   FormId Which need to be show.
  @param    *NeedAddItem     Whether need to add the menu in the network device list.

  @retval  TRUE              Has mac address device path.
  @retval  FALSE             NOT Has mac address device path.

**/
BOOLEAN
IsMacAddressDevicePath (
  IN  VOID        *Node,
  IN EFI_FORM_ID  NextShowFormId,
  OUT BOOLEAN     *NeedAddItem
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  CHAR16                    *Buffer;
  BOOLEAN                   ReturnVal;

  ASSERT (Node != NULL);
  *NeedAddItem = FALSE;
  ReturnVal    = FALSE;
  Buffer       = NULL;

  DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)Node;

  //
  // find the partition device path node
  //
  while (!IsDevicePathEnd (DevicePath)) {
    if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
        (DevicePathSubType (DevicePath) == MSG_MAC_ADDR_DP))
    {
      ReturnVal = TRUE;

      if (DEVICE_MANAGER_FORM_ID == NextShowFormId) {
        *NeedAddItem = TRUE;
        break;
      }

      if (!GetMacAddressString ((MAC_ADDR_DEVICE_PATH *)DevicePath, &Buffer)) {
        break;
      }

      if (NETWORK_DEVICE_FORM_ID == NextShowFormId) {
        if (StrCmp (Buffer, mSelectedMacAddrString) == 0) {
          *NeedAddItem = TRUE;
        }

        break;
      }

      if (NETWORK_DEVICE_LIST_FORM_ID == NextShowFormId) {
        //
        // Same handle may has two network child handle, so the questionid
        // has the offset of SAME_HANDLE_KEY_OFFSET.
        //
        if (AddIdToMacDeviceList (Buffer)) {
          *NeedAddItem = TRUE;
        }

        break;
      }
    }

    DevicePath = NextDevicePathNode (DevicePath);
  }

  if (Buffer != NULL) {
    FreePool (Buffer);
  }

  return ReturnVal;
}

/**
  Check to see if the device path is for the network device.

  @param Handle          The HII handle which include the mac address device path.
  @param NextShowFormId  The FormId of the form which will be show next time.
  @param ItemCount       The new add Mac address item count.

  @retval  TRUE          Need to add new item in the menu.
  @return  FALSE         Do not need to add the menu about the network.

**/
BOOLEAN
IsNeedAddNetworkMenu (
  IN      EFI_HII_HANDLE  Handle,
  IN      EFI_FORM_ID     NextShowFormId,
  OUT     UINTN           *ItemCount
  )
{
  EFI_STATUS                           Status;
  UINTN                                EntryCount;
  UINTN                                Index;
  EFI_HANDLE                           DriverHandle;
  EFI_HANDLE                           ControllerHandle;
  EFI_DEVICE_PATH_PROTOCOL             *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL             *TmpDevicePath;
  EFI_DEVICE_PATH_PROTOCOL             *ChildDevicePath;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  *OpenInfoBuffer;
  BOOLEAN                              IsNeedAdd;

  IsNeedAdd      = FALSE;
  OpenInfoBuffer = NULL;
  if ((Handle == NULL) || (ItemCount == NULL)) {
    return FALSE;
  }

  *ItemCount = 0;

  Status = gHiiDatabase->GetPackageListHandle (gHiiDatabase, Handle, &DriverHandle);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // Get the device path by the got Driver handle .
  //
  Status = gBS->HandleProtocol (DriverHandle, &gEfiDevicePathProtocolGuid, (VOID **)&DevicePath);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  TmpDevicePath = DevicePath;

  //
  // Check whether this device path include mac address device path.
  // If this path has mac address path, get the value whether need
  // add this info to the menu and return.
  // Else check more about the child handle devcie path.
  //
  if (IsMacAddressDevicePath (TmpDevicePath, NextShowFormId, &IsNeedAdd)) {
    if ((NETWORK_DEVICE_LIST_FORM_ID == NextShowFormId) && IsNeedAdd) {
      (*ItemCount) = 1;
    }

    return IsNeedAdd;
  }

  //
  // Search whether this path is the controller path, not he child handle path.
  // And the child handle has the network devcie connected.
  //
  TmpDevicePath = DevicePath;
  Status        = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &TmpDevicePath, &ControllerHandle);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (!IsDevicePathEnd (TmpDevicePath)) {
    return FALSE;
  }

  //
  // Retrieve the list of agents that are consuming the specific protocol
  // on ControllerHandle.
  // The buffer point by OpenInfoBuffer need be free at this function.
  //
  Status = gBS->OpenProtocolInformation (
                  ControllerHandle,
                  &gEfiPciIoProtocolGuid,
                  &OpenInfoBuffer,
                  &EntryCount
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // Inspect if ChildHandle is one of the agents.
  //
  Status = EFI_UNSUPPORTED;
  for (Index = 0; Index < EntryCount; Index++) {
    //
    // Query all the children created by the controller handle's driver
    //
    if ((OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
      Status = gBS->OpenProtocol (
                      OpenInfoBuffer[Index].ControllerHandle,
                      &gEfiDevicePathProtocolGuid,
                      (VOID **)&ChildDevicePath,
                      NULL,
                      NULL,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }

      //
      // Check whether this device path include mac address device path.
      //
      if (!IsMacAddressDevicePath (ChildDevicePath, NextShowFormId, &IsNeedAdd)) {
        //
        // If this path not has mac address path, check the other.
        //
        continue;
      } else {
        //
        // If need to update the NETWORK_DEVICE_LIST_FORM, try to get more.
        //
        if ((NETWORK_DEVICE_LIST_FORM_ID == NextShowFormId)) {
          if (IsNeedAdd) {
            (*ItemCount) += 1;
          }

          continue;
        } else {
          //
          // If need to update other form, return whether need to add to the menu.
          //
          goto Done;
        }
      }
    }
  }

Done:
  if (OpenInfoBuffer != NULL) {
    FreePool (OpenInfoBuffer);
  }

  return IsNeedAdd;
}

/**
  Dynamic create Hii information for Device Manager.

  @param   NextShowFormId     The FormId which need to be show.

**/
VOID
CreateDeviceManagerForm (
  IN EFI_FORM_ID  NextShowFormId
  )
{
  UINTN               Index;
  EFI_STRING          String;
  EFI_STRING_ID       Token;
  EFI_STRING_ID       TokenHelp;
  EFI_HII_HANDLE      *HiiHandles;
  EFI_HII_HANDLE      HiiHandle;
  EFI_GUID            FormSetGuid;
  VOID                *StartOpCodeHandle;
  VOID                *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL  *StartLabel;
  EFI_IFR_GUID_LABEL  *EndLabel;
  BOOLEAN             AddNetworkMenu;
  UINTN               AddItemCount;
  UINTN               NewStringLen;
  EFI_STRING          NewStringTitle;
  CHAR16              *DevicePathStr;
  EFI_STRING_ID       DevicePathId;
  EFI_IFR_FORM_SET    *Buffer;
  UINTN               BufferSize;
  UINT8               ClassGuidNum;
  EFI_GUID            *ClassGuid;
  UINTN               TempSize;
  UINT8               *Ptr;
  EFI_STATUS          Status;

  TempSize   = 0;
  BufferSize = 0;
  Buffer     = NULL;

  HiiHandle      = gDeviceManagerPrivate.HiiHandle;
  AddNetworkMenu = FALSE;
  AddItemCount   = 0;
  //
  // If need show the Network device list form, clear the old save list first.
  //
  if ((NextShowFormId == NETWORK_DEVICE_LIST_FORM_ID) && (mMacDeviceList.CurListLen > 0)) {
    mMacDeviceList.CurListLen = 0;
  }

  //
  // Update the network device form titile.
  //
  if (NextShowFormId == NETWORK_DEVICE_FORM_ID) {
    String = HiiGetString (HiiHandle, STRING_TOKEN (STR_FORM_NETWORK_DEVICE_TITLE_HEAD), NULL);
    if (String == NULL) {
      return;
    }

    NewStringLen   = StrLen (mSelectedMacAddrString) * 2;
    NewStringLen  += (StrLen (String) + 2) * 2;
    NewStringTitle = AllocatePool (NewStringLen);
    UnicodeSPrint (NewStringTitle, NewStringLen, L"%s %s", String, mSelectedMacAddrString);
    HiiSetString (HiiHandle, STRING_TOKEN (STR_FORM_NETWORK_DEVICE_TITLE), NewStringTitle, NULL);
    FreePool (String);
    FreePool (NewStringTitle);
  }

  //
  // Allocate space for creation of UpdateData Buffer
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel               = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  //
  // According to the next show Form id(mNextShowFormId) to decide which form need to update.
  //
  StartLabel->Number = (UINT16)(LABEL_FORM_ID_OFFSET + NextShowFormId);

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel               = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  //
  // Get all the Hii handles
  //
  HiiHandles = HiiGetHiiHandles (NULL);
  if (HiiHandles == NULL) {
    ASSERT (HiiHandles != NULL);
    return;
  }

  //
  // Search for formset of each class type
  //
  for (Index = 0; HiiHandles[Index] != NULL; Index++) {
    Status = HiiGetFormSetFromHiiHandle (HiiHandles[Index], &Buffer, &BufferSize);
    if (EFI_ERROR (Status)) {
      continue;
    }

    Ptr = (UINT8 *)Buffer;
    while (TempSize < BufferSize) {
      TempSize += ((EFI_IFR_OP_HEADER *)Ptr)->Length;
      if (((EFI_IFR_OP_HEADER *)Ptr)->Length <= OFFSET_OF (EFI_IFR_FORM_SET, Flags)) {
        Ptr += ((EFI_IFR_OP_HEADER *)Ptr)->Length;
        continue;
      }

      ClassGuidNum = (UINT8)(((EFI_IFR_FORM_SET *)Ptr)->Flags & 0x3);
      ClassGuid    = (EFI_GUID *)(VOID *)(Ptr + sizeof (EFI_IFR_FORM_SET));
      while (ClassGuidNum-- > 0) {
        if (CompareGuid (&gEfiHiiPlatformSetupFormsetGuid, ClassGuid) == 0) {
          ClassGuid++;
          continue;
        }

        String = HiiGetString (HiiHandles[Index], ((EFI_IFR_FORM_SET *)Ptr)->FormSetTitle, NULL);
        if (String == NULL) {
          String = HiiGetString (HiiHandle, STRING_TOKEN (STR_MISSING_STRING), NULL);
          ASSERT (String != NULL);
        }

        Token = HiiSetString (HiiHandle, 0, String, NULL);
        FreePool (String);

        String = HiiGetString (HiiHandles[Index], ((EFI_IFR_FORM_SET *)Ptr)->Help, NULL);
        if (String == NULL) {
          String = HiiGetString (HiiHandle, STRING_TOKEN (STR_MISSING_STRING), NULL);
          ASSERT (String != NULL);
        }

        TokenHelp = HiiSetString (HiiHandle, 0, String, NULL);
        FreePool (String);

        CopyMem (&FormSetGuid, &((EFI_IFR_FORM_SET *)Ptr)->Guid, sizeof (EFI_GUID));

        //
        // Network device process
        //
        if (IsNeedAddNetworkMenu (HiiHandles[Index], NextShowFormId, &AddItemCount)) {
          if (NextShowFormId == DEVICE_MANAGER_FORM_ID) {
            //
            // Only show one menu item "Network Config" in the device manger form.
            //
            if (!AddNetworkMenu) {
              AddNetworkMenu = TRUE;
              HiiCreateGotoOpCode (
                StartOpCodeHandle,
                NETWORK_DEVICE_LIST_FORM_ID,
                STRING_TOKEN (STR_FORM_NETWORK_DEVICE_LIST_TITLE),
                STRING_TOKEN (STR_FORM_NETWORK_DEVICE_LIST_HELP),
                EFI_IFR_FLAG_CALLBACK,
                (EFI_QUESTION_ID)QUESTION_NETWORK_DEVICE_ID
                );
            }
          } else if (NextShowFormId == NETWORK_DEVICE_LIST_FORM_ID) {
            //
            // In network device list form, same mac address device only show one menu.
            //
            while (AddItemCount > 0) {
              HiiCreateGotoOpCode (
                StartOpCodeHandle,
                NETWORK_DEVICE_FORM_ID,
                mMacDeviceList.NodeList[mMacDeviceList.CurListLen - AddItemCount].PromptId,
                STRING_TOKEN (STR_NETWORK_DEVICE_HELP),
                EFI_IFR_FLAG_CALLBACK,
                mMacDeviceList.NodeList[mMacDeviceList.CurListLen - AddItemCount].QuestionId
                );
              AddItemCount -= 1;
            }
          } else if (NextShowFormId == NETWORK_DEVICE_FORM_ID) {
            //
            // In network device form, only the selected mac address device need to be show.
            //
            DevicePathStr = DmExtractDevicePathFromHiiHandle (HiiHandles[Index]);
            DevicePathId  = 0;
            if (DevicePathStr != NULL) {
              DevicePathId =  HiiSetString (HiiHandle, 0, DevicePathStr, NULL);
              FreePool (DevicePathStr);
            }

            HiiCreateGotoExOpCode (
              StartOpCodeHandle,
              0,
              Token,
              TokenHelp,
              0,
              (EFI_QUESTION_ID)(Index + DEVICE_KEY_OFFSET),
              0,
              &FormSetGuid,
              DevicePathId
              );
          }
        } else {
          //
          // Not network device process, only need to show at device manger form.
          //
          if (NextShowFormId == DEVICE_MANAGER_FORM_ID) {
            DevicePathStr = DmExtractDevicePathFromHiiHandle (HiiHandles[Index]);
            DevicePathId  = 0;
            if (DevicePathStr != NULL) {
              DevicePathId =  HiiSetString (HiiHandle, 0, DevicePathStr, NULL);
              FreePool (DevicePathStr);
            }

            HiiCreateGotoExOpCode (
              StartOpCodeHandle,
              0,
              Token,
              TokenHelp,
              0,
              (EFI_QUESTION_ID)(Index + DEVICE_KEY_OFFSET),
              0,
              &FormSetGuid,
              DevicePathId
              );
          }
        }

        break;
      }

      Ptr += ((EFI_IFR_OP_HEADER *)Ptr)->Length;
    }

    FreePool (Buffer);
    Buffer     = NULL;
    TempSize   = 0;
    BufferSize = 0;
  }

  HiiUpdateForm (
    HiiHandle,
    &mDeviceManagerGuid,
    NextShowFormId,
    StartOpCodeHandle,
    EndOpCodeHandle
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
  FreePool (HiiHandles);
}

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Request         A null-terminated Unicode string in <ConfigRequest> format.
  @param Progress        On return, points to a character in the Request string.
                         Points to the string's null terminator if request was successful.
                         Points to the most recent '&' before the first failing name/value
                         pair (or the beginning of the string if the failure is in the
                         first name/value pair) if the request was not successful.
  @param Results         A null-terminated Unicode string in <ConfigAltResp> format which
                         has all values filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval  EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
DeviceManagerExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT EFI_STRING                            *Progress,
  OUT EFI_STRING                            *Results
  )
{
  if ((Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  return EFI_NOT_FOUND;
}

/**
  This function processes the results of changes in configuration.

  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Configuration   A null-terminated Unicode string in <ConfigResp> format.
  @param Progress        A pointer to a string filled in with the offset of the most
                         recent '&' before the first failing name/value pair (or the
                         beginning of the string if the failure is in the first
                         name/value pair) or the terminating NULL if all was successful.

  @retval  EFI_SUCCESS            The Results is processed successfully.
  @retval  EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
DeviceManagerRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                            *Progress
  )
{
  if ((Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;

  return EFI_NOT_FOUND;
}

/**
  This function is invoked if user selected a interactive opcode from Device Manager's
  Formset. If user set VBIOS, the new value is saved to EFI variable.

  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_INVALID_PARAMETER The setup browser call this function with invalid parameters.

**/
EFI_STATUS
EFIAPI
DeviceManagerCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  EFI_BROWSER_ACTION                    Action,
  IN  EFI_QUESTION_ID                       QuestionId,
  IN  UINT8                                 Type,
  IN  EFI_IFR_TYPE_VALUE                    *Value,
  OUT EFI_BROWSER_ACTION_REQUEST            *ActionRequest
  )
{
  UINTN  CurIndex;

  if (Action == EFI_BROWSER_ACTION_FORM_OPEN) {
    //
    // Means enter the device manager form.
    // Update device manager page when form opens, because options may add or remove.
    //
    if (QuestionId == 0x1212) {
      CreateDeviceManagerForm (DEVICE_MANAGER_FORM_ID);
    }

    return EFI_SUCCESS;
  } else if (Action != EFI_BROWSER_ACTION_CHANGING) {
    //
    // Do nothing for other UEFI Action. Only do call back when data is changed.
    //
    return EFI_UNSUPPORTED;
  }

  if ((Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((QuestionId < MAX_KEY_SECTION_LEN + NETWORK_DEVICE_LIST_KEY_OFFSET) && (QuestionId >= NETWORK_DEVICE_LIST_KEY_OFFSET)) {
    //
    // If user select the mac address, need to record mac address string to support next form show.
    //
    for (CurIndex = 0; CurIndex < mMacDeviceList.CurListLen; CurIndex++) {
      if (mMacDeviceList.NodeList[CurIndex].QuestionId == QuestionId) {
        mSelectedMacAddrString = HiiGetString (gDeviceManagerPrivate.HiiHandle, mMacDeviceList.NodeList[CurIndex].PromptId, NULL);
      }
    }

    CreateDeviceManagerForm (NETWORK_DEVICE_FORM_ID);
  } else if (QuestionId == QUESTION_NETWORK_DEVICE_ID) {
    CreateDeviceManagerForm (NETWORK_DEVICE_LIST_FORM_ID);
  }

  return EFI_SUCCESS;
}

/**
  Install Boot Manager Menu driver.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCEESS  Install Boot manager menu success.
  @retval  Other        Return error status.

**/
EFI_STATUS
EFIAPI
DeviceManagerUiLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  gDeviceManagerPrivate.DriverHandle = NULL;
  Status                             = gBS->InstallMultipleProtocolInterfaces (
                                              &gDeviceManagerPrivate.DriverHandle,
                                              &gEfiDevicePathProtocolGuid,
                                              &mDeviceManagerHiiVendorDevicePath,
                                              &gEfiHiiConfigAccessProtocolGuid,
                                              &gDeviceManagerPrivate.ConfigAccess,
                                              NULL
                                              );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data.
  //
  gDeviceManagerPrivate.HiiHandle = HiiAddPackages (
                                      &mDeviceManagerGuid,
                                      gDeviceManagerPrivate.DriverHandle,
                                      DeviceManagerVfrBin,
                                      DeviceManagerUiLibStrings,
                                      NULL
                                      );
  ASSERT (gDeviceManagerPrivate.HiiHandle != NULL);

  //
  // The device manager form contains a page listing all the network
  // controllers in the system. This list can only be populated if all
  // handles have been connected, so do it here.
  //
  EfiBootManagerConnectAll ();

  return EFI_SUCCESS;
}

/**
  Unloads the application and its installed protocol.

  @param  ImageHandle     Handle that identifies the image to be unloaded.
  @param  SystemTable     The system table.

  @retval EFI_SUCCESS           The image has been unloaded.
**/
EFI_STATUS
EFIAPI
DeviceManagerUiLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  gDeviceManagerPrivate.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mDeviceManagerHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gDeviceManagerPrivate.ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  HiiRemovePackages (gDeviceManagerPrivate.HiiHandle);

  return EFI_SUCCESS;
}
