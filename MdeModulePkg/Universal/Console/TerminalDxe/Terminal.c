/** @file
  Produces Simple Text Input Protocol, Simple Text Input Extended Protocol and
  Simple Text Output Protocol upon Serial IO Protocol.

Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Terminal.h"

//
// Globals
//
EFI_DRIVER_BINDING_PROTOCOL  gTerminalDriverBinding = {
  TerminalDriverBindingSupported,
  TerminalDriverBindingStart,
  TerminalDriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_GUID  *mTerminalType[] = {
  &gEfiPcAnsiGuid,
  &gEfiVT100Guid,
  &gEfiVT100PlusGuid,
  &gEfiVTUTF8Guid,
  &gEfiTtyTermGuid,
  &gEdkiiLinuxTermGuid,
  &gEdkiiXtermR6Guid,
  &gEdkiiVT400Guid,
  &gEdkiiSCOTermGuid
};

CHAR16  *mSerialConsoleNames[] = {
  L"PC-ANSI Serial Console",
  L"VT-100 Serial Console",
  L"VT-100+ Serial Console",
  L"VT-UTF8 Serial Console",
  L"Tty Terminal Serial Console",
  L"Linux Terminal Serial Console",
  L"Xterm R6 Serial Console",
  L"VT-400 Serial Console",
  L"SCO Terminal Serial Console"
};

TERMINAL_DEV  mTerminalDevTemplate = {
  TERMINAL_DEV_SIGNATURE,
  NULL,
  0,
  NULL,
  NULL,
  {   // SimpleTextInput
    TerminalConInReset,
    TerminalConInReadKeyStroke,
    NULL
  },
  {   // SimpleTextOutput
    TerminalConOutReset,
    TerminalConOutOutputString,
    TerminalConOutTestString,
    TerminalConOutQueryMode,
    TerminalConOutSetMode,
    TerminalConOutSetAttribute,
    TerminalConOutClearScreen,
    TerminalConOutSetCursorPosition,
    TerminalConOutEnableCursor,
    NULL
  },
  {                                           // SimpleTextOutputMode
    1,                                        // MaxMode
    0,                                        // Mode
    EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK), // Attribute
    0,                                        // CursorColumn
    0,                                        // CursorRow
    TRUE                                      // CursorVisible
  },
  NULL, // TerminalConsoleModeData
  0,    // SerialInTimeOut

  NULL, // RawFifo
  NULL, // UnicodeFiFo
  NULL, // EfiKeyFiFo
  NULL, // EfiKeyFiFoForNotify

  NULL, // ControllerNameTable
  NULL, // TimerEvent
  NULL, // TwoSecondTimeOut
  INPUT_STATE_DEFAULT,
  RESET_STATE_DEFAULT,
  {
    0,
    0,
    0
  },
  0,
  FALSE,
  {   // SimpleTextInputEx
    TerminalConInResetEx,
    TerminalConInReadKeyStrokeEx,
    NULL,
    TerminalConInSetState,
    TerminalConInRegisterKeyNotify,
    TerminalConInUnregisterKeyNotify,
  },
  {   // NotifyList
    NULL,
    NULL,
  },
  NULL // KeyNotifyProcessEvent
};

TERMINAL_CONSOLE_MODE_DATA  mTerminalConsoleModeData[] = {
  { 80,  25 },
  { 80,  50 },
  { 100, 31 },  //  800 x 600
  { 128, 40 },  // 1024 x 768
  { 160, 42 },  // 1280 x 800
  { 240, 56 },  // 1920 x 1080
  //
  // New modes can be added here.
  //
};

/**
  Convert the GUID representation of terminal type to enum type.

  @param Guid  The GUID representation of terminal type.

  @return  The terminal type in enum type.
**/
TERMINAL_TYPE
TerminalTypeFromGuid (
  IN EFI_GUID  *Guid
  )
{
  TERMINAL_TYPE  Type;

  for (Type = 0; Type < ARRAY_SIZE (mTerminalType); Type++) {
    if (CompareGuid (Guid, mTerminalType[Type])) {
      break;
    }
  }

  return Type;
}

