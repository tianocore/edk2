/** @file
handles console redirection from boot manager

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BootMaintenanceManager.h"

/**
  Function compares a device path data structure to that of all the nodes of a
  second device path instance.

  @param  Multi                 A pointer to a multi-instance device path data
                                structure.
  @param  Single                A pointer to a single-instance device path data
                                structure.

  @retval TRUE                  If the Single device path is contained within Multi device path.
  @retval FALSE                 The Single device path is not match within Multi device path.

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

  if (Multi == NULL || Single  == NULL) {
    return FALSE;
  }

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
  Check whether the device path node is ISA Serial Node.

  @param Acpi           Device path node to be checked

  @retval TRUE          It's ISA Serial Node.
  @retval FALSE         It's NOT ISA Serial Node.

**/
BOOLEAN
IsIsaSerialNode (
  IN ACPI_HID_DEVICE_PATH *Acpi
  )
{
  return (BOOLEAN) (
      (DevicePathType (Acpi) == ACPI_DEVICE_PATH) &&
      (DevicePathSubType (Acpi) == ACPI_DP) &&
      (ReadUnaligned32 (&Acpi->HID) == EISA_PNP_ID (0x0501))
      );
}

/**
  Update Com Ports attributes from DevicePath

  @param DevicePath      DevicePath that contains Com ports

  @retval EFI_SUCCESS   The update is successful.

**/
EFI_STATUS
UpdateComAttributeFromVariable (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  Update the multi-instance device path of Terminal Device based on
  the global TerminalMenu. If ChangeTernimal is TRUE, the terminal
  device path in the Terminal Device in TerminalMenu is also updated.

  @param DevicePath      The multi-instance device path.
  @param ChangeTerminal  TRUE, then device path in the Terminal Device
                         in TerminalMenu is also updated; FALSE, no update.

  @return EFI_SUCCESS    The function completes successfully.

**/
EFI_STATUS
ChangeTerminalDevicePath (
  IN OUT    EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN        BOOLEAN                   ChangeTerminal
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  EFI_DEVICE_PATH_PROTOCOL  *Node1;
  ACPI_HID_DEVICE_PATH      *Acpi;
  UART_DEVICE_PATH          *Uart;
  UART_DEVICE_PATH          *Uart1;
  UINTN                     Com;
  BM_TERMINAL_CONTEXT       *NewTerminalContext;
  BM_MENU_ENTRY             *NewMenuEntry;

  Node  = DevicePath;
  Node  = NextDevicePathNode (Node);
  Com   = 0;
  while (!IsDevicePathEnd (Node)) {
    Acpi = (ACPI_HID_DEVICE_PATH *) Node;
    if (IsIsaSerialNode (Acpi)) {
      CopyMem (&Com, &Acpi->UID, sizeof (UINT32));
    }

    NewMenuEntry = BOpt_GetMenuEntry (&TerminalMenu, Com);

    NewTerminalContext = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;
    if ((DevicePathType (Node) == MESSAGING_DEVICE_PATH) && (DevicePathSubType (Node) == MSG_UART_DP)) {
      Uart = (UART_DEVICE_PATH *) Node;
      CopyMem (
        &Uart->BaudRate,
        &NewTerminalContext->BaudRate,
        sizeof (UINT64)
        );

      CopyMem (
        &Uart->DataBits,
        &NewTerminalContext->DataBits,
        sizeof (UINT8)
        );

      CopyMem (
        &Uart->Parity,
        &NewTerminalContext->Parity,
        sizeof (UINT8)
        );

      CopyMem (
        &Uart->StopBits,
        &NewTerminalContext->StopBits,
        sizeof (UINT8)
        );
      //
      // Change the device path in the ComPort
      //
      if (ChangeTerminal) {
        Node1 = NewTerminalContext->DevicePath;
        Node1 = NextDevicePathNode (Node1);
        while (!IsDevicePathEnd (Node1)) {
          if ((DevicePathType (Node1) == MESSAGING_DEVICE_PATH) && (DevicePathSubType (Node1) == MSG_UART_DP)) {
            Uart1 = (UART_DEVICE_PATH *) Node1;
            CopyMem (
              &Uart1->BaudRate,
              &NewTerminalContext->BaudRate,
              sizeof (UINT64)
              );

            CopyMem (
              &Uart1->DataBits,
              &NewTerminalContext->DataBits,
              sizeof (UINT8)
              );

            CopyMem (
              &Uart1->Parity,
              &NewTerminalContext->Parity,
              sizeof (UINT8)
              );

            CopyMem (
              &Uart1->StopBits,
              &NewTerminalContext->StopBits,
              sizeof (UINT8)
              );
            break;
          }
          //
          // end if
          //
          Node1 = NextDevicePathNode (Node1);
        }
        //
        // end while
        //
        break;
      }
    }

    Node = NextDevicePathNode (Node);
  }

  return EFI_SUCCESS;

}

/**
  Update the device path that describing a terminal device
  based on the new BaudRate, Data Bits, parity and Stop Bits
  set.

  @param DevicePath terminal device's path

**/
VOID
ChangeVariableDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  ACPI_HID_DEVICE_PATH      *Acpi;
  UART_DEVICE_PATH          *Uart;
  UINTN                     Com;
  BM_TERMINAL_CONTEXT       *NewTerminalContext;
  BM_MENU_ENTRY             *NewMenuEntry;

  Node  = DevicePath;
  Node  = NextDevicePathNode (Node);
  Com   = 0;
  while (!IsDevicePathEnd (Node)) {
    Acpi = (ACPI_HID_DEVICE_PATH *) Node;
    if (IsIsaSerialNode (Acpi)) {
      CopyMem (&Com, &Acpi->UID, sizeof (UINT32));
    }

    if ((DevicePathType (Node) == MESSAGING_DEVICE_PATH) && (DevicePathSubType (Node) == MSG_UART_DP)) {
      NewMenuEntry = BOpt_GetMenuEntry (
                      &TerminalMenu,
                      Com
                      );
      ASSERT (NewMenuEntry != NULL);
      NewTerminalContext  = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;
      Uart                = (UART_DEVICE_PATH *) Node;
      CopyMem (
        &Uart->BaudRate,
        &NewTerminalContext->BaudRate,
        sizeof (UINT64)
        );

      CopyMem (
        &Uart->DataBits,
        &NewTerminalContext->DataBits,
        sizeof (UINT8)
        );

      CopyMem (
        &Uart->Parity,
        &NewTerminalContext->Parity,
        sizeof (UINT8)
        );

      CopyMem (
        &Uart->StopBits,
        &NewTerminalContext->StopBits,
        sizeof (UINT8)
        );
    }

    Node = NextDevicePathNode (Node);
  }
}

