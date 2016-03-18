/** @file

  MBR Partition Entry and Table structure defintions.

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _MBR_H_
#define _MBR_H_

#include "CommonLib.h"

#pragma pack(1)

#define MAX_MBR_PARTITIONS          4

//
// MBR Partition Entry
//
typedef struct {
  UINT8  BootIndicator;
  UINT8  StartHead;
  UINT8  StartSector;
  UINT8  StartTrack;
  UINT8  OSType;
  UINT8  EndHead;
  UINT8  EndSector;
  UINT8  EndTrack;
  UINT32 StartingLBA;
  UINT32 SizeInLBA;
} MBR_PARTITION_RECORD;

//
// MBR Partition table
//
typedef struct {
  UINT8                 BootCode[440];
  UINT32                UniqueMbrSignature;
  UINT16                Unknown;
  MBR_PARTITION_RECORD  PartitionRecord[MAX_MBR_PARTITIONS];
  UINT16                Signature;
} MASTER_BOOT_RECORD;

#pragma pack()

#define MBR_SIGNATURE               0xAA55
#define EXTENDED_DOS_PARTITION      0x05
#define EXTENDED_WINDOWS_PARTITION  0x0F

#endif
