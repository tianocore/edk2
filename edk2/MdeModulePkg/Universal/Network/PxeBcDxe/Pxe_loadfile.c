/** @file

Copyright (c) 2004 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  pxe_loadfile.c

Abstract:
  An implementation of the load file protocol for network devices.


**/


#include "Bc.h"

#define DO_MENU     (EFI_SUCCESS)
#define NO_MENU     (DO_MENU + 1)
#define LOCAL_BOOT  (EFI_ABORTED)
#define AUTO_SELECT (NO_MENU)

#define NUMBER_ROWS   25  // we set to mode 0
#define MAX_MENULIST  23

#define Ctl(x)  (0x1F & (x))

typedef union {
  DHCPV4_OP_STRUCT          *OpPtr;
  PXE_BOOT_MENU_ENTRY       *CurrentMenuItemPtr;
  PXE_OP_DISCOVERY_CONTROL  *DiscCtlOpStr;
  PXE_OP_BOOT_MENU          *MenuPtr;
  UINT8                     *BytePtr;
} UNION_PTR;

UINTN  mPropeller;

/**
  PxeBc callback routine for status updates and aborts.

  @param  This                                        Pointer to PxeBcCallback
                                                      interface
  @param  Function                                    PxeBc function ID#
  @param  Received                                    Receive/transmit flag
  @param  PacketLength                                Length of received packet (0
                                                      == idle callback)
  @param  PacketPtr                                   Pointer to received packet
                                                      (NULL == idle callback)

  @retval EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE
                                                      EFI_PXE_BASE_CODE_CALLBACK_STATUS_ABORT
                                                      -

**/
EFI_PXE_BASE_CODE_CALLBACK_STATUS
EFIAPI
bc_callback (
  IN EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL  * This,
  IN EFI_PXE_BASE_CODE_FUNCTION           Function,
  IN BOOLEAN                              Received,
  IN UINT32                               PacketLength,
  IN EFI_PXE_BASE_CODE_PACKET             * PacketPtr OPTIONAL
  )
{
  EFI_INPUT_KEY Key;
  UINTN         Row;
  UINTN         Col;

  mPropeller = 0;
  //
  // Resolve Warning 4 unreferenced parameter problem
  //
  This = This;

  //
  // Check for user abort.
  //
  if (gST->ConIn->ReadKeyStroke (gST->ConIn, &Key) == EFI_SUCCESS) {
    if (!Key.ScanCode) {
      if (Key.UnicodeChar == Ctl ('c')) {
        return EFI_PXE_BASE_CODE_CALLBACK_STATUS_ABORT;
      }
    } else if (Key.ScanCode == SCAN_ESC) {
      return EFI_PXE_BASE_CODE_CALLBACK_STATUS_ABORT;
    }
  }
  //
  // Do nothing if this is a receive.
  //
  if (Received) {
    return EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE;
  }
  //
  // The display code is only for these functions.
  //
  switch (Function) {
  case EFI_PXE_BASE_CODE_FUNCTION_MTFTP:
    //
    // If this is a transmit and not a M/TFTP open request,
    // return now.  Do not print a dot for each M/TFTP packet
    // that is sent, only for the open packets.
    //
    if (PacketLength != 0 && PacketPtr != NULL) {
      if (PacketPtr->Raw[0x1C] != 0x00 || PacketPtr->Raw[0x1D] != 0x01) {
        return EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE;
      }
    }

    break;

  case EFI_PXE_BASE_CODE_FUNCTION_DHCP:
  case EFI_PXE_BASE_CODE_FUNCTION_DISCOVER:
    break;

  default:
    return EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE;
  }
  //
  // Display routines
  //
  if (PacketLength != 0 && PacketPtr != NULL) {
    //
    // Display a '.' when a packet is transmitted.
    //
    AsciiPrint (".");
  } else if (PacketLength == 0 && PacketPtr == NULL) {
    //
    // Display a propeller when waiting for packets if at
    // least 200 ms have passed.
    //
    Row = gST->ConOut->Mode->CursorRow;
    Col = gST->ConOut->Mode->CursorColumn;

    AsciiPrint ("%c", "/-\\|"[mPropeller]);
    gST->ConOut->SetCursorPosition (gST->ConOut, Col, Row);

    mPropeller = (mPropeller + 1) & 3;
  }

  return EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE;
}

EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL  _bc_callback = {
  EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL_REVISION,
  &bc_callback
};


/**
  Display an IPv4 address in dot notation.

  @param  Ptr                                         Pointer to IPv4 address.

  @return None

**/
VOID
PrintIpv4 (
  UINT8 *Ptr
  )
{
  if (Ptr != NULL) {
    AsciiPrint ("%d.%d.%d.%d", Ptr[0], Ptr[1], Ptr[2], Ptr[3]);
  }
}


/**
  Display client and server IP information.

  @param  Private                                     Pointer to PxeBc interface

  @return None

**/
VOID
ShowMyInfo (
  IN PXE_BASECODE_DEVICE *Private
  )
{
  EFI_PXE_BASE_CODE_MODE  *PxeBcMode;
  UINTN                   Index;

  //
  // Do nothing if a NULL pointer is passed in.
  //
  if (Private == NULL) {
    return ;
  }
  //
  // Get pointer to PXE BaseCode mode structure
  //
  PxeBcMode = Private->EfiBc.Mode;

  //
  // Display client IP address
  //
  AsciiPrint ("\rCLIENT IP: ");
  PrintIpv4 (PxeBcMode->StationIp.v4.Addr);

  //
  // Display subnet mask
  //
  AsciiPrint ("  MASK: ");
  PrintIpv4 (PxeBcMode->SubnetMask.v4.Addr);

  //
  // Display DHCP and proxyDHCP IP addresses
  //
  if (PxeBcMode->ProxyOfferReceived) {
    AsciiPrint ("\nDHCP IP: ");
    PrintIpv4 (((DHCPV4_OP_SERVER_IP *) DHCPV4_ACK_BUFFER.OpAdds.PktOptAdds[OP_DHCP_SERVER_IP_IX - 1])->Ip.Addr);

    AsciiPrint ("  PROXY IP: ");
    PrintIpv4 (((DHCPV4_OP_SERVER_IP *) PXE_OFFER_BUFFER.OpAdds.PktOptAdds[OP_DHCP_SERVER_IP_IX - 1])->Ip.Addr);
  } else {
    AsciiPrint ("  DHCP IP: ");
    PrintIpv4 (((DHCPV4_OP_SERVER_IP *) DHCPV4_ACK_BUFFER.OpAdds.PktOptAdds[OP_DHCP_SERVER_IP_IX - 1])->Ip.Addr);
  }
  //
  // Display gateway IP addresses
  //
  for (Index = 0; Index < PxeBcMode->RouteTableEntries; ++Index) {
    if ((Index % 3) == 0) {
      AsciiPrint ("\r\nGATEWAY IP:");
    }

    AsciiPrint (" ");
    PrintIpv4 (PxeBcMode->RouteTable[Index].GwAddr.v4.Addr);
    AsciiPrint (" ");
  }

  AsciiPrint ("\n");
}


