/** @file

Copyright (c) 1999 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  VolInfo.c

Abstract:

  The tool dumps the contents of a firmware volume

**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <FvLib.h>
#include <Common/UefiBaseTypes.h>
#include <Common/UefiCapsule.h>
#include <Common/PiFirmwareFile.h>
#include <Common/PiFirmwareVolume.h>
#include <Guid/PiFirmwareFileSystem.h>
#include <IndustryStandard/PeImage.h>
#include <Protocol/GuidedSectionExtraction.h>

#include "Compress.h"
#include "Decompress.h"
#include "VolInfo.h"
#include "CommonLib.h"
#include "EfiUtilityMsgs.h"
#include "FirmwareVolumeBufferLib.h"
#include "OsPath.h"
#include "ParseGuidedSectionTools.h"
#include "StringFuncs.h"

//
// Utility global variables
//

EFI_GUID  gEfiCrc32GuidedSectionExtractionProtocolGuid = EFI_CRC32_GUIDED_SECTION_EXTRACTION_PROTOCOL_GUID;

#define UTILITY_MAJOR_VERSION      0
#define UTILITY_MINOR_VERSION      82

#define UTILITY_NAME         "VolInfo"

#define EFI_SECTION_ERROR EFIERR (100)

#define MAX_BASENAME_LEN  60  // not good to hardcode, but let's be reasonable

//
// Structure to keep a list of guid-to-basenames
//
typedef struct _GUID_TO_BASENAME {
  struct _GUID_TO_BASENAME  *Next;
  INT8                      Guid[PRINTED_GUID_BUFFER_SIZE];
  INT8                      BaseName[MAX_BASENAME_LEN];
} GUID_TO_BASENAME;

static GUID_TO_BASENAME *mGuidBaseNameList = NULL;

//
// Store GUIDed Section guid->tool mapping
//
EFI_HANDLE mParsedGuidedSectionTools = NULL;

CHAR8* mUtilityFilename = NULL;

EFI_STATUS
ParseGuidBaseNameFile (
  CHAR8    *FileName
  );

EFI_STATUS
FreeGuidBaseNameList (
  VOID
  );

EFI_STATUS
PrintGuidName (
  IN UINT8    *GuidStr
  );

EFI_STATUS
ParseSection (
  IN UINT8  *SectionBuffer,
  IN UINT32 BufferLength
  );

EFI_STATUS
DumpDepexSection (
  IN UINT8    *Ptr,
  IN UINT32   SectionLength
  );

STATIC
EFI_STATUS
ReadHeader (
  IN FILE       *InputFile,
  OUT UINT32    *FvSize,
  OUT BOOLEAN   *ErasePolarity
  );

STATIC
EFI_STATUS
PrintFileInfo (
  EFI_FIRMWARE_VOLUME_HEADER  *FvImage,
  EFI_FFS_FILE_HEADER         *FileHeader,
  BOOLEAN                     ErasePolarity
  );

static
EFI_STATUS
PrintFvInfo (
  IN VOID                         *Fv,
  IN BOOLEAN                      IsChildFv
  );

static
VOID
LoadGuidedSectionToolsTxt (
  IN CHAR8* FirmwareVolumeFilename
  );

void
Usage (
  VOID
  );

int
main (
  int       argc,
  char      *argv[]
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  argc  - GC_TODO: add argument description
  ]     - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  FILE                        *InputFile;
  int                         BytesRead;
  EFI_FIRMWARE_VOLUME_HEADER  *FvImage;
  UINT32                      FvSize;
  EFI_STATUS                  Status;
  int                         Offset;
  BOOLEAN                     ErasePolarity;

  SetUtilityName (UTILITY_NAME);
  //
  // Print utility header
  //
  printf ("%s Tiano Firmware Volume FFS image info.  Version %d.%d %s, %s\n",
    UTILITY_NAME,
    UTILITY_MAJOR_VERSION,
    UTILITY_MINOR_VERSION,
    __BUILD_VERSION,
    __DATE__
    );

  //
  // Save, and then skip filename arg
  //
  mUtilityFilename = argv[0];
  argc--;
  argv++;

  Offset = 0;

  //
  // If they specified -x xref guid/basename cross-reference files, process it.
  // This will print the basename beside each file guid. To use it, specify
  // -x xref_filename to processdsc, then use xref_filename as a parameter
  // here.
  //
  while (argc > 2) {
    if ((strcmp(argv[0], "-x") == 0) || (strcmp(argv[0], "--xref") == 0)) {
      ParseGuidBaseNameFile (argv[1]);
      printf("ParseGuidBaseNameFile: %s\n", argv[1]);
      argc -= 2;
      argv += 2;
    } else if (strcmp(argv[0], "--offset") == 0) {
      //
      // Hex or decimal?
      //
      if ((argv[1][0] == '0') && (tolower ((int)argv[1][1]) == 'x')) {
        if (sscanf (argv[1], "%x", &Offset) != 1) {
          Error (NULL, 0, 1003, "Invalid option value", "Offset = %s", argv[1]);
          return GetUtilityStatus ();
        }
      } else {
        if (sscanf (argv[1], "%d", &Offset) != 1) {
          Error (NULL, 0, 1003, "Invalid option value", "Offset = %s", argv[1]);
          return GetUtilityStatus ();
        }
        //
        // See if they said something like "64K"
        //
        if (tolower ((int)argv[1][strlen (argv[1]) - 1]) == 'k') {
          Offset *= 1024;
        }
      }

      argc -= 2;
      argv += 2;
    } else {
      Usage ();
      return -1;
    }
  }
  //
  // Check for proper number of arguments
  //
  if (argc != 1) {
    Usage ();
    return -1;
  }
  //
  // Look for help options
  //
  if ((strcmp(argv[0], "-h") == 0) || (strcmp(argv[0], "--help") == 0) || 
      (strcmp(argv[0], "-?") == 0) || (strcmp(argv[0], "/?") == 0)) {
    Usage();
    return STATUS_ERROR;
  }

  //
  // Open the file containing the FV
  //
  InputFile = fopen (argv[0], "rb");
  if (InputFile == NULL) {
    Error (NULL, 0, 0001, "Error opening the input file", argv[0]);
    return GetUtilityStatus ();
  }
  //
  // Skip over pad bytes if specified. This is used if they prepend 0xff
  // data to the FV image binary.
  //
  if (Offset != 0) {
    fseek (InputFile, Offset, SEEK_SET);
  }
  //
  // Determine size of FV
  //
  Status = ReadHeader (InputFile, &FvSize, &ErasePolarity);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 0003, "error parsing FV image", "%s Header is invalid", argv[0]);
    fclose (InputFile);
    return GetUtilityStatus ();
  }
  //
  // Allocate a buffer for the FV image
  //
  FvImage = malloc (FvSize);
  if (FvImage == NULL) {
    Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
    fclose (InputFile);
    return GetUtilityStatus ();
  }
  //
  // Seek to the start of the image, then read the entire FV to the buffer
  //
  fseek (InputFile, Offset, SEEK_SET);
  BytesRead = fread (FvImage, 1, FvSize, InputFile);
  fclose (InputFile);
  if ((unsigned int) BytesRead != FvSize) {
    Error (NULL, 0, 0004, "error reading FvImage from", argv[0]);
    free (FvImage);
    return GetUtilityStatus ();
  }

  LoadGuidedSectionToolsTxt (argv[0]);

  PrintFvInfo (FvImage, FALSE);

  //
  // Clean up
  //
  free (FvImage);
  FreeGuidBaseNameList ();
  return GetUtilityStatus ();
}


static
EFI_STATUS
PrintFvInfo (
  IN VOID                         *Fv,
  IN BOOLEAN                      IsChildFv
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Fv            - Firmware Volume to print information about
  IsChildFv     - Flag specifies whether the input FV is a child FV.

Returns:

  EFI_STATUS

--*/
{
  EFI_STATUS                  Status;
  UINTN                       NumberOfFiles;
  BOOLEAN                     ErasePolarity;
  UINTN                       FvSize;
  EFI_FFS_FILE_HEADER         *CurrentFile;
  UINTN                       Key;

  Status = FvBufGetSize (Fv, &FvSize);

  NumberOfFiles = 0;
  ErasePolarity =
    (((EFI_FIRMWARE_VOLUME_HEADER*)Fv)->Attributes & EFI_FVB2_ERASE_POLARITY) ?
      TRUE : FALSE;

  //
  // Get the first file
  //
  Key = 0;
  Status = FvBufFindNextFile (Fv, &Key, (VOID **) &CurrentFile);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 0003, "error parsing FV image", "cannot find the first file in the FV image");
    return GetUtilityStatus ();
  }
  //
  // Display information about files found
  //
  while (CurrentFile != NULL) {
    //
    // Increment the number of files counter
    //
    NumberOfFiles++;

    //
    // Display info about this file
    //
    Status = PrintFileInfo (Fv, CurrentFile, ErasePolarity);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 0003, "error parsing FV image", "failed to parse a file in the FV");
      return GetUtilityStatus ();
    }
    //
    // Get the next file
    //
    Status = FvBufFindNextFile (Fv, &Key, (VOID **) &CurrentFile);
    if (Status == EFI_NOT_FOUND) {
      CurrentFile = NULL;
    } else if (EFI_ERROR (Status)) {
      Error (NULL, 0, 0003, "error parsing FV image", "cannot find the next file in the FV image");
      return GetUtilityStatus ();
    }
  }

  if (IsChildFv) {
    printf ("There are a total of %d files in the child FV\n", (int) NumberOfFiles);
  } else {
    printf ("There are a total of %d files in this FV\n", (int) NumberOfFiles);
  }

  return EFI_SUCCESS;
}

