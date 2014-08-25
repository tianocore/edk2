/** @file

Abstract:
  Patch the BPB information in boot sector image file.
  Patch the MBR code in MBR image file.

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <stdio.h>
#include <string.h>
#include "fat.h"
#include "mbr.h"
#include "EfiUtilityMsgs.h"
#include "ParseInf.h"

#define DEBUG_WARN  0x1
#define DEBUG_ERROR 0x2

//
// Utility Name
//
#define UTILITY_NAME  "BootSectImage"

//
// Utility version information
//
#define UTILITY_MAJOR_VERSION 0
#define UTILITY_MINOR_VERSION 1

void
Version (
  void
  )
/*++

Routine Description:

  Displays the standard utility information to SDTOUT

Arguments:

  None

Returns:

  None

--*/
{
  printf ("%s Version %d.%d %s\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION, __BUILD_VERSION);
  printf ("Copyright (c) 1999-2014 Intel Corporation. All rights reserved.\n");
  printf ("\n  The BootSectImage tool prints information or patch destination file by source\n");
  printf ("  file for BIOS Parameter Block (BPB) or Master Boot Record (MBR).\n");
}

void
Usage (
  void
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:


Returns:

  GC_TODO: add return values

--*/
{
  Version();
  printf ("\nUsage: \n\
   BootSectImage\n\
     [-f, --force force patch even if the FAT type of SrcImage and DstImage mismatch]\n\
     [-m, --mbr process MBR instead of boot sector]\n\
     [-p, --parse parse SrcImageFile]\n\
     [-o, --output DstImage]\n\
     [-g, --patch patch DstImage using data from SrcImageFile]\n\
     [-v, --verbose]\n\
     [--version]\n\
     [-q, --quiet disable all messages except fatal errors]\n\
     [-d, --debug[#]\n\
     [-h, --help]\n\
     [SrcImageFile]\n");
}

int WriteToFile (
  void *BootSector, 
  char *FileName
  )
/*++
Routine Description:
  Write 512 bytes boot sector to file.

Arguments:
  BootSector - point to a buffer containing 512 bytes boot sector to write
  FileName   - file to write to

Return:
  int        - number of bytes wrote,
                 512 indicates write successful
                 0 indicates write failure
--*/
{
  FILE *FileHandle;
  int  result;

  FileHandle = fopen (LongFilePath (FileName), "r+b");
  if (FileHandle == NULL) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "Open file: %s", FileName);
    return 0;
  }
  fseek (FileHandle, 0, SEEK_SET);

  result = fwrite (BootSector, 1, 512, FileHandle);
  if (result != 512) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "Write file: %s", FileName);
    result = 0;
  }

  fclose (FileHandle);
  return result;
}

int ReadFromFile (
  void *BootSector, 
  char *FileName
  )
/*++
Routine Description:
  Read first 512 bytes from file.

Arguments:
  BootSector - point to a buffer receiving the first 512 bytes data from file
  FileName   - file to read from

Return:
  int        - number of bytes read,
                 512 indicates read successful
                 0 indicates read failure
--*/
{
  FILE *FileHandle;
  int  result;

  FileHandle = fopen (LongFilePath (FileName), "rb");
  if (FileHandle == NULL) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E0001: Error opening file: %s", FileName);
    return 0;
  }

  result = fread (BootSector, 1, 512, FileHandle);
  if (result != 512) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E0004: Error reading file: %s", FileName);
    result = 0;
  }

  fclose (FileHandle);
  return result;
}

char *
FatTypeToString (
  IN FAT_TYPE        FatType
  )
/*++
Routine Description:
  Convert enum type of FatType to string
--*/
{
  switch (FatType) {
  case FatTypeFat12:
    return "FAT12";
  case FatTypeFat16:
    return "FAT16";
  case FatTypeFat32:
    return "FAT32";
  default:
    break;
  }
  return "FAT Unknown";
}

FAT_TYPE
GetFatType (
  IN FAT_BPB_STRUCT  *FatBpb
  )
