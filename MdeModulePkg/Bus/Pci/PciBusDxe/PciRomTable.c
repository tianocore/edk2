/** @file
  Set up ROM Table for PCI Bus module.

Copyright (c) 2006 - 2017, Intel Corporation. All rights reserved.<BR>
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
  VOID        *RomImage;
  UINT64      RomSize;
} PCI_ROM_IMAGE;

UINTN          mNumberOfPciRomImages     = 0;
UINTN          mMaxNumberOfPciRomImages  = 0;
PCI_ROM_IMAGE  *mRomImageTable           = NULL;

/**
  Add the Rom Image to internal database for later PCI light enumeration.

  @param ImageHandle    Option Rom image handle.
  @param Seg            Segment of PCI space.
  @param Bus            Bus NO of PCI space.
  @param Dev            Dev NO of PCI space.
  @param Func           Func NO of PCI space.
  @param RomImage       Option Rom buffer.
  @param RomSize        Size of Option Rom buffer.
**/
VOID
PciRomAddImageMapping (
  IN  EFI_HANDLE  ImageHandle,
  IN  UINTN       Seg,
  IN  UINT8       Bus,
  IN  UINT8       Dev,
  IN  UINT8       Func,
  IN  VOID        *RomImage,
  IN  UINT64      RomSize
  )
{
  PCI_ROM_IMAGE   *NewTable;

  if (mNumberOfPciRomImages == mMaxNumberOfPciRomImages) {

    NewTable = ReallocatePool (
                 mMaxNumberOfPciRomImages * sizeof (PCI_ROM_IMAGE),
                 (mMaxNumberOfPciRomImages + 0x20) * sizeof (PCI_ROM_IMAGE),
                 mRomImageTable
                 );
    if (NewTable == NULL) {
      return ;
    }

    mRomImageTable            = NewTable;
    mMaxNumberOfPciRomImages += 0x20;
  }

  mRomImageTable[mNumberOfPciRomImages].ImageHandle = ImageHandle;
  mRomImageTable[mNumberOfPciRomImages].Seg         = Seg;
  mRomImageTable[mNumberOfPciRomImages].Bus         = Bus;
  mRomImageTable[mNumberOfPciRomImages].Dev         = Dev;
  mRomImageTable[mNumberOfPciRomImages].Func        = Func;
  mRomImageTable[mNumberOfPciRomImages].RomImage    = RomImage;
  mRomImageTable[mNumberOfPciRomImages].RomSize     = RomSize;
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
        PciIoDevice->PciIo.RomImage = mRomImageTable[Index].RomImage;
        PciIoDevice->PciIo.RomSize  = mRomImageTable[Index].RomSize;
      }
    }
  }

  return Found;
}