/**
  Test to see if this driver supports Controller.

  @param  This                Protocol instance pointer.
  @param  Controller          Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval EFI_ALREADY_STARTED This driver is already running on this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
TerminalDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_SERIAL_IO_PROTOCOL    *SerialIo;
  VENDOR_DEVICE_PATH        *Node;

  //
  // If remaining device path is not NULL, then make sure it is a
  // device path that describes a terminal communications protocol.
  //
  if (RemainingDevicePath != NULL) {
    //
    // Check if RemainingDevicePath is the End of Device Path Node,
    // if yes, go on checking other conditions
    //
    if (!IsDevicePathEnd (RemainingDevicePath)) {
      //
      // If RemainingDevicePath isn't the End of Device Path Node,
      // check its validation
      //
      Node = (VENDOR_DEVICE_PATH *)RemainingDevicePath;

      if ((Node->Header.Type != MESSAGING_DEVICE_PATH) ||
          (Node->Header.SubType != MSG_VENDOR_DP) ||
          (DevicePathNodeLength (&Node->Header) != sizeof (VENDOR_DEVICE_PATH)))
      {
        return EFI_UNSUPPORTED;
      }

      //
      // only supports PC ANSI, VT100, VT100+, VT-UTF8, TtyTerm
      // Linux, XtermR6, VT400 and SCO terminal types
      //
      if (TerminalTypeFromGuid (&Node->Guid) == ARRAY_SIZE (mTerminalType)) {
        return EFI_UNSUPPORTED;
      }
    }
  }

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  // The Controller must support the Serial I/O Protocol.
  // This driver is a bus driver with at most 1 child device, so it is
  // ok for it to be already started.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSerialIoProtocolGuid,
                  (VOID **)&SerialIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiSerialIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  //
  // Open the EFI Device Path protocol needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close protocol, don't use device path protocol in the Support() function
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Free notify functions list.

  @param  ListHead               The list head

  @retval EFI_SUCCESS            Free the notify list successfully.
  @retval EFI_INVALID_PARAMETER  ListHead is NULL.

**/
EFI_STATUS
TerminalFreeNotifyList (
  IN OUT LIST_ENTRY  *ListHead
  )
{
  TERMINAL_CONSOLE_IN_EX_NOTIFY  *NotifyNode;

  if (ListHead == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  while (!IsListEmpty (ListHead)) {
    NotifyNode = CR (
                   ListHead->ForwardLink,
                   TERMINAL_CONSOLE_IN_EX_NOTIFY,
                   NotifyEntry,
                   TERMINAL_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                   );
    RemoveEntryList (ListHead->ForwardLink);
    FreePool (NotifyNode);
  }

  return EFI_SUCCESS;
}

/**
  Initialize all the text modes which the terminal console supports.

  It returns information for available text modes that the terminal can support.

  @param[out] TextModeCount      The total number of text modes that terminal console supports.

  @return   The buffer to the text modes column and row information.
            Caller is responsible to free it when it's non-NULL.

**/
TERMINAL_CONSOLE_MODE_DATA *
InitializeTerminalConsoleTextMode (
  OUT INT32  *TextModeCount
  )
{
  TERMINAL_CONSOLE_MODE_DATA  *TextModeData;

  ASSERT (TextModeCount != NULL);

  TextModeData = AllocateCopyPool (sizeof (mTerminalConsoleModeData), mTerminalConsoleModeData);
  if (TextModeData == NULL) {
    return NULL;
  }

  *TextModeCount = ARRAY_SIZE (mTerminalConsoleModeData);

  DEBUG_CODE_BEGIN ();
  INT32  Index;

  for (Index = 0; Index < *TextModeCount; Index++) {
    DEBUG ((
      DEBUG_INFO,
      "Terminal - Mode %d, Column = %d, Row = %d\n",
      Index,
      TextModeData[Index].Columns,
      TextModeData[Index].Rows
      ));
  }

  DEBUG_CODE_END ();
  return TextModeData;
}

/**
  Stop the terminal state machine.

  @param TerminalDevice    The terminal device.
**/
VOID
StopTerminalStateMachine (
  TERMINAL_DEV  *TerminalDevice
  )
{
  EFI_TPL  OriginalTpl;

  OriginalTpl = gBS->RaiseTPL (TPL_NOTIFY);

  gBS->CloseEvent (TerminalDevice->TimerEvent);
  gBS->CloseEvent (TerminalDevice->TwoSecondTimeOut);

  gBS->RestoreTPL (OriginalTpl);
}

/**
  Start the terminal state machine.

  @param TerminalDevice    The terminal device.
**/
VOID
StartTerminalStateMachine (
  TERMINAL_DEV  *TerminalDevice
  )
{
  EFI_STATUS  Status;

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  TerminalConInTimerHandler,
                  TerminalDevice,
                  &TerminalDevice->TimerEvent
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->SetTimer (
                  TerminalDevice->TimerEvent,
                  TimerPeriodic,
                  KEYBOARD_TIMER_INTERVAL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &TerminalDevice->TwoSecondTimeOut
                  );
  ASSERT_EFI_ERROR (Status);
}

/**
  Initialize the controller name table.

  @param TerminalType        The terminal type.
  @param ControllerNameTable The controller name table.

  @retval EFI_SUCCESS  The controller name table is initialized successfully.
  @retval others       Return status of AddUnicodeString2 ().
**/
EFI_STATUS
InitializeControllerNameTable (
  TERMINAL_TYPE             TerminalType,
  EFI_UNICODE_STRING_TABLE  **ControllerNameTable
  )
{
  EFI_STATUS                Status;
  EFI_UNICODE_STRING_TABLE  *Table;

  ASSERT (TerminalType < ARRAY_SIZE (mTerminalType));
  Table  = NULL;
  Status = AddUnicodeString2 (
             "eng",
             gTerminalComponentName.SupportedLanguages,
             &Table,
             mSerialConsoleNames[TerminalType],
             TRUE
             );
  if (!EFI_ERROR (Status)) {
    Status = AddUnicodeString2 (
               "en",
               gTerminalComponentName2.SupportedLanguages,
               &Table,
               mSerialConsoleNames[TerminalType],
               FALSE
               );
    if (EFI_ERROR (Status)) {
      FreeUnicodeStringTable (Table);
    }
  }

  if (!EFI_ERROR (Status)) {
    *ControllerNameTable = Table;
  }

  return Status;
}

/**
  Start this driver on Controller by opening a Serial IO protocol,
  reading Device Path, and creating a child handle with a Simple Text In,
  Simple Text In Ex and Simple Text Out protocol, and device path protocol.
  And store Console Device Environment Variables.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to Controller.
  @retval EFI_ALREADY_STARTED  This driver is already running on Controller.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
TerminalDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                           Status;
  EFI_SERIAL_IO_PROTOCOL               *SerialIo;
  EFI_DEVICE_PATH_PROTOCOL             *ParentDevicePath;
  EFI_DEVICE_PATH_PROTOCOL             *Vendor;
  EFI_HANDLE                           SerialIoHandle;
  EFI_SERIAL_IO_MODE                   *Mode;
  UINTN                                SerialInTimeOut;
  TERMINAL_DEV                         *TerminalDevice;
  UINT8                                TerminalType;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  *OpenInfoBuffer;
  UINTN                                EntryCount;
  UINTN                                Index;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL      *SimpleTextOutput;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL       *SimpleTextInput;
  EFI_UNICODE_STRING_TABLE             *ControllerNameTable;

  //
  // Get the Device Path Protocol to build the device path of the child device
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  ASSERT ((Status == EFI_SUCCESS) || (Status == EFI_ALREADY_STARTED));
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  //
  // Open the Serial I/O Protocol BY_DRIVER.  It might already be started.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSerialIoProtocolGuid,
                  (VOID **)&SerialIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  ASSERT ((Status == EFI_SUCCESS) || (Status == EFI_ALREADY_STARTED));
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  if (!IsHotPlugDevice (ParentDevicePath)) {
    //
    // if the serial device is a hot plug device, do not update the
    // ConInDev, ConOutDev, and StdErrDev variables.
    //
    TerminalUpdateConsoleDevVariable (EFI_CON_IN_DEV_VARIABLE_NAME, ParentDevicePath);
    TerminalUpdateConsoleDevVariable (EFI_CON_OUT_DEV_VARIABLE_NAME, ParentDevicePath);
    TerminalUpdateConsoleDevVariable (EFI_ERR_OUT_DEV_VARIABLE_NAME, ParentDevicePath);
  }

  //
  // Do not create any child for END remaining device path.
  //
  if ((RemainingDevicePath != NULL) && IsDevicePathEnd (RemainingDevicePath)) {
    return EFI_SUCCESS;
  }

  if (Status == EFI_ALREADY_STARTED) {
    if (RemainingDevicePath == NULL) {
      //
      // If RemainingDevicePath is NULL or is the End of Device Path Node
      //
      return EFI_SUCCESS;
    }

    //
    // This driver can only produce one child per serial port.
    // Change its terminal type as remaining device path requests.
    //
    Status = gBS->OpenProtocolInformation (
                    Controller,
                    &gEfiSerialIoProtocolGuid,
                    &OpenInfoBuffer,
                    &EntryCount
                    );
    if (!EFI_ERROR (Status)) {
      Status = EFI_NOT_FOUND;
      for (Index = 0; Index < EntryCount; Index++) {
        if ((OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
          Status = gBS->OpenProtocol (
                          OpenInfoBuffer[Index].ControllerHandle,
                          &gEfiSimpleTextInProtocolGuid,
                          (VOID **)&SimpleTextInput,
                          This->DriverBindingHandle,
                          Controller,
                          EFI_OPEN_PROTOCOL_GET_PROTOCOL
                          );
          if (!EFI_ERROR (Status)) {
            TerminalDevice = TERMINAL_CON_IN_DEV_FROM_THIS (SimpleTextInput);
            TerminalType   = TerminalTypeFromGuid (&((VENDOR_DEVICE_PATH *)RemainingDevicePath)->Guid);
            ASSERT (TerminalType < ARRAY_SIZE (mTerminalType));
            if (TerminalDevice->TerminalType != TerminalType) {
              Status = InitializeControllerNameTable (TerminalType, &ControllerNameTable);
              if (!EFI_ERROR (Status)) {
                StopTerminalStateMachine (TerminalDevice);
                //
                // Update the device path
                //
                Vendor = TerminalDevice->DevicePath;
                Status = gBS->LocateDevicePath (&gEfiSerialIoProtocolGuid, &Vendor, &SerialIoHandle);
                ASSERT_EFI_ERROR (Status);
                CopyGuid (&((VENDOR_DEVICE_PATH *)Vendor)->Guid, mTerminalType[TerminalType]);
                Status = gBS->ReinstallProtocolInterface (
                                TerminalDevice->Handle,
                                &gEfiDevicePathProtocolGuid,
                                TerminalDevice->DevicePath,
                                TerminalDevice->DevicePath
                                );
                if (!EFI_ERROR (Status)) {
                  TerminalDevice->TerminalType = TerminalType;
                  StartTerminalStateMachine (TerminalDevice);
                  FreeUnicodeStringTable (TerminalDevice->ControllerNameTable);
                  TerminalDevice->ControllerNameTable = ControllerNameTable;
                } else {
                  //
                  // Restore the device path on failure
                  //
                  CopyGuid (&((VENDOR_DEVICE_PATH *)Vendor)->Guid, mTerminalType[TerminalDevice->TerminalType]);
                  FreeUnicodeStringTable (ControllerNameTable);
                }
              }
            }
          }

          break;
        }
      }

      FreePool (OpenInfoBuffer);
    }

    return Status;
  }

  //
  // Initialize the Terminal Dev
  //
  TerminalDevice = AllocateCopyPool (sizeof (TERMINAL_DEV), &mTerminalDevTemplate);
  if (TerminalDevice == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto CloseProtocols;
  }

  if (RemainingDevicePath == NULL) {
    //
    // If RemainingDevicePath is NULL, use default terminal type
    //
    TerminalDevice->TerminalType = PcdGet8 (PcdDefaultTerminalType);
  } else {
    //
    // End of Device Path Node is handled in above.
    //
    ASSERT (!IsDevicePathEnd (RemainingDevicePath));
    //
    // If RemainingDevicePath isn't the End of Device Path Node,
    // Use the RemainingDevicePath to determine the terminal type
    //
    TerminalDevice->TerminalType = TerminalTypeFromGuid (&((VENDOR_DEVICE_PATH *)RemainingDevicePath)->Guid);
  }

  ASSERT (TerminalDevice->TerminalType < ARRAY_SIZE (mTerminalType));
  TerminalDevice->SerialIo = SerialIo;

  //
  // Build the component name for the child device
  //
  Status = InitializeControllerNameTable (TerminalDevice->TerminalType, &TerminalDevice->ControllerNameTable);
  if (EFI_ERROR (Status)) {
    goto FreeResources;
  }

  //
  // Build the device path for the child device
  //
  Status = SetTerminalDevicePath (TerminalDevice->TerminalType, ParentDevicePath, &TerminalDevice->DevicePath);
  if (EFI_ERROR (Status)) {
    goto FreeResources;
  }

  InitializeListHead (&TerminalDevice->NotifyList);
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  TerminalConInWaitForKeyEx,
                  TerminalDevice,
                  &TerminalDevice->SimpleInputEx.WaitForKeyEx
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  TerminalConInWaitForKey,
                  TerminalDevice,
                  &TerminalDevice->SimpleInput.WaitForKey
                  );
  ASSERT_EFI_ERROR (Status);
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  KeyNotifyProcessHandler,
                  TerminalDevice,
                  &TerminalDevice->KeyNotifyProcessEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Allocates and initializes the FIFO buffer to be zero, used for accommodating
  // the pre-read pending characters.
  //
  TerminalDevice->RawFiFo = AllocateZeroPool (sizeof (RAW_DATA_FIFO));
  if (TerminalDevice->RawFiFo == NULL) {
    goto FreeResources;
  }

  TerminalDevice->UnicodeFiFo = AllocateZeroPool (sizeof (UNICODE_FIFO));
  if (TerminalDevice->UnicodeFiFo == NULL) {
    goto FreeResources;
  }

  TerminalDevice->EfiKeyFiFo = AllocateZeroPool (sizeof (EFI_KEY_FIFO));
  if (TerminalDevice->EfiKeyFiFo == NULL) {
    goto FreeResources;
  }

  TerminalDevice->EfiKeyFiFoForNotify = AllocateZeroPool (sizeof (EFI_KEY_FIFO));
  if (TerminalDevice->EfiKeyFiFoForNotify == NULL) {
    goto FreeResources;
  }

  //
  // Set the timeout value of serial buffer for keystroke response performance issue
  //
  Mode = TerminalDevice->SerialIo->Mode;

  SerialInTimeOut = 0;
  if (Mode->BaudRate != 0) {
    SerialInTimeOut = (1 + Mode->DataBits + Mode->StopBits) * 2 * 1000000 / (UINTN)Mode->BaudRate;
  }

  Status = TerminalDevice->SerialIo->SetAttributes (
                                       TerminalDevice->SerialIo,
                                       Mode->BaudRate,
                                       Mode->ReceiveFifoDepth,
                                       (UINT32)SerialInTimeOut,
                                       (EFI_PARITY_TYPE)(Mode->Parity),
                                       (UINT8)Mode->DataBits,
                                       (EFI_STOP_BITS_TYPE)(Mode->StopBits)
                                       );
  if (EFI_ERROR (Status)) {
    //
    // if set attributes operation fails, invalidate
    // the value of SerialInTimeOut,thus make it
    // inconsistent with the default timeout value
    // of serial buffer. This will invoke the recalculation
    // in the readkeystroke routine.
    //
    TerminalDevice->SerialInTimeOut = 0;
  } else {
    TerminalDevice->SerialInTimeOut = SerialInTimeOut;
  }

  SimpleTextOutput = &TerminalDevice->SimpleTextOutput;
  SimpleTextInput  = &TerminalDevice->SimpleInput;

  //
  // Initialize SimpleTextOut instance
  //
  SimpleTextOutput->Mode                  = &TerminalDevice->SimpleTextOutputMode;
  TerminalDevice->TerminalConsoleModeData = InitializeTerminalConsoleTextMode (
                                              &SimpleTextOutput->Mode->MaxMode
                                              );
  if (TerminalDevice->TerminalConsoleModeData == NULL) {
    goto FreeResources;
  }

  //
  // For terminal devices, cursor is always visible
  //
  SimpleTextOutput->Mode->CursorVisible = TRUE;
  Status                                = SimpleTextOutput->SetAttribute (SimpleTextOutput, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
  if (!EFI_ERROR (Status)) {
    Status = SimpleTextOutput->Reset (SimpleTextOutput, FALSE);
  }

  if (EFI_ERROR (Status)) {
    goto ReportError;
  }

  //
  // Initialize SimpleTextInput instance
  //
  Status = SimpleTextInput->Reset (SimpleTextInput, FALSE);
  if (EFI_ERROR (Status)) {
    goto ReportError;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &TerminalDevice->Handle,
                  &gEfiSimpleTextInProtocolGuid,
                  &TerminalDevice->SimpleInput,
                  &gEfiSimpleTextInputExProtocolGuid,
                  &TerminalDevice->SimpleInputEx,
                  &gEfiSimpleTextOutProtocolGuid,
                  &TerminalDevice->SimpleTextOutput,
                  &gEfiDevicePathProtocolGuid,
                  TerminalDevice->DevicePath,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiSerialIoProtocolGuid,
                    (VOID **)&TerminalDevice->SerialIo,
                    This->DriverBindingHandle,
                    TerminalDevice->Handle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
    ASSERT_EFI_ERROR (Status);
    StartTerminalStateMachine (TerminalDevice);
    return Status;
  }

ReportError:
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_ERROR_CODE | EFI_ERROR_MINOR,
    (EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_CONTROLLER_ERROR),
    ParentDevicePath
    );

FreeResources:
  ASSERT (TerminalDevice != NULL);

  if (TerminalDevice->SimpleInput.WaitForKey != NULL) {
    gBS->CloseEvent (TerminalDevice->SimpleInput.WaitForKey);
  }

  if (TerminalDevice->SimpleInputEx.WaitForKeyEx != NULL) {
    gBS->CloseEvent (TerminalDevice->SimpleInputEx.WaitForKeyEx);
  }

  if (TerminalDevice->KeyNotifyProcessEvent != NULL) {
    gBS->CloseEvent (TerminalDevice->KeyNotifyProcessEvent);
  }

  if (TerminalDevice->RawFiFo != NULL) {
    FreePool (TerminalDevice->RawFiFo);
  }

  if (TerminalDevice->UnicodeFiFo != NULL) {
    FreePool (TerminalDevice->UnicodeFiFo);
  }

  if (TerminalDevice->EfiKeyFiFo != NULL) {
    FreePool (TerminalDevice->EfiKeyFiFo);
  }

  if (TerminalDevice->EfiKeyFiFoForNotify != NULL) {
    FreePool (TerminalDevice->EfiKeyFiFoForNotify);
  }

  if (TerminalDevice->ControllerNameTable != NULL) {
    FreeUnicodeStringTable (TerminalDevice->ControllerNameTable);
  }

  if (TerminalDevice->DevicePath != NULL) {
    FreePool (TerminalDevice->DevicePath);
  }

  if (TerminalDevice->TerminalConsoleModeData != NULL) {
    FreePool (TerminalDevice->TerminalConsoleModeData);
  }

  FreePool (TerminalDevice);

CloseProtocols:

  //
  // Remove Parent Device Path from
  // the Console Device Environment Variables
  //
  TerminalRemoveConsoleDevVariable (EFI_CON_IN_DEV_VARIABLE_NAME, ParentDevicePath);
  TerminalRemoveConsoleDevVariable (EFI_CON_OUT_DEV_VARIABLE_NAME, ParentDevicePath);
  TerminalRemoveConsoleDevVariable (EFI_ERR_OUT_DEV_VARIABLE_NAME, ParentDevicePath);

  Status = gBS->CloseProtocol (
                  Controller,
                  &gEfiSerialIoProtocolGuid,
                  This->DriverBindingHandle,
                  Controller
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CloseProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  This->DriverBindingHandle,
                  Controller
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Stop this driver on Controller by closing Simple Text In, Simple Text
  In Ex, Simple Text Out protocol, and removing parent device path from
  Console Device Environment Variables.

  @param  This              Protocol instance pointer.
  @param  Controller        Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed Controller.
  @retval other             This driver could not be removed from this device.

**/
EFI_STATUS
EFIAPI
TerminalDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                       Status;
  UINTN                            Index;
  BOOLEAN                          AllChildrenStopped;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *SimpleTextOutput;
  TERMINAL_DEV                     *TerminalDevice;
  EFI_DEVICE_PATH_PROTOCOL         *ParentDevicePath;
  EFI_SERIAL_IO_PROTOCOL           *SerialIo;

  //
  // Complete all outstanding transactions to Controller.
  // Don't allow any new transaction to Controller to be started.
  //
  if (NumberOfChildren == 0) {
    //
    // Close the bus driver
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&ParentDevicePath,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // Remove Parent Device Path from
    // the Console Device Environment Variables
    //
    TerminalRemoveConsoleDevVariable (EFI_CON_IN_DEV_VARIABLE_NAME, ParentDevicePath);
    TerminalRemoveConsoleDevVariable (EFI_CON_OUT_DEV_VARIABLE_NAME, ParentDevicePath);
    TerminalRemoveConsoleDevVariable (EFI_ERR_OUT_DEV_VARIABLE_NAME, ParentDevicePath);

    gBS->CloseProtocol (
           Controller,
           &gEfiSerialIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    return EFI_SUCCESS;
  }

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {
    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[Index],
                    &gEfiSimpleTextOutProtocolGuid,
                    (VOID **)&SimpleTextOutput,
                    This->DriverBindingHandle,
                    ChildHandleBuffer[Index],
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      TerminalDevice = TERMINAL_CON_OUT_DEV_FROM_THIS (SimpleTextOutput);

      gBS->CloseProtocol (
             Controller,
             &gEfiSerialIoProtocolGuid,
             This->DriverBindingHandle,
             ChildHandleBuffer[Index]
             );

      Status = gBS->UninstallMultipleProtocolInterfaces (
                      ChildHandleBuffer[Index],
                      &gEfiSimpleTextInProtocolGuid,
                      &TerminalDevice->SimpleInput,
                      &gEfiSimpleTextInputExProtocolGuid,
                      &TerminalDevice->SimpleInputEx,
                      &gEfiSimpleTextOutProtocolGuid,
                      &TerminalDevice->SimpleTextOutput,
                      &gEfiDevicePathProtocolGuid,
                      TerminalDevice->DevicePath,
                      NULL
                      );
      if (EFI_ERROR (Status)) {
        gBS->OpenProtocol (
               Controller,
               &gEfiSerialIoProtocolGuid,
               (VOID **)&SerialIo,
               This->DriverBindingHandle,
               ChildHandleBuffer[Index],
               EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
               );
      } else {
        FreeUnicodeStringTable (TerminalDevice->ControllerNameTable);
        StopTerminalStateMachine (TerminalDevice);
        gBS->CloseEvent (TerminalDevice->SimpleInput.WaitForKey);
        gBS->CloseEvent (TerminalDevice->SimpleInputEx.WaitForKeyEx);
        gBS->CloseEvent (TerminalDevice->KeyNotifyProcessEvent);
        TerminalFreeNotifyList (&TerminalDevice->NotifyList);
        FreePool (TerminalDevice->DevicePath);
        FreePool (TerminalDevice->TerminalConsoleModeData);
        FreePool (TerminalDevice);
      }
    }

    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Compare a device path data structure to that of all the nodes of a
  second device path instance.

  @param  Multi          A pointer to a multi-instance device path data structure.
  @param  Single         A pointer to a single-instance device path data structure.

  @retval TRUE           If the Single is contained within Multi.
  @retval FALSE          The Single is not match within Multi.

**/
BOOLEAN
MatchDevicePaths (
  IN  EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN  EFI_DEVICE_PATH_PROTOCOL  *Single
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathInst;
  UINTN                     Size;

  DevicePath     = Multi;
  DevicePathInst = GetNextDevicePathInstance (&DevicePath, &Size);
  //
  // Search for the match of 'Single' in 'Multi'
  //
  while (DevicePathInst != NULL) {
    //
    // If the single device path is found in multiple device paths,
    // return success
    //
    if (CompareMem (Single, DevicePathInst, Size) == 0) {
      FreePool (DevicePathInst);
      return TRUE;
    }

    FreePool (DevicePathInst);
    DevicePathInst = GetNextDevicePathInstance (&DevicePath, &Size);
  }

  return FALSE;
}

/**
  Update terminal device path in Console Device Environment Variables.

  @param  VariableName           The Console Device Environment Variable.
  @param  ParentDevicePath       The terminal device path to be updated.

**/
VOID
TerminalUpdateConsoleDevVariable (
  IN CHAR16                    *VariableName,
  IN EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath
  )
{
  EFI_STATUS                 Status;
  UINTN                      NameSize;
  UINTN                      VariableSize;
  TERMINAL_TYPE              TerminalType;
  EFI_DEVICE_PATH_PROTOCOL   *Variable;
  EFI_DEVICE_PATH_PROTOCOL   *NewVariable;
  EFI_DEVICE_PATH_PROTOCOL   *TempDevicePath;
  EDKII_SET_VARIABLE_STATUS  *SetVariableStatus;

  //
  // Get global variable and its size according to the name given.
  //
  Status = GetEfiGlobalVariable2 (VariableName, (VOID **)&Variable, NULL);
  if (Status == EFI_NOT_FOUND) {
    Status   = EFI_SUCCESS;
    Variable = NULL;
  }

  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Append terminal device path onto the variable.
  //
  for (TerminalType = 0; TerminalType < ARRAY_SIZE (mTerminalType); TerminalType++) {
    SetTerminalDevicePath (TerminalType, ParentDevicePath, &TempDevicePath);

    if (TempDevicePath != NULL) {
      if (!MatchDevicePaths (Variable, TempDevicePath)) {
        NewVariable = AppendDevicePathInstance (Variable, TempDevicePath);
        if (NewVariable != NULL) {
          if (Variable != NULL) {
            FreePool (Variable);
          }

          Variable = NewVariable;
        }
      }

      FreePool (TempDevicePath);
    }
  }

  VariableSize = GetDevicePathSize (Variable);

  Status = gRT->SetVariable (
                  VariableName,
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  VariableSize,
                  Variable
                  );

  if (EFI_ERROR (Status)) {
    NameSize          = StrSize (VariableName);
    SetVariableStatus = AllocatePool (sizeof (EDKII_SET_VARIABLE_STATUS) + NameSize + VariableSize);
    if (SetVariableStatus != NULL) {
      CopyGuid (&SetVariableStatus->Guid, &gEfiGlobalVariableGuid);
      SetVariableStatus->NameSize   = NameSize;
      SetVariableStatus->DataSize   = VariableSize;
      SetVariableStatus->SetStatus  = Status;
      SetVariableStatus->Attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
      CopyMem (SetVariableStatus + 1, VariableName, NameSize);
      CopyMem (((UINT8 *)(SetVariableStatus + 1)) + NameSize, Variable, VariableSize);

      REPORT_STATUS_CODE_EX (
        EFI_ERROR_CODE,
        PcdGet32 (PcdErrorCodeSetVariable),
        0,
        NULL,
        &gEdkiiStatusCodeDataTypeVariableGuid,
        SetVariableStatus,
        sizeof (EDKII_SET_VARIABLE_STATUS) + NameSize + VariableSize
        );

      FreePool (SetVariableStatus);
    }
  }

  FreePool (Variable);

  return;
}

/**
  Remove terminal device path from Console Device Environment Variables.

  @param  VariableName           Console Device Environment Variables.
  @param  ParentDevicePath       The terminal device path to be updated.

**/
VOID
TerminalRemoveConsoleDevVariable (
  IN CHAR16                    *VariableName,
  IN EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath
  )
{
  EFI_STATUS                Status;
  BOOLEAN                   FoundOne;
  BOOLEAN                   Match;
  UINTN                     VariableSize;
  UINTN                     InstanceSize;
  TERMINAL_TYPE             TerminalType;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  EFI_DEVICE_PATH_PROTOCOL  *Variable;
  EFI_DEVICE_PATH_PROTOCOL  *OriginalVariable;
  EFI_DEVICE_PATH_PROTOCOL  *NewVariable;
  EFI_DEVICE_PATH_PROTOCOL  *SavedNewVariable;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;

  Instance = NULL;

  //
  // Get global variable and its size according to the name given.
  //
  GetEfiGlobalVariable2 (VariableName, (VOID **)&Variable, NULL);
  if (Variable == NULL) {
    return;
  }

  FoundOne         = FALSE;
  OriginalVariable = Variable;
  NewVariable      = NULL;

  //
  // Get first device path instance from Variable
  //
  Instance = GetNextDevicePathInstance (&Variable, &InstanceSize);
  if (Instance == NULL) {
    FreePool (OriginalVariable);
    return;
  }

  //
  // Loop through all the device path instances of Variable
  //
  do {
    //
    // Loop through all the terminal types that this driver supports
    //
    Match = FALSE;
    for (TerminalType = 0; TerminalType < ARRAY_SIZE (mTerminalType); TerminalType++) {
      SetTerminalDevicePath (TerminalType, ParentDevicePath, &TempDevicePath);

      //
      // Compare the generated device path to the current device path instance
      //
      if (TempDevicePath != NULL) {
        if (CompareMem (Instance, TempDevicePath, InstanceSize) == 0) {
          Match    = TRUE;
          FoundOne = TRUE;
        }

        FreePool (TempDevicePath);
      }
    }

    //
    // If a match was not found, then keep the current device path instance
    //
    if (!Match) {
      SavedNewVariable = NewVariable;
      NewVariable      = AppendDevicePathInstance (NewVariable, Instance);
      if (SavedNewVariable != NULL) {
        FreePool (SavedNewVariable);
      }
    }

    //
    // Get next device path instance from Variable
    //
    FreePool (Instance);
    Instance = GetNextDevicePathInstance (&Variable, &InstanceSize);
  } while (Instance != NULL);

  FreePool (OriginalVariable);

  if (FoundOne) {
    if (NewVariable != NULL) {
      VariableSize = GetDevicePathSize (NewVariable);

      Status = gRT->SetVariable (
                      VariableName,
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      VariableSize,
                      NewVariable
                      );
      //
      // Shrinking variable with existing variable driver implementation shouldn't fail.
      //
      ASSERT_EFI_ERROR (Status);
    }
  }

  if (NewVariable != NULL) {
    FreePool (NewVariable);
  }

  return;
}

/**
  Build terminal device path according to terminal type.

  @param  TerminalType           The terminal type is PC ANSI, VT100, VT100+ or VT-UTF8.
  @param  ParentDevicePath       Parent device path.
  @param  TerminalDevicePath     Returned terminal device path, if building successfully.

  @retval EFI_UNSUPPORTED        Terminal does not belong to the supported type.
  @retval EFI_OUT_OF_RESOURCES   Generate terminal device path failed.
  @retval EFI_SUCCESS            Build terminal device path successfully.

**/
EFI_STATUS
SetTerminalDevicePath (
  IN  TERMINAL_TYPE             TerminalType,
  IN  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL  **TerminalDevicePath
  )
{
  VENDOR_DEVICE_PATH  Node;

  ASSERT (TerminalType < ARRAY_SIZE (mTerminalType));
  Node.Header.Type    = MESSAGING_DEVICE_PATH;
  Node.Header.SubType = MSG_VENDOR_DP;
  SetDevicePathNodeLength (&Node.Header, sizeof (VENDOR_DEVICE_PATH));
  CopyGuid (&Node.Guid, mTerminalType[TerminalType]);

  //
  // Append the terminal node onto parent device path
  // to generate a complete terminal device path.
  //
  *TerminalDevicePath = AppendDevicePathNode (
                          ParentDevicePath,
                          (EFI_DEVICE_PATH_PROTOCOL *)&Node
                          );
  if (*TerminalDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  The user Entry Point for module Terminal. The user code starts with this function.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeTerminal (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gTerminalDriverBinding,
             ImageHandle,
             &gTerminalComponentName,
             &gTerminalComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Check if the device supports hot-plug through its device path.

  This function could be updated to check more types of Hot Plug devices.
  Currently, it checks USB and PCCard device.

  @param  DevicePath            Pointer to device's device path.

  @retval TRUE                  The devcie is a hot-plug device
  @retval FALSE                 The devcie is not a hot-plug device.

**/
BOOLEAN
IsHotPlugDevice (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *CheckDevicePath;

  CheckDevicePath = DevicePath;
  while (!IsDevicePathEnd (CheckDevicePath)) {
    //
    // Check device whether is hot plug device or not throught Device Path
    //
    if ((DevicePathType (CheckDevicePath) == MESSAGING_DEVICE_PATH) &&
        ((DevicePathSubType (CheckDevicePath) == MSG_USB_DP) ||
         (DevicePathSubType (CheckDevicePath) == MSG_USB_CLASS_DP) ||
         (DevicePathSubType (CheckDevicePath) == MSG_USB_WWID_DP)))
    {
      //
      // If Device is USB device
      //
      return TRUE;
    }

    if ((DevicePathType (CheckDevicePath) == HARDWARE_DEVICE_PATH) &&
        (DevicePathSubType (CheckDevicePath) == HW_PCCARD_DP))
    {
      //
      // If Device is PCCard
      //
      return TRUE;
    }

    CheckDevicePath = NextDevicePathNode (CheckDevicePath);
  }

  return FALSE;
}
