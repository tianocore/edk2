/**@file
  PS/2 Keyboard driver
  Routines that access 8042 keyboard controller

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ps2Keyboard.h"

//
// Function declarations
//
UINT8
KeyReadDataRegister (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  );

VOID
KeyWriteDataRegister (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn,
  IN UINT8                   Data
  );

VOID
KeyWriteCommandRegister (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn,
  IN UINT8                   Data
  );

VOID
KeyboardError (
  IN KEYBOARD_CONSOLE_IN_DEV*ConsoleIn,
  IN CHAR16                 *ErrMsg // should be a unicode string
  );

EFI_STATUS
GetScancodeBufHead (
  KEYBOARD_CONSOLE_IN_DEV  *ConsoleIn,
  IN UINT32                Count,
  OUT UINT8                *Buf
  );

EFI_STATUS
PopScancodeBufHead (
  KEYBOARD_CONSOLE_IN_DEV  *ConsoleIn,
  IN UINT32                Count,
  OUT UINT8                *Buf
  );

EFI_STATUS
KeyboardWrite (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn,
  IN UINT8                   Data
  );

EFI_STATUS
KeyboardCommand (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn,
  IN UINT8                   Data
  );

EFI_STATUS
KeyboardWaitForValue (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn,
  IN UINT8                   Value
  );

struct {
  UINT8  ScanCode;
  UINT16  EfiScanCode;
  CHAR16  UnicodeChar;
  CHAR16  ShiftUnicodeChar;
}
ConvertKeyboardScanCodeToEfiKey[] = {

  {
    0x01,  //   Escape
    SCAN_ESC,
    0x00,
    0x00
  },
  {
    0x02,
    SCAN_NULL,
    '1',
    '!'
  },
  {
    0x03,
    SCAN_NULL,
    '2',
    '@'
  },
  {
    0x04,
    SCAN_NULL,
    '3',
    '#'
  },
  {
    0x05,
    SCAN_NULL,
    '4',
    '$'
  },
  {
    0x06,
    SCAN_NULL,
    '5',
    '%'
  },
  {
    0x07,
    SCAN_NULL,
    '6',
    '^'
  },
  {
    0x08,
    SCAN_NULL,
    '7',
    '&'
  },
  {
    0x09,
    SCAN_NULL,
    '8',
    '*'
  },
  {
    0x0A,
    SCAN_NULL,
    '9',
    '('
  },
  {
    0x0B,
    SCAN_NULL,
    '0',
    ')'
  },
  {
    0x0C,
    SCAN_NULL,
    '-',
    '_'
  },
  {
    0x0D,
    SCAN_NULL,
    '=',
    '+'
  },
  {
    0x0E, //  BackSpace
    SCAN_NULL,
    0x08,
    0x08
  },
  {
    0x0F, //  Tab
    SCAN_NULL,
    0x09,
    0x09
  },
  {
    0x10,
    SCAN_NULL,
    'q',
    'Q'
  },
  {
    0x11,
    SCAN_NULL,
    'w',
    'W'
  },
  {
    0x12,
    SCAN_NULL,
    'e',
    'E'
  },
  {
    0x13,
    SCAN_NULL,
    'r',
    'R'
  },
  {
    0x14,
    SCAN_NULL,
    't',
    'T'
  },
  {
    0x15,
    SCAN_NULL,
    'y',
    'Y'
  },
  {
    0x16,
    SCAN_NULL,
    'u',
    'U'
  },
  {
    0x17,
    SCAN_NULL,
    'i',
    'I'
  },
  {
    0x18,
    SCAN_NULL,
    'o',
    'O'
  },
  {
    0x19,
    SCAN_NULL,
    'p',
    'P'
  },
  {
    0x1a,
    SCAN_NULL,
    '[',
    '{'
  },
  {
    0x1b,
    SCAN_NULL,
    ']',
    '}'
  },
  {
    0x1c, //   Enter
    SCAN_NULL,
    0x0d,
    0x0d
  },
  {
    0x1d,
    SCAN_NULL,
    0x00,
    0x00
  },
  {
    0x1e,
    SCAN_NULL,
    'a',
    'A'
  },
  {
    0x1f,
    SCAN_NULL,
    's',
    'S'
  },
  {
    0x20,
    SCAN_NULL,
    'd',
    'D'
  },
  {
    0x21,
    SCAN_NULL,
    'f',
    'F'
  },
  {
    0x22,
    SCAN_NULL,
    'g',
    'G'
  },
  {
    0x23,
    SCAN_NULL,
    'h',
    'H'
  },
  {
    0x24,
    SCAN_NULL,
    'j',
    'J'
  },
  {
    0x25,
    SCAN_NULL,
    'k',
    'K'
  },
  {
    0x26,
    SCAN_NULL,
    'l',
    'L'
  },
  {
    0x27,
    SCAN_NULL,
    ';',
    ':'
  },
  {
    0x28,
    SCAN_NULL,
    '\'',
    '"'
  },
  {
    0x29,
    SCAN_NULL,
    '`',
    '~'
  },
  {
    0x2a, //   Left Shift
    SCAN_NULL,
    0x00,
    0x00
  },
  {
    0x2b,
    SCAN_NULL,
    '\\',
    '|'
  },
  {
    0x2c,
    SCAN_NULL,
    'z',
    'Z'
  },
  {
    0x2d,
    SCAN_NULL,
    'x',
    'X'
  },
  {
    0x2e,
    SCAN_NULL,
    'c',
    'C'
  },
  {
    0x2f,
    SCAN_NULL,
    'v',
    'V'
  },
  {
    0x30,
    SCAN_NULL,
    'b',
    'B'
  },
  {
    0x31,
    SCAN_NULL,
    'n',
    'N'
  },
  {
    0x32,
    SCAN_NULL,
    'm',
    'M'
  },
  {
    0x33,
    SCAN_NULL,
    ',',
    '<'
  },
  {
    0x34,
    SCAN_NULL,
    '.',
    '>'
  },
  {
    0x35,
    SCAN_NULL,
    '/',
    '?'
  },
  {
    0x36, //Right Shift
    SCAN_NULL,
    0x00,
    0x00
  },
  {
    0x37, // Numeric Keypad *
    SCAN_NULL,
    '*',
    '*'
  },
  {
    0x38,  //Left Alt/Extended Right Alt
    SCAN_NULL,
    0x00,
    0x00
  },
  {
    0x39,
    SCAN_NULL,
    ' ',
    ' '
  },
  {
    0x3A, //CapsLock
    SCAN_NULL,
    0x00,
    0x00
  },
  {
    0x3B,
    SCAN_F1,
    0x00,
    0x00
  },
  {
    0x3C,
    SCAN_F2,
    0x00,
    0x00
  },
  {
    0x3D,
    SCAN_F3,
    0x00,
    0x00
  },
  {
    0x3E,
    SCAN_F4,
    0x00,
    0x00
  },
  {
    0x3F,
    SCAN_F5,
    0x00,
    0x00
  },
  {
    0x40,
    SCAN_F6,
    0x00,
    0x00
  },
  {
    0x41,
    SCAN_F7,
    0x00,
    0x00
  },
  {
    0x42,
    SCAN_F8,
    0x00,
    0x00
  },
  {
    0x43,
    SCAN_F9,
    0x00,
    0x00
  },
  {
    0x44,
    SCAN_F10,
    0x00,
    0x00
  },
  {
    0x45, // NumLock
    SCAN_NULL,
    0x00,
    0x00
  },
  {
    0x46, //  ScrollLock
    SCAN_NULL,
    0x00,
    0x00
  },
  {
    0x47,
    SCAN_HOME,
    '7',
    '7'
  },
  {
    0x48,
    SCAN_UP,
    '8',
    '8'
  },
  {
    0x49,
    SCAN_PAGE_UP,
    '9',
    '9'
  },
  {
    0x4a,
    SCAN_NULL,
    '-',
    '-'
  },
  {
    0x4b,
    SCAN_LEFT,
    '4',
    '4'
  },
  {
    0x4c, //  Numeric Keypad 5
    SCAN_NULL,
    '5',
    '5'
  },
  {
    0x4d,
    SCAN_RIGHT,
    '6',
    '6'
  },
  {
    0x4e,
    SCAN_NULL,
    '+',
    '+'
  },
  {
    0x4f,
    SCAN_END,
    '1',
    '1'
  },
  {
    0x50,
    SCAN_DOWN,
    '2',
    '2'
  },
  {
    0x51,
    SCAN_PAGE_DOWN,
    '3',
    '3'
  },
  {
    0x52,
    SCAN_INSERT,
    '0',
    '0'
  },
  {
    0x53,
    SCAN_DELETE,
    '.',
    '.'
  },
  {
    0x57,
    SCAN_F11,
    0x00,
    0x00
  },
  {
    0x58,
    SCAN_F12,
    0x00,
    0x00
  },
  {
    0x5B,  //Left LOGO
    SCAN_NULL,
    0x00,
    0x00
  },  
  {
    0x5C,  //Right LOGO
    SCAN_NULL,
    0x00,
    0x00
  },  
  {
    0x5D,  //Menu key
    SCAN_NULL,
    0x00,
    0x00
  },    
  {
    TABLE_END,
    TABLE_END,
    SCAN_NULL,
    SCAN_NULL
  },
};


//
// The WaitForValue time out
//
UINTN  mWaitForValueTimeOut = KEYBOARD_WAITFORVALUE_TIMEOUT;

BOOLEAN          mEnableMouseInterface;

/**
  Read data register 

  @param ConsoleIn Pointer to instance of KEYBOARD_CONSOLE_IN_DEV

  @return return the value 

**/
UINT8
KeyReadDataRegister (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  )