/*++
Routine Description:
  Determine the FAT type according to BIOS Paramater Block (BPB) data

Arguments:
  FatBpb - BIOS Parameter Block (BPB) data, 512 Bytes

Return:
  FatTypeUnknown - Cannot determine the FAT type
  FatTypeFat12   - FAT12
  FatTypeFat16   - FAT16
  FatTypeFat32   - FAT32
--*/
{
  FAT_TYPE FatType;
  UINTN    RootDirSectors;
  UINTN    FATSz;
  UINTN    TotSec;
  UINTN    DataSec;
  UINTN    CountOfClusters;
  CHAR8    FilSysType[9];

  FatType = FatTypeUnknown;

  //
  // Simple check
  //
  if (FatBpb->Fat12_16.Signature != FAT_BS_SIGNATURE) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT - Signature Invalid - %04x, expected: %04x",
        FatBpb->Fat12_16.Signature, FAT_BS_SIGNATURE);
    return FatTypeUnknown;
  }

  //
  // Check according to FAT spec
  //
  if ((FatBpb->Fat12_16.BS_jmpBoot[0] != FAT_BS_JMP1) &&
      (FatBpb->Fat12_16.BS_jmpBoot[0] != FAT_BS_JMP2)) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT - BS_jmpBoot - %02x, expected: %02x or %02x",
        FatBpb->Fat12_16.BS_jmpBoot[0], FAT_BS_JMP1, FAT_BS_JMP2);
    return FatTypeUnknown;
  }

  if ((FatBpb->Fat12_16.BPB_BytsPerSec != 512) &&
      (FatBpb->Fat12_16.BPB_BytsPerSec != 1024) &&
      (FatBpb->Fat12_16.BPB_BytsPerSec != 2048) &&
      (FatBpb->Fat12_16.BPB_BytsPerSec != 4096)) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT - BPB_BytsPerSec - %04x, expected: %04x, %04x, %04x, or %04x",
        FatBpb->Fat12_16.BPB_BytsPerSec, 512, 1024, 2048, 4096);
    return FatTypeUnknown;
  }
  if (FatBpb->Fat12_16.BPB_BytsPerSec != 512) {
    DebugMsg (NULL, 0, DEBUG_WARN, NULL, "ERROR: E3003: FAT - BPB_BytsPerSec - %04x, expected: %04x",
        FatBpb->Fat12_16.BPB_BytsPerSec, 512);
  }
  if ((FatBpb->Fat12_16.BPB_SecPerClus != 1) &&
      (FatBpb->Fat12_16.BPB_SecPerClus != 2) &&
      (FatBpb->Fat12_16.BPB_SecPerClus != 4) &&
      (FatBpb->Fat12_16.BPB_SecPerClus != 8) &&
      (FatBpb->Fat12_16.BPB_SecPerClus != 16) &&
      (FatBpb->Fat12_16.BPB_SecPerClus != 32) &&
      (FatBpb->Fat12_16.BPB_SecPerClus != 64) &&
      (FatBpb->Fat12_16.BPB_SecPerClus != 128)) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT - BPB_SecPerClus - %02x, expected: %02x, %02x, %02x, %02x, %02x, %02x, %02x, or %02x",
        FatBpb->Fat12_16.BPB_BytsPerSec, 1, 2, 4, 8, 16, 32, 64, 128);
    return FatTypeUnknown;
  }
  if (FatBpb->Fat12_16.BPB_BytsPerSec * FatBpb->Fat12_16.BPB_SecPerClus > 32 * 1024) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT - BPB_BytsPerSec * BPB_SecPerClus - %08x, expected: <= %08x",
        FatBpb->Fat12_16.BPB_BytsPerSec * FatBpb->Fat12_16.BPB_SecPerClus, 32 * 1024);
    return FatTypeUnknown;
  }
  if (FatBpb->Fat12_16.BPB_RsvdSecCnt == 0) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT - BPB_RsvdSecCnt - %04x, expected: Non-Zero Value",
        FatBpb->Fat12_16.BPB_RsvdSecCnt);
    return FatTypeUnknown;
  }
  if (FatBpb->Fat12_16.BPB_NumFATs != 2) {
    DebugMsg (NULL, 0, DEBUG_WARN, NULL, "ERROR: E3003: FAT - BPB_NumFATs - %02x, expected: %02x",
        FatBpb->Fat12_16.BPB_NumFATs, 2);
  }
  if ((FatBpb->Fat12_16.BPB_Media != 0xF0) &&
      (FatBpb->Fat12_16.BPB_Media != 0xF8) &&
      (FatBpb->Fat12_16.BPB_Media != 0xF9) &&
      (FatBpb->Fat12_16.BPB_Media != 0xFA) &&
      (FatBpb->Fat12_16.BPB_Media != 0xFB) &&
      (FatBpb->Fat12_16.BPB_Media != 0xFC) &&
      (FatBpb->Fat12_16.BPB_Media != 0xFD) &&
      (FatBpb->Fat12_16.BPB_Media != 0xFE) &&
      (FatBpb->Fat12_16.BPB_Media != 0xFF)) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT - BPB_Media - %02x, expected: %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, or %02x",
        FatBpb->Fat12_16.BPB_Media, 0xF0, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF);
    return FatTypeUnknown;
  }

  //
  // Algo in FAT spec
  //
  RootDirSectors = ((FatBpb->Fat12_16.BPB_RootEntCnt * sizeof(FAT_DIRECTORY_ENTRY)) +
                    (FatBpb->Fat12_16.BPB_BytsPerSec - 1)) /
                   FatBpb->Fat12_16.BPB_BytsPerSec;

  if (FatBpb->Fat12_16.BPB_FATSz16 != 0) {
    FATSz = FatBpb->Fat12_16.BPB_FATSz16;
  } else {
    FATSz = FatBpb->Fat32.BPB_FATSz32;
  }
  if (FATSz == 0) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT - BPB_FATSz16, BPB_FATSz32 - 0, expected: Non-Zero Value");
    return FatTypeUnknown;
  }

  if (FatBpb->Fat12_16.BPB_TotSec16 != 0) {
    TotSec = FatBpb->Fat12_16.BPB_TotSec16;
  } else {
    TotSec = FatBpb->Fat12_16.BPB_TotSec32;
  }
  if (TotSec == 0) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT - BPB_TotSec16, BPB_TotSec32 - 0, expected: Non-Zero Value");
    return FatTypeUnknown;
  }

  DataSec = TotSec - (
                      FatBpb->Fat12_16.BPB_RsvdSecCnt +
                      FatBpb->Fat12_16.BPB_NumFATs * FATSz +
                      RootDirSectors
                     );

  CountOfClusters = DataSec / FatBpb->Fat12_16.BPB_SecPerClus;

  if (CountOfClusters < FAT_MAX_FAT12_CLUSTER) {
    FatType = FatTypeFat12;
  } else if (CountOfClusters < FAT_MAX_FAT16_CLUSTER) {
    FatType = FatTypeFat16;
  } else {
    FatType = FatTypeFat32;
  }
  //
  // Check according to FAT spec
  //
  if (((FatType == FatTypeFat12) || (FatType == FatTypeFat16)) &&
       (FatBpb->Fat12_16.BPB_RsvdSecCnt != 1)) {
    DebugMsg (NULL, 0, DEBUG_WARN, NULL, "ERROR: E3003: FAT12_16 - BPB_RsvdSecCnt - %04x, expected: %04x",
        FatBpb->Fat12_16.BPB_RsvdSecCnt, 1);
  }
  if ((FatType == FatTypeFat32) &&
       (FatBpb->Fat12_16.BPB_RsvdSecCnt != 32)) {
    DebugMsg (NULL, 0, DEBUG_WARN, NULL, "ERROR: E3003: FAT32 - BPB_RsvdSecCnt - %04x, expected: %04x",
        FatBpb->Fat12_16.BPB_RsvdSecCnt, 32);
  }
  if ((FatType == FatTypeFat16) &&
      (FatBpb->Fat12_16.BPB_RootEntCnt != 512)) {
    printf ("WARNING: FAT16: BPB_RootEntCnt - %04x, expected - %04x\n",
        FatBpb->Fat12_16.BPB_RootEntCnt, 512);
  }
  if ((FatType == FatTypeFat32) &&
      (FatBpb->Fat12_16.BPB_RootEntCnt != 0)) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT32 - BPB_RootEntCnt - %04x, expected: %04x",
        FatBpb->Fat12_16.BPB_RootEntCnt, 0);
    return FatTypeUnknown;
  }
  if ((FatType == FatTypeFat32) &&
      (FatBpb->Fat12_16.BPB_TotSec16 != 0)) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT32 - BPB_TotSec16 - %04x, expected: %04x",
        FatBpb->Fat12_16.BPB_TotSec16, 0);
    return FatTypeUnknown;
  }
  if ((FatType == FatTypeFat32) &&
      (FatBpb->Fat12_16.BPB_FATSz16 != 0)) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT32 - BPB_FATSz16 - %04x, expected: %04x",
        FatBpb->Fat12_16.BPB_FATSz16, 0);
    return FatTypeUnknown;
  }
  if ((FatType == FatTypeFat32) &&
      (FatBpb->Fat12_16.BPB_TotSec32 == 0)) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT32 - BPB_TotSec32 - %04x, expected: Non-Zero",
        (unsigned) FatBpb->Fat12_16.BPB_TotSec32);
    return FatTypeUnknown;
  }
  if ((FatType == FatTypeFat32) &&
      (FatBpb->Fat32.BPB_FATSz32 == 0)) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT32 - BPB_FATSz32 - %08x, expected: Non-Zero",
        (unsigned) FatBpb->Fat32.BPB_FATSz32);
    return FatTypeUnknown;
  }
  if ((FatType == FatTypeFat32) &&
      (FatBpb->Fat32.BPB_FSVer != 0)) {
    DebugMsg (NULL, 0, DEBUG_WARN, NULL, "ERROR: E3003: FAT32 - BPB_FSVer - %08x, expected: %04x",
        FatBpb->Fat32.BPB_FSVer, 0);
  }
  if ((FatType == FatTypeFat32) &&
      (FatBpb->Fat32.BPB_RootClus != 2)) {
    DebugMsg (NULL, 0, DEBUG_WARN, NULL, "ERROR: E3003: FAT32 - BPB_RootClus - %08x, expected: %04x",
        (unsigned) FatBpb->Fat32.BPB_RootClus, 2);
  }
  if ((FatType == FatTypeFat32) &&
      (FatBpb->Fat32.BPB_FSInfo != 1)) {
    DebugMsg (NULL, 0, DEBUG_WARN, NULL, "ERROR: E3003: FAT32 - BPB_FSInfo - %08x, expected: %04x",
        FatBpb->Fat32.BPB_FSInfo, 1);
  }
  if ((FatType == FatTypeFat32) &&
      (FatBpb->Fat32.BPB_BkBootSec != 6)) {
    DebugMsg (NULL, 0, DEBUG_WARN, NULL, "ERROR: E3003: FAT32 - BPB_BkBootSec - %08x, expected: %04x",
        FatBpb->Fat32.BPB_BkBootSec, 6);
  }
  if ((FatType == FatTypeFat32) &&
      ((*(UINT32 *)FatBpb->Fat32.BPB_Reserved != 0) ||
       (*((UINT32 *)FatBpb->Fat32.BPB_Reserved + 1) != 0) ||
       (*((UINT32 *)FatBpb->Fat32.BPB_Reserved + 2) != 0))) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT32 - BPB_Reserved - %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x, expected: 0",
        FatBpb->Fat32.BPB_Reserved[0],
        FatBpb->Fat32.BPB_Reserved[1],
        FatBpb->Fat32.BPB_Reserved[2],
        FatBpb->Fat32.BPB_Reserved[3],
        FatBpb->Fat32.BPB_Reserved[4],
        FatBpb->Fat32.BPB_Reserved[5],
        FatBpb->Fat32.BPB_Reserved[6],
        FatBpb->Fat32.BPB_Reserved[7],
        FatBpb->Fat32.BPB_Reserved[8],
        FatBpb->Fat32.BPB_Reserved[9],
        FatBpb->Fat32.BPB_Reserved[10],
        FatBpb->Fat32.BPB_Reserved[11]);
    return FatTypeUnknown;
  }
  if (((FatType == FatTypeFat12) || (FatType == FatTypeFat16)) &&
       (FatBpb->Fat12_16.BS_Reserved1 != 0)) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT12_16 - BS_Reserved1 - %02x, expected: 0\n",
        FatBpb->Fat12_16.BS_Reserved1);
    return FatTypeUnknown;
  }
  if ((FatType == FatTypeFat32) &&
      (FatBpb->Fat32.BS_Reserved1 != 0)) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT32 - BS_Reserved1 - %02x, expected: 0\n",
        FatBpb->Fat32.BS_Reserved1);
    return FatTypeUnknown;
  }
  if (((FatType == FatTypeFat12) || (FatType == FatTypeFat16)) &&
       (FatBpb->Fat12_16.BS_BootSig != FAT_BS_BOOTSIG)) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT12_16 - BS_BootSig - %02x, expected: %02x\n",
        FatBpb->Fat12_16.BS_BootSig, FAT_BS_BOOTSIG);
    return FatTypeUnknown;
  }
  if ((FatType == FatTypeFat32) &&
      (FatBpb->Fat32.BS_BootSig != FAT_BS_BOOTSIG)) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3003: FAT32 - BS_BootSig - %02x, expected: %02x\n",
        FatBpb->Fat32.BS_BootSig, FAT_BS_BOOTSIG);
    return FatTypeUnknown;
  }
  
  if ((FatType == FatTypeFat12) || (FatType == FatTypeFat16)) {
    memcpy (FilSysType, FatBpb->Fat12_16.BS_FilSysType, 8);
    FilSysType[8] = 0;
    if ((FatType == FatTypeFat12) && 
        (strcmp (FilSysType, FAT12_FILSYSTYPE) != 0) &&
        (strcmp (FilSysType, FAT_FILSYSTYPE) != 0)) {
      DebugMsg (NULL, 0, DEBUG_WARN, NULL, "ERROR: E3003: FAT12 - BS_FilSysType - %s, expected: %s, or %s\n",
          FilSysType, FAT12_FILSYSTYPE, FAT_FILSYSTYPE);
    }
    if ((FatType == FatTypeFat16) && 
        (strcmp (FilSysType, FAT16_FILSYSTYPE) != 0) &&
        (strcmp (FilSysType, FAT_FILSYSTYPE) != 0)) {
      DebugMsg (NULL, 0, DEBUG_WARN, NULL, "ERROR: E3003: FAT16 - BS_FilSysType - %s, expected: %s, or %s\n",
          FilSysType, FAT16_FILSYSTYPE, FAT_FILSYSTYPE);
    }
  }
  if (FatType == FatTypeFat32) {
    memcpy (FilSysType, FatBpb->Fat32.BS_FilSysType, 8);
    FilSysType[8] = 0;
    if (strcmp (FilSysType, FAT32_FILSYSTYPE) != 0) {
      DebugMsg (NULL, 0, DEBUG_WARN, NULL, "ERROR: E3003: FAT32 - BS_FilSysType - %s, expected: %s\n",
          FilSysType, FAT32_FILSYSTYPE);
    }
  }

  //
  // pass all check, get FAT type
  //
  return FatType;
}


