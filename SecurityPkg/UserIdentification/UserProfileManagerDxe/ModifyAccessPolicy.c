/** @file
  The functions for access policy modification.
    
Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UserProfileManager.h"

/**
  Collect all the access policy data to mUserInfo.AccessPolicy, 
  and save it to user profile.

**/
VOID
SaveAccessPolicy (
  VOID
  )
{
  EFI_STATUS                    Status;
  UINTN                         OffSet;
  UINTN                         Size;
  EFI_USER_INFO_ACCESS_CONTROL  Control;
  EFI_USER_INFO_HANDLE          UserInfo;
  EFI_USER_INFO                 *Info;

  if (mUserInfo.AccessPolicy != NULL) {
    FreePool (mUserInfo.AccessPolicy);
  }
  mUserInfo.AccessPolicy          = NULL;
  mUserInfo.AccessPolicyLen       = 0;
  mUserInfo.AccessPolicyModified  = TRUE;
  OffSet                          = 0;
  
  //
  // Save access right.
  //
  Size = sizeof (EFI_USER_INFO_ACCESS_CONTROL);
  if (mUserInfo.AccessPolicyLen - OffSet < Size) {
    ExpandMemory (OffSet, Size);
  }

  Control.Type = mAccessInfo.AccessRight;
  Control.Size = (UINT32) Size;
  CopyMem (mUserInfo.AccessPolicy + OffSet, &Control, sizeof (Control));
  OffSet += sizeof (Control);
  
  //
  // Save access setup.
  //
  Size = sizeof (EFI_USER_INFO_ACCESS_CONTROL) + sizeof (EFI_GUID);
  if (mUserInfo.AccessPolicyLen - OffSet < Size) {
    ExpandMemory (OffSet, Size);
  }

  Control.Type = EFI_USER_INFO_ACCESS_SETUP;
  Control.Size = (UINT32) Size;  
  CopyMem (mUserInfo.AccessPolicy + OffSet, &Control, sizeof (Control));
  OffSet += sizeof (Control);
  
  if (mAccessInfo.AccessSetup == ACCESS_SETUP_NORMAL) {
    CopyGuid ((EFI_GUID *) (mUserInfo.AccessPolicy + OffSet), &gEfiUserInfoAccessSetupNormalGuid);
  } else if (mAccessInfo.AccessSetup == ACCESS_SETUP_RESTRICTED) {
    CopyGuid ((EFI_GUID *) (mUserInfo.AccessPolicy + OffSet), &gEfiUserInfoAccessSetupRestrictedGuid);
  } else if (mAccessInfo.AccessSetup == ACCESS_SETUP_ADMIN) {
    CopyGuid ((EFI_GUID *) (mUserInfo.AccessPolicy + OffSet), &gEfiUserInfoAccessSetupAdminGuid);
  }
  OffSet += sizeof (EFI_GUID);
  
  //
  // Save access of boot order.
  //
  Size = sizeof (EFI_USER_INFO_ACCESS_CONTROL) + sizeof (UINT32);
  if (mUserInfo.AccessPolicyLen - OffSet < Size) {
    ExpandMemory (OffSet, Size);
  }

  Control.Type = EFI_USER_INFO_ACCESS_BOOT_ORDER;
  Control.Size = (UINT32) Size;  
  CopyMem (mUserInfo.AccessPolicy + OffSet, &Control, sizeof (Control));
  OffSet += sizeof (Control);

  CopyMem ((UINT8 *) (mUserInfo.AccessPolicy + OffSet), &mAccessInfo.AccessBootOrder, sizeof (UINT32));
  OffSet += sizeof (UINT32);
  
  //
  // Save permit load.
  //
  if (mAccessInfo.LoadPermitLen > 0) {
    Size = sizeof (EFI_USER_INFO_ACCESS_CONTROL) + mAccessInfo.LoadPermitLen;
    if (mUserInfo.AccessPolicyLen - OffSet < Size) {
      ExpandMemory (OffSet, Size);
    }

    Control.Type = EFI_USER_INFO_ACCESS_PERMIT_LOAD;
    Control.Size = (UINT32) Size;  
    CopyMem (mUserInfo.AccessPolicy + OffSet, &Control, sizeof (Control));
    OffSet += sizeof (Control);
  
    CopyMem (mUserInfo.AccessPolicy + OffSet, mAccessInfo.LoadPermit, mAccessInfo.LoadPermitLen);
    OffSet += mAccessInfo.LoadPermitLen;
  }
  
  //
  // Save forbid load.
  //
  if (mAccessInfo.LoadForbidLen > 0) {
    Size = sizeof (EFI_USER_INFO_ACCESS_CONTROL) + mAccessInfo.LoadForbidLen;
    if (mUserInfo.AccessPolicyLen - OffSet < Size) {
      ExpandMemory (OffSet, Size);
    }

    Control.Type = EFI_USER_INFO_ACCESS_FORBID_LOAD;
    Control.Size = (UINT32) Size;  
    CopyMem (mUserInfo.AccessPolicy + OffSet, &Control, sizeof (Control));
    OffSet += sizeof (Control);
    
    CopyMem (mUserInfo.AccessPolicy + OffSet, mAccessInfo.LoadForbid, mAccessInfo.LoadForbidLen);
    OffSet += mAccessInfo.LoadForbidLen;
  }
  
  //
  // Save permit connect.
  //
  if (mAccessInfo.ConnectPermitLen > 0) {
    Size = sizeof (EFI_USER_INFO_ACCESS_CONTROL) + mAccessInfo.ConnectPermitLen;
    if (mUserInfo.AccessPolicyLen - OffSet < Size) {
      ExpandMemory (OffSet, Size);
    }

    Control.Type = EFI_USER_INFO_ACCESS_PERMIT_CONNECT;
    Control.Size = (UINT32) Size;  
    CopyMem (mUserInfo.AccessPolicy + OffSet, &Control, sizeof (Control));
    OffSet += sizeof (Control);
    
    CopyMem (mUserInfo.AccessPolicy + OffSet, mAccessInfo.ConnectPermit, mAccessInfo.ConnectPermitLen);
    OffSet += mAccessInfo.ConnectPermitLen;
  }
  
  //
  // Save forbid connect.
  //
  if (mAccessInfo.ConnectForbidLen > 0) {
    Size = sizeof (EFI_USER_INFO_ACCESS_CONTROL) + mAccessInfo.ConnectForbidLen;
    if (mUserInfo.AccessPolicyLen - OffSet < Size) {
      ExpandMemory (OffSet, Size);
    }

    Control.Type = EFI_USER_INFO_ACCESS_FORBID_CONNECT;
    Control.Size = (UINT32) Size;  
    CopyMem (mUserInfo.AccessPolicy + OffSet, &Control, sizeof (Control));
    OffSet += sizeof (Control);
    
    CopyMem (mUserInfo.AccessPolicy + OffSet, mAccessInfo.ConnectForbid, mAccessInfo.ConnectForbidLen);
    OffSet += mAccessInfo.ConnectForbidLen;
  }

  mUserInfo.AccessPolicyLen = OffSet;

  //
  // Save access policy.
  //
  if (mUserInfo.AccessPolicyModified && (mUserInfo.AccessPolicyLen > 0) && (mUserInfo.AccessPolicy != NULL)) {
    Info = AllocateZeroPool (sizeof (EFI_USER_INFO) + mUserInfo.AccessPolicyLen);
    if (Info == NULL) {
      return ;
    }

    Status = FindInfoByType (mModifyUser, EFI_USER_INFO_ACCESS_POLICY_RECORD, &UserInfo);
    if (!EFI_ERROR (Status)) {
      Info->InfoType    = EFI_USER_INFO_ACCESS_POLICY_RECORD;
      Info->InfoAttribs = EFI_USER_INFO_STORAGE_PLATFORM_NV |
                          EFI_USER_INFO_PUBLIC |
                          EFI_USER_INFO_EXCLUSIVE;
      Info->InfoSize    = (UINT32) (sizeof (EFI_USER_INFO) + mUserInfo.AccessPolicyLen);
      CopyMem ((UINT8 *) (Info + 1), mUserInfo.AccessPolicy, mUserInfo.AccessPolicyLen);
      Status = mUserManager->SetInfo (
                               mUserManager,
                               mModifyUser,
                               &UserInfo,
                               Info,
                               Info->InfoSize
                               );
      mUserInfo.AccessPolicyModified = FALSE;
    }
    FreePool (Info);
  }

  if (mAccessInfo.ConnectForbid != NULL) {
    FreePool (mAccessInfo.ConnectForbid);
    mAccessInfo.ConnectForbid = NULL;
  }

  if (mAccessInfo.ConnectPermit != NULL) {
    FreePool (mAccessInfo.ConnectPermit);
    mAccessInfo.ConnectPermit = NULL;
  }

  if (mAccessInfo.LoadForbid != NULL) {
    FreePool (mAccessInfo.LoadForbid);
    mAccessInfo.LoadForbid = NULL;
  }

  if (mAccessInfo.LoadPermit != NULL) {
    FreePool (mAccessInfo.LoadPermit);
    mAccessInfo.LoadPermit = NULL;
  }
}

