/**@file
	Implementation for EFI_SIMPLE_TEXT_IN_PROTOCOL protocol.
	
Copyright (c) 2006 - 2007 Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "Terminal.h"

EFI_STATUS
EFIAPI
TerminalConInReset (
  IN  EFI_SIMPLE_TEXT_IN_PROTOCOL  *This,
  IN  BOOLEAN                      ExtendedVerification
  )
/*++
  Routine Description:
  
    Implements EFI_SIMPLE_TEXT_IN_PROTOCOL.Reset().
    This driver only perform dependent serial device reset regardless of 
    the value of ExtendeVerification
  
  Arguments:
  
    This - Indicates the calling context.
    
    ExtendedVerification - Skip by this driver.
        
  Returns:
  
    EFI_SUCCESS
       The reset operation succeeds.   
    
    EFI_DEVICE_ERROR
      The dependent serial port reset fails.
                
--*/
{
  EFI_STATUS    Status;
  TERMINAL_DEV  *TerminalDevice;

  TerminalDevice = TERMINAL_CON_IN_DEV_FROM_THIS (This);

  //
  // Report progress code here
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_PC_RESET,
    TerminalDevice->DevicePath
    );

  Status = TerminalDevice->SerialIo->Reset (TerminalDevice->SerialIo);

  //
  // clear all the internal buffer for keys
  //
  InitializeRawFiFo (TerminalDevice);
  InitializeUnicodeFiFo (TerminalDevice);
  InitializeEfiKeyFiFo (TerminalDevice);

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_PERIPHERAL_LOCAL_CONSOLE | EFI_P_EC_CONTROLLER_ERROR,
      TerminalDevice->DevicePath
      );
  }

  return Status;
}

EFI_STATUS
EFIAPI
TerminalConInReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_IN_PROTOCOL  *This,
  OUT EFI_INPUT_KEY                *Key
  )
/*++
  Routine Description:
  
    Implements EFI_SIMPLE_TEXT_IN_PROTOCOL.ReadKeyStroke().
      
  Arguments:
  
    This - Indicates the calling context.
    
    Key  - A pointer to a buffer that is filled in with the keystroke
        information for the key that was sent from terminal.        
        
  Returns:
  
    EFI_SUCCESS
      The keystroke information is returned successfully.
       
    EFI_NOT_READY
      There is no keystroke data available.
 
    EFI_DEVICE_ERROR
      The dependent serial device encounters error.
                
--*/
{
  TERMINAL_DEV  *TerminalDevice;
  EFI_STATUS    Status;

  //
  // Initialize *Key to nonsense value.
  //
  Key->ScanCode     = SCAN_NULL;
  Key->UnicodeChar  = 0;
  //
  //  get TERMINAL_DEV from "This" parameter.
  //
  TerminalDevice  = TERMINAL_CON_IN_DEV_FROM_THIS (This);

  Status          = TerminalConInCheckForKey (This);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_READY;
  }

  EfiKeyFiFoRemoveOneKey (TerminalDevice, Key);

  return EFI_SUCCESS;

}

VOID
TranslateRawDataToEfiKey (
  IN  TERMINAL_DEV      *TerminalDevice
  )
/*++
    Step1: Turn raw data into Unicode (according to different encode).
    Step2: Translate Unicode into key information. 
    (according to different terminal standard).
--*/
{
  switch (TerminalDevice->TerminalType) {

  case PcAnsiType:
  case VT100Type:
  case VT100PlusType:
    AnsiRawDataToUnicode (TerminalDevice);
    UnicodeToEfiKey (TerminalDevice);
    break;

  case VTUTF8Type:
    //
    // Process all the raw data in the RawFIFO,
    // put the processed key into UnicodeFIFO.
    //
    VTUTF8RawDataToUnicode (TerminalDevice);

    //
    // Translate all the Unicode data in the UnicodeFIFO to Efi key,
    // then put into EfiKeyFIFO.
    //
    UnicodeToEfiKey (TerminalDevice);

    break;
  }
}