void
ParseBootSector (
  char *FileName
  )
{
  FAT_BPB_STRUCT  FatBpb;
  FAT_TYPE        FatType;
  
  if (ReadFromFile ((void *)&FatBpb, FileName) == 0) {
    return ;
  }
  
  FatType = GetFatType (&FatBpb);
  if (FatType <= FatTypeUnknown || FatType >= FatTypeMax) {
    printf ("ERROR: E3002: Unknown FAT Type!\n");
    return;
  }

  printf ("\nBoot Sector %s:\n", FatTypeToString (FatType));
  printf ("\n");
  printf ("  Offset Title                        Data\n");
  printf ("==================================================================\n");
  printf ("  0      JMP instruction              %02x %02x %02x\n",
                                                 FatBpb.Fat12_16.BS_jmpBoot[0],
                                                 FatBpb.Fat12_16.BS_jmpBoot[1],
                                                 FatBpb.Fat12_16.BS_jmpBoot[2]);
  printf ("  3      OEM                          %c%c%c%c%c%c%c%c\n",
                                                 FatBpb.Fat12_16.BS_OEMName[0],
                                                 FatBpb.Fat12_16.BS_OEMName[1],
                                                 FatBpb.Fat12_16.BS_OEMName[2],
                                                 FatBpb.Fat12_16.BS_OEMName[3],
                                                 FatBpb.Fat12_16.BS_OEMName[4],
                                                 FatBpb.Fat12_16.BS_OEMName[5],
                                                 FatBpb.Fat12_16.BS_OEMName[6],
                                                 FatBpb.Fat12_16.BS_OEMName[7]);
  printf ("\n");
  printf ("BIOS Parameter Block\n");
  printf ("  B      Bytes per sector             %04x\n", FatBpb.Fat12_16.BPB_BytsPerSec);
  printf ("  D      Sectors per cluster          %02x\n", FatBpb.Fat12_16.BPB_SecPerClus);
  printf ("  E      Reserved sectors             %04x\n", FatBpb.Fat12_16.BPB_RsvdSecCnt);
  printf ("  10     Number of FATs               %02x\n", FatBpb.Fat12_16.BPB_NumFATs);
  printf ("  11     Root entries                 %04x\n", FatBpb.Fat12_16.BPB_RootEntCnt);
  printf ("  13     Sectors (under 32MB)         %04x\n", FatBpb.Fat12_16.BPB_TotSec16);
  printf ("  15     Media descriptor             %02x\n", FatBpb.Fat12_16.BPB_Media);
  printf ("  16     Sectors per FAT (small vol.) %04x\n", FatBpb.Fat12_16.BPB_FATSz16);
  printf ("  18     Sectors per track            %04x\n", FatBpb.Fat12_16.BPB_SecPerTrk);
  printf ("  1A     Heads                        %04x\n", FatBpb.Fat12_16.BPB_NumHeads);
  printf ("  1C     Hidden sectors               %08x\n", (unsigned) FatBpb.Fat12_16.BPB_HiddSec);
  printf ("  20     Sectors (over 32MB)          %08x\n", (unsigned) FatBpb.Fat12_16.BPB_TotSec32);
  printf ("\n");
  if (FatType != FatTypeFat32) {
    printf ("  24     BIOS drive                   %02x\n", FatBpb.Fat12_16.BS_DrvNum);
    printf ("  25     (Unused)                     %02x\n", FatBpb.Fat12_16.BS_Reserved1);
    printf ("  26     Ext. boot signature          %02x\n", FatBpb.Fat12_16.BS_BootSig);
    printf ("  27     Volume serial number         %08x\n", (unsigned) FatBpb.Fat12_16.BS_VolID);
    printf ("  2B     Volume lable                 %c%c%c%c%c%c%c%c%c%c%c\n",
                                                   FatBpb.Fat12_16.BS_VolLab[0],
                                                   FatBpb.Fat12_16.BS_VolLab[1],
                                                   FatBpb.Fat12_16.BS_VolLab[2],
                                                   FatBpb.Fat12_16.BS_VolLab[3],
                                                   FatBpb.Fat12_16.BS_VolLab[4],
                                                   FatBpb.Fat12_16.BS_VolLab[5],
                                                   FatBpb.Fat12_16.BS_VolLab[6],
                                                   FatBpb.Fat12_16.BS_VolLab[7],
                                                   FatBpb.Fat12_16.BS_VolLab[8],
                                                   FatBpb.Fat12_16.BS_VolLab[9],
                                                   FatBpb.Fat12_16.BS_VolLab[10]);
    printf ("  36     File system                  %c%c%c%c%c%c%c%c\n",
                                                   FatBpb.Fat12_16.BS_FilSysType[0],
                                                   FatBpb.Fat12_16.BS_FilSysType[1],
                                                   FatBpb.Fat12_16.BS_FilSysType[2],
                                                   FatBpb.Fat12_16.BS_FilSysType[3],
                                                   FatBpb.Fat12_16.BS_FilSysType[4],
                                                   FatBpb.Fat12_16.BS_FilSysType[5],
                                                   FatBpb.Fat12_16.BS_FilSysType[6],
                                                   FatBpb.Fat12_16.BS_FilSysType[7]);
    printf ("\n");
  } else {
    printf ("FAT32 Section\n");
    printf ("  24     Sectors per FAT (large vol.) %08x\n", (unsigned) FatBpb.Fat32.BPB_FATSz32);
    printf ("  28     Flags                        %04x\n", FatBpb.Fat32.BPB_ExtFlags);
    printf ("  2A     Version                      %04x\n", FatBpb.Fat32.BPB_FSVer);
    printf ("  2C     Root dir 1st cluster         %08x\n", (unsigned) FatBpb.Fat32.BPB_RootClus);
    printf ("  30     FSInfo sector                %04x\n", FatBpb.Fat32.BPB_FSInfo);
    printf ("  32     Backup boot sector           %04x\n", FatBpb.Fat32.BPB_BkBootSec);
    printf ("  34     (Reserved)                   %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
                                                   FatBpb.Fat32.BPB_Reserved[0],
                                                   FatBpb.Fat32.BPB_Reserved[1],
                                                   FatBpb.Fat32.BPB_Reserved[2],
                                                   FatBpb.Fat32.BPB_Reserved[3],
                                                   FatBpb.Fat32.BPB_Reserved[4],
                                                   FatBpb.Fat32.BPB_Reserved[5],
                                                   FatBpb.Fat32.BPB_Reserved[6],
                                                   FatBpb.Fat32.BPB_Reserved[7],
                                                   FatBpb.Fat32.BPB_Reserved[8],
                                                   FatBpb.Fat32.BPB_Reserved[9],
                                                   FatBpb.Fat32.BPB_Reserved[10],
                                                   FatBpb.Fat32.BPB_Reserved[11]);
    printf ("\n");
    printf ("  40     BIOS drive                   %02x\n", FatBpb.Fat32.BS_DrvNum);
    printf ("  41     (Unused)                     %02x\n", FatBpb.Fat32.BS_Reserved1);
    printf ("  42     Ext. boot signature          %02x\n", FatBpb.Fat32.BS_BootSig);
    printf ("  43     Volume serial number         %08x\n", (unsigned) FatBpb.Fat32.BS_VolID);
    printf ("  47     Volume lable                 %c%c%c%c%c%c%c%c%c%c%c\n",
                                                   FatBpb.Fat32.BS_VolLab[0],
                                                   FatBpb.Fat32.BS_VolLab[1],
                                                   FatBpb.Fat32.BS_VolLab[2],
                                                   FatBpb.Fat32.BS_VolLab[3],
                                                   FatBpb.Fat32.BS_VolLab[4],
                                                   FatBpb.Fat32.BS_VolLab[5],
                                                   FatBpb.Fat32.BS_VolLab[6],
                                                   FatBpb.Fat32.BS_VolLab[7],
                                                   FatBpb.Fat32.BS_VolLab[8],
                                                   FatBpb.Fat32.BS_VolLab[9],
                                                   FatBpb.Fat32.BS_VolLab[10]);
    printf ("  52     File system                  %c%c%c%c%c%c%c%c\n",
                                                   FatBpb.Fat32.BS_FilSysType[0],
                                                   FatBpb.Fat32.BS_FilSysType[1],
                                                   FatBpb.Fat32.BS_FilSysType[2],
                                                   FatBpb.Fat32.BS_FilSysType[3],
                                                   FatBpb.Fat32.BS_FilSysType[4],
                                                   FatBpb.Fat32.BS_FilSysType[5],
                                                   FatBpb.Fat32.BS_FilSysType[6],
                                                   FatBpb.Fat32.BS_FilSysType[7]);
    printf ("\n");
  }
  printf ("  1FE    Signature                    %04x\n", FatBpb.Fat12_16.Signature);
  printf ("\n");

  
  return ;
}

