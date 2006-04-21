/*++
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
        Print.c

Abstract:

--*/


#include <Bc.h>

UINTN
EFIAPI
AsciiPrint (
  IN CONST CHAR8 *Format,
  ...
  )
/*++

Routine Description:

  Print function for a maximum of PXE_MAX_PRINT_BUFFER ascii 
  characters.

Arguments:

  Format - Ascii format string see file header for more details.

  ...    - Vararg list consumed by processing Format.

Returns: 

  Number of characters printed.

--*/
{
  UINTN   Return;
  VA_LIST Marker;
  UINTN   Index;
  UINTN   MaxIndex;
  CHAR16  Buffer[PXE_MAX_PRINT_BUFFER];
  CHAR16  UnicodeFormat[PXE_MAX_PRINT_BUFFER];

  MaxIndex = AsciiStrLen ((CHAR8 *) Format);
  if (MaxIndex > PXE_MAX_PRINT_BUFFER) {
    //
    // Format string was too long for use to process.
    //
    return 0;
  }

  for (Index = 0; Index < PXE_MAX_PRINT_BUFFER; Index++) {
    UnicodeFormat[Index] = (CHAR16) Format[Index];
  }

  VA_START (Marker, Format);
  Return = UnicodeVSPrint (Buffer, sizeof (Buffer), UnicodeFormat, Marker);
  VA_END (Marker);

  //
  // Need to convert to Unicode to do an OutputString
  //

  if (gST->ConOut != NULL) {
    //
    // To be extra safe make sure ConOut has been initialized
    //
    gST->ConOut->OutputString (gST->ConOut, Buffer);
  }

  return Return;
}
