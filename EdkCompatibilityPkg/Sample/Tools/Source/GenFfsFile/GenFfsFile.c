/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GenFfsFile.c

Abstract:

  This file contains functions required to generate a Firmware File System
  file.

--*/

#include "TianoCommon.h"
#include "EfiFirmwareFileSystem.h"
#include "EfiFirmwareVolumeHeader.h"
#include "EfiImageFormat.h"
#include "EfiImage.h"
#include "ParseInf.h"
#include "Compress.h"
#include "EfiCustomizedCompress.h"
#include "crc32.h"
#include "GenFfsFile.h"
#include <stdio.h>
#include <ctype.h>  // for isalpha()
//
// include file for _spawnv
//
#include <process.h>
#include <stdlib.h>
#include <string.h>
#include "CommonLib.h"
#include "EfiUtilityMsgs.h"
#include "SimpleFileParsing.h"

#define UTILITY_NAME    "GenFfsFile"
#define UTILITY_VERSION "v1.1"
#define MAX_ARRAY_SIZE  100

static
INT32
GetNextLine (
  OUT CHAR8       *Destination,
  IN FILE         *Package,
  IN OUT UINT32   *LineNumber
  );

static
void
CheckSlash (
  IN OUT CHAR8  *String,
  IN FILE       *In,
  IN OUT UINT32 *LineNumber
  );

static
INT32
FindSectionInPackage (
  IN CHAR8        *BuildDirectory,
  IN FILE         *OverridePackage,
  IN OUT UINT32   *LineNumber
  );

static
STATUS
ProcessCommandLineArgs (
  int     Argc,
  char    *Argv[]
  );

static
void
PrintUsage (
  void
  );

static
void
AddMacro (
  UINT8   *MacroString
  );

static
UINT8 *
GetMacroValue (
  UINT8   *MacroName
  );
    
static
void
FreeMacros (
  );

static
STATUS
ReplaceMacros (
  UINT8   *InputFile,
  UINT8   *OutputFile
  );
    
//
// Linked list to keep track of all macros
//
typedef struct _MACRO {
  struct _MACRO   *Next;
  UINT8           *Name;
  UINT8           *Value;
} MACRO;

//
// Keep globals in this structure
//
static struct {
  UINT8   BuildDirectory[_MAX_PATH];
  UINT8   PrimaryPackagePath[_MAX_PATH];
  UINT8   OverridePackagePath[_MAX_PATH];
  UINT8   OutputFilePath[_MAX_PATH];
  BOOLEAN Verbose;
  MACRO   *MacroList;
} mGlobals;

static EFI_GUID mZeroGuid = { 0 };

static UINT8  MinFfsDataAlignOverride = 0;

static
void
StripQuotes (
  IN OUT CHAR8 *String
  )
/*++

Routine Description:

  Removes quotes and/or whitespace from around a string

Arguments:

 String    - String to remove quotes from

Returns:

  None

--*/
{
  UINTN Index;
  UINTN Index2;
  UINTN StrLen;

  Index2  = strspn (String, "\" \t\n");
  StrLen  = strlen (String);

  for (Index = Index2; String[Index] != '\"', Index < StrLen; Index++) {
    String[Index - Index2] = String[Index];
  }

  String[Index - Index2] = 0;
}

static
void
PrintUsage (
  void
  )
/*++

Routine Description:

  Print Error / Help message.

Arguments:

  void

Returns:

  None

--*/
{
  int         Index;
  const char  *Str[] = {
    UTILITY_NAME" "UTILITY_VERSION" - Intel Generate FFS File Utility",
    "  Copyright (C), 2004 - 2009 Intel Corporation",
    
#if ( defined(UTILITY_BUILD) && defined(UTILITY_VENDOR) )
    "  Built from "UTILITY_BUILD", project of "UTILITY_VENDOR,
#endif
    "",
    "Usage:",
    "  "UTILITY_NAME" [OPTION]...",
    "Options:",
    "  -b BuildDirectory  Specifies the full path to the component build directory",
    "  -p1 P1Path         Specifies fully qualified file name to the primary package",
    "                     file. This file will normally exist in the same directory",
    "                     as the makefile for the component. Required.",
    "  -p2 P2Path         Specifies fully qualified file name to the override",
    "                     package. This file will normally exist in the build tip.",
    "                     Optional.",
    "  -d Name=Value      Add a macro definition for the package file. Optional.",
    "  -o OutputFile      Specifies the file name of output file. Optional.",
    "  -v                 Verbose. Optional.",
    NULL
  };
  for (Index = 0; Str[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Str[Index]);
  }
}

static
INT32
TestComment (
  IN CHAR8  *String,
  IN FILE   *In
  )
/*++

Routine Description:

  Tests input string to see if it is a comment, and if so goes to the next line in the file that is not a comment

Arguments:

  String      - String to test

  In          - Open file to move pointer within

Returns:

  -1          - End of file reached
   0          - Not a comment
   1          - Comment bypassed

--*/
{
  CHAR8 CharBuffer;

  CharBuffer = 0;
  if ((String[0] == '/') && (String[1] == '/')) {
    while (CharBuffer != '\n') {
      fscanf (In, "%c", &CharBuffer);
      if (feof (In)) {
        return -1;
      }
    }
  } else {
    return 0;
  }

  return 1;
}

static
void
BreakString (
  IN CONST CHAR8 *Source,
  OUT CHAR8      *Destination,
  IN INTN        Direction
  )
/*++

Routine Description:

  Takes an input string and returns either the part before the =, or the part after the =, depending on direction

Arguments:

  Source      - String to break

  Destination - Buffer to place new string in

  Direction   - 0 to return all of source string before =
                1 to return all of source string after =

Returns:

  None

--*/
{
  UINTN Index;
  UINTN Index2;

  Index   = 0;
  Index2  = 0;

  if (strchr (Source, '=') == NULL) {
    strcpy (Destination, Source);

    return ;
  }

  if (Direction == 0) {
    //
    // return part of string before =
    //
    while (Source[Index] != '=') {
      Destination[Index] = Source[Index++];
    }

    Destination[Index] = 0;
  } else {
    //
    // return part of string after =
    //
    strcpy (Destination, strchr (Source, '=') + 1);
  }
}

static
INT32
GetNextLine (
  OUT CHAR8       *Destination,
  IN FILE         *Package,
  IN OUT UINT32   *LineNumber
  )
/*++

Routine Description:

  Gets the next non-commented line from the file

Arguments:

  Destination - Where to put string

  Package     - Package to get string from
  
  LineNumber  - The actual line number.

Returns:

  -1          - End of file reached
   0          - Success

--*/
{
  CHAR8 String[_MAX_PATH];
  fscanf (Package, "%s", &String);
  if (feof (Package)) {
    return -1;
  }

  while (TestComment (String, Package) == 1) {
    fscanf (Package, "%s", &String);
    if (feof (Package)) {
      return -1;
    }
  }

  strcpy (Destination, String);
  return 0;
}

static
VOID
CheckSlash (
  IN OUT CHAR8  *String,
  IN FILE       *In,
  IN OUT UINT32 *LineNumber
  )
/*++

Routine Description:

  Checks to see if string is line continuation character, if so goes to next valid line

Arguments:

  String      - String to test

  In          - Open file to move pointer within
  
  LineNumber  - The line number.

Returns:

  None

--*/
{
  CHAR8 ByteBuffer;
  ByteBuffer = 0;

  switch (String[0]) {

  case '\\':
    while (String[0] == '\\') {
      while (ByteBuffer != '\n') {
        fscanf (In, "%c", &ByteBuffer);
      }
      (*LineNumber)++;
      if (GetNextLine (String, In, LineNumber) == -1) {
        return ;
      }
    }
    break;

  case '\n':
    (*LineNumber)++;
    while (String[0] == '\n') {
      if (GetNextLine (String, In, LineNumber) == -1) {
        return ;
      }
    }
    break;

  default:
    break;

  }

}

static
INT32
FindSectionInPackage (
  IN CHAR8        *BuildDirectory,
  IN FILE         *OverridePackage,
  IN OUT UINT32   *LineNumber
  )
/*++

Routine Description:

  Finds the matching section within the package

Arguments:

  BuildDirectory  - name of section to find

  OverridePackage - Package file to search within
  
  LineNumber      - The line number.

Returns:

  -1          - End of file reached
   0          - Success

--*/
{
  CHAR8 String[_MAX_PATH];
  CHAR8 NewString[_MAX_PATH];
  String[0] = 0;

  while (strcmp (BuildDirectory, String) != 0) {
    if (GetNextLine (NewString, OverridePackage, LineNumber) != 0) {
      return -1;
    }

    if (NewString[0] == '[') {
      if (NewString[strlen (NewString) - 1] != ']') {
        //
        // have to construct string.
        //
        strcpy (String, NewString + 1);

        while (1) {
          fscanf (OverridePackage, "%s", &NewString);
          if (feof (OverridePackage)) {
            return -1;
          }

          if (NewString[0] != ']') {
            if (strlen (String) != 0) {
              strcat (String, " ");
            }

            strcat (String, NewString);
            if (String[strlen (String) - 1] == ']') {
              String[strlen (String) - 1] = 0;
              break;
            }
          } else {
            break;
          }
        }
      } else {
        NewString[strlen (NewString) - 1] = 0;
        strcpy (String, NewString + 1);
      }
    }
  }

  return 0;
}

static
EFI_STATUS
GenSimpleGuidSection (
  IN OUT UINT8  *FileBuffer,
  IN OUT UINT32 *BufferSize,
  IN UINT32     DataSize,
  IN EFI_GUID   SignGuid,
  IN UINT16     GuidedSectionAttributes
  )