void
PatchBootSector (
  char *DestFileName,
  char *SourceFileName,
  BOOLEAN ForcePatch
  )
/*++
Routine Description:
  Patch destination file according to the information from source file.
  Only patch BPB data but leave boot code un-touched.

Arguments:
  DestFileName   - Destination file to patch
  SourceFileName - Source file where patch from
--*/
{
  FAT_BPB_STRUCT  DestFatBpb;
  FAT_BPB_STRUCT  SourceFatBpb;
  FAT_TYPE        DestFatType;
  FAT_TYPE        SourceFatType;
  CHAR8           VolLab[11];
  CHAR8           FilSysType[8];
  
  if (ReadFromFile ((void *)&DestFatBpb, DestFileName) == 0) {
    return ;
  }
  if (ReadFromFile ((void *)&SourceFatBpb, SourceFileName) == 0) {
    return ;
  }
  
  DestFatType = GetFatType (&DestFatBpb);
  SourceFatType = GetFatType (&SourceFatBpb);

  if (DestFatType != SourceFatType) {
    //
    // FAT type mismatch
    //
    if (ForcePatch) {
      DebugMsg (NULL, 0, DEBUG_WARN, NULL, "ERROR: E3004: FAT type mismatch: Source - %s, Dest - %s", 
        FatTypeToString(SourceFatType), FatTypeToString(DestFatType));
    } else {
      DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3004: FAT type mismatch: Source - %s, Dest - %s", 
        FatTypeToString(SourceFatType), FatTypeToString(DestFatType));
      return ;
    }
  }

  if (SourceFatType <= FatTypeUnknown || SourceFatType >= FatTypeMax) {
    DebugMsg (NULL, 0, DEBUG_ERROR, NULL, "ERROR: E3002: Unknown FAT Type!\n");
    return;
  }

  //
  // Copy BPB/boot data (excluding BS_jmpBoot, BS_OEMName, BootCode and Signature) from SourceFatBpb to DestFatBpb
  //
  printf ("Patching %s BPB: ", FatTypeToString (SourceFatType));
  if (SourceFatType != FatTypeFat32) {
    memcpy (
      &DestFatBpb.Fat12_16.BPB_BytsPerSec,
      &SourceFatBpb.Fat12_16.BPB_BytsPerSec,
      ((UINTN)&DestFatBpb.Fat12_16.Reserved - (UINTN)&DestFatBpb.Fat12_16.BPB_BytsPerSec)
      );
  } else {
    memcpy (
      &DestFatBpb.Fat32.BPB_BytsPerSec,
      &SourceFatBpb.Fat32.BPB_BytsPerSec,
      ((UINTN)&DestFatBpb.Fat32.Reserved - (UINTN)&DestFatBpb.Fat32.BPB_BytsPerSec)
      );
  }

  //
  // Set BS_VolLab and BS_FilSysType of DestFatBpb
  //
  //        BS_VolLab     BS_FilSysType
  // FAT12: EFI FAT12     FAT12
  // FAT16: EFI FAT16     FAT16
  // FAT32: EFI FAT32     FAT32
  //
  if (SourceFatType == FatTypeFat32) {
    memcpy (VolLab, "EFI FAT32  ", sizeof(VolLab));
    memcpy (FilSysType, FAT32_FILSYSTYPE, sizeof(FilSysType));
  } else if (SourceFatType == FatTypeFat16) {
    memcpy (VolLab, "EFI FAT16  ", sizeof(VolLab));
    memcpy (FilSysType, FAT16_FILSYSTYPE, sizeof(FilSysType));
  } else {
    memcpy (VolLab, "EFI FAT12  ", sizeof(VolLab));
    memcpy (FilSysType, FAT12_FILSYSTYPE, sizeof(FilSysType));
  }
  if (SourceFatType != FatTypeFat32) {
    memcpy (DestFatBpb.Fat12_16.BS_VolLab, VolLab, sizeof(VolLab));
    memcpy (DestFatBpb.Fat12_16.BS_FilSysType, FilSysType, sizeof(FilSysType));
  } else {
    memcpy (DestFatBpb.Fat32.BS_VolLab, VolLab, sizeof(VolLab));
    memcpy (DestFatBpb.Fat32.BS_FilSysType, FilSysType, sizeof(FilSysType));
  }
  
  //
  // Set Signature of DestFatBpb to 55AA
  //
  DestFatBpb.Fat12_16.Signature = FAT_BS_SIGNATURE;

  //
  // Write DestFatBpb
  //
  if (WriteToFile ((void *)&DestFatBpb, DestFileName)) {
    printf ("successful!\n");
  } else {
    printf ("failed!\n");
  }

  return ;
}