/**
  Display prompt and wait for input.

  @param  Private                                     Pointer to PxeBc interface
  @param  BootPromptPtr                               Pointer to PXE boot prompt
                                                      option

  @retval AUTO_SELECT                                 DO_MENU -
  @retval NO_MENU
  @retval LOCAL_BOOT

**/
EFI_STATUS
DoPrompt (
  PXE_BASECODE_DEVICE *Private,
  PXE_OP_BOOT_PROMPT  *BootPromptPtr
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   TimeoutEvent;
  EFI_EVENT   SecondsEvent;
  INT32       SecColumn;
  INT32       SecRow;
  UINT8       SaveChar;
  UINT8       SecsLeft;

  //
  // if auto select, just get right to it
  //
  if (BootPromptPtr->Timeout == PXE_BOOT_PROMPT_AUTO_SELECT) {
    return AUTO_SELECT;
  }
  //
  // if no timeout, go directly to display of menu
  //
  if (BootPromptPtr->Timeout == PXE_BOOT_PROMPT_NO_TIMEOUT) {
    return DO_MENU;
  }
  //
  //
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &TimeoutEvent
                  );

  if (EFI_ERROR (Status)) {
    return DO_MENU;
  }

  Status = gBS->SetTimer (
                  TimeoutEvent,
                  TimerRelative,
                  BootPromptPtr->Timeout * 10000000 + 100000
                  );

  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (TimeoutEvent);
    return DO_MENU;
  }
  //
  //
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &SecondsEvent
                  );

  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (TimeoutEvent);
    return DO_MENU;
  }

  Status = gBS->SetTimer (
                  SecondsEvent,
                  TimerPeriodic,
                  10000000
                  );  /* 1 second */

  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (SecondsEvent);
    gBS->CloseEvent (TimeoutEvent);
    return DO_MENU;
  }
  //
  // display the prompt
  // IMPORTANT!  This prompt is an ASCII character string that may
  // not be terminated with a NULL byte.
  //
  SaveChar  = BootPromptPtr->Prompt[BootPromptPtr->Header.Length - 1];
  BootPromptPtr->Prompt[BootPromptPtr->Header.Length - 1] = 0;

  AsciiPrint ("%a ", BootPromptPtr->Prompt);
  BootPromptPtr->Prompt[BootPromptPtr->Header.Length - 1] = SaveChar;

  //
  // wait until time expires or selection made - menu or local
  //
  SecColumn = gST->ConOut->Mode->CursorColumn;
  SecRow    = gST->ConOut->Mode->CursorRow;
  SecsLeft  = BootPromptPtr->Timeout;

  gST->ConOut->SetCursorPosition (gST->ConOut, SecColumn, SecRow);
  AsciiPrint ("(%d) ", SecsLeft);

  //
  // set the default action to be AUTO_SELECT
  //
  Status = AUTO_SELECT;

  while (EFI_ERROR (gBS->CheckEvent (TimeoutEvent))) {
    EFI_INPUT_KEY Key;

    if (!EFI_ERROR (gBS->CheckEvent (SecondsEvent))) {
      --SecsLeft;
      gST->ConOut->SetCursorPosition (gST->ConOut, SecColumn, SecRow);
      AsciiPrint ("(%d) ", SecsLeft);
    }

    if (gST->ConIn->ReadKeyStroke (gST->ConIn, &Key) == EFI_NOT_READY) {
      UINT8       Buffer[512];
      UINTN       BufferSize;

      BufferSize = sizeof Buffer;

      Status = Private->EfiBc.UdpRead (
                                &Private->EfiBc,
                                EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_IP |
                                EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT |
                                EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_PORT,
                                NULL, /* dest ip */
                                NULL, /* dest port */
                                NULL, /* src ip */
                                NULL, /* src port */
                                NULL, /* hdr size */
                                NULL, /* hdr ptr */
                                &BufferSize,
                                Buffer
                                );

      continue;
    }

    if (Key.ScanCode == 0) {
      switch (Key.UnicodeChar) {
      case Ctl ('c'):
        Status = LOCAL_BOOT;
        break;

      case Ctl ('m'):
      case 'm':
      case 'M':
        Status = DO_MENU;
        break;

      default:
        continue;
      }
    } else {
      switch (Key.ScanCode) {
      case SCAN_F8:
        Status = DO_MENU;
        break;

      case SCAN_ESC:
        Status = LOCAL_BOOT;
        break;

      default:
        continue;
      }
    }

    break;
  }

  gBS->CloseEvent (SecondsEvent);
  gBS->CloseEvent (TimeoutEvent);

  gST->ConOut->SetCursorPosition (gST->ConOut, SecColumn, SecRow);
  AsciiPrint ("     ");

  return Status;
}


/**
  Display one menu item.

  @param  MenuItemPtr                                 Pointer to PXE menu item
                                                      option.

  @return None

**/
VOID
PrintMenuItem (
  PXE_BOOT_MENU_ENTRY *MenuItemPtr
  )
{
  UINT8 Length;
  UINT8 SaveChar;

  Length                    = (UINT8) MIN (70, MenuItemPtr->DataLen);
  SaveChar                  = MenuItemPtr->Data[Length];

  MenuItemPtr->Data[Length] = 0;
  AsciiPrint ("     %a\n", MenuItemPtr->Data);
  MenuItemPtr->Data[Length] = SaveChar;
}


