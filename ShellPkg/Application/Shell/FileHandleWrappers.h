/** @file
  EFI_FILE_PROTOCOL wrappers for other items (Like Environment Variables, StdIn, StdOut, StdErr, etc...)

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SHELL_FILE_HANDLE_WRAPPERS_HEADER_
#define _SHELL_FILE_HANDLE_WRAPPERS_HEADER_

typedef struct {
  LIST_ENTRY    Link;
  CHAR16        *Buffer;
} SHELL_LINE_LIST;

typedef struct {
  UINTN              LogCount;
  SHELL_LINE_LIST    *Log;
} SHELL_LINE_LOG;

#endif //_SHELL_FILE_HANDLE_WRAPPERS_HEADER_