void
ParseMbr (
  char *FileName
  )
{
  MASTER_BOOT_RECORD  Mbr;
  
  if (ReadFromFile ((void *)&Mbr, FileName) == 0) {
    return ;
  }
 
  printf ("\nMaster Boot Record:\n");
  printf ("\n");
  printf ("  Offset Title                        Value\n");
  printf ("==================================================================\n");
  printf ("  0      Master bootstrap loader code (not list)\n");
  printf ("  1B8    Windows disk signature       %08x\n", (unsigned) Mbr.UniqueMbrSignature);
  printf ("\n");
  printf ("Partition Table Entry #1\n");
  printf ("  1BE    80 = active partition        %02x\n", Mbr.PartitionRecord[0].BootIndicator);
  printf ("  1BF    Start head                   %02x\n", Mbr.PartitionRecord[0].StartHead);
  printf ("  1C0    Start sector                 %02x\n", Mbr.PartitionRecord[0].StartSector);
  printf ("  1C1    Start cylinder               %02x\n", Mbr.PartitionRecord[0].StartTrack);
  printf ("  1C2    Partition type indicator     %02x\n", Mbr.PartitionRecord[0].OSType);
  printf ("  1C3    End head                     %02x\n", Mbr.PartitionRecord[0].EndHead);
  printf ("  1C4    End sector                   %02x\n", Mbr.PartitionRecord[0].EndSector);
  printf ("  1C5    End cylinder                 %02x\n", Mbr.PartitionRecord[0].EndTrack);
  printf ("  1C6    Sectors preceding partition  %08x\n", (unsigned) Mbr.PartitionRecord[0].StartingLBA);
  printf ("  1CA    Sectors in partition         %08x\n", (unsigned) Mbr.PartitionRecord[0].SizeInLBA);
  printf ("\n");
  printf ("Partition Table Entry #2\n");
  printf ("  1CE    80 = active partition        %02x\n", Mbr.PartitionRecord[1].BootIndicator);
  printf ("  1CF    Start head                   %02x\n", Mbr.PartitionRecord[1].StartHead);
  printf ("  1D0    Start sector                 %02x\n", Mbr.PartitionRecord[1].StartSector);
  printf ("  1D1    Start cylinder               %02x\n", Mbr.PartitionRecord[1].StartTrack);
  printf ("  1D2    Partition type indicator     %02x\n", Mbr.PartitionRecord[1].OSType);
  printf ("  1D3    End head                     %02x\n", Mbr.PartitionRecord[1].EndHead);
  printf ("  1D4    End sector                   %02x\n", Mbr.PartitionRecord[1].EndSector);
  printf ("  1D5    End cylinder                 %02x\n", Mbr.PartitionRecord[1].EndTrack);
  printf ("  1D6    Sectors preceding partition  %08x\n", (unsigned) Mbr.PartitionRecord[1].StartingLBA);
  printf ("  1DA    Sectors in partition         %08x\n", (unsigned) Mbr.PartitionRecord[1].SizeInLBA);
  printf ("\n");
  printf ("Partition Table Entry #3\n");
  printf ("  1DE    80 = active partition        %02x\n", Mbr.PartitionRecord[2].BootIndicator);
  printf ("  1DF    Start head                   %02x\n", Mbr.PartitionRecord[2].StartHead);
  printf ("  1E0    Start sector                 %02x\n", Mbr.PartitionRecord[2].StartSector);
  printf ("  1E1    Start cylinder               %02x\n", Mbr.PartitionRecord[2].StartTrack);
  printf ("  1E2    Partition type indicator     %02x\n", Mbr.PartitionRecord[2].OSType);
  printf ("  1E3    End head                     %02x\n", Mbr.PartitionRecord[2].EndHead);
  printf ("  1E4    End sector                   %02x\n", Mbr.PartitionRecord[2].EndSector);
  printf ("  1E5    End cylinder                 %02x\n", Mbr.PartitionRecord[2].EndTrack);
  printf ("  1E6    Sectors preceding partition  %08x\n", (unsigned) Mbr.PartitionRecord[2].StartingLBA);
  printf ("  1EA    Sectors in partition         %08x\n", (unsigned) Mbr.PartitionRecord[2].SizeInLBA);
  printf ("\n");
  printf ("Partition Table Entry #4\n");
  printf ("  1EE    80 = active partition        %02x\n", Mbr.PartitionRecord[3].BootIndicator);
  printf ("  1EF    Start head                   %02x\n", Mbr.PartitionRecord[3].StartHead);
  printf ("  1F0    Start sector                 %02x\n", Mbr.PartitionRecord[3].StartSector);
  printf ("  1F1    Start cylinder               %02x\n", Mbr.PartitionRecord[3].StartTrack);
  printf ("  1F2    Partition type indicator     %02x\n", Mbr.PartitionRecord[3].OSType);
  printf ("  1F3    End head                     %02x\n", Mbr.PartitionRecord[3].EndHead);
  printf ("  1F4    End sector                   %02x\n", Mbr.PartitionRecord[3].EndSector);
  printf ("  1F5    End cylinder                 %02x\n", Mbr.PartitionRecord[3].EndTrack);
  printf ("  1F6    Sectors preceding partition  %08x\n", (unsigned) Mbr.PartitionRecord[3].StartingLBA);
  printf ("  1FA    Sectors in partition         %08x\n", (unsigned) Mbr.PartitionRecord[3].SizeInLBA);
  printf ("\n");
  printf ("  1FE    Signature                    %04x\n", Mbr.Signature);
  printf ("\n");

  return ;
}