UINT32
GetOccupiedSize (
  IN UINT32  ActualSize,
  IN UINT32  Alignment
  )
/*++

Routine Description:

  This function returns the next larger size that meets the alignment 
  requirement specified.

Arguments:

  ActualSize      The size.
  Alignment       The desired alignment.
    
Returns:
 
  EFI_SUCCESS             Function completed successfully.
  EFI_ABORTED             The function encountered an error.

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
CHAR8 *
SectionNameToStr (
  IN EFI_SECTION_TYPE   Type
  )
/*++

Routine Description:

  Converts EFI Section names to Strings

Arguments:

  Type  - The EFI Section type

Returns:

  CHAR8* - Pointer to the String containing the section name.

--*/
{
  CHAR8 *SectionStr;
  CHAR8 *SectionTypeStringTable[] = {
    //
    // 0X00
    //
    "EFI_SECTION_ALL",
    //
    // 0x01
    //
    "EFI_SECTION_COMPRESSION",
    //
    // 0x02
    //
    "EFI_SECTION_GUID_DEFINED",    
    //
    // 0x03
    //
    "Unknown section type - Reserved 0x03",
    //
    // 0x04
    //
    "Unknown section type - Reserved 0x04",
    //
    // 0x05
    //
    "Unknown section type - Reserved 0x05",
    //
    // 0x06
    //
    "Unknown section type - Reserved 0x06",
    //
    // 0x07
    //
    "Unknown section type - Reserved 0x07",
    //
    // 0x08
    //
    "Unknown section type - Reserved 0x08",
    //
    // 0x09
    //
    "Unknown section type - Reserved 0x09",
    //
    // 0x0A
    //
    "Unknown section type - Reserved 0x0A",
    //
    // 0x0B
    //
    "Unknown section type - Reserved 0x0B",
    //
    // 0x0C
    //
    "Unknown section type - Reserved 0x0C",
    //
    // 0x0D
    //
    "Unknown section type - Reserved 0x0D",
    //
    // 0x0E
    //
    "Unknown section type - Reserved 0x0E",
    //
    // 0x0F
    //
    "Unknown section type - Reserved 0x0E",
    //
    // 0x10
    //
    "EFI_SECTION_PE32",
    //
    // 0x11
    //
    "EFI_SECTION_PIC",
    //
    // 0x12
    //
    "EFI_SECTION_TE",    
    //
    // 0x13
    //
    "EFI_SECTION_DXE_DEPEX", 
    //
    // 0x14
    //
    "EFI_SECTION_VERSION",
    //
    // 0x15
    //
    "EFI_SECTION_USER_INTERFACE",
    //
    // 0x16
    //
    "EFI_SECTION_COMPATIBILITY16",
    //
    // 0x17
    //
    "EFI_SECTION_FIRMWARE_VOLUME_IMAGE ",
    //
    // 0x18
    //
    "EFI_SECTION_FREEFORM_SUBTYPE_GUID ",
    //
    // 0x19
    //
    "EFI_SECTION_RAW",
    //
    // 0x1A
    //
    "Unknown section type - 0x1A",
    //
    // 0x1B
    //
    "EFI_SECTION_PEI_DEPEX",
    //
    // 0x1C
    //
    "EFI_SECTION_SMM_DEPEX",
    //
    // 0x1C+
    //
    "Unknown section type - Reserved - beyond last defined section"
  };

  if (Type > EFI_SECTION_LAST_SECTION_TYPE) {
    Type = EFI_SECTION_LAST_SECTION_TYPE + 1;
  }

  SectionStr = malloc (100);
  if (SectionStr == NULL) {
    printf ("Error: Out of memory resources.\n");
    return SectionStr;
  }
  strcpy (SectionStr, SectionTypeStringTable[Type]);
  return SectionStr;
}

