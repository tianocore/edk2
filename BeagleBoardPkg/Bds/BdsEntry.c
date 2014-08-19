/** @file
  The entry of the embedded BDS. This BDS does not follow the Boot Manager requirements
  of the UEFI specification as it is designed to implement an embedded systmes
  propriatary boot scheme.

  This template assume a DXE driver produces a SerialIo protocol not using the EFI
  driver module and it will attempt to connect a console on top of this.

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BdsEntry.h"


BOOLEAN     gConsolePresent = FALSE;

EFI_BDS_ARCH_PROTOCOL  gBdsProtocol = {
  BdsEntry,
};




/**
  This function uses policy data from the platform to determine what operating
  system or system utility should be loaded and invoked.  This function call
  also optionally make the use of user input to determine the operating system
  or system utility to be loaded and invoked.  When the DXE Core has dispatched
  all the drivers on the dispatch queue, this function is called.  This
  function will attempt to connect the boot devices required to load and invoke
  the selected operating system or system utility.  During this process,
  additional firmware volumes may be discovered that may contain addition DXE
  drivers that can be dispatched by the DXE Core.   If a boot device cannot be
  fully connected, this function calls the DXE Service Dispatch() to allow the
  DXE drivers from any newly discovered firmware volumes to be dispatched.
  Then the boot device connection can be attempted again.  If the same boot
  device connection operation fails twice in a row, then that boot device has
  failed, and should be skipped.  This function should never return.

  @param  This             The EFI_BDS_ARCH_PROTOCOL instance.

  @return None.

**/
VOID
EFIAPI
BdsEntry (
  IN EFI_BDS_ARCH_PROTOCOL  *This
  )
{
  EFI_STATUS                Status;
  UINTN                     NoHandles;
  EFI_HANDLE                *Buffer;
  EFI_HANDLE                FvHandle;
  EFI_HANDLE                ImageHandle;
  EFI_HANDLE                UsbDeviceHandle;
  EFI_GUID                  NameGuid;
  UINTN                     Size;
  UINTN                     HandleCount;
  UINTN                     OldHandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL  *LoadImageDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *FileSystemDevicePath;

  PERF_END   (NULL, "DXE", NULL, 0);
  PERF_START (NULL, "BDS", NULL, 0);


  //
  // Now do the EFI stuff
  //
  Size = 0x100;
  gST->FirmwareVendor = AllocateRuntimePool (Size);
  ASSERT (gST->FirmwareVendor != NULL);

  UnicodeSPrint (gST->FirmwareVendor, Size, L"BeagleBoard EFI %a %a", __DATE__, __TIME__);

  //
  // Now we need to setup the EFI System Table with information about the console devices.
  // This code is normally in the console spliter driver on platforms that support multiple
  // consoles at the same time
  //
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiSimpleTextOutProtocolGuid, NULL, &NoHandles, &Buffer);
  if (!EFI_ERROR (Status)) {
    // Use the first SimpleTextOut we find and update the EFI System Table
    gST->ConsoleOutHandle = Buffer[0];
    gST->StandardErrorHandle = Buffer[0];
    Status = gBS->HandleProtocol (Buffer[0], &gEfiSimpleTextOutProtocolGuid, (VOID **)&gST->ConOut);
    ASSERT_EFI_ERROR (Status);

    gST->StdErr = gST->ConOut;

    gST->ConOut->OutputString (gST->ConOut, L"BDS: Console Started!!!!\n\r");
    FreePool (Buffer);

    gConsolePresent = TRUE;
  }


  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiSimpleTextInProtocolGuid, NULL, &NoHandles, &Buffer);
  if (!EFI_ERROR (Status)) {
    // Use the first SimpleTextIn we find and update the EFI System Table
    gST->ConsoleInHandle = Buffer[0];
    Status = gBS->HandleProtocol (Buffer[0], &gEfiSimpleTextInProtocolGuid, (VOID **)&gST->ConIn);
    ASSERT_EFI_ERROR (Status);

    FreePool (Buffer);
  }

  //
  // We now have EFI Consoles up and running. Print () will work now. DEBUG () and ASSERT () worked
  // prior to this point as they were configured to use a more primative output scheme.
  //

  //
  //Perform Connect
  //
  HandleCount = 0;
  while (1) {
    OldHandleCount = HandleCount;
    Status = gBS->LocateHandleBuffer (
                    AllHandles,
                    NULL,
                    NULL,
                    &HandleCount,
                    &HandleBuffer
                    );
    if (EFI_ERROR (Status)) {
      break;
    }

    if (HandleCount == OldHandleCount) {
      break;
    }

    for (Index = 0; Index < HandleCount; Index++) {
      gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
    }
  }

  EfiSignalEventReadyToBoot ();

  //Locate handles for SimpleFileSystem protocol
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (!EFI_ERROR(Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      //Get the device path
      FileSystemDevicePath = DevicePathFromHandle(HandleBuffer[Index]);
      if (FileSystemDevicePath == NULL) {
        continue;
      }

      //Check if UsbIo is on any handles in the device path.
      Status = gBS->LocateDevicePath(&gEfiUsbIoProtocolGuid, &FileSystemDevicePath, &UsbDeviceHandle);
      if (EFI_ERROR(Status)) {
        continue;
      }

      //Check if Usb stick has a magic EBL file.
      LoadImageDevicePath = FileDevicePath(HandleBuffer[Index], L"Ebl.efi");
      Status = gBS->LoadImage (TRUE, gImageHandle, LoadImageDevicePath, NULL, 0, &ImageHandle);
      if (EFI_ERROR(Status)) {
        continue;
      }

      //Boot to Shell on USB stick.
      Status = gBS->StartImage (ImageHandle, NULL, NULL);
      if (EFI_ERROR(Status)) {
        continue;
      }
    }
  }

  //
  // Normal UEFI behavior is to process Globally Defined Variables as defined in Chapter 3
  // (Boot Manager) of the UEFI specification. For this embedded system we don't do this.
  //

  //
  // Search all the FVs for an application with a UI Section of Ebl. A .FDF file can be used
  // to control the names of UI sections in an FV.
  //
  Status = FindApplicationMatchingUiSection (L"Ebl", &FvHandle, &NameGuid);
  if (!EFI_ERROR (Status)) {

    //Boot to Shell.
    Status = LoadPeCoffSectionFromFv (FvHandle, &NameGuid);

    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "Boot from Shell failed. Status: %r\n", Status));
    }
  }

  //
  // EFI does not define the behaviour if all boot attemps fail and the last one returns.
  // So we make a policy choice to reset the system since this BDS does not have a UI.
  //
  gRT->ResetSystem (EfiResetShutdown, Status, 0, NULL);

  return ;
}


EFI_STATUS
EFIAPI
BdsInitialize (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install protocol interface
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiBdsArchProtocolGuid, &gBdsProtocol,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}


