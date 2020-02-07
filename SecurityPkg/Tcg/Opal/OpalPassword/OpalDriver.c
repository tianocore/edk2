/** @file
  Entrypoint of Opal UEFI Driver and contains all the logic to
  register for new Opal device instances.

Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

// This UEFI driver consumes EFI_STORAGE_SECURITY_PROTOCOL instances and installs an
// HII GUI to manage Opal features if the device is Opal capable
// If the Opal device is being managed by the UEFI Driver, it shall provide a popup
// window during boot requesting a user password

#include "OpalDriver.h"
#include "OpalHii.h"

EFI_GUID mOpalDeviceLockBoxGuid = OPAL_DEVICE_LOCKBOX_GUID;

BOOLEAN                 mOpalEndOfDxe = FALSE;
OPAL_REQUEST_VARIABLE   *mOpalRequestVariable = NULL;
UINTN                   mOpalRequestVariableSize = 0;
CHAR16                  mPopUpString[100];

OPAL_DRIVER mOpalDriver;

//
// Globals
//
EFI_DRIVER_BINDING_PROTOCOL gOpalDriverBinding = {
  OpalEfiDriverBindingSupported,
  OpalEfiDriverBindingStart,
  OpalEfiDriverBindingStop,
  0x1b,
  NULL,
  NULL
};

/**

  The function determines the available actions for the OPAL_DISK provided.

  @param[in]   SupportedAttributes   The supported attributes for the device.
  @param[in]   LockingFeature        The locking status for the device.
  @param[in]   OwnerShip             The ownership for the device.
  @param[out]  AvalDiskActions       Pointer to fill-out with appropriate disk actions.

**/
TCG_RESULT
EFIAPI
OpalSupportGetAvailableActions(
  IN  OPAL_DISK_SUPPORT_ATTRIBUTE      *SupportedAttributes,
  IN  TCG_LOCKING_FEATURE_DESCRIPTOR   *LockingFeature,
  IN  UINT16                           OwnerShip,
  OUT OPAL_DISK_ACTIONS                *AvalDiskActions
  )
{
  BOOLEAN ExistingPassword;

  NULL_CHECK(AvalDiskActions);

  AvalDiskActions->AdminPass = 1;
  AvalDiskActions->UserPass = 0;
  AvalDiskActions->DisableUser = 0;
  AvalDiskActions->Unlock = 0;

  //
  // Revert is performed on locking sp, so only allow if locking sp is enabled
  //
  if (LockingFeature->LockingEnabled) {
    AvalDiskActions->Revert = 1;
  }

  //
  // Psid revert is available for any device with media encryption support or pyrite 2.0 type support.
  //
  if (SupportedAttributes->PyriteSscV2 || SupportedAttributes->MediaEncryption) {

    //
    // Only allow psid revert if media encryption is enabled or pyrite 2.0 type support..
    // Otherwise, someone who steals a disk can psid revert the disk and the user Data is still
    // intact and accessible
    //
    AvalDiskActions->PsidRevert = 1;
    AvalDiskActions->RevertKeepDataForced = 0;

    //
    // Secure erase is performed by generating a new encryption key
    // this is only available if encryption is supported
    //
    AvalDiskActions->SecureErase = 1;
  } else {
    AvalDiskActions->PsidRevert = 0;
    AvalDiskActions->SecureErase = 0;

    //
    // If no media encryption is supported, then a revert (using password) will not
    // erase the Data (since you can't generate a new encryption key)
    //
    AvalDiskActions->RevertKeepDataForced = 1;
  }

  if (LockingFeature->Locked) {
    AvalDiskActions->Unlock = 1;
  } else {
    AvalDiskActions->Unlock = 0;
  }

  //
  // Only allow user to set password if an admin password exists
  //
  ExistingPassword = OpalUtilAdminPasswordExists(OwnerShip, LockingFeature);
  AvalDiskActions->UserPass = ExistingPassword;

  //
  // This will still show up even if there isn't a user, which is fine
  //
  AvalDiskActions->DisableUser = ExistingPassword;

  return TcgResultSuccess;
}

/**
  Enable Opal Feature for the input device.

  @param[in]      Session            The opal session for the opal device.
  @param[in]      Msid               Msid
  @param[in]      MsidLength         Msid Length
  @param[in]      Password           Admin password
  @param[in]      PassLength         Length of password in bytes

**/
TCG_RESULT
EFIAPI
OpalSupportEnableOpalFeature (
  IN OPAL_SESSION              *Session,
  IN VOID                      *Msid,
  IN UINT32                    MsidLength,
  IN VOID                      *Password,
  IN UINT32                    PassLength
  )
{
  TCG_RESULT   Ret;

  NULL_CHECK(Session);
  NULL_CHECK(Msid);
  NULL_CHECK(Password);

  Ret = OpalUtilSetAdminPasswordAsSid(
                          Session,
                          Msid,
                          MsidLength,
                          Password,
                          PassLength
                          );
  if (Ret == TcgResultSuccess) {
    //
    // Enable global locking range
    //
    Ret = OpalUtilSetOpalLockingRange(
                              Session,
                              Password,
                              PassLength,
                              OPAL_LOCKING_SP_LOCKING_GLOBALRANGE,
                              0,
                              0,
                              TRUE,
                              TRUE,
                              FALSE,
                              FALSE
                              );
  }

  return Ret;
}

/**
  Update password for the Opal disk.

  @param[in, out] OpalDisk          The disk to update password.
  @param[in]      Password          The input password.
  @param[in]      PasswordLength    The input password length.

**/
VOID
OpalSupportUpdatePassword (
  IN OUT OPAL_DISK      *OpalDisk,
  IN VOID               *Password,
  IN UINT32             PasswordLength
  )
{
  CopyMem (OpalDisk->Password, Password, PasswordLength);
  OpalDisk->PasswordLength = (UINT8) PasswordLength;
}

/**
  Extract device info from the device path.

  @param[in]  DevicePath        Device path info for the device.
  @param[out] DevInfoLength     Device information length needed.
  @param[out] DevInfo           Device information extracted.

**/
VOID
ExtractDeviceInfoFromDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT UINT32                    *DevInfoLength,
  OUT OPAL_DEVICE_LOCKBOX_DATA  *DevInfo OPTIONAL
  )
{
  EFI_DEVICE_PATH_PROTOCOL      *TmpDevPath;
  EFI_DEVICE_PATH_PROTOCOL      *TmpDevPath2;
  PCI_DEVICE_PATH               *PciDevPath;
  UINT8                         DeviceType;
  UINT8                         BusNum;
  OPAL_PCI_DEVICE               *PciDevice;

  ASSERT (DevicePath != NULL);
  ASSERT (DevInfoLength != NULL);

  DeviceType = OPAL_DEVICE_TYPE_UNKNOWN;
  *DevInfoLength = 0;

  TmpDevPath = DevicePath;

  //
  // Get device type.
  //
  while (!IsDevicePathEnd (TmpDevPath)) {
    if ((TmpDevPath->Type == MESSAGING_DEVICE_PATH) &&
        (TmpDevPath->SubType == MSG_SATA_DP || TmpDevPath->SubType == MSG_NVME_NAMESPACE_DP)) {
      if (DevInfo != NULL) {
        DevInfo->DevicePathLength = (UINT32) GetDevicePathSize (DevicePath);
        CopyMem (DevInfo->DevicePath, DevicePath, DevInfo->DevicePathLength);
      }

      DeviceType = (TmpDevPath->SubType == MSG_SATA_DP) ? OPAL_DEVICE_TYPE_ATA : OPAL_DEVICE_TYPE_NVME;
      *DevInfoLength = sizeof (OPAL_DEVICE_LOCKBOX_DATA) + (UINT32) GetDevicePathSize (DevicePath);
      break;
    }
    TmpDevPath = NextDevicePathNode (TmpDevPath);
  }

  //
  // Get device info.
  //
  BusNum = 0;
  TmpDevPath = DevicePath;
  TmpDevPath2 = NextDevicePathNode (DevicePath);
  while (!IsDevicePathEnd (TmpDevPath2)) {
    if (TmpDevPath->Type == HARDWARE_DEVICE_PATH && TmpDevPath->SubType == HW_PCI_DP) {
      PciDevPath = (PCI_DEVICE_PATH *) TmpDevPath;
      if ((TmpDevPath2->Type == MESSAGING_DEVICE_PATH) &&
          (TmpDevPath2->SubType == MSG_SATA_DP || TmpDevPath2->SubType == MSG_NVME_NAMESPACE_DP)) {
        if (DevInfo != NULL) {
          PciDevice = &DevInfo->Device;
          PciDevice->Segment = 0;
          PciDevice->Bus = BusNum;
          PciDevice->Device = PciDevPath->Device;
          PciDevice->Function = PciDevPath->Function;
        }
      } else {
        if (TmpDevPath2->Type == HARDWARE_DEVICE_PATH && TmpDevPath2->SubType == HW_PCI_DP) {
          BusNum = PciRead8 (PCI_LIB_ADDRESS (BusNum, PciDevPath->Device, PciDevPath->Function, PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET));
        }
      }
    }

    TmpDevPath  = NextDevicePathNode (TmpDevPath);
    TmpDevPath2 = NextDevicePathNode (TmpDevPath2);
  }

  ASSERT (DeviceType != OPAL_DEVICE_TYPE_UNKNOWN);
  return;
}

/**
  Build OPAL device info and save them to LockBox.

 **/
