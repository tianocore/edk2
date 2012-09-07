/** @file
  PCI Rom supporting funtions declaration for PCI Bus module.

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_PCI_OPTION_ROM_SUPPORT_H_
#define _EFI_PCI_OPTION_ROM_SUPPORT_H_


/**
  Initialize a PCI LoadFile2 instance.

  @param PciIoDevice   PCI IO Device.

**/
VOID
InitializePciLoadFile2 (
  IN PCI_IO_DEVICE       *PciIoDevice
  );

/**
  Causes the driver to load a specified file.

  @param This        Indicates a pointer to the calling context.
  @param FilePath    The device specific path of the file to load.
  @param BootPolicy  Should always be FALSE.
  @param BufferSize  On input the size of Buffer in bytes. On output with a return
                     code of EFI_SUCCESS, the amount of data transferred to Buffer.
                     On output with a return code of EFI_BUFFER_TOO_SMALL,
                     the size of Buffer required to retrieve the requested file.
  @param Buffer      The memory buffer to transfer the file to. If Buffer is NULL,
                     then no the size of the requested file is returned in BufferSize.

  @retval EFI_SUCCESS           The file was loaded.
  @retval EFI_UNSUPPORTED       BootPolicy is TRUE.
  @retval EFI_INVALID_PARAMETER FilePath is not a valid device path, or
                                BufferSize is NULL.
  @retval EFI_NOT_FOUND         Not found PCI Option Rom on PCI device.
  @retval EFI_DEVICE_ERROR      Failed to decompress PCI Option Rom image.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize is too small to read the current directory entry.
                                BufferSize has been updated with the size needed to complete the request.

**/
EFI_STATUS
EFIAPI
LoadFile2 (
  IN EFI_LOAD_FILE2_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL *FilePath,
  IN BOOLEAN                  BootPolicy,
  IN OUT UINTN                *BufferSize,
  IN VOID                     *Buffer      OPTIONAL
  );

/**
  Get Pci device's oprom information.

  @param PciIoDevice    Input Pci device instance.
                        Output Pci device instance with updated OptionRom size.

  @retval EFI_NOT_FOUND Pci device has not Option Rom.
  @retval EFI_SUCCESS   Pci device has Option Rom.

**/
EFI_STATUS
GetOpRomInfo (
  IN OUT PCI_IO_DEVICE    *PciIoDevice
  );

/**
  Load Option Rom image for specified PCI device.

  @param PciDevice Pci device instance.
  @param RomBase   Base address of Option Rom.

  @retval EFI_OUT_OF_RESOURCES No enough memory to hold image.
  @retval EFI_SUCESS           Successfully loaded Option Rom.

**/
EFI_STATUS
LoadOpRomImage (
  IN PCI_IO_DEVICE   *PciDevice,
  IN UINT64          RomBase
  );

/**
  Enable/Disable Option Rom decode.

  @param PciDevice    Pci device instance.
  @param RomBarIndex  The BAR index of the standard PCI Configuration header to use as the
                      base address for resource range. The legal range for this field is 0..5.
  @param RomBar       Base address of Option Rom.
  @param Enable       Flag for enable/disable decode.

**/
VOID
RomDecode (
  IN PCI_IO_DEVICE   *PciDevice,
  IN UINT8           RomBarIndex,
  IN UINT32          RomBar,
  IN BOOLEAN         Enable
  );

/**
  Load and start the Option Rom image.

  @param PciDevice       Pci device instance.

  @retval EFI_SUCCESS    Successfully loaded and started PCI Option Rom image.
  @retval EFI_NOT_FOUND  Failed to process PCI Option Rom image.

**/
EFI_STATUS
ProcessOpRomImage (
  IN PCI_IO_DEVICE   *PciDevice
  );

#endif
