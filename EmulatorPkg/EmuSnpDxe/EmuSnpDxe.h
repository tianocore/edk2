/** @file

 Copyright (c) 2010, Apple, Inc. All rights reserved.<BR>

    SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  EmuSnp.h

Abstract:

-**/

#ifndef _EMU_SNP_H_
#define _EMU_SNP_H_

#include <Uefi.h>

#include <Protocol/SimpleNetwork.h>
#include <Protocol/DevicePath.h>
#include <Protocol/EmuIoThunk.h>
#include <Protocol/EmuSnp.h>


#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NetLib.h>

#define NET_ETHER_HEADER_SIZE     14

//
//  Private data for driver.
//
#define EMU_SNP_PRIVATE_DATA_SIGNATURE SIGNATURE_32( 'U', 'S', 'N', 'P' )

typedef struct {
  UINTN                       Signature;
  EMU_IO_THUNK_PROTOCOL       *IoThunk;
  EMU_SNP_PROTOCOL            *Io;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;

  EFI_HANDLE                  EfiHandle;
  EFI_HANDLE                  DeviceHandle;

  EFI_SIMPLE_NETWORK_PROTOCOL Snp;
  EFI_SIMPLE_NETWORK_MODE     Mode;

  EFI_UNICODE_STRING_TABLE    *ControllerNameTable;

} EMU_SNP_PRIVATE_DATA;

#define EMU_SNP_PRIVATE_DATA_FROM_SNP_THIS(a) \
      CR( a, EMU_SNP_PRIVATE_DATA, Snp, EMU_SNP_PRIVATE_DATA_SIGNATURE )

extern EFI_DRIVER_BINDING_PROTOCOL    gEmuSnpDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL    gEmuSnpDriverComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL   gEmuSnpDriverComponentName2;

/**
  Test to see if this driver supports ControllerHandle. This service
  is called by the EFI boot service ConnectController(). In
  order to make drivers as small as possible, there are a few calling
  restrictions for this service. ConnectController() must
  follow these calling restrictions. If any other agent wishes to call
  Supported() it must also follow these calling restrictions.

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_UNSUPPORTED     This driver does not support this device

**/
EFI_STATUS
EFIAPI
EmuSnpDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * RemainingDevicePath OPTIONAL
  );

/**
  Start this driver on ControllerHandle. This service is called by the
  EFI boot service ConnectController(). In order to make
  drivers as small as possible, there are a few calling restrictions for
  this service. ConnectController() must follow these
  calling restrictions. If any other agent wishes to call Start() it
  must also follow these calling restrictions.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          Always succeeds.

**/
EFI_STATUS
EFIAPI
EmuSnpDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * RemainingDevicePath OPTIONAL
  );

/**
  Stop this driver on ControllerHandle. This service is called by the
  EFI boot service DisconnectController(). In order to
  make drivers as small as possible, there are a few calling
  restrictions for this service. DisconnectController()
  must follow these calling restrictions. If any other agent wishes
  to call Stop() it must also follow these calling restrictions.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       Always succeeds.

**/
EFI_STATUS
EFIAPI
EmuSnpDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

