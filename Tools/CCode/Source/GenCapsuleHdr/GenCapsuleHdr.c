/*++

Copyright (c)  2002-2006 Intel Corporation. All rights reserved
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  GenCapsuleHdr.c  

Abstract:

  Generate a capsule header for a file, and optionally prepend the
  header to a file or list of files.

--*/

#define _UNICODE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <Common/UefiBaseTypes.h>
#include <Common/MultiPhase.h>
#include <Common/Capsule.h>
#include <Common/FirmwareVolumeImageFormat.h>
#include <Common/FirmwareVolumeHeader.h>
#include <Common/FirmwareFileSystem.h>  // for FV header GUID
#include <Guid/Capsule.h>
#include <Guid/FirmwareFileSystem.h>  // for FV header GUID

#include "CommonLib.h"
#include "EfiUtilityMsgs.h"

#define MAX_PATH                  256

#define UTILITY_NAME              "GenCapsuleHdr"
#define UTILITY_MAJOR_VERSION     1
#define UTILITY_MINOR_VERSION     0

#define UNICODE_BACKSLASH         L'\\'
#define UNICODE_FILE_START        0xFEFF
#define UNICODE_CR                0x000D
#define UNICODE_LF                0x000A
#define UNICODE_NULL              0x0000
#define UNICODE_SPACE             L' '
#define UNICODE_SLASH             L'/'
#define UNICODE_DOUBLE_QUOTE      L'"'
#define UNICODE_A                 L'A'
#define UNICODE_F                 L'F'
#define UNICODE_Z                 L'Z'
#define UNICODE_a                 L'a'
#define UNICODE_f                 L'f'
#define UNICODE_z                 L'z'
#define UNICODE_0                 L'0'
#define UNICODE_9                 L'9'
#define UNICODE_TAB               L'\t'

#define OEM_HEADER_STRING         L"OemHeader"
#define AUTHOR_INFO_STRING        L"AuthorInfo"
#define REVISION_INFO_STRING      L"RevisionInfo"
#define SHORT_DESCRIPTION_STRING  L"ShortDescription"
#define LONG_DESCRIPTION_STRING   L"LongDescription"
#define EQUAL_STRING              L"="
#define OPEN_BRACE_STRING         L"{"
#define CLOSE_BRACE_STRING        L"}"
#define GUID_STRING               L"GUID"
#define DATA_STRING               L"DATA"

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
#define UEFI_CAPSULE_HEADER_NO_FALAGS      0
#define UEFI_CAPSULE_HEADER_RESET_FALAGS   CAPSULE_FLAGS_PERSIST_ACROSS_RESET
#define UEFI_CAPSULE_HEADER_ALL_FALAGS     (CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE)
#endif

typedef wchar_t WCHAR;

typedef struct _FILE_LIST {
  struct _FILE_LIST *Next;
  INT8              FileName[MAX_PATH];
} FILE_LIST;

typedef struct _SIZE_LIST {
  struct _SIZE_LIST *Next;
  UINT32            Size;
} SIZE_LIST;

typedef struct {
  INT8    FileName[MAX_PATH];
  WCHAR   *FileBuffer;
  WCHAR   *FileBufferPtr;
  UINT32  FileSize;
  FILE    *FilePtr;
  BOOLEAN EndOfFile;
  UINT32  LineNum;
} SOURCE_FILE;

//
// Here's all our globals.
//
static struct {
  BOOLEAN   Dump;
  BOOLEAN   Verbose;
  BOOLEAN   JoinMode;
  INT8      ScriptFileName[MAX_PATH];
  INT8      OutputFileName[MAX_PATH];
  FILE_LIST *FileList;
  FILE      *OutFptr;
  SIZE_LIST *SizeList;
  SIZE_LIST *LastSize;
  SIZE_LIST *CurrentSize;
} mOptions;

static EFI_GUID mEfiCapsuleHeaderGuid = EFI_CAPSULE_GUID;

void
CreateGuid (
  EFI_GUID *Guid
  );

static
STATUS
ProcessArgs (
  int   Argc,
  char  *Argv[]
  );

static
void
SkipWhiteSpace (
  SOURCE_FILE *SourceFile
  );

static
STATUS
GetHexValue (
  SOURCE_FILE  *SourceFile,
  UINT32       *Value,
  UINT32       NumDigits
  );

static
BOOLEAN
GetSplitFileName (
  INT8    *BaseFileName,
  INT8    *NewFileName,
  UINT32  SequenceNumber
  );

static
STATUS
SplitCapsule (
  INT8 *CapsuleFileName
  );

static
void
Version (
  VOID
  );

static
void
Usage (
  VOID
  );

static
void
DumpCapsule (
  VOID
  );

static
STATUS
JoinCapsule (
  VOID
  );

static
STATUS
DumpCapsuleHeaderStrings (
  UINT8   *SectionName,
  WCHAR   *Buffer
  );

static
STATUS
CheckFirmwareVolumeHeader (
  INT8    *FileName,
  INT8    *Buffer,
  UINT32  BufferSize
  );

static
BOOLEAN
IsToken (
  SOURCE_FILE *File,
  WCHAR       *Token
  );

static
BOOLEAN
GetNumber (
  INT8    *Str,
  UINT32  *Value
  );

static
STATUS
ProcessScriptFile (
  INT8                *ScriptFileName,
  FILE                *OutFptr,
  EFI_CAPSULE_HEADER  *CapsuleHeader
  );

static
STATUS
ParseCapsuleInfo (
  SOURCE_FILE       *SourceFile,
  FILE              *OutFptr,
  WCHAR             *SectionName
  );

static
STATUS
CreateCapsule (
  VOID
  );

static
STATUS
ParseOemInfo (
  SOURCE_FILE       *SourceFile,
  FILE              *OutFptr
  );

static
BOOLEAN
IsWhiteSpace (
  WCHAR Char
  );

static
BOOLEAN
EndOfFile (
  SOURCE_FILE *File
  );

int
main (
  int   Argc,
  char  *Argv[]
  )
/*++

Routine Description:
  Call the routine to process the command-line arguments, then
  dispatch to the appropriate function.
  
Arguments:
  Standard C main() argc and argv.

Returns:
  0      if successful
  nonzero otherwise
  
--*/
// GC_TODO:    Argc - add argument and description to function comment
// GC_TODO:    ] - add argument and description to function comment
{
  STATUS    Status;
  FILE_LIST *NextFile;
  //
  // Specify our program name to the error printing routines.
  //
  SetUtilityName (UTILITY_NAME);
  //
  // Process the command-line arguments
  //
  Status = ProcessArgs (Argc, Argv);
  if (Status == STATUS_SUCCESS) {
    if (mOptions.Dump) {
      DumpCapsule ();
    } else if (mOptions.JoinMode) {
      JoinCapsule ();
    } else {
      CreateCapsule ();
    }
  }
  //
  // Cleanup
  //
  while (mOptions.FileList != NULL) {
    NextFile = mOptions.FileList->Next;
    free (mOptions.FileList);
    mOptions.FileList = NextFile;
  }

  while (mOptions.SizeList != NULL) {
    mOptions.CurrentSize = mOptions.SizeList->Next;
    free (mOptions.SizeList);
    mOptions.SizeList = mOptions.CurrentSize;
  }

  return GetUtilityStatus ();
}

