/** @file
  Include windows.h addressing conflicts with forced include of Base.h

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef WIN_INCLUDE_H_
#define WIN_INCLUDE_H_

#define GUID         _WINNT_DUP_GUID_____
#define _LIST_ENTRY  _WINNT_DUP_LIST_ENTRY_FORWARD
#define LIST_ENTRY   _WINNT_DUP_LIST_ENTRY
#undef  VOID

#pragma warning (push)
#pragma warning (disable : 4668)

#include <windows.h>

#pragma warning (pop)

#undef GUID
#undef _LIST_ENTRY
#undef LIST_ENTRY
#define VOID  void

#endif