/**
  Changes the state of a network interface from "stopped" to "started".

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
EmuSnpStart(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This
  );

/**
  Changes the state of a network interface from "started" to "stopped".

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
EmuSnpStop(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This
  );

/**
  Resets a network adapter and allocates the transmit and receive buffers
  required by the network interface; optionally, also requests allocation
  of additional transmit and receive buffers.

  @param  This              Protocol instance pointer.
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

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
EmuSnpInitialize(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  IN UINTN              ExtraRxBufferSize OPTIONAL,
  IN UINTN              ExtraTxBufferSize OPTIONAL
  );

/**
  Resets a network adapter and re-initializes it with the parameters that were
  provided in the previous call to Initialize().

  @param  This                 Protocol instance pointer.
  @param  ExtendedVerification Indicates that the driver may perform a more
                               exhaustive verification operation of the device
                               during reset.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
EmuSnpReset(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  IN BOOLEAN              ExtendedVerification
  );

/**
  Resets a network adapter and leaves it in a state that is safe for
  another driver to initialize.

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
EmuSnpShutdown(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This
  );

/**
  Manages the multicast receive filters of a network interface.

  @param  This               Protocol instance pointer.
  @param  EnableBits         A bit mask of receive filters to enable on the network interface.
  @param  DisableBits        A bit mask of receive filters to disable on the network interface.
  @param  ResetMcastFilter   Set to TRUE to reset the contents of the multicast receive
                             filters on the network interface to their default values.
  @param  McastFilterCount   Number of multicast HW MAC addresses in the new
                             MCastFilter list. This value must be less than or equal to
                             the MCastFilterCnt field of EFI_SIMPLE_NETWORK_MODE. This
                             field is optional if ResetMCastFilter is TRUE.
  @param  McastFilter        A pointer to a list of new multicast receive filter HW MAC
                             addresses. This list will replace any existing multicast
                             HW MAC address list. This field is optional if
                             ResetMCastFilter is TRUE.

  @retval EFI_SUCCESS           The multicast receive filter list was updated.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.

**/
EFI_STATUS
EFIAPI
EmuSnpReceiveFilters(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  IN UINT32              EnableBits,
  IN UINT32              DisableBits,
  IN BOOLEAN              ResetMcastFilter,
  IN UINTN              McastFilterCount OPTIONAL,
  IN EFI_MAC_ADDRESS*          McastFilter OPTIONAL
  );

