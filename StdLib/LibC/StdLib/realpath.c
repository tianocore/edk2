/** @file
  Implement the realpath function.

  Copyright (c) 2011 - 2014, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include <LibConfig.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <errno.h>

/** The realpath() function shall derive, from the pathname pointed to by
    file_name, an absolute pathname that names the same file, whose resolution
    does not involve '.', '..', or symbolic links.

    The generated pathname shall be stored as a null-terminated string, up to a
    maximum of {PATH_MAX} bytes, in the buffer pointed to by resolved_name.

    If resolved_name is a null pointer, the behavior of realpath() is
    implementation-defined.

  @param[in] file_name            The filename to convert.
  @param[in,out] resolved_name    The resultant name.

  @retval NULL                    An error occured.
  @return resolved_name.
**/
char *
realpath(
  char *file_name,
  char *resolved_name
  )
{
  CHAR16 *Temp;
  if (file_name == NULL || resolved_name == NULL) {
    errno = EINVAL;
    return (NULL);
  }
  Temp = AllocateZeroPool((1+AsciiStrLen(file_name))*sizeof(CHAR16));
  if (Temp == NULL) {
    errno = ENOMEM;
    return (NULL);
  }
  AsciiStrToUnicodeStr(file_name, Temp);
  PathCleanUpDirectories(Temp);
  UnicodeStrToAsciiStr(Temp, resolved_name);
  return (resolved_name);
}
