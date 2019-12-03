/** @file
  Routines that access 8042 keyboard controller

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ps2Keyboard.h"

struct {
  UINT8   ScanCode;             ///< follows value defined in Scan Code Set1
  UINT16  EfiScanCode;
  CHAR16  UnicodeChar;
  CHAR16  ShiftUnicodeChar;
}
ConvertKeyboardScanCodeToEfiKey[] = {

  {
    0x01,  //   Escape
    SCAN_ESC,
    0x0000,
    0x0000
  },
  {
    0x02,
    SCAN_NULL,
    L'1',
    L'!'
  },
  {
    0x03,
    SCAN_NULL,
    L'2',
    L'@'
  },
  {
    0x04,
    SCAN_NULL,
    L'3',
    L'#'
  },
  {
    0x05,
    SCAN_NULL,
    L'4',
    L'$'
  },
  {
    0x06,
    SCAN_NULL,
    L'5',
    L'%'
  },
  {
    0x07,
    SCAN_NULL,
    L'6',
    L'^'
  },
  {
    0x08,
    SCAN_NULL,
    L'7',
    L'&'
  },
  {
    0x09,
    SCAN_NULL,
    L'8',
    L'*'
  },
  {
    0x0A,
    SCAN_NULL,
    L'9',
    L'('
  },
  {
    0x0B,
    SCAN_NULL,
    L'0',
    L')'
  },
  {
    0x0C,
    SCAN_NULL,
    L'-',
    L'_'
  },
  {
    0x0D,
    SCAN_NULL,
    L'=',
    L'+'
  },
  {
    0x0E, //  BackSpace
    SCAN_NULL,
    0x0008,
    0x0008
  },
  {
    0x0F, //  Tab
    SCAN_NULL,
    0x0009,
    0x0009
  },
  {
    0x10,
    SCAN_NULL,
    L'q',
    L'Q'
  },
  {
    0x11,
    SCAN_NULL,
    L'w',
    L'W'
  },
  {
    0x12,
    SCAN_NULL,
    L'e',
    L'E'
  },
  {
    0x13,
    SCAN_NULL,
    L'r',
    L'R'
  },
  {
    0x14,
    SCAN_NULL,
    L't',
    L'T'
  },
  {
    0x15,
    SCAN_NULL,
    L'y',
    L'Y'
  },
  {
    0x16,
    SCAN_NULL,
    L'u',
    L'U'
  },
  {
    0x17,
    SCAN_NULL,
    L'i',
    L'I'
  },
  {
    0x18,
    SCAN_NULL,
    L'o',
    L'O'
  },
  {
    0x19,
    SCAN_NULL,
    L'p',
    L'P'
  },
  {
    0x1a,
    SCAN_NULL,
    L'[',
    L'{'
  },
  {
    0x1b,
    SCAN_NULL,
    L']',
    L'}'
  },
  {
    0x1c, //   Enter
    SCAN_NULL,
    0x000d,
    0x000d
  },
  {
    0x1d,
    SCAN_NULL,
    0x0000,
    0x0000
  },
  {
    0x1e,
    SCAN_NULL,
    L'a',
    L'A'
  },
  {
    0x1f,
    SCAN_NULL,
    L's',
    L'S'
  },
  {
    0x20,
    SCAN_NULL,
    L'd',
    L'D'
  },
  {
    0x21,
    SCAN_NULL,
    L'f',
    L'F'
  },
  {
    0x22,
    SCAN_NULL,
    L'g',
    L'G'
  },
  {
    0x23,
    SCAN_NULL,
    L'h',
    L'H'
  },
  {
    0x24,
    SCAN_NULL,
    L'j',
    L'J'
  },
  {
    0x25,
    SCAN_NULL,
    L'k',
    L'K'
  },
  {
    0x26,
    SCAN_NULL,
    L'l',
    L'L'
  },
  {
    0x27,
    SCAN_NULL,
    L';',
    L':'
  },
  {
    0x28,
    SCAN_NULL,
    L'\'',
    L'"'
  },
  {
    0x29,
    SCAN_NULL,
    L'`',
    L'~'
  },
  {
    0x2a, //   Left Shift
    SCAN_NULL,
    0x0000,
    0x0000
  },
  {
    0x2b,
    SCAN_NULL,
    L'\\',
    L'|'
  },
  {
    0x2c,
    SCAN_NULL,
    L'z',
    L'Z'
  },
  {
    0x2d,
    SCAN_NULL,
    L'x',
    L'X'
  },
  {
    0x2e,
    SCAN_NULL,
    L'c',
    L'C'
  },
  {
    0x2f,
    SCAN_NULL,
    L'v',
    L'V'
  },
  {
    0x30,
    SCAN_NULL,
    L'b',
    L'B'
  },
  {
    0x31,
    SCAN_NULL,
    L'n',
    L'N'
  },
  {
    0x32,
    SCAN_NULL,
    L'm',
    L'M'
  },
  {
    0x33,
    SCAN_NULL,
    L',',
    L'<'
  },
  {
    0x34,
    SCAN_NULL,
    L'.',
    L'>'
  },
  {
    0x35,
    SCAN_NULL,
    L'/',
    L'?'
  },
  {
    0x36, //Right Shift
    SCAN_NULL,
    0x0000,
    0x0000
  },
  {
    0x37, // Numeric Keypad *
    SCAN_NULL,
    L'*',
    L'*'
  },
  {
    0x38,  //Left Alt/Extended Right Alt
    SCAN_NULL,
    0x0000,
    0x0000
  },
  {
    0x39,
    SCAN_NULL,
    L' ',
    L' '
  },
  {
    0x3A, //CapsLock
    SCAN_NULL,
    0x0000,
    0x0000
  },
  {
    0x3B,
    SCAN_F1,
    0x0000,
    0x0000
  },
  {
    0x3C,
    SCAN_F2,
    0x0000,
    0x0000
  },
  {
    0x3D,
    SCAN_F3,
    0x0000,
    0x0000
  },
  {
    0x3E,
    SCAN_F4,
    0x0000,
    0x0000
  },
  {
    0x3F,
    SCAN_F5,
    0x0000,
    0x0000
  },
  {
    0x40,
    SCAN_F6,
    0x0000,
    0x0000
  },
  {
    0x41,
    SCAN_F7,
    0x0000,
    0x0000
  },
  {
    0x42,
    SCAN_F8,
    0x0000,
    0x0000
  },
  {
    0x43,
    SCAN_F9,
    0x0000,
    0x0000
  },
  {
    0x44,
    SCAN_F10,
    0x0000,
    0x0000
  },
  {
    0x45, // NumLock
    SCAN_NULL,
    0x0000,
    0x0000
  },
  {
    0x46, //  ScrollLock
    SCAN_NULL,
    0x0000,
    0x0000
  },
  {
    0x47,
    SCAN_HOME,
    L'7',
    L'7'
  },
  {
    0x48,
    SCAN_UP,
    L'8',
    L'8'
  },
  {
    0x49,
    SCAN_PAGE_UP,
    L'9',
    L'9'
  },
  {
    0x4a,
    SCAN_NULL,
    L'-',
    L'-'
  },
  {
    0x4b,
    SCAN_LEFT,
    L'4',
    L'4'
  },
  {
    0x4c, //  Numeric Keypad 5
    SCAN_NULL,
    L'5',
    L'5'
  },
  {
    0x4d,
    SCAN_RIGHT,
    L'6',
    L'6'
  },
  {
    0x4e,
    SCAN_NULL,
    L'+',
    L'+'
  },
  {
    0x4f,
    SCAN_END,
    L'1',
    L'1'
  },
  {
    0x50,
    SCAN_DOWN,
    L'2',
    L'2'
  },
  {
    0x51,
    SCAN_PAGE_DOWN,
    L'3',
    L'3'
  },
  {
    0x52,
    SCAN_INSERT,
    L'0',
    L'0'
  },
  {
    0x53,
    SCAN_DELETE,
    L'.',
    L'.'
  },
  {
    0x57,
    SCAN_F11,
    0x0000,
    0x0000
  },
  {
    0x58,
    SCAN_F12,
    0x0000,
    0x0000
  },
  {
    0x5B,  //Left LOGO
    SCAN_NULL,
    0x0000,
    0x0000
  },
  {
    0x5C,  //Right LOGO
    SCAN_NULL,
    0x0000,
    0x0000
  },
  {
    0x5D,  //Menu key
    SCAN_NULL,
    0x0000,
    0x0000
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
  Return the count of scancode in the queue.

  @param Queue     Pointer to instance of SCAN_CODE_QUEUE.

  @return          Count of the scancode.
**/
UINTN
GetScancodeBufCount (
  IN SCAN_CODE_QUEUE       *Queue
  )
{
  if (Queue->Head <= Queue->Tail) {
    return Queue->Tail - Queue->Head;
  } else {
    return Queue->Tail + KEYBOARD_SCAN_CODE_MAX_COUNT - Queue->Head;
  }
}