{
  EFI_ISA_IO_PROTOCOL                 *IsaIo;
  UINT8                               Data;

  //
  // Use IsaIo protocol to perform IO operations
  //
  IsaIo = ConsoleIn->IsaIo;

  IsaIo->Io.Read (
              IsaIo,
              EfiIsaIoWidthUint8,
              ConsoleIn->DataRegisterAddress,
              1,
              &Data
              );

  return Data;
}

/**
  Write data register

  @param ConsoleIn Pointer to instance of KEYBOARD_CONSOLE_IN_DEV
  @param Data      value wanted to be written

**/
VOID
KeyWriteDataRegister (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn,
  IN UINT8                   Data
  )

{
  EFI_ISA_IO_PROTOCOL                 *IsaIo;

  //
  // Use IsaIo protocol to perform IO operations
  //
  IsaIo = ConsoleIn->IsaIo;

  IsaIo->Io.Write (
              IsaIo,
              EfiIsaIoWidthUint8,
              ConsoleIn->DataRegisterAddress,
              1,
              &Data
              );

  //
  // outp(ConsoleIn->DataRegisterAddress, Data);
  //
}

/**
  Read status register

  @param ConsoleIn  Pointer to instance of KEYBOARD_CONSOLE_IN_DEV

  @return value in status register

**/
UINT8
KeyReadStatusRegister (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  )
{
  EFI_ISA_IO_PROTOCOL                 *IsaIo;
  UINT8                               Data;

  //
  // Use IsaIo protocol to perform IO operations
  //
  IsaIo = ConsoleIn->IsaIo;

  IsaIo->Io.Read (
              IsaIo,
              EfiIsaIoWidthUint8,
              ConsoleIn->StatusRegisterAddress,
              1,
              &Data
              );

  return Data;

}

/**
  Write command register 

  @param ConsoleIn Pointer to instance of KEYBOARD_CONSOLE_IN_DEV
  @param Data      The value wanted to be written

**/

VOID
KeyWriteCommandRegister (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn,
  IN UINT8                   Data
  )
{
  EFI_ISA_IO_PROTOCOL                 *IsaIo;

  //
  // Use IsaIo protocol to perform IO operations
  //
  IsaIo = ConsoleIn->IsaIo;

  IsaIo->Io.Write (
              IsaIo,
              EfiIsaIoWidthUint8,
              ConsoleIn->CommandRegisterAddress,
              1,
              &Data
              );

}

/**
  Display error message

  @param ConsoleIn Pointer to instance of KEYBOARD_CONSOLE_IN_DEV
  @param ErrMsg    Unicode string of error message
  
**/
VOID
KeyboardError (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn,
  IN CHAR16                  *ErrMsg
  )
{
  ConsoleIn->KeyboardErr = TRUE;

  //
  // gST -> ConOut -> OutputString (gST -> ConOut, L"Keyboard Driver: ");
  // gST -> ConOut -> OutputString (gST -> ConOut, ErrMsg);
  //
}