VOID
BuildOpalDeviceInfo (
  VOID
  )
{
  EFI_STATUS                  Status;
  OPAL_DEVICE_LOCKBOX_DATA    *DevInfo;
  OPAL_DEVICE_LOCKBOX_DATA    *TempDevInfo;
  UINTN                       TotalDevInfoLength;
  UINT32                      DevInfoLength;
  OPAL_DRIVER_DEVICE          *TmpDev;
  UINT8                       DummyData;
  BOOLEAN                     S3InitDevicesExist;
  UINTN                       S3InitDevicesLength;
  EFI_DEVICE_PATH_PROTOCOL    *S3InitDevices;
  EFI_DEVICE_PATH_PROTOCOL    *S3InitDevicesBak;

  //
  // Build OPAL device info and save them to LockBox.
  //
  TotalDevInfoLength = 0;
  TmpDev = mOpalDriver.DeviceList;
  while (TmpDev != NULL) {
    ExtractDeviceInfoFromDevicePath (
      TmpDev->OpalDisk.OpalDevicePath,
      &DevInfoLength,
      NULL
      );
    TotalDevInfoLength += DevInfoLength;
    TmpDev = TmpDev->Next;
  }

  if (TotalDevInfoLength == 0) {
    return;
  }

  S3InitDevicesLength = sizeof (DummyData);
  Status = RestoreLockBox (
             &gS3StorageDeviceInitListGuid,
             &DummyData,
             &S3InitDevicesLength
             );
  ASSERT ((Status == EFI_NOT_FOUND) || (Status == EFI_BUFFER_TOO_SMALL));
  if (Status == EFI_NOT_FOUND) {
    S3InitDevices      = NULL;
    S3InitDevicesExist = FALSE;
  } else if (Status == EFI_BUFFER_TOO_SMALL) {
    S3InitDevices = AllocatePool (S3InitDevicesLength);
    ASSERT (S3InitDevices != NULL);
    if (S3InitDevices == NULL) {
      return;
    }

    Status = RestoreLockBox (
               &gS3StorageDeviceInitListGuid,
               S3InitDevices,
               &S3InitDevicesLength
               );
    ASSERT_EFI_ERROR (Status);
    S3InitDevicesExist = TRUE;
  } else {
    return;
  }

  DevInfo = AllocateZeroPool (TotalDevInfoLength);
  ASSERT (DevInfo != NULL);
  if (DevInfo == NULL) {
    return;
  }

  TempDevInfo = DevInfo;
  TmpDev      = mOpalDriver.DeviceList;
  while (TmpDev != NULL) {
    ExtractDeviceInfoFromDevicePath (
      TmpDev->OpalDisk.OpalDevicePath,
      &DevInfoLength,
      TempDevInfo
      );
    TempDevInfo->Length = DevInfoLength;
    TempDevInfo->OpalBaseComId = TmpDev->OpalDisk.OpalBaseComId;
    CopyMem (
      TempDevInfo->Password,
      TmpDev->OpalDisk.Password,
      TmpDev->OpalDisk.PasswordLength
      );
    TempDevInfo->PasswordLength = TmpDev->OpalDisk.PasswordLength;

    S3InitDevicesBak = S3InitDevices;
    S3InitDevices    = AppendDevicePathInstance (
                         S3InitDevicesBak,
                         TmpDev->OpalDisk.OpalDevicePath
                         );
    if (S3InitDevicesBak != NULL) {
      FreePool (S3InitDevicesBak);
    }
    ASSERT (S3InitDevices != NULL);
    if (S3InitDevices == NULL) {
      return;
    }

    TempDevInfo = (OPAL_DEVICE_LOCKBOX_DATA *) ((UINTN) TempDevInfo + DevInfoLength);
    TmpDev = TmpDev->Next;
  }

  Status = SaveLockBox (
             &mOpalDeviceLockBoxGuid,
             DevInfo,
             TotalDevInfoLength
             );
  ASSERT_EFI_ERROR (Status);

  Status = SetLockBoxAttributes (
             &mOpalDeviceLockBoxGuid,
             LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY
             );
  ASSERT_EFI_ERROR (Status);

  S3InitDevicesLength = GetDevicePathSize (S3InitDevices);
  if (S3InitDevicesExist) {
    Status = UpdateLockBox (
               &gS3StorageDeviceInitListGuid,
               0,
               S3InitDevices,
               S3InitDevicesLength
               );
    ASSERT_EFI_ERROR (Status);
  } else {
    Status = SaveLockBox (
               &gS3StorageDeviceInitListGuid,
               S3InitDevices,
               S3InitDevicesLength
               );
    ASSERT_EFI_ERROR (Status);

    Status = SetLockBoxAttributes (
               &gS3StorageDeviceInitListGuid,
               LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY
               );
    ASSERT_EFI_ERROR (Status);
  }

  ZeroMem (DevInfo, TotalDevInfoLength);
  FreePool (DevInfo);
  FreePool (S3InitDevices);
}

/**

  Send BlockSid command if needed.

**/
VOID
SendBlockSidCommand (
  VOID
  )
{
  OPAL_DRIVER_DEVICE                         *Itr;
  TCG_RESULT                                 Result;
  OPAL_SESSION                               Session;
  UINT32                                     PpStorageFlag;

  PpStorageFlag = Tcg2PhysicalPresenceLibGetManagementFlags ();
  if ((PpStorageFlag & TCG2_BIOS_STORAGE_MANAGEMENT_FLAG_ENABLE_BLOCK_SID) != 0) {
    //
    // Send BlockSID command to each Opal disk
    //
    Itr = mOpalDriver.DeviceList;
    while (Itr != NULL) {
      if (Itr->OpalDisk.SupportedAttributes.BlockSid) {
        ZeroMem(&Session, sizeof(Session));
        Session.Sscp = Itr->OpalDisk.Sscp;
        Session.MediaId = Itr->OpalDisk.MediaId;
        Session.OpalBaseComId = Itr->OpalDisk.OpalBaseComId;

        DEBUG ((DEBUG_INFO, "OpalPassword: EndOfDxe point, send BlockSid command to device!\n"));
        Result = OpalBlockSid (&Session, TRUE);  // HardwareReset must always be TRUE
        if (Result != TcgResultSuccess) {
          DEBUG ((DEBUG_ERROR, "OpalBlockSid fail\n"));
          break;
        }

        //
        // Record BlockSID command has been sent.
        //
        Itr->OpalDisk.SentBlockSID = TRUE;
      }

      Itr = Itr->Next;
    }
  }
}

/**
  Notification function of EFI_END_OF_DXE_EVENT_GROUP_GUID event group.

  This is a notification function registered on EFI_END_OF_DXE_EVENT_GROUP_GUID event group.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
OpalEndOfDxeEventNotify (
  EFI_EVENT                               Event,
  VOID                                    *Context
  )
{
  OPAL_DRIVER_DEVICE    *TmpDev;

  DEBUG ((DEBUG_INFO, "%a() - enter\n", __FUNCTION__));

  mOpalEndOfDxe = TRUE;

  if (mOpalRequestVariable != NULL) {
    //
    // Free the OPAL request variable buffer here
    // as the OPAL requests should have been processed.
    //
    FreePool (mOpalRequestVariable);
    mOpalRequestVariable = NULL;
    mOpalRequestVariableSize = 0;
  }

  //
  // If no any device, return directly.
  //
  if (mOpalDriver.DeviceList == NULL) {
    gBS->CloseEvent (Event);
    return;
  }

  BuildOpalDeviceInfo ();

  //
  // Zero passsword.
  //
  TmpDev = mOpalDriver.DeviceList;
  while (TmpDev != NULL) {
    ZeroMem (TmpDev->OpalDisk.Password, TmpDev->OpalDisk.PasswordLength);
    TmpDev = TmpDev->Next;
  }

  //
  // Send BlockSid command if needed.
  //
  SendBlockSidCommand ();

  DEBUG ((DEBUG_INFO, "%a() - exit\n", __FUNCTION__));

  gBS->CloseEvent (Event);
}

/**
  Get Psid input from the popup window.

  @param[in]  Dev           The device which need Psid to process Psid Revert
                            OPAL request.
  @param[in]  PopUpString   Pop up string.
  @param[in]  PopUpString2  Pop up string in line 2.
  @param[in]  PopUpString3  Pop up string in line 3.

  @param[out] PressEsc      Whether user escape function through Press ESC.

  @retval Psid string if success. NULL if failed.

**/
CHAR8 *
OpalDriverPopUpPsidInput (
  IN OPAL_DRIVER_DEVICE     *Dev,
  IN CHAR16                 *PopUpString,
  IN CHAR16                 *PopUpString2,
  IN CHAR16                 *PopUpString3,
  OUT BOOLEAN               *PressEsc
  )
{
  EFI_INPUT_KEY             InputKey;
  UINTN                     InputLength;
  CHAR16                    Mask[PSID_CHARACTER_LENGTH + 1];
  CHAR16                    Unicode[PSID_CHARACTER_LENGTH + 1];
  CHAR8                     *Ascii;

  ZeroMem(Unicode, sizeof(Unicode));
  ZeroMem(Mask, sizeof(Mask));

  *PressEsc = FALSE;

  gST->ConOut->ClearScreen(gST->ConOut);

  InputLength = 0;
  while (TRUE) {
    Mask[InputLength] = L'_';
    if (PopUpString2 == NULL) {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &InputKey,
        PopUpString,
        L"---------------------",
        Mask,
        NULL
      );
    } else {
      if (PopUpString3 == NULL) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &InputKey,
          PopUpString,
          PopUpString2,
          L"---------------------",
          Mask,
          NULL
        );
      } else {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &InputKey,
          PopUpString,
          PopUpString2,
          PopUpString3,
          L"---------------------",
          Mask,
          NULL
        );
      }
    }

    //
    // Check key.
    //
    if (InputKey.ScanCode == SCAN_NULL) {
      //
      // password finished
      //
      if (InputKey.UnicodeChar == CHAR_CARRIAGE_RETURN) {
        //
        // Add the null terminator.
        //
        Unicode[InputLength] = 0;
        Mask[InputLength] = 0;
        break;
      } else if ((InputKey.UnicodeChar == CHAR_NULL) ||
                 (InputKey.UnicodeChar == CHAR_TAB) ||
                 (InputKey.UnicodeChar == CHAR_LINEFEED)
                ) {
        continue;
      } else {
        //
        // delete last key entered
        //
        if (InputKey.UnicodeChar == CHAR_BACKSPACE) {
          if (InputLength > 0) {
            Unicode[InputLength] = 0;
            Mask[InputLength] = 0;
            InputLength--;
          }
        } else {
          //
          // add Next key entry
          //
          Unicode[InputLength] = InputKey.UnicodeChar;
          Mask[InputLength] = InputKey.UnicodeChar;
          InputLength++;
          if (InputLength == PSID_CHARACTER_LENGTH) {
            //
            // Add the null terminator.
            //
            Unicode[InputLength] = 0;
            Mask[InputLength] = 0;
            break;
          }
        }
      }
    }

    //
    // exit on ESC
    //
    if (InputKey.ScanCode == SCAN_ESC) {
      *PressEsc = TRUE;
      break;
    }
  }

  gST->ConOut->ClearScreen(gST->ConOut);

  if (InputLength == 0 || InputKey.ScanCode == SCAN_ESC) {
    ZeroMem (Unicode, sizeof (Unicode));
    ZeroMem (Mask, sizeof (Mask));
    return NULL;
  }

  Ascii = AllocateZeroPool (PSID_CHARACTER_LENGTH + 1);
  if (Ascii == NULL) {
    ZeroMem (Unicode, sizeof (Unicode));
    ZeroMem (Mask, sizeof (Mask));
    return NULL;
  }

  UnicodeStrToAsciiStrS (Unicode, Ascii, PSID_CHARACTER_LENGTH + 1);
  ZeroMem (Unicode, sizeof (Unicode));
  ZeroMem (Mask, sizeof (Mask));

  return Ascii;
}


