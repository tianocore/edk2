/** @file
  The implementation of the __assert function used internally by the assert macro
  to insert diagnostic messages into code.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>

#include  <stdio.h>
#include  <stdlib.h>

/** Internal helper function for the assert macro.
    The __assert function prints a diagnostic message then exits the
    currently running application.

    This function should NEVER be called directly.

    Some pre-processors do not provide the __func__ identifier.  When that is
    the case, __func__ will be NULL.  This function accounts for this and
    will modify the diagnostic message appropriately.


    @param[in]    file          The name of the file containing the assert.
    @param[in]    func          The name of the function containing the assert
                                or NULL.
    @param[in]    line          The line number the assert is located on.
    @param[in]    failedexpr    A literal representation of the assert's expression.

    @return       The __assert function will never return.  It terminates execution
                  of the current application and returns to the environment that
                  the application was launched from.
**/
void
__assert(
  IN  const char *file,
  IN  const char *func,
  IN  int         line,
  IN  const char *failedexpr
  )
{
  if (func == NULL)
    printf("Assertion failed: (%s), file %s, line %d.\n",
                failedexpr, file, line);
  else
    printf("Assertion failed: (%s), file %s, function %s, line %d.\n",
                failedexpr, file, func, line);
  abort();
  /* NOTREACHED */
}