static
STATUS
CreateCapsule (
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
  FILE                        *InFptr;
  FILE_LIST                   *FileList;
  INT8                        *Buffer;
  UINT32                      Size;
  EFI_CAPSULE_HEADER          CapsuleHeader;
  UINT8                       Zero;
  UINT8                       FirstFile;
  UINT32                      CapsuleHeaderSize;
  long                        InsertedBlockMapEntryOffset;
  EFI_FV_BLOCK_MAP_ENTRY      InsertedBlockMapEntry;
  UINT64                      FirmwareVolumeSize;
  long                        FileSize;
  EFI_FIRMWARE_VOLUME_HEADER  FVHeader;

  Buffer                      = NULL;
  InFptr                      = NULL;
  FirmwareVolumeSize          = 0;
  CapsuleHeaderSize           = 0;
  InsertedBlockMapEntryOffset = 0;
  memset (&InsertedBlockMapEntry, 0, sizeof (EFI_FV_BLOCK_MAP_ENTRY));
  memset (&FVHeader, 0, sizeof (EFI_FIRMWARE_VOLUME_HEADER));

  if ((mOptions.OutFptr = fopen (mOptions.OutputFileName, "wb")) == NULL) {
    Error (NULL, 0, 0, mOptions.OutputFileName, "failed to open output file for writing");
    return STATUS_ERROR;
  }

  memset ((char *) &CapsuleHeader, 0, sizeof (CapsuleHeader));
  memcpy ((void *) &CapsuleHeader.CapsuleGuid, (void *) &mEfiCapsuleHeaderGuid, sizeof (EFI_GUID));
  CapsuleHeader.HeaderSize        = sizeof (EFI_CAPSULE_HEADER);
  CapsuleHeader.CapsuleImageSize  = sizeof (EFI_CAPSULE_HEADER);
  if (mOptions.ScriptFileName[0] != 0) {
    if (ProcessScriptFile (mOptions.ScriptFileName, mOptions.OutFptr, &CapsuleHeader) != STATUS_SUCCESS) {
      goto Done;
    }
  } else {
    //
    // Insert a default capsule header
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
    CapsuleHeader.HeaderSize = sizeof (EFI_CAPSULE_HEADER);
    CapsuleHeader.Flags   = UEFI_CAPSULE_HEADER_ALL_FALAGS;
#endif
    CapsuleHeader.OffsetToCapsuleBody = sizeof (EFI_CAPSULE_HEADER);

   if (fwrite ((void *) &CapsuleHeader, sizeof (CapsuleHeader), 1, mOptions.OutFptr) != 1) {
     Error (NULL, 0, 0, mOptions.OutputFileName, "failed to write to file");
     goto Done;
   }
 }

  CapsuleHeaderSize = CapsuleHeader.OffsetToCapsuleBody;
  //
  // Now copy the contents of any other files specified on the command
  // line to the output file. Files must be FFS files, which are aligned
  // on 8-byte boundaries. Don't align the first file, since it's the start
  // of the image once the capsule header has been removed.
  //
  FileList  = mOptions.FileList;
  FirstFile = 1;
  Zero      = 0;
  while (FileList != NULL) {
    if ((InFptr = fopen (FileList->FileName, "rb")) == NULL) {
      Error (NULL, 0, 0, FileList->FileName, "failed to open file for reading");
      goto Done;
    }
    //
    // Allocate a buffer into which we can read the file.
    //
    fseek (InFptr, 0, SEEK_END);
    Size = ftell (InFptr);
    rewind (InFptr);
    Buffer = (char *) malloc (Size);
    if (Buffer == NULL) {
      Error (__FILE__, __LINE__, 0, FileList->FileName, "failed to allocate buffer to read file into");
      goto Done;
    }

    if (fread ((void *) Buffer, Size, 1, InFptr) != 1) {
      Error (NULL, 0, 0, FileList->FileName, "failed to read file contents");
      goto Done;
    }

    if (Size > 0) {
      //
      // Align the write of the first bytes from the file if not the first file
      //
      if (FirstFile) {
        //
        // First file must be a firmware volume. Double-check, and then insert
        // an additional block map entry so we can add more files from the command line
        //
        if (CheckFirmwareVolumeHeader (FileList->FileName, Buffer, Size) != STATUS_SUCCESS) {
          goto Done;
        }
        //
        // Save a copy of the firmware volume header for later
        //
        memcpy (&FVHeader, Buffer, sizeof (EFI_FIRMWARE_VOLUME_HEADER));
        FirmwareVolumeSize = FVHeader.FvLength;
        if (FileList->Next != NULL) {
          //
          // Copy the firmware volume header
          //
          InsertedBlockMapEntryOffset = CapsuleHeaderSize + FVHeader.HeaderLength;
          if (fwrite (Buffer, FVHeader.HeaderLength, 1, mOptions.OutFptr) != 1) {
            Error (NULL, 0, 0, mOptions.OutputFileName, "failed to write to file");
            goto Done;
          }

          if (fwrite (&InsertedBlockMapEntry, sizeof (EFI_FV_BLOCK_MAP_ENTRY), 1, mOptions.OutFptr) != 1) {
            Error (NULL, 0, 0, mOptions.OutputFileName, "failed to write to file");
            goto Done;
          }

          if (fwrite (
                Buffer + FVHeader.HeaderLength,
                Size - FVHeader.HeaderLength,
                1,
                mOptions.OutFptr
                ) != 1) {
            Error (NULL, 0, 0, mOptions.OutputFileName, "failed to write to file");
            goto Done;
          }
        } else {
          //
          // Copy the file contents as-is
          //
          if (fwrite ((void *) Buffer, Size, 1, mOptions.OutFptr) != 1) {
            Error (NULL, 0, 0, mOptions.OutputFileName, "failed to write to file");
            goto Done;
          }
        }
      } else {
        while ((ftell (mOptions.OutFptr) - CapsuleHeaderSize) & 0x07) {
          if (fwrite ((void *) &Zero, 1, 1, mOptions.OutFptr) != 1) {
            Error (NULL, 0, 0, mOptions.OutputFileName, "failed to write to file");
            goto Done;
          }
        }

        if (fwrite ((void *) Buffer, Size, 1, mOptions.OutFptr) != 1) {
          Error (NULL, 0, 0, mOptions.OutputFileName, "failed to write to file");
          goto Done;
        }
      }

      FirstFile = 0;
    }

    free (Buffer);
    Buffer = NULL;
    fclose (InFptr);
    InFptr    = NULL;
    FileList  = FileList->Next;
  }

Done:
  if (Buffer != NULL) {
    free (Buffer);
  }

  if (InFptr != NULL) {
    fclose (InFptr);
  }
  //
  // If we inserted an additional block map entry, then fix it up. Fix up the
  // FV header as well to reflect our new size.
  //
  if (InsertedBlockMapEntryOffset != 0) {
    FileSize                        = ftell (mOptions.OutFptr);
    InsertedBlockMapEntry.NumBlocks = 1;
    InsertedBlockMapEntry.BlockLength = (UINT32) ((UINT64) FileSize - (UINT64) CapsuleHeaderSize - FirmwareVolumeSize - sizeof (EFI_FV_BLOCK_MAP_ENTRY));
    fseek (mOptions.OutFptr, InsertedBlockMapEntryOffset, SEEK_SET);
    fwrite (&InsertedBlockMapEntry, sizeof (EFI_FV_BLOCK_MAP_ENTRY), 1, mOptions.OutFptr);
    //
    // Fix up the firmware volume header and write it out
    //
    fseek (mOptions.OutFptr, CapsuleHeaderSize, SEEK_SET);
    FVHeader.FvLength = FileSize - CapsuleHeaderSize;
    FVHeader.HeaderLength += sizeof (EFI_FV_BLOCK_MAP_ENTRY);
    fwrite (&FVHeader, sizeof (EFI_FIRMWARE_VOLUME_HEADER) - sizeof (EFI_FV_BLOCK_MAP_ENTRY), 1, mOptions.OutFptr);
    //
    // Reposition to the end of the file
    //
  }
  //
  // Close files and free the global string lists we allocated memory for
  //
  if (mOptions.OutFptr != NULL) {
    //
    // We should now know the full capsule image size. Update the header and write it again.
    //
    fseek (mOptions.OutFptr, 0, SEEK_END);
    Size  = ftell (mOptions.OutFptr);
    CapsuleHeader.CapsuleImageSize = Size;
    fseek (mOptions.OutFptr, 0, SEEK_SET);
    if (fwrite ((void *) &CapsuleHeader, sizeof (CapsuleHeader), 1, mOptions.OutFptr) != 1) {
      Error (NULL, 0, 0, mOptions.OutputFileName, "failed to write to file");
    }

    fseek (mOptions.OutFptr, 0, SEEK_END);
    fclose (mOptions.OutFptr);
    mOptions.OutFptr = NULL;
  }
  //
  // If they are doing split capsule output, then split it up now.
  //
  if ((mOptions.Dump == 0) && (GetUtilityStatus () == STATUS_SUCCESS) && (mOptions.SizeList != NULL)) {
    SplitCapsule (mOptions.OutputFileName);
  }

  return STATUS_SUCCESS;
}

static
STATUS
ProcessScriptFile (
  INT8                *ScriptFileName,
  FILE                *OutFptr,
  EFI_CAPSULE_HEADER  *CapsuleHeader
  )
