/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FileSystemVolumeLabelInfo.h

Abstract:

  FileSystemVolumeLabelInfo protocol as defined in the EFI 1.0 specification.

--*/

#ifndef _FILE_SYSTEM_VOLUME_LABEL_INFO_H_
#define _FILE_SYSTEM_VOLUME_LABEL_INFO_H_

#define EFI_FILE_SYSTEM_VOLUME_LABEL_INFO_ID_GUID \
  { \
    0xDB47D7D3, 0xFE81, 0x11d3, {0x9A, 0x35, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D} \
  }

typedef struct {
  CHAR16  VolumeLabel[1];
} EFI_FILE_SYSTEM_VOLUME_LABEL_INFO;

#define SIZE_OF_EFI_FILE_SYSTEM_VOLUME_LABEL_INFO EFI_FIELD_OFFSET (EFI_FILE_SYSTEM_VOLUME_LABEL_INFO, VolumeLabel)

extern EFI_GUID gEfiFileSystemVolumeLabelInfoIdGuid;

#endif
