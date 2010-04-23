/** @file
  This file declares the hardware-device class GUIDs that may be used by the 
  PEIM that produces the Virtual Block I/O PPI.

  These GUIDs are hardware-device class GUIDs that would be imported only by the
  Virtual Block I/O PEIM.  This virtual PEIM imports only the actual Block I/O 
  PPIs from the device-class ones listed here and published a single instance of
  the Block I/O PPI for consumption by the File System PEIM.  In the parlance of
  the Framework DXE software stack, this Virtual Block I/O PEIM is actually 
  embodying the functionality of the partition driver.  Thsi Virtual Block I/O
  PEIM has to multiple the multiple possible instances of Block I/O and also know
  how to parse at least El Torito for CD-ROM, and perhaps Master Boot Record(MBR)
  and GUID Partition Table(GPT) in the future.
  
Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  These GUIDs are defined in Framework Recovery Specification Version 0.9

**/

#ifndef _PEI_BLOCK_IO_GUID_H_
#define _PEI_BLOCK_IO_GUID_H_

///
/// Global ID for an IDE class recovery device.
///
#define EFI_PEI_IDE_BLOCK_IO_PPI \
  { \
    0x0964e5b22, 0x6459, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b }  \
  }

///
/// Global ID for a Floppy class recovery device.
///
#define EFI_PEI_144_FLOPPY_BLOCK_IO_PPI \
  { \
    0xda6855bd, 0x07b7, 0x4c05, { 0x9e, 0xd8, 0xe2, 0x59, 0xfd, 0x36, 0x0e, 0x22 }  \
  }

extern EFI_GUID gEfiPeiIdeBlockIoPpiGuid;
extern EFI_GUID gEfiPei144FloppyBlockIoPpiGuid;

#endif
