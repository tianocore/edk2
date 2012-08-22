/** @file
  Implement defer image load services for user identification in UEFI2.2.

Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeDeferImageLoadLib.h"

//
// Handle for the Deferred Image Load Protocol instance produced by this driver.
//
EFI_HANDLE                       mDeferredImageHandle = NULL;
BOOLEAN                          mIsProtocolInstalled = FALSE;
EFI_USER_MANAGER_PROTOCOL        *mUserManager        = NULL;
DEFERRED_IMAGE_TABLE             mDeferredImage       = {
  0,       // Deferred image count
  NULL     // The deferred image info
};

EFI_DEFERRED_IMAGE_LOAD_PROTOCOL gDeferredImageLoad   = {
  GetDefferedImageInfo
};

/**
  Get the image type.

  @param[in]    File    This is a pointer to the device path of the file
                        that is being dispatched. 

  @return       UINT32  Image Type             

**/
UINT32
GetFileType (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL   *File
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        DeviceHandle; 
  EFI_DEVICE_PATH_PROTOCOL          *TempDevicePath;
  EFI_BLOCK_IO_PROTOCOL             *BlockIo;

  //
  // First check to see if File is from a Firmware Volume
  //
  DeviceHandle      = NULL;
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *)File;
  Status = gBS->LocateDevicePath (
                  &gEfiFirmwareVolume2ProtocolGuid,
                  &TempDevicePath,
                  &DeviceHandle
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    DeviceHandle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    NULL,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      return IMAGE_FROM_FV;
    }
  }

  //
  // Next check to see if File is from a Block I/O device
  //
  DeviceHandle   = NULL;
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *)File;
  Status = gBS->LocateDevicePath (
                  &gEfiBlockIoProtocolGuid,
                  &TempDevicePath,
                  &DeviceHandle
                  );
  if (!EFI_ERROR (Status)) {
    BlockIo = NULL;
    Status = gBS->OpenProtocol (
                    DeviceHandle,
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlockIo,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status) && BlockIo != NULL) {
      if (BlockIo->Media != NULL) {
        if (BlockIo->Media->RemovableMedia) {
          //
          // Block I/O is present and specifies the media is removable
          //
          return IMAGE_FROM_REMOVABLE_MEDIA;
        } else {
          //
          // Block I/O is present and specifies the media is not removable
          //
          return IMAGE_FROM_FIXED_MEDIA;
        }
      }
    }
  }

  //
  // File is not in a Firmware Volume or on a Block I/O device, so check to see if 
  // the device path supports the Simple File System Protocol.
  //
  DeviceHandle   = NULL;
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *)File;
  Status = gBS->LocateDevicePath (
                  &gEfiSimpleFileSystemProtocolGuid,
                  &TempDevicePath,
                  &DeviceHandle
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Simple File System is present without Block I/O, so assume media is fixed.
    //
    return IMAGE_FROM_FIXED_MEDIA;
  }

  //
  // File is not from an FV, Block I/O or Simple File System, so the only options
  // left are a PCI Option ROM and a Load File Protocol such as a PXE Boot from a NIC.  
  //
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *)File;
  while (!IsDevicePathEndType (TempDevicePath)) {
    switch (DevicePathType (TempDevicePath)) {
    
    case MEDIA_DEVICE_PATH:
      if (DevicePathSubType (TempDevicePath) == MEDIA_RELATIVE_OFFSET_RANGE_DP) {
        return IMAGE_FROM_OPTION_ROM;
      }
      break;

    case MESSAGING_DEVICE_PATH:
      if (DevicePathSubType(TempDevicePath) == MSG_MAC_ADDR_DP) {
        return IMAGE_FROM_REMOVABLE_MEDIA;
      } 
      break;

    default:
      break;
    }
    TempDevicePath = NextDevicePathNode (TempDevicePath);
  }
  return IMAGE_UNKNOWN; 
}