/**
  Display and process menu.

  @param  Private                                     Pointer to PxeBc interface
  @param  RxBufferPtr                                 Pointer to receive buffer

  @retval NO_MENU
  @retval LOCAL_BOOT

**/
EFI_STATUS
DoMenu (
  PXE_BASECODE_DEVICE *Private,
  DHCP_RECEIVE_BUFFER *RxBufferPtr
  )
{
  PXE_OP_DISCOVERY_CONTROL  *DiscoveryControlPtr;
  PXE_BOOT_MENU_ENTRY       *MenuItemPtrs[MAX_MENULIST];
  EFI_STATUS                Status;
  UNION_PTR                 Ptr;
  UINTN                     SaveNumRte;
  UINTN                     TopRow;
  UINTN                     MenuLth;
  UINTN                     NumMenuItems;
  UINTN                     Index;
  UINTN                     Longest;
  UINTN                     Selected;
  UINT16                    Type;
  UINT16                    Layer;
  BOOLEAN                   Done;

  Selected  = 0;
  Layer     = 0;

  DEBUG ((DEBUG_WARN, "\nDoMenu()  Enter."));

  /* see if we have a menu/prompt */
  if (!(RxBufferPtr->OpAdds.Status & DISCOVER_TYPE)) {
    DEBUG (
      (DEBUG_WARN,
      "\nDoMenu()  No menu/prompt info.  OpAdds.Status == %xh  ",
      RxBufferPtr->OpAdds.Status)
      );

    return NO_MENU;
  }

  DiscoveryControlPtr = (PXE_OP_DISCOVERY_CONTROL *) RxBufferPtr->OpAdds.PxeOptAdds[VEND_PXE_DISCOVERY_CONTROL_IX - 1];

  //
  // if not USE_BOOTFILE or no bootfile given, must have menu stuff
  //
  if ((DiscoveryControlPtr->ControlBits & USE_BOOTFILE) && RxBufferPtr->OpAdds.PktOptAdds[OP_DHCP_BOOTFILE_IX - 1]) {
    DEBUG ((DEBUG_WARN, "\nDoMenu()  DHCP w/ bootfile.  "));
    return NO_MENU;
  }
  //
  // do prompt & menu if necessary
  //
  Status = DoPrompt (Private, (PXE_OP_BOOT_PROMPT *) RxBufferPtr->OpAdds.PxeOptAdds[VEND_PXE_BOOT_PROMPT_IX - 1]);

  if (Status == LOCAL_BOOT) {
    DEBUG ((DEBUG_WARN, "\nDoMenu()  DoPrompt() returned LOCAL_BOOT.  "));

    return Status;
  }

  Ptr.BytePtr             = (UINT8 *) RxBufferPtr->OpAdds.PxeOptAdds[VEND_PXE_BOOT_MENU_IX - 1];

  MenuLth                 = Ptr.MenuPtr->Header.Length;
  Ptr.CurrentMenuItemPtr  = Ptr.MenuPtr->MenuItem;

  //
  // build menu items array
  //
  for (Longest = NumMenuItems = Index = 0; Index < MenuLth && NumMenuItems < MAX_MENULIST;) {
    UINTN lth;

    lth = Ptr.CurrentMenuItemPtr->DataLen + sizeof (*Ptr.CurrentMenuItemPtr) - sizeof (Ptr.CurrentMenuItemPtr->Data);

    MenuItemPtrs[NumMenuItems++] = Ptr.CurrentMenuItemPtr;

    if (lth > Longest) {
      //
      // check if too long
      //
      if ((Longest = lth) > 70 + (sizeof (*Ptr.CurrentMenuItemPtr) - sizeof (Ptr.CurrentMenuItemPtr->Data))) {
        Longest = 70 + (sizeof (*Ptr.CurrentMenuItemPtr) - sizeof (Ptr.CurrentMenuItemPtr->Data));
      }
    }

    Index += lth;
    Ptr.BytePtr += lth;
  }

  if (Status != AUTO_SELECT) {
    UINT8 BlankBuf[75];

    SetMem (BlankBuf, sizeof BlankBuf, ' ');
    BlankBuf[Longest + 5 - (sizeof (*Ptr.CurrentMenuItemPtr) - sizeof (Ptr.CurrentMenuItemPtr->Data))] = 0;
    AsciiPrint ("\n");

    //
    // now put up menu
    //
    for (Index = 0; Index < NumMenuItems; ++Index) {
      PrintMenuItem (MenuItemPtrs[Index]);
    }

    TopRow = gST->ConOut->Mode->CursorRow - NumMenuItems;

    //
    // now wait for a selection
    //
    Done = FALSE;
    do {
      //
      // highlight selection
      //
      EFI_INPUT_KEY Key;
      UINTN         NewSelected;

      NewSelected = Selected;

      //
      // highlight selected row
      //
      gST->ConOut->SetAttribute (
                    gST->ConOut,
                    EFI_TEXT_ATTR (EFI_BLACK, EFI_LIGHTGRAY)
                    );
      gST->ConOut->SetCursorPosition (gST->ConOut, 0, TopRow + Selected);

      AsciiPrint (" --->%a\r", BlankBuf);

      PrintMenuItem (MenuItemPtrs[Selected]);
      gST->ConOut->SetAttribute (
                    gST->ConOut,
                    EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK)
                    );
      gST->ConOut->SetCursorPosition (gST->ConOut, 0, TopRow + NumMenuItems);

      //
      // wait for a keystroke
      //
      while (gST->ConIn->ReadKeyStroke (gST->ConIn, &Key) == EFI_NOT_READY) {
        UINT8 TmpBuf[512];
        UINTN TmpBufLen;

        TmpBufLen = sizeof TmpBuf;

        Private->EfiBc.UdpRead (
                        &Private->EfiBc,
                        EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_IP |
                        EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT |
                        EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_PORT,
                        NULL, /* dest ip */
                        NULL, /* dest port */
                        NULL, /* src ip */
                        NULL, /* src port */
                        NULL, /* hdr size */
                        NULL, /* hdr ptr */
                        &TmpBufLen,
                        TmpBuf
                        );
      }

      if (!Key.ScanCode) {
        switch (Key.UnicodeChar) {
        case Ctl ('c'):
          Key.ScanCode = SCAN_ESC;
          break;

        case Ctl ('j'): /* linefeed */
        case Ctl ('m'): /* return */
          Done = TRUE;
          break;

        case Ctl ('i'): /* tab */
        case ' ':
        case 'd':
        case 'D':
          Key.ScanCode = SCAN_DOWN;
          break;

        case Ctl ('h'): /* backspace */
        case 'u':
        case 'U':
          Key.ScanCode = SCAN_UP;
          break;

        default:
          Key.ScanCode = 0;
        }
      }

      switch (Key.ScanCode) {
      case SCAN_LEFT:
      case SCAN_UP:
        if (NewSelected) {
          --NewSelected;
        }

        break;

      case SCAN_DOWN:
      case SCAN_RIGHT:
        if (++NewSelected == NumMenuItems) {
          --NewSelected;
        }

        break;

      case SCAN_PAGE_UP:
      case SCAN_HOME:
        NewSelected = 0;
        break;

      case SCAN_PAGE_DOWN:
      case SCAN_END:
        NewSelected = NumMenuItems - 1;
        break;

      case SCAN_ESC:
        return LOCAL_BOOT;
      }

      /* unhighlight last selected row */
      gST->ConOut->SetCursorPosition (gST->ConOut, 5, TopRow + Selected);

      AsciiPrint ("%a\r", BlankBuf);

      PrintMenuItem (MenuItemPtrs[Selected]);

      Selected = NewSelected;
    } while (!Done);
  }

  SaveNumRte  = Private->EfiBc.Mode->RouteTableEntries;

  Type        = NTOHS (MenuItemPtrs[Selected]->Type);

  if (Type == 0) {
    DEBUG ((DEBUG_WARN, "\nDoMenu()  Local boot selected.  "));
    return LOCAL_BOOT;
  }

  AsciiPrint ("Discover");

  Status = Private->EfiBc.Discover (
                            &Private->EfiBc,
                            Type,
                            &Layer,
                            (BOOLEAN) (Private->EfiBc.Mode->BisSupported && Private->EfiBc.Mode->BisDetected),
                            0
                            );

  if (EFI_ERROR (Status)) {
    AsciiPrint ("\r                    \r");

    DEBUG (
      (DEBUG_WARN,
      "\nDoMenu()  Return w/ %xh (%r).",
      Status,
      Status)
      );

    return Status;
  }

  AsciiPrint ("\rBOOT_SERVER_IP: ");
  PrintIpv4 ((UINT8 *) &Private->ServerIp);

  for (Index = SaveNumRte; Index < Private->EfiBc.Mode->RouteTableEntries; ++Index) {
    if ((Index % 3) == 0) {
      AsciiPrint ("\r\nGATEWAY IP:");
    }

    AsciiPrint (" ");
    PrintIpv4 ((UINT8 *) &Private->EfiBc.Mode->RouteTable[Index].GwAddr);
    AsciiPrint (" ");
  }

  AsciiPrint ("\n");

  DEBUG ((DEBUG_WARN, "\nDoMenu()  Return w/ EFI_SUCCESS.  "));

  return EFI_SUCCESS;
}


