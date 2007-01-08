/*++

Copyright (c)  1999 - 2006 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  GenBsfFixup.c

Abstract:

  Utility to Fixup the SEC component for IA32.  This is an 
  interim tool in place of the full GenBsfImage support for 
  IA32.
  This tool supports the Synch3 update

--*/

#include "BaseTypes.h"
#include "UefiBaseTypes.h"
#include "EfiImage.h"
#include "FirmwareVolumeHeader.h"
#include "FirmwareVolumeImageFormat.h"
#include "ParseInf.h"
#include "CommonLib.h"
#include "FirmwareFileSystem.h"
#include "FvLib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

//
// BugBug -- this port to the new FFS is really weird.
//           A lot of the file-header stuff has been ported, but
//           not the section information.
//

#define UTILITY_NAME "GenBsfFixup"
#define UTILITY_MAJOR_VERSION 0
#define UTILITY_MINOR_VERSION 1

UINT32  gFixup;

UINT32
GetOccupiedSize (
  IN UINT32  ActualSize,
  IN UINT32  Alignment
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  ActualSize  - GC_TODO: add argument description
  Alignment   - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  UINT32  OccupiedSize;

  OccupiedSize = ActualSize;
  while ((OccupiedSize & (Alignment - 1)) != 0) {
    OccupiedSize++;
  }

  return OccupiedSize;
}

