/** @file

 Copyright (c) 2010, Apple, Inc. All rights reserved.<BR>
 Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

    This program and the accompanying materials
    are licensed and made available under the terms and conditions of the BSD License
    which accompanies this distribution. The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  EmuSnp.c

Abstract:

-**/

#include "EmuSnpDxe.h"



EFI_SIMPLE_NETWORK_PROTOCOL gEmuSnpTemplate = {
  EFI_SIMPLE_NETWORK_PROTOCOL_REVISION,
  EmuSnpStart,
  EmuSnpStop,
  EmuSnpInitialize,
  EmuSnpReset,
  EmuSnpShutdown,
  EmuSnpReceiveFilters,
  EmuSnpStationAddress,
  EmuSnpStatistics,
  EmuSnpMcastIptoMac,
  EmuSnpNvdata,
  EmuSnpGetStatus,
  EmuSnpTransmit,
  EmuSnpReceive,
  NULL,                     // WaitForPacket
  NULL                      // Mode
 };

EFI_SIMPLE_NETWORK_MODE gEmuSnpModeTemplate = {                 
  EfiSimpleNetworkStopped,      //  State
  NET_ETHER_ADDR_LEN,           //  HwAddressSize
  NET_ETHER_HEADER_SIZE,        //  MediaHeaderSize
  1500,                         //  MaxPacketSize
  0,                            //  NvRamSize
  0,                            //  NvRamAccessSize
  0,                            //  ReceiveFilterMask
  0,                            //  ReceiveFilterSetting
  MAX_MCAST_FILTER_CNT,         //  MaxMCastFilterCount
  0,                            //  MCastFilterCount
  {
    { { 0 } }
  },                            //  MCastFilter
  {
    { 0 }
  },                            //  CurrentAddress
  {
    { 0 }
  },                            //  BroadcastAddress
  {
    { 0 }
  },                            //  PermanentAddress
  NET_IFTYPE_ETHERNET,          //  IfType
  FALSE,                        //  MacAddressChangeable
  FALSE,                        //  MultipleTxSupported
  FALSE,                        //  MediaPresentSupported
  TRUE                          //  MediaPresent
};


