/*++
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    ConsoleOption.c
    
Abstract:

    handles console redirection from boot manager


Revision History

--*/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "BootMaint.h"

EFI_DEVICE_PATH_PROTOCOL  *
DevicePathInstanceDup (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevPath
  );

EFI_STATUS
UpdateComAttributeFromVariable (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

EFI_STATUS
ChangeTerminalDevicePath (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  BOOLEAN                   ChangeTerminal
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  EFI_DEVICE_PATH_PROTOCOL  *Node1;
  ACPI_HID_DEVICE_PATH      *Acpi;
  UART_DEVICE_PATH          *Uart;
  UART_DEVICE_PATH          *Uart1;
  UINTN                     Com;
  UINT32                    Match;
  BM_TERMINAL_CONTEXT       *NewTerminalContext;
  BM_MENU_ENTRY             *NewMenuEntry;

  Match = EISA_PNP_ID (0x0501);
  Node  = DevicePath;
  Node  = NextDevicePathNode (Node);
  Com   = 0;
  while (!IsDevicePathEnd (Node)) {
    if ((DevicePathType (Node) == ACPI_DEVICE_PATH) && (DevicePathSubType (Node) == ACPI_DP)) {
      Acpi = (ACPI_HID_DEVICE_PATH *) Node;
      if (CompareMem (&Acpi->HID, &Match, sizeof (UINT32)) == 0) {
        CopyMem (&Com, &Acpi->UID, sizeof (UINT32));
      }
    }

    NewMenuEntry = BOpt_GetMenuEntry (&TerminalMenu, Com);
    if (NULL == NewMenuEntry) {
      return EFI_NOT_FOUND;
    }

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

VOID
ChangeVariableDevicePath (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  ACPI_HID_DEVICE_PATH      *Acpi;
  UART_DEVICE_PATH          *Uart;
  UINTN                     Com;
  UINT32                    Match;
  BM_TERMINAL_CONTEXT       *NewTerminalContext;
  BM_MENU_ENTRY             *NewMenuEntry;

  Match = EISA_PNP_ID (0x0501);
  Node  = DevicePath;
  Node  = NextDevicePathNode (Node);
  Com   = 0;
  while (!IsDevicePathEnd (Node)) {
    if ((DevicePathType (Node) == ACPI_DEVICE_PATH) && (DevicePathSubType (Node) == ACPI_DP)) {
      Acpi = (ACPI_HID_DEVICE_PATH *) Node;
      if (CompareMem (&Acpi->HID, &Match, sizeof (UINT32)) == 0) {
        CopyMem (&Com, &Acpi->UID, sizeof (UINT32));
      }
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

  return ;
}

BOOLEAN
IsTerminalDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  OUT TYPE_OF_TERMINAL         *Termi,
  OUT UINTN                    *Com
  );

EFI_STATUS
LocateSerialIo (
  VOID
  )
/*++

Routine Description:
  Build a list containing all serial devices 
  
Arguments:
  
Returns:
  
--*/
{
  UINT8                     *Ptr;
  UINTN                     Index;
  UINTN                     Index2;
  UINTN                     NoHandles;
  EFI_HANDLE                *Handles;
  EFI_STATUS                Status;
  ACPI_HID_DEVICE_PATH      *Acpi;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINT32                    Match;
  EFI_SERIAL_IO_PROTOCOL    *SerialIo;
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

  for (Index = 0; Index < NoHandles; Index++) {
    //
    // Check to see whether the handle has DevicePath Protocol installed
    //
    gBS->HandleProtocol (
          Handles[Index],
          &gEfiDevicePathProtocolGuid,
          &DevicePath
          );
    Ptr = (UINT8 *) DevicePath;
    while (*Ptr != END_DEVICE_PATH_TYPE) {
      Ptr++;
    }

    Ptr   = Ptr - sizeof (UART_DEVICE_PATH) - sizeof (ACPI_HID_DEVICE_PATH);
    Acpi  = (ACPI_HID_DEVICE_PATH *) Ptr;
    Match = EISA_PNP_ID (0x0501);

    if (CompareMem (&Acpi->HID, &Match, sizeof (UINT32)) == 0) {
      NewMenuEntry = BOpt_CreateMenuEntry (BM_TERMINAL_CONTEXT_SELECT);
      if (!NewMenuEntry) {
        return EFI_OUT_OF_RESOURCES;
      }

      NewTerminalContext = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;
      CopyMem (&NewMenuEntry->OptionNumber, &Acpi->UID, sizeof (UINT32));
      NewTerminalContext->DevicePath = DevicePathInstanceDup (DevicePath);
      //
      // BugBug: I have no choice, calling EfiLibStrFromDatahub will hang the system!
      // coz' the misc data for each platform is not correct, actually it's the device path stored in
      // datahub which is not completed, so a searching for end of device path will enter a
      // dead-loop.
      //
      NewMenuEntry->DisplayString = EfiLibStrFromDatahub (DevicePath);
      if (NULL == NewMenuEntry->DisplayString) {
        NewMenuEntry->DisplayString = DevicePathToStr (DevicePath);
      }

      NewMenuEntry->HelpString = NULL;

      gBS->HandleProtocol (
            Handles[Index],
            &gEfiSerialIoProtocolGuid,
            &SerialIo
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
  //
  // Get L"ConOut", L"ConIn" and L"ErrOut" from the Var
  //
  OutDevicePath = EfiLibGetVariable (L"ConOut", &gEfiGlobalVariableGuid);
  InpDevicePath = EfiLibGetVariable (L"ConIn", &gEfiGlobalVariableGuid);
  ErrDevicePath = EfiLibGetVariable (L"ErrOut", &gEfiGlobalVariableGuid);
  if (OutDevicePath) {
    UpdateComAttributeFromVariable (OutDevicePath);
  }

  if (InpDevicePath) {
    UpdateComAttributeFromVariable (InpDevicePath);
  }

  if (ErrDevicePath) {
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

    for (Index2 = 0; Index2 < 4; Index2++) {
      CopyMem (&Vendor.Guid, &Guid[Index2], sizeof (EFI_GUID));
      SetDevicePathNodeLength (&Vendor.Header, sizeof (VENDOR_DEVICE_PATH));
      NewDevicePath = AppendDevicePathNode (
                        NewTerminalContext->DevicePath,
                        (EFI_DEVICE_PATH_PROTOCOL *) &Vendor
                        );
      SafeFreePool (NewMenuEntry->HelpString);
      //
      // NewMenuEntry->HelpString = DevicePathToStr (NewDevicePath);
      // NewMenuEntry->DisplayString = NewMenuEntry->HelpString;
      //
      NewMenuEntry->HelpString = NULL;

      if (BdsLibMatchDevicePaths (OutDevicePath, NewDevicePath)) {
        NewTerminalContext->IsConOut      = TRUE;
        NewTerminalContext->TerminalType  = (UINT8) Index2;
      }

      if (BdsLibMatchDevicePaths (InpDevicePath, NewDevicePath)) {
        NewTerminalContext->IsConIn       = TRUE;
        NewTerminalContext->TerminalType  = (UINT8) Index2;
      }

      if (BdsLibMatchDevicePaths (ErrDevicePath, NewDevicePath)) {
        NewTerminalContext->IsStdErr      = TRUE;
        NewTerminalContext->TerminalType  = (UINT8) Index2;
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
UpdateComAttributeFromVariable (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
/*++

Routine Description:
  Update Com Ports attributes from DevicePath
  
Arguments:
  DevicePath  -   DevicePath that contains Com ports
  
Returns:
  
--*/
{
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  EFI_DEVICE_PATH_PROTOCOL  *SerialNode;
  ACPI_HID_DEVICE_PATH      *Acpi;
  UART_DEVICE_PATH          *Uart;
  UART_DEVICE_PATH          *Uart1;
  UINT32                    Match;
  UINTN                     TerminalNumber;
  BM_MENU_ENTRY             *NewMenuEntry;
  BM_TERMINAL_CONTEXT       *NewTerminalContext;
  UINTN                     Index;

  Match           = EISA_PNP_ID (0x0501);
  Node            = DevicePath;
  Node            = NextDevicePathNode (Node);
  TerminalNumber  = 0;
  for (Index = 0; Index < TerminalMenu.MenuNumber; Index++) {
    while (!IsDevicePathEnd (Node)) {
      if ((DevicePathType (Node) == ACPI_DEVICE_PATH) && (DevicePathSubType (Node) == ACPI_DP)) {
        Acpi = (ACPI_HID_DEVICE_PATH *) Node;
        if (CompareMem (&Acpi->HID, &Match, sizeof (UINT32)) == 0) {
          CopyMem (&TerminalNumber, &Acpi->UID, sizeof (UINT32));
        }
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

EFI_DEVICE_PATH_PROTOCOL *
DevicePathInstanceDup (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevPath
  )
/*++

Routine Description:
  Function creates a device path data structure that identically matches the 
  device path passed in.

Arguments:
  DevPath      - A pointer to a device path data structure.

Returns:

  The new copy of DevPath is created to identically match the input.  
  Otherwise, NULL is returned.

--*/
{
  EFI_DEVICE_PATH_PROTOCOL  *NewDevPath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathInst;
  EFI_DEVICE_PATH_PROTOCOL  *Temp;
  UINT8                     *Ptr;
  UINTN                     Size;

  //
  // get the size of an instance from the input
  //
  Temp            = DevPath;
  DevicePathInst  = GetNextDevicePathInstance (&Temp, &Size);

  //
  // Make a copy and set proper end type
  //
  NewDevPath = NULL;
  if (Size) {
    NewDevPath = AllocateZeroPool (Size);
    ASSERT (NewDevPath != NULL);
  }

  if (NewDevPath) {
    CopyMem (NewDevPath, DevicePathInst, Size);
    Ptr = (UINT8 *) NewDevPath;
    Ptr += Size - sizeof (EFI_DEVICE_PATH_PROTOCOL);
    Temp = (EFI_DEVICE_PATH_PROTOCOL *) Ptr;
    SetDevicePathEndNode (Temp);
  }

  return NewDevPath;
}

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
  BM_TERMINAL_CONTEXT       *NewTerminalContext;
  TYPE_OF_TERMINAL          Terminal;
  BM_MENU_ENTRY             *NewTerminalMenuEntry;
  UINTN                     Com;
  BM_MENU_OPTION            *ConsoleMenu;

  DevicePath    = NULL;
  AllDevicePath = NULL;
  AllCount      = 0;
  switch (ConsoleMenuType) {
  case BM_CONSOLE_IN_CONTEXT_SELECT:
    ConsoleMenu = &ConsoleInpMenu;
    DevicePath = EfiLibGetVariable (
                  L"ConIn",
                  &gEfiGlobalVariableGuid
                  );

    AllDevicePath = EfiLibGetVariable (
                      L"ConInDev",
                      &gEfiGlobalVariableGuid
                      );
    break;

  case BM_CONSOLE_OUT_CONTEXT_SELECT:
    ConsoleMenu = &ConsoleOutMenu;
    DevicePath = EfiLibGetVariable (
                  L"ConOut",
                  &gEfiGlobalVariableGuid
                  );

    AllDevicePath = EfiLibGetVariable (
                      L"ConOutDev",
                      &gEfiGlobalVariableGuid
                      );
    break;

  case BM_CONSOLE_ERR_CONTEXT_SELECT:
    ConsoleMenu = &ConsoleErrMenu;
    DevicePath = EfiLibGetVariable (
                  L"ErrOut",
                  &gEfiGlobalVariableGuid
                  );

    AllDevicePath = EfiLibGetVariable (
                      L"ErrOutDev",
                      &gEfiGlobalVariableGuid
                      );
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
  // Following is menu building up for Console Out Devices
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

    NewConsoleContext->DevicePath = DevicePathInstanceDup (DevicePathInst);
    NewMenuEntry->DisplayString   = EfiLibStrFromDatahub (NewConsoleContext->DevicePath);
    if (NULL == NewMenuEntry->DisplayString) {
      NewMenuEntry->DisplayString = DevicePathToStr (NewConsoleContext->DevicePath);
    }

    NewConsoleContext->IsTerminal = IsTerminalDevicePath (
                                      NewConsoleContext->DevicePath,
                                      &Terminal,
                                      &Com
                                      );

    NewConsoleContext->IsActive = BdsLibMatchDevicePaths (
                                    DevicePath,
                                    NewConsoleContext->DevicePath
                                    );
    NewTerminalMenuEntry  = NULL;
    NewTerminalContext    = NULL;

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

EFI_STATUS
GetAllConsoles (
  VOID
  )
/*++

Routine Description:
  Build up ConsoleOutMenu, ConsoleInpMenu and ConsoleErrMenu

Arguments:
  
Returns:
  EFI_SUCCESS 
  Others
  
--*/
{
  GetConsoleMenu (BM_CONSOLE_IN_CONTEXT_SELECT);
  GetConsoleMenu (BM_CONSOLE_OUT_CONTEXT_SELECT);
  GetConsoleMenu (BM_CONSOLE_ERR_CONTEXT_SELECT);
  return EFI_SUCCESS;
}

EFI_STATUS
FreeAllConsoles (
  VOID
  )
/*++

Routine Description:
  Free ConsoleOutMenu, ConsoleInpMenu and ConsoleErrMenu

Arguments:
  
Returns:
  EFI_SUCCESS 
  Others
  
--*/
{
  BOpt_FreeMenu (&ConsoleOutMenu);
  BOpt_FreeMenu (&ConsoleInpMenu);
  BOpt_FreeMenu (&ConsoleErrMenu);
  BOpt_FreeMenu (&TerminalMenu);
  return EFI_SUCCESS;
}

BOOLEAN
IsTerminalDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  OUT TYPE_OF_TERMINAL         *Termi,
  OUT UINTN                    *Com
  )
/*++

Routine Description:
  Test whether DevicePath is a valid Terminal

Arguments:
  DevicePath  -   DevicePath to be checked
  Termi       -   If is terminal, give its type
  Com         -   If is Com Port, give its type
  
Returns:
  TRUE        -   If DevicePath point to a Terminal
  FALSE
  
--*/
{
  UINT8                 *Ptr;
  BOOLEAN               IsTerminal;
  VENDOR_DEVICE_PATH    *Vendor;
  ACPI_HID_DEVICE_PATH  *Acpi;
  UINT32                Match;
  EFI_GUID              TempGuid;

  IsTerminal = FALSE;

  //
  // Parse the Device Path, should be change later!!!
  //
  Ptr = (UINT8 *) DevicePath;
  while (*Ptr != END_DEVICE_PATH_TYPE) {
    Ptr++;
  }

  Ptr     = Ptr - sizeof (VENDOR_DEVICE_PATH);
  Vendor  = (VENDOR_DEVICE_PATH *) Ptr;

  //
  // There are four kinds of Terminal types
  // check to see whether this devicepath
  // is one of that type
  //
  CopyMem (&TempGuid, &Vendor->Guid, sizeof (EFI_GUID));

  if (CompareGuid (&TempGuid, &Guid[0])) {
    *Termi      = PC_ANSI;
    IsTerminal  = TRUE;
  } else {
    if (CompareGuid (&TempGuid, &Guid[1])) {
      *Termi      = VT_100;
      IsTerminal  = TRUE;
    } else {
      if (CompareGuid (&TempGuid, &Guid[2])) {
        *Termi      = VT_100_PLUS;
        IsTerminal  = TRUE;
      } else {
        if (CompareGuid (&TempGuid, &Guid[3])) {
          *Termi      = VT_UTF8;
          IsTerminal  = TRUE;
        } else {
          IsTerminal = FALSE;
        }
      }
    }
  }

  if (!IsTerminal) {
    return FALSE;
  }

  Ptr   = Ptr - sizeof (UART_DEVICE_PATH) - sizeof (ACPI_HID_DEVICE_PATH);
  Acpi  = (ACPI_HID_DEVICE_PATH *) Ptr;
  Match = EISA_PNP_ID (0x0501);
  if (CompareMem (&Acpi->HID, &Match, sizeof (UINT32)) == 0) {
    CopyMem (Com, &Acpi->UID, sizeof (UINT32));
  } else {
    return FALSE;
  }

  return TRUE;
}
