/** @file
  Boot functions implementation for UefiPxeBc Driver.

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PxeBcImpl.h"


/**
  Display the string of the boot item.

  If the length of the boot item string beyond 70 Char, just display 70 Char.

  @param[in]  Str           The pointer to the string.
  @param[in]  Len           The length of the string.

**/
VOID
PxeBcDisplayBootItem (
  IN UINT8                 *Str,
  IN UINT8                 Len
  )
{
  UINT8                    Tmp;

  //
  // Cut off the chars behind 70th.
  //
  Len       = (UINT8) MIN (PXEBC_DISPLAY_MAX_LINE, Len);
  Tmp       = Str[Len];
  Str[Len]  = 0;
  AsciiPrint ("%a \n", Str);

  //
  // Restore the original 70th char.
  //
  Str[Len]  = Tmp;
}


/**
  Select and maintain the boot prompt if needed.

  @param[in]  Private          Pointer to PxeBc private data.

  @retval EFI_SUCCESS          Selected boot prompt done.
  @retval EFI_TIMEOUT          Selected boot prompt timed out.
  @retval EFI_NOT_FOUND        The proxy offer is not Pxe10.
  @retval EFI_ABORTED          User cancelled the operation.
  @retval EFI_NOT_READY        Reading the input key from the keyboard has not finish.

**/
EFI_STATUS
PxeBcSelectBootPrompt (
  IN PXEBC_PRIVATE_DATA      *Private
  )
{
  PXEBC_DHCP_PACKET_CACHE    *Cache;
  PXEBC_VENDOR_OPTION        *VendorOpt;
  EFI_PXE_BASE_CODE_MODE     *Mode;
  EFI_EVENT                  TimeoutEvent;
  EFI_EVENT                  DescendEvent;
  EFI_INPUT_KEY              InputKey;
  EFI_STATUS                 Status;
  UINT32                     OfferType;
  UINT8                      Timeout;
  UINT8                      *Prompt;
  UINT8                      PromptLen;
  INT32                      SecCol;
  INT32                      SecRow;

  TimeoutEvent = NULL;
  DescendEvent = NULL;
  Mode         = Private->PxeBc.Mode;
  Cache        = Mode->ProxyOfferReceived ? &Private->ProxyOffer : &Private->DhcpAck;
  OfferType    = Mode->UsingIpv6 ? Cache->Dhcp6.OfferType : Cache->Dhcp4.OfferType;

  //
  // Only DhcpPxe10 and ProxyPxe10 offer needs boot prompt.
  //
  if (OfferType != PxeOfferTypeProxyPxe10 && OfferType != PxeOfferTypeDhcpPxe10) {
    return EFI_NOT_FOUND;
  }

  //
  // There is no specified ProxyPxe10 for IPv6 in PXE and UEFI spec.
  //
  ASSERT (!Mode->UsingIpv6);

  VendorOpt = &Cache->Dhcp4.VendorOpt;
  //
  // According to the PXE specification 2.1, Table 2-1 PXE DHCP Options,
  // we must not consider a boot prompt or boot menu if all of the following hold:
  //   - the PXE_DISCOVERY_CONTROL tag(6) is present inside the Vendor Options(43), and has bit 3 set  
  //   - a boot file name has been presented in the initial DHCP or ProxyDHCP offer packet.
  //
  if (IS_DISABLE_PROMPT_MENU (VendorOpt->DiscoverCtrl) &&
      Cache->Dhcp4.OptList[PXEBC_DHCP4_TAG_INDEX_BOOTFILE] != NULL) {
    return EFI_ABORTED;
  }
  
  if (!IS_VALID_BOOT_PROMPT (VendorOpt->BitMap)) {
    return EFI_TIMEOUT;
  }

  Timeout   = VendorOpt->MenuPrompt->Timeout;
  Prompt    = VendorOpt->MenuPrompt->Prompt;
  PromptLen = (UINT8) (VendorOpt->MenuPromptLen - 1);

  //
  // The valid scope of Timeout refers to PXE2.1 spec.
  //
  if (Timeout == 0) {
    return EFI_TIMEOUT;
  }
  if (Timeout == 255) {
    return EFI_SUCCESS;
  }

  //
  // Create and start a timer as timeout event.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &TimeoutEvent
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->SetTimer (
                  TimeoutEvent,
                  TimerRelative,
                  Timeout * TICKS_PER_SECOND
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Create and start a periodic timer as descend event by second.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &DescendEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = gBS->SetTimer (
                  DescendEvent,
                  TimerPeriodic,
                  TICKS_PER_SECOND
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Display the boot item and cursor on the screen.
  //
  SecCol = gST->ConOut->Mode->CursorColumn;
  SecRow = gST->ConOut->Mode->CursorRow;

  PxeBcDisplayBootItem (Prompt, PromptLen);

  gST->ConOut->SetCursorPosition (gST->ConOut, SecCol + PromptLen, SecRow);
  AsciiPrint ("(%d) ", Timeout--);

  Status = EFI_TIMEOUT;
  while (EFI_ERROR (gBS->CheckEvent (TimeoutEvent))) {
    if (!EFI_ERROR (gBS->CheckEvent (DescendEvent))) {
      gST->ConOut->SetCursorPosition (gST->ConOut, SecCol + PromptLen, SecRow);
      AsciiPrint ("(%d) ", Timeout--);
    }
    if (gST->ConIn->ReadKeyStroke (gST->ConIn, &InputKey) == EFI_NOT_READY) {
      gBS->Stall (10 * TICKS_PER_MS);
      continue;
    }
    //
    // Parse the input key by user.
    // If <F8> or <Ctrl> + <M> is pressed, return success to display the boot menu.
    //
    if (InputKey.ScanCode == 0) {

      switch (InputKey.UnicodeChar) {

      case CTRL ('c'):
        Status = EFI_ABORTED;
        break;

      case CTRL ('m'):
      case 'm':
      case 'M':
        Status = EFI_SUCCESS;
        break;

      default:
        continue;
      }

    } else {

      switch (InputKey.ScanCode) {

      case SCAN_F8:
        Status = EFI_SUCCESS;
        break;

      case SCAN_ESC:
        Status = EFI_ABORTED;
        break;

      default:
        continue;
      }
    }

    break;
  }

  //
  // Reset the cursor on the screen.
  //
  gST->ConOut->SetCursorPosition (gST->ConOut, 0 , SecRow + 1);

ON_EXIT:
  if (DescendEvent != NULL) {
    gBS->CloseEvent (DescendEvent);
  }
  if (TimeoutEvent != NULL) {
    gBS->CloseEvent (TimeoutEvent);
  }

  return Status;
}


/**
  Select the boot menu by user's input.

  @param[in]  Private         Pointer to PxeBc private data.
  @param[out] Type            The type of the menu.
  @param[in]  UseDefaultItem  Use default item or not.

  @retval EFI_ABORTED     User cancel operation.
  @retval EFI_SUCCESS     Select the boot menu success.
  @retval EFI_NOT_READY   Read the input key from the keybroad has not finish.

**/
EFI_STATUS
PxeBcSelectBootMenu (
  IN  PXEBC_PRIVATE_DATA              *Private,
  OUT UINT16                          *Type,
  IN  BOOLEAN                         UseDefaultItem
  )
{
  EFI_PXE_BASE_CODE_MODE     *Mode;
  PXEBC_DHCP_PACKET_CACHE    *Cache;
  PXEBC_VENDOR_OPTION        *VendorOpt;
  EFI_INPUT_KEY              InputKey;
  UINT32                     OfferType;
  UINT8                      MenuSize;
  UINT8                      MenuNum;
  INT32                      TopRow;
  UINT16                     Select;
  UINT16                     LastSelect;
  UINT8                      Index;
  BOOLEAN                    Finish;
  CHAR8                      Blank[PXEBC_DISPLAY_MAX_LINE];
  PXEBC_BOOT_MENU_ENTRY      *MenuItem;
  PXEBC_BOOT_MENU_ENTRY      *MenuArray[PXEBC_MENU_MAX_NUM];

  Finish    = FALSE;
  Select    = 0;
  Index     = 0;
  *Type     = 0;
  Mode      = Private->PxeBc.Mode;
  Cache     = Mode->ProxyOfferReceived ? &Private->ProxyOffer : &Private->DhcpAck;
  OfferType = Mode->UsingIpv6 ? Cache->Dhcp6.OfferType : Cache->Dhcp4.OfferType;

  //
  // There is no specified DhcpPxe10/ProxyPxe10 for IPv6 in PXE and UEFI spec.
  //
  ASSERT (!Mode->UsingIpv6);
  ASSERT (OfferType == PxeOfferTypeProxyPxe10 || OfferType == PxeOfferTypeDhcpPxe10);

  VendorOpt = &Cache->Dhcp4.VendorOpt;
  if (!IS_VALID_BOOT_MENU (VendorOpt->BitMap)) {
    return EFI_SUCCESS;
  }

  //
  // Display the boot menu on the screen.
  //
  SetMem (Blank, sizeof(Blank), ' ');

  MenuSize  = VendorOpt->BootMenuLen;
  MenuItem  = VendorOpt->BootMenu;

  if (MenuSize == 0) {
    return EFI_DEVICE_ERROR;
  }

  while (MenuSize > 0 && Index < PXEBC_MENU_MAX_NUM) {
    ASSERT (MenuItem != NULL);
    MenuArray[Index]  = MenuItem;
    MenuSize          = (UINT8) (MenuSize - (MenuItem->DescLen + 3));
    MenuItem          = (PXEBC_BOOT_MENU_ENTRY *) ((UINT8 *) MenuItem + MenuItem->DescLen + 3);
    Index++;
  }

  if (UseDefaultItem) {
    ASSERT (MenuArray[0] != NULL);
    CopyMem (Type, &MenuArray[0]->Type, sizeof (UINT16));
    *Type = NTOHS (*Type);
    return EFI_SUCCESS;
  }

  MenuNum = Index;

  for (Index = 0; Index < MenuNum; Index++) {
    ASSERT (MenuArray[Index] != NULL);
    PxeBcDisplayBootItem (MenuArray[Index]->DescStr, MenuArray[Index]->DescLen);
  }

  TopRow = gST->ConOut->Mode->CursorRow - MenuNum;

  //
  // Select the boot item by user in the boot menu.
  //
  do {
    //
    // Highlight selected row.
    //
    gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_BLACK, EFI_LIGHTGRAY));
    gST->ConOut->SetCursorPosition (gST->ConOut, 0, TopRow + Select);
    ASSERT (Select < PXEBC_MENU_MAX_NUM);
    ASSERT (MenuArray[Select] != NULL);
    Blank[MenuArray[Select]->DescLen] = 0;
    AsciiPrint ("%a\r", Blank);
    PxeBcDisplayBootItem (MenuArray[Select]->DescStr, MenuArray[Select]->DescLen);
    gST->ConOut->SetCursorPosition (gST->ConOut, 0, TopRow + MenuNum);
    LastSelect = Select;

    while (gST->ConIn->ReadKeyStroke (gST->ConIn, &InputKey) == EFI_NOT_READY) {
      gBS->Stall (10 * TICKS_PER_MS);
    }

    if (InputKey.ScanCode == 0) {
      switch (InputKey.UnicodeChar) {
      case CTRL ('c'):
        InputKey.ScanCode = SCAN_ESC;
        break;

      case CTRL ('j'):  /* linefeed */
      case CTRL ('m'):  /* return */
        Finish = TRUE;
        break;

      case CTRL ('i'):  /* tab */
      case ' ':
      case 'd':
      case 'D':
        InputKey.ScanCode = SCAN_DOWN;
        break;

      case CTRL ('h'):  /* backspace */
      case 'u':
      case 'U':
        InputKey.ScanCode = SCAN_UP;
        break;

      default:
        InputKey.ScanCode = 0;
      }
    }

    switch (InputKey.ScanCode) {
    case SCAN_LEFT:
    case SCAN_UP:
      if (Select != 0) {
        Select--;
      }
      break;

    case SCAN_DOWN:
    case SCAN_RIGHT:
      if (++Select == MenuNum) {
        Select--;
      }
      break;

    case SCAN_PAGE_UP:
    case SCAN_HOME:
      Select = 0;
      break;

    case SCAN_PAGE_DOWN:
    case SCAN_END:
      Select = (UINT16) (MenuNum - 1);
      break;

    case SCAN_ESC:
      return EFI_ABORTED;
    }

    //
    // Unhighlight the last selected row.
    //
    gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
    gST->ConOut->SetCursorPosition (gST->ConOut, 0, TopRow + LastSelect);
    ASSERT (LastSelect < PXEBC_MENU_MAX_NUM);
    ASSERT (MenuArray[LastSelect] != NULL);
    Blank[MenuArray[LastSelect]->DescLen] = 0;
    AsciiPrint ("%a\r", Blank);
    PxeBcDisplayBootItem (MenuArray[LastSelect]->DescStr, MenuArray[LastSelect]->DescLen);
    gST->ConOut->SetCursorPosition (gST->ConOut, 0, TopRow + MenuNum);
  } while (!Finish);

  //
  // Swap the byte order.
  //
  ASSERT (Select < PXEBC_MENU_MAX_NUM);
  ASSERT (MenuArray[Select] != NULL);
  CopyMem (Type, &MenuArray[Select]->Type, sizeof (UINT16));
  *Type = NTOHS (*Type);

  return EFI_SUCCESS;
}


