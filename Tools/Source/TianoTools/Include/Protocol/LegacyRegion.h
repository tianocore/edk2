/** @file
  This protocol manages the legacy memory regions between 0xc0000 - 0xfffff

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    

  Module Name:  LegacyRegion.h

  @par Revision Reference:
  This protocol is defined in Framework for EFI Compatibility Support Module spec
  Version 0.96

**/

#ifndef _EFI_LEGACY_REGION_H_
#define _EFI_LEGACY_REGION_H_

#define EFI_LEGACY_REGION_PROTOCOL_GUID \
  { \
    0xfc9013a, 0x568, 0x4ba9, {0x9b, 0x7e, 0xc9, 0xc3, 0x90, 0xa6, 0x60, 0x9b } \
  }

typedef struct _EFI_LEGACY_REGION_PROTOCOL EFI_LEGACY_REGION_PROTOCOL;

/**
  Sets hardware to decode or not decode a region.

  @param  This Indicates the EFI_LEGACY_REGION_PROTOCOL instance
  
  @param  Start Start of region to decode.
  
  @param  Length Size in bytes of the region.
  
  @param  On Decode/nondecode flag.

  @retval EFI_SUCCESS Decode range successfully changed.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_REGION_DECODE) (
  IN EFI_LEGACY_REGION_PROTOCOL           *This,
  IN  UINT32                              Start,
  IN  UINT32                              Length,
  IN  BOOLEAN                             *On
  );

/**
  Sets a region to read only.

  @param  This Indicates the EFI_LEGACY_REGION_PROTOCOL instance
  
  @param  Start Start of region to lock.
  
  @param  Length Size in bytes of the region.
  
  @param  Granularity Lock attribute affects this granularity in bytes.

  @retval EFI_SUCCESS The region was made read only.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_REGION_LOCK) (
  IN EFI_LEGACY_REGION_PROTOCOL           *This,
  IN  UINT32                              Start,
  IN  UINT32                              Length,
  OUT UINT32                              *Granularity OPTIONAL
  );

/**
  Sets a region to read only and ensures that flash is locked from being 
  inadvertently modified.

  @param  This Indicates the EFI_LEGACY_REGION_PROTOCOL instance
  
  @param  Start Start of region to lock.
  
  @param  Length Size in bytes of the region.
  
  @param  Granularity Lock attribute affects this granularity in bytes.

  @retval EFI_SUCCESS The region was made read only and flash is locked.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_REGION_BOOT_LOCK) (
  IN EFI_LEGACY_REGION_PROTOCOL           *This,
  IN  UINT32                              Start,
  IN  UINT32                              Length,
  OUT UINT32                              *Granularity OPTIONAL
  );

/**
  Sets a region to read-write.

  @param  This Indicates the EFI_LEGACY_REGION_PROTOCOL instance
  
  @param  Start Start of region to lock.
  
  @param  Length Size in bytes of the region.
  
  @param  Granularity Lock attribute affects this granularity in bytes.

  @retval EFI_SUCCESS The region was successfully made read-write.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_REGION_UNLOCK) (
  IN EFI_LEGACY_REGION_PROTOCOL           *This,
  IN  UINT32                              Start,
  IN  UINT32                              Length,
  OUT UINT32                              *Granularity OPTIONAL
  );

/**
  @par Protocol Description:
  Abstracts the hardware control of the physical address region 0xC0000¨C0xFFFFF 
  for the traditional BIOS.

  @param Decode
  Specifies a region for the chipset to decode

  @param Lock
  Makes the specified OpROM region read only or locked.

  @param BootLock
  Sets a region to read only and ensures tat flash is locked from 
  inadvertent modification.

  @param Unlock
  Makes the specified OpROM region read-write or unlocked. 

**/
struct _EFI_LEGACY_REGION_PROTOCOL {
  EFI_LEGACY_REGION_DECODE    Decode;
  EFI_LEGACY_REGION_LOCK      Lock;
  EFI_LEGACY_REGION_BOOT_LOCK BootLock;
  EFI_LEGACY_REGION_UNLOCK    UnLock;
};

extern EFI_GUID gEfiLegacyRegionProtocolGuid;

#endif
