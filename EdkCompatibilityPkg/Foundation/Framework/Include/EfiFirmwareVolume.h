/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiFirmwareVolume.h

Abstract:

--*/

#ifndef _EFI_FIRMWARE_VOLUME_H_
#define _EFI_FIRMWARE_VOLUME_H_


#define FV_DEVICE_SIGNATURE           EFI_SIGNATURE_32 ('_', 'F', 'V', '_')
#define EFI_FV_FILE_ATTRIB_ALIGNMENT  0x0000001F
#define EFI_FV_UNRELIABLE_WRITE       0x00000000
#define EFI_FV_RELIABLE_WRITE         0x00000001


//
// EFI_FV_ATTRIBUTES bit definitions
//
typedef UINT64  EFI_FV_ATTRIBUTES;
typedef UINT32  EFI_FV_FILE_ATTRIBUTES;
typedef UINT32  EFI_FV_WRITE_POLICY;


typedef struct {
  EFI_GUID                *NameGuid;
  EFI_FV_FILETYPE         Type;
  EFI_FV_FILE_ATTRIBUTES  FileAttributes;
  VOID                    *Buffer;
  UINT32                  BufferSize;
} EFI_FV_WRITE_FILE_DATA;

#endif
