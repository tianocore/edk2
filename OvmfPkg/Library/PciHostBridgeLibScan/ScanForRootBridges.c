/** @file
  Scan the entire PCI bus for root bridges to support OVMF on Bhyve and Xen.

  Copyright (C) 2021, Red Hat, Inc.
  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Pci.h>            // EFI_PCI_COMMAND_IO_SPACE
#include <Library/BaseLib.h>                 // DisableInterrupts()
#include <Library/BaseMemoryLib.h>           // ZeroMem()
#include <Library/DebugLib.h>                // ASSERT()
#include <Library/MemoryAllocationLib.h>     // ReallocatePool()
#include <Library/PciHostBridgeLib.h>        // PCI_ROOT_BRIDGE_APERTURE
#include <Library/PciHostBridgeUtilityLib.h> // PciHostBridgeUtilityInitRoot...
#include <Library/PciLib.h>                  // PciRead32()
#include <Protocol/PciRootBridgeIo.h>        // EFI_PCI_ATTRIBUTE_ISA_IO

#include "PciHostBridge.h"

STATIC
VOID
PcatPciRootBridgeBarExisted (
  IN  UINTN   Address,
  OUT UINT32  *OriginalValue,
  OUT UINT32  *Value
  )
{
  //
  // Preserve the original value
  //
  *OriginalValue = PciRead32 (Address);

  //
  // Disable timer interrupt while the BAR is probed
  //
  DisableInterrupts ();

  PciWrite32 (Address, 0xFFFFFFFF);
  *Value = PciRead32 (Address);
  PciWrite32 (Address, *OriginalValue);

  //
  // Enable interrupt
  //
  EnableInterrupts ();
}

#define PCI_COMMAND_DECODE  ((UINT16)(EFI_PCI_COMMAND_IO_SPACE |\
                                     EFI_PCI_COMMAND_MEMORY_SPACE))
STATIC
VOID
PcatPciRootBridgeDecodingDisable (
  IN  UINTN  Address
  )
{
  UINT16  Value;

  Value = PciRead16 (Address);
  if (Value & PCI_COMMAND_DECODE) {
    PciWrite16 (Address, Value & ~(UINT32)PCI_COMMAND_DECODE);
  }
}

STATIC
VOID
PcatPciRootBridgeParseBars (
  IN UINT16                    Command,
  IN UINTN                     Bus,
  IN UINTN                     Device,
  IN UINTN                     Function,
  IN UINTN                     BarOffsetBase,
  IN UINTN                     BarOffsetEnd,
  IN PCI_ROOT_BRIDGE_APERTURE  *Io,
  IN PCI_ROOT_BRIDGE_APERTURE  *Mem,
  IN PCI_ROOT_BRIDGE_APERTURE  *MemAbove4G

  )
{
  UINT32                    OriginalValue;
  UINT32                    Value;
  UINT32                    OriginalUpperValue;
  UINT32                    UpperValue;
  UINT64                    Mask;
  UINTN                     Offset;
  UINT64                    Base;
  UINT64                    Length;
  UINT64                    Limit;
  PCI_ROOT_BRIDGE_APERTURE  *MemAperture;

  // Disable address decoding for every device before OVMF starts sizing it
  PcatPciRootBridgeDecodingDisable (
    PCI_LIB_ADDRESS (Bus, Device, Function, PCI_COMMAND_OFFSET)
    );

  for (Offset = BarOffsetBase; Offset < BarOffsetEnd; Offset += sizeof (UINT32)) {
    PcatPciRootBridgeBarExisted (
      PCI_LIB_ADDRESS (Bus, Device, Function, Offset),
      &OriginalValue,
      &Value
      );
    if (Value == 0) {
      continue;
    }

    if ((Value & BIT0) == BIT0) {
      //
      // IO Bar
      //
      if (Command & EFI_PCI_COMMAND_IO_SPACE) {
        Mask   = 0xfffffffc;
        Base   = OriginalValue & Mask;
        Length = ((~(Value & Mask)) & Mask) + 0x04;
        if (!(Value & 0xFFFF0000)) {
          Length &= 0x0000FFFF;
        }

        Limit = Base + Length - 1;

        if (Base < Limit) {
          if (Io->Base > Base) {
            Io->Base = Base;
          }

          if (Io->Limit < Limit) {
            Io->Limit = Limit;
          }
        }
      }
    } else {
      //
      // Mem Bar
      //
      if (Command & EFI_PCI_COMMAND_MEMORY_SPACE) {
        Mask   = 0xfffffff0;
        Base   = OriginalValue & Mask;
        Length = Value & Mask;

        if ((Value & (BIT1 | BIT2)) == 0) {
          //
          // 32bit
          //
          Length = ((~Length) + 1) & 0xffffffff;

          MemAperture = Mem;
        } else {
          //
          // 64bit
          //
          Offset += 4;
          PcatPciRootBridgeBarExisted (
            PCI_LIB_ADDRESS (Bus, Device, Function, Offset),
            &OriginalUpperValue,
            &UpperValue
            );

          Base   = Base | LShiftU64 ((UINT64)OriginalUpperValue, 32);
          Length = Length | LShiftU64 ((UINT64)UpperValue, 32);
          Length = (~Length) + 1;

          if (Base < BASE_4GB) {
            MemAperture = Mem;
          } else {
            MemAperture = MemAbove4G;
          }
        }

        Limit = Base + Length - 1;
        if (Base < Limit) {
          if (MemAperture->Base > Base) {
            MemAperture->Base = Base;
          }

          if (MemAperture->Limit < Limit) {
            MemAperture->Limit = Limit;
          }
        }
      }
    }
  }
}

STATIC PCI_ROOT_BRIDGE_APERTURE  mNonExistAperture = { MAX_UINT64, 0 };

PCI_ROOT_BRIDGE *
ScanForRootBridges (
  UINTN  *NumberOfRootBridges
  )
{
  UINTN                     PrimaryBus;
  UINTN                     SubBus;
  UINT8                     Device;
  UINT8                     Function;
  UINTN                     NumberOfDevices;
  UINTN                     Address;
  PCI_TYPE01                Pci;
  UINT64                    Attributes;
  UINT64                    Base;
  UINT64                    Limit;
  UINT64                    Value;
  PCI_ROOT_BRIDGE_APERTURE  Io, Mem, MemAbove4G, *MemAperture;
  PCI_ROOT_BRIDGE           *RootBridges;
  UINTN                     BarOffsetEnd;

  *NumberOfRootBridges = 0;
  RootBridges          = NULL;

  //
  // After scanning all the PCI devices on the PCI root bridge's primary bus,
  // update the Primary Bus Number for the next PCI root bridge to be this PCI
  // root bridge's subordinate bus number + 1.
  //
  for (PrimaryBus = 0; PrimaryBus <= PCI_MAX_BUS; PrimaryBus = SubBus + 1) {
    SubBus     = PrimaryBus;
    Attributes = 0;

    ZeroMem (&Io, sizeof (Io));
    ZeroMem (&Mem, sizeof (Mem));
    ZeroMem (&MemAbove4G, sizeof (MemAbove4G));
    Io.Base = Mem.Base = MemAbove4G.Base = MAX_UINT64;
    //
    // Scan all the PCI devices on the primary bus of the PCI root bridge
    //
    for (Device = 0, NumberOfDevices = 0; Device <= PCI_MAX_DEVICE; Device++) {
      for (Function = 0; Function <= PCI_MAX_FUNC; Function++) {
        //
        // Compute the PCI configuration address of the PCI device to probe
        //
        Address = PCI_LIB_ADDRESS (PrimaryBus, Device, Function, 0);

        //
        // Read the Vendor ID from the PCI Configuration Header
        //
        if (PciRead16 (Address) == MAX_UINT16) {
          if (Function == 0) {
            //
            // If the PCI Configuration Read fails, or a PCI device does not
            // exist, then skip this entire PCI device
            //
            break;
          } else {
            //
            // If PCI function != 0, VendorId == 0xFFFF, we continue to search
            // PCI function.
            //
            continue;
          }
        }

        //
        // Read the entire PCI Configuration Header
        //
        PciReadBuffer (Address, sizeof (Pci), &Pci);

        //
        // Increment the number of PCI device found on the primary bus of the
        // PCI root bridge
        //
        NumberOfDevices++;

        //
        // Look for devices with the VGA Palette Snoop enabled in the COMMAND
        // register of the PCI Config Header
        //
        if ((Pci.Hdr.Command & EFI_PCI_COMMAND_VGA_PALETTE_SNOOP) != 0) {
          Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO;
          Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16;
        }

        BarOffsetEnd = 0;

        //
        // PCI-PCI Bridge
        //
        if (IS_PCI_BRIDGE (&Pci)) {
          //
          // Get the Bus range that the PPB is decoding
          //
          if (Pci.Bridge.SubordinateBus > SubBus) {
            //
            // If the subordinate bus number of the PCI-PCI bridge is greater
            // than the PCI root bridge's current subordinate bus number,
            // then update the PCI root bridge's subordinate bus number
            //
            SubBus = Pci.Bridge.SubordinateBus;
          }

          //
          // Get the I/O range that the PPB is decoding
          //
          Value = Pci.Bridge.IoBase & 0x0f;
          Base  = ((UINT32)Pci.Bridge.IoBase & 0xf0) << 8;
          Limit = (((UINT32)Pci.Bridge.IoLimit & 0xf0) << 8) | 0x0fff;
          if (Value == BIT0) {
            Base  |= ((UINT32)Pci.Bridge.IoBaseUpper16 << 16);
            Limit |= ((UINT32)Pci.Bridge.IoLimitUpper16 << 16);
          }

          if (Base < Limit) {
            if (Io.Base > Base) {
              Io.Base = Base;
            }

            if (Io.Limit < Limit) {
              Io.Limit = Limit;
            }
          }

          //
          // Get the Memory range that the PPB is decoding
          //
          Base  = ((UINT32)Pci.Bridge.MemoryBase & 0xfff0) << 16;
          Limit = (((UINT32)Pci.Bridge.MemoryLimit & 0xfff0) << 16) | 0xfffff;
          if (Base < Limit) {
            if (Mem.Base > Base) {
              Mem.Base = Base;
            }

            if (Mem.Limit < Limit) {
              Mem.Limit = Limit;
            }
          }

          //
          // Get the Prefetchable Memory range that the PPB is decoding
          // and merge it into Memory range
          //
          Value = Pci.Bridge.PrefetchableMemoryBase & 0x0f;
          Base  = ((UINT32)Pci.Bridge.PrefetchableMemoryBase & 0xfff0) << 16;
          Limit = (((UINT32)Pci.Bridge.PrefetchableMemoryLimit & 0xfff0)
                   << 16) | 0xfffff;
          MemAperture = &Mem;
          if (Value == BIT0) {
            Base       |= LShiftU64 (Pci.Bridge.PrefetchableBaseUpper32, 32);
            Limit      |= LShiftU64 (Pci.Bridge.PrefetchableLimitUpper32, 32);
            MemAperture = &MemAbove4G;
          }

          if (Base < Limit) {
            if (MemAperture->Base > Base) {
              MemAperture->Base = Base;
            }

            if (MemAperture->Limit < Limit) {
              MemAperture->Limit = Limit;
            }
          }

          //
          // Look at the PPB Configuration for legacy decoding attributes
          //
          if ((Pci.Bridge.BridgeControl & EFI_PCI_BRIDGE_CONTROL_ISA)
              == EFI_PCI_BRIDGE_CONTROL_ISA)
          {
            Attributes |= EFI_PCI_ATTRIBUTE_ISA_IO;
            Attributes |= EFI_PCI_ATTRIBUTE_ISA_IO_16;
            Attributes |= EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO;
          }

          if ((Pci.Bridge.BridgeControl & EFI_PCI_BRIDGE_CONTROL_VGA)
              == EFI_PCI_BRIDGE_CONTROL_VGA)
          {
            Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO;
            Attributes |= EFI_PCI_ATTRIBUTE_VGA_MEMORY;
            Attributes |= EFI_PCI_ATTRIBUTE_VGA_IO;
            if ((Pci.Bridge.BridgeControl & EFI_PCI_BRIDGE_CONTROL_VGA_16)
                != 0)
            {
              Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16;
              Attributes |= EFI_PCI_ATTRIBUTE_VGA_IO_16;
            }
          }

          BarOffsetEnd = OFFSET_OF (PCI_TYPE01, Bridge.Bar[2]);
        } else {
          //
          // Parse the BARs of the PCI device to get what I/O Ranges, Memory
          // Ranges, and Prefetchable Memory Ranges the device is decoding
          //
          if ((Pci.Hdr.HeaderType & HEADER_LAYOUT_CODE) == HEADER_TYPE_DEVICE) {
            BarOffsetEnd = OFFSET_OF (PCI_TYPE00, Device.Bar[6]);
          }
        }

        PcatPciRootBridgeParseBars (
          Pci.Hdr.Command,
          PrimaryBus,
          Device,
          Function,
          OFFSET_OF (PCI_TYPE00, Device.Bar),
          BarOffsetEnd,
          &Io,
          &Mem,
          &MemAbove4G
          );

        //
        // See if the PCI device is an IDE controller
        //
        if (IS_CLASS2 (
              &Pci,
              PCI_CLASS_MASS_STORAGE,
              PCI_CLASS_MASS_STORAGE_IDE
              ))
        {
          if (Pci.Hdr.ClassCode[0] & 0x80) {
            Attributes |= EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO;
            Attributes |= EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO;
          }

          if (Pci.Hdr.ClassCode[0] & 0x01) {
            Attributes |= EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO;
          }

          if (Pci.Hdr.ClassCode[0] & 0x04) {
            Attributes |= EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO;
          }
        }

        //
        // See if the PCI device is a legacy VGA controller or
        // a standard VGA controller
        //
        if (IS_CLASS2 (&Pci, PCI_CLASS_OLD, PCI_CLASS_OLD_VGA) ||
            IS_CLASS2 (&Pci, PCI_CLASS_DISPLAY, PCI_CLASS_DISPLAY_VGA)
            )
        {
          Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO;
          Attributes |= EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16;
          Attributes |= EFI_PCI_ATTRIBUTE_VGA_MEMORY;
          Attributes |= EFI_PCI_ATTRIBUTE_VGA_IO;
          Attributes |= EFI_PCI_ATTRIBUTE_VGA_IO_16;
        }

        //
        // See if the PCI Device is a PCI - ISA or PCI - EISA
        // or ISA_POSITIVE_DECODE Bridge device
        //
        if (Pci.Hdr.ClassCode[2] == PCI_CLASS_BRIDGE) {
          if ((Pci.Hdr.ClassCode[1] == PCI_CLASS_BRIDGE_ISA) ||
              (Pci.Hdr.ClassCode[1] == PCI_CLASS_BRIDGE_EISA) ||
              (Pci.Hdr.ClassCode[1] == PCI_CLASS_BRIDGE_ISA_PDECODE))
          {
            Attributes |= EFI_PCI_ATTRIBUTE_ISA_IO;
            Attributes |= EFI_PCI_ATTRIBUTE_ISA_IO_16;
            Attributes |= EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO;
          }
        }

        //
        // If this device is not a multi function device, then skip the rest
        // of this PCI device
        //
        if ((Function == 0) && !IS_PCI_MULTI_FUNC (&Pci)) {
          break;
        }
      }
    }

    //
    // If at least one PCI device was found on the primary bus of this PCI
    // root bridge, then the PCI root bridge exists.
    //
    if (NumberOfDevices > 0) {
      RootBridges = ReallocatePool (
                      (*NumberOfRootBridges) * sizeof (PCI_ROOT_BRIDGE),
                      (*NumberOfRootBridges + 1) * sizeof (PCI_ROOT_BRIDGE),
                      RootBridges
                      );
      ASSERT (RootBridges != NULL);
      PciHostBridgeUtilityInitRootBridge (
        Attributes,
        Attributes,
        0,
        FALSE,
        TRUE /* NoExtendedConfigSpace */,
        (UINT8)PrimaryBus,
        (UINT8)SubBus,
        &Io,
        &Mem,
        &MemAbove4G,
        &mNonExistAperture,
        &mNonExistAperture,
        &RootBridges[*NumberOfRootBridges]
        );
      RootBridges[*NumberOfRootBridges].ResourceAssigned = TRUE;
      //
      // Increment the index for the next PCI Root Bridge
      //
      (*NumberOfRootBridges)++;
    }
  }

  return RootBridges;
}