/**
  Get password input from the popup window.

  @param[in]  Dev           The device which need password to unlock or
                            process OPAL request.
  @param[in]  PopUpString1  Pop up string 1.
  @param[in]  PopUpString2  Pop up string 2.
  @param[in]  PopUpString3  Pop up string 3.
  @param[out] PressEsc      Whether user escape function through Press ESC.

  @retval Password string if success. NULL if failed.

**/
CHAR8 *
OpalDriverPopUpPasswordInput (
  IN OPAL_DRIVER_DEVICE     *Dev,
  IN CHAR16                 *PopUpString1,
  IN CHAR16                 *PopUpString2,
  IN CHAR16                 *PopUpString3,
  OUT BOOLEAN               *PressEsc
  )
{
  EFI_INPUT_KEY             InputKey;
  UINTN                     InputLength;
  CHAR16                    Mask[OPAL_MAX_PASSWORD_SIZE + 1];
  CHAR16                    Unicode[OPAL_MAX_PASSWORD_SIZE + 1];
  CHAR8                     *Ascii;

  ZeroMem(Unicode, sizeof(Unicode));
  ZeroMem(Mask, sizeof(Mask));

  *PressEsc = FALSE;

  gST->ConOut->ClearScreen(gST->ConOut);

  InputLength = 0;
  while (TRUE) {
    Mask[InputLength] = L'_';
    if (PopUpString2 == NULL) {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &InputKey,
        PopUpString1,
        L"---------------------",
        Mask,
        NULL
      );
    } else {
      if (PopUpString3 == NULL) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &InputKey,
          PopUpString1,
          PopUpString2,
          L"---------------------",
          Mask,
          NULL
        );
      } else {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &InputKey,
          PopUpString1,
          PopUpString2,
          PopUpString3,
          L"---------------------",
          Mask,
          NULL
        );
      }
    }

    //
    // Check key.
    //
    if (InputKey.ScanCode == SCAN_NULL) {
      //
      // password finished
      //
      if (InputKey.UnicodeChar == CHAR_CARRIAGE_RETURN) {
        //
        // Add the null terminator.
        //
        Unicode[InputLength] = 0;
        Mask[InputLength] = 0;
        break;
      } else if ((InputKey.UnicodeChar == CHAR_NULL) ||
                 (InputKey.UnicodeChar == CHAR_TAB) ||
                 (InputKey.UnicodeChar == CHAR_LINEFEED)
                ) {
        continue;
      } else {
        //
        // delete last key entered
        //
        if (InputKey.UnicodeChar == CHAR_BACKSPACE) {
          if (InputLength > 0) {
            Unicode[InputLength] = 0;
            Mask[InputLength] = 0;
            InputLength--;
          }
        } else {
          //
          // add Next key entry
          //
          Unicode[InputLength] = InputKey.UnicodeChar;
          Mask[InputLength] = L'*';
          InputLength++;
          if (InputLength == OPAL_MAX_PASSWORD_SIZE) {
            //
            // Add the null terminator.
            //
            Unicode[InputLength] = 0;
            Mask[InputLength] = 0;
            break;
          }
        }
      }
    }

    //
    // exit on ESC
    //
    if (InputKey.ScanCode == SCAN_ESC) {
      *PressEsc = TRUE;
      break;
    }
  }

  gST->ConOut->ClearScreen(gST->ConOut);

  if (InputLength == 0 || InputKey.ScanCode == SCAN_ESC) {
    ZeroMem (Unicode, sizeof (Unicode));
    return NULL;
  }

  Ascii = AllocateZeroPool (OPAL_MAX_PASSWORD_SIZE + 1);
  if (Ascii == NULL) {
    ZeroMem (Unicode, sizeof (Unicode));
    return NULL;
  }

  UnicodeStrToAsciiStrS (Unicode, Ascii, OPAL_MAX_PASSWORD_SIZE + 1);
  ZeroMem (Unicode, sizeof (Unicode));

  return Ascii;
}

/**
  Get pop up string.

  @param[in] Dev            The OPAL device.
  @param[in] RequestString  Request string.

  @return Pop up string.

**/
CHAR16 *
OpalGetPopUpString (
  IN OPAL_DRIVER_DEVICE *Dev,
  IN CHAR16             *RequestString
  )
{
  if (Dev->Name16 == NULL) {
    UnicodeSPrint (mPopUpString, sizeof (mPopUpString), L"%s Disk", RequestString);
  } else {
    UnicodeSPrint (mPopUpString, sizeof (mPopUpString), L"%s %s", RequestString, Dev->Name16);
  }

  return mPopUpString;
}

/**
  Check if disk is locked, show popup window and ask for password if it is.

  @param[in] Dev            The device which need to be unlocked.
  @param[in] RequestString  Request string.

**/
VOID
OpalDriverRequestPassword (
  IN OPAL_DRIVER_DEVICE *Dev,
  IN CHAR16             *RequestString
  )
{
  UINT8                 Count;
  BOOLEAN               IsEnabled;
  BOOLEAN               IsLocked;
  CHAR8                 *Password;
  UINT32                PasswordLen;
  OPAL_SESSION          Session;
  BOOLEAN               PressEsc;
  EFI_INPUT_KEY         Key;
  TCG_RESULT            Ret;
  CHAR16                *PopUpString;

  if (Dev == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO, "%a()\n", __FUNCTION__));

  PopUpString = OpalGetPopUpString (Dev, RequestString);

  Count = 0;

  IsEnabled = OpalFeatureEnabled (&Dev->OpalDisk.SupportedAttributes, &Dev->OpalDisk.LockingFeature);
  if (IsEnabled) {
    ZeroMem(&Session, sizeof(Session));
    Session.Sscp = Dev->OpalDisk.Sscp;
    Session.MediaId = Dev->OpalDisk.MediaId;
    Session.OpalBaseComId = Dev->OpalDisk.OpalBaseComId;

    IsLocked = OpalDeviceLocked (&Dev->OpalDisk.SupportedAttributes, &Dev->OpalDisk.LockingFeature);

    //
    // Add PcdSkipOpalPasswordPrompt to determin whether to skip password prompt.
    // Due to board design, device may not power off during system warm boot, which result in
    // security status remain unlocked status, hence we add device security status check here.
    //
    // If device is in the locked status, device keeps locked and system continues booting.
    // If device is in the unlocked status, system is forced shutdown to support security requirement.
    //
    if (PcdGetBool (PcdSkipOpalPasswordPrompt)) {
      if (IsLocked) {
        return;
      } else {
        gRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);
      }
    }

    while (Count < MAX_PASSWORD_TRY_COUNT) {
      Password = OpalDriverPopUpPasswordInput (Dev, PopUpString, NULL, NULL, &PressEsc);
      if (PressEsc) {
        if (IsLocked) {
          //
          // Current device in the lock status and
          // User not input password and press ESC,
          // keep device in lock status and continue boot.
          //
          do {
            CreatePopUp (
              EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
              &Key,
              L"Press ENTER to skip the request and continue boot,",
              L"Press ESC to input password again",
              NULL
              );
          } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

          if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
            gST->ConOut->ClearScreen(gST->ConOut);
            //
            // Keep lock and continue boot.
            //
            return;
          } else {
            //
            // Let user input password again.
            //
            continue;
          }
        } else {
          //
          // Current device in the unlock status and
          // User not input password and press ESC,
          // Shutdown the device.
          //
          do {
            CreatePopUp (
              EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
              &Key,
              L"Press ENTER to shutdown, Press ESC to input password again",
              NULL
              );
          } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

          if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
            gRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);
          } else {
            //
            // Let user input password again.
            //
            continue;
          }
        }
      }

      if (Password == NULL) {
        Count ++;
        continue;
      }
      PasswordLen = (UINT32) AsciiStrLen(Password);

      if (IsLocked) {
        Ret = OpalUtilUpdateGlobalLockingRange(&Session, Password, PasswordLen, FALSE, FALSE);
      } else {
        Ret = OpalUtilUpdateGlobalLockingRange(&Session, Password, PasswordLen, TRUE, TRUE);
        if (Ret == TcgResultSuccess) {
          Ret = OpalUtilUpdateGlobalLockingRange(&Session, Password, PasswordLen, FALSE, FALSE);
        }
      }

      if (Ret == TcgResultSuccess) {
        OpalSupportUpdatePassword (&Dev->OpalDisk, Password, PasswordLen);
        DEBUG ((DEBUG_INFO, "%s Success\n", RequestString));
      } else {
        DEBUG ((DEBUG_INFO, "%s Failure\n", RequestString));
      }

      if (Password != NULL) {
        ZeroMem (Password, PasswordLen);
        FreePool (Password);
      }

      if (Ret == TcgResultSuccess) {
        break;
      }

      //
      // Check whether opal device's Tries value has reach the TryLimit value, if yes, force a shutdown
      // before accept new password.
      //
      if (Ret == TcgResultFailureInvalidType) {
        Count = MAX_PASSWORD_TRY_COUNT;
        break;
      }

      Count++;

      do {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Invalid password.",
          L"Press ENTER to retry",
          NULL
          );
      } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
    }

    if (Count >= MAX_PASSWORD_TRY_COUNT) {
      do {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Opal password retry count exceeds the limit. Must shutdown!",
          L"Press ENTER to shutdown",
          NULL
          );
      } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

      gRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    }
  }
}