/**
  Retrieve ACPI UID of UART from device path

  @param Handle          The handle for the UART device.
  @param AcpiUid         The ACPI UID on output.

  @retval  TRUE   Find valid UID from device path
  @retval  FALSE  Can't find

**/
BOOLEAN
RetrieveUartUid (
  IN EFI_HANDLE   Handle,
  IN OUT UINT32   *AcpiUid
  )
{
  EFI_STATUS                Status;
  ACPI_HID_DEVICE_PATH      *Acpi;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DevicePath
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Acpi = NULL;
  for (; !IsDevicePathEnd (DevicePath); DevicePath = NextDevicePathNode (DevicePath)) {
    if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) && (DevicePathSubType (DevicePath) == MSG_UART_DP)) {
      break;
    }
    //
    // Acpi points to the node before the Uart node
    //
    Acpi = (ACPI_HID_DEVICE_PATH *) DevicePath;
  }

  if ((Acpi != NULL) && IsIsaSerialNode (Acpi)) {
    if (AcpiUid != NULL) {
      CopyMem (AcpiUid, &Acpi->UID, sizeof (UINT32));
    }
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Sort Uart handles array with Acpi->UID from low to high.

  @param Handles         EFI_SERIAL_IO_PROTOCOL handle buffer
  @param NoHandles       EFI_SERIAL_IO_PROTOCOL handle count
**/
VOID
SortedUartHandle (
  IN  EFI_HANDLE *Handles,
  IN  UINTN      NoHandles
  )
{
  UINTN       Index1;
  UINTN       Index2;
  UINTN       Position;
  UINT32      AcpiUid1;
  UINT32      AcpiUid2;
  UINT32      TempAcpiUid;
  EFI_HANDLE  TempHandle;

  for (Index1 = 0; Index1 < NoHandles-1; Index1++) {
    if (!RetrieveUartUid (Handles[Index1], &AcpiUid1)) {
      continue;
    }
    TempHandle  = Handles[Index1];
    Position    = Index1;
    TempAcpiUid = AcpiUid1;

    for (Index2 = Index1+1; Index2 < NoHandles; Index2++) {
      if (!RetrieveUartUid (Handles[Index2], &AcpiUid2)) {
        continue;
      }
      if (AcpiUid2 < TempAcpiUid) {
        TempAcpiUid = AcpiUid2;
        TempHandle  = Handles[Index2];
        Position    = Index2;
      }
    }
    Handles[Position] = Handles[Index1];
    Handles[Index1]   = TempHandle;
  }
}

/**
  Test whether DevicePath is a valid Terminal


  @param DevicePath      DevicePath to be checked
  @param Termi           If DevicePath is valid Terminal, terminal type is returned.
  @param Com             If DevicePath is valid Terminal, Com Port type is returned.

  @retval  TRUE         If DevicePath point to a Terminal.
  @retval  FALSE        If DevicePath does not point to a Terminal.

**/
BOOLEAN
IsTerminalDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  OUT TYPE_OF_TERMINAL         *Termi,
  OUT UINTN                    *Com
  );