/**
  Create an action OpCode with QuestionID and DevicePath on a given OpCodeHandle.

  @param[in]  QuestionID            The question ID.
  @param[in]  DevicePath            Points to device path.
  @param[in]  OpCodeHandle          Points to container for dynamic created opcodes.

**/
VOID
AddDevicePath (
  IN  UINTN                                     QuestionID,
  IN  EFI_DEVICE_PATH_PROTOCOL                  *DevicePath,
  IN     VOID                                   *OpCodeHandle
  )
{
  EFI_STATUS                        Status;
  EFI_DEVICE_PATH_PROTOCOL          *Next;
  EFI_STRING_ID                     NameID;
  EFI_STRING                        DriverName;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL  *DevicePathText;

  //
  // Locate device path to text protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiDevicePathToTextProtocolGuid,
                  NULL,
                  (VOID **) &DevicePathText
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }
  
  //
  // Get driver file name node.
  //
  Next = DevicePath;
  while (!IsDevicePathEnd (Next)) {
    DevicePath  = Next;
    Next        = NextDevicePathNode (Next);
  }

  //
  // Display the device path in form.
  //
  DriverName = DevicePathText->ConvertDevicePathToText (DevicePath, FALSE, FALSE);
  NameID = HiiSetString (mCallbackInfo->HiiHandle, 0, DriverName, NULL);
  FreePool (DriverName);
  if (NameID == 0) {
    return ;
  }

  HiiCreateActionOpCode (
    OpCodeHandle,                   // Container for dynamic created opcodes
    (UINT16) QuestionID,            // Question ID
    NameID,                         // Prompt text
    STRING_TOKEN (STR_NULL_STRING), // Help text
    EFI_IFR_FLAG_CALLBACK,          // Question flag
    0                               // Action String ID
    );
}