/**
  Timer event handler: read a series of scancodes from 8042
  and put them into memory scancode buffer.
  it read as much scancodes to either fill
  the memory buffer or empty the keyboard buffer.
  It is registered as running under TPL_NOTIFY

  @param Event - The timer event
  @param Context - A KEYBOARD_CONSOLE_IN_DEV pointer

**/
VOID
EFIAPI
KeyboardTimerHandler (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )

{
  UINT8                   Data;
  EFI_TPL                 OldTpl;
  KEYBOARD_CONSOLE_IN_DEV *ConsoleIn;

  ConsoleIn = Context;

  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  if (((KEYBOARD_CONSOLE_IN_DEV *) Context)->KeyboardErr) {
    //
    // Leave critical section and return
    //
    gBS->RestoreTPL (OldTpl);
    return ;
  }
  //
  // To let KB driver support Hot plug, here should skip the 'resend' command  for the case that
  // KB is not connected to system. If KB is not connected to system, driver will find there's  something
  // error in the following code and wait for the input buffer empty, this waiting time shoulb be  short enough since
  // this is a NOTIFY TPL period function, or the system performance will degrade hardly when KB is not connected.
  // Just skip the 'resend' process simply.
  //

  Data = 0;

  //
  // if there is no key present, just return
  //
  if ((KeyReadStatusRegister (Context) & 0x21) != 0x1) {
    //
    // Leave critical section and return
    //
    gBS->RestoreTPL (OldTpl);

    return ;
  }
  //
  // Read one byte of the scan code and store it into the memory buffer
  //
  if (ConsoleIn->ScancodeBufCount < KEYBOARD_BUFFER_MAX_COUNT) {

    Data = KeyReadDataRegister (Context);
    //
    // put the scancode into the memory scancode buffer
    //
    ConsoleIn->ScancodeBufCount++;
    ConsoleIn->ScancodeBufEndPos++;
    if (ConsoleIn->ScancodeBufEndPos >= KEYBOARD_BUFFER_MAX_COUNT) {
      ConsoleIn->ScancodeBufEndPos = 0;
    }

    ConsoleIn->ScancodeBuf[ConsoleIn->ScancodeBufEndPos] = Data;

    //
    // Handle Alt+Ctrl+Del Key combination
    //
    switch (Data) {
    case SCANCODE_CTRL_MAKE:
      ConsoleIn->Ctrled = TRUE;
      break;

    case SCANCODE_CTRL_BREAK:
      ConsoleIn->Ctrled = FALSE;
      break;

    case SCANCODE_ALT_MAKE:
      ConsoleIn->Alted = TRUE;
      break;

    case SCANCODE_ALT_BREAK:
      ConsoleIn->Alted = FALSE;
      break;
    }
    //
    // if Alt+Ctrl+Del, Reboot the System
    //
    if (ConsoleIn->Ctrled && ConsoleIn->Alted && Data == 0x53) {
      gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
    }
  }
  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);

  return ;
}

/**
  Read several bytes from the scancode buffer without removing them.
  This function is called to see if there are enough bytes of scancode
  representing a single key.

  @param Count - Number of bytes to be read
  @param Buf - Store the results

  @retval EFI_SUCCESS success to scan the keyboard code
  @retval EFI_NOT_READY invalid parameter
**/
EFI_STATUS
GetScancodeBufHead (
  KEYBOARD_CONSOLE_IN_DEV    *ConsoleIn,
  IN UINT32                  Count,
  OUT UINT8                  *Buf
  )
{
  UINT32  Index;
  UINT32  Pos;

  Index = 0;
  Pos   = 0;

  //
  // check the valid range of parameter 'Count'
  //
  if (Count <= 0 || ConsoleIn->ScancodeBufCount < Count) {
    return EFI_NOT_READY;
  }
  //
  // retrieve the values
  //
  for (Index = 0; Index < Count; Index++) {

    if (Index == 0) {

      Pos = ConsoleIn->ScancodeBufStartPos;
    } else {

      Pos = Pos + 1;
      if (Pos >= KEYBOARD_BUFFER_MAX_COUNT) {
        Pos = 0;
      }
    }

    Buf[Index] = ConsoleIn->ScancodeBuf[Pos];
  }

  return EFI_SUCCESS;
}

/**

  Read & remove several bytes from the scancode buffer.
  This function is usually called after GetScancodeBufHead()

  @param Count - Number of bytes to be read
  @param Buf - Store the results

  @retval EFI_SUCCESS success to scan the keyboard code
  @retval EFI_NOT_READY invalid parameter
**/
EFI_STATUS
PopScancodeBufHead (
  KEYBOARD_CONSOLE_IN_DEV   *ConsoleIn,
  IN UINT32                 Count,
  OUT UINT8                 *Buf
  )
{
  UINT32  Index;

  Index = 0;

  //
  // Check the valid range of parameter 'Count'
  //
  if (Count <= 0 || ConsoleIn->ScancodeBufCount < Count) {
    return EFI_NOT_READY;
  }
  //
  // Retrieve and remove the values
  //
  for (Index = 0; Index < Count; Index++) {

    if (Index != 0) {

      ConsoleIn->ScancodeBufStartPos++;
      if (ConsoleIn->ScancodeBufStartPos >= KEYBOARD_BUFFER_MAX_COUNT) {
        ConsoleIn->ScancodeBufStartPos = 0;
      }
    }

    Buf[Index] = ConsoleIn->ScancodeBuf[ConsoleIn->ScancodeBufStartPos];
    ConsoleIn->ScancodeBufCount--;
  }

  ConsoleIn->ScancodeBufStartPos++;
  if (ConsoleIn->ScancodeBufStartPos >= KEYBOARD_BUFFER_MAX_COUNT) {
    ConsoleIn->ScancodeBufStartPos = 0;
  }

  return EFI_SUCCESS;
}

/**
  Read key value 

  @param ConsoleIn - Pointer to instance of KEYBOARD_CONSOLE_IN_DEV
  @param Data      - Pointer to outof buffer for keeping key value

  @retval EFI_TIMEOUT Status resigter time out
  @retval EFI_SUCCESS Success to read keyboard

**/
EFI_STATUS
KeyboardRead (
  IN KEYBOARD_CONSOLE_IN_DEV  *ConsoleIn,
  OUT UINT8                   *Data
  )

