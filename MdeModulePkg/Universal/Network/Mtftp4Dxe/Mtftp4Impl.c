/** @file
  Interface routine for Mtftp4.

(C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "Mtftp4Impl.h"


/**
  Clean up the MTFTP session to get ready for new operation.

  @param  Instance               The MTFTP session to clean up
  @param  Result                 The result to return to the caller who initiated
                                 the operation.
**/
VOID
Mtftp4CleanOperation (
  IN OUT MTFTP4_PROTOCOL        *Instance,
  IN     EFI_STATUS             Result
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  MTFTP4_BLOCK_RANGE        *Block;
  EFI_MTFTP4_TOKEN          *Token;

  //
  // Free various resources.
  //
  Token = Instance->Token;

  if (Token != NULL) {
    Token->Status = Result;

    if (Token->Event != NULL) {
      gBS->SignalEvent (Token->Event);
    }

    Instance->Token = NULL;
  }

  ASSERT (Instance->UnicastPort != NULL);
  UdpIoCleanIo (Instance->UnicastPort);

  if (Instance->LastPacket != NULL) {
    NetbufFree (Instance->LastPacket);
    Instance->LastPacket = NULL;
  }

  if (Instance->McastUdpPort != NULL) {
    gBS->CloseProtocol (
           Instance->McastUdpPort->UdpHandle,
           &gEfiUdp4ProtocolGuid,
           gMtftp4DriverBinding.DriverBindingHandle,
           Instance->Handle
           );
    UdpIoFreeIo (Instance->McastUdpPort);
    Instance->McastUdpPort = NULL;
  }

  NET_LIST_FOR_EACH_SAFE (Entry, Next, &Instance->Blocks) {
    Block = NET_LIST_USER_STRUCT (Entry, MTFTP4_BLOCK_RANGE, Link);
    RemoveEntryList (Entry);
    FreePool (Block);
  }

  ZeroMem (&Instance->RequestOption, sizeof (MTFTP4_OPTION));

  Instance->Operation     = 0;

  Instance->BlkSize       = MTFTP4_DEFAULT_BLKSIZE;
  Instance->WindowSize    = 1;
  Instance->TotalBlock    = 0;
  Instance->AckedBlock    = 0;
  Instance->LastBlock     = 0;
  Instance->ServerIp      = 0;
  Instance->ListeningPort = 0;
  Instance->ConnectedPort = 0;
  Instance->Gateway       = 0;
  Instance->PacketToLive  = 0;
  Instance->MaxRetry      = 0;
  Instance->CurRetry      = 0;
  Instance->Timeout       = 0;
  Instance->McastIp       = 0;
  Instance->McastPort     = 0;
  Instance->Master        = TRUE;
}


/**
  Check packet for GetInfo.

  GetInfo is implemented with EfiMtftp4ReadFile. It use Mtftp4GetInfoCheckPacket
  to inspect the first packet from server, then abort the session.

  @param  This                   The MTFTP4 protocol instance
  @param  Token                  The user's token
  @param  PacketLen              The length of the packet
  @param  Packet                 The received packet.

  @retval EFI_ABORTED            Abort the ReadFile operation and return.

**/
EFI_STATUS
EFIAPI
Mtftp4GetInfoCheckPacket (
  IN EFI_MTFTP4_PROTOCOL    *This,
  IN EFI_MTFTP4_TOKEN       *Token,
  IN UINT16                 PacketLen,
  IN EFI_MTFTP4_PACKET      *Packet
  )
{
  MTFTP4_GETINFO_STATE      *State;
  EFI_STATUS                Status;
  UINT16                    OpCode;
  EFI_MTFTP4_ERROR_HEADER  *ErrorHeader;

  State   = (MTFTP4_GETINFO_STATE *) Token->Context;
  OpCode   = NTOHS (Packet->OpCode);

  //
  // Set the GetInfo's return status according to the OpCode.
  //
  switch (OpCode) {
  case EFI_MTFTP4_OPCODE_ERROR:
    ErrorHeader = (EFI_MTFTP4_ERROR_HEADER *) Packet;
    if (ErrorHeader->ErrorCode == EFI_MTFTP4_ERRORCODE_FILE_NOT_FOUND) {
      DEBUG ((EFI_D_ERROR, "TFTP error code 1 (File Not Found)\n"));
    } else {
      DEBUG ((EFI_D_ERROR, "TFTP error code %d\n", ErrorHeader->ErrorCode));
    }
    State->Status = EFI_TFTP_ERROR;
    break;

  case EFI_MTFTP4_OPCODE_OACK:
    State->Status = EFI_SUCCESS;
    break;

  default:
    State->Status = EFI_PROTOCOL_ERROR;
  }

  //
  // Allocate buffer then copy the packet over. Use gBS->AllocatePool
  // in case AllocatePool will implements something tricky.
  //
  Status = gBS->AllocatePool (EfiBootServicesData, PacketLen, (VOID **) State->Packet);

  if (EFI_ERROR (Status)) {
    State->Status = EFI_OUT_OF_RESOURCES;
    return EFI_ABORTED;
  }

  *(State->PacketLen) = PacketLen;
  CopyMem (*(State->Packet), Packet, PacketLen);

  return EFI_ABORTED;
}


