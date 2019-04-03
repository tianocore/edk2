/** @file
This file contains the PcdValue structure definition.

Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "CommonLib.h"
#include "PcdValueCommon.h"

typedef enum {
  PcdDataTypeBoolean,
  PcdDataTypeUint8,
  PcdDataTypeUint16,
  PcdDataTypeUint32,
  PcdDataTypeUint64,
  PcdDataTypePointer
} PCD_DATA_TYPE;

typedef struct {
  CHAR8          *SkuName;
  CHAR8          *DefaultValueName;
  CHAR8          *TokenSpaceGuidName;
  CHAR8          *TokenName;
  CHAR8          *DataType;
  CHAR8          *Value;
  PCD_DATA_TYPE  PcdDataType;
} PCD_ENTRY;

PCD_ENTRY  *PcdList;
UINT32     PcdListLength;

VOID
STATIC
RecordToken (
  UINT8   *FileBuffer,
  UINT32  PcdIndex,
  UINT32  TokenIndex,
  UINT32  TokenStart,
  UINT32  TokenEnd
  )
/*++

Routine Description:

  Record new token information

Arguments:

  FileBuffer    File Buffer to be record
  PcdIndex      Index of PCD in database
  TokenIndex    Index of Token
  TokenStart    Start of Token
  TokenEnd      End of Token

Returns:

  None
--*/
{
  CHAR8  *Token;

  Token = malloc (TokenEnd - TokenStart + 1);
  if (Token == NULL) {
    return;
  }
  memcpy (Token, &FileBuffer[TokenStart], TokenEnd - TokenStart);
  Token[TokenEnd - TokenStart] = 0;
  switch (TokenIndex) {
  case 0:
    PcdList[PcdIndex].SkuName = Token;
    break;
  case 1:
    PcdList[PcdIndex].DefaultValueName = Token;
    break;
  case 2:
    PcdList[PcdIndex].TokenSpaceGuidName = Token;
    break;
  case 3:
    PcdList[PcdIndex].TokenName = Token;
    break;
  case 4:
    PcdList[PcdIndex].DataType = Token;
    if (strcmp (Token, "BOOLEAN") == 0) {
      PcdList[PcdIndex].PcdDataType = PcdDataTypeBoolean;
    } else if (strcmp (Token, "UINT8") == 0) {
      PcdList[PcdIndex].PcdDataType = PcdDataTypeUint8;
    } else if (strcmp (Token, "UINT16") == 0) {
      PcdList[PcdIndex].PcdDataType = PcdDataTypeUint16;
    } else if (strcmp (Token, "UINT32") == 0) {
      PcdList[PcdIndex].PcdDataType = PcdDataTypeUint32;
    } else if (strcmp (Token, "UINT64") == 0) {
      PcdList[PcdIndex].PcdDataType = PcdDataTypeUint64;
    } else {
      PcdList[PcdIndex].PcdDataType = PcdDataTypePointer;
    }
    break;
  case 5:
    PcdList[PcdIndex].Value = Token;
    break;
  default:
    free (Token);
    break;
  }
}

int
STATIC
LookupPcdIndex (
  CHAR8  *SkuName             OPTIONAL,
  CHAR8  *DefaultValueName    OPTIONAL,
  CHAR8  *TokenSpaceGuidName,
  CHAR8  *TokenName
  )
/*++

Routine Description:

  Get PCD index in Pcd database

Arguments:

  SkuName               SkuName String
  DefaultValueName      DefaultValueName String
  TokenSpaceGuidName    TokenSpaceGuidName String
  TokenName             TokenName String

Returns:

  Index of PCD in Pcd database
--*/
{
  UINT32  Index;

  if (SkuName == NULL) {
    SkuName = "DEFAULT";
  }
  if (DefaultValueName == NULL) {
    DefaultValueName = "DEFAULT";
  }
  for (Index = 0; Index < PcdListLength; Index++) {
    if (strcmp(PcdList[Index].TokenSpaceGuidName, TokenSpaceGuidName) != 0) {
      continue;
    }
    if (strcmp(PcdList[Index].TokenName, TokenName) != 0) {
      continue;
    }
    if (strcmp(PcdList[Index].SkuName, SkuName) != 0) {
      continue;
    }
    if (strcmp(PcdList[Index].DefaultValueName, DefaultValueName) != 0) {
      continue;
    }
    return Index;
  }
  return -1;
}

