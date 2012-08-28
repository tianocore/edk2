/*++

Copyright (c) 2005 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
    PcatPciRootBridgeIo.c
    
Abstract:

    EFI PC AT PCI Root Bridge Io Protocol

Revision History

--*/

#include "PcatPciRootBridge.h"

BOOLEAN                  mPciOptionRomTableInstalled = FALSE;
EFI_PCI_OPTION_ROM_TABLE mPciOptionRomTable          = {0, NULL};

EFI_STATUS
EFIAPI
PcatRootBridgeIoIoRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 UserAddress,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *UserBuffer
  )
{
  return gCpuIo->Io.Read (
                      gCpuIo,
                      (EFI_CPU_IO_PROTOCOL_WIDTH) Width,
                      UserAddress,
                      Count,
                      UserBuffer
                      );
}

EFI_STATUS
EFIAPI
PcatRootBridgeIoIoWrite (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                                 UserAddress,
  IN UINTN                                  Count,
  IN OUT VOID                               *UserBuffer
  )
{
  return gCpuIo->Io.Write (
                      gCpuIo,
                      (EFI_CPU_IO_PROTOCOL_WIDTH) Width,
                      UserAddress,
                      Count,
                      UserBuffer
                      );

}

EFI_STATUS
PcatRootBridgeIoGetIoPortMapping (
  OUT EFI_PHYSICAL_ADDRESS  *IoPortMapping,
  OUT EFI_PHYSICAL_ADDRESS  *MemoryPortMapping
  )
/*++

  Get the IO Port Mapping.  For IA-32 it is always 0.
  
--*/
{
  *IoPortMapping = 0;
  *MemoryPortMapping = 0;

  return EFI_SUCCESS;
}

EFI_STATUS
PcatRootBridgeIoPciRW (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN BOOLEAN                                Write,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                                 UserAddress,
  IN UINTN                                  Count,
  IN OUT VOID                               *UserBuffer
  )
{
  PCI_CONFIG_ACCESS_CF8             Pci;
  PCI_CONFIG_ACCESS_CF8             PciAligned;
  UINT32                            InStride;
  UINT32                            OutStride;
  UINTN                             PciData;
  UINTN                             PciDataStride;
  PCAT_PCI_ROOT_BRIDGE_INSTANCE     *PrivateData;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS  PciAddress;
  UINT64                            PciExpressRegAddr;
  BOOLEAN                           UsePciExpressAccess;

  if ((UINT32)Width >= EfiPciWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }
  
  if ((Width & 0x03) >= EfiPciWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }
  
  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(This);

  InStride    = 1 << (Width & 0x03);
  OutStride   = InStride;
  if (Width >= EfiPciWidthFifoUint8 && Width <= EfiPciWidthFifoUint64) {
    InStride = 0;
  }

  if (Width >= EfiPciWidthFillUint8 && Width <= EfiPciWidthFillUint64) {
    OutStride = 0;
  }

  UsePciExpressAccess = FALSE;

  CopyMem (&PciAddress, &UserAddress, sizeof(UINT64));

  if (PciAddress.ExtendedRegister > 0xFF) {
    //
    // Check PciExpressBaseAddress
    //
    if ((PrivateData->PciExpressBaseAddress == 0) ||
        (PrivateData->PciExpressBaseAddress >= MAX_ADDRESS)) {
      return EFI_UNSUPPORTED;
    } else {
      UsePciExpressAccess = TRUE;
    }
  } else {
    if (PciAddress.ExtendedRegister != 0) {
      Pci.Bits.Reg = PciAddress.ExtendedRegister & 0xFF;
    } else {
      Pci.Bits.Reg = PciAddress.Register;
    }
    //
    // Note: We can also use PciExpress access here, if wanted.
    //
  }

  if (!UsePciExpressAccess) {
    Pci.Bits.Func     = PciAddress.Function;
    Pci.Bits.Dev      = PciAddress.Device;
    Pci.Bits.Bus      = PciAddress.Bus;
    Pci.Bits.Reserved = 0;
    Pci.Bits.Enable   = 1;

    //
    // PCI Config access are all 32-bit alligned, but by accessing the
    //  CONFIG_DATA_REGISTER (0xcfc) with different widths more cycle types
    //  are possible on PCI.
    //
    // To read a byte of PCI config space you load 0xcf8 and 
    //  read 0xcfc, 0xcfd, 0xcfe, 0xcff
    //
    PciDataStride = Pci.Bits.Reg & 0x03;

    while (Count) {
      PciAligned = Pci;
      PciAligned.Bits.Reg &= 0xfc;
      PciData = (UINTN)PrivateData->PciData + PciDataStride;
      EfiAcquireLock(&PrivateData->PciLock);
      This->Io.Write (This, EfiPciWidthUint32, PrivateData->PciAddress, 1, &PciAligned);
      if (Write) {
        This->Io.Write (This, Width, PciData, 1, UserBuffer);
      } else {
        This->Io.Read (This, Width, PciData, 1, UserBuffer);
      }
      EfiReleaseLock(&PrivateData->PciLock);
      UserBuffer = ((UINT8 *)UserBuffer) + OutStride;
      PciDataStride = (PciDataStride + InStride) % 4;
      Pci.Bits.Reg += InStride;
      Count -= 1;
    }
  } else {
    //
    // Access PCI-Express space by using memory mapped method.
    //
    PciExpressRegAddr = (PrivateData->PciExpressBaseAddress) |
                        (PciAddress.Bus      << 20) |
                        (PciAddress.Device   << 15) |
                        (PciAddress.Function << 12);
    if (PciAddress.ExtendedRegister != 0) {
      PciExpressRegAddr += PciAddress.ExtendedRegister;
    } else {
      PciExpressRegAddr += PciAddress.Register;
    }
    while (Count) {
      if (Write) {
        This->Mem.Write (This, Width, (UINTN) PciExpressRegAddr, 1, UserBuffer);
      } else {
        This->Mem.Read (This, Width, (UINTN) PciExpressRegAddr, 1, UserBuffer);
      }

      UserBuffer = ((UINT8 *) UserBuffer) + OutStride;
      PciExpressRegAddr += InStride;
      Count -= 1;
    }
  }
  
  return EFI_SUCCESS;
}

