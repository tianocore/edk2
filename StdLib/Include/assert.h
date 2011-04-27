/** @file
  Provides a definition of the assert macro.

  This header file defines the assert macro and refers to the NDEBUG macro,
  which is NOT defined in this file.

  Unlike other header files, assert.h is designed to be included multiple
  times, with potentially different behavior on each inclusion.

  If the NDEBUG macro is defined at the point where assert.h
  is included, the assert macro is defined so as to not produce code.
  Otherwise, the assertion is tested and if the assertion is true a
  diagnostic message of the form
  "ASSERT <FileName>(<LineNumber>): <Description>\n" is produced.
  A true assertion will also result in the application being terminated.

  The behavior of the assert macro can be further modified by setting attributes
  in the PcdDebugPropertyMask PCD entry when building the Application Toolkit.
  If DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED bit of PcdDebugProperyMask is set
  then CpuBreakpoint() is called. Otherwise, if the
  DEBUG_PROPERTY_ASSERT_DEADLOOP_ENABLED bit of PcdDebugProperyMask is set then
  CpuDeadLoop() is called.  If neither of these bits are set, then the
  application will be terminated immediately after the message is printed to
  the debug output device.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include  <sys/EfiCdefs.h>

#undef  assert        ///< Remove any existing definition for assert.

extern void
__assert(const char *func, const char *file, int line, const char *failedexpr);

/** The assert macro puts diagnostic tests into programs; it expands to a
    void expression.

    When it is executed, if expression (which shall have a scalar type) is
    false (that is, compares equal to 0), the assert macro writes information
    about the particular call that failed (including the text of the argument,
    the name of the source file, the source line number, and the name of the
    enclosing function - the latter are respectively the values of the
    preprocessing macros __FILE__ and __LINE__ and of the identifier __func__)
    on the standard error stream. It then calls the abort function.

  If NDEBUG is not defined, Expression is evaluated.  If Expression evaluates to FALSE, then
  __assert is called passing in the source filename, source function, source line number,
  and the Expression.

  @param  Expression  Boolean expression.

@{
**/
#ifdef  NDEBUG
#define assert(Expression)  /* ignored */

#else
#define assert(Expression)   ((Expression) ? (void)0 :\
                              __assert(__func__, __FILE__, __LINE__, #Expression) )
#endif
/// @}
/* END of file assert.h */