/*++

Routine Description:
  Parse a capsule header script file.

Arguments:
  ScriptFileName    - name of script file to parse
  OutFptr           - output to dump binary data
  CapsuleHeader     - capsule header to update with size info
                      of parsed fields in the script file

Returns:
  STATUS_SUCCESS - if all went well

--*/
{
#if 0
  STATUS      Status;
  SOURCE_FILE SourceFile;
  WCHAR       *WScriptFileName;
  BOOLEAN     InComment;

  if (fwrite (CapsuleHeader, sizeof (EFI_CAPSULE_HEADER), 1, OutFptr) != 1) {
    Error (NULL, 0, 0, "failed to write capsule header to output file", NULL);
    return STATUS_ERROR;
  }

  memset (&SourceFile, 0, sizeof (SOURCE_FILE));
  strcpy (SourceFile.FileName, ScriptFileName);

  Status = STATUS_ERROR;
  //
  // Open the input unicode script file and read it into a buffer
  //
  WScriptFileName = (WCHAR *) malloc ((strlen (ScriptFileName) + 1) * sizeof (WCHAR));
  if (WScriptFileName == NULL) {
    Error (__FILE__, __LINE__, 0, "failed to allocate memory", NULL);
    return STATUS_ERROR;
  }

  swprintf (WScriptFileName, L"%S", ScriptFileName);
  if ((SourceFile.FilePtr = _wfopen (WScriptFileName, L"r")) == NULL) {
    free (WScriptFileName);
    Error (NULL, 0, 0, ScriptFileName, "failed to open script file for reading");
    goto Done;
  }

  free (WScriptFileName);
  fseek (SourceFile.FilePtr, 0, SEEK_END);
  SourceFile.FileSize = ftell (SourceFile.FilePtr);
  rewind (SourceFile.FilePtr);
  SourceFile.FileBuffer = (WCHAR *) malloc (SourceFile.FileSize + sizeof (WCHAR));
  if (SourceFile.FileBuffer == NULL) {
    Error (__FILE__, __LINE__, 0, ScriptFileName, "failed to allocate memory to read in file contents");
    goto Done;
  }

  if (fread (SourceFile.FileBuffer, SourceFile.FileSize, 1, SourceFile.FilePtr) != 1) {
    Error (NULL, 0, 0, ScriptFileName, "failed to read file contents");
    goto Done;
  }

  SourceFile.FileBufferPtr  = SourceFile.FileBuffer;
  SourceFile.LineNum        = 1;
  if (SourceFile.FileBuffer[0] != UNICODE_FILE_START) {
    Error (ScriptFileName, 1, 0, "file does not appear to be a unicode file", NULL);
    goto Done;
  }

  SourceFile.FileBufferPtr++;
  SourceFile.FileBuffer[SourceFile.FileSize / sizeof (WCHAR)] = 0;
  //
  // Walk the source file buffer and replace all carriage returns with 0 so
  // we can print from the file contents on parse errors.
  //
  InComment = 0;
  while (!EndOfFile (&SourceFile)) {
    if (SourceFile.FileBufferPtr[0] == UNICODE_CR) {
      SourceFile.FileBufferPtr[0] = 0;
      InComment                   = 0;
    } else if (SourceFile.FileBufferPtr[0] == UNICODE_LF) {
      InComment = 0;
    } else if (InComment) {
      SourceFile.FileBufferPtr[0] = UNICODE_SPACE;
    } else if ((SourceFile.FileBufferPtr[0] == UNICODE_SLASH) && (SourceFile.FileBufferPtr[1] == UNICODE_SLASH)) {
      InComment                   = 1;
      SourceFile.FileBufferPtr[0] = UNICODE_SPACE;
    }

    SourceFile.FileBufferPtr++;
  }
  //
  // Reposition to the start of the file, but skip over the unicode file start
  //
  SourceFile.FileBufferPtr = SourceFile.FileBuffer;
  SourceFile.FileBufferPtr++;
  SourceFile.EndOfFile                    = 0;
  CapsuleHeader->OffsetToOemDefinedHeader = ftell (OutFptr);
  //
  // Parse the OEM bytes
  //
  if (ParseOemInfo (&SourceFile, OutFptr) != STATUS_SUCCESS) {
    goto Done;
  }
  //
  // Parse the author information
  //
  CapsuleHeader->OffsetToAuthorInformation = ftell (OutFptr);
  if (ParseCapsuleInfo (&SourceFile, OutFptr, AUTHOR_INFO_STRING) != STATUS_SUCCESS) {
    goto Done;
  }
  //
  // Parse the revision information
  //
  CapsuleHeader->OffsetToRevisionInformation = ftell (OutFptr);
  if (ParseCapsuleInfo (&SourceFile, OutFptr, REVISION_INFO_STRING) != STATUS_SUCCESS) {
    goto Done;
  }
  //
  // Parse the short description
  //
  CapsuleHeader->OffsetToShortDescription = ftell (OutFptr);
  if (ParseCapsuleInfo (&SourceFile, OutFptr, SHORT_DESCRIPTION_STRING) != STATUS_SUCCESS) {
    goto Done;
  }
  //
  // Parse the long description
  //
  CapsuleHeader->OffsetToLongDescription = ftell (OutFptr);
  if (ParseCapsuleInfo (&SourceFile, OutFptr, LONG_DESCRIPTION_STRING) != STATUS_SUCCESS) {
    goto Done;
  }
  //
  // Better be end of contents
  //
  SkipWhiteSpace (&SourceFile);
  if (!EndOfFile (&SourceFile)) {
    Error (ScriptFileName, SourceFile.LineNum, 0, NULL, "expected end-of-file, not %.20S", SourceFile.FileBufferPtr);
    goto Done;
  }

  CapsuleHeader->OffsetToCapsuleBody = ftell (OutFptr);
  rewind (OutFptr);
  fwrite (CapsuleHeader, sizeof (EFI_CAPSULE_HEADER), 1, OutFptr);
  fseek (OutFptr, 0, SEEK_END);
  Status = STATUS_SUCCESS;
Done:
  if (SourceFile.FilePtr != NULL) {
    fclose (SourceFile.FilePtr);
  }

  if (SourceFile.FileBuffer != NULL) {
    free (SourceFile.FileBuffer);
  }

  return Status;

#endif
  return STATUS_SUCCESS;
}
//
// Parse the OEM data of format:
//      OemInfo {
//            GUID = 12345676-1234-1234-123456789ABC
//            DATA = 0x01, 0x02, 0x03...
//      }
//
static
STATUS
ParseOemInfo (
  SOURCE_FILE       *SourceFile,
  FILE              *OutFptr
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  SourceFile  - GC_TODO: add argument description
  OutFptr     - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  long                    OemHeaderOffset;
  UINT32                  Data;
  EFI_CAPSULE_OEM_HEADER  OemHeader;
  STATUS                  Status;
  UINT32                  DigitCount;
  WCHAR                   *SaveFilePos;
  UINT8                   ByteData;

  Status = STATUS_ERROR;
  memset (&OemHeader, 0, sizeof (EFI_CAPSULE_OEM_HEADER));
  OemHeaderOffset       = ftell (OutFptr);
  OemHeader.HeaderSize  = sizeof (EFI_CAPSULE_OEM_HEADER);
  if (fwrite (&OemHeader, sizeof (EFI_CAPSULE_OEM_HEADER), 1, OutFptr) != 1) {
    Error (NULL, 0, 0, "failed to write OEM header to output file", NULL);
    goto Done;
  }

  if (!IsToken (SourceFile, OEM_HEADER_STRING)) {
    Error (
      SourceFile->FileName,
      SourceFile->LineNum,
      0,
      NULL,
      "expected %S, not %.20S",
      OEM_HEADER_STRING,
      SourceFile->FileBufferPtr
      );
    goto Done;
  }

  if (!IsToken (SourceFile, EQUAL_STRING)) {
    Error (
      SourceFile->FileName,
      SourceFile->LineNum,
      0,
      NULL,
      "expected %S, not %.20S",
      EQUAL_STRING,
      SourceFile->FileBufferPtr
      );
    goto Done;
  }

  if (!IsToken (SourceFile, OPEN_BRACE_STRING)) {
    Error (
      SourceFile->FileName,
      SourceFile->LineNum,
      0,
      NULL,
      "expected %S, not %.20S",
      OPEN_BRACE_STRING,
      SourceFile->FileBufferPtr
      );
    goto Done;
  }
  //
  // Look for:  GUID = xxxxxxx-xxxx-xxxx-xxxxxxxxxxxxx
  //
  if (!IsToken (SourceFile, GUID_STRING)) {
    Error (
      SourceFile->FileName,
      SourceFile->LineNum,
      0,
      NULL,
      "expected %S, not %.20S",
      GUID_STRING,
      SourceFile->FileBufferPtr
      );
    goto Done;
  }

  if (!IsToken (SourceFile, EQUAL_STRING)) {
    Error (
      SourceFile->FileName,
      SourceFile->LineNum,
      0,
      NULL,
      "expected %S, not %.20S",
      EQUAL_STRING,
      SourceFile->FileBufferPtr
      );
    goto Done;
  }
  //
  // Parse the xxxxxxxx-xxxx-xxxx-xxxx portion of the GUID
  //
  SkipWhiteSpace (SourceFile);
  if (GetHexValue (SourceFile, &Data, 8) != STATUS_SUCCESS) {
    Error (SourceFile->FileName, SourceFile->LineNum, 0, "invalid GUID", NULL);
    goto Done;
  }

  OemHeader.OemGuid.Data1 = Data;
  if (!IsToken (SourceFile, L"-")) {
    Error (
      SourceFile->FileName,
      SourceFile->LineNum,
      0,
      NULL,
      "expected dash in GUID, not %S",
      SourceFile->FileBufferPtr
      );
    goto Done;
  }
  //
  // Get 3 word values
  //
  for (DigitCount = 0; DigitCount < 3; DigitCount++) {
    if (GetHexValue (SourceFile, &Data, 4) != STATUS_SUCCESS) {
      Error (SourceFile->FileName, SourceFile->LineNum, 0, "invalid GUID", NULL);
      goto Done;
    }

    switch (DigitCount) {
    case 0:
      OemHeader.OemGuid.Data2 = (UINT16) Data;
      break;

    case 1:
      OemHeader.OemGuid.Data3 = (UINT16) Data;
      break;

    case 2:
      OemHeader.OemGuid.Data4[1]  = (UINT8) Data;
      OemHeader.OemGuid.Data4[0]  = (UINT8) (Data >> 8);
      break;
    }

    if (!IsToken (SourceFile, L"-")) {
      Error (
        SourceFile->FileName,
        SourceFile->LineNum,
        0,
        NULL,
        "expected dash in GUID, not %S",
        SourceFile->FileBufferPtr
        );
      goto Done;
    }
  }
  //
  // Pick up the last 6 bytes of the GUID
  //
  SaveFilePos = SourceFile->FileBufferPtr;
  for (DigitCount = 0; DigitCount < 6; DigitCount++) {
    if (GetHexValue (SourceFile, &Data, 2) != STATUS_SUCCESS) {
      Error (SourceFile->FileName, SourceFile->LineNum, 0, "invalid GUID", NULL);
      goto Done;
    }

    OemHeader.OemGuid.Data4[DigitCount + 2] = (UINT8) Data;
  }
  //
  // Now read raw OEM data bytes. May or may not be present.
  //    DATA = 0x01, 0x02, 0x02...
  //
  if (IsToken (SourceFile, CLOSE_BRACE_STRING)) {
    Status = STATUS_SUCCESS;
    goto Done;
  }

  if (!IsToken (SourceFile, DATA_STRING)) {
    Error (
      SourceFile->FileName,
      SourceFile->LineNum,
      0,
      NULL,
      "expected %S, not %.20S",
      DATA_STRING,
      SourceFile->FileBufferPtr
      );
    goto Done;
  }

  if (!IsToken (SourceFile, EQUAL_STRING)) {
    Error (
      SourceFile->FileName,
      SourceFile->LineNum,
      0,
      NULL,
      "expected %S, not %.20S",
      EQUAL_STRING,
      SourceFile->FileBufferPtr
      );
    goto Done;
  }

  while (!EndOfFile (SourceFile)) {
    if (IsToken (SourceFile, CLOSE_BRACE_STRING)) {
      Status = STATUS_SUCCESS;
      goto Done;
    }

    if (IsToken (SourceFile, L"0x")) {
      if (swscanf (SourceFile->FileBufferPtr, L"%x", &Data) != 1) {
        Error (
          SourceFile->FileName,
          SourceFile->LineNum,
          0,
          NULL,
          "expected hex byte value, not %.20S",
          SourceFile->FileBufferPtr
          );
        goto Done;
      }

      if (Data &~0xFF) {
        Error (
          SourceFile->FileName,
          SourceFile->LineNum,
          0,
          NULL,
          "expected byte hex byte value at %.20S",
          SourceFile->FileBufferPtr
          );
        goto Done;
      }
      //
      // Skip over the hex digits, then write the data
      //
      while (iswxdigit (SourceFile->FileBufferPtr[0])) {
        SourceFile->FileBufferPtr++;
      }

      ByteData = (UINT8) Data;
      if (fwrite (&ByteData, 1, 1, OutFptr) != 1) {
        Error (NULL, 0, 0, "failed to write OEM data to output file", NULL);
        goto Done;
      }

      OemHeader.HeaderSize++;
      //
      // Optional comma
      //
      IsToken (SourceFile, L",");
    } else {
      Error (
        SourceFile->FileName,
        SourceFile->LineNum,
        0,
        NULL,
        "expected hex OEM data, not %.20S",
        SourceFile->FileBufferPtr
        );
      goto Done;
    }
  }

  if (EndOfFile (SourceFile)) {
    Error (
      SourceFile->FileName,
      SourceFile->LineNum,
      0,
      NULL,
      "expected %S close to OEM header data",
      CLOSE_BRACE_STRING
      );
    goto Done;
  }

  Status = STATUS_SUCCESS;
Done:
  //
  // re-write the oem header if no errors
  //
  if (Status == STATUS_SUCCESS) {
    fseek (OutFptr, OemHeaderOffset, SEEK_SET);
    if (fwrite (&OemHeader, sizeof (EFI_CAPSULE_OEM_HEADER), 1, OutFptr) != 1) {
      Error (NULL, 0, 0, "failed to write OEM header to output file", NULL);
      goto Done;
    }

    fseek (OutFptr, 0, SEEK_END);
  }

  return Status;
}