VOID
ScanPciBus(
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *IoDev,
  UINT16                           MinBus,
  UINT16                           MaxBus,
  UINT16                           MinDevice,
  UINT16                           MaxDevice,
  UINT16                           MinFunc,
  UINT16                           MaxFunc,
  EFI_PCI_BUS_SCAN_CALLBACK        Callback,
  VOID                             *Context
  )
  
{
  UINT16      Bus;
  UINT16      Device;
  UINT16      Func;
  UINT64      Address;
  PCI_TYPE00  PciHeader;

  //
  // Loop through all busses
  //
  for (Bus = MinBus; Bus <= MaxBus; Bus++) {
    //  
    // Loop 32 devices per bus
    //
    for (Device = MinDevice; Device <= MaxDevice; Device++) {
      //
      // Loop through 8 functions per device
      //
      for (Func = MinFunc; Func <= MaxFunc; Func++) {

        //
        // Compute the EFI Address required to access the PCI Configuration Header of this PCI Device
        //
        Address = EFI_PCI_ADDRESS (Bus, Device, Func, 0);

        //
        // Read the VendorID from this PCI Device's Confioguration Header
        //
        IoDev->Pci.Read (IoDev, EfiPciWidthUint16, Address, 1, &PciHeader.Hdr.VendorId);
    
        //
        // If VendorId = 0xffff, there does not exist a device at this 
        // location. For each device, if there is any function on it, 
        // there must be 1 function at Function 0. So if Func = 0, there
        // will be no more functions in the same device, so we can break
        // loop to deal with the next device.
        //  
        if (PciHeader.Hdr.VendorId == 0xffff && Func == 0) {
          break;
        }
        
        if (PciHeader.Hdr.VendorId != 0xffff) {

          //
          // Read the HeaderType to determine if this is a multi-function device
          //
          IoDev->Pci.Read (IoDev, EfiPciWidthUint8, Address + 0x0e, 1, &PciHeader.Hdr.HeaderType);

          //
          // Call the callback function for the device that was found
          //
          Callback(
            IoDev, 
            MinBus, MaxBus,
            MinDevice, MaxDevice,
            MinFunc, MaxFunc,
            Bus,
            Device,
            Func,
            Context
            );

          //
          // If this is not a multi-function device, we can leave the loop 
          // to deal with the next device.
          //
          if ((PciHeader.Hdr.HeaderType & HEADER_TYPE_MULTI_FUNCTION) == 0x00 && Func == 0) {
            break;
          }
        }  
      }
    }
  }
}

