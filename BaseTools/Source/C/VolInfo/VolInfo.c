/** @file
The tool dumps the contents of a firmware volume

Copyright (c) 1999 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#ifdef __GNUC__
#include <unistd.h>
#else
#include <direct.h>
#endif

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
#include "ParseInf.h"
#include "PeCoffLib.h"

//
// Utility global variables
//

EFI_GUID  gEfiCrc32GuidedSectionExtractionProtocolGuid = EFI_CRC32_GUIDED_SECTION_EXTRACTION_PROTOCOL_GUID;

#define UTILITY_MAJOR_VERSION      1
#define UTILITY_MINOR_VERSION      0

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

BOOLEAN EnableHash = FALSE;
CHAR8 *OpenSslPath = NULL;

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

EFI_STATUS
CombinePath (
  IN  CHAR8* DefaultPath,
  IN  CHAR8* AppendPath,
  OUT CHAR8* NewPath
);

void
Usage (
  VOID
  );

UINT32
UnicodeStrLen (
  IN CHAR16 *String
  )
  /*++

  Routine Description:

  Returns the length of a null-terminated unicode string.

  Arguments:

    String - The pointer to a null-terminated unicode string.

  Returns:

    N/A

  --*/
{
  UINT32  Length;

  for (Length = 0; *String != L'\0'; String++, Length++) {
    ;
  }
  return Length;
}