/**
  Read several bytes from the scancode buffer without removing them.
  This function is called to see if there are enough bytes of scancode
  representing a single key.

  @param Queue     Pointer to instance of SCAN_CODE_QUEUE.
  @param Count     Number of bytes to be read
  @param Buf       Store the results

  @retval EFI_SUCCESS   success to scan the keyboard code
  @retval EFI_NOT_READY invalid parameter
**/
EFI_STATUS
GetScancodeBufHead (
  IN  SCAN_CODE_QUEUE        *Queue,
  IN  UINTN                  Count,
  OUT UINT8                  *Buf
  )
{
  UINTN                      Index;
  UINTN                      Pos;

  //
  // check the valid range of parameter 'Count'
  //
  if (GetScancodeBufCount (Queue) < Count) {
    return EFI_NOT_READY;
  }
  //
  // retrieve the values
  //
  for (Index = 0, Pos = Queue->Head; Index < Count; Index++, Pos = (Pos + 1) % KEYBOARD_SCAN_CODE_MAX_COUNT) {
    Buf[Index] = Queue->Buffer[Pos];
  }

  return EFI_SUCCESS;
}

/**

  Read & remove several bytes from the scancode buffer.
  This function is usually called after GetScancodeBufHead()

  @param Queue     Pointer to instance of SCAN_CODE_QUEUE.
  @param Count     Number of bytes to be read
  @param Buf       Store the results

  @retval EFI_SUCCESS success to scan the keyboard code
  @retval EFI_NOT_READY invalid parameter
**/
EFI_STATUS
PopScancodeBufHead (
  IN  SCAN_CODE_QUEUE       *Queue,
  IN  UINTN                 Count,
  OUT UINT8                 *Buf OPTIONAL
  )
{
  UINTN                     Index;

  //
  // Check the valid range of parameter 'Count'
  //
  if (GetScancodeBufCount (Queue) < Count) {
    return EFI_NOT_READY;
  }
  //
  // Retrieve and remove the values
  //
  for (Index = 0; Index < Count; Index++, Queue->Head = (Queue->Head + 1) % KEYBOARD_SCAN_CODE_MAX_COUNT) {
    if (Buf != NULL) {
      Buf[Index] = Queue->Buffer[Queue->Head];
    }
  }

  return EFI_SUCCESS;
}