/**
  Get current user's access right.

  @param[out]  AccessControl Points to the user's access control data, the
                             caller should free data buffer.
  @param[in]   AccessType    The type of user access control.

  @retval      EFI_SUCCESS   Get current user access control successfully
  @retval      others        Fail to get current user access control

**/
EFI_STATUS
GetAccessControl (
  OUT  EFI_USER_INFO_ACCESS_CONTROL     **AccessControl,
  IN   UINT32                           AccessType
  )
{
  EFI_STATUS                    Status;
  EFI_USER_INFO_HANDLE          UserInfo;
  EFI_USER_INFO                 *Info;
  UINTN                         InfoSize;
  EFI_USER_INFO_ACCESS_CONTROL  *Access;
  EFI_USER_PROFILE_HANDLE       CurrentUser;
  UINTN                         CheckLen;
  EFI_USER_MANAGER_PROTOCOL     *UserManager;

  CurrentUser = NULL;
  Status = gBS->LocateProtocol (
                  &gEfiUserManagerProtocolGuid,
                  NULL,
                  (VOID **) &UserManager
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }
  
  //
  // Get current user access information.
  //
  UserManager->Current (UserManager, &CurrentUser);

  UserInfo = NULL;
  Info     = NULL;
  InfoSize = 0;
  while (TRUE) {
    //
    // Get next user information.
    //
    Status = UserManager->GetNextInfo (UserManager, CurrentUser, &UserInfo);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = UserManager->GetInfo (
                            UserManager,
                            CurrentUser,
                            UserInfo,
                            Info,
                            &InfoSize
                            );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      if (Info != NULL) {
        FreePool (Info);
      }
      Info = AllocateZeroPool (InfoSize);
      ASSERT (Info != NULL);
      Status = UserManager->GetInfo (
                              UserManager,
                              CurrentUser,
                              UserInfo,
                              Info,
                              &InfoSize
                              );
    }

    if (EFI_ERROR (Status)) {
      break;
    }
    
    ASSERT (Info != NULL);
    if (Info->InfoType != EFI_USER_INFO_ACCESS_POLICY_RECORD) {
      continue;
    }
    
    //
    // Get specified access information.
    //
    CheckLen  = 0;
    while (CheckLen < Info->InfoSize - sizeof (EFI_USER_INFO)) {
      Access = (EFI_USER_INFO_ACCESS_CONTROL *) ((UINT8 *) (Info + 1) + CheckLen);
      if ((Access->Type == AccessType)) {
        *AccessControl = AllocateZeroPool (Access->Size);
        ASSERT (*AccessControl != NULL);
        CopyMem (*AccessControl, Access, Access->Size);
        FreePool (Info);
        return EFI_SUCCESS;
      }
      CheckLen += Access->Size;
    }
  }
  
  if (Info != NULL) {
    FreePool (Info);
  }
  return EFI_NOT_FOUND;
}


/**
  Convert the '/' to '\' in the specified string.

  @param[in, out]  Str       Points to the string to convert.

**/
VOID
ConvertDPStr (
  IN OUT EFI_STRING                     Str 
  )
{
  INTN                                  Count;
  INTN                                  Index;
  
  Count = StrSize(Str) / 2 - 1;

  if (Count < 4) {
    return;
  }
  
  //
  // Convert device path string.
  //
  Index = Count - 1;
  while (Index > 0) {
    //
    // Find the last '/'.
    //
    for (Index = Count - 1; Index > 0; Index--) {
      if (Str[Index] == L'/')
        break;
    }

    //
    // Check next char.
    //
    if (Str[Index + 1] == L'\\')
      return;
    
    Str[Index] = L'\\';
    
    //
    // Check previous char.
    //
    if ((Index > 0) && (Str[Index - 1] == L'\\')) {
      CopyMem (&Str[Index - 1], &Str[Index], (UINTN) ((Count - Index + 1) * sizeof (CHAR16)));
      return;
    }
    Index--;
  }
}


