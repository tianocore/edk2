/** @file
Basic Ascii AvSPrintf() function named VSPrint(). VSPrint() enables very
simple implemenation of SPrint() and Print() to support debug.

You can not Print more than EFI_DRIVER_LIB_MAX_PRINT_BUFFER characters at a
time. This makes the implementation very simple.

VSPrint, Print, SPrint format specification has the follwoing form

%type

type:
  'S','s' - argument is an Unicode string
  'c' - argument is an ascii character
  '%' - Print a %


Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "FormDisplay.h"


/**
  Set Buffer to Value for Size bytes.

  @param  Buffer                 Memory to set.
  @param  Size                   Number of bytes to set
  @param  Value                  Value of the set operation.

**/
VOID
SetUnicodeMem (
  IN VOID   *Buffer,
  IN UINTN  Size,
  IN CHAR16 Value
  )
{
  CHAR16  *Ptr;

  Ptr = Buffer;
  while ((Size--)  != 0) {
    *(Ptr++) = Value;
  }
}

