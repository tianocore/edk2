/** @file
  This is a test application that demonstrates how to use the sorting functions.

  Copyright (c) 2009-2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/SortLib.h>

INTN Test(CONST VOID*b1, CONST VOID*b2)
{
  if (*(INTN*)b1 == *(INTN*)b2) {
    return (0);
  }
  if (*(INTN*)b1 < *(INTN*)b2) {
    return(-1);
  }
  return (1);
}

/**
  UEFI application entry point which has an interface similar to a
  standard C main function.

  The ShellCEntryLib library instance wrappers the actual UEFI application
  entry point and calls this ShellAppMain function.

  @param  ImageHandle  The image handle of the UEFI Application.
  @param  SystemTable  A pointer to the EFI System Table.

  @retval  0               The application exited normally.
  @retval  Other           An error occurred.

**/
INTN
EFIAPI
ShellAppMain (
  IN UINTN Argc,
  IN CHAR16 **Argv
  )
{
  INTN Array[10] = {2,3,4,1,5,6,7,8,1,5};
  Print(L"Array = %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\r\n", Array[0],Array[1],Array[2],Array[3],Array[4],Array[5],Array[6],Array[7],Array[8],Array[9]);
  PerformQuickSort(Array, 10, sizeof(INTN), Test);
  Print(L"POST-SORT\r\n");
  Print(L"Array = %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\r\n", Array[0],Array[1],Array[2],Array[3],Array[4],Array[5],Array[6],Array[7],Array[8],Array[9]);
  return 0;
}
