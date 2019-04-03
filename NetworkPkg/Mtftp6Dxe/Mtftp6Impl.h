/** @file
  Mtftp6 internal data structure and definition declaration.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_MTFTP6_IMPL_H__
#define __EFI_MTFTP6_IMPL_H__

#include <Uefi.h>

#include <Protocol/Udp6.h>
#include <Protocol/Mtftp6.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/DriverBinding.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/NetLib.h>
#include <Library/PrintLib.h>

typedef struct _MTFTP6_SERVICE  MTFTP6_SERVICE;
typedef struct _MTFTP6_INSTANCE MTFTP6_INSTANCE;

#include "Mtftp6Driver.h"
#include "Mtftp6Option.h"
#include "Mtftp6Support.h"

#define MTFTP6_SERVICE_SIGNATURE       SIGNATURE_32 ('M', 'F', '6', 'S')
#define MTFTP6_INSTANCE_SIGNATURE      SIGNATURE_32 ('M', 'F', '6', 'I')

#define MTFTP6_DEFAULT_SERVER_CMD_PORT 69
#define MTFTP6_DEFAULT_TIMEOUT         3
#define MTFTP6_GET_MAPPING_TIMEOUT     3
#define MTFTP6_DEFAULT_MAX_RETRY       5
#define MTFTP6_DEFAULT_BLK_SIZE        512
#define MTFTP6_DEFAULT_WINDOWSIZE      1
#define MTFTP6_TICK_PER_SECOND         10000000U

#define MTFTP6_SERVICE_FROM_THIS(a)    CR (a, MTFTP6_SERVICE, ServiceBinding, MTFTP6_SERVICE_SIGNATURE)
#define MTFTP6_INSTANCE_FROM_THIS(a)   CR (a, MTFTP6_INSTANCE, Mtftp6, MTFTP6_INSTANCE_SIGNATURE)

extern EFI_MTFTP6_PROTOCOL             gMtftp6ProtocolTemplate;

typedef struct _MTFTP6_GETINFO_CONTEXT{
  EFI_MTFTP6_PACKET             **Packet;
  UINT32                        *PacketLen;
  EFI_STATUS                    Status;
} MTFTP6_GETINFO_CONTEXT;

//
// Control block for MTFTP6 instance, it's per configuration data.
//
struct _MTFTP6_INSTANCE {
  UINT32                        Signature;
  EFI_HANDLE                    Handle;
  LIST_ENTRY                    Link;
  EFI_MTFTP6_PROTOCOL           Mtftp6;
  MTFTP6_SERVICE                *Service;
  EFI_MTFTP6_CONFIG_DATA        *Config;

  EFI_MTFTP6_TOKEN              *Token;
  MTFTP6_EXT_OPTION_INFO        ExtInfo;

  UINT16                        BlkSize;
  UINT16                        LastBlk;
  LIST_ENTRY                    BlkList;

  UINT16                        Operation;

  UINT16                        WindowSize;

  //
  // Record the total received and saved block number.
  //
  UINT64                        TotalBlock;

  //
  // Record the acked block number.
  //
  UINT64                        AckedBlock;

  EFI_IPv6_ADDRESS              ServerIp;
  UINT16                        ServerCmdPort;
  UINT16                        ServerDataPort;
  UDP_IO                        *UdpIo;

  EFI_IPv6_ADDRESS              McastIp;
  UINT16                        McastPort;
  UDP_IO                        *McastUdpIo;

  NET_BUF                       *LastPacket;
  UINT32                        CurRetry;
  UINT32                        MaxRetry;
  UINT32                        PacketToLive;
  UINT32                        Timeout;

  EFI_TPL                       OldTpl;
  BOOLEAN                       IsTransmitted;
  BOOLEAN                       IsMaster;
  BOOLEAN                       InDestroy;
};

//
// Control block for MTFTP6 service, it's per Nic handle.
//
struct _MTFTP6_SERVICE {
  UINT32                        Signature;
  EFI_SERVICE_BINDING_PROTOCOL  ServiceBinding;
  EFI_HANDLE                    Controller;
  EFI_HANDLE                    Image;

  UINT16                        ChildrenNum;
  LIST_ENTRY                    Children;
  //
  // It is used to be as internal calculagraph for all instances.
  //
  EFI_EVENT                     Timer;
  //
  // It is used to maintain the parent-child relationship between
  // mtftp driver and udp driver.
  //
  UDP_IO                        *DummyUdpIo;
};

typedef struct {
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  UINTN                         NumberOfChildren;
  EFI_HANDLE                    *ChildHandleBuffer;
} MTFTP6_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT;

/**
  Returns the current operating mode data for the MTFTP6 instance.

  The GetModeData() function returns the current operating mode and
  cached data packet for the MTFTP6 instance.

  @param[in]  This               Pointer to the EFI_MTFTP6_PROTOCOL instance.
  @param[out] ModeData           The buffer in which the EFI MTFTPv6 Protocol driver mode
                                 data is returned.

  @retval  EFI_SUCCESS           The configuration data was returned successfully.
  @retval  EFI_OUT_OF_RESOURCES  The required mode data could not be allocated.
  @retval  EFI_INVALID_PARAMETER This is NULL, or ModeData is NULL.

**/
EFI_STATUS
EFIAPI
EfiMtftp6GetModeData (
  IN  EFI_MTFTP6_PROTOCOL    *This,
  OUT EFI_MTFTP6_MODE_DATA   *ModeData
  );

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
                                 - MtftpCofigData.ServerIp is not a valid IPv6 unicast address.
                                 Note: It does not match the UEFI 2.3 Specification.
  @retval  EFI_ACCESS_DENIED     - The configuration could not be changed at this time because there
                                   is some MTFTP background operation in progress.
                                 - MtftpCofigData.LocalPort is already in use.
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
  );

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
  @param[in]  ModeStr            Pointer to null-terminated ASCII mode string. If NULL, octet will be used
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
                                    - OverrideData.ServerIp is not a valid unicast IPv6 address.
  @retval  EFI_UNSUPPORTED          One or more options in the OptionList are unsupported by
                                    this implementation.
  @retval  EFI_NOT_STARTED          The EFI MTFTPv6 Protocol driver has not been started.
  @retval  EFI_NO_MAPPING           The underlying IPv6 driver was responsible for choosing a source
                                    address for this instance, but no source address was available for use.
  @retval  EFI_ACCESS_DENIED        The previous operation has not completed yet.
  @retval  EFI_OUT_OF_RESOURCES     Required system resources could not be allocated.
  @retval  EFI_TFTP_ERROR           An MTFTPv6 ERROR packet was received and is in the Packet.
  @retval  EFI_NETWORK_UNREACHABLE  An ICMP network unreachable error packet was received, and the Packet is set to NULL.
                                    Note: It is not defined in the UEFI 2.3 Specification.
  @retval  EFI_HOST_UNREACHABLE     An ICMP host unreachable error packet was received, and the Packet is set to NULL.
                                    Note: It is not defined in the UEFI 2.3 Specification.
  @retval  EFI_PROTOCOL_UNREACHABLE An ICMP protocol unreachable error packet was received, and the Packet is set to NULL.
                                    Note: It is not defined in the UEFI 2.3 Specification.
  @retval  EFI_PORT_UNREACHABLE     An ICMP port unreachable error packet was received, and the Packet is set to NULL.
  @retval  EFI_ICMP_ERROR           Some other ICMP ERROR packet was received, and the Packet is set to NULL.
                                    Note: It does not match the UEFI 2.3 Specification.
  @retval  EFI_PROTOCOL_ERROR       An unexpected MTFTPv6 packet was received and is in the Packet.
  @retval  EFI_TIMEOUT              No responses were received from the MTFTPv6 server.
  @retval  EFI_DEVICE_ERROR         An unexpected network error or system error occurred.

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
  );

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
                                 are not needed any more.

  @retval  EFI_SUCCESS           The OACK packet was valid and the OptionCount, and
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
  );