/**
  Changes the state of a network interface from "stopped" to "started".

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
EmuSnpStart(
  IN EFI_SIMPLE_NETWORK_PROTOCOL    *This
  )
{
  EFI_STATUS              Status;
  EMU_SNP_PRIVATE_DATA    *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_SNP_THIS (This);

  Status = Private->Io->Start (Private->Io);
  return Status;
}


/**
  Changes the state of a network interface from "started" to "stopped".

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
EmuSnpStop (
  IN EFI_SIMPLE_NETWORK_PROTOCOL    *This
  )
{
  EFI_STATUS              Status;
  EMU_SNP_PRIVATE_DATA    *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_SNP_THIS (This);

  Status = Private->Io->Stop (Private->Io);
  return Status;
}


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
EmuSnpInitialize (
  IN EFI_SIMPLE_NETWORK_PROTOCOL    *This,
  IN UINTN                          ExtraRxBufferSize OPTIONAL,
  IN UINTN                          ExtraTxBufferSize OPTIONAL
  )
{
  EFI_STATUS              Status;
  EMU_SNP_PRIVATE_DATA    *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_SNP_THIS (This);

  Status = Private->Io->Initialize (Private->Io, ExtraRxBufferSize, ExtraTxBufferSize);
  return Status;
}

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
EmuSnpReset (
  IN EFI_SIMPLE_NETWORK_PROTOCOL    *This,
  IN BOOLEAN                        ExtendedVerification
  )
{
  EFI_STATUS              Status;
  EMU_SNP_PRIVATE_DATA    *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_SNP_THIS (This);

  Status = Private->Io->Reset (Private->Io, ExtendedVerification);
  return Status;
}

/**
  Resets a network adapter and leaves it in a state that is safe for
  another driver to initialize.

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
EmuSnpShutdown (
  IN EFI_SIMPLE_NETWORK_PROTOCOL    *This
  )
{
  EFI_STATUS              Status;
  EMU_SNP_PRIVATE_DATA    *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_SNP_THIS (This);

  Status = Private->Io->Shutdown (Private->Io);
  return Status;
}

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
EmuSnpReceiveFilters (
  IN EFI_SIMPLE_NETWORK_PROTOCOL    *This,
  IN UINT32                         EnableBits,
  IN UINT32                         DisableBits,
  IN BOOLEAN                        ResetMcastFilter,
  IN UINTN                          McastFilterCount OPTIONAL,
  IN EFI_MAC_ADDRESS                *McastFilter OPTIONAL
  )
{
  EFI_STATUS              Status;
  EMU_SNP_PRIVATE_DATA    *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_SNP_THIS (This);

  Status = Private->Io->ReceiveFilters (
                          Private->Io,
                          EnableBits,
                          DisableBits,
                          ResetMcastFilter,
                          McastFilterCount,
                          McastFilter
                          );
  return Status;
}

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
EmuSnpStationAddress (
  IN EFI_SIMPLE_NETWORK_PROTOCOL    *This,
  IN BOOLEAN                        Reset,
  IN EFI_MAC_ADDRESS                *NewMacAddr OPTIONAL
  )
{
  EFI_STATUS              Status;
  EMU_SNP_PRIVATE_DATA    *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_SNP_THIS (This);

  Status = Private->Io->StationAddress (Private->Io, Reset, NewMacAddr);
  return Status;
}

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
EmuSnpStatistics (
  IN EFI_SIMPLE_NETWORK_PROTOCOL    *This,
  IN BOOLEAN                        Reset,
  IN OUT UINTN                      *StatisticsSize OPTIONAL,
  OUT EFI_NETWORK_STATISTICS        *StatisticsTable OPTIONAL
  )
{
  EFI_STATUS              Status;
  EMU_SNP_PRIVATE_DATA    *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_SNP_THIS (This);

  Status = Private->Io->Statistics (Private->Io, Reset, StatisticsSize, StatisticsTable);
  return Status;
}

/**
  Converts a multicast IP address to a multicast HW MAC address.

  @param  This Protocol instance pointer.
  @param  Ipv6 Set to TRUE if the multicast IP address is IPv6 [RFC 2460]. Set
               to FALSE if the multicast IP address is IPv4 [RFC 791].
  @param  Ip   The multicast IP address that is to be converted to a multicast
               HW MAC address.
  @param  Mac  The multicast HW MAC address that is to be generated from IP.

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
EmuSnpMcastIptoMac (
  IN EFI_SIMPLE_NETWORK_PROTOCOL    *This,
  IN BOOLEAN                        Ipv6,
  IN EFI_IP_ADDRESS                 *Ip,
  OUT EFI_MAC_ADDRESS               *Mac
  )
{
  EFI_STATUS              Status;
  EMU_SNP_PRIVATE_DATA    *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_SNP_THIS (This);

  Status = Private->Io->MCastIpToMac (Private->Io, Ipv6, Ip, Mac);
  return Status;
}


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
EmuSnpNvdata (
  IN EFI_SIMPLE_NETWORK_PROTOCOL    *This,
  IN BOOLEAN                        ReadOrWrite,
  IN UINTN                          Offset,
  IN UINTN                          BufferSize,
  IN OUT VOID                       *Buffer
  )
{
  EFI_STATUS              Status;
  EMU_SNP_PRIVATE_DATA    *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_SNP_THIS (This);

  Status = Private->Io->NvData (Private->Io, ReadOrWrite, Offset, BufferSize, Buffer);
  return Status;
}


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
EmuSnpGetStatus (
  IN EFI_SIMPLE_NETWORK_PROTOCOL    *This,
  OUT UINT32                        *InterruptStatus,
  OUT VOID                          **TxBuffer
  )
{
  EFI_STATUS              Status;
  EMU_SNP_PRIVATE_DATA    *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_SNP_THIS (This);

  Status = Private->Io->GetStatus (Private->Io, InterruptStatus, TxBuffer);
  return Status;
}


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
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_NOT_STARTED       The network interface has not been started.

**/
EFI_STATUS
EFIAPI
EmuSnpTransmit (
  IN EFI_SIMPLE_NETWORK_PROTOCOL    *This,
  IN UINTN                          HeaderSize,
  IN UINTN                          BufferSize,
  IN VOID*                          Buffer,
  IN EFI_MAC_ADDRESS                *SrcAddr OPTIONAL,
  IN EFI_MAC_ADDRESS                *DestAddr OPTIONAL,
  IN UINT16                         *Protocol OPTIONAL
  )
{
  EFI_STATUS              Status;
  EMU_SNP_PRIVATE_DATA    *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_SNP_THIS (This);

  Status = Private->Io->Transmit (
                          Private->Io,
                          HeaderSize,
                          BufferSize,
                          Buffer,
                          SrcAddr,
                          DestAddr,
                          Protocol
                          );
  return Status;
}

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

  @retval EFI_SUCCESS           The received data was stored in Buffer, and BufferSize has
                                been updated to the number of bytes received.
  @retval EFI_NOT_READY         The network interface is too busy to accept this transmit
                                request.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize parameter is too small.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.

