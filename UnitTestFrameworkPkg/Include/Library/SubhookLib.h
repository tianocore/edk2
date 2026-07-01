/** @file
  SubhookLib class with APIs from the subhook project

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#if defined (__i386__) || defined (__amd64__) || defined (__x86_64__) || defined (_M_IX86) || defined (_M_AMD64) || defined (_M_X64)

#define SUBHOOK_STATIC
  #include <subhook.h>

#else

//
// Stub for non-x86 hosts: allows FunctionMockLib.h to compile but the
// MOCK_FUNCTION_INTERNAL_* macros are not functional on this architecture
// (subhook has no hooking backend for it).
//
  #ifdef __cplusplus

namespace subhook {
class Hook {
public:
  Hook (
        )
  {
  }

  Hook (
        void *,
        void *
        )
  {
  }

  bool
  Install (
    )
  {
    return false;
  }

  bool
  Install (
    void *,
    void *
    )
  {
    return false;
  }

  bool
  Remove (
    )
  {
    return false;
  }

  bool
  IsInstalled (
    ) const
  {
    return false;
  }

private:
  Hook (
        const Hook &
        );
  void
  operator= (
    const Hook &
    );
};
} // namespace subhook

  #endif // __cplusplus

#endif