STATIC
EFI_STATUS
ReadHeader (
  IN FILE       *InputFile,
  OUT UINT32    *FvSize,
  OUT BOOLEAN   *ErasePolarity
  )
/*++

Routine Description:

  This function determines the size of the FV and the erase polarity.  The 
  erase polarity is the FALSE value for file state.

Arguments:

  InputFile       The file that contains the FV image.
  FvSize          The size of the FV.
  ErasePolarity   The FV erase polarity.
    
Returns:
 
  EFI_SUCCESS             Function completed successfully.
  EFI_INVALID_PARAMETER   A required parameter was NULL or is out of range.
  EFI_ABORTED             The function encountered an error.

--*/
{
  EFI_FIRMWARE_VOLUME_HEADER  VolumeHeader;
  EFI_FV_BLOCK_MAP_ENTRY      BlockMap;
  UINTN                       Signature[2];
  UINTN                       BytesRead;
  UINT32                      Size;

  BytesRead = 0;
  Size      = 0;
  //
  // Check input parameters
  //
  if (InputFile == NULL || FvSize == NULL || ErasePolarity == NULL) {
    Error (__FILE__, __LINE__, 0, "application error", "invalid parameter to function");
    return EFI_INVALID_PARAMETER;
  }
  //
  // Read the header
  //
  fread (&VolumeHeader, sizeof (EFI_FIRMWARE_VOLUME_HEADER) - sizeof (EFI_FV_BLOCK_MAP_ENTRY), 1, InputFile);
  BytesRead     = sizeof (EFI_FIRMWARE_VOLUME_HEADER) - sizeof (EFI_FV_BLOCK_MAP_ENTRY);
  Signature[0]  = VolumeHeader.Signature;
  Signature[1]  = 0;

  //
  // Print FV header information
  //
  printf ("Signature:        %s (%X)\n", (char *) Signature, (unsigned) VolumeHeader.Signature);
  printf ("Attributes:       %X\n", (unsigned) VolumeHeader.Attributes);

  if (VolumeHeader.Attributes & EFI_FVB2_READ_DISABLED_CAP) {
    printf ("       EFI_FVB2_READ_DISABLED_CAP\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_READ_ENABLED_CAP) {
    printf ("       EFI_FVB2_READ_ENABLED_CAP\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_READ_STATUS) {
    printf ("       EFI_FVB2_READ_STATUS\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_WRITE_DISABLED_CAP) {
    printf ("       EFI_FVB2_WRITE_DISABLED_CAP\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_WRITE_ENABLED_CAP) {
    printf ("       EFI_FVB2_WRITE_ENABLED_CAP\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_WRITE_STATUS) {
    printf ("       EFI_FVB2_WRITE_STATUS\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_LOCK_CAP) {
    printf ("       EFI_FVB2_LOCK_CAP\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_LOCK_STATUS) {
    printf ("       EFI_FVB2_LOCK_STATUS\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_STICKY_WRITE) {
    printf ("       EFI_FVB2_STICKY_WRITE\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_MEMORY_MAPPED) {
    printf ("       EFI_FVB2_MEMORY_MAPPED\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ERASE_POLARITY) {
    printf ("       EFI_FVB2_ERASE_POLARITY\n");
    *ErasePolarity = TRUE;
  }

#if (PI_SPECIFICATION_VERSION < 0x00010000)
  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT) {
    printf ("       EFI_FVB2_ALIGNMENT\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_2) {
    printf ("       EFI_FVB2_ALIGNMENT_2\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_4) {
    printf ("       EFI_FVB2_ALIGNMENT_4\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_8) {
    printf ("       EFI_FVB2_ALIGNMENT_8\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_16) {
    printf ("       EFI_FVB2_ALIGNMENT_16\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_32) {
    printf ("       EFI_FVB2_ALIGNMENT_32\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_64) {
    printf ("        EFI_FVB2_ALIGNMENT_64\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_128) {
    printf ("        EFI_FVB2_ALIGNMENT_128\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_256) {
    printf ("        EFI_FVB2_ALIGNMENT_256\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_512) {
    printf ("        EFI_FVB2_ALIGNMENT_512\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_1K) {
    printf ("        EFI_FVB2_ALIGNMENT_1K\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_2K) {
    printf ("        EFI_FVB2_ALIGNMENT_2K\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_4K) {
    printf ("        EFI_FVB2_ALIGNMENT_4K\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_8K) {
    printf ("        EFI_FVB2_ALIGNMENT_8K\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_16K) {
    printf ("        EFI_FVB2_ALIGNMENT_16K\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_32K) {
    printf ("        EFI_FVB2_ALIGNMENT_32K\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_64K) {
    printf ("        EFI_FVB2_ALIGNMENT_64K\n");
  }
  
#else

  if (VolumeHeader.Attributes & EFI_FVB2_READ_LOCK_CAP) {
    printf ("       EFI_FVB2_READ_LOCK_CAP\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_READ_LOCK_STATUS) {
    printf ("       EFI_FVB2_READ_LOCK_STATUS\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_WRITE_LOCK_CAP) {
    printf ("       EFI_FVB2_WRITE_LOCK_CAP\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_WRITE_LOCK_STATUS) {
    printf ("       EFI_FVB2_WRITE_LOCK_STATUS\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_1) {
    printf ("       EFI_FVB2_ALIGNMENT_1\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_2) {
    printf ("        EFI_FVB2_ALIGNMENT_2\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_4) {
    printf ("        EFI_FVB2_ALIGNMENT_4\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_8) {
    printf ("        EFI_FVB2_ALIGNMENT_8\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_16) {
    printf ("        EFI_FVB2_ALIGNMENT_16\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_32) {
    printf ("        EFI_FVB2_ALIGNMENT_32\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_64) {
    printf ("        EFI_FVB2_ALIGNMENT_64\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_128) {
    printf ("        EFI_FVB2_ALIGNMENT_128\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_256) {
    printf ("        EFI_FVB2_ALIGNMENT_256\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_512) {
    printf ("        EFI_FVB2_ALIGNMENT_512\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_1K) {
    printf ("        EFI_FVB2_ALIGNMENT_1K\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_2K) {
    printf ("        EFI_FVB2_ALIGNMENT_2K\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_4K) {
    printf ("        EFI_FVB2_ALIGNMENT_4K\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_8K) {
    printf ("        EFI_FVB2_ALIGNMENT_8K\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_16K) {
    printf ("        EFI_FVB2_ALIGNMENT_16K\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_32K) {
    printf ("        EFI_FVB2_ALIGNMENT_32K\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_64K) {
    printf ("        EFI_FVB2_ALIGNMENT_64K\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_128K) {
    printf ("        EFI_FVB2_ALIGNMENT_128K\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_256K) {
    printf ("        EFI_FVB2_ALIGNMENT_256K\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_512K) {
    printf ("        EFI_FVB2_ALIGNMENT_512K\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_1M) {
    printf ("        EFI_FVB2_ALIGNMENT_1M\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_2M) {
    printf ("        EFI_FVB2_ALIGNMENT_2M\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_4M) {
    printf ("        EFI_FVB2_ALIGNMENT_4M\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_8M) {
    printf ("        EFI_FVB2_ALIGNMENT_8M\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_16M) {
    printf ("        EFI_FVB2_ALIGNMENT_16M\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_32M) {
    printf ("        EFI_FVB2_ALIGNMENT_32M\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_64M) {
    printf ("        EFI_FVB2_ALIGNMENT_64M\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_128M) {
    printf ("        EFI_FVB2_ALIGNMENT_128M\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_64M) {
    printf ("        EFI_FVB2_ALIGNMENT_64M\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_128M) {
    printf ("        EFI_FVB2_ALIGNMENT_128M\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_256M) {
    printf ("        EFI_FVB2_ALIGNMENT_256M\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_512M) {
    printf ("        EFI_FVB2_ALIGNMENT_512M\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_1G) {
    printf ("        EFI_FVB2_ALIGNMENT_1G\n");
  }

  if (VolumeHeader.Attributes & EFI_FVB2_ALIGNMENT_2G) {
    printf ("        EFI_FVB2_ALIGNMENT_2G\n");
  }

#endif
  printf ("Header Length:         0x%08X\n", VolumeHeader.HeaderLength);
  printf ("File System ID:        ");
  PrintGuid (&VolumeHeader.FileSystemGuid);
  //
  // printf ("\n");
  //
  printf ("Revision:              0x%04X\n", VolumeHeader.Revision);

  do {
    fread (&BlockMap, sizeof (EFI_FV_BLOCK_MAP_ENTRY), 1, InputFile);
    BytesRead += sizeof (EFI_FV_BLOCK_MAP_ENTRY);

    if (BlockMap.NumBlocks != 0) {
      printf ("Number of Blocks:      0x%08X\n", (unsigned) BlockMap.NumBlocks);
      printf ("Block Length:          0x%08X\n", (unsigned) BlockMap.Length);
      Size += BlockMap.NumBlocks * BlockMap.Length;
    }

  } while (!(BlockMap.NumBlocks == 0 && BlockMap.Length == 0));

  if (BytesRead != VolumeHeader.HeaderLength) {
    printf ("ERROR: Header length not consistent with Block Maps!\n");
    return EFI_ABORTED;
  }

  if (VolumeHeader.FvLength != Size) {
    printf ("ERROR: Volume Size not consistant with Block Maps!\n");
    return EFI_ABORTED;
  }

  printf ("Total Volume Size:     0x%08X\n", (unsigned) Size);

  *FvSize = Size;

  //
  // rewind (InputFile);
  //
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
PrintFileInfo (
  EFI_FIRMWARE_VOLUME_HEADER  *FvImage,
  EFI_FFS_FILE_HEADER         *FileHeader,
  BOOLEAN                     ErasePolarity
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FvImage       - GC_TODO: add argument description
  FileHeader    - GC_TODO: add argument description
  ErasePolarity - GC_TODO: add argument description

Returns:

  EFI_SUCCESS - GC_TODO: Add description for return value
  EFI_ABORTED - GC_TODO: Add description for return value

--*/
{
  UINT32              FileLength;
  UINT8               FileState;
  UINT8               Checksum;
  EFI_FFS_FILE_HEADER BlankHeader;
  EFI_STATUS          Status;
  UINT8               GuidBuffer[PRINTED_GUID_BUFFER_SIZE];
#if (PI_SPECIFICATION_VERSION < 0x00010000) 
  UINT16              *Tail;
#endif
  //
  // Check if we have free space
  //
  if (ErasePolarity) {
    memset (&BlankHeader, -1, sizeof (EFI_FFS_FILE_HEADER));
  } else {
    memset (&BlankHeader, 0, sizeof (EFI_FFS_FILE_HEADER));
  }

  if (memcmp (&BlankHeader, FileHeader, sizeof (EFI_FFS_FILE_HEADER)) == 0) {
    return EFI_SUCCESS;
  }
  //
  // Print file information.
  //
  printf ("============================================================\n");

  printf ("File Name:        ");
  PrintGuidToBuffer (&FileHeader->Name, GuidBuffer, sizeof (GuidBuffer), TRUE);
  printf ("%s  ", GuidBuffer);
  PrintGuidName (GuidBuffer);
  printf ("\n");

  //
  //  PrintGuid (&FileHeader->Name);
  //  printf ("\n");
  //
  FileLength = GetLength (FileHeader->Size);
  printf ("File Offset:      0x%08X\n", (unsigned) ((UINTN) FileHeader - (UINTN) FvImage));
  printf ("File Length:      0x%08X\n", (unsigned) FileLength);
  printf ("File Attributes:  0x%02X\n", FileHeader->Attributes);
  printf ("File State:       0x%02X\n", FileHeader->State);

  //
  // Print file state
  //
  FileState = GetFileState (ErasePolarity, FileHeader);

  switch (FileState) {

  case EFI_FILE_HEADER_CONSTRUCTION:
    printf ("        EFI_FILE_HEADER_CONSTRUCTION\n");
    return EFI_SUCCESS;

  case EFI_FILE_HEADER_INVALID:
    printf ("        EFI_FILE_HEADER_INVALID\n");
    return EFI_SUCCESS;

  case EFI_FILE_HEADER_VALID:
    printf ("        EFI_FILE_HEADER_VALID\n");
    Checksum  = CalculateSum8 ((UINT8 *) FileHeader, sizeof (EFI_FFS_FILE_HEADER));
    Checksum  = (UINT8) (Checksum - FileHeader->IntegrityCheck.Checksum.File);
    Checksum  = (UINT8) (Checksum - FileHeader->State);
    if (Checksum != 0) {
      printf ("ERROR: Header checksum invalid.\n");
      return EFI_ABORTED;
    }

    return EFI_SUCCESS;

  case EFI_FILE_DELETED:
    printf ("        EFI_FILE_DELETED\n");

  case EFI_FILE_MARKED_FOR_UPDATE:
    printf ("        EFI_FILE_MARKED_FOR_UPDATE\n");

  case EFI_FILE_DATA_VALID:
    printf ("        EFI_FILE_DATA_VALID\n");

    //
    // Calculate header checksum
    //
    Checksum  = CalculateSum8 ((UINT8 *) FileHeader, sizeof (EFI_FFS_FILE_HEADER));
    Checksum  = (UINT8) (Checksum - FileHeader->IntegrityCheck.Checksum.File);
    Checksum  = (UINT8) (Checksum - FileHeader->State);
    if (Checksum != 0) {
      Error (NULL, 0, 0003, "error parsing FFS file", "FFS file with Guid %s has invalid header checksum", GuidBuffer);
      return EFI_ABORTED;
    }

    FileLength = GetLength (FileHeader->Size);

    if (FileHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
      //
      // Calculate file checksum
      //
      Checksum  = CalculateSum8 ((UINT8 *) (FileHeader + 1), FileLength - sizeof (EFI_FFS_FILE_HEADER));
      Checksum  = Checksum + FileHeader->IntegrityCheck.Checksum.File;
      if (Checksum != 0) {
        Error (NULL, 0, 0003, "error parsing FFS file", "FFS file with Guid %s has invalid file checksum", GuidBuffer);
        return EFI_ABORTED;
      }
    } else {
      if (FileHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM) {
        Error (NULL, 0, 0003, "error parsing FFS file", "FFS file with Guid %s has invalid header checksum -- not set to fixed value of 0xAA", GuidBuffer);
        return EFI_ABORTED;
      }
    }
#if (PI_SPECIFICATION_VERSION < 0x00010000)    
    //
    // Verify tail if present
    //
    if (FileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
      //
      // Verify tail is complement of integrity check field in the header.
      //
      Tail = (UINT16 *) ((UINTN) FileHeader + GetLength (FileHeader->Size) - sizeof (EFI_FFS_INTEGRITY_CHECK));
      if (FileHeader->IntegrityCheck.TailReference != (UINT16)~(*Tail)) {
        Error (NULL, 0, 0003, "error parsing FFS file", \
        "FFS file with Guid %s failed in the integrity check, tail is not the complement of the header field", GuidBuffer);
        return EFI_ABORTED;
      }
    }
 #endif   
    break;

  default:
    Error (NULL, 0, 0003, "error parsing FFS file", "FFS file with Guid %s has the invalid/unrecognized file state bits", GuidBuffer);
    return EFI_ABORTED;
  }

  printf ("File Type:        0x%02X  ", FileHeader->Type);

  switch (FileHeader->Type) {

  case EFI_FV_FILETYPE_RAW:
    printf ("EFI_FV_FILETYPE_RAW\n");
    break;

  case EFI_FV_FILETYPE_FREEFORM:
    printf ("EFI_FV_FILETYPE_FREEFORM\n");
    break;

  case EFI_FV_FILETYPE_SECURITY_CORE:
    printf ("EFI_FV_FILETYPE_SECURITY_CORE\n");
    break;

  case EFI_FV_FILETYPE_PEI_CORE:
    printf ("EFI_FV_FILETYPE_PEI_CORE\n");
    break;

  case EFI_FV_FILETYPE_DXE_CORE:
    printf ("EFI_FV_FILETYPE_DXE_CORE\n");
    break;

  case EFI_FV_FILETYPE_PEIM:
    printf ("EFI_FV_FILETYPE_PEIM\n");
    break;

  case EFI_FV_FILETYPE_DRIVER:
    printf ("EFI_FV_FILETYPE_DRIVER\n");
    break;

  case EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER:
    printf ("EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER\n");
    break;

  case EFI_FV_FILETYPE_APPLICATION:
    printf ("EFI_FV_FILETYPE_APPLICATION\n");
    break;

  case EFI_FV_FILETYPE_SMM:
    printf ("EFI_FV_FILETYPE_SMM\n");
    break;

  case EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE:
    printf ("EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE\n");
    break;

  case EFI_FV_FILETYPE_COMBINED_SMM_DXE:
    printf ("EFI_FV_FILETYPE_COMBINED_SMM_DXE\n");
    break;

  case EFI_FV_FILETYPE_SMM_CORE:
    printf ("EFI_FV_FILETYPE_SMM_CORE\n");
    break;

  case EFI_FV_FILETYPE_FFS_PAD:
    printf ("EFI_FV_FILETYPE_FFS_PAD\n");
    break;

  default:
    printf ("\nERROR: Unrecognized file type %X.\n", FileHeader->Type);
    return EFI_ABORTED;
    break;
  }

  switch (FileHeader->Type) {

  case EFI_FV_FILETYPE_ALL:
  case EFI_FV_FILETYPE_RAW:
  case EFI_FV_FILETYPE_FFS_PAD:
    break;

  default:
    //
    // All other files have sections
    //
    Status = ParseSection (
              (UINT8 *) ((UINTN) FileHeader + sizeof (EFI_FFS_FILE_HEADER)),
              GetLength (FileHeader->Size) - sizeof (EFI_FFS_FILE_HEADER)
              );
    if (EFI_ERROR (Status)) {
      //
      // printf ("ERROR: Parsing the FFS file.\n");
      //
      return EFI_ABORTED;
    }
    break;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
ParseSection (
  IN UINT8  *SectionBuffer,
  IN UINT32 BufferLength
  )
/*++

Routine Description:

  Parses EFI Sections

Arguments:

  SectionBuffer - Buffer containing the section to parse.
  BufferLength  - Length of SectionBuffer

Returns:

  EFI_SECTION_ERROR - Problem with section parsing.
                      (a) compression errors
                      (b) unrecognized section 
  EFI_UNSUPPORTED - Do not know how to parse the section.
  EFI_SUCCESS - Section successfully parsed.
  EFI_OUT_OF_RESOURCES - Memory allocation failed.

--*/
{
  EFI_SECTION_TYPE    Type;
  UINT8               *Ptr;
  UINT32              SectionLength;
  CHAR8               *SectionName;
  EFI_STATUS          Status;
  UINT32              ParsedLength;
  UINT8               *CompressedBuffer;
  UINT32              CompressedLength;
  UINT8               *UncompressedBuffer;
  UINT32              UncompressedLength;
  UINT8               *ToolOutputBuffer;
  UINT32              ToolOutputLength;
  UINT8               CompressionType;
  UINT32              DstSize;
  UINT32              ScratchSize;
  UINT8               *ScratchBuffer;
  DECOMPRESS_FUNCTION DecompressFunction;
  GETINFO_FUNCTION    GetInfoFunction;
  // CHAR16              *name;
  CHAR8               *ExtractionTool;
  CHAR8               *ToolInputFile;
  CHAR8               *ToolOutputFile;
  CHAR8               *SystemCommandFormatString;
  CHAR8               *SystemCommand;

  ParsedLength = 0;
  while (ParsedLength < BufferLength) {
    Ptr           = SectionBuffer + ParsedLength;

    SectionLength = GetLength (((EFI_COMMON_SECTION_HEADER *) Ptr)->Size);
    Type          = ((EFI_COMMON_SECTION_HEADER *) Ptr)->Type;

    //
    // This is sort of an odd check, but is necessary because FFS files are
    // padded to a QWORD boundary, meaning there is potentially a whole section
    // header worth of 0xFF bytes.
    //
    if (SectionLength == 0xffffff && Type == 0xff) {
      ParsedLength += 4;
      continue;
    }

    SectionName = SectionNameToStr (Type);
    printf ("------------------------------------------------------------\n");
    printf ("  Type:  %s\n  Size:  0x%08X\n", SectionName, (unsigned) SectionLength);
    free (SectionName);

    switch (Type) {
    case EFI_SECTION_RAW:
    case EFI_SECTION_PE32:
    case EFI_SECTION_PIC:
    case EFI_SECTION_TE:
      // default is no more information
      break;

    case EFI_SECTION_USER_INTERFACE:
      // name = &((EFI_USER_INTERFACE_SECTION *) Ptr)->FileNameString;
      // printf ("  String: %s\n", &name);
      break;

    case EFI_SECTION_FIRMWARE_VOLUME_IMAGE:
      Status = PrintFvInfo (((EFI_FIRMWARE_VOLUME_IMAGE_SECTION*)Ptr) + 1, TRUE);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 0003, "printing of FV section contents failed", NULL);
        return EFI_SECTION_ERROR;
      }
      break;

    case EFI_SECTION_COMPATIBILITY16:
    case EFI_SECTION_FREEFORM_SUBTYPE_GUID:
      //
      // Section does not contain any further header information.
      //
      break;

    case EFI_SECTION_PEI_DEPEX:
    case EFI_SECTION_DXE_DEPEX:
    case EFI_SECTION_SMM_DEPEX:
      DumpDepexSection (Ptr, SectionLength);
      break;

    case EFI_SECTION_VERSION:
      printf ("  Build Number:  0x%02X\n", ((EFI_VERSION_SECTION *) Ptr)->BuildNumber);
      printf ("  Version Strg:  %s\n", (char*) ((EFI_VERSION_SECTION *) Ptr)->VersionString);
      break;

    case EFI_SECTION_COMPRESSION:
      UncompressedBuffer  = NULL;
      CompressedLength    = SectionLength - sizeof (EFI_COMPRESSION_SECTION);
      UncompressedLength  = ((EFI_COMPRESSION_SECTION *) Ptr)->UncompressedLength;
      CompressionType     = ((EFI_COMPRESSION_SECTION *) Ptr)->CompressionType;
      printf ("  Uncompressed Length:  0x%08X\n", (unsigned) UncompressedLength);

      if (CompressionType == EFI_NOT_COMPRESSED) {
        printf ("  Compression Type:  EFI_NOT_COMPRESSED\n");
        if (CompressedLength != UncompressedLength) {
          Error (
            NULL,
            0,
            0,
            "file is not compressed, but the compressed length does not match the uncompressed length",
            NULL
            );
          return EFI_SECTION_ERROR;
        }

        UncompressedBuffer = Ptr + sizeof (EFI_COMPRESSION_SECTION);
      } else if (CompressionType == EFI_STANDARD_COMPRESSION) {
        GetInfoFunction     = EfiGetInfo;
        DecompressFunction  = EfiDecompress;
        printf ("  Compression Type:  EFI_STANDARD_COMPRESSION\n");

        CompressedBuffer  = Ptr + sizeof (EFI_COMPRESSION_SECTION);

        Status            = GetInfoFunction (CompressedBuffer, CompressedLength, &DstSize, &ScratchSize);
        if (EFI_ERROR (Status)) {
          Error (NULL, 0, 0003, "error getting compression info from compression section", NULL);
          return EFI_SECTION_ERROR;
        }

        if (DstSize != UncompressedLength) {
          Error (NULL, 0, 0003, "compression error in the compression section", NULL);
          return EFI_SECTION_ERROR;
        }

        ScratchBuffer       = malloc (ScratchSize);
        UncompressedBuffer  = malloc (UncompressedLength);
        if ((ScratchBuffer == NULL) || (UncompressedBuffer == NULL)) {
          return EFI_OUT_OF_RESOURCES;
        }
        Status = DecompressFunction (
                  CompressedBuffer,
                  CompressedLength,
                  UncompressedBuffer,
                  UncompressedLength,
                  ScratchBuffer,
                  ScratchSize
                  );
        free (ScratchBuffer);
        if (EFI_ERROR (Status)) {
          Error (NULL, 0, 0003, "decompress failed", NULL);
          free (UncompressedBuffer);
          return EFI_SECTION_ERROR;
        }
      } else {
        Error (NULL, 0, 0003, "unrecognized compression type", "type 0x%X", CompressionType);
        return EFI_SECTION_ERROR;
      }

      Status = ParseSection (UncompressedBuffer, UncompressedLength);

      if (CompressionType == EFI_STANDARD_COMPRESSION) {
        //
        // We need to deallocate Buffer
        //
        free (UncompressedBuffer);
      }

      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 0003, "failed to parse section", NULL);
        return EFI_SECTION_ERROR;
      }
      break;

    case EFI_SECTION_GUID_DEFINED:
      printf ("  SectionDefinitionGuid:  ");
      PrintGuid (&((EFI_GUID_DEFINED_SECTION *) Ptr)->SectionDefinitionGuid);
      printf ("\n");
      printf ("  DataOffset:             0x%04X\n", (unsigned) ((EFI_GUID_DEFINED_SECTION *) Ptr)->DataOffset);
      printf ("  Attributes:             0x%04X\n", (unsigned) ((EFI_GUID_DEFINED_SECTION *) Ptr)->Attributes);

      ExtractionTool =
        LookupGuidedSectionToolPath (
          mParsedGuidedSectionTools,
          &((EFI_GUID_DEFINED_SECTION *) Ptr)->SectionDefinitionGuid
          );

      if (ExtractionTool != NULL) {

        ToolInputFile = CloneString (tmpnam (NULL));
        ToolOutputFile = CloneString (tmpnam (NULL));

        //
        // Construction 'system' command string
        //
        SystemCommandFormatString = "%s -d -o %s %s";
        SystemCommand = malloc (
          strlen (SystemCommandFormatString) +
          strlen (ExtractionTool) +
          strlen (ToolInputFile) +
          strlen (ToolOutputFile) +
          1
          );
        sprintf (
          SystemCommand,
          SystemCommandFormatString,
          ExtractionTool,
          ToolOutputFile,
          ToolInputFile
          );
        free (ExtractionTool);

        Status =
          PutFileImage (
            ToolInputFile,
            (CHAR8*) SectionBuffer + ((EFI_GUID_DEFINED_SECTION *) Ptr)->DataOffset,
            BufferLength - ((EFI_GUID_DEFINED_SECTION *) Ptr)->DataOffset
            );

        system (SystemCommand);
        remove (ToolInputFile);
        free (ToolInputFile);

        Status =
          GetFileImage (
            ToolOutputFile,
            (CHAR8 **)&ToolOutputBuffer,
            &ToolOutputLength
            );
        remove (ToolOutputFile);
        free (ToolOutputFile);
        if (EFI_ERROR (Status)) {
          Error (NULL, 0, 0004, "unable to read decoded GUIDED section", NULL);
          return EFI_SECTION_ERROR;
        }

        Status = ParseSection (
                  ToolOutputBuffer,
                  ToolOutputLength
                  );
        if (EFI_ERROR (Status)) {
          Error (NULL, 0, 0003, "parse of decoded GUIDED section failed", NULL);
          return EFI_SECTION_ERROR;
        }

      //
      // Check for CRC32 sections which we can handle internally if needed.
      //
      } else if (!CompareGuid (
                   &((EFI_GUID_DEFINED_SECTION *) Ptr)->SectionDefinitionGuid,
                   &gEfiCrc32GuidedSectionExtractionProtocolGuid
                   )
          ) {
        //
        // CRC32 guided section
        //
        Status = ParseSection (
                  SectionBuffer + ((EFI_GUID_DEFINED_SECTION *) Ptr)->DataOffset,
                  BufferLength - ((EFI_GUID_DEFINED_SECTION *) Ptr)->DataOffset
                  );
        if (EFI_ERROR (Status)) {
          Error (NULL, 0, 0003, "parse of CRC32 GUIDED section failed", NULL);
          return EFI_SECTION_ERROR;
        }
      } else {
        //
        // We don't know how to parse it now.
        //
        Error (NULL, 0, 0003, "Error parsing section", \
        "EFI_SECTION_GUID_DEFINED cannot be parsed at this time. Tool to decode this section should have been defined in GuidedSectionTools.txt (built in the FV directory).");
        return EFI_UNSUPPORTED;
      }
      break;

    default:
      //
      // Unknown section, return error
      //
      Error (NULL, 0, 0003, "unrecognized section type found", "section type = 0x%X", Type);
      return EFI_SECTION_ERROR;
    }

    ParsedLength += SectionLength;
    //
    // We make then next section begin on a 4-byte boundary
    //
    ParsedLength = GetOccupiedSize (ParsedLength, 4);
  }

  if (ParsedLength < BufferLength) {
    Error (NULL, 0, 0003, "sections do not completely fill the sectioned buffer being parsed", NULL);
    return EFI_SECTION_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
DumpDepexSection (
  IN UINT8    *Ptr,
  IN UINT32   SectionLength
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Ptr           - GC_TODO: add argument description
  SectionLength - GC_TODO: add argument description

Returns:

  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  UINT8 GuidBuffer[PRINTED_GUID_BUFFER_SIZE];

  //
  // Need at least a section header + data
  //
  if (SectionLength <= sizeof (EFI_COMMON_SECTION_HEADER)) {
    return EFI_SUCCESS;
  }

  Ptr += sizeof (EFI_COMMON_SECTION_HEADER);
  SectionLength -= sizeof (EFI_COMMON_SECTION_HEADER);
  while (SectionLength > 0) {
    printf ("        ");
    switch (*Ptr) {
    case EFI_DEP_BEFORE:
      printf ("BEFORE\n");
      Ptr++;
      SectionLength--;
      break;

    case EFI_DEP_AFTER:
      printf ("AFTER\n");
      Ptr++;
      SectionLength--;
      break;

    case EFI_DEP_PUSH:
      printf ("PUSH\n        ");
      PrintGuidToBuffer ((EFI_GUID *) (Ptr + 1), GuidBuffer, sizeof (GuidBuffer), TRUE);
      printf ("%s  ", GuidBuffer);
      PrintGuidName (GuidBuffer);
      printf ("\n");
      //
      // PrintGuid ((EFI_GUID *)(Ptr + 1));
      //
      Ptr += 17;
      SectionLength -= 17;
      break;

    case EFI_DEP_AND:
      printf ("AND\n");
      Ptr++;
      SectionLength--;
      break;

    case EFI_DEP_OR:
      printf ("OR\n");
      Ptr++;
      SectionLength--;
      break;

    case EFI_DEP_NOT:
      printf ("NOT\n");
      Ptr++;
      SectionLength--;
      break;

    case EFI_DEP_TRUE:
      printf ("TRUE\n");
      Ptr++;
      SectionLength--;
      break;

    case EFI_DEP_FALSE:
      printf ("FALSE\n");
      Ptr++;
      SectionLength--;
      break;

    case EFI_DEP_END:
      printf ("END DEPEX\n");
      Ptr++;
      SectionLength--;
      break;

    case EFI_DEP_SOR:
      printf ("SOR\n");
      Ptr++;
      SectionLength--;
      break;

    default:
      printf ("Unrecognized byte in depex: 0x%X\n", *Ptr);
      return EFI_SUCCESS;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PrintGuidName (
  IN UINT8    *GuidStr
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  GuidStr - GC_TODO: add argument description

Returns:

  EFI_SUCCESS - GC_TODO: Add description for return value
  EFI_INVALID_PARAMETER - GC_TODO: Add description for return value

--*/
{
  GUID_TO_BASENAME  *GPtr;
  //
  // If we have a list of guid-to-basenames, then go through the list to
  // look for a guid string match. If found, print the basename to stdout,
  // otherwise return a failure.
  //
  GPtr = mGuidBaseNameList;
  while (GPtr != NULL) {
    if (_stricmp ((CHAR8*) GuidStr, (CHAR8*) GPtr->Guid) == 0) {
      printf ("%s", GPtr->BaseName);
      return EFI_SUCCESS;
    }

    GPtr = GPtr->Next;
  }

  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
ParseGuidBaseNameFile (
  CHAR8    *FileName
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FileName  - GC_TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_OUT_OF_RESOURCES - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  FILE              *Fptr;
  CHAR8             Line[MAX_LINE_LEN];
  GUID_TO_BASENAME  *GPtr;

  if ((Fptr = fopen (FileName, "r")) == NULL) {
    printf ("ERROR: Failed to open input cross-reference file '%s'\n", FileName);
    return EFI_DEVICE_ERROR;
  }

  while (fgets (Line, sizeof (Line), Fptr) != NULL) {
    //
    // Allocate space for another guid/basename element
    //
    GPtr = malloc (sizeof (GUID_TO_BASENAME));
    if (GPtr == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    memset ((char *) GPtr, 0, sizeof (GUID_TO_BASENAME));
    if (sscanf (Line, "%s %s", GPtr->Guid, GPtr->BaseName) == 2) {
      GPtr->Next        = mGuidBaseNameList;
      mGuidBaseNameList = GPtr;
    } else {
      //
      // Some sort of error. Just continue.
      //
      free (GPtr);
    }
  }

  fclose (Fptr);
  return EFI_SUCCESS;
}

EFI_STATUS
FreeGuidBaseNameList (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  GUID_TO_BASENAME  *Next;

  while (mGuidBaseNameList != NULL) {
    Next = mGuidBaseNameList->Next;
    free (mGuidBaseNameList);
    mGuidBaseNameList = Next;
  }

  return EFI_SUCCESS;
}


static
VOID
LoadGuidedSectionToolsTxt (
  IN CHAR8* FirmwareVolumeFilename
  )
{
  CHAR8* PeerFilename;
  CHAR8* Places[] = {
    NULL,
    //NULL,
    };
  UINTN Index;

  Places[0] = FirmwareVolumeFilename;
  //Places[1] = mUtilityFilename;

  mParsedGuidedSectionTools = NULL;

  for (Index = 0; Index < (sizeof(Places)/sizeof(Places[0])); Index++) {
    PeerFilename = OsPathPeerFilePath (Places[Index], "GuidedSectionTools.txt");
    //printf("Loading %s...\n", PeerFilename);
    if (OsPathExists (PeerFilename)) {
      mParsedGuidedSectionTools = ParseGuidedSectionToolsFile (PeerFilename);
    }
    free (PeerFilename);
    if (mParsedGuidedSectionTools != NULL) {
      return;
    }
  }
}


void
Usage (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  GC_TODO: add return values

--*/
{
  //
  // Summary usage
  //
  fprintf (stdout, "Usage: %s [options] <input_file>\n\n", UTILITY_NAME);

  //
  // Copyright declaration
  // 
  fprintf (stdout, "Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.\n\n");

  //
  // Details Option
  //
  fprintf (stdout, "Options:\n");
  fprintf (stdout, "  -x xref, --xref xref\n\
            Parse basename to file-guid cross reference file(s).\n");
  fprintf (stdout, "  --offset offset\n\
            Offset of file to start processing FV at.\n");
  fprintf (stdout, "  -h, --help\n\
            Show this help message and exit.\n");

}