**/
EFI_STATUS
EFIAPI
EmuSnpReceive (
  IN EFI_SIMPLE_NETWORK_PROTOCOL    *This,
  OUT UINTN                         *HeaderSize OPTIONAL,
  IN OUT UINTN                      *BuffSize,
  OUT VOID                          *Buffer,
  OUT EFI_MAC_ADDRESS               *SourceAddr OPTIONAL,
  OUT EFI_MAC_ADDRESS               *DestinationAddr OPTIONAL,
  OUT UINT16                        *Protocol OPTIONAL
  )
{
  EFI_STATUS              Status;
  EMU_SNP_PRIVATE_DATA    *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_SNP_THIS (This);

  Status = Private->Io->Receive (
                          Private->Io,
                          HeaderSize,
                          BuffSize,
                          Buffer,
                          SourceAddr,
                          DestinationAddr,
                          Protocol
                          );
  return Status;
}



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
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                Status;
  EMU_IO_THUNK_PROTOCOL     *EmuIoThunk;
  MAC_ADDR_DEVICE_PATH      *Node;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;

  if (RemainingDevicePath != NULL) {
    if (!IsDevicePathEnd (RemainingDevicePath)) {
      Node = (MAC_ADDR_DEVICE_PATH *)RemainingDevicePath;
      if (Node->Header.Type != MESSAGING_DEVICE_PATH ||
          Node->Header.SubType != MSG_MAC_ADDR_DP) {
        // If the remaining device path does not match we don't support the request
        return EFI_UNSUPPORTED;
      }
    }
  }


  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEmuIoThunkProtocolGuid,
                  (VOID **)&EmuIoThunk,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
        ControllerHandle,
        &gEmuIoThunkProtocolGuid,
        This->DriverBindingHandle,
        ControllerHandle
        );


  //
  // Open the EFI Device Path protocol needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Make sure GUID is for a SNP handle.
  //
  Status = EFI_UNSUPPORTED;
  if (CompareGuid (EmuIoThunk->Protocol, &gEmuSnpProtocolGuid)) {
    Status = EFI_SUCCESS;
  }

  //
  // Close protocol, don't use device path protocol in the Support() function
  //
  gBS->CloseProtocol (
        ControllerHandle,
        &gEfiDevicePathProtocolGuid,
        This->DriverBindingHandle,
        ControllerHandle
        );

  return Status;
}


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
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                  Status;
  EMU_IO_THUNK_PROTOCOL       *EmuIoThunk;
  EMU_SNP_PRIVATE_DATA        *Private;
  MAC_ADDR_DEVICE_PATH        Node;
  EFI_DEVICE_PATH_PROTOCOL    *ParentDevicePath;

  Private = NULL;

  //
  // Grab the protocols we need
  //
  Status = gBS->OpenProtocol(
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  ( VOID ** ) &ParentDevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEmuIoThunkProtocolGuid,
                  (VOID **)&EmuIoThunk,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!CompareGuid (EmuIoThunk->Protocol, &gEmuSnpProtocolGuid)) {
    return EFI_UNSUPPORTED;
  }

  Status = EmuIoThunk->Open (EmuIoThunk);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  //  Allocate the private data.
  //
  Private = AllocateZeroPool (sizeof (EMU_SNP_PRIVATE_DATA));
  if (Private == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  CopyMem (&Private->Snp, &gEmuSnpTemplate, sizeof (EFI_SIMPLE_NETWORK_PROTOCOL));
  CopyMem (&Private->Mode, &gEmuSnpModeTemplate, sizeof (EFI_SIMPLE_NETWORK_MODE));

  Private->Signature    = EMU_SNP_PRIVATE_DATA_SIGNATURE;
  Private->IoThunk      = EmuIoThunk;
  Private->Io           = EmuIoThunk->Interface;
  Private->EfiHandle    = ControllerHandle;
  Private->DeviceHandle = NULL;
  Private->Snp.Mode     = &Private->Mode;
  Private->ControllerNameTable = NULL;


  Status = Private->Io->CreateMapping (Private->Io, &Private->Mode);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Build the device path by appending the MAC node to the ParentDevicePath
  // from the EmuIo handle.
  //
  ZeroMem (&Node, sizeof (MAC_ADDR_DEVICE_PATH));

  Node.Header.Type     = MESSAGING_DEVICE_PATH;
  Node.Header.SubType  = MSG_MAC_ADDR_DP;
  Node.IfType          = Private->Mode.IfType;

  SetDevicePathNodeLength ((EFI_DEVICE_PATH_PROTOCOL * )&Node, sizeof (MAC_ADDR_DEVICE_PATH));

  CopyMem (&Node.MacAddress, &Private->Mode.CurrentAddress, sizeof (EFI_MAC_ADDRESS));

  //
  // Build the device path by appending the MAC node to the ParentDevicePath from the EmuIo handle.
  //
  Private->DevicePath = AppendDevicePathNode (ParentDevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&Node);
  if ( Private->DevicePath == NULL ) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  AddUnicodeString2 (
    "eng",
    gEmuSnpDriverComponentName.SupportedLanguages,
    &Private->ControllerNameTable,
    EmuIoThunk->ConfigString,
    TRUE
    );

  AddUnicodeString2 (
    "en",
    gEmuSnpDriverComponentName2.SupportedLanguages,
    &Private->ControllerNameTable,
    EmuIoThunk->ConfigString,
    FALSE
    );

  //
  // Create Child Handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces(
                  &Private->DeviceHandle,
                  &gEfiSimpleNetworkProtocolGuid, &Private->Snp,
                  &gEfiDevicePathProtocolGuid,    Private->DevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Open For Child Device
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEmuIoThunkProtocolGuid,
                  (VOID **)&EmuIoThunk,
                  This->DriverBindingHandle,
                  Private->DeviceHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );

