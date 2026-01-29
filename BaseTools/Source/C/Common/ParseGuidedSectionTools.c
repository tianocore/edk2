/** @file
Helper functions for parsing GuidedSectionTools.txt

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "MemoryFile.h"
#include "CommonLib.h"
#include "EfiUtilityMsgs.h"
#include "ParseInf.h"
#include "ParseGuidedSectionTools.h"
#include "StringFuncs.h"


//
// Local types / structures
//

typedef struct _GUID_SEC_TOOL_ENTRY {
  EFI_GUID   Guid;
  CHAR8*     Name;
  CHAR8*     Path;
  struct _GUID_SEC_TOOL_ENTRY *Next;
} GUID_SEC_TOOL_ENTRY;

//
// Function Implementation
//

/**
  This function parses the tools_def.txt file.  It returns a
  EFI_HANDLE object which can be used for the other library
  functions and should be passed to FreeParsedGuidedSectionToolsHandle
  to free resources when the tools_def.txt information is no
  longer needed.

  @param InputFile     Path name of file to read

  @retval NULL if error parsing
  @retval A non-NULL EFI_HANDLE otherwise
**/
EFI_HANDLE
ParseGuidedSectionToolsFile (
  IN CHAR8    *InputFile
  )
{
  EFI_STATUS Status;
  EFI_HANDLE MemoryFile;
  EFI_HANDLE ParsedGuidedSectionTools;

  Status = GetMemoryFile (InputFile, &MemoryFile);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  ParsedGuidedSectionTools = ParseGuidedSectionToolsMemoryFile (MemoryFile);

  FreeMemoryFile (MemoryFile);

  return ParsedGuidedSectionTools;
}

/**
  This function parses the tools_def.txt file.  It returns a
  EFI_HANDLE object which can be used for the other library
  functions and should be passed to FreeParsedGuidedSectionToolsHandle
  to free resources when the tools_def.txt information is no
  longer needed.

  @param InputFile     Memory file image.

  @retval NULL if error or EOF
  @retval InputBuffer otherwise
**/
EFI_HANDLE
ParseGuidedSectionToolsMemoryFile (
  IN EFI_HANDLE    InputFile
  )
{
  EFI_STATUS  Status;
  CHAR8       *NextLine;
  STRING_LIST *Tool;
  EFI_GUID    Guid;
  GUID_SEC_TOOL_ENTRY *FirstGuidTool;
  GUID_SEC_TOOL_ENTRY *LastGuidTool;
  GUID_SEC_TOOL_ENTRY *NewGuidTool;

  FirstGuidTool = NULL;
  LastGuidTool  = NULL;

  while (TRUE) {
    NextLine = ReadMemoryFileLine (InputFile);
    if (NextLine == NULL) {
      break;
    }

    Status = StripInfDscStringInPlace (NextLine);
    if (EFI_ERROR (Status)) {
      free (NextLine);
      break;
    }

    if (NextLine[0] == '\0') {
      free (NextLine);
      continue;
    }

    Tool = SplitStringByWhitespace (NextLine);
    if ((Tool != NULL) &&
        (Tool->Count == 3)
       ) {
      Status = StringToGuid (Tool->Strings[0], &Guid);
      if (!EFI_ERROR (Status)) {
        NewGuidTool = malloc (sizeof (GUID_SEC_TOOL_ENTRY));
        if (NewGuidTool != NULL) {
          memcpy (&(NewGuidTool->Guid), &Guid, sizeof (Guid));
          NewGuidTool->Name = CloneString(Tool->Strings[1]);
          NewGuidTool->Path = CloneString(Tool->Strings[2]);
          NewGuidTool->Next = NULL;

          if (FirstGuidTool == NULL) {
            FirstGuidTool = NewGuidTool;
          } else {
            LastGuidTool->Next = NewGuidTool;
          }
          LastGuidTool = NewGuidTool;
        }
      }
    }

    if (Tool != NULL) {
      FreeStringList (Tool);
    }
    free (NextLine);
  }

  return FirstGuidTool;
}

/**
  This function looks up the appropriate tool to use for extracting
  a GUID defined FV section.

  @param ParsedGuidedSectionToolsHandle    A parsed GUID section tools handle.
  @param SectionGuid                       The GUID for the section.

  @retval NULL     if no tool is found or there is another error
  @retval Non-NULL The tool to use to access the section contents.  (The caller
             must free the memory associated with this string.)
**/
CHAR8*
LookupGuidedSectionToolPath (
  IN EFI_HANDLE ParsedGuidedSectionToolsHandle,
  IN EFI_GUID   *SectionGuid
  )
{
  GUID_SEC_TOOL_ENTRY *GuidTool;

  GuidTool = (GUID_SEC_TOOL_ENTRY*)ParsedGuidedSectionToolsHandle;
  if (GuidTool == NULL) {
    return NULL;
  }

  for ( ; GuidTool != NULL; GuidTool = GuidTool->Next) {
    if (CompareGuid (&(GuidTool->Guid), SectionGuid) == 0) {
      return CloneString (GuidTool->Path);
    }
  }

  return NULL;
}


