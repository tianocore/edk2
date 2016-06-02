/** @file
  Entrypoint of Opal UEFI Driver and contains all the logic to
  register for new Opal device instances.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

// This UEFI driver consumes EFI_STORAGE_SECURITY_PROTOCOL instances and installs an
// HII GUI to manage Opal features if the device is Opal capable
// If the Opal device is being managed by the UEFI Driver, it shall provide a popup
// window during boot requesting a user password

#include "OpalDriver.h"
#include "OpalDriverPrivate.h"
#include "OpalHii.h"

OPAL_DRIVER mOpalDriver;

#define MAX_PASSWORD_SIZE        32
#define MAX_PASSWORD_TRY_COUNT   5

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
  Get password input from the popup windows, and unlock the device.

  @param[in]       Dev          The device which need to be unlock.
  @param[out]      PressEsc     Whether user escape function through Press ESC.

  @retval   Password string if success. NULL if failed.

**/
CHAR8 *
OpalDriverPopUpHddPassword (
  IN  OPAL_DRIVER_DEVICE         *Dev,
  OUT BOOLEAN                    *PressEsc
  )
{
  EFI_INPUT_KEY                InputKey;
  UINTN                        InputLength;
  CHAR16                       Mask[MAX_PASSWORD_SIZE + 1];
  CHAR16                       Unicode[MAX_PASSWORD_SIZE + 1];
  CHAR8                        *Ascii;
  CHAR16                       *PopUpString;
  UINTN                        StrLength;

  ZeroMem(Unicode, sizeof(Unicode));
  ZeroMem(Mask, sizeof(Mask));

  StrLength = StrLen(Dev->Name16);
  PopUpString = (CHAR16*) AllocateZeroPool ((8 + StrLength) * 2);
  *PressEsc = FALSE;

  if (Dev->Name16 == NULL) {
    UnicodeSPrint(PopUpString, StrLen(L"Unlock Disk") + 1, L"Unlock Disk");
  } else {
    UnicodeSPrint(PopUpString, StrLen(L"Unlock ") + StrLength + 1, L"Unlock %s", Dev->Name16);
  }

  gST->ConOut->ClearScreen(gST->ConOut);

  InputLength = 0;
  while (TRUE) {
    Mask[InputLength] = L'_';
    CreatePopUp(
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &InputKey,
        PopUpString,
        L"---------------------",
        Mask,
        NULL
    );

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
        InputLength++;
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
          if (InputLength == MAX_PASSWORD_SIZE) {
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
    return NULL;
  }

  Ascii = AllocateZeroPool (MAX_PASSWORD_SIZE + 1);
  if (Ascii == NULL) {
    return NULL;
  }

  UnicodeStrToAsciiStrS (Unicode, Ascii, MAX_PASSWORD_SIZE + 1);
  ZeroMem (Unicode, sizeof (Unicode));

  return Ascii;
}

/**
  Check if disk is locked, show popup window and ask for password if it is

  @param[in]       Dev          The device which need to be unlock.

**/
VOID
OpalDriverRequestPassword (
  OPAL_DRIVER_DEVICE       *Dev
  )
{
  UINT8               Count;
  BOOLEAN             IsEnabled;
  CHAR8               *Password;
  UINT32              PasswordLen;
  TCG_RESULT          Ret;
  EFI_INPUT_KEY       Key;
  OPAL_SESSION        Session;
  BOOLEAN             PressEsc;
  BOOLEAN             Locked;

  if (Dev == NULL) {
    return;
  }

  Count = 0;

  IsEnabled = OpalFeatureEnabled (&Dev->OpalDisk.SupportedAttributes, &Dev->OpalDisk.LockingFeature);
  if (IsEnabled) {
    ZeroMem(&Session, sizeof(Session));
    Session.Sscp = Dev->OpalDisk.Sscp;
    Session.MediaId = Dev->OpalDisk.MediaId;
    Session.OpalBaseComId = Dev->OpalDisk.OpalBaseComId;

    Locked = OpalDeviceLocked (&Dev->OpalDisk.SupportedAttributes, &Dev->OpalDisk.LockingFeature);

    while (Count < MAX_PASSWORD_TRY_COUNT) {
      Password = OpalDriverPopUpHddPassword (Dev, &PressEsc);
      if (PressEsc) {
        if (Locked) {
          //
          // Current device in the lock status and
          // User not input password and press ESC,
          // keep device in lock status and continue boot.
          //
          do {
            CreatePopUp (
                    EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
                    &Key,
                    L"Press ENTER to skip password, Press ESC to input password",
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
                    L"Press ENTER to shutdown, Press ESC to input password",
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

      if (Locked) {
        Ret = OpalSupportUnlock(&Session, Password, PasswordLen, Dev->OpalDevicePath);
      } else {
        Ret = OpalSupportLock(&Session, Password, PasswordLen, Dev->OpalDevicePath);
        if (Ret == TcgResultSuccess) {
          Ret = OpalSupportUnlock(&Session, Password, PasswordLen, Dev->OpalDevicePath);
        }
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
  ReadyToBoot callback to send BlockSid command.

  @param  Event   Pointer to this event
  @param  Context Event handler private Data

**/
VOID
EFIAPI
ReadyToBootCallback (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  OPAL_DRIVER_DEVICE                         *Itr;
  TCG_RESULT                                 Result;
  OPAL_SESSION                               Session;
  UINT32                                     PpStorageFlag;

  gBS->CloseEvent (Event);

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

        Result = OpalBlockSid (&Session, TRUE);  // HardwareReset must always be TRUE
        if (Result != TcgResultSuccess) {
          DEBUG ((DEBUG_ERROR, "OpalBlockSid fail\n"));
          break;
        }
      }

      Itr = Itr->Next;
    }
  }
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

  @retval EFI_SUCESS     This function always complete successfully.
**/
EFI_STATUS
EFIAPI
EfiDriverEntryPoint(
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE*  SystemTable
  )
{
  EFI_STATUS                     Status;
  EFI_EVENT                      ReadyToBootEvent;

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

  //
  // register a ReadyToBoot event callback for sending BlockSid command
  //
  Status = EfiCreateEventReadyToBootEx (
                  TPL_CALLBACK,
                  ReadyToBootCallback,
                  (VOID *) &ImageHandle,
                  &ReadyToBootEvent
                  );

  //
  // Install Hii packages.
  //
  HiiInstall();

  return Status;
}

/**
  Tests to see if this driver supports a given controller.

  This function checks to see if the controller contains an instance of the
  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL and the EFI_BLOCK_IO_PROTOCL
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
  EFI_BLOCK_IO_PROTOCOL*                  BlkIo;

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

  //
  // Test EFI_BLOCK_IO_PROTOCOL on controller Handle, required by EFI_STORAGE_SECURITY_COMMAND_PROTOCOL
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
    DEBUG((DEBUG_INFO, "No EFI_BLOCK_IO_PROTOCOL on controller\n"));
    return Status;
  }

  //
  // Close protocol and reopen in Start call
  //
  gBS->CloseProtocol(
    Controller,
    &gEfiBlockIoProtocolGuid,
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
  Opal HII list in order to expose Opal managmeent options.

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

  //
  // Save mediaId
  //
  Dev->MediaId = BlkIo->Media->MediaId;

  gBS->CloseProtocol(
    Controller,
    &gEfiBlockIoProtocolGuid,
    This->DriverBindingHandle,
    Controller
    );

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
  // check if device is locked and prompt for password
  //
  OpalDriverRequestPassword (Dev);

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