/**
  Check whether the DevicePath2 is identical with DevicePath1, or identical with
  DevicePath1's child device path.

  If DevicePath2 is identical with DevicePath1, or with DevicePath1's child device
  path, then TRUE returned. Otherwise, FALSE is returned.
  
  If DevicePath1 is NULL, then ASSERT().
  If DevicePath2 is NULL, then ASSERT().

  @param[in]  DevicePath1   A pointer to a device path.
  @param[in]  DevicePath2   A pointer to a device path.

  @retval     TRUE          Two device paths are identical , or DevicePath2 is 
                            DevicePath1's child device path.
  @retval     FALSE         Two device paths are not identical, and DevicePath2 
                            is not DevicePath1's child device path.

**/
BOOLEAN
CheckDevicePath (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL          *DevicePath1,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL          *DevicePath2
  )
{
  EFI_STATUS                            Status;
  EFI_STRING                            DevicePathStr1;
  EFI_STRING                            DevicePathStr2;
  UINTN                                 StrLen1;
  UINTN                                 StrLen2;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL      *DevicePathText;
  BOOLEAN                               DevicePathEqual;

  ASSERT (DevicePath1 != NULL);
  ASSERT (DevicePath2 != NULL);
  
  DevicePathEqual = FALSE;
  DevicePathText  = NULL;
  Status = gBS->LocateProtocol ( 
                  &gEfiDevicePathToTextProtocolGuid,
                  NULL,
                  (VOID **) &DevicePathText
                  );
  ASSERT (Status == EFI_SUCCESS);
  
  //
  // Get first device path string.
  //
  DevicePathStr1 = DevicePathText->ConvertDevicePathToText (DevicePath1, TRUE, TRUE);
  ConvertDPStr (DevicePathStr1);
  //
  // Get second device path string.
  //
  DevicePathStr2 = DevicePathText->ConvertDevicePathToText (DevicePath2, TRUE, TRUE);
  ConvertDPStr (DevicePathStr2);
  
  //
  // Compare device path string.
  //
  StrLen1 = StrSize (DevicePathStr1);
  StrLen2 = StrSize (DevicePathStr2);
  if (StrLen1 > StrLen2) {
    DevicePathEqual = FALSE;
    goto Done;
  }
  
  if (CompareMem (DevicePathStr1, DevicePathStr2, StrLen1) == 0) {
    DevicePathEqual = TRUE;
  }

Done:
  FreePool (DevicePathStr1);
  FreePool (DevicePathStr2);
  return DevicePathEqual;
}


/**
  Check whether the image pointed to by DevicePath is in the device path list 
  specified by AccessType.  

  @param[in] DevicePath  Points to device path.
  @param[in] AccessType  The type of user access control.
 
  @retval    TURE        The DevicePath is in the specified List.
  @retval    FALSE       The DevicePath is not in the specified List.

**/
BOOLEAN
IsDevicePathInList (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
  IN        UINT32                     AccessType
  )
{
  EFI_STATUS                            Status;
  EFI_USER_INFO_ACCESS_CONTROL          *Access;
  EFI_DEVICE_PATH_PROTOCOL              *Path;
  UINTN                                 OffSet;  

  Status = GetAccessControl (&Access, AccessType);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }  

  OffSet = 0;
  while (OffSet < Access->Size - sizeof (EFI_USER_INFO_ACCESS_CONTROL)) {
    Path = (EFI_DEVICE_PATH_PROTOCOL*)((UINT8*)(Access + 1) + OffSet);    
    if (CheckDevicePath (Path, DevicePath)) {
      //
      // The device path is found in list.
      //
      FreePool (Access);
      return TRUE;
    }  
    OffSet += GetDevicePathSize (Path);
  }
  
  FreePool (Access);
  return FALSE; 
}


