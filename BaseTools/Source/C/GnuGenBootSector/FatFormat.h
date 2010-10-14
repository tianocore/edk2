/** @file

  Fat file system structure and definition.

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

--*/

#ifndef _FAT_BPB_H_
#define _FAT_BPB_H_

#include "CommonLib.h"

#pragma pack(1)

typedef struct {
  //
  // Fat common field
  //
  UINT8              BS_jmpBoot[3];
  CHAR8              BS_OEMName[8];
  UINT16             BPB_BytsPerSec;
  UINT8              BPB_SecPerClus;
  UINT16             BPB_RsvdSecCnt;
  UINT8              BPB_NumFATs;
  UINT16             BPB_RootEntCnt;
  UINT16             BPB_TotSec16;
  UINT8              BPB_Media;
  UINT16             BPB_FATSz16;
  UINT16             BPB_SecPerTrk;
  UINT16             BPB_NumHeads;
  UINT32             BPB_HiddSec;
  UINT32             BPB_TotSec32;

  //
  // Fat12/16 specific field
  //
  UINT8              BS_DrvNum;
  UINT8              BS_Reserved1;
  UINT8              BS_BootSig;
  UINT32             BS_VolID;
  CHAR8              BS_VolLab[11];
  CHAR8              BS_FilSysType[8];

  //
  // Boot Code and Data
  //
  UINT8              Reserved[448];

  //
  // Fat common signature - 0xAA55
  //
  UINT16             Signature;
} FAT12_16_BPB_STRUCT;

typedef struct {
  //
  // Fat common field
  //
  UINT8              BS_jmpBoot[3];
  CHAR8              BS_OEMName[8];
  UINT16             BPB_BytsPerSec;
  UINT8              BPB_SecPerClus;
  UINT16             BPB_RsvdSecCnt;
  UINT8              BPB_NumFATs;
  UINT16             BPB_RootEntCnt;
  UINT16             BPB_TotSec16;
  UINT8              BPB_Media;
  UINT16             BPB_FATSz16;
  UINT16             BPB_SecPerTrk;
  UINT16             BPB_NumHeads;
  UINT32             BPB_HiddSec;
  UINT32             BPB_TotSec32;

  //
  // Fat32 specific field
  //
  UINT32             BPB_FATSz32;
  UINT16             BPB_ExtFlags;
  UINT16             BPB_FSVer;
  UINT32             BPB_RootClus;
  UINT16             BPB_FSInfo;
  UINT16             BPB_BkBootSec;
  UINT8              BPB_Reserved[12];
  UINT8              BS_DrvNum;
  UINT8              BS_Reserved1;
  UINT8              BS_BootSig;
  UINT32             BS_VolID;
  CHAR8              BS_VolLab[11];
  CHAR8              BS_FilSysType[8];

  //
  // Boot Code and Data
  //
  UINT8              Reserved[420];

  //
  // Fat common signature - 0xAA55
  //
  UINT16             Signature;
} FAT32_BPB_STRUCT;

typedef union {
  FAT12_16_BPB_STRUCT   Fat12_16;
  FAT32_BPB_STRUCT      Fat32;
} FAT_BPB_STRUCT;

typedef enum {
  FatTypeUnknown,
  FatTypeFat12,
  FatTypeFat16,
  FatTypeFat32,
  FatTypeMax
} FAT_TYPE;

typedef struct {
  CHAR8              DIR_Name[11];
  UINT8              DIR_Attr;
  UINT8              DIR_NTRes;
  UINT8              DIR_CrtTimeTenth;
  UINT16             DIR_CrtTime;
  UINT16             DIR_CrtDate;
  UINT16             DIR_LstAccDate;
  UINT16             DIR_FstClusHI;
  UINT16             DIR_WrtTime;
  UINT16             DIR_WrtDate;
  UINT16             DIR_FstClusLO;
  UINT32             DIR_FileSize;
} FAT_DIRECTORY_ENTRY;

#pragma pack()

#define FAT_MAX_FAT12_CLUSTER         0xFF5
#define FAT_MAX_FAT16_CLUSTER         0xFFF5

#define FAT_BS_SIGNATURE      0xAA55
#define FAT_BS_BOOTSIG        0x29
#define FAT_BS_JMP1           0xEB
#define FAT_BS_JMP2           0xE9
#define FAT_FILSYSTYPE        "FAT     "
#define FAT12_FILSYSTYPE      "FAT12   "
#define FAT16_FILSYSTYPE      "FAT16   "
#define FAT32_FILSYSTYPE      "FAT32   "

#endif