/**
  Push one byte to the scancode buffer.

  @param Queue     Pointer to instance of SCAN_CODE_QUEUE.
  @param Scancode  The byte to push.
**/
VOID
PushScancodeBufTail (
  IN  SCAN_CODE_QUEUE       *Queue,
  IN  UINT8                 Scancode
  )
{
  if (GetScancodeBufCount (Queue) == KEYBOARD_SCAN_CODE_MAX_COUNT - 1) {
    PopScancodeBufHead (Queue, 1, NULL);
  }

  Queue->Buffer[Queue->Tail] = Scancode;
  Queue->Tail = (Queue->Tail + 1) % KEYBOARD_SCAN_CODE_MAX_COUNT;
}

/**
  Read data register .

  @param ConsoleIn Pointer to instance of KEYBOARD_CONSOLE_IN_DEV

  @return return the value

**/
UINT8
KeyReadDataRegister (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  )

{
  return IoRead8 (ConsoleIn->DataRegisterAddress);
}

/**
  Write data register.

  @param ConsoleIn Pointer to instance of KEYBOARD_CONSOLE_IN_DEV
  @param Data      value wanted to be written

**/
VOID
KeyWriteDataRegister (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn,
  IN UINT8                   Data
  )
{
  IoWrite8 (ConsoleIn->DataRegisterAddress, Data);
}

/**
  Read status register.

  @param ConsoleIn  Pointer to instance of KEYBOARD_CONSOLE_IN_DEV

  @return value in status register

**/
UINT8
KeyReadStatusRegister (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  )
{
  return IoRead8 (ConsoleIn->StatusRegisterAddress);
}

/**
  Write command register .

  @param ConsoleIn Pointer to instance of KEYBOARD_CONSOLE_IN_DEV
  @param Data      The value wanted to be written

**/
VOID
KeyWriteCommandRegister (
  IN KEYBOARD_CONSOLE_IN_DEV *ConsoleIn,
  IN UINT8                   Data
  )
{
  IoWrite8 (ConsoleIn->CommandRegisterAddress, Data);
}