/**
  Parse out the boot information from the last Dhcp4 reply packet.

  @param[in, out] Private      Pointer to PxeBc private data.
  @param[out]     BufferSize   Size of the boot file to be downloaded.

  @retval EFI_SUCCESS          Successfully parsed out all the boot information.
  @retval Others               Failed to parse out the boot information.

**/
EFI_STATUS
PxeBcDhcp4BootInfo (
  IN OUT PXEBC_PRIVATE_DATA   *Private,
     OUT UINT64               *BufferSize
  )
{
  EFI_PXE_BASE_CODE_PROTOCOL  *PxeBc;
  EFI_PXE_BASE_CODE_MODE      *Mode;
  EFI_STATUS                  Status;
  PXEBC_DHCP4_PACKET_CACHE    *Cache4;
  UINT16                      Value;
  PXEBC_VENDOR_OPTION         *VendorOpt;
  PXEBC_BOOT_SVR_ENTRY        *Entry;
  
  PxeBc       = &Private->PxeBc;
  Mode        = PxeBc->Mode;
  Status      = EFI_SUCCESS;
  *BufferSize = 0;

  //
  // Get the last received Dhcp4 reply packet.
  //
  if (Mode->PxeReplyReceived) {
    Cache4 = &Private->PxeReply.Dhcp4;
  } else if (Mode->ProxyOfferReceived) {
    Cache4 = &Private->ProxyOffer.Dhcp4;
  } else {
    Cache4 = &Private->DhcpAck.Dhcp4;
  }

  ASSERT (Cache4->OptList[PXEBC_DHCP4_TAG_INDEX_BOOTFILE] != NULL);

  //
  // Parse the boot server address.
  // If prompt/discover is disabled, get the first boot server from the boot servers list.
  // Otherwise, parse the boot server Ipv4 address from next server address field in DHCP header.
  // If all these fields are not available, use option 54 instead.
  //
  VendorOpt = &Cache4->VendorOpt;
  if (IS_DISABLE_PROMPT_MENU (VendorOpt->DiscoverCtrl) && IS_VALID_BOOT_SERVERS (VendorOpt->BitMap)) {
    Entry = VendorOpt->BootSvr;
    if (VendorOpt->BootSvrLen >= sizeof (PXEBC_BOOT_SVR_ENTRY) && Entry->IpCnt > 0) {
      CopyMem (
        &Private->ServerIp,
        &Entry->IpAddr[0],
        sizeof (EFI_IPv4_ADDRESS)
        );
    }
  }
  if (Private->ServerIp.Addr[0] == 0) {
    //
    // ServerIp.Addr[0] equals zero means we failed to get IP address from boot server list.
    // Try to use next server address field.
    //
    CopyMem (
      &Private->ServerIp,
      &Cache4->Packet.Offer.Dhcp4.Header.ServerAddr,
      sizeof (EFI_IPv4_ADDRESS)
      );
  }
  if (Private->ServerIp.Addr[0] == 0) {
    //
    // Still failed , use the IP address from option 54.
    //
    CopyMem (
      &Private->ServerIp,
      Cache4->OptList[PXEBC_DHCP4_TAG_INDEX_SERVER_ID]->Data,
      sizeof (EFI_IPv4_ADDRESS)
      );
  }

  //
  // Parse the boot file name by option.
  //
  Private->BootFileName = Cache4->OptList[PXEBC_DHCP4_TAG_INDEX_BOOTFILE]->Data;

  if (Cache4->OptList[PXEBC_DHCP4_TAG_INDEX_BOOTFILE_LEN] != NULL) {
    //
    // Parse the boot file size by option.
    //
    CopyMem (&Value, Cache4->OptList[PXEBC_DHCP4_TAG_INDEX_BOOTFILE_LEN]->Data, sizeof (Value));
    Value       = NTOHS (Value);
    //
    // The field of boot file size is 512 bytes in unit.
    //
    *BufferSize = 512 * Value;
  } else {
    //
    // Get the bootfile size by tftp command if no option available.
    //
    Status = PxeBc->Mtftp (
                      PxeBc,
                      EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE,
                      NULL,
                      FALSE,
                      BufferSize,
                      &Private->BlockSize,
                      &Private->ServerIp,
                      Private->BootFileName,
                      NULL,
                      FALSE
                      );
  }

  //
  // Save the value of boot file size.
  //
  Private->BootFileSize = (UINTN) *BufferSize;

  //
  // Display all the information: boot server address, boot file name and boot file size.
  //
  AsciiPrint ("\n  Server IP address is ");
  PxeBcShowIp4Addr (&Private->ServerIp.v4);
  AsciiPrint ("\n  NBP filename is %a", Private->BootFileName);
  AsciiPrint ("\n  NBP filesize is %d Bytes", Private->BootFileSize);

  return Status;
}