static
STATUS
ParseCapsuleInfo (
  SOURCE_FILE       *SourceFile,
  FILE              *OutFptr,
  WCHAR             *SectionName
  )
// GC_TODO: function comment should start with '/*++'
//
// GC_TODO: function comment is missing 'Routine Description:'
// GC_TODO: function comment is missing 'Arguments:'
// GC_TODO: function comment is missing 'Returns:'
// GC_TODO:    SourceFile - add argument and description to function comment
// GC_TODO:    OutFptr - add argument and description to function comment
// GC_TODO:    SectionName - add argument and description to function comment
// Parse:  eng "string " "parts"
//          spa "string " "parts"
// Write out: "eng string parts\0spa string parts\0\0
//
{
  STATUS  Status;
  int     StringCount;
  WCHAR   Zero;
  WCHAR   Spacebar;

  Status    = STATUS_ERROR;
  Zero      = 0;
  Spacebar  = UNICODE_SPACE;

  if (!IsToken (SourceFile, SectionName)) {
    Error (
      SourceFile->FileName,
      SourceFile->LineNum,
      0,
      NULL,
      "expected %S, not %.20S",
      SectionName,
      SourceFile->FileBufferPtr
      );
    goto Done;
  }

  if (!IsToken (SourceFile, EQUAL_STRING)) {
    Error (
      SourceFile->FileName,
      SourceFile->LineNum,
      0,
      NULL,
      "expected %S, not %.20S",
      EQUAL_STRING,
      SourceFile->FileBufferPtr
      );
    goto Done;
  }

  if (!IsToken (SourceFile, OPEN_BRACE_STRING)) {
    Error (
      SourceFile->FileName,
      SourceFile->LineNum,
      0,
      NULL,
      "expected %S, not %.20S",
      OPEN_BRACE_STRING,
      SourceFile->FileBufferPtr
      );
    goto Done;
  }

  while (!EndOfFile (SourceFile)) {
    if (IsToken (SourceFile, CLOSE_BRACE_STRING)) {
      break;
    }
    //
    // Look for language identifier (3 lowercase chars)
    //
    if ((SourceFile->FileBufferPtr[0] >= UNICODE_a) &&
        (SourceFile->FileBufferPtr[0] <= UNICODE_z) &&
        (SourceFile->FileBufferPtr[1] >= UNICODE_a) &&
        (SourceFile->FileBufferPtr[1] <= UNICODE_z) &&
        (SourceFile->FileBufferPtr[2] >= UNICODE_a) &&
        (SourceFile->FileBufferPtr[2] <= UNICODE_z) &&
        IsWhiteSpace (SourceFile->FileBufferPtr[3])
          ) {
      //
      // Write the 3 chars followed by a spacebar, and then look for opening quote
      //
      fwrite (SourceFile->FileBufferPtr, sizeof (WCHAR), 1, OutFptr);
      SourceFile->FileBufferPtr++;
      fwrite (SourceFile->FileBufferPtr, sizeof (WCHAR), 1, OutFptr);
      SourceFile->FileBufferPtr++;
      fwrite (SourceFile->FileBufferPtr, sizeof (WCHAR), 1, OutFptr);
      SourceFile->FileBufferPtr++;
      fwrite (&Spacebar, sizeof (WCHAR), 1, OutFptr);
      StringCount = 0;
      while (IsToken (SourceFile, L"\"")) {
        StringCount++;
        while (!EndOfFile (SourceFile)) {
          if (SourceFile->FileBufferPtr[0] == UNICODE_DOUBLE_QUOTE) {
            SourceFile->FileBufferPtr++;
            break;
          } else if ((SourceFile->FileBufferPtr[0] == UNICODE_LF) || (SourceFile->FileBufferPtr[0] == 0)) {
            Error (SourceFile->FileName, SourceFile->LineNum, 0, "missing closing quote on string", NULL);
            goto Done;
          } else {
            fwrite (SourceFile->FileBufferPtr, sizeof (WCHAR), 1, OutFptr);
            SourceFile->FileBufferPtr++;
          }
        }
      }

      if (StringCount == 0) {
        Error (
          SourceFile->FileName,
          SourceFile->LineNum,
          0,
          NULL,
          "expected quoted string, not %.20S",
          SourceFile->FileBufferPtr
          );
        goto Done;
      }
      //
      // This string's null terminator
      //
      fwrite (&Zero, sizeof (WCHAR), 1, OutFptr);
    } else {
      Error (
        SourceFile->FileName,
        SourceFile->LineNum,
        0,
        NULL,
        "expected valid language identifer, not %.20S",
        SourceFile->FileBufferPtr
        );
      goto Done;
    }
  }
  //
  // Double null terminator
  //
  fwrite (&Zero, sizeof (WCHAR), 1, OutFptr);
  Status = STATUS_SUCCESS;
Done:
  return Status;
}

static
STATUS
SplitCapsule (
  INT8    *CapsuleFileName
  )
/*++

Routine Description:
  We've created an entire capsule image. Now split it up into the 
  size pieces they requested.

Arguments:
  CapsuleFileName  - name of an existing capsule file on disk

Returns:
  STATUS_SUCCESS - if no problems

Notes:
  This implementation reads in the entire capsule image from
  disk, then overwrites the original file with the first
  in the series.

--*/
{
#if 0
  EFI_CAPSULE_HEADER  *CapHdr;

  EFI_CAPSULE_HEADER  Hdr;
  FILE                *CapFptr;
  FILE                *OutFptr;
  UINT32              SizeLeft;
  UINT32              CurrentSize;
  UINT32              DataSize;
  UINT32              SequenceNumber;
  INT8                *Buffer;
  INT8                FileName[MAX_PATH];
  STATUS              Status;
  UINT32              FileSize;
  //
  // Figure out the total size, then rewind the input file and
  // read the entire thing in
  //
  if ((CapFptr = fopen (CapsuleFileName, "rb")) == NULL) {
    Error (NULL, 0, 0, CapsuleFileName, "failed to open capsule image for reading");
    return STATUS_ERROR;
  }

  OutFptr = NULL;
  Status  = STATUS_SUCCESS;
  fseek (CapFptr, 0, SEEK_END);
  SizeLeft = ftell (CapFptr);
  fseek (CapFptr, 0, SEEK_SET);
  CapHdr = (EFI_CAPSULE_HEADER *) malloc (SizeLeft);
  if (CapHdr == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    goto FailDone;
  }

  if (fread (CapHdr, SizeLeft, 1, CapFptr) != 1) {
    Error (NULL, 0, 0, "failed to read capsule contents", "split failed");
    goto FailDone;
  }

  fclose (CapFptr);
  CapFptr = NULL;
  //
  // Get a GUID to fill in the InstanceId GUID in the header
  //
  CreateGuid (&CapHdr->InstanceId);
  SequenceNumber = 0;
  //
  // If the split size is larger than the original capsule image, then
  // we're done.
  //
  if (mOptions.SizeList->Size >= SizeLeft) {
    mOptions.SizeList->Size = SizeLeft;
    goto Done;
  }
  //
  // First size has to be big enough for the original header
  //
  if (mOptions.SizeList->Size < CapHdr->OffsetToCapsuleBody) {
    Error (NULL, 0, 0, "first split size is insufficient for the original capsule header", NULL);
    goto FailDone;
  }
  //
  // Initialize the header we'll use on all but the first part
  //
  memset (&Hdr, 0, sizeof (Hdr));
  Hdr.CapsuleGuid         = CapHdr->CapsuleGuid;
  Hdr.HeaderSize          = sizeof (Hdr);
  Hdr.Flags               = CapHdr->Flags;
  Hdr.InstanceId          = CapHdr->InstanceId;
  Hdr.CapsuleImageSize    = CapHdr->CapsuleImageSize;
  Hdr.OffsetToCapsuleBody = Hdr.HeaderSize;
  Hdr.SequenceNumber      = 1;
  //
  // printf ("Created %s - 0x%X bytes\n", CapsuleFileName, mOptions.SizeList->Size);
  //
  Buffer = (UINT8 *) CapHdr;
  //
  // Walk the list of sizes and write out a capsule header, and
  // then the raw capsule data.
  //
  //  SizeLeft -= mOptions.SizeList->Size;
  //
  mOptions.CurrentSize = mOptions.SizeList;
  while (SizeLeft) {
    CurrentSize = mOptions.CurrentSize->Size;
    GetSplitFileName (mOptions.OutputFileName, FileName, SequenceNumber);
    if ((OutFptr = fopen (FileName, "wb")) == NULL) {
      Error (NULL, 0, 0, FileName, "failed to open split file for writing");
      goto FailDone;
    }

    if (Buffer == (UINT8 *) CapHdr) {
      //
      // First part -- write out original header and data
      //
      if (fwrite (Buffer, CurrentSize, 1, OutFptr) != 1) {
        Error (NULL, 0, 0, FileName, "failed to write to split image file");
        goto FailDone;
      }

      SizeLeft -= CurrentSize;
      Buffer += CurrentSize;
      DataSize  = CurrentSize;
      FileSize  = CurrentSize;
    } else {
      //
      // Not the first part. Write the default header, and then the raw bytes from the
      // original image.
      //
      if (CurrentSize <= sizeof (Hdr)) {
        Error (NULL, 0, 0, "split size too small for capsule header + data", "0x%X", CurrentSize);
        goto FailDone;
      }

      DataSize = CurrentSize - sizeof (Hdr);
      if (DataSize > SizeLeft) {
        DataSize = SizeLeft;
      }

      if (fwrite (&Hdr, sizeof (Hdr), 1, OutFptr) != 1) {
        Error (NULL, 0, 0, FileName, "failed to write capsule header to output file");
        fclose (OutFptr);
        goto FailDone;
      }

      if (fwrite (Buffer, DataSize, 1, OutFptr) != 1) {
        Error (NULL, 0, 0, FileName, "failed to write capsule data to output file");
        fclose (OutFptr);
        goto FailDone;
      }

      Hdr.SequenceNumber++;
      Buffer += DataSize;
      SizeLeft -= DataSize;
      FileSize = DataSize + sizeof (Hdr);
    }
    //
    // Next size in list if there is one
    //
    if (mOptions.CurrentSize->Next != NULL) {
      mOptions.CurrentSize = mOptions.CurrentSize->Next;
    }

    SequenceNumber++;
    fclose (OutFptr);
    OutFptr = NULL;
    printf ("Created %s - 0x%X bytes (0x%X bytes of data)\n", FileName, FileSize, DataSize);
  }

  goto Done;
FailDone:
  Status = STATUS_ERROR;
Done:
  if (CapHdr != NULL) {
    free (CapHdr);
  }

  if (CapFptr != NULL) {
    fclose (CapFptr);
  }

  if (OutFptr != NULL) {
    fclose (OutFptr);
  }

  return Status;

#endif
   return STATUS_SUCCESS;
}

