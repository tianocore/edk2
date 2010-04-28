/** @file
  Block IO (memory mapped)

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __BLOCK_MMIO_H__
#define __BLOCK_MMIO_H__

#include <Protocol/BlockIo.h>

#define BLOCK_MMIO_PROTOCOL_GUID \
  { \
    0x6b558ce3, 0x69e5, 0x4c67, {0xa6, 0x34, 0xf7, 0xfe, 0x72, 0xad, 0xbe, 0x84 } \
  }

typedef struct _BLOCK_MMIO_PROTOCOL  BLOCK_MMIO_PROTOCOL;


///
///  This protocol provides control over block devices.
///
struct _BLOCK_MMIO_PROTOCOL {
  ///
  /// The revision to which the block IO interface adheres. All future
  /// revisions must be backwards compatible. If a future version is not
  /// back wards compatible, it is not the same GUID.
  ///
  UINT64                Revision;
  ///
  /// Pointer to the EFI_BLOCK_IO_MEDIA data for this device.
  ///
  EFI_BLOCK_IO_MEDIA    *Media;

  EFI_PHYSICAL_ADDRESS  BaseAddress;

};

extern EFI_GUID gBlockMmioProtocolGuid;

#endif