/**
  Process Enable Feature OPAL request.

  @param[in] Dev            The device which has Enable Feature OPAL request.
  @param[in] RequestString  Request string.

**/
VOID
ProcessOpalRequestEnableFeature (
  IN OPAL_DRIVER_DEVICE *Dev,
  IN CHAR16             *RequestString
  )
{
  UINT8                 Count;
  CHAR8                 *Password;
  UINT32                PasswordLen;
  CHAR8                 *PasswordConfirm;
  UINT32                PasswordLenConfirm;
  OPAL_SESSION          Session;
  BOOLEAN               PressEsc;
  EFI_INPUT_KEY         Key;
  TCG_RESULT            Ret;
  CHAR16                *PopUpString;

  if (Dev == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO, "%a()\n", __FUNCTION__));

  PopUpString = OpalGetPopUpString (Dev, RequestString);

  Count = 0;

  ZeroMem(&Session, sizeof(Session));
  Session.Sscp = Dev->OpalDisk.Sscp;
  Session.MediaId = Dev->OpalDisk.MediaId;
  Session.OpalBaseComId = Dev->OpalDisk.OpalBaseComId;

  while (Count < MAX_PASSWORD_TRY_COUNT) {
    Password = OpalDriverPopUpPasswordInput (Dev, PopUpString, L"Please type in your new password", NULL, &PressEsc);
    if (PressEsc) {
        do {
          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"Press ENTER to skip the request and continue boot,",
            L"Press ESC to input password again",
            NULL
            );
        } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

        if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
          gST->ConOut->ClearScreen(gST->ConOut);
          return;
        } else {
          //
          // Let user input password again.
          //
          continue;
        }
    }

    if (Password == NULL) {
      Count ++;
      continue;
    }
    PasswordLen = (UINT32) AsciiStrLen(Password);

    PasswordConfirm = OpalDriverPopUpPasswordInput (Dev, PopUpString, L"Please confirm your new password", NULL, &PressEsc);
    if (PasswordConfirm == NULL) {
      ZeroMem (Password, PasswordLen);
      FreePool (Password);
      Count ++;
      continue;
    }
    PasswordLenConfirm = (UINT32) AsciiStrLen(PasswordConfirm);
    if ((PasswordLen != PasswordLenConfirm) ||
        (CompareMem (Password, PasswordConfirm, PasswordLen) != 0)) {
      ZeroMem (Password, PasswordLen);
      FreePool (Password);
      ZeroMem (PasswordConfirm, PasswordLenConfirm);
      FreePool (PasswordConfirm);
      do {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Passwords are not the same.",
          L"Press ENTER to retry",
          NULL
          );
      } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
      Count ++;
      continue;
    }

    if (PasswordConfirm != NULL) {
      ZeroMem (PasswordConfirm, PasswordLenConfirm);
      FreePool (PasswordConfirm);
    }

    Ret = OpalSupportEnableOpalFeature (&Session, Dev->OpalDisk.Msid,  Dev->OpalDisk.MsidLength, Password, PasswordLen);
    if (Ret == TcgResultSuccess) {
      OpalSupportUpdatePassword (&Dev->OpalDisk, Password, PasswordLen);
      DEBUG ((DEBUG_INFO, "%s Success\n", RequestString));
    } else {
      DEBUG ((DEBUG_INFO, "%s Failure\n", RequestString));
    }

    if (Password != NULL) {
      ZeroMem (Password, PasswordLen);
      FreePool (Password);
    }

    if (Ret == TcgResultSuccess) {
      break;
    }

    Count++;

    do {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Request failed.",
        L"Press ENTER to retry",
        NULL
        );
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
  }

  if (Count >= MAX_PASSWORD_TRY_COUNT) {
    do {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Opal password retry count exceeds the limit.",
        L"Press ENTER to skip the request and continue boot",
        NULL
        );
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
    gST->ConOut->ClearScreen(gST->ConOut);
  }
}

/**
  Process Disable User OPAL request.

  @param[in] Dev            The device which has Disable User OPAL request.
  @param[in] RequestString  Request string.

**/
VOID
ProcessOpalRequestDisableUser (
  IN OPAL_DRIVER_DEVICE *Dev,
  IN CHAR16             *RequestString
  )
{
  UINT8                 Count;
  CHAR8                 *Password;
  UINT32                PasswordLen;
  OPAL_SESSION          Session;
  BOOLEAN               PressEsc;
  EFI_INPUT_KEY         Key;
  TCG_RESULT            Ret;
  BOOLEAN               PasswordFailed;
  CHAR16                *PopUpString;

  if (Dev == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO, "%a()\n", __FUNCTION__));

  PopUpString = OpalGetPopUpString (Dev, RequestString);

  Count = 0;

  ZeroMem(&Session, sizeof(Session));
  Session.Sscp = Dev->OpalDisk.Sscp;
  Session.MediaId = Dev->OpalDisk.MediaId;
  Session.OpalBaseComId = Dev->OpalDisk.OpalBaseComId;

  while (Count < MAX_PASSWORD_TRY_COUNT) {
    Password = OpalDriverPopUpPasswordInput (Dev, PopUpString, NULL, NULL, &PressEsc);
    if (PressEsc) {
        do {
          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"Press ENTER to skip the request and continue boot,",
            L"Press ESC to input password again",
            NULL
            );
        } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

        if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
          gST->ConOut->ClearScreen(gST->ConOut);
          return;
        } else {
          //
          // Let user input password again.
          //
          continue;
        }
    }

    if (Password == NULL) {
      Count ++;
      continue;
    }
    PasswordLen = (UINT32) AsciiStrLen(Password);

    Ret = OpalUtilDisableUser(&Session, Password, PasswordLen, &PasswordFailed);
    if (Ret == TcgResultSuccess) {
      OpalSupportUpdatePassword (&Dev->OpalDisk, Password, PasswordLen);
      DEBUG ((DEBUG_INFO, "%s Success\n", RequestString));
    } else {
      DEBUG ((DEBUG_INFO, "%s Failure\n", RequestString));
    }

    if (Password != NULL) {
      ZeroMem (Password, PasswordLen);
      FreePool (Password);
    }

    if (Ret == TcgResultSuccess) {
      break;
    }

    Count++;

    do {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Invalid password, request failed.",
        L"Press ENTER to retry",
        NULL
        );
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
  }

  if (Count >= MAX_PASSWORD_TRY_COUNT) {
    do {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Opal password retry count exceeds the limit.",
        L"Press ENTER to skip the request and continue boot",
        NULL
        );
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
    gST->ConOut->ClearScreen(gST->ConOut);
  }
}

/**
  Process Psid Revert OPAL request.

  @param[in] Dev            The device which has Psid Revert OPAL request.
  @param[in] RequestString  Request string.

**/
VOID
ProcessOpalRequestPsidRevert (
  IN OPAL_DRIVER_DEVICE *Dev,
  IN CHAR16             *RequestString
  )
{
  UINT8                 Count;
  CHAR8                 *Psid;
  UINT32                PsidLen;
  OPAL_SESSION          Session;
  BOOLEAN               PressEsc;
  EFI_INPUT_KEY         Key;
  TCG_RESULT            Ret;
  CHAR16                *PopUpString;
  CHAR16                *PopUpString2;
  CHAR16                *PopUpString3;
  UINTN                 BufferSize;

  if (Dev == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO, "%a()\n", __FUNCTION__));

  PopUpString = OpalGetPopUpString (Dev, RequestString);

  if (Dev->OpalDisk.EstimateTimeCost > MAX_ACCEPTABLE_REVERTING_TIME) {
    BufferSize = StrSize (L"Warning: Revert action will take about ####### seconds");
    PopUpString2 = AllocateZeroPool (BufferSize);
    ASSERT (PopUpString2 != NULL);
    UnicodeSPrint (
        PopUpString2,
        BufferSize,
        L"WARNING: Revert action will take about %d seconds",
        Dev->OpalDisk.EstimateTimeCost
      );
    PopUpString3 = L"DO NOT power off system during the revert action!";
  } else {
    PopUpString2 = NULL;
    PopUpString3 = NULL;
  }

  Count = 0;

  ZeroMem(&Session, sizeof(Session));
  Session.Sscp = Dev->OpalDisk.Sscp;
  Session.MediaId = Dev->OpalDisk.MediaId;
  Session.OpalBaseComId = Dev->OpalDisk.OpalBaseComId;

  while (Count < MAX_PSID_TRY_COUNT) {
    Psid = OpalDriverPopUpPsidInput (Dev, PopUpString, PopUpString2, PopUpString3, &PressEsc);
    if (PressEsc) {
        do {
          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"Press ENTER to skip the request and continue boot,",
            L"Press ESC to input Psid again",
            NULL
            );
        } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

        if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
          gST->ConOut->ClearScreen(gST->ConOut);
          goto Done;
        } else {
          //
          // Let user input Psid again.
          //
          continue;
        }
    }

    if (Psid == NULL) {
      Count ++;
      continue;
    }
    PsidLen = (UINT32) AsciiStrLen(Psid);

    Ret = OpalUtilPsidRevert(&Session, Psid, PsidLen);
    if (Ret == TcgResultSuccess) {
      DEBUG ((DEBUG_INFO, "%s Success\n", RequestString));
    } else {
      DEBUG ((DEBUG_INFO, "%s Failure\n", RequestString));
    }

    if (Psid != NULL) {
      ZeroMem (Psid, PsidLen);
      FreePool (Psid);
    }

    if (Ret == TcgResultSuccess) {
      break;
    }

    Count++;

    do {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Invalid Psid, request failed.",
        L"Press ENTER to retry",
        NULL
        );
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
  }

  if (Count >= MAX_PSID_TRY_COUNT) {
    do {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Opal Psid retry count exceeds the limit.",
        L"Press ENTER to skip the request and continue boot",
        NULL
        );
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
    gST->ConOut->ClearScreen(gST->ConOut);
  }

Done:
  if (PopUpString2 != NULL) {
    FreePool (PopUpString2);
  }
}

