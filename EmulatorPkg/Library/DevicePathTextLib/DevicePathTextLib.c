/** @file
  Null Platform Hook Library instance.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

#include <Protocol/EmuThunk.h>
#include <Protocol/EmuGraphicsWindow.h>
#include <Protocol/EmuBlockIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/EmuThread.h>

#include <Library/BaseLib.h>
#include <Library/DevicePathToTextLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>


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

/**
  Converts a text device path node to Hardware Vendor device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Hardware Vendor device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextEmuThunk (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16              *Str;
  VENDOR_DEVICE_PATH  *Vendor;

  Str    = GetNextParamStr (&TextDeviceNode);
  Vendor = (VENDOR_DEVICE_PATH *) CreateDeviceNode (
                                     HARDWARE_DEVICE_PATH,
                                     HW_VENDOR_DP,
                                     (UINT16) sizeof (VENDOR_DEVICE_PATH)
                                     );
  CopyGuid (&Vendor->Guid, &gEmuThunkProtocolGuid);
  return (EFI_DEVICE_PATH_PROTOCOL *) Vendor;
}

/**
  Converts a text device path node to Hardware Vendor device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Hardware Vendor device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextEmuThread (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16              *Str;
  VENDOR_DEVICE_PATH  *Vendor;

  Str    = GetNextParamStr (&TextDeviceNode);
  Vendor = (VENDOR_DEVICE_PATH *) CreateDeviceNode (
                                     HARDWARE_DEVICE_PATH,
                                     HW_VENDOR_DP,
                                     (UINT16) sizeof (VENDOR_DEVICE_PATH)
                                     );
  CopyGuid (&Vendor->Guid, &gEmuThreadThunkProtocolGuid);
  return (EFI_DEVICE_PATH_PROTOCOL *) Vendor;
}

/**
  Converts a text device path node to Hardware Vendor device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Hardware Vendor device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextEmuFs (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16                       *Str;
  EMU_VENDOR_DEVICE_PATH_NODE  *Vendor;

  Str = GetNextParamStr (&TextDeviceNode);
  Vendor    = (EMU_VENDOR_DEVICE_PATH_NODE *) CreateDeviceNode (
                                                   HARDWARE_DEVICE_PATH,
                                                   HW_VENDOR_DP,
                                                   (UINT16) sizeof (EMU_VENDOR_DEVICE_PATH_NODE)
                                                   );
  CopyGuid (&Vendor->VendorDevicePath.Guid, &gEfiSimpleFileSystemProtocolGuid);
  Vendor->Instance = (UINT32) StrDecimalToUintn (Str);

  return (EFI_DEVICE_PATH_PROTOCOL *) Vendor;
}

/**
  Register the Filter function

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor executed correctly.

**/
EFI_STATUS
EFIAPI
DevicePathToTextLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )

{
  DevPathToTextSetVendorDevicePathFilter (DevPathToTextVendorLib);
  DevicePathFromTextAddFilter (L"EmuThunk", DevPathFromTextEmuThunk);
  DevicePathFromTextAddFilter (L"EmuThread", DevPathFromTextEmuThread);
  DevicePathFromTextAddFilter (L"EmuFs", DevPathFromTextEmuFs);
  return EFI_SUCCESS;
}
