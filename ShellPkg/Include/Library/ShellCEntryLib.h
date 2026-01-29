/** @file
  Provides application point extension for "C" style main function.

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SHELL_C_ENTRY_LIB_
#define _SHELL_C_ENTRY_LIB_

/**
  UEFI application entry point which has an interface similar to a
  standard C main function.

  The ShellCEntryLib library instance wrappers the actual UEFI application
  entry point and calls this ShellAppMain function.

  @param[in]  Argc  The number of parameters.
  @param[in]  Argv  The array of pointers to parameters.

  @retval  0               The application exited normally.
  @retval  Other           An error occurred.

**/
INTN
EFIAPI
ShellAppMain (
  IN UINTN   Argc,
  IN CHAR16  **Argv
  );

#endif