/**
  Process Admin Revert OPAL request.

  @param[in] Dev            The device which has Revert OPAL request.
  @param[in] KeepUserData   Whether to keep user data or not.
  @param[in] RequestString  Request string.

**/
VOID
ProcessOpalRequestRevert (
  IN OPAL_DRIVER_DEVICE *Dev,
  IN BOOLEAN            KeepUserData,
  IN CHAR16             *RequestString
  )
{
  UINT8                 Count;
  CHAR8                 *Password;
  UINT32                PasswordLen;
  OPAL_SESSION          Session;
  BOOLEAN               PressEsc;
  EFI_INPUT_KEY         Key;
  TCG_RESULT            Ret;
  BOOLEAN               PasswordFailed;
  CHAR16                *PopUpString;
  CHAR16                *PopUpString2;
  CHAR16                *PopUpString3;
  UINTN                 BufferSize;

  if (Dev == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO, "%a()\n", __FUNCTION__));

  PopUpString = OpalGetPopUpString (Dev, RequestString);

  if ((!KeepUserData) &&
      (Dev->OpalDisk.EstimateTimeCost > MAX_ACCEPTABLE_REVERTING_TIME)) {
    BufferSize = StrSize (L"Warning: Revert action will take about ####### seconds");
    PopUpString2 = AllocateZeroPool (BufferSize);
    ASSERT (PopUpString2 != NULL);
    UnicodeSPrint (
        PopUpString2,
        BufferSize,
        L"WARNING: Revert action will take about %d seconds",
        Dev->OpalDisk.EstimateTimeCost
      );
    PopUpString3 = L"DO NOT power off system during the revert action!";
  } else {
    PopUpString2 = NULL;
    PopUpString3 = NULL;
  }

  Count = 0;

  ZeroMem(&Session, sizeof(Session));
  Session.Sscp = Dev->OpalDisk.Sscp;
  Session.MediaId = Dev->OpalDisk.MediaId;
  Session.OpalBaseComId = Dev->OpalDisk.OpalBaseComId;

  while (Count < MAX_PASSWORD_TRY_COUNT) {
    Password = OpalDriverPopUpPasswordInput (Dev, PopUpString, PopUpString2, PopUpString3, &PressEsc);
    if (PressEsc) {
        do {
          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"Press ENTER to skip the request and continue boot,",
            L"Press ESC to input password again",
            NULL
            );
        } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

        if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
          gST->ConOut->ClearScreen(gST->ConOut);
          goto Done;
        } else {
          //
          // Let user input password again.
          //
          continue;
        }
    }

    if (Password == NULL) {
      Count ++;
      continue;
    }
    PasswordLen = (UINT32) AsciiStrLen(Password);

    if ((Dev->OpalDisk.SupportedAttributes.PyriteSsc == 1) &&
        (Dev->OpalDisk.LockingFeature.MediaEncryption == 0)) {
      //
      // For pyrite type device which does not support media encryption,
      // it does not accept "Keep User Data" parameter.
      // So here hardcode a FALSE for this case.
      //
      Ret = OpalUtilRevert(
              &Session,
              FALSE,
              Password,
              PasswordLen,
              &PasswordFailed,
              Dev->OpalDisk.Msid,
              Dev->OpalDisk.MsidLength
              );
    } else {
      Ret = OpalUtilRevert(
              &Session,
              KeepUserData,
              Password,
              PasswordLen,
              &PasswordFailed,
              Dev->OpalDisk.Msid,
              Dev->OpalDisk.MsidLength
              );
    }
    if (Ret == TcgResultSuccess) {
      OpalSupportUpdatePassword (&Dev->OpalDisk, Password, PasswordLen);
      DEBUG ((DEBUG_INFO, "%s Success\n", RequestString));
    } else {
      DEBUG ((DEBUG_INFO, "%s Failure\n", RequestString));
    }

    if (Password != NULL) {
      ZeroMem (Password, PasswordLen);
      FreePool (Password);
    }

    if (Ret == TcgResultSuccess) {
      break;
    }

    Count++;

    do {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Invalid password, request failed.",
        L"Press ENTER to retry",
        NULL
        );
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
  }

  if (Count >= MAX_PASSWORD_TRY_COUNT) {
    do {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Opal password retry count exceeds the limit.",
        L"Press ENTER to skip the request and continue boot",
        NULL
        );
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
    gST->ConOut->ClearScreen(gST->ConOut);
  }

Done:
  if (PopUpString2 != NULL) {
    FreePool (PopUpString2);
  }
}

/**
  Process Secure Erase OPAL request.

  @param[in] Dev            The device which has Secure Erase OPAL request.
  @param[in] RequestString  Request string.

**/
VOID
ProcessOpalRequestSecureErase (
  IN OPAL_DRIVER_DEVICE *Dev,
  IN CHAR16             *RequestString
  )
{
  UINT8                 Count;
  CHAR8                 *Password;
  UINT32                PasswordLen;
  OPAL_SESSION          Session;
  BOOLEAN               PressEsc;
  EFI_INPUT_KEY         Key;
  TCG_RESULT            Ret;
  BOOLEAN               PasswordFailed;
  CHAR16                *PopUpString;
  CHAR16                *PopUpString2;
  CHAR16                *PopUpString3;
  UINTN                 BufferSize;

  if (Dev == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO, "%a()\n", __FUNCTION__));

  PopUpString = OpalGetPopUpString (Dev, RequestString);

  if (Dev->OpalDisk.EstimateTimeCost > MAX_ACCEPTABLE_REVERTING_TIME) {
    BufferSize = StrSize (L"Warning: Secure erase action will take about ####### seconds");
    PopUpString2 = AllocateZeroPool (BufferSize);
    ASSERT (PopUpString2 != NULL);
    UnicodeSPrint (
        PopUpString2,
        BufferSize,
        L"WARNING: Secure erase action will take about %d seconds",
        Dev->OpalDisk.EstimateTimeCost
      );
    PopUpString3 = L"DO NOT power off system during the action!";
  } else {
    PopUpString2 = NULL;
    PopUpString3 = NULL;
  }
  Count = 0;

  ZeroMem(&Session, sizeof(Session));
  Session.Sscp = Dev->OpalDisk.Sscp;
  Session.MediaId = Dev->OpalDisk.MediaId;
  Session.OpalBaseComId = Dev->OpalDisk.OpalBaseComId;

  while (Count < MAX_PASSWORD_TRY_COUNT) {
    Password = OpalDriverPopUpPasswordInput (Dev, PopUpString, PopUpString2, PopUpString3, &PressEsc);
    if (PressEsc) {
        do {
          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"Press ENTER to skip the request and continue boot,",
            L"Press ESC to input password again",
            NULL
            );
        } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

        if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
          gST->ConOut->ClearScreen(gST->ConOut);
          goto Done;
        } else {
          //
          // Let user input password again.
          //
          continue;
        }
    }

    if (Password == NULL) {
      Count ++;
      continue;
    }
    PasswordLen = (UINT32) AsciiStrLen(Password);

    Ret = OpalUtilSecureErase(&Session, Password, PasswordLen, &PasswordFailed);
    if (Ret == TcgResultSuccess) {
      OpalSupportUpdatePassword (&Dev->OpalDisk, Password, PasswordLen);
      DEBUG ((DEBUG_INFO, "%s Success\n", RequestString));
    } else {
      DEBUG ((DEBUG_INFO, "%s Failure\n", RequestString));
    }

    if (Password != NULL) {
      ZeroMem (Password, PasswordLen);
      FreePool (Password);
    }

    if (Ret == TcgResultSuccess) {
      break;
    }

    Count++;

    do {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Invalid password, request failed.",
        L"Press ENTER to retry",
        NULL
        );
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
  }

  if (Count >= MAX_PASSWORD_TRY_COUNT) {
    do {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Opal password retry count exceeds the limit.",
        L"Press ENTER to skip the request and continue boot",
        NULL
        );
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
    gST->ConOut->ClearScreen(gST->ConOut);
  }

Done:
  if (PopUpString2 != NULL) {
    FreePool (PopUpString2);
  }
}

/**
  Process Set Admin Pwd OPAL request.

  @param[in] Dev            The device which has Set Admin Pwd Feature OPAL request.
  @param[in] RequestString  Request string.

**/
VOID
ProcessOpalRequestSetUserPwd (
  IN OPAL_DRIVER_DEVICE *Dev,
  IN CHAR16             *RequestString
  )
{
  UINT8                 Count;
  CHAR8                 *OldPassword;
  UINT32                OldPasswordLen;
  CHAR8                 *Password;
  UINT32                PasswordLen;
  CHAR8                 *PasswordConfirm;
  UINT32                PasswordLenConfirm;
  OPAL_SESSION          Session;
  BOOLEAN               PressEsc;
  EFI_INPUT_KEY         Key;
  TCG_RESULT            Ret;
  CHAR16                *PopUpString;

  if (Dev == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO, "%a()\n", __FUNCTION__));

  PopUpString = OpalGetPopUpString (Dev, RequestString);

  Count = 0;

  while (Count < MAX_PASSWORD_TRY_COUNT) {
    OldPassword = OpalDriverPopUpPasswordInput (Dev, PopUpString, L"Please type in your password", NULL, &PressEsc);
    if (PressEsc) {
        do {
          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"Press ENTER to skip the request and continue boot,",
            L"Press ESC to input password again",
            NULL
            );
        } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

        if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
          gST->ConOut->ClearScreen(gST->ConOut);
          return;
        } else {
          //
          // Let user input password again.
          //
          continue;
        }
    }

    if (OldPassword == NULL) {
      Count ++;
      continue;
    }
    OldPasswordLen = (UINT32) AsciiStrLen(OldPassword);

    ZeroMem(&Session, sizeof(Session));
    Session.Sscp = Dev->OpalDisk.Sscp;
    Session.MediaId = Dev->OpalDisk.MediaId;
    Session.OpalBaseComId = Dev->OpalDisk.OpalBaseComId;
    Ret = OpalUtilVerifyPassword (&Session, OldPassword, OldPasswordLen, OPAL_LOCKING_SP_USER1_AUTHORITY);
    if (Ret == TcgResultSuccess) {
      DEBUG ((DEBUG_INFO, "Verify with USER1 authority : Success\n"));
    } else {
      Ret = OpalUtilVerifyPassword (&Session, OldPassword, OldPasswordLen, OPAL_LOCKING_SP_ADMIN1_AUTHORITY);
      if (Ret == TcgResultSuccess) {
        DEBUG ((DEBUG_INFO, "Verify with ADMIN1 authority: Success\n"));
      } else {
        ZeroMem (OldPassword, OldPasswordLen);
        FreePool (OldPassword);
        DEBUG ((DEBUG_INFO, "Verify: Failure\n"));
        do {
          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"Incorrect password.",
            L"Press ENTER to retry",
            NULL
            );
        } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
        Count ++;
        continue;
      }
    }

    Password = OpalDriverPopUpPasswordInput (Dev, PopUpString, L"Please type in your new password", NULL, &PressEsc);
    if (Password == NULL) {
      ZeroMem (OldPassword, OldPasswordLen);
      FreePool (OldPassword);
      Count ++;
      continue;
    }
    PasswordLen = (UINT32) AsciiStrLen(Password);

    PasswordConfirm = OpalDriverPopUpPasswordInput (Dev, PopUpString, L"Please confirm your new password", NULL, &PressEsc);
    if (PasswordConfirm == NULL) {
      ZeroMem (OldPassword, OldPasswordLen);
      FreePool (OldPassword);
      ZeroMem (Password, PasswordLen);
      FreePool (Password);
      Count ++;
      continue;
    }
    PasswordLenConfirm = (UINT32) AsciiStrLen(PasswordConfirm);
    if ((PasswordLen != PasswordLenConfirm) ||
        (CompareMem (Password, PasswordConfirm, PasswordLen) != 0)) {
      ZeroMem (OldPassword, OldPasswordLen);
      FreePool (OldPassword);
      ZeroMem (Password, PasswordLen);
      FreePool (Password);
      ZeroMem (PasswordConfirm, PasswordLenConfirm);
      FreePool (PasswordConfirm);
      do {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Passwords are not the same.",
          L"Press ENTER to retry",
          NULL
          );
      } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
      Count ++;
      continue;
    }

    if (PasswordConfirm != NULL) {
      ZeroMem (PasswordConfirm, PasswordLenConfirm);
      FreePool (PasswordConfirm);
    }

    ZeroMem(&Session, sizeof(Session));
    Session.Sscp = Dev->OpalDisk.Sscp;
    Session.MediaId = Dev->OpalDisk.MediaId;
    Session.OpalBaseComId = Dev->OpalDisk.OpalBaseComId;
    Ret = OpalUtilSetUserPassword(
            &Session,
            OldPassword,
            OldPasswordLen,
            Password,
            PasswordLen
            );
    if (Ret == TcgResultSuccess) {
      OpalSupportUpdatePassword (&Dev->OpalDisk, Password, PasswordLen);
      DEBUG ((DEBUG_INFO, "%s Success\n", RequestString));
    } else {
      DEBUG ((DEBUG_INFO, "%s Failure\n", RequestString));
    }

    if (OldPassword != NULL) {
      ZeroMem (OldPassword, OldPasswordLen);
      FreePool (OldPassword);
    }

    if (Password != NULL) {
      ZeroMem (Password, PasswordLen);
      FreePool (Password);
    }

    if (Ret == TcgResultSuccess) {
      break;
    }

    Count++;

    do {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Request failed.",
        L"Press ENTER to retry",
        NULL
        );
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
  }

  if (Count >= MAX_PASSWORD_TRY_COUNT) {
    do {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Opal password retry count exceeds the limit.",
        L"Press ENTER to skip the request and continue boot",
        NULL
        );
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
    gST->ConOut->ClearScreen(gST->ConOut);
  }
}