/**
  Display error message.

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
}

/**
  Timer event handler: read a series of scancodes from 8042
  and put them into memory scancode buffer.
  it read as much scancodes to either fill
  the memory buffer or empty the keyboard buffer.
  It is registered as running under TPL_NOTIFY

  @param Event       The timer event
  @param Context     A KEYBOARD_CONSOLE_IN_DEV pointer

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

  ConsoleIn = (KEYBOARD_CONSOLE_IN_DEV *) Context;

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
  // KB is not connected to system. If KB is not connected to system, driver will find there's something
  // error in the following code and wait for the input buffer empty, this waiting time shoulb be short enough since
  // this is a NOTIFY TPL period function, or the system performance will degrade hardly when KB is not connected.
  // Just skip the 'resend' process simply.
  //

  while ((KeyReadStatusRegister (ConsoleIn) & (KEYBOARD_STATUS_REGISTER_TRANSMIT_TIMEOUT|KEYBOARD_STATUS_REGISTER_HAS_OUTPUT_DATA)) ==
      KEYBOARD_STATUS_REGISTER_HAS_OUTPUT_DATA
     ) {
    //
    // Read one byte of the scan code and store it into the memory buffer
    //
    Data = KeyReadDataRegister (ConsoleIn);
    PushScancodeBufTail (&ConsoleIn->ScancodeQueue, Data);
  }
  KeyGetchar (ConsoleIn);

  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);
}

/**
  Read key value .

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
    if (KeyReadStatusRegister (ConsoleIn) & KEYBOARD_STATUS_REGISTER_HAS_OUTPUT_DATA) {
      RegFilled = 1;
      *Data     = KeyReadDataRegister (ConsoleIn);
      break;
    }

    MicroSecondDelay (30);
  }

  if (RegFilled == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  write key to keyboard

  @param ConsoleIn Pointer to instance of KEYBOARD_CONSOLE_IN_DEV
  @param Data      value wanted to be written

  @retval EFI_TIMEOUT   The input buffer register is full for putting new value util timeout
  @retval EFI_SUCCESS   The new value is sucess put into input buffer register.

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
    if ((KeyReadStatusRegister (ConsoleIn) & 0x02) == 0) {
      RegEmptied = 1;
      break;
    }

    MicroSecondDelay (30);
  }

  if (RegEmptied == 0) {
    return EFI_TIMEOUT;
  }
  //
  // Write it
  //
  KeyWriteDataRegister (ConsoleIn, Data);

  return EFI_SUCCESS;
}

/**
  Issue keyboard command.

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
    if ((KeyReadStatusRegister (ConsoleIn) & 0x02) == 0) {
      RegEmptied = 1;
      break;
    }

    MicroSecondDelay (30);
  }

  if (RegEmptied == 0) {
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
    if ((KeyReadStatusRegister (ConsoleIn) & 0x02) == 0) {
      RegEmptied = 1;
      break;
    }

    MicroSecondDelay (30);
  }

  if (RegEmptied == 0) {
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

      MicroSecondDelay (30);
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
  if (GotIt == 1) {
    return EFI_SUCCESS;
  } else {
    return EFI_TIMEOUT;
  }

}

/**
  Show keyboard status lights according to
  indicators in ConsoleIn.

  @param ConsoleIn Pointer to instance of KEYBOARD_CONSOLE_IN_DEV

  @return status of updating keyboard register

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
  Initialize the key state.

  @param  ConsoleIn     The KEYBOARD_CONSOLE_IN_DEV instance.
  @param  KeyState      A pointer to receive the key state information.
**/
VOID
InitializeKeyState (
  IN  KEYBOARD_CONSOLE_IN_DEV *ConsoleIn,
  OUT EFI_KEY_STATE           *KeyState
  )
{
  KeyState->KeyShiftState  = EFI_SHIFT_STATE_VALID
                           | (ConsoleIn->LeftCtrl   ? EFI_LEFT_CONTROL_PRESSED  : 0)
                           | (ConsoleIn->RightCtrl  ? EFI_RIGHT_CONTROL_PRESSED : 0)
                           | (ConsoleIn->LeftAlt    ? EFI_LEFT_ALT_PRESSED      : 0)
                           | (ConsoleIn->RightAlt   ? EFI_RIGHT_ALT_PRESSED     : 0)
                           | (ConsoleIn->LeftShift  ? EFI_LEFT_SHIFT_PRESSED    : 0)
                           | (ConsoleIn->RightShift ? EFI_RIGHT_SHIFT_PRESSED   : 0)
                           | (ConsoleIn->LeftLogo   ? EFI_LEFT_LOGO_PRESSED     : 0)
                           | (ConsoleIn->RightLogo  ? EFI_RIGHT_LOGO_PRESSED    : 0)
                           | (ConsoleIn->Menu       ? EFI_MENU_KEY_PRESSED      : 0)
                           | (ConsoleIn->SysReq     ? EFI_SYS_REQ_PRESSED       : 0)
                           ;
  KeyState->KeyToggleState = EFI_TOGGLE_STATE_VALID
                           | (ConsoleIn->CapsLock   ? EFI_CAPS_LOCK_ACTIVE :   0)
                           | (ConsoleIn->NumLock    ? EFI_NUM_LOCK_ACTIVE :    0)
                           | (ConsoleIn->ScrollLock ? EFI_SCROLL_LOCK_ACTIVE : 0)
                           | (ConsoleIn->IsSupportPartialKey ? EFI_KEY_STATE_EXPOSED : 0)
                           ;
}