VOID
CheckForRom (
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *IoDev,
  UINT16                           MinBus,
  UINT16                           MaxBus,
  UINT16                           MinDevice,
  UINT16                           MaxDevice,
  UINT16                           MinFunc,
  UINT16                           MaxFunc,
  UINT16                           Bus,
  UINT16                           Device,
  UINT16                           Func,
  IN VOID                          *VoidContext
  )
{
  EFI_STATUS                                 Status;
  PCAT_PCI_ROOT_BRIDGE_SCAN_FOR_ROM_CONTEXT  *Context;
  UINT64                                     Address;
  PCI_TYPE00                                 PciHeader;
  PCI_TYPE01                                 *PciBridgeHeader;
  UINT32                                     Register;
  UINT32                                     RomBar;
  UINT32                                     RomBarSize;
  EFI_PHYSICAL_ADDRESS                       RomBuffer;
  UINT32                                     MaxRomSize;
  EFI_PCI_EXPANSION_ROM_HEADER               EfiRomHeader;
  PCI_DATA_STRUCTURE                         Pcir;
  EFI_PCI_OPTION_ROM_DESCRIPTOR              *TempPciOptionRomDescriptors;
  BOOLEAN                                    LastImage;

  Context = (PCAT_PCI_ROOT_BRIDGE_SCAN_FOR_ROM_CONTEXT *)VoidContext;

  Address = EFI_PCI_ADDRESS (Bus, Device, Func, 0);

  //
  // Save the contents of the PCI Configuration Header
  //
  IoDev->Pci.Read (IoDev, EfiPciWidthUint32, Address, sizeof(PciHeader)/sizeof(UINT32), &PciHeader);

  if (IS_PCI_BRIDGE(&PciHeader)) {

    PciBridgeHeader = (PCI_TYPE01 *)(&PciHeader);

    //
    // See if the PCI-PCI Bridge has its secondary interface enabled.
    //
    if (PciBridgeHeader->Bridge.SubordinateBus >= PciBridgeHeader->Bridge.SecondaryBus) {

      //
      // Disable the Prefetchable Memory Window
      //
      Register = 0x00000000;
      IoDev->Pci.Write (IoDev, EfiPciWidthUint16, Address + 0x26, 1, &Register);
      IoDev->Pci.Write (IoDev, EfiPciWidthUint32, Address + 0x2c, 1, &Register);
      Register = 0xffffffff;
      IoDev->Pci.Write (IoDev, EfiPciWidthUint16, Address + 0x24, 1, &Register);
      IoDev->Pci.Write (IoDev, EfiPciWidthUint16, Address + 0x28, 1, &Register);

      //
      // Program Memory Window to the PCI Root Bridge Memory Window
      //
      IoDev->Pci.Write (IoDev, EfiPciWidthUint16, Address + 0x20, 4, &Context->PpbMemoryWindow);

      //
      // Enable the Memory decode for the PCI-PCI Bridge
      //
      IoDev->Pci.Read (IoDev, EfiPciWidthUint16, Address + 4, 1, &Register);
      Register |= 0x02;
      IoDev->Pci.Write (IoDev, EfiPciWidthUint16, Address + 4, 1, &Register);

      //
      // Recurse on the Secondary Bus Number
      //
      ScanPciBus(
        IoDev,
        PciBridgeHeader->Bridge.SecondaryBus, PciBridgeHeader->Bridge.SecondaryBus, 
        0, PCI_MAX_DEVICE, 
        0, PCI_MAX_FUNC, 
        CheckForRom, Context
        );
    }
  } else {

    //
    // Check if an Option ROM Register is present and save the Option ROM Window Register
    //
    RomBar = 0xffffffff;
    IoDev->Pci.Write (IoDev, EfiPciWidthUint32, Address + 0x30, 1, &RomBar);
    IoDev->Pci.Read (IoDev, EfiPciWidthUint32, Address + 0x30, 1, &RomBar);

    RomBarSize = (~(RomBar & 0xfffff800)) + 1;

    //
    // Make sure the size of the ROM is between 0 and 16 MB
    //
    if (RomBarSize > 0 && RomBarSize <= 0x01000000) {

      //
      // Program Option ROM Window Register to the PCI Root Bridge Window and Enable the Option ROM Window
      //
      RomBar = (Context->PpbMemoryWindow & 0xffff) << 16;
      RomBar = ((RomBar - 1) & (~(RomBarSize - 1))) + RomBarSize;
      if (RomBar < (Context->PpbMemoryWindow & 0xffff0000)) {
        MaxRomSize = (Context->PpbMemoryWindow & 0xffff0000) - RomBar;
        RomBar = RomBar + 1;
        IoDev->Pci.Write (IoDev, EfiPciWidthUint32, Address + 0x30, 1, &RomBar);
        IoDev->Pci.Read  (IoDev, EfiPciWidthUint32, Address + 0x30, 1, &RomBar);
        RomBar = RomBar - 1;

        //
        // Enable the Memory decode for the PCI Device
        //
        IoDev->Pci.Read (IoDev, EfiPciWidthUint16, Address + 4, 1, &Register);
        Register |= 0x02;
        IoDev->Pci.Write (IoDev, EfiPciWidthUint16, Address + 4, 1, &Register);

        //
        // Follow the chain of images to determine the size of the Option ROM present
        // Keep going until the last image is found by looking at the Indicator field
        // or the size of an image is 0, or the size of all the images is bigger than the
        // size of the window programmed into the PPB.
        //
        RomBarSize = 0;
        do {

          LastImage = TRUE;

          ZeroMem (&EfiRomHeader, sizeof(EfiRomHeader));
          IoDev->Mem.Read (
            IoDev, 
            EfiPciWidthUint8, 
            RomBar + RomBarSize, 
            sizeof(EfiRomHeader),
            &EfiRomHeader
            );

          Pcir.ImageLength = 0;

          if (EfiRomHeader.Signature == PCI_EXPANSION_ROM_HEADER_SIGNATURE &&
              EfiRomHeader.PcirOffset != 0 &&
              (EfiRomHeader.PcirOffset & 3) == 0 &&
              RomBarSize + EfiRomHeader.PcirOffset + sizeof (PCI_DATA_STRUCTURE) <= MaxRomSize) {
            ZeroMem (&Pcir, sizeof(Pcir));
            IoDev->Mem.Read (
              IoDev, 
              EfiPciWidthUint8, 
              RomBar + RomBarSize + EfiRomHeader.PcirOffset, 
              sizeof(Pcir),
              &Pcir
              );

            if (Pcir.Signature != PCI_DATA_STRUCTURE_SIGNATURE) {
              break;
            }
            if (RomBarSize + Pcir.ImageLength * 512 > MaxRomSize) {
              break;
            }
            if ((Pcir.Indicator & 0x80) == 0x00) {
              LastImage = FALSE;
            }

            RomBarSize += Pcir.ImageLength * 512;
          }
        } while (!LastImage && RomBarSize < MaxRomSize && Pcir.ImageLength !=0);

        if (RomBarSize > 0) {

          //
          // Allocate a memory buffer for the Option ROM contents.
          //
          Status = gBS->AllocatePages(
                          AllocateAnyPages,
                          EfiBootServicesData,
                          EFI_SIZE_TO_PAGES(RomBarSize),
                          &RomBuffer
                          );

          if (!EFI_ERROR (Status)) {

            //
            // Copy the contents of the Option ROM to the memory buffer
            //
            IoDev->Mem.Read (IoDev, EfiPciWidthUint32, RomBar, RomBarSize / sizeof(UINT32), (VOID *)(UINTN)RomBuffer);

            Status = gBS->AllocatePool(
                            EfiBootServicesData,
                            ((UINT32)mPciOptionRomTable.PciOptionRomCount + 1) * sizeof(EFI_PCI_OPTION_ROM_DESCRIPTOR),
                            (VOID **) &TempPciOptionRomDescriptors
                            );
            if (mPciOptionRomTable.PciOptionRomCount > 0) {
              CopyMem(
                TempPciOptionRomDescriptors, 
                mPciOptionRomTable.PciOptionRomDescriptors, 
                (UINT32)mPciOptionRomTable.PciOptionRomCount * sizeof(EFI_PCI_OPTION_ROM_DESCRIPTOR)
                );

              gBS->FreePool(mPciOptionRomTable.PciOptionRomDescriptors);
            }

            mPciOptionRomTable.PciOptionRomDescriptors = TempPciOptionRomDescriptors; 

            TempPciOptionRomDescriptors = &(mPciOptionRomTable.PciOptionRomDescriptors[(UINT32)mPciOptionRomTable.PciOptionRomCount]);

            TempPciOptionRomDescriptors->RomAddress              = RomBuffer;
            TempPciOptionRomDescriptors->MemoryType              = EfiBootServicesData;
            TempPciOptionRomDescriptors->RomLength               = RomBarSize;
            TempPciOptionRomDescriptors->Seg                     = (UINT32)IoDev->SegmentNumber;
            TempPciOptionRomDescriptors->Bus                     = (UINT8)Bus;
            TempPciOptionRomDescriptors->Dev                     = (UINT8)Device;
            TempPciOptionRomDescriptors->Func                    = (UINT8)Func;
            TempPciOptionRomDescriptors->ExecutedLegacyBiosImage = TRUE;
            TempPciOptionRomDescriptors->DontLoadEfiRom          = FALSE;

            mPciOptionRomTable.PciOptionRomCount++;
          }
        }

        //
        // Disable the Memory decode for the PCI-PCI Bridge
        //
        IoDev->Pci.Read (IoDev, EfiPciWidthUint16, Address + 4, 1, &Register);
        Register &= (~0x02);
        IoDev->Pci.Write (IoDev, EfiPciWidthUint16, Address + 4, 1, &Register);
      }
    }
  }

  //
  // Restore the PCI Configuration Header 
  //
  IoDev->Pci.Write (IoDev, EfiPciWidthUint32, Address, sizeof(PciHeader)/sizeof(UINT32), &PciHeader);
}