/**
  Process Set Admin Pwd OPAL request.

  @param[in] Dev            The device which has Set Admin Pwd Feature OPAL request.
  @param[in] RequestString  Request string.

**/
VOID
ProcessOpalRequestSetAdminPwd (
  IN OPAL_DRIVER_DEVICE *Dev,
  IN CHAR16             *RequestString
  )
{
  UINT8                 Count;
  CHAR8                 *OldPassword;
  UINT32                OldPasswordLen;
  CHAR8                 *Password;
  UINT32                PasswordLen;
  CHAR8                 *PasswordConfirm;
  UINT32                PasswordLenConfirm;
  OPAL_SESSION          Session;
  BOOLEAN               PressEsc;
  EFI_INPUT_KEY         Key;
  TCG_RESULT            Ret;
  CHAR16                *PopUpString;

  if (Dev == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO, "%a()\n", __FUNCTION__));

  PopUpString = OpalGetPopUpString (Dev, RequestString);

  Count = 0;

  while (Count < MAX_PASSWORD_TRY_COUNT) {
    OldPassword = OpalDriverPopUpPasswordInput (Dev, PopUpString, L"Please type in your password", NULL, &PressEsc);
    if (PressEsc) {
        do {
          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"Press ENTER to skip the request and continue boot,",
            L"Press ESC to input password again",
            NULL
            );
        } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

        if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
          gST->ConOut->ClearScreen(gST->ConOut);
          return;
        } else {
          //
          // Let user input password again.
          //
          continue;
        }
    }

    if (OldPassword == NULL) {
      Count ++;
      continue;
    }
    OldPasswordLen = (UINT32) AsciiStrLen(OldPassword);

    ZeroMem(&Session, sizeof(Session));
    Session.Sscp = Dev->OpalDisk.Sscp;
    Session.MediaId = Dev->OpalDisk.MediaId;
    Session.OpalBaseComId = Dev->OpalDisk.OpalBaseComId;
    Ret = OpalUtilVerifyPassword (&Session, OldPassword, OldPasswordLen, OPAL_LOCKING_SP_ADMIN1_AUTHORITY);
    if (Ret == TcgResultSuccess) {
      DEBUG ((DEBUG_INFO, "Verify: Success\n"));
    } else {
      ZeroMem (OldPassword, OldPasswordLen);
      FreePool (OldPassword);
      DEBUG ((DEBUG_INFO, "Verify: Failure\n"));
      do {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Incorrect password.",
          L"Press ENTER to retry",
          NULL
          );
      } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
      Count ++;
      continue;
    }

    Password = OpalDriverPopUpPasswordInput (Dev, PopUpString, L"Please type in your new password", NULL, &PressEsc);
    if (Password == NULL) {
      ZeroMem (OldPassword, OldPasswordLen);
      FreePool (OldPassword);
      Count ++;
      continue;
    }
    PasswordLen = (UINT32) AsciiStrLen(Password);

    PasswordConfirm = OpalDriverPopUpPasswordInput (Dev, PopUpString, L"Please confirm your new password", NULL, &PressEsc);
    if (PasswordConfirm == NULL) {
      ZeroMem (OldPassword, OldPasswordLen);
      FreePool (OldPassword);
      ZeroMem (Password, PasswordLen);
      FreePool (Password);
      Count ++;
      continue;
    }
    PasswordLenConfirm = (UINT32) AsciiStrLen(PasswordConfirm);
    if ((PasswordLen != PasswordLenConfirm) ||
        (CompareMem (Password, PasswordConfirm, PasswordLen) != 0)) {
      ZeroMem (OldPassword, OldPasswordLen);
      FreePool (OldPassword);
      ZeroMem (Password, PasswordLen);
      FreePool (Password);
      ZeroMem (PasswordConfirm, PasswordLenConfirm);
      FreePool (PasswordConfirm);
      do {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Passwords are not the same.",
          L"Press ENTER to retry",
          NULL
          );
      } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
      Count ++;
      continue;
    }

    if (PasswordConfirm != NULL) {
      ZeroMem (PasswordConfirm, PasswordLenConfirm);
      FreePool (PasswordConfirm);
    }


    ZeroMem(&Session, sizeof(Session));
    Session.Sscp = Dev->OpalDisk.Sscp;
    Session.MediaId = Dev->OpalDisk.MediaId;
    Session.OpalBaseComId = Dev->OpalDisk.OpalBaseComId;
    Ret = OpalUtilSetAdminPassword(
            &Session,
            OldPassword,
            OldPasswordLen,
            Password,
            PasswordLen
            );
    if (Ret == TcgResultSuccess) {
      OpalSupportUpdatePassword (&Dev->OpalDisk, Password, PasswordLen);
      DEBUG ((DEBUG_INFO, "%s Success\n", RequestString));
    } else {
      DEBUG ((DEBUG_INFO, "%s Failure\n", RequestString));
    }

    if (OldPassword != NULL) {
      ZeroMem (OldPassword, OldPasswordLen);
      FreePool (OldPassword);
    }

    if (Password != NULL) {
      ZeroMem (Password, PasswordLen);
      FreePool (Password);
    }

    if (Ret == TcgResultSuccess) {
      break;
    }

    Count++;

    do {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Request failed.",
        L"Press ENTER to retry",
        NULL
        );
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
  }

  if (Count >= MAX_PASSWORD_TRY_COUNT) {
    do {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Opal password retry count exceeds the limit.",
        L"Press ENTER to skip the request and continue boot",
        NULL
        );
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
    gST->ConOut->ClearScreen(gST->ConOut);
  }
}