UINT64
__PcdGet (
  CHAR8  *SkuName             OPTIONAL,
  CHAR8  *DefaultValueName    OPTIONAL,
  CHAR8  *TokenSpaceGuidName,
  CHAR8  *TokenName
  )
/*++

Routine Description:

  Get PCD value

Arguments:

  SkuName               SkuName String
  DefaultValueName      DefaultValueName String
  TokenSpaceGuidName    TokenSpaceGuidName String
  TokenName             TokenName String

Returns:

  PCD value
--*/
{
  int    Index;
  CHAR8  *End;

  Index = LookupPcdIndex (SkuName, DefaultValueName, TokenSpaceGuidName, TokenName);
  if (Index < 0) {
    fprintf (stderr, "PCD %s.%s.%s.%s is not in database\n", SkuName, DefaultValueName, TokenSpaceGuidName, TokenName);
    exit (EXIT_FAILURE);
  }
  switch (PcdList[Index].PcdDataType) {
  case PcdDataTypeBoolean:
  case PcdDataTypeUint8:
  case PcdDataTypeUint16:
  case PcdDataTypeUint32:
    return (UINT64)strtoul(PcdList[Index].Value, &End, 16);
    break;
  case PcdDataTypeUint64:
    return (UINT64)strtoul(PcdList[Index].Value, &End, 16);
    break;
  case PcdDataTypePointer:
    fprintf (stderr, "PCD %s.%s.%s.%s is structure.  Use PcdGetPtr()\n", SkuName, DefaultValueName, TokenSpaceGuidName, TokenName);
    exit (EXIT_FAILURE);
    break;
  }
  return 0;
}

VOID
__PcdSet (
  CHAR8   *SkuName             OPTIONAL,
  CHAR8   *DefaultValueName    OPTIONAL,
  CHAR8   *TokenSpaceGuidName,
  CHAR8   *TokenName,
  UINT64  Value
  )
/*++

Routine Description:

  Set PCD value

Arguments:

  SkuName               SkuName String
  DefaultValueName      DefaultValueName String
  TokenSpaceGuidName    TokenSpaceGuidName String
  TokenName             TokenName String
  Value                 PCD value to be set

Returns:

  None
--*/
{
  int    Index;

  Index = LookupPcdIndex (SkuName, DefaultValueName, TokenSpaceGuidName, TokenName);
  if (Index < 0) {
    fprintf (stderr, "PCD %s.%s.%s.%s is not in database\n", SkuName, DefaultValueName, TokenSpaceGuidName, TokenName);
    exit (EXIT_FAILURE);
  }
  free(PcdList[Index].Value);
  PcdList[Index].Value = malloc(20);
  switch (PcdList[Index].PcdDataType) {
  case PcdDataTypeBoolean:
    if (Value == 0) {
      strcpy (PcdList[Index].Value, "0x00");
    } else {
      strcpy (PcdList[Index].Value, "0x01");
    }
    break;
  case PcdDataTypeUint8:
    sprintf(PcdList[Index].Value, "0x%02x", (UINT8)(Value & 0xff));
    break;
  case PcdDataTypeUint16:
    sprintf(PcdList[Index].Value, "0x%04x", (UINT16)(Value & 0xffff));
    break;
  case PcdDataTypeUint32:
    sprintf(PcdList[Index].Value, "0x%08x", (UINT32)(Value & 0xffffffff));
    break;
  case PcdDataTypeUint64:
    sprintf(PcdList[Index].Value, "0x%016llx", (unsigned long long)Value);
    break;
  case PcdDataTypePointer:
    fprintf (stderr, "PCD %s.%s.%s.%s is structure.  Use PcdSetPtr()\n", SkuName, DefaultValueName, TokenSpaceGuidName, TokenName);
    exit (EXIT_FAILURE);
    break;
  }
}

VOID *
__PcdGetPtr (
  CHAR8   *SkuName             OPTIONAL,
  CHAR8   *DefaultValueName    OPTIONAL,
  CHAR8   *TokenSpaceGuidName,
  CHAR8   *TokenName,
  UINT32  *Size
  )
