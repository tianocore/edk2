/** @file

 The API to create the binary.

 Copyright (c) 2011-2019, Intel Corporation. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BinaryCreate.h"
#ifndef __GNUC__
#define GENSEC_RAW      "GenSec -s %s \"%s\" -o \"%s\" > NUL"
#else
#define GENSEC_RAW      "GenSec -s %s \"%s\" -o \"%s\" > /dev/null"
#endif

//
// The guid is to for FFS of BFV.
//
EFI_GUID gEfiFfsBfvForMultiPlatformGuid = EFI_FFS_BFV_FOR_MULTIPLATFORM_GUID;
EFI_GUID gEfiFfsBfvForMultiPlatformGuid2 = EFI_FFS_BFV_FOR_MULTIPLATFORM_GUID2;

/**
  Convert a GUID to a string.

  @param[in]   Guid    Pointer to GUID to print.

  @return The string after convert.
**/
static
CHAR8 *
LibBfmGuidToStr (
  IN  EFI_GUID  *Guid
)
{
  CHAR8 * Buffer;

  Buffer = NULL;

  if (Guid == NULL) {
    return NULL;
  }

  Buffer = (CHAR8 *) malloc (36 + 1);

  if (Buffer == NULL) {
    return NULL;
  }
  memset (Buffer, '\0', 36 + 1);

  sprintf (
      Buffer,
      "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      Guid->Data1,
      Guid->Data2,
      Guid->Data3,
      Guid->Data4[0],
      Guid->Data4[1],
      Guid->Data4[2],
      Guid->Data4[3],
      Guid->Data4[4],
      Guid->Data4[5],
      Guid->Data4[6],
      Guid->Data4[7]
      );

  return Buffer;
}

/**
  Create the Ras section in FFS

  @param[in]   InputFilePath   .efi file, it's optional unless process PE/TE section.
  @param[in]   OutputFilePath  .te or .pe file

  @retval EFI_SUCCESS

**/
EFI_STATUS
CreateRawSection (
  IN CHAR8*     InputFilePath,
  IN CHAR8*     OutputFilePath
  )
{
  INT32        ReturnValue;
  CHAR8*       SystemCommand;

  SystemCommand             = NULL;
  SystemCommand = malloc (
    strlen (GENSEC_RAW) +
    strlen ("EFI_SECTION_RAW") +
    strlen (InputFilePath) +
    strlen (OutputFilePath) +
    1
  );
  if (NULL == SystemCommand) {
    return EFI_OUT_OF_RESOURCES;
  }
  sprintf (
    SystemCommand,
    GENSEC_RAW,
    "EFI_SECTION_RAW",
    InputFilePath,
    OutputFilePath
  );
  ReturnValue = system (SystemCommand);
  free(SystemCommand);

  if (ReturnValue != 0) {
    printf ("Error. Call GenSec failed.\n");
    return EFI_ABORTED;
  }
  return EFI_SUCCESS;
}

/**
  Create the Ras type of FFS

  @param[in]   InputFilePath   .efi file, it's optional unless process PE/TE section.
  @param[in]   OutputFilePath  .te or .pe file

  @retval EFI_SUCCESS

**/
EFI_STATUS
CreateRawFfs (
  IN CHAR8**    InputFilePaths,
  IN CHAR8*     OutputFilePath,
  IN BOOLEAN    SizeOptimized
  )
{
  INT32        ReturnValue;
  CHAR8*       SystemCommandFormatString;
  CHAR8*       SystemCommand;
  CHAR8*       GuidStr;
  CHAR8*       FilePathFormatStr;
  CHAR8*       FilePathStr;
  UINT32       Index;
  UINT32       StrLen;
  UINT32       Size;

  SystemCommand    = NULL;
  GuidStr          = NULL;
  FilePathStr      = NULL;
  StrLen           = 0;

  FilePathFormatStr = " -i \"";

  for (Index = 0; InputFilePaths[Index] != NULL; Index++) {
    Size = strlen (FilePathFormatStr) + strlen (InputFilePaths[Index]) + 2; // 2 menas "" "
    if (FilePathStr == NULL) {
      FilePathStr = malloc (Size);
      if (NULL == FilePathStr) {
        return EFI_OUT_OF_RESOURCES;
      }
    } else {
      FilePathStr = realloc (FilePathStr, StrLen + Size);
      if (NULL == FilePathStr) {
        return EFI_OUT_OF_RESOURCES;
      }
    }
    memset (FilePathStr + StrLen, ' ', Size);
    memcpy (FilePathStr + StrLen, FilePathFormatStr, strlen(FilePathFormatStr));
    memcpy(FilePathStr + StrLen + strlen(FilePathFormatStr), InputFilePaths[Index], strlen(InputFilePaths[Index]));
    StrLen += Size;
    *(FilePathStr + StrLen - 2) = '\"';
  }
   if (FilePathStr == NULL) {
    return EFI_ABORTED;
  }
  *(FilePathStr + StrLen - 1)= '\0';


  if (SizeOptimized) {
    GuidStr = LibBfmGuidToStr(&gEfiFfsBfvForMultiPlatformGuid2);
  } else {
    GuidStr  = LibBfmGuidToStr(&gEfiFfsBfvForMultiPlatformGuid);
  }
  if (NULL == GuidStr) {
    free (FilePathStr);
    return EFI_OUT_OF_RESOURCES;
  }
  SystemCommandFormatString = "GenFfs -t %s %s -g %s -o \"%s\"";
  SystemCommand = malloc (
    strlen (SystemCommandFormatString) +
    strlen ("EFI_FV_FILETYPE_FREEFORM") +
    strlen (FilePathStr) +
    strlen (GuidStr) +
    strlen (OutputFilePath) +
    1
    );
  if (NULL == SystemCommand) {
    free (GuidStr);
    free (FilePathStr);
    return EFI_OUT_OF_RESOURCES;
  }
  sprintf (
    SystemCommand,
    "GenFfs -t %s %s -g %s -o \"%s\"",
    "EFI_FV_FILETYPE_FREEFORM",// -t
    FilePathStr,               // -i
    GuidStr,                   // -g
    OutputFilePath             // -o
    );
  ReturnValue = system (SystemCommand);
  free(SystemCommand);
  free (FilePathStr);
  free (GuidStr);

  if (ReturnValue != 0) {
    printf ("Error. Call GenFfs failed.\n");
    return EFI_ABORTED;
  }
  return EFI_SUCCESS;
}

