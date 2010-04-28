/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  VcCheck.c

Abstract:

  We have found problems with the Visual C++ SP4 and the /O1 flag.
  If this tests ask a question you have the wrong version of Visual C++
  on your system

  This test assumes the tools are being compiled with the same complier
  as the Tiano code. 

  Please see $(EFI_SOURCE)\EFI2.0 Developer's Manual.doc to get the
  correct version of Visual C++

--*/

#include <stdio.h>

_int16  gGloba16;

int
CheckLostCode (
  int Value
  )
/*++

Routine Description:
  This routine is used to test for compiler isseus with /O1.
  If the /O1 compiler option, and C2.dll is got from Visual C++ SP5
  (version: 6.00.8168.0), the assember codes after default branch will be
  losted. (Execute "cl Visual Ccheck.c /O1 /FAsc" to get detail information)

Arguments:
  Value - Test case

Returns: 
  Test to see if comiler error is present.

--*/
{
  switch (Value) {
  case 0:
    break;

  default:
    _asm
    {
      mov bx, 1
      mov gGloba16, bx
    }

    return 1;
  }

  _asm
  {
    mov bx, 0
    mov gGloba16, bx
  }

  return 0;
}

int
main (
  void
  )
/*++

Routine Description:
  This utility is checking for a known Visual C++ compiler issues. To remove this 
  question from the build follow the steps in the developers manual.

Arguments:
  NONE

Returns: 
  0 - Compiler version is O.K.
  1 - Compiler version is Bad

--*/
{
  int   result;
  char  select;

  gGloba16  = 0xFF;
  result    = 0;

  CheckLostCode (0);
  result += (gGloba16 == 0) ? 0 : 1;

  CheckLostCode (1);
  result += (gGloba16 == 1) ? 0 : 1;

  if (result != 0) {
    printf ("Warning: C2.dll is incorrect.\n Please see $(EFI_SOURCE)\\EFI2.0 Developer's Manual.doc for corrective action.\n");
    printf ("Would you want to continue?(Y/N)");

    scanf ("%c", &select);
    if ((select == 'Y') || (select == 'y')) {
      return 0;
    } else {
      return 1;
    }
  }

  return 0;
}