/*++

Routine Description:

  Get PCD value buffer

Arguments:

  SkuName               SkuName String
  DefaultValueName      DefaultValueName String
  TokenSpaceGuidName    TokenSpaceGuidName String
  TokenName             TokenName String
  Size                  Size of PCD value buffer

Returns:

  PCD value buffer
--*/
{
  int    Index;
  CHAR8   *Value;
  UINT8   *Buffer;
  CHAR8   *End;

  Index = LookupPcdIndex (SkuName, DefaultValueName, TokenSpaceGuidName, TokenName);
  if (Index < 0) {
    fprintf (stderr, "PCD %s.%s.%s.%s is not in database\n", SkuName, DefaultValueName, TokenSpaceGuidName, TokenName);
    exit (EXIT_FAILURE);
  }
  switch (PcdList[Index].PcdDataType) {
  case PcdDataTypeBoolean:
  case PcdDataTypeUint8:
  case PcdDataTypeUint16:
  case PcdDataTypeUint32:
  case PcdDataTypeUint64:
    fprintf (stderr, "PCD %s.%s.%s.%s is a value.  Use PcdGet()\n", SkuName, DefaultValueName, TokenSpaceGuidName, TokenName);
    exit (EXIT_FAILURE);
    break;
  case PcdDataTypePointer:
    Value = &PcdList[Index].Value[1];
    for (*Size = 0, strtoul(Value, &End, 16); Value != End; strtoul(Value, &End, 16), *Size = *Size + 1) {
      Value = End + 1;
    }
    Buffer = malloc(*Size + 1);
    if (Buffer == NULL) {
      *Size = 0;
      return NULL;
    }
    Value = &PcdList[Index].Value[1];
    for (*Size = 0, Buffer[*Size] = (UINT8) strtoul(Value, &End, 16); Value != End; *Size = *Size + 1, Buffer[*Size] = (UINT8) strtoul(Value, &End, 16)) {
      Value = End + 1;
    }
    return Buffer;
  }
  *Size = 0;
  return 0;
}

VOID
__PcdSetPtr (
  CHAR8   *SkuName             OPTIONAL,
  CHAR8   *DefaultValueName    OPTIONAL,
  CHAR8   *TokenSpaceGuidName,
  CHAR8   *TokenName,
  UINT32  Size,
  UINT8   *Value
  )
/*++

Routine Description:

  Set PCD value buffer

Arguments:

  SkuName               SkuName String
  DefaultValueName      DefaultValueName String
  TokenSpaceGuidName    TokenSpaceGuidName String
  TokenName             TokenName String
  Size                  Size of PCD value
  Value                 Pointer to the updated PCD value buffer

Returns:

  None
--*/
{
  int    Index;
  UINT32  ValueIndex;

  Index = LookupPcdIndex (SkuName, DefaultValueName, TokenSpaceGuidName, TokenName);
  if (Index < 0) {
    fprintf (stderr, "PCD %s.%s.%s.%s is not in database\n", SkuName, DefaultValueName, TokenSpaceGuidName, TokenName);
    exit (EXIT_FAILURE);
  }
  switch (PcdList[Index].PcdDataType) {
  case PcdDataTypeBoolean:
  case PcdDataTypeUint8:
  case PcdDataTypeUint16:
  case PcdDataTypeUint32:
  case PcdDataTypeUint64:
    fprintf (stderr, "PCD %s.%s.%s.%s is a value.  Use PcdGet()\n", SkuName, DefaultValueName, TokenSpaceGuidName, TokenName);
    exit (EXIT_FAILURE);
    break;
  case PcdDataTypePointer:
    free(PcdList[Index].Value);
    PcdList[Index].Value = malloc(Size * 5 + 3);
    PcdList[Index].Value[0] = '{';
    for (ValueIndex = 0; ValueIndex < Size; ValueIndex++) {
      sprintf(&PcdList[Index].Value[1 + ValueIndex * 5], "0x%02x,", Value[ValueIndex]);
    }
    PcdList[Index].Value[1 + Size * 5 - 1] = '}';
    PcdList[Index].Value[1 + Size * 5    ] = 0;
    break;
  }
}

VOID
STATIC
ReadInputFile (
  CHAR8   *InputFileName,
  UINT8   **FileBuffer,
  UINT32  *FileSize
  )