VOID
EFIAPI
TerminalConInWaitForKey (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
/*++
  Routine Description:
  
    Event notification function for EFI_SIMPLE_TEXT_IN_PROTOCOL.WaitForKey event
    Signal the event if there is key available     
  
  Arguments:
  
    Event - Indicates the event that invoke this function.
    
    Context - Indicates the calling context.
        
  Returns:
  
    N/A
                
--*/
{
  //
  // Someone is waiting on the keystroke event, if there's
  // a key pending, signal the event
  //
  // Context is the pointer to EFI_SIMPLE_TEXT_IN_PROTOCOL
  //
  if (!EFI_ERROR (TerminalConInCheckForKey (Context))) {

    gBS->SignalEvent (Event);
  }
}

EFI_STATUS
TerminalConInCheckForKey (
  IN  EFI_SIMPLE_TEXT_IN_PROTOCOL  *This
  )
/*++
  Routine Description:
  
    Check for a pending key in the Efi Key FIFO or Serial device buffer.
  
  Arguments:
  
    This - Indicates the calling context.
        
  Returns:
  
    EFI_SUCCESS
       There is key pending.   
    
    EFI_NOT_READY
      There is no key pending.
      
    EFI_DEVICE_ERROR
                
--*/
{
  EFI_STATUS              Status;
  TERMINAL_DEV            *TerminalDevice;
  UINT32                  Control;
  UINT8                   Input;
  EFI_SERIAL_IO_MODE      *Mode;
  EFI_SERIAL_IO_PROTOCOL  *SerialIo;
  UINTN                   SerialInTimeOut;

  TerminalDevice  = TERMINAL_CON_IN_DEV_FROM_THIS (This);

  SerialIo        = TerminalDevice->SerialIo;
  if (SerialIo == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  //  if current timeout value for serial device is not identical with
  //  the value saved in TERMINAL_DEV structure, then recalculate the
  //  timeout value again and set serial attribute according to this value.
  //
  Mode = SerialIo->Mode;
  if (Mode->Timeout != TerminalDevice->SerialInTimeOut) {

    SerialInTimeOut = 0;
    if (Mode->BaudRate != 0) {
      SerialInTimeOut = (1 + Mode->DataBits + Mode->StopBits) * 2 * 1000000 / (UINTN) Mode->BaudRate;
    }

    Status = SerialIo->SetAttributes (
                        SerialIo,
                        Mode->BaudRate,
                        Mode->ReceiveFifoDepth,
                        (UINT32) SerialInTimeOut,
                        (EFI_PARITY_TYPE) (Mode->Parity),
                        (UINT8) Mode->DataBits,
                        (EFI_STOP_BITS_TYPE) (Mode->StopBits)
                        );

    if (EFI_ERROR (Status)) {
      TerminalDevice->SerialInTimeOut = 0;
    } else {
      TerminalDevice->SerialInTimeOut = SerialInTimeOut;
    }
  }
  //
  //  check whether serial buffer is empty
  //
  Status = SerialIo->GetControl (SerialIo, &Control);

  if (Control & EFI_SERIAL_INPUT_BUFFER_EMPTY) {
    //
    // Translate all the raw data in RawFIFO into EFI Key,
    // according to different terminal type supported.
    //
    TranslateRawDataToEfiKey (TerminalDevice);

    //
    //  if there is pre-fetched Efi Key in EfiKeyFIFO buffer,
    //  return directly.
    //
    if (!IsEfiKeyFiFoEmpty (TerminalDevice)) {
      return EFI_SUCCESS;
    } else {
      return EFI_NOT_READY;
    }
  }
  //
  // Fetch all the keys in the serial buffer,
  // and insert the byte stream into RawFIFO.
  //
  do {

    Status = GetOneKeyFromSerial (TerminalDevice->SerialIo, &Input);

    if (EFI_ERROR (Status)) {
      if (Status == EFI_DEVICE_ERROR) {
        REPORT_STATUS_CODE_WITH_DEVICE_PATH (
          EFI_ERROR_CODE | EFI_ERROR_MINOR,
          EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_INPUT_ERROR,
          TerminalDevice->DevicePath
          );
      }
      break;
    }

    RawFiFoInsertOneKey (TerminalDevice, Input);
  } while (TRUE);

  //
  // Translate all the raw data in RawFIFO into EFI Key,
  // according to different terminal type supported.
  //
  TranslateRawDataToEfiKey (TerminalDevice);

  if (IsEfiKeyFiFoEmpty (TerminalDevice)) {
    return EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GetOneKeyFromSerial (
  EFI_SERIAL_IO_PROTOCOL  *SerialIo,
  UINT8                   *Input
  )
/*++
    Get one key out of serial buffer.
    If serial buffer is empty, return EFI_NOT_READY;
    if reading serial buffer encounter error, returns EFI_DEVICE_ERROR;
    if reading serial buffer successfully, put the fetched key to 
    the parameter "Input", and return EFI_SUCCESS.
--*/
{
  EFI_STATUS  Status;
  UINTN       Size;

  Size    = 1;
  *Input  = 0;

  Status  = SerialIo->Read (SerialIo, &Size, Input);

  if (EFI_ERROR (Status)) {

    if (Status == EFI_TIMEOUT) {
      return EFI_NOT_READY;
    }

    return EFI_DEVICE_ERROR;

  }

  if (*Input == 0) {
    return EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}

BOOLEAN
RawFiFoInsertOneKey (
  TERMINAL_DEV      *TerminalDevice,
  UINT8             Input
  )
/*++
    Insert one byte raw data into the Raw Data FIFO.
    If FIFO is FULL before data insertion,
    return FALSE, and the key is lost.
--*/
{
  UINT8 Tail;

  Tail = TerminalDevice->RawFiFo.Tail;

  if (IsRawFiFoFull (TerminalDevice)) {
    //
    // Raw FIFO is full
    //
    return FALSE;
  }

  TerminalDevice->RawFiFo.Data[Tail]  = Input;

  TerminalDevice->RawFiFo.Tail        = (UINT8) ((Tail + 1) % (RAW_FIFO_MAX_NUMBER + 1));

  return TRUE;
}

BOOLEAN
RawFiFoRemoveOneKey (
  TERMINAL_DEV  *TerminalDevice,
  UINT8         *Output
  )
/*++
    Remove one byte raw data out of the Raw Data FIFO.
    If FIFO buffer is empty before remove operation,
    return FALSE.
--*/
{
  UINT8 Head;

  Head = TerminalDevice->RawFiFo.Head;

  if (IsRawFiFoEmpty (TerminalDevice)) {
    //
    //  FIFO is empty
    //
    *Output = 0;
    return FALSE;
  }

  *Output                       = TerminalDevice->RawFiFo.Data[Head];

  TerminalDevice->RawFiFo.Head  = (UINT8) ((Head + 1) % (RAW_FIFO_MAX_NUMBER + 1));

  return TRUE;
}

BOOLEAN
IsRawFiFoEmpty (
  TERMINAL_DEV  *TerminalDevice
  )
/*++
    Clarify whether FIFO buffer is empty.
--*/
{
  if (TerminalDevice->RawFiFo.Head == TerminalDevice->RawFiFo.Tail) {
    return TRUE;
  } else {
    return FALSE;
  }
}

BOOLEAN
IsRawFiFoFull (
  TERMINAL_DEV  *TerminalDevice
  )
/*++
    Clarify whether FIFO buffer is full.
--*/
{
  UINT8 Tail;
  UINT8 Head;

  Tail  = TerminalDevice->RawFiFo.Tail;
  Head  = TerminalDevice->RawFiFo.Head;

  if (((Tail + 1) % (RAW_FIFO_MAX_NUMBER + 1)) == Head) {

    return TRUE;
  }

  return FALSE;
}

BOOLEAN
EfiKeyFiFoInsertOneKey (
  TERMINAL_DEV      *TerminalDevice,
  EFI_INPUT_KEY     Key
  )
/*++
    Insert one pre-fetched key into the FIFO buffer.
    If FIFO buffer is FULL before key insertion,
    return FALSE, and the key is lost.
--*/
{
  UINT8 Tail;

  Tail = TerminalDevice->EfiKeyFiFo.Tail;

  if (IsEfiKeyFiFoFull (TerminalDevice)) {
    //
    // Efi Key FIFO is full
    //
    return FALSE;
  }

  TerminalDevice->EfiKeyFiFo.Data[Tail] = Key;

  TerminalDevice->EfiKeyFiFo.Tail       = (UINT8) ((Tail + 1) % (FIFO_MAX_NUMBER + 1));

  return TRUE;
}

BOOLEAN
EfiKeyFiFoRemoveOneKey (
  TERMINAL_DEV  *TerminalDevice,
  EFI_INPUT_KEY *Output
  )
/*++
    Remove one pre-fetched key out of the FIFO buffer.
    If FIFO buffer is empty before remove operation,
    return FALSE.
--*/
{
  UINT8 Head;

  Head = TerminalDevice->EfiKeyFiFo.Head;

  if (IsEfiKeyFiFoEmpty (TerminalDevice)) {
    //
    //  FIFO is empty
    //
    Output->ScanCode    = SCAN_NULL;
    Output->UnicodeChar = 0;
    return FALSE;
  }

  *Output                         = TerminalDevice->EfiKeyFiFo.Data[Head];

  TerminalDevice->EfiKeyFiFo.Head = (UINT8) ((Head + 1) % (FIFO_MAX_NUMBER + 1));

  return TRUE;
}

BOOLEAN
IsEfiKeyFiFoEmpty (
  TERMINAL_DEV  *TerminalDevice
  )
/*++
    Clarify whether FIFO buffer is empty.
--*/
{
  if (TerminalDevice->EfiKeyFiFo.Head == TerminalDevice->EfiKeyFiFo.Tail) {
    return TRUE;
  } else {
    return FALSE;
  }
}

BOOLEAN
IsEfiKeyFiFoFull (
  TERMINAL_DEV  *TerminalDevice
  )
/*++
    Clarify whether FIFO buffer is full.
--*/
{
  UINT8 Tail;
  UINT8 Head;

  Tail  = TerminalDevice->EfiKeyFiFo.Tail;
  Head  = TerminalDevice->EfiKeyFiFo.Head;

  if (((Tail + 1) % (FIFO_MAX_NUMBER + 1)) == Head) {

    return TRUE;
  }

  return FALSE;
}

BOOLEAN
UnicodeFiFoInsertOneKey (
  TERMINAL_DEV      *TerminalDevice,
  UINT16            Input
  )
/*++
    Insert one pre-fetched key into the FIFO buffer.
    If FIFO buffer is FULL before key insertion,
    return FALSE, and the key is lost.
--*/
{
  UINT8 Tail;

  Tail = TerminalDevice->UnicodeFiFo.Tail;

  if (IsUnicodeFiFoFull (TerminalDevice)) {
    //
    // Unicode FIFO is full
    //
    return FALSE;
  }

  TerminalDevice->UnicodeFiFo.Data[Tail]  = Input;

  TerminalDevice->UnicodeFiFo.Tail        = (UINT8) ((Tail + 1) % (FIFO_MAX_NUMBER + 1));

  return TRUE;
}

BOOLEAN
UnicodeFiFoRemoveOneKey (
  TERMINAL_DEV  *TerminalDevice,
  UINT16        *Output
  )
/*++
    Remove one pre-fetched key out of the FIFO buffer.
    If FIFO buffer is empty before remove operation,
    return FALSE.
--*/
{
  UINT8 Head;

  Head = TerminalDevice->UnicodeFiFo.Head;

  if (IsUnicodeFiFoEmpty (TerminalDevice)) {
    //
    //  FIFO is empty
    //
    Output = NULL;
    return FALSE;
  }

  *Output = TerminalDevice->UnicodeFiFo.Data[Head];

  TerminalDevice->UnicodeFiFo.Head = (UINT8) ((Head + 1) % (FIFO_MAX_NUMBER + 1));

  return TRUE;
}

BOOLEAN
IsUnicodeFiFoEmpty (
  TERMINAL_DEV  *TerminalDevice
  )
/*++
    Clarify whether FIFO buffer is empty.
--*/
{
  if (TerminalDevice->UnicodeFiFo.Head == TerminalDevice->UnicodeFiFo.Tail) {
    return TRUE;
  } else {
    return FALSE;
  }
}

BOOLEAN
IsUnicodeFiFoFull (
  TERMINAL_DEV  *TerminalDevice
  )
/*++
    Clarify whether FIFO buffer is full.
--*/
{
  UINT8 Tail;
  UINT8 Head;

  Tail  = TerminalDevice->UnicodeFiFo.Tail;
  Head  = TerminalDevice->UnicodeFiFo.Head;

  if (((Tail + 1) % (FIFO_MAX_NUMBER + 1)) == Head) {

    return TRUE;
  }

  return FALSE;
}

UINT8
UnicodeFiFoGetKeyCount (
  TERMINAL_DEV    *TerminalDevice
  )
{
  UINT8 Tail;
  UINT8 Head;

  Tail  = TerminalDevice->UnicodeFiFo.Tail;
  Head  = TerminalDevice->UnicodeFiFo.Head;

  if (Tail >= Head) {
    return (UINT8) (Tail - Head);
  } else {
    return (UINT8) (Tail + FIFO_MAX_NUMBER + 1 - Head);
  }
}

STATIC
VOID
UnicodeToEfiKeyFlushState (
  IN  TERMINAL_DEV    *TerminalDevice
  )
{
  EFI_INPUT_KEY Key;

  if (TerminalDevice->InputState & INPUT_STATE_ESC) {
    Key.ScanCode    = SCAN_ESC;
    Key.UnicodeChar = 0;
    EfiKeyFiFoInsertOneKey (TerminalDevice, Key);
  }

  if (TerminalDevice->InputState & INPUT_STATE_CSI) {
    Key.ScanCode    = SCAN_NULL;
    Key.UnicodeChar = CSI;
    EfiKeyFiFoInsertOneKey (TerminalDevice, Key);
  }

  if (TerminalDevice->InputState & INPUT_STATE_LEFTOPENBRACKET) {
    Key.ScanCode    = SCAN_NULL;
    Key.UnicodeChar = LEFTOPENBRACKET;
    EfiKeyFiFoInsertOneKey (TerminalDevice, Key);
  }

  if (TerminalDevice->InputState & INPUT_STATE_O) {
    Key.ScanCode    = SCAN_NULL;
    Key.UnicodeChar = 'O';
    EfiKeyFiFoInsertOneKey (TerminalDevice, Key);
  }

  if (TerminalDevice->InputState & INPUT_STATE_2) {
    Key.ScanCode    = SCAN_NULL;
    Key.UnicodeChar = '2';
    EfiKeyFiFoInsertOneKey (TerminalDevice, Key);
  }

  gBS->SetTimer (
        TerminalDevice->TwoSecondTimeOut,
        TimerCancel,
        0
        );

  TerminalDevice->InputState = INPUT_STATE_DEFAULT;
}

VOID
UnicodeToEfiKey (
  IN  TERMINAL_DEV    *TerminalDevice
  )
/*++
  Routine Description:
  
    Converts a stream of Unicode characters from a terminal input device into EFI Keys that
    can be read through the Simple Input Protocol.  The table below shows the keyboard
    input mappings that this function supports.  If the ESC sequence listed in one of the 
    columns is presented, then it is translated into the coorespoding EFI Scan Code.  If a
    matching sequence is not found, then the raw key strokes are converted into EFI Keys.
    
    2 seconds are allowed for an ESC sequence to be completed.  If the ESC sequence is not 
    completed in 2 seconds, then the raw key strokes of the partial ESC sequence are 
    converted into EFI Keys.
    
    There is one special input sequence that will force the system to reset.  
    This is ESC R ESC r ESC R.
  
  Arguments:

    TerminaDevice : The terminal device to use to translate raw input into EFI Keys
        
  Returns:

    None

Symbols used in table below
===========================
  ESC = 0x1B  
  CSI = 0x9B  
  DEL = 0x7f  
  ^   = CTRL

+=========+======+===========+==========+==========+
|         | EFI  | EFI 1.10  |          |          |
|         | Scan |           |  VT100+  |          |
|   KEY   | Code |  PC ANSI  |  VTUTF8  |   VT100  |
+=========+======+===========+==========+==========+
| NULL    | 0x00 |           |          |          |
| UP      | 0x01 | ESC [ A   | ESC [ A  | ESC [ A  |
| DOWN    | 0x02 | ESC [ B   | ESC [ B  | ESC [ B  |
| RIGHT   | 0x03 | ESC [ C   | ESC [ C  | ESC [ C  | 
| LEFT    | 0x04 | ESC [ D   | ESC [ D  | ESC [ D  |
| HOME    | 0x05 | ESC [ H   | ESC h    | ESC [ H  |
| END     | 0x06 | ESC [ F   | ESC k    | ESC [ K  |
| INSERT  | 0x07 | ESC [ @   | ESC +    | ESC [ @  |
|         |      | ESC [ L   |          | ESC [ L  |
| DELETE  | 0x08 | ESC [ X   | ESC -    | ESC [ P  |
| PG UP   | 0x09 | ESC [ I   | ESC ?    | ESC [ V  |
|         |      |           |          | ESC [ ?  |
| PG DOWN | 0x0A | ESC [ G   | ESC /    | ESC [ U  |
|         |      |           |          | ESC [ /  |
| F1      | 0x0B | ESC [ M   | ESC 1    | ESC O P  |
| F2      | 0x0C | ESC [ N   | ESC 2    | ESC O Q  |
| F3      | 0x0D | ESC [ O   | ESC 3    | ESC O w  |
| F4      | 0x0E | ESC [ P   | ESC 4    | ESC O x  |
| F5      | 0x0F | ESC [ Q   | ESC 5    | ESC O t  |
| F6      | 0x10 | ESC [ R   | ESC 6    | ESC O u  |
| F7      | 0x11 | ESC [ S   | ESC 7    | ESC O q  |
| F8      | 0x12 | ESC [ T   | ESC 8    | ESC O r  |
| F9      | 0x13 | ESC [ U   | ESC 9    | ESC O p  |
| F10     | 0x14 | ESC [ V   | ESC 0    | ESC O M  |
| Escape  | 0x17 | ESC       | ESC      | ESC      |
+=========+======+===========+==========+=========+

Special Mappings
================
ESC R ESC r ESC R = Reset System

--*/
{
  EFI_STATUS          Status;
  EFI_STATUS          TimerStatus;
  UINT16              UnicodeChar;
  EFI_INPUT_KEY       Key;
  BOOLEAN             SetDefaultResetState;
  
  TimerStatus = gBS->CheckEvent (TerminalDevice->TwoSecondTimeOut);

  if (!EFI_ERROR (TimerStatus)) {
    UnicodeToEfiKeyFlushState (TerminalDevice);
    TerminalDevice->ResetState = RESET_STATE_DEFAULT;
  }

  while (!IsUnicodeFiFoEmpty(TerminalDevice)) {
    
    if (TerminalDevice->InputState != INPUT_STATE_DEFAULT) {
      //
      // Check to see if the 2 second timer has expired
      //
      TimerStatus = gBS->CheckEvent (TerminalDevice->TwoSecondTimeOut);
      if (!EFI_ERROR (TimerStatus)) {
        UnicodeToEfiKeyFlushState (TerminalDevice);
        TerminalDevice->ResetState = RESET_STATE_DEFAULT;
      }
    }

    //
    // Fetch one Unicode character from the Unicode FIFO
    //
    UnicodeFiFoRemoveOneKey (TerminalDevice,&UnicodeChar);

    SetDefaultResetState = TRUE;

    switch (TerminalDevice->InputState) {
    case INPUT_STATE_DEFAULT:

      break;

    case INPUT_STATE_ESC:

      if (UnicodeChar == LEFTOPENBRACKET) {
        TerminalDevice->InputState |= INPUT_STATE_LEFTOPENBRACKET;
        TerminalDevice->ResetState = RESET_STATE_DEFAULT;
        continue;
      }

      if (UnicodeChar == 'O' && TerminalDevice->TerminalType == VT100Type) {
        TerminalDevice->InputState |= INPUT_STATE_O;
        TerminalDevice->ResetState = RESET_STATE_DEFAULT;
        continue;
      }

      Key.ScanCode = SCAN_NULL;
      
      if (TerminalDevice->TerminalType == VT100PlusType || 
          TerminalDevice->TerminalType == VTUTF8Type) {
        switch (UnicodeChar) {
        case '1': 
          Key.ScanCode = SCAN_F1;         
          break;
        case '2': 
          Key.ScanCode = SCAN_F2;         
          break;
        case '3': 
          Key.ScanCode = SCAN_F3;         
          break;
        case '4': 
          Key.ScanCode = SCAN_F4;         
          break;
        case '5': 
          Key.ScanCode = SCAN_F5;         
          break;
        case '6': 
          Key.ScanCode = SCAN_F6;         
          break;
        case '7': 
          Key.ScanCode = SCAN_F7;         
          break;
        case '8': 
          Key.ScanCode = SCAN_F8;         
          break;
        case '9': 
          Key.ScanCode = SCAN_F9;         
          break;
        case '0': 
          Key.ScanCode = SCAN_F10;        
          break;
        case 'h': 
          Key.ScanCode = SCAN_HOME;       
          break;
        case 'k': 
          Key.ScanCode = SCAN_END;        
          break;
        case '+': 
          Key.ScanCode = SCAN_INSERT;     
          break;
        case '-': 
          Key.ScanCode = SCAN_DELETE;     
          break;
        case '/': 
          Key.ScanCode = SCAN_PAGE_DOWN;  
          break;
        case '?': 
          Key.ScanCode = SCAN_PAGE_UP;    
          break;        
        default :                                 
          break;
        }
      }
      
      switch (UnicodeChar) {
      case 'R': 
        if (TerminalDevice->ResetState == RESET_STATE_DEFAULT) {
          TerminalDevice->ResetState = RESET_STATE_ESC_R;
          SetDefaultResetState = FALSE;
        } else if (TerminalDevice->ResetState == RESET_STATE_ESC_R_ESC_r) {
          gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
        }
        Key.ScanCode = SCAN_NULL;
        break;
      case 'r': 
        if (TerminalDevice->ResetState == RESET_STATE_ESC_R) {
          TerminalDevice->ResetState = RESET_STATE_ESC_R_ESC_r;
          SetDefaultResetState = FALSE;
        }
        Key.ScanCode = SCAN_NULL;
        break;
      default : 
        break;
      }

      if (SetDefaultResetState) {
        TerminalDevice->ResetState = RESET_STATE_DEFAULT;
      }

      if (Key.ScanCode != SCAN_NULL) {
        Key.UnicodeChar = 0;
        EfiKeyFiFoInsertOneKey (TerminalDevice,Key);
        TerminalDevice->InputState = INPUT_STATE_DEFAULT;
        UnicodeToEfiKeyFlushState (TerminalDevice);
        continue;
      }

      UnicodeToEfiKeyFlushState (TerminalDevice);

      break;

    case INPUT_STATE_ESC | INPUT_STATE_O:

      TerminalDevice->ResetState = RESET_STATE_DEFAULT;

      Key.ScanCode = SCAN_NULL;
      
      if (TerminalDevice->TerminalType == VT100Type) {
        switch (UnicodeChar) {
        case 'P': 
          Key.ScanCode = SCAN_F1;         
          break;
        case 'Q': 
          Key.ScanCode = SCAN_F2;         
          break;
        case 'w': 
          Key.ScanCode = SCAN_F3;         
          break;
        case 'x': 
          Key.ScanCode = SCAN_F4;         
          break;
        case 't': 
          Key.ScanCode = SCAN_F5;         
          break;
        case 'u': 
          Key.ScanCode = SCAN_F6;         
          break;
        case 'q': 
          Key.ScanCode = SCAN_F7;         
          break;
        case 'r': 
          Key.ScanCode = SCAN_F8;         
          break;
        case 'p': 
          Key.ScanCode = SCAN_F9;         
          break;
        case 'M': 
          Key.ScanCode = SCAN_F10;        
          break;
        default :                                 
          break;
        }
      }

      if (Key.ScanCode != SCAN_NULL) {
        Key.UnicodeChar = 0;
        EfiKeyFiFoInsertOneKey (TerminalDevice,Key);
        TerminalDevice->InputState = INPUT_STATE_DEFAULT;
        UnicodeToEfiKeyFlushState (TerminalDevice);
        continue;
      }

      UnicodeToEfiKeyFlushState (TerminalDevice);

      break;

    case INPUT_STATE_ESC | INPUT_STATE_LEFTOPENBRACKET:
    
      TerminalDevice->ResetState = RESET_STATE_DEFAULT;
      
      Key.ScanCode = SCAN_NULL;
      
      if (TerminalDevice->TerminalType == PcAnsiType    ||
          TerminalDevice->TerminalType == VT100Type     ||
          TerminalDevice->TerminalType == VT100PlusType || 
          TerminalDevice->TerminalType == VTUTF8Type) {
        switch (UnicodeChar) {
        case 'A': 
          Key.ScanCode = SCAN_UP;         
          break;
        case 'B': 
          Key.ScanCode = SCAN_DOWN;       
          break;
        case 'C': 
          Key.ScanCode = SCAN_RIGHT;      
          break;
        case 'D': 
          Key.ScanCode = SCAN_LEFT;       
          break;
        case 'H': 
          if (TerminalDevice->TerminalType == PcAnsiType ||
              TerminalDevice->TerminalType == VT100Type) {
            Key.ScanCode = SCAN_HOME;       
          }
          break;
        case 'F': 
          if (TerminalDevice->TerminalType == PcAnsiType) {
            Key.ScanCode = SCAN_END;
          }
          break;
        case 'K': 
          if (TerminalDevice->TerminalType == VT100Type) {
            Key.ScanCode = SCAN_END;        
          }
          break;
        case 'L':   
        case '@': 
          if (TerminalDevice->TerminalType == PcAnsiType ||
              TerminalDevice->TerminalType == VT100Type) {
            Key.ScanCode = SCAN_INSERT;     
          }
          break;
        case 'X': 
          if (TerminalDevice->TerminalType == PcAnsiType) {
            Key.ScanCode = SCAN_DELETE;
          }
          break;
        case 'P': 
          if (TerminalDevice->TerminalType == VT100Type) {
            Key.ScanCode = SCAN_DELETE;        
          } else if (TerminalDevice->TerminalType == PcAnsiType) {
            Key.ScanCode = SCAN_F4;
          }
          break;
        case 'I': 
          if (TerminalDevice->TerminalType == PcAnsiType) {
            Key.ScanCode = SCAN_PAGE_UP;
          }
          break;        
        case 'V': 
          if (TerminalDevice->TerminalType == PcAnsiType) {
            Key.ScanCode = SCAN_F10;
          }  
        case '?': 
          if (TerminalDevice->TerminalType == VT100Type) {
            Key.ScanCode = SCAN_PAGE_UP;        
          }
          break;
        case 'G': 
          if (TerminalDevice->TerminalType == PcAnsiType) {
            Key.ScanCode = SCAN_PAGE_DOWN;
          }
          break;        
        case 'U': 
          if (TerminalDevice->TerminalType == PcAnsiType) {
            Key.ScanCode = SCAN_F9;
          }
        case '/': 
          if (TerminalDevice->TerminalType == VT100Type) {
            Key.ScanCode = SCAN_PAGE_DOWN;        
          }
          break;
        case 'M': 
          if (TerminalDevice->TerminalType == PcAnsiType) {
            Key.ScanCode = SCAN_F1;
          }
          break;        
        case 'N': 
          if (TerminalDevice->TerminalType == PcAnsiType) {
            Key.ScanCode = SCAN_F2;
          }
          break;        
        case 'O': 
          if (TerminalDevice->TerminalType == PcAnsiType) {
            Key.ScanCode = SCAN_F3;
          }
          break;        
        case 'Q': 
          if (TerminalDevice->TerminalType == PcAnsiType) {
            Key.ScanCode = SCAN_F5;
          }
          break;        
        case 'R': 
          if (TerminalDevice->TerminalType == PcAnsiType) {
            Key.ScanCode = SCAN_F6;
          }
          break;        
        case 'S': 
          if (TerminalDevice->TerminalType == PcAnsiType) {
            Key.ScanCode = SCAN_F7;
          }
          break;        
        case 'T': 
          if (TerminalDevice->TerminalType == PcAnsiType) {
            Key.ScanCode = SCAN_F8;
          }
          break;        
        default : 
          break;
        }
      }

      if (Key.ScanCode != SCAN_NULL) {
        Key.UnicodeChar = 0;
        EfiKeyFiFoInsertOneKey (TerminalDevice,Key);
        TerminalDevice->InputState = INPUT_STATE_DEFAULT;
        UnicodeToEfiKeyFlushState (TerminalDevice);
        continue;
      }

      UnicodeToEfiKeyFlushState (TerminalDevice);

      break;

    
    default:
      //
      // Invalid state. This should never happen.
      //
      ASSERT (FALSE);

      UnicodeToEfiKeyFlushState (TerminalDevice);

      break;
    }

    if (UnicodeChar == ESC) {
      TerminalDevice->InputState = INPUT_STATE_ESC;
    }
    
    if (TerminalDevice->InputState != INPUT_STATE_DEFAULT) {
      Status = gBS->SetTimer(
                      TerminalDevice->TwoSecondTimeOut,
                      TimerRelative,
                      (UINT64)20000000
                      );
      ASSERT_EFI_ERROR (Status);
      continue;
    }

    if (SetDefaultResetState) {
      TerminalDevice->ResetState = RESET_STATE_DEFAULT;
    }

    if (UnicodeChar == DEL) {
      Key.ScanCode    = SCAN_DELETE;
      Key.UnicodeChar = 0;
    } else {
      Key.ScanCode    = SCAN_NULL;
      Key.UnicodeChar = UnicodeChar;
    }

    EfiKeyFiFoInsertOneKey (TerminalDevice,Key);
  }
}
