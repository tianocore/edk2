/** @file
  Provides interface to shell MAN file parser.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SHELL_MAN_FILE_PARSER_HEADER_
#define _SHELL_MAN_FILE_PARSER_HEADER_

#include <Uefi.h>
#include <Library/ShellProtocolInteractivityLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/HiiPackageList.h>
#include <Protocol/Shell.h>

/**
  parses through the MAN file specified by SHELL_FILE_HANDLE and returns the
  detailed help for any sub section specified in the comma separated list of
  sections provided.  If the end of the file or a .TH section is found then
  return.

  Upon a successful return the caller is responsible to free the memory in *HelpText

  @param[in] Handle             FileHandle to read from
  @param[in] Sections           name of command's sub sections to find
  @param[out] HelpText          pointer to pointer to string where text goes.
  @param[out] HelpSize          pointer to size of allocated HelpText (may be updated)
  @param[in] Ascii              TRUE if the file is ASCII, FALSE otherwise.

  @retval EFI_OUT_OF_RESOURCES  a memory allocation failed.
  @retval EFI_SUCCESS           the section was found and its description stored in
                                an allocated buffer.
**/
EFI_STATUS
ManFileFindSections (
  IN SHELL_FILE_HANDLE  Handle,
  IN CONST CHAR16       *Sections,
  OUT CHAR16            **HelpText,
  OUT UINTN             *HelpSize,
  IN BOOLEAN            Ascii
  );

#endif //_SHELL_MAN_FILE_PARSER_HEADER_
