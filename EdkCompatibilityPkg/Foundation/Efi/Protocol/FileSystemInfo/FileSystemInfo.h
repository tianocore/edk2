/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FileSystemInfo.h

Abstract:

  FileSystemInfo protocol as defined in the EFI 1.0 specification.

 
--*/

#ifndef _FILE_SYSTEM_INFO_H_
#define _FILE_SYSTEM_INFO_H_

#define EFI_FILE_SYSTEM_INFO_ID_GUID \
  { \
    0x9576e93, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} \
  }

typedef struct {
  UINT64  Size;
  BOOLEAN ReadOnly;
  UINT64  VolumeSize;
  UINT64  FreeSpace;
  UINT32  BlockSize;
  CHAR16  VolumeLabel[1];
} EFI_FILE_SYSTEM_INFO;

//
// The VolumeLabel field of the EFI_FILE_SYSTEM_INFO data structure is variable length.
// Whenever code needs to know the size of the EFI_FILE_SYSTEM_INFO data structure, it needs
// to be the size of the data structure without the VolumeLable field.  The following macro
// computes this size correctly no matter how big the VolumeLable array is declared.
// This is required to make the EFI_FILE_SYSTEM_INFO data structure ANSI compilant.
//
#define SIZE_OF_EFI_FILE_SYSTEM_INFO  EFI_FIELD_OFFSET (EFI_FILE_SYSTEM_INFO, VolumeLabel)

extern EFI_GUID gEfiFileSystemInfoGuid;

#endif
