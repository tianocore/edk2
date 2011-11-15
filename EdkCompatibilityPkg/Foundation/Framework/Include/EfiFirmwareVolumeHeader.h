/*++

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiFirmwareVolumeHeader.h

Abstract:

  Defines data structure that is the volume header found at the beginning of
  all firmware volumes that are either memory mapped, or have an
  associated FirmwareVolumeBlock protocol.

--*/

#ifndef _EFI_FIRMWARE_VOLUME_HEADER_H_
#define _EFI_FIRMWARE_VOLUME_HEADER_H_


//
// Firmware Volume Block Attributes bit definitions
// They are the shared between Framework and PI1.0.
//
typedef UINT32  EFI_FVB_ATTRIBUTES;

#define EFI_FVB_READ_DISABLED_CAP   0x00000001
#define EFI_FVB_READ_ENABLED_CAP    0x00000002
#define EFI_FVB_READ_STATUS         0x00000004

#define EFI_FVB_WRITE_DISABLED_CAP  0x00000008
#define EFI_FVB_WRITE_ENABLED_CAP   0x00000010
#define EFI_FVB_WRITE_STATUS        0x00000020

#define EFI_FVB_LOCK_CAP            0x00000040
#define EFI_FVB_LOCK_STATUS         0x00000080

#define EFI_FVB_STICKY_WRITE        0x00000200
#define EFI_FVB_MEMORY_MAPPED       0x00000400
#define EFI_FVB_ERASE_POLARITY      0x00000800


#if (PI_SPECIFICATION_VERSION < 0x00010000)

#define EFI_FVB_ALIGNMENT_CAP       0x00008000
#define EFI_FVB_ALIGNMENT_2         0x00010000
#define EFI_FVB_ALIGNMENT_4         0x00020000
#define EFI_FVB_ALIGNMENT_8         0x00040000
#define EFI_FVB_ALIGNMENT_16        0x00080000
#define EFI_FVB_ALIGNMENT_32        0x00100000
#define EFI_FVB_ALIGNMENT_64        0x00200000
#define EFI_FVB_ALIGNMENT_128       0x00400000
#define EFI_FVB_ALIGNMENT_256       0x00800000
#define EFI_FVB_ALIGNMENT_512       0x01000000
#define EFI_FVB_ALIGNMENT_1K        0x02000000
#define EFI_FVB_ALIGNMENT_2K        0x04000000
#define EFI_FVB_ALIGNMENT_4K        0x08000000
#define EFI_FVB_ALIGNMENT_8K        0x10000000
#define EFI_FVB_ALIGNMENT_16K       0x20000000
#define EFI_FVB_ALIGNMENT_32K       0x40000000
#define EFI_FVB_ALIGNMENT_64K       0x80000000

#define EFI_FVB_CAPABILITIES  (EFI_FVB_READ_DISABLED_CAP | \
                              EFI_FVB_READ_ENABLED_CAP | \
                              EFI_FVB_WRITE_DISABLED_CAP | \
                              EFI_FVB_WRITE_ENABLED_CAP | \
                              EFI_FVB_LOCK_CAP \
                              )

#define EFI_FVB_STATUS    (EFI_FVB_READ_STATUS | EFI_FVB_WRITE_STATUS | EFI_FVB_LOCK_STATUS)



#else

#define EFI_FVB2_READ_LOCK_CAP        0x00001000
#define EFI_FVB2_READ_LOCK_STATUS     0x00002000

#define EFI_FVB2_WRITE_LOCK_CAP       0x00004000
#define EFI_FVB2_WRITE_LOCK_STATUS    0x00008000

