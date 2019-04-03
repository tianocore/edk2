/** @file
Header file for helper functions for parsing GuidedSectionTools.txt

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_PARSE_GUIDED_SECTION_TOOLS_H
#define _EFI_PARSE_GUIDED_SECTION_TOOLS_H

#include <Common/UefiBaseTypes.h>

//
// Functions declarations
//

EFI_HANDLE
ParseGuidedSectionToolsFile (
  IN CHAR8    *InputFile
  )
;
/**

Routine Description:

  This function parses the tools_def.txt file.  It returns a
  EFI_HANDLE object which can be used for the other library
  functions and should be passed to FreeParsedToolsDefHandle
  to free resources when the tools_def.txt information is no
  longer needed.

Arguments:

  InputFile     Path name of file to read

Returns:

  NULL if error parsing
  A non-NULL EFI_HANDLE otherwise

**/


EFI_HANDLE
ParseGuidedSectionToolsMemoryFile (
  IN EFI_HANDLE    InputFile
  )
;
/**

Routine Description:

  This function parses the tools_def.txt file.  It returns a
  EFI_HANDLE object which can be used for the other library
  functions and should be passed to FreeParsedToolsDefHandle
  to free resources when the tools_def.txt information is no
  longer needed.

Arguments:

  InputFile     Memory file image.

Returns:

  NULL if error parsing
  A non-NULL EFI_HANDLE otherwise

**/

CHAR8*
LookupGuidedSectionToolPath (
  IN EFI_HANDLE ParsedGuidedSectionToolsHandle,
  IN EFI_GUID   *SectionGuid
  )
;
/**

Routine Description:

  This function looks up the appropriate tool to use for extracting
  a GUID defined FV section.

Arguments:

  ParsedGuidedSectionToolsHandle    A parsed GUID section tools handle.
  SectionGuid                       The GUID for the section.

Returns:

  NULL     - if no tool is found or there is another error
  Non-NULL - The tool to use to access the section contents.  (The caller
             must free the memory associated with this string.)

**/

EFI_STATUS
FreeParsedGuidedSectionToolsHandle (
  IN EFI_HANDLE ParsedGuidedSectionToolsHandle
  )
;
/**

Routine Description:

  Frees resources that were allocated by ParseGuidedSectionToolsFile.
  After freeing these resources, the information that was parsed
  is no longer accessible.

Arguments:

  ParsedToolDefHandle   Handle returned from ParseGuidedSectionToolsFile

Returns:

  EFI_STATUS

**/

#endif