/**
  Check whether the DevicePath is in the device path forbid list 
  (mAccessInfo.LoadForbid).

  @param[in]  DevicePath           Points to device path.
  
  @retval TRUE     The DevicePath is in the device path forbid list.
  @retval FALSE    The DevicePath is not in the device path forbid list.

**/
BOOLEAN
IsLoadForbidden (
  IN  EFI_DEVICE_PATH_PROTOCOL                  *DevicePath
  )
{
  UINTN                     OffSet;
  UINTN                     DPSize;
  UINTN                     Size;
  EFI_DEVICE_PATH_PROTOCOL  *Dp;

  OffSet = 0;
  Size   = GetDevicePathSize (DevicePath);
  //
  // Check each device path.
  //
  while (OffSet < mAccessInfo.LoadForbidLen) {
    Dp      = (EFI_DEVICE_PATH_PROTOCOL *) (mAccessInfo.LoadForbid + OffSet);
    DPSize  = GetDevicePathSize (Dp);
    //
    // Compare device path.
    //
    if ((DPSize == Size) && (CompareMem (DevicePath, Dp, Size) == 0)) {
      return TRUE;
    }
    OffSet += DPSize;
  }
  return FALSE;
}


/**
  Display the permit load device path in the loadable device path list.

**/
VOID
DisplayLoadPermit(
  VOID
  )
{
  EFI_STATUS          Status;
  CHAR16              *Order;
  UINTN               OrderSize;
  UINTN               ListCount;
  UINTN               Index;
  UINT8               *Var;
  UINT8               *VarPtr;
  CHAR16              VarName[12];
  VOID                *StartOpCodeHandle;
  VOID                *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL  *StartLabel;
  EFI_IFR_GUID_LABEL  *EndLabel;

  //
  // Get DriverOrder.
  //
  OrderSize = 0;
  Status    = gRT->GetVariable (
                     L"DriverOrder", 
                     &gEfiGlobalVariableGuid, 
                     NULL, 
                     &OrderSize, 
                     NULL
                     );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return ;
  }

  Order = AllocateZeroPool (OrderSize);
  if (Order == NULL) {
    return ;
  }

  Status = gRT->GetVariable (
                  L"DriverOrder", 
                  &gEfiGlobalVariableGuid, 
                  NULL, 
                  &OrderSize, 
                  Order
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }
  
  //
  // Initialize the container for dynamic opcodes.
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode.
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                        StartOpCodeHandle,
                                        &gEfiIfrTianoGuid,
                                        NULL,
                                        sizeof (EFI_IFR_GUID_LABEL)
                                        );
  StartLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number        = LABEL_PERMIT_LOAD_FUNC;

  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                      EndOpCodeHandle,
                                      &gEfiIfrTianoGuid,
                                      NULL,
                                      sizeof (EFI_IFR_GUID_LABEL)
                                      );
  EndLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number        = LABEL_END;

  //
  // Add each driver option.
  //
  Var       = NULL;
  ListCount = OrderSize / sizeof (UINT16);
  for (Index = 0; Index < ListCount; Index++) {
    //
    // Get driver device path.
    //
    UnicodeSPrint (VarName, sizeof (VarName), L"Driver%04x", Order[Index]);
    GetEfiGlobalVariable2 (VarName, (VOID**)&Var, NULL);
    if (Var == NULL) {
      continue;
    }
    
    //
    // Check whether the driver is already forbidden.
    //
    
    VarPtr = Var;
    //
    // Skip attribute.
    //
    VarPtr += sizeof (UINT32);

    //
    // Skip device path lenth.
    //
    VarPtr += sizeof (UINT16);

    //
    // Skip descript string.
    //
    VarPtr += StrSize ((UINT16 *) VarPtr);

    if (IsLoadForbidden ((EFI_DEVICE_PATH_PROTOCOL *) VarPtr)) {
      FreePool (Var);
      Var = NULL;
      continue;
    }

    AddDevicePath (
      KEY_MODIFY_USER | KEY_MODIFY_AP_DP | KEY_LOAD_PERMIT_MODIFY | Order[Index],
      (EFI_DEVICE_PATH_PROTOCOL *) VarPtr,
      StartOpCodeHandle
      );
    FreePool (Var);
    Var = NULL;
  }

  HiiUpdateForm (
    mCallbackInfo->HiiHandle, // HII handle
    &gUserProfileManagerGuid, // Formset GUID
    FORMID_PERMIT_LOAD_DP,    // Form ID
    StartOpCodeHandle,        // Label for where to insert opcodes
    EndOpCodeHandle           // Replace data
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  //
  // Clear Environment.
  //
  if (Var != NULL) {
    FreePool (Var);
  }
  FreePool (Order);
}


