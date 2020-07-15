/** @file
  This EFI_MTFTP6_PROTOCOL interface implementation.

  It supports the following RFCs:
   RFC1350 - THE TFTP PROTOCOL (REVISION 2)
   RFC2090 - TFTP Multicast Option
   RFC2347 - TFTP Option Extension
   RFC2348 - TFTP Blocksize Option
   RFC2349 - TFTP Timeout Interval and Transfer Size Options

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Mtftp6Impl.h"

EFI_MTFTP6_PROTOCOL gMtftp6ProtocolTemplate = {
  EfiMtftp6GetModeData,
  EfiMtftp6Configure,
  EfiMtftp6GetInfo,
  EfiMtftp6ParseOptions,
  EfiMtftp6ReadFile,
  EfiMtftp6WriteFile,
  EfiMtftp6ReadDirectory,
  EfiMtftp6Poll
  };

/**
  Returns the current operating mode data for the MTFTP6 instance.

  The GetModeData() function returns the current operating mode and
  cached data packet for the MTFTP6 instance.

  @param[in]  This               Pointer to the EFI_MTFTP6_PROTOCOL instance.
  @param[out] ModeData           The buffer in which the EFI MTFTPv6 Protocol driver mode
                                 data is returned.

  @retval  EFI_SUCCESS           The configuration data was returned successfully.
  @retval  EFI_OUT_OF_RESOURCES  The required mode data could not be allocated.
  @retval  EFI_INVALID_PARAMETER This is NULL or ModeData is NULL.

**/
EFI_STATUS
EFIAPI
EfiMtftp6GetModeData (
  IN  EFI_MTFTP6_PROTOCOL    *This,
  OUT EFI_MTFTP6_MODE_DATA   *ModeData
  )
{
  MTFTP6_INSTANCE  *Instance;
  EFI_TPL          OldTpl;

  if (This == NULL || ModeData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl   = gBS->RaiseTPL (TPL_CALLBACK);
  Instance = MTFTP6_INSTANCE_FROM_THIS (This);

  //
  // Copy back the configure data if the instance already configured.
  //
  if (Instance->Config != NULL) {
    CopyMem (
      &ModeData->ConfigData,
      Instance->Config,
      sizeof (EFI_MTFTP6_CONFIG_DATA)
      );
  } else {
    ZeroMem (
      &ModeData->ConfigData,
      sizeof (EFI_MTFTP6_CONFIG_DATA)
      );
  }

  //
  // Set the current support options in mode data.
  //
  ModeData->SupportedOptionCount = MTFTP6_SUPPORTED_OPTIONS_NUM;
  ModeData->SupportedOptions     = (UINT8 **) mMtftp6SupportedOptions;

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}


/**
  Initializes, changes, or resets the default operational setting for
  this EFI MTFTPv6 Protocol driver instance.

  The Configure() function is used to set and change the configuration
  data for this EFI MTFTPv6 Protocol driver instance. The configuration
  data can be reset to startup defaults by calling Configure() with
  MtftpConfigData set to NULL. Whenever the instance is reset, any
  pending operation is aborted. By changing the EFI MTFTPv6 Protocol
  driver instance configuration data, the client can connect to
  different MTFTPv6 servers. The configuration parameters in
  MtftpConfigData are used as the default parameters in later MTFTPv6
  operations and can be overridden in later operations.

  @param[in]  This               Pointer to the EFI_MTFTP6_PROTOCOL instance.
  @param[in]  MtftpConfigData    Pointer to the configuration data structure.

  @retval  EFI_SUCCESS           The EFI MTFTPv6 Protocol instance was configured successfully.
  @retval  EFI_INVALID_PARAMETER One or more following conditions are TRUE:
                                 - This is NULL.
                                 - MtftpConfigData.StationIp is neither zero nor one
                                   of the configured IP addresses in the underlying IPv6 driver.
                                 - MtftpConfigData.ServerIp is not a valid IPv6 unicast address.
                                 Note: It does not match the UEFI 2.3 Specification.
  @retval  EFI_ACCESS_DENIED     - The configuration could not be changed at this time because there
                                   is some MTFTP background operation in progress.
                                 - MtftpConfigData.LocalPort is already in use.
                                 Note: It does not match the UEFI 2.3 Specification.
  @retval  EFI_NO_MAPPING        The underlying IPv6 driver was responsible for choosing a source
                                 address for this instance, but no source address was available for use.
  @retval  EFI_OUT_OF_RESOURCES  The EFI MTFTPv6 Protocol driver instance data could not be
                                 allocated.
                                 Note: It is not defined in the UEFI 2.3 Specification.
  @retval  EFI_DEVICE_ERROR      An unexpected system or network error occurred. The EFI
                                 MTFTPv6 Protocol driver instance is not configured.
                                 Note: It is not defined in the UEFI 2.3 Specification.

**/
EFI_STATUS
EFIAPI
EfiMtftp6Configure (
  IN EFI_MTFTP6_PROTOCOL    *This,
  IN EFI_MTFTP6_CONFIG_DATA *MtftpConfigData     OPTIONAL
  )
{
  MTFTP6_SERVICE            *Service;
  MTFTP6_INSTANCE           *Instance;
  EFI_UDP6_PROTOCOL         *Udp6;
  EFI_UDP6_CONFIG_DATA      Udp6Cfg;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (MtftpConfigData != NULL && !NetIp6IsValidUnicast (&MtftpConfigData->ServerIp)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl   = gBS->RaiseTPL (TPL_CALLBACK);
  Instance = MTFTP6_INSTANCE_FROM_THIS (This);
  Service  = Instance->Service;
  Status   = EFI_SUCCESS;

  if (MtftpConfigData == NULL) {
    //
    // Configure the instance as NULL to abort the current session.
    //
    Mtftp6OperationClean (Instance, EFI_ABORTED);
    FreePool (Instance->Config);
    Instance->Config = NULL;
  } else {
    //
    // It's not allowed to configure one instance twice without configure null.
    //
    if (Instance->Config != NULL) {
      Status = EFI_ACCESS_DENIED;
      goto ON_EXIT;
    }
    //
    // Allocate the configure buffer of the instance and store the user's data.
    //
    Instance->Config = AllocateZeroPool (sizeof (EFI_MTFTP6_CONFIG_DATA));

    if (Instance->Config == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    CopyMem (Instance->Config, MtftpConfigData, sizeof (EFI_MTFTP6_CONFIG_DATA));

    //
    // Don't configure the udpio here because each operation might override
    // the configuration, so delay udpio configuration in each operation.
    //
    if (Instance->UdpIo == NULL) {
      Instance->UdpIo = UdpIoCreateIo (
                          Service->Controller,
                          Service->Image,
                          Mtftp6ConfigDummyUdpIo,
                          UDP_IO_UDP6_VERSION,
                          NULL
                          );
      if (Instance->UdpIo != NULL) {
        Status = gBS->OpenProtocol (
                        Instance->UdpIo->UdpHandle,
                        &gEfiUdp6ProtocolGuid,
                        (VOID **) &Udp6,
                        Service->Image,
                        Instance->Handle,
                        EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                        );
        if (EFI_ERROR (Status)) {
          goto ON_EXIT;
        }
      }
    }

    if (Instance->UdpIo == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    //
    // Continue to configure the downside Udp6 instance by user's data.
    //
    ZeroMem (&Udp6Cfg, sizeof (EFI_UDP6_CONFIG_DATA));

    Udp6Cfg.AcceptPromiscuous  = FALSE;
    Udp6Cfg.AcceptAnyPort      = FALSE;
    Udp6Cfg.AllowDuplicatePort = FALSE;
    Udp6Cfg.TrafficClass       = 0;
    Udp6Cfg.HopLimit           = 128;
    Udp6Cfg.ReceiveTimeout     = 0;
    Udp6Cfg.TransmitTimeout    = 0;
    Udp6Cfg.StationPort        = Instance->Config->LocalPort;
    Udp6Cfg.RemotePort         = Instance->Config->InitialServerPort;

    CopyMem (
      &Udp6Cfg.StationAddress,
      &Instance->Config->StationIp,
      sizeof(EFI_IPv6_ADDRESS)
      );

    CopyMem (
      &Udp6Cfg.RemoteAddress,
      &Instance->Config->ServerIp,
      sizeof (EFI_IPv6_ADDRESS)
      );

    Udp6   = Instance->UdpIo->Protocol.Udp6;
    Status = Udp6->Configure (Udp6, &Udp6Cfg);

    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

ON_EXIT:
  if (EFI_ERROR (Status)) {
    if (Instance->Config != NULL) {
      FreePool (Instance->Config);
      Instance->Config = NULL;
    }
    if (Instance->UdpIo != NULL) {
      UdpIoFreeIo (Instance->UdpIo);
      Instance->UdpIo = NULL;
    }
  }
  gBS->RestoreTPL (OldTpl);
  return Status;
}


/**
  Get the information of the download from the server.

  The GetInfo() function assembles an MTFTPv6 request packet
  with options, sends it to the MTFTPv6 server, and may return
  an MTFTPv6 OACK, MTFTPv6 ERROR, or ICMP ERROR packet. Retries
  occur only if no response packets are received from the MTFTPv6
  server before the timeout expires.

  @param[in]  This               Pointer to the EFI_MTFTP6_PROTOCOL instance.
  @param[in]  OverrideData       Data that is used to override the existing parameters. If NULL, the
                                 default parameters that were set in the EFI_MTFTP6_PROTOCOL.Configure()
                                 function are used.
  @param[in]  Filename           Pointer to null-terminated ASCII file name string.
  @param[in]  ModeStr            Pointer to null-terminated ASCII mode string. If NULL, octet will be used.
  @param[in]  OptionCount        Number of option/value string pairs in OptionList.
  @param[in]  OptionList         Pointer to array of option/value string pairs. Ignored if
                                 OptionCount is zero.
  @param[out] PacketLength       The number of bytes in the returned packet.
  @param[out] Packet             The pointer to the received packet. This buffer must be freed by
                                 the caller.

  @retval  EFI_SUCCESS              An MTFTPv6 OACK packet was received and is in the Packet.
                                    Note: It does not match the UEFI 2.3 Specification.
  @retval  EFI_INVALID_PARAMETER    One or more of the following conditions is TRUE:
                                    - This is NULL.
                                    - Filename is NULL.
                                    - OptionCount is not zero and OptionList is NULL.
                                    - One or more options in OptionList have wrong format.
                                    - PacketLength is NULL.
                                    - OverrideData.ServerIp is not valid unicast IPv6 addresses.
  @retval  EFI_UNSUPPORTED          One or more options in the OptionList are unsupported by
                                    this implementation.
  @retval  EFI_NOT_STARTED          The EFI MTFTPv6 Protocol driver has not been started.
  @retval  EFI_NO_MAPPING           The underlying IPv6 driver was responsible for choosing a source
                                    address for this instance, but no source address was available for use.
  @retval  EFI_ACCESS_DENIED        The previous operation has not completed yet.
  @retval  EFI_OUT_OF_RESOURCES     Required system resources could not be allocated.
  @retval  EFI_TFTP_ERROR           An MTFTPv6 ERROR packet was received and is in the Packet.
  @retval  EFI_NETWORK_UNREACHABLE  An ICMP network unreachable error packet was received and the Packet is set to NULL.
                                    Note: It is not defined in UEFI 2.3 Specification.
  @retval  EFI_HOST_UNREACHABLE     An ICMP host unreachable error packet was received and the Packet is set to NULL.
                                    Note: It is not defined in the UEFI 2.3 Specification.
  @retval  EFI_PROTOCOL_UNREACHABLE An ICMP protocol unreachable error packet was received and the Packet is set to NULL.
                                    Note: It is not defined in the UEFI 2.3 Specification.
  @retval  EFI_PORT_UNREACHABLE     An ICMP port unreachable error packet was received and the Packet is set to NULL.
  @retval  EFI_ICMP_ERROR           Some other ICMP ERROR packet was received and the Packet is set to NULL.
                                    Note: It does not match the UEFI 2.3 Specification.
  @retval  EFI_PROTOCOL_ERROR       An unexpected MTFTPv6 packet was received and is in the Packet.
  @retval  EFI_TIMEOUT              No responses were received from the MTFTPv6 server.
  @retval  EFI_DEVICE_ERROR         An unexpected network error or system error occurred.
  @retval  EFI_NO_MEDIA             There was a media error.

**/
EFI_STATUS
EFIAPI
EfiMtftp6GetInfo (
  IN  EFI_MTFTP6_PROTOCOL      *This,
  IN  EFI_MTFTP6_OVERRIDE_DATA *OverrideData         OPTIONAL,
  IN  UINT8                    *Filename,
  IN  UINT8                    *ModeStr              OPTIONAL,
  IN  UINT8                    OptionCount,
  IN  EFI_MTFTP6_OPTION        *OptionList           OPTIONAL,
  OUT UINT32                   *PacketLength,
  OUT EFI_MTFTP6_PACKET        **Packet              OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_MTFTP6_TOKEN          Token;
  MTFTP6_GETINFO_CONTEXT    Context;

  if (This == NULL ||
      Filename == NULL ||
      PacketLength == NULL ||
      (OptionCount != 0 && OptionList == NULL) ||
      (OverrideData != NULL && !NetIp6IsValidUnicast (&OverrideData->ServerIp))
      ) {
    return EFI_INVALID_PARAMETER;
  }

  if (Packet != NULL) {
    *Packet = NULL;
  }

  *PacketLength         = 0;

  Context.Packet        = Packet;
  Context.PacketLen     = PacketLength;
  Context.Status        = EFI_SUCCESS;

  //
  // Fill fields of the Token for GetInfo operation.
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
  Token.Context         = &Context;
  Token.CheckPacket     = Mtftp6CheckPacket;
  Token.TimeoutCallback = NULL;
  Token.PacketNeeded    = NULL;

  //
  // Start the GetInfo operation by issue the Token.
  //
  Status = Mtftp6OperationStart (This, &Token, EFI_MTFTP6_OPCODE_RRQ);

  if (Status == EFI_ABORTED) {
    //
    // Return the status if failed to issue.
    //
    return Context.Status;
  }

  return Status;
}


/**
  Parse the options in an MTFTPv6 OACK packet.

  The ParseOptions() function parses the option fields in an MTFTPv6 OACK
  packet and returns the number of options that were found, and optionally,
  a list of pointers to the options in the packet. If one or more of the
  option fields are not valid, then EFI_PROTOCOL_ERROR is returned and
  *OptionCount and *OptionList stop at the last valid option.

  @param[in]  This               Pointer to the EFI_MTFTP6_PROTOCOL instance.
  @param[in]  PacketLen          Length of the OACK packet to be parsed.
  @param[in]  Packet             Pointer to the OACK packet to be parsed.
  @param[out] OptionCount        Pointer to the number of options in the following OptionList.
  @param[out] OptionList         Pointer to EFI_MTFTP6_OPTION storage. Each pointer in the
                                 OptionList points to the corresponding MTFTP option buffer
                                 in the Packet. Call the EFI Boot Service FreePool() to
                                 release the OptionList if the options in this OptionList
                                 are not needed anymore.

  @retval  EFI_SUCCESS           The OACK packet was valid and the OptionCount and
                                 OptionList parameters have been updated.
  @retval  EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                 - PacketLen is 0.
                                 - Packet is NULL or Packet is not a valid MTFTPv6 packet.
                                 - OptionCount is NULL.
  @retval  EFI_NOT_FOUND         No options were found in the OACK packet.
  @retval  EFI_OUT_OF_RESOURCES  Storage for the OptionList array can not be allocated.
  @retval  EFI_PROTOCOL_ERROR    One or more of the option fields is invalid.

**/
EFI_STATUS
EFIAPI
EfiMtftp6ParseOptions (
  IN     EFI_MTFTP6_PROTOCOL    *This,
  IN     UINT32                 PacketLen,
  IN     EFI_MTFTP6_PACKET      *Packet,
  OUT    UINT32                 *OptionCount,
  OUT    EFI_MTFTP6_OPTION      **OptionList          OPTIONAL
  )
{
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return Mtftp6ParseStart (Packet, PacketLen, OptionCount, OptionList);
}


/**
  Download a file from an MTFTPv6 server.

  The ReadFile() function is used to initialize and start an MTFTPv6 download
  process, and optionally, wait for completion. When the download operation
  completes, whether successfully or not, the Token.Status field is updated
  by the EFI MTFTPv6 Protocol driver, and then Token.Event is signaled if it
  is not NULL.
  Data can be downloaded from the MTFTPv6 server into either of the following
  locations:
  - A fixed buffer that is pointed to by Token.Buffer
  - A download service function that is pointed to by Token.CheckPacket.
  If both Token.Buffer and Token.CheckPacket are used, then Token.CheckPacket
  will be called first. If the call is successful, the packet will be stored
  in Token.Buffer.

  @param[in]  This               Pointer to the EFI_MTFTP6_PROTOCOL instance.
  @param[in]  Token              Pointer to the token structure to provide the parameters that are
                                 used in this operation.

  @retval  EFI_SUCCESS              The data file has been transferred successfully.
  @retval  EFI_OUT_OF_RESOURCES     Required system resources could not be allocated.
  @retval  EFI_BUFFER_TOO_SMALL     BufferSize is not zero but not large enough to hold the
                                    downloaded data in downloading process.
                                    Note: It does not match the UEFI 2.3 Specification.
  @retval  EFI_ABORTED              Current operation is aborted by user.
  @retval  EFI_NETWORK_UNREACHABLE  An ICMP network unreachable error packet was received.
                                    Note: It is not defined in the UEFI 2.3 Specification.
  @retval  EFI_HOST_UNREACHABLE     An ICMP host unreachable error packet was received.
                                    Note: It is not defined in the UEFI 2.3 Specification.
  @retval  EFI_PROTOCOL_UNREACHABLE An ICMP protocol unreachable error packet was received.
                                    Note: It is not defined in the UEFI 2.3 Specification.
  @retval  EFI_PORT_UNREACHABLE     An ICMP port unreachable error packet was received.
                                    Note: It is not defined in the UEFI 2.3 Specification.
  @retval  EFI_ICMP_ERROR           An ICMP ERROR packet was received.
  @retval  EFI_TIMEOUT              No responses were received from the MTFTPv6 server.
  @retval  EFI_TFTP_ERROR           An MTFTPv6 ERROR packet was received.
  @retval  EFI_DEVICE_ERROR         An unexpected network error or system error occurred.
  @retval  EFI_NO_MEDIA             There was a media error.

**/
EFI_STATUS
EFIAPI
EfiMtftp6ReadFile (
  IN EFI_MTFTP6_PROTOCOL    *This,
  IN EFI_MTFTP6_TOKEN       *Token
  )
{
  return Mtftp6OperationStart (This, Token, EFI_MTFTP6_OPCODE_RRQ);
}


/**
  Send a file to an MTFTPv6 server.

  The WriteFile() function is used to initialize an uploading operation
  with the given option list and optionally wait for completion. If one
  or more of the options is not supported by the server, the unsupported
  options are ignored and a standard TFTP process starts instead. When
  the upload process completes, whether successfully or not, Token.Event
  is signaled, and the EFI MTFTPv6 Protocol driver updates Token.Status.
  The caller can supply the data to be uploaded in the following two modes:
  - Through the user-provided buffer
  - Through a callback function
  With the user-provided buffer, the Token.BufferSize field indicates
  the length of the buffer, and the driver will upload the data in the
  buffer. With an EFI_MTFTP6_PACKET_NEEDED callback function, the driver
  will call this callback function to get more data from the user to upload.

  @param[in]  This               Pointer to the EFI_MTFTP6_PROTOCOL instance.
  @param[in]  Token              Pointer to the token structure to provide the parameters that are
                                 used in this operation.

  @retval  EFI_SUCCESS           The upload session has started.
  @retval  EFI_UNSUPPORTED       The operation is not supported by this implementation.
  @retval  EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                 - This is NULL.
                                 - Token is NULL.
                                 - Token.Filename is NULL.
                                 - Token.OptionCount is not zero and Token.OptionList is NULL.
                                 - One or more options in Token.OptionList have wrong format.
                                 - Token.Buffer and Token.PacketNeeded are both NULL.
                                 - Token.OverrideData.ServerIp is not a valid unicast IPv6 address.
  @retval  EFI_UNSUPPORTED       One or more options in the Token.OptionList are not
                                 supported by this implementation.
  @retval  EFI_NOT_STARTED       The EFI MTFTPv6 Protocol driver has not been started.
  @retval  EFI_NO_MAPPING        The underlying IPv6 driver was responsible for choosing a source
                                 address for this instance, but no source address was available for use.
  @retval  EFI_ALREADY_STARTED   This Token is already being used in another MTFTPv6 session.
  @retval  EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval  EFI_ACCESS_DENIED     The previous operation has not completed yet.
  @retval  EFI_DEVICE_ERROR      An unexpected network error or system error occurred.

**/
EFI_STATUS
EFIAPI
EfiMtftp6WriteFile (
  IN EFI_MTFTP6_PROTOCOL    *This,
  IN EFI_MTFTP6_TOKEN       *Token
  )
{
  return Mtftp6OperationStart (This, Token, EFI_MTFTP6_OPCODE_WRQ);
}


/**
  Download a data file directory from an MTFTPv6 server.

  The ReadDirectory() function is used to return a list of files on the
  MTFTPv6 server that are logically (or operationally) related to
  Token.Filename. The directory request packet that is sent to the server
  is built with the option list that was provided by the caller, if present.
  The file information that the server returns is put into either of
  the following locations:
  - A fixed buffer that is pointed to by Token.Buffer.
  - A download service function that is pointed to by Token.CheckPacket.
  If both Token.Buffer and Token.CheckPacket are used, then Token.CheckPacket
  will be called first. If the call is successful, the packet will be stored
  in Token.Buffer.

  @param[in]  This               Pointer to the EFI_MTFTP6_PROTOCOL instance.
  @param[in]  Token              Pointer to the token structure to provide the parameters that are
                                 used in this operation.

  @retval  EFI_SUCCESS           The MTFTPv6 related file "directory" has been downloaded.
  @retval  EFI_UNSUPPORTED       The EFI MTFTPv6 Protocol driver does not support this function.
  @retval  EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                 - This is NULL.
                                 - Token is NULL.
                                 - Token.Filename is NULL.
                                 - Token.OptionCount is not zero and Token.OptionList is NULL.
                                 - One or more options in Token.OptionList have wrong format.
                                 - Token.Buffer and Token.CheckPacket are both NULL.
                                 - Token.OverrideData.ServerIp is not valid unicast IPv6 addresses.
  @retval  EFI_UNSUPPORTED       One or more options in the Token.OptionList are not
                                 supported by this implementation.
  @retval  EFI_NOT_STARTED       The EFI MTFTPv6 Protocol driver has not been started.
  @retval  EFI_NO_MAPPING        The underlying IPv6 driver was responsible for choosing a source
                                 address for this instance, but no source address was available for use.
  @retval  EFI_ALREADY_STARTED   This Token is already being used in another MTFTPv6 session.
  @retval  EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval  EFI_ACCESS_DENIED     The previous operation has not completed yet.
  @retval  EFI_DEVICE_ERROR      An unexpected network error or system error occurred.

**/
EFI_STATUS
EFIAPI
EfiMtftp6ReadDirectory (
  IN EFI_MTFTP6_PROTOCOL        *This,
  IN EFI_MTFTP6_TOKEN           *Token
  )
{
  return Mtftp6OperationStart (This, Token, EFI_MTFTP6_OPCODE_DIR);
}


/**
  Polls for incoming data packets and processes outgoing data packets.

  The Poll() function can be used by network drivers and applications
  to increase the rate that data packets are moved between the
  communications device and the transmit and receive queues. In some
  systems, the periodic timer event in the managed network driver may
  not poll the underlying communications device fast enough to transmit
  and/or receive all data packets without missing incoming packets or
  dropping outgoing packets. Drivers and applications that are
  experiencing packet loss should try calling the Poll() function
  more often.

  @param[in]  This                   The MTFTP6 protocol instance.


  @retval  EFI_SUCCESS           Incoming or outgoing data was processed.
  @retval  EFI_NOT_STARTED       This EFI MTFTPv6 Protocol instance has not been started.
  @retval  EFI_INVALID_PARAMETER This is NULL.
  @retval  EFI_DEVICE_ERROR      An unexpected system or network error occurred.
  @retval  EFI_TIMEOUT           Data was dropped out of the transmit and/or receive queue.
                                 Consider increasing the polling rate.

**/
EFI_STATUS
EFIAPI
EfiMtftp6Poll (
  IN EFI_MTFTP6_PROTOCOL    *This
  )
{
  MTFTP6_INSTANCE           *Instance;
  EFI_UDP6_PROTOCOL         *Udp6;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = MTFTP6_INSTANCE_FROM_THIS (This);

  //
  // Check the instance whether configured or in destroy.
  //
  if (Instance->Config == NULL) {
    return EFI_NOT_STARTED;
  }

  Udp6 = Instance->UdpIo->Protocol.Udp6;

  return Udp6->Poll (Udp6);
}