VOID
Unicode2AsciiString (
  IN  CHAR16 *Source,
  OUT CHAR8  *Destination
  )
  /*++

  Routine Description:

  Convert a null-terminated unicode string to a null-terminated ascii string.

  Arguments:

    Source      - The pointer to the null-terminated input unicode string.
    Destination - The pointer to the null-terminated output ascii string.

  Returns:

    N/A

  --*/
{
  while (*Source != '\0') {
    *(Destination++) = (CHAR8) *(Source++);
  }
  //
  // End the ascii with a NULL.
  //
  *Destination = '\0';
}

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
  UINT64                      LogLevel;
  CHAR8                       *OpenSslEnv;
  CHAR8                       *OpenSslCommand;

  SetUtilityName (UTILITY_NAME);
  //
  // Print utility header
  //
  printf ("%s Version %d.%d Build %s\n",
    UTILITY_NAME,
    UTILITY_MAJOR_VERSION,
    UTILITY_MINOR_VERSION,
    __BUILD_VERSION
    );

  if (argc == 1) {
    Usage ();
    return -1;
  }

  argc--;
  argv++;
  LogLevel = 0;
  Offset = 0;

  //
  // Look for help options
  //
  if ((strcmp(argv[0], "-h") == 0) || (strcmp(argv[0], "--help") == 0) ||
      (strcmp(argv[0], "-?") == 0) || (strcmp(argv[0], "/?") == 0)) {
    Usage();
    return  STATUS_SUCCESS;
  }
  //
  // Version has already be printed, so just return success
  //
  if (strcmp(argv[0], "--version") == 0) {
    return  STATUS_SUCCESS;
  }

  //
  // If they specified -x xref guid/basename cross-reference files, process it.
  // This will print the basename beside each file guid. To use it, specify
  // -x xref_filename to processdsc, then use xref_filename as a parameter
  // here.
  //
  while (argc > 0) {
    if ((strcmp(argv[0], "-x") == 0) || (strcmp(argv[0], "--xref") == 0)) {
      ParseGuidBaseNameFile (argv[1]);
      printf("ParseGuidBaseNameFile: %s\n", argv[1]);
      argc -= 2;
      argv += 2;
      continue;
    }
    if (strcmp(argv[0], "--offset") == 0) {
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
      continue;
    }
    if ((stricmp (argv[0], "--hash") == 0)) {
      if (EnableHash == TRUE) {
        //
        // --hash already given in the option, ignore this one
        //
        argc --;
        argv ++;
        continue;
      }
      EnableHash = TRUE;
      OpenSslCommand = "openssl";
      OpenSslEnv = getenv("OPENSSL_PATH");
      if (OpenSslEnv == NULL) {
        OpenSslPath = OpenSslCommand;
      } else {
        //
        // We add quotes to the Openssl Path in case it has space characters
        //
        OpenSslPath = malloc(2+strlen(OpenSslEnv)+strlen(OpenSslCommand)+1);
        if (OpenSslPath == NULL) {
          Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
          return GetUtilityStatus ();
        }
        CombinePath(OpenSslEnv, OpenSslCommand, OpenSslPath);
      }
      if (OpenSslPath == NULL){
        Error (NULL, 0, 3000, "Open SSL command not available.  Please verify PATH or set OPENSSL_PATH.", NULL);
        return GetUtilityStatus ();
      }
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-v") == 0) || (stricmp (argv[0], "--verbose") == 0)) {
      SetPrintLevel (VERBOSE_LOG_LEVEL);
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-q") == 0) || (stricmp (argv[0], "--quiet") == 0)) {
      SetPrintLevel (KEY_LOG_LEVEL);
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-d") == 0) || (stricmp (argv[0], "--debug") == 0)) {
      Status = AsciiStringToUint64 (argv[1], FALSE, &LogLevel);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        return -1;
      }
      if (LogLevel > 9) {
        Error (NULL, 0, 1003, "Invalid option value", "Debug Level range is 0-9, current input level is %d", (int) LogLevel);
        return -1;
      }
      SetPrintLevel (LogLevel);
      DebugMsg (NULL, 0, 9, "Debug Mode Set", "Debug Output Mode Level %s is set!", argv[1]);
      argc -= 2;
      argv += 2;
      continue;
    }

    mUtilityFilename = argv[0];
    argc --;
    argv ++;
  }

  //
  // Open the file containing the FV
  //
  if (mUtilityFilename == NULL) {
    Error (NULL, 0, 1001, "Missing option", "Input files are not specified");
    return GetUtilityStatus ();
  }
  InputFile = fopen (LongFilePath (mUtilityFilename), "rb");
  if (InputFile == NULL) {
    Error (NULL, 0, 0001, "Error opening the input file", mUtilityFilename);
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
    Error (NULL, 0, 0003, "error parsing FV image", "%s Header is invalid", mUtilityFilename);
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
    Error (NULL, 0, 0004, "error reading FvImage from", mUtilityFilename);
    free (FvImage);
    return GetUtilityStatus ();
  }

  LoadGuidedSectionToolsTxt (mUtilityFilename);

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
  size_t                      ReadSize;

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
  ReadSize = fread (&VolumeHeader, sizeof (EFI_FIRMWARE_VOLUME_HEADER) - sizeof (EFI_FV_BLOCK_MAP_ENTRY), 1, InputFile);
  if (ReadSize != 1) {
    return EFI_ABORTED;
  }
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
    ReadSize = fread (&BlockMap, sizeof (EFI_FV_BLOCK_MAP_ENTRY), 1, InputFile);
    if (ReadSize != 1) {
      return EFI_ABORTED;
    }
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
    printf ("ERROR: Volume Size not consistent with Block Maps!\n");
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
  EFI_FFS_FILE_HEADER2 BlankHeader;
  EFI_STATUS          Status;
  UINT8               GuidBuffer[PRINTED_GUID_BUFFER_SIZE];
  UINT32              HeaderSize;
#if (PI_SPECIFICATION_VERSION < 0x00010000)
  UINT16              *Tail;