static
BOOLEAN
GetSplitFileName (
  INT8    *BaseFileName,
  INT8    *NewFileName,
  UINT32  SequenceNumber
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  BaseFileName    - GC_TODO: add argument description
  NewFileName     - GC_TODO: add argument description
  SequenceNumber  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  /*++

Routine Description:
  Given an initial split capsule file name and a sequence number,
  create an appropriate file name for this split of a capsule image.

Arguments:
  BaseFileName   - name of of the first split file in the series
  NewFileName    - output name of the split file
  SequenceNumber - 0-based sequence number of split images

Returns:
  TRUE   - name created successfully
  FALSE  - otherwise

--*/
  INT8    *Ptr;
  INT8    *Part2Start;
  UINT32  Digits;
  UINT32  Len;
  UINT32  BaseOffset;
  //
  // Work back from the end of the file name and see if there is a number somewhere
  //
  for (Ptr = BaseFileName + strlen (BaseFileName) - 1; (Ptr > BaseFileName) && !isdigit (*Ptr); Ptr--)
    ;
  if ((Ptr == BaseFileName) && (!isdigit (*Ptr))) {
    //
    // Found no number, so just add it to the end
    //
    sprintf (NewFileName, "%s%d", BaseFileName, SequenceNumber);
    return TRUE;
  } else {
    //
    // Found a number. Look back to find the first digit.
    //
    Part2Start = Ptr + 1;
    for (Digits = 1; isdigit (*Ptr) && (Ptr > BaseFileName); Ptr--, Digits++)
      ;
    if (!isdigit (*Ptr)) {
      Ptr++;
      Digits--;
    }

    BaseOffset      = atoi (Ptr);
    SequenceNumber  = SequenceNumber + BaseOffset;
    if (Digits > 1) {
      //
      // Copy the first part of the original file name to the new filename
      // This is the path for filenames with format path\name001.cap
      //
      Len = (UINT32) Ptr - (UINT32) BaseFileName;
      strncpy (NewFileName, BaseFileName, Len);
      sprintf (NewFileName + Len, "%0*d", Digits, SequenceNumber);
      strcat (NewFileName, Part2Start);
      return TRUE;
    } else {
      //
      // Only one digit found. This is the path for filenames with
      // format path\name1.cap
      //
      Len = (UINT32) Ptr - (UINT32) BaseFileName + 1;
      strncpy (NewFileName, BaseFileName, Len);
      sprintf (NewFileName + Len - 1, "%d", SequenceNumber);
      strcat (NewFileName, Part2Start);
      return TRUE;
    }
  }
}

static
BOOLEAN
IsWhiteSpace (
  WCHAR  Char
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Char  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  switch (Char) {
  case UNICODE_SPACE:
  case UNICODE_TAB:
  case UNICODE_NULL:
  case UNICODE_CR:
  case UNICODE_LF:
    return TRUE;

  default:
    return FALSE;
  }
}

static
BOOLEAN
IsToken (
  SOURCE_FILE *File,
  WCHAR       *Token
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  File  - GC_TODO: add argument description
  Token - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  SkipWhiteSpace (File);
  if (EndOfFile (File)) {
    return FALSE;
  }

  if (wcsncmp (Token, File->FileBufferPtr, wcslen (Token)) == 0) {
    File->FileBufferPtr += wcslen (Token);
    return TRUE;
  }

  return FALSE;
}

static
STATUS
CheckFirmwareVolumeHeader (
  INT8    *FileName,
  INT8    *Buffer,
  UINT32  BufferSize
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FileName    - GC_TODO: add argument description
  Buffer      - GC_TODO: add argument description
  BufferSize  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  EFI_FIRMWARE_VOLUME_HEADER  *Hdr;
  EFI_GUID                    FVHeaderGuid = EFI_FIRMWARE_FILE_SYSTEM_GUID;

  Hdr           = (EFI_FIRMWARE_VOLUME_HEADER *) Buffer;
  if (Hdr->Signature != EFI_FVH_SIGNATURE) {
    Error (NULL, 0, 0, FileName, "file does not appear to be a firmware volume (bad signature)");
    return STATUS_ERROR;
  }

  if (Hdr->Revision != EFI_FVH_REVISION) {
    Error (NULL, 0, 0, FileName, "unsupported firmware volume header version");
    return STATUS_ERROR;
  }

  if (Hdr->FvLength > BufferSize) {
    Error (NULL, 0, 0, FileName, "malformed firmware volume -- FvLength > file size");
    return STATUS_ERROR;
  }

  if (memcmp (&Hdr->FileSystemGuid, &FVHeaderGuid, sizeof (EFI_GUID)) != 0) {
    Error (NULL, 0, 0, FileName, "invalid FFS GUID in firmware volume header");
    return STATUS_ERROR;
  }

  return STATUS_SUCCESS;
}