/**
  Get value 8- or 16-bit value from DHCP option.

  @param  OpPtr                                       Pointer to DHCP option

  @return Value from DHCP option

**/
UINT16
GetValue (
  DHCPV4_OP_STRUCT *OpPtr
  )
{
  if (OpPtr->Header.Length == 1) {
    return OpPtr->Data[0];
  } else {
    return NTOHS (OpPtr->Data);
  }
}


/**
  Locate opcode in buffer.

  @param  BufferPtr                                   Pointer to buffer
  @param  BufferLen                                   Length of buffer
  @param  OpCode                                      Option number

  @return Pointer to opcode, may be NULL

**/
UINT8 *
_PxeBcFindOpt (
  UINT8 *BufferPtr,
  UINTN BufferLen,
  UINT8 OpCode
  )
{
  if (BufferPtr == NULL) {
    return NULL;
  }

  while (BufferLen != 0) {
    if (*BufferPtr == OpCode) {
      return BufferPtr;
    }

    switch (*BufferPtr) {
    case OP_END:
      return NULL;

    case OP_PAD:
      ++BufferPtr;
      --BufferLen;
      continue;
    }

    if ((UINTN) BufferLen <= (UINTN) 2 + BufferPtr[1]) {
      return NULL;
    }

    BufferLen -= 2 + BufferPtr[1];
    BufferPtr += 2 + BufferPtr[1];
  }

  return NULL;
}


