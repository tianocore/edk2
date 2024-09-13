/** @file
  Implementation for EFI_SIMPLE_TEXT_INPUT_PROTOCOL protocol.

(C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (C) 2016 Silicon Graphics, Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Terminal.h"

/**
  Reads the next keystroke from the input device. The WaitForKey Event can
  be used to test for existence of a keystroke via WaitForEvent () call.

  @param  TerminalDevice           Terminal driver private structure
  @param  KeyData                  A pointer to a buffer that is filled in with the
                                   keystroke state data for the key that was
                                   pressed.

  @retval EFI_SUCCESS              The keystroke information was returned.
  @retval EFI_NOT_READY            There was no keystroke data available.
  @retval EFI_INVALID_PARAMETER    KeyData is NULL.

**/
EFI_STATUS
ReadKeyStrokeWorker (
  IN  TERMINAL_DEV  *TerminalDevice,
  OUT EFI_KEY_DATA  *KeyData
  )
{
  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  KeyData->KeyState.KeyShiftState  = 0;
  KeyData->KeyState.KeyToggleState = 0;

  if (!EfiKeyFiFoRemoveOneKey (TerminalDevice, &KeyData->Key)) {
    return EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}

/**
  Implements EFI_SIMPLE_TEXT_INPUT_PROTOCOL.Reset().
  This driver only perform dependent serial device reset regardless of
  the value of ExtendeVerification

  @param  This                     Indicates the calling context.
  @param  ExtendedVerification     Skip by this driver.

  @retval EFI_SUCCESS              The reset operation succeeds.
  @retval EFI_DEVICE_ERROR         The dependent serial port reset fails.

**/
EFI_STATUS
EFIAPI
TerminalConInReset (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  IN  BOOLEAN                         ExtendedVerification
  )
{
  EFI_STATUS    Status;
  TERMINAL_DEV  *TerminalDevice;

  TerminalDevice = TERMINAL_CON_IN_DEV_FROM_THIS (This);

  //
  // Report progress code here
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_PC_RESET),
    TerminalDevice->DevicePath
    );

  Status = TerminalDevice->SerialIo->Reset (TerminalDevice->SerialIo);

  //
  // Make all the internal buffer empty for keys
  //
  TerminalDevice->RawFiFo->Head             = TerminalDevice->RawFiFo->Tail;
  TerminalDevice->UnicodeFiFo->Head         = TerminalDevice->UnicodeFiFo->Tail;
  TerminalDevice->EfiKeyFiFo->Head          = TerminalDevice->EfiKeyFiFo->Tail;
  TerminalDevice->EfiKeyFiFoForNotify->Head = TerminalDevice->EfiKeyFiFoForNotify->Tail;

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_CONTROLLER_ERROR),
      TerminalDevice->DevicePath
      );
  }

  if (!EFI_ERROR (Status)) {
    Status = TerminalDevice->SerialIo->SetControl (TerminalDevice->SerialIo, EFI_SERIAL_DATA_TERMINAL_READY|EFI_SERIAL_REQUEST_TO_SEND);
  }

  return Status;
}

/**
  Implements EFI_SIMPLE_TEXT_INPUT_PROTOCOL.ReadKeyStroke().

  @param  This                Indicates the calling context.
  @param  Key                 A pointer to a buffer that is filled in with the
                              keystroke information for the key that was sent
                              from terminal.

  @retval EFI_SUCCESS         The keystroke information is returned successfully.
  @retval EFI_NOT_READY       There is no keystroke data available.
  @retval EFI_DEVICE_ERROR    The dependent serial device encounters error.
  @retval EFI_UNSUPPORTED     The device does not support the ability to read
                              keystroke data.

**/
EFI_STATUS
EFIAPI
TerminalConInReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  OUT EFI_INPUT_KEY                   *Key
  )
{
  TERMINAL_DEV  *TerminalDevice;
  EFI_STATUS    Status;
  EFI_KEY_DATA  KeyData;

  //
  //  get TERMINAL_DEV from "This" parameter.
  //
  TerminalDevice = TERMINAL_CON_IN_DEV_FROM_THIS (This);

  Status = ReadKeyStrokeWorker (TerminalDevice, &KeyData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem (Key, &KeyData.Key, sizeof (EFI_INPUT_KEY));

  return EFI_SUCCESS;
}

/**
  Check if the key already has been registered.

  If both RegsiteredData and InputData is NULL, then ASSERT().

  @param  RegsiteredData           A pointer to a buffer that is filled in with the
                                   keystroke state data for the key that was
                                   registered.
  @param  InputData                A pointer to a buffer that is filled in with the
                                   keystroke state data for the key that was
                                   pressed.

  @retval TRUE                     Key be pressed matches a registered key.
  @retval FALSE                    Match failed.

**/
BOOLEAN
IsKeyRegistered (
  IN EFI_KEY_DATA  *RegsiteredData,
  IN EFI_KEY_DATA  *InputData
  )
{
  ASSERT (RegsiteredData != NULL && InputData != NULL);

  if ((RegsiteredData->Key.ScanCode    != InputData->Key.ScanCode) ||
      (RegsiteredData->Key.UnicodeChar != InputData->Key.UnicodeChar))
  {
    return FALSE;
  }

  return TRUE;
}

/**
  Event notification function for EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL.WaitForKeyEx event
  Signal the event if there is key available

  @param  Event                    Indicates the event that invoke this function.
  @param  Context                  Indicates the calling context.

**/
VOID
EFIAPI
TerminalConInWaitForKeyEx (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  TerminalConInWaitForKey (Event, Context);
}

//
// Simple Text Input Ex protocol functions
//

/**
  Reset the input device and optionally run diagnostics

  @param  This                     Protocol instance pointer.
  @param  ExtendedVerification     Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS              The device was reset.
  @retval EFI_DEVICE_ERROR         The device is not functioning properly and could
                                   not be reset.

**/
EFI_STATUS
EFIAPI
TerminalConInResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  )
{
  EFI_STATUS    Status;
  TERMINAL_DEV  *TerminalDevice;

  TerminalDevice = TERMINAL_CON_IN_EX_DEV_FROM_THIS (This);

  Status = TerminalDevice->SimpleInput.Reset (&TerminalDevice->SimpleInput, ExtendedVerification);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Reads the next keystroke from the input device. The WaitForKey Event can
  be used to test for existence of a keystroke via WaitForEvent () call.

  @param  This                     Protocol instance pointer.
  @param  KeyData                  A pointer to a buffer that is filled in with the
                                   keystroke state data for the key that was
                                   pressed.

  @retval EFI_SUCCESS              The keystroke information was returned.
  @retval EFI_NOT_READY            There was no keystroke data available.
  @retval EFI_DEVICE_ERROR         The keystroke information was not returned due
                                   to hardware errors.
  @retval EFI_INVALID_PARAMETER    KeyData is NULL.
  @retval EFI_UNSUPPORTED          The device does not support the ability to read
                                   keystroke data.

**/
EFI_STATUS
EFIAPI
TerminalConInReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  OUT EFI_KEY_DATA                       *KeyData
  )
{
  TERMINAL_DEV  *TerminalDevice;

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TerminalDevice = TERMINAL_CON_IN_EX_DEV_FROM_THIS (This);

  return ReadKeyStrokeWorker (TerminalDevice, KeyData);
}

