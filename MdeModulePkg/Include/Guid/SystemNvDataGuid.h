/** @file
  This file defines NvData Hob GUIDs for System Non Volatile HOB entries 
  and the corresponding hob data structure. NvData Hob can be used to report 
  the region of the system non volatile data for the specific purpose, 
  such as FTW region, Error log region.
  
  It also defines NvDataFv GUID. 
  This guid can be used as FileSystemGuid in EFI_FIRMWARE_VOLUME_HEADER if 
  this FV image contains NV data, such as NV variable data.
  This guid can also be used as the signature of FTW working block header.

Copyright (c) 2006 - 2009, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __SYSTEM_NV_DATA_GUID_H__
#define __SYSTEM_NV_DATA_GUID_H__

#define EFI_SYSTEM_NV_DATA_FV_GUID \
  {0xfff12b8d, 0x7696, 0x4c8b, {0xa9, 0x85, 0x27, 0x47, 0x7, 0x5b, 0x4f, 0x50} }

#define EFI_SYSTEM_NV_DATA_HOB_GUID \
  {0xd6e5092d, 0xc7b2, 0x4872, {0xaf, 0x66, 0xfd, 0xc0, 0xe6, 0xf9, 0x5e, 0x78} }

///
/// Hob entry for NV data region
///
typedef struct {
  EFI_GUID                  SystemNvDataHobGuid; ///> EFI_SYSTEM_NV_DATA_HOB_GUID
  EFI_GUID                  SystemNvDataFvGuid;  ///> Guid specifies the NvData Fv for the specific purpose, such as FTW, Error Log.
  EFI_LBA                   StartLba;            ///> The starting logical block index.
  UINTN                     StartLbaOffset;      ///> Offset into the starting block at which to the start of NvData region.
  EFI_LBA                   EndLba;              ///> The last logical block index.
  UINTN                     EndLbaOffset;        ///> Offset into the last block at which to the end of Nvdata region.
  UINT32                    DataTypeSignature;   ///> NvData type in the specified NV range.
} NV_SYSTEM_DATA_GUID_TYPE;

extern EFI_GUID gEfiSystemNvDataHobGuid;

///
/// NvDataFv GUID used as the signature of FTW working block header.
///
extern EFI_GUID gEfiSystemNvDataFvGuid;

#endif
