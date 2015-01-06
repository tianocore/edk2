/** @file
*
*  Copyright (c) 2014-2015, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "ArmJunoDxeInternal.h"

#include <Protocol/BlockIo.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/BaseMemoryLib.h>
#include <Library/BdsLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Guid/ArmGlobalVariableHob.h>
#include <Guid/ArmPlatformEvents.h>
#include <Guid/EventGroup.h>
#include <Guid/Fdt.h>
#include <Guid/FileInfo.h>

#include <libfdt.h>

#define FDT_DEFAULT_FILENAME  L"juno"

#define IS_DEVICE_PATH_NODE(node,type,subtype) (((node)->Type == (type)) && ((node)->SubType == (subtype)))

// Hardware Vendor Device Path node for the Juno NOR Flash. We use the Juno NOR Flash if the user
// has not specified another filesystem location into the UEFI Variable 'Fdt'.
// The Juno NOR Flash has its own filesystem format (supported by ArmPlatformPkg/FileSystem/BootMonFs).
STATIC CONST struct {
  VENDOR_DEVICE_PATH NorGuid;
  EFI_DEVICE_PATH    End;
} mJunoNorFlashDevicePath = {
  {
    { HARDWARE_DEVICE_PATH, HW_VENDOR_DP, { sizeof (VENDOR_DEVICE_PATH), 0 } },
    {0xE7223039, 0x5836, 0x41E1, { 0xB5, 0x42, 0xD7, 0xEC, 0x73, 0x6C, 0x5E, 0x59} }
  },
  { END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 } }
};

STATIC EFI_DEVICE_PATH* mFdtFileSystemDevicePath = NULL;
STATIC CHAR16* mFdtFileName = NULL;

STATIC BOOLEAN mFdtTableInstalled = FALSE;

/**
  See definition EFI_DRIVER_BINDING_PROTOCOL.Supported()
**/
EFI_STATUS
EFIAPI
JunoFdtSupported (
  IN        EFI_DRIVER_BINDING_PROTOCOL *DriverBinding,
  IN        EFI_HANDLE                   ControllerHandle,
  IN        EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;

  //
  // Check if the Handle support the Simple File System Protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  gImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Check if a DevicePath is attached to the handle
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath,
                  gImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Check if the Device Path is the one from the NOR Flash
  if (CompareMem (mFdtFileSystemDevicePath, DevicePath, GetDevicePathSize (mFdtFileSystemDevicePath)) != 0) {
    return EFI_NOT_FOUND;
  }

  gBS->CloseProtocol (ControllerHandle, &gEfiDevicePathProtocolGuid, gImageHandle, ControllerHandle);
  return Status;
}

/**
  This function is used to print messages back to the user.

  We use the Serial terminal for these messages as the gST->ConOut might not be initialized at this stage.

  @param Message    Message to display to the user
**/
STATIC
VOID
PrintMessage (
  IN CHAR8* Message,
  ...
  )
{
  UINTN   CharCount;
  CHAR8   Buffer[100];
  VA_LIST Marker;

  VA_START (Marker, Message);
  CharCount = AsciiVSPrint (Buffer, sizeof (Buffer), Message, Marker);
  VA_END (Marker);

  SerialPortWrite ((UINT8*)Buffer, CharCount);
}