/*++

Routine Description:

  add GUIDed section header for the data buffer.
  data stays in same location (overwrites source data).

Arguments:

  FileBuffer  - Buffer containing data to sign

  BufferSize  - On input, the size of FileBuffer. On output, the size of
                actual section data (including added section header).

  DataSize    - Length of data to Sign

  SignGuid    - Guid to be add.
  
  GuidedSectionAttributes - The section attribute.

Returns:

  EFI_SUCCESS           - Successful
  EFI_OUT_OF_RESOURCES  - Not enough resource.

--*/
{
  UINT32                    TotalSize;

  EFI_GUID_DEFINED_SECTION  GuidSectionHeader;
  UINT8                     *SwapBuffer;

  SwapBuffer = NULL;

  if (DataSize == 0) {
    *BufferSize = 0;

    return EFI_SUCCESS;
  }

  TotalSize = DataSize + sizeof (EFI_GUID_DEFINED_SECTION);
  GuidSectionHeader.CommonHeader.Type     = EFI_SECTION_GUID_DEFINED;
  GuidSectionHeader.CommonHeader.Size[0]  = (UINT8) (TotalSize & 0xff);
  GuidSectionHeader.CommonHeader.Size[1]  = (UINT8) ((TotalSize & 0xff00) >> 8);
  GuidSectionHeader.CommonHeader.Size[2]  = (UINT8) ((TotalSize & 0xff0000) >> 16);
  memcpy (&(GuidSectionHeader.SectionDefinitionGuid), &SignGuid, sizeof (EFI_GUID));
  GuidSectionHeader.Attributes  = GuidedSectionAttributes;
  GuidSectionHeader.DataOffset  = sizeof (EFI_GUID_DEFINED_SECTION);

  SwapBuffer                    = (UINT8 *) malloc (DataSize);
  if (SwapBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  memcpy (SwapBuffer, FileBuffer, DataSize);
  memcpy (FileBuffer, &GuidSectionHeader, sizeof (EFI_GUID_DEFINED_SECTION));
  memcpy (FileBuffer + sizeof (EFI_GUID_DEFINED_SECTION), SwapBuffer, DataSize);

  //
  // Make sure section ends on a DWORD boundary
  //
  while ((TotalSize & 0x03) != 0) {
    FileBuffer[TotalSize] = 0;
    TotalSize++;
  }

  *BufferSize = TotalSize;

  if (SwapBuffer != NULL) {
    free (SwapBuffer);
  }

  return EFI_SUCCESS;
}

static
EFI_STATUS
CompressSection (
  UINT8  *FileBuffer,
  UINT32 *BufferSize,
  UINT32 DataSize,
  CHAR8  *Type
  )
/*++

Routine Description:

  Compress the data and add section header for the compressed data.
  Compressed data (with section header) stays in same location as the source
  (overwrites source data).

Arguments:

  FileBuffer  - Buffer containing data to Compress

  BufferSize  - On input, the size of FileBuffer. On output, the size of
                actual compressed data (including added section header).
                When buffer is too small, this value indicates the size needed.

  DataSize    - The size of data to compress

  Type        - The compression type (not used currently).
                Assume EFI_HEAVY_COMPRESSION.

Returns:

  EFI_BUFFER_TOO_SMALL - Buffer size is too small.
  EFI_UNSUPPORTED      - Compress type can not be supported.
  EFI_SUCCESS          - Successful
  EFI_OUT_OF_RESOURCES - Not enough resource.

--*/
{
  EFI_STATUS              Status;
  UINT8                   *CompData;
  UINT32                  CompSize;
  UINT32                  TotalSize;
  EFI_COMPRESSION_SECTION CompressionSet;
  UINT8                   CompressionType;
  COMPRESS_FUNCTION       CompressFunction;

  Status            = EFI_SUCCESS;
  CompData          = NULL;
  CompSize          = 0;
  TotalSize         = 0;
  CompressFunction  = NULL;

  //
  // Get the compress type
  //
  if (_strcmpi (Type, "Dummy") == 0) {
    //
    // Added "Dummy" to keep backward compatibility.
    //
    CompressionType   = EFI_STANDARD_COMPRESSION;
    CompressFunction  = (COMPRESS_FUNCTION) TianoCompress;

  } else if (_strcmpi (Type, "LZH") == 0) {
    //
    // EFI stardard compression (LZH)
    //
    CompressionType   = EFI_STANDARD_COMPRESSION;
    CompressFunction  = (COMPRESS_FUNCTION) TianoCompress;

  } else {
    //
    // Customized compression
    //
    Status = SetCustomizedCompressionType (Type);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    CompressionType   = EFI_CUSTOMIZED_COMPRESSION;
    CompressFunction  = (COMPRESS_FUNCTION) CustomizedCompress;
  }
  //
  // Compress the raw data
  //
  Status = CompressFunction (FileBuffer, DataSize, CompData, &CompSize);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    CompData = malloc (CompSize);
    if (!CompData) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = CompressFunction (FileBuffer, DataSize, CompData, &CompSize);
  }

  if (EFI_ERROR (Status)) {
    if (CompData != NULL) {
      free (CompData);
    }

    return Status;
  }

  TotalSize = CompSize + sizeof (EFI_COMPRESSION_SECTION);

  //
  // Buffer too small?
  //
  if (TotalSize > *BufferSize) {
    *BufferSize = TotalSize;
    if (CompData != NULL) {
      free (CompData);
    }

    return EFI_BUFFER_TOO_SMALL;
  }
  //
  // Add the section header for the compressed data
  //
  CompressionSet.CommonHeader.Type    = EFI_SECTION_COMPRESSION;
  CompressionSet.CommonHeader.Size[0] = (UINT8) (TotalSize & 0xff);
  CompressionSet.CommonHeader.Size[1] = (UINT8) ((TotalSize & 0xff00) >> 8);
  CompressionSet.CommonHeader.Size[2] = (UINT8) ((TotalSize & 0xff0000) >> 16);
  CompressionSet.CompressionType      = CompressionType;
  CompressionSet.UncompressedLength   = DataSize;

  //
  // Copy header and data to the buffer
  //
  memcpy (FileBuffer, &CompressionSet, sizeof (EFI_COMPRESSION_SECTION));
  memcpy (FileBuffer + sizeof (CompressionSet), CompData, CompSize);

  //
  // Make sure section ends on a DWORD boundary
  //
  while ((TotalSize & 0x03) != 0) {
    FileBuffer[TotalSize] = 0;
    TotalSize++;
  }

  *BufferSize = TotalSize;

  if (CompData != NULL) {
    free (CompData);
  }

  return EFI_SUCCESS;
}

static
void
StripParens (
  IN OUT CHAR8 *String
  )
/*++

Routine Description:

  Removes Parenthesis from around a string

Arguments:

 String    - String to remove parens from

Returns:

  None

--*/
{
  INT32 Index;

  if (String[0] != '(') {
    return ;
  }

  for (Index = 1; String[Index] != ')'; Index++) {
    String[Index - 1] = String[Index];
    if (String[Index] == 0) {
      return ;
    }
  }

  String[Index - 1] = 0;

  return ;
}

static
void
StripEqualMark (
  IN OUT CHAR8 *String
  )
/*++

Routine Description:

  Removes Equal Mark from around a string

Arguments:

 String    - String to remove equal mark from

Returns:

  None

--*/
{
  INT32 Index;

  if (String[0] != '=' && String[strlen (String) - 1] != '=') {
    return ;
  }

  if (String[0] == '=') {

    for (Index = 1; String[Index] != 0; Index++) {
      String[Index - 1] = String[Index];
    }

    String[Index - 1] = 0;
  }

  if (String[strlen (String) - 1] == '=') {
    String[strlen (String) - 1] = 0;
  }

  return ;
}

static
void
SplitAttributesField (
  IN CHAR8       *Buffer,
  IN CHAR8       *AttributesArray[],
  IN OUT UINT32  *NumberOfAttributes
  )
/*
  NumberOfAttributes: on input, it specifies the current number of attributes
                      stored in AttributeArray.
                      on output, it is updated to the latest number of attributes
                      stored in AttributesArray.
*/
{
  UINT32  Index;
  UINT32  Index2;
  UINT32  z;
  CHAR8   *CharBuffer;

  CharBuffer  = NULL;
  CharBuffer  = (CHAR8 *) malloc (_MAX_PATH);
  ZeroMem (CharBuffer, _MAX_PATH);

  for (Index = 0, z = 0, Index2 = 0; Index < strlen (Buffer); Index++) {

    if (Buffer[Index] != '|') {
      CharBuffer[z] = Buffer[Index];
      z++;
    } else {

      CharBuffer[z] = 0;
      AttributesArray[*NumberOfAttributes + Index2] = CharBuffer;
      Index2++;

      //
      // allocate new char buffer for the next attributes string
      //
      CharBuffer = (CHAR8 *) malloc (_MAX_PATH);
      ZeroMem (CharBuffer, _MAX_PATH);
      z = 0;
    }
  }

  CharBuffer[z] = 0;
  //
  // record the last attributes string in the Buffer
  //
  AttributesArray[*NumberOfAttributes + Index2] = CharBuffer;
  Index2++;

  *NumberOfAttributes += Index2;

  return ;
}