/**
  Display the forbid load device path list (mAccessInfo.LoadForbid).

**/
VOID
DisplayLoadForbid (
  VOID
  )
{
  UINTN                     Offset;
  UINTN                     DPSize;
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL  *Dp;
  VOID                      *StartOpCodeHandle;
  VOID                      *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL        *StartLabel;
  EFI_IFR_GUID_LABEL        *EndLabel;

  //
  // Initialize the container for dynamic opcodes.
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode.
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                        StartOpCodeHandle,
                                        &gEfiIfrTianoGuid,
                                        NULL,
                                        sizeof (EFI_IFR_GUID_LABEL)
                                        );
  StartLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number        = LABLE_FORBID_LOAD_FUNC;

  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                      EndOpCodeHandle,
                                      &gEfiIfrTianoGuid,
                                      NULL,
                                      sizeof (EFI_IFR_GUID_LABEL)
                                      );
  EndLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number        = LABEL_END;

  //
  // Add each forbid load drivers.
  //
  Offset  = 0;
  Index   = 0;
  while (Offset < mAccessInfo.LoadForbidLen) {
    Dp      = (EFI_DEVICE_PATH_PROTOCOL *) (mAccessInfo.LoadForbid + Offset);
    DPSize  = GetDevicePathSize (Dp);
    AddDevicePath (
      KEY_MODIFY_USER | KEY_MODIFY_AP_DP | KEY_LOAD_FORBID_MODIFY | Index,
      Dp,
      StartOpCodeHandle
      );
    Index++;
    Offset += DPSize;
  }

  HiiUpdateForm (
    mCallbackInfo->HiiHandle, // HII handle
    &gUserProfileManagerGuid, // Formset GUID
    FORMID_FORBID_LOAD_DP,    // Form ID
    StartOpCodeHandle,        // Label for where to insert opcodes
    EndOpCodeHandle           // Replace data
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
}


