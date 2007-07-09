/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    ansi.c
    
Abstract: 
    

Revision History
--*/


#include "Terminal.h"

VOID
AnsiRawDataToUnicode (
  IN  TERMINAL_DEV    *TerminalDevice
  )
{
  UINT8 RawData;

  //
  // pop the raw data out from the raw fifo,
  // and translate it into unicode, then push
  // the unicode into unicode fifo, until the raw fifo is empty.
  //
  while (!IsRawFiFoEmpty (TerminalDevice)) {

    RawFiFoRemoveOneKey (TerminalDevice, &RawData);

    UnicodeFiFoInsertOneKey (TerminalDevice, (UINT16) RawData);
  }
}

EFI_STATUS
AnsiTestString (
  IN  TERMINAL_DEV    *TerminalDevice,
  IN  CHAR16          *WString
  )
{
  CHAR8 GraphicChar;

  //
  // support three kind of character:
  // valid ascii, valid efi control char, valid text graphics.
  //
  for (; *WString != CHAR_NULL; WString++) {

    if ( !(TerminalIsValidAscii (*WString) || 
        TerminalIsValidEfiCntlChar (*WString) ||
        TerminalIsValidTextGraphics (*WString, &GraphicChar, NULL) )) {

      return EFI_UNSUPPORTED;
    }
  }

  return EFI_SUCCESS;
}