static
INT32
GetToolArguments (
  CHAR8       *ToolArgumentsArray[],
  FILE        *Package,
  CHAR8       **PtrInputFileName,
  CHAR8       **PtrOutputFileName,
  EFI_GUID    *Guid,
  UINT16      *GuidedSectionAttributes
  )
{
  CHAR8       Buffer[_MAX_PATH];
  BOOLEAN     ArgumentsFlag;
  BOOLEAN     InputFlag;
  BOOLEAN     OutputFlag;
  BOOLEAN     GuidFlag;
  BOOLEAN     AttributesFlag;
  UINT32      argc;
  UINT32      Index2;
  UINT32      z;
  CHAR8       *CharBuffer;
  INT32       ReturnValue;
  EFI_STATUS  Status;

  CHAR8       *AttributesArray[MAX_ARRAY_SIZE];
  UINT32      NumberOfAttributes;
  CHAR8       *InputFileName;
  CHAR8       *OutputFileName;
  UINT32      LineNumber;
  Buffer[_MAX_PATH];

  ArgumentsFlag   = FALSE;
  InputFlag       = FALSE;
  OutputFlag      = FALSE;
  GuidFlag        = FALSE;
  AttributesFlag  = FALSE;
  //
  // Start at 1, since ToolArgumentsArray[0]
  // is the program name.
  //
  argc            = 1;
  Index2              = 0;

  z                   = 0;
  ReturnValue         = 0;
  NumberOfAttributes  = 0;
  InputFileName       = NULL;
  OutputFileName      = NULL;

  ZeroMem (Buffer, _MAX_PATH);
  ZeroMem (AttributesArray, sizeof (CHAR8 *) * MAX_ARRAY_SIZE);
  LineNumber = 0;
  while (Buffer[0] != ')') {

    if (GetNextLine (Buffer, Package, &LineNumber) != -1) {
      CheckSlash (Buffer, Package, &LineNumber);
      StripEqualMark (Buffer);
    } else {
      Error (NULL, 0, 0, "failed to get next line from package file", NULL);
      return -1;
    }

    if (Buffer[0] == ')') {
      break;
    } else if (_strcmpi (Buffer, "ARGS") == 0) {

      ArgumentsFlag   = TRUE;
      AttributesFlag  = FALSE;
      continue;

    } else if (_strcmpi (Buffer, "INPUT") == 0) {

      InputFlag       = TRUE;
      ArgumentsFlag   = FALSE;
      AttributesFlag  = FALSE;
      continue;

    } else if (_strcmpi (Buffer, "OUTPUT") == 0) {

      OutputFlag      = TRUE;
      ArgumentsFlag   = FALSE;
      AttributesFlag  = FALSE;
      continue;

    } else if (_strcmpi (Buffer, "GUID") == 0) {

      GuidFlag        = TRUE;
      ArgumentsFlag   = FALSE;
      AttributesFlag  = FALSE;
      //
      // fetch the GUID for the section
      //
      continue;

    } else if (_strcmpi (Buffer, "ATTRIBUTES") == 0) {

      AttributesFlag  = TRUE;
      ArgumentsFlag   = FALSE;
      //
      // fetch the GUIDed Section's Attributes
      //
      continue;

    } else if (_strcmpi (Buffer, "") == 0) {
      continue;
    }
    //
    // get all command arguments into ToolArgumentsArray
    //
    if (ArgumentsFlag) {

      StripEqualMark (Buffer);

      CharBuffer = (CHAR8 *) malloc (_MAX_PATH);
      if (CharBuffer == NULL) {
        goto ErrorExit;
      }

      ZeroMem (CharBuffer, sizeof (_MAX_PATH));

      ToolArgumentsArray[argc] = CharBuffer;

      strcpy (ToolArgumentsArray[argc], Buffer);

      argc += 1;
      ToolArgumentsArray[argc] = NULL;
      continue;
    }

    if (InputFlag) {

      StripEqualMark (Buffer);

      InputFileName = (CHAR8 *) malloc (_MAX_PATH);
      if (InputFileName == NULL) {
        goto ErrorExit;
      }

      ZeroMem (InputFileName, sizeof (_MAX_PATH));

      strcpy (InputFileName, Buffer);

      InputFlag = FALSE;
      continue;
    }

    if (OutputFlag) {

      StripEqualMark (Buffer);

      OutputFileName = (CHAR8 *) malloc (_MAX_PATH);
      if (OutputFileName == NULL) {
        goto ErrorExit;
      }

      ZeroMem (OutputFileName, sizeof (_MAX_PATH));

      strcpy (OutputFileName, Buffer);

      OutputFlag = FALSE;
      continue;
    }

    if (GuidFlag) {

      StripEqualMark (Buffer);

      Status = StringToGuid (Buffer, Guid);
      if (EFI_ERROR (Status)) {
        ReturnValue = -1;
        goto ErrorExit;
      }

      GuidFlag = FALSE;
    }

    if (AttributesFlag) {

      StripEqualMark (Buffer);

      //
      // there might be no space between each attribute in the statement,
      // split them aside and return each attribute string
      // in the AttributesArray
      //
      SplitAttributesField (Buffer, AttributesArray, &NumberOfAttributes);
    }
  }
  //
  // ReplaceVariableInBuffer (ToolArgumentsArray,&i,"INPUT",InputVariable,j);
  // ReplaceVariableInBuffer (ToolArgumentsArray,&i,"OUTPUT",&TargetFileName,1);
  //
  for (z = 0; z < NumberOfAttributes; z++) {
    if (_strcmpi (AttributesArray[z], "PROCESSING_REQUIRED") == 0) {
      *GuidedSectionAttributes |= EFI_GUIDED_SECTION_PROCESSING_REQUIRED;
    } else if (_strcmpi (AttributesArray[z], "AUTH_STATUS_VALID") == 0) {
      *GuidedSectionAttributes |= EFI_GUIDED_SECTION_AUTH_STATUS_VALID;
    }
  }

ErrorExit:

  for (Index2 = 0; Index2 < MAX_ARRAY_SIZE; Index2++) {
    if (AttributesArray[Index2] == NULL) {
      break;
    }

    free (AttributesArray[Index2]);
  }

  *PtrInputFileName   = InputFileName;
  *PtrOutputFileName  = OutputFileName;

  return ReturnValue;
}

static
INT32
ProcessScript (
  IN OUT UINT8   *FileBuffer,
  IN FILE        *Package,
  IN CHAR8       *BuildDirectory,
  IN BOOLEAN     ForceUncompress
  )
/*++

Routine Description:

  Signs the section, data stays in same location

Arguments:

  FileBuffer  - Data Buffer

  Package     - Points to curly brace in Image Script

  BuildDirectory     - Name of the source directory parameter
  
  ForceUncompress   - Whether to force uncompress.

Returns:

  Number of bytes added to file buffer
  -1 on error

--*/
{
  EFI_STATUS  Status;
  UINT32      Size;
  UINT32      OldSize;
  UINT32      Adjust;
  UINT16      TeStrippedSize;
  CHAR8       Buffer[_MAX_PATH];
  CHAR8       Type[_MAX_PATH];
  CHAR8       FileName[_MAX_PATH];
  INT32       Index3;
  INT32       Index2;
  UINT32      ReturnValue;
  UINT8       ByteBuffer;
  FILE        *InFile;
  UINT32      SourceDataSize;
  CHAR8       *ToolArgumentsArray[MAX_ARRAY_SIZE];
  CHAR8       *OutputFileName;
  CHAR8       *InputFileName;
  CHAR8       ToolName[_MAX_PATH];
  FILE        *OutputFile;
  FILE        *InputFile;
  UINT8       Temp;
  int         returnint;
  UINT32      LineNumber;
  BOOLEAN     IsError;
  EFI_GUID    SignGuid;
  UINT16      GuidedSectionAttributes;
  UINT8       *TargetFileBuffer;

  OutputFileName          = NULL;
  InputFileName           = NULL;
  OutputFile              = NULL;
  InputFile               = NULL;
  IsError                 = FALSE;
  GuidedSectionAttributes = 0;
  TargetFileBuffer        = NULL;

  Size                    = 0;
  LineNumber              = 0;
  Buffer[0]               = 0;
  for (Index3 = 0; Index3 < MAX_ARRAY_SIZE; ++Index3) {
    ToolArgumentsArray[Index3] = NULL;
  }

  while (Buffer[0] != '}') {
    if (GetNextLine (Buffer, Package, &LineNumber) != -1) {
      CheckSlash (Buffer, Package, &LineNumber);
    } else {
      printf ("ERROR in IMAGE SCRIPT!\n");
      IsError = TRUE;
      goto Done;
    }

    if (_strcmpi (Buffer, "Compress") == 0) {
      //
      // Handle compress
      //
      //
      // read compression type
      //
      if (GetNextLine (Buffer, Package, &LineNumber) != -1) {
        CheckSlash (Buffer, Package, &LineNumber);
      }

      StripParens (Buffer);
      strcpy (Type, Buffer);
      //
      // build buffer
      //
      while (Buffer[0] != '{') {
        if (GetNextLine (Buffer, Package, &LineNumber) != -1) {
          CheckSlash (Buffer, Package, &LineNumber);
        }
      }

      ReturnValue = ProcessScript (&FileBuffer[Size], Package, BuildDirectory, ForceUncompress);
      if (ReturnValue == -1) {
        IsError = TRUE;
        goto Done;
      }
      //
      // Call compress routine on buffer.
      // Occasionally, compressed data + section header would
      // be largere than the source and EFI_BUFFER_TOO_SMALL is
      // returned from CompressSection()
      //
      SourceDataSize = ReturnValue;

      if (!ForceUncompress) {

        Status = CompressSection (
                  &FileBuffer[Size],
                  &ReturnValue,
                  SourceDataSize,
                  Type
                  );

        if (Status == EFI_BUFFER_TOO_SMALL) {
          Status = CompressSection (
                    &FileBuffer[Size],
                    &ReturnValue,
                    SourceDataSize,
                    Type
                    );
        }

        if (EFI_ERROR (Status)) {
          IsError = TRUE;
          goto Done;
        }
      }

      Size += ReturnValue;

    } else if (_strcmpi (Buffer, "Tool") == 0) {

      ZeroMem (ToolName, _MAX_PATH);
      ZeroMem (ToolArgumentsArray, sizeof (CHAR8 *) * MAX_ARRAY_SIZE);
      ZeroMem (&SignGuid, sizeof (EFI_GUID));

      //
      // handle signing Tool
      //
      while (Buffer[0] != '(') {
        if (GetNextLine (Buffer, Package, &LineNumber) != -1) {
          CheckSlash (Buffer, Package, &LineNumber);
        }
      }

      if (_strcmpi (Buffer, "(") == 0) {
        if (GetNextLine (Buffer, Package, &LineNumber) != -1) {
          CheckSlash (Buffer, Package, &LineNumber);
        }
      }

      StripParens (Buffer);
      strcpy (ToolName, Buffer);
      ToolArgumentsArray[0] = ToolName;

      //
      // read ARGS
      //
      if (GetToolArguments (
            ToolArgumentsArray,
            Package,
            &InputFileName,
            &OutputFileName,
            &SignGuid,
            &GuidedSectionAttributes
            ) == -1) {
        IsError = TRUE;
        goto Done;
      }
      //
      // if the tool need input file,
      // dump the file buffer to the specified input file.
      //
      if (InputFileName != NULL) {
        InputFile = fopen (InputFileName, "wb");
        if (InputFile == NULL) {
          Error (NULL, 0, 0, InputFileName, "failed to open output file for writing");
          IsError = TRUE;
          goto Done;
        }

        fwrite (FileBuffer, sizeof (UINT8), Size, InputFile);
        fclose (InputFile);
        InputFile = NULL;
        free (InputFileName);
        InputFileName = NULL;
      }
      //
      // dispatch signing tool
      //
      returnint = _spawnv (_P_WAIT, ToolName, ToolArgumentsArray);
      if (returnint != 0) {
        Error (NULL, 0, 0, ToolName, "external tool failed");
        IsError = TRUE;
        goto Done;
      }
      //
      // if the tool has output file,
      // dump the output file to the file buffer
      //
      if (OutputFileName != NULL) {

        OutputFile = fopen (OutputFileName, "rb");
        if (OutputFile == NULL) {
          Error (NULL, 0, 0, OutputFileName, "failed to open output file for writing");
          IsError = TRUE;
          goto Done;
        }

        TargetFileBuffer  = &FileBuffer[Size];
        SourceDataSize    = Size;

        fread (&Temp, sizeof (UINT8), 1, OutputFile);
        while (!feof (OutputFile)) {
          FileBuffer[Size++] = Temp;
          fread (&Temp, sizeof (UINT8), 1, OutputFile);
        }

        while ((Size & 0x03) != 0) {
          FileBuffer[Size] = 0;
          Size++;
        }

        SourceDataSize = Size - SourceDataSize;

        fclose (OutputFile);
        OutputFile = NULL;
        free (OutputFileName);
        OutputFileName = NULL;

        if (CompareGuid (&SignGuid, &mZeroGuid) != 0) {
          ReturnValue = SourceDataSize;
          Status = GenSimpleGuidSection (
                    TargetFileBuffer,
                    &ReturnValue,
                    SourceDataSize,
                    SignGuid,
                    GuidedSectionAttributes
                    );
          if (EFI_ERROR (Status)) {
            IsError = TRUE;
            goto Done;
          }

          Size = ReturnValue;
        }
      }

    } else if (Buffer[0] != '}') {
      //
      // if we are here, we should see either a file name,
      // or a }.
      //
      Index3      = 0;
      FileName[0] = 0;
      //
      // Prepend the build directory to the file name if the
      // file name does not already contain a full path.
      //
      if (!isalpha (Buffer[0]) || (Buffer[1] != ':')) {
        sprintf (FileName, "%s\\", BuildDirectory);
      }
      
      while (Buffer[Index3] != '\n') {
        if (Buffer[Index3] == 0) {
          break;
        } else {
          Index2              = strlen (FileName);
          FileName[Index2++]  = Buffer[Index3++];
          FileName[Index2]    = 0;
        }
      }

      InFile = fopen (FileName, "rb");
      if (InFile == NULL) {
        Error (NULL, 0, 0, FileName, "failed to open file for reading");
        IsError = TRUE;
        goto Done;
      }

      OldSize = Size;
      fread (&ByteBuffer, sizeof (UINT8), 1, InFile);
      while (!feof (InFile)) {
        FileBuffer[Size++] = ByteBuffer;
        fread (&ByteBuffer, sizeof (UINT8), 1, InFile);
      }

      fclose (InFile);
      InFile = NULL;

      //
      // Adjust the TE Section for IPF so that the function entries are 16-byte aligned.
      //
      if (Size - OldSize >= sizeof (EFI_COMMON_SECTION_HEADER) + sizeof (EFI_TE_IMAGE_HEADER) &&
          ((EFI_COMMON_SECTION_HEADER *) &FileBuffer[OldSize])->Type == EFI_SECTION_TE        &&
          ((EFI_TE_IMAGE_HEADER *) &FileBuffer[OldSize + 4])->Machine == EFI_IMAGE_MACHINE_IA64) {
        TeStrippedSize = ((EFI_TE_IMAGE_HEADER *) &FileBuffer[OldSize + 4])->StrippedSize;
        Adjust = TeStrippedSize - (OldSize + sizeof (EFI_COMMON_SECTION_HEADER) + sizeof (EFI_TE_IMAGE_HEADER));
        Adjust &= 15;
        if (Adjust > 0) {
          memmove (&FileBuffer[OldSize + Adjust], &FileBuffer[OldSize], Size - OldSize);
          //
          // Pad with RAW Section type
          //
          *(UINT32 *)&FileBuffer[OldSize] = 0x19000000 | Adjust;
          Size += Adjust;
          //
          // Make sure the Data alignment in FFS header is no less than 1 (16-byte aligned)
          //
          MinFfsDataAlignOverride = 1;
        }
      }

      //
      // Make sure section ends on a DWORD boundary
      //
      while ((Size & 0x03) != 0) {
        FileBuffer[Size] = 0;
        Size++;
      }

    }
  }

Done:
  for (Index3 = 1; Index3 < MAX_ARRAY_SIZE; Index3++) {
    if (ToolArgumentsArray[Index3] == NULL) {
      break;
    }

    free (ToolArgumentsArray[Index3]);
  }

  if (IsError) {
    return -1;
  }

  return Size;

}

