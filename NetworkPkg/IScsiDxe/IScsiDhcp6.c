/** @file
  iSCSI DHCP6 related configuration routines.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "IScsiImpl.h"

/**
  Extract the Root Path option and get the required target information from
  Boot File Uniform Resource Locator (URL) Option.

  @param[in]       RootPath      The RootPath string.
  @param[in]       Length        Length of the RootPath option payload.
  @param[in, out]  ConfigData    The iSCSI session configuration data read from
                                 nonvolatile device.

  @retval EFI_SUCCESS            All required information is extracted from the
                                 RootPath option.
  @retval EFI_NOT_FOUND          The RootPath is not an iSCSI RootPath.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_INVALID_PARAMETER  The RootPath is malformatted.

**/
EFI_STATUS
IScsiDhcp6ExtractRootPath (
  IN     CHAR8                        *RootPath,
  IN     UINT16                       Length,
  IN OUT ISCSI_ATTEMPT_CONFIG_NVDATA  *ConfigData
  )
{
  EFI_STATUS                   Status;
  UINT16                       IScsiRootPathIdLen;
  CHAR8                        *TmpStr;
  ISCSI_ROOT_PATH_FIELD        Fields[RP_FIELD_IDX_MAX];
  ISCSI_ROOT_PATH_FIELD        *Field;
  UINT32                       FieldIndex;
  UINT8                        Index;
  ISCSI_SESSION_CONFIG_NVDATA  *ConfigNvData;
  EFI_IP_ADDRESS               Ip;
  UINT8                        IpMode;

  ConfigNvData          = &ConfigData->SessionConfigData;
  ConfigNvData->DnsMode = FALSE;
  //
  // "iscsi:"<servername>":"<protocol>":"<port>":"<LUN>":"<targetname>
  //
  IScsiRootPathIdLen = (UINT16)AsciiStrLen (ISCSI_ROOT_PATH_ID);

  if ((Length <= IScsiRootPathIdLen) ||
      (CompareMem (RootPath, ISCSI_ROOT_PATH_ID, IScsiRootPathIdLen) != 0))
  {
    return EFI_NOT_FOUND;
  }

  //
  // Skip the iSCSI RootPath ID "iscsi:".
  //
  RootPath = RootPath + IScsiRootPathIdLen;
  Length   = (UINT16)(Length - IScsiRootPathIdLen);

  TmpStr = (CHAR8 *)AllocatePool (Length + 1);
  if (TmpStr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (TmpStr, RootPath, Length);
  TmpStr[Length] = '\0';

  Index      = 0;
  FieldIndex = 0;
  ZeroMem (&Fields[0], sizeof (Fields));

  //
  // Extract SERVERNAME field in the Root Path option.
  //
  if (TmpStr[Index] != ISCSI_ROOT_PATH_ADDR_START_DELIMITER) {
    //
    // The servername is expressed as domain name.
    //
    ConfigNvData->DnsMode = TRUE;
  } else {
    Index++;
  }

  Fields[RP_FIELD_IDX_SERVERNAME].Str = &TmpStr[Index];

  if (!ConfigNvData->DnsMode) {
    while ((TmpStr[Index] != ISCSI_ROOT_PATH_ADDR_END_DELIMITER) && (Index < Length)) {
      Index++;
    }

    //
    // Skip ']' and ':'.
    //
    TmpStr[Index] = '\0';
    Index        += 2;
  } else {
    while ((TmpStr[Index] != ISCSI_ROOT_PATH_FIELD_DELIMITER) && (Index < Length)) {
      Index++;
    }

    //
    // Skip ':'.
    //
    TmpStr[Index] = '\0';
    Index        += 1;
  }

  Fields[RP_FIELD_IDX_SERVERNAME].Len = (UINT8)AsciiStrLen (Fields[RP_FIELD_IDX_SERVERNAME].Str);

  //
  // Extract others fields in the Root Path option string.
  //
  for (FieldIndex = 1; (FieldIndex < RP_FIELD_IDX_MAX) && (Index < Length); FieldIndex++) {
    if (TmpStr[Index] != ISCSI_ROOT_PATH_FIELD_DELIMITER) {
      Fields[FieldIndex].Str = &TmpStr[Index];
    }

    while ((TmpStr[Index] != ISCSI_ROOT_PATH_FIELD_DELIMITER) && (Index < Length)) {
      Index++;
    }

    if (TmpStr[Index] == ISCSI_ROOT_PATH_FIELD_DELIMITER) {
      if (FieldIndex != RP_FIELD_IDX_TARGETNAME) {
        TmpStr[Index] = '\0';
        Index++;
      }

      if (Fields[FieldIndex].Str != NULL) {
        Fields[FieldIndex].Len = (UINT8)AsciiStrLen (Fields[FieldIndex].Str);
      }
    }
  }

  if (FieldIndex != RP_FIELD_IDX_MAX) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  if ((Fields[RP_FIELD_IDX_SERVERNAME].Str == NULL) ||
      (Fields[RP_FIELD_IDX_TARGETNAME].Str == NULL) ||
      (Fields[RP_FIELD_IDX_PROTOCOL].Len > 1)
      )
  {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  //
  // Get the IP address of the target.
  //
  Field = &Fields[RP_FIELD_IDX_SERVERNAME];
  if (ConfigNvData->IpMode < IP_MODE_AUTOCONFIG) {
    IpMode = ConfigNvData->IpMode;
  } else {
    IpMode = ConfigData->AutoConfigureMode;
  }

  //
  // Server name is expressed as domain name, just save it.
  //
  if (ConfigNvData->DnsMode) {
    if ((Field->Len + 2) > sizeof (ConfigNvData->TargetUrl)) {
      return EFI_INVALID_PARAMETER;
    }

    CopyMem (&ConfigNvData->TargetUrl, Field->Str, Field->Len);
    ConfigNvData->TargetUrl[Field->Len + 1] = '\0';
  } else {
    ZeroMem (&ConfigNvData->TargetUrl, sizeof (ConfigNvData->TargetUrl));
    Status = IScsiAsciiStrToIp (Field->Str, IpMode, &Ip);
    CopyMem (&ConfigNvData->TargetIp, &Ip, sizeof (EFI_IP_ADDRESS));

    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

  //
  // Check the protocol type.
  //
  Field = &Fields[RP_FIELD_IDX_PROTOCOL];
  if ((Field->Str != NULL) && ((*(Field->Str) - '0') != EFI_IP_PROTO_TCP)) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  //
  // Get the port of the iSCSI target.
  //
  Field = &Fields[RP_FIELD_IDX_PORT];
  if (Field->Str != NULL) {
    ConfigNvData->TargetPort = (UINT16)AsciiStrDecimalToUintn (Field->Str);
  } else {
    ConfigNvData->TargetPort = ISCSI_WELL_KNOWN_PORT;
  }

  //
  // Get the LUN.
  //
  Field = &Fields[RP_FIELD_IDX_LUN];
  if (Field->Str != NULL) {
    Status = IScsiAsciiStrToLun (Field->Str, ConfigNvData->BootLun);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  } else {
    ZeroMem (ConfigNvData->BootLun, sizeof (ConfigNvData->BootLun));
  }

  //
  // Get the target iSCSI Name.
  //
  Field = &Fields[RP_FIELD_IDX_TARGETNAME];

  if (AsciiStrLen (Field->Str) > ISCSI_NAME_MAX_SIZE - 1) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  //
  // Validate the iSCSI name.
  //
  Status = IScsiNormalizeName (Field->Str, AsciiStrLen (Field->Str));
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  AsciiStrCpyS (ConfigNvData->TargetName, ISCSI_NAME_MAX_SIZE, Field->Str);

ON_EXIT:

  FreePool (TmpStr);

  return Status;
}

/**
  EFI_DHCP6_INFO_CALLBACK is provided by the consumer of the EFI DHCPv6 Protocol
  instance to intercept events that occurs in the DHCPv6 Information Request
  exchange process.

  @param[in]  This              Pointer to the EFI_DHCP6_PROTOCOL instance that
                                is used to configure this  callback function.
  @param[in]  Context           Pointer to the context that is initialized in
                                the EFI_DHCP6_PROTOCOL.InfoRequest().
  @param[in]  Packet            Pointer to Reply packet that has been received.
                                The EFI DHCPv6 Protocol instance is responsible
                                for freeing the buffer.

  @retval EFI_SUCCESS           Tell the EFI DHCPv6 Protocol instance to finish
                                Information Request exchange process.
  @retval EFI_NOT_READY         Tell the EFI DHCPv6 Protocol instance to continue
                                Information Request exchange process.
  @retval EFI_ABORTED           Tell the EFI DHCPv6 Protocol instance to abort
                                the Information Request exchange process.
  @retval EFI_UNSUPPORTED       Tell the EFI DHCPv6 Protocol instance to finish
                                the Information Request exchange process because some
                                request information are not received.

**/
EFI_STATUS
EFIAPI
IScsiDhcp6ParseReply (
  IN EFI_DHCP6_PROTOCOL  *This,
  IN VOID                *Context,
  IN EFI_DHCP6_PACKET    *Packet
  )
{
  EFI_STATUS                   Status;
  UINT32                       Index;
  UINT32                       OptionCount;
  EFI_DHCP6_PACKET_OPTION      *BootFileOpt;
  EFI_DHCP6_PACKET_OPTION      **OptionList;
  ISCSI_ATTEMPT_CONFIG_NVDATA  *ConfigData;
  UINT16                       ParaLen;

  OptionCount = 0;
  BootFileOpt = NULL;

  Status = This->Parse (This, Packet, &OptionCount, NULL);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return EFI_NOT_READY;
  }

  OptionList = AllocateZeroPool (OptionCount * sizeof (EFI_DHCP6_PACKET_OPTION *));
  if (OptionList == NULL) {
    return EFI_NOT_READY;
  }

  Status = This->Parse (This, Packet, &OptionCount, OptionList);
  if (EFI_ERROR (Status)) {
    Status = EFI_NOT_READY;
    goto Exit;
  }

  ConfigData = (ISCSI_ATTEMPT_CONFIG_NVDATA *)Context;

  for (Index = 0; Index < OptionCount; Index++) {
    OptionList[Index]->OpCode = NTOHS (OptionList[Index]->OpCode);
    OptionList[Index]->OpLen  = NTOHS (OptionList[Index]->OpLen);

    //
    // Get DNS server addresses from this reply packet.
    //
    if (OptionList[Index]->OpCode == DHCP6_OPT_DNS_SERVERS) {
      if (((OptionList[Index]->OpLen & 0xf) != 0) || (OptionList[Index]->OpLen == 0)) {
        Status = EFI_UNSUPPORTED;
        goto Exit;
      }

      //
      // Primary DNS server address.
      //
      CopyMem (&ConfigData->PrimaryDns, &OptionList[Index]->Data[0], sizeof (EFI_IPv6_ADDRESS));

      if (OptionList[Index]->OpLen > 16) {
        //
        // Secondary DNS server address
        //
        CopyMem (&ConfigData->SecondaryDns, &OptionList[Index]->Data[16], sizeof (EFI_IPv6_ADDRESS));
      }
    } else if (OptionList[Index]->OpCode == DHCP6_OPT_BOOT_FILE_URL) {
      //
      // The server sends this option to inform the client about an URL to a boot file.
      //
      BootFileOpt = OptionList[Index];
    } else if (OptionList[Index]->OpCode == DHCP6_OPT_BOOT_FILE_PARAM) {
      //
      // The server sends this option to inform the client about DHCP6 server address.
      //
      if (OptionList[Index]->OpLen < 18) {
        Status = EFI_UNSUPPORTED;
        goto Exit;
      }

      //
      // Check param-len 1, should be 16 bytes.
      //
      CopyMem (&ParaLen, &OptionList[Index]->Data[0], sizeof (UINT16));
      if (NTOHS (ParaLen) != 16) {
        Status = EFI_UNSUPPORTED;
        goto Exit;
      }

      CopyMem (&ConfigData->DhcpServer, &OptionList[Index]->Data[2], sizeof (EFI_IPv6_ADDRESS));
    }
  }

  if (BootFileOpt == NULL) {
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  //
  // Get iSCSI root path from Boot File Uniform Resource Locator (URL) Option
  //
  Status = IScsiDhcp6ExtractRootPath (
             (CHAR8 *)BootFileOpt->Data,
             BootFileOpt->OpLen,
             ConfigData
             );

Exit:

  FreePool (OptionList);
  return Status;
}

/**
  Parse the DHCP ACK to get the address configuration and DNS information.

  @param[in]       Image         The handle of the driver image.
  @param[in]       Controller    The handle of the controller;
  @param[in, out]  ConfigData    The attempt configuration data.

  @retval EFI_SUCCESS            The DNS information is got from the DHCP ACK.
  @retval EFI_NO_MAPPING         DHCP failed to acquire address and other
                                 information.
  @retval EFI_INVALID_PARAMETER  The DHCP ACK's DNS option is malformatted.
  @retval EFI_DEVICE_ERROR       Some unexpected error occurred.
  @retval EFI_OUT_OF_RESOURCES   There is no sufficient resource to finish the
                                 operation.
  @retval EFI_NO_MEDIA           There was a media error.

**/
EFI_STATUS
IScsiDoDhcp6 (
  IN     EFI_HANDLE                   Image,
  IN     EFI_HANDLE                   Controller,
  IN OUT ISCSI_ATTEMPT_CONFIG_NVDATA  *ConfigData
  )
{
  EFI_HANDLE                Dhcp6Handle;
  EFI_DHCP6_PROTOCOL        *Dhcp6;
  EFI_STATUS                Status;
  EFI_STATUS                TimerStatus;
  EFI_DHCP6_PACKET_OPTION   *Oro;
  EFI_DHCP6_RETRANSMISSION  InfoReqReXmit;
  EFI_EVENT                 Timer;
  EFI_STATUS                MediaStatus;

  //
  // Check media status before doing DHCP.
  //
  MediaStatus = EFI_SUCCESS;
  NetLibDetectMediaWaitTimeout (Controller, ISCSI_CHECK_MEDIA_GET_DHCP_WAITING_TIME, &MediaStatus);
  if (MediaStatus != EFI_SUCCESS) {
    AsciiPrint ("\n  Error: Could not detect network connection.\n");
    return EFI_NO_MEDIA;
  }

  //
  // iSCSI will only request target info from DHCPv6 server.
  //
  if (!ConfigData->SessionConfigData.TargetInfoFromDhcp) {
    return EFI_SUCCESS;
  }

  Dhcp6Handle = NULL;
  Dhcp6       = NULL;
  Oro         = NULL;
  Timer       = NULL;

  //
  // Create a DHCP6 child instance and get the protocol.
  //
  Status = NetLibCreateServiceChild (
             Controller,
             Image,
             &gEfiDhcp6ServiceBindingProtocolGuid,
             &Dhcp6Handle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Dhcp6Handle,
                  &gEfiDhcp6ProtocolGuid,
                  (VOID **)&Dhcp6,
                  Image,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Oro = AllocateZeroPool (sizeof (EFI_DHCP6_PACKET_OPTION) + 5);
  if (Oro == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Ask the server to reply with DNS and Boot File URL options by info request.
  // All members in EFI_DHCP6_PACKET_OPTION are in network order.
  //
  Oro->OpCode  = HTONS (DHCP6_OPT_ORO);
  Oro->OpLen   = HTONS (2 * 3);
  Oro->Data[1] = DHCP6_OPT_DNS_SERVERS;
  Oro->Data[3] = DHCP6_OPT_BOOT_FILE_URL;
  Oro->Data[5] = DHCP6_OPT_BOOT_FILE_PARAM;

  InfoReqReXmit.Irt = 4;
  InfoReqReXmit.Mrc = 1;
  InfoReqReXmit.Mrt = 10;
  InfoReqReXmit.Mrd = 30;

  Status = Dhcp6->InfoRequest (
                    Dhcp6,
                    TRUE,
                    Oro,
                    0,
                    NULL,
                    &InfoReqReXmit,
                    NULL,
                    IScsiDhcp6ParseReply,
                    ConfigData
                    );
  if (Status == EFI_NO_MAPPING) {
    Status = gBS->CreateEvent (EVT_TIMER, TPL_CALLBACK, NULL, NULL, &Timer);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    Status = gBS->SetTimer (
                    Timer,
                    TimerRelative,
                    ISCSI_GET_MAPPING_TIMEOUT
                    );

    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    do {
      TimerStatus = gBS->CheckEvent (Timer);

      if (!EFI_ERROR (TimerStatus)) {
        Status = Dhcp6->InfoRequest (
                          Dhcp6,
                          TRUE,
                          Oro,
                          0,
                          NULL,
                          &InfoReqReXmit,
                          NULL,
                          IScsiDhcp6ParseReply,
                          ConfigData
                          );
      }
    } while (TimerStatus == EFI_NOT_READY);
  }

ON_EXIT:

  if (Oro != NULL) {
    FreePool (Oro);
  }

  if (Timer != NULL) {
    gBS->CloseEvent (Timer);
  }

  if (Dhcp6 != NULL) {
    gBS->CloseProtocol (
           Dhcp6Handle,
           &gEfiDhcp6ProtocolGuid,
           Image,
           Controller
           );
  }

  NetLibDestroyServiceChild (
    Controller,
    Image,
    &gEfiDhcp6ServiceBindingProtocolGuid,
    Dhcp6Handle
    );

  return Status;
}
