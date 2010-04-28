/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FirmwareVolume2.h

Abstract:

  PI 1.0 spec definition.

--*/

#ifndef __FIRMWARE_VOLUME2_H__
#define __FIRMWARE_VOLUME2_H__
#include "EfiImageFormat.h"
#include "EfiFirmwareVolume.h"

//
// Firmware Volume Protocol GUID definition
//
#define EFI_FIRMWARE_VOLUME2_PROTOCOL_GUID \
  { 0x220e73b6, 0x6bdb, 0x4413, {0x84, 0x5, 0xb9, 0x74, 0xb1, 0x8, 0x61, 0x9a} }


EFI_FORWARD_DECLARATION (EFI_FIRMWARE_VOLUME2_PROTOCOL);


//
// ************************************************************
// EFI_FV2_ATTRIBUTES bit definitions
// ************************************************************
//
#define EFI_FV2_READ_DISABLE_CAP       0x0000000000000001
#define EFI_FV2_READ_ENABLE_CAP        0x0000000000000002
#define EFI_FV2_READ_STATUS            0x0000000000000004

#define EFI_FV2_WRITE_DISABLE_CAP      0x0000000000000008
#define EFI_FV2_WRITE_ENABLE_CAP       0x0000000000000010
#define EFI_FV2_WRITE_STATUS           0x0000000000000020

#define EFI_FV2_LOCK_CAP               0x0000000000000040
#define EFI_FV2_LOCK_STATUS            0x0000000000000080
#define EFI_FV2_WRITE_POLICY_RELIABLE  0x0000000000000100

#define EFI_FV2_READ_LOCK_CAP          0x0000000000001000
#define EFI_FV2_READ_LOCK_STATUS       0x0000000000002000
#define EFI_FV2_WRITE_LOCK_CAP         0x0000000000004000
#define EFI_FV2_WRITE_LOCK_STATUS      0x0000000000008000
#define EFI_FV2_ALIGNMENT              0x00000000001F0000


#define EFI_FV2_ALIGNMENT_1            0x0000000000000000
#define EFI_FV2_ALIGNMENT_2            0x0000000000010000
#define EFI_FV2_ALIGNMENT_4            0x0000000000020000
#define EFI_FV2_ALIGNMENT_8            0x0000000000030000
#define EFI_FV2_ALIGNMENT_16           0x0000000000040000
#define EFI_FV2_ALIGNMENT_32           0x0000000000050000
#define EFI_FV2_ALIGNMENT_64           0x0000000000060000
#define EFI_FV2_ALIGNMENT_128          0x0000000000070000
#define EFI_FV2_ALIGNMENT_256          0x0000000000080000
#define EFI_FV2_ALIGNMENT_512          0x0000000000090000
#define EFI_FV2_ALIGNMENT_1K           0x00000000000A0000
#define EFI_FV2_ALIGNMENT_2K           0x00000000000B0000
#define EFI_FV2_ALIGNMENT_4K           0x00000000000C0000
#define EFI_FV2_ALIGNMENT_8K           0x00000000000D0000
#define EFI_FV2_ALIGNMENT_16K          0x00000000000E0000
#define EFI_FV2_ALIGNMENT_32K          0x00000000000F0000
#define EFI_FV2_ALIGNMENT_64K          0x0000000000100000
#define EFI_FV2_ALIGNMENT_128K         0x0000000000110000
#define EFI_FV2_ALIGNMENT_256K         0x0000000000120000
#define EFI_FV2_ALIGNMENT_512K         0x0000000000130000
#define EFI_FV2_ALIGNMENT_1M           0x0000000000140000
#define EFI_FV2_ALIGNMENT_2M           0x0000000000150000
#define EFI_FV2_ALIGNMENT_4M           0x0000000000160000
#define EFI_FV2_ALIGNMENT_8M           0x0000000000170000
#define EFI_FV2_ALIGNMENT_16M          0x0000000000180000
#define EFI_FV2_ALIGNMENT_32M          0x0000000000190000
#define EFI_FV2_ALIGNMENT_64M          0x00000000001A0000
#define EFI_FV2_ALIGNMENT_128M         0x00000000001B0000
#define EFI_FV2_ALIGNMENT_256M         0x00000000001C0000
#define EFI_FV2_ALIGNMENT_512M         0x00000000001D0000
#define EFI_FV2_ALIGNMENT_1G           0x00000000001E0000
#define EFI_FV2_ALIGNMENT_2G           0x00000000001F0000