/**
  Check whether the image pointed to by DevicePath is permitted to load.  

  @param[in] DevicePath  Points to device path
 
  @retval    TURE        The image pointed by DevicePath is permitted to load.
  @retval    FALSE       The image pointed by DevicePath is forbidden to load.

**/
BOOLEAN
VerifyDevicePath (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
{
  if (IsDevicePathInList (DevicePath, EFI_USER_INFO_ACCESS_PERMIT_LOAD)) {
    //
    // This access control overrides any restrictions put in place by the 
    // EFI_USER_INFO_ACCESS_FORBID_LOAD record.
    //
    return TRUE;
  }
  
  if (IsDevicePathInList (DevicePath, EFI_USER_INFO_ACCESS_FORBID_LOAD)) {
    //
    // The device path is found in the forbidden list.
    //
    return FALSE;
  }
  
  return TRUE; 
}


/**
  Check the image pointed by DevicePath is a boot option or not.  

  @param[in] DevicePath  Points to device path.
 
  @retval    TURE        The image pointed by DevicePath is a boot option.
  @retval    FALSE       The image pointed by DevicePath is not a boot option.

**/
BOOLEAN
IsBootOption (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL      *DevicePath
  )
{
  EFI_STATUS                        Status;
  UINT16                            *BootOrderList;
  UINTN                             BootOrderListSize;
  UINTN                             Index;
  CHAR16                            StrTemp[20];
  UINT8                             *OptionBuffer;
  UINT8                             *OptionPtr;
  EFI_DEVICE_PATH_PROTOCOL          *OptionDevicePath;
  
  //
  // Get BootOrder
  //
  BootOrderListSize = 0;
  BootOrderList     = NULL;  
  Status = gRT->GetVariable (
                  L"BootOrder", 
                  &gEfiGlobalVariableGuid, 
                  NULL, 
                  &BootOrderListSize, 
                  NULL
                  );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    BootOrderList = AllocateZeroPool (BootOrderListSize);
    ASSERT (BootOrderList != NULL);
    Status = gRT->GetVariable (
                    L"BootOrder", 
                    &gEfiGlobalVariableGuid, 
                    NULL, 
                    &BootOrderListSize, 
                    BootOrderList
                    );
  }
  
  if (EFI_ERROR (Status)) {
    //
    // No Boot option
    //
    return FALSE;
  }

  OptionBuffer = NULL;
  for (Index = 0; Index < BootOrderListSize / sizeof (UINT16); Index++) {
    //
    // Try to find the DevicePath in BootOption
    //
    UnicodeSPrint (StrTemp, sizeof (StrTemp), L"Boot%04x", Index);
    GetEfiGlobalVariable2 (StrTemp, (VOID**)&OptionBuffer, NULL);
    if (OptionBuffer == NULL) {
      continue;
    }

    //
    // Check whether the image is forbidden.
    //
    
    OptionPtr = OptionBuffer;
    //
    // Skip attribute.
    //
    OptionPtr += sizeof (UINT32);

    //
    // Skip device path length.
    //
    OptionPtr += sizeof (UINT16);

    //
    // Skip descript string
    //
    OptionPtr += StrSize ((UINT16 *) OptionPtr);
 
    //
    // Now OptionPtr points to Device Path.
    //
    OptionDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) OptionPtr;

    if (CheckDevicePath (DevicePath, OptionDevicePath)) {
      FreePool (OptionBuffer);
      OptionBuffer = NULL;
      return TRUE;
    }
    FreePool (OptionBuffer);
    OptionBuffer = NULL;
  }

  if (BootOrderList != NULL) {
    FreePool (BootOrderList);
  }

  return FALSE;
}


/**
  Add the image info to a deferred image list.

  @param[in]  ImageDevicePath  A pointer to the device path of a image.                                
  @param[in]  Image            Points to the first byte of the image, or NULL if the 
                               image is not available.
  @param[in]  ImageSize        The size of the image, or 0 if the image is not available.
  
**/
VOID
PutDefferedImageInfo (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL    *ImageDevicePath,
  IN        VOID                        *Image,
  IN        UINTN                       ImageSize
  )
{
  DEFERRED_IMAGE_INFO    *CurImageInfo;
  UINTN                  PathSize;

  //
  // Expand memory for the new deferred image.
  //
  if (mDeferredImage.Count == 0) {
    mDeferredImage.ImageInfo = AllocatePool (sizeof (DEFERRED_IMAGE_INFO));
    ASSERT (mDeferredImage.ImageInfo != NULL);
  } else {
    CurImageInfo = AllocatePool ((mDeferredImage.Count + 1) * sizeof (DEFERRED_IMAGE_INFO));
    ASSERT (CurImageInfo != NULL);
    
    CopyMem (
      CurImageInfo, 
      mDeferredImage.ImageInfo,
      mDeferredImage.Count * sizeof (DEFERRED_IMAGE_INFO)
      );
    FreePool (mDeferredImage.ImageInfo);
    mDeferredImage.ImageInfo = CurImageInfo;
  }
  mDeferredImage.Count++;
  
  //
  // Save the deferred image information.
  //
  CurImageInfo = &mDeferredImage.ImageInfo[mDeferredImage.Count - 1];
  PathSize     = GetDevicePathSize (ImageDevicePath);
  CurImageInfo->ImageDevicePath = AllocateZeroPool (PathSize);
  ASSERT (CurImageInfo->ImageDevicePath != NULL);
  CopyMem (CurImageInfo->ImageDevicePath, ImageDevicePath, PathSize);

  CurImageInfo->Image      = Image;
  CurImageInfo->ImageSize  = ImageSize;
  CurImageInfo->BootOption = IsBootOption (ImageDevicePath);
}