/**
  See definition EFI_DRIVER_BINDING_PROTOCOL.Start ()
**/
EFI_STATUS
EFIAPI
JunoFdtStart (
  IN        EFI_DRIVER_BINDING_PROTOCOL *DriverBinding,
  IN        EFI_HANDLE                   ControllerHandle,
  IN        EFI_DEVICE_PATH_PROTOCOL    *DevicePath OPTIONAL
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *BootMonFs;
  EFI_FILE_PROTOCOL               *Fs;
  EFI_FILE_PROTOCOL               *File;
  UINTN                            Size;
  EFI_PHYSICAL_ADDRESS             FdtBlob;
  EFI_FILE_INFO                   *FileInfo;

  if (mFdtTableInstalled) {
    return EFI_ALREADY_STARTED;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID**)&BootMonFs,
                  gImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Try to Open the volume and get root directory
  Status = BootMonFs->OpenVolume (BootMonFs, &Fs);
  if (EFI_ERROR (Status)) {
    PrintMessage ("Warning: Fail to open file system that should contain FDT file.\n");
    goto CLOSE_PROTOCOL;
  }

  File = NULL;
  Status = Fs->Open (Fs, &File, mFdtFileName, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    PrintMessage ("Warning: Fail to load FDT file '%s'.\n", mFdtFileName);
    goto CLOSE_PROTOCOL;
  }

  Size = 0;
  File->GetInfo (File, &gEfiFileInfoGuid, &Size, NULL);
  FileInfo = AllocatePool (Size);
  Status = File->GetInfo (File, &gEfiFileInfoGuid, &Size, FileInfo);
  if (EFI_ERROR (Status)) {
    goto CLOSE_FILE;
  }

  // Get the file size
  Size = FileInfo->FileSize;
  FreePool (FileInfo);

  // The FDT blob is attached to the Configuration Table. It is better to load it as Runtime Service Data
  // to prevent the kernel to overwrite its data
  Status = gBS->AllocatePages (AllocateAnyPages, EfiRuntimeServicesData, EFI_SIZE_TO_PAGES (Size), &FdtBlob);
  if (!EFI_ERROR (Status)) {
    Status = File->Read (File, &Size, (VOID*)(UINTN)(FdtBlob));
    if (EFI_ERROR (Status)) {
      gBS->FreePages (FdtBlob, EFI_SIZE_TO_PAGES (Size));
    } else {
      // Check the FDT header is valid. We only make this check in DEBUG mode in case the FDT header change on
      // production device and this ASSERT() becomes not valid.
      ASSERT (fdt_check_header ((VOID*)(UINTN)(FdtBlob)) == 0);

      // Ensure the Size of the Device Tree is smaller than the size of the read file
      ASSERT ((UINTN)fdt_totalsize ((VOID*)(UINTN)FdtBlob) <= Size);

      // Install the FDT into the Configuration Table
      Status = gBS->InstallConfigurationTable (&gFdtTableGuid, (VOID*)(UINTN)(FdtBlob));
      if (!EFI_ERROR (Status)) {
        mFdtTableInstalled = TRUE;
      }
    }
  }

CLOSE_FILE:
  File->Close (File);

CLOSE_PROTOCOL:
  // We do not need the FileSystem protocol
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiSimpleFileSystemProtocolGuid,
         gImageHandle,
         ControllerHandle);

  return Status;
}

/**
  See definition EFI_DRIVER_BINDING_PROTOCOL.Stop()
**/
EFI_STATUS
EFIAPI
JunoFdtStop (
  IN        EFI_DRIVER_BINDING_PROTOCOL *DriverBinding,
  IN        EFI_HANDLE                   ControllerHandle,
  IN        UINTN                        NumberOfChildren,
  IN        EFI_HANDLE                  *ChildHandleBuffer OPTIONAL
  )
{
  UINTN      Index;
  VOID*      FdtBlob;
  UINTN      FdtSize;

  // Look for FDT Table
  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    // Check for correct GUID type
    if (CompareGuid (&gFdtTableGuid, &(gST->ConfigurationTable[Index].VendorGuid))) {
      FdtBlob = gST->ConfigurationTable[Index].VendorTable;
      FdtSize = (UINTN)fdt_totalsize (FdtBlob);

      // Uninstall the FDT Configuration Table
      gBS->InstallConfigurationTable (&gFdtTableGuid, NULL);

      // Free the memory
      gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)FdtBlob, EFI_SIZE_TO_PAGES (FdtSize));

      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

//
// Driver Binding Protocol for Juno FDT support
//
EFI_DRIVER_BINDING_PROTOCOL mJunoFdtBinding = {
  JunoFdtSupported,
  JunoFdtStart,
  JunoFdtStop,
  0xa,
  NULL,
  NULL
};