/**
  Check whether the override data is valid.

  It will first validate whether the server is a valid unicast. If a gateway
  is provided in the Override, it also check that it is a unicast on the
  connected network.

  @param  Instance               The MTFTP instance
  @param  Override               The override data to validate.

  @retval TRUE                   The override data is valid
  @retval FALSE                  The override data is invalid

**/
BOOLEAN
Mtftp4OverrideValid (
  IN MTFTP4_PROTOCOL          *Instance,
  IN EFI_MTFTP4_OVERRIDE_DATA *Override
  )
{
  EFI_MTFTP4_CONFIG_DATA    *Config;
  IP4_ADDR                  Ip;
  IP4_ADDR                  Netmask;
  IP4_ADDR                  Gateway;

  CopyMem (&Ip, &Override->ServerIp, sizeof (IP4_ADDR));
  if (IP4_IS_UNSPECIFIED (NTOHL (Ip)) || IP4_IS_LOCAL_BROADCAST (NTOHL (Ip))) {
    return FALSE;
  }

  Config = &Instance->Config;

  CopyMem (&Gateway, &Override->GatewayIp, sizeof (IP4_ADDR));
  Gateway = NTOHL (Gateway);

  if (!Config->UseDefaultSetting && (Gateway != 0)) {
    CopyMem (&Netmask, &Config->SubnetMask, sizeof (IP4_ADDR));
    CopyMem (&Ip, &Config->StationIp, sizeof (IP4_ADDR));

    Netmask = NTOHL (Netmask);
    Ip      = NTOHL (Ip);

    if ((Netmask != 0 && !NetIp4IsUnicast (Gateway, Netmask)) || !IP4_NET_EQUAL (Gateway, Ip, Netmask)) {
      return FALSE;
    }
  }

  return TRUE;
}