/**
  Returns information about a deferred image.

  This function returns information about a single deferred image. The deferred images are 
  numbered consecutively, starting with 0.  If there is no image which corresponds to 
  ImageIndex, then EFI_NOT_FOUND is returned. All deferred images may be returned by 
  iteratively calling this function until EFI_NOT_FOUND is returned.
  Image may be NULL and ImageSize set to 0 if the decision to defer execution was made 
  because of the location of the executable image, rather than its actual contents.  

  @param[in]  This             Points to this instance of the EFI_DEFERRED_IMAGE_LOAD_PROTOCOL.
  @param[in]  ImageIndex       Zero-based index of the deferred index.
  @param[out] ImageDevicePath  On return, points to a pointer to the device path of the image. 
                               The device path should not be freed by the caller. 
  @param[out] Image            On return, points to the first byte of the image or NULL if the 
                               image is not available. The image should not be freed by the caller
                               unless LoadImage() has been successfully called.  
  @param[out] ImageSize        On return, the size of the image, or 0 if the image is not available.
  @param[out] BootOption       On return, points to TRUE if the image was intended as a boot option 
                               or FALSE if it was not intended as a boot option. 
 
  @retval EFI_SUCCESS           Image information returned successfully.
  @retval EFI_NOT_FOUND         ImageIndex does not refer to a valid image.
  @retval EFI_INVALID_PARAMETER ImageDevicePath is NULL or Image is NULL or ImageSize is NULL or 
                                BootOption is NULL.
  
**/
EFI_STATUS
EFIAPI
GetDefferedImageInfo (
  IN     EFI_DEFERRED_IMAGE_LOAD_PROTOCOL  *This,
  IN     UINTN                             ImageIndex,
     OUT EFI_DEVICE_PATH_PROTOCOL          **ImageDevicePath,
     OUT VOID                              **Image,
     OUT UINTN                             *ImageSize,
     OUT BOOLEAN                           *BootOption
  )
{
  DEFERRED_IMAGE_INFO   *ReqImageInfo;

  //
  // Check the parameter.
  //

  if ((This == NULL) || (ImageSize == NULL) || (Image == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  if ((ImageDevicePath == NULL) || (BootOption == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (ImageIndex >= mDeferredImage.Count) {
    return EFI_NOT_FOUND;
  }
  
  //
  // Get the request deferred image.
  // 
  ReqImageInfo = &mDeferredImage.ImageInfo[ImageIndex];
   
  *ImageDevicePath = ReqImageInfo->ImageDevicePath;
  *Image           = ReqImageInfo->Image;
  *ImageSize       = ReqImageInfo->ImageSize;
  *BootOption      = ReqImageInfo->BootOption;
  
  return EFI_SUCCESS;
}


/**
  Provides the service of deferring image load based on platform policy control,
  and installs Deferred Image Load Protocol.

  @param[in]  AuthenticationStatus  This is the authentication status returned from the 
                                    security measurement services for the input file.
  @param[in]  File                  This is a pointer to the device path of the file that
                                    is being dispatched. This will optionally be used for
                                    logging.
  @param[in]  FileBuffer            File buffer matches the input file device path.
  @param[in]  FileSize              Size of File buffer matches the input file device path.
  @param[in]  BootPolicy            A boot policy that was used to call LoadImage() UEFI service.

  @retval EFI_SUCCESS               FileBuffer is NULL and current user has permission to start
                                    UEFI device drivers on the device path specified by DevicePath.
  @retval EFI_SUCCESS               The file specified by DevicePath and non-NULL
                                    FileBuffer did authenticate, and the platform policy dictates
                                    that the DXE Foundation may use the file.
  @retval EFI_SECURITY_VIOLATION    FileBuffer is NULL and the user has no
                                    permission to start UEFI device drivers on the device path specified
                                    by DevicePath.
  @retval EFI_SECURITY_VIOLATION    FileBuffer is not NULL and the user has no permission to load
                                    drivers from the device path specified by DevicePath. The
                                    image has been added into the list of the deferred images.
  @retval EFI_ACCESS_DENIED         The file specified by File and FileBuffer did not
                                    authenticate, and the platform policy dictates that the DXE
                                    Foundation many not use File.

**/
EFI_STATUS
EFIAPI
DxeDeferImageLoadHandler (
  IN  UINT32                           AuthenticationStatus,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL   *File,
  IN  VOID                             *FileBuffer,
  IN  UINTN                            FileSize,
  IN  BOOLEAN                          BootPolicy
  )
{
  EFI_STATUS                           Status;
  EFI_USER_PROFILE_HANDLE              CurrentUser;
  UINT32                               Policy;
  UINT32                               FileType;

  //
  // Ignore if File is NULL.
  //
  if (File == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Check whether user has a logon.
  // 
  CurrentUser = NULL;
  if (mUserManager != NULL) {
    mUserManager->Current (mUserManager, &CurrentUser);
    if (CurrentUser != NULL) {
      //
      // The user is logon; verify the FilePath by current user access policy.
      //
      if (!VerifyDevicePath (File)) {
        DEBUG ((EFI_D_ERROR, "[Security] The image is forbidden to load!\n"));
        return EFI_SECURITY_VIOLATION;
      }
      return EFI_SUCCESS;
    }
  }
  
  //
  // Still no user logon.
  // Check the file type and get policy setting.
  //
  FileType = GetFileType (File);
  Policy   = PcdGet32 (PcdDeferImageLoadPolicy);
  if ((Policy & FileType) == FileType) {
    //
    // This file type is secure to load.
    //
    return EFI_SUCCESS;
  }
 
  DEBUG ((EFI_D_ERROR, "[Security] No user identified, the image is deferred to load!\n"));
  PutDefferedImageInfo (File, FileBuffer, FileSize);

  //
  // Install the Deferred Image Load Protocol onto a new handle.
  //
  if (!mIsProtocolInstalled) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &mDeferredImageHandle,
                    &gEfiDeferredImageLoadProtocolGuid,
                    &gDeferredImageLoad,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
    mIsProtocolInstalled = TRUE;
  }

  return EFI_ACCESS_DENIED;
}

/**
  Locate user manager protocol when user manager is installed.  

  @param[in] Event    The Event that is being processed, not used.
  @param[in] Context  Event Context, not used. 

**/
VOID
EFIAPI
FindUserManagerProtocol (
  IN EFI_EVENT    Event,
  IN VOID*        Context
  )
{
  gBS->LocateProtocol (
         &gEfiUserManagerProtocolGuid,
         NULL,
         (VOID **) &mUserManager
         );
  
}


/**
  Register security handler for deferred image load.

  @param[in]  ImageHandle   ImageHandle of the loaded driver.
  @param[in]  SystemTable   Pointer to the EFI System Table.

  @retval EFI_SUCCESS   The handlers were registered successfully.
**/
EFI_STATUS
EFIAPI
DxeDeferImageLoadLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  VOID                 *Registration;
  
  //
  // Register user manager notification function.
  //
  EfiCreateProtocolNotifyEvent (
    &gEfiUserManagerProtocolGuid, 
    TPL_CALLBACK,
    FindUserManagerProtocol,
    NULL,
    &Registration
    );
  
  return RegisterSecurity2Handler (
           DxeDeferImageLoadHandler,
           EFI_AUTH_OPERATION_DEFER_IMAGE_LOAD 
           );      
}