/**
  Set certain state for the input device.

  @param  This                     Protocol instance pointer.
  @param  KeyToggleState           A pointer to the EFI_KEY_TOGGLE_STATE to set the
                                   state for the input device.

  @retval EFI_SUCCESS              The device state was set successfully.
  @retval EFI_DEVICE_ERROR         The device is not functioning correctly and
                                   could not have the setting adjusted.
  @retval EFI_UNSUPPORTED          The device does not have the ability to set its
                                   state.
  @retval EFI_INVALID_PARAMETER    KeyToggleState is NULL.

**/
EFI_STATUS
EFIAPI
TerminalConInSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  )
{
  if (KeyToggleState == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*KeyToggleState & EFI_TOGGLE_STATE_VALID) != EFI_TOGGLE_STATE_VALID) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Register a notification function for a particular keystroke for the input device.

  @param  This                     Protocol instance pointer.
  @param  KeyData                  A pointer to a buffer that is filled in with
                                   the keystroke information for the key that was
                                   pressed. If KeyData.Key, KeyData.KeyState.KeyToggleState
                                   and KeyData.KeyState.KeyShiftState are 0, then any incomplete
                                   keystroke will trigger a notification of the KeyNotificationFunction.
  @param  KeyNotificationFunction  Points to the function to be called when the key
                                   sequence is typed specified by KeyData. This notification function
                                   should be called at <=TPL_CALLBACK.
  @param  NotifyHandle             Points to the unique handle assigned to the
                                   registered notification.

  @retval EFI_SUCCESS              The notification function was registered
                                   successfully.
  @retval EFI_OUT_OF_RESOURCES     Unable to allocate resources for necessary data
                                   structures.
  @retval EFI_INVALID_PARAMETER    KeyData or NotifyHandle is NULL.