/**
  Get scancode from scancode buffer and translate into EFI-scancode and unicode defined by EFI spec.

  The function is always called in TPL_NOTIFY.

  @param ConsoleIn KEYBOARD_CONSOLE_IN_DEV instance pointer

**/
VOID
KeyGetchar (
  IN OUT KEYBOARD_CONSOLE_IN_DEV *ConsoleIn
  )
{
  EFI_STATUS                     Status;
  UINT16                         ScanCode;
  BOOLEAN                        Extend0;
  BOOLEAN                        Extend1;
  UINTN                          Index;
  EFI_KEY_DATA                   KeyData;
  LIST_ENTRY                     *Link;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY  *CurrentNotify;
  //
  // 3 bytes most
  //
  UINT8                          ScancodeArr[3];
  UINT32                         ScancodeArrPos;

  //
  // Check if there are enough bytes of scancode representing a single key
  // available in the buffer
  //
  while (TRUE) {
    Extend0        = FALSE;
    Extend1        = FALSE;
    ScancodeArrPos = 0;
    Status  = GetScancodeBufHead (&ConsoleIn->ScancodeQueue, ScancodeArrPos + 1, ScancodeArr);
    if (EFI_ERROR (Status)) {
      return ;
    }

    if (ScancodeArr[ScancodeArrPos] == SCANCODE_EXTENDED0) {
      //
      // E0 to look ahead 2 bytes
      //
      Extend0 = TRUE;
      ScancodeArrPos = 1;
      Status         = GetScancodeBufHead (&ConsoleIn->ScancodeQueue, ScancodeArrPos + 1, ScancodeArr);
      if (EFI_ERROR (Status)) {
        return ;
      }
    } else if (ScancodeArr[ScancodeArrPos] == SCANCODE_EXTENDED1) {
      //
      // E1 to look ahead 3 bytes
      //
      Extend1 = TRUE;
      ScancodeArrPos = 2;
      Status         = GetScancodeBufHead (&ConsoleIn->ScancodeQueue, ScancodeArrPos + 1, ScancodeArr);
      if (EFI_ERROR (Status)) {
        return ;
      }
    }
    //
    // if we reach this position, scancodes for a key is in buffer now,pop them
    //
    Status = PopScancodeBufHead (&ConsoleIn->ScancodeQueue, ScancodeArrPos + 1, ScancodeArr);
    ASSERT_EFI_ERROR (Status);

    //
    // store the last available byte, this byte of scancode will be checked
    //
    ScanCode = ScancodeArr[ScancodeArrPos];

    if (!Extend1) {
      //
      // Check for special keys and update the driver state.
      //
      switch (ScanCode) {

      case SCANCODE_CTRL_MAKE:
        if (Extend0) {
          ConsoleIn->RightCtrl = TRUE;
        } else {
          ConsoleIn->LeftCtrl  = TRUE;
        }
        break;
      case SCANCODE_CTRL_BREAK:
        if (Extend0) {
          ConsoleIn->RightCtrl = FALSE;
        } else {
          ConsoleIn->LeftCtrl  = FALSE;
        }
        break;

      case SCANCODE_ALT_MAKE:
          if (Extend0) {
            ConsoleIn->RightAlt = TRUE;
          } else {
            ConsoleIn->LeftAlt  = TRUE;
          }
        break;
      case SCANCODE_ALT_BREAK:
          if (Extend0) {
            ConsoleIn->RightAlt = FALSE;
          } else {
            ConsoleIn->LeftAlt  = FALSE;
          }
        break;

      case SCANCODE_LEFT_SHIFT_MAKE:
        //
        // To avoid recognize PRNT_SCRN key as a L_SHIFT key
        // because PRNT_SCRN key generates E0 followed by L_SHIFT scan code.
        // If it the second byte of the PRNT_ScRN skip it.
        //
        if (!Extend0) {
          ConsoleIn->LeftShift  = TRUE;
          break;
        }
        continue;

      case SCANCODE_LEFT_SHIFT_BREAK:
        if (!Extend0) {
          ConsoleIn->LeftShift = FALSE;
        }
        break;

      case SCANCODE_RIGHT_SHIFT_MAKE:
        ConsoleIn->RightShift = TRUE;
        break;
      case SCANCODE_RIGHT_SHIFT_BREAK:
        ConsoleIn->RightShift = FALSE;
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
        if (Extend0) {
          ConsoleIn->SysReq = TRUE;
        }
        break;
      case SCANCODE_SYS_REQ_BREAK:
        if (Extend0) {
          ConsoleIn->SysReq = FALSE;
        }
        break;

      case SCANCODE_SYS_REQ_MAKE_WITH_ALT:
        ConsoleIn->SysReq = TRUE;
        break;
      case SCANCODE_SYS_REQ_BREAK_WITH_ALT:
        ConsoleIn->SysReq = FALSE;
        break;

      case SCANCODE_CAPS_LOCK_MAKE:
        ConsoleIn->CapsLock = (BOOLEAN)!ConsoleIn->CapsLock;
        UpdateStatusLights (ConsoleIn);
        break;
      case SCANCODE_NUM_LOCK_MAKE:
        ConsoleIn->NumLock = (BOOLEAN)!ConsoleIn->NumLock;
        UpdateStatusLights (ConsoleIn);
        break;
      case SCANCODE_SCROLL_LOCK_MAKE:
        if (!Extend0) {
          ConsoleIn->ScrollLock = (BOOLEAN)!ConsoleIn->ScrollLock;
          UpdateStatusLights (ConsoleIn);
        }
        break;
      }
    }

    //
    // If this is above the valid range, ignore it
    //
    if (ScanCode >= SCANCODE_MAX_MAKE) {
      continue;
    } else {
      break;
    }
  }

  //
  // Handle Ctrl+Alt+Del hotkey
  //
  if ((ConsoleIn->LeftCtrl || ConsoleIn->RightCtrl) &&
      (ConsoleIn->LeftAlt  || ConsoleIn->RightAlt ) &&
      ScanCode == SCANCODE_DELETE_MAKE
     ) {
    gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
  }

  //
  // Save the Shift/Toggle state
  //
  InitializeKeyState (ConsoleIn, &KeyData.KeyState);
  KeyData.Key.ScanCode    = SCAN_NULL;
  KeyData.Key.UnicodeChar = CHAR_NULL;

  //
  // Key Pad "/" shares the same scancode as that of "/" except Key Pad "/" has E0 prefix
  //
  if (Extend0 && ScanCode == 0x35) {
    KeyData.Key.UnicodeChar = L'/';
    KeyData.Key.ScanCode    = SCAN_NULL;

  //
  // PAUSE shares the same scancode as that of NUM except PAUSE has E1 prefix
  //
  } else if (Extend1 && ScanCode == SCANCODE_NUM_LOCK_MAKE) {
    KeyData.Key.UnicodeChar = CHAR_NULL;
    KeyData.Key.ScanCode    = SCAN_PAUSE;

  //
  // PAUSE shares the same scancode as that of SCROLL except PAUSE (CTRL pressed) has E0 prefix
  //
  } else if (Extend0 && ScanCode == SCANCODE_SCROLL_LOCK_MAKE) {
    KeyData.Key.UnicodeChar = CHAR_NULL;
    KeyData.Key.ScanCode    = SCAN_PAUSE;

  //
  // PRNT_SCRN shares the same scancode as that of Key Pad "*" except PRNT_SCRN has E0 prefix
  //
  } else if (Extend0 && ScanCode == SCANCODE_SYS_REQ_MAKE) {
    KeyData.Key.UnicodeChar = CHAR_NULL;
    KeyData.Key.ScanCode    = SCAN_NULL;

  //
  // Except the above special case, all others can be handled by convert table
  //
  } else {
    for (Index = 0; ConvertKeyboardScanCodeToEfiKey[Index].ScanCode != TABLE_END; Index++) {
      if (ScanCode == ConvertKeyboardScanCodeToEfiKey[Index].ScanCode) {
        KeyData.Key.ScanCode    = ConvertKeyboardScanCodeToEfiKey[Index].EfiScanCode;
        KeyData.Key.UnicodeChar = ConvertKeyboardScanCodeToEfiKey[Index].UnicodeChar;

        if ((ConsoleIn->LeftShift || ConsoleIn->RightShift) &&
            (ConvertKeyboardScanCodeToEfiKey[Index].UnicodeChar != ConvertKeyboardScanCodeToEfiKey[Index].ShiftUnicodeChar)) {
          KeyData.Key.UnicodeChar = ConvertKeyboardScanCodeToEfiKey[Index].ShiftUnicodeChar;
          //
          // Need not return associated shift state if a class of printable characters that
          // are normally adjusted by shift modifiers. e.g. Shift Key + 'f' key = 'F'
          //
          KeyData.KeyState.KeyShiftState &= ~(EFI_LEFT_SHIFT_PRESSED | EFI_RIGHT_SHIFT_PRESSED);
        }
        //
        // alphabetic key is affected by CapsLock State
        //
        if (ConsoleIn->CapsLock) {
          if (KeyData.Key.UnicodeChar >= L'a' && KeyData.Key.UnicodeChar <= L'z') {
            KeyData.Key.UnicodeChar = (UINT16) (KeyData.Key.UnicodeChar - L'a' + L'A');
          } else if (KeyData.Key.UnicodeChar >= L'A' && KeyData.Key.UnicodeChar <= L'Z') {
            KeyData.Key.UnicodeChar = (UINT16) (KeyData.Key.UnicodeChar - L'A' + L'a');
          }
        }
        break;
      }
    }
  }

  //
  // distinguish numeric key pad keys' 'up symbol' and 'down symbol'
  //
  if (ScanCode >= 0x47 && ScanCode <= 0x53) {
    if (ConsoleIn->NumLock && !(ConsoleIn->LeftShift || ConsoleIn->RightShift) && !Extend0) {
      KeyData.Key.ScanCode = SCAN_NULL;
    } else if (ScanCode != 0x4a && ScanCode != 0x4e) {
      KeyData.Key.UnicodeChar = CHAR_NULL;
    }
  }

  //
  // If the key can not be converted then just return.
  //
  if (KeyData.Key.ScanCode == SCAN_NULL && KeyData.Key.UnicodeChar == CHAR_NULL) {
    if (!ConsoleIn->IsSupportPartialKey) {
      return ;
    }
  }

  //
  // Signal KeyNotify process event if this key pressed matches any key registered.
  //
  for (Link = GetFirstNode (&ConsoleIn->NotifyList); !IsNull (&ConsoleIn->NotifyList, Link); Link = GetNextNode (&ConsoleIn->NotifyList, Link)) {
    CurrentNotify = CR (
                      Link,
                      KEYBOARD_CONSOLE_IN_EX_NOTIFY,
                      NotifyEntry,
                      KEYBOARD_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                      );
    if (IsKeyRegistered (&CurrentNotify->KeyData, &KeyData)) {
      //
      // The key notification function needs to run at TPL_CALLBACK
      // while current TPL is TPL_NOTIFY. It will be invoked in
      // KeyNotifyProcessHandler() which runs at TPL_CALLBACK.
      //
      PushEfikeyBufTail (&ConsoleIn->EfiKeyQueueForNotify, &KeyData);
      gBS->SignalEvent (ConsoleIn->KeyNotifyProcessEvent);
      break;
    }
  }

  PushEfikeyBufTail (&ConsoleIn->EfiKeyQueue, &KeyData);
}

