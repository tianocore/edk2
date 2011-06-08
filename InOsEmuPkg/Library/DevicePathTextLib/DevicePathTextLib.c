/** @file
  Null Platform Hook Library instance.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Protocol/EmuThunk.h>
#include <Protocol/EmuGraphicsWindow.h>
#include <Protocol/EmuBlockIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/EmuThread.h>

#include <Library/DevicePathTextLib.h>
#include <Library/BaseMemoryLib.h>


/**
  Converts a Vendor device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

  @return EFI_NOT_FOUND if no string representation exists.
  @return EFI_SUCCESS   a string representation was created.
**/
EFI_STATUS
EFIAPI
DevPathToTextVendorLib (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  EMU_VENDOR_DEVICE_PATH_NODE  *Vendor;
  CHAR16                       *Type;

  Vendor = (EMU_VENDOR_DEVICE_PATH_NODE *)DevPath;
  if (CompareGuid (&Vendor->VendorDevicePath.Guid, &gEmuThunkProtocolGuid)) {
    CatPrint (Str, L"EmuThunk()");
    return EFI_SUCCESS;
  }
  if (CompareGuid (&Vendor->VendorDevicePath.Guid, &gEmuGraphicsWindowProtocolGuid)) {
    CatPrint (Str, L"EmuGraphics(%d)", Vendor->Instance);
    return EFI_SUCCESS;
  }
  if (CompareGuid (&Vendor->VendorDevicePath.Guid, &gEfiSimpleFileSystemProtocolGuid)) {
    CatPrint (Str, L"EmuFs(%d)", Vendor->Instance);
    return EFI_SUCCESS;
  }
  if (CompareGuid (&Vendor->VendorDevicePath.Guid, &gEmuBlockIoProtocolGuid)) {
    CatPrint (Str, L"EmuBlk(%d)", Vendor->Instance);
    return EFI_SUCCESS;
  }
  if (CompareGuid (&Vendor->VendorDevicePath.Guid, &gEmuThreadThunkProtocolGuid)) {
    CatPrint (Str, L"EmuThread()");
    return EFI_SUCCESS;
  }
  
  return EFI_NOT_FOUND;
}