VOID
SaveCommandRegister (
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *IoDev,
  UINT16                           MinBus,
  UINT16                           MaxBus,
  UINT16                           MinDevice,
  UINT16                           MaxDevice,
  UINT16                           MinFunc,
  UINT16                           MaxFunc,
  UINT16                           Bus,
  UINT16                           Device,
  UINT16                           Func,
  IN VOID                          *VoidContext
  )

{
  PCAT_PCI_ROOT_BRIDGE_SCAN_FOR_ROM_CONTEXT  *Context;
  UINT64  Address;
  UINTN   Index;
  UINT16  Command;

  Context = (PCAT_PCI_ROOT_BRIDGE_SCAN_FOR_ROM_CONTEXT *)VoidContext;

  Address = EFI_PCI_ADDRESS (Bus, Device, Func, 4);

  Index = (Bus - MinBus) * (PCI_MAX_DEVICE+1) * (PCI_MAX_FUNC+1) + Device * (PCI_MAX_FUNC+1) + Func;

  IoDev->Pci.Read (IoDev, EfiPciWidthUint16, Address, 1, &Context->CommandRegisterBuffer[Index]);

  //
  // Clear the memory enable bit
  //
  Command = (UINT16) (Context->CommandRegisterBuffer[Index] & (~0x02));

  IoDev->Pci.Write (IoDev, EfiPciWidthUint16, Address, 1, &Command);
}