/**
  Process OPAL request.

  @param[in] Dev            The device which has OPAL request.

**/
VOID
ProcessOpalRequest (
  IN OPAL_DRIVER_DEVICE     *Dev
  )
{
  EFI_STATUS                Status;
  OPAL_REQUEST_VARIABLE     *TempVariable;
  OPAL_REQUEST_VARIABLE     *Variable;
  UINTN                     VariableSize;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathInVariable;
  UINTN                     DevicePathSizeInVariable;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     DevicePathSize;
  BOOLEAN                   KeepUserData;

  DEBUG ((DEBUG_INFO, "%a() - enter\n", __FUNCTION__));

  if (mOpalRequestVariable == NULL) {
    Status = GetVariable2 (
               OPAL_REQUEST_VARIABLE_NAME,
               &gHiiSetupVariableGuid,
               (VOID **) &Variable,
               &VariableSize
               );
    if (EFI_ERROR (Status) || (Variable == NULL)) {
      return;
    }
    mOpalRequestVariable = Variable;
    mOpalRequestVariableSize = VariableSize;

    //
    // Delete the OPAL request variable.
    //
    Status = gRT->SetVariable (
                    OPAL_REQUEST_VARIABLE_NAME,
                    (EFI_GUID *) &gHiiSetupVariableGuid,
                    0,
                    0,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  } else {
    Variable = mOpalRequestVariable;
    VariableSize = mOpalRequestVariableSize;
  }

  //
  // Process the OPAL requests.
  //
  TempVariable = Variable;
  while ((VariableSize > sizeof (OPAL_REQUEST_VARIABLE)) &&
         (VariableSize >= TempVariable->Length) &&
         (TempVariable->Length > sizeof (OPAL_REQUEST_VARIABLE))) {
    DevicePathInVariable = (EFI_DEVICE_PATH_PROTOCOL *) ((UINTN) TempVariable + sizeof (OPAL_REQUEST_VARIABLE));
    DevicePathSizeInVariable = GetDevicePathSize (DevicePathInVariable);
    DevicePath = Dev->OpalDisk.OpalDevicePath;
    DevicePathSize = GetDevicePathSize (DevicePath);
    if ((DevicePathSize == DevicePathSizeInVariable) &&
        (CompareMem (DevicePath, DevicePathInVariable, DevicePathSize) == 0)) {
      //
      // Found the node for the OPAL device.
      //
      if (TempVariable->OpalRequest.SetAdminPwd != 0) {
        ProcessOpalRequestSetAdminPwd (Dev, L"Update Admin Pwd:");
      }
      if (TempVariable->OpalRequest.SetUserPwd != 0) {
        ProcessOpalRequestSetUserPwd (Dev, L"Set User Pwd:");
      }
      if (TempVariable->OpalRequest.SecureErase!= 0) {
        ProcessOpalRequestSecureErase (Dev, L"Secure Erase:");
      }
      if (TempVariable->OpalRequest.Revert != 0) {
        KeepUserData = (BOOLEAN) TempVariable->OpalRequest.KeepUserData;
        ProcessOpalRequestRevert (
          Dev,
          KeepUserData,
          KeepUserData ? L"Admin Revert(keep):" : L"Admin Revert:"
          );
      }
      if (TempVariable->OpalRequest.PsidRevert != 0) {
        ProcessOpalRequestPsidRevert (Dev, L"Psid Revert:");
      }
      if (TempVariable->OpalRequest.DisableUser != 0) {
        ProcessOpalRequestDisableUser (Dev, L"Disable User:");
      }
      if (TempVariable->OpalRequest.EnableFeature != 0) {
        ProcessOpalRequestEnableFeature (Dev, L"Enable Feature:");
      }

      //
      // Update Device ownership.
      // Later BlockSID command may block the update.
      //
      OpalDiskUpdateOwnerShip (&Dev->OpalDisk);

      break;
    }

    VariableSize -= TempVariable->Length;
    TempVariable = (OPAL_REQUEST_VARIABLE *) ((UINTN) TempVariable + TempVariable->Length);
  }

  DEBUG ((DEBUG_INFO, "%a() - exit\n", __FUNCTION__));
}

/**
  Add new device to the global device list.

  @param Dev             New create device.

**/
VOID
AddDeviceToTail(
  IN OPAL_DRIVER_DEVICE    *Dev
  )
{
  OPAL_DRIVER_DEVICE                *TmpDev;

  if (mOpalDriver.DeviceList == NULL) {
    mOpalDriver.DeviceList = Dev;
  } else {
    TmpDev = mOpalDriver.DeviceList;
    while (TmpDev->Next != NULL) {
      TmpDev = TmpDev->Next;
    }

    TmpDev->Next = Dev;
  }
}

/**
  Remove one device in the global device list.

  @param Dev             The device need to be removed.

**/
VOID
RemoveDevice (
  IN OPAL_DRIVER_DEVICE    *Dev
  )
{
  OPAL_DRIVER_DEVICE                *TmpDev;

  if (mOpalDriver.DeviceList == NULL) {
    return;
  }

  if (mOpalDriver.DeviceList == Dev) {
    mOpalDriver.DeviceList = NULL;
    return;
  }

  TmpDev = mOpalDriver.DeviceList;
  while (TmpDev->Next != NULL) {
    if (TmpDev->Next == Dev) {
      TmpDev->Next = Dev->Next;
      break;
    }
  }
}

/**
  Get current device count.

  @retval  return the current created device count.

**/
UINT8
GetDeviceCount (
  VOID
  )
{
  UINT8                Count;
  OPAL_DRIVER_DEVICE   *TmpDev;

  Count = 0;
  TmpDev = mOpalDriver.DeviceList;

  while (TmpDev != NULL) {
    Count++;
    TmpDev = TmpDev->Next;
  }

  return Count;
}

/**
  Get devcie list info.

  @retval     return the device list pointer.
**/
OPAL_DRIVER_DEVICE*
OpalDriverGetDeviceList(
  VOID
  )
{
  return mOpalDriver.DeviceList;
}

/**
  Stop this Controller.

  @param  Dev               The device need to be stopped.

**/
VOID
OpalDriverStopDevice (
  OPAL_DRIVER_DEVICE     *Dev
  )
{
  //
  // free each name
  //
  FreePool(Dev->Name16);

  //
  // remove OPAL_DRIVER_DEVICE from the list
  // it updates the controllerList pointer
  //
  RemoveDevice(Dev);

  //
  // close protocols that were opened
  //
  gBS->CloseProtocol(
    Dev->Handle,
    &gEfiStorageSecurityCommandProtocolGuid,
    gOpalDriverBinding.DriverBindingHandle,
    Dev->Handle
    );

  gBS->CloseProtocol(
    Dev->Handle,
    &gEfiBlockIoProtocolGuid,
    gOpalDriverBinding.DriverBindingHandle,
    Dev->Handle
    );

  FreePool(Dev);
}

/**
  Get devcie name through the component name protocol.

  @param[in]       AllHandlesBuffer   The handle buffer for current system.
  @param[in]       NumAllHandles      The number of handles for the handle buffer.
  @param[in]       Dev                The device which need to get name.
  @param[in]       UseComp1           Whether use component name or name2 protocol.

  @retval     TRUE        Find the name for this device.
  @retval     FALSE       Not found the name for this device.
**/
BOOLEAN
OpalDriverGetDeviceNameByProtocol(
  EFI_HANDLE             *AllHandlesBuffer,
  UINTN                  NumAllHandles,
  OPAL_DRIVER_DEVICE     *Dev,
  BOOLEAN                UseComp1
  )
{
  EFI_HANDLE*                   ProtocolHandlesBuffer;
  UINTN                         NumProtocolHandles;
  EFI_STATUS                    Status;
  EFI_COMPONENT_NAME2_PROTOCOL* Cnp1_2; // efi component name and componentName2 have same layout
  EFI_GUID                      Protocol;
  UINTN                         StrLength;
  EFI_DEVICE_PATH_PROTOCOL*     TmpDevPath;
  UINTN                         Index1;
  UINTN                         Index2;
  EFI_HANDLE                    TmpHandle;
  CHAR16                        *DevName;

  if (Dev == NULL || AllHandlesBuffer == NULL || NumAllHandles == 0) {
    return FALSE;
  }

  Protocol = UseComp1 ? gEfiComponentNameProtocolGuid : gEfiComponentName2ProtocolGuid;

  //
  // Find all EFI_HANDLES with protocol
  //
  Status = gBS->LocateHandleBuffer(
               ByProtocol,
               &Protocol,
               NULL,
               &NumProtocolHandles,
               &ProtocolHandlesBuffer
               );
  if (EFI_ERROR(Status)) {
    return FALSE;
  }


  //
  // Exit early if no supported devices
  //
  if (NumProtocolHandles == 0) {
    return FALSE;
  }

  //
  // Get printable name by iterating through all protocols
  // using the handle as the child, and iterate through all handles for the controller
  // exit loop early once found, if not found, then delete device
  // storage security protocol instances already exist, add them to internal list
  //
  Status = EFI_DEVICE_ERROR;
  for (Index1 = 0; Index1 < NumProtocolHandles; Index1++) {
    DevName = NULL;

    if (Dev->Name16 != NULL) {
      return TRUE;
    }

    TmpHandle = ProtocolHandlesBuffer[Index1];

    Status = gBS->OpenProtocol(
                 TmpHandle,
                 &Protocol,
                 (VOID**)&Cnp1_2,
                 gImageHandle,
                 NULL,
                 EFI_OPEN_PROTOCOL_GET_PROTOCOL
                 );
    if (EFI_ERROR(Status) || Cnp1_2 == NULL) {
      continue;
    }

    //
    // Use all handles array as controller handle
    //
    for (Index2 = 0; Index2 < NumAllHandles; Index2++) {
      Status = Cnp1_2->GetControllerName(
                   Cnp1_2,
                   AllHandlesBuffer[Index2],
                   Dev->Handle,
                   LANGUAGE_ISO_639_2_ENGLISH,
                   &DevName
                   );
      if (EFI_ERROR(Status)) {
        Status = Cnp1_2->GetControllerName(
                     Cnp1_2,
                     AllHandlesBuffer[Index2],
                     Dev->Handle,
                     LANGUAGE_RFC_3066_ENGLISH,
                     &DevName
                     );
      }
      if (!EFI_ERROR(Status) && DevName != NULL) {
        StrLength = StrLen(DevName) + 1;        // Add one for NULL terminator
        Dev->Name16 = AllocateZeroPool(StrLength * sizeof (CHAR16));
        ASSERT (Dev->Name16 != NULL);
        StrCpyS (Dev->Name16, StrLength, DevName);
        Dev->NameZ = (CHAR8*)AllocateZeroPool(StrLength);
        UnicodeStrToAsciiStrS (DevName, Dev->NameZ, StrLength);

        //
        // Retrieve bridge BDF info and port number or namespace depending on type
        //
        TmpDevPath = NULL;
        Status = gBS->OpenProtocol(
            Dev->Handle,
            &gEfiDevicePathProtocolGuid,
            (VOID**)&TmpDevPath,
            gImageHandle,
            NULL,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
            );
        if (!EFI_ERROR(Status)) {
          Dev->OpalDevicePath = DuplicateDevicePath (TmpDevPath);
          return TRUE;
        }

        if (Dev->Name16 != NULL) {
          FreePool(Dev->Name16);
          Dev->Name16 = NULL;
        }
        if (Dev->NameZ != NULL) {
          FreePool(Dev->NameZ);
          Dev->NameZ = NULL;
        }
      }
    }
  }

  return FALSE;
}

/**
  Get devcie name through the component name protocol.

  @param[in]       Dev                The device which need to get name.

  @retval     TRUE        Find the name for this device.
  @retval     FALSE       Not found the name for this device.
**/
BOOLEAN
OpalDriverGetDriverDeviceName(
  OPAL_DRIVER_DEVICE          *Dev
  )
{
  EFI_HANDLE*                  AllHandlesBuffer;
  UINTN                        NumAllHandles;
  EFI_STATUS                   Status;

  if (Dev == NULL) {
    DEBUG((DEBUG_ERROR | DEBUG_INIT, "OpalDriverGetDriverDeviceName Exiting, Dev=NULL\n"));
    return FALSE;
  }

  //
  // Iterate through ComponentName2 handles to get name, if fails, try ComponentName
  //
  if (Dev->Name16 == NULL) {
    DEBUG((DEBUG_ERROR | DEBUG_INIT, "Name is null, update it\n"));
    //
    // Find all EFI_HANDLES
    //
    Status = gBS->LocateHandleBuffer(
                 AllHandles,
                 NULL,
                 NULL,
                 &NumAllHandles,
                 &AllHandlesBuffer
                 );
    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_INFO, "LocateHandleBuffer for AllHandles failed %r\n", Status ));
      return FALSE;
    }

    //
    // Try component Name2
    //
    if (!OpalDriverGetDeviceNameByProtocol(AllHandlesBuffer, NumAllHandles, Dev, FALSE)) {
      DEBUG((DEBUG_ERROR | DEBUG_INIT, "ComponentName2 failed to get device name, try ComponentName\n"));
      if (!OpalDriverGetDeviceNameByProtocol(AllHandlesBuffer, NumAllHandles, Dev, TRUE)) {
        DEBUG((DEBUG_ERROR | DEBUG_INIT, "ComponentName failed to get device name, skip device\n"));
        return FALSE;
      }
    }
  }

  return TRUE;
}