/**
  Poll the UDP to get the IP4 default address, which may be retrieved
  by DHCP.

  The default time out value is 5 seconds. If IP has retrieved the default address,
  the UDP is reconfigured.

  @param  Instance               The Mtftp instance
  @param  UdpIo                  The UDP_IO to poll
  @param  UdpCfgData             The UDP configure data to reconfigure the UDP_IO

  @retval TRUE                   The default address is retrieved and UDP is reconfigured.
  @retval FALSE                  Some error occured.

**/
BOOLEAN
Mtftp4GetMapping (
  IN MTFTP4_PROTOCOL        *Instance,
  IN UDP_IO                 *UdpIo,
  IN EFI_UDP4_CONFIG_DATA   *UdpCfgData
  )
{
  MTFTP4_SERVICE            *Service;
  EFI_IP4_MODE_DATA         Ip4Mode;
  EFI_UDP4_PROTOCOL         *Udp;
  EFI_STATUS                Status;

  ASSERT (Instance->Config.UseDefaultSetting);

  Service = Instance->Service;
  Udp     = UdpIo->Protocol.Udp4;

  Status = gBS->SetTimer (
                  Service->TimerToGetMap,
                  TimerRelative,
                  MTFTP4_TIME_TO_GETMAP * TICKS_PER_SECOND
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  while (EFI_ERROR (gBS->CheckEvent (Service->TimerToGetMap))) {
    Udp->Poll (Udp);

    if (!EFI_ERROR (Udp->GetModeData (Udp, NULL, &Ip4Mode, NULL, NULL)) &&
        Ip4Mode.IsConfigured) {

      Udp->Configure (Udp, NULL);
      return (BOOLEAN) (Udp->Configure (Udp, UdpCfgData) == EFI_SUCCESS);
    }
  }

  return FALSE;
}


/**
  Configure the UDP port for unicast receiving.

  @param  UdpIo                  The UDP_IO instance
  @param  Instance               The MTFTP session

  @retval EFI_SUCCESS            The UDP port is successfully configured for the
                                 session to unicast receive.

**/
EFI_STATUS
Mtftp4ConfigUnicastPort (
  IN UDP_IO                 *UdpIo,
  IN MTFTP4_PROTOCOL        *Instance
  )
{
  EFI_MTFTP4_CONFIG_DATA    *Config;
  EFI_UDP4_CONFIG_DATA      UdpConfig;
  EFI_STATUS                Status;
  IP4_ADDR                  Ip;

  Config = &Instance->Config;

  UdpConfig.AcceptBroadcast    = FALSE;
  UdpConfig.AcceptPromiscuous  = FALSE;
  UdpConfig.AcceptAnyPort      = FALSE;
  UdpConfig.AllowDuplicatePort = FALSE;
  UdpConfig.TypeOfService      = 0;
  UdpConfig.TimeToLive         = 64;
  UdpConfig.DoNotFragment      = FALSE;
  UdpConfig.ReceiveTimeout     = 0;
  UdpConfig.TransmitTimeout    = 0;
  UdpConfig.UseDefaultAddress  = Config->UseDefaultSetting;
  IP4_COPY_ADDRESS (&UdpConfig.StationAddress, &Config->StationIp);
  IP4_COPY_ADDRESS (&UdpConfig.SubnetMask, &Config->SubnetMask);
  UdpConfig.StationPort        = Config->LocalPort;
  UdpConfig.RemotePort         = 0;

  Ip = HTONL (Instance->ServerIp);
  IP4_COPY_ADDRESS (&UdpConfig.RemoteAddress, &Ip);

  Status = UdpIo->Protocol.Udp4->Configure (UdpIo->Protocol.Udp4, &UdpConfig);

  if ((Status == EFI_NO_MAPPING) && Mtftp4GetMapping (Instance, UdpIo, &UdpConfig)) {
    return EFI_SUCCESS;
  }

  if (!Config->UseDefaultSetting && !EFI_IP4_EQUAL (&mZeroIp4Addr, &Config->GatewayIp)) {
    //
    // The station IP address is manually configured and the Gateway IP is not 0.
    // Add the default route for this UDP instance.
    //
    Status = UdpIo->Protocol.Udp4->Routes (
                                     UdpIo->Protocol.Udp4,
                                     FALSE,
                                     &mZeroIp4Addr,
                                     &mZeroIp4Addr,
                                     &Config->GatewayIp
                                     );
    if (EFI_ERROR (Status)) {
      UdpIo->Protocol.Udp4->Configure (UdpIo->Protocol.Udp4, NULL);
    }
  }
  return Status;
}


/**
  Start the MTFTP session to do the operation, such as read file,
  write file, and read directory.

  @param  This                   The MTFTP session
  @param  Token                  The token than encapsues the user's request.
  @param  Operation              The operation to do

  @retval EFI_INVALID_PARAMETER  Some of the parameters are invalid.
  @retval EFI_NOT_STARTED        The MTFTP session hasn't been configured.
  @retval EFI_ALREADY_STARTED    There is pending operation for the session.
  @retval EFI_SUCCESS            The operation is successfully started.

**/
EFI_STATUS
Mtftp4Start (
  IN EFI_MTFTP4_PROTOCOL    *This,
  IN EFI_MTFTP4_TOKEN       *Token,
  IN UINT16                 Operation
  )
{
  MTFTP4_PROTOCOL           *Instance;
  EFI_MTFTP4_OVERRIDE_DATA  *Override;
  EFI_MTFTP4_CONFIG_DATA    *Config;
  EFI_TPL                   OldTpl;
  EFI_STATUS                Status;
  EFI_STATUS                TokenStatus;

  //
  // Validate the parameters
  //
  if ((This == NULL) || (Token == NULL) || (Token->Filename == NULL) ||
      ((Token->OptionCount != 0) && (Token->OptionList == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // User must provide at least one method to collect the data for download.
  //
  if (((Operation == EFI_MTFTP4_OPCODE_RRQ) || (Operation == EFI_MTFTP4_OPCODE_DIR)) &&
      ((Token->Buffer == NULL) && (Token->CheckPacket == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // User must provide at least one method to provide the data for upload.
  //
  if ((Operation == EFI_MTFTP4_OPCODE_WRQ) &&
     ((Token->Buffer == NULL) && (Token->PacketNeeded == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = MTFTP4_PROTOCOL_FROM_THIS (This);

  Status      = EFI_SUCCESS;
  TokenStatus = EFI_SUCCESS;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (Instance->State != MTFTP4_STATE_CONFIGED) {
    Status = EFI_NOT_STARTED;
  }

  if (Instance->Operation != 0) {
    Status = EFI_ACCESS_DENIED;
  }

  if ((Token->OverrideData != NULL) && !Mtftp4OverrideValid (Instance, Token->OverrideData)) {
    Status = EFI_INVALID_PARAMETER;
  }

  if (EFI_ERROR (Status)) {
    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  //
  // Set the Operation now to prevent the application start other
  // operations.
  //
  Instance->Operation = Operation;
  Override            = Token->OverrideData;

  if (Token->OptionCount != 0) {
    Status = Mtftp4ParseOption (
               Token->OptionList,
               Token->OptionCount,
               TRUE,
               Instance->Operation,
               &Instance->RequestOption
               );

    if (EFI_ERROR (Status)) {
      TokenStatus = EFI_DEVICE_ERROR;
      goto ON_ERROR;
    }
  }

  //
  // Set the operation parameters from the configuration or override data.
  //
  Config                  = &Instance->Config;
  Instance->Token         = Token;
  Instance->BlkSize       = MTFTP4_DEFAULT_BLKSIZE;
  Instance->WindowSize    = MTFTP4_DEFAULT_WINDOWSIZE;

  CopyMem (&Instance->ServerIp, &Config->ServerIp, sizeof (IP4_ADDR));
  Instance->ServerIp      = NTOHL (Instance->ServerIp);

  Instance->ListeningPort = Config->InitialServerPort;
  Instance->ConnectedPort = 0;

  CopyMem (&Instance->Gateway, &Config->GatewayIp, sizeof (IP4_ADDR));
  Instance->Gateway       = NTOHL (Instance->Gateway);

  Instance->MaxRetry      = Config->TryCount;
  Instance->Timeout       = Config->TimeoutValue;
  Instance->Master        = TRUE;

  if (Override != NULL) {
    CopyMem (&Instance->ServerIp, &Override->ServerIp, sizeof (IP4_ADDR));
    CopyMem (&Instance->Gateway, &Override->GatewayIp, sizeof (IP4_ADDR));

    Instance->ServerIp      = NTOHL (Instance->ServerIp);
    Instance->Gateway       = NTOHL (Instance->Gateway);

    Instance->ListeningPort = Override->ServerPort;
    Instance->MaxRetry      = Override->TryCount;
    Instance->Timeout       = Override->TimeoutValue;
  }

  if (Instance->ListeningPort == 0) {
    Instance->ListeningPort = MTFTP4_DEFAULT_SERVER_PORT;
  }

  if (Instance->MaxRetry == 0) {
    Instance->MaxRetry = MTFTP4_DEFAULT_RETRY;
  }

  if (Instance->Timeout == 0) {
    Instance->Timeout = MTFTP4_DEFAULT_TIMEOUT;
  }

  //
  // Config the unicast UDP child to send initial request
  //
  Status = Mtftp4ConfigUnicastPort (Instance->UnicastPort, Instance);
  if (EFI_ERROR (Status)) {
    TokenStatus = EFI_DEVICE_ERROR;
    goto ON_ERROR;
  }

  //
  // Set initial status.
  //
  Token->Status = EFI_NOT_READY;

  //
  // Build and send an initial requests
  //
  if (Operation == EFI_MTFTP4_OPCODE_WRQ) {
    Status = Mtftp4WrqStart (Instance, Operation);
  } else {
    Status = Mtftp4RrqStart (Instance, Operation);
  }

  if (EFI_ERROR (Status)) {
    TokenStatus = EFI_DEVICE_ERROR;
    goto ON_ERROR;
  }

  gBS->RestoreTPL(OldTpl);

  if (Token->Event != NULL) {
    return EFI_SUCCESS;
  }

  //
  // Return immediately for asynchronous operation or poll the
  // instance for synchronous operation.
  //
  while (Token->Status == EFI_NOT_READY) {
    This->Poll (This);
  }

  return Token->Status;

ON_ERROR:
  Mtftp4CleanOperation (Instance, TokenStatus);
  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  Reads the current operational settings.

  The GetModeData()function reads the current operational settings of this
  EFI MTFTPv4 Protocol driver instance.

  @param  This                   Pointer to the EFI_MTFTP4_PROTOCOL instance.
  @param  ModeData               Pointer to storage for the EFI MTFTPv4 Protocol
                                 driver mode data.

  @retval EFI_SUCCESS            The configuration data was successfully returned.
  @retval EFI_OUT_OF_RESOURCES   The required mode data could not be allocated.
  @retval EFI_INVALID_PARAMETER  This is NULL or ModeData is NULL.

**/
EFI_STATUS
EFIAPI
EfiMtftp4GetModeData (
  IN     EFI_MTFTP4_PROTOCOL    *This,
     OUT EFI_MTFTP4_MODE_DATA  *ModeData
  )
{
  MTFTP4_PROTOCOL  *Instance;
  EFI_TPL          OldTpl;

  if ((This == NULL) || (ModeData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance                         = MTFTP4_PROTOCOL_FROM_THIS (This);
  CopyMem(&ModeData->ConfigData, &Instance->Config, sizeof (Instance->Config));
  ModeData->SupportedOptionCount   = MTFTP4_SUPPORTED_OPTIONS;
  ModeData->SupportedOptoins       = (UINT8 **) mMtftp4SupportedOptions;
  ModeData->UnsupportedOptionCount = 0;
  ModeData->UnsupportedOptoins     = NULL;

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}



/**
  Initializes, changes, or resets the default operational setting for this
  EFI MTFTPv4 Protocol driver instance.

  The Configure() function is used to set and change the configuration data for
  this EFI MTFTPv4 Protocol driver instance. The configuration data can be reset
  to startup defaults by calling Configure() with MtftpConfigData set to NULL.
  Whenever the instance is reset, any pending operation is aborted. By changing
  the EFI MTFTPv4 Protocol driver instance configuration data, the client can
  connect to different MTFTPv4 servers. The configuration parameters in
  MtftpConfigData are used as the default parameters in later MTFTPv4 operations
  and can be overridden in later operations.

  @param  This                   Pointer to the EFI_MTFTP4_PROTOCOL instance
  @param  ConfigData             MtftpConfigDataPointer to the configuration data
                                 structure

  @retval EFI_SUCCESS            The EFI MTFTPv4 Protocol driver was configured
                                 successfully.
  @retval EFI_INVALID_PARAMETER  One or more following conditions are TRUE:
                                 1.This is NULL.
                                 2.MtftpConfigData.UseDefaultSetting is FALSE and
                                   MtftpConfigData.StationIp is not a valid IPv4
                                   unicast address.
                                 3.MtftpCofigData.UseDefaultSetting is FALSE and
                                   MtftpConfigData.SubnetMask is invalid.
                                 4.MtftpCofigData.ServerIp is not a valid IPv4
                                   unicast address.
                                 5.MtftpConfigData.UseDefaultSetting is FALSE and
                                   MtftpConfigData.GatewayIp is not a valid IPv4
                                   unicast address or is not in the same subnet
                                   with station address.
  @retval EFI_ACCESS_DENIED      The EFI configuration could not be changed at this
                                 time because there is one MTFTP background operation
                                 in progress.
  @retval EFI_NO_MAPPING         When using a default address, configuration
                                 (DHCP, BOOTP, RARP, etc.) has not finished yet.
  @retval EFI_UNSUPPORTED        A configuration protocol (DHCP, BOOTP, RARP, etc.)
                                 could not be located when clients choose to use
                                 the default address settings.
  @retval EFI_OUT_OF_RESOURCES   The EFI MTFTPv4 Protocol driver instance data could
                                 not be allocated.
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
                                 The EFI MTFTPv4 Protocol driver instance is not
                                 configured.

**/
EFI_STATUS
EFIAPI
EfiMtftp4Configure (
  IN EFI_MTFTP4_PROTOCOL    *This,
  IN EFI_MTFTP4_CONFIG_DATA *ConfigData
  )
{
  MTFTP4_PROTOCOL           *Instance;
  EFI_TPL                   OldTpl;
  IP4_ADDR                  Ip;
  IP4_ADDR                  Netmask;
  IP4_ADDR                  Gateway;
  IP4_ADDR                  ServerIp;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = MTFTP4_PROTOCOL_FROM_THIS (This);

  if (ConfigData == NULL) {
    //
    // Reset the operation if ConfigData is NULL
    //
    OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

    Mtftp4CleanOperation (Instance, EFI_ABORTED);
    ZeroMem (&Instance->Config, sizeof (EFI_MTFTP4_CONFIG_DATA));
    Instance->State = MTFTP4_STATE_UNCONFIGED;

    gBS->RestoreTPL (OldTpl);

  } else {
    //
    // Configure the parameters for new operation.
    //
    CopyMem (&Ip, &ConfigData->StationIp, sizeof (IP4_ADDR));
    CopyMem (&Netmask, &ConfigData->SubnetMask, sizeof (IP4_ADDR));
    CopyMem (&Gateway, &ConfigData->GatewayIp, sizeof (IP4_ADDR));
    CopyMem (&ServerIp, &ConfigData->ServerIp, sizeof (IP4_ADDR));

    Ip       = NTOHL (Ip);
    Netmask  = NTOHL (Netmask);
    Gateway  = NTOHL (Gateway);
    ServerIp = NTOHL (ServerIp);

    if (ServerIp == 0 || IP4_IS_LOCAL_BROADCAST (ServerIp)) {
      return EFI_INVALID_PARAMETER;
    }

    if (!ConfigData->UseDefaultSetting &&
        ((!IP4_IS_VALID_NETMASK (Netmask) || (Netmask != 0 && !NetIp4IsUnicast (Ip, Netmask))))) {

      return EFI_INVALID_PARAMETER;
    }

    if ((Gateway != 0) &&
        ((Netmask != 0xFFFFFFFF && !IP4_NET_EQUAL (Gateway, Ip, Netmask)) || (Netmask != 0 && !NetIp4IsUnicast (Gateway, Netmask)))) {

      return EFI_INVALID_PARAMETER;
    }

    OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

    if ((Instance->State == MTFTP4_STATE_CONFIGED) && (Instance->Operation != 0)) {
      gBS->RestoreTPL (OldTpl);
      return EFI_ACCESS_DENIED;
    }

    CopyMem(&Instance->Config, ConfigData, sizeof (*ConfigData));;
    Instance->State = MTFTP4_STATE_CONFIGED;

    gBS->RestoreTPL (OldTpl);
  }

  return EFI_SUCCESS;
}



/**
  Parses the options in an MTFTPv4 OACK packet.

  The ParseOptions() function parses the option fields in an MTFTPv4 OACK packet
  and returns the number of options that were found and optionally a list of
  pointers to the options in the packet.
  If one or more of the option fields are not valid, then EFI_PROTOCOL_ERROR is
  returned and *OptionCount and *OptionList stop at the last valid option.
  The OptionList is allocated by this function, and caller should free it when used.

  @param  This                   Pointer to the EFI_MTFTP4_PROTOCOL instance.
  @param  PacketLen              Length of the OACK packet to be parsed.
  @param  Packet                 Pointer to the OACK packet to be parsed.
  @param  OptionCount            Pointer to the number of options in following OptionList.
  @param  OptionList             Pointer to EFI_MTFTP4_OPTION storage. Call the
                                 EFI Boot Service FreePool() to release theOptionList
                                 if the options in this OptionList are not needed
                                 any more

  @retval EFI_SUCCESS            The OACK packet was valid and the OptionCount and
                                 OptionList parameters have been updated.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 1.PacketLen is 0.
                                 2.Packet is NULL or Packet is not a valid MTFTPv4 packet.
                                 3.OptionCount is NULL.
  @retval EFI_NOT_FOUND          No options were found in the OACK packet.
  @retval EFI_OUT_OF_RESOURCES   Storage for the OptionList array cannot be allocated.
  @retval EFI_PROTOCOL_ERROR     One or more of the option fields is invalid.

**/
EFI_STATUS
EFIAPI
EfiMtftp4ParseOptions (
  IN     EFI_MTFTP4_PROTOCOL    *This,
  IN     UINT32                 PacketLen,
  IN     EFI_MTFTP4_PACKET      *Packet,
     OUT UINT32                 *OptionCount,
     OUT EFI_MTFTP4_OPTION      **OptionList          OPTIONAL
  )
{
  EFI_STATUS                Status;

  if ((This == NULL) || (PacketLen < MTFTP4_OPCODE_LEN) ||
      (Packet == NULL) || (OptionCount == NULL)) {

    return EFI_INVALID_PARAMETER;
  }

  Status = Mtftp4ExtractOptions (Packet, PacketLen, OptionCount, OptionList);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (*OptionCount == 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}


/**
  Downloads a file from an MTFTPv4 server.

  The ReadFile() function is used to initialize and start an MTFTPv4 download
  process and optionally wait for completion. When the download operation completes,
  whether successfully or not, the Token.Status field is updated by the EFI MTFTPv4
  Protocol driver and then Token.Event is signaled (if it is not NULL).
  Data can be downloaded from the MTFTPv4 server into either of the following locations:
  1.A fixed buffer that is pointed to by Token.Buffer
  2.A download service function that is pointed to by Token.CheckPacket
  If both Token.Buffer and Token.CheckPacket are used, then Token.CheckPacket
  will be called first. If the call is successful, the packet will be stored in
  Token.Buffer.

  @param  This                  Pointer to the EFI_MTFTP4_PROTOCOL instance
  @param  Token                 Pointer to the token structure to provide the
                                parameters that are used in this operation.

  @retval EFI_SUCCESS           The data file has been transferred successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_BUFFER_TOO_SMALL  BufferSize is not large enough to hold the downloaded
                                data in downloading process.
  @retval EFI_ABORTED           Current operation is aborted by user.
  @retval EFI_ICMP_ERROR        An ICMP ERROR packet was received.
  @retval EFI_TIMEOUT           No responses were received from the MTFTPv4 server.
  @retval EFI_TFTP_ERROR        An MTFTPv4 ERROR packet was received.
  @retval EFI_DEVICE_ERROR      An unexpected network error or system error occurred.
  @retval EFI_NO_MEDIA          There was a media error.

**/
EFI_STATUS
EFIAPI
EfiMtftp4ReadFile (
  IN EFI_MTFTP4_PROTOCOL    *This,
  IN EFI_MTFTP4_TOKEN       *Token
  )
{
  return Mtftp4Start (This, Token, EFI_MTFTP4_OPCODE_RRQ);
}


/**
  Sends a data file to an MTFTPv4 server. May be unsupported in some EFI implementations

  The WriteFile() function is used to initialize an uploading operation with the
  given option list and optionally wait for completion. If one or more of the
  options is not supported by the server, the unsupported options are ignored and
  a standard TFTP process starts instead. When the upload process completes,
  whether successfully or not, Token.Event is signaled, and the EFI MTFTPv4 Protocol
  driver updates Token.Status.
  The caller can supply the data to be uploaded in the following two modes:
  1.Through the user-provided buffer
  2.Through a callback function
  With the user-provided buffer, the Token.BufferSize field indicates the length
  of the buffer, and the driver will upload the data in the buffer. With an
  EFI_MTFTP4_PACKET_NEEDED callback function, the driver will call this callback
  function to get more data from the user to upload. See the definition of
  EFI_MTFTP4_PACKET_NEEDED for more information. These two modes cannot be used at
  the same time. The callback function will be ignored if the user provides the buffer.

  @param  This                   Pointer to the EFI_MTFTP4_PROTOCOL instance.
  @param  Token                  Pointer to the token structure to provide the
                                 parameters that are used in this function

  @retval EFI_SUCCESS            The upload session has started.
  @retval EFI_UNSUPPORTED        The operation is not supported by this implementation.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 1. This is NULL.
                                 2. Token is NULL.
                                 3. Token.Filename is NULL.
                                 4. Token.OptionCount is not zero and
                                    Token.OptionList is NULL.
                                 5. One or more options in Token.OptionList have wrong
                                    format.
                                 6. Token.Buffer and Token.PacketNeeded are both
                                    NULL.
                                 7. One or more IPv4 addresses in Token.OverrideData
                                    are not valid unicast IPv4 addresses if
                                    Token.OverrideData is not NULL.
  @retval EFI_UNSUPPORTED        One or more options in the Token.OptionList are in the
                                 unsupported list of structure EFI_MTFTP4_MODE_DATA.
  @retval EFI_NOT_STARTED        The EFI MTFTPv4 Protocol driver has not been started.
  @retval EFI_NO_MAPPING         When using a default address, configuration (DHCP,
                                 BOOTP, RARP, etc.) is not finished yet.
  @retval EFI_ALREADY_STARTED    This Token is already being used in another MTFTPv4
                                 session.
  @retval EFI_OUT_OF_RESOURCES   Required system resources could not be allocated.
  @retval EFI_ACCESS_DENIED      The previous operation has not completed yet.
  @retval EFI_DEVICE_ERROR       An unexpected network error or system error occurred.

**/
EFI_STATUS
EFIAPI
EfiMtftp4WriteFile (
  IN EFI_MTFTP4_PROTOCOL    *This,
  IN EFI_MTFTP4_TOKEN       *Token
  )
{
  return Mtftp4Start (This, Token, EFI_MTFTP4_OPCODE_WRQ);
}


/**
  Downloads a data file "directory" from an MTFTPv4 server.
  May be unsupported in some EFI implementations

  The ReadDirectory() function is used to return a list of files on the MTFTPv4
  server that are logically (or operationally) related to Token.Filename. The
  directory request packet that is sent to the server is built with the option
  list that was provided by caller, if present.
  The file information that the server returns is put into either of the following
  locations:
  1.A fixed buffer that is pointed to by Token.Buffer
  2.A download service function that is pointed to by Token.CheckPacket
  If both Token.Buffer and Token.CheckPacket are used, then Token.CheckPacket will
  be called first. If the call is successful, the packet will be stored in Token.Buffer.
  The returned directory listing in the Token.Buffer or EFI_MTFTP4_PACKET consists
  of a list of two or three variable-length ASCII strings, each terminated by a
  null character, for each file in the directory. If the multicast option is involved,
  the first field of each directory entry is the static multicast IP address and
  UDP port number that is associated with the file name. The format of the field
  is ip:ip:ip:ip:port. If the multicast option is not involved, this field and its
  terminating null character are not present.
  The next field of each directory entry is the file name and the last field is
  the file information string. The information string contains the file size and
  the create/modify timestamp. The format of the information string is filesize
  yyyy-mm-dd hh:mm:ss:ffff. The timestamp is Coordinated Universal Time
  (UTC; also known as Greenwich Mean Time [GMT]).
  The only difference between ReadFile and ReadDirectory is the opcode used.

  @param  This                   Pointer to the EFI_MTFTP4_PROTOCOL instance
  @param  Token                  Pointer to the token structure to provide the
                                 parameters that are used in this function

  @retval EFI_SUCCESS            The MTFTPv4 related file "directory" has been downloaded.
  @retval EFI_UNSUPPORTED        The operation is not supported by this implementation.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 1. This is NULL.
                                 2. Token is NULL.
                                 3. Token.Filename is NULL.
                                 4. Token.OptionCount is not zero and
                                    Token.OptionList is NULL.
                                 5. One or more options in Token.OptionList have wrong
                                    format.
                                 6. Token.Buffer and Token.PacketNeeded are both
                                    NULL.
                                 7. One or more IPv4 addresses in Token.OverrideData
                                    are not valid unicast IPv4 addresses if
                                    Token.OverrideData is not NULL.
  @retval EFI_UNSUPPORTED        One or more options in the Token.OptionList are in the
                                 unsupported list of structure EFI_MTFTP4_MODE_DATA.
  @retval EFI_NOT_STARTED        The EFI MTFTPv4 Protocol driver has not been started.
  @retval EFI_NO_MAPPING         When using a default address, configuration (DHCP,
                                 BOOTP, RARP, etc.) is not finished yet.
  @retval EFI_ALREADY_STARTED    This Token is already being used in another MTFTPv4
                                 session.
  @retval EFI_OUT_OF_RESOURCES   Required system resources could not be allocated.
  @retval EFI_ACCESS_DENIED      The previous operation has not completed yet.
  @retval EFI_DEVICE_ERROR       An unexpected network error or system error occurred.

**/
EFI_STATUS
EFIAPI
EfiMtftp4ReadDirectory (
  IN EFI_MTFTP4_PROTOCOL        *This,
  IN EFI_MTFTP4_TOKEN           *Token
  )
{
  return Mtftp4Start (This, Token, EFI_MTFTP4_OPCODE_DIR);
}


/**
  Gets information about a file from an MTFTPv4 server.

  The GetInfo() function assembles an MTFTPv4 request packet with options;
  sends it to the MTFTPv4 server; and may return an MTFTPv4 OACK, MTFTPv4 ERROR,
  or ICMP ERROR packet. Retries occur only if no response packets are received
  from the MTFTPv4 server before the timeout expires.
  It is implemented with EfiMtftp4ReadFile: build a token, then pass it to
  EfiMtftp4ReadFile. In its check packet callback abort the opertions.

  @param  This                   Pointer to the EFI_MTFTP4_PROTOCOL instance
  @param  OverrideData           Data that is used to override the existing
                                 parameters. If NULL, the default parameters that
                                 were set in the EFI_MTFTP4_PROTOCOL.Configure()
                                 function are used
  @param  Filename               Pointer to null-terminated ASCII file name string
  @param  ModeStr                Pointer to null-terminated ASCII mode string. If NULL, "octet"
                                 will be used
  @param  OptionCount            Number of option/value string pairs in OptionList
  @param  OptionList             Pointer to array of option/value string pairs.
                                 Ignored if OptionCount is zero
  @param  PacketLength           The number of bytes in the returned packet
  @param  Packet                 PacketThe pointer to the received packet. This
                                 buffer must be freed by the caller.

  @retval EFI_SUCCESS            An MTFTPv4 OACK packet was received and is in
                                 the Buffer.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 1.This is NULL.
                                 2.Filename is NULL.
                                 3.OptionCount is not zero and OptionList is NULL.
                                 4.One or more options in OptionList have wrong format.
                                 5.PacketLength is NULL.
                                 6.One or more IPv4 addresses in OverrideData are
                                   not valid unicast IPv4 addresses if OverrideData
                                   is not NULL.
  @retval EFI_UNSUPPORTED        One or more options in the OptionList are in the
                                 unsupported list of structure EFI_MTFTP4_MODE_DATA
  @retval EFI_NOT_STARTED        The EFI MTFTPv4 Protocol driver has not been started.
  @retval EFI_NO_MAPPING         When using a default address, configuration (DHCP,
                                 BOOTP, RARP, etc.) has not finished yet.
  @retval EFI_ACCESS_DENIED      The previous operation has not completed yet.
  @retval EFI_OUT_OF_RESOURCES   Required system resources could not be allocated.
  @retval EFI_TFTP_ERROR         An MTFTPv4 ERROR packet was received and is in
                                 the Buffer.
  @retval EFI_ICMP_ERROR         An ICMP ERROR packet was received and the Packet
                                 is set to NULL.
  @retval EFI_PROTOCOL_ERROR     An unexpected MTFTPv4 packet was received and is
                                 in the Buffer.
  @retval EFI_TIMEOUT            No responses were received from the MTFTPv4 server.
  @retval EFI_DEVICE_ERROR       An unexpected network error or system error occurred.
  @retval EFI_NO_MEDIA           There was a media error.

**/
EFI_STATUS
EFIAPI
EfiMtftp4GetInfo (
  IN     EFI_MTFTP4_PROTOCOL      *This,
  IN     EFI_MTFTP4_OVERRIDE_DATA *OverrideData        OPTIONAL,
  IN     UINT8                    *Filename,
  IN     UINT8                    *ModeStr             OPTIONAL,
  IN     UINT8                    OptionCount,
  IN     EFI_MTFTP4_OPTION        *OptionList          OPTIONAL,
     OUT UINT32                   *PacketLength,
     OUT EFI_MTFTP4_PACKET        **Packet             OPTIONAL
  )
{
  EFI_MTFTP4_TOKEN          Token;
  MTFTP4_GETINFO_STATE      State;
  EFI_STATUS                Status;

  if ((This == NULL) || (Filename == NULL) || (PacketLength == NULL) ||
      ((OptionCount != 0) && (OptionList == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  if (Packet != NULL) {
    *Packet = NULL;
  }

  *PacketLength         = 0;
  State.Packet          = Packet;
  State.PacketLen       = PacketLength;
  State.Status          = EFI_SUCCESS;

  //
  // Fill in the Token to issue an synchronous ReadFile operation
  //
  Token.Status          = EFI_SUCCESS;
  Token.Event           = NULL;
  Token.OverrideData    = OverrideData;
  Token.Filename        = Filename;
  Token.ModeStr         = ModeStr;
  Token.OptionCount     = OptionCount;
  Token.OptionList      = OptionList;
  Token.BufferSize      = 0;
  Token.Buffer          = NULL;
  Token.Context         = &State;
  Token.CheckPacket     = Mtftp4GetInfoCheckPacket;
  Token.TimeoutCallback = NULL;
  Token.PacketNeeded    = NULL;

  Status                = EfiMtftp4ReadFile (This, &Token);

  if (EFI_ABORTED == Status) {
    return State.Status;
  }

  return Status;
}

/**
  Polls for incoming data packets and processes outgoing data packets.

  The Poll() function can be used by network drivers and applications to increase
  the rate that data packets are moved between the communications device and the
  transmit and receive queues.
  In some systems, the periodic timer event in the managed network driver may not
  poll the underlying communications device fast enough to transmit and/or receive
  all data packets without missing incoming packets or dropping outgoing packets.
  Drivers and applications that are experiencing packet loss should try calling
  the Poll() function more often.

  @param  This                   Pointer to the EFI_MTFTP4_PROTOCOL instance

  @retval EFI_SUCCESS            Incoming or outgoing data was processed.
  @retval EFI_NOT_STARTED        This EFI MTFTPv4 Protocol instance has not been started.
  @retval EFI_NO_MAPPING         When using a default address, configuration (DHCP,
                                 BOOTP, RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
  @retval EFI_TIMEOUT            Data was dropped out of the transmit and/or receive
                                 queue. Consider increasing the polling rate.

**/
EFI_STATUS
EFIAPI
EfiMtftp4Poll (
  IN EFI_MTFTP4_PROTOCOL    *This
  )
{
  MTFTP4_PROTOCOL           *Instance;
  EFI_UDP4_PROTOCOL         *Udp;
  EFI_STATUS                Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = MTFTP4_PROTOCOL_FROM_THIS (This);

  if (Instance->State == MTFTP4_STATE_UNCONFIGED) {
    return EFI_NOT_STARTED;
  } else if (Instance->State == MTFTP4_STATE_DESTROY) {
    return EFI_DEVICE_ERROR;
  }

  Udp = Instance->UnicastPort->Protocol.Udp4;
  Status = Udp->Poll (Udp);
  Mtftp4OnTimerTick (NULL, Instance->Service);
  return Status;
}

EFI_MTFTP4_PROTOCOL gMtftp4ProtocolTemplate = {
  EfiMtftp4GetModeData,
  EfiMtftp4Configure,
  EfiMtftp4GetInfo,
  EfiMtftp4ParseOptions,
  EfiMtftp4ReadFile,
  EfiMtftp4WriteFile,
  EfiMtftp4ReadDirectory,
  EfiMtftp4Poll
};
