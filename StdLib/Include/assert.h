/** @file
  Provides a definition of the assert macro used to insert diagnostic messages
  into code.

  This header file defines the assert macro and refers to the NDEBUG macro,
  which is NOT defined in this file.

  Unlike other header files, assert.h is designed to be included multiple
  times, with potentially different behavior on each inclusion.

  If the NDEBUG macro is defined at the point where assert.h
  is included, the assert macro is defined so as to not produce code.
  Otherwise, the assertion is tested and if the assertion is FALSE
  (e.g. evaluates to 0) a diagnostic message of the form<BR>
  "Assertion failed: (EXPR), file FILE, function FUNC, line LINE.\n"<BR>
  is produced.
  A FALSE evaluation will also result in the application being aborted.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <sys/EfiCdefs.h>

#undef  assert        ///< Remove any existing definition for assert.

/** Internal helper function for the assert macro.
    The __assert function prints a diagnostic message then exits the
    currently running application.

    This function should NEVER be called directly.

    Some pre-processors do not provide the __func__ identifier.  When that is
    the case, __func__ will be NULL.  This function accounts for this and
    will modify the diagnostic message appropriately.


    @param[in]    file          The name of the file containing the assert.
    @param[in]    func          The name of the function containing the assert.
    @param[in]    line          The line number the assert is located on.
    @param[in]    failedexpr    A literal representation of the assert's expression.

    @return       The __assert function will never return.  It aborts the
                  current application and returns to the environment that
                  the application was launched from.
**/
extern void
__assert(const char *file, const char *func, int line, const char *failedexpr);

/** The assert macro puts diagnostic tests into programs; it expands to a
    void expression.

    When it is executed, if expression (which must have a scalar type) is
    FALSE (that is, compares equal to 0), the assert macro writes information
    about the particular call that failed (including the text of the argument,
    the name of the source file, the source line number, and the name of the
    enclosing function - the latter are respectively the values of the
    preprocessing macros __FILE__ and __LINE__ and of the identifier __func__)
    on the standard error stream. It then calls the abort function.

  If NDEBUG is not defined, Expression is evaluated.  If Expression evaluates to FALSE,
  then __assert is called passing in the source filename, source function, source
  line number, and the Expression.

  @param  Expression  Boolean expression.

@{
**/
#ifdef  NDEBUG
#define assert(Expression)  /* ignored */

#else
#define assert(Expression)   ((Expression) ? (void)0 :\
                              __assert(__FILE__, __func__, __LINE__, #Expression) )
#endif
/// @}
/* END of file assert.h */
