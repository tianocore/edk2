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

/**
  This function parses the tools_def.txt file.  It returns a
  EFI_HANDLE object which can be used for the other library
  functions and should be passed to FreeParsedToolsDefHandle
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
;

/**
  This function parses the tools_def.txt file.  It returns a
  EFI_HANDLE object which can be used for the other library
  functions and should be passed to FreeParsedToolsDefHandle
  to free resources when the tools_def.txt information is no
  longer needed.

  @param InputFile     Memory file image.

  @retval NULL if error parsing
  @retval A non-NULL EFI_HANDLE otherwise
**/
EFI_HANDLE
ParseGuidedSectionToolsMemoryFile (
  IN EFI_HANDLE    InputFile
  )
;

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
;

/**
  Frees resources that were allocated by ParseGuidedSectionToolsFile.
  After freeing these resources, the information that was parsed
  is no longer accessible.

  @param ParsedToolDefHandle   Handle returned from ParseGuidedSectionToolsFile

  @return EFI_STATUS
**/
EFI_STATUS
FreeParsedGuidedSectionToolsHandle (
  IN EFI_HANDLE ParsedGuidedSectionToolsHandle
  )
;

#endif