/**
  Display the permit connect device path.

**/
VOID
DisplayConnectPermit (
  VOID
  )
{
  //
  // Note: 
  // As no architect protocol/interface to be called in ConnectController()
  // to verify the device path, just add a place holder for permitted connect
  // device path.
  //
}


/**
  Display the forbid connect device path list.

**/
VOID
DisplayConnectForbid (
  VOID
  )
{
  //
  // Note: 
  // As no architect protocol/interface to be called in ConnectController()
  // to verify the device path, just add a place holder for forbidden connect
  // device path.
  //
}


/**
  Delete the specified device path by DriverIndex from the forbid device path 
  list (mAccessInfo.LoadForbid).

  @param[in]  DriverIndex   The index of driver in forbidden device path list.
  
**/
VOID
DeleteFromForbidLoad (
  IN  UINT16                                    DriverIndex
  )
{
  UINTN                     OffSet;
  UINTN                     DPSize;
  UINTN                     OffLen;
  EFI_DEVICE_PATH_PROTOCOL  *Dp;

  OffSet = 0;
  //
  // Find the specified device path.
  //
  while ((OffSet < mAccessInfo.LoadForbidLen) && (DriverIndex > 0)) {
    Dp      = (EFI_DEVICE_PATH_PROTOCOL *) (mAccessInfo.LoadForbid + OffSet);
    DPSize  = GetDevicePathSize (Dp);
    OffSet += DPSize;
    DriverIndex--;
  }
  
  //
  // Specified device path found.
  //
  if (DriverIndex == 0) {
    Dp      = (EFI_DEVICE_PATH_PROTOCOL *) (mAccessInfo.LoadForbid + OffSet);
    DPSize  = GetDevicePathSize (Dp);
    OffLen  = mAccessInfo.LoadForbidLen - OffSet - DPSize;
    if (OffLen > 0) {
      CopyMem (
        mAccessInfo.LoadForbid + OffSet, 
        mAccessInfo.LoadForbid + OffSet + DPSize, 
        OffLen
        );
    }
    mAccessInfo.LoadForbidLen -= DPSize;
  }
}


/**
  Add the specified device path by DriverIndex to the forbid device path 
  list (mAccessInfo.LoadForbid).

  @param[in]  DriverIndex   The index of driver saved in driver options.
  
**/
VOID
AddToForbidLoad (
  IN  UINT16                                    DriverIndex
  )
{
  UINTN       DevicePathLen;
  UINT8       *Var;
  UINT8       *VarPtr;
  UINTN       NewLen;
  UINT8       *NewFL;
  CHAR16      VarName[13];

  //
  // Get loadable driver device path.
  //
  UnicodeSPrint  (VarName, sizeof (VarName), L"Driver%04x", DriverIndex);
  GetEfiGlobalVariable2 (VarName, (VOID**)&Var, NULL);
  if (Var == NULL) {
    return;
  }
  
  //
  // Save forbid load driver.
  //
  
  VarPtr = Var;
  //
  // Skip attribute.
  //
  VarPtr += sizeof (UINT32);

  DevicePathLen = *(UINT16 *) VarPtr;
  //
  // Skip device path length.
  //
  VarPtr += sizeof (UINT16);

  //
  // Skip description string.
  //
  VarPtr += StrSize ((UINT16 *) VarPtr);

  NewLen  = mAccessInfo.LoadForbidLen + DevicePathLen;
  NewFL   = AllocateZeroPool (NewLen);
  if (NewFL == NULL) {
    FreePool (Var);
    return ;
  }

  if (mAccessInfo.LoadForbidLen > 0) {
    CopyMem (NewFL, mAccessInfo.LoadForbid, mAccessInfo.LoadForbidLen);
    FreePool (mAccessInfo.LoadForbid);
  }

  CopyMem (NewFL + mAccessInfo.LoadForbidLen, VarPtr, DevicePathLen);
  mAccessInfo.LoadForbidLen = NewLen;
  mAccessInfo.LoadForbid    = NewFL;
  FreePool (Var);
}