/**
  Modifies or resets the current station address, if supported.

  @param  This         Protocol instance pointer.
  @param  Reset        Flag used to reset the station address to the network interfaces
                       permanent address.
  @param  NewMacAddr   New station address to be used for the network interface.

  @retval EFI_UNSUPPORTED       Not supported yet.

**/
EFI_STATUS
EFIAPI
EmuSnpStationAddress(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  IN BOOLEAN              Reset,
  IN EFI_MAC_ADDRESS*          NewMacAddr OPTIONAL
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
  @retval EFI_UNSUPPORTED       Not supported yet.

**/
EFI_STATUS
EFIAPI
EmuSnpStatistics(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  IN BOOLEAN              Reset,
  IN OUT UINTN*            StatisticsSize OPTIONAL,
  OUT EFI_NETWORK_STATISTICS*      StatisticsTable OPTIONAL
  );

/**
  Converts a multicast IP address to a multicast HW MAC address.

  @param  This  Protocol instance pointer.
  @param  Ipv6  Set to TRUE if the multicast IP address is IPv6 [RFC 2460]. Set
                to FALSE if the multicast IP address is IPv4 [RFC 791].
  @param  Ip    The multicast IP address that is to be converted to a multicast
                HW MAC address.
  @param  Mac   The multicast HW MAC address that is to be generated from IP.

  @retval EFI_SUCCESS           The multicast IP address was mapped to the multicast
                                HW MAC address.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_BUFFER_TOO_SMALL  The Statistics buffer was too small. The current buffer
                                size needed to hold the statistics is returned in
                                StatisticsSize.
  @retval EFI_UNSUPPORTED       Not supported yet.

**/
EFI_STATUS
EFIAPI
EmuSnpMcastIptoMac(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  IN BOOLEAN              Ipv6,
  IN EFI_IP_ADDRESS*          Ip,
  OUT EFI_MAC_ADDRESS*        Mac
  );

/**
  Performs read and write operations on the NVRAM device attached to a
  network interface.

  @param  This         Protocol instance pointer.
  @param  ReadOrWrite  TRUE for read operations, FALSE for write operations.
  @param  Offset       Byte offset in the NVRAM device at which to start the read or
                       write operation. This must be a multiple of NvRamAccessSize and
                       less than NvRamSize.
  @param  BufferSize   The number of bytes to read or write from the NVRAM device.
                       This must also be a multiple of NvramAccessSize.
  @param  Buffer       A pointer to the data buffer.

  @retval EFI_UNSUPPORTED       Not supported yet.

**/
EFI_STATUS
EFIAPI
EmuSnpNvdata(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  IN BOOLEAN              ReadOrWrite,
  IN UINTN              Offset,
  IN UINTN              BufferSize,
  IN OUT VOID*            Buffer
  );

/**
  Reads the current interrupt status and recycled transmit buffer status from
  a network interface.

  @param  This            Protocol instance pointer.
  @param  InterruptStatus A pointer to the bit mask of the currently active interrupts
                          If this is NULL, the interrupt status will not be read from
                          the device. If this is not NULL, the interrupt status will
                          be read from the device. When the  interrupt status is read,
                          it will also be cleared. Clearing the transmit  interrupt
                          does not empty the recycled transmit buffer array.
  @param  TxBuffer        Recycled transmit buffer address. The network interface will
                          not transmit if its internal recycled transmit buffer array
                          is full. Reading the transmit buffer does not clear the
                          transmit interrupt. If this is NULL, then the transmit buffer
                          status will not be read. If there are no transmit buffers to
                          recycle and TxBuf is not NULL, * TxBuf will be set to NULL.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
EmuSnpGetStatus(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  OUT UINT32*              InterruptStatus,
  OUT VOID**              TxBuffer
  );

/**
  Places a packet in the transmit queue of a network interface.

  @param  This       Protocol instance pointer.
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
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_ACCESS_DENIED     Error acquire global lock for operation.

**/
EFI_STATUS
EFIAPI
EmuSnpTransmit(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  IN UINTN              HeaderSize,
  IN UINTN              BufferSize,
  IN VOID*              Buffer,
  IN EFI_MAC_ADDRESS*          SrcAddr OPTIONAL,
  IN EFI_MAC_ADDRESS*          DestAddr OPTIONAL,
  IN UINT16*              Protocol OPTIONAL
  );

/**
  Receives a packet from a network interface.

  @param  This             Protocol instance pointer.
  @param  HeaderSize       The size, in bytes, of the media header received on the network
                           interface. If this parameter is NULL, then the media header size
                           will not be returned.
  @param  BuffSize         On entry, the size, in bytes, of Buffer. On exit, the size, in
                           bytes, of the packet that was received on the network interface.
  @param  Buffer           A pointer to the data buffer to receive both the media header and
                           the data.
  @param  SourceAddr       The source HW MAC address. If this parameter is NULL, the
                           HW MAC source address will not be extracted from the media
                           header.
  @param  DestinationAddr  The destination HW MAC address. If this parameter is NULL,
                           the HW MAC destination address will not be extracted from the
                           media header.
  @param  Protocol         The media header type. If this parameter is NULL, then the
                           protocol will not be extracted from the media header. See
                           RFC 1700 section "Ether Types" for examples.

  @retval  EFI_SUCCESS           The received data was stored in Buffer, and BufferSize has
                                 been updated to the number of bytes received.
  @retval  EFI_NOT_READY         The network interface is too busy to accept this transmit
                                 request.
  @retval  EFI_BUFFER_TOO_SMALL  The BufferSize parameter is too small.
  @retval  EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval  EFI_ACCESS_DENIED     Error acquire global lock for operation.

**/
EFI_STATUS
EFIAPI
EmuSnpReceive(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  OUT UINTN*              HeaderSize OPTIONAL,
  IN OUT UINTN*            BuffSize,
  OUT VOID*              Buffer,
  OUT EFI_MAC_ADDRESS*        SourceAddr OPTIONAL,
  OUT EFI_MAC_ADDRESS*        DestinationAddr OPTIONAL,
  OUT UINT16*              Protocol OPTIONAL
  );

VOID
EFIAPI
EmuSnpWaitForPacketNotify(
  IN EFI_EVENT            Event,
  IN VOID*              Private
  );

#endif  // _EMU_SNP_H_
