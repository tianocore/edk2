/** @file
  This is a test application that demonstrates how to use the sorting functions.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/SortLib.h>

/**
  Test comparator.

  @param[in] b1   The first INTN
  @param[in] b2   The other INTN

  @retval 0       They are the same.
  @retval -1      b1 is less than b2
  @retval 1       b1 is greater then b2
**/
INTN
EFIAPI
Test (
  CONST VOID  *b1,
  CONST VOID  *b2
  )
{
  if (*(INTN *)b1 == *(INTN *)b2) {
    return (0);
  }

  if (*(INTN *)b1 < *(INTN *)b2) {
    return (-1);
  }

  return (1);
}

/**
  UEFI application entry point which has an interface similar to a
  standard C main function.

  The ShellCEntryLib library instance wrappers the actual UEFI application
  entry point and calls this ShellAppMain function.

  @param  Argc             Argument count
  @param  Argv             The parsed arguments

  @retval  0               The application exited normally.
  @retval  Other           An error occurred.

**/
INTN
EFIAPI
ShellAppMain (
  IN UINTN   Argc,
  IN CHAR16  **Argv
  )
{
  INTN  Array[10];

  Array[0] = 2;
  Array[1] = 3;
  Array[2] = 4;
  Array[3] = 1;
  Array[4] = 5;
  Array[5] = 6;
  Array[6] = 7;
  Array[7] = 8;
  Array[8] = 1;
  Array[9] = 5;

  Print (L"Array = %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\r\n", Array[0], Array[1], Array[2], Array[3], Array[4], Array[5], Array[6], Array[7], Array[8], Array[9]);
  PerformQuickSort (Array, 10, sizeof (INTN), Test);
  Print (L"POST-SORT\r\n");
  Print (L"Array = %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\r\n", Array[0], Array[1], Array[2], Array[3], Array[4], Array[5], Array[6], Array[7], Array[8], Array[9]);
  return 0;
}
