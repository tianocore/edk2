/** @file
  Set up ROM Table for PCI Bus module.

Copyright (c) 2006 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PciBus.h"

//
// PCI ROM image information
//
typedef struct {
  EFI_HANDLE    ImageHandle;
  UINTN         Seg;
  UINT8         Bus;
  UINT8         Dev;
  UINT8         Func;
  VOID          *RomImage;
  UINT64        RomSize;
} PCI_ROM_IMAGE;

UINTN          mNumberOfPciRomImages    = 0;
UINTN          mMaxNumberOfPciRomImages = 0;
PCI_ROM_IMAGE  *mRomImageTable          = NULL;

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
  UINTN          Index;
  PCI_ROM_IMAGE  *NewTable;

  for (Index = 0; Index < mNumberOfPciRomImages; Index++) {
    if ((mRomImageTable[Index].Seg  == Seg) &&
        (mRomImageTable[Index].Bus  == Bus) &&
        (mRomImageTable[Index].Dev  == Dev) &&
        (mRomImageTable[Index].Func == Func))
    {
      //
      // Expect once RomImage and RomSize are recorded, they will be passed in
      // later when updating ImageHandle. They may also be updated with new
      // values if the platform provides an override of RomImage and RomSize.
      //
      break;
    }
  }

  if (Index == mNumberOfPciRomImages) {
    //
    // Rom Image Table buffer needs to grow.
    //
    if (mNumberOfPciRomImages == mMaxNumberOfPciRomImages) {
      NewTable = ReallocatePool (
                   mMaxNumberOfPciRomImages * sizeof (PCI_ROM_IMAGE),
                   (mMaxNumberOfPciRomImages + 0x20) * sizeof (PCI_ROM_IMAGE),
                   mRomImageTable
                   );
      if (NewTable == NULL) {
        return;
      }

      mRomImageTable            = NewTable;
      mMaxNumberOfPciRomImages += 0x20;
    }

    //
    // Record the new PCI device
    //
    mRomImageTable[Index].Seg  = Seg;
    mRomImageTable[Index].Bus  = Bus;
    mRomImageTable[Index].Dev  = Dev;
    mRomImageTable[Index].Func = Func;
    mNumberOfPciRomImages++;
  }

  mRomImageTable[Index].ImageHandle = ImageHandle;
  mRomImageTable[Index].RomImage    = RomImage;
  mRomImageTable[Index].RomSize     = RomSize;
}

/**
  Get Option rom driver's mapping for PCI device.

  @param PciIoDevice Device instance.

  @retval TRUE   Found Image mapping.
  @retval FALSE  Cannot found image mapping.

**/
BOOLEAN
PciRomGetImageMapping (
  IN  PCI_IO_DEVICE  *PciIoDevice
  )
{
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo;
  UINTN                            Index;

  PciRootBridgeIo = PciIoDevice->PciRootBridgeIo;

  for (Index = 0; Index < mNumberOfPciRomImages; Index++) {
    if ((mRomImageTable[Index].Seg  == PciRootBridgeIo->SegmentNumber) &&
        (mRomImageTable[Index].Bus  == PciIoDevice->BusNumber) &&
        (mRomImageTable[Index].Dev  == PciIoDevice->DeviceNumber) &&
        (mRomImageTable[Index].Func == PciIoDevice->FunctionNumber))
    {
      if (mRomImageTable[Index].ImageHandle != NULL) {
        AddDriver (PciIoDevice, mRomImageTable[Index].ImageHandle, NULL);
      }

      PciIoDevice->PciIo.RomImage = mRomImageTable[Index].RomImage;
      PciIoDevice->PciIo.RomSize  = mRomImageTable[Index].RomSize;
      return TRUE;
    }
  }

  return FALSE;
}
