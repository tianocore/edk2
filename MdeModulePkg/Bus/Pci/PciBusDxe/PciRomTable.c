/** @file
  Set up ROM Table for PCI Bus module.

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PciBus.h"

//
// PCI ROM image information
//
typedef struct {
  EFI_HANDLE  ImageHandle;
  UINTN       Seg;
  UINT8       Bus;
  UINT8       Dev;
  UINT8       Func;
  UINT64      RomAddress;
  UINT64      RomLength;
} EFI_PCI_ROM_IMAGE_MAPPING;

UINTN                      mNumberOfPciRomImages     = 0;
UINTN                      mMaxNumberOfPciRomImages  = 0;
EFI_PCI_ROM_IMAGE_MAPPING  *mRomImageTable           = NULL;

/**
  Add the Rom Image to internal database for later PCI light enumeration.

  @param ImageHandle    Option Rom image handle.
  @param Seg            Segment of PCI space.
  @param Bus            Bus NO of PCI space.
  @param Dev            Dev NO of PCI space.
  @param Func           Func NO of PCI space.
  @param RomAddress     Base address of OptionRom.
  @param RomLength      Length of rom image.

**/
VOID
PciRomAddImageMapping (
  IN  EFI_HANDLE  ImageHandle,
  IN  UINTN       Seg,
  IN  UINT8       Bus,
  IN  UINT8       Dev,
  IN  UINT8       Func,
  IN  UINT64      RomAddress,
  IN  UINT64      RomLength
  )
{
  EFI_PCI_ROM_IMAGE_MAPPING *TempMapping;

  if (mNumberOfPciRomImages >= mMaxNumberOfPciRomImages) {

    mMaxNumberOfPciRomImages += 0x20;

    TempMapping = NULL;
    TempMapping = AllocatePool (mMaxNumberOfPciRomImages * sizeof (EFI_PCI_ROM_IMAGE_MAPPING));
    if (TempMapping == NULL) {
      return ;
    }

    CopyMem (TempMapping, mRomImageTable, mNumberOfPciRomImages * sizeof (EFI_PCI_ROM_IMAGE_MAPPING));

    if (mRomImageTable != NULL) {
      FreePool (mRomImageTable);
    }

    mRomImageTable = TempMapping;
  }

  mRomImageTable[mNumberOfPciRomImages].ImageHandle = ImageHandle;
  mRomImageTable[mNumberOfPciRomImages].Seg         = Seg;
  mRomImageTable[mNumberOfPciRomImages].Bus         = Bus;
  mRomImageTable[mNumberOfPciRomImages].Dev         = Dev;
  mRomImageTable[mNumberOfPciRomImages].Func        = Func;
  mRomImageTable[mNumberOfPciRomImages].RomAddress  = RomAddress;
  mRomImageTable[mNumberOfPciRomImages].RomLength   = RomLength;
  mNumberOfPciRomImages++;
}

/**
  Get Option rom driver's mapping for PCI device.

  @param PciIoDevice Device instance.

  @retval TRUE   Found Image mapping.
  @retval FALSE  Cannot found image mapping.

**/
BOOLEAN
PciRomGetImageMapping (
  IN  PCI_IO_DEVICE                       *PciIoDevice
  )
{
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;
  UINTN                           Index;
  BOOLEAN                         Found;

  PciRootBridgeIo = PciIoDevice->PciRootBridgeIo;
  Found           = FALSE;

  for (Index = 0; Index < mNumberOfPciRomImages; Index++) {
    if (mRomImageTable[Index].Seg  == PciRootBridgeIo->SegmentNumber &&
        mRomImageTable[Index].Bus  == PciIoDevice->BusNumber         &&
        mRomImageTable[Index].Dev  == PciIoDevice->DeviceNumber      &&
        mRomImageTable[Index].Func == PciIoDevice->FunctionNumber    ) {
        Found = TRUE;

      if (mRomImageTable[Index].ImageHandle != NULL) {
        AddDriver (PciIoDevice, mRomImageTable[Index].ImageHandle);
      } else {
        PciIoDevice->PciIo.RomImage = (VOID *) (UINTN) mRomImageTable[Index].RomAddress;
        PciIoDevice->PciIo.RomSize  = (UINTN) mRomImageTable[Index].RomLength;
      }
    }
  }

  return Found;
}
