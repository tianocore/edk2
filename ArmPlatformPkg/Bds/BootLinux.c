/** @file
*
*  Copyright (c) 2011 - 2015, ARM Limited. All rights reserved.
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

#include "BdsInternal.h"

// This GUID is defined in the INGF file of ArmPkg/Application/LinuxLoader
CONST EFI_GUID mLinuxLoaderAppGuid = { 0x701f54f2, 0x0d70, 0x4b89, { 0xbc, 0x0a, 0xd9, 0xca, 0x25, 0x37, 0x90, 0x59 }};

// Device path of the EFI Linux Loader in the Firmware Volume
EFI_DEVICE_PATH* mLinuxLoaderDevicePath = NULL;

STATIC
BOOLEAN
HasFilePathEfiExtension (
  IN CHAR16* FilePath
  )
{
  return (StrCmp (FilePath + (StrSize (FilePath) / sizeof (CHAR16)) - 5, L".EFI") == 0) ||
         (StrCmp (FilePath + (StrSize (FilePath) / sizeof (CHAR16)) - 5, L".efi") == 0);
}

/**
 * This function check if the DevicePath defines an EFI binary
 *
 * This function is used when the BDS support Linux loader to
 * detect if the binary is an EFI application or potentially a
 * Linux kernel.
 */
EFI_STATUS
IsEfiBinary (
  IN  EFI_DEVICE_PATH* DevicePath,
  OUT BOOLEAN          *EfiBinary
  )
{
  EFI_STATUS              Status;
  CHAR16*                 FileName;
  EFI_DEVICE_PATH*        PrevDevicePathNode;
  EFI_DEVICE_PATH*        DevicePathNode;
  EFI_PHYSICAL_ADDRESS    Image;
  UINTN                   FileSize;
  EFI_IMAGE_DOS_HEADER*   DosHeader;
  UINTN                   PeCoffHeaderOffset;
  EFI_IMAGE_NT_HEADERS32* NtHeader;

  ASSERT (EfiBinary != NULL);

  //
  // Check if the last node of the device path is a FilePath node
  //
  PrevDevicePathNode = NULL;
  DevicePathNode = DevicePath;
  while ((DevicePathNode != NULL) && !IsDevicePathEnd (DevicePathNode)) {
    PrevDevicePathNode = DevicePathNode;
    DevicePathNode     = NextDevicePathNode (DevicePathNode);
  }

  if ((PrevDevicePathNode != NULL) &&
      (PrevDevicePathNode->Type == MEDIA_DEVICE_PATH) &&
      (PrevDevicePathNode->SubType == MEDIA_FILEPATH_DP))
  {
    FileName = ((FILEPATH_DEVICE_PATH*)PrevDevicePathNode)->PathName;
  } else {
    FileName = NULL;
  }

  if (FileName == NULL) {
    Print (L"Is an EFI Application? ");
    Status = GetHIInputBoolean (EfiBinary);
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  } else if (HasFilePathEfiExtension (FileName)) {
    *EfiBinary = TRUE;
  } else {
    // Check if the file exist
    Status = BdsLoadImage (DevicePath, AllocateAnyPages, &Image, &FileSize);
    if (!EFI_ERROR (Status)) {

      DosHeader = (EFI_IMAGE_DOS_HEADER *)(UINTN) Image;
      if (DosHeader->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
        //
        // DOS image header is present,
        // so read the PE header after the DOS image header.
        //
        PeCoffHeaderOffset = DosHeader->e_lfanew;
      } else {
        PeCoffHeaderOffset = 0;
      }

      //
      // Check PE/COFF image.
      //
      NtHeader = (EFI_IMAGE_NT_HEADERS32 *)(UINTN) (Image + PeCoffHeaderOffset);
      if (NtHeader->Signature != EFI_IMAGE_NT_SIGNATURE) {
        *EfiBinary = FALSE;
      } else {
        *EfiBinary = TRUE;
      }

      // Free memory
      gBS->FreePages (Image, EFI_SIZE_TO_PAGES (FileSize));
    } else {
      // If we did not manage to open it then ask for the type
      Print (L"Is an EFI Application? ");
      Status = GetHIInputBoolean (EfiBinary);
      if (EFI_ERROR (Status)) {
        return EFI_ABORTED;
      }
    }
  }

  return EFI_SUCCESS;
}