/**
  Build a list containing all serial devices.


  @retval EFI_SUCCESS The function complete successfully.
  @retval EFI_UNSUPPORTED No serial ports present.

**/
EFI_STATUS
LocateSerialIo (
  VOID
  )
{
  UINTN                     Index;
  UINTN                     Index2;
  UINTN                     NoHandles;
  EFI_HANDLE                *Handles;
  EFI_STATUS                Status;
  ACPI_HID_DEVICE_PATH      *Acpi;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_SERIAL_IO_PROTOCOL    *SerialIo;
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  EFI_DEVICE_PATH_PROTOCOL  *OutDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *InpDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *ErrDevicePath;
  BM_MENU_ENTRY             *NewMenuEntry;
  BM_TERMINAL_CONTEXT       *NewTerminalContext;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  VENDOR_DEVICE_PATH        Vendor;

  //
  // Get all handles that have SerialIo protocol installed
  //
  InitializeListHead (&TerminalMenu.Head);
  TerminalMenu.MenuNumber = 0;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSerialIoProtocolGuid,
                  NULL,
                  &NoHandles,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    //
    // No serial ports present
    //
    return EFI_UNSUPPORTED;
  }

  //
  // Sort Uart handles array with Acpi->UID from low to high
  // then Terminal menu can be built from low Acpi->UID to high Acpi->UID
  //
  SortedUartHandle (Handles, NoHandles);

  for (Index = 0; Index < NoHandles; Index++) {
    //
    // Check to see whether the handle has DevicePath Protocol installed
    //
    gBS->HandleProtocol (
          Handles[Index],
          &gEfiDevicePathProtocolGuid,
          (VOID **) &DevicePath
          );

    Acpi = NULL;
    for (Node = DevicePath; !IsDevicePathEnd (Node); Node = NextDevicePathNode (Node)) {
      if ((DevicePathType (Node) == MESSAGING_DEVICE_PATH) && (DevicePathSubType (Node) == MSG_UART_DP)) {
        break;
      }
      //
      // Acpi points to the node before Uart node
      //
      Acpi = (ACPI_HID_DEVICE_PATH *) Node;
    }

    if ((Acpi != NULL) && IsIsaSerialNode (Acpi)) {
      NewMenuEntry = BOpt_CreateMenuEntry (BM_TERMINAL_CONTEXT_SELECT);
      if (NewMenuEntry == NULL) {
        FreePool (Handles);
        return EFI_OUT_OF_RESOURCES;
      }

      NewTerminalContext = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;
      CopyMem (&NewMenuEntry->OptionNumber, &Acpi->UID, sizeof (UINT32));
      NewTerminalContext->DevicePath = DuplicateDevicePath (DevicePath);
      //
      // BugBug: I have no choice, calling EfiLibStrFromDatahub will hang the system!
      // coz' the misc data for each platform is not correct, actually it's the device path stored in
      // datahub which is not completed, so a searching for end of device path will enter a
      // dead-loop.
      //
      NewMenuEntry->DisplayString = EfiLibStrFromDatahub (DevicePath);
      if (NULL == NewMenuEntry->DisplayString) {
        NewMenuEntry->DisplayString = UiDevicePathToStr (DevicePath);
      }

      NewMenuEntry->HelpString = NULL;

      NewMenuEntry->DisplayStringToken = HiiSetString (mBmmCallbackInfo->BmmHiiHandle, 0, NewMenuEntry->DisplayString, NULL);

      NewMenuEntry->HelpStringToken = NewMenuEntry->DisplayStringToken;

      gBS->HandleProtocol (
            Handles[Index],
            &gEfiSerialIoProtocolGuid,
            (VOID **) &SerialIo
            );

      CopyMem (
        &NewTerminalContext->BaudRate,
        &SerialIo->Mode->BaudRate,
        sizeof (UINT64)
        );

      CopyMem (
        &NewTerminalContext->DataBits,
        &SerialIo->Mode->DataBits,
        sizeof (UINT8)
        );

      CopyMem (
        &NewTerminalContext->Parity,
        &SerialIo->Mode->Parity,
        sizeof (UINT8)
        );

      CopyMem (
        &NewTerminalContext->StopBits,
        &SerialIo->Mode->StopBits,
        sizeof (UINT8)
        );
      InsertTailList (&TerminalMenu.Head, &NewMenuEntry->Link);
      TerminalMenu.MenuNumber++;
    }
  }
  if (Handles != NULL) {
    FreePool (Handles);
  }

  //
  // Get L"ConOut", L"ConIn" and L"ErrOut" from the Var
  //
  GetEfiGlobalVariable2 (L"ConOut", (VOID**)&OutDevicePath, NULL);
  GetEfiGlobalVariable2 (L"ConIn", (VOID**)&InpDevicePath, NULL);
  GetEfiGlobalVariable2 (L"ErrOut", (VOID**)&ErrDevicePath, NULL);
  if (OutDevicePath != NULL) {
    UpdateComAttributeFromVariable (OutDevicePath);
  }

  if (InpDevicePath != NULL) {
    UpdateComAttributeFromVariable (InpDevicePath);
  }

  if (ErrDevicePath != NULL) {
    UpdateComAttributeFromVariable (ErrDevicePath);
  }

  for (Index = 0; Index < TerminalMenu.MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (&TerminalMenu, Index);
    if (NULL == NewMenuEntry) {
      return EFI_NOT_FOUND;
    }

    NewTerminalContext                = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;

    NewTerminalContext->TerminalType  = 0;
    NewTerminalContext->IsConIn       = FALSE;
    NewTerminalContext->IsConOut      = FALSE;
    NewTerminalContext->IsStdErr      = FALSE;

    Vendor.Header.Type                = MESSAGING_DEVICE_PATH;
    Vendor.Header.SubType             = MSG_VENDOR_DP;

    for (Index2 = 0; Index2 < (ARRAY_SIZE (TerminalTypeGuid)); Index2++) {
      CopyMem (&Vendor.Guid, &TerminalTypeGuid[Index2], sizeof (EFI_GUID));
      SetDevicePathNodeLength (&Vendor.Header, sizeof (VENDOR_DEVICE_PATH));
      NewDevicePath = AppendDevicePathNode (
                        NewTerminalContext->DevicePath,
                        (EFI_DEVICE_PATH_PROTOCOL *) &Vendor
                        );
      if (NewMenuEntry->HelpString != NULL) {
        FreePool (NewMenuEntry->HelpString);
      }
      //
      // NewMenuEntry->HelpString = UiDevicePathToStr (NewDevicePath);
      // NewMenuEntry->DisplayString = NewMenuEntry->HelpString;
      //
      NewMenuEntry->HelpString = NULL;

      NewMenuEntry->DisplayStringToken = HiiSetString (mBmmCallbackInfo->BmmHiiHandle, 0, NewMenuEntry->DisplayString, NULL);

      NewMenuEntry->HelpStringToken = NewMenuEntry->DisplayStringToken;

      if (MatchDevicePaths (OutDevicePath, NewDevicePath)) {
        NewTerminalContext->IsConOut      = TRUE;
        NewTerminalContext->TerminalType  = (UINT8) Index2;
      }

      if (MatchDevicePaths (InpDevicePath, NewDevicePath)) {
        NewTerminalContext->IsConIn       = TRUE;
        NewTerminalContext->TerminalType  = (UINT8) Index2;
      }

      if (MatchDevicePaths (ErrDevicePath, NewDevicePath)) {
        NewTerminalContext->IsStdErr      = TRUE;
        NewTerminalContext->TerminalType  = (UINT8) Index2;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Update Com Ports attributes from DevicePath

  @param DevicePath      DevicePath that contains Com ports

  @retval EFI_SUCCESS   The update is successful.
  @retval EFI_NOT_FOUND Can not find specific menu entry
**/
EFI_STATUS
UpdateComAttributeFromVariable (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  EFI_DEVICE_PATH_PROTOCOL  *SerialNode;
  ACPI_HID_DEVICE_PATH      *Acpi;
  UART_DEVICE_PATH          *Uart;
  UART_DEVICE_PATH          *Uart1;
  UINTN                     TerminalNumber;
  BM_MENU_ENTRY             *NewMenuEntry;
  BM_TERMINAL_CONTEXT       *NewTerminalContext;
  UINTN                     Index;

  Node            = DevicePath;
  Node            = NextDevicePathNode (Node);
  TerminalNumber  = 0;
  for (Index = 0; Index < TerminalMenu.MenuNumber; Index++) {
    while (!IsDevicePathEnd (Node)) {
      Acpi = (ACPI_HID_DEVICE_PATH *) Node;
      if (IsIsaSerialNode (Acpi)) {
        CopyMem (&TerminalNumber, &Acpi->UID, sizeof (UINT32));
      }

      if ((DevicePathType (Node) == MESSAGING_DEVICE_PATH) && (DevicePathSubType (Node) == MSG_UART_DP)) {
        Uart          = (UART_DEVICE_PATH *) Node;
        NewMenuEntry  = BOpt_GetMenuEntry (&TerminalMenu, TerminalNumber);
        if (NULL == NewMenuEntry) {
          return EFI_NOT_FOUND;
        }

        NewTerminalContext = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;
        CopyMem (
          &NewTerminalContext->BaudRate,
          &Uart->BaudRate,
          sizeof (UINT64)
          );

        CopyMem (
          &NewTerminalContext->DataBits,
          &Uart->DataBits,
          sizeof (UINT8)
          );

        CopyMem (
          &NewTerminalContext->Parity,
          &Uart->Parity,
          sizeof (UINT8)
          );

        CopyMem (
          &NewTerminalContext->StopBits,
          &Uart->StopBits,
          sizeof (UINT8)
          );

        SerialNode  = NewTerminalContext->DevicePath;
        SerialNode  = NextDevicePathNode (SerialNode);
        while (!IsDevicePathEnd (SerialNode)) {
          if ((DevicePathType (SerialNode) == MESSAGING_DEVICE_PATH) && (DevicePathSubType (SerialNode) == MSG_UART_DP)) {
            //
            // Update following device paths according to
            // previous acquired uart attributes
            //
            Uart1 = (UART_DEVICE_PATH *) SerialNode;
            CopyMem (
              &Uart1->BaudRate,
              &NewTerminalContext->BaudRate,
              sizeof (UINT64)
              );

            CopyMem (
              &Uart1->DataBits,
              &NewTerminalContext->DataBits,
              sizeof (UINT8)
              );
            CopyMem (
              &Uart1->Parity,
              &NewTerminalContext->Parity,
              sizeof (UINT8)
              );
            CopyMem (
              &Uart1->StopBits,
              &NewTerminalContext->StopBits,
              sizeof (UINT8)
              );

            break;
          }

          SerialNode = NextDevicePathNode (SerialNode);
        }
        //
        // end while
        //
      }

      Node = NextDevicePathNode (Node);
    }
    //
    // end while
    //
  }

  return EFI_SUCCESS;
}

/**
  Build up Console Menu based on types passed in. The type can
  be BM_CONSOLE_IN_CONTEXT_SELECT, BM_CONSOLE_OUT_CONTEXT_SELECT
  and BM_CONSOLE_ERR_CONTEXT_SELECT.

  @param ConsoleMenuType Can be BM_CONSOLE_IN_CONTEXT_SELECT, BM_CONSOLE_OUT_CONTEXT_SELECT
                         and BM_CONSOLE_ERR_CONTEXT_SELECT.

  @retval EFI_UNSUPPORTED The type passed in is not in the 3 types defined.
  @retval EFI_NOT_FOUND   If the EFI Variable defined in UEFI spec with name "ConOutDev",
                          "ConInDev" or "ConErrDev" doesn't exists.
  @retval EFI_OUT_OF_RESOURCES Not enough resource to complete the operations.
  @retval EFI_SUCCESS          Function completes successfully.

**/
EFI_STATUS
GetConsoleMenu (
  IN UINTN              ConsoleMenuType
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *AllDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *MultiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathInst;
  UINTN                     Size;
  UINTN                     AllCount;
  UINTN                     Index;
  UINTN                     Index2;
  BM_MENU_ENTRY             *NewMenuEntry;
  BM_CONSOLE_CONTEXT        *NewConsoleContext;
  TYPE_OF_TERMINAL          Terminal;
  UINTN                     Com;
  BM_MENU_OPTION            *ConsoleMenu;

  DevicePath    = NULL;
  AllDevicePath = NULL;
  AllCount      = 0;
  switch (ConsoleMenuType) {
  case BM_CONSOLE_IN_CONTEXT_SELECT:
    ConsoleMenu   = &ConsoleInpMenu;
    GetEfiGlobalVariable2 (L"ConIn", (VOID**)&DevicePath, NULL);
    GetEfiGlobalVariable2 (L"ConInDev", (VOID**)&AllDevicePath, NULL);
    break;

  case BM_CONSOLE_OUT_CONTEXT_SELECT:
    ConsoleMenu   = &ConsoleOutMenu;
    GetEfiGlobalVariable2 (L"ConOut", (VOID**)&DevicePath, NULL);
    GetEfiGlobalVariable2 (L"ConOutDev", (VOID**)&AllDevicePath, NULL);
    break;

  case BM_CONSOLE_ERR_CONTEXT_SELECT:
    ConsoleMenu   = &ConsoleErrMenu;
    GetEfiGlobalVariable2 (L"ErrOut", (VOID**)&DevicePath, NULL);
    GetEfiGlobalVariable2 (L"ErrOutDev", (VOID**)&AllDevicePath, NULL);
    break;

  default:
    return EFI_UNSUPPORTED;
  }

  if (NULL == AllDevicePath) {
    return EFI_NOT_FOUND;
  }

  InitializeListHead (&ConsoleMenu->Head);

  AllCount                = EfiDevicePathInstanceCount (AllDevicePath);
  ConsoleMenu->MenuNumber = 0;
  //
  // Following is menu building up for Console Devices selected.
  //
  MultiDevicePath = AllDevicePath;
  Index2          = 0;
  for (Index = 0; Index < AllCount; Index++) {
    DevicePathInst  = GetNextDevicePathInstance (&MultiDevicePath, &Size);

    NewMenuEntry    = BOpt_CreateMenuEntry (BM_CONSOLE_CONTEXT_SELECT);
    if (NULL == NewMenuEntry) {
      return EFI_OUT_OF_RESOURCES;
    }

    NewConsoleContext             = (BM_CONSOLE_CONTEXT *) NewMenuEntry->VariableContext;
    NewMenuEntry->OptionNumber    = Index2;

    NewConsoleContext->DevicePath = DuplicateDevicePath (DevicePathInst);
    ASSERT (NewConsoleContext->DevicePath != NULL);
    NewMenuEntry->DisplayString   = EfiLibStrFromDatahub (NewConsoleContext->DevicePath);
    if (NULL == NewMenuEntry->DisplayString) {
      NewMenuEntry->DisplayString = UiDevicePathToStr (NewConsoleContext->DevicePath);
    }

    NewMenuEntry->DisplayStringToken = HiiSetString (mBmmCallbackInfo->BmmHiiHandle, 0, NewMenuEntry->DisplayString, NULL);

    if (NULL == NewMenuEntry->HelpString) {
      NewMenuEntry->HelpStringToken = NewMenuEntry->DisplayStringToken;
    } else {
      NewMenuEntry->HelpStringToken = HiiSetString (mBmmCallbackInfo->BmmHiiHandle, 0, NewMenuEntry->HelpString, NULL);
    }

    NewConsoleContext->IsTerminal = IsTerminalDevicePath (
                                      NewConsoleContext->DevicePath,
                                      &Terminal,
                                      &Com
                                      );

    NewConsoleContext->IsActive = MatchDevicePaths (
                                    DevicePath,
                                    NewConsoleContext->DevicePath
                                    );

    if (NewConsoleContext->IsTerminal) {
      BOpt_DestroyMenuEntry (NewMenuEntry);
    } else {
      Index2++;
      ConsoleMenu->MenuNumber++;
      InsertTailList (&ConsoleMenu->Head, &NewMenuEntry->Link);
    }
  }

  return EFI_SUCCESS;
}

/**
  Build up ConsoleOutMenu, ConsoleInpMenu and ConsoleErrMenu

  @retval EFI_SUCCESS    The function always complete successfully.

**/
EFI_STATUS
GetAllConsoles (
  VOID
  )
{
  GetConsoleMenu (BM_CONSOLE_IN_CONTEXT_SELECT);
  GetConsoleMenu (BM_CONSOLE_OUT_CONTEXT_SELECT);
  GetConsoleMenu (BM_CONSOLE_ERR_CONTEXT_SELECT);
  return EFI_SUCCESS;
}

/**
  Free ConsoleOutMenu, ConsoleInpMenu and ConsoleErrMenu

  @retval EFI_SUCCESS    The function always complete successfully.
**/
EFI_STATUS
FreeAllConsoles (
  VOID
  )
{
  BOpt_FreeMenu (&ConsoleOutMenu);
  BOpt_FreeMenu (&ConsoleInpMenu);
  BOpt_FreeMenu (&ConsoleErrMenu);
  BOpt_FreeMenu (&TerminalMenu);
  return EFI_SUCCESS;
}

/**
  Test whether DevicePath is a valid Terminal


  @param DevicePath      DevicePath to be checked
  @param Termi           If DevicePath is valid Terminal, terminal type is returned.
  @param Com             If DevicePath is valid Terminal, Com Port type is returned.

  @retval  TRUE         If DevicePath point to a Terminal.
  @retval  FALSE        If DevicePath does not point to a Terminal.

**/
BOOLEAN
IsTerminalDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  OUT TYPE_OF_TERMINAL         *Termi,
  OUT UINTN                    *Com
  )
{
  BOOLEAN                   IsTerminal;
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  VENDOR_DEVICE_PATH        *Vendor;
  UART_DEVICE_PATH          *Uart;
  ACPI_HID_DEVICE_PATH      *Acpi;
  UINTN                     Index;

  IsTerminal = FALSE;

  Uart   = NULL;
  Vendor = NULL;
  Acpi   = NULL;
  for (Node = DevicePath; !IsDevicePathEnd (Node); Node = NextDevicePathNode (Node)) {
    //
    // Vendor points to the node before the End node
    //
    Vendor = (VENDOR_DEVICE_PATH *) Node;

    if ((DevicePathType (Node) == MESSAGING_DEVICE_PATH) && (DevicePathSubType (Node) == MSG_UART_DP)) {
      Uart = (UART_DEVICE_PATH *) Node;
    }

    if (Uart == NULL) {
      //
      // Acpi points to the node before the UART node
      //
      Acpi = (ACPI_HID_DEVICE_PATH *) Node;
    }
  }

  if (Vendor == NULL ||
      DevicePathType (Vendor) != MESSAGING_DEVICE_PATH ||
      DevicePathSubType (Vendor) != MSG_VENDOR_DP ||
      Uart == NULL) {
    return FALSE;
  }

  //
  // There are 9 kinds of Terminal types
  // check to see whether this devicepath
  // is one of that type
  //
  for (Index = 0; Index < ARRAY_SIZE (TerminalTypeGuid); Index++) {
    if (CompareGuid (&Vendor->Guid, &TerminalTypeGuid[Index])) {
      *Termi = Index;
      IsTerminal = TRUE;
      break;
    }
  }

  if (Index == ARRAY_SIZE (TerminalTypeGuid)) {
    IsTerminal = FALSE;
  }

  if (!IsTerminal) {
    return FALSE;
  }

  if ((Acpi != NULL) && IsIsaSerialNode (Acpi)) {
    CopyMem (Com, &Acpi->UID, sizeof (UINT32));
  } else {
    return FALSE;
  }

  return TRUE;
}

/**
  Get mode number according to column and row

  @param CallbackData    The BMM context data.
**/
VOID
GetConsoleOutMode (
  IN  BMM_CALLBACK_DATA    *CallbackData
  )
{
  UINTN                         Col;
  UINTN                         Row;
  UINTN                         CurrentCol;
  UINTN                         CurrentRow;
  UINTN                         Mode;
  UINTN                         MaxMode;
  EFI_STATUS                    Status;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *ConOut;

  ConOut   = gST->ConOut;
  MaxMode  = (UINTN) (ConOut->Mode->MaxMode);

  CurrentCol = PcdGet32 (PcdSetupConOutColumn);
  CurrentRow = PcdGet32 (PcdSetupConOutRow);
  for (Mode = 0; Mode < MaxMode; Mode++) {
    Status = ConOut->QueryMode (ConOut, Mode, &Col, &Row);
    if (!EFI_ERROR(Status)) {
      if (CurrentCol == Col && CurrentRow == Row) {
        CallbackData->BmmFakeNvData.ConsoleOutMode = (UINT16) Mode;
        break;
      }
    }
  }
}

/**

  Initialize console input device check box to ConsoleInCheck[MAX_MENU_NUMBER]
  in BMM_FAKE_NV_DATA structure.

  @param CallbackData    The BMM context data.

**/
VOID
GetConsoleInCheck (
  IN  BMM_CALLBACK_DATA    *CallbackData
  )
{
  UINT16              Index;
  BM_MENU_ENTRY       *NewMenuEntry;
  UINT8               *ConInCheck;
  BM_CONSOLE_CONTEXT  *NewConsoleContext;
  BM_TERMINAL_CONTEXT *NewTerminalContext;

  ASSERT (CallbackData != NULL);

  ConInCheck = &CallbackData->BmmFakeNvData.ConsoleInCheck[0];
  for (Index = 0; ((Index < ConsoleInpMenu.MenuNumber) && \
       (Index < MAX_MENU_NUMBER)) ; Index++) {
    NewMenuEntry      = BOpt_GetMenuEntry (&ConsoleInpMenu, Index);
    NewConsoleContext = (BM_CONSOLE_CONTEXT *) NewMenuEntry->VariableContext;
    ConInCheck[Index] = NewConsoleContext->IsActive;
  }

  for (Index = 0; Index < TerminalMenu.MenuNumber; Index++) {
    NewMenuEntry                = BOpt_GetMenuEntry (&TerminalMenu, Index);
    NewTerminalContext          = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;
    ASSERT (Index + ConsoleInpMenu.MenuNumber < MAX_MENU_NUMBER);
    ConInCheck[Index + ConsoleInpMenu.MenuNumber] = NewTerminalContext->IsConIn;
  }
}

/**

  Initialize console output device check box to ConsoleOutCheck[MAX_MENU_NUMBER]
  in BMM_FAKE_NV_DATA structure.

  @param CallbackData    The BMM context data.

**/
VOID
GetConsoleOutCheck (
  IN  BMM_CALLBACK_DATA    *CallbackData
  )
{
  UINT16              Index;
  BM_MENU_ENTRY       *NewMenuEntry;
  UINT8               *ConOutCheck;
  BM_CONSOLE_CONTEXT  *NewConsoleContext;
  BM_TERMINAL_CONTEXT *NewTerminalContext;

  ASSERT (CallbackData != NULL);
  ConOutCheck = &CallbackData->BmmFakeNvData.ConsoleOutCheck[0];
  for (Index = 0; ((Index < ConsoleOutMenu.MenuNumber) && \
       (Index < MAX_MENU_NUMBER)) ; Index++) {
    NewMenuEntry      = BOpt_GetMenuEntry (&ConsoleOutMenu, Index);
    NewConsoleContext = (BM_CONSOLE_CONTEXT *) NewMenuEntry->VariableContext;
    ConOutCheck[Index] = NewConsoleContext->IsActive;
  }

  for (Index = 0; Index < TerminalMenu.MenuNumber; Index++) {
    NewMenuEntry                = BOpt_GetMenuEntry (&TerminalMenu, Index);
    NewTerminalContext          = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;
    ASSERT (Index + ConsoleOutMenu.MenuNumber < MAX_MENU_NUMBER);
    ConOutCheck[Index + ConsoleOutMenu.MenuNumber] = NewTerminalContext->IsConOut;
  }
}

/**

  Initialize standard error output device check box to ConsoleErrCheck[MAX_MENU_NUMBER]
  in BMM_FAKE_NV_DATA structure.

  @param CallbackData    The BMM context data.

**/
VOID
GetConsoleErrCheck (
  IN  BMM_CALLBACK_DATA    *CallbackData
  )
{
  UINT16              Index;
  BM_MENU_ENTRY       *NewMenuEntry;
  UINT8               *ConErrCheck;
  BM_CONSOLE_CONTEXT  *NewConsoleContext;
  BM_TERMINAL_CONTEXT *NewTerminalContext;

  ASSERT (CallbackData != NULL);
  ConErrCheck = &CallbackData->BmmFakeNvData.ConsoleErrCheck[0];
  for (Index = 0; ((Index < ConsoleErrMenu.MenuNumber) && \
       (Index < MAX_MENU_NUMBER)) ; Index++) {
    NewMenuEntry      = BOpt_GetMenuEntry (&ConsoleErrMenu, Index);
    NewConsoleContext = (BM_CONSOLE_CONTEXT *) NewMenuEntry->VariableContext;
    ConErrCheck[Index] = NewConsoleContext->IsActive;
  }

  for (Index = 0; Index < TerminalMenu.MenuNumber; Index++) {
    NewMenuEntry                = BOpt_GetMenuEntry (&TerminalMenu, Index);
    NewTerminalContext          = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;
    ASSERT (Index + ConsoleErrMenu.MenuNumber < MAX_MENU_NUMBER);
    ConErrCheck[Index + ConsoleErrMenu.MenuNumber] = NewTerminalContext->IsStdErr;
  }
}

/**

  Initialize terminal attributes (baudrate, data rate, stop bits, parity and terminal type)
  to BMM_FAKE_NV_DATA structure.

  @param CallbackData    The BMM context data.

**/
VOID
GetTerminalAttribute (
  IN  BMM_CALLBACK_DATA    *CallbackData
  )
{
  BMM_FAKE_NV_DATA     *CurrentFakeNVMap;
  BM_MENU_ENTRY        *NewMenuEntry;
  BM_TERMINAL_CONTEXT  *NewTerminalContext;
  UINT16               TerminalIndex;
  UINT8                AttributeIndex;

  ASSERT (CallbackData != NULL);

  CurrentFakeNVMap = &CallbackData->BmmFakeNvData;
  for (TerminalIndex = 0; ((TerminalIndex < TerminalMenu.MenuNumber) && \
       (TerminalIndex < MAX_MENU_NUMBER)); TerminalIndex++) {
    NewMenuEntry        = BOpt_GetMenuEntry (&TerminalMenu, TerminalIndex);
    NewTerminalContext  = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;
    for (AttributeIndex = 0; AttributeIndex < sizeof (BaudRateList) / sizeof (BaudRateList [0]); AttributeIndex++) {
      if (NewTerminalContext->BaudRate == (UINT64) (BaudRateList[AttributeIndex].Value)) {
        NewTerminalContext->BaudRateIndex = AttributeIndex;
        break;
      }
    }
    for (AttributeIndex = 0; AttributeIndex < ARRAY_SIZE (DataBitsList); AttributeIndex++) {
      if (NewTerminalContext->DataBits == (UINT64) (DataBitsList[AttributeIndex].Value)) {
        NewTerminalContext->DataBitsIndex = AttributeIndex;
        break;
      }
    }

    for (AttributeIndex = 0; AttributeIndex < ARRAY_SIZE (ParityList); AttributeIndex++) {
      if (NewTerminalContext->Parity == (UINT64) (ParityList[AttributeIndex].Value)) {
        NewTerminalContext->ParityIndex = AttributeIndex;
        break;
      }
    }

    for (AttributeIndex = 0; AttributeIndex < ARRAY_SIZE (StopBitsList); AttributeIndex++) {
      if (NewTerminalContext->StopBits == (UINT64) (StopBitsList[AttributeIndex].Value)) {
        NewTerminalContext->StopBitsIndex = AttributeIndex;
        break;
      }
    }
    CurrentFakeNVMap->COMBaudRate[TerminalIndex]     = NewTerminalContext->BaudRateIndex;
    CurrentFakeNVMap->COMDataRate[TerminalIndex]     = NewTerminalContext->DataBitsIndex;
    CurrentFakeNVMap->COMStopBits[TerminalIndex]     = NewTerminalContext->StopBitsIndex;
    CurrentFakeNVMap->COMParity[TerminalIndex]       = NewTerminalContext->ParityIndex;
    CurrentFakeNVMap->COMTerminalType[TerminalIndex] = NewTerminalContext->TerminalType;
    CurrentFakeNVMap->COMFlowControl[TerminalIndex]  = NewTerminalContext->FlowControl;
  }
}