static
UINT8
StringToType (
  IN CHAR8 *String
  )
/*++

Routine Description:

  Converts File Type String to value.  EFI_FV_FILETYPE_ALL indicates that an
  unrecognized file type was specified.

Arguments:

  String    - File type string

Returns:

  File Type Value

--*/
{
  if (_strcmpi (String, "EFI_FV_FILETYPE_RAW") == 0) {
    return EFI_FV_FILETYPE_RAW;
  }

  if (_strcmpi (String, "EFI_FV_FILETYPE_FREEFORM") == 0) {
    return EFI_FV_FILETYPE_FREEFORM;
  }

  if (_strcmpi (String, "EFI_FV_FILETYPE_SECURITY_CORE") == 0) {
    return EFI_FV_FILETYPE_SECURITY_CORE;
  }

  if (_strcmpi (String, "EFI_FV_FILETYPE_PEI_CORE") == 0) {
    return EFI_FV_FILETYPE_PEI_CORE;
  }

  if (_strcmpi (String, "EFI_FV_FILETYPE_DXE_CORE") == 0) {
    return EFI_FV_FILETYPE_DXE_CORE;
  }

  if (_strcmpi (String, "EFI_FV_FILETYPE_PEIM") == 0) {
    return EFI_FV_FILETYPE_PEIM;
  }

  if (_strcmpi (String, "EFI_FV_FILETYPE_DRIVER") == 0) {
    return EFI_FV_FILETYPE_DRIVER;
  }

  if (_strcmpi (String, "EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER") == 0) {
    return EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER;
  }

  if (_strcmpi (String, "EFI_FV_FILETYPE_APPLICATION") == 0) {
    return EFI_FV_FILETYPE_APPLICATION;
  }

  if (_strcmpi (String, "EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE") == 0) {
    return EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE;
  }

  return EFI_FV_FILETYPE_ALL;
}

static
UINT32
AdjustFileSize (
  IN UINT8  *FileBuffer,
  IN UINT32 FileSize
  )
/*++

Routine Description:
  Adjusts file size to insure sectioned file is exactly the right length such
  that it ends on exactly the last byte of the last section.  ProcessScript()
  may have padded beyond the end of the last section out to a 4 byte boundary.
  This padding is stripped.

Arguments:
  FileBuffer  - Data Buffer - contains a section stream
  FileSize    - Size of FileBuffer as returned from ProcessScript()

Returns:
  Corrected size of file.

--*/
{
  UINT32                    TotalLength;
  UINT32                    CurrentLength;
  UINT32                    SectionLength;
  UINT32                    SectionStreamLength;
  EFI_COMMON_SECTION_HEADER *SectionHeader;
  EFI_COMMON_SECTION_HEADER *NextSectionHeader;

  TotalLength         = 0;
  CurrentLength       = 0;
  SectionStreamLength = FileSize;

  SectionHeader       = (EFI_COMMON_SECTION_HEADER *) FileBuffer;

  while (TotalLength < SectionStreamLength) {
    SectionLength = *((UINT32 *) SectionHeader->Size) & 0x00ffffff;
    TotalLength += SectionLength;

    if (TotalLength == SectionStreamLength) {
      return TotalLength;
    }
    //
    // Move to the next byte following the section...
    //
    SectionHeader = (EFI_COMMON_SECTION_HEADER *) ((UINT8 *) SectionHeader + SectionLength);
    CurrentLength = (UINTN) SectionHeader - (UINTN) FileBuffer;

    //
    // Figure out where the next section begins
    //
    NextSectionHeader = (EFI_COMMON_SECTION_HEADER *) ((UINT8 *) SectionHeader + 3);
    NextSectionHeader = (EFI_COMMON_SECTION_HEADER *) ((UINTN) NextSectionHeader &~ (UINTN) 3);
    TotalLength += (UINTN) NextSectionHeader - (UINTN) SectionHeader;
    SectionHeader = NextSectionHeader;
  }

  return CurrentLength;
}

static
INT32
MainEntry (
  INT32     argc,
  CHAR8     *argv[],
  BOOLEAN   ForceUncompress
  )
/*++

Routine Description:

  MainEntry function.

Arguments:

  argc            - Number of command line parameters.
  argv            - Array of pointers to command line parameter strings.
  ForceUncompress - If TRUE, force to do not compress the sections even if compression
                    is specified in the script. Otherwise, FALSE.

Returns:
  STATUS_SUCCESS  - Function exits successfully.
  STATUS_ERROR    - Some error occurred during execution.

--*/
{
  FILE                    *PrimaryPackage;
  FILE                    *OverridePackage;
  FILE                    *Out;
  CHAR8                   BaseName[_MAX_PATH];
  EFI_GUID                FfsGuid;
  CHAR8                   GuidString[_MAX_PATH];
  EFI_FFS_FILE_HEADER     FileHeader;
  CHAR8                   FileType[_MAX_PATH];
  EFI_FFS_FILE_ATTRIBUTES FfsAttrib;
  EFI_FFS_FILE_ATTRIBUTES FfsAttribDefined;
  UINT64                  FfsAlignment;
  UINT32                  FfsAlignment32;
  CHAR8                   InputString[_MAX_PATH];
  BOOLEAN                 ImageScriptInOveride;
  UINT32                  FileSize;
  UINT8                   *FileBuffer;
  EFI_STATUS              Status;
  UINT32                  LineNumber;
#if (PI_SPECIFICATION_VERSION < 0x00010000)  
  EFI_FFS_FILE_TAIL       TailValue;
#endif
  BaseName[0]       = 0;
  FileType[0]       = 0;
  FfsAttrib         = 0;
  FfsAttribDefined  = 0;
  FfsAlignment      = 0;
  FfsAlignment32    = 0;
  PrimaryPackage    = NULL;
  Out               = NULL;
  OverridePackage   = NULL;
  FileBuffer        = NULL;

  strcpy (GuidString, "00000000-0000-0000-0000-000000000000");
  Status = StringToGuid (GuidString, &FfsGuid);
  if (Status != 0) {
    Error (NULL, 0, 0, GuidString, "error parsing GUID string");
    return STATUS_ERROR;
  }

  GuidString[0]         = 0;
  ImageScriptInOveride  = FALSE;
  //
  // Initialize the simple file parsing routines. Then open
  // the primary package file for parsing.
  //
  SFPInit ();
  if (SFPOpenFile (mGlobals.PrimaryPackagePath) != STATUS_SUCCESS) {
    Error (NULL, 0, 0, mGlobals.PrimaryPackagePath, "unable to open primary package file");
    goto Done;
  }
  //
  // First token in the file must be "PACKAGE.INF"
  //
  if (!SFPIsToken ("PACKAGE.INF")) {
    Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected 'PACKAGE.INF'", NULL);
    goto Done;
  }
  //
  // Find the [.] section
  //
  if (!SFPSkipToToken ("[.]")) {
    Error (mGlobals.PrimaryPackagePath, 1, 0, "could not locate [.] section in package file", NULL);
    goto Done;
  }
  //
  // Start parsing the data. The algorithm is essentially the same for each keyword:
  //   1. Identify the keyword
  //   2. Verify that the keyword/value pair has not already been defined
  //   3. Set some flag indicating that the keyword/value pair has been defined
  //   4. Skip over the "="
  //   5. Get the value, which may be a number, TRUE, FALSE, or a string.
  //
  while (1) {
    if (SFPIsToken ("BASE_NAME")) {
      //
      // Found BASE_NAME, format:
      //   BASE_NAME = MyBaseName
      //
      if (BaseName[0] != 0) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "BASE_NAME already defined", NULL);
        goto Done;
      }

      if (!SFPIsToken ("=")) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected '='", NULL);
        goto Done;
      }

      if (!SFPGetNextToken (BaseName, sizeof (BaseName))) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected valid base name", NULL);
        goto Done;
      }
    } else if (SFPIsToken ("IMAGE_SCRIPT")) {
      //
      // Found IMAGE_SCRIPT. Break out and process below.
      //
      break;
    } else if (SFPIsToken ("FFS_FILEGUID")) {
      //
      // found FILEGUID, format:
      //   FFS_FILEGUID = F7845C4F-EDF5-42C5-BD8F-A02AF63DD93A
      //
      if (GuidString[0] != 0) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "FFS_FILEGUID already defined", NULL);
        goto Done;
      }

      if (!SFPIsToken ("=")) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected '='", NULL);
        goto Done;
      }

      if (SFPGetGuidToken (GuidString, sizeof (GuidString)) != TRUE) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected file GUID", NULL);
        goto Done;
      }

      Status = StringToGuid (GuidString, &FfsGuid);
      if (Status != 0) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected valid file GUID", NULL);
        goto Done;
      }
    } else if (SFPIsToken ("FFS_FILETYPE")) {
      //
      // ***********************************************************************
      //
      // Found FFS_FILETYPE, format:
      //  FFS_FILETYPE = EFI_FV_FILETYPE_APPLICATION
      //
      if (FileType[0] != 0) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "FFS_FILETYPE previously defined", NULL);
        goto Done;
      }

      if (!SFPIsToken ("=")) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected '='", NULL);
        goto Done;
      }

      if (!SFPGetNextToken (FileType, sizeof (FileType))) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected valid FFS_FILETYPE", NULL);
        goto Done;
      }
    }