/**
  Find option in packet

  @param  PacketPtr                                   Pointer to packet
  @param  OpCode                                      option number

  @return Pointer to option in packet

**/
UINT8 *
PxeBcFindDhcpOpt (
  EFI_PXE_BASE_CODE_PACKET  *PacketPtr,
  UINT8                     OpCode
  )
{
  UINTN PacketLen;
  UINT8 Overload;
  UINT8 *OptionBufferPtr;

  //
  //
  //
  PacketLen = 380;
  Overload  = 0;

  //
  // Figure size of DHCP option space.
  //
  OptionBufferPtr = _PxeBcFindOpt (
                      PacketPtr->Dhcpv4.DhcpOptions,
                      380,
                      OP_DHCP_MAX_MESSAGE_SZ
                      );

  if (OptionBufferPtr != NULL) {
    if (OptionBufferPtr[1] == 2) {
      UINT16  n;

      CopyMem (&n, &OptionBufferPtr[2], 2);
      PacketLen = HTONS (n);

      if (PacketLen < sizeof (EFI_PXE_BASE_CODE_DHCPV4_PACKET)) {
        PacketLen = 380;
      } else {
        PacketLen -= (PacketPtr->Dhcpv4.DhcpOptions - &PacketPtr->Dhcpv4.BootpOpcode) + 28;
      }
    }
  }
  //
  // Look for option overloading.
  //
  OptionBufferPtr = _PxeBcFindOpt (
                      PacketPtr->Dhcpv4.DhcpOptions,
                      PacketLen,
                      OP_DHCP_OPTION_OVERLOAD
                      );

  if (OptionBufferPtr != NULL) {
    if (OptionBufferPtr[1] == 1) {
      Overload = OptionBufferPtr[2];
    }
  }
  //
  // Look for caller's option.
  //
  OptionBufferPtr = _PxeBcFindOpt (
                      PacketPtr->Dhcpv4.DhcpOptions,
                      PacketLen,
                      OpCode
                      );

  if (OptionBufferPtr != NULL) {
    return OptionBufferPtr;
  }

  if (Overload & OVLD_FILE) {
    OptionBufferPtr = _PxeBcFindOpt (PacketPtr->Dhcpv4.BootpBootFile, 128, OpCode);

    if (OptionBufferPtr != NULL) {
      return OptionBufferPtr;
    }
  }

  if (Overload & OVLD_SRVR_NAME) {
    OptionBufferPtr = _PxeBcFindOpt (PacketPtr->Dhcpv4.BootpSrvName, 64, OpCode);

    if (OptionBufferPtr != NULL) {
      return OptionBufferPtr;
    }
  }

  return NULL;
}


/**
  Download file into buffer

  @param  Private                                     Pointer to PxeBc interface
  @param  BufferSize                                  pointer to size of download
                                                      buffer
  @param  Buffer                                      Pointer to buffer

  @return EFI_BUFFER_TOO_SMALL -
  @return EFI_NOT_FOUND -
  @return EFI_PROTOCOL_ERROR -

**/
EFI_STATUS
DownloadFile (
  IN PXE_BASECODE_DEVICE  *Private,
  IN OUT UINT64           *BufferSize,
  IN VOID                 *Buffer
  )
{
  EFI_PXE_BASE_CODE_MTFTP_INFO  MtftpInfo;
  EFI_PXE_BASE_CODE_TFTP_OPCODE OpCode;
  DHCP_RECEIVE_BUFFER           *RxBuf;
  EFI_STATUS                    Status;
  UINTN                         BlockSize;

  RxBuf     = (DHCP_RECEIVE_BUFFER *) Private->BootServerReceiveBuffer;
  BlockSize = 0x8000;

  DEBUG ((EFI_D_WARN, "\nDownloadFile()  Enter."));

  if (Buffer == NULL || *BufferSize == 0 || *BufferSize < Private->FileSize) {
    if (Private->FileSize != 0) {
      *BufferSize = Private->FileSize;
      return EFI_BUFFER_TOO_SMALL;
    }

    AsciiPrint ("\nTSize");

    OpCode = EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE;
  } else if (RxBuf->OpAdds.Status & WfM11a_TYPE) {
    OpCode = EFI_PXE_BASE_CODE_MTFTP_READ_FILE;

    ZeroMem (&MtftpInfo, sizeof MtftpInfo);

    *(IPV4_ADDR *) &MtftpInfo.MCastIp = *(IPV4_ADDR *) RxBuf->OpAdds.PxeOptAdds[VEND_PXE_MTFTP_IP - 1]->Data;

    CopyMem (
      &MtftpInfo.CPort,
      RxBuf->OpAdds.PxeOptAdds[VEND_PXE_MTFTP_CPORT - 1]->Data,
      sizeof MtftpInfo.CPort
      );

    CopyMem (
      &MtftpInfo.SPort,
      RxBuf->OpAdds.PxeOptAdds[VEND_PXE_MTFTP_SPORT - 1]->Data,
      sizeof MtftpInfo.SPort
      );

    MtftpInfo.ListenTimeout   = GetValue (RxBuf->OpAdds.PxeOptAdds[VEND_PXE_MTFTP_TMOUT - 1]);

    MtftpInfo.TransmitTimeout = GetValue (RxBuf->OpAdds.PxeOptAdds[VEND_PXE_MTFTP_DELAY - 1]);

    AsciiPrint ("\nMTFTP");
  } else {
    AsciiPrint ("\nTFTP");

    OpCode = EFI_PXE_BASE_CODE_TFTP_READ_FILE;
  }

  Private->FileSize = 0;

  RxBuf->OpAdds.PktOptAdds[OP_DHCP_BOOTFILE_IX - 1]->Data[RxBuf->OpAdds.PktOptAdds[OP_DHCP_BOOTFILE_IX - 1]->Header.Length] = 0;

  Status = Private->EfiBc.Mtftp (
                            &Private->EfiBc,
                            OpCode,
                            Buffer,
                            FALSE,
                            BufferSize,
                            &BlockSize,
                            &Private->ServerIp,
                            (UINT8 *) RxBuf->OpAdds.PktOptAdds[OP_DHCP_BOOTFILE_IX - 1]->Data,
                            &MtftpInfo,
                            FALSE
                            );

  if (Status != EFI_SUCCESS && Status != EFI_BUFFER_TOO_SMALL) {
    DEBUG ((DEBUG_WARN, "\nDownloadFile()  Exit #1 %Xh", Status));
    return Status;
  }

  if (sizeof (UINTN) < sizeof (UINT64) && *BufferSize > 0xFFFFFFFF) {
    Private->FileSize = 0xFFFFFFFF;
  } else {
    Private->FileSize = (UINTN) *BufferSize;
  }

  if (OpCode == EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE) {
    DEBUG ((DEBUG_WARN, "\nDownloadFile()  Exit #2"));
    return EFI_BUFFER_TOO_SMALL;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "\nDownloadFile()  Exit #3 %Xh", Status));
    return Status;
  }

  if (Private->EfiBc.Mode->BisSupported && Private->EfiBc.Mode->BisDetected && Private->EfiBc.Mode->PxeBisReplyReceived) {
    UINT64  CredentialLen;
    UINT8   CredentialFilename[256];
    UINT8   *op;
    VOID    *CredentialBuffer;

    //
    // Get name of credential file.  It may be in the BOOTP
    // bootfile field or a DHCP option.
    //
    ZeroMem (CredentialFilename, sizeof CredentialFilename);

    op = PxeBcFindDhcpOpt (&Private->EfiBc.Mode->PxeBisReply, OP_DHCP_BOOTFILE);

    if (op != NULL) {
      if (op[1] == 0) {
        /* No credential filename */
        return EFI_NOT_FOUND;
      }

      CopyMem (CredentialFilename, &op[2], op[1]);
    } else {
      if (Private->EfiBc.Mode->PxeBisReply.Dhcpv4.BootpBootFile[0] == 0) {
        /* No credential filename */
        return EFI_NOT_FOUND;
      }

      CopyMem (CredentialFilename, &op[2], 128);
    }
    //
    // Get size of credential file.  It may be available as a
    // DHCP option.  If not, use the TFTP get file size.
    //
    CredentialLen = 0;

    op            = PxeBcFindDhcpOpt (&Private->EfiBc.Mode->PxeBisReply, OP_BOOT_FILE_SZ);

    if (op != NULL) {
      /*
       * This is actually the size of the credential file
       * buffer.  The actual credential file size will be
       * returned when we download the file.
       */
      if (op[1] == 2) {
        UINT16  n;

        CopyMem (&n, &op[2], 2);
        CredentialLen = HTONS (n) * 512;
      }
    }

    if (CredentialLen == 0) {
      BlockSize = 8192;

      Status = Private->EfiBc.Mtftp (
                                &Private->EfiBc,
                                EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE,
                                NULL,
                                FALSE,
                                &CredentialLen,
                                &BlockSize,
                                &Private->ServerIp,
                                CredentialFilename,
                                NULL,
                                FALSE
                                );

      if (EFI_ERROR (Status)) {
        return Status;
      }

      if (CredentialLen == 0) {
        //
        // %%TBD -- EFI error for invalid credential
        // file.
        //
        return EFI_PROTOCOL_ERROR;
      }
    }
    //
    // Allocate credential file buffer.
    //
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    (UINTN) CredentialLen,
                    &CredentialBuffer
                    );

    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // Download credential file.
    //
    BlockSize = 8192;

    Status = Private->EfiBc.Mtftp (
                              &Private->EfiBc,
                              EFI_PXE_BASE_CODE_TFTP_READ_FILE,
                              CredentialBuffer,
                              FALSE,
                              &CredentialLen,
                              &BlockSize,
                              &Private->ServerIp,
                              CredentialFilename,
                              NULL,
                              FALSE
                              );

    if (EFI_ERROR (Status)) {
      gBS->FreePool (CredentialBuffer);
      return Status;
    }
    //
    // Verify credentials.
    //
    if (PxebcBisVerify (Private, Buffer, Private->FileSize, CredentialBuffer, (UINTN) CredentialLen)) {
      Status = EFI_SUCCESS;
    } else {
      //
      // %%TBD -- An EFI error code for failing credential verification.
      //
      Status = EFI_PROTOCOL_ERROR;
    }

    gBS->FreePool (CredentialBuffer);
  }

  return Status;
}


