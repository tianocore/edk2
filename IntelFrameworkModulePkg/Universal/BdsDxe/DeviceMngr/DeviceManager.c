/** @file
  The platform device manager reference implementation

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DeviceManager.h"

DEVICE_MANAGER_CALLBACK_DATA  gDeviceManagerPrivate = {
  DEVICE_MANAGER_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  NULL,
  NULL,
  {
    FakeExtractConfig,
    FakeRouteConfig,
    DeviceManagerCallback
  },
  {
    FakeExtractConfig,
    FakeRouteConfig,
    DriverHealthCallback
  }
};

#define  MAX_MAC_ADDRESS_NODE_LIST_LEN    10

//
// Which Mac Address string is select
// it will decide what menu need to show in the NETWORK_DEVICE_FORM_ID form.
//
EFI_STRING  mSelectedMacAddrString;

//
// Which form Id need to be show.
//
EFI_FORM_ID      mNextShowFormId = DEVICE_MANAGER_FORM_ID;

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
    DEVICE_MANAGER_FORMSET_GUID
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

HII_VENDOR_DEVICE_PATH  mDriverHealthHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
          (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    DRIVER_HEALTH_FORMSET_GUID
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
  Formset. The decision by user is saved to gCallbackKey for later processing. If
  user set VBIOS, the new value is saved to EFI variable.

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
    // All other action return unsupported.
    //
    return EFI_UNSUPPORTED;
  }

  if (Value == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  gCallbackKey = QuestionId;
  if ((QuestionId < MAX_KEY_SECTION_LEN + NETWORK_DEVICE_LIST_KEY_OFFSET) && (QuestionId >= NETWORK_DEVICE_LIST_KEY_OFFSET)) {
    //
    // If user select the mac address, need to record mac address string to support next form show.
    //
    for (CurIndex = 0; CurIndex < mMacDeviceList.CurListLen; CurIndex ++) {
      if (mMacDeviceList.NodeList[CurIndex].QuestionId == QuestionId) {
         mSelectedMacAddrString = HiiGetString (gDeviceManagerPrivate.HiiHandle, mMacDeviceList.NodeList[CurIndex].PromptId, NULL);
      }
    }
  }

  return EFI_SUCCESS;
}

/**

  This function registers HII packages to HII database.

  @retval  EFI_SUCCESS           HII packages for the Device Manager were registered successfully.
  @retval  EFI_OUT_OF_RESOURCES  HII packages for the Device Manager failed to be registered.

**/
EFI_STATUS
InitializeDeviceManager (
  VOID
  )
{
  EFI_STATUS                  Status;

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gDeviceManagerPrivate.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mDeviceManagerHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gDeviceManagerPrivate.ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gDeviceManagerPrivate.DriverHealthHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mDriverHealthHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gDeviceManagerPrivate.DriverHealthConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  mMacDeviceList.CurListLen = 0;
  mMacDeviceList.MaxListLen = 0;

  return Status;
}