/*++

Routine Description:

  Read the file buffer from the input file.

Arguments:

  InputFileName Point to the input file name.
  FileBuffer    Point to the input file buffer.
  FileSize      Size of the file buffer.

Returns:

  None
--*/
{
  FILE    *InputFile;
  UINT32  BytesRead;

  //
  // Open Input file and read file data.
  //
  InputFile = fopen (InputFileName, "rb");
  if (InputFile == NULL) {
    fprintf (stderr, "Error opening file %s\n", InputFileName);
    exit (EXIT_FAILURE);
  }

  //
  // Go to the end so that we can determine the file size
  //
  if (fseek (InputFile, 0, SEEK_END)) {
    fprintf (stderr, "Error reading input file %s\n", InputFileName);
    fclose (InputFile);
    exit (EXIT_FAILURE);
  }

  //
  // Get the file size
  //
  *FileSize = ftell (InputFile);
  if (*FileSize == -1) {
    fprintf (stderr, "Error parsing the input file %s\n", InputFileName);
    fclose (InputFile);
    exit (EXIT_FAILURE);
  }

  //
  // Allocate a buffer
  //
  *FileBuffer = malloc (*FileSize);
  if (*FileBuffer == NULL) {
    fprintf (stderr, "Can not allocate buffer for input input file %s\n", InputFileName);
    fclose (InputFile);
    exit (EXIT_FAILURE);
  }

  //
  // Reset to the beginning of the file
  //
  if (fseek (InputFile, 0, SEEK_SET)) {
    fprintf (stderr, "Error reading the input file %s\n", InputFileName);
    fclose (InputFile);
    free (*FileBuffer);
    exit (EXIT_FAILURE);
  }

  //
  // Read all of the file contents.
  //
  BytesRead = fread (*FileBuffer, sizeof (UINT8), *FileSize, InputFile);
  if (BytesRead != *FileSize * sizeof (UINT8)) {
    fprintf (stderr, "Error reading the input file %s\n", InputFileName);
    fclose (InputFile);
    free (*FileBuffer);
    exit (EXIT_FAILURE);
  }

  //
  // Close the file
  //
  fclose (InputFile);
}

VOID
STATIC
ParseFile (
  UINT8   *FileBuffer,
  UINT32  FileSize
  )
/*++

Routine Description:

  Read the initial PCD value from the input file buffer.

Arguments:

  FileBuffer  Point to the input file buffer.
  FileSize    Size of the file buffer.

Returns:

  None
--*/
{
  UINT32  Index;
  UINT32  NumLines;
  UINT32  TokenIndex;
  UINT32  TokenStart;

  for (Index = 0, NumLines = 0; Index < FileSize; Index++) {
    if (FileBuffer[Index] == '\n') {
      NumLines++;
    }
  }
  PcdList = malloc((NumLines + 1) * sizeof(PcdList[0]));

  for (Index = 0, TokenIndex = 0, PcdListLength = 0, TokenStart = 0; Index < FileSize; Index++) {
    if (FileBuffer[Index] == ' ') {
      continue;
    }
    if (FileBuffer[Index] == '|' || FileBuffer[Index] == '.' || FileBuffer[Index] == '\n' || FileBuffer[Index] == '\r') {
      RecordToken (FileBuffer, PcdListLength, TokenIndex, TokenStart, Index);
      if (FileBuffer[Index] == '\n' || FileBuffer[Index] == '\r') {
        if (TokenIndex != 0) {
          PcdListLength++;
          TokenIndex = 0;
        }
      } else {
        TokenIndex++;
      }
      TokenStart = Index + 1;
      continue;
    }
  }
  if (Index > TokenStart) {
    RecordToken (FileBuffer, PcdListLength, TokenIndex, TokenStart, Index);
    if (TokenIndex != 0) {
      PcdListLength++;
    }
  }
}

VOID
STATIC
WriteOutputFile (
  CHAR8   *OutputFileName
  )
/*++

Routine Description:

  Write the updated PCD value into the output file name.

Arguments:

  OutputFileName  Point to the output file name.

Returns:

  None
--*/
{
  FILE    *OutputFile;
  UINT32  Index;

  //
  // Open output file
  //
  OutputFile = fopen (OutputFileName, "wb");
  if (OutputFile == NULL) {
    fprintf (stderr, "Error opening file %s\n", OutputFileName);
    exit (EXIT_FAILURE);
  }

  for (Index = 0; Index < PcdListLength; Index++) {
    fprintf (
      OutputFile,
      "%s.%s.%s.%s|%s|%s\n",
      PcdList[Index].SkuName,
      PcdList[Index].DefaultValueName,
      PcdList[Index].TokenSpaceGuidName,
      PcdList[Index].TokenName,
      PcdList[Index].DataType,
      PcdList[Index].Value
      );
  }

  //
  // Done, write output file.
  //
  if (OutputFile != NULL) {
    fclose (OutputFile);
  }
}