/**
  Perform 8042 controller and keyboard Initialization.
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
  UINT32                  TryTime;

  Status                 = EFI_SUCCESS;
  mEnableMouseInterface  = TRUE;
  TryTime                = 0;

  //
  // Get Ps2 policy to set this
  //
  gBS->LocateProtocol (
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
  // output buffer full bits within MAX TRY times
  //
  if ((KeyReadStatusRegister (ConsoleIn) & KEYBOARD_STATUS_REGISTER_HAS_OUTPUT_DATA) != 0) {
    while (!EFI_ERROR (Status) && TryTime < KEYBOARD_MAX_TRY) {
      Status = KeyboardRead (ConsoleIn, &CommandByte);
      TryTime ++;
    }
    //
    // Exceed the max try times. The device may be error.
    //
    if (TryTime == KEYBOARD_MAX_TRY) {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }
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
  if ((KeyReadStatusRegister (ConsoleIn) & KEYBOARD_STATUS_REGISTER_SYSTEM_FLAG) != 0) {
    if (!PcdGetBool (PcdFastPS2Detection)) {
      //
      // 8042 controller is already setup (by myself or by mouse driver):
      //   See whether mouse interface is already enabled
      //   which determines whether we should enable it later
      //
      //
      // Read the command byte of 8042 controller
      //
      Status = KeyboardCommand (ConsoleIn, KEYBOARD_8042_COMMAND_READ);
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
      if ((CommandByte & 0x20) != 0) {
        mEnableMouseInterface = FALSE;
      } else {
        mEnableMouseInterface = TRUE;
      }
    } else {
      mEnableMouseInterface = FALSE;
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
    if (!PcdGetBool (PcdFastPS2Detection)) {
      Status = KeyboardCommand (ConsoleIn, KEYBOARD_8042_COMMAND_DISABLE_KEYBOARD_INTERFACE);
      if (EFI_ERROR (Status)) {
        KeyboardError (ConsoleIn, L"\n\r");
        goto Done;
      }

      Status = KeyboardCommand (ConsoleIn, KEYBOARD_8042_COMMAND_DISABLE_MOUSE_INTERFACE);
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
      Status = KeyboardCommand (ConsoleIn, KEYBOARD_8042_COMMAND_CONTROLLER_SELF_TEST);
      if (EFI_ERROR (Status)) {
        KeyboardError (ConsoleIn, L"8042 controller command write error!\n\r");
        goto Done;
      }

      Status = KeyboardWaitForValue (ConsoleIn, 0x55);
      if (EFI_ERROR (Status)) {
        KeyboardError (ConsoleIn, L"8042 controller self test failed!\n\r");
        goto Done;
      }
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
  Status = KeyboardCommand (ConsoleIn, KEYBOARD_8042_COMMAND_WRITE);
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
  ConsoleIn->ScancodeQueue.Head = 0;
  ConsoleIn->ScancodeQueue.Tail = 0;
  ConsoleIn->EfiKeyQueue.Head   = 0;
  ConsoleIn->EfiKeyQueue.Tail   = 0;
  ConsoleIn->EfiKeyQueueForNotify.Head = 0;
  ConsoleIn->EfiKeyQueueForNotify.Tail = 0;

  //
  // Reset the status indicators
  //
  ConsoleIn->CapsLock   = FALSE;
  ConsoleIn->NumLock    = FALSE;
  ConsoleIn->ScrollLock = FALSE;
  ConsoleIn->LeftCtrl   = FALSE;
  ConsoleIn->RightCtrl  = FALSE;
  ConsoleIn->LeftAlt    = FALSE;
  ConsoleIn->RightAlt   = FALSE;
  ConsoleIn->LeftShift  = FALSE;
  ConsoleIn->RightShift = FALSE;
  ConsoleIn->LeftLogo   = FALSE;
  ConsoleIn->RightLogo  = FALSE;
  ConsoleIn->Menu       = FALSE;
  ConsoleIn->SysReq     = FALSE;

  ConsoleIn->IsSupportPartialKey = FALSE;
  //
  // For resetting keyboard is not mandatory before booting OS and sometimes keyboard responses very slow,
  // and to support KB hot plug, we need to let the InitKB succeed no matter whether there is a KB device connected
  // to system. So we only do the real resetting for keyboard when user asks and there is a real KB connected t system,
  // and normally during booting an OS, it's skipped.
  //
  if (ExtendedVerification && CheckKeyboardConnect (ConsoleIn)) {
    //
    // Additional verifications for keyboard interface
    //
    //
    // Keyboard Interface Test
    //
    Status = KeyboardCommand (ConsoleIn, KEYBOARD_8042_COMMAND_KEYBOARD_INTERFACE_SELF_TEST);
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
    Status = KeyboardWrite (ConsoleIn, KEYBOARD_8048_COMMAND_RESET);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"8042 controller data write error!\n\r");
      goto Done;
    }

    Status = KeyboardWaitForValue (ConsoleIn, KEYBOARD_8048_RETURN_8042_ACK);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"Some specific value not aquired from 8042 controller!\n\r");
      goto Done;
    }
    //
    // wait for BAT completion code
    //
    mWaitForValueTimeOut  = KEYBOARD_BAT_TIMEOUT;

    Status                = KeyboardWaitForValue (ConsoleIn, KEYBOARD_8048_RETURN_8042_BAT_SUCCESS);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"Keyboard self test failed!\n\r");
      goto Done;
    }

    mWaitForValueTimeOut = KEYBOARD_WAITFORVALUE_TIMEOUT;

    //
    // Set Keyboard to use Scan Code Set 2
    //
    Status = KeyboardWrite (ConsoleIn, KEYBOARD_8048_COMMAND_SELECT_SCAN_CODE_SET);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"8042 controller data write error!\n\r");
      goto Done;
    }

    Status = KeyboardWaitForValue (ConsoleIn, KEYBOARD_8048_RETURN_8042_ACK);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"Some specific value not aquired from 8042 controller!\n\r");
      goto Done;
    }

    Status = KeyboardWrite (ConsoleIn, 0x02);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"8042 controller data write error!!\n\r");
      goto Done;
    }

    Status = KeyboardWaitForValue (ConsoleIn, KEYBOARD_8048_RETURN_8042_ACK);
    if (EFI_ERROR (Status)) {
      KeyboardError (ConsoleIn, L"Some specific value not aquired from 8042 controller!\n\r");
      goto Done;
    }

  //
  // Clear Keyboard Scancode Buffer
  //
  Status = KeyboardWrite (ConsoleIn, KEYBOARD_8048_COMMAND_CLEAR_OUTPUT_DATA);
  if (EFI_ERROR (Status)) {
    KeyboardError (ConsoleIn, L"8042 controller data write error!\n\r");
    goto Done;
  }

  Status = KeyboardWaitForValue (ConsoleIn, KEYBOARD_8048_RETURN_8042_ACK);
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
    Status1 = KeyboardCommand (ConsoleIn, KEYBOARD_8042_COMMAND_ENABLE_MOUSE_INTERFACE);
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

  //
  // enable keyboard itself and wait for its ack
  // If can't receive ack, Keyboard should not be connected.
  //
  if (!PcdGetBool (PcdFastPS2Detection)) {
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
  } else {
    return TRUE;
  }
}