static
void
DumpCapsule (
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
#if 0
  FILE                        *InFptr;
  FILE_LIST                   *FileList;
  EFI_CAPSULE_HEADER          CapsuleHeader;
  EFI_FIRMWARE_VOLUME_HEADER  FVHeader;
  EFI_CAPSULE_OEM_HEADER      *OemHeader;
  UINT8                       *BPtr;
  UINT32                      FileSize;
  UINT32                      CapsuleHeaderDataSize;
  UINT8                       ByteCount;
  UINT8                       *CapsuleHeaderData;
  BOOLEAN                     SplitImage;

  InFptr            = NULL;
  CapsuleHeaderData = NULL;
  FileList          = mOptions.FileList;
  while (FileList != NULL) {
    if ((InFptr = fopen (FileList->FileName, "rb")) == NULL) {
      Error (NULL, 0, 0, FileList->FileName, "failed to open file for reading");
      goto Done;
    }

    if (fread (&CapsuleHeader, sizeof (EFI_CAPSULE_HEADER), 1, InFptr) != 1) {
      Error (NULL, 0, 0, FileList->FileName, "failed to read capsule header");
      goto Done;
    }

    fseek (InFptr, 0, SEEK_END);
    FileSize = ftell (InFptr);
    if (CapsuleHeader.CapsuleImageSize > FileSize) {
      SplitImage = TRUE;
    } else {
      SplitImage = FALSE;
    }

    printf (
      "Capsule %s Size=0x%X CargoSize=0x%X\n",
      FileList->FileName,
      FileSize,
      FileSize - CapsuleHeader.OffsetToCapsuleBody
      );
    printf (
      "  GUID                  %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
      CapsuleHeader.CapsuleGuid.Data1,
      (UINT32) CapsuleHeader.CapsuleGuid.Data2,
      (UINT32) CapsuleHeader.CapsuleGuid.Data3,
      (UINT32) CapsuleHeader.CapsuleGuid.Data4[0],
      (UINT32) CapsuleHeader.CapsuleGuid.Data4[1],
      (UINT32) CapsuleHeader.CapsuleGuid.Data4[2],
      (UINT32) CapsuleHeader.CapsuleGuid.Data4[3],
      (UINT32) CapsuleHeader.CapsuleGuid.Data4[4],
      (UINT32) CapsuleHeader.CapsuleGuid.Data4[5],
      (UINT32) CapsuleHeader.CapsuleGuid.Data4[6],
      (UINT32) CapsuleHeader.CapsuleGuid.Data4[7]
      );
    if (memcmp (&CapsuleHeader.CapsuleGuid, &mEfiCapsuleHeaderGuid, sizeof (EFI_GUID)) != 0) {
      printf ("  INVALID GUID");
    }

    printf ("\n");
    printf ("  Header size           0x%08X\n", CapsuleHeader.HeaderSize);
    printf ("  Flags                 0x%08X\n", CapsuleHeader.Flags);
    if (!SplitImage) {
      printf ("  Capsule image size    0x%08X\n", CapsuleHeader.CapsuleImageSize);
    } else {
      printf ("  Capsule image size    0x%08X (split)\n", CapsuleHeader.CapsuleImageSize);
    }

    printf ("  Sequence number       %d\n", CapsuleHeader.SequenceNumber);
    printf (
      "  InstanceId            %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
      CapsuleHeader.InstanceId.Data1,
      (UINT32) CapsuleHeader.InstanceId.Data2,
      (UINT32) CapsuleHeader.InstanceId.Data3,
      (UINT32) CapsuleHeader.InstanceId.Data4[0],
      (UINT32) CapsuleHeader.InstanceId.Data4[1],
      (UINT32) CapsuleHeader.InstanceId.Data4[2],
      (UINT32) CapsuleHeader.InstanceId.Data4[3],
      (UINT32) CapsuleHeader.InstanceId.Data4[4],
      (UINT32) CapsuleHeader.InstanceId.Data4[5],
      (UINT32) CapsuleHeader.InstanceId.Data4[6],
      (UINT32) CapsuleHeader.InstanceId.Data4[7]
      );
    printf ("  Offset to capsule     0x%X\n", CapsuleHeader.OffsetToCapsuleBody);
    //
    // Dump header data if there
    //
    CapsuleHeaderDataSize = CapsuleHeader.OffsetToCapsuleBody - CapsuleHeader.HeaderSize;
    if (CapsuleHeaderDataSize != 0) {
      CapsuleHeaderData = (UINT8 *) malloc (CapsuleHeaderDataSize);
      if (CapsuleHeaderData == NULL) {
        Error (
          NULL,
          0,
          0,
          "failed to allocate memory to read in capsule header data",
          "0x%X bytes",
          CapsuleHeaderDataSize
          );
        goto Done;
      }

      fseek (InFptr, CapsuleHeader.HeaderSize, SEEK_SET);
      if (fread (CapsuleHeaderData, CapsuleHeaderDataSize, 1, InFptr) != 1) {
        Error (
          NULL,
          0,
          0,
          "failed to read capsule header data contents from file",
          "0x%X bytes",
          CapsuleHeaderDataSize
          );
        goto Done;
      }
      //
      // ************************************************************************
      //
      // OEM HEADER
      //
      // ************************************************************************
      //
      if (CapsuleHeader.OffsetToOemDefinedHeader != 0) {
        OemHeader = (EFI_CAPSULE_OEM_HEADER *) (CapsuleHeaderData + CapsuleHeader.OffsetToOemDefinedHeader - CapsuleHeader.HeaderSize);
        printf ("  OEM Header\n");
        printf (
          "    GUID                %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
          OemHeader->OemGuid.Data1,
          (UINT32) OemHeader->OemGuid.Data2,
          (UINT32) OemHeader->OemGuid.Data3,
          (UINT32) OemHeader->OemGuid.Data4[0],
          (UINT32) OemHeader->OemGuid.Data4[1],
          (UINT32) OemHeader->OemGuid.Data4[2],
          (UINT32) OemHeader->OemGuid.Data4[3],
          (UINT32) OemHeader->OemGuid.Data4[4],
          (UINT32) OemHeader->OemGuid.Data4[5],
          (UINT32) OemHeader->OemGuid.Data4[6],
          (UINT32) OemHeader->OemGuid.Data4[7]
          );
        printf ("    Header size:        0x%X\n", OemHeader->HeaderSize);
        printf ("    OEM data");
        BPtr = (UINT8 *) (OemHeader + 1);
        for (ByteCount = 0; ByteCount < OemHeader->HeaderSize - sizeof (EFI_CAPSULE_OEM_HEADER); ByteCount++) {
          if ((ByteCount & 0x7) == 0) {
            printf ("\n      ");
          }

          printf ("%02X ", (UINT32) *BPtr);
          BPtr++;
        }

        printf ("\n");
      }
      //
      // ************************************************************************
      //
      // Author, revision, short description, and long description information
      //
      // ************************************************************************
      //
      if (CapsuleHeader.OffsetToAuthorInformation != 0) {
        if (DumpCapsuleHeaderStrings (
              "Author information",
              (WCHAR *) (CapsuleHeaderData + CapsuleHeader.OffsetToAuthorInformation - CapsuleHeader.HeaderSize)
              ) != STATUS_SUCCESS) {
          goto Done;
        }
      }

      if (CapsuleHeader.OffsetToRevisionInformation != 0) {
        if (DumpCapsuleHeaderStrings (
              "Revision information",
              (WCHAR *) (CapsuleHeaderData + CapsuleHeader.OffsetToRevisionInformation - CapsuleHeader.HeaderSize)
              ) != STATUS_SUCCESS) {
          goto Done;
        }
      }

      if (CapsuleHeader.OffsetToShortDescription != 0) {
        if (DumpCapsuleHeaderStrings (
              "Short description",
              (WCHAR *) (CapsuleHeaderData + CapsuleHeader.OffsetToShortDescription - CapsuleHeader.HeaderSize)
              ) != STATUS_SUCCESS) {
          goto Done;
        }
      }

      if (CapsuleHeader.OffsetToLongDescription != 0) {
        if (DumpCapsuleHeaderStrings (
              "Long description",
              (WCHAR *) (CapsuleHeaderData + CapsuleHeader.OffsetToLongDescription - CapsuleHeader.HeaderSize)
              ) != STATUS_SUCCESS) {
          goto Done;
        }
      }
    }
    //
    // If it's not a split image, or it is a split image and this is the first in the series, then
    // dump the cargo volume.
    //
    if ((!SplitImage) || (CapsuleHeader.SequenceNumber == 0)) {
      printf ("  Cargo FV dump\n");
      fseek (InFptr, CapsuleHeader.OffsetToCapsuleBody, SEEK_SET);
      if (fread (&FVHeader, sizeof (EFI_FIRMWARE_VOLUME_HEADER), 1, InFptr) != 1) {
        Error (NULL, 0, 0, FileList->FileName, "failed to read cargo FV header");
        goto Done;
      }

      printf ("    FV length           0x%X", FVHeader.FvLength);
      if (FileSize - CapsuleHeader.OffsetToCapsuleBody != FVHeader.FvLength) {
        if (!SplitImage) {
          printf (" ERROR: expected 0x%X to jive with file size on disk", FileSize - CapsuleHeader.OffsetToCapsuleBody);
        }
      }

      printf ("\n");
      printf ("    Signature           0x%X ", FVHeader.Signature);
      if (FVHeader.Signature == EFI_FVH_SIGNATURE) {
        printf ("_FVH\n");
      } else {
        printf ("INVALID\n");
      }

      printf ("    FV header length    0x%X\n", (UINT32) FVHeader.HeaderLength);
      printf ("    Revision            0x%X\n", (UINT32) FVHeader.Revision);
      printf ("\n");
    }

    FileList = FileList->Next;
  }

Done:
  if (InFptr != NULL) {
    fclose (InFptr);
  }

  if (CapsuleHeaderData != NULL) {
    free (CapsuleHeaderData);
  }
#endif
}

static
STATUS
JoinCapsule (
  VOID
  )