{
  UINT32  TimeOut;
  UINT32  RegFilled;

  TimeOut   = 0;
  RegFilled = 0;

  //
  // wait till output buffer full then perform the read
  //
  for (TimeOut = 0; TimeOut < KEYBOARD_TIMEOUT; TimeOut += 30) {
    if (KeyReadStatusRegister (ConsoleIn) & 0x01) {
      RegFilled = 1;
      *Data     = KeyReadDataRegister (ConsoleIn);
      break;
    }

    gBS->Stall (30);
  }

  if (!RegFilled) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  write key to keyboard

  @param ConsoleIn Pointer to instance of KEYBOARD_CONSOLE_IN_DEV
  @param Data      value wanted to be written

  @retval EFI_TIMEOUT - GC_TODO: Add description for return value
  @retval EFI_SUCCESS - GC_TODO: Add description for return value

**/
EFI_STATUS
KeyboardWrite (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn,
  IN UINT8                   Data
  )
{
  UINT32  TimeOut;
  UINT32  RegEmptied;

  TimeOut     = 0;
  RegEmptied  = 0;

  //
  // wait for input buffer empty
  //
  for (TimeOut = 0; TimeOut < KEYBOARD_TIMEOUT; TimeOut += 30) {
    if (!(KeyReadStatusRegister (ConsoleIn) & 0x02)) {
      RegEmptied = 1;
      break;
    }

    gBS->Stall (30);
  }

  if (!RegEmptied) {
    return EFI_TIMEOUT;
  }
  //
  // Write it
  //
  KeyWriteDataRegister (ConsoleIn, Data);

  return EFI_SUCCESS;
}

/**
  Issue keyboard command

  @param ConsoleIn Pointer to instance of KEYBOARD_CONSOLE_IN_DEV
  @param Data      The buff holding the command 

  @retval EFI_TIMEOUT Keyboard is not ready to issuing 
  @retval EFI_SUCCESS Success to issue keyboard command

**/
EFI_STATUS
KeyboardCommand (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn,
  IN UINT8                   Data
  )
{
  UINT32  TimeOut;
  UINT32  RegEmptied;

  TimeOut     = 0;
  RegEmptied  = 0;

  //
  // Wait For Input Buffer Empty
  //
  for (TimeOut = 0; TimeOut < KEYBOARD_TIMEOUT; TimeOut += 30) {
    if (!(KeyReadStatusRegister (ConsoleIn) & 0x02)) {
      RegEmptied = 1;
      break;
    }

    gBS->Stall (30);
  }

  if (!RegEmptied) {
    return EFI_TIMEOUT;
  }
  //
  // issue the command
  //
  KeyWriteCommandRegister (ConsoleIn, Data);

  //
  // Wait For Input Buffer Empty again
  //
  RegEmptied = 0;
  for (TimeOut = 0; TimeOut < KEYBOARD_TIMEOUT; TimeOut += 30) {
    if (!(KeyReadStatusRegister (ConsoleIn) & 0x02)) {
      RegEmptied = 1;
      break;
    }

    gBS->Stall (30);
  }

  if (!RegEmptied) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  wait for a specific value to be presented on
  8042 Data register by keyboard and then read it,
  used in keyboard commands ack

  @param ConsoleIn Pointer to instance of KEYBOARD_CONSOLE_IN_DEV
  @param Value     the value wanted to be waited.

  @retval EFI_TIMEOUT Fail to get specific value in given time
  @retval EFI_SUCCESS Success to get specific value in given time.
  
**/
EFI_STATUS
KeyboardWaitForValue (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn,
  IN UINT8                   Value
  )
{
  UINT8   Data;
  UINT32  TimeOut;
  UINT32  SumTimeOut;
  UINT32  GotIt;

  GotIt       = 0;
  TimeOut     = 0;
  SumTimeOut  = 0;

  //
  // Make sure the initial value of 'Data' is different from 'Value'
  //
  Data = 0;
  if (Data == Value) {
    Data = 1;
  }
  //
  // Read from 8042 (multiple times if needed)
  // until the expected value appears
  // use SumTimeOut to control the iteration
  //
  while (1) {
    //
    // Perform a read
    //
    for (TimeOut = 0; TimeOut < KEYBOARD_TIMEOUT; TimeOut += 30) {
      if (KeyReadStatusRegister (ConsoleIn) & 0x01) {
        Data = KeyReadDataRegister (ConsoleIn);
        break;
      }

      gBS->Stall (30);
    }

    SumTimeOut += TimeOut;

    if (Data == Value) {
      GotIt = 1;
      break;
    }

    if (SumTimeOut >= mWaitForValueTimeOut) {
      break;
    }
  }
  //
  // Check results
  //
  if (GotIt) {
    return EFI_SUCCESS;
  } else {
    return EFI_TIMEOUT;
  }

}

/**
  Show keyboard status lights according to
  indicators in ConsoleIn.

  @param ConsoleIn Pointer to instance of KEYBOARD_CONSOLE_IN_DEV
  
  @return status

**/
EFI_STATUS
UpdateStatusLights (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  )
{
  EFI_STATUS  Status;
  UINT8       Command;

  //
  // Send keyboard command
  //
  Status = KeyboardWrite (ConsoleIn, 0xed);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  KeyboardWaitForValue (ConsoleIn, 0xfa);

  //
  // Light configuration
  //
  Command = 0;
  if (ConsoleIn->CapsLock) {
    Command |= 4;
  }

  if (ConsoleIn->NumLock) {
    Command |= 2;
  }

  if (ConsoleIn->ScrollLock) {
    Command |= 1;
  }

  Status = KeyboardWrite (ConsoleIn, Command);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  KeyboardWaitForValue (ConsoleIn, 0xfa);
  return Status;
}

/**
  Get scancode from scancode buffer
  and translate into EFI-scancode and unicode defined by EFI spec
  The function is always called in TPL_NOTIFY

  @param ConsoleIn KEYBOARD_CONSOLE_IN_DEV instance pointer

  @retval EFI_NOT_READY - Input from console not ready yet.
  @retval EFI_SUCCESS   - Function executed successfully.

**/
EFI_STATUS
KeyGetchar (
  IN OUT KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  )
{
  EFI_STATUS  Status;
  UINT8       ScanCode;
  UINT8       Readed;
  BOOLEAN     Extended;
  UINT8       ScancodeArr[4];
  UINTN       Index;
  //
  // 4 bytes most
  //
  UINT32      ScancodeArrPos;
  //
  // point to the current position in ScancodeArr
  //

  Readed          = 0;
  Extended        = FALSE;
  ScancodeArrPos  = 0;

  //
  // Read one byte of the scan code and store it into the memory buffer
  // This block of code is added to insert an action that is equivalent to
  // the timer event handling function, so as to increase the frequency of
  // detecting the availability of keys. Timer event has a max frequency of
  // 18Hz which is insufficient
  //
  //
  // To let KB driver support Hot plug, here should skip the 'resend' command  for the case that
  // KB is not connected to system. If KB is not connected to system, driver will find there's  something
  // error in the following code and wait for the input buffer empty, this waiting time shoulb be  short enough since
  // this is a NOTIFY TPL period function, or the system performance will degrade hardly when KB is not connected.
  // Just skip the 'resend' process simply.
  //


  if (((KeyReadStatusRegister (ConsoleIn) & 0x21) == 0x1) && (ConsoleIn->ScancodeBufCount < KEYBOARD_BUFFER_MAX_COUNT)) {

    Readed = KeyReadDataRegister (ConsoleIn);
    //
    // put the scancode into the memory scancode buffer
    //
    ConsoleIn->ScancodeBufCount++;
    ConsoleIn->ScancodeBufEndPos++;
    if (ConsoleIn->ScancodeBufEndPos >= KEYBOARD_BUFFER_MAX_COUNT) {
      ConsoleIn->ScancodeBufEndPos = 0;
    }

    ConsoleIn->ScancodeBuf[ConsoleIn->ScancodeBufEndPos] = Readed;

    //
    // Handle Alt+Ctrl+Del Key combination
    //
    switch (Readed) {

    case SCANCODE_CTRL_MAKE:
      ConsoleIn->Ctrled = TRUE;
      break;

    case SCANCODE_CTRL_BREAK:
      ConsoleIn->Ctrled = FALSE;
      break;

    case SCANCODE_ALT_MAKE:
      ConsoleIn->Alted = TRUE;
      break;

    case SCANCODE_ALT_BREAK:
      ConsoleIn->Alted = FALSE;
      break;
    }
    //
    // if Alt+Ctrl+Del, Reboot the System
    //
    if (ConsoleIn->Ctrled && ConsoleIn->Alted && Readed == 0x53) {
      gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
    }
  }
  //
  // Check if there are enough bytes of scancode representing a single key
  // available in the buffer
  //
  while (1) {

    Status          = GetScancodeBufHead (ConsoleIn, 1, ScancodeArr);
    ScancodeArrPos  = 0;
    if (EFI_ERROR (Status)) {
      return EFI_NOT_READY;
    }

    if (ScancodeArr[ScancodeArrPos] == SCANCODE_EXTENDED) {
      Extended        = TRUE;
      Status          = GetScancodeBufHead (ConsoleIn, 2, ScancodeArr);
      ScancodeArrPos  = 1;
      if (EFI_ERROR (Status)) {
        return EFI_NOT_READY;
      }
    }
    //
    // Checks for key scancode for PAUSE:E1-1D/45-E1/9D-C5
    // if present, ignore them
    //
    if (ScancodeArr[ScancodeArrPos] == SCANCODE_EXTENDED1) {

      Status          = GetScancodeBufHead (ConsoleIn, 2, ScancodeArr);
      ScancodeArrPos  = 1;

      if (EFI_ERROR (Status)) {
        return EFI_NOT_READY;
      }

      Status          = GetScancodeBufHead (ConsoleIn, 3, ScancodeArr);
      ScancodeArrPos  = 2;

      if (EFI_ERROR (Status)) {
        return EFI_NOT_READY;
      }

      PopScancodeBufHead (ConsoleIn, 3, ScancodeArr);
      return EFI_NOT_READY;
    }
    //
    // if we reach this position, scancodes for a key is in buffer now,pop them
    //
    Status = PopScancodeBufHead (ConsoleIn, ScancodeArrPos + 1, ScancodeArr);
    if (EFI_ERROR (Status)) {
      return EFI_NOT_READY;
    }
    //
    // store the last available byte, this byte of scancode will be checked
    //
    ScanCode = ScancodeArr[ScancodeArrPos];

    //
    // Check for special keys and update the driver state.
    //
    switch (ScanCode) {

    case SCANCODE_CTRL_MAKE:
      ConsoleIn->Ctrl = TRUE;
      break;

    case SCANCODE_CTRL_BREAK:
      ConsoleIn->Ctrl = FALSE;
      break;

    case SCANCODE_ALT_MAKE:
      ConsoleIn->Alt = TRUE;
      break;

    case SCANCODE_ALT_BREAK:
      ConsoleIn->Alt = FALSE;
      break;

    case SCANCODE_LEFT_SHIFT_MAKE:
      if (!Extended) {
        ConsoleIn->Shift     = TRUE;
        ConsoleIn->LeftShift = TRUE;
      }      
      break;
    case SCANCODE_RIGHT_SHIFT_MAKE:
      if (!Extended) {
        ConsoleIn->Shift = TRUE;
        ConsoleIn->RightShift = TRUE;
      }
      break;

    case SCANCODE_LEFT_SHIFT_BREAK:
      if (!Extended) {
        ConsoleIn->Shift     = FALSE;
        ConsoleIn->LeftShift = FALSE;
      } else {
        ConsoleIn->SysReq    = FALSE;
      }      
      break;
    case SCANCODE_RIGHT_SHIFT_BREAK:
      if (!Extended) {
        ConsoleIn->Shift = FALSE;
        ConsoleIn->RightShift = FALSE;
      }
      break;

    case SCANCODE_LEFT_LOGO_MAKE:
      ConsoleIn->LeftLogo = TRUE;
      break;    
    case SCANCODE_LEFT_LOGO_BREAK:
      ConsoleIn->LeftLogo = FALSE;
      break;          
    case SCANCODE_RIGHT_LOGO_MAKE:
      ConsoleIn->RightLogo = TRUE;
      break;
    case SCANCODE_RIGHT_LOGO_BREAK:
      ConsoleIn->RightLogo = FALSE;
      break;      
    case SCANCODE_MENU_MAKE:
      ConsoleIn->Menu = TRUE;
      break;
    case SCANCODE_MENU_BREAK:
      ConsoleIn->Menu = FALSE;
      break;      
    case SCANCODE_SYS_REQ_MAKE:
      if (Extended) {
        ConsoleIn->SysReq = TRUE;
      }
    case SCANCODE_CAPS_LOCK_MAKE:
      ConsoleIn->CapsLock = (BOOLEAN)!ConsoleIn->CapsLock;
      UpdateStatusLights (ConsoleIn);
      break;

    case SCANCODE_NUM_LOCK_MAKE:
      ConsoleIn->NumLock = (BOOLEAN)!ConsoleIn->NumLock;
      UpdateStatusLights (ConsoleIn);
      break;

    case SCANCODE_SCROLL_LOCK_MAKE:
      ConsoleIn->ScrollLock = (BOOLEAN)!ConsoleIn->ScrollLock;
      UpdateStatusLights (ConsoleIn);
      break;
    }
    //
    // If this is a BREAK Key or above the valid range, ignore it
    //
    if (ScanCode >= SCANCODE_MAX_MAKE) {
      continue;
    } else {
      break;
    }
  }
  //
  // Treat Numeric Key Pad "/" specially
  //
  if (Extended && ScanCode == 0x35) {
    ConsoleIn->Key.ScanCode     = SCAN_NULL;
    ConsoleIn->Key.UnicodeChar  = '/';
    return EFI_SUCCESS;
  }
  //
  // Convert Keyboard ScanCode into an EFI Key
  //
  for (Index = 0; ConvertKeyboardScanCodeToEfiKey[Index].ScanCode != TABLE_END; Index += 1) {
    if (ScanCode == ConvertKeyboardScanCodeToEfiKey[Index].ScanCode) {
      ConsoleIn->Key.ScanCode = ConvertKeyboardScanCodeToEfiKey[Index].EfiScanCode;
      if (ConsoleIn->Shift) {
        ConsoleIn->Key.UnicodeChar = ConvertKeyboardScanCodeToEfiKey[Index].ShiftUnicodeChar;
        //
        // Need not return associated shift state if a class of printable characters that
        // are normally adjusted by shift modifiers. e.g. Shift Key + 'f' key = 'F'
        //
        if (ConsoleIn->Key.UnicodeChar >= 'A' && ConsoleIn->Key.UnicodeChar <= 'Z') {
          ConsoleIn->LeftShift  = FALSE;
          ConsoleIn->RightShift = FALSE;
        }
      } else {
        ConsoleIn->Key.UnicodeChar = ConvertKeyboardScanCodeToEfiKey[Index].UnicodeChar;
      }
      //
      // alphabetic key is affected by CapsLock State
      //
      if (ConsoleIn->CapsLock) {
        if (ConsoleIn->Key.UnicodeChar >= 'a' && ConsoleIn->Key.UnicodeChar <= 'z') {
          ConsoleIn->Key.UnicodeChar = ConvertKeyboardScanCodeToEfiKey[Index].ShiftUnicodeChar;
        } else if (ConsoleIn->Key.UnicodeChar >= 'A' && ConsoleIn->Key.UnicodeChar <= 'Z') {
          ConsoleIn->Key.UnicodeChar = ConvertKeyboardScanCodeToEfiKey[Index].UnicodeChar;
        }
      }
      //
      // Translate the CTRL-Alpha characters to their corresponding control value (ctrl-a = 0x0001 through ctrl-Z = 0x001A)
      //
      if (ConsoleIn->Ctrled) {
        if (ConsoleIn->Key.UnicodeChar >= 'a' && ConsoleIn->Key.UnicodeChar <= 'z') {
          ConsoleIn->Key.UnicodeChar = (UINT16) (ConsoleIn->Key.UnicodeChar - 'a' + 1);
        } else if (ConsoleIn->Key.UnicodeChar >= 'A' && ConsoleIn->Key.UnicodeChar <= 'Z') {
          ConsoleIn->Key.UnicodeChar = (UINT16) (ConsoleIn->Key.UnicodeChar - 'A' + 1);
        }
      }

      break;
    }
  }

  //
  // distinguish numeric key pad keys' 'up symbol' and 'down symbol'
  //
  if (ScanCode >= 0x47 && ScanCode <= 0x53) {

    if (ConsoleIn->NumLock && !ConsoleIn->Shift && !Extended) {
      ConsoleIn->Key.ScanCode = SCAN_NULL;
    } else if (ScanCode != 0x4a && ScanCode != 0x4e) {
      ConsoleIn->Key.UnicodeChar = 0x00;
    }
  }
  //
  // If the key can not be converted then just return.
  //
  if (ConsoleIn->Key.ScanCode == SCAN_NULL && ConsoleIn->Key.UnicodeChar == 0x00) {
    return EFI_NOT_READY;
  }

  //
  // Save the Shift/Toggle state
  //
  if (ConsoleIn->Ctrl) {
    ConsoleIn->KeyState.KeyShiftState  |= (Extended == TRUE) ? EFI_RIGHT_CONTROL_PRESSED : EFI_LEFT_CONTROL_PRESSED;
  }                                    
  if (ConsoleIn->Alt) {                
    ConsoleIn->KeyState.KeyShiftState  |= (Extended == TRUE) ? EFI_RIGHT_ALT_PRESSED : EFI_LEFT_ALT_PRESSED;
  }                                    
  if (ConsoleIn->LeftShift) {          
    ConsoleIn->KeyState.KeyShiftState  |= EFI_LEFT_SHIFT_PRESSED;
  }                                    
  if (ConsoleIn->RightShift) {         
    ConsoleIn->KeyState.KeyShiftState  |= EFI_RIGHT_SHIFT_PRESSED;
  }                                    
  if (ConsoleIn->LeftLogo) {           
    ConsoleIn->KeyState.KeyShiftState  |= EFI_LEFT_LOGO_PRESSED;
  }                                    
  if (ConsoleIn->RightLogo) {          
    ConsoleIn->KeyState.KeyShiftState  |= EFI_RIGHT_LOGO_PRESSED;
  }                                    
  if (ConsoleIn->Menu) {               
    ConsoleIn->KeyState.KeyShiftState  |= EFI_MENU_KEY_PRESSED;
  }                                    
  if (ConsoleIn->SysReq) {             
    ConsoleIn->KeyState.KeyShiftState  |= EFI_SYS_REQ_PRESSED;
  }  
  if (ConsoleIn->CapsLock) {
    ConsoleIn->KeyState.KeyToggleState |= EFI_CAPS_LOCK_ACTIVE;
  }
  if (ConsoleIn->NumLock) {
    ConsoleIn->KeyState.KeyToggleState |= EFI_NUM_LOCK_ACTIVE;
  }
  if (ConsoleIn->ScrollLock) {
    ConsoleIn->KeyState.KeyToggleState |= EFI_SCROLL_LOCK_ACTIVE;
  }

  return EFI_SUCCESS;
}

/**
  Perform 8042 controller and keyboard Initialization
  If ExtendedVerification is TRUE, do additional test for
  the keyboard interface

  @param ConsoleIn - KEYBOARD_CONSOLE_IN_DEV instance pointer
  @param ExtendedVerification - indicates a thorough initialization

  @retval EFI_DEVICE_ERROR Fail to init keyboard
  @retval EFI_SUCCESS      Success to init keyboard
**/
EFI_STATUS
InitKeyboard (
  IN OUT KEYBOARD_CONSOLE_IN_DEV *ConsoleIn,
  IN BOOLEAN                     ExtendedVerification
  )
{
  EFI_STATUS              Status;
  EFI_STATUS              Status1;
  UINT8                   CommandByte;
  EFI_PS2_POLICY_PROTOCOL *Ps2Policy;

  Status                = EFI_SUCCESS;
  mEnableMouseInterface  = TRUE;

  //
  // Get Ps2 policy to set this
  //
  Status = gBS->LocateProtocol (
                  &gEfiPs2PolicyProtocolGuid,
                  NULL,
                  (VOID **) &Ps2Policy
                  );

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_KEYBOARD | EFI_P_KEYBOARD_PC_CLEAR_BUFFER,
    ConsoleIn->DevicePath
    );

  //
  // Perform a read to cleanup the Status Register's
  // output buffer full bits
  //
  while (!EFI_ERROR (Status)) {
    Status = KeyboardRead (ConsoleIn, &CommandByte);
  }
  //
  // We should disable mouse interface during the initialization process
  // since mouse device output could block keyboard device output in the
  // 60H port of 8042 controller.
  //
  // So if we are not initializing 8042 controller for the
  // first time, we have to remember the previous mouse interface
  // enabling state
  //
  // Test the system flag in to determine whether this is the first
  // time initialization
  //
  if ((KeyReadStatusRegister (ConsoleIn) & 0x04)) {
    //
    // 8042 controller is already setup (by myself or by mouse driver):
    //   See whether mouse interface is already enabled
    //   which determines whether we should enable it later
    //
    //
    // Read the command byte of 8042 controller
    //
    Status = KeyboardCommand (ConsoleIn, 0x20);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"\n\r");
      goto Done;
    }

    Status = KeyboardRead (ConsoleIn, &CommandByte);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"\n\r");
      goto Done;
    }
    //
    // Test the mouse enabling bit
    //
    if (CommandByte & 0x20) {
      mEnableMouseInterface = FALSE;
    } else {
      mEnableMouseInterface = TRUE;
    }

  } else {
    //
    // 8042 controller is not setup yet:
    //   8042 controller selftest;
    //   Don't enable mouse interface later.
    //
    //
    // Disable keyboard and mouse interfaces
    //
    Status = KeyboardCommand (ConsoleIn, 0xad);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"\n\r");
      goto Done;
    }

    Status = KeyboardCommand (ConsoleIn, 0xa7);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"\n\r");
      goto Done;
    }

    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_PROGRESS_CODE,
      EFI_PERIPHERAL_KEYBOARD | EFI_P_KEYBOARD_PC_SELF_TEST,
      ConsoleIn->DevicePath
      );
    //
    // 8042 Controller Self Test
    //
    Status = KeyboardCommand (ConsoleIn, 0xaa);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"8042 controller command write error!\n\r");
      goto Done;
    }

    Status = KeyboardWaitForValue (ConsoleIn, 0x55);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"8042 controller self test failed!\n\r");
      goto Done;
    }
    //
    // Don't enable mouse interface later
    //
    mEnableMouseInterface = FALSE;

  }

  if (Ps2Policy != NULL) {
    Ps2Policy->Ps2InitHardware (ConsoleIn->Handle);
  }
  //
  // Write 8042 Command Byte, set System Flag
  // While at the same time:
  //  1. disable mouse interface,
  //  2. enable kbd interface,
  //  3. enable PC/XT kbd translation mode
  //  4. enable mouse and kbd interrupts
  //
  //  ( Command Byte bits:
  //  7: Reserved
  //  6: PC/XT translation mode
  //  5: Disable Auxiliary device interface
  //  4: Disable keyboard interface
  //  3: Reserved
  //  2: System Flag
  //  1: Enable Auxiliary device interrupt
  //  0: Enable Keyboard interrupt )
  //
  Status = KeyboardCommand (ConsoleIn, 0x60);
  if (EFI_ERROR (Status)) {
    KeyboardError (ConsoleIn, L"8042 controller command write error!\n\r");
    goto Done;
  }

  Status = KeyboardWrite (ConsoleIn, 0x67);
  if (EFI_ERROR (Status)) {
    KeyboardError (ConsoleIn, L"8042 controller data write error!\n\r");
    goto Done;
  }

  //
  // Clear Memory Scancode Buffer
  //
  ConsoleIn->ScancodeBufStartPos  = 0;
  ConsoleIn->ScancodeBufEndPos    = KEYBOARD_BUFFER_MAX_COUNT - 1;
  ConsoleIn->ScancodeBufCount     = 0;
  ConsoleIn->Ctrled               = FALSE;
  ConsoleIn->Alted                = FALSE;

  //
  // Reset the status indicators
  //
  ConsoleIn->Ctrl       = FALSE;
  ConsoleIn->Alt        = FALSE;
  ConsoleIn->Shift      = FALSE;
  ConsoleIn->CapsLock   = FALSE;
  ConsoleIn->NumLock    = FALSE;
  ConsoleIn->ScrollLock = FALSE;
  ConsoleIn->LeftShift  = FALSE;
  ConsoleIn->RightShift = FALSE;
  ConsoleIn->LeftLogo   = FALSE;
  ConsoleIn->RightLogo  = FALSE;
  ConsoleIn->Menu       = FALSE;
  ConsoleIn->SysReq     = FALSE;  

  //
  // For reseting keyboard is not mandatory before booting OS and sometimes keyboard responses very slow,
  // and to support KB hot plug, we need to let the InitKB succeed no matter whether there is a KB device connected
  // to system. So we only do the real reseting for keyboard when user asks and there is a real KB connected t system,
  // and normally during booting an OS, it's skipped.
  //
  if (ExtendedVerification && CheckKeyboardConnect (ConsoleIn)) {
    //
    // Additional verifications for keyboard interface
    //
    //
    // Keyboard Interface Test
    //
    Status = KeyboardCommand (ConsoleIn, 0xab);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"8042 controller command write error!\n\r");
      goto Done;
    }

    Status = KeyboardWaitForValue (ConsoleIn, 0x00);
    if (EFI_ERROR (Status)) {
      KeyboardError (
        ConsoleIn,
        L"Some specific value not aquired from 8042 controller!\n\r"
        );
      goto Done;
    }
    //
    // Keyboard reset with a BAT(Basic Assurance Test)
    //
    Status = KeyboardWrite (ConsoleIn, 0xff);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"8042 controller data write error!\n\r");
      goto Done;
    }

    Status = KeyboardWaitForValue (ConsoleIn, 0xfa);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"Some specific value not aquired from 8042 controller!\n\r");
      goto Done;
    }
    //
    // wait for BAT completion code
    //
    mWaitForValueTimeOut  = KEYBOARD_BAT_TIMEOUT;

    Status                = KeyboardWaitForValue (ConsoleIn, 0xaa);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"Keyboard self test failed!\n\r");
      goto Done;
    }

    mWaitForValueTimeOut = KEYBOARD_WAITFORVALUE_TIMEOUT;

    //
    // Set Keyboard to use Scan Code Set 2
    //
    Status = KeyboardWrite (ConsoleIn, 0xf0);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"8042 controller data write error!\n\r");
      goto Done;
    }

    Status = KeyboardWaitForValue (ConsoleIn, 0xfa);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"Some specific value not aquired from 8042 controller!\n\r");
      goto Done;
    }

    Status = KeyboardWrite (ConsoleIn, 0x02);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"8042 controller data write error!!\n\r");
      goto Done;
    }

    Status = KeyboardWaitForValue (ConsoleIn, 0xfa);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"Some specific value not aquired from 8042 controller!\n\r");
      goto Done;
    }

  //
  // Clear Keyboard Scancode Buffer
  //
  Status = KeyboardWrite (ConsoleIn, 0xf4);
  if (EFI_ERROR (Status)) {
    KeyboardError (ConsoleIn, L"8042 controller data write error!\n\r");
    goto Done;
  }

  Status = KeyboardWaitForValue (ConsoleIn, 0xfa);
  if (EFI_ERROR (Status)) {
    KeyboardError (ConsoleIn, L"Some specific value not aquired from 8042 controller!\n\r");
    goto Done;
  }
  //
  if (Ps2Policy != NULL) {
    if ((Ps2Policy->KeyboardLight & EFI_KEYBOARD_CAPSLOCK) == EFI_KEYBOARD_CAPSLOCK) {
      ConsoleIn->CapsLock = TRUE;
    }

    if ((Ps2Policy->KeyboardLight & EFI_KEYBOARD_NUMLOCK) == EFI_KEYBOARD_NUMLOCK) {
      ConsoleIn->NumLock = TRUE;
    }

    if ((Ps2Policy->KeyboardLight & EFI_KEYBOARD_SCROLLLOCK) == EFI_KEYBOARD_SCROLLLOCK) {
      ConsoleIn->ScrollLock = TRUE;
    }
  }
  //
  // Update Keyboard Lights
  //
  Status = UpdateStatusLights (ConsoleIn);
  if (EFI_ERROR (Status)) {
    KeyboardError (ConsoleIn, L"Update keyboard status lights error!\n\r");
    goto Done;
    }
  }
  //
  // At last, we can now enable the mouse interface if appropriate
  //
