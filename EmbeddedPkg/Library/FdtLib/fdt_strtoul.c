#/* @file
#  Copyright (c) 2018, Linaro Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
#*/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

unsigned long
strtoul (
  const char  *nptr,
  char        **endptr,
  int         base
  )
{
  RETURN_STATUS  Status;
  UINTN          ReturnValue;

  ASSERT (base == 10 || base == 16);

  if (base == 10) {
    Status = AsciiStrDecimalToUintnS (nptr, endptr, &ReturnValue);
  } else if (base == 16) {
    Status = AsciiStrHexToUintnS (nptr, endptr, &ReturnValue);
  } else {
    Status = RETURN_INVALID_PARAMETER;
  }

  if (RETURN_ERROR (Status)) {
    return MAX_UINTN;
  }

  return ReturnValue;
}