#endif
  //
  // Check if we have free space
  //
  HeaderSize = FvBufGetFfsHeaderSize(FileHeader);
  if (ErasePolarity) {
    memset (&BlankHeader, -1, HeaderSize);
  } else {
    memset (&BlankHeader, 0, HeaderSize);
  }

  if (memcmp (&BlankHeader, FileHeader, HeaderSize) == 0) {
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
  FileLength = FvBufGetFfsFileSize (FileHeader);
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
    Checksum  = CalculateSum8 ((UINT8 *) FileHeader, HeaderSize);
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
    Checksum  = CalculateSum8 ((UINT8 *) FileHeader, HeaderSize);
    Checksum  = (UINT8) (Checksum - FileHeader->IntegrityCheck.Checksum.File);
    Checksum  = (UINT8) (Checksum - FileHeader->State);
    if (Checksum != 0) {
      Error (NULL, 0, 0003, "error parsing FFS file", "FFS file with Guid %s has invalid header checksum", GuidBuffer);
      return EFI_ABORTED;
    }

    FileLength = FvBufGetFfsFileSize (FileHeader);

    if (FileHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
      //
      // Calculate file checksum
      //
      Checksum  = CalculateSum8 ((UINT8 *)FileHeader + HeaderSize, FileLength - HeaderSize);
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

  case EFI_FV_FILETYPE_MM_STANDALONE:
    printf ("EFI_FV_FILETYPE_MM_STANDALONE\n");
    break;

  case EFI_FV_FILETYPE_MM_CORE_STANDALONE:
    printf ("EFI_FV_FILETYPE_MM_CORE_STANDALONE\n");
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
              (UINT8 *) ((UINTN) FileHeader + HeaderSize),
              FvBufGetFfsFileSize (FileHeader) - HeaderSize
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
RebaseImageRead (
  IN     VOID    *FileHandle,
  IN     UINTN   FileOffset,
  IN OUT UINT32  *ReadSize,
  OUT    VOID    *Buffer
  )
/*++

Routine Description:

  Support routine for the PE/COFF Loader that reads a buffer from a PE/COFF file

Arguments:

  FileHandle - The handle to the PE/COFF file

  FileOffset - The offset, in bytes, into the file to read

  ReadSize   - The number of bytes to read from the file starting at FileOffset

  Buffer     - A pointer to the buffer to read the data into.

Returns:

  EFI_SUCCESS - ReadSize bytes of data were read into Buffer from the PE/COFF file starting at FileOffset

--*/
{
  CHAR8   *Destination8;
  CHAR8   *Source8;
  UINT32  Length;

  Destination8  = Buffer;
  Source8       = (CHAR8 *) ((UINTN) FileHandle + FileOffset);
  Length        = *ReadSize;
  while (Length--) {
    *(Destination8++) = *(Source8++);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SetAddressToSectionHeader (
  IN     CHAR8   *FileName,
  IN OUT UINT8   *FileBuffer,
  IN     UINT64  NewPe32BaseAddress
  )
/*++

Routine Description:

  Set new base address into the section header of PeImage

Arguments:

  FileName           - Name of file
  FileBuffer         - Pointer to PeImage.
  NewPe32BaseAddress - New Base Address for PE image.

Returns:

  EFI_SUCCESS          Set new base address into this image successfully.

--*/
{
  EFI_STATUS                            Status;
  PE_COFF_LOADER_IMAGE_CONTEXT          ImageContext;
  UINTN                                 Index;
  EFI_IMAGE_OPTIONAL_HEADER_UNION       *ImgHdr;
  EFI_IMAGE_SECTION_HEADER              *SectionHeader;

  //
  // Initialize context
  //
  memset (&ImageContext, 0, sizeof (ImageContext));
  ImageContext.Handle     = (VOID *) FileBuffer;
  ImageContext.ImageRead  = (PE_COFF_LOADER_READ_FILE) RebaseImageRead;
  Status                  = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 3000, "Invalid", "The input PeImage %s is not valid", FileName);
    return Status;
  }

  if (ImageContext.RelocationsStripped) {
    Error (NULL, 0, 3000, "Invalid", "The input PeImage %s has no relocation to be fixed up", FileName);
    return Status;
  }

  //
  // Get PeHeader pointer
  //
  ImgHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)(FileBuffer + ImageContext.PeCoffHeaderOffset);

  //
  // Get section header list
  //
  SectionHeader = (EFI_IMAGE_SECTION_HEADER *) (
    (UINTN) ImgHdr +
    sizeof (UINT32) +
    sizeof (EFI_IMAGE_FILE_HEADER) +
    ImgHdr->Pe32.FileHeader.SizeOfOptionalHeader
    );

  //
  // Set base address into the first section header that doesn't point to code section.
  //
  for (Index = 0; Index < ImgHdr->Pe32.FileHeader.NumberOfSections; Index ++, SectionHeader ++) {
    if ((SectionHeader->Characteristics & EFI_IMAGE_SCN_CNT_CODE) == 0) {
      *(UINT64 *) &SectionHeader->PointerToRelocations = NewPe32BaseAddress;
      break;
    }
  }

  //
  // BaseAddress is set to section header.
  //
  return EFI_SUCCESS;
}

EFI_STATUS
RebaseImage (
  IN     CHAR8   *FileName,
  IN OUT UINT8   *FileBuffer,
  IN     UINT64  NewPe32BaseAddress
  )
/*++

Routine Description:

  Set new base address into PeImage, and fix up PeImage based on new address.

Arguments:

  FileName           - Name of file
  FileBuffer         - Pointer to PeImage.
  NewPe32BaseAddress - New Base Address for PE image.

Returns:

  EFI_INVALID_PARAMETER   - BaseAddress is not valid.
  EFI_SUCCESS             - Update PeImage is correctly.

--*/
{
  EFI_STATUS                            Status;
  PE_COFF_LOADER_IMAGE_CONTEXT          ImageContext;
  UINTN                                 Index;
  EFI_IMAGE_OPTIONAL_HEADER_UNION       *ImgHdr;
  UINT8                                 *MemoryImagePointer;
  EFI_IMAGE_SECTION_HEADER              *SectionHeader;

  //
  // Initialize context
  //
  memset (&ImageContext, 0, sizeof (ImageContext));
  ImageContext.Handle     = (VOID *) FileBuffer;
  ImageContext.ImageRead  = (PE_COFF_LOADER_READ_FILE) RebaseImageRead;
  Status                  = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 3000, "Invalid", "The input PeImage %s is not valid", FileName);
    return Status;
  }

  if (ImageContext.RelocationsStripped) {
    Error (NULL, 0, 3000, "Invalid", "The input PeImage %s has no relocation to be fixed up", FileName);
    return Status;
  }

  //
  // Get PeHeader pointer
  //
  ImgHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)(FileBuffer + ImageContext.PeCoffHeaderOffset);

  //
  // Load and Relocate Image Data
  //
  MemoryImagePointer = (UINT8 *) malloc ((UINTN) ImageContext.ImageSize + ImageContext.SectionAlignment);
  if (MemoryImagePointer == NULL) {
    Error (NULL, 0, 4001, "Resource", "memory cannot be allocated on rebase of %s", FileName);
    return EFI_OUT_OF_RESOURCES;
  }
  memset ((VOID *) MemoryImagePointer, 0, (UINTN) ImageContext.ImageSize + ImageContext.SectionAlignment);
  ImageContext.ImageAddress = ((UINTN) MemoryImagePointer + ImageContext.SectionAlignment - 1) & (~((INT64)ImageContext.SectionAlignment - 1));

  Status =  PeCoffLoaderLoadImage (&ImageContext);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 3000, "Invalid", "LocateImage() call failed on rebase of %s", FileName);
    free ((VOID *) MemoryImagePointer);
    return Status;
  }

  ImageContext.DestinationAddress = NewPe32BaseAddress;
  Status                          = PeCoffLoaderRelocateImage (&ImageContext);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 3000, "Invalid", "RelocateImage() call failed on rebase of %s", FileName);
    free ((VOID *) MemoryImagePointer);
    return Status;
  }

  //
  // Copy Relocated data to raw image file.
  //
  SectionHeader = (EFI_IMAGE_SECTION_HEADER *) (
    (UINTN) ImgHdr +
    sizeof (UINT32) +
    sizeof (EFI_IMAGE_FILE_HEADER) +
    ImgHdr->Pe32.FileHeader.SizeOfOptionalHeader
    );

  for (Index = 0; Index < ImgHdr->Pe32.FileHeader.NumberOfSections; Index ++, SectionHeader ++) {
    CopyMem (
      FileBuffer + SectionHeader->PointerToRawData,
      (VOID*) (UINTN) (ImageContext.ImageAddress + SectionHeader->VirtualAddress),
      SectionHeader->SizeOfRawData
      );
  }

  free ((VOID *) MemoryImagePointer);

  //
  // Update Image Base Address
  //
  if (ImgHdr->Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    ImgHdr->Pe32.OptionalHeader.ImageBase = (UINT32) NewPe32BaseAddress;
  } else if (ImgHdr->Pe32Plus.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    ImgHdr->Pe32Plus.OptionalHeader.ImageBase = NewPe32BaseAddress;
  } else {
    Error (NULL, 0, 3000, "Invalid", "unknown PE magic signature %X in PE32 image %s",
      ImgHdr->Pe32.OptionalHeader.Magic,
      FileName
      );
    return EFI_ABORTED;
  }

  //
  // Set new base address into section header
  //
  Status = SetAddressToSectionHeader (FileName, FileBuffer, NewPe32BaseAddress);

  return Status;
}