/**
  Start PXE DHCP.  Get DHCP and proxyDHCP information.
  Display remote boot menu and prompt.  Select item from menu.

  @param  Private                                     Pointer to PxeBc interface
  @param  BufferSize                                  Pointer to download buffer
                                                      size
  @param  Buffer                                      Pointer to download buffer

  @retval EFI_SUCCESS
  @retval EFI_NOT_READY

**/
EFI_STATUS
LoadfileStart (
  IN PXE_BASECODE_DEVICE  *Private,
  IN OUT UINT64           *BufferSize,
  IN VOID                 *Buffer
  )
{
  EFI_PXE_BASE_CODE_MODE      *PxeBcMode;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;
  EFI_SIMPLE_NETWORK_MODE     *SnpMode;
  EFI_STATUS                  Status;
  VOID                        *RxBuf;

  DEBUG ((DEBUG_WARN, "\nLoadfileStart()  Enter."));

  //
  // Try to start BaseCode, for now only IPv4 is supported
  // so don't try to start using IPv6.
  //
  Status = Private->EfiBc.Start (&Private->EfiBc, FALSE);

  if (EFI_ERROR (Status)) {
    if (Status != EFI_ALREADY_STARTED) {
      DEBUG ((DEBUG_NET, "\nLoadfileStart()  Exit  BC.Start() == %xh", Status));
      return Status;
    }
  }
  //
  // Get pointers to PXE mode structure, SNP protocol structure
  // and SNP mode structure.
  //
  PxeBcMode = Private->EfiBc.Mode;
  Snp       = Private->SimpleNetwork;
  SnpMode   = Snp->Mode;

  //
  // Display client MAC address, like 16-bit PXE ROMs
  //
  AsciiPrint ("\nCLIENT MAC ADDR: ");

  {
    UINTN Index;
    UINTN hlen;

    hlen = SnpMode->HwAddressSize;

    for (Index = 0; Index < hlen; ++Index) {
      AsciiPrint ("%02x ", SnpMode->CurrentAddress.Addr[Index]);
    }
  }

  AsciiPrint ("\nDHCP");

  Status = Private->EfiBc.Dhcp (&Private->EfiBc, TRUE);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "\nLoadfileStart()  Exit  BC.Dhcp() == %Xh", Status));
    AsciiPrint ("\r               \r");
    return Status;
  }

  ShowMyInfo (Private);

  RxBuf = PxeBcMode->ProxyOfferReceived ? &PXE_OFFER_BUFFER : &DHCPV4_ACK_BUFFER;
