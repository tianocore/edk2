/** @file
  The platform device manager reference implementation

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DeviceManager.h"

DEVICE_MANAGER_CALLBACK_DATA  gDeviceManagerPrivate = {
  DEVICE_MANAGER_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  {
    FakeExtractConfig,
    FakeRouteConfig,
    DeviceManagerCallback
  }
};

#define  MAX_MAC_ADDRESS_NODE_LIST_LEN    10

EFI_GUID mDeviceManagerGuid = DEVICE_MANAGER_FORMSET_GUID;

//
// Which Mac Address string is select
// it will decide what menu need to show in the NETWORK_DEVICE_FORM_ID form.
//
EFI_STRING  mSelectedMacAddrString;

//
// The Mac Address show in the NETWORK_DEVICE_LIST_FORM_ID
//
MAC_ADDRESS_NODE_LIST  mMacDeviceList;

DEVICE_MANAGER_MENU_ITEM  mDeviceManagerMenuItemTable[] = {
  { STRING_TOKEN (STR_DISK_DEVICE),     EFI_DISK_DEVICE_CLASS },
  { STRING_TOKEN (STR_VIDEO_DEVICE),    EFI_VIDEO_DEVICE_CLASS },
  { STRING_TOKEN (STR_NETWORK_DEVICE),  EFI_NETWORK_DEVICE_CLASS },
  { STRING_TOKEN (STR_INPUT_DEVICE),    EFI_INPUT_DEVICE_CLASS },
  { STRING_TOKEN (STR_ON_BOARD_DEVICE), EFI_ON_BOARD_DEVICE_CLASS },
  { STRING_TOKEN (STR_OTHER_DEVICE),    EFI_OTHER_DEVICE_CLASS }
};

HII_VENDOR_DEVICE_PATH  mDeviceManagerHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    //
    // {102579A0-3686-466e-ACD8-80C087044F4A}
    //
    { 0x102579a0, 0x3686, 0x466e, { 0xac, 0xd8, 0x80, 0xc0, 0x87, 0x4, 0x4f, 0x4a } }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { 
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

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
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  UINTN CurIndex;
  
  if (Action != EFI_BROWSER_ACTION_CHANGING) {
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
    for (CurIndex = 0; CurIndex < mMacDeviceList.CurListLen; CurIndex ++) {
      if (mMacDeviceList.NodeList[CurIndex].QuestionId == QuestionId) {
         mSelectedMacAddrString = HiiGetString (gDeviceManagerPrivate.HiiHandle, mMacDeviceList.NodeList[CurIndex].PromptId, NULL);
      }
    }
    CreateDeviceManagerForm(NETWORK_DEVICE_FORM_ID);
  } else if(QuestionId == QUESTION_NETWORK_DEVICE_ID){
    CreateDeviceManagerForm(NETWORK_DEVICE_LIST_FORM_ID);
  }

  return EFI_SUCCESS;
}

/**
  Get the mac address string from the device path.
  if the device path has the vlan, get the vanid also.
  
  @param MacAddressNode              Device path begin with mac address 
  @param PBuffer                     Output string buffer contain mac address.

**/
BOOLEAN 
GetMacAddressString(
  IN  MAC_ADDR_DEVICE_PATH   *MacAddressNode,
  OUT CHAR16                 **PBuffer
  )
{
  UINTN                 HwAddressSize;
  UINTN                 Index;
  UINT8                 *HwAddress;
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  UINT16                VlanId;
  CHAR16                *String;
  UINTN                 BufferLen;

  VlanId = 0;
  String = NULL;
  ASSERT(MacAddressNode != NULL);

  HwAddressSize = sizeof (EFI_MAC_ADDRESS);
  if (MacAddressNode->IfType == 0x01 || MacAddressNode->IfType == 0x00) {
    HwAddressSize = 6;
  }

  //
  // The output format is MAC:XX:XX:XX:...\XXXX
  // The size is the Number size + ":" size + Vlan size(\XXXX) + End
  //
  BufferLen = (4 + 2 * HwAddressSize + (HwAddressSize - 1) + 5 + 1) * sizeof (CHAR16);
  String = AllocateZeroPool (BufferLen);
  if (String == NULL) {
    return FALSE;
  }

  *PBuffer = String;
  StrCpyS(String, BufferLen / sizeof (CHAR16), L"MAC:");
  String += 4;
  
  //
  // Convert the MAC address into a unicode string.
  //
  HwAddress = &MacAddressNode->MacAddress.Addr[0];
  for (Index = 0; Index < HwAddressSize; Index++) {
    String += UnicodeValueToString (String, PREFIX_ZERO | RADIX_HEX, *(HwAddress++), 2);
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
    if (Node->Type == MESSAGING_DEVICE_PATH && Node->SubType == MSG_VLAN_DP) {
      VlanId = ((VLAN_DEVICE_PATH *) Node)->VlanId;
    }
    Node = NextDevicePathNode (Node);
  }

  if (VlanId != 0) {
    *String++ = L'\\';
    String += UnicodeValueToString (String, PREFIX_ZERO | RADIX_HEX, VlanId, 4);
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
  IN  EFI_STRING        MacAddrString
  )
{
  MENU_INFO_ITEM *TempDeviceList;
  UINTN          Index;
  EFI_STRING     StoredString;
  EFI_STRING_ID  PromptId;
  EFI_HII_HANDLE HiiHandle;

  HiiHandle =   gDeviceManagerPrivate.HiiHandle;
  TempDeviceList = NULL;

  for (Index = 0; Index < mMacDeviceList.CurListLen; Index ++) {
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

  PromptId = HiiSetString(HiiHandle, 0, MacAddrString, NULL);
  //
  // If not in the list, save it.
  //
  if (mMacDeviceList.MaxListLen > mMacDeviceList.CurListLen + 1) {
    mMacDeviceList.NodeList[mMacDeviceList.CurListLen].PromptId = PromptId;
    mMacDeviceList.NodeList[mMacDeviceList.CurListLen].QuestionId = (EFI_QUESTION_ID) (mMacDeviceList.CurListLen + NETWORK_DEVICE_LIST_KEY_OFFSET);
  } else {
    mMacDeviceList.MaxListLen += MAX_MAC_ADDRESS_NODE_LIST_LEN;
    if (mMacDeviceList.CurListLen != 0) {
      TempDeviceList = (MENU_INFO_ITEM *)AllocateCopyPool (sizeof (MENU_INFO_ITEM) * mMacDeviceList.MaxListLen, (VOID *)mMacDeviceList.NodeList);
    } else {
      TempDeviceList = (MENU_INFO_ITEM *)AllocatePool (sizeof (MENU_INFO_ITEM) * mMacDeviceList.MaxListLen);
    }
    
    if (TempDeviceList == NULL) {
      return FALSE;
    }
    TempDeviceList[mMacDeviceList.CurListLen].PromptId = PromptId;  
    TempDeviceList[mMacDeviceList.CurListLen].QuestionId = (EFI_QUESTION_ID) (mMacDeviceList.CurListLen + NETWORK_DEVICE_LIST_KEY_OFFSET);
    
    if (mMacDeviceList.CurListLen > 0) {
      FreePool(mMacDeviceList.NodeList);
    }
    
    mMacDeviceList.NodeList = TempDeviceList;
  }
  mMacDeviceList.CurListLen ++;

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

  @retval  TRUE             Has mac address device path.
  @retval  FALSE            NOT Has mac address device path.  

**/
BOOLEAN
IsMacAddressDevicePath (
  IN  VOID          *Node,
  IN EFI_FORM_ID    NextShowFormId,
  OUT BOOLEAN       *NeedAddItem
  )
{
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  CHAR16                     *Buffer;
  BOOLEAN                    ReturnVal;
  
  ASSERT (Node != NULL);
  *NeedAddItem = FALSE;
  ReturnVal    = FALSE;
  Buffer    = NULL;

  DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) Node;

  //
  // find the partition device path node
  //
  while (!IsDevicePathEnd (DevicePath)) {
    if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
       (DevicePathSubType (DevicePath) == MSG_MAC_ADDR_DP)) {
      ReturnVal = TRUE;
      
      if (DEVICE_MANAGER_FORM_ID == NextShowFormId) {
        *NeedAddItem = TRUE;
        break;
      } 
      
      if (!GetMacAddressString((MAC_ADDR_DEVICE_PATH*)DevicePath, &Buffer)) {
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
  IN      EFI_HII_HANDLE      Handle,
  IN      EFI_FORM_ID         NextShowFormId,
  OUT     UINTN               *ItemCount
  )
{
  EFI_STATUS     Status;
  UINTN          EntryCount;
  UINTN          Index;  
  EFI_HII_HANDLE HiiDeviceManagerHandle;
  EFI_HANDLE     DriverHandle;
  EFI_HANDLE     ControllerHandle;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL   *TmpDevicePath;
  EFI_DEVICE_PATH_PROTOCOL   *ChildDevicePath;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY   *OpenInfoBuffer;
  BOOLEAN        IsNeedAdd;

  HiiDeviceManagerHandle = gDeviceManagerPrivate.HiiHandle;
  IsNeedAdd  = FALSE;
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
  Status = gBS->HandleProtocol (DriverHandle, &gEfiDevicePathProtocolGuid, (VOID **) &DevicePath);
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
  if (IsMacAddressDevicePath(TmpDevicePath, NextShowFormId,&IsNeedAdd)) {
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
  Status = gBS->LocateDevicePath(&gEfiDevicePathProtocolGuid, &TmpDevicePath, &ControllerHandle);
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
                      (VOID **) &ChildDevicePath,
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
      if (!IsMacAddressDevicePath(ChildDevicePath, NextShowFormId,&IsNeedAdd)) {
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
  This  function  registers  HII  packages  to  HII  database.

**/
VOID
InitializeDeviceManager (
  VOID
)
{
  EFI_STATUS                  Status;

  if (!gConnectAllHappened){
    EfiBootManagerConnectAll();
    gConnectAllHappened = TRUE;
  }

  gDeviceManagerPrivate.DriverHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
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
                  UiAppStrings,
                  NULL
                  );
  ASSERT (gDeviceManagerPrivate.HiiHandle != NULL);
}

/**
  Remove the installed packages from the HII Database. 

**/
VOID
FreeDeviceManager(
  VOID
)
{
  EFI_STATUS                  Status;

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
}

/**
  Dynamic create Hii information for Device Manager.

  @param   NextShowFormId     The FormId which need to be show.

**/
VOID
CreateDeviceManagerForm(
  IN EFI_FORM_ID      NextShowFormId
)
{
  UINTN                       Index;
  EFI_STRING                  String;
  EFI_STRING_ID               Token;
  EFI_STRING_ID               TokenHelp;
  EFI_HII_HANDLE              *HiiHandles;
  EFI_HII_HANDLE              HiiHandle;
  UINTN                       SkipCount;
  EFI_STRING_ID               FormSetTitle;
  EFI_STRING_ID               FormSetHelp;
  EFI_GUID                    FormSetGuid;
  VOID                        *StartOpCodeHandle;
  VOID                        *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL          *StartLabel;
  EFI_IFR_GUID_LABEL          *EndLabel;
  BOOLEAN                     AddNetworkMenu;
  UINTN                       AddItemCount;
  UINTN                       NewStringLen;
  EFI_STRING                  NewStringTitle;
  CHAR16                      *DevicePathStr;
  EFI_STRING_ID               DevicePathId;

  HiiHandle = gDeviceManagerPrivate.HiiHandle;
  AddNetworkMenu = FALSE;
  AddItemCount = 0;

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
    String = HiiGetString (HiiHandle, STRING_TOKEN (STR_FORM_NETWORK_DEVICE_TITLE), NULL);
    NewStringLen = StrLen(mSelectedMacAddrString) * 2;
    NewStringLen += (StrLen(String) + 2) * 2;
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
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  //
  // According to the next show Form id(mNextShowFormId) to decide which form need to update.
  //
  StartLabel->Number       = (UINT16) (LABEL_FORM_ID_OFFSET + NextShowFormId);

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  //
  // Get all the Hii handles
  //
  HiiHandles = HiiGetHiiHandles (NULL);
  ASSERT (HiiHandles != NULL);

  //
  // Search for formset of each class type
  //
  SkipCount = 0;
  for (Index = 0; HiiHandles[Index] != NULL; Index++) {
    //
    //  The QuestionId in the form which will call the driver form has this asssumption.
    //  QuestionId = Handle Index + NETWORK_DEVICE_LIST_KEY_OFFSET;
    //  Different QuestionId at least has the section of NETWORK_DEVICE_LIST_KEY_OFFSET.
    //
    ASSERT(Index < MAX_KEY_SECTION_LEN);
    if (!ExtractDisplayedHiiFormFromHiiHandle (HiiHandles[Index], &gEfiHiiPlatformSetupFormsetGuid, SkipCount, &FormSetTitle, &FormSetHelp, &FormSetGuid)) {
      SkipCount = 0;
      continue;
    }

    String = HiiGetString (HiiHandles[Index], FormSetTitle, NULL);
    if (String == NULL) {
      String = HiiGetString (HiiHandle, STR_MISSING_STRING, NULL);
      ASSERT (String != NULL);
    }
    Token = HiiSetString (HiiHandle, 0, String, NULL);
    FreePool (String);

    String = HiiGetString (HiiHandles[Index], FormSetHelp, NULL);
    if (String == NULL) {
      String = HiiGetString (HiiHandle, STR_MISSING_STRING, NULL);
      ASSERT (String != NULL);
    }
    TokenHelp = HiiSetString (HiiHandle, 0, String, NULL);
    FreePool (String);

    //
    // Network device process
    // 
    if (IsNeedAddNetworkMenu (HiiHandles[Index], NextShowFormId,&AddItemCount)) {
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
            (EFI_QUESTION_ID) QUESTION_NETWORK_DEVICE_ID
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
        DevicePathStr = ExtractDevicePathFromHiiHandle(HiiHandles[Index]);
        DevicePathId  = 0;
        if (DevicePathStr != NULL){
          DevicePathId =  HiiSetString (HiiHandle, 0, DevicePathStr, NULL);
          FreePool(DevicePathStr);
        }
        HiiCreateGotoExOpCode (
          StartOpCodeHandle,
          0,
          Token,
          TokenHelp,
          0,
          (EFI_QUESTION_ID) (Index + DEVICE_KEY_OFFSET),
          0,
          &FormSetGuid,    
          DevicePathId
          );
      }
    } else {
      //
      // 
      // Not network device process, only need to show at device manger form.
      //
      if (NextShowFormId == DEVICE_MANAGER_FORM_ID) {
        DevicePathStr = ExtractDevicePathFromHiiHandle(HiiHandles[Index]);
        DevicePathId  = 0;
        if (DevicePathStr != NULL){
          DevicePathId =  HiiSetString (HiiHandle, 0, DevicePathStr, NULL);
          FreePool(DevicePathStr);
        }
        HiiCreateGotoExOpCode (
          StartOpCodeHandle,
          0,
          Token,
          TokenHelp,
          0,
          (EFI_QUESTION_ID) (Index + DEVICE_KEY_OFFSET),
          0,
          &FormSetGuid,
          DevicePathId
          );
      }
    }
    //
    //One packagelist may has more than one form package,
    //Index-- means keep current HiiHandle and still extract from the packagelist, 
    //SkipCount++ means skip the formset which was found before in the same form package. 
    //
    SkipCount++;
    Index--;
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

