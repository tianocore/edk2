/** @file

Copyright (c) 2006 - 2009, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _EFI_PCI_OP_ROM_SUPPORT_H_
#define _EFI_PCI_OP_ROM_SUPPORT_H_

#include <Protocol/LoadFile2.h>

/**
  Initialize a PCI LoadFile2 instance.
  
  @param PciIoDevice   PCI IO Device.

**/
VOID
InitializePciLoadFile2 (
  PCI_IO_DEVICE       *PciIoDevice
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

  Check if the RomImage contains EFI Images.

  @param  RomImage  The ROM address of Image for check. 
  @param  RomSize   Size of ROM for check.

  @retval TRUE     ROM contain EFI Image.
  @retval FALSE    ROM not contain EFI Image.
  
**/
BOOLEAN
ContainEfiImage (
  IN VOID            *RomImage,
  IN UINT64          RomSize
  ); 


/**
  Get Pci device's oprom infor bits.
  
  @param PciIoDevice Pci device instance

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