/**
  Main entry for this driver.

  @param ImageHandle     Image Handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCCESS    This function always complete successfully.
**/
EFI_STATUS
EFIAPI
EfiDriverEntryPoint(
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE*  SystemTable
  )
{
  EFI_STATUS                     Status;
  EFI_EVENT                      EndOfDxeEvent;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gOpalDriverBinding,
             ImageHandle,
             &gOpalComponentName,
             &gOpalComponentName2
             );

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Install protocols to Opal driver Handle failed\n"));
    return Status ;
  }

  //
  // Initialize Driver object
  //
  ZeroMem(&mOpalDriver, sizeof(mOpalDriver));
  mOpalDriver.Handle = ImageHandle;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OpalEndOfDxeEventNotify,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &EndOfDxeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Install Hii packages.
  //
  HiiInstall();

  return Status;
}

/**
  Tests to see if this driver supports a given controller.

  This function checks to see if the controller contains an instance of the
  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL and the EFI_BLOCK_IO_PROTOCOL
  and returns EFI_SUCCESS if it does.

  @param[in]  This                  A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle      The Handle of the controller to test. This Handle
                                    must support a protocol interface that supplies
                                    an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  This parameter is ignored.

  @retval EFI_SUCCESS               The device contains required protocols
  @retval EFI_ALREADY_STARTED       The device specified by ControllerHandle and
                                    RemainingDevicePath is already being managed by the driver
                                    specified by This.
  @retval EFI_ACCESS_DENIED         The device specified by ControllerHandle and
                                    RemainingDevicePath is already being managed by a different
                                    driver or an application that requires exclusive access.
                                    Currently not implemented.
  @retval EFI_UNSUPPORTED           The device does not contain requires protocols

**/
EFI_STATUS
EFIAPI
OpalEfiDriverBindingSupported(
  IN EFI_DRIVER_BINDING_PROTOCOL* This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL*    RemainingDevicePath
  )
{
  EFI_STATUS                              Status;
  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL*  SecurityCommand;

  if (mOpalEndOfDxe) {
    return EFI_UNSUPPORTED;
  }

  //
  // Test EFI_STORAGE_SECURITY_COMMAND_PROTOCOL on controller Handle.
  //
  Status = gBS->OpenProtocol(
    Controller,
    &gEfiStorageSecurityCommandProtocolGuid,
    ( VOID ** )&SecurityCommand,
    This->DriverBindingHandle,
    Controller,
    EFI_OPEN_PROTOCOL_BY_DRIVER
    );

  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Close protocol and reopen in Start call
  //
  gBS->CloseProtocol(
      Controller,
      &gEfiStorageSecurityCommandProtocolGuid,
      This->DriverBindingHandle,
      Controller
      );


  return EFI_SUCCESS;
}

/**
  Enables Opal Management on a supported device if available.

  The start function is designed to be called after the Opal UEFI Driver has confirmed the
  "controller", which is a child Handle, contains the EF_STORAGE_SECURITY_COMMAND protocols.
  This function will complete the other necessary checks, such as verifying the device supports
  the correct version of Opal.  Upon verification, it will add the device to the
  Opal HII list in order to expose Opal management options.

  @param[in]  This                  A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle      The Handle of the controller to start. This Handle
                                    must support a protocol interface that supplies
                                    an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath   A pointer to the remaining portion of a device path.  This
                                    parameter is ignored by device drivers, and is optional for bus
                                    drivers. For a bus driver, if this parameter is NULL, then handles
                                    for all the children of Controller are created by this driver.
                                    If this parameter is not NULL and the first Device Path Node is
                                    not the End of Device Path Node, then only the Handle for the
                                    child device specified by the first Device Path Node of
                                    RemainingDevicePath is created by this driver.
                                    If the first Device Path Node of RemainingDevicePath is
                                    the End of Device Path Node, no child Handle is created by this
                                    driver.

  @retval EFI_SUCCESS               Opal management was enabled.
  @retval EFI_DEVICE_ERROR          The device could not be started due to a device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES      The request could not be completed due to a lack of resources.
  @retval Others                    The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
OpalEfiDriverBindingStart(
  IN EFI_DRIVER_BINDING_PROTOCOL* This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL*    RemainingDevicePath
  )
{
  EFI_STATUS                  Status;
  EFI_BLOCK_IO_PROTOCOL       *BlkIo;
  OPAL_DRIVER_DEVICE          *Dev;
  OPAL_DRIVER_DEVICE          *Itr;
  BOOLEAN                     Result;

  Itr = mOpalDriver.DeviceList;
  while (Itr != NULL) {
    if (Controller == Itr->Handle) {
      return EFI_SUCCESS;
    }
    Itr = Itr->Next;
  }

  //
  // Create internal device for tracking.  This allows all disks to be tracked
  // by same HII form
  //
  Dev = (OPAL_DRIVER_DEVICE*)AllocateZeroPool(sizeof(OPAL_DRIVER_DEVICE));
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  Dev->Handle = Controller;

  //
  // Open EFI_STORAGE_SECURITY_COMMAND_PROTOCOL to perform Opal supported checks
  //
  Status = gBS->OpenProtocol(
    Controller,
    &gEfiStorageSecurityCommandProtocolGuid,
    (VOID **)&Dev->Sscp,
    This->DriverBindingHandle,
    Controller,
    EFI_OPEN_PROTOCOL_BY_DRIVER
    );
  if (EFI_ERROR(Status)) {
    FreePool(Dev);
    return Status;
  }

  //
  // Open EFI_BLOCK_IO_PROTOCOL on controller Handle, required by EFI_STORAGE_SECURITY_COMMAND_PROTOCOL
  // function APIs
  //
  Status = gBS->OpenProtocol(
    Controller,
    &gEfiBlockIoProtocolGuid,
    (VOID **)&BlkIo,
    This->DriverBindingHandle,
    Controller,
    EFI_OPEN_PROTOCOL_BY_DRIVER
    );
  if (EFI_ERROR(Status)) {
    //
    // Block_IO not supported on handle
    //
    if(Status == EFI_UNSUPPORTED) {
      BlkIo = NULL;
    } else {
      //
      // Close storage security that was opened
      //
      gBS->CloseProtocol(
          Controller,
          &gEfiStorageSecurityCommandProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );

      FreePool(Dev);
      return Status;
    }
  }

  //
  // Save mediaId
  //
  if(BlkIo == NULL) {
    // If no Block IO present, use defined MediaId value.
    Dev->MediaId = 0x0;
  } else {
    Dev->MediaId = BlkIo->Media->MediaId;

    gBS->CloseProtocol(
      Controller,
      &gEfiBlockIoProtocolGuid,
      This->DriverBindingHandle,
      Controller
    );
  }

  //
  // Acquire Ascii printable name of child, if not found, then ignore device
  //
  Result = OpalDriverGetDriverDeviceName (Dev);
  if (!Result) {
    goto Done;
  }

  Status = OpalDiskInitialize (Dev);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  AddDeviceToTail(Dev);

  //
  // Check if device is locked and prompt for password.
  //
  OpalDriverRequestPassword (Dev, L"Unlock:");

  //
  // Process OPAL request from last boot.
  //
  ProcessOpalRequest (Dev);

  return EFI_SUCCESS;

Done:
  //
  // free device, close protocols and exit
  //
  gBS->CloseProtocol(
      Controller,
      &gEfiStorageSecurityCommandProtocolGuid,
      This->DriverBindingHandle,
      Controller
      );

  FreePool(Dev);

  return EFI_DEVICE_ERROR;
}

/**
  Stop this driver on Controller.

  @param  This              Protocol instance pointer.
  @param  Controller        Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed Controller.
  @retval other             This driver could not be removed from this device.

**/
EFI_STATUS
EFIAPI
OpalEfiDriverBindingStop(
  EFI_DRIVER_BINDING_PROTOCOL*    This,
  EFI_HANDLE                      Controller,
  UINTN                           NumberOfChildren,
  EFI_HANDLE*                     ChildHandleBuffer
  )
{
  OPAL_DRIVER_DEVICE*   Itr;

  Itr = mOpalDriver.DeviceList;

  //
  // does Controller match any of the devices we are managing for Opal
  //
  while (Itr != NULL) {
    if (Itr->Handle == Controller) {
      OpalDriverStopDevice (Itr);
      return EFI_SUCCESS;
    }

    Itr = Itr->Next;
  }

  return EFI_NOT_FOUND;
}


/**
  Unloads UEFI Driver.  Very useful for debugging and testing.

  @param ImageHandle            Image Handle this driver.

  @retval EFI_SUCCESS           This function always complete successfully.
  @retval EFI_INVALID_PARAMETER The input ImageHandle is not valid.
**/
EFI_STATUS
EFIAPI
OpalEfiDriverUnload (
  IN EFI_HANDLE ImageHandle
  )
{
  EFI_STATUS                     Status;
  OPAL_DRIVER_DEVICE                   *Itr;

  Status = EFI_SUCCESS;

  if (ImageHandle != gImageHandle) {
    return (EFI_INVALID_PARAMETER);
  }

  //
  // Uninstall any interface added to each device by us
  //
  while (mOpalDriver.DeviceList) {
    Itr = mOpalDriver.DeviceList;
    //
    // Remove OPAL_DRIVER_DEVICE from the list
    // it updates the controllerList pointer
    //
    OpalDriverStopDevice(Itr);
  }

  //
  // Uninstall the HII capability
  //
  Status = HiiUninstall();

  return Status;
}