void
PatchMbr (
  char *DestFileName,
  char *SourceFileName
  )
{
  MASTER_BOOT_RECORD  DestMbr;
  MASTER_BOOT_RECORD  SourceMbr;
  
  if (ReadFromFile ((void *)&DestMbr, DestFileName) == 0) {
    return ;
  }
  if (ReadFromFile ((void *)&SourceMbr, SourceFileName) == 0) {
    return ;
  }
  
  if (SourceMbr.Signature != MBR_SIGNATURE) {
    printf ("ERROR: E3000: Invalid MBR!\n");
    return;
  }

  printf ("Patching MBR:\n");
  memcpy (
    &DestMbr.PartitionRecord[0],
    &SourceMbr.PartitionRecord[0],
    sizeof(DestMbr.PartitionRecord)
    );

  DestMbr.Signature = MBR_SIGNATURE;


  if (WriteToFile ((void *)&DestMbr, DestFileName)) {
    printf ("\tsuccessful!\n");
  }

  return ;
}


int
main (
  int argc,
  char *argv[]
  )
{
  char *SrcImage;
  char *DstImage;
  BOOLEAN ForcePatch;    // -f
  BOOLEAN ProcessMbr;    // -m
  BOOLEAN DoParse;       // -p SrcImage or -g SrcImage DstImage
  BOOLEAN Verbose;       // -v
  UINT64  LogLevel;
  EFI_STATUS EfiStatus;

  SrcImage = DstImage = NULL;
  ForcePatch = FALSE;
  ProcessMbr = FALSE;
  DoParse    = TRUE;
  Verbose    = FALSE;

  SetUtilityName ("bootsectimage");

  argc--; argv++;

  if (argc == 0) {
    Usage ();
    return -1;
  }

  while (argc != 0) {
    if (strcmp (*argv, "-f") == 0 || strcmp (*argv, "--force") == 0) {
      ForcePatch = TRUE;
    } else if (strcmp (*argv, "-p") == 0 || strcmp (*argv, "--parse") == 0) {
      DoParse    = TRUE;
      argc--; argv++;
      if (argc < 1) {
        Usage ();
        return -1;
      }
      SrcImage   = *argv;
    } else if (strcmp (*argv, "-g") == 0 || strcmp (*argv, "--patch") == 0) {
      DoParse    = FALSE;
      argc--; argv++;
      if (argc < 2) {
        Usage ();
        return -1;
      }
      SrcImage   = *argv;
      argc--; argv++;
      DstImage   = *argv;
    } else if (strcmp (*argv, "-m") == 0 || strcmp (*argv, "--mbr") == 0) {
      ProcessMbr = TRUE;
    } else if (strcmp (*argv, "-v") == 0 || strcmp (*argv, "--verbose") == 0) {
      Verbose    = TRUE;
    } else if (strcmp (*argv, "--version") == 0) {
      Version();
      return 0;
    } else if ((stricmp (*argv, "-d") == 0) || (stricmp (*argv, "--debug") == 0)) {
      argc--; argv++;
      if (argc < 1) {
        Usage ();
        return -1;
      }
      EfiStatus = AsciiStringToUint64 (*argv, FALSE, &LogLevel);
      if (EFI_ERROR (EfiStatus)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", "--debug", *argv);
        return 1;
      }
      if (LogLevel > 9) {
        Error (NULL, 0, 1003, "Invalid option value", "Debug Level range is 0-9, currnt input level is %d", (int) LogLevel);
        return 1;
      }
      SetPrintLevel (LogLevel);
      DebugMsg (NULL, 0, 9, "Debug Mode Set", "Debug Output Mode Level %s is set!", *argv);
    } else {
      Usage ();
      return -1;
    }

    argc--; argv++;
  }

  if (ForcePatch && DoParse) {
    printf ("ERROR: E1002: Conflicting options: -f, -p. Cannot apply force(-f) to parse(-p)!\n");
    Usage ();
    return -1;
  }
  if (ForcePatch && !DoParse && ProcessMbr) {
    printf ("ERROR: E1002: Conflicting options: -f, -g -m. Cannot apply force(-f) to processing MBR (-g -m)!\n");
    Usage ();
    return -1;
  }

  if (Verbose) {
    SetPrintLevel (VERBOSE_LOG_LEVEL);
  } else {
    SetPrintLevel (KEY_LOG_LEVEL);
  }

  if (DoParse) {
    if (ProcessMbr) {
      ParseMbr (SrcImage);
    } else {
      ParseBootSector (SrcImage);
    }
  } else {
    if (ProcessMbr) {
      PatchMbr (DstImage, SrcImage);
    } else {
      PatchBootSector (DstImage, SrcImage, ForcePatch);
    }
  }

  return 0;
}