/**
  Notification function of EFI_END_OF_DXE_EVENT_GROUP_GUID event group.

  This is a notification function registered on EFI_END_OF_DXE_EVENT_GROUP_GUID event group.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.

**/
STATIC
VOID
EFIAPI
LoadFdtOnEvent (
  EFI_EVENT                               Event,
  VOID                                    *Context
  )
{
  EFI_DEVICE_PATH *DevicePathNode;
  EFI_HANDLE       Handle;
  EFI_STATUS       Status;
  UINTN            VariableSize;
  CHAR16*          FdtDevicePathStr;
  EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL  *EfiDevicePathFromTextProtocol;
  EFI_EVENT        ArmPlatformUpdateFdtEvent;

  //
  // Read the 'FDT' UEFI Variable to know where we should we read the blob from.
  // The 'Fdt' variable contains either the full device path or only the filename of the FDT.
  // If 'Fdt' only contains the filename then we assume its location is on the NOR Flash.
  //
  VariableSize     = 0;
  Status = gRT->GetVariable (L"Fdt", &gArmGlobalVariableGuid, NULL, &VariableSize, mFdtFileSystemDevicePath);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    // Get the environment variable value
    mFdtFileSystemDevicePath = AllocatePool (VariableSize);
    if (mFdtFileSystemDevicePath != NULL) {
      Status = gRT->GetVariable (L"Fdt", &gArmGlobalVariableGuid, NULL, &VariableSize, mFdtFileSystemDevicePath);
      if (EFI_ERROR (Status)) {
        FreePool (mFdtFileSystemDevicePath);
        ASSERT_EFI_ERROR (Status);
        return;
      }
    } else {
      ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
      return;
    }
  } else if (Status == EFI_NOT_FOUND) {
    // If the 'Fdt' variable does not exist then we get the FDT location from the PCD
    FdtDevicePathStr = (CHAR16*)PcdGetPtr (PcdFdtDevicePath);

    Status = gBS->LocateProtocol (&gEfiDevicePathFromTextProtocolGuid, NULL, (VOID **)&EfiDevicePathFromTextProtocol);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return;
    }

    // Conversion of the Device Path string into EFI Device Path
    mFdtFileSystemDevicePath = EfiDevicePathFromTextProtocol->ConvertTextToDevicePath (FdtDevicePathStr);
  }

  if (mFdtFileSystemDevicePath != NULL) {
    // Look for the FDT filename that should be contained into the FilePath device path node
    DevicePathNode = mFdtFileSystemDevicePath;
    while (!IsDevicePathEnd (DevicePathNode)) {
      if (IS_DEVICE_PATH_NODE (DevicePathNode, MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP)) {
        // Extract the name from the File Path Node. The name of the Filename is the size of the
        // device path node minus the size of the device path node header.
        mFdtFileName = AllocateCopyPool (
            DevicePathNodeLength (DevicePathNode) - sizeof(EFI_DEVICE_PATH_PROTOCOL),
            ((FILEPATH_DEVICE_PATH*)DevicePathNode)->PathName);
        if (mFdtFileName == NULL) {
          ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
          return;
        }

        // We remove the FilePath device path node from the FileSystem Device Path
        // because it will never match a device path installed by the FileSystem driver
        SetDevicePathEndNode (DevicePathNode);
        break;
      }
      DevicePathNode = NextDevicePathNode (DevicePathNode);
    }

    // The UEFI Variable might just contain the FDT filename. In this case we assume the FileSystem is
    // the NOR Flash based one (ie: BootMonFs).
    // If it was only containing the FilePath device node then the previous condition should have
    // replaced it by the End Device Path Node.
    if (IsDevicePathEndType (mFdtFileSystemDevicePath)) {
      mFdtFileSystemDevicePath = (EFI_DEVICE_PATH*)&mJunoNorFlashDevicePath;
    }
  } else {
    // Fallback on the NOR Flash filesystem
    mFdtFileSystemDevicePath = (EFI_DEVICE_PATH*)&mJunoNorFlashDevicePath;
  }

  // If the FDT FileName has been provided during the FileSystem identification
  if (mFdtFileName == NULL) {
    mFdtFileName = AllocateCopyPool (StrSize (FDT_DEFAULT_FILENAME), FDT_DEFAULT_FILENAME);
    if (mFdtFileName == NULL) {
      ASSERT_EFI_ERROR (Status);
      return;
    }
  }

  // Context is not NULL when this function is called for a gEfiEndOfDxeEventGroupGuid event
  if (Context) {
    // Install the Binding protocol to verify when the FileSystem that contains the FDT has been installed
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &gImageHandle,
                    &gEfiDriverBindingProtocolGuid, &mJunoFdtBinding,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return;
    }

    // Register the event triggered when the 'Fdt' variable is updated.
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    LoadFdtOnEvent,
                    NULL,
                    &gArmPlatformUpdateFdtEventGuid,
                    &ArmPlatformUpdateFdtEvent
                    );
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Force to connect the FileSystem that contains the FDT
  //
  BdsConnectDevicePath (mFdtFileSystemDevicePath, &Handle, NULL);
}

STATIC CONST BOOLEAN mIsEndOfDxeEvent = TRUE;

EFI_STATUS
JunoFdtInstall (
  IN EFI_HANDLE                            ImageHandle
  )
{
  EFI_STATUS Status;
  EFI_EVENT  EndOfDxeEvent;

  // Register the event handling function to set the End Of DXE flag.
  // We wait until the end of the DXE phase to load the FDT to make sure
  // all the required drivers (NOR Flash, UEFI Variable, BootMonFs) are dispatched
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  LoadFdtOnEvent,
                  &mIsEndOfDxeEvent,
                  &gEfiEndOfDxeEventGroupGuid,
                  &EndOfDxeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