/**
  Download a file from an MTFTPv6 server.

  The ReadFile() function is used to initialize and start an MTFTPv6 download
  process and optionally wait for completion. When the download operation
  completes, whether successfully or not, the Token.Status field is updated
  by the EFI MTFTPv6 Protocol driver, and then Token.Event is signaled if it
  is not NULL.
  Data can be downloaded from the MTFTPv6 server into either of the following
  locations:
  - A fixed buffer that is pointed to by Token.Buffer.
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

**/
EFI_STATUS
EFIAPI
EfiMtftp6ReadFile (
  IN EFI_MTFTP6_PROTOCOL    *This,
  IN EFI_MTFTP6_TOKEN       *Token
  );

/**
  Send a file to an MTFTPv6 server.

  The WriteFile() function is used to initialize an uploading operation
  with the given option list, and optionally, wait for completion. If one
  or more of the options is not supported by the server, the unsupported
  options are ignored and a standard TFTP process starts instead. When
  the upload process completes, whether successfully or not, Token.Event
  is signaled, and the EFI MTFTPv6 Protocol driver updates Token.Status.
  The caller can supply the data to be uploaded in the following two modes:
  - Through the user-provided buffer.
  - Through a callback function.
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
EfiMtftp6WriteFile (
  IN EFI_MTFTP6_PROTOCOL    *This,
  IN EFI_MTFTP6_TOKEN       *Token
  );

/**
  Download a data file directory from an MTFTPv6 server.

  The ReadDirectory() function is used to return a list of files on the
  MTFTPv6 server that are logically (or operationally) related to
  Token.Filename. The directory request packet that is sent to the server
  is built with the option list that was provided by caller, if present.
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
  );

/**
  Polls for incoming data packets and processes outgoing data packets.

  The Poll() function can be used by network drivers and applications
  to increase the rate that data packets are moved between the
  communications device and the transmit and receive queues.In some
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
  );

#endif