#if (PI_SPECIFICATION_VERSION < 0x00010000)    
    else if (SFPIsToken ("FFS_ATTRIB_HEADER_EXTENSION")) {
      //
      // ***********************************************************************
      //
      // Found: FFS_ATTRIB_HEADER_EXTENSION = FALSE
      // Spec says the bit is for future expansion, and must be false.
      //
      if (FfsAttribDefined & FFS_ATTRIB_HEADER_EXTENSION) {
        Error (
          mGlobals.PrimaryPackagePath,
          SFPGetLineNumber (),
          0,
          "FFS_ATTRIB_HEADER_EXTENSION previously defined",
          NULL
          );
        goto Done;
      }

      FfsAttribDefined |= FFS_ATTRIB_HEADER_EXTENSION;
      if (!SFPIsToken ("=")) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected '='", NULL);
        goto Done;
      }

      if (SFPIsToken ("TRUE")) {
        Error (
          mGlobals.PrimaryPackagePath,
          SFPGetLineNumber (),
          0,
          "only FFS_ATTRIB_HEADER_EXTENSION = FALSE is supported",
          NULL
          );
        goto Done;
      } else if (SFPIsToken ("FALSE")) {
        //
        // Default is FALSE
        //
      } else {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected 'FALSE'", NULL);
        goto Done;
      }
    }
#else
    else if (SFPIsToken ("FFS_ATTRIB_FIXED")) {
      //
      // ***********************************************************************
      //
      // Found: FFS_ATTRIB_FIXED = TRUE | FALSE
      //
      if (FfsAttribDefined & FFS_ATTRIB_FIXED) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "FFS_ATTRIB_FIXED previously defined", NULL);
        goto Done;
      }

      FfsAttribDefined |= FFS_ATTRIB_FIXED;
      if (!SFPIsToken ("=")) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected '='", NULL);
        goto Done;
      }

      if (SFPIsToken ("TRUE")) {
        FfsAttrib |= FFS_ATTRIB_FIXED;
      } else if (SFPIsToken ("FALSE")) {
        //
        // Default is FALSE
        //
      } else {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected 'TRUE' or 'FALSE'", NULL);
        goto Done;
      }
    } 
#endif
    else if (SFPIsToken ("FFS_ATTRIB_TAIL_PRESENT")) {
      //
      // ***********************************************************************
      //
      // Found: FFS_ATTRIB_TAIL_PRESENT = TRUE | FALSE
      //
      if (FfsAttribDefined & FFS_ATTRIB_TAIL_PRESENT) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "FFS_ATTRIB_TAIL_PRESENT previously defined", NULL);
        goto Done;
      }

      FfsAttribDefined |= FFS_ATTRIB_TAIL_PRESENT;
      if (!SFPIsToken ("=")) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected '='", NULL);
        goto Done;
      }

      if (SFPIsToken ("TRUE")) {
        FfsAttrib |= FFS_ATTRIB_TAIL_PRESENT;
      } else if (SFPIsToken ("FALSE")) {
        //
        // Default is FALSE
        //
      } else {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected 'TRUE' or 'FALSE'", NULL);
        goto Done;
      }
    } else if (SFPIsToken ("FFS_ATTRIB_RECOVERY")) {
      //
      // ***********************************************************************
      //
      // Found: FFS_ATTRIB_RECOVERY = TRUE | FALSE
      //
      if (FfsAttribDefined & FFS_ATTRIB_RECOVERY) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "FFS_ATTRIB_RECOVERY previously defined", NULL);
        goto Done;
      }

      FfsAttribDefined |= FFS_ATTRIB_RECOVERY;
      if (!SFPIsToken ("=")) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected '='", NULL);
        goto Done;
      }

      if (SFPIsToken ("TRUE")) {
        FfsAttrib |= FFS_ATTRIB_RECOVERY;
      } else if (SFPIsToken ("FALSE")) {
        //
        // Default is FALSE
        //
      } else {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected 'TRUE' or 'FALSE'", NULL);
        goto Done;
      }
    } else if (SFPIsToken ("FFS_ATTRIB_CHECKSUM")) {
      //
      // ***********************************************************************
      //
      // Found: FFS_ATTRIB_CHECKSUM = TRUE | FALSE
      //
      if (FfsAttribDefined & FFS_ATTRIB_CHECKSUM) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "FFS_ATTRIB_CHECKSUM previously defined", NULL);
        goto Done;
      }

      FfsAttribDefined |= FFS_ATTRIB_CHECKSUM;
      if (!SFPIsToken ("=")) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected '='", NULL);
        goto Done;
      }

      if (SFPIsToken ("TRUE")) {
        FfsAttrib |= FFS_ATTRIB_CHECKSUM;
      } else if (SFPIsToken ("FALSE")) {
        //
        // Default is FALSE
        //
      } else {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected 'TRUE' or 'FALSE'", NULL);
        goto Done;
      }
    } else if (SFPIsToken ("FFS_ALIGNMENT") || SFPIsToken ("FFS_ATTRIB_DATA_ALIGNMENT")) {
      //
      // ***********************************************************************
      //
      // Found FFS_ALIGNMENT, formats:
      //   FFS_ALIGNMENT = 0-7
      //   FFS_ATTRIB_DATA_ALIGNMENT = 0-7
      //
      if (FfsAttribDefined & FFS_ATTRIB_DATA_ALIGNMENT) {
        Error (
          mGlobals.PrimaryPackagePath,
          SFPGetLineNumber (),
          0,
          "FFS_ALIGNMENT/FFS_ATTRIB_DATA_ALIGNMENT previously defined",
          NULL
          );
        goto Done;
      }

      FfsAttribDefined |= FFS_ATTRIB_DATA_ALIGNMENT;
      if (!SFPIsToken ("=")) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected '='", NULL);
        goto Done;
      }

      if (!SFPGetNumber (&FfsAlignment32)) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected numeric value for alignment", NULL);
        goto Done;
      }

      if (FfsAlignment32 > 7) {
        Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, "expected 0 <= alignment <= 7", NULL);
        goto Done;
      }

      FfsAttrib |= (((EFI_FFS_FILE_ATTRIBUTES) FfsAlignment32) << 3);
    } else {
      SFPGetNextToken (InputString, sizeof (InputString));
      Error (mGlobals.PrimaryPackagePath, SFPGetLineNumber (), 0, InputString, "unrecognized/unexpected token");
      goto Done;
    }
  }
  //
  // Close the primary package file
  //
  SFPCloseFile ();
  //
  // TODO: replace code below with basically a copy of the code above. Don't
  // forget to reset the FfsAttribDefined variable first. Also, you'll need
  // to somehow keep track of whether or not the basename is defined multiple
  // times in the override package. Ditto on the file GUID.
  //
  if (mGlobals.OverridePackagePath[0] != 0) {
    OverridePackage = fopen (mGlobals.OverridePackagePath, "r");
    //
    // NOTE: For package override to work correctly, the code below must be modified to
    //       SET or CLEAR bits properly. For example, if the primary package set
    //       FFS_ATTRIB_CHECKSUM = TRUE, and the override set FFS_ATTRIB_CHECKSUM = FALSE, then
    //       we'd need to clear the bit below. Since this is not happening, I'm guessing that
    //       the override functionality is not being used, so should be made obsolete. If I'm
    //       wrong, and it is being used, then it needs to be fixed. Thus emit an error if it is
    //       used, and we'll address it then.  4/10/2003
    //
    Error (__FILE__, __LINE__, 0, "package override functionality is not implemented correctly", NULL);
    goto Done;
  } else {
    OverridePackage = NULL;
  }