static
void
Version (
  VOID
  )
{
  printf ("%s v%d.%d -EDK Utility to Fixup the SEC component for IA32.\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION);
  printf ("Copyright (c) 1999-2006 Intel Corporation. All rights reserved.\n");
}


VOID
Usage (
  VOID
  )
{
  Version();
  printf ("\nUsage: " UTILITY_NAME " FvVolumeImageFile AddressOfFvInMemory OffsetOfFixup OutputFileName \n");
}

int
ReadHeader (
  FILE    *In,
  UINT32  *FvSize
  )
/*++

Routine Description:

  Reads in Firmware Volume information from the volume header file.

Arguments:

  In: Firmware Volume header file to read from
  FvSize: Size of Firmware Volume

Returns:

  int: Number of bytes read

--*/
{
  EFI_FIRMWARE_VOLUME_HEADER  VolumeHeader;
  EFI_FV_BLOCK_MAP_ENTRY      BlockMap;
  INT32                       SigTemp[2];
  INT32                       Invert;
  INT32                       bytesread;
  UINT32                      size;

  size      = 0;
  Invert    = 0;
  bytesread = 0;

  if (In == NULL) {
    printf ("Error: Input file is NULL.\n");
    return -1;
  }

  fread (&VolumeHeader, sizeof (EFI_FIRMWARE_VOLUME_HEADER) - sizeof (EFI_FV_BLOCK_MAP_ENTRY), 1, In);
  bytesread   = sizeof (EFI_FIRMWARE_VOLUME_HEADER) - sizeof (EFI_FV_BLOCK_MAP_ENTRY);
  SigTemp[0]  = VolumeHeader.Signature;
  SigTemp[1]  = 0;

  if (VolumeHeader.Attributes & EFI_FVB_ERASE_POLARITY) {
    Invert = 1;
  }

  do {
    fread (&BlockMap, sizeof (EFI_FV_BLOCK_MAP_ENTRY), 1, In);
    bytesread += sizeof (EFI_FV_BLOCK_MAP_ENTRY);

    if (BlockMap.NumBlocks != 0) {
      size += BlockMap.NumBlocks * BlockMap.BlockLength;
    }

  } while (BlockMap.NumBlocks != 0);

  *FvSize = size;
  rewind (In);

  if (Invert == 1) {
    bytesread *= -1;
  }

  return bytesread;
}

UINT32
GetSectionLength (
  IN  UINT32  *Length
  )
/*++

Routine Description:

  Converts a UINT8[3] array to a UINT32

Arguments:

  Length        A pointer to a 3 byte array

Returns:

  UINT32:       The length.

--*/
{
  return *Length & 0x00FFFFFF;
}

int
Readfile (
  UINT8   *FvImage,
  int     bytes,
  int     Invert
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FvImage - GC_TODO: add argument description
  bytes   - GC_TODO: add argument description
  Invert  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  UINT32                    FileLength;
  UINT32                    OccupiedFileLength;
  EFI_FFS_FILE_HEADER       *FileHeader;
  UINT8                     FileState;
  UINT8                     Checksum;
  UINT8                     *Ptr;
  UINT32                    SectionLength;
  EFI_COMMON_SECTION_HEADER *SectionHeader;
  EFI_IMAGE_NT_HEADERS      *PeHeader;
  UINT32                    PeiCoreOffset;

  Ptr         = FvImage + bytes;

  FileHeader  = (EFI_FFS_FILE_HEADER *) Ptr;

  FileState   = GetFileState ((UINT8) Invert, FileHeader);

  switch (FileState) {
  case EFI_FILE_HEADER_CONSTRUCTION:
  case EFI_FILE_HEADER_INVALID:
    return sizeof (EFI_FFS_FILE_HEADER);

  case EFI_FILE_HEADER_VALID:
    Checksum  = CalculateSum8 ((UINT8 *) FileHeader, sizeof (EFI_FFS_FILE_HEADER));
    Checksum  = (UINT8) (Checksum - FileHeader->IntegrityCheck.Checksum.File);
    Checksum  = (UINT8) (Checksum - FileHeader->State);
    if (Checksum != 0) {
      return -1;
    }
    //
    // Now do the fixup stuff - begin
    //
    if (FileHeader->Type == EFI_FV_FILETYPE_PEI_CORE) {
      SectionHeader = (EFI_COMMON_SECTION_HEADER *) FileHeader + sizeof (EFI_FFS_FILE_HEADER);
      SectionLength = GetSectionLength ((UINT32 *) &SectionHeader->Size[0]);

      printf ("Section length is 0x%X\n", SectionLength);

      if (SectionHeader->Type == EFI_SECTION_PE32) {

        gFixup    = bytes + sizeof (EFI_FFS_FILE_HEADER) + sizeof (EFI_COMMON_SECTION_HEADER);

        PeHeader  = (EFI_IMAGE_NT_HEADERS *) Ptr + sizeof (EFI_FFS_FILE_HEADER) + sizeof (EFI_COMMON_SECTION_HEADER);

        if (((EFI_IMAGE_DOS_HEADER *) PeHeader)->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
          //
          // DOS image header is present, so read the PE header after the DOS image header
          //
          PeHeader = (EFI_IMAGE_NT_HEADERS *) ((UINTN) PeHeader + (UINTN) ((((EFI_IMAGE_DOS_HEADER *) PeHeader)->e_lfanew) & 0x0ffff));

        }

        PeiCoreOffset = (UINTN) ((UINTN) (PeHeader->OptionalHeader.AddressOfEntryPoint & 0x0ffffffff));

        gFixup += PeiCoreOffset;
      }
    }

    FileLength          = GetLength (FileHeader->Size);
    OccupiedFileLength  = GetOccupiedSize (FileLength, 8);
    return OccupiedFileLength;

  case EFI_FILE_DATA_VALID:
    //
    // Calculate header checksum
    //
    Checksum  = CalculateSum8 ((UINT8 *) FileHeader, sizeof (EFI_FFS_FILE_HEADER));
    Checksum  = (UINT8) (Checksum - FileHeader->IntegrityCheck.Checksum.File);
    Checksum  = (UINT8) (Checksum - FileHeader->State);
    if (Checksum != 0) {
      return -1;
    }
    //
    // Determine file length
    //
    FileLength          = GetLength (FileHeader->Size);
    OccupiedFileLength  = GetOccupiedSize (FileLength, 8);

    //
    // Determine if file checksum is valid or fixed
    //
    if (FileHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
      Checksum  = CalculateSum8 (Ptr, FileLength);
      Checksum  = (UINT8) (Checksum - FileHeader->State);
      if (Checksum != 0) {
        return -1;
      }
    } else {
      if (FileHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM) {
        return -1;
      }
    }
    break;

  case EFI_FILE_MARKED_FOR_UPDATE:
  case EFI_FILE_DELETED:
    //
    // Calculate header checksum
    //
    Checksum  = CalculateSum8 ((UINT8 *) FileHeader, sizeof (EFI_FFS_FILE_HEADER));
    Checksum  = (UINT8) (Checksum - FileHeader->IntegrityCheck.Checksum.File);
    Checksum  = (UINT8) (Checksum - FileHeader->State);
    if (Checksum != 0) {
      return -1;
    }
    //
    // Determine file length
    //
    FileLength          = GetLength (FileHeader->Size);
    OccupiedFileLength  = GetOccupiedSize (FileLength, 8);

    //
    // Determine if file checksum is valid or fixed
    //
    if (FileHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
      Checksum  = CalculateSum8 (Ptr, FileLength);
      Checksum  = (UINT8) (Checksum - FileHeader->State);
      if (Checksum != 0) {
        return -1;
      }
    } else {
      if (FileHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM) {
        return -1;
      }
    }

    return OccupiedFileLength;

  default:
    return sizeof (EFI_FFS_FILE_HEADER);
  }

  return OccupiedFileLength;
}

int
main (
  int argc,
  char*argv[]
  )
/*++

Routine Description:

  Runs GenBsfFixup

Arguments:

  argc: number of command line arguments

  arg[0] = This file name
  arg[1] = Firmware Volume Name
  arg[2] = Base Address to relocate
  arg[3] = Relative offset of the fixup to perform
  arg[4] = Output File Name

Returns:
    
  int: 0 code success, -1 code failure

--*/
{
  FILE        *In;
  FILE        *Out;
  int         ByteStart;
  int         Invert;
  int         Index;
  int         cnt;
  UINT8       *FvImage;
  int         ByteRead;
  UINT32      FvSize;
  UINT64      delta;
  UINT32      Idx;
  UINT64      FvOffset;
  EFI_STATUS  Status;

  Index   = 0;
  Invert  = 0;
 
  if (argc == 1) {
    Usage();
    return -1;
  }
  
  if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0) ||
      (strcmp(argv[1], "-?") == 0) || (strcmp(argv[1], "/?") == 0)) {
    Usage();
    return -1;
  }
  
  if ((strcmp(argv[1], "-V") == 0) || (strcmp(argv[1], "--version") == 0)) {
    Version();
    return -1;
  }
 
  if (argc != 5) {
    Usage();
    return -1;
  }

  In = fopen (argv[1], "rb");

  if (In == NULL) {
    printf ("Unable to open FV image file \"%s\"\n", argv[1]);
    return -1;
  }

  ByteStart = ReadHeader (In, &FvSize);

  if (ByteStart < 0) {
    Invert = 1;
    ByteStart *= -1;
  }

  FvImage = malloc (FvSize);
  if (FvImage == NULL) {
    printf ("Cannot allocate memory\n");
    fclose (In);
    return -1;
  }

  ByteRead = fread (FvImage, 1, FvSize, In);

  if ((unsigned int) ByteRead != FvSize) {
    printf ("Read File error\n");
    fclose (In);
    return -1;
  }

  cnt = 0;
  while ((unsigned int) ByteStart < FvSize && cnt != -1) {
    cnt = Readfile (FvImage, ByteStart, Invert);

    if (cnt != -1) {
      ByteStart += cnt;
    }

    if (cnt != sizeof (EFI_FFS_FILE_HEADER)) {
      Index++;
    }
  }

  if (cnt == -1) {
    printf ("Firmware Volume image corrupted\n");
    return -1;
  }

  fclose (In);

  Out = fopen (argv[4], "wb");

  if (Out == NULL) {
    printf ("Unable to open FV image file \"%s\"\n", argv[4]);
    return -1;
  }

  In = fopen (argv[1], "rb");

  if (In == NULL) {
    printf ("Unable to open FV image file \"%s\"\n", argv[1]);
    return -1;
  }

  if (gFixup != 0) {

    printf ("Fixup of 0x%X\n", gFixup);

    Status = AsciiStringToUint64 (argv[2], TRUE, &FvOffset);

    gFixup += (UINT32) FvOffset;

    ByteStart = ReadHeader (In, &FvSize);

    Readfile (FvImage, ByteStart, Invert);

    cnt     = 0;
    Status  = AsciiStringToUint64 (argv[3], TRUE, &delta);

    fclose (In);
    In = fopen (argv[1], "rb");

    if (In == NULL) {
      printf ("Unable to open FV image file \"%s\"\n", argv[1]);
      return -1;
    }

    for (Idx = 0; Idx < delta - FvOffset; Idx++) {
      fputc (fgetc (In), Out);
    }

    fwrite (&gFixup, sizeof (UINT32), 1, Out);
    fseek (In, sizeof (UINT32), SEEK_CUR);

    for (Idx = 0; Idx < FvSize - (delta - FvOffset) - sizeof (UINT32); Idx++) {
      fputc (fgetc (In), Out);
    }

    fclose (In);
    fclose (Out);
  } else {
    printf ("There was no fixup to perform\n");
  }

  free (FvImage);

  return 0;
}
