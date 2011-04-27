/**
  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
//#include  <Uefi.h>
//#include  <Library/UefiLib.h>

#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>

#include  <stdio.h>
#include  <stdlib.h>

void
EFIAPI
__assert(const char *func, const char *file, int line, const char *failedexpr)
{
  if (func == NULL)
    printf("Assertion failed: (%s), file %s, line %d.\n",
                failedexpr, file, line);
  else
    printf("Assertion failed: (%s), function %s, file %s, line %d.\n",
                failedexpr, func, file, line);
  abort();
  /* NOTREACHED */
}