#define EFI_FVB2_ALIGNMENT            0x001F0000
#define EFI_FVB2_ALIGNMENT_1          0x00000000
#define EFI_FVB2_ALIGNMENT_2          0x00010000
#define EFI_FVB2_ALIGNMENT_4          0x00020000
#define EFI_FVB2_ALIGNMENT_8          0x00030000
#define EFI_FVB2_ALIGNMENT_16         0x00040000
#define EFI_FVB2_ALIGNMENT_32         0x00050000
#define EFI_FVB2_ALIGNMENT_64         0x00060000
#define EFI_FVB2_ALIGNMENT_128        0x00070000
#define EFI_FVB2_ALIGNMENT_256        0x00080000
#define EFI_FVB2_ALIGNMENT_512        0x00090000
#define EFI_FVB2_ALIGNMENT_1K         0x000A0000
#define EFI_FVB2_ALIGNMENT_2K         0x000B0000
#define EFI_FVB2_ALIGNMENT_4K         0x000C0000
#define EFI_FVB2_ALIGNMENT_8K         0x000D0000
#define EFI_FVB2_ALIGNMENT_16K        0x000E0000
#define EFI_FVB2_ALIGNMENT_32K        0x000F0000
#define EFI_FVB2_ALIGNMENT_64K        0x00100000
#define EFI_FVB2_ALIGNMENT_128K       0x00110000
#define EFI_FVB2_ALIGNMENT_256K       0x00120000
#define EFI_FVB2_ALIGNMENT_512K       0x00130000
#define EFI_FVB2_ALIGNMENT_1M         0x00140000
#define EFI_FVB2_ALIGNMENT_2M         0x00150000
#define EFI_FVB2_ALIGNMENT_4M         0x00160000
#define EFI_FVB2_ALIGNMENT_8M         0x00170000
#define EFI_FVB2_ALIGNMENT_16M        0x00180000
#define EFI_FVB2_ALIGNMENT_32M        0x00190000
#define EFI_FVB2_ALIGNMENT_64M        0x001A0000
#define EFI_FVB2_ALIGNMENT_128M       0x001B0000
#define EFI_FVB2_ALIGNMENT_256M       0x001C0000
#define EFI_FVB2_ALIGNMENT_512M       0x001D0000
#define EFI_FVB2_ALIGNMENT_1G         0x001E0000
#define EFI_FVB2_ALIGNMENT_2G         0x001F0000

#define EFI_FVB_CAPABILITIES  (EFI_FVB_READ_DISABLED_CAP | \
                              EFI_FVB_READ_ENABLED_CAP   | \
                              EFI_FVB_WRITE_DISABLED_CAP | \
                              EFI_FVB_WRITE_ENABLED_CAP  | \
                              EFI_FVB_LOCK_CAP           | \
                              EFI_FVB2_READ_LOCK_CAP     | \
                              EFI_FVB2_WRITE_LOCK_CAP    )

#define EFI_FVB_STATUS    (EFI_FVB_READ_STATUS | EFI_FVB_WRITE_STATUS       | \
                           EFI_FVB_LOCK_STATUS | EFI_FVB2_READ_LOCK_STATUS  | \
                           EFI_FVB2_WRITE_LOCK_STATUS )



//
// Extended Header Header. Located via ExtHeaderOffset.
// The extended headers follow each other in memory and are 
// terminated by ExtHeaderType EFI_FV_EXT_TYPE_END
//
#define EFI_FV_EXT_TYPE_END   0x00
typedef struct {
  UINT16  ExtEntrySize;
  UINT16  ExtEntryType;
} EFI_FIRMWARE_VOLUME_EXT_ENTRY;

//
// GUID that maps OEM file types to GUIDs
//
#define EFI_FV_EXT_TYPE_OEM_TYPE  0x01
typedef struct {
  EFI_FIRMWARE_VOLUME_EXT_ENTRY   Hdr;
  UINT32                          TypeMask;
  EFI_GUID                        Types[1];
} EFI_FIRMWARE_VOLUME_EXT_HEADER_OEM_TYPE;

//
// Unique name of the FV
//
typedef struct {
  EFI_GUID          FvName;
  UINT32            ExtHeaderSize;
} EFI_FIRMWARE_VOLUME_EXT_HEADER;


#endif


//
// Firmware Volume Header Revision definition
//
#define EFI_FVH_REVISION  0x01
//
// PI1.0 define Firmware Volume Header Revision to 2
//
#define EFI_FVH_PI_REVISION  0x02

//
// Firmware Volume Header Signature definition
//
#define EFI_FVH_SIGNATURE EFI_SIGNATURE_32 ('_', 'F', 'V', 'H')

//
// Firmware Volume Header Block Map Entry definition
//
typedef struct {
  UINT32  NumBlocks;
  UINT32  BlockLength;
} EFI_FV_BLOCK_MAP_ENTRY;

//
// Firmware Volume Header definition
//
typedef struct {
  UINT8                   ZeroVector[16];
  EFI_GUID                FileSystemGuid;
  UINT64                  FvLength;
  UINT32                  Signature;
  EFI_FVB_ATTRIBUTES      Attributes;
  UINT16                  HeaderLength;
  UINT16                  Checksum;
#if (PI_SPECIFICATION_VERSION < 0x00010000)
  UINT8                   Reserved[3];
#else
  UINT16                  ExtHeaderOffset;
  UINT8                   Reserved[1];
#endif
  UINT8                   Revision;
  EFI_FV_BLOCK_MAP_ENTRY  FvBlockMap[1];
} EFI_FIRMWARE_VOLUME_HEADER;

#endif