#define RxBufferPtr ((DHCP_RECEIVE_BUFFER *) RxBuf)

  Status = DoMenu (Private, RxBufferPtr);

  if (Status == EFI_SUCCESS) {
    //
    // did a discovery - take info from discovery packet
    //
    RxBuf = &PXE_ACK_BUFFER;
  } else if (Status == NO_MENU) {
    //
    // did not do a discovery - take info from rxbuf
    //
    Private->ServerIp.Addr[0] = RxBufferPtr->u.Dhcpv4.siaddr;

    if (!(Private->ServerIp.Addr[0])) {
      *(IPV4_ADDR *) &Private->ServerIp = *(IPV4_ADDR *) RxBufferPtr->OpAdds.PktOptAdds[OP_DHCP_SERVER_IP_IX - 1]->Data;
    }
  } else {
    DEBUG ((DEBUG_WARN, "\nLoadfileStart()  Exit  DoMenu() == %Xh", Status));
    return Status;
  }

  if (!RxBufferPtr->OpAdds.PktOptAdds[OP_DHCP_BOOTFILE_IX - 1]) {
    DEBUG ((DEBUG_WARN, "\nLoadfileStart()  Exit  Not ready?"));
    return EFI_NOT_READY;
  }
  //
  // check for file size option sent
  //
  if (RxBufferPtr->OpAdds.PktOptAdds[OP_BOOT_FILE_SZ_IX - 1]) {
    Private->FileSize = 512 * NTOHS (RxBufferPtr->OpAdds.PktOptAdds[OP_BOOT_FILE_SZ_IX - 1]->Data);
  }

  Private->BootServerReceiveBuffer  = RxBufferPtr;

  Status = DownloadFile (Private, BufferSize, Buffer);

  DEBUG (
    (DEBUG_WARN,
    "\nLoadfileStart()  Exit.  DownloadFile() = %Xh",
    Status)
    );

  return Status;
}


