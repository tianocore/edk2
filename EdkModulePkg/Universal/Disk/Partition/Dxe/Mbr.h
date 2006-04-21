/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Mbr.h
  
Abstract:

  Data Structures required for detecting MBR Partitions
  
Revision History

--*/

#ifndef _MBR_H_
#define _MBR_H_

#pragma pack(1)

#define MBR_SIGNATURE               0xaa55
#define MIN_MBR_DEVICE_SIZE         0x80000
#define MBR_ERRATA_PAD              0x40000 // 128 MB
#define EXTENDED_DOS_PARTITION      0x05
#define EXTENDED_WINDOWS_PARTITION  0x0F
#define MAX_MBR_PARTITIONS          4

#define EFI_PARTITION               0xef
#define MBR_SIZE                    512

//
// MBR Partition Entry
//
typedef struct {
  UINT8 BootIndicator;
  UINT8 StartHead;
  UINT8 StartSector;
  UINT8 StartTrack;
  UINT8 OSIndicator;
  UINT8 EndHead;
  UINT8 EndSector;
  UINT8 EndTrack;
  UINT8 StartingLBA[4];
  UINT8 SizeInLBA[4];
} MBR_PARTITION_RECORD;

//
// MBR Partition table
//
typedef struct {
  UINT8                 BootStrapCode[440];
  UINT8                 UniqueMbrSignature[4];
  UINT8                 Unknown[2];
  MBR_PARTITION_RECORD  Partition[MAX_MBR_PARTITIONS];
  UINT16                Signature;
} MASTER_BOOT_RECORD;

#pragma pack()

#endif
