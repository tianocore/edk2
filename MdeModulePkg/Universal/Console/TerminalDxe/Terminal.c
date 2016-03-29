/** @file
  Produces Simple Text Input Protocol, Simple Text Input Extended Protocol and
  Simple Text Output Protocol upon Serial IO Protocol.

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "Terminal.h"

//
// Globals
//
EFI_DRIVER_BINDING_PROTOCOL gTerminalDriverBinding = {
  TerminalDriverBindingSupported,
  TerminalDriverBindingStart,
  TerminalDriverBindingStop,
  0xa,
  NULL,
  NULL
};


EFI_GUID  *gTerminalType[] = {
  &gEfiPcAnsiGuid,
  &gEfiVT100Guid,
  &gEfiVT100PlusGuid,
  &gEfiVTUTF8Guid,
  &gEfiTtyTermGuid
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
  {   // SimpleTextOutputMode
    1,                                           // MaxMode
    0,                                           // Mode
    EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK),    // Attribute
    0,                                           // CursorColumn
    0,                                           // CursorRow
    TRUE                                         // CursorVisible
  },
  NULL, // TerminalConsoleModeData
  0,  // SerialInTimeOut

  NULL, // RawFifo
  NULL, // UnicodeFiFo
  NULL, // EfiKeyFiFo

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
  }
};

TERMINAL_CONSOLE_MODE_DATA mTerminalConsoleModeData[] = {
  {100, 31},
  //
  // New modes can be added here.
  //
};

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
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
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
      Node = (VENDOR_DEVICE_PATH *) RemainingDevicePath;

      if (Node->Header.Type != MESSAGING_DEVICE_PATH ||
          Node->Header.SubType != MSG_VENDOR_DP ||
          DevicePathNodeLength(&Node->Header) != sizeof(VENDOR_DEVICE_PATH)) {

        return EFI_UNSUPPORTED;

      }
      //
      // only supports PC ANSI, VT100, VT100+, VT-UTF8, and TtyTerm terminal types
      //
      if (!CompareGuid (&Node->Guid, &gEfiPcAnsiGuid) &&
          !CompareGuid (&Node->Guid, &gEfiVT100Guid) &&
          !CompareGuid (&Node->Guid, &gEfiVT100PlusGuid) &&
          !CompareGuid (&Node->Guid, &gEfiVTUTF8Guid) &&
          !CompareGuid (&Node->Guid, &gEfiTtyTermGuid)) {

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
                  (VOID **) &SerialIo,
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
                  (VOID **) &ParentDevicePath,
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
  Build the terminal device path for the child device according to the
  terminal type.

  @param  ParentDevicePath         Parent device path.
  @param  RemainingDevicePath      A specific child device.

  @return The child device path built.

**/
EFI_DEVICE_PATH_PROTOCOL*
EFIAPI
BuildTerminalDevpath  (
  IN EFI_DEVICE_PATH_PROTOCOL       *ParentDevicePath,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL          *TerminalDevicePath;
  UINT8                             TerminalType;
  VENDOR_DEVICE_PATH                *Node;
  EFI_STATUS                        Status;

  TerminalDevicePath = NULL;

  //
  // Use the RemainingDevicePath to determine the terminal type
  //
  Node = (VENDOR_DEVICE_PATH *) RemainingDevicePath;
  if (Node == NULL) {
    TerminalType = PcdGet8 (PcdDefaultTerminalType);

  } else if (CompareGuid (&Node->Guid, &gEfiPcAnsiGuid)) {

    TerminalType = PCANSITYPE;

  } else if (CompareGuid (&Node->Guid, &gEfiVT100Guid)) {

    TerminalType = VT100TYPE;

  } else if (CompareGuid (&Node->Guid, &gEfiVT100PlusGuid)) {

    TerminalType = VT100PLUSTYPE;

  } else if (CompareGuid (&Node->Guid, &gEfiVTUTF8Guid)) {

    TerminalType = VTUTF8TYPE;

  } else if (CompareGuid (&Node->Guid, &gEfiTtyTermGuid)) {

    TerminalType = TTYTERMTYPE;

  } else {
    return NULL;
  }

  //
  // Build the device path for the child device
  //
  Status = SetTerminalDevicePath (
            TerminalType,
            ParentDevicePath,
            &TerminalDevicePath
            );
  if (EFI_ERROR (Status)) {
    return NULL;
  }
  return TerminalDevicePath;
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

  DevicePath      = Multi;
  DevicePathInst  = GetNextDevicePathInstance (&DevicePath, &Size);
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
  Check whether the terminal device path is in the global variable.

  @param  VariableName          Pointer to one global variable.
  @param  TerminalDevicePath    Pointer to the terminal device's device path.

  @retval TRUE                  The devcie is in the global variable.
  @retval FALSE                 The devcie is not in the global variable.

**/
BOOLEAN
IsTerminalInConsoleVariable (
  IN CHAR16                    *VariableName,
  IN EFI_DEVICE_PATH_PROTOCOL  *TerminalDevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Variable;
  BOOLEAN                   ReturnFlag;

  //
  // Get global variable and its size according to the name given.
  //
  GetEfiGlobalVariable2 (VariableName, (VOID**)&Variable, NULL);
  if (Variable == NULL) {
    return FALSE;
  }

  //
  // Check whether the terminal device path is one of the variable instances.
  //
  ReturnFlag = MatchDevicePaths (Variable, TerminalDevicePath);

  FreePool (Variable);

  return ReturnFlag;
}

/**
  Free notify functions list.

  @param  ListHead               The list head

  @retval EFI_SUCCESS            Free the notify list successfully.
  @retval EFI_INVALID_PARAMETER  ListHead is NULL.

**/
EFI_STATUS
TerminalFreeNotifyList (
  IN OUT LIST_ENTRY           *ListHead
  )
{
  TERMINAL_CONSOLE_IN_EX_NOTIFY *NotifyNode;

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
  @param[out] TextModeData       The buffer to the text modes column and row information.
                                 Caller is responsible to free it when it's non-NULL.

  @retval EFI_SUCCESS            The supporting mode information is returned.
  @retval EFI_INVALID_PARAMETER  The parameters are invalid.

**/
EFI_STATUS
InitializeTerminalConsoleTextMode (
  OUT UINTN                         *TextModeCount,
  OUT TERMINAL_CONSOLE_MODE_DATA    **TextModeData
  )
{
  UINTN                       Index;
  UINTN                       Count;
  TERMINAL_CONSOLE_MODE_DATA  *ModeBuffer;
  TERMINAL_CONSOLE_MODE_DATA  *NewModeBuffer;
  UINTN                       ValidCount;
  UINTN                       ValidIndex;
  
  if ((TextModeCount == NULL) || (TextModeData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  Count = sizeof (mTerminalConsoleModeData) / sizeof (TERMINAL_CONSOLE_MODE_DATA);
  
  //
  // Get defined mode buffer pointer.
  //
  ModeBuffer = mTerminalConsoleModeData;
    
  //
  // Here we make sure that the final mode exposed does not include the duplicated modes,
  // and does not include the invalid modes which exceed the max column and row.
  // Reserve 2 modes for 80x25, 80x50 of terminal console.
  //
  NewModeBuffer = AllocateZeroPool (sizeof (TERMINAL_CONSOLE_MODE_DATA) * (Count + 2));
  ASSERT (NewModeBuffer != NULL);

  //
  // Mode 0 and mode 1 is for 80x25, 80x50 according to UEFI spec.
  //
  ValidCount = 0;  

  NewModeBuffer[ValidCount].Columns = 80;
  NewModeBuffer[ValidCount].Rows    = 25;
  ValidCount++;

  NewModeBuffer[ValidCount].Columns = 80;
  NewModeBuffer[ValidCount].Rows    = 50;
  ValidCount++;
  
  //
  // Start from mode 2 to put the valid mode other than 80x25 and 80x50 in the output mode buffer.
  //
  for (Index = 0; Index < Count; Index++) {
    if ((ModeBuffer[Index].Columns == 0) || (ModeBuffer[Index].Rows == 0)) {
      //
      // Skip the pre-defined mode which is invalid.
      //
      continue;
    }
    for (ValidIndex = 0; ValidIndex < ValidCount; ValidIndex++) {
      if ((ModeBuffer[Index].Columns == NewModeBuffer[ValidIndex].Columns) &&
          (ModeBuffer[Index].Rows == NewModeBuffer[ValidIndex].Rows)) {
        //
        // Skip the duplicated mode.
        //
        break;
      }
    }
    if (ValidIndex == ValidCount) {
      NewModeBuffer[ValidCount].Columns = ModeBuffer[Index].Columns;
      NewModeBuffer[ValidCount].Rows    = ModeBuffer[Index].Rows;
      ValidCount++;
    }
  }
 
  DEBUG_CODE (
    for (Index = 0; Index < ValidCount; Index++) {
      DEBUG ((EFI_D_INFO, "Terminal - Mode %d, Column = %d, Row = %d\n", 
                           Index, NewModeBuffer[Index].Columns, NewModeBuffer[Index].Rows));  
    }
  );
  
  //
  // Return valid mode count and mode information buffer.
  //
  *TextModeCount = ValidCount;
  *TextModeData  = NewModeBuffer;
  return EFI_SUCCESS;
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
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                          Status;
  EFI_SERIAL_IO_PROTOCOL              *SerialIo;
  EFI_DEVICE_PATH_PROTOCOL            *ParentDevicePath;
  VENDOR_DEVICE_PATH                  *Node;
  EFI_SERIAL_IO_MODE                  *Mode;
  UINTN                               SerialInTimeOut;
  TERMINAL_DEV                        *TerminalDevice;
  UINT8                               TerminalType;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *OpenInfoBuffer;
  UINTN                               EntryCount;
  UINTN                               Index;
  EFI_DEVICE_PATH_PROTOCOL            *DevicePath;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL     *SimpleTextOutput;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL      *SimpleTextInput;
  BOOLEAN                             ConInSelected;
  BOOLEAN                             ConOutSelected;
  BOOLEAN                             NullRemaining;
  BOOLEAN                             SimTxtInInstalled;
  BOOLEAN                             SimTxtOutInstalled;
  BOOLEAN                             FirstEnter;
  UINTN                               ModeCount;

  TerminalDevice     = NULL;
  ConInSelected      = FALSE;
  ConOutSelected     = FALSE;
  NullRemaining      = FALSE;
  SimTxtInInstalled  = FALSE;
  SimTxtOutInstalled = FALSE;
  FirstEnter         = FALSE;
  //
  // Get the Device Path Protocol to build the device path of the child device
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    return Status;
  }

  //
  // Open the Serial I/O Protocol BY_DRIVER.  It might already be started.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSerialIoProtocolGuid,
                  (VOID **) &SerialIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    return Status;
  }

  if (Status != EFI_ALREADY_STARTED) {
    //
    // the serial I/O protocol never be opened before, it is the first
    // time to start the serial Io controller
    //
    FirstEnter = TRUE;
  }

  //
  // Serial I/O is not already open by this driver, then tag the handle
  // with the Terminal Driver GUID and update the ConInDev, ConOutDev, and
  // StdErrDev variables with the list of possible terminal types on this
  // serial port.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiCallerIdGuid,
                  NULL,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Controller,
                    &gEfiCallerIdGuid,
                    DuplicateDevicePath (ParentDevicePath),
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    if (!IsHotPlugDevice (ParentDevicePath)) {
      //
      // if the serial device is a hot plug device, do not update the
      // ConInDev, ConOutDev, and StdErrDev variables.
      //
      TerminalUpdateConsoleDevVariable (L"ConInDev", ParentDevicePath);
      TerminalUpdateConsoleDevVariable (L"ConOutDev", ParentDevicePath);
      TerminalUpdateConsoleDevVariable (L"ErrOutDev", ParentDevicePath);
    }
  }

  //
  // Check the requirement for the SimpleTxtIn and SimpleTxtOut protocols
  //
  // Simple In/Out Protocol will not be installed onto the handle if the
  // device path to the handle is not present in the ConIn/ConOut
  // environment variable. But If RemainingDevicePath is NULL, then always
  // produce both Simple In and Simple Text Output Protocols. This is required
  // for the connect all sequences to make sure all possible consoles are
  // produced no matter what the current values of ConIn, ConOut, or StdErr are.
  //
  if (RemainingDevicePath == NULL) {
    NullRemaining = TRUE;
  }

  DevicePath = BuildTerminalDevpath (ParentDevicePath, RemainingDevicePath);
  if (DevicePath != NULL) {
    ConInSelected  = IsTerminalInConsoleVariable (L"ConIn", DevicePath);
    ConOutSelected = IsTerminalInConsoleVariable (L"ConOut", DevicePath);
    FreePool (DevicePath);
  } else {
    goto Error;
  }
  //
  // Not create the child terminal handle if both Simple In/In Ex and
  // Simple text Out protocols are not required to be published
  //
  if ((!ConInSelected)&&(!ConOutSelected)&&(!NullRemaining)) {
    goto Error;
  }

  //
  // create the child terminal handle during first entry
  //
  if (FirstEnter) {
    //
    // First enther the start funciton
    //
    FirstEnter = FALSE;
    //
    // Make sure a child handle does not already exist.  This driver can only
    // produce one child per serial port.
    //
    Status = gBS->OpenProtocolInformation (
                    Controller,
                    &gEfiSerialIoProtocolGuid,
                    &OpenInfoBuffer,
                    &EntryCount
                    );
    if (!EFI_ERROR (Status)) {
      Status = EFI_SUCCESS;
      for (Index = 0; Index < EntryCount; Index++) {
        if ((OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
          Status = EFI_ALREADY_STARTED;
        }
      }

      FreePool (OpenInfoBuffer);
      if (EFI_ERROR (Status)) {
          goto Error;
      }
    }

    //
    // If RemainingDevicePath is NULL, use default terminal type
    //
    if (RemainingDevicePath == NULL) {
      TerminalType = PcdGet8 (PcdDefaultTerminalType);
      //
      // Must be between PCANSITYPE (0) and TTYTERMTYPE (4)
      //
      ASSERT (TerminalType <= TTYTERMTYPE);
    } else if (!IsDevicePathEnd (RemainingDevicePath)) {
      //
      // If RemainingDevicePath isn't the End of Device Path Node,
      // Use the RemainingDevicePath to determine the terminal type
      //
      Node = (VENDOR_DEVICE_PATH *)RemainingDevicePath;
      if (CompareGuid (&Node->Guid, &gEfiPcAnsiGuid)) {
        TerminalType = PCANSITYPE;
      } else if (CompareGuid (&Node->Guid, &gEfiVT100Guid)) {
        TerminalType = VT100TYPE;
      } else if (CompareGuid (&Node->Guid, &gEfiVT100PlusGuid)) {
        TerminalType = VT100PLUSTYPE;
      } else if (CompareGuid (&Node->Guid, &gEfiVTUTF8Guid)) {
        TerminalType = VTUTF8TYPE;
      } else if (CompareGuid (&Node->Guid, &gEfiTtyTermGuid)) {
        TerminalType = TTYTERMTYPE;
      } else {
        goto Error;
      }
    } else {
      //
      // If RemainingDevicePath is the End of Device Path Node,
      // skip enumerate any device and return EFI_SUCESSS
      //
      return EFI_SUCCESS;
    }

    //
    // Initialize the Terminal Dev
    //
    TerminalDevice = AllocateCopyPool (sizeof (TERMINAL_DEV), &mTerminalDevTemplate);
    if (TerminalDevice == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Error;
    }

    TerminalDevice->TerminalType  = TerminalType;
    TerminalDevice->SerialIo      = SerialIo;

    InitializeListHead (&TerminalDevice->NotifyList);
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_WAIT,
                    TPL_NOTIFY,
                    TerminalConInWaitForKeyEx,
                    TerminalDevice,
                    &TerminalDevice->SimpleInputEx.WaitForKeyEx
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    Status = gBS->CreateEvent (
                    EVT_NOTIFY_WAIT,
                    TPL_NOTIFY,
                    TerminalConInWaitForKey,
                    TerminalDevice,
                    &TerminalDevice->SimpleInput.WaitForKey
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
    //
    // Allocates and initializes the FIFO buffer to be zero, used for accommodating
    // the pre-read pending characters.
    //
    TerminalDevice->RawFiFo     = AllocateZeroPool (sizeof (RAW_DATA_FIFO));
    if (TerminalDevice->RawFiFo == NULL) {
      goto Error;
    }
    TerminalDevice->UnicodeFiFo = AllocateZeroPool (sizeof (UNICODE_FIFO));
    if (TerminalDevice->UnicodeFiFo == NULL) {
      goto Error;
    }
    TerminalDevice->EfiKeyFiFo  = AllocateZeroPool (sizeof (EFI_KEY_FIFO));
    if (TerminalDevice->EfiKeyFiFo == NULL) {
      goto Error;
    }

    //
    // Set the timeout value of serial buffer for
    // keystroke response performance issue
    //
    Mode            = TerminalDevice->SerialIo->Mode;

    SerialInTimeOut = 0;
    if (Mode->BaudRate != 0) {
      SerialInTimeOut = (1 + Mode->DataBits + Mode->StopBits) * 2 * 1000000 / (UINTN) Mode->BaudRate;
    }

    Status = TerminalDevice->SerialIo->SetAttributes (
                                        TerminalDevice->SerialIo,
                                        Mode->BaudRate,
                                        Mode->ReceiveFifoDepth,
                                        (UINT32) SerialInTimeOut,
                                        (EFI_PARITY_TYPE) (Mode->Parity),
                                        (UINT8) Mode->DataBits,
                                        (EFI_STOP_BITS_TYPE) (Mode->StopBits)
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
    //
    // Set Simple Text Output Protocol from template.
    //
    SimpleTextOutput = CopyMem (
                         &TerminalDevice->SimpleTextOutput,
                         &mTerminalDevTemplate.SimpleTextOutput,
                         sizeof (mTerminalDevTemplate.SimpleTextOutput)
                         );
    SimpleTextOutput->Mode = &TerminalDevice->SimpleTextOutputMode;
    
    Status = InitializeTerminalConsoleTextMode (&ModeCount, &TerminalDevice->TerminalConsoleModeData);
    if (EFI_ERROR (Status)) {
      goto ReportError;
    }
    TerminalDevice->SimpleTextOutputMode.MaxMode = (INT32) ModeCount;
    
    //
    // For terminal devices, cursor is always visible
    //
    TerminalDevice->SimpleTextOutputMode.CursorVisible = TRUE;
    Status = TerminalConOutSetAttribute (
               SimpleTextOutput,
               EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK)
               );
    if (EFI_ERROR (Status)) {
      goto ReportError;
    }

    //
    // Build the component name for the child device
    //
    TerminalDevice->ControllerNameTable = NULL;
    switch (TerminalDevice->TerminalType) {
    case PCANSITYPE:
      AddUnicodeString2 (
        "eng",
        gTerminalComponentName.SupportedLanguages,
        &TerminalDevice->ControllerNameTable,
        (CHAR16 *)L"PC-ANSI Serial Console",
        TRUE
        );
      AddUnicodeString2 (
        "en",
        gTerminalComponentName2.SupportedLanguages,
        &TerminalDevice->ControllerNameTable,
        (CHAR16 *)L"PC-ANSI Serial Console",
        FALSE
        );

      break;

    case VT100TYPE:
      AddUnicodeString2 (
        "eng",
        gTerminalComponentName.SupportedLanguages,
        &TerminalDevice->ControllerNameTable,
        (CHAR16 *)L"VT-100 Serial Console",
        TRUE
        );
      AddUnicodeString2 (
        "en",
        gTerminalComponentName2.SupportedLanguages,
        &TerminalDevice->ControllerNameTable,
        (CHAR16 *)L"VT-100 Serial Console",
        FALSE
        );

      break;

    case VT100PLUSTYPE:
      AddUnicodeString2 (
        "eng",
        gTerminalComponentName.SupportedLanguages,
        &TerminalDevice->ControllerNameTable,
        (CHAR16 *)L"VT-100+ Serial Console",
        TRUE
        );
      AddUnicodeString2 (
        "en",
        gTerminalComponentName2.SupportedLanguages,
        &TerminalDevice->ControllerNameTable,
        (CHAR16 *)L"VT-100+ Serial Console",
        FALSE
        );

      break;

    case VTUTF8TYPE:
      AddUnicodeString2 (
        "eng",
        gTerminalComponentName.SupportedLanguages,
        &TerminalDevice->ControllerNameTable,
        (CHAR16 *)L"VT-UTF8 Serial Console",
        TRUE
        );
      AddUnicodeString2 (
        "en",
        gTerminalComponentName2.SupportedLanguages,
        &TerminalDevice->ControllerNameTable,
        (CHAR16 *)L"VT-UTF8 Serial Console",
        FALSE
        );

      break;

    case TTYTERMTYPE:
      AddUnicodeString2 (
        "eng",
        gTerminalComponentName.SupportedLanguages,
        &TerminalDevice->ControllerNameTable,
        (CHAR16 *)L"Tty Terminal Serial Console",
        TRUE
        );
      AddUnicodeString2 (
        "en",
        gTerminalComponentName2.SupportedLanguages,
        &TerminalDevice->ControllerNameTable,
        (CHAR16 *)L"Tty Terminal Serial Console",
        FALSE
        );

      break;
    }

    //
    // Build the device path for the child device
    //
    Status = SetTerminalDevicePath (
              TerminalDevice->TerminalType,
              ParentDevicePath,
              &TerminalDevice->DevicePath
              );
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    Status = TerminalConOutReset (SimpleTextOutput, FALSE);
    if (EFI_ERROR (Status)) {
      goto ReportError;
    }

    Status = TerminalConOutSetMode (SimpleTextOutput, 0);
    if (EFI_ERROR (Status)) {
      goto ReportError;
    }

    Status = TerminalConOutEnableCursor (SimpleTextOutput, TRUE);
    if (EFI_ERROR (Status)) {
      goto ReportError;
    }

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

    Status = gBS->InstallProtocolInterface (
                    &TerminalDevice->Handle,
                    &gEfiDevicePathProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    TerminalDevice->DevicePath
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    //
    // Register the Parent-Child relationship via
    // EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiSerialIoProtocolGuid,
                    (VOID **) &TerminalDevice->SerialIo,
                    This->DriverBindingHandle,
                    TerminalDevice->Handle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
  }

  //
  // Find the child handle, and get its TerminalDevice private data
  //
  Status = gBS->OpenProtocolInformation (
                  Controller,
                  &gEfiSerialIoProtocolGuid,
                  &OpenInfoBuffer,
                  &EntryCount
                  );
  if (!EFI_ERROR (Status)) {
    Status = EFI_NOT_FOUND;
    ASSERT (OpenInfoBuffer != NULL);
    for (Index = 0; Index < EntryCount; Index++) {
      if ((OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
        //
        // Find the child terminal handle.
        // Test whether the SimpleTxtIn and SimpleTxtOut have been published
        //
        Status = gBS->OpenProtocol (
                        OpenInfoBuffer[Index].ControllerHandle,
                        &gEfiSimpleTextInProtocolGuid,
                        (VOID **) &SimpleTextInput,
                        This->DriverBindingHandle,
                        OpenInfoBuffer[Index].ControllerHandle,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                        );
        if (!EFI_ERROR (Status)) {
          SimTxtInInstalled = TRUE;
          TerminalDevice = TERMINAL_CON_IN_DEV_FROM_THIS (SimpleTextInput);
        }

        Status = gBS->OpenProtocol (
                        OpenInfoBuffer[Index].ControllerHandle,
                        &gEfiSimpleTextOutProtocolGuid,
                        (VOID **) &SimpleTextOutput,
                        This->DriverBindingHandle,
                        OpenInfoBuffer[Index].ControllerHandle,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                        );
        if (!EFI_ERROR (Status)) {
          SimTxtOutInstalled = TRUE;
          TerminalDevice = TERMINAL_CON_OUT_DEV_FROM_THIS (SimpleTextOutput);
        }
        Status = EFI_SUCCESS;
        break;
      }
    }

    FreePool (OpenInfoBuffer);
    if (EFI_ERROR (Status)) {
      goto ReportError;
    }
  } else {
    goto ReportError;
  }

  ASSERT (TerminalDevice != NULL);
  //
  // Only do the reset if the device path is in the Conout variable
  //
  if (ConInSelected && !SimTxtInInstalled) {
    Status = TerminalDevice->SimpleInput.Reset (
                                          &TerminalDevice->SimpleInput,
                                          FALSE
                                          );
    if (EFI_ERROR (Status)) {
      //
      // Need to report Error Code first
      //
      goto ReportError;
    }
  }

  //
  // Only output the configure string to remote terminal if the device path
  // is in the Conout variable
  //
  if (ConOutSelected && !SimTxtOutInstalled) {
    Status = TerminalDevice->SimpleTextOutput.SetAttribute (
                                                        &TerminalDevice->SimpleTextOutput,
                                                        EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK)
                                                        );
    if (EFI_ERROR (Status)) {
      goto ReportError;
    }

    Status = TerminalDevice->SimpleTextOutput.Reset (
                                                &TerminalDevice->SimpleTextOutput,
                                                FALSE
                                                );
    if (EFI_ERROR (Status)) {
      goto ReportError;
    }

    Status = TerminalDevice->SimpleTextOutput.SetMode (
                                                &TerminalDevice->SimpleTextOutput,
                                                0
                                                );
    if (EFI_ERROR (Status)) {
      goto ReportError;
    }

    Status = TerminalDevice->SimpleTextOutput.EnableCursor (
                                                &TerminalDevice->SimpleTextOutput,
                                                TRUE
                                                );
    if (EFI_ERROR (Status)) {
      goto ReportError;
    }
  }

  //
  // Simple In/Out Protocol will not be installed onto the handle if the
  // device path to the handle is not present in the ConIn/ConOut
  // environment variable. But If RemainingDevicePath is NULL, then always
  // produce both Simple In and Simple Text Output Protocols. This is required
  // for the connect all sequences to make sure all possible consoles are
  // produced no matter what the current values of ConIn, ConOut, or StdErr are.
  //
  if (!SimTxtInInstalled && (ConInSelected || NullRemaining)) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &TerminalDevice->Handle,
                    &gEfiSimpleTextInProtocolGuid,
                    &TerminalDevice->SimpleInput,
                    &gEfiSimpleTextInputExProtocolGuid,
                    &TerminalDevice->SimpleInputEx,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
  }

  if (!SimTxtOutInstalled && (ConOutSelected || NullRemaining)) {
    Status = gBS->InstallProtocolInterface (
                    &TerminalDevice->Handle,
                    &gEfiSimpleTextOutProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &TerminalDevice->SimpleTextOutput
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
  }

  return EFI_SUCCESS;

ReportError:
  //
  // Report error code before exiting
  //
  DevicePath = ParentDevicePath;
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_ERROR_CODE | EFI_ERROR_MINOR,
    (EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_CONTROLLER_ERROR),
    DevicePath
    );

Error:
  //
  // Use the Stop() function to free all resources allocated in Start()
  //
  if (TerminalDevice != NULL) {

    if (TerminalDevice->Handle != NULL) {
      This->Stop (This, Controller, 1, &TerminalDevice->Handle);
    } else {

      if (TerminalDevice->TwoSecondTimeOut != NULL) {
        gBS->CloseEvent (TerminalDevice->TwoSecondTimeOut);
      }

      if (TerminalDevice->TimerEvent != NULL) {
        gBS->CloseEvent (TerminalDevice->TimerEvent);
      }

      if (TerminalDevice->SimpleInput.WaitForKey != NULL) {
        gBS->CloseEvent (TerminalDevice->SimpleInput.WaitForKey);
      }

      if (TerminalDevice->SimpleInputEx.WaitForKeyEx != NULL) {
        gBS->CloseEvent (TerminalDevice->SimpleInputEx.WaitForKeyEx);
      }

      TerminalFreeNotifyList (&TerminalDevice->NotifyList);

      if (TerminalDevice->RawFiFo != NULL) {
        FreePool (TerminalDevice->RawFiFo);
      }
      if (TerminalDevice->UnicodeFiFo != NULL) {
        FreePool (TerminalDevice->UnicodeFiFo);
      }
      if (TerminalDevice->EfiKeyFiFo != NULL) {
        FreePool (TerminalDevice->EfiKeyFiFo);
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
    }
  }

  This->Stop (This, Controller, 0, NULL);

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
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Controller,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  )
{
  EFI_STATUS                       Status;
  UINTN                            Index;
  BOOLEAN                          AllChildrenStopped;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *SimpleTextOutput;
  TERMINAL_DEV                     *TerminalDevice;
  EFI_DEVICE_PATH_PROTOCOL         *ParentDevicePath;
  EFI_SERIAL_IO_PROTOCOL           *SerialIo;
  EFI_DEVICE_PATH_PROTOCOL         *DevicePath;

  Status = gBS->HandleProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

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
                    &gEfiCallerIdGuid,
                    (VOID **) &ParentDevicePath,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      //
      // Remove Parent Device Path from
      // the Console Device Environment Variables
      //
      TerminalRemoveConsoleDevVariable (L"ConInDev", ParentDevicePath);
      TerminalRemoveConsoleDevVariable (L"ConOutDev", ParentDevicePath);
      TerminalRemoveConsoleDevVariable (L"ErrOutDev", ParentDevicePath);

      //
      // Uninstall the Terminal Driver's GUID Tag from the Serial controller
      //
      Status = gBS->UninstallMultipleProtocolInterfaces (
                      Controller,
                      &gEfiCallerIdGuid,
                      ParentDevicePath,
                      NULL
                      );

      //
      // Free the ParentDevicePath that was duplicated in Start()
      //
      if (!EFI_ERROR (Status)) {
        FreePool (ParentDevicePath);
      }
    }

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
                    (VOID **) &SimpleTextOutput,
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
              (VOID **) &SerialIo,
              This->DriverBindingHandle,
              ChildHandleBuffer[Index],
              EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
              );
      } else {

        if (TerminalDevice->ControllerNameTable != NULL) {
          FreeUnicodeStringTable (TerminalDevice->ControllerNameTable);
        }

        gBS->CloseEvent (TerminalDevice->TimerEvent);
        gBS->CloseEvent (TerminalDevice->TwoSecondTimeOut);
        gBS->CloseEvent (TerminalDevice->SimpleInput.WaitForKey);
        gBS->CloseEvent (TerminalDevice->SimpleInputEx.WaitForKeyEx);
        TerminalFreeNotifyList (&TerminalDevice->NotifyList);
        FreePool (TerminalDevice->DevicePath);
        if (TerminalDevice->TerminalConsoleModeData != NULL) {
          FreePool (TerminalDevice->TerminalConsoleModeData);
        }
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
  EFI_STATUS                Status;
  UINTN                     NameSize;
  UINTN                     VariableSize;
  UINT8                     TerminalType;
  EFI_DEVICE_PATH_PROTOCOL  *Variable;
  EFI_DEVICE_PATH_PROTOCOL  *NewVariable;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  EDKII_SET_VARIABLE_STATUS *SetVariableStatus;

  //
  // Get global variable and its size according to the name given.
  //
  GetEfiGlobalVariable2 (VariableName, (VOID**)&Variable, NULL);
  if (Variable == NULL) {
    return;
  }

  //
  // Append terminal device path onto the variable.
  //
  for (TerminalType = PCANSITYPE; TerminalType <= TTYTERMTYPE; TerminalType++) {
    SetTerminalDevicePath (TerminalType, ParentDevicePath, &TempDevicePath);
    NewVariable = AppendDevicePathInstance (Variable, TempDevicePath);
    ASSERT (NewVariable != NULL);
    if (Variable != NULL) {
      FreePool (Variable);
    }

    if (TempDevicePath != NULL) {
      FreePool (TempDevicePath);
    }

    Variable = NewVariable;
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
    NameSize = StrSize (VariableName);
    SetVariableStatus = AllocatePool (sizeof (EDKII_SET_VARIABLE_STATUS) + NameSize + VariableSize);
    if (SetVariableStatus != NULL) {
      CopyGuid (&SetVariableStatus->Guid, &gEfiGlobalVariableGuid);
      SetVariableStatus->NameSize   = NameSize;
      SetVariableStatus->DataSize   = VariableSize;
      SetVariableStatus->SetStatus  = Status;
      SetVariableStatus->Attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
      CopyMem (SetVariableStatus + 1,                          VariableName, NameSize);
      CopyMem (((UINT8 *) (SetVariableStatus + 1)) + NameSize, Variable,     VariableSize);

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

  return ;
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
  UINT8                     TerminalType;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  EFI_DEVICE_PATH_PROTOCOL  *Variable;
  EFI_DEVICE_PATH_PROTOCOL  *OriginalVariable;
  EFI_DEVICE_PATH_PROTOCOL  *NewVariable;
  EFI_DEVICE_PATH_PROTOCOL  *SavedNewVariable;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;

  Instance  = NULL;

  //
  // Get global variable and its size according to the name given.
  //
  GetEfiGlobalVariable2 (VariableName, (VOID**)&Variable, NULL);
  if (Variable == NULL) {
    return ;
  }

  FoundOne          = FALSE;
  OriginalVariable  = Variable;
  NewVariable       = NULL;

  //
  // Get first device path instance from Variable
  //
  Instance = GetNextDevicePathInstance (&Variable, &InstanceSize);
  if (Instance == NULL) {
    FreePool (OriginalVariable);
    return ;
  }
  //
  // Loop through all the device path instances of Variable
  //
  do {
    //
    // Loop through all the terminal types that this driver supports
    //
    Match = FALSE;
    for (TerminalType = PCANSITYPE; TerminalType <= TTYTERMTYPE; TerminalType++) {

      SetTerminalDevicePath (TerminalType, ParentDevicePath, &TempDevicePath);

      //
      // Compare the generated device path to the current device path instance
      //
      if (TempDevicePath != NULL) {
        if (CompareMem (Instance, TempDevicePath, InstanceSize) == 0) {
          Match     = TRUE;
          FoundOne  = TRUE;
        }

        FreePool (TempDevicePath);
      }
    }
    //
    // If a match was not found, then keep the current device path instance
    //
    if (!Match) {
      SavedNewVariable  = NewVariable;
      NewVariable       = AppendDevicePathInstance (NewVariable, Instance);
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

  if (NewVariable != NULL) {
    FreePool (NewVariable);
  }

  return ;
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
  IN  UINT8                       TerminalType,
  IN  EFI_DEVICE_PATH_PROTOCOL    *ParentDevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL    **TerminalDevicePath
  )
{
  VENDOR_DEVICE_PATH  Node;

  *TerminalDevicePath = NULL;
  Node.Header.Type    = MESSAGING_DEVICE_PATH;
  Node.Header.SubType = MSG_VENDOR_DP;

  //
  // Generate terminal device path node according to terminal type.
  //
  switch (TerminalType) {

  case PCANSITYPE:
    CopyGuid (&Node.Guid, &gEfiPcAnsiGuid);
    break;

  case VT100TYPE:
    CopyGuid (&Node.Guid, &gEfiVT100Guid);
    break;

  case VT100PLUSTYPE:
    CopyGuid (&Node.Guid, &gEfiVT100PlusGuid);
    break;

  case VTUTF8TYPE:
    CopyGuid (&Node.Guid, &gEfiVTUTF8Guid);
    break;

  case TTYTERMTYPE:
    CopyGuid (&Node.Guid, &gEfiTtyTermGuid);
    break;

  default:
    return EFI_UNSUPPORTED;
  }

  //
  // Get VENDOR_DEVCIE_PATH size and put into Node.Header
  //
  SetDevicePathNodeLength (
    &Node.Header,
    sizeof (VENDOR_DEVICE_PATH)
    );

  //
  // Append the terminal node onto parent device path
  // to generate a complete terminal device path.
  //
  *TerminalDevicePath = AppendDevicePathNode (
                          ParentDevicePath,
                          (EFI_DEVICE_PATH_PROTOCOL *) &Node
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
InitializeTerminal(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

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
  IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL     *CheckDevicePath;

  CheckDevicePath = DevicePath;
  while (!IsDevicePathEnd (CheckDevicePath)) {
    //
    // Check device whether is hot plug device or not throught Device Path
    //
    if ((DevicePathType (CheckDevicePath) == MESSAGING_DEVICE_PATH) &&
        (DevicePathSubType (CheckDevicePath) == MSG_USB_DP ||
         DevicePathSubType (CheckDevicePath) == MSG_USB_CLASS_DP ||
         DevicePathSubType (CheckDevicePath) == MSG_USB_WWID_DP)) {
      //
      // If Device is USB device
      //
      return TRUE;
    }
    if ((DevicePathType (CheckDevicePath) == HARDWARE_DEVICE_PATH) &&
        (DevicePathSubType (CheckDevicePath) == HW_PCCARD_DP)) {
      //
      // If Device is PCCard
      //
      return TRUE;
    }

    CheckDevicePath = NextDevicePathNode (CheckDevicePath);
  }

  return FALSE;
}

