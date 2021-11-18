/** @file
  The EMU_SNP_PROTOCOL provides services to initialize a network interface,
  transmit packets, receive packets, and close a network interface.


Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
Portitions copyright (c) 2011, Apple Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EMU_SNP_H__
#define __EMU_SNP_H__

#include <Protocol/SimpleNetwork.h>

#define EMU_SNP_PROTOCOL_GUID \
 { 0xFD5FBE54, 0x8C35, 0xB345, { 0x8A, 0x0F, 0x7A, 0xC8, 0xA5, 0xFD, 0x05, 0x21 } }

typedef struct _EMU_SNP_PROTOCOL EMU_SNP_PROTOCOL;

/**
  Register storage for SNP Mode.

  @param  This Protocol instance pointer.
  @param  Mode SimpleNetworkProtocol Mode structure passed into driver.

  @retval EFI_SUCCESS           The network interface was started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.

**/
typedef
EFI_STATUS
(EFIAPI *EMU_SNP_CREATE_MAPPING)(
  IN EMU_SNP_PROTOCOL         *This,
  IN EFI_SIMPLE_NETWORK_MODE  *Mode
  );

/**
  Changes the state of a network interface from "stopped" to "started".

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           The network interface was started.
  @retval EFI_ALREADY_STARTED   The network interface is already in the started state.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
typedef
EFI_STATUS
(EFIAPI *EMU_SNP_START)(
  IN EMU_SNP_PROTOCOL  *This
  );

/**
  Changes the state of a network interface from "started" to "stopped".

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           The network interface was stopped.
  @retval EFI_ALREADY_STARTED   The network interface is already in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
typedef
EFI_STATUS
(EFIAPI *EMU_SNP_STOP)(
  IN EMU_SNP_PROTOCOL  *This
  );

/**
  Resets a network adapter and allocates the transmit and receive buffers
  required by the network interface; optionally, also requests allocation
  of additional transmit and receive buffers.

  @param  This              The protocol instance pointer.
  @param  ExtraRxBufferSize The size, in bytes, of the extra receive buffer space
                            that the driver should allocate for the network interface.
                            Some network interfaces will not be able to use the extra
                            buffer, and the caller will not know if it is actually
                            being used.
  @param  ExtraTxBufferSize The size, in bytes, of the extra transmit buffer space
                            that the driver should allocate for the network interface.
                            Some network interfaces will not be able to use the extra
                            buffer, and the caller will not know if it is actually
                            being used.

  @retval EFI_SUCCESS           The network interface was initialized.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_OUT_OF_RESOURCES  There was not enough memory for the transmit and
                                receive buffers.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
typedef
EFI_STATUS
(EFIAPI *EMU_SNP_INITIALIZE)(
  IN EMU_SNP_PROTOCOL                    *This,
  IN UINTN                               ExtraRxBufferSize  OPTIONAL,
  IN UINTN                               ExtraTxBufferSize  OPTIONAL
  );

/**
  Resets a network adapter and re-initializes it with the parameters that were
  provided in the previous call to Initialize().

  @param  This                 The protocol instance pointer.
  @param  ExtendedVerification Indicates that the driver may perform a more
                               exhaustive verification operation of the device
                               during reset.

  @retval EFI_SUCCESS           The network interface was reset.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
typedef
EFI_STATUS
(EFIAPI *EMU_SNP_RESET)(
  IN EMU_SNP_PROTOCOL   *This,
  IN BOOLEAN            ExtendedVerification
  );

/**
  Resets a network adapter and leaves it in a state that is safe for
  another driver to initialize.

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           The network interface was shutdown.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
typedef
EFI_STATUS
(EFIAPI *EMU_SNP_SHUTDOWN)(
  IN EMU_SNP_PROTOCOL  *This
  );

/**
  Manages the multicast receive filters of a network interface.

  @param  This             The protocol instance pointer.
  @param  Enable           A bit mask of receive filters to enable on the network interface.
  @param  Disable          A bit mask of receive filters to disable on the network interface.
  @param  ResetMCastFilter Set to TRUE to reset the contents of the multicast receive
                           filters on the network interface to their default values.
  @param  McastFilterCnt   Number of multicast HW MAC addresses in the new
                           MCastFilter list. This value must be less than or equal to
                           the MCastFilterCnt field of EMU_SNP_MODE. This
                           field is optional if ResetMCastFilter is TRUE.
  @param  MCastFilter      A pointer to a list of new multicast receive filter HW MAC
                           addresses. This list will replace any existing multicast
                           HW MAC address list. This field is optional if
                           ResetMCastFilter is TRUE.

  @retval EFI_SUCCESS           The multicast receive filter list was updated.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
typedef
EFI_STATUS
(EFIAPI *EMU_SNP_RECEIVE_FILTERS)(
  IN EMU_SNP_PROTOCOL                             *This,
  IN UINT32                                       Enable,
  IN UINT32                                       Disable,
  IN BOOLEAN                                      ResetMCastFilter,
  IN UINTN                                        MCastFilterCnt     OPTIONAL,
  IN EFI_MAC_ADDRESS                              *MCastFilter OPTIONAL
  );

/**
  Modifies or resets the current station address, if supported.

  @param  This  The protocol instance pointer.
  @param  Reset Flag used to reset the station address to the network interfaces
                permanent address.
  @param  New   The new station address to be used for the network interface.

  @retval EFI_SUCCESS           The network interfaces station address was updated.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
typedef
EFI_STATUS
(EFIAPI *EMU_SNP_STATION_ADDRESS)(
  IN EMU_SNP_PROTOCOL            *This,
  IN BOOLEAN                     Reset,
  IN EFI_MAC_ADDRESS             *New OPTIONAL
  );

/**
  Resets or collects the statistics on a network interface.

  @param  This            Protocol instance pointer.
  @param  Reset           Set to TRUE to reset the statistics for the network interface.
  @param  StatisticsSize  On input the size, in bytes, of StatisticsTable. On
                          output the size, in bytes, of the resulting table of
                          statistics.
  @param  StatisticsTable A pointer to the EFI_NETWORK_STATISTICS structure that
                          contains the statistics.

  @retval EFI_SUCCESS           The statistics were collected from the network interface.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_BUFFER_TOO_SMALL  The Statistics buffer was too small. The current buffer
                                size needed to hold the statistics is returned in
                                StatisticsSize.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
typedef
EFI_STATUS
(EFIAPI *EMU_SNP_STATISTICS)(
  IN EMU_SNP_PROTOCOL                     *This,
  IN BOOLEAN                              Reset,
  IN OUT UINTN                            *StatisticsSize   OPTIONAL,
  OUT EFI_NETWORK_STATISTICS              *StatisticsTable  OPTIONAL
  );

/**
  Converts a multicast IP address to a multicast HW MAC address.

  @param  This The protocol instance pointer.
  @param  IPv6 Set to TRUE if the multicast IP address is IPv6 [RFC 2460]. Set
               to FALSE if the multicast IP address is IPv4 [RFC 791].
  @param  IP   The multicast IP address that is to be converted to a multicast
               HW MAC address.
  @param  MAC  The multicast HW MAC address that is to be generated from IP.

  @retval EFI_SUCCESS           The multicast IP address was mapped to the multicast
                                HW MAC address.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_BUFFER_TOO_SMALL  The Statistics buffer was too small. The current buffer
                                size needed to hold the statistics is returned in
                                StatisticsSize.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
typedef
EFI_STATUS
(EFIAPI *EMU_SNP_MCAST_IP_TO_MAC)(
  IN EMU_SNP_PROTOCOL                     *This,
  IN BOOLEAN                              IPv6,
  IN EFI_IP_ADDRESS                       *IP,
  OUT EFI_MAC_ADDRESS                     *MAC
  );

/**
  Performs read and write operations on the NVRAM device attached to a
  network interface.

  @param  This       The protocol instance pointer.
  @param  ReadWrite  TRUE for read operations, FALSE for write operations.
  @param  Offset     Byte offset in the NVRAM device at which to start the read or
                     write operation. This must be a multiple of NvRamAccessSize and
                     less than NvRamSize.
  @param  BufferSize The number of bytes to read or write from the NVRAM device.
                     This must also be a multiple of NvramAccessSize.
  @param  Buffer     A pointer to the data buffer.

  @retval EFI_SUCCESS           The NVRAM access was performed.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
typedef
EFI_STATUS
(EFIAPI *EMU_SNP_NVDATA)(
  IN EMU_SNP_PROTOCOL                     *This,
  IN BOOLEAN                              ReadWrite,
  IN UINTN                                Offset,
  IN UINTN                                BufferSize,
  IN OUT VOID                             *Buffer
  );

/**
  Reads the current interrupt status and recycled transmit buffer status from
  a network interface.

  @param  This            The protocol instance pointer.
  @param  InterruptStatus A pointer to the bit mask of the currently active interrupts
                          If this is NULL, the interrupt status will not be read from
                          the device. If this is not NULL, the interrupt status will
                          be read from the device. When the  interrupt status is read,
                          it will also be cleared. Clearing the transmit  interrupt
                          does not empty the recycled transmit buffer array.
  @param  TxBuf           Recycled transmit buffer address. The network interface will
                          not transmit if its internal recycled transmit buffer array
                          is full. Reading the transmit buffer does not clear the
                          transmit interrupt. If this is NULL, then the transmit buffer
                          status will not be read. If there are no transmit buffers to
                          recycle and TxBuf is not NULL, * TxBuf will be set to NULL.

  @retval EFI_SUCCESS           The status of the network interface was retrieved.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
typedef
EFI_STATUS
(EFIAPI *EMU_SNP_GET_STATUS)(
  IN EMU_SNP_PROTOCOL                     *This,
  OUT UINT32                              *InterruptStatus OPTIONAL,
  OUT VOID                                **TxBuf OPTIONAL
  );

/**
  Places a packet in the transmit queue of a network interface.

  @param  This       The protocol instance pointer.
  @param  HeaderSize The size, in bytes, of the media header to be filled in by
                     the Transmit() function. If HeaderSize is non-zero, then it
                     must be equal to This->Mode->MediaHeaderSize and the DestAddr
                     and Protocol parameters must not be NULL.
  @param  BufferSize The size, in bytes, of the entire packet (media header and
                     data) to be transmitted through the network interface.
  @param  Buffer     A pointer to the packet (media header followed by data) to be
                     transmitted. This parameter cannot be NULL. If HeaderSize is zero,
                     then the media header in Buffer must already be filled in by the
                     caller. If HeaderSize is non-zero, then the media header will be
                     filled in by the Transmit() function.
  @param  SrcAddr    The source HW MAC address. If HeaderSize is zero, then this parameter
                     is ignored. If HeaderSize is non-zero and SrcAddr is NULL, then
                     This->Mode->CurrentAddress is used for the source HW MAC address.
  @param  DestAddr   The destination HW MAC address. If HeaderSize is zero, then this
                     parameter is ignored.
  @param  Protocol   The type of header to build. If HeaderSize is zero, then this
                     parameter is ignored. See RFC 1700, section "Ether Types", for
                     examples.

  @retval EFI_SUCCESS           The packet was placed on the transmit queue.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_NOT_READY         The network interface is too busy to accept this transmit request.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize parameter is too small.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
typedef
EFI_STATUS
(EFIAPI *EMU_SNP_TRANSMIT)(
  IN EMU_SNP_PROTOCOL                     *This,
  IN UINTN                                HeaderSize,
  IN UINTN                                BufferSize,
  IN VOID                                 *Buffer,
  IN EFI_MAC_ADDRESS                      *SrcAddr  OPTIONAL,
  IN EFI_MAC_ADDRESS                      *DestAddr OPTIONAL,
  IN UINT16                               *Protocol OPTIONAL
  );

/**
  Receives a packet from a network interface.

  @param  This       The protocol instance pointer.
  @param  HeaderSize The size, in bytes, of the media header received on the network
                     interface. If this parameter is NULL, then the media header size
                     will not be returned.
  @param  BufferSize On entry, the size, in bytes, of Buffer. On exit, the size, in
                     bytes, of the packet that was received on the network interface.
  @param  Buffer     A pointer to the data buffer to receive both the media header and
                     the data.
  @param  SrcAddr    The source HW MAC address. If this parameter is NULL, the
                     HW MAC source address will not be extracted from the media
                     header.
  @param  DestAddr   The destination HW MAC address. If this parameter is NULL,
                     the HW MAC destination address will not be extracted from the
                     media header.
  @param  Protocol   The media header type. If this parameter is NULL, then the
                     protocol will not be extracted from the media header. See
                     RFC 1700 section "Ether Types" for examples.

  @retval  EFI_SUCCESS           The received data was stored in Buffer, and BufferSize has
                                 been updated to the number of bytes received.
  @retval  EFI_NOT_STARTED       The network interface has not been started.
  @retval  EFI_NOT_READY         The network interface is too busy to accept this transmit
                                 request.
  @retval  EFI_BUFFER_TOO_SMALL  The BufferSize parameter is too small.
  @retval  EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval  EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval  EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
typedef
EFI_STATUS
(EFIAPI *EMU_SNP_RECEIVE)(
  IN EMU_SNP_PROTOCOL                     *This,
  OUT UINTN                               *HeaderSize OPTIONAL,
  IN OUT UINTN                            *BufferSize,
  OUT VOID                                *Buffer,
  OUT EFI_MAC_ADDRESS                     *SrcAddr    OPTIONAL,
  OUT EFI_MAC_ADDRESS                     *DestAddr   OPTIONAL,
  OUT UINT16                              *Protocol   OPTIONAL
  );

#define EMU_SNP_PROTOCOL_REVISION  0x00010000

//
// Revision defined in EFI1.1
//
#define EMU_SNP_INTERFACE_REVISION  EMU_SNP_PROTOCOL_REVISION

///
/// The EMU_SNP_PROTOCOL protocol abstracts OS network sercices
/// from the EFI driver that produces EFI Simple Network Protocol.
///
struct _EMU_SNP_PROTOCOL {
  EMU_SNP_CREATE_MAPPING     CreateMapping;
  EMU_SNP_START              Start;
  EMU_SNP_STOP               Stop;
  EMU_SNP_INITIALIZE         Initialize;
  EMU_SNP_RESET              Reset;
  EMU_SNP_SHUTDOWN           Shutdown;
  EMU_SNP_RECEIVE_FILTERS    ReceiveFilters;
  EMU_SNP_STATION_ADDRESS    StationAddress;
  EMU_SNP_STATISTICS         Statistics;
  EMU_SNP_MCAST_IP_TO_MAC    MCastIpToMac;
  EMU_SNP_NVDATA             NvData;
  EMU_SNP_GET_STATUS         GetStatus;
  EMU_SNP_TRANSMIT           Transmit;
  EMU_SNP_RECEIVE            Receive;
};

extern EFI_GUID  gEmuSnpProtocolGuid;

#endif