/**
  Loadfile interface for PxeBc interface

  @param  This                                        Pointer to Loadfile interface
  @param  FilePath                                    Not used and not checked
  @param  BootPolicy                                  Must be TRUE
  @param  BufferSize                                  Pointer to buffer size
  @param  Buffer                                      Pointer to download buffer or
                                                      NULL

  @return EFI_INVALID_PARAMETER -
  @return EFI_UNSUPPORTED -
  @return EFI_SUCCESS -
  @return EFI_BUFFER_TOO_SMALL -

**/
EFI_STATUS
EFIAPI
LoadFile (
  IN EFI_LOAD_FILE_PROTOCOL           *This,
  IN EFI_DEVICE_PATH_PROTOCOL         *FilePath,
  IN BOOLEAN                          BootPolicy,
  IN OUT UINTN                        *BufferSize,
  IN OUT VOID                         *Buffer
  )
{
  LOADFILE_DEVICE *LoadfilePtr;
  UINT64          TmpBufSz;
  INT32           OrigMode;
  INT32           OrigAttribute;
  BOOLEAN         RemoveCallback;
  BOOLEAN         NewMakeCallback;
  EFI_STATUS      Status;
  EFI_STATUS      TempStatus;

  //
  // The following line is only used for passing ICC build.
  //
  DEBUG ((EFI_D_INFO, "FilePath = %p\n", FilePath));

  //
  //
  //
  OrigMode        = gST->ConOut->Mode->Mode;
  OrigAttribute   = gST->ConOut->Mode->Attribute;
  RemoveCallback  = FALSE;

  AsciiPrint ("Running LoadFile()\n");

  //
  // Resolve Warning 4 unreferenced parameter problem
  //
  FilePath = NULL;

  //
  // If either if these parameters are NULL, we cannot continue.
  //
  if (This == NULL || BufferSize == NULL) {
    DEBUG ((DEBUG_WARN, "\nLoadFile()  This or BufferSize == NULL"));
    return EFI_INVALID_PARAMETER;
  }
  //
  // We only support BootPolicy == TRUE
  //
  if (!BootPolicy) {
    DEBUG ((DEBUG_WARN, "\nLoadFile()  BootPolicy == FALSE"));
    return EFI_UNSUPPORTED;
  }
  //
  // Get pointer to LoadFile protocol structure.
  //
  LoadfilePtr = CR (This, LOADFILE_DEVICE, LoadFile, LOADFILE_DEVICE_SIGNATURE);

  if (LoadfilePtr == NULL) {
    DEBUG (
      (DEBUG_NET,
      "\nLoadFile()  Could not get pointer to LoadFile structure")
      );
    return EFI_INVALID_PARAMETER;
  }
  //
  // Lock interface
  //
  EfiAcquireLock (&LoadfilePtr->Lock);

  //
  // Set console output mode and display attribute
  //
  if (OrigMode != 0) {
    gST->ConOut->SetMode (gST->ConOut, 0);
  }

  gST->ConOut->SetAttribute (
                gST->ConOut,
                EFI_TEXT_ATTR (EFI_LIGHTGRAY,EFI_BLACK)
                );

  //
  // See if BaseCode already has a Callback protocol attached.
  // If there is none, attach our own Callback protocol.
  //
  Status = gBS->HandleProtocol (
                  LoadfilePtr->Private->Handle,
                  &gEfiPxeBaseCodeCallbackProtocolGuid,
                  (VOID *) &LoadfilePtr->Private->CallbackProtocolPtr
                  );

  if (Status == EFI_SUCCESS) {
    //
    // There is already a callback routine.  Do nothing.
    //
    DEBUG ((DEBUG_WARN, "\nLoadFile()  BC callback exists."));

  } else if (Status == EFI_UNSUPPORTED) {
    //
    // No BaseCode Callback protocol found.  Add our own.
    //
    Status = gBS->InstallProtocolInterface (
                    &LoadfilePtr->Private->Handle,
                    &gEfiPxeBaseCodeCallbackProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &_bc_callback
                    );

    DEBUG ((DEBUG_WARN, "\nLoadFile()  Callback install status == %xh", Status));

    RemoveCallback = (BOOLEAN) (Status == EFI_SUCCESS);

    if (LoadfilePtr->Private->EfiBc.Mode != NULL && LoadfilePtr->Private->EfiBc.Mode->Started) {
      NewMakeCallback = TRUE;
      LoadfilePtr->Private->EfiBc.SetParameters (
                                    &LoadfilePtr->Private->EfiBc,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &NewMakeCallback
                                    );
    }

  } else {
    DEBUG ((DEBUG_WARN, "\nLoadFile()  Callback check status == %xh", Status));
  }
  //
  // Check for starting or for continuing after already getting
  // the file size.
  //
  if (LoadfilePtr->Private->FileSize == 0) {
    TmpBufSz  = 0;
    Status    = LoadfileStart (LoadfilePtr->Private, &TmpBufSz, Buffer);

    if (sizeof (UINTN) < sizeof (UINT64) && TmpBufSz > 0xFFFFFFFF) {
      *BufferSize = 0xFFFFFFFF;
    } else {
      *BufferSize = (UINTN) TmpBufSz;
    }

    if (Status == EFI_BUFFER_TOO_SMALL) {
      //
      // This is done so loadfile will work even if the boot manager
      // did not make the first call with Buffer == NULL.
      //
      Buffer = NULL;
    }
  } else if (Buffer == NULL) {
    DEBUG ((DEBUG_WARN, "\nLoadfile()  Get buffer size"));

    //
    // Continuing from previous LoadFile request.  Make sure there
    // is a buffer and that it is big enough.
    //
    *BufferSize = LoadfilePtr->Private->FileSize;
    Status      = EFI_BUFFER_TOO_SMALL;
  } else {
    DEBUG ((DEBUG_WARN, "\nLoadFile()  Download file"));

    //
    // Everything looks good, try to download the file.
    //
    TmpBufSz  = *BufferSize;
    Status    = DownloadFile (LoadfilePtr->Private, &TmpBufSz, Buffer);

    //
    // Next call to loadfile will start DHCP process again.
    //
    LoadfilePtr->Private->FileSize = 0;
  }
  //
  // If we added a callback protocol, now is the time to remove it.
  //
  if (RemoveCallback) {
    NewMakeCallback = FALSE;
    TempStatus = LoadfilePtr->Private->EfiBc.SetParameters (
                                          &LoadfilePtr->Private->EfiBc,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &NewMakeCallback
                                          );

    if (TempStatus == EFI_SUCCESS) {
      gBS->UninstallProtocolInterface (
            LoadfilePtr->Private->Handle,
            &gEfiPxeBaseCodeCallbackProtocolGuid,
            &_bc_callback
            );
    }
  }
  //
  // Restore display mode and attribute
  //
  if (OrigMode != 0) {
    gST->ConOut->SetMode (gST->ConOut, OrigMode);
  }

  gST->ConOut->SetAttribute (gST->ConOut, OrigAttribute);

  //
  // Unlock interface
  //
  EfiReleaseLock (&LoadfilePtr->Lock);

  DEBUG ((DEBUG_WARN, "\nBC.Loadfile()  Status == %xh\n", Status));

  if (Status == EFI_SUCCESS) {
    return EFI_SUCCESS;

  } else if (Status == EFI_BUFFER_TOO_SMALL) {
    //
    // Error is only displayed when we are actually trying to
    // download the boot image.
    //
    if (Buffer == NULL) {
      return EFI_BUFFER_TOO_SMALL;
    }

    AsciiPrint ("\nPXE-E05: Download buffer is smaller than requested file.\n");

  } else if (Status == EFI_DEVICE_ERROR) {
    AsciiPrint ("\nPXE-E07: Network device error.  Check network connection.\n");

  } else if (Status == EFI_OUT_OF_RESOURCES) {
    AsciiPrint ("\nPXE-E09: Could not allocate I/O buffers.\n");

  } else if (Status == EFI_NO_MEDIA) {
    AsciiPrint ("\nPXE-E12: Could not detect network connection.  Check cable.\n");

  } else if (Status == EFI_NO_RESPONSE) {
    AsciiPrint ("\nPXE-E16: Valid PXE offer not received.\n");

  } else if (Status == EFI_TIMEOUT) {
    AsciiPrint ("\nPXE-E18: Timeout.  Server did not respond.\n");

  } else if (Status == EFI_ABORTED) {
    AsciiPrint ("\nPXE-E21: Remote boot cancelled.\n");

  } else if (Status == EFI_ICMP_ERROR) {
    AsciiPrint ("\nPXE-E22: Client received ICMP error from server.\n");

    if (LoadfilePtr->Private->EfiBc.Mode != NULL) {
      if (LoadfilePtr->Private->EfiBc.Mode->IcmpErrorReceived) {

      AsciiPrint (
          "PXE-E98: Type: %xh  Code: %xh  ",
          LoadfilePtr->Private->EfiBc.Mode->IcmpError.Type,
          LoadfilePtr->Private->EfiBc.Mode->IcmpError.Code
          );

        switch (LoadfilePtr->Private->EfiBc.Mode->IcmpError.Type) {
        case 0x03:
          switch (LoadfilePtr->Private->EfiBc.Mode->IcmpError.Code) {
          case 0x00:              /* net unreachable */
          AsciiPrint ("Net unreachable");
            break;

          case 0x01:              /* host unreachable */
          AsciiPrint ("Host unreachable");
            break;

          case 0x02:              /* protocol unreachable */
          AsciiPrint ("Protocol unreachable");
            break;

          case 0x03:              /* port unreachable */
          AsciiPrint ("Port unreachable");
            break;

          case 0x04:              /* Fragmentation needed */
          AsciiPrint ("Fragmentation needed");
            break;

          case 0x05:              /* Source route failed */
          AsciiPrint ("Source route failed");
            break;
          }

          break;
        }

      AsciiPrint ("\n");
      }
    }

  } else if (Status == EFI_TFTP_ERROR) {
    AsciiPrint ("\nPXE-E23: Client received TFTP error from server.\n");

    if (LoadfilePtr->Private->EfiBc.Mode != NULL) {
      if (LoadfilePtr->Private->EfiBc.Mode->TftpErrorReceived) {
      AsciiPrint (
          "PXE-E98: Code: %xh  %a\n",
          LoadfilePtr->Private->EfiBc.Mode->TftpError.ErrorCode,
          LoadfilePtr->Private->EfiBc.Mode->TftpError.ErrorString
          );
      }
    }

  } else {
    AsciiPrint ("\nPXE-E99: Unexpected network error: %xh\n", Status);
  }

  LoadfilePtr->Private->EfiBc.Stop (&LoadfilePtr->Private->EfiBc);

  return Status;
}
