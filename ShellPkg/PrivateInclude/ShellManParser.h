/** @file
  Provides interface to shell MAN file parser.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __UEFI_SHELL_INTERNALS_SHELL_MAN_FILE_PARSER__
#define __UEFI_SHELL_INTERNALS_SHELL_MAN_FILE_PARSER__

#include <Uefi.h>

/**
  This function returns the help information for the specified command. The help text
  will be parsed from a UEFI Shell manual page. (see UEFI Shell 2.0 Appendix B)

  If Sections is specified, then each section name listed will be compared in a casesensitive
  manner, to the section names described in Appendix B. If the section exists,
  it will be appended to the returned help text. If the section does not exist, no
  information will be returned. If Sections is NULL, then all help text information
  available will be returned.

  if BriefDesc is NULL, then the breif description will not be savedd separately,
  but placed first in the main HelpText.

  @param[in] ManFileName        Points to the NULL-terminated UEFI Shell MAN file name.
  @param[in] Command            Points to the NULL-terminated UEFI Shell command name.
  @param[in] Sections           Points to the NULL-terminated comma-delimited
                                section names to return. If NULL, then all
                                sections will be returned.
  @param[out] BriefDesc         On return, points to a callee-allocated buffer
                                containing brief description text.
  @param[out] HelpText          On return, points to a callee-allocated buffer
                                containing all specified help text.

  @retval EFI_SUCCESS           The help text was returned.
  @retval EFI_OUT_OF_RESOURCES  The necessary buffer could not be allocated to hold the
                                returned help text.
  @retval EFI_INVALID_PARAMETER HelpText is NULL
  @retval EFI_NOT_FOUND         There is no help text available for Command.
**/
EFI_STATUS
ProcessManFile (
  IN CONST CHAR16  *ManFileName,
  IN CONST CHAR16  *Command,
  IN CONST CHAR16  *Sections OPTIONAL,
  OUT CHAR16       **BriefDesc,
  OUT CHAR16       **HelpText
  );

#endif //__UEFI_SHELL_INTERNALS_SHELL_MAN_FILE_PARSER__