/*++

Routine Description:
  Join split capsule images into a single image. This is the
  support function for the -j command-line option.

Arguments:
  None.

Returns:
  STATUS_SUCCESS - no problems encountered

--*/
{
#if 0
  UINT32              Size;
  FILE                *InFptr;
  FILE                *OutFptr;
  INT8                *Buffer;
  FILE_LIST           *FileList;
  STATUS              Status;
  EFI_CAPSULE_HEADER  CapHdr;
  EFI_CAPSULE_HEADER  *CapHdrPtr;
  UINT32              SizeLeft;
  UINT32              SequenceNumber;
  //
  // Must have at least two files for join mode
  //
  if ((mOptions.FileList == NULL) || (mOptions.FileList->Next == NULL)) {
    Error (NULL, 0, 0, "must specify at least two file names to join", NULL);
    return STATUS_ERROR;
  }
  //
  // Open the output file
  //
  if ((OutFptr = fopen (mOptions.OutputFileName, "wb")) == NULL) {
    Error (NULL, 0, 0, mOptions.OutputFileName, "failed to open output file for writing");
    return STATUS_ERROR;
  }

  FileList        = mOptions.FileList;
  Buffer          = NULL;
  SequenceNumber  = 0;
  InFptr          = NULL;
  SizeLeft        = 0;
  while (FileList != NULL) {
    if ((InFptr = fopen (FileList->FileName, "rb")) == NULL) {
      Error (NULL, 0, 0, FileList->FileName, "failed to open file for reading");
      goto FailDone;
    }
    //
    // Allocate a buffer into which we can read the file.
    //
    fseek (InFptr, 0, SEEK_END);
    Size = ftell (InFptr);
    rewind (InFptr);
    Buffer = (char *) malloc (Size);
    if (Buffer == NULL) {
      Error (__FILE__, __LINE__, 0, FileList->FileName, "failed to allocate buffer to read file into");
      goto FailDone;
    }

    CapHdrPtr = (EFI_CAPSULE_HEADER *) Buffer;
    if (fread ((void *) Buffer, Size, 1, InFptr) != 1) {
      Error (NULL, 0, 0, FileList->FileName, "failed to read file contents");
      goto FailDone;
    }
    //
    // Check the header for validity. Check size first.
    //
    if (Size < sizeof (EFI_CAPSULE_HEADER)) {
      Error (NULL, 0, 0, FileList->FileName, "file size is insufficient for a capsule header");
      goto FailDone;
    }
    //
    // Check GUID
    //
    if (memcmp (&CapHdrPtr->CapsuleGuid, &mEfiCapsuleHeaderGuid, sizeof (EFI_GUID)) != 0) {
      Error (NULL, 0, 0, FileList->FileName, "invalid capsule GUID");
      goto FailDone;
    }
    //
    // Check sequence number
    //
    if (CapHdrPtr->SequenceNumber != SequenceNumber) {
      Error (
        NULL,
        0,
        0,
        FileList->FileName,
        "invalid sequence number %d (expected %d)",
        CapHdrPtr->SequenceNumber,
        SequenceNumber
        );
      goto FailDone;
    }
    //
    // If the first file, read save the capsule header
    //
    if (SequenceNumber == 0) {
      memcpy (&CapHdr, CapHdrPtr, sizeof (EFI_CAPSULE_HEADER));
      //
      // Erase the InstanceId GUID
      //
      memset (&CapHdrPtr->InstanceId, 0, sizeof (EFI_GUID));
      if (fwrite (Buffer, Size, 1, OutFptr) != 1) {
        Error (NULL, 0, 0, FileList->FileName, "failed to write contents to output file");
        goto FailDone;
      }

      if (CapHdr.CapsuleImageSize < Size) {
        Error (NULL, 0, 0, FileList->FileName, "capsule image size in capsule header < image size");
        goto FailDone;
      }

      SizeLeft = CapHdr.CapsuleImageSize - Size;
    } else {
      //
      // Check the GUID against the first file's GUID
      //
      if (memcmp (&CapHdr.CapsuleGuid, &CapHdrPtr->CapsuleGuid, sizeof (EFI_GUID)) != 0) {
        Error (NULL, 0, 0, FileList->FileName, "GUID does not match first file's GUID");
        goto FailDone;
      }
      //
      // Make sure we're not throwing out any header info
      //
      if (CapHdrPtr->OffsetToCapsuleBody > sizeof (EFI_CAPSULE_HEADER)) {
        //
        // Could be the split information, so just emit a warning
        //
        Warning (
          NULL,
          0,
          0,
          FileList->FileName,
          "image appears to have additional capsule header information -- ignoring"
          );
      } else if (CapHdrPtr->OffsetToCapsuleBody < sizeof (EFI_CAPSULE_HEADER)) {
        Error (NULL, 0, 0, FileList->FileName, "offset to capsule body in capsule header is insufficient");
        goto FailDone;
      }

      if (fwrite (Buffer + CapHdrPtr->OffsetToCapsuleBody, Size - CapHdrPtr->OffsetToCapsuleBody, 1, OutFptr) != 1) {
        Error (NULL, 0, 0, mOptions.OutputFileName, "failed to write to file");
        goto FailDone;
      }

      if (SizeLeft < (Size - CapHdrPtr->OffsetToCapsuleBody)) {
        Error (NULL, 0, 0, "sum of image sizes exceeds size specified in initial capsule header", NULL);
        goto FailDone;
      }
      //
      // printf ("FILE: %s OffsetToCapsuleBody=0x%X\n", FileList->FileName, CapHdrPtr->OffsetToCapsuleBody);
      //
      SizeLeft = SizeLeft - (Size - CapHdrPtr->OffsetToCapsuleBody);
    }
    //
    // printf ("FILE: %s sizeleft=0x%X\n", FileList->FileName, SizeLeft);
    //
    fclose (InFptr);
    InFptr = NULL;
    free (Buffer);
    Buffer    = NULL;
    FileList  = FileList->Next;
    SequenceNumber++;
  }

  if (SizeLeft) {
    Error (NULL, 0, 0, "sum of capsule images is insufficient", "0x%X bytes missing", SizeLeft);
    goto FailDone;
  }

  Status = STATUS_SUCCESS;
  goto Done;
FailDone:
  Status = STATUS_ERROR;
Done:
  if (InFptr != NULL) {
    fclose (InFptr);
  }

  if (OutFptr != NULL) {
    fclose (OutFptr);
  }

  if (Buffer != NULL) {
    free (Buffer);
  }

  return Status;

#endif
return STATUS_SUCCESS;
}

static
STATUS
DumpCapsuleHeaderStrings (
  UINT8   *SectionName,
  WCHAR   *Buffer
  )
/*++

Routine Description:
  Given a pointer to string data from a capsule header, dump
  the strings.

Arguments:
  SectionName  - name of the capsule header section to which
                 the string data pertains
  Buffer       - pointer to string data from a capsule header

Returns:
  STATUS_SUCCESS - all went well

--*/
{
  printf ("  %s\n", SectionName);
  while (*Buffer) {
    printf ("    Language: %S\n", Buffer);
    while (*Buffer) {
      Buffer++;
    }

    Buffer++;
    while (*Buffer) {
      if (wcslen (Buffer) > 60) {
        printf ("      %.60S\n", Buffer);
        Buffer += 60;
      } else {
        printf ("      %S\n", Buffer);
        Buffer += wcslen (Buffer);
      }
    }

    Buffer++;
  }

  return STATUS_SUCCESS;
}

static
STATUS
GetHexValue (
  SOURCE_FILE  *SourceFile,
  UINT32       *Value,
  UINT32       NumDigits
  )
/*++

Routine Description:
  Scan a hex value from the input stream.
  
Arguments:
  SourceFile   - input file contents
  Value        - returned value
  NumDigits    - number of digits to read

Returns:
  STATUS_SUCCESS - if NumDigits were read from the file
  STATUS_ERROR   - otherwise

  
--*/
{
  WCHAR   *SaveFilePos;
  UINT32  Digits;
  WCHAR   Nibble;

  SaveFilePos = SourceFile->FileBufferPtr;
  *Value      = 0;
  Digits      = NumDigits;
  while (Digits > 0) {
    Nibble = SourceFile->FileBufferPtr[0];
    if ((Nibble >= UNICODE_0) && (Nibble <= UNICODE_9)) {
      *Value = (*Value << 4) | (Nibble - UNICODE_0);
    } else if ((Nibble >= UNICODE_A) && (Nibble <= UNICODE_F)) {
      *Value = (*Value << 4) | (Nibble - UNICODE_A + 0x10);
    } else if ((Nibble >= UNICODE_a) && (Nibble <= UNICODE_f)) {
      *Value = (*Value << 4) | (Nibble - UNICODE_a + 0x10);
    } else {
      Error (
        SourceFile->FileName,
        SourceFile->LineNum,
        0,
        NULL,
        "expected %d valid hex nibbles at %.20S",
        NumDigits,
        SaveFilePos
        );
      return STATUS_ERROR;
    }

    SourceFile->FileBufferPtr++;
    Digits--;
  }

  return STATUS_SUCCESS;
}

static
BOOLEAN
EndOfFile (
  SOURCE_FILE *File
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  File  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  if ((UINT32) File->FileBufferPtr - (UINT32) File->FileBuffer >= File->FileSize) {
    File->EndOfFile = TRUE;
  }
  //
  // Reposition to the end of the file if we went beyond
  //
  if (File->EndOfFile) {
    File->FileBufferPtr = File->FileBuffer + File->FileSize / sizeof (WCHAR);
  }

  return File->EndOfFile;
}

static
void
SkipWhiteSpace (
  SOURCE_FILE *SourceFile
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  SourceFile  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  while (!EndOfFile (SourceFile)) {
    switch (*SourceFile->FileBufferPtr) {
    case UNICODE_NULL:
    case UNICODE_CR:
    case UNICODE_SPACE:
    case UNICODE_TAB:
      SourceFile->FileBufferPtr++;
      break;

    case UNICODE_LF:
      SourceFile->FileBufferPtr++;
      SourceFile->LineNum++;
      break;

    default:
      return ;
    }
  }
}
//
// Parse a number. Possible format:
//   1234
//   1234k
//   1234K
//   1M
//   1m
//   0x100
//
static
BOOLEAN
GetNumber (
  INT8    *Str,
  UINT32  *Value
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Str   - GC_TODO: add argument description
  Value - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  UINT32  LValue;

  *Value  = 0;
  LValue  = 0;
  if (!isdigit (Str[0])) {
    return FALSE;
  }
  //
  // Look for hex number
  //
  if ((Str[0] == '0') && (tolower (Str[1]) == 'x')) {
    Str += 2;
    if (Str[0] == 0) {
      return FALSE;
    }

    while (Str[0]) {
      if ((Str[0] >= '0') && (Str[0] <= '9')) {
        LValue = (LValue << 4) | (Str[0] - '0');
      } else if ((Str[0] >= 'A') && (Str[0] <= 'F')) {
        LValue = (LValue << 4) | (Str[0] - 'A' + 0x10);
      } else if ((Str[0] >= 'a') && (Str[0] <= 'f')) {
        LValue = (LValue << 4) | (Str[0] - 'a' + 0x10);
      } else {
        break;
      }

      Str++;
    }
  } else {
    LValue = atoi (Str);
    while (isdigit (*Str)) {
      Str++;
    }
  }
  //
  // If string left over, better be one character we recognize
  //
  if (Str[0]) {
    if (Str[1]) {
      return FALSE;
    }

    switch (Str[0]) {
    case 'k':
    case 'K':
      LValue *= 1024;
      break;

    case 'm':
    case 'M':
      LValue *= 1024 * 1024;
      break;

    default:
      return FALSE;
    }
  }

  *Value = LValue;
  return TRUE;
}
//
// Process the command-line arguments
//
static
STATUS
ProcessArgs (
  int   Argc,
  char  *Argv[]
  )