EFI_STATUS
CombinePath (
  IN  CHAR8* DefaultPath,
  IN  CHAR8* AppendPath,
  OUT CHAR8* NewPath
)
{
  UINT32 DefaultPathLen;
  UINT64 Index;
  CHAR8  QuotesStr[] = "\"";
  strcpy(NewPath, QuotesStr);
  DefaultPathLen = strlen(DefaultPath);
  strcat(NewPath, DefaultPath);
  Index = 0;
  for (; Index < DefaultPathLen + 1; Index ++) {
    if (NewPath[Index] == '\\' || NewPath[Index] == '/') {
      if (NewPath[Index + 1] != '\0') {
        NewPath[Index] = '/';
      }
    }
  }
  if (NewPath[Index -1] != '/') {
    NewPath[Index] = '/';
    NewPath[Index + 1] = '\0';
  }
  strcat(NewPath, AppendPath);
  strcat(NewPath, QuotesStr);
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
  UINT32              SectionHeaderLen;
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
  CHAR8               *SystemCommand;
  EFI_GUID            *EfiGuid;
  UINT16              DataOffset;
  UINT16              Attributes;
  UINT32              RealHdrLen;
  CHAR8               *ToolInputFileName;
  CHAR8               *ToolOutputFileName;
  CHAR8               *UIFileName;

  ParsedLength = 0;
  ToolInputFileName = NULL;
  ToolOutputFileName = NULL;

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

    //
    // Get real section file size
    //
    SectionLength = GetSectionFileLength ((EFI_COMMON_SECTION_HEADER *) Ptr);
    SectionHeaderLen = GetSectionHeaderLength((EFI_COMMON_SECTION_HEADER *)Ptr);

    SectionName = SectionNameToStr (Type);
    if (SectionName != NULL) {
      printf ("------------------------------------------------------------\n");
      printf ("  Type:  %s\n  Size:  0x%08X\n", SectionName, (unsigned) SectionLength);
      free (SectionName);
    }

    switch (Type) {
    case EFI_SECTION_RAW:
    case EFI_SECTION_PIC:
    case EFI_SECTION_TE:
      // default is no more information
      break;

    case EFI_SECTION_PE32:
      if (EnableHash) {
        ToolInputFileName  = "edk2Temp_InputEfi.tmp";
        ToolOutputFileName = "edk2Temp_OutputHash.tmp";
        RebaseImage(ToolInputFileName, (UINT8*)Ptr + SectionHeaderLen, 0);
        PutFileImage (
          ToolInputFileName,
          (CHAR8*)Ptr + SectionHeaderLen,
          SectionLength - SectionHeaderLen
          );

        SystemCommand = malloc (
          strlen (OPENSSL_COMMAND_FORMAT_STRING) +
          strlen (OpenSslPath) +
          strlen (ToolInputFileName) +
          strlen (ToolOutputFileName) +
          1
          );
        if (SystemCommand == NULL) {
          Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
          return EFI_OUT_OF_RESOURCES;
        }
        sprintf (
          SystemCommand,
          OPENSSL_COMMAND_FORMAT_STRING,
          OpenSslPath,
          ToolOutputFileName,
          ToolInputFileName
          );

        if (system (SystemCommand) != EFI_SUCCESS) {
          Error (NULL, 0, 3000, "Open SSL command not available.  Please verify PATH or set OPENSSL_PATH.", NULL);
        }
        else {
          FILE *fp;
          CHAR8 *StrLine;
          CHAR8 *NewStr;
          UINT32 nFileLen;
          if((fp = fopen(ToolOutputFileName,"r")) == NULL) {
            Error (NULL, 0, 0004, "Hash the PE32 image failed.", NULL);
          }
          else {
            fseek(fp,0,SEEK_SET);
            fseek(fp,0,SEEK_END);
            nFileLen = ftell(fp);
            fseek(fp,0,SEEK_SET);
            StrLine = malloc(nFileLen);
            if (StrLine == NULL) {
              fclose(fp);
              free (SystemCommand);
              Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
              return EFI_OUT_OF_RESOURCES;
            }
            fgets(StrLine, nFileLen, fp);
            NewStr = strrchr (StrLine, '=');
            printf ("  SHA1: %s\n", NewStr + 1);
            free (StrLine);
            fclose(fp);
          }
        }
        remove(ToolInputFileName);
        remove(ToolOutputFileName);
        free (SystemCommand);
      }
      break;

    case EFI_SECTION_USER_INTERFACE:
      UIFileName = (CHAR8 *) malloc (UnicodeStrLen (((EFI_USER_INTERFACE_SECTION *) Ptr)->FileNameString) + 1);
      if (UIFileName == NULL) {
        Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
        return EFI_OUT_OF_RESOURCES;
      }
      Unicode2AsciiString (((EFI_USER_INTERFACE_SECTION *) Ptr)->FileNameString, UIFileName);
      printf ("  String: %s\n", UIFileName);
      free (UIFileName);
      break;

    case EFI_SECTION_FIRMWARE_VOLUME_IMAGE:
      Status = PrintFvInfo (Ptr + SectionHeaderLen, TRUE);
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
      printf ("  Build Number:  0x%02X\n", *(UINT16 *)(Ptr + SectionHeaderLen));
      printf ("  Version Strg:  %s\n", (char*) (Ptr + SectionHeaderLen + sizeof (UINT16)));
      break;

    case EFI_SECTION_COMPRESSION:
      UncompressedBuffer  = NULL;
      if (SectionHeaderLen == sizeof (EFI_COMMON_SECTION_HEADER)) {
        RealHdrLen = sizeof(EFI_COMPRESSION_SECTION);
        UncompressedLength  = ((EFI_COMPRESSION_SECTION *)Ptr)->UncompressedLength;
        CompressionType     = ((EFI_COMPRESSION_SECTION *)Ptr)->CompressionType;
      } else {
        RealHdrLen = sizeof(EFI_COMPRESSION_SECTION2);
        UncompressedLength  = ((EFI_COMPRESSION_SECTION2 *)Ptr)->UncompressedLength;
        CompressionType     = ((EFI_COMPRESSION_SECTION2 *)Ptr)->CompressionType;
      }
      CompressedLength    = SectionLength - RealHdrLen;
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

        UncompressedBuffer = Ptr + RealHdrLen;
      } else if (CompressionType == EFI_STANDARD_COMPRESSION) {
        GetInfoFunction     = EfiGetInfo;
        DecompressFunction  = EfiDecompress;
        printf ("  Compression Type:  EFI_STANDARD_COMPRESSION\n");

        CompressedBuffer  = Ptr + RealHdrLen;

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
        if (ScratchBuffer == NULL) {
          Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
          return EFI_OUT_OF_RESOURCES;
        }
        UncompressedBuffer  = malloc (UncompressedLength);
        if (UncompressedBuffer == NULL) {
          free (ScratchBuffer);
          Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
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
      if (SectionHeaderLen == sizeof(EFI_COMMON_SECTION_HEADER)) {
        EfiGuid = &((EFI_GUID_DEFINED_SECTION *) Ptr)->SectionDefinitionGuid;
        DataOffset = ((EFI_GUID_DEFINED_SECTION *) Ptr)->DataOffset;
        Attributes = ((EFI_GUID_DEFINED_SECTION *) Ptr)->Attributes;
      } else {
        EfiGuid = &((EFI_GUID_DEFINED_SECTION2 *) Ptr)->SectionDefinitionGuid;
        DataOffset = ((EFI_GUID_DEFINED_SECTION2 *) Ptr)->DataOffset;
        Attributes = ((EFI_GUID_DEFINED_SECTION2 *) Ptr)->Attributes;
      }
      printf ("  SectionDefinitionGuid:  ");
      PrintGuid (EfiGuid);
      printf ("\n");
      printf ("  DataOffset:             0x%04X\n", (unsigned) DataOffset);
      printf ("  Attributes:             0x%04X\n", (unsigned) Attributes);

      ExtractionTool =
        LookupGuidedSectionToolPath (
          mParsedGuidedSectionTools,
          EfiGuid
          );

      if (ExtractionTool != NULL) {
       #ifndef __GNUC__
        ToolInputFile = CloneString (tmpnam (NULL));
        ToolOutputFile = CloneString (tmpnam (NULL));
       #else
        char tmp1[] = "/tmp/fileXXXXXX";
        char tmp2[] = "/tmp/fileXXXXXX";
        int fd1;
        int fd2;
        fd1 = mkstemp(tmp1);
        fd2 = mkstemp(tmp2);
        ToolInputFile = CloneString(tmp1);
        ToolOutputFile = CloneString(tmp2);
        close(fd1);
        close(fd2);
       #endif

        if ((ToolInputFile == NULL) || (ToolOutputFile == NULL)) {
          if (ToolInputFile != NULL) {
            free (ToolInputFile);
          }
          if (ToolOutputFile != NULL) {
            free (ToolOutputFile);
          }
          free (ExtractionTool);

          Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
          return EFI_OUT_OF_RESOURCES;
        }

        //
        // Construction 'system' command string
        //
        SystemCommand = malloc (
          strlen (EXTRACT_COMMAND_FORMAT_STRING) +
          strlen (ExtractionTool) +
          strlen (ToolInputFile) +
          strlen (ToolOutputFile) +
          1
          );
        if (SystemCommand == NULL) {
          free (ToolInputFile);
          free (ToolOutputFile);
          free (ExtractionTool);

          Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
          return EFI_OUT_OF_RESOURCES;
        }
        sprintf (
          SystemCommand,
          EXTRACT_COMMAND_FORMAT_STRING,
          ExtractionTool,
          ToolOutputFile,
          ToolInputFile
          );
        free (ExtractionTool);

        Status =
          PutFileImage (
            ToolInputFile,
            (CHAR8*) SectionBuffer + DataOffset,
            BufferLength - DataOffset
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
        free (SystemCommand);
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
                   EfiGuid,
                   &gEfiCrc32GuidedSectionExtractionProtocolGuid
                   )
          ) {
        //
        // CRC32 guided section
        //
        Status = ParseSection (
                  SectionBuffer + DataOffset,
                  BufferLength - DataOffset
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

  Ptr += GetSectionHeaderLength((EFI_COMMON_SECTION_HEADER *)Ptr);
  SectionLength -= GetSectionHeaderLength((EFI_COMMON_SECTION_HEADER *)Ptr);
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
  CHAR8             FormatString[MAX_LINE_LEN];
  GUID_TO_BASENAME  *GPtr;

  if ((Fptr = fopen (LongFilePath (FileName), "r")) == NULL) {
    printf ("ERROR: Failed to open input cross-reference file '%s'\n", FileName);
    return EFI_DEVICE_ERROR;
  }

  //
  // Generate the format string for fscanf
  //
  sprintf (
    FormatString,
    "%%%us %%%us",
    (unsigned) sizeof (GPtr->Guid) - 1,
    (unsigned) sizeof (GPtr->BaseName) - 1
    );

  while (fgets (Line, sizeof (Line), Fptr) != NULL) {
    //
    // Allocate space for another guid/basename element
    //
    GPtr = malloc (sizeof (GUID_TO_BASENAME));
    if (GPtr == NULL) {
      fclose (Fptr);
      return EFI_OUT_OF_RESOURCES;
    }

    memset ((char *) GPtr, 0, sizeof (GUID_TO_BASENAME));
    if (sscanf (Line, FormatString, GPtr->Guid, GPtr->BaseName) == 2) {
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
  fprintf (stdout, "Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.\n\n");
  fprintf (stdout, "  Display Tiano Firmware Volume FFS image information\n\n");

  //
  // Details Option
  //
  fprintf (stdout, "optional arguments:\n");
  fprintf (stdout, "  -h, --help\n\
            Show this help message and exit\n");
  fprintf (stdout, "  --version\n\
           Show program's version number and exit\n");
  fprintf (stdout, "  -d [DEBUG], --debug [DEBUG]\n\
            Output DEBUG statements, where DEBUG_LEVEL is 0 (min) - 9 (max)\n");
  fprintf (stdout, "  -v, --verbose\n\
            Print informational statements\n");
  fprintf (stdout, "  -q, --quiet\n\
            Returns the exit code, error messages will be displayed\n");
  fprintf (stdout, "  -s, --silent\n\
            Returns only the exit code; informational and error\n\
            messages are not displayed\n");
  fprintf (stdout, "  -x XREF_FILENAME, --xref XREF_FILENAME\n\
            Parse the basename to file-guid cross reference file(s)\n");
  fprintf (stdout, "  -f OFFSET, --offset OFFSET\n\
            The offset from the start of the input file to start \n\
            processing an FV\n");
  fprintf (stdout, "  --hash\n\
            Generate HASH value of the entire PE image\n");
  fprintf (stdout, "  --sfo\n\
            Reserved for future use\n");
}