VOID
RestoreCommandRegister (
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *IoDev,
  UINT16                           MinBus,
  UINT16                           MaxBus,
  UINT16                           MinDevice,
  UINT16                           MaxDevice,
  UINT16                           MinFunc,
  UINT16                           MaxFunc,
  UINT16                           Bus,
  UINT16                           Device,
  UINT16                           Func,
  IN VOID                          *VoidContext
  )

{
  PCAT_PCI_ROOT_BRIDGE_SCAN_FOR_ROM_CONTEXT  *Context;
  UINT64                                     Address;
  UINTN                                      Index;

  Context = (PCAT_PCI_ROOT_BRIDGE_SCAN_FOR_ROM_CONTEXT *)VoidContext;

  Address = EFI_PCI_ADDRESS (Bus, Device, Func, 4);

  Index = (Bus - MinBus) * (PCI_MAX_DEVICE+1) * (PCI_MAX_FUNC+1) + Device * (PCI_MAX_FUNC+1) + Func;

  IoDev->Pci.Write (IoDev, EfiPciWidthUint16, Address, 1, &Context->CommandRegisterBuffer[Index]);
}

EFI_STATUS
ScanPciRootBridgeForRoms(
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *IoDev
  )
  
{
  EFI_STATUS                                 Status;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR          *Descriptors; 
  UINT16                                     MinBus;
  UINT16                                     MaxBus;
  UINT64                                     RootWindowBase;
  UINT64                                     RootWindowLimit;
  PCAT_PCI_ROOT_BRIDGE_SCAN_FOR_ROM_CONTEXT  Context;

  if (mPciOptionRomTableInstalled == FALSE) {
    gBS->InstallConfigurationTable(&gEfiPciOptionRomTableGuid, &mPciOptionRomTable);
    mPciOptionRomTableInstalled = TRUE;
  }

  Status = IoDev->Configuration(IoDev, (VOID **) &Descriptors);
  if (EFI_ERROR (Status) || Descriptors == NULL) {
    return EFI_NOT_FOUND;
  }

  MinBus = 0xffff;
  MaxBus = 0xffff;
  RootWindowBase  = 0;
  RootWindowLimit = 0;
  while (Descriptors->Desc != ACPI_END_TAG_DESCRIPTOR) {
    //
    // Find bus range
    //
    if (Descriptors->ResType == ACPI_ADDRESS_SPACE_TYPE_BUS) {
      MinBus = (UINT16)Descriptors->AddrRangeMin;
      MaxBus = (UINT16)Descriptors->AddrRangeMax;
    }
    //
    // Find memory descriptors that are not prefetchable
    //
    if (Descriptors->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM && Descriptors->SpecificFlag == 0) {
      //
      // Find Memory Descriptors that are less than 4GB, so the PPB Memory Window can be used for downstream devices
      //
      if (Descriptors->AddrRangeMax < 0x100000000ULL) {
        //
        // Find the largest Non-Prefetchable Memory Descriptor that is less than 4GB
        //
        if ((Descriptors->AddrRangeMax - Descriptors->AddrRangeMin) > (RootWindowLimit - RootWindowBase)) {
          RootWindowBase  = Descriptors->AddrRangeMin;
          RootWindowLimit = Descriptors->AddrRangeMax;
        }
      }
    }
    Descriptors ++;
  }

  //
  // Make sure a bus range was found
  //
  if (MinBus == 0xffff || MaxBus == 0xffff) {
    return EFI_NOT_FOUND;
  }

  //
  // Make sure a non-prefetchable memory region was found
  //
  if (RootWindowBase == 0 && RootWindowLimit == 0) {
    return EFI_NOT_FOUND;
  }

  //
  // Round the Base and Limit values to 1 MB boudaries
  //
  RootWindowBase  = ((RootWindowBase - 1) & 0xfff00000) + 0x00100000;
  RootWindowLimit = ((RootWindowLimit + 1) & 0xfff00000) - 1;

  //
  // Make sure that the size of the rounded window is greater than zero
  //
  if (RootWindowLimit <= RootWindowBase) {
    return EFI_NOT_FOUND;
  }

  //
  // Allocate buffer to save the Command register from all the PCI devices
  //
  Context.CommandRegisterBuffer = NULL;
  Status = gBS->AllocatePool(
                  EfiBootServicesData,
                  sizeof(UINT16) * (MaxBus - MinBus + 1) * (PCI_MAX_DEVICE+1) * (PCI_MAX_FUNC+1),
                  (VOID **) &Context.CommandRegisterBuffer
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Context.PpbMemoryWindow   = (((UINT32)RootWindowBase) >> 16) | ((UINT32)RootWindowLimit & 0xffff0000);

  //
  // Save the Command register from all the PCI devices, and disable the I/O, Mem, and BusMaster bits
  //
  ScanPciBus(
    IoDev,
    MinBus, MaxBus, 
    0, PCI_MAX_DEVICE, 
    0, PCI_MAX_FUNC, 
    SaveCommandRegister, &Context
    );

  //
  // Recursively scan all the busses for PCI Option ROMs
  //
  ScanPciBus(
    IoDev,
    MinBus, MinBus, 
    0, PCI_MAX_DEVICE, 
    0, PCI_MAX_FUNC, 
    CheckForRom, &Context
    );

  //
  // Restore the Command register in all the PCI devices
  //
  ScanPciBus(
    IoDev,
    MinBus, MaxBus, 
    0, PCI_MAX_DEVICE, 
    0, PCI_MAX_FUNC, 
    RestoreCommandRegister, &Context
    );

  //
  // Free the buffer used to save all the Command register values
  //
  gBS->FreePool(Context.CommandRegisterBuffer);

  return EFI_SUCCESS;
}