/**
  Extract the displayed formset for given HII handle and class guid.

  @param Handle          The HII handle.
  @param SetupClassGuid  The class guid specifies which form set will be displayed.
  @param SkipCount       Skip some formsets which has processed before.
  @param FormSetTitle    Formset title string.
  @param FormSetHelp     Formset help string.
  @param FormSetGuid     Return the formset guid for this formset.

  @retval  TRUE          The formset for given HII handle will be displayed.
  @return  FALSE         The formset for given HII handle will not be displayed.

**/
BOOLEAN
ExtractDisplayedHiiFormFromHiiHandle (
  IN      EFI_HII_HANDLE      Handle,
  IN      EFI_GUID            *SetupClassGuid,
  IN      UINTN               SkipCount,
  OUT     EFI_STRING_ID       *FormSetTitle,
  OUT     EFI_STRING_ID       *FormSetHelp,
  OUT     EFI_GUID            **FormSetGuid
  )
{
  EFI_STATUS                   Status;
  UINTN                        BufferSize;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINT8                        *Package;
  UINT8                        *OpCodeData;
  UINT32                       Offset;
  UINT32                       Offset2;
  UINT32                       PackageListLength;
  EFI_HII_PACKAGE_HEADER       PackageHeader;
  EFI_GUID                     *ClassGuid;
  UINT8                        ClassGuidNum;

  ASSERT (Handle != NULL);
  ASSERT (SetupClassGuid != NULL);
  ASSERT (FormSetTitle != NULL);
  ASSERT (FormSetHelp != NULL);

  *FormSetTitle = 0;
  *FormSetHelp  = 0;
  ClassGuidNum  = 0;
  ClassGuid     = NULL;

  //
  // Get HII PackageList
  //
  BufferSize = 0;
  HiiPackageList = NULL;
  Status = gHiiDatabase->ExportPackageLists (gHiiDatabase, Handle, &BufferSize, HiiPackageList);
  //
  // Handle is a invalid handle. Check if Handle is corrupted.
  //
  ASSERT (Status != EFI_NOT_FOUND);
  //
  // The return status should always be EFI_BUFFER_TOO_SMALL as input buffer's size is 0.
  //
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  HiiPackageList = AllocatePool (BufferSize);
  ASSERT (HiiPackageList != NULL);

  Status = gHiiDatabase->ExportPackageLists (gHiiDatabase, Handle, &BufferSize, HiiPackageList);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // Get Form package from this HII package List
  //
  Offset = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  Offset2 = 0;
  PackageListLength = ReadUnaligned32 (&HiiPackageList->PackageLength);

  while (Offset < PackageListLength) {
    Package = ((UINT8 *) HiiPackageList) + Offset;
    CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));

    if (PackageHeader.Type == EFI_HII_PACKAGE_FORMS) {
      //
      // Search FormSet Opcode in this Form Package
      //
      Offset2 = sizeof (EFI_HII_PACKAGE_HEADER);
      while (Offset2 < PackageHeader.Length) {
        OpCodeData = Package + Offset2;
        Offset2 += ((EFI_IFR_OP_HEADER *) OpCodeData)->Length;

        if (((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode == EFI_IFR_FORM_SET_OP) {
          if (SkipCount != 0) {
            SkipCount --;
            continue;
          }

          if (((EFI_IFR_OP_HEADER *) OpCodeData)->Length > OFFSET_OF (EFI_IFR_FORM_SET, Flags)) {
            //
            // Find FormSet OpCode
            //
            ClassGuidNum = (UINT8) (((EFI_IFR_FORM_SET *) OpCodeData)->Flags & 0x3);
            ClassGuid = (EFI_GUID *) (VOID *)(OpCodeData + sizeof (EFI_IFR_FORM_SET));
            while (ClassGuidNum-- > 0) {
              if (CompareGuid (SetupClassGuid, ClassGuid)) {
                CopyMem (FormSetTitle, &((EFI_IFR_FORM_SET *) OpCodeData)->FormSetTitle, sizeof (EFI_STRING_ID));
                CopyMem (FormSetHelp, &((EFI_IFR_FORM_SET *) OpCodeData)->Help, sizeof (EFI_STRING_ID));
                *FormSetGuid = AllocateCopyPool (sizeof (EFI_GUID), &((EFI_IFR_FORM_SET *) OpCodeData)->Guid);
                ASSERT (*FormSetGuid != NULL);
                FreePool (HiiPackageList);
                return TRUE;
              }
              ClassGuid ++;
            }
           } else {
             CopyMem (FormSetTitle, &((EFI_IFR_FORM_SET *) OpCodeData)->FormSetTitle, sizeof (EFI_STRING_ID));
             CopyMem (FormSetHelp, &((EFI_IFR_FORM_SET *) OpCodeData)->Help, sizeof (EFI_STRING_ID));
             *FormSetGuid = AllocateCopyPool (sizeof (EFI_GUID), &((EFI_IFR_FORM_SET *) OpCodeData)->Guid);
             ASSERT (*FormSetGuid != NULL);
             FreePool (HiiPackageList);
             return TRUE;
          }
        }
      }
    }

    //
    // Go to next package
    //
    Offset += PackageHeader.Length;
  }

  FreePool (HiiPackageList);

  return FALSE;
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
    if (Node->Type == MESSAGING_DEVICE_PATH && Node->SubType == MSG_VLAN_DP) {
      VlanId = ((VLAN_DEVICE_PATH *) Node)->VlanId;
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
    TempDeviceList[mMacDeviceList.CurListLen].PromptId = PromptId;
    TempDeviceList[mMacDeviceList.CurListLen].QuestionId = (EFI_QUESTION_ID) (mMacDeviceList.CurListLen + NETWORK_DEVICE_LIST_KEY_OFFSET);

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

  @param    *Node           Input device which need to be check.
  @param    *NeedAddItem    Whether need to add the menu in the network device list.

  @retval  TRUE             Has mac address device path.
  @retval  FALSE            NOT Has mac address device path.

**/
BOOLEAN
IsMacAddressDevicePath (
  IN  VOID    *Node,
  OUT BOOLEAN *NeedAddItem
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

      if (DEVICE_MANAGER_FORM_ID == mNextShowFormId) {
        *NeedAddItem = TRUE;
        break;
      }

      if (!GetMacAddressString((MAC_ADDR_DEVICE_PATH*)DevicePath, &Buffer)) {
        break;
      }

      if (NETWORK_DEVICE_FORM_ID == mNextShowFormId) {
        if (StrCmp (Buffer, mSelectedMacAddrString) == 0) {
          *NeedAddItem = TRUE;
        }
        break;
      }

      if (NETWORK_DEVICE_LIST_FORM_ID == mNextShowFormId) {
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
  @param ItemCount       The new add Mac address item count.

  @retval  TRUE          Need to add new item in the menu.
  @return  FALSE         Do not need to add the menu about the network.

**/
BOOLEAN
IsNeedAddNetworkMenu (
  IN      EFI_HII_HANDLE      Handle,
  OUT     UINTN               *ItemCount
  )
{
  EFI_STATUS     Status;
  UINTN          EntryCount;
  UINTN          Index;
  EFI_HANDLE     DriverHandle;
  EFI_HANDLE     ControllerHandle;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL   *TmpDevicePath;
  EFI_DEVICE_PATH_PROTOCOL   *ChildDevicePath;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY   *OpenInfoBuffer;
  BOOLEAN        IsNeedAdd;

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
  if (IsMacAddressDevicePath(TmpDevicePath, &IsNeedAdd)) {
    if ((NETWORK_DEVICE_LIST_FORM_ID == mNextShowFormId) && IsNeedAdd) {
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
      if (!IsMacAddressDevicePath(ChildDevicePath, &IsNeedAdd)) {
        //
        // If this path not has mac address path, check the other.
        //
        continue;
      } else {
        //
        // If need to update the NETWORK_DEVICE_LIST_FORM, try to get more.
        //
        if ((NETWORK_DEVICE_LIST_FORM_ID == mNextShowFormId)) {
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
  Get HiiHandle total number.

  @param   HiiHandles              The input HiiHandle array.

  @retval  the Hiihandle count.

**/
UINTN
GetHiiHandleCount (
  IN EFI_HII_HANDLE              *HiiHandles
  )
{
  UINTN  Index;

  for (Index = 0; HiiHandles[Index] != NULL; Index++) {
  }

  return Index;
}

/**
  Insert the new HiiHandle + FormsetGuid at the NewPair[InsertOffset].

  @param   HiiHandles              The input HiiHandle array.
  @param   GuidLists               The input form set guid lists.
  @param   ArrayCount              The input array count, new array will be arraycount + 1 size.
  @param   Offset                  The current used HiiHandle's Offset.
  @param   FormSetGuid             The new found formset guid.

**/
VOID
AdjustArrayData (
  IN OUT EFI_HII_HANDLE              **HiiHandles,
  IN OUT EFI_GUID                    ***GuidLists,
  IN     UINTN                       ArrayCount,
  IN     UINTN                       Offset,
  IN     EFI_GUID                    *FormSetGuid
  )
{
  EFI_HII_HANDLE              *NewHiiHandles;
  EFI_GUID                    **NewGuidLists;

  //
  // +2 means include the new HiiHandle and the last empty NULL pointer.
  //
  NewHiiHandles = AllocateZeroPool ((ArrayCount + 2) * sizeof (EFI_HII_HANDLE));
  ASSERT (NewHiiHandles != NULL);

  CopyMem (NewHiiHandles, *HiiHandles, Offset * sizeof (EFI_HII_HANDLE));
  NewHiiHandles[Offset] = NewHiiHandles[Offset - 1];
  CopyMem (NewHiiHandles + Offset + 1, *HiiHandles + Offset, (ArrayCount - Offset) * sizeof (EFI_HII_HANDLE));

  NewGuidLists = AllocateZeroPool ((ArrayCount + 2) * sizeof (EFI_GUID *));
  ASSERT (NewGuidLists != NULL);

  CopyMem (NewGuidLists, *GuidLists, Offset * sizeof (EFI_GUID *));
  NewGuidLists[Offset] = FormSetGuid;

  FreePool (*HiiHandles);
  *HiiHandles = NewHiiHandles;
  FreePool (*GuidLists);
  *GuidLists = NewGuidLists;
}

/**
  Call the browser and display the device manager to allow user
  to configure the platform.

  This function create the dynamic content for device manager. It includes
  section header for all class of devices, one-of opcode to set VBIOS.

  @retval  EFI_SUCCESS             Operation is successful.
  @return  Other values if failed to clean up the dynamic content from HII
           database.

**/
EFI_STATUS
CallDeviceManager (
  VOID
  )
{
  EFI_STATUS                  Status;
  UINTN                       Index;
  EFI_STRING                  String;
  EFI_STRING_ID               Token;
  EFI_STRING_ID               TokenHelp;
  EFI_HII_HANDLE              *HiiHandles;
  EFI_HII_HANDLE              HiiHandle;
  EFI_STRING_ID               FormSetTitle;
  EFI_STRING_ID               FormSetHelp;
  EFI_BROWSER_ACTION_REQUEST  ActionRequest;
  VOID                        *StartOpCodeHandle;
  VOID                        *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL          *StartLabel;
  EFI_IFR_GUID_LABEL          *EndLabel;
  UINTN                       NumHandles;
  EFI_HANDLE                  *DriverHealthHandles;
  BOOLEAN                     AddNetworkMenu;
  UINTN                       AddItemCount;
  UINTN                       NewStringLen;
  EFI_STRING                  NewStringTitle;
  EFI_GUID                    **GuidLists;
  UINTN                       HandleNum;
  UINTN                       SkipCount;
  EFI_GUID                    *FormSetGuid;

  GuidLists     = NULL;
  HiiHandles    = NULL;
  Status        = EFI_SUCCESS;
  gCallbackKey  = 0;
  NumHandles    = 0;
  DriverHealthHandles = NULL;
  AddNetworkMenu = FALSE;
  AddItemCount   = 0;
  SkipCount      = 0;
  FormSetGuid    = NULL;

  //
  // Connect all prior to entering the platform setup menu.
  //
  if (!gConnectAllHappened) {
    BdsLibConnectAllDriversToAllControllers ();
    gConnectAllHappened = TRUE;
  }

  HiiHandle = gDeviceManagerPrivate.HiiHandle;
  if (HiiHandle == NULL) {
    //
    // Publish our HII data.
    //
    HiiHandle = HiiAddPackages (
                  &gDeviceManagerFormSetGuid,
                  gDeviceManagerPrivate.DriverHandle,
                  DeviceManagerVfrBin,
                  BdsDxeStrings,
                  NULL
                  );
    if (HiiHandle == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    gDeviceManagerPrivate.HiiHandle = HiiHandle;
  }

  //
  // If need show the Network device list form, clear the old save list first.
  //
  if ((mNextShowFormId == NETWORK_DEVICE_LIST_FORM_ID) && (mMacDeviceList.CurListLen > 0)) {
    mMacDeviceList.CurListLen = 0;
  }

  //
  // Update the network device form titile.
  //
  if (mNextShowFormId == NETWORK_DEVICE_FORM_ID) {
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
  StartLabel->Number       = (UINT16) (LABEL_FORM_ID_OFFSET + mNextShowFormId);

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

  HandleNum = GetHiiHandleCount (HiiHandles);
  GuidLists = AllocateZeroPool ((HandleNum + 1) * sizeof (EFI_GUID *));
  ASSERT (GuidLists != NULL);

  //
  // Search for formset of each class type
  //
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

    //
    // One HiiHandle has more than one formset can be shown,
    // Insert a new pair of HiiHandle + Guid to the HiiHandles and GuidLists list.
    //
    if (SkipCount > 0) {
      AdjustArrayData (&HiiHandles, &GuidLists, HandleNum, Index + 1, FormSetGuid);
      HandleNum ++;
      Index ++;
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
    if (IsNeedAddNetworkMenu (HiiHandles[Index], &AddItemCount)) {
      if (mNextShowFormId == DEVICE_MANAGER_FORM_ID) {
        //
        // Only show one menu item "Network Config" in the device manger form.
        //
        if (!AddNetworkMenu) {
          AddNetworkMenu = TRUE;
          HiiCreateGotoOpCode (
            StartOpCodeHandle,
            INVALID_FORM_ID,
            STRING_TOKEN (STR_FORM_NETWORK_DEVICE_LIST_TITLE),
            STRING_TOKEN (STR_FORM_NETWORK_DEVICE_LIST_HELP),
            EFI_IFR_FLAG_CALLBACK,
            (EFI_QUESTION_ID) QUESTION_NETWORK_DEVICE_ID
            );
        }
      } else if (mNextShowFormId == NETWORK_DEVICE_LIST_FORM_ID) {
        //
        // In network device list form, same mac address device only show one menu.
        //
        while (AddItemCount > 0) {
            HiiCreateGotoOpCode (
              StartOpCodeHandle,
              INVALID_FORM_ID,
              mMacDeviceList.NodeList[mMacDeviceList.CurListLen - AddItemCount].PromptId,
              STRING_TOKEN (STR_NETWORK_DEVICE_HELP),
              EFI_IFR_FLAG_CALLBACK,
              mMacDeviceList.NodeList[mMacDeviceList.CurListLen - AddItemCount].QuestionId
              );
            AddItemCount -= 1;
          }
      } else if (mNextShowFormId == NETWORK_DEVICE_FORM_ID) {
        //
        // In network device form, only the selected mac address device need to be show.
        //
        HiiCreateGotoOpCode (
          StartOpCodeHandle,
          INVALID_FORM_ID,
          Token,
          TokenHelp,
          EFI_IFR_FLAG_CALLBACK,
          (EFI_QUESTION_ID) (Index + DEVICE_KEY_OFFSET)
          );
      }
    } else {
      //
      //
      // Not network device process, only need to show at device manger form.
      //
      if (mNextShowFormId == DEVICE_MANAGER_FORM_ID) {
        HiiCreateGotoOpCode (
          StartOpCodeHandle,
          INVALID_FORM_ID,
          Token,
          TokenHelp,
          EFI_IFR_FLAG_CALLBACK,
          (EFI_QUESTION_ID) (Index + DEVICE_KEY_OFFSET)
          );
      }
    }

    //
    // Try to find more formset in this HiiHandle.
    //
    SkipCount++;
    Index--;
  }

  Status = gBS->LocateHandleBuffer (
                ByProtocol,
                &gEfiDriverHealthProtocolGuid,
                NULL,
                &NumHandles,
                &DriverHealthHandles
                );

  //
  // If there are no drivers installed driver health protocol, do not create driver health entry in UI
  //
  if (NumHandles != 0) {
    //
    // If driver health protocol is installed, create Driver Health subtitle and entry
    //
    HiiCreateSubTitleOpCode (StartOpCodeHandle, STRING_TOKEN (STR_DM_DRIVER_HEALTH_TITLE), 0, 0, 0);
    HiiCreateGotoOpCode (
      StartOpCodeHandle,
      DRIVER_HEALTH_FORM_ID,
      STRING_TOKEN(STR_DRIVER_HEALTH_ALL_HEALTHY),      // Prompt text
      STRING_TOKEN(STR_DRIVER_HEALTH_STATUS_HELP),      // Help text
      EFI_IFR_FLAG_CALLBACK,
      DEVICE_MANAGER_KEY_DRIVER_HEALTH                  // Question ID
      );

    //
    // Check All Driver health status
    //
    if (!PlaformHealthStatusCheck ()) {
      //
      // At least one driver in the platform are not in healthy status
      //
      HiiSetString (HiiHandle, STRING_TOKEN (STR_DRIVER_HEALTH_ALL_HEALTHY), GetStringById (STRING_TOKEN (STR_DRIVER_NOT_HEALTH)), NULL);
    } else {
      //
      // For the string of STR_DRIVER_HEALTH_ALL_HEALTHY previously has been updated and we need to update it while re-entry.
      //
      HiiSetString (HiiHandle, STRING_TOKEN (STR_DRIVER_HEALTH_ALL_HEALTHY), GetStringById (STRING_TOKEN (STR_DRIVER_HEALTH_ALL_HEALTHY)), NULL);
    }
  }

  HiiUpdateForm (
    HiiHandle,
    &gDeviceManagerFormSetGuid,
    mNextShowFormId,
    StartOpCodeHandle,
    EndOpCodeHandle
    );

  ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  Status = gFormBrowser2->SendForm (
                           gFormBrowser2,
                           &HiiHandle,
                           1,
                           &gDeviceManagerFormSetGuid,
                           mNextShowFormId,
                           NULL,
                           &ActionRequest
                           );
  if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
    EnableResetRequired ();
  }

  //
  // We will have returned from processing a callback, selected
  // a target to display
  //
  if ((gCallbackKey >= DEVICE_KEY_OFFSET)) {
    ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
    Status = gFormBrowser2->SendForm (
                             gFormBrowser2,
                             &HiiHandles[gCallbackKey - DEVICE_KEY_OFFSET],
                             1,
                             GuidLists[gCallbackKey - DEVICE_KEY_OFFSET],
                             0,
                             NULL,
                             &ActionRequest
                             );

    if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
      EnableResetRequired ();
    }

    //
    // Force return to Device Manager
    //
    gCallbackKey = FRONT_PAGE_KEY_DEVICE_MANAGER;
    goto Done;
  }

  //
  // Driver Health item chose.
  //
  if (gCallbackKey == DEVICE_MANAGER_KEY_DRIVER_HEALTH) {
    CallDriverHealth ();
    //
    // Force return to Device Manager
    //
    gCallbackKey = FRONT_PAGE_KEY_DEVICE_MANAGER;
    goto Done;
  }

  //
  // Enter from device manager and into the network device list.
  //
  if (gCallbackKey == QUESTION_NETWORK_DEVICE_ID) {
    mNextShowFormId = NETWORK_DEVICE_LIST_FORM_ID;
    gCallbackKey = FRONT_PAGE_KEY_DEVICE_MANAGER;
    goto Done;
  }

  //
  // In this case, go from the network device list to the specify device.
  //
  if ((gCallbackKey < MAX_KEY_SECTION_LEN + NETWORK_DEVICE_LIST_KEY_OFFSET ) && (gCallbackKey >= NETWORK_DEVICE_LIST_KEY_OFFSET)) {
    mNextShowFormId = NETWORK_DEVICE_FORM_ID;
    gCallbackKey = FRONT_PAGE_KEY_DEVICE_MANAGER;
    goto Done;
  }

  //
  // Select the ESC, the gCallbackKey == 0.
  //
  if(mNextShowFormId - 1 < DEVICE_MANAGER_FORM_ID) {
    mNextShowFormId = DEVICE_MANAGER_FORM_ID;
  } else {
    mNextShowFormId = (UINT16) (mNextShowFormId - 1);
    gCallbackKey = FRONT_PAGE_KEY_DEVICE_MANAGER;
  }

Done:
  //
  // Remove our packagelist from HII database.
  //
  HiiRemovePackages (HiiHandle);
  gDeviceManagerPrivate.HiiHandle = NULL;

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
  FreePool (HiiHandles);

  for (Index = 0; Index < HandleNum; Index++) {
    if (GuidLists[Index] != NULL) {
      FreePool (GuidLists[Index]);
    }
  }
  FreePool (GuidLists);

  return Status;
}

/**
  This function is invoked if user selected a interactive opcode from Driver Health's
  Formset. The decision by user is saved to gCallbackKey for later processing.

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
DriverHealthCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  if (Action == EFI_BROWSER_ACTION_CHANGED) {
    if ((Value == NULL) || (ActionRequest == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    gCallbackKey = QuestionId;

    //
    // Request to exit SendForm(), so as to switch to selected form
    //
    *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;

    return EFI_SUCCESS;
  }

  //
  // All other action return unsupported.
  //
  return EFI_UNSUPPORTED;
}

/**
  Collect and display the platform's driver health relative information, allow user to do interactive
  operation while the platform is unhealthy.

  This function display a form which divided into two parts. The one list all modules which has installed
  driver health protocol. The list usually contain driver name, controller name, and it's health info.
  While the driver name can't be retrieved, will use device path as backup. The other part of the form provide
  a choice to the user to repair all platform.

**/
VOID
CallDriverHealth (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_HII_HANDLE              HiiHandle;
  EFI_BROWSER_ACTION_REQUEST  ActionRequest;
  EFI_IFR_GUID_LABEL          *StartLabel;
  EFI_IFR_GUID_LABEL          *StartLabelRepair;
  EFI_IFR_GUID_LABEL          *EndLabel;
  EFI_IFR_GUID_LABEL          *EndLabelRepair;
  VOID                        *StartOpCodeHandle;
  VOID                        *EndOpCodeHandle;
  VOID                        *StartOpCodeHandleRepair;
  VOID                        *EndOpCodeHandleRepair;
  UINTN                       Index;
  EFI_STRING_ID               Token;
  EFI_STRING_ID               TokenHelp;
  EFI_STRING                  String;
  EFI_STRING                  TmpString;
  EFI_STRING                  DriverName;
  EFI_STRING                  ControllerName;
  LIST_ENTRY                  DriverHealthList;
  DRIVER_HEALTH_INFO          *DriverHealthInfo;
  LIST_ENTRY                  *Link;
  EFI_DEVICE_PATH_PROTOCOL    *DriverDevicePath;
  BOOLEAN                     RebootRequired;
  BOOLEAN                     IsControllerNameEmpty;
  UINTN                       StringSize;

  Index               = 0;
  DriverHealthInfo    = NULL;
  DriverDevicePath    = NULL;
  IsControllerNameEmpty = FALSE;
  InitializeListHead (&DriverHealthList);

  HiiHandle = gDeviceManagerPrivate.DriverHealthHiiHandle;
  if (HiiHandle == NULL) {
    //
    // Publish Driver Health HII data.
    //
    HiiHandle = HiiAddPackages (
                  &gDeviceManagerFormSetGuid,
                  gDeviceManagerPrivate.DriverHealthHandle,
                  DriverHealthVfrBin,
                  BdsDxeStrings,
                  NULL
                  );
    if (HiiHandle == NULL) {
      return;
    }

    gDeviceManagerPrivate.DriverHealthHiiHandle = HiiHandle;
  }

  //
  // Allocate space for creation of UpdateData Buffer
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  StartOpCodeHandleRepair = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandleRepair != NULL);

  EndOpCodeHandleRepair = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandleRepair != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = LABEL_DRIVER_HEALTH;

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabelRepair = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandleRepair, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabelRepair->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabelRepair->Number       = LABEL_DRIVER_HEALTH_REAPIR_ALL;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_DRIVER_HEALTH_END;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabelRepair = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandleRepair, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabelRepair->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabelRepair->Number       = LABEL_DRIVER_HEALTH_REAPIR_ALL_END;

  HiiCreateSubTitleOpCode (StartOpCodeHandle, STRING_TOKEN (STR_DH_STATUS_LIST), 0, 0, 1);

  Status = GetAllControllersHealthStatus (&DriverHealthList);
  ASSERT (Status != EFI_OUT_OF_RESOURCES);

  Link = GetFirstNode (&DriverHealthList);

  while (!IsNull (&DriverHealthList, Link)) {
    DriverHealthInfo = DEVICE_MANAGER_HEALTH_INFO_FROM_LINK (Link);

    Status = DriverHealthGetDriverName (DriverHealthInfo->DriverHandle, &DriverName);
    if (EFI_ERROR (Status)) {
      //
      // Can not get the Driver name, so use the Device path
      //
      DriverDevicePath = DevicePathFromHandle (DriverHealthInfo->DriverHandle);
      DriverName       = DevicePathToStr (DriverDevicePath);
    }
    StringSize = StrSize (DriverName);

    Status = DriverHealthGetControllerName (
               DriverHealthInfo->DriverHandle,
               DriverHealthInfo->ControllerHandle,
               DriverHealthInfo->ChildHandle,
               &ControllerName
               );

    if (!EFI_ERROR (Status)) {
      IsControllerNameEmpty = FALSE;
      StringSize += StrLen (L"    ") * sizeof(CHAR16);
      StringSize += StrLen (ControllerName) * sizeof(CHAR16);
    } else {
      IsControllerNameEmpty = TRUE;
    }

    //
    // Add the message of the Module itself provided after the string item.
    //
    if ((DriverHealthInfo->MessageList != NULL) && (DriverHealthInfo->MessageList->StringId != 0)) {
       TmpString = HiiGetString (
                     DriverHealthInfo->MessageList->HiiHandle,
                     DriverHealthInfo->MessageList->StringId,
                     NULL
                     );
       ASSERT (TmpString != NULL);

       StringSize += StrLen (L"    ") * sizeof(CHAR16);
       StringSize += StrLen (TmpString) * sizeof(CHAR16);

       String = (EFI_STRING) AllocateZeroPool (StringSize);
       ASSERT (String != NULL);

       StrCpyS (String, StringSize / sizeof(CHAR16), DriverName);
       if (!IsControllerNameEmpty) {
        StrCatS (String, StringSize / sizeof(CHAR16), L"    ");
        StrCatS (String, StringSize / sizeof(CHAR16), ControllerName);
       }

       StrCatS (String, StringSize / sizeof(CHAR16), L"    ");
       StrCatS (String, StringSize / sizeof(CHAR16), TmpString);

    } else {
      //
      // Update the string will be displayed base on the driver's health status
      //
      switch(DriverHealthInfo->HealthStatus) {
      case EfiDriverHealthStatusRepairRequired:
        TmpString = GetStringById (STRING_TOKEN (STR_REPAIR_REQUIRED));
        break;
      case EfiDriverHealthStatusConfigurationRequired:
        TmpString = GetStringById (STRING_TOKEN (STR_CONFIGURATION_REQUIRED));
        break;
      case EfiDriverHealthStatusFailed:
        TmpString = GetStringById (STRING_TOKEN (STR_OPERATION_FAILED));
        break;
      case EfiDriverHealthStatusReconnectRequired:
        TmpString = GetStringById (STRING_TOKEN (STR_RECONNECT_REQUIRED));
        break;
      case EfiDriverHealthStatusRebootRequired:
        TmpString = GetStringById (STRING_TOKEN (STR_REBOOT_REQUIRED));
        break;
      default:
        TmpString = GetStringById (STRING_TOKEN (STR_DRIVER_HEALTH_HEALTHY));
        break;
      }
      ASSERT (TmpString != NULL);

      StringSize += StrLen (TmpString) * sizeof(CHAR16);

      String = (EFI_STRING) AllocateZeroPool (StringSize);
      ASSERT (String != NULL);

      StrCpyS (String, StringSize / sizeof (CHAR16), DriverName);
      if (!IsControllerNameEmpty) {
        StrCatS (String, StringSize / sizeof (CHAR16), L"    ");
        StrCatS (String, StringSize / sizeof (CHAR16), ControllerName);
      }

      StrCatS (String, StringSize / sizeof (CHAR16), TmpString);
    }

    FreePool (TmpString);

    Token = HiiSetString (HiiHandle, 0, String, NULL);
    FreePool (String);

    TokenHelp = HiiSetString (HiiHandle, 0, GetStringById( STRING_TOKEN (STR_DH_REPAIR_SINGLE_HELP)), NULL);

    HiiCreateActionOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (Index + DRIVER_HEALTH_KEY_OFFSET),
      Token,
      TokenHelp,
      EFI_IFR_FLAG_CALLBACK,
      0
      );
    Index++;
    Link = GetNextNode (&DriverHealthList, Link);
  }

  //
  // Add End Opcode for Subtitle
  //
  HiiCreateEndOpCode (StartOpCodeHandle);

  HiiCreateSubTitleOpCode (StartOpCodeHandleRepair, STRING_TOKEN (STR_DRIVER_HEALTH_REPAIR_ALL), 0, 0, 1);
  TokenHelp = HiiSetString (HiiHandle, 0, GetStringById( STRING_TOKEN (STR_DH_REPAIR_ALL_HELP)), NULL);

  if (PlaformHealthStatusCheck ()) {
    //
    // No action need to do for the platform
    //
    Token = HiiSetString (HiiHandle, 0, GetStringById( STRING_TOKEN (STR_DRIVER_HEALTH_ALL_HEALTHY)), NULL);
    HiiCreateActionOpCode (
      StartOpCodeHandleRepair,
      0,
      Token,
      TokenHelp,
      EFI_IFR_FLAG_READ_ONLY,
      0
      );
  } else {
    //
    // Create ActionOpCode only while the platform need to do health related operation.
    //
    Token = HiiSetString (HiiHandle, 0, GetStringById( STRING_TOKEN (STR_DH_REPAIR_ALL_TITLE)), NULL);
    HiiCreateActionOpCode (
      StartOpCodeHandleRepair,
      (EFI_QUESTION_ID) DRIVER_HEALTH_REPAIR_ALL_KEY,
      Token,
      TokenHelp,
      EFI_IFR_FLAG_CALLBACK,
      0
      );
  }

  HiiCreateEndOpCode (StartOpCodeHandleRepair);

  Status = HiiUpdateForm (
             HiiHandle,
             &gDriverHealthFormSetGuid,
             DRIVER_HEALTH_FORM_ID,
             StartOpCodeHandle,
             EndOpCodeHandle
             );
  ASSERT (Status != EFI_NOT_FOUND);
  ASSERT (Status != EFI_BUFFER_TOO_SMALL);

  Status = HiiUpdateForm (
            HiiHandle,
            &gDriverHealthFormSetGuid,
            DRIVER_HEALTH_FORM_ID,
            StartOpCodeHandleRepair,
            EndOpCodeHandleRepair
    );
  ASSERT (Status != EFI_NOT_FOUND);
  ASSERT (Status != EFI_BUFFER_TOO_SMALL);

  ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  Status = gFormBrowser2->SendForm (
                           gFormBrowser2,
                           &HiiHandle,
                           1,
                           &gDriverHealthFormSetGuid,
                           DRIVER_HEALTH_FORM_ID,
                           NULL,
                           &ActionRequest
                           );
  if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
    EnableResetRequired ();
  }

  //
  // We will have returned from processing a callback - user either hit ESC to exit, or selected
  // a target to display.
  // Process the diver health status states here.
  //
  if (gCallbackKey >= DRIVER_HEALTH_KEY_OFFSET && gCallbackKey != DRIVER_HEALTH_REPAIR_ALL_KEY) {
    ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;

    Link = GetFirstNode (&DriverHealthList);
    Index = 0;

    while (!IsNull (&DriverHealthList, Link)) {
      //
      // Got the item relative node in the List
      //
      if (Index == (gCallbackKey - DRIVER_HEALTH_KEY_OFFSET)) {
        DriverHealthInfo = DEVICE_MANAGER_HEALTH_INFO_FROM_LINK (Link);
        //
        // Process the driver's healthy status for the specify module
        //
        RebootRequired = FALSE;
        ProcessSingleControllerHealth (
          DriverHealthInfo->DriverHealth,
          DriverHealthInfo->ControllerHandle,
          DriverHealthInfo->ChildHandle,
          DriverHealthInfo->HealthStatus,
          &(DriverHealthInfo->MessageList),
          DriverHealthInfo->HiiHandle,
          &RebootRequired
          );
        if (RebootRequired) {
          gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
        }
        break;
      }
      Index++;
      Link = GetNextNode (&DriverHealthList, Link);
    }

    if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
      EnableResetRequired ();
    }

    //
    // Force return to the form of Driver Health in Device Manager
    //
    gCallbackKey = DRIVER_HEALTH_RETURN_KEY;
  }

  //
  // Repair the whole platform
  //
  if (gCallbackKey == DRIVER_HEALTH_REPAIR_ALL_KEY) {
    ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;

    PlatformRepairAll (&DriverHealthList);

    gCallbackKey = DRIVER_HEALTH_RETURN_KEY;
  }

  //
  // Remove driver health packagelist from HII database.
  //
  HiiRemovePackages (HiiHandle);
  gDeviceManagerPrivate.DriverHealthHiiHandle = NULL;

  //
  // Free driver health info list
  //
  while (!IsListEmpty (&DriverHealthList)) {

    Link = GetFirstNode(&DriverHealthList);
    DriverHealthInfo = DEVICE_MANAGER_HEALTH_INFO_FROM_LINK (Link);
    RemoveEntryList (Link);

    if (DriverHealthInfo->MessageList != NULL) {
      FreePool(DriverHealthInfo->MessageList);
      FreePool (DriverHealthInfo);
    }
  }

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
  HiiFreeOpCodeHandle (StartOpCodeHandleRepair);
  HiiFreeOpCodeHandle (EndOpCodeHandleRepair);

  if (gCallbackKey == DRIVER_HEALTH_RETURN_KEY) {
    //
    // Force return to Driver Health Form
    //
    gCallbackKey = DEVICE_MANAGER_KEY_DRIVER_HEALTH;
    CallDriverHealth ();
  }
}


/**
  Check the Driver Health status of a single controller and try to process it if not healthy.

  This function called by CheckAllControllersHealthStatus () function in order to process a specify
  contoller's health state.

  @param DriverHealthList   A Pointer to the list contain all of the platform driver health information.
  @param DriverHandle       The handle of driver.
  @param ControllerHandle   The class guid specifies which form set will be displayed.
  @param ChildHandle        The handle of the child controller to retrieve the health
                            status on.  This is an optional parameter that may be NULL.
  @param DriverHealth       A pointer to the EFI_DRIVER_HEALTH_PROTOCOL instance.
  @param HealthStatus       The health status of the controller.

  @retval EFI_INVALID_PARAMETER   HealthStatus or DriverHealth is NULL.
  @retval HealthStatus            The Health status of specify controller.
  @retval EFI_OUT_OF_RESOURCES    The list of Driver Health Protocol handles can not be retrieved.
  @retval EFI_NOT_FOUND           No controller in the platform install Driver Health Protocol.
  @retval EFI_SUCCESS             The Health related operation has been taken successfully.

**/
EFI_STATUS
EFIAPI
GetSingleControllerHealthStatus (
  IN OUT LIST_ENTRY                   *DriverHealthList,
  IN EFI_HANDLE                       DriverHandle,
  IN EFI_HANDLE                       ControllerHandle,  OPTIONAL
  IN EFI_HANDLE                       ChildHandle,       OPTIONAL
  IN EFI_DRIVER_HEALTH_PROTOCOL       *DriverHealth,
  IN EFI_DRIVER_HEALTH_STATUS         *HealthStatus
  )
{
  EFI_STATUS                     Status;
  EFI_DRIVER_HEALTH_HII_MESSAGE  *MessageList;
  EFI_HII_HANDLE                 FormHiiHandle;
  DRIVER_HEALTH_INFO             *DriverHealthInfo;

  if (HealthStatus == NULL) {
    //
    // If HealthStatus is NULL, then return EFI_INVALID_PARAMETER
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  // Assume the HealthStatus is healthy
  //
  *HealthStatus = EfiDriverHealthStatusHealthy;

  if (DriverHealth == NULL) {
    //
    // If DriverHealth is NULL, then return EFI_INVALID_PARAMETER
    //
    return EFI_INVALID_PARAMETER;
  }

  if (ControllerHandle == NULL) {
    //
    // If ControllerHandle is NULL, the return the cumulative health status of the driver
    //
    Status = DriverHealth->GetHealthStatus (DriverHealth, NULL, NULL, HealthStatus, NULL, NULL);
    if (*HealthStatus == EfiDriverHealthStatusHealthy) {
      //
      // Add the driver health related information into the list
      //
      DriverHealthInfo = AllocateZeroPool (sizeof (DRIVER_HEALTH_INFO));
      if (DriverHealthInfo == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      DriverHealthInfo->Signature          = DEVICE_MANAGER_DRIVER_HEALTH_INFO_SIGNATURE;
      DriverHealthInfo->DriverHandle       = DriverHandle;
      DriverHealthInfo->ControllerHandle   = NULL;
      DriverHealthInfo->ChildHandle        = NULL;
      DriverHealthInfo->HiiHandle          = NULL;
      DriverHealthInfo->DriverHealth       = DriverHealth;
      DriverHealthInfo->MessageList        = NULL;
      DriverHealthInfo->HealthStatus       = *HealthStatus;

      InsertTailList (DriverHealthList, &DriverHealthInfo->Link);
    }
    return Status;
  }

  MessageList   = NULL;
  FormHiiHandle = NULL;

  //
  // Collect the health status with the optional HII message list
  //
  Status = DriverHealth->GetHealthStatus (DriverHealth, ControllerHandle, ChildHandle, HealthStatus, &MessageList, &FormHiiHandle);

  if (EFI_ERROR (Status)) {
    //
    // If the health status could not be retrieved, then return immediately
    //
    return Status;
  }

  //
  // Add the driver health related information into the list
  //
  DriverHealthInfo = AllocateZeroPool (sizeof (DRIVER_HEALTH_INFO));
  if (DriverHealthInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DriverHealthInfo->Signature          = DEVICE_MANAGER_DRIVER_HEALTH_INFO_SIGNATURE;
  DriverHealthInfo->DriverHandle       = DriverHandle;
  DriverHealthInfo->ControllerHandle   = ControllerHandle;
  DriverHealthInfo->ChildHandle        = ChildHandle;
  DriverHealthInfo->HiiHandle          = FormHiiHandle;
  DriverHealthInfo->DriverHealth       = DriverHealth;
  DriverHealthInfo->MessageList        = MessageList;
  DriverHealthInfo->HealthStatus       = *HealthStatus;

  InsertTailList (DriverHealthList, &DriverHealthInfo->Link);

  return EFI_SUCCESS;
}

/**
  Collects all the EFI Driver Health Protocols currently present in the EFI Handle Database,
  and queries each EFI Driver Health Protocol to determine if one or more of the controllers
  managed by each EFI Driver Health Protocol instance are not healthy.

  @param DriverHealthList   A Pointer to the list contain all of the platform driver health
                            information.

  @retval    EFI_NOT_FOUND         No controller in the platform install Driver Health Protocol.
  @retval    EFI_SUCCESS           All the controllers in the platform are healthy.
  @retval    EFI_OUT_OF_RESOURCES  The list of Driver Health Protocol handles can not be retrieved.

**/
EFI_STATUS
GetAllControllersHealthStatus (
  IN OUT LIST_ENTRY  *DriverHealthList
  )
{
  EFI_STATUS                 Status;
  UINTN                      NumHandles;
  EFI_HANDLE                 *DriverHealthHandles;
  EFI_DRIVER_HEALTH_PROTOCOL *DriverHealth;
  EFI_DRIVER_HEALTH_STATUS   HealthStatus;
  UINTN                      DriverHealthIndex;
  EFI_HANDLE                 *Handles;
  UINTN                      HandleCount;
  UINTN                      ControllerIndex;
  UINTN                      ChildIndex;

  //
  // Initialize local variables
  //
  Handles                 = NULL;
  DriverHealthHandles     = NULL;
  NumHandles              = 0;
  HandleCount             = 0;

  HealthStatus = EfiDriverHealthStatusHealthy;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDriverHealthProtocolGuid,
                  NULL,
                  &NumHandles,
                  &DriverHealthHandles
                  );

  if (Status == EFI_NOT_FOUND || NumHandles == 0) {
    //
    // If there are no Driver Health Protocols handles, then return EFI_NOT_FOUND
    //
    return EFI_NOT_FOUND;
  }

  if (EFI_ERROR (Status) || DriverHealthHandles == NULL) {
    //
    // If the list of Driver Health Protocol handles can not be retrieved, then
    // return EFI_OUT_OF_RESOURCES
    //
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Check the health status of all controllers in the platform
  // Start by looping through all the Driver Health Protocol handles in the handle database
  //
  for (DriverHealthIndex = 0; DriverHealthIndex < NumHandles; DriverHealthIndex++) {
    //
    // Skip NULL Driver Health Protocol handles
    //
    if (DriverHealthHandles[DriverHealthIndex] == NULL) {
      continue;
    }

    //
    // Retrieve the Driver Health Protocol from DriverHandle
    //
    Status = gBS->HandleProtocol (
                    DriverHealthHandles[DriverHealthIndex],
                    &gEfiDriverHealthProtocolGuid,
                    (VOID **)&DriverHealth
                    );
    if (EFI_ERROR (Status)) {
      //
      // If the Driver Health Protocol can not be retrieved, then skip to the next
      // Driver Health Protocol handle
      //
      continue;
    }

    //
    // Check the health of all the controllers managed by a Driver Health Protocol handle
    //
    Status = GetSingleControllerHealthStatus (DriverHealthList, DriverHealthHandles[DriverHealthIndex], NULL, NULL, DriverHealth, &HealthStatus);

    //
    // If Status is an error code, then the health information could not be retrieved, so assume healthy
    // and skip to the next Driver Health Protocol handle
    //
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // If all the controllers managed by this Driver Health Protocol are healthy, then skip to the next
    // Driver Health Protocol handle
    //
    if (HealthStatus == EfiDriverHealthStatusHealthy) {
      continue;
    }

    //
    // See if the list of all handles in the handle database has been retrieved
    //
    //
    if (Handles == NULL) {
      //
      // Retrieve the list of all handles from the handle database
      //
      Status = gBS->LocateHandleBuffer (
        AllHandles,
        NULL,
        NULL,
        &HandleCount,
        &Handles
        );
      if (EFI_ERROR (Status) || Handles == NULL) {
        //
        // If all the handles in the handle database can not be retrieved, then
        // return EFI_OUT_OF_RESOURCES
        //
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
    }
    //
    // Loop through all the controller handles in the handle database
    //
    for (ControllerIndex = 0; ControllerIndex < HandleCount; ControllerIndex++) {
      //
      // Skip NULL controller handles
      //
      if (Handles[ControllerIndex] == NULL) {
        continue;
      }

      Status = GetSingleControllerHealthStatus (DriverHealthList, DriverHealthHandles[DriverHealthIndex], Handles[ControllerIndex], NULL, DriverHealth, &HealthStatus);
      if (EFI_ERROR (Status)) {
        //
        // If Status is an error code, then the health information could not be retrieved, so assume healthy
        //
        HealthStatus = EfiDriverHealthStatusHealthy;
      }

      //
      // If CheckHealthSingleController() returned an error on a terminal state, then do not check the health of child controllers
      //
      if (EFI_ERROR (Status)) {
        continue;
      }

      //
      // Loop through all the child handles in the handle database
      //
      for (ChildIndex = 0; ChildIndex < HandleCount; ChildIndex++) {
        //
        // Skip NULL child handles
        //
        if (Handles[ChildIndex] == NULL) {
          continue;
        }

        Status = GetSingleControllerHealthStatus (DriverHealthList, DriverHealthHandles[DriverHealthIndex], Handles[ControllerIndex], Handles[ChildIndex], DriverHealth, &HealthStatus);
        if (EFI_ERROR (Status)) {
          //
          // If Status is an error code, then the health information could not be retrieved, so assume healthy
          //
          HealthStatus = EfiDriverHealthStatusHealthy;
        }

        //
        // If CheckHealthSingleController() returned an error on a terminal state, then skip to the next child
        //
        if (EFI_ERROR (Status)) {
          continue;
        }
      }
    }
  }

  Status = EFI_SUCCESS;

Done:
  if (Handles != NULL) {
    gBS->FreePool (Handles);
  }
  if (DriverHealthHandles != NULL) {
    gBS->FreePool (DriverHealthHandles);
  }

  return Status;
}


/**
  Check the healthy status of the platform, this function will return immediately while found one driver
  in the platform are not healthy.

  @retval FALSE      at least one driver in the platform are not healthy.
  @retval TRUE       No controller install Driver Health Protocol,
                     or all controllers in the platform are in healthy status.
**/
BOOLEAN
PlaformHealthStatusCheck (
  VOID
  )
{
  EFI_DRIVER_HEALTH_STATUS          HealthStatus;
  EFI_STATUS                        Status;
  UINTN                             Index;
  UINTN                             NoHandles;
  EFI_HANDLE                        *DriverHealthHandles;
  EFI_DRIVER_HEALTH_PROTOCOL        *DriverHealth;
  BOOLEAN                           AllHealthy;

  //
  // Initialize local variables
  //
  DriverHealthHandles = NULL;
  DriverHealth        = NULL;

  HealthStatus = EfiDriverHealthStatusHealthy;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDriverHealthProtocolGuid,
                  NULL,
                  &NoHandles,
                  &DriverHealthHandles
                  );
  //
  // There are no handles match the search for Driver Health Protocol has been installed.
  //
  if (Status == EFI_NOT_FOUND) {
    return TRUE;
  }
  //
  // Assume all modules are healthy.
  //
  AllHealthy = TRUE;

  //
  // Found one or more Handles.
  //
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < NoHandles; Index++) {
      Status = gBS->HandleProtocol (
                      DriverHealthHandles[Index],
                      &gEfiDriverHealthProtocolGuid,
                      (VOID **) &DriverHealth
                      );
      if (!EFI_ERROR (Status)) {
        Status = DriverHealth->GetHealthStatus (
                                 DriverHealth,
                                 NULL,
                                 NULL,
                                 &HealthStatus,
                                 NULL,
                                 NULL
                                 );
      }
      //
      // Get the healthy status of the module
      //
      if (!EFI_ERROR (Status)) {
         if (HealthStatus != EfiDriverHealthStatusHealthy) {
           //
           // Return immediately one driver's status not in healthy.
           //
           return FALSE;
         }
      }
    }
  }
  return AllHealthy;
}

/**
  Processes a single controller using the EFI Driver Health Protocol associated with
  that controller. This algorithm continues to query the GetHealthStatus() service until
  one of the legal terminal states of the EFI Driver Health Protocol is reached. This may
  require the processing of HII Messages, HII Form, and invocation of repair operations.

  @param DriverHealth       A pointer to the EFI_DRIVER_HEALTH_PROTOCOL instance.
  @param ControllerHandle   The class guid specifies which form set will be displayed.
  @param ChildHandle        The handle of the child controller to retrieve the health
                            status on.  This is an optional parameter that may be NULL.
  @param HealthStatus       The health status of the controller.
  @param MessageList        An array of warning or error messages associated
                            with the controller specified by ControllerHandle and
                            ChildHandle.  This is an optional parameter that may be NULL.
  @param FormHiiHandle      The HII handle for an HII form associated with the
                            controller specified by ControllerHandle and ChildHandle.
  @param RebootRequired     Indicate whether a reboot is required to repair the controller.
**/
VOID
ProcessSingleControllerHealth (
  IN  EFI_DRIVER_HEALTH_PROTOCOL         *DriverHealth,
  IN  EFI_HANDLE                         ControllerHandle, OPTIONAL
  IN  EFI_HANDLE                         ChildHandle,      OPTIONAL
  IN  EFI_DRIVER_HEALTH_STATUS           HealthStatus,
  IN  EFI_DRIVER_HEALTH_HII_MESSAGE      **MessageList,    OPTIONAL
  IN  EFI_HII_HANDLE                     FormHiiHandle,
  IN OUT BOOLEAN                         *RebootRequired
  )
{
  EFI_STATUS                         Status;
  EFI_DRIVER_HEALTH_STATUS           LocalHealthStatus;

  LocalHealthStatus = HealthStatus;
  //
  // If the module need to be repaired or reconfiguration,  will process it until
  // reach a terminal status. The status from EfiDriverHealthStatusRepairRequired after repair
  // will be in (Health, Failed, Configuration Required).
  //
  while(LocalHealthStatus == EfiDriverHealthStatusConfigurationRequired ||
        LocalHealthStatus == EfiDriverHealthStatusRepairRequired) {

    if (LocalHealthStatus == EfiDriverHealthStatusRepairRequired) {
      Status = DriverHealth->Repair (
                               DriverHealth,
                               ControllerHandle,
                               ChildHandle,
                               RepairNotify
                               );
    }
    //
    // Via a form of the driver need to do configuration provided to process of status in
    // EfiDriverHealthStatusConfigurationRequired. The status after configuration should be in
    // (Healthy, Reboot Required, Failed, Reconnect Required, Repair Required).
    //
    if (LocalHealthStatus == EfiDriverHealthStatusConfigurationRequired) {
      if (FormHiiHandle != NULL) {
        Status = gFormBrowser2->SendForm (
                                  gFormBrowser2,
                                  &FormHiiHandle,
                                  1,
                                  &gEfiHiiDriverHealthFormsetGuid,
                                  0,
                                  NULL,
                                  NULL
                                  );
        ASSERT( !EFI_ERROR (Status));
      } else {
        //
        // Exit the loop in case no FormHiiHandle is supplied to prevent dead-loop
        //
        break;
      }
    }

    Status = DriverHealth->GetHealthStatus (
                              DriverHealth,
                              ControllerHandle,
                              ChildHandle,
                              &LocalHealthStatus,
                              NULL,
                              &FormHiiHandle
                              );
    ASSERT_EFI_ERROR (Status);

    if (*MessageList != NULL) {
      ProcessMessages (*MessageList);
    }
  }

  //
  // Health status in {Healthy, Failed} may also have Messages need to process
  //
  if (LocalHealthStatus == EfiDriverHealthStatusHealthy || LocalHealthStatus == EfiDriverHealthStatusFailed) {
    if (*MessageList != NULL) {
      ProcessMessages (*MessageList);
    }
  }
  //
  // Check for RebootRequired or ReconnectRequired
  //
  if (LocalHealthStatus == EfiDriverHealthStatusRebootRequired) {
    *RebootRequired = TRUE;
  }

  //
  // Do reconnect if need.
  //
  if (LocalHealthStatus == EfiDriverHealthStatusReconnectRequired) {
    Status = gBS->DisconnectController (ControllerHandle, NULL, NULL);
    if (EFI_ERROR (Status)) {
      //
      // Disconnect failed.  Need to promote reconnect to a reboot.
      //
      *RebootRequired = TRUE;
    } else {
      gBS->ConnectController (ControllerHandle, NULL, NULL, TRUE);
    }
  }
}


/**
  Reports the progress of a repair operation.

  @param[in]  Value             A value between 0 and Limit that identifies the current
                                progress of the repair operation.

  @param[in]  Limit             The maximum value of Value for the current repair operation.
                                For example, a driver that wants to specify progress in
                                percent would use a Limit value of 100.

  @retval EFI_SUCCESS           The progress of a repair operation is reported successfully.

**/
EFI_STATUS
EFIAPI
RepairNotify (
  IN  UINTN Value,
  IN  UINTN Limit
  )
{
  UINTN Percent;

  if (Limit  == 0) {
    Print(L"Repair Progress Undefined\n\r");
  } else {
    Percent = Value * 100 / Limit;
    Print(L"Repair Progress = %3d%%\n\r", Percent);
  }
  return EFI_SUCCESS;
}

/**
  Processes a set of messages returned by the GetHealthStatus ()
  service of the EFI Driver Health Protocol

  @param    MessageList  The MessageList point to messages need to processed.

**/
VOID
ProcessMessages (
  IN  EFI_DRIVER_HEALTH_HII_MESSAGE      *MessageList
  )
{
  UINTN                           MessageIndex;
  EFI_STRING                      MessageString;

  for (MessageIndex = 0;
       MessageList[MessageIndex].HiiHandle != NULL;
       MessageIndex++) {

    MessageString = HiiGetString (
                        MessageList[MessageIndex].HiiHandle,
                        MessageList[MessageIndex].StringId,
                        NULL
                        );
    if (MessageString != NULL) {
      //
      // User can customize the output. Just simply print out the MessageString like below.
      // Also can use the HiiHandle to display message on the front page.
      //
      // Print(L"%s\n",MessageString);
      // gBS->Stall (100000);
    }
  }

}

/**
  Repair the whole platform.

  This function is the main entry for user choose "Repair All" in the front page.
  It will try to do recovery job till all the driver health protocol installed modules
  reach a terminal state.

  @param DriverHealthList   A Pointer to the list contain all of the platform driver health
                            information.

**/
VOID
PlatformRepairAll (
  IN LIST_ENTRY  *DriverHealthList
  )
{
  DRIVER_HEALTH_INFO          *DriverHealthInfo;
  LIST_ENTRY                  *Link;
  BOOLEAN                     RebootRequired;

  ASSERT (DriverHealthList != NULL);

  RebootRequired = FALSE;

  for ( Link = GetFirstNode (DriverHealthList)
      ; !IsNull (DriverHealthList, Link)
      ; Link = GetNextNode (DriverHealthList, Link)
      ) {
    DriverHealthInfo = DEVICE_MANAGER_HEALTH_INFO_FROM_LINK (Link);
    //
    // Do driver health status operation by each link node
    //
    ASSERT (DriverHealthInfo != NULL);

    ProcessSingleControllerHealth (
      DriverHealthInfo->DriverHealth,
      DriverHealthInfo->ControllerHandle,
      DriverHealthInfo->ChildHandle,
      DriverHealthInfo->HealthStatus,
      &(DriverHealthInfo->MessageList),
      DriverHealthInfo->HiiHandle,
      &RebootRequired
      );
  }

  if (RebootRequired) {
    gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
  }
}

/**

  Select the best matching language according to front page policy for best user experience.

  This function supports both ISO 639-2 and RFC 4646 language codes, but language
  code types may not be mixed in a single call to this function.

  @param  SupportedLanguages   A pointer to a Null-terminated ASCII string that
                               contains a set of language codes in the format
                               specified by Iso639Language.
  @param  Iso639Language       If TRUE, then all language codes are assumed to be
                               in ISO 639-2 format.  If FALSE, then all language
                               codes are assumed to be in RFC 4646 language format.

  @retval NULL                 The best matching language could not be found in SupportedLanguages.
  @retval NULL                 There are not enough resources available to return the best matching
                               language.
  @retval Other                A pointer to a Null-terminated ASCII string that is the best matching
                               language in SupportedLanguages.
**/
CHAR8 *
DriverHealthSelectBestLanguage (
  IN CHAR8        *SupportedLanguages,
  IN BOOLEAN      Iso639Language
  )
{
  CHAR8           *LanguageVariable;
  CHAR8           *BestLanguage;

  GetEfiGlobalVariable2 (Iso639Language ? L"Lang" : L"PlatformLang", (VOID**)&LanguageVariable, NULL);

  BestLanguage = GetBestLanguage(
                   SupportedLanguages,
                   Iso639Language,
                   (LanguageVariable != NULL) ? LanguageVariable : "",
                   Iso639Language ? "eng" : "en-US",
                   NULL
                   );
  if (LanguageVariable != NULL) {
    FreePool (LanguageVariable);
  }

  return BestLanguage;
}



/**

  This is an internal worker function to get the Component Name (2) protocol interface
  and the language it supports.

  @param  ProtocolGuid         A pointer to an EFI_GUID. It points to Component Name (2) protocol GUID.
  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  ComponentName        A pointer to the Component Name (2) protocol interface.
  @param  SupportedLanguage    The best suitable language that matches the SupportedLangues interface for the
                               located Component Name (2) instance.

  @retval EFI_SUCCESS          The Component Name (2) protocol instance is successfully located and we find
                               the best matching language it support.
  @retval EFI_UNSUPPORTED      The input Language is not supported by the Component Name (2) protocol.
  @retval Other                Some error occurs when locating Component Name (2) protocol instance or finding
                               the supported language.

**/
EFI_STATUS
GetComponentNameWorker (
  IN  EFI_GUID                    *ProtocolGuid,
  IN  EFI_HANDLE                  DriverBindingHandle,
  OUT EFI_COMPONENT_NAME_PROTOCOL **ComponentName,
  OUT CHAR8                       **SupportedLanguage
  )
{
  EFI_STATUS                      Status;

  //
  // Locate Component Name (2) protocol on the driver binging handle.
  //
  Status = gBS->OpenProtocol (
                 DriverBindingHandle,
                 ProtocolGuid,
                 (VOID **) ComponentName,
                 NULL,
                 NULL,
                 EFI_OPEN_PROTOCOL_GET_PROTOCOL
                 );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Apply shell policy to select the best language.
  //
  *SupportedLanguage = DriverHealthSelectBestLanguage (
                         (*ComponentName)->SupportedLanguages,
                         (BOOLEAN) (ProtocolGuid == &gEfiComponentNameProtocolGuid)
                         );
  if (*SupportedLanguage == NULL) {
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

/**

  This is an internal worker function to get driver name from Component Name (2) protocol interface.


  @param  ProtocolGuid         A pointer to an EFI_GUID. It points to Component Name (2) protocol GUID.
  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  DriverName           A pointer to the Unicode string to return. This Unicode string is the name
                               of the driver specified by This.

  @retval EFI_SUCCESS          The driver name is successfully retrieved from Component Name (2) protocol
                               interface.
  @retval Other                The driver name cannot be retrieved from Component Name (2) protocol
                               interface.

**/
EFI_STATUS
GetDriverNameWorker (
  IN  EFI_GUID    *ProtocolGuid,
  IN  EFI_HANDLE  DriverBindingHandle,
  OUT CHAR16      **DriverName
  )
{
  EFI_STATUS                     Status;
  CHAR8                          *BestLanguage;
  EFI_COMPONENT_NAME_PROTOCOL    *ComponentName;

  //
  // Retrieve Component Name (2) protocol instance on the driver binding handle and
  // find the best language this instance supports.
  //
  Status = GetComponentNameWorker (
             ProtocolGuid,
             DriverBindingHandle,
             &ComponentName,
             &BestLanguage
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the driver name from Component Name (2) protocol instance on the driver binging handle.
  //
  Status = ComponentName->GetDriverName (
                            ComponentName,
                            BestLanguage,
                            DriverName
                            );
  FreePool (BestLanguage);

  return Status;
}

/**

  This function gets driver name from Component Name 2 protocol interface and Component Name protocol interface
  in turn. It first tries UEFI 2.0 Component Name 2 protocol interface and try to get the driver name.
  If the attempt fails, it then gets the driver name from EFI 1.1 Component Name protocol for backward
  compatibility support.

  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  DriverName           A pointer to the Unicode string to return. This Unicode string is the name
                               of the driver specified by This.

  @retval EFI_SUCCESS          The driver name is successfully retrieved from Component Name (2) protocol
                               interface.
  @retval Other                The driver name cannot be retrieved from Component Name (2) protocol
                               interface.

**/
EFI_STATUS
DriverHealthGetDriverName (
  IN  EFI_HANDLE  DriverBindingHandle,
  OUT CHAR16      **DriverName
  )
{
  EFI_STATUS      Status;

  //
  // Get driver name from UEFI 2.0 Component Name 2 protocol interface.
  //
  Status = GetDriverNameWorker (&gEfiComponentName2ProtocolGuid, DriverBindingHandle, DriverName);
  if (EFI_ERROR (Status)) {
    //
    // If it fails to get the driver name from Component Name protocol interface, we should fall back on
    // EFI 1.1 Component Name protocol interface.
    //
    Status = GetDriverNameWorker (&gEfiComponentNameProtocolGuid, DriverBindingHandle, DriverName);
  }

  return Status;
}



/**
  This function gets controller name from Component Name 2 protocol interface and Component Name protocol interface
  in turn. It first tries UEFI 2.0 Component Name 2 protocol interface and try to get the controller name.
  If the attempt fails, it then gets the controller name from EFI 1.1 Component Name protocol for backward
  compatibility support.

  @param  ProtocolGuid         A pointer to an EFI_GUID. It points to Component Name (2) protocol GUID.
  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  ControllerHandle     The handle of a controller that the driver specified by This is managing.
                               This handle specifies the controller whose name is to be returned.
  @param  ChildHandle          The handle of the child controller to retrieve the name of. This is an
                               optional parameter that may be NULL. It will be NULL for device drivers.
                               It will also be NULL for bus drivers that attempt to retrieve the name
                               of the bus controller. It will not be NULL for a bus driver that attempts
                               to retrieve the name of a child controller.
  @param  ControllerName       A pointer to the Unicode string to return. This Unicode string
                               is the name of the controller specified by ControllerHandle and ChildHandle.

  @retval  EFI_SUCCESS         The controller name is successfully retrieved from Component Name (2) protocol
                               interface.
  @retval  Other               The controller name cannot be retrieved from Component Name (2) protocol.

**/
EFI_STATUS
GetControllerNameWorker (
  IN  EFI_GUID    *ProtocolGuid,
  IN  EFI_HANDLE  DriverBindingHandle,
  IN  EFI_HANDLE  ControllerHandle,
  IN  EFI_HANDLE  ChildHandle,
  OUT CHAR16      **ControllerName
  )
{
  EFI_STATUS                     Status;
  CHAR8                          *BestLanguage;
  EFI_COMPONENT_NAME_PROTOCOL    *ComponentName;

  //
  // Retrieve Component Name (2) protocol instance on the driver binding handle and
  // find the best language this instance supports.
  //
  Status = GetComponentNameWorker (
             ProtocolGuid,
             DriverBindingHandle,
             &ComponentName,
             &BestLanguage
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the controller name from Component Name (2) protocol instance on the driver binging handle.
  //
  Status = ComponentName->GetControllerName (
                            ComponentName,
                            ControllerHandle,
                            ChildHandle,
                            BestLanguage,
                            ControllerName
                            );
  FreePool (BestLanguage);

  return Status;
}

/**

  This function gets controller name from Component Name 2 protocol interface and Component Name protocol interface
  in turn. It first tries UEFI 2.0 Component Name 2 protocol interface and try to get the controller name.
  If the attempt fails, it then gets the controller name from EFI 1.1 Component Name protocol for backward
  compatibility support.

  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  ControllerHandle     The handle of a controller that the driver specified by This is managing.
                               This handle specifies the controller whose name is to be returned.
  @param  ChildHandle          The handle of the child controller to retrieve the name of. This is an
                               optional parameter that may be NULL. It will be NULL for device drivers.
                               It will also be NULL for bus drivers that attempt to retrieve the name
                               of the bus controller. It will not be NULL for a bus driver that attempts
                               to retrieve the name of a child controller.
  @param  ControllerName       A pointer to the Unicode string to return. This Unicode string
                               is the name of the controller specified by ControllerHandle and ChildHandle.

  @retval EFI_SUCCESS          The controller name is successfully retrieved from Component Name (2) protocol
                               interface.
  @retval Other                The controller name cannot be retrieved from Component Name (2) protocol.

**/
EFI_STATUS
DriverHealthGetControllerName (
  IN  EFI_HANDLE  DriverBindingHandle,
  IN  EFI_HANDLE  ControllerHandle,
  IN  EFI_HANDLE  ChildHandle,
  OUT CHAR16      **ControllerName
  )
{
  EFI_STATUS      Status;

  //
  // Get controller name from UEFI 2.0 Component Name 2 protocol interface.
  //
  Status = GetControllerNameWorker (
             &gEfiComponentName2ProtocolGuid,
             DriverBindingHandle,
             ControllerHandle,
             ChildHandle,
             ControllerName
             );
  if (EFI_ERROR (Status)) {
    //
    // If it fails to get the controller name from Component Name protocol interface, we should fall back on
    // EFI 1.1 Component Name protocol interface.
    //
    Status = GetControllerNameWorker (
               &gEfiComponentNameProtocolGuid,
               DriverBindingHandle,
               ControllerHandle,
               ChildHandle,
               ControllerName
               );
  }

  return Status;
}