#ifdef OVERRIDE_SUPPORTED
  if (OverridePackage != NULL) {
    //
    // Parse override package file
    //
    fscanf (OverridePackage, "%s", &InputString);
    if (_strcmpi (InputString, "PACKAGE.INF") != 0) {
      Error (mGlobals.OverridePackagePath, 1, 0, "invalid package file", "expected 'PACKAGE.INF'");
      goto Done;
    }
    //
    // Match [dir] to Build Directory
    //
    if (FindSectionInPackage (mGlobals.BuildDirectory, OverridePackage, &LineNumber) != 0) {
      Error (mGlobals.OverridePackagePath, 1, 0, mGlobals.BuildDirectory, "section not found in package file");
      goto Done;
    }

    InputString[0] = 0;
    while ((InputString[0] != '[') && (!feof (OverridePackage))) {
      if (GetNextLine (InputString, OverridePackage, &LineNumber) != -1) {
        if (InputString[0] != '[') {
here:
          if (_strcmpi (InputString, "BASE_NAME") == 0) {
            //
            // found BASE_NAME, next is = and string.
            //
            fscanf (OverridePackage, "%s", &InputString);
            CheckSlash (InputString, OverridePackage, &LineNumber);
            if (strlen (InputString) == 1) {
              //
              // string is just =
              //
              fscanf (OverridePackage, "%s", &InputString);
              CheckSlash (InputString, OverridePackage, &LineNumber);
              strcpy (BaseName, InputString);
            } else {
              BreakString (InputString, InputString, 1);
              strcpy (BaseName, InputString);
            }
          } else if (_strcmpi (InputString, "IMAGE_SCRIPT") == 0) {
            //
            // found IMAGE_SCRIPT, come back later to process it
            //
            ImageScriptInOveride = TRUE;
            fscanf (OverridePackage, "%s", &InputString);
          } else if (_strcmpi (InputString, "FFS_FILEGUID") == 0) {
            //
            // found FILEGUID, next is = and string.
            //
            fscanf (OverridePackage, "%s", &InputString);
            CheckSlash (InputString, OverridePackage, &LineNumber);
            if (strlen (InputString) == 1) {
              //
              // string is just =
              //
              fscanf (OverridePackage, "%s", &InputString);
              CheckSlash (InputString, OverridePackage, &LineNumber);
              Status = StringToGuid (InputString, &FfsGuid);
              if (Status != 0) {
                Error (mGlobals.OverridePackagePath, 1, 0, InputString, "bad FFS_FILEGUID format");
                goto Done;
              }
            } else {
              BreakString (InputString, InputString, 1);
              Status = StringToGuid (InputString, &FfsGuid);
              if (Status != 0) {
                Error (mGlobals.OverridePackagePath, 1, 0, InputString, "bad FFS_FILEGUID format");
                goto Done;
              }
            }
          } else if (_strcmpi (InputString, "FFS_FILETYPE") == 0) {
            //
            // found FILETYPE, next is = and string.
            //
            fscanf (OverridePackage, "%s", &InputString);
            CheckSlash (InputString, OverridePackage, &LineNumber);
            if (strlen (InputString) == 1) {
              //
              // string is just =
              //
              fscanf (OverridePackage, "%s", &InputString);
              CheckSlash (InputString, OverridePackage, &LineNumber);
              strcpy (FileType, InputString);
            } else {
              BreakString (InputString, InputString, 1);
              strcpy (FileType, InputString);
            }

          } else if (_strcmpi (InputString, "FFS_ATTRIB_RECOVERY") == 0) {
            //
            // found FFS_ATTRIB_RECOVERY, next is = and string.
            //
            fscanf (OverridePackage, "%s", &InputString);
            CheckSlash (InputString, OverridePackage, &LineNumber);
            if (strlen (InputString) == 1) {
              //
              // string is just =
              //
              fscanf (OverridePackage, "%s", &InputString);
              CheckSlash (InputString, OverridePackage, &LineNumber);
              if (_strcmpi (InputString, "TRUE") == 0) {
                FfsAttrib |= FFS_ATTRIB_RECOVERY;
              }
            } else {
              BreakString (InputString, InputString, 1);
              if (_strcmpi (InputString, "TRUE") == 0) {
                FfsAttrib |= FFS_ATTRIB_RECOVERY;
              }
            }
          } else if (_strcmpi (InputString, "FFS_ATTRIB_CHECKSUM") == 0) {
            //
            // found FFS_ATTRIB_CHECKSUM, next is = and string.
            //
            fscanf (OverridePackage, "%s", &InputString);
            CheckSlash (InputString, OverridePackage, &LineNumber);
            if (strlen (InputString) == 1) {
              //
              // string is just =
              //
              fscanf (OverridePackage, "%s", &InputString);
              CheckSlash (InputString, OverridePackage, &LineNumber);
              if (_strcmpi (InputString, "TRUE") == 0) {
                FfsAttrib |= FFS_ATTRIB_CHECKSUM;
              }
            } else {
              BreakString (InputString, InputString, 1);
              if (_strcmpi (InputString, "TRUE") == 0) {
                FfsAttrib |= FFS_ATTRIB_CHECKSUM;
              }
            }
          } else if (_strcmpi (InputString, "FFS_ALIGNMENT") == 0) {
            //
            // found FFS_ALIGNMENT, next is = and string.
            //
            fscanf (OverridePackage, "%s", &InputString);
            CheckSlash (InputString, OverridePackage, &LineNumber);
            if (strlen (InputString) == 1) {
              //
              // string is just =
              //
              fscanf (OverridePackage, "%s", &InputString);
              CheckSlash (InputString, OverridePackage, &LineNumber);
            } else {
              BreakString (InputString, InputString, 1);
            }

            AsciiStringToUint64 (InputString, FALSE, &FfsAlignment);
            if (FfsAlignment > 7) {
              Error (mGlobals.OverridePackagePath, 1, 0, InputString, "invalid FFS_ALIGNMENT value");
              goto Done;
            }

            FfsAttrib |= (((EFI_FFS_FILE_ATTRIBUTES) FfsAlignment) << 3);
          } else if (strchr (InputString, '=') != NULL) {
            BreakString (InputString, String, 1);
            fseek (OverridePackage, (-1 * (strlen (String) + 1)), SEEK_CUR);
            BreakString (InputString, InputString, 0);
            goto here;
          }
        }
      }
    }
  }