Done:
  if (EFI_ERROR (Status)) {
    if (Private != NULL) {
      FreePool (Private);
    }
    if (ParentDevicePath != NULL) {
      gBS->CloseProtocol(
            ControllerHandle,
            &gEfiDevicePathProtocolGuid,
            This->DriverBindingHandle,
            ControllerHandle
            );
    }
  }

  return Status;
}

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
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     ControllerHandle,
  IN UINTN                          NumberOfChildren,
  IN EFI_HANDLE                     *ChildHandleBuffer
  )
{
  EFI_STATUS                  Status;
  EMU_SNP_PRIVATE_DATA        *Private = NULL;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;

  //
  // Complete all outstanding transactions to Controller.
  // Don't allow any new transaction to Controller to be started.
  //
  if (NumberOfChildren == 0) {
    //
    // Close the bus driver
    //
    Status = gBS->CloseProtocol (
                    ControllerHandle,
                    &gEmuIoThunkProtocolGuid,
                    This->DriverBindingHandle,
                    ControllerHandle
                    );

    Status = gBS->CloseProtocol (
                    ControllerHandle,
                    &gEfiDevicePathProtocolGuid,
                    This->DriverBindingHandle,
                    ControllerHandle
                    );
    return Status;
  }

  ASSERT (NumberOfChildren == 1);


  //
  // Get our context back.
  //
  Status = gBS->OpenProtocol(
                  ChildHandleBuffer[0],
                  &gEfiSimpleNetworkProtocolGuid,
                  ( VOID ** ) &Snp,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Private = EMU_SNP_PRIVATE_DATA_FROM_SNP_THIS (Snp);
  Status = Private->IoThunk->Close (Private->IoThunk);

  Status = gBS->CloseProtocol(
                  ChildHandleBuffer[0],
                  &gEmuIoThunkProtocolGuid,
                  This->DriverBindingHandle,
                  Private->DeviceHandle
                  );

  Status = gBS->UninstallMultipleProtocolInterfaces(
                  ChildHandleBuffer[0],
                  &gEfiSimpleNetworkProtocolGuid,   &Private->Snp,
                  &gEfiDevicePathProtocolGuid,      Private->DevicePath,
                  NULL
                  );

  FreePool (Private->DevicePath);
  FreeUnicodeStringTable (Private->ControllerNameTable);
  FreePool (Private);

  return EFI_SUCCESS;
}


EFI_DRIVER_BINDING_PROTOCOL gEmuSnpDriverBinding = {
  EmuSnpDriverBindingSupported,
  EmuSnpDriverBindingStart,
  EmuSnpDriverBindingStop,
  0xA,
  NULL,
  NULL
};



/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.

  @param  ImageHandle           The firmware allocated handle for the UEFI image.
  @param  SystemTable           A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
InitializeEmuSnpDriver (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS            Status;

  //
  // Install the Driver Protocols
  //
  Status = EfiLibInstallDriverBindingComponentName2(
              ImageHandle,
              SystemTable,
              &gEmuSnpDriverBinding,
              ImageHandle,
              &gEmuSnpDriverComponentName,
              &gEmuSnpDriverComponentName2
              );

  return Status;
}