Done:

  if (mEnableMouseInterface) {
    //
    // Enable mouse interface
    //
    Status1 = KeyboardCommand (ConsoleIn, 0xa8);
    if (EFI_ERROR (Status1)) {
      KeyboardError (ConsoleIn, L"8042 controller command write error!\n\r");
      return EFI_DEVICE_ERROR;
    }
  }

  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  } else {
    return EFI_DEVICE_ERROR;
  }

}

/**
  Disable the keyboard interface of the 8042 controller

  @param ConsoleIn   - the device instance

  @return status of issuing disable command

**/
EFI_STATUS
DisableKeyboard (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  )
{
  EFI_STATUS  Status;

  //
  // Disable keyboard interface
  //
  Status = KeyboardCommand (ConsoleIn, 0xad);
  if (EFI_ERROR (Status)) {
    KeyboardError (ConsoleIn, L"\n\r");
    return EFI_DEVICE_ERROR;
  }

  return Status;
}

/**
  Check whether there is Ps/2 Keyboard device in system by 0xF4 Keyboard Command
  If Keyboard receives 0xF4, it will respond with 'ACK'. If it doesn't respond, the device
  should not be in system.

  @param[in]  ConsoleIn             Keyboard Private Data Structure

  @retval     TRUE                  Keyboard in System.
  @retval     FALSE                 Keyboard not in System.
**/
BOOLEAN
EFIAPI
CheckKeyboardConnect (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  )
{
  EFI_STATUS     Status;
  UINTN          WaitForValueTimeOutBcakup;

  Status = EFI_SUCCESS;
  //
  // enable keyboard itself and wait for its ack
  // If can't receive ack, Keyboard should not be connected.
  //
  Status = KeyboardWrite (
             ConsoleIn,
             KEYBOARD_KBEN
             );

  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  //
  // wait for 1s
  //
  WaitForValueTimeOutBcakup = mWaitForValueTimeOut;
  mWaitForValueTimeOut = KEYBOARD_WAITFORVALUE_TIMEOUT;
  Status = KeyboardWaitForValue (
             ConsoleIn,
             KEYBOARD_CMDECHO_ACK
             );
  mWaitForValueTimeOut = WaitForValueTimeOutBcakup;

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}