#endif // #ifdef OVERRIDE_SUPPORTED
  //
  // Require that they specified a file GUID at least, since that's how we're
  // naming the file.
  //
  if (GuidString[0] == 0) {
    Error (mGlobals.PrimaryPackagePath, 1, 0, "FFS_FILEGUID must be specified", NULL);
    return STATUS_ERROR;
  }
  //
  // Build Header and process image script
  //
  FileBuffer = (UINT8 *) malloc ((1024 * 1024 * 16) * sizeof (UINT8));
  if (FileBuffer == NULL) {
    Error (__FILE__, __LINE__, 0, "memory allocation failed", NULL);
    goto Done;
  }

  FileSize = 0;
  if (ImageScriptInOveride) {
#ifdef OVERRIDE_SUPPORTED
    rewind (OverridePackage);
    LineNumber = 0;
    FindSectionInPackage (mGlobals.BuildDirectory, OverridePackage, &LineNumber);
    while (_strcmpi (InputString, "IMAGE_SCRIPT") != 0) {
      GetNextLine (InputString, OverridePackage, &LineNumber);
      CheckSlash (InputString, OverridePackage, &LineNumber);
      if (strchr (InputString, '=') != NULL) {
        BreakString (InputString, InputString, 0);
      }
    }

    while (InputString[0] != '{') {
      GetNextLine (InputString, OverridePackage, &LineNumber);
      CheckSlash (InputString, OverridePackage, &LineNumber);
    }
    //
    // Found start of image script, process it
    //
    FileSize += ProcessScript (FileBuffer, OverridePackage, mGlobals.BuildDirectory, ForceUncompress);
    if (FileSize == -1) {
      Error (NULL, 0, 0, "failed to process script", NULL);
      goto Done;
    }

    if (StringToType (FileType) != EFI_FV_FILETYPE_RAW) {
      FileSize = AdjustFileSize (FileBuffer, FileSize);
    }

    if (BaseName[0] == '\"') {
      StripQuotes (BaseName);
    }

    if (mGlobals.OutputFilePath[0]) {
      //
      // Use user specified output file name
      //
      strcpy (InputString, mGlobals.OutputFilePath);
    } else {
      //
      // Construct the output file name according to FileType
      //
      if (BaseName[0] != 0) {
        sprintf (InputString, "%s-%s", GuidString, BaseName);
      } else {
        strcpy (InputString, GuidString);
      }

      switch (StringToType (FileType)) {

      case EFI_FV_FILETYPE_SECURITY_CORE:
        strcat (InputString, ".SEC");
        break;

      case EFI_FV_FILETYPE_PEIM:
      case EFI_FV_FILETYPE_PEI_CORE:
      case EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER:
        strcat (InputString, ".PEI");
        break;

      case EFI_FV_FILETYPE_DRIVER:
      case EFI_FV_FILETYPE_DXE_CORE:
        strcat (InputString, ".DXE");
        break;

      case EFI_FV_FILETYPE_APPLICATION:
        strcat (InputString, ".APP");
        break;

      case EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE:
        strcat (InputString, ".FVI");
        break;

      case EFI_FV_FILETYPE_RAW:
        strcat (InputString, ".RAW");
        break;

      case EFI_FV_FILETYPE_ALL:
        Error (mGlobals.OverridePackagePath, 1, 0, "invalid FFS file type for this utility", NULL);
        goto Done;

      default:
        strcat (InputString, ".FFS");
        break;
      }
    }

    if (ForceUncompress) {
      strcat (InputString, ".ORG");
    }
	
    Out = fopen (InputString, "wb");
    if (Out == NULL) {
      Error (NULL, 0, 0, InputString, "could not open output file for writing");
      goto Done;
    }
    //
    // create ffs header
    //
    memset (&FileHeader, 0, sizeof (EFI_FFS_FILE_HEADER));
    memcpy (&FileHeader.Name, &FfsGuid, sizeof (EFI_GUID));
    FileHeader.Type       = StringToType (FileType);
    if (((FfsAttrib & FFS_ATTRIB_DATA_ALIGNMENT) >> 3) < MinFfsDataAlignOverride) {
      FfsAttrib = (FfsAttrib & ~FFS_ATTRIB_DATA_ALIGNMENT) | (MinFfsDataAlignOverride << 3);
    }
    FileHeader.Attributes = FfsAttrib;
    //
    // Now FileSize includes the EFI_FFS_FILE_HEADER
    //
    FileSize += sizeof (EFI_FFS_FILE_HEADER);
    FileHeader.Size[0]  = (UINT8) (FileSize & 0xFF);
    FileHeader.Size[1]  = (UINT8) ((FileSize & 0xFF00) >> 8);
    FileHeader.Size[2]  = (UINT8) ((FileSize & 0xFF0000) >> 16);
    //
    // Fill in checksums and state, these must be zero for checksumming
    //
    // FileHeader.IntegrityCheck.Checksum.Header = 0;
    // FileHeader.IntegrityCheck.Checksum.File = 0;
    // FileHeader.State = 0;
    //
    FileHeader.IntegrityCheck.Checksum.Header = CalculateChecksum8 (
                                                  (UINT8 *) &FileHeader,
                                                  sizeof (EFI_FFS_FILE_HEADER)
                                                  );
    if (FileHeader.Attributes & FFS_ATTRIB_CHECKSUM) {
#if (PI_SPECIFICATION_VERSION < 0x00010000)  
      FileHeader.IntegrityCheck.Checksum.File = CalculateChecksum8 ((UINT8 *) &FileHeader, FileSize);
#else
      FileHeader.IntegrityCheck.Checksum.File = CalculateChecksum8 ((UINT8 *) ((UINTN)&FileHeader + sizeof (EFI_FFS_FILE_HEADER)), FileSize - sizeof (EFI_FFS_FILE_HEADER));
#endif
    } else {
      FileHeader.IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
    }

    FileHeader.State = EFI_FILE_HEADER_CONSTRUCTION | EFI_FILE_HEADER_VALID | EFI_FILE_DATA_VALID;
    //
    // write header
    //
    if (fwrite (&FileHeader, sizeof (FileHeader), 1, Out) != 1) {
      Error (NULL, 0, 0, "failed to write file header to output file", NULL);
      goto Done;
    }
    //
    // write data
    //
    if (fwrite (FileBuffer, FileSize - sizeof (EFI_FFS_FILE_HEADER), 1, Out) != 1) {
      Error (NULL, 0, 0, "failed to write all bytes to output file", NULL);
      goto Done;
    }

    fclose (Out);
    Out = NULL;
#endif // #ifdef OVERRIDE_SUPPORTED
  } else {
    //
    // Open primary package file and process the IMAGE_SCRIPT section
    //
    PrimaryPackage = fopen (mGlobals.PrimaryPackagePath, "r");
    if (PrimaryPackage == NULL) {
      Error (NULL, 0, 0, mGlobals.PrimaryPackagePath, "unable to open primary package file");
      goto Done;
    }

    LineNumber = 1;
    FindSectionInPackage (".", PrimaryPackage, &LineNumber);
    while (_strcmpi (InputString, "IMAGE_SCRIPT") != 0) {
      GetNextLine (InputString, PrimaryPackage, &LineNumber);
      CheckSlash (InputString, PrimaryPackage, &LineNumber);
      if (strchr (InputString, '=') != NULL) {
        BreakString (InputString, InputString, 0);
      }
    }

    while (InputString[0] != '{') {
      GetNextLine (InputString, PrimaryPackage, &LineNumber);
      CheckSlash (InputString, PrimaryPackage, &LineNumber);
    }
    //
    // Found start of image script, process it
    //
    FileSize += ProcessScript (FileBuffer, PrimaryPackage, mGlobals.BuildDirectory, ForceUncompress);
    if (FileSize == -1) {
      Error (NULL, 0, 0, "failed to process script", NULL);
      goto Done;
    }

    if (StringToType (FileType) != EFI_FV_FILETYPE_RAW) {
      FileSize = AdjustFileSize (FileBuffer, FileSize);
    }

    if (BaseName[0] == '\"') {
      StripQuotes (BaseName);
    }

    if (mGlobals.OutputFilePath[0]) {
      //
      // Use user specified output file name
      //
      strcpy (InputString, mGlobals.OutputFilePath);
    } else {
      //
      // Construct the output file name according to FileType
      //
      if (BaseName[0] != 0) {
        sprintf (InputString, "%s-%s", GuidString, BaseName);
      } else {
        strcpy (InputString, GuidString);
      }

      switch (StringToType (FileType)) {

      case EFI_FV_FILETYPE_SECURITY_CORE:
        strcat (InputString, ".SEC");
        break;

      case EFI_FV_FILETYPE_PEIM:
      case EFI_FV_FILETYPE_PEI_CORE:
      case EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER:
        strcat (InputString, ".PEI");
        break;

      case EFI_FV_FILETYPE_DRIVER:
      case EFI_FV_FILETYPE_DXE_CORE:
        strcat (InputString, ".DXE");
        break;

      case EFI_FV_FILETYPE_APPLICATION:
        strcat (InputString, ".APP");
        break;

      case EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE:
        strcat (InputString, ".FVI");
        break;

      case EFI_FV_FILETYPE_RAW:
        strcat (InputString, ".RAW");
        break;

      case EFI_FV_FILETYPE_ALL:
        Error (mGlobals.PrimaryPackagePath, 1, 0, "invalid FFS file type for this utility", NULL);
        goto Done;

      default:
        strcat (InputString, ".FFS");
        break;
      }
    }

    if (ForceUncompress) {
      strcat (InputString, ".ORG");
    }

    Out = fopen (InputString, "wb");
    if (Out == NULL) {
      Error (NULL, 0, 0, InputString, "failed to open output file for writing");
      goto Done;
    }
    //
    // Initialize the FFS file header
    //
    memset (&FileHeader, 0, sizeof (EFI_FFS_FILE_HEADER));
    memcpy (&FileHeader.Name, &FfsGuid, sizeof (EFI_GUID));
    FileHeader.Type       = StringToType (FileType);
    if (((FfsAttrib & FFS_ATTRIB_DATA_ALIGNMENT) >> 3) < MinFfsDataAlignOverride) {
      FfsAttrib = (FfsAttrib & ~FFS_ATTRIB_DATA_ALIGNMENT) | (MinFfsDataAlignOverride << 3);
    }
    FileHeader.Attributes = FfsAttrib;
    //
    // From this point on FileSize includes the size of the EFI_FFS_FILE_HEADER
    //
    FileSize += sizeof (EFI_FFS_FILE_HEADER);
    //
    // If using a tail, then it adds two bytes
    //
    if (FileHeader.Attributes & FFS_ATTRIB_TAIL_PRESENT) {
      //
      // Tail is not allowed for pad and 0-length files
      //
      if ((FileHeader.Type == EFI_FV_FILETYPE_FFS_PAD) || (FileSize == sizeof (EFI_FFS_FILE_HEADER))) {
        Error (
          mGlobals.PrimaryPackagePath,
          1,
          0,
          "FFS_ATTRIB_TAIL_PRESENT=TRUE is invalid for PAD or 0-length files",
          NULL
          );
        goto Done;
      }

      FileSize += sizeof (EFI_FFS_FILE_TAIL);
    }

    FileHeader.Size[0]  = (UINT8) (FileSize & 0xFF);
    FileHeader.Size[1]  = (UINT8) ((FileSize & 0xFF00) >> 8);
    FileHeader.Size[2]  = (UINT8) ((FileSize & 0xFF0000) >> 16);
    //
    // Fill in checksums and state, they must be 0 for checksumming.
    //
    // FileHeader.IntegrityCheck.Checksum.Header = 0;
    // FileHeader.IntegrityCheck.Checksum.File = 0;
    // FileHeader.State = 0;
    //
    FileHeader.IntegrityCheck.Checksum.Header = CalculateChecksum8 (
                                                  (UINT8 *) &FileHeader,
                                                  sizeof (EFI_FFS_FILE_HEADER)
                                                  );
    if (FileHeader.Attributes & FFS_ATTRIB_CHECKSUM) {
      //
      // Cheating here.  Since the header checksums, just calculate the checksum of the body.
      // Checksum does not include the tail
      //
      if (FileHeader.Attributes & FFS_ATTRIB_TAIL_PRESENT) {
        FileHeader.IntegrityCheck.Checksum.File = CalculateChecksum8 (
                                                    FileBuffer,
                                                    FileSize - sizeof (EFI_FFS_FILE_HEADER) - sizeof (EFI_FFS_FILE_TAIL)
                                                    );
      } else {
        FileHeader.IntegrityCheck.Checksum.File = CalculateChecksum8 (
                                                    FileBuffer,
                                                    FileSize - sizeof (EFI_FFS_FILE_HEADER)
                                                    );
      }
    } else {
      FileHeader.IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
    }
    //
    // Set the state now. Spec says the checksum assumes the state is 0
    //
    FileHeader.State = EFI_FILE_HEADER_CONSTRUCTION | EFI_FILE_HEADER_VALID | EFI_FILE_DATA_VALID;

#if (PI_SPECIFICATION_VERSION < 0x00010000)

    //
    // If there is a tail, then set it
    //
    if (FileHeader.Attributes & FFS_ATTRIB_TAIL_PRESENT) {
      TailValue = FileHeader.IntegrityCheck.TailReference;
      TailValue = (UINT16) (~TailValue);
      memcpy (
        (UINT8 *) FileBuffer + FileSize - sizeof (EFI_FFS_FILE_HEADER) - sizeof (EFI_FFS_FILE_TAIL),
        &TailValue,
        sizeof (TailValue)
        );
    }
#endif    
    //
    // Write the FFS file header
    //
    if (fwrite (&FileHeader, sizeof (FileHeader), 1, Out) != 1) {
      Error (NULL, 0, 0, "failed to write file header contents", NULL);
      goto Done;
    }
    //
    // Write data
    //
    if (fwrite (FileBuffer, FileSize - sizeof (EFI_FFS_FILE_HEADER), 1, Out) != 1) {
      Error (NULL, 0, 0, "failed to write file contents", NULL);
      goto Done;
    }
  }

Done:
  SFPCloseFile ();
  if (Out != NULL) {
    fclose (Out);
  }

  if (PrimaryPackage != NULL) {
    fclose (PrimaryPackage);
  }

  if (FileBuffer != NULL) {
    free (FileBuffer);
  }

  if (OverridePackage != NULL) {
    fclose (OverridePackage);
  }

  return GetUtilityStatus ();
}

int
main (
  INT32 argc,
  CHAR8 *argv[]
  )
/*++

Routine Description:

  Main function.

Arguments:

  argc - Number of command line parameters.
  argv - Array of pointers to parameter strings.

Returns:
  STATUS_SUCCESS - Utility exits successfully.
  STATUS_ERROR   - Some error occurred during execution.

--*/
{
  STATUS  Status;
  //
  // Set the name of our utility for error reporting purposes.
  //
  SetUtilityName (UTILITY_NAME);
  Status = ProcessCommandLineArgs (argc, argv);
  FreeMacros ();
  if (Status != STATUS_SUCCESS) {
    return Status;
  }

  Status = MainEntry (argc, argv, TRUE);
  if (Status == STATUS_SUCCESS) {
    MainEntry (argc, argv, FALSE);
  }
  //
  // If any errors were reported via the standard error reporting
  // routines, then the status has been saved. Get the value and
  // return it to the caller.
  //
  return GetUtilityStatus ();
}

static
STATUS
ProcessCommandLineArgs (
  int     Argc,
  char    *Argv[]
  )