VOID
STATIC
Usage (
  VOID
  )
/*++

Routine Description:

  Displays the utility usage syntax to STDOUT

Arguments:

  None

Returns:

  None

--*/
{
  fprintf (stdout, "Usage: -i <input_file> -o <output_file>\n\n");
  fprintf (stdout, "optional arguments:\n");
  fprintf (stdout, "  -h, --help            Show this help message and exit\n");
  fprintf (stdout, "  -i INPUT_FILENAME, --input INPUT_FILENAME\n\
                        PCD Database Input file name\n");
  fprintf (stdout, "  -o OUTPUT_FILENAME, --output OUTPUT_FILENAME\n\
                        PCD Database Output file name\n");
}

VOID
STATIC
ParseArguments (
  int    argc,
  char   *argv[],
  CHAR8  **InputFileName,
  CHAR8  **OutputFileName
  )
/*++

Routine Description:

  Parse the input parameters to get the input/output file name.

Arguments:

  argc            Number of command line parameters.
  argv            Array of pointers to parameter strings.
  InputFileName   Point to the input file name.
  OutputFileName  Point to the output file name.

Returns:

  None
--*/
{
  if (argc == 1) {
    fprintf (stderr, "Missing options\n");
    exit (EXIT_FAILURE);
  }

  //
  // Parse command line
  //
  argc--;
  argv++;

  if ((stricmp (argv[0], "-h") == 0) || (stricmp (argv[0], "--help") == 0)) {
    Usage ();
    exit (EXIT_SUCCESS);
  }

  while (argc > 0) {
    if ((stricmp (argv[0], "-i") == 0) || (stricmp (argv[0], "--input") == 0)) {
      if (argv[1] == NULL || argv[1][0] == '-') {
        fprintf (stderr, "Invalid option value.  Input File name is missing for -i option\n");
        exit (EXIT_FAILURE);
      }
      *InputFileName = argv[1];
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-o") == 0) || (stricmp (argv[0], "--output") == 0)) {
      if (argv[1] == NULL || argv[1][0] == '-') {
        fprintf (stderr, "Invalid option value.  Output File name is missing for -i option\n");
        exit (EXIT_FAILURE);
      }
      *OutputFileName = argv[1];
      argc -= 2;
      argv += 2;
      continue;
    }

    if (argv[0][0] == '-') {
      fprintf (stderr, "Unknown option %s\n", argv[0]);
      exit (EXIT_FAILURE);
    }
    argc --;
    argv ++;
  }

  //
  // Check Input parameters
  //
  if (*InputFileName == NULL) {
    fprintf (stderr, "Missing option.  Input files is not specified\n");
    exit (EXIT_FAILURE);
  }

  if (*OutputFileName == NULL) {
    fprintf (stderr, "Missing option.  Output file is not specified\n");
    exit (EXIT_FAILURE);
  }
}

int
PcdValueMain (
  int   argc,
  char  *argv[]
  )
/*++

Routine Description:

  Main function updates PCD values.

Arguments:

  argc            Number of command line parameters.
  argv            Array of pointers to parameter strings.

Returns:
  EXIT_SUCCESS
--*/
{
  CHAR8   *InputFileName;
  CHAR8   *OutputFileName;
  UINT8   *FileBuffer;
  UINT32  FileSize;

  InputFileName = NULL;
  OutputFileName = NULL;

  //
  // Parse the input arguments
  //
  ParseArguments (argc, argv, &InputFileName, &OutputFileName);

  //
  // Open Input file and read file data.
  //
  ReadInputFile (InputFileName, &FileBuffer, &FileSize);

  //
  // Read the initial Pcd value
  //
  ParseFile (FileBuffer, FileSize);

  //
  // Customize PCD values in the PCD Database
  //
  PcdEntryPoint ();

  //
  // Save the updated PCD value
  //
  WriteOutputFile (OutputFileName);

  exit (EXIT_SUCCESS);
}
