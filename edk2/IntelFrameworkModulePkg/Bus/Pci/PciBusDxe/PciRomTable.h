/** @file
  Option Rom Support for PCI Bus Driver

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _EFI_PCI_ROM_TABLE_H_
#define _EFI_PCI_ROM_TABLE_H_

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
  IN EFI_HANDLE  ImageHandle,
  IN UINTN       Seg,
  IN UINT8       Bus,
  IN UINT8       Dev,
  IN UINT8       Func,
  IN UINT64      RomAddress,
  IN UINT64      RomLength
  );
/**
  Load all option rom image to PCI driver list.
  
  @param This             Pointer to protocol instance EFI_DRIVER_BINDING_PROTOCOL.
  @param PciRootBridgeIo  Root bridge Io instance.
  @param PciIoDevice      device instance.
**/
EFI_STATUS
PciRomGetRomResourceFromPciOptionRomTable (
  IN EFI_DRIVER_BINDING_PROTOCOL      *This,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  PCI_IO_DEVICE                       *PciIoDevice
  );

/**
  Get Option rom driver's mapping for PCI device.
  
  @param PciIoDevice Device instance.

**/
EFI_STATUS
PciRomGetImageMapping (
  PCI_IO_DEVICE                       *PciIoDevice
  );

#endif