#define EFI_FV_FILE_ATTRIB_FIXED           0x00000100
#define EFI_FV_FILE_ATTRIB_MEMORY_MAPPED   0x00000200


//
// Protocol API definitions
//

typedef
EFI_STATUS
(EFIAPI *EFI_FV_GET_ATTRIBUTES) (
  IN  CONST EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
  OUT EFI_FV_ATTRIBUTES                  *Attributes
  );

typedef
EFI_STATUS
(EFIAPI *EFI_FV_SET_ATTRIBUTES) (
  IN CONST EFI_FIRMWARE_VOLUME2_PROTOCOL   *This,
  IN OUT EFI_FV_ATTRIBUTES                *Attributes
  );

typedef
EFI_STATUS
(EFIAPI *EFI_FV_READ_FILE) (
  IN CONST EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  IN CONST EFI_GUID                       *NameGuid,
  IN OUT VOID                             **Buffer,
  IN OUT UINTN                            *BufferSize,
  OUT EFI_FV_FILETYPE                     *FoundType,
  OUT EFI_FV_FILE_ATTRIBUTES              *FileAttributes,
  OUT UINT32                              *AuthenticationStatus
  );

typedef
EFI_STATUS
(EFIAPI *EFI_FV_READ_SECTION) (
  IN CONST EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  IN CONST EFI_GUID                       *NameGuid,
  IN EFI_SECTION_TYPE                     SectionType,
  IN UINTN                                SectionInstance,
  IN OUT VOID                             **Buffer,
  IN OUT UINTN                            *BufferSize,
  OUT UINT32                              *AuthenticationStatus
  );


typedef
EFI_STATUS
(EFIAPI *EFI_FV_WRITE_FILE) (
  IN CONST EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  IN UINT32                               NumberOfFiles,
  IN EFI_FV_WRITE_POLICY                  WritePolicy,
  IN EFI_FV_WRITE_FILE_DATA               *FileData
  );

typedef
EFI_STATUS
(EFIAPI *EFI_FV_GET_NEXT_FILE) (
  IN CONST EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  IN OUT VOID                             *Key,
  IN OUT EFI_FV_FILETYPE                  *FileType,
  OUT EFI_GUID                            *NameGuid,
  OUT EFI_FV_FILE_ATTRIBUTES              *Attributes,
  OUT UINTN                               *Size
  );

typedef
EFI_STATUS
(EFIAPI *EFI_FV_GET_INFO) (
  IN  CONST EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  IN  CONST EFI_GUID                       *InformationType,
  IN  OUT UINTN                            *BufferSize,
  OUT    VOID                              *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_FV_SET_INFO) (
  IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  IN CONST  EFI_GUID                       *InformationType,
  IN UINTN                                 BufferSize,
  IN CONST VOID                           *Buffer
  );


struct _EFI_FIRMWARE_VOLUME2_PROTOCOL {
  EFI_FV_GET_ATTRIBUTES    GetVolumeAttributes;
  EFI_FV_SET_ATTRIBUTES    SetVolumeAttributes;
  EFI_FV_READ_FILE         ReadFile;
  EFI_FV_READ_SECTION      ReadSection;
  EFI_FV_WRITE_FILE        WriteFile;
  EFI_FV_GET_NEXT_FILE     GetNextFile;
  UINT32                   KeySize;
  EFI_HANDLE               ParentHandle;
  EFI_FV_GET_INFO          GetInfo;
  EFI_FV_SET_INFO          SetInfo;
};

extern EFI_GUID gEfiFirmwareVolume2ProtocolGuid;

#endif