/**
  Parse out the boot information from the last Dhcp6 reply packet.

  @param[in, out] Private      Pointer to PxeBc private data.
  @param[out]     BufferSize   Size of the boot file to be downloaded.

  @retval EFI_SUCCESS          Successfully parsed out all the boot information.
  @retval EFI_BUFFER_TOO_SMALL
  @retval Others               Failed to parse out the boot information.

**/
EFI_STATUS
PxeBcDhcp6BootInfo (
  IN OUT PXEBC_PRIVATE_DATA   *Private,
     OUT UINT64               *BufferSize
  )
{
  EFI_PXE_BASE_CODE_PROTOCOL  *PxeBc;
  EFI_PXE_BASE_CODE_MODE      *Mode;
  EFI_STATUS                  Status;
  PXEBC_DHCP6_PACKET_CACHE    *Cache6;
  UINT16                      Value;

  PxeBc       = &Private->PxeBc;
  Mode        = PxeBc->Mode;
  Status      = EFI_SUCCESS;
  *BufferSize = 0;

  //
  // Get the last received Dhcp6 reply packet.
  //
  if (Mode->PxeReplyReceived) {
    Cache6 = &Private->PxeReply.Dhcp6;
  } else if (Mode->ProxyOfferReceived) {
    Cache6 = &Private->ProxyOffer.Dhcp6;
  } else {
    Cache6 = &Private->DhcpAck.Dhcp6;
  }

  ASSERT (Cache6->OptList[PXEBC_DHCP6_IDX_BOOT_FILE_URL] != NULL);

  //
  // Parse (m)tftp server ip address and bootfile name.
  //
  Status = PxeBcExtractBootFileUrl (
             &Private->BootFileName,
             &Private->ServerIp.v6,
             (CHAR8 *) (Cache6->OptList[PXEBC_DHCP6_IDX_BOOT_FILE_URL]->Data),
             NTOHS (Cache6->OptList[PXEBC_DHCP6_IDX_BOOT_FILE_URL]->OpLen)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set the station address to IP layer.
  //
  Status = PxeBcSetIp6Address (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Parse the value of boot file size.
  //
  if (Cache6->OptList[PXEBC_DHCP6_IDX_BOOT_FILE_PARAM] != NULL) {
    //
    // Parse it out if have the boot file parameter option.
    //
    Status = PxeBcExtractBootFileParam ((CHAR8 *) Cache6->OptList[PXEBC_DHCP6_IDX_BOOT_FILE_PARAM]->Data, &Value);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // The field of boot file size is 512 bytes in unit.
    //
    *BufferSize = 512 * Value;
  } else {
    //
    // Send get file size command by tftp if option unavailable.
    //
    Status = PxeBc->Mtftp (
                      PxeBc,
                      EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE,
                      NULL,
                      FALSE,
                      BufferSize,
                      &Private->BlockSize,
                      &Private->ServerIp,
                      Private->BootFileName,
                      NULL,
                      FALSE
                      );
  }

  //
  // Save the value of boot file size.
  //
  Private->BootFileSize = (UINTN) *BufferSize;

  //
  // Display all the information: boot server address, boot file name and boot file size.
  //
  AsciiPrint ("\n  Server IP address is ");
  PxeBcShowIp6Addr (&Private->ServerIp.v6);
  AsciiPrint ("\n  NBP filename is %a", Private->BootFileName);
  AsciiPrint ("\n  NBP filesize is %d Bytes", Private->BootFileSize);

  return Status;
}


/**
  Extract the discover information and boot server entry from the
  cached packets if unspecified.

  @param[in]      Private      Pointer to PxeBc private data.
  @param[in]      Type         The type of bootstrap to perform.
  @param[in, out] DiscoverInfo Pointer to EFI_PXE_BASE_CODE_DISCOVER_INFO.
  @param[out]     BootEntry    Pointer to PXEBC_BOOT_SVR_ENTRY.
  @param[out]     SrvList      Pointer to EFI_PXE_BASE_CODE_SRVLIST.

  @retval EFI_SUCCESS       Successfully extracted the information.
  @retval EFI_DEVICE_ERROR  Failed to extract the information.

**/
EFI_STATUS
PxeBcExtractDiscoverInfo (
  IN     PXEBC_PRIVATE_DATA               *Private,
  IN     UINT16                           Type,
  IN OUT EFI_PXE_BASE_CODE_DISCOVER_INFO  **DiscoverInfo,
     OUT PXEBC_BOOT_SVR_ENTRY             **BootEntry,
     OUT EFI_PXE_BASE_CODE_SRVLIST        **SrvList
  )
{
  EFI_PXE_BASE_CODE_MODE          *Mode;
  PXEBC_DHCP4_PACKET_CACHE        *Cache4;
  PXEBC_VENDOR_OPTION             *VendorOpt;
  PXEBC_BOOT_SVR_ENTRY            *Entry;
  BOOLEAN                         IsFound;
  EFI_PXE_BASE_CODE_DISCOVER_INFO *Info;
  UINT16                          Index;

  Mode = Private->PxeBc.Mode;
  Info = *DiscoverInfo;

  if (Mode->UsingIpv6) {
    Info->IpCnt    = 1;
    Info->UseUCast = TRUE;

    Info->SrvList[0].Type              = Type;
    Info->SrvList[0].AcceptAnyResponse = FALSE;

    //
    // There is no vendor options specified in DHCPv6, so take BootFileUrl in the last cached packet.
    //
    CopyMem (&Info->SrvList[0].IpAddr, &Private->ServerIp, sizeof (EFI_IP_ADDRESS));

    *SrvList  = Info->SrvList;
  } else {
    Entry     = NULL;
    IsFound   = FALSE;
    Cache4    = (Mode->ProxyOfferReceived) ? &Private->ProxyOffer.Dhcp4 : &Private->DhcpAck.Dhcp4;
    VendorOpt = &Cache4->VendorOpt;

    if (!Mode->DhcpAckReceived || !IS_VALID_DISCOVER_VENDOR_OPTION (VendorOpt->BitMap)) {
      //
      // Address is not acquired or no discovery options.
      //
      return EFI_INVALID_PARAMETER;
    }

    //
    // Parse the boot server entry from the vendor option in the last cached packet.
    //
    Info->UseMCast    = (BOOLEAN) !IS_DISABLE_MCAST_DISCOVER (VendorOpt->DiscoverCtrl);
    Info->UseBCast    = (BOOLEAN) !IS_DISABLE_BCAST_DISCOVER (VendorOpt->DiscoverCtrl);
    Info->MustUseList = (BOOLEAN) IS_ENABLE_USE_SERVER_LIST (VendorOpt->DiscoverCtrl);
    Info->UseUCast    = (BOOLEAN) IS_VALID_BOOT_SERVERS (VendorOpt->BitMap);

    if (Info->UseMCast) {
      //
      // Get the multicast discover ip address from vendor option if has.
      //
      CopyMem (&Info->ServerMCastIp.v4, &VendorOpt->DiscoverMcastIp, sizeof (EFI_IPv4_ADDRESS));
    }

    Info->IpCnt = 0;

    if (Info->UseUCast) {
      Entry = VendorOpt->BootSvr;

      while (((UINT8) (Entry - VendorOpt->BootSvr)) < VendorOpt->BootSvrLen) {
        if (Entry->Type == HTONS (Type)) {
          IsFound = TRUE;
          break;
        }
        Entry = GET_NEXT_BOOT_SVR_ENTRY (Entry);
      }

      if (!IsFound) {
        return EFI_DEVICE_ERROR;
      }

      Info->IpCnt = Entry->IpCnt;
      if (Info->IpCnt >= 1) {
        *DiscoverInfo = AllocatePool (sizeof (*Info) + (Info->IpCnt - 1) * sizeof (**SrvList));
        if (*DiscoverInfo == NULL) {
          return EFI_OUT_OF_RESOURCES;       
        }     
        CopyMem (*DiscoverInfo, Info, sizeof (*Info));
        Info = *DiscoverInfo;
      }

      for (Index = 0; Index < Info->IpCnt; Index++) {
        CopyMem (&Info->SrvList[Index].IpAddr, &Entry->IpAddr[Index], sizeof (EFI_IPv4_ADDRESS));
        Info->SrvList[Index].AcceptAnyResponse = !Info->MustUseList;
        Info->SrvList[Index].Type = NTOHS (Entry->Type);
      }
    }

    *BootEntry = Entry;
    *SrvList   = Info->SrvList;
  }

  return EFI_SUCCESS;
}


/**
  Build the discover packet and send out for boot server.

  @param[in]  Private               Pointer to PxeBc private data.
  @param[in]  Type                  PxeBc option boot item type.
  @param[in]  Layer                 Pointer to option boot item layer.
  @param[in]  UseBis                Use BIS or not.
  @param[in]  DestIp                Pointer to the destination address.
  @param[in]  IpCount               The count of the server address.
  @param[in]  SrvList               Pointer to the server address list.

  @retval     EFI_SUCCESS           Successfully discovered boot file.
  @retval     EFI_OUT_OF_RESOURCES  Failed to allocate resource.
  @retval     EFI_NOT_FOUND         Can't get the PXE reply packet.
  @retval     Others                Failed to discover boot file.

**/
EFI_STATUS
PxeBcDiscoverBootServer (
  IN  PXEBC_PRIVATE_DATA                *Private,
  IN  UINT16                            Type,
  IN  UINT16                            *Layer,
  IN  BOOLEAN                           UseBis,
  IN  EFI_IP_ADDRESS                    *DestIp,
  IN  UINT16                            IpCount,
  IN  EFI_PXE_BASE_CODE_SRVLIST         *SrvList
  )
{
  if (Private->PxeBc.Mode->UsingIpv6) {
    return PxeBcDhcp6Discover (
             Private,
             Type,
             Layer,
             UseBis,
             DestIp
             );
  } else {
    return PxeBcDhcp4Discover (
             Private,
             Type,
             Layer,
             UseBis,
             DestIp,
             IpCount,
             SrvList
             );
  }
}


/**
  Discover all the boot information for boot file.

  @param[in, out] Private      Pointer to PxeBc private data.
  @param[out]     BufferSize   Size of the boot file to be downloaded.

  @retval EFI_SUCCESS          Successfully obtained all the boot information .
  @retval EFI_BUFFER_TOO_SMALL The buffer size is not enough for boot file.
  @retval EFI_ABORTED          User cancel current operation.
  @retval Others               Failed to parse out the boot information.

**/
EFI_STATUS
PxeBcDiscoverBootFile (
  IN OUT PXEBC_PRIVATE_DATA   *Private,
     OUT UINT64               *BufferSize
  )
{
  EFI_PXE_BASE_CODE_PROTOCOL  *PxeBc;
  EFI_PXE_BASE_CODE_MODE      *Mode;
  EFI_STATUS                  Status;
  UINT16                      Type;
  UINT16                      Layer;
  BOOLEAN                     UseBis;

  PxeBc = &Private->PxeBc;
  Mode  = PxeBc->Mode;
  Type  = EFI_PXE_BASE_CODE_BOOT_TYPE_BOOTSTRAP;
  Layer = EFI_PXE_BASE_CODE_BOOT_LAYER_INITIAL;

  //
  // Start D.O.R.A/S.A.R.R exchange to acquire station ip address and
  // other pxe boot information.
  //
  Status = PxeBc->Dhcp (PxeBc, TRUE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Select a boot server from boot server list.
  //
  Status = PxeBcSelectBootPrompt (Private);

  if (Status == EFI_SUCCESS) {
    //
    // Choose by user's input.
    //
    Status = PxeBcSelectBootMenu (Private, &Type, FALSE);
  } else if (Status == EFI_TIMEOUT) {
    //
    // Choose by default item.
    //
    Status = PxeBcSelectBootMenu (Private, &Type, TRUE);
  }

  if (!EFI_ERROR (Status)) {

    if (Type == EFI_PXE_BASE_CODE_BOOT_TYPE_BOOTSTRAP) {
      //
      // Local boot(PXE bootstrap server) need abort
      //
      return EFI_ABORTED;
    }

    //
    // Start to discover the boot server to get (m)tftp server ip address, bootfile
    // name and bootfile size.
    //
    UseBis = (BOOLEAN) (Mode->BisSupported && Mode->BisDetected);
    Status = PxeBc->Discover (PxeBc, Type, &Layer, UseBis, NULL);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (Mode->PxeReplyReceived && !Mode->ProxyOfferReceived) {
      //
      // Some network boot loader only search the packet in Mode.ProxyOffer to get its server
      // IP address, so we need to store a copy of Mode.PxeReply packet into Mode.ProxyOffer.
      //
      if (Mode->UsingIpv6) {
        CopyMem (
          &Mode->ProxyOffer.Dhcpv6,
          &Mode->PxeReply.Dhcpv6,
          Private->PxeReply.Dhcp6.Packet.Ack.Length
          );
      } else {
        CopyMem (
          &Mode->ProxyOffer.Dhcpv4,
          &Mode->PxeReply.Dhcpv4,
          Private->PxeReply.Dhcp4.Packet.Ack.Length
          );      
      }
      Mode->ProxyOfferReceived = TRUE;
    }
  }

  //
  // Parse the boot information.
  //
  if (Mode->UsingIpv6) {
    Status = PxeBcDhcp6BootInfo (Private, BufferSize);
  } else {
    Status = PxeBcDhcp4BootInfo (Private, BufferSize);
  }

  return Status;
}


/**
  Install PxeBaseCodeCallbackProtocol if not installed before.

  @param[in, out] Private           Pointer to PxeBc private data.
  @param[out]     NewMakeCallback   If TRUE, it is a new callback.
                                    Otherwise, it is not new callback.
  @retval EFI_SUCCESS          PxeBaseCodeCallbackProtocol installed succesfully.
  @retval Others               Failed to install PxeBaseCodeCallbackProtocol.

**/
EFI_STATUS
PxeBcInstallCallback (
  IN OUT PXEBC_PRIVATE_DATA   *Private,
     OUT BOOLEAN              *NewMakeCallback
  )
{
  EFI_PXE_BASE_CODE_PROTOCOL  *PxeBc;
  EFI_STATUS                  Status;

  //
  // Check whether PxeBaseCodeCallbackProtocol already installed.
  //
  PxeBc  = &Private->PxeBc;
  Status = gBS->HandleProtocol (
                  Private->Controller,
                  &gEfiPxeBaseCodeCallbackProtocolGuid,
                  (VOID **) &Private->PxeBcCallback
                  );
  if (Status == EFI_UNSUPPORTED) {

    CopyMem (
      &Private->LoadFileCallback,
      &gPxeBcCallBackTemplate,
      sizeof (EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL)
      );

    //
    // Install a default callback if user didn't offer one.
    //
    Status = gBS->InstallProtocolInterface (
                    &Private->Controller,
                    &gEfiPxeBaseCodeCallbackProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &Private->LoadFileCallback
                    );

    (*NewMakeCallback) = (BOOLEAN) (Status == EFI_SUCCESS);

    Status = PxeBc->SetParameters (PxeBc, NULL, NULL, NULL, NULL, NewMakeCallback);
    if (EFI_ERROR (Status)) {
      PxeBc->Stop (PxeBc);
      return Status;
    }
  }

  return EFI_SUCCESS;
}


/**
  Uninstall PxeBaseCodeCallbackProtocol.

  @param[in]  Private           Pointer to PxeBc private data.
  @param[in]  NewMakeCallback   If TRUE, it is a new callback.
                                Otherwise, it is not new callback.

**/
VOID
PxeBcUninstallCallback (
  IN PXEBC_PRIVATE_DATA        *Private,
  IN BOOLEAN                   NewMakeCallback
  )
{
  EFI_PXE_BASE_CODE_PROTOCOL   *PxeBc;

  PxeBc = &Private->PxeBc;

  if (NewMakeCallback) {

    NewMakeCallback = FALSE;

    PxeBc->SetParameters (PxeBc, NULL, NULL, NULL, NULL, &NewMakeCallback);

    gBS->UninstallProtocolInterface (
          Private->Controller,
          &gEfiPxeBaseCodeCallbackProtocolGuid,
          &Private->LoadFileCallback
          );
  }
}


/**
  Download one of boot file in the list, and it's special for IPv6.

  @param[in]      Private           Pointer to PxeBc private data.
  @param[in, out] BufferSize        Size of user buffer for input;
                                    required buffer size for output.
  @param[in]      Buffer            Pointer to user buffer.

  @retval EFI_SUCCESS               Read one of boot file in the list successfully.
  @retval EFI_BUFFER_TOO_SMALL      The buffer size is not enough for boot file.
  @retval EFI_NOT_FOUND             There is no proper boot file available.
  @retval Others                    Failed to download boot file in the list.

**/
EFI_STATUS
PxeBcReadBootFileList (
  IN     PXEBC_PRIVATE_DATA           *Private,
  IN OUT UINT64                       *BufferSize,
  IN     VOID                         *Buffer           OPTIONAL
  )
{
  EFI_STATUS                          Status;
  EFI_PXE_BASE_CODE_PROTOCOL          *PxeBc;

  PxeBc        = &Private->PxeBc;

  //
  // Try to download the boot file if everything is ready.
  //
  if (Buffer != NULL) {
    Status = PxeBc->Mtftp (
                      PxeBc,
                      EFI_PXE_BASE_CODE_TFTP_READ_FILE,
                      Buffer,
                      FALSE,
                      BufferSize,
                      &Private->BlockSize,
                      &Private->ServerIp,
                      Private->BootFileName,
                      NULL,
                      FALSE
                      );


  } else {
    Status      = EFI_BUFFER_TOO_SMALL;
  }

  return Status;
}


/**
  Load boot file into user buffer.

  @param[in]      Private           Pointer to PxeBc private data.
  @param[in, out] BufferSize        Size of user buffer for input;
                                    required buffer size for output.
  @param[in]      Buffer            Pointer to user buffer.

  @retval EFI_SUCCESS          Get all the boot information successfully.
  @retval EFI_BUFFER_TOO_SMALL The buffer size is not enough for boot file.
  @retval EFI_ABORTED          User cancelled the current operation.
  @retval Others               Failed to parse out the boot information.

**/
EFI_STATUS
PxeBcLoadBootFile (
  IN     PXEBC_PRIVATE_DATA           *Private,
  IN OUT UINTN                        *BufferSize,
  IN     VOID                         *Buffer         OPTIONAL
  )
{
  BOOLEAN                             NewMakeCallback;
  UINT64                              RequiredSize;
  UINT64                              CurrentSize;
  EFI_STATUS                          Status;
  EFI_PXE_BASE_CODE_PROTOCOL          *PxeBc;
  EFI_PXE_BASE_CODE_MODE              *PxeBcMode;

  NewMakeCallback = FALSE;
  PxeBc           = &Private->PxeBc;
  PxeBcMode       = &Private->Mode;
  CurrentSize     = *BufferSize;
  RequiredSize    = 0;

  //
  // Install pxebc callback protocol if hasn't been installed yet.
  //
  Status = PxeBcInstallCallback (Private, &NewMakeCallback);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  if (Private->BootFileSize == 0) {
    //
    // Discover the boot information about the bootfile if hasn't.
    //
    Status = PxeBcDiscoverBootFile (Private, &RequiredSize);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    if (PXEBC_IS_SIZE_OVERFLOWED (RequiredSize)) {
      //
      // It's error if the required buffer size is beyond the system scope.
      //
      Status = EFI_DEVICE_ERROR;
      goto ON_EXIT;
    } else if (RequiredSize > 0) {
      //
      // Get the right buffer size of the bootfile required.
      //
      if (CurrentSize < RequiredSize || Buffer == NULL) {
        //
        // It's buffer too small if the size of user buffer is smaller than the required.
        //
        CurrentSize = RequiredSize;
        Status      = EFI_BUFFER_TOO_SMALL;
        goto ON_EXIT;
      }
      CurrentSize = RequiredSize;
    } else if (RequiredSize == 0 && PxeBcMode->UsingIpv6) {
      //
      // Try to download another bootfile in list if failed to get the filesize of the last one.
      // It's special for the case of IPv6.
      //
      Status = PxeBcReadBootFileList (Private, &CurrentSize, Buffer);
      goto ON_EXIT;
    }
  } else if (CurrentSize < Private->BootFileSize || Buffer == NULL ) {
    //
    // It's buffer too small if the size of user buffer is smaller than the required.
    //
    CurrentSize = Private->BootFileSize;
    Status      = EFI_BUFFER_TOO_SMALL;
    goto ON_EXIT;
  }

  //
  // Begin to download the bootfile if everything is ready.
  //
  AsciiPrint ("\n Downloading NBP file...\n");
  if (PxeBcMode->UsingIpv6) {
    Status = PxeBcReadBootFileList (
               Private,
               &CurrentSize,
               Buffer
               );
  } else {
    Status = PxeBc->Mtftp (
                      PxeBc,
                      EFI_PXE_BASE_CODE_TFTP_READ_FILE,
                      Buffer,
                      FALSE,
                      &CurrentSize,
                      &Private->BlockSize,
                      &Private->ServerIp,
                      Private->BootFileName,
                      NULL,
                      FALSE
                      );
  }

ON_EXIT:
  *BufferSize = (UINTN) CurrentSize;
  PxeBcUninstallCallback(Private, NewMakeCallback);

  if (Status == EFI_SUCCESS) {
    AsciiPrint ("\n  Succeed to download NBP file.\n");
    return EFI_SUCCESS;
  } else if (Status == EFI_BUFFER_TOO_SMALL && Buffer != NULL) {
    AsciiPrint ("\n  PXE-E05: Buffer size is smaller than the requested file.\n");
  } else if (Status == EFI_DEVICE_ERROR) {
    AsciiPrint ("\n  PXE-E07: Network device error.\n");
  } else if (Status == EFI_OUT_OF_RESOURCES) {
    AsciiPrint ("\n  PXE-E09: Could not allocate I/O buffers.\n");
  } else if (Status == EFI_NO_MEDIA) {
    AsciiPrint ("\n  PXE-E12: Could not detect network connection.\n");
  } else if (Status == EFI_NO_RESPONSE) {
    AsciiPrint ("\n  PXE-E16: No offer received.\n");
  } else if (Status == EFI_TIMEOUT) {
    AsciiPrint ("\n  PXE-E18: Server response timeout.\n");
  } else if (Status == EFI_ABORTED) {
    AsciiPrint ("\n  PXE-E21: Remote boot cancelled.\n");
  } else if (Status == EFI_ICMP_ERROR) {
    AsciiPrint ("\n  PXE-E22: Client received ICMP error from server.\n");
  } else if (Status == EFI_TFTP_ERROR) {
    AsciiPrint ("\n  PXE-E23: Client received TFTP error from server.\n");
  } else if (Status == EFI_NOT_FOUND) {
    AsciiPrint ("\n  PXE-E53: No boot filename received.\n");
  } else if (Status != EFI_BUFFER_TOO_SMALL) {
    AsciiPrint ("\n  PXE-E99: Unexpected network error.\n");
  }

  return Status;
}

