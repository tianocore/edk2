#/* @file
#  Copyright (c) 2018, Linaro Limited. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#*/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

unsigned long strtoul(const char *nptr, char **endptr, int base)
{
    RETURN_STATUS   Status;
    UINTN           ReturnValue;

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