/*++

Routine Description:

  Processes command line arguments.

Arguments:

  Argc   - Number of command line arguments
  Argv[] - Array of files input on command line  

Returns:

  STATUS_ERROR    - Function exited with an error
  STATUS_SUCCESS  - Function executed successfully

--*/
{
  FILE_LIST *NewFile;

  FILE_LIST *LastFile;
  SIZE_LIST *NewSize;
  
  NewFile = NULL;
  NewSize = NULL;
  
  //
  // Clear our globals
  //
  memset ((char *) &mOptions, 0, sizeof (mOptions));

  //
  // Skip program name
  //
  Argc--;
  Argv++;

  if (Argc == 0) {
    Usage ();
    return STATUS_ERROR;
  }
  
  if ((strcmp(Argv[0], "-h") == 0) || (strcmp(Argv[0], "--help") == 0) ||
      (strcmp(Argv[0], "-?") == 0) || (strcmp(Argv[0], "/?") == 0)) {
    Usage();
    return STATUS_ERROR;
  }
  
  if ((strcmp(Argv[0], "-V") == 0) || (strcmp(Argv[0], "--version") == 0)) {
    Version();
    return STATUS_ERROR;
  }
  
  if (Argc == 1) {
    Usage ();
    return STATUS_ERROR;
  }
 
  //
  // Process until no more options
  //
  while ((Argc > 0) && (Argv[0][0] == '-')) {
    if (stricmp (Argv[0], "-script") == 0) {
      //
      // Check for one more arg
      //
      if (Argc > 1) {
        //
        // Save the file name
        //
        if (strlen (Argv[1]) >= sizeof (mOptions.ScriptFileName)) {
          Error (NULL, 0, 0, NULL, "input script file name length exceeds internal buffer size");
          
          if (NewFile != NULL) {
            free (NewFile);
          }
          if (NewSize != NULL) {
            free (NewSize);
          }
          
          return STATUS_ERROR;
        }

        strcpy (mOptions.ScriptFileName, Argv[1]);
      } else {
        Error (NULL, 0, 0, Argv[0], "missing script file name with option");
        Usage ();
        
        if (NewFile != NULL) {
          free (NewFile);
        }
        if (NewSize != NULL) {
          free (NewSize);
        }       

        return STATUS_ERROR;
      }

      Argc--;
      Argv++;
      //
      // -o outfilename -- specify output file name (required)
      //
    } else if (stricmp (Argv[0], "-o") == 0) {
      //
      // check for one more arg
      //
      if (Argc > 1) {
        //
        // Try to open the file
        //
        // if ((mOptions.OutFptr = fopen (Argv[1], "wb")) == NULL) {
        //  Error (NULL, 0, 0, Argv[1], "failed to open output file for writing");
        //  return STATUS_ERROR;
        // }
        //
        strcpy (mOptions.OutputFileName, Argv[1]);
      } else {
        Error (NULL, 0, 0, Argv[0], "missing output filename with option");
        Usage ();
        
        if (NewFile != NULL) {
          free (NewFile);
        }
        if (NewSize != NULL) {
          free (NewSize);
        }
                
        return STATUS_ERROR;
      }

      Argc--;
      Argv++;
    } else if (stricmp (Argv[0], "-j") == 0) {
      mOptions.JoinMode = TRUE;
      //
      // -split <size> option (multiple allowed)
      //
    } else if (stricmp (Argv[0], "-split") == 0) {
      if (Argc > 1) {
        NewSize = (SIZE_LIST *) malloc (sizeof (SIZE_LIST));
        if (NewSize == NULL) {
          Error (NULL, 0, 0, "memory allocation failure", NULL);

          if (NewFile != NULL) {
            free (NewFile);
          }
          if (NewSize != NULL) {
            free (NewSize);
          }
          
          return STATUS_ERROR;
        }

        memset (NewSize, 0, sizeof (SIZE_LIST));
        //
        // Get the size from the next arg, and then add this size
        // to our size list
        //
        if (!GetNumber (Argv[1], &NewSize->Size)) {
          Error (NULL, 0, 0, Argv[1], "invalid split size argument");

          if (NewFile != NULL) {
            free (NewFile);
          }
          if (NewSize != NULL) {
            free (NewSize);
          }       
          
          return STATUS_ERROR;
        }

        if (mOptions.SizeList == NULL) {
          mOptions.SizeList     = NewSize;
          mOptions.CurrentSize  = NewSize;
        } else {
          mOptions.LastSize->Next = NewSize;
        }

        mOptions.LastSize = NewSize;
        free (NewSize);
      } else {
        Error (NULL, 0, 0, Argv[0], "missing size parameter with option");
        Usage ();

        if (NewFile != NULL) {
          free (NewFile);
        }
        if (NewSize != NULL) {
          free (NewSize);
        }
        
        return STATUS_ERROR;
      }

      Argc--;
      Argv++;      
    } else if ((stricmp (Argv[0], "-h") == 0) || (strcmp (Argv[0], "-?") == 0)) {
      Usage ();
      
      if (NewFile != NULL) {
        free (NewFile);
      }
      if (NewSize != NULL) {
        free (NewSize);
      }
      
      return STATUS_ERROR;
      //
      // Default minimum header
      //
    } else if (stricmp (Argv[0], "-dump") == 0) {
      mOptions.Dump = TRUE;
    } else if (stricmp (Argv[0], "-v") == 0) {
      mOptions.Verbose = TRUE;
    } else {
      Error (NULL, 0, 0, Argv[0], "unrecognized option");
      Usage ();
      
      if (NewFile != NULL) {
        free (NewFile);
      }
      if (NewSize != NULL) {
        free (NewSize);
      }
      
      return STATUS_ERROR;
    }

    Argc--;
    Argv++;
  }
  //
  // Can't -j join files and -s split output capsule
  //
  if ((mOptions.SizeList != NULL) && (mOptions.JoinMode)) {
    Error (NULL, 0, 0, "cannot specify both -j and -size", NULL);
    
    if (NewFile != NULL) {
      free (NewFile);
    }
    if (NewSize != NULL) {
      free (NewSize);
    }
    
    return STATUS_ERROR;
  }
  //
  // Must have specified an output file name if not -dump
  //
  if ((mOptions.Dump == 0) && (mOptions.OutputFileName[0] == 0)) {
    Error (NULL, 0, 0, NULL, "-o OutputFileName must be specified");
    Usage ();
    
    if (NewFile != NULL) {
      free (NewFile);
    }
    if (NewSize != NULL) {
      free (NewSize);
    }
    
    return STATUS_ERROR;
  }
  //
  // Rest of arguments are input files. The first one is a firmware
  // volume image, and the rest are FFS files that are to be inserted
  // into the firmware volume.
  //
  LastFile = NULL;
  while (Argc > 0) {
    NewFile = (FILE_LIST *) malloc (sizeof (FILE_LIST));
    if (NewFile == NULL) {
      Error (NULL, 0, 0, "memory allocation failure", NULL);

      if (NewFile != NULL) {
        free (NewFile);
      }
      if (NewSize != NULL) {
        free (NewSize);
      }
    
      return STATUS_ERROR;
    }

    memset ((char *) NewFile, 0, sizeof (FILE_LIST));
    strcpy (NewFile->FileName, Argv[0]);
    if (mOptions.FileList == NULL) {
      mOptions.FileList = NewFile;
    } else {
      if (LastFile == NULL) {
        LastFile = NewFile;
      } else {
        LastFile->Next = NewFile;
      }      
    }

    LastFile = NewFile;
    Argc--;
    Argv++;
  }

  //
  // Must have provided at least one file name
  //
  if (mOptions.FileList == NULL) {
    Error (NULL, 0, 0, "must specify at least one file name", NULL);
    Usage ();
    
    if (NewFile != NULL) {
      free (NewFile);
    }
    if (NewSize != NULL) {
      free (NewSize);
    }
    
    return STATUS_ERROR;
  }

  return STATUS_SUCCESS;
}

static
void 
Version(
  void
  )
/*++

Routine Description:

  Print out version information for this utility.

Arguments:

  None
  
Returns:

  None
  
--*/ 
{
  printf ("%s v%d.%d -EDK utility to create a capsule header.\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION);
  printf ("Copyright (c) 1999-2006 Intel Corporation. All rights reserved.\n");
}

static
void
Usage (
  VOID
  )
/*++

Routine Description:

  Print usage information for this utility.
  
Arguments:

  None.

Returns:

  Nothing.
  
--*/
{
  int               Index;
  static const char *Str[] = {
    "\nUsage: "UTILITY_NAME " {options} [CapsuleFV]",
    //
    // {FfsFileNames}",
    //
    "    Options include:",
    "      -h,--help,-?,/?  to display help messages",
    "      -V,--version     to display version information",
    "      -script fname    to take capsule header info from unicode script",
    "                       file fname",
    "      -o fname         write output to file fname (required)",
    "      -split size      split capsule image into multiple output files",
    "      -dump            to dump a capsule header",
    "      -v               for verbose output\n",
    "      -j               to join split capsule images into a single image",
    "",
    "    CapsuleFV is the name of an existing well-formed Tiano firmware",
    "    volume file.",
    //
    // FfsFileNames are the names of one or more Tiano FFS files to",
    // "    insert into the output capsule image.",
    //
    NULL
  };
  
  Version();
  
  for (Index = 0; Str[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Str[Index]);
  }
}
