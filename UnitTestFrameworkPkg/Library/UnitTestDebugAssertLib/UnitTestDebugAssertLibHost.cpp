/** @file
  Unit Test Debug Assert Library for host-based environments

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdexcept>

#ifdef NULL
  #undef NULL
#endif

extern "C" {
  #include <Uefi.h>
  #include <UnitTestFrameworkTypes.h>
  #include <Library/BaseLib.h>
  #include <Library/UnitTestLib.h>

  //
  // If address sanitizer is enabled, then declare the function that is used to
  // handle custom long jump implementation.
  //
 #ifdef __SANITIZE_ADDRESS__
  void
  __asan_handle_no_return (
    );

 #endif

  ///
  /// Point to jump buffer used with SetJump()/LongJump() to test if a function
  /// under test generates an expected ASSERT() condition.
  ///
  BASE_LIBRARY_JUMP_BUFFER  *gUnitTestExpectAssertFailureJumpBuffer = NULL;

  /**
    Unit test library replacement for DebugAssert() in DebugLib.

    If FileName is NULL, then a <FileName> string of "(NULL) Filename" is printed.
    If Description is NULL, then a <Description> string of "(NULL) Description" is printed.

    @param  FileName     The pointer to the name of the source file that generated the assert condition.
    @param  LineNumber   The line number in the source file that generated the assert condition
    @param  Description  The pointer to the description of the assert condition.

  **/
  VOID
  EFIAPI
  UnitTestDebugAssert (
    IN CONST CHAR8  *FileName,
    IN UINTN        LineNumber,
    IN CONST CHAR8  *Description
    )
  {
    CHAR8  Message[256];

    if (gUnitTestExpectAssertFailureJumpBuffer != NULL) {
      UT_LOG_INFO ("Detected expected ASSERT: %a(%d): %a\n", FileName, LineNumber, Description);

      //
      // If address sanitizer is enabled, then inform sanitizer that a no return
      // function is being called that will reset to a previous stack frame.
      // This is required to avoid false positives from the address sanitizer
      // due to the use of a custom long jump implementation.
      //
 #ifdef __SANITIZE_ADDRESS__
      __asan_handle_no_return ();
 #endif

      LongJump (gUnitTestExpectAssertFailureJumpBuffer, 1);
    } else {
      if (GetActiveFrameworkHandle () != NULL) {
        AsciiStrCpyS (Message, sizeof (Message), "Detected unexpected ASSERT(");
        AsciiStrCatS (Message, sizeof (Message), Description);
        AsciiStrCatS (Message, sizeof (Message), ")");
        UnitTestAssertTrue (FALSE, "", LineNumber, FileName, Message);
      } else {
        snprintf (Message, sizeof (Message), "Detected unexpected ASSERT: %s(%d): %s\n", FileName, (INT32)(UINT32)LineNumber, Description);
        throw std::runtime_error (Message);
      }
    }
  }
}
