/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BoxDraw.c

Abstract:
  Lib functions to support Box Draw Unicode code pages.



Revision History

--*/

#include "Tiano.h"
#include "EfiDriverLib.h"



typedef struct {
  CHAR16  Unicode;
  CHAR8   PcAnsi;
  CHAR8   Ascii;
} UNICODE_TO_CHAR;


//
// This list is used to define the valid extend chars.
// It also provides a mapping from Unicode to PCANSI or
// ASCII. The ASCII mapping we just made up.
//
//

static UNICODE_TO_CHAR UnicodeToPcAnsiOrAscii[] = {
  {BOXDRAW_HORIZONTAL,                 0xc4, L'-'}, 
  {BOXDRAW_VERTICAL,                   0xb3, L'|'},
  {BOXDRAW_DOWN_RIGHT,                 0xda, L'/'},
  {BOXDRAW_DOWN_LEFT,                  0xbf, L'\\'},
  {BOXDRAW_UP_RIGHT,                   0xc0, L'\\'},
  {BOXDRAW_UP_LEFT,                    0xd9, L'/'},
  {BOXDRAW_VERTICAL_RIGHT,             0xc3, L'|'},
  {BOXDRAW_VERTICAL_LEFT,              0xb4, L'|'},
  {BOXDRAW_DOWN_HORIZONTAL,            0xc2, L'+'},
  {BOXDRAW_UP_HORIZONTAL,              0xc1, L'+'},
  {BOXDRAW_VERTICAL_HORIZONTAL,        0xc5, L'+'},
  {BOXDRAW_DOUBLE_HORIZONTAL,          0xcd, L'-'},
  {BOXDRAW_DOUBLE_VERTICAL,            0xba, L'|'},
  {BOXDRAW_DOWN_RIGHT_DOUBLE,          0xd5, L'/'},
  {BOXDRAW_DOWN_DOUBLE_RIGHT,          0xd6, L'/'},
  {BOXDRAW_DOUBLE_DOWN_RIGHT,          0xc9, L'/'},
  {BOXDRAW_DOWN_LEFT_DOUBLE,           0xb8, L'\\'},
  {BOXDRAW_DOWN_DOUBLE_LEFT,           0xb7, L'\\'},
  {BOXDRAW_DOUBLE_DOWN_LEFT,           0xbb, L'\\'},
  {BOXDRAW_UP_RIGHT_DOUBLE,            0xd4, L'\\'},
  {BOXDRAW_UP_DOUBLE_RIGHT,            0xd3, L'\\'},
  {BOXDRAW_DOUBLE_UP_RIGHT,            0xc8, L'\\'},
  {BOXDRAW_UP_LEFT_DOUBLE,             0xbe, L'/'},
  {BOXDRAW_UP_DOUBLE_LEFT,             0xbd, L'/'},
  {BOXDRAW_DOUBLE_UP_LEFT,             0xbc, L'/'},
  {BOXDRAW_VERTICAL_RIGHT_DOUBLE,      0xc6, L'|'},
  {BOXDRAW_VERTICAL_DOUBLE_RIGHT,      0xc7, L'|'},
  {BOXDRAW_DOUBLE_VERTICAL_RIGHT,      0xcc, L'|'},
  {BOXDRAW_VERTICAL_LEFT_DOUBLE,       0xb5, L'|'},
  {BOXDRAW_VERTICAL_DOUBLE_LEFT,       0xb6, L'|'},
  {BOXDRAW_DOUBLE_VERTICAL_LEFT,       0xb9, L'|'},
  {BOXDRAW_DOWN_HORIZONTAL_DOUBLE,     0xd1, L'+'},
  {BOXDRAW_DOWN_DOUBLE_HORIZONTAL,     0xd2, L'+'},
  {BOXDRAW_DOUBLE_DOWN_HORIZONTAL,     0xcb, L'+'},
  {BOXDRAW_UP_HORIZONTAL_DOUBLE,       0xcf, L'+'},
  {BOXDRAW_UP_DOUBLE_HORIZONTAL,       0xd0, L'+'},
  {BOXDRAW_DOUBLE_UP_HORIZONTAL,       0xca, L'+'},
  {BOXDRAW_VERTICAL_HORIZONTAL_DOUBLE, 0xd8, L'+'},
  {BOXDRAW_VERTICAL_DOUBLE_HORIZONTAL, 0xd7, L'+'},
  {BOXDRAW_DOUBLE_VERTICAL_HORIZONTAL, 0xce, L'+'},

  {BLOCKELEMENT_FULL_BLOCK,            0xdb, L'*'},
  {BLOCKELEMENT_LIGHT_SHADE,           0xb0, L'+'},

  {GEOMETRICSHAPE_UP_TRIANGLE,         0x1e, L'^'},
  {GEOMETRICSHAPE_RIGHT_TRIANGLE,      0x10, L'>'},
  {GEOMETRICSHAPE_DOWN_TRIANGLE,       0x1f, L'v'},
  {GEOMETRICSHAPE_LEFT_TRIANGLE,       0x11, L'<'},

  {ARROW_LEFT,                         0x3c, L'<'},

  {ARROW_UP,                           0x18, L'^'},

  {ARROW_RIGHT,                        0x3e, L'>'},

  {ARROW_DOWN,                         0x19, L'v'},
  
  {0x0000,                             0x00, L'\0'},
};


