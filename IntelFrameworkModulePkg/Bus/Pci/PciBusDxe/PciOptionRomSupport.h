/**@file

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _EFI_PCI_OP_ROM_SUPPORT_H
#define _EFI_PCI_OP_ROM_SUPPORT_H

/**
  Get Pci device's oprom infor bits.
  
  @retval EFI_NOT_FOUND Pci device has not oprom
  @retval EFI_SUCCESS   Pci device has oprom
**/
EFI_STATUS
GetOpRomInfo (
  IN PCI_IO_DEVICE    *PciIoDevice
  );

/**
  Load option rom image for specified PCI device
  
  @param PciDevice Pci device instance
  @param RomBase   Base address of oprom.
  
  @retval EFI_OUT_OF_RESOURCES not enough memory to hold image
  @retval EFI_SUCESS           Success
**/
EFI_STATUS
LoadOpRomImage (
  IN PCI_IO_DEVICE   *PciDevice,
  IN UINT64          RomBase
  );

/**
  enable/disable oprom decode
  
  @param PciDevice    pci device instance
  @param RomBarIndex  The BAR index of the standard PCI Configuration header to use as the
                      base address for resource range. The legal range for this field is 0..5.
  @param RomBar       Base address of rom
  @param Enable       Flag for enable/disable decode.
  
  @retval EFI_SUCCESS Success
**/
EFI_STATUS
RomDecode (
  IN PCI_IO_DEVICE   *PciDevice,
  IN UINT8           RomBarIndex,
  IN UINT32          RomBar,
  IN BOOLEAN         Enable
  );

/**
  Process the oprom image.
  
  @param PciDevice Pci device instance
**/
EFI_STATUS
ProcessOpRomImage (
  PCI_IO_DEVICE   *PciDevice
  );

#endif
