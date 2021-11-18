/** @file
  Provides interface to shell MAN file parser.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SHELL_MAN_FILE_PARSER_HEADER_
#define _SHELL_MAN_FILE_PARSER_HEADER_

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
