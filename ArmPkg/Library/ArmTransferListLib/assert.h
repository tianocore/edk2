/** @file
  Standard C assert header support for EDK2.

  This header provides assert functionality for third-party libraries
  that expect standard C library headers.

Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ASSERT_H
#define _ASSERT_H

#include <Library/DebugLib.h>

//
// Map standard C assert to EDK2 ASSERT
//
#ifdef assert
  #undef assert
#endif

#define assert(Expression)  ASSERT(Expression)

#endif /* _ASSERT_H */