/*++

Routine Description:
  Process the command line arguments.

Arguments:
  Argc - as passed in to main()
  Argv - as passed in to main()

Returns:
  STATUS_SUCCESS    - arguments all ok
  STATUS_ERROR      - problem with args, so caller should exit

--*/
{
  STATUS       Status;
  UINT8        *OriginalPrimaryPackagePath;
  UINT8        *OriginalOverridePackagePath;
  UINT8        *PackageName;
  
  //
  // If no args, then print usage instructions and return an error
  //
  if (Argc == 1) {
    PrintUsage ();
    return STATUS_ERROR;
  }
  
  OriginalPrimaryPackagePath = NULL;
  OriginalOverridePackagePath = NULL;
  memset (&mGlobals, 0, sizeof (mGlobals));
  Argc--;
  Argv++;
  while (Argc > 0) {
    if (_strcmpi (Argv[0], "-b") == 0) {
      //
      // OPTION: -b BuildDirectory
      // Make sure there is another argument, then save it to our globals.
      //
      if (Argc < 2) {
        Error (NULL, 0, 0, "-b option requires the build directory name", NULL);
        return STATUS_ERROR;
      }

      if (mGlobals.BuildDirectory[0]) {
        Error (NULL, 0, 0, Argv[0], "option can only be specified once");
        return STATUS_ERROR;
      }

      strcpy (mGlobals.BuildDirectory, Argv[1]);
      Argc--;
      Argv++;
    } else if (_strcmpi (Argv[0], "-p1") == 0) {
      //
      // OPTION: -p1 PrimaryPackageFile
      // Make sure there is another argument, then save it to our globals.
      //
      if (Argc < 2) {
        Error (NULL, 0, 0, Argv[0], "option requires the primary package file name");
        return STATUS_ERROR;
      }

      if (OriginalPrimaryPackagePath) {
        Error (NULL, 0, 0, Argv[0], "option can only be specified once");
        return STATUS_ERROR;
      }
      
      OriginalPrimaryPackagePath = Argv[1];
      Argc--;
      Argv++;
    } else if (_strcmpi (Argv[0], "-p2") == 0) {
      //
      // OPTION: -p2 OverridePackageFile
      // Make sure there is another argument, then save it to our globals.
      //
      if (Argc < 2) {
        Error (NULL, 0, 0, Argv[0], "option requires the override package file name");
        return STATUS_ERROR;
      }

      if (OriginalOverridePackagePath) {
        Error (NULL, 0, 0, Argv[0], "option can only be specified once");
        return STATUS_ERROR;
      }
      
      OriginalOverridePackagePath = Argv[1];
      Argc--;
      Argv++;
    } else if (_strcmpi (Argv[0], "-o") == 0) {
      //
      // OPTION: -o OutputFilePath
      // Make sure there is another argument, then save it to out globals.
      //
      if (Argc < 2) {
        Error (NULL, 0, 0, Argv[0], "option requires the output file name");
        return STATUS_ERROR;
      }
      if (mGlobals.OutputFilePath[0]) {
        Error (NULL, 0, 0, Argv[0], "option can only be specified once");
        return STATUS_ERROR;
      }

      strcpy (mGlobals.OutputFilePath, Argv[1]);
      Argc--;
      Argv++;
    } else if (_strcmpi (Argv[0], "-v") == 0) {
      //
      // OPTION: -v       verbose
      //
      mGlobals.Verbose = TRUE;
    } else if (_strcmpi (Argv[0], "-d") == 0) {
      //
      // OPTION: -d  name=value
      // Make sure there is another argument, then add it to our macro list.
      //
      if (Argc < 2) {
        Error (NULL, 0, 0, Argv[0], "option requires the macro definition");
        return STATUS_ERROR;
      }
      
      AddMacro (Argv[1]);
      Argc--;
      Argv++;
    } else if (_strcmpi (Argv[0], "-h") == 0) {
      //
      // OPTION: -h      help
      //
      PrintUsage ();
      return STATUS_ERROR;
    } else if (_strcmpi (Argv[0], "-?") == 0) {
      //
      // OPTION:  -?      help
      //
      PrintUsage ();
      return STATUS_ERROR;
    } else {
      Error (NULL, 0, 0, Argv[0], "unrecognized option");
      PrintUsage ();
      return STATUS_ERROR;
    }

    Argv++;
    Argc--;
  }

  //
  // Must have at least specified the build directory
  //
  if (!mGlobals.BuildDirectory[0]) {
    Error (NULL, 0, 0, "must specify build directory", NULL);
    return STATUS_ERROR;
  }
  
  //
  // Must have at least specified the package file name
  //
  if (OriginalPrimaryPackagePath == NULL) {
    Error (NULL, 0, 0, "must specify primary package file", NULL);
    return STATUS_ERROR;
  }

  PackageName = OriginalPrimaryPackagePath + strlen (OriginalPrimaryPackagePath);
  while ((*PackageName != '\\') && (*PackageName != '/') && 
         (PackageName != OriginalPrimaryPackagePath)) {
    PackageName--;
  }
  //
  // Skip the '\' or '/'
  //
  if (PackageName != OriginalPrimaryPackagePath) {
    PackageName++;
  }
  sprintf (mGlobals.PrimaryPackagePath, "%s\\%s.new", mGlobals.BuildDirectory, PackageName);
  Status = ReplaceMacros (OriginalPrimaryPackagePath, mGlobals.PrimaryPackagePath);
  if (Status == STATUS_WARNING) {
    //
    // No macro replacement, use the previous package file
    //
    strcpy (mGlobals.PrimaryPackagePath, OriginalPrimaryPackagePath);
  } else if (Status != STATUS_SUCCESS) {
    return Status;
  }
  
  if (OriginalOverridePackagePath != NULL) {
    PackageName = OriginalOverridePackagePath + strlen (OriginalOverridePackagePath);
    while ((*PackageName != '\\') && (*PackageName != '/') && 
           (PackageName != OriginalOverridePackagePath)) {
      PackageName--;
    }
    //
    // Skip the '\' or '/'
    //
    if (PackageName != OriginalOverridePackagePath) {
      PackageName++;
    }    
    sprintf (mGlobals.OverridePackagePath, "%s\\%s.new", mGlobals.BuildDirectory, PackageName);
    Status = ReplaceMacros (OriginalOverridePackagePath, mGlobals.OverridePackagePath);
    if (Status == STATUS_WARNING) {
      //
      // No macro replacement, use the previous package file
      //
      strcpy (mGlobals.OverridePackagePath, OriginalOverridePackagePath);
    } else if (Status != STATUS_SUCCESS) {
        return Status;
    }    
  }

  return STATUS_SUCCESS;
}

static
void
AddMacro (
  UINT8   *MacroString
  )
/*++

Routine Description:

  Add or override a macro definition.

Arguments:

  MacroString  - macro definition string: name=value

Returns:

  None

--*/  
{
  MACRO    *Macro;
  MACRO    *NewMacro;
  UINT8    *Value;
  
  //
  // Seperate macro name and value by '\0'
  //
  for (Value = MacroString; *Value && (*Value != '='); Value++);
  
  if (*Value == '=') {
    *Value = '\0';
    Value ++;
  }
  
  //
  // We now have a macro name and value. 
  // Look for an existing macro and overwrite it.
  //
  Macro = mGlobals.MacroList;
  while (Macro) {
    if (_strcmpi (MacroString, Macro->Name) == 0) {
      Macro->Value = Value;
      return;
    }

    Macro = Macro->Next;
  }
  
  //
  // Does not exist, create a new one
  //
  NewMacro = (MACRO *) malloc (sizeof (MACRO));
  memset ((UINT8 *) NewMacro, 0, sizeof (MACRO));
  NewMacro->Name   = MacroString;
  NewMacro->Value  = Value;

  //
  // Add it to the head of the list.
  //
  NewMacro->Next = mGlobals.MacroList;
  mGlobals.MacroList = NewMacro;
  
  return;
}

static
UINT8 *
GetMacroValue (
  UINT8   *MacroName
  )
/*++

Routine Description:

  Look up a macro.

Arguments:

  MacroName  - The name of macro

Returns:

  Pointer to the value of the macro if found
  NULL if the macro is not found

--*/   
{

  MACRO  *Macro;
  UINT8  *Value;

  //
  // Scan for macro
  //
  Macro = mGlobals.MacroList;
  while (Macro) {
    if (_strcmpi (MacroName, Macro->Name) == 0) {
      return Macro->Value;
    }
    Macro = Macro->Next;
  }
  
  //
  // Try environment variable
  //
  Value = getenv (MacroName);
  if (Value == NULL) {
    printf ("Environment variable %s not found!\n", MacroName);
  }   
  return Value;
}
  
static
void
FreeMacros (
  )
/*++

Routine Description:

  Free the macro list.

Arguments:

  None

Returns:

  None

--*/    
{
  MACRO    *Macro;
  MACRO    *NextMacro;
  
  Macro = mGlobals.MacroList;
  while (Macro) {
    NextMacro = Macro->Next;
    free (Macro);
    Macro = NextMacro;
  }
  mGlobals.MacroList = NULL;
  
  return;
}

static
STATUS
ReplaceMacros (
  UINT8   *InputFile,
  UINT8   *OutputFile
  )
/*++

Routine Description:

  Replace all the macros in InputFile to create the OutputFile.

Arguments:

  InputFile         - Input package file for macro replacement
  OutputFile        - Output package file after macro replacement

Returns:

  STATUS_SUCCESS    - Output package file is created successfully after the macro replacement.
  STATUS_WARNING    - Output package file is not created because of no macro replacement.
  STATUS_ERROR      - Some error occurred during execution.

--*/    
{
  FILE   *Fptr;
  UINT8  *SaveStart;
  UINT8  *FromPtr;
  UINT8  *ToPtr;
  UINT8  *Value;
  UINT8  *FileBuffer;
  UINTN  FileSize;
  
  //
  // Get the file size, and then read the entire thing into memory.
  // Allocate extra space for a terminator character.
  //
  if ((Fptr = fopen (InputFile, "r")) == NULL) {
    Error (NULL, 0, 0, InputFile, "can't open input file");
    return STATUS_ERROR;    
  }
  fseek (Fptr, 0, SEEK_END);
  FileSize = ftell (Fptr);
  fseek (Fptr, 0, SEEK_SET);
  FileBuffer = malloc (FileSize + 1);
  if (FileBuffer == NULL) {
    fclose (Fptr);
    Error (NULL, 0, 0, InputFile, "file buffer memory allocation failure");
    return STATUS_ERROR;
  }
  fread (FileBuffer, FileSize, 1, Fptr);
  FileBuffer[FileSize] = '\0';
  fclose (Fptr);
    
  //
  // Walk the entire file, replacing $(MACRO_NAME).
  //
  Fptr = NULL;
  FromPtr = FileBuffer;
  SaveStart = FromPtr;
  while (*FromPtr) {
    if ((*FromPtr == '$') && (*(FromPtr + 1) == '(')) {
      FromPtr += 2;
      for (ToPtr = FromPtr; *ToPtr && (*ToPtr != ')'); ToPtr++);
      if (*ToPtr) {
        //
        // Find an $(MACRO_NAME), replace it
        //
        *ToPtr = '\0';
        Value = GetMacroValue (FromPtr);
        *(FromPtr-2)= '\0';
        if (Fptr == NULL) {
          if ((Fptr = fopen (OutputFile, "w")) == NULL) {
            free (FileBuffer);
            Error (NULL, 0, 0, OutputFile, "can't open output file");
            return STATUS_ERROR;    
          }
        }
        if (Value != NULL) {
          fprintf (Fptr, "%s%s", SaveStart, Value);
        } else {
          fprintf (Fptr, "%s", SaveStart);
        }
        //
        // Continue macro replacement for the remaining string line
        //
        FromPtr = ToPtr+1;
        SaveStart = FromPtr;
        continue;
      } else {
        break;
      }
    } else {
      FromPtr++;
    }
  }
  if (Fptr != NULL) {
    fprintf (Fptr, "%s", SaveStart);
  }
  
  free (FileBuffer);
  if (Fptr != NULL) {
    fclose (Fptr);
    return STATUS_SUCCESS;
  } else {
    return STATUS_WARNING;
  }
}