BOOLEAN
LibIsValidTextGraphics (
  IN  CHAR16  Graphic,
  OUT CHAR8   *PcAnsi,    OPTIONAL
  OUT CHAR8   *Ascii      OPTIONAL
  )
/*++

Routine Description:

    Detects if a Unicode char is for Box Drawing text graphics.

Arguments:

    Grphic  - Unicode char to test.

    PcAnsi  - Optional pointer to return PCANSI equivalent of Graphic.

    Asci    - Optional pointer to return Ascii equivalent of Graphic.

Returns:

    TRUE if Gpaphic is a supported Unicode Box Drawing character.

--*/
{
  UNICODE_TO_CHAR     *Table;

  if ((((Graphic & 0xff00) != 0x2500) && ((Graphic & 0xff00) != 0x2100))) {
   
    //
    // Unicode drawing code charts are all in the 0x25xx range, 
    //  arrows are 0x21xx
    //
    return FALSE;
  }

  for (Table = UnicodeToPcAnsiOrAscii; Table->Unicode != 0x0000; Table++) {
    if (Graphic == Table->Unicode) {
      if (PcAnsi != NULL) {
        *PcAnsi = Table->PcAnsi; 
      }
      if (Ascii != NULL) {
        *Ascii = Table->Ascii;
      }
      return TRUE;
    }
  }
  return FALSE;
}


BOOLEAN
IsValidAscii (
  IN  CHAR16  Ascii
  )
/*++

Routine Description:

  Is it valid ascii char?  

Arguments:

  Ascii                 - The char to check

Returns: 

  TRUE                  - Is a ascii char
  FALSE                 - Not a ascii char

--*/
{
  if ((Ascii >= 0x20) && (Ascii <= 0x7f)) {
    return TRUE;
  }              
  return FALSE;
}


BOOLEAN
IsValidEfiCntlChar (
  IN  CHAR16  CharC
  )
/*++

Routine Description:

  Is it valid EFI control char?  

Arguments:

  Ascii                 - The char to check

Returns: 

  TRUE                  - Is a valid EFI control char
  FALSE                 - Not a valid EFI control char

--*/
{
  if (CharC == CHAR_NULL || CharC == CHAR_BACKSPACE || CharC == CHAR_LINEFEED || CharC == CHAR_CARRIAGE_RETURN) {
    return TRUE;
  }              
  return FALSE;
}