**/
EFI_STATUS
EFIAPI
TerminalConInRegisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_DATA                       *KeyData,
  IN EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                              **NotifyHandle
  )
{
  TERMINAL_DEV                   *TerminalDevice;
  TERMINAL_CONSOLE_IN_EX_NOTIFY  *NewNotify;
  LIST_ENTRY                     *Link;
  LIST_ENTRY                     *NotifyList;
  TERMINAL_CONSOLE_IN_EX_NOTIFY  *CurrentNotify;

  if ((KeyData == NULL) || (NotifyHandle == NULL) || (KeyNotificationFunction == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  TerminalDevice = TERMINAL_CON_IN_EX_DEV_FROM_THIS (This);

  //
  // Return EFI_SUCCESS if the (KeyData, NotificationFunction) is already registered.
  //
  NotifyList = &TerminalDevice->NotifyList;
  for (Link = GetFirstNode (NotifyList); !IsNull (NotifyList, Link); Link = GetNextNode (NotifyList, Link)) {
    CurrentNotify = CR (
                      Link,
                      TERMINAL_CONSOLE_IN_EX_NOTIFY,
                      NotifyEntry,
                      TERMINAL_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                      );
    if (IsKeyRegistered (&CurrentNotify->KeyData, KeyData)) {
      if (CurrentNotify->KeyNotificationFn == KeyNotificationFunction) {
        *NotifyHandle = CurrentNotify;
        return EFI_SUCCESS;
      }
    }
  }

  //
  // Allocate resource to save the notification function
  //
  NewNotify = (TERMINAL_CONSOLE_IN_EX_NOTIFY *)AllocateZeroPool (sizeof (TERMINAL_CONSOLE_IN_EX_NOTIFY));
  if (NewNotify == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewNotify->Signature         = TERMINAL_CONSOLE_IN_EX_NOTIFY_SIGNATURE;
  NewNotify->KeyNotificationFn = KeyNotificationFunction;
  CopyMem (&NewNotify->KeyData, KeyData, sizeof (EFI_KEY_DATA));
  InsertTailList (&TerminalDevice->NotifyList, &NewNotify->NotifyEntry);

  *NotifyHandle = NewNotify;

  return EFI_SUCCESS;
}

/**
  Remove a registered notification function from a particular keystroke.

  @param  This                     Protocol instance pointer.
  @param  NotificationHandle       The handle of the notification function being
                                   unregistered.

  @retval EFI_SUCCESS              The notification function was unregistered
                                   successfully.
  @retval EFI_INVALID_PARAMETER    The NotificationHandle is invalid.

**/
EFI_STATUS
EFIAPI
TerminalConInUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  )
{
  TERMINAL_DEV                   *TerminalDevice;
  LIST_ENTRY                     *Link;
  TERMINAL_CONSOLE_IN_EX_NOTIFY  *CurrentNotify;
  LIST_ENTRY                     *NotifyList;

  if (NotificationHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TerminalDevice = TERMINAL_CON_IN_EX_DEV_FROM_THIS (This);

  NotifyList = &TerminalDevice->NotifyList;
  for (Link = GetFirstNode (NotifyList); !IsNull (NotifyList, Link); Link = GetNextNode (NotifyList, Link)) {
    CurrentNotify = CR (
                      Link,
                      TERMINAL_CONSOLE_IN_EX_NOTIFY,
                      NotifyEntry,
                      TERMINAL_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                      );
    if (CurrentNotify == NotificationHandle) {
      //
      // Remove the notification function from NotifyList and free resources
      //
      RemoveEntryList (&CurrentNotify->NotifyEntry);

      gBS->FreePool (CurrentNotify);
      return EFI_SUCCESS;
    }
  }

  //
  // Can not find the matching entry in database.
  //
  return EFI_INVALID_PARAMETER;
}

/**
  Translate raw data into Unicode (according to different encode), and
  translate Unicode into key information. (according to different standard).

  @param  TerminalDevice       Terminal driver private structure.

**/
VOID
TranslateRawDataToEfiKey (
  IN  TERMINAL_DEV  *TerminalDevice
  )
{
  switch (TerminalDevice->TerminalType) {
    case TerminalTypePcAnsi:
    case TerminalTypeVt100:
    case TerminalTypeVt100Plus:
    case TerminalTypeTtyTerm:
    case TerminalTypeLinux:
    case TerminalTypeXtermR6:
    case TerminalTypeVt400:
    case TerminalTypeSCO:
      AnsiRawDataToUnicode (TerminalDevice);
      UnicodeToEfiKey (TerminalDevice);
      break;

    case TerminalTypeVtUtf8:
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

/**
  Event notification function for EFI_SIMPLE_TEXT_INPUT_PROTOCOL.WaitForKey event
  Signal the event if there is key available

  @param  Event                    Indicates the event that invoke this function.
  @param  Context                  Indicates the calling context.

**/
VOID
EFIAPI
TerminalConInWaitForKey (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  //
  // Someone is waiting on the keystroke event, if there's
  // a key pending, signal the event
  //
  if (!IsEfiKeyFiFoEmpty ((TERMINAL_DEV *)Context)) {
    gBS->SignalEvent (Event);
  }
}

/**
  Timer handler to poll the key from serial.

  @param  Event                    Indicates the event that invoke this function.
  @param  Context                  Indicates the calling context.
**/
VOID
EFIAPI
TerminalConInTimerHandler (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS              Status;
  TERMINAL_DEV            *TerminalDevice;
  UINT32                  Control;
  UINT8                   Input;
  EFI_SERIAL_IO_MODE      *Mode;
  EFI_SERIAL_IO_PROTOCOL  *SerialIo;
  UINTN                   SerialInTimeOut;

  TerminalDevice = (TERMINAL_DEV *)Context;

  SerialIo = TerminalDevice->SerialIo;
  if (SerialIo == NULL) {
    return;
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
      //
      // According to BAUD rate to calculate the timeout value.
      //
      SerialInTimeOut = (1 + Mode->DataBits + Mode->StopBits) * 2 * 1000000 / (UINTN)Mode->BaudRate;
    }

    Status = SerialIo->SetAttributes (
                         SerialIo,
                         Mode->BaudRate,
                         Mode->ReceiveFifoDepth,
                         (UINT32)SerialInTimeOut,
                         (EFI_PARITY_TYPE)(Mode->Parity),
                         (UINT8)Mode->DataBits,
                         (EFI_STOP_BITS_TYPE)(Mode->StopBits)
                         );

    if (EFI_ERROR (Status)) {
      TerminalDevice->SerialInTimeOut = 0;
    } else {
      TerminalDevice->SerialInTimeOut = SerialInTimeOut;
    }
  }

  //
  // Check whether serial buffer is empty.
  // Skip the key transfer loop only if the SerialIo protocol instance
  // successfully reports EFI_SERIAL_INPUT_BUFFER_EMPTY.
  //
  Status = SerialIo->GetControl (SerialIo, &Control);
  if (EFI_ERROR (Status) || ((Control & EFI_SERIAL_INPUT_BUFFER_EMPTY) == 0)) {
    //
    // Fetch all the keys in the serial buffer,
    // and insert the byte stream into RawFIFO.
    //
    while (!IsRawFiFoFull (TerminalDevice)) {
      Status = GetOneKeyFromSerial (TerminalDevice->SerialIo, &Input);

      if (EFI_ERROR (Status)) {
        if (Status == EFI_DEVICE_ERROR) {
          REPORT_STATUS_CODE_WITH_DEVICE_PATH (
            EFI_ERROR_CODE | EFI_ERROR_MINOR,
            (EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_INPUT_ERROR),
            TerminalDevice->DevicePath
            );
        }

        break;
      }

      RawFiFoInsertOneKey (TerminalDevice, Input);
    }
  }

  //
  // Translate all the raw data in RawFIFO into EFI Key,
  // according to different terminal type supported.
  //
  TranslateRawDataToEfiKey (TerminalDevice);
}

/**
  Process key notify.

  @param  Event                 Indicates the event that invoke this function.
  @param  Context               Indicates the calling context.
**/
VOID
EFIAPI
KeyNotifyProcessHandler (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  BOOLEAN                        HasKey;
  TERMINAL_DEV                   *TerminalDevice;
  EFI_INPUT_KEY                  Key;
  EFI_KEY_DATA                   KeyData;
  LIST_ENTRY                     *Link;
  LIST_ENTRY                     *NotifyList;
  TERMINAL_CONSOLE_IN_EX_NOTIFY  *CurrentNotify;
  EFI_TPL                        OldTpl;

  TerminalDevice = (TERMINAL_DEV *)Context;

  //
  // Invoke notification functions.
  //
  NotifyList = &TerminalDevice->NotifyList;
  while (TRUE) {
    //
    // Enter critical section
    //
    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    HasKey = EfiKeyFiFoForNotifyRemoveOneKey (TerminalDevice->EfiKeyFiFoForNotify, &Key);
    CopyMem (&KeyData.Key, &Key, sizeof (EFI_INPUT_KEY));
    KeyData.KeyState.KeyShiftState  = 0;
    KeyData.KeyState.KeyToggleState = 0;
    //
    // Leave critical section
    //
    gBS->RestoreTPL (OldTpl);
    if (!HasKey) {
      break;
    }

    for (Link = GetFirstNode (NotifyList); !IsNull (NotifyList, Link); Link = GetNextNode (NotifyList, Link)) {
      CurrentNotify = CR (Link, TERMINAL_CONSOLE_IN_EX_NOTIFY, NotifyEntry, TERMINAL_CONSOLE_IN_EX_NOTIFY_SIGNATURE);
      if (IsKeyRegistered (&CurrentNotify->KeyData, &KeyData)) {
        CurrentNotify->KeyNotificationFn (&KeyData);
      }
    }
  }
}

/**
  Get one key out of serial buffer.

  @param  SerialIo           Serial I/O protocol attached to the serial device.
  @param  Output             The fetched key.

  @retval EFI_NOT_READY      If serial buffer is empty.
  @retval EFI_DEVICE_ERROR   If reading serial buffer encounter error.
  @retval EFI_SUCCESS        If reading serial buffer successfully, put
                             the fetched key to the parameter output.

**/
EFI_STATUS
GetOneKeyFromSerial (
  EFI_SERIAL_IO_PROTOCOL  *SerialIo,
  UINT8                   *Output
  )
{
  EFI_STATUS  Status;
  UINTN       Size;

  Size    = 1;
  *Output = 0;

  //
  // Read one key from serial I/O device.
  //
  Status = SerialIo->Read (SerialIo, &Size, Output);

  if (EFI_ERROR (Status)) {
    if (Status == EFI_TIMEOUT) {
      return EFI_NOT_READY;
    }

    return EFI_DEVICE_ERROR;
  }

  if (*Output == 0) {
    return EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}

/**
  Insert one byte raw data into the Raw Data FIFO.

  @param  TerminalDevice       Terminal driver private structure.
  @param  Input                The key will be input.

  @retval TRUE                 If insert successfully.
  @retval FALSE                If Raw Data buffer is full before key insertion,
                               and the key is lost.

**/
BOOLEAN
RawFiFoInsertOneKey (
  TERMINAL_DEV  *TerminalDevice,
  UINT8         Input
  )
{
  UINT8  Tail;

  Tail = TerminalDevice->RawFiFo->Tail;

  if (IsRawFiFoFull (TerminalDevice)) {
    //
    // Raw FIFO is full
    //
    return FALSE;
  }

  TerminalDevice->RawFiFo->Data[Tail] = Input;

  TerminalDevice->RawFiFo->Tail = (UINT8)((Tail + 1) % (RAW_FIFO_MAX_NUMBER + 1));

  return TRUE;
}

/**
  Remove one pre-fetched key out of the Raw Data FIFO.

  @param  TerminalDevice       Terminal driver private structure.
  @param  Output               The key will be removed.

  @retval TRUE                 If insert successfully.
  @retval FALSE                If Raw Data FIFO buffer is empty before remove operation.

**/
BOOLEAN
RawFiFoRemoveOneKey (
  TERMINAL_DEV  *TerminalDevice,
  UINT8         *Output
  )
{
  UINT8  Head;

  Head = TerminalDevice->RawFiFo->Head;

  if (IsRawFiFoEmpty (TerminalDevice)) {
    //
    //  FIFO is empty
    //
    *Output = 0;
    return FALSE;
  }

  *Output = TerminalDevice->RawFiFo->Data[Head];

  TerminalDevice->RawFiFo->Head = (UINT8)((Head + 1) % (RAW_FIFO_MAX_NUMBER + 1));

  return TRUE;
}

/**
  Clarify whether Raw Data FIFO buffer is empty.

  @param  TerminalDevice       Terminal driver private structure

  @retval TRUE                 If Raw Data FIFO buffer is empty.
  @retval FALSE                If Raw Data FIFO buffer is not empty.

**/
BOOLEAN
IsRawFiFoEmpty (
  TERMINAL_DEV  *TerminalDevice
  )
{
  if (TerminalDevice->RawFiFo->Head == TerminalDevice->RawFiFo->Tail) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Clarify whether Raw Data FIFO buffer is full.

  @param  TerminalDevice       Terminal driver private structure

  @retval TRUE                 If Raw Data FIFO buffer is full.
  @retval FALSE                If Raw Data FIFO buffer is not full.

**/
BOOLEAN
IsRawFiFoFull (
  TERMINAL_DEV  *TerminalDevice
  )
{
  UINT8  Tail;
  UINT8  Head;

  Tail = TerminalDevice->RawFiFo->Tail;
  Head = TerminalDevice->RawFiFo->Head;

  if (((Tail + 1) % (RAW_FIFO_MAX_NUMBER + 1)) == Head) {
    return TRUE;
  }

  return FALSE;
}

/**
  Insert one pre-fetched key into the FIFO buffer.

  @param  EfiKeyFiFo            Pointer to instance of EFI_KEY_FIFO.
  @param  Input                 The key will be input.

  @retval TRUE                  If insert successfully.
  @retval FALSE                 If FIFO buffer is full before key insertion,
                                and the key is lost.

**/
BOOLEAN
EfiKeyFiFoForNotifyInsertOneKey (
  EFI_KEY_FIFO   *EfiKeyFiFo,
  EFI_INPUT_KEY  *Input
  )
{
  UINT8  Tail;

  Tail = EfiKeyFiFo->Tail;

  if (IsEfiKeyFiFoForNotifyFull (EfiKeyFiFo)) {
    //
    // FIFO is full
    //
    return FALSE;
  }

  CopyMem (&EfiKeyFiFo->Data[Tail], Input, sizeof (EFI_INPUT_KEY));

  EfiKeyFiFo->Tail = (UINT8)((Tail + 1) % (FIFO_MAX_NUMBER + 1));

  return TRUE;
}

/**
  Remove one pre-fetched key out of the FIFO buffer.

  @param  EfiKeyFiFo            Pointer to instance of EFI_KEY_FIFO.
  @param  Output                The key will be removed.

  @retval TRUE                  If remove successfully.
  @retval FALSE                 If FIFO buffer is empty before remove operation.

**/
BOOLEAN
EfiKeyFiFoForNotifyRemoveOneKey (
  EFI_KEY_FIFO   *EfiKeyFiFo,
  EFI_INPUT_KEY  *Output
  )
{
  UINT8  Head;

  Head = EfiKeyFiFo->Head;
  ASSERT (Head < FIFO_MAX_NUMBER + 1);

  if (IsEfiKeyFiFoForNotifyEmpty (EfiKeyFiFo)) {
    //
    // FIFO is empty
    //
    Output->ScanCode    = SCAN_NULL;
    Output->UnicodeChar = 0;
    return FALSE;
  }

  CopyMem (Output, &EfiKeyFiFo->Data[Head], sizeof (EFI_INPUT_KEY));

  EfiKeyFiFo->Head = (UINT8)((Head + 1) % (FIFO_MAX_NUMBER + 1));

  return TRUE;
}

/**
  Clarify whether FIFO buffer is empty.

  @param  EfiKeyFiFo            Pointer to instance of EFI_KEY_FIFO.

  @retval TRUE                  If FIFO buffer is empty.
  @retval FALSE                 If FIFO buffer is not empty.

**/
BOOLEAN
IsEfiKeyFiFoForNotifyEmpty (
  EFI_KEY_FIFO  *EfiKeyFiFo
  )
{
  if (EfiKeyFiFo->Head == EfiKeyFiFo->Tail) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Clarify whether FIFO buffer is full.

  @param  EfiKeyFiFo            Pointer to instance of EFI_KEY_FIFO.

  @retval TRUE                  If FIFO buffer is full.
  @retval FALSE                 If FIFO buffer is not full.

**/
BOOLEAN
IsEfiKeyFiFoForNotifyFull (
  EFI_KEY_FIFO  *EfiKeyFiFo
  )
{
  UINT8  Tail;
  UINT8  Head;

  Tail = EfiKeyFiFo->Tail;
  Head = EfiKeyFiFo->Head;

  if (((Tail + 1) % (FIFO_MAX_NUMBER + 1)) == Head) {
    return TRUE;
  }

  return FALSE;
}

/**
  Insert one pre-fetched key into the FIFO buffer.

  @param  TerminalDevice       Terminal driver private structure.
  @param  Key                  The key will be input.

  @retval TRUE                 If insert successfully.
  @retval FALSE                If FIFO buffer is full before key insertion,
                               and the key is lost.

**/
BOOLEAN
EfiKeyFiFoInsertOneKey (
  TERMINAL_DEV   *TerminalDevice,
  EFI_INPUT_KEY  *Key
  )
{
  UINT8                          Tail;
  LIST_ENTRY                     *Link;
  LIST_ENTRY                     *NotifyList;
  TERMINAL_CONSOLE_IN_EX_NOTIFY  *CurrentNotify;
  EFI_KEY_DATA                   KeyData;

  Tail = TerminalDevice->EfiKeyFiFo->Tail;

  CopyMem (&KeyData.Key, Key, sizeof (EFI_INPUT_KEY));
  KeyData.KeyState.KeyShiftState  = 0;
  KeyData.KeyState.KeyToggleState = 0;

  //
  // Signal KeyNotify process event if this key pressed matches any key registered.
  //
  NotifyList = &TerminalDevice->NotifyList;
  for (Link = GetFirstNode (NotifyList); !IsNull (NotifyList, Link); Link = GetNextNode (NotifyList, Link)) {
    CurrentNotify = CR (
                      Link,
                      TERMINAL_CONSOLE_IN_EX_NOTIFY,
                      NotifyEntry,
                      TERMINAL_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                      );
    if (IsKeyRegistered (&CurrentNotify->KeyData, &KeyData)) {
      //
      // The key notification function needs to run at TPL_CALLBACK
      // while current TPL is TPL_NOTIFY. It will be invoked in
      // KeyNotifyProcessHandler() which runs at TPL_CALLBACK.
      //
      EfiKeyFiFoForNotifyInsertOneKey (TerminalDevice->EfiKeyFiFoForNotify, Key);
      gBS->SignalEvent (TerminalDevice->KeyNotifyProcessEvent);
      break;
    }
  }

  if (IsEfiKeyFiFoFull (TerminalDevice)) {
    //
    // Efi Key FIFO is full
    //
    return FALSE;
  }

  CopyMem (&TerminalDevice->EfiKeyFiFo->Data[Tail], Key, sizeof (EFI_INPUT_KEY));

  TerminalDevice->EfiKeyFiFo->Tail = (UINT8)((Tail + 1) % (FIFO_MAX_NUMBER + 1));

  return TRUE;
}

/**
  Remove one pre-fetched key out of the FIFO buffer.

  @param  TerminalDevice       Terminal driver private structure.
  @param  Output               The key will be removed.

  @retval TRUE                 If insert successfully.
  @retval FALSE                If FIFO buffer is empty before remove operation.

**/
BOOLEAN
EfiKeyFiFoRemoveOneKey (
  TERMINAL_DEV   *TerminalDevice,
  EFI_INPUT_KEY  *Output
  )
{
  UINT8  Head;

  Head = TerminalDevice->EfiKeyFiFo->Head;
  ASSERT (Head < FIFO_MAX_NUMBER + 1);

  if (IsEfiKeyFiFoEmpty (TerminalDevice)) {
    //
    //  FIFO is empty
    //
    Output->ScanCode    = SCAN_NULL;
    Output->UnicodeChar = 0;
    return FALSE;
  }

  CopyMem (Output, &TerminalDevice->EfiKeyFiFo->Data[Head], sizeof (EFI_INPUT_KEY));

  TerminalDevice->EfiKeyFiFo->Head = (UINT8)((Head + 1) % (FIFO_MAX_NUMBER + 1));

  return TRUE;
}

/**
  Clarify whether FIFO buffer is empty.

  @param  TerminalDevice       Terminal driver private structure

  @retval TRUE                 If FIFO buffer is empty.
  @retval FALSE                If FIFO buffer is not empty.

**/
BOOLEAN
IsEfiKeyFiFoEmpty (
  TERMINAL_DEV  *TerminalDevice
  )
{
  if (TerminalDevice->EfiKeyFiFo->Head == TerminalDevice->EfiKeyFiFo->Tail) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Clarify whether FIFO buffer is full.

  @param  TerminalDevice       Terminal driver private structure

  @retval TRUE                 If FIFO buffer is full.
  @retval FALSE                If FIFO buffer is not full.

**/
BOOLEAN
IsEfiKeyFiFoFull (
  TERMINAL_DEV  *TerminalDevice
  )
{
  UINT8  Tail;
  UINT8  Head;

  Tail = TerminalDevice->EfiKeyFiFo->Tail;
  Head = TerminalDevice->EfiKeyFiFo->Head;

  if (((Tail + 1) % (FIFO_MAX_NUMBER + 1)) == Head) {
    return TRUE;
  }

  return FALSE;
}

/**
  Insert one pre-fetched key into the Unicode FIFO buffer.

  @param  TerminalDevice       Terminal driver private structure.
  @param  Input                The key will be input.

  @retval TRUE                 If insert successfully.
  @retval FALSE                If Unicode FIFO buffer is full before key insertion,
                               and the key is lost.

**/
BOOLEAN
UnicodeFiFoInsertOneKey (
  TERMINAL_DEV  *TerminalDevice,
  UINT16        Input
  )
{
  UINT8  Tail;

  Tail = TerminalDevice->UnicodeFiFo->Tail;
  ASSERT (Tail < FIFO_MAX_NUMBER + 1);

  if (IsUnicodeFiFoFull (TerminalDevice)) {
    //
    // Unicode FIFO is full
    //
    return FALSE;
  }

  TerminalDevice->UnicodeFiFo->Data[Tail] = Input;

  TerminalDevice->UnicodeFiFo->Tail = (UINT8)((Tail + 1) % (FIFO_MAX_NUMBER + 1));

  return TRUE;
}

/**
  Remove one pre-fetched key out of the Unicode FIFO buffer.
  The caller should guarantee that Unicode FIFO buffer is not empty
  by IsUnicodeFiFoEmpty ().

  @param  TerminalDevice       Terminal driver private structure.
  @param  Output               The key will be removed.

**/
VOID
UnicodeFiFoRemoveOneKey (
  TERMINAL_DEV  *TerminalDevice,
  UINT16        *Output
  )
{
  UINT8  Head;

  Head = TerminalDevice->UnicodeFiFo->Head;
  ASSERT (Head < FIFO_MAX_NUMBER + 1);

  *Output = TerminalDevice->UnicodeFiFo->Data[Head];

  TerminalDevice->UnicodeFiFo->Head = (UINT8)((Head + 1) % (FIFO_MAX_NUMBER + 1));
}

/**
  Clarify whether Unicode FIFO buffer is empty.

  @param  TerminalDevice       Terminal driver private structure

  @retval TRUE                 If Unicode FIFO buffer is empty.
  @retval FALSE                If Unicode FIFO buffer is not empty.

**/
BOOLEAN
IsUnicodeFiFoEmpty (
  TERMINAL_DEV  *TerminalDevice
  )
{
  if (TerminalDevice->UnicodeFiFo->Head == TerminalDevice->UnicodeFiFo->Tail) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Clarify whether Unicode FIFO buffer is full.

  @param  TerminalDevice       Terminal driver private structure

  @retval TRUE                 If Unicode FIFO buffer is full.
  @retval FALSE                If Unicode FIFO buffer is not full.

**/
BOOLEAN
IsUnicodeFiFoFull (
  TERMINAL_DEV  *TerminalDevice
  )
{
  UINT8  Tail;
  UINT8  Head;

  Tail = TerminalDevice->UnicodeFiFo->Tail;
  Head = TerminalDevice->UnicodeFiFo->Head;

  if (((Tail + 1) % (FIFO_MAX_NUMBER + 1)) == Head) {
    return TRUE;
  }

  return FALSE;
}

/**
  Update the Unicode characters from a terminal input device into EFI Keys FIFO.

  @param TerminalDevice   The terminal device to use to translate raw input into EFI Keys

**/
VOID
UnicodeToEfiKeyFlushState (
  IN  TERMINAL_DEV  *TerminalDevice
  )
{
  EFI_INPUT_KEY  Key;
  UINT32         InputState;

  InputState = TerminalDevice->InputState;

  if (IsEfiKeyFiFoFull (TerminalDevice)) {
    return;
  }

  if ((InputState & INPUT_STATE_ESC) != 0) {
    Key.ScanCode    = SCAN_ESC;
    Key.UnicodeChar = 0;
    EfiKeyFiFoInsertOneKey (TerminalDevice, &Key);
  }

  if ((InputState & INPUT_STATE_CSI) != 0) {
    Key.ScanCode    = SCAN_NULL;
    Key.UnicodeChar = CSI;
    EfiKeyFiFoInsertOneKey (TerminalDevice, &Key);
  }

  if ((InputState & INPUT_STATE_LEFTOPENBRACKET) != 0) {
    Key.ScanCode    = SCAN_NULL;
    Key.UnicodeChar = LEFTOPENBRACKET;
    EfiKeyFiFoInsertOneKey (TerminalDevice, &Key);
  }

  if ((InputState & INPUT_STATE_O) != 0) {
    Key.ScanCode    = SCAN_NULL;
    Key.UnicodeChar = 'O';
    EfiKeyFiFoInsertOneKey (TerminalDevice, &Key);
  }

  if ((InputState & INPUT_STATE_2) != 0) {
    Key.ScanCode    = SCAN_NULL;
    Key.UnicodeChar = '2';
    EfiKeyFiFoInsertOneKey (TerminalDevice, &Key);
  }

  //
  // Cancel the timer.
  //
  gBS->SetTimer (
         TerminalDevice->TwoSecondTimeOut,
         TimerCancel,
         0
         );

  TerminalDevice->InputState = INPUT_STATE_DEFAULT;
}

/**
  Converts a stream of Unicode characters from a terminal input device into EFI Keys that
  can be read through the Simple Input Protocol.

  The table below shows the keyboard input mappings that this function supports.
  If the ESC sequence listed in one of the columns is presented, then it is translated
  into the corresponding EFI Scan Code.  If a matching sequence is not found, then the raw
  key strokes are converted into EFI Keys.

  2 seconds are allowed for an ESC sequence to be completed.  If the ESC sequence is not
  completed in 2 seconds, then the raw key strokes of the partial ESC sequence are
  converted into EFI Keys.
  There is one special input sequence that will force the system to reset.
  This is ESC R ESC r ESC R.

  Note: current implementation support terminal types include: PC ANSI, VT100+/VTUTF8, VT100.
        The table below is not same with UEFI Spec 2.3 Appendix B Table 201(not support ANSI X3.64 /
        DEC VT200-500 and extra support PC ANSI, VT100)since UEFI Table 201 is just an example.

  Symbols used in table below
  ===========================
    ESC = 0x1B
    CSI = 0x9B
    DEL = 0x7f
    ^   = CTRL

  +=========+======+===========+==========+==========+
  |         | EFI  | UEFI 2.0  |          |          |
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
  | F11     | 0x15 |           | ESC !    |          |
  | F12     | 0x16 |           | ESC @    |          |
  +=========+======+===========+==========+==========+

Putty function key map:
  +=========+======+===========+=============+=============+=============+=========+
  |         | EFI  |           |             |             |             |         |
  |         | Scan |  VT100+   |             |  Normal     |             |         |
  |   KEY   | Code |  VTUTF8   | Xterm R6    |  VT400      | Linux       | SCO     |
  +=========+======+===========+=============+=============+=============+=========+
  | F1      | 0x0B | ESC O P   | ESC O P     | ESC [ 1 1 ~ | ESC [ [ A   | ESC [ M |
  | F2      | 0x0C | ESC O Q   | ESC O Q     | ESC [ 1 2 ~ | ESC [ [ B   | ESC [ N |
  | F3      | 0x0D | ESC O R   | ESC O R     | ESC [ 1 3 ~ | ESC [ [ C   | ESC [ O |
  | F4      | 0x0E | ESC O S   | ESC O S     | ESC [ 1 4 ~ | ESC [ [ D   | ESC [ P |
  | F5      | 0x0F | ESC O T   | ESC [ 1 5 ~ | ESC [ 1 5 ~ | ESC [ [ E   | ESC [ Q |
  | F6      | 0x10 | ESC O U   | ESC [ 1 7 ~ | ESC [ 1 7 ~ | ESC [ 1 7 ~ | ESC [ R |
  | F7      | 0x11 | ESC O V   | ESC [ 1 8 ~ | ESC [ 1 8 ~ | ESC [ 1 8 ~ | ESC [ S |
  | F8      | 0x12 | ESC O W   | ESC [ 1 9 ~ | ESC [ 1 9 ~ | ESC [ 1 9 ~ | ESC [ T |
  | F9      | 0x13 | ESC O X   | ESC [ 2 0 ~ | ESC [ 2 0 ~ | ESC [ 2 0 ~ | ESC [ U |
  | F10     | 0x14 | ESC O Y   | ESC [ 2 1 ~ | ESC [ 2 1 ~ | ESC [ 2 1 ~ | ESC [ V |
  | Escape  | 0x17 | ESC       | ESC         | ESC         | ESC         | ESC     |
  | F11     | 0x15 | ESC O Z   | ESC [ 2 3 ~ | ESC [ 2 3 ~ | ESC [ 2 3 ~ | ESC [ W |
  | F12     | 0x16 | ESC O [   | ESC [ 2 4 ~ | ESC [ 2 4 ~ | ESC [ 2 4 ~ | ESC [ X |
  +=========+======+===========+=============+=============+=============+=========+

  Special Mappings
  ================
  ESC R ESC r ESC R = Reset System

  @param TerminalDevice   The terminal device to use to translate raw input into EFI Keys

**/
VOID
UnicodeToEfiKey (
  IN  TERMINAL_DEV  *TerminalDevice
  )
{
  EFI_STATUS     Status;
  EFI_STATUS     TimerStatus;
  UINT16         UnicodeChar;
  EFI_INPUT_KEY  Key;
  BOOLEAN        SetDefaultResetState;

  TimerStatus = gBS->CheckEvent (TerminalDevice->TwoSecondTimeOut);

  if (!EFI_ERROR (TimerStatus)) {
    UnicodeToEfiKeyFlushState (TerminalDevice);
    TerminalDevice->ResetState = RESET_STATE_DEFAULT;
  }

  while (!IsUnicodeFiFoEmpty (TerminalDevice) && !IsEfiKeyFiFoFull (TerminalDevice)) {
    if (TerminalDevice->InputState != INPUT_STATE_DEFAULT) {
      //
      // Check to see if the 2 seconds timer has expired
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
    UnicodeFiFoRemoveOneKey (TerminalDevice, &UnicodeChar);

    SetDefaultResetState = TRUE;

    switch (TerminalDevice->InputState) {
      case INPUT_STATE_DEFAULT:

        break;

      case INPUT_STATE_ESC:

        if (UnicodeChar == LEFTOPENBRACKET) {
          TerminalDevice->InputState |= INPUT_STATE_LEFTOPENBRACKET;
          TerminalDevice->ResetState  = RESET_STATE_DEFAULT;
          continue;
        }

        if ((UnicodeChar == 'O') && ((TerminalDevice->TerminalType == TerminalTypeVt100) ||
                                     (TerminalDevice->TerminalType == TerminalTypeTtyTerm) ||
                                     (TerminalDevice->TerminalType == TerminalTypeXtermR6) ||
                                     (TerminalDevice->TerminalType == TerminalTypeVt100Plus) ||
                                     (TerminalDevice->TerminalType == TerminalTypeVtUtf8)))
        {
          TerminalDevice->InputState |= INPUT_STATE_O;
          TerminalDevice->ResetState  = RESET_STATE_DEFAULT;
          continue;
        }

        Key.ScanCode = SCAN_NULL;

        if ((TerminalDevice->TerminalType == TerminalTypeVt100Plus) ||
            (TerminalDevice->TerminalType == TerminalTypeVtUtf8))
        {
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
            case '!':
              Key.ScanCode = SCAN_F11;
              break;
            case '@':
              Key.ScanCode = SCAN_F12;
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
            default:
              break;
          }
        }

        switch (UnicodeChar) {
          case 'R':
            if (TerminalDevice->ResetState == RESET_STATE_DEFAULT) {
              TerminalDevice->ResetState = RESET_STATE_ESC_R;
              SetDefaultResetState       = FALSE;
            } else if (TerminalDevice->ResetState == RESET_STATE_ESC_R_ESC_R) {
              gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
            }

            Key.ScanCode = SCAN_NULL;
            break;
          case 'r':
            if (TerminalDevice->ResetState == RESET_STATE_ESC_R) {
              TerminalDevice->ResetState = RESET_STATE_ESC_R_ESC_R;
              SetDefaultResetState       = FALSE;
            }

            Key.ScanCode = SCAN_NULL;
            break;
          default:
            break;
        }

        if (SetDefaultResetState) {
          TerminalDevice->ResetState = RESET_STATE_DEFAULT;
        }

        if (Key.ScanCode != SCAN_NULL) {
          Key.UnicodeChar = 0;
          EfiKeyFiFoInsertOneKey (TerminalDevice, &Key);
          TerminalDevice->InputState = INPUT_STATE_DEFAULT;
          UnicodeToEfiKeyFlushState (TerminalDevice);
          continue;
        }

        UnicodeToEfiKeyFlushState (TerminalDevice);

        break;

      case INPUT_STATE_ESC | INPUT_STATE_O:

        TerminalDevice->ResetState = RESET_STATE_DEFAULT;

        Key.ScanCode = SCAN_NULL;

        if (TerminalDevice->TerminalType == TerminalTypeVt100) {
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
            default:
              break;
          }
        } else if (TerminalDevice->TerminalType == TerminalTypeTtyTerm) {
          /* Also accept VT100 escape codes for F1-F4, HOME and END for TTY term */
          switch (UnicodeChar) {
            case 'P':
              Key.ScanCode = SCAN_F1;
              break;
            case 'Q':
              Key.ScanCode = SCAN_F2;
              break;
            case 'R':
              Key.ScanCode = SCAN_F3;
              break;
            case 'S':
              Key.ScanCode = SCAN_F4;
              break;
            case 'H':
              Key.ScanCode = SCAN_HOME;
              break;
            case 'F':
              Key.ScanCode = SCAN_END;
              break;
          }
        } else if ((TerminalDevice->TerminalType == TerminalTypeVt100Plus) ||
                   (TerminalDevice->TerminalType == TerminalTypeVtUtf8))
        {
          switch (UnicodeChar) {
            case 'P':
              Key.ScanCode = SCAN_F1;
              break;
            case 'Q':
              Key.ScanCode = SCAN_F2;
              break;
            case 'R':
              Key.ScanCode = SCAN_F3;
              break;
            case 'S':
              Key.ScanCode = SCAN_F4;
              break;
            case 'T':
              Key.ScanCode = SCAN_F5;
              break;
            case 'U':
              Key.ScanCode = SCAN_F6;
              break;
            case 'V':
              Key.ScanCode = SCAN_F7;
              break;
            case 'W':
              Key.ScanCode = SCAN_F8;
              break;
            case 'X':
              Key.ScanCode = SCAN_F9;
              break;
            case 'Y':
              Key.ScanCode = SCAN_F10;
              break;
            case 'Z':
              Key.ScanCode = SCAN_F11;
              break;
            case '[':
              Key.ScanCode = SCAN_F12;
              break;
          }
        } else if (TerminalDevice->TerminalType == TerminalTypeXtermR6) {
          switch (UnicodeChar) {
            case 'P':
              Key.ScanCode = SCAN_F1;
              break;
            case 'Q':
              Key.ScanCode = SCAN_F2;
              break;
            case 'R':
              Key.ScanCode = SCAN_F3;
              break;
            case 'S':
              Key.ScanCode = SCAN_F4;
              break;
          }
        }

        if (Key.ScanCode != SCAN_NULL) {
          Key.UnicodeChar = 0;
          EfiKeyFiFoInsertOneKey (TerminalDevice, &Key);
          TerminalDevice->InputState = INPUT_STATE_DEFAULT;
          UnicodeToEfiKeyFlushState (TerminalDevice);
          continue;
        }

        UnicodeToEfiKeyFlushState (TerminalDevice);

        break;

      case INPUT_STATE_ESC | INPUT_STATE_LEFTOPENBRACKET:

        if ((UnicodeChar == '1') && ((TerminalDevice->TerminalType == TerminalTypeXtermR6) ||
                                     (TerminalDevice->TerminalType == TerminalTypeVt400) ||
                                     (TerminalDevice->TerminalType == TerminalTypeLinux)))
        {
          TerminalDevice->InputState |= INPUT_STATE_1;
          continue;
        }

        if ((UnicodeChar == '2') && ((TerminalDevice->TerminalType == TerminalTypeXtermR6) ||
                                     (TerminalDevice->TerminalType == TerminalTypeVt400) ||
                                     (TerminalDevice->TerminalType == TerminalTypeLinux)))
        {
          TerminalDevice->InputState |= INPUT_STATE_2;
          continue;
        }

        if ((UnicodeChar == LEFTOPENBRACKET) && (TerminalDevice->TerminalType == TerminalTypeLinux)) {
          TerminalDevice->InputState |= INPUT_STATE_LEFTOPENBRACKET_2ND;
          continue;
        }

        TerminalDevice->ResetState = RESET_STATE_DEFAULT;

        Key.ScanCode = SCAN_NULL;

        if ((TerminalDevice->TerminalType == TerminalTypePcAnsi) ||
            (TerminalDevice->TerminalType == TerminalTypeVt100) ||
            (TerminalDevice->TerminalType == TerminalTypeVt100Plus) ||
            (TerminalDevice->TerminalType == TerminalTypeVtUtf8) ||
            (TerminalDevice->TerminalType == TerminalTypeTtyTerm) ||
            (TerminalDevice->TerminalType == TerminalTypeLinux) ||
            (TerminalDevice->TerminalType == TerminalTypeXtermR6) ||
            (TerminalDevice->TerminalType == TerminalTypeVt400) ||
            (TerminalDevice->TerminalType == TerminalTypeSCO))
        {
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
              if ((TerminalDevice->TerminalType == TerminalTypePcAnsi) ||
                  (TerminalDevice->TerminalType == TerminalTypeVt100) ||
                  (TerminalDevice->TerminalType == TerminalTypeTtyTerm))
              {
                Key.ScanCode = SCAN_HOME;
              }

              break;
            case 'F':
              if ((TerminalDevice->TerminalType == TerminalTypePcAnsi) ||
                  (TerminalDevice->TerminalType == TerminalTypeTtyTerm))
              {
                Key.ScanCode = SCAN_END;
              }

              break;
            case 'K':
              if (TerminalDevice->TerminalType == TerminalTypeVt100) {
                Key.ScanCode = SCAN_END;
              }

              break;
            case 'L':
            case '@':
              if ((TerminalDevice->TerminalType == TerminalTypePcAnsi) ||
                  (TerminalDevice->TerminalType == TerminalTypeVt100))
              {
                Key.ScanCode = SCAN_INSERT;
              }

              break;
            case 'X':
              if (TerminalDevice->TerminalType == TerminalTypePcAnsi) {
                Key.ScanCode = SCAN_DELETE;
              } else if (TerminalDevice->TerminalType == TerminalTypeSCO) {
                Key.ScanCode = SCAN_F12;
              }

              break;
            case 'P':
              if (TerminalDevice->TerminalType == TerminalTypeVt100) {
                Key.ScanCode = SCAN_DELETE;
              } else if ((TerminalDevice->TerminalType == TerminalTypePcAnsi) ||
                         (TerminalDevice->TerminalType == TerminalTypeSCO))
              {
                Key.ScanCode = SCAN_F4;
              }

              break;
            case 'I':
              if (TerminalDevice->TerminalType == TerminalTypePcAnsi) {
                Key.ScanCode = SCAN_PAGE_UP;
              }

              break;
            case 'V':
              if ((TerminalDevice->TerminalType == TerminalTypePcAnsi) ||
                  (TerminalDevice->TerminalType == TerminalTypeSCO))
              {
                Key.ScanCode = SCAN_F10;
              }

              break;
            case '?':
              if (TerminalDevice->TerminalType == TerminalTypeVt100) {
                Key.ScanCode = SCAN_PAGE_UP;
              }

              break;
            case 'G':
              if (TerminalDevice->TerminalType == TerminalTypePcAnsi) {
                Key.ScanCode = SCAN_PAGE_DOWN;
              }

              break;
            case 'U':
              if ((TerminalDevice->TerminalType == TerminalTypePcAnsi) ||
                  (TerminalDevice->TerminalType == TerminalTypeSCO))
              {
                Key.ScanCode = SCAN_F9;
              }

              break;
            case '/':
              if (TerminalDevice->TerminalType == TerminalTypeVt100) {
                Key.ScanCode = SCAN_PAGE_DOWN;
              }

              break;
            case 'M':
              if ((TerminalDevice->TerminalType == TerminalTypePcAnsi) ||
                  (TerminalDevice->TerminalType == TerminalTypeSCO))
              {
                Key.ScanCode = SCAN_F1;
              }

              break;
            case 'N':
              if ((TerminalDevice->TerminalType == TerminalTypePcAnsi) ||
                  (TerminalDevice->TerminalType == TerminalTypeSCO))
              {
                Key.ScanCode = SCAN_F2;
              }

              break;
            case 'O':
              if ((TerminalDevice->TerminalType == TerminalTypePcAnsi) ||
                  (TerminalDevice->TerminalType == TerminalTypeSCO))
              {
                Key.ScanCode = SCAN_F3;
              }

              break;
            case 'Q':
              if ((TerminalDevice->TerminalType == TerminalTypePcAnsi) ||
                  (TerminalDevice->TerminalType == TerminalTypeSCO))
              {
                Key.ScanCode = SCAN_F5;
              }

              break;
            case 'R':
              if ((TerminalDevice->TerminalType == TerminalTypePcAnsi) ||
                  (TerminalDevice->TerminalType == TerminalTypeSCO))
              {
                Key.ScanCode = SCAN_F6;
              }

              break;
            case 'S':
              if ((TerminalDevice->TerminalType == TerminalTypePcAnsi) ||
                  (TerminalDevice->TerminalType == TerminalTypeSCO))
              {
                Key.ScanCode = SCAN_F7;
              }

              break;
            case 'T':
              if ((TerminalDevice->TerminalType == TerminalTypePcAnsi) ||
                  (TerminalDevice->TerminalType == TerminalTypeSCO))
              {
                Key.ScanCode = SCAN_F8;
              }

              break;
            case 'W':
              if (TerminalDevice->TerminalType == TerminalTypeSCO) {
                Key.ScanCode = SCAN_F11;
              }

              break;
            default:
              break;
          }
        }

        /*
         * The VT220 escape codes that the TTY terminal accepts all have
         * numeric codes, and there are no ambiguous prefixes shared with
         * other terminal types.
         */
        if ((TerminalDevice->TerminalType == TerminalTypeTtyTerm) &&
            (Key.ScanCode == SCAN_NULL) &&
            (UnicodeChar >= '0') &&
            (UnicodeChar <= '9'))
        {
          TerminalDevice->TtyEscapeStr[0] = UnicodeChar;
          TerminalDevice->TtyEscapeIndex  = 1;
          TerminalDevice->InputState     |= INPUT_STATE_LEFTOPENBRACKET_TTY;
          continue;
        }

        if (Key.ScanCode != SCAN_NULL) {
          Key.UnicodeChar = 0;
          EfiKeyFiFoInsertOneKey (TerminalDevice, &Key);
          TerminalDevice->InputState = INPUT_STATE_DEFAULT;
          UnicodeToEfiKeyFlushState (TerminalDevice);
          continue;
        }

        UnicodeToEfiKeyFlushState (TerminalDevice);

        break;

      case INPUT_STATE_ESC | INPUT_STATE_LEFTOPENBRACKET | INPUT_STATE_1:

        TerminalDevice->ResetState = RESET_STATE_DEFAULT;

        Key.ScanCode = SCAN_NULL;

        if ((TerminalDevice->TerminalType == TerminalTypeXtermR6) ||
            (TerminalDevice->TerminalType == TerminalTypeVt400) ||
            (TerminalDevice->TerminalType == TerminalTypeLinux))
        {
          switch (UnicodeChar) {
            case '1':
              if (TerminalDevice->TerminalType == TerminalTypeVt400) {
                Key.ScanCode = SCAN_F1;
              }

              break;
            case '2':
              if (TerminalDevice->TerminalType == TerminalTypeVt400) {
                Key.ScanCode = SCAN_F2;
              }

              break;
            case '3':
              if (TerminalDevice->TerminalType == TerminalTypeVt400) {
                Key.ScanCode = SCAN_F3;
              }

              break;
            case '4':
              if (TerminalDevice->TerminalType == TerminalTypeVt400) {
                Key.ScanCode = SCAN_F4;
              }

              break;
            case '5':
              if ((TerminalDevice->TerminalType == TerminalTypeXtermR6) ||
                  (TerminalDevice->TerminalType == TerminalTypeVt400))
              {
                Key.ScanCode = SCAN_F5;
              }

              break;
            case '7':
              Key.ScanCode = SCAN_F6;
              break;
            case '8':
              Key.ScanCode = SCAN_F7;
              break;
            case '9':
              Key.ScanCode = SCAN_F8;
              break;
          }
        }

        if (Key.ScanCode != SCAN_NULL) {
          Key.UnicodeChar = 0;
          EfiKeyFiFoInsertOneKey (TerminalDevice, &Key);
          TerminalDevice->InputState = INPUT_STATE_DEFAULT;
          UnicodeToEfiKeyFlushState (TerminalDevice);
          continue;
        }

        UnicodeToEfiKeyFlushState (TerminalDevice);

        break;

      case INPUT_STATE_ESC | INPUT_STATE_LEFTOPENBRACKET | INPUT_STATE_2:

        TerminalDevice->InputState = INPUT_STATE_DEFAULT;
        Key.ScanCode               = SCAN_NULL;
        if ((TerminalDevice->TerminalType == TerminalTypeXtermR6) ||
            (TerminalDevice->TerminalType == TerminalTypeVt400) ||
            (TerminalDevice->TerminalType == TerminalTypeLinux))
        {
          switch (UnicodeChar) {
            case '0':
              Key.ScanCode = SCAN_F9;
              break;
            case '1':
              Key.ScanCode = SCAN_F10;
              break;
            case '3':
              Key.ScanCode = SCAN_F11;
              break;
            case '4':
              Key.ScanCode = SCAN_F12;
              break;
          }
        }

        if (Key.ScanCode != SCAN_NULL) {
          Key.UnicodeChar = 0;
          EfiKeyFiFoInsertOneKey (TerminalDevice, &Key);
          TerminalDevice->InputState = INPUT_STATE_DEFAULT;
          UnicodeToEfiKeyFlushState (TerminalDevice);
          continue;
        }

        UnicodeToEfiKeyFlushState (TerminalDevice);

        break;

      case INPUT_STATE_ESC | INPUT_STATE_LEFTOPENBRACKET | INPUT_STATE_LEFTOPENBRACKET_2ND:

        TerminalDevice->InputState = INPUT_STATE_DEFAULT;
        Key.ScanCode               = SCAN_NULL;

        if (TerminalDevice->TerminalType == TerminalTypeLinux) {
          switch (UnicodeChar) {
            case 'A':
              Key.ScanCode = SCAN_F1;
              break;
            case 'B':
              Key.ScanCode = SCAN_F2;
              break;
            case 'C':
              Key.ScanCode = SCAN_F3;
              break;
            case 'D':
              Key.ScanCode = SCAN_F4;
              break;
            case 'E':
              Key.ScanCode = SCAN_F5;
              break;
          }
        }

        if (Key.ScanCode != SCAN_NULL) {
          Key.UnicodeChar = 0;
          EfiKeyFiFoInsertOneKey (TerminalDevice, &Key);
          TerminalDevice->InputState = INPUT_STATE_DEFAULT;
          UnicodeToEfiKeyFlushState (TerminalDevice);
          continue;
        }

        UnicodeToEfiKeyFlushState (TerminalDevice);

        break;

      case INPUT_STATE_ESC | INPUT_STATE_LEFTOPENBRACKET | INPUT_STATE_LEFTOPENBRACKET_TTY:
        /*
         * Here we handle the VT220 escape codes that we accept.  This
         * state is only used by the TTY terminal type.
         */
        Key.ScanCode = SCAN_NULL;
        if (TerminalDevice->TerminalType == TerminalTypeTtyTerm) {
          if ((UnicodeChar == '~') && (TerminalDevice->TtyEscapeIndex <= 2)) {
            UINT16  EscCode;
            TerminalDevice->TtyEscapeStr[TerminalDevice->TtyEscapeIndex] = 0; /* Terminate string */
            EscCode                                                      = (UINT16)StrDecimalToUintn (TerminalDevice->TtyEscapeStr);
            switch (EscCode) {
              case 2:
                Key.ScanCode = SCAN_INSERT;
                break;
              case 3:
                Key.ScanCode = SCAN_DELETE;
                break;
              case 5:
                Key.ScanCode = SCAN_PAGE_UP;
                break;
              case 6:
                Key.ScanCode = SCAN_PAGE_DOWN;
                break;
              case 11:
              case 12:
              case 13:
              case 14:
              case 15:
                Key.ScanCode = SCAN_F1 + EscCode - 11;
                break;
              case 17:
              case 18:
              case 19:
              case 20:
              case 21:
                Key.ScanCode = SCAN_F6 + EscCode - 17;
                break;
              case 23:
              case 24:
                Key.ScanCode = SCAN_F11 + EscCode - 23;
                break;
              default:
                break;
            }
          } else if (TerminalDevice->TtyEscapeIndex == 1) {
            /* 2 character escape code   */
            TerminalDevice->TtyEscapeStr[TerminalDevice->TtyEscapeIndex++] = UnicodeChar;
            continue;
          } else {
            DEBUG ((DEBUG_ERROR, "Unexpected state in escape2\n"));
          }
        }

        TerminalDevice->ResetState = RESET_STATE_DEFAULT;

        if (Key.ScanCode != SCAN_NULL) {
          Key.UnicodeChar = 0;
          EfiKeyFiFoInsertOneKey (TerminalDevice, &Key);
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

    if (UnicodeChar == CSI) {
      TerminalDevice->InputState = INPUT_STATE_CSI;
    }

    if (TerminalDevice->InputState != INPUT_STATE_DEFAULT) {
      Status = gBS->SetTimer (
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
      if (TerminalDevice->TerminalType == TerminalTypeTtyTerm) {
        Key.ScanCode    = SCAN_NULL;
        Key.UnicodeChar = CHAR_BACKSPACE;
      } else {
        Key.ScanCode    = SCAN_DELETE;
        Key.UnicodeChar = 0;
      }
    } else {
      Key.ScanCode    = SCAN_NULL;
      Key.UnicodeChar = UnicodeChar;
    }

    EfiKeyFiFoInsertOneKey (TerminalDevice, &Key);
  }
}
