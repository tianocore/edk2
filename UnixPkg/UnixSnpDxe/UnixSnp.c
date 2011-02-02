/** @file

 Copyright (c) 2010, Apple, Inc. All rights reserved.<BR>

    This program and the accompanying materials
    are licensed and made available under the terms and conditions of the BSD License
    which accompanies this distribution. The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  UnixSnp.c

Abstract:

-**/

#include <Library/PrintLib.h>

#include "UnixSnp.h"

EFI_DRIVER_BINDING_PROTOCOL gUnixSnpDriverBinding =
{
  UnixSnpDriverBindingSupported,
  UnixSnpDriverBindingStart,
  UnixSnpDriverBindingStop,
  0xA,
  NULL,
  NULL
};

/**
  Changes the state of a network interface from "stopped" to "started".

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
UnixSnpStart(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This
  );
  
/**
  Changes the state of a network interface from "started" to "stopped".

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
UnixSnpStop(
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
UnixSnpInitialize(
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
UnixSnpReset(
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
UnixSnpShutdown(
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
UnixSnpReceiveFilters(
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
UnixSnpStationAddress(
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
UnixSnpStatistics(
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
UnixSnpMcastIptoMac(
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
UnixSnpNvdata(
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
UnixSnpGetStatus(
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
UnixSnpTransmit(
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
UnixSnpReceive(
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
UnixSnpWaitForPacketNotify(
  IN EFI_EVENT            Event,
  IN VOID*              Private
  );

//
// Strange, but there doesn't appear to be any structure for the Ethernet header in edk2...
//

typedef struct
{
  UINT8                DstAddr[ NET_ETHER_ADDR_LEN ];
  UINT8                SrcAddr[ NET_ETHER_ADDR_LEN ];
  UINT16                Type;
} EthernetHeader;

UNIX_SNP_PRIVATE_DATA gUnixSnpPrivateTemplate =
{
  UNIX_SNP_PRIVATE_DATA_SIGNATURE,      // Signature
  NULL,                    // UnixThunk
  NULL,                    // DeviceHandle
  NULL,                    // DevicePath
  { 0 },                    // MacAddress
  NULL,                    // InterfaceName
  0,                      // ReadBufferSize
  NULL,                    // ReadBuffer
  NULL,                    // CurrentReadPointer
  NULL,                    // EndReadPointer
  0,                      // BpfFd
  {                    // Snp
    EFI_SIMPLE_NETWORK_PROTOCOL_REVISION,  // Revision
    UnixSnpStart,              // Start
    UnixSnpStop,              // Stop
    UnixSnpInitialize,            // Initialize
    UnixSnpReset,              // Reset
    UnixSnpShutdown,            // Shutdown
    UnixSnpReceiveFilters,          // ReceiveFilters
    UnixSnpStationAddress,          // StationAddress
    UnixSnpStatistics,            // Statistics
    UnixSnpMcastIptoMac,          // MCastIpToMac
    UnixSnpNvdata,              // NvData
    UnixSnpGetStatus,            // GetStatus
    UnixSnpTransmit,            // Transmit
    UnixSnpReceive,              // Receive
    NULL,                  // WaitForPacket
    NULL                  // Mode
  },
  {                    // Mode
    EfiSimpleNetworkStopped,        //  State
    NET_ETHER_ADDR_LEN,            //  HwAddressSize
    NET_ETHER_HEADER_SIZE,          //  MediaHeaderSize
    1500,                  //  MaxPacketSize
    0,                    //  NvRamSize
    0,                    //  NvRamAccessSize
    0,                    //  ReceiveFilterMask
    0,                    //  ReceiveFilterSetting
    MAX_MCAST_FILTER_CNT,          //  MaxMCastFilterCount
    0,                    //  MCastFilterCount
    {
      0
    },                    //  MCastFilter
    {
      0
    },                    //  CurrentAddress
    {
      0
    },                    //  BroadcastAddress
    {
      0
    },                    //  PermanentAddress
    NET_IFTYPE_ETHERNET,          //  IfType
    FALSE,                  //  MacAddressChangeable
    FALSE,                  //  MultipleTxSupported
    FALSE,                  //  MediaPresentSupported
    TRUE                  //  MediaPresent
  }
};

STATIC EFI_STATUS
GetInterfaceMacAddr(
  IN UNIX_SNP_PRIVATE_DATA*      Private,
  IN EFI_UNIX_IO_PROTOCOL*      UnixIo
  )
{
  struct ifaddrs*            IfAddrs;
  struct ifaddrs*            If;
  struct sockaddr_dl*          IfSdl;
  EFI_STATUS              Status;
  INTN                Result;

  Result = UnixIo->UnixThunk->GetIfAddrs( &IfAddrs );
  if ( Result != 0 )
  {
    return( EFI_UNSUPPORTED );
  }

  //
  // Convert the interface name to ASCII so we can find it.
  //
  Private->InterfaceName = AllocateZeroPool( StrLen( UnixIo->EnvString ) );

  if ( !Private->InterfaceName )
  {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  UnicodeStrToAsciiStr( UnixIo->EnvString, Private->InterfaceName );

  If = IfAddrs;

  while ( If != NULL )
  {
    IfSdl = ( struct sockaddr_dl * ) If->ifa_addr;

    if ( IfSdl->sdl_family == AF_LINK )
    {
      if ( !AsciiStrCmp( Private->InterfaceName, If->ifa_name ) )
      {
        CopyMem( &Private->MacAddress, LLADDR( IfSdl ), NET_ETHER_ADDR_LEN );

        Status = EFI_SUCCESS;
        break;
      }
    }

    If = If->ifa_next;
  }

Exit:
  ( VOID ) UnixIo->UnixThunk->FreeIfAddrs( IfAddrs );

  return( Status );
}


STATIC EFI_STATUS
OpenBpfFileDescriptor(
  IN UNIX_SNP_PRIVATE_DATA*    Private,
  OUT INTN*            Fd
  )
{
  CHAR8              BfpDeviceName[ 256 ];
  INTN              Index;
  EFI_STATUS            Status = EFI_OUT_OF_RESOURCES;
  INTN              Result;

  //
  // Open a Berkeley Packet Filter device.  This must be done as root, so this is probably
  // the place which is most likely to fail...
  //
  for ( Index = 0; TRUE; Index++ )
  {
    AsciiSPrint( BfpDeviceName, sizeof( BfpDeviceName ), "/dev/bpf%d", Index );

    *Fd = Private->UnixThunk->Open( BfpDeviceName, O_RDWR, 0 );

    if ( *Fd >= 0 )
    {
      Status = EFI_SUCCESS;
      break;
    }

    Result = Private->UnixThunk->GetErrno();
    if ( Result == EACCES )
    {
      DEBUG( ( EFI_D_ERROR, "Permissions on '%a' are incorrect.  Fix with 'sudo chmod 666 %a'.\n",
          BfpDeviceName, BfpDeviceName ) );
    }
    if ( Result != EBUSY )
    {
      break;
    }
  }

  return( Status );
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
UnixSnpDriverBindingSupported(
  IN EFI_DRIVER_BINDING_PROTOCOL*    This,
  IN EFI_HANDLE            ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL*    RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS              Status;
  EFI_UNIX_IO_PROTOCOL*        UnixIo;

  //
  // Open the I/O abstraction needed to perform the supported test.
  //
  Status = gBS->OpenProtocol(
          ControllerHandle,
          &gEfiUnixIoProtocolGuid,
          ( VOID ** ) &UnixIo,
          This->DriverBindingHandle,
          ControllerHandle,
          EFI_OPEN_PROTOCOL_BY_DRIVER
          );

  if ( EFI_ERROR( Status ) )
  {
    return( Status );
  }

  //
  // Validate GUID
  //
  Status = EFI_UNSUPPORTED;
  if ( CompareGuid( UnixIo->TypeGuid, &gEfiUnixNetworkGuid ) )
  {
    Status = EFI_SUCCESS;
  }

  //
  // Close the I/O abstraction used to perform the supported test.
  //
  gBS->CloseProtocol(
          ControllerHandle,
          &gEfiUnixIoProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );

  return( Status );
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
UnixSnpDriverBindingStart(
  IN EFI_DRIVER_BINDING_PROTOCOL*    This,
  IN EFI_HANDLE            ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL*    RemainingDevicePath OPTIONAL
  )
{
  MAC_ADDR_DEVICE_PATH      Node;
  EFI_DEVICE_PATH_PROTOCOL*    ParentDevicePath = NULL;
  EFI_UNIX_IO_PROTOCOL*      UnixIo;
  UNIX_SNP_PRIVATE_DATA*      Private = NULL;
  EFI_STATUS            Status;
  BOOLEAN              CreateDevice;

  //
  // Grab the protocols we need.
  //
  Status = gBS->OpenProtocol(
          ControllerHandle,
          &gEfiDevicePathProtocolGuid,
          ( VOID ** ) &ParentDevicePath,
          This->DriverBindingHandle,
          ControllerHandle,
          EFI_OPEN_PROTOCOL_BY_DRIVER
          );
  if ( EFI_ERROR( Status ) )
  {
    goto ErrorExit;
  }

  //
  // Open the I/O abstraction needed to perform the supported test.
  //
  Status = gBS->OpenProtocol(
          ControllerHandle,
          &gEfiUnixIoProtocolGuid,
          ( VOID ** ) &UnixIo,
          This->DriverBindingHandle,
          ControllerHandle,
          EFI_OPEN_PROTOCOL_BY_DRIVER
          );
  if ( EFI_ERROR( Status ) )
  {
    goto ErrorExit;
  }

  //
  // Validate GUID
  //
  if ( !CompareGuid( UnixIo->TypeGuid, &gEfiUnixNetworkGuid ) )
  {
    Status = EFI_UNSUPPORTED;
    goto ErrorExit;
  }

  CreateDevice = TRUE;
  if ( ( RemainingDevicePath != NULL ) && IsDevicePathEnd( RemainingDevicePath ) )
  {
    CreateDevice = FALSE;
  }

  if ( CreateDevice )
  {
    //
    //  Allocate the private data.
    //
    Private = AllocateCopyPool( sizeof( UNIX_SNP_PRIVATE_DATA ), &gUnixSnpPrivateTemplate );
    if ( Private == NULL )
    {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorExit;
    }

    Status = GetInterfaceMacAddr( Private, UnixIo );
    if ( EFI_ERROR( Status ) )
    {
      goto ErrorExit;
    }

    Private->UnixThunk = UnixIo->UnixThunk;

    Private->Snp.Mode = &Private->Mode;

    //
    // Set the broadcast address.
    //
    SetMem( &Private->Mode.BroadcastAddress, sizeof( EFI_MAC_ADDRESS ), 0xFF );

    CopyMem( &Private->Mode.CurrentAddress, &Private->MacAddress, sizeof( EFI_MAC_ADDRESS ) );
    CopyMem( &Private->Mode.PermanentAddress, &Private->MacAddress, sizeof( EFI_MAC_ADDRESS ) );

    //
    // Since the fake SNP is based on a real NIC, to avoid conflict with the host NIC
    // network stack, we use a different MAC address.
    // So just change the last byte of the MAC address for the real NIC.
    //
    Private->Mode.CurrentAddress.Addr[ NET_ETHER_ADDR_LEN - 1 ]++;

    //
    // Build the device path by appending the MAC node to the ParentDevicePath
    // from the UnixIo handle.
    //
    ZeroMem( &Node, sizeof( MAC_ADDR_DEVICE_PATH ) );

    Node.Header.Type  = MESSAGING_DEVICE_PATH;
    Node.Header.SubType  = MSG_MAC_ADDR_DP;
    Node.IfType      = Private->Mode.IfType;

    SetDevicePathNodeLength( ( EFI_DEVICE_PATH_PROTOCOL * ) &Node, sizeof( MAC_ADDR_DEVICE_PATH ) );

    CopyMem( &Node.MacAddress, &Private->Mode.CurrentAddress, sizeof( EFI_MAC_ADDRESS ) );

    //
    // Build the device path by appending the MAC node to the ParentDevicePath from the UnixIo handle.
    //
    Private->DevicePath = AppendDevicePathNode( ParentDevicePath, ( EFI_DEVICE_PATH_PROTOCOL * ) &Node );
    if ( Private->DevicePath == NULL )
    {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorExit;
    }

    Status = gBS->InstallMultipleProtocolInterfaces(
            &Private->DeviceHandle,
            &gEfiSimpleNetworkProtocolGuid,
            &Private->Snp,
            &gEfiDevicePathProtocolGuid,
            Private->DevicePath,
            NULL
            );
    if ( EFI_ERROR( Status ) )
    {
      goto ErrorExit;
    }

    Status = gBS->OpenProtocol(
            ControllerHandle,
            &gEfiUnixIoProtocolGuid,
            ( VOID ** ) &UnixIo,
            This->DriverBindingHandle,
            Private->DeviceHandle,
            EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
            );
    if ( EFI_ERROR( Status ) )
    {
      goto ErrorExit;
    }
  }
  return( Status );

ErrorExit:
  if ( Private->InterfaceName != NULL )
  {
    FreePool( Private->InterfaceName );
    Private->InterfaceName = NULL;
  }
  if ( Private != NULL )
  {
    FreePool( Private );
  }
  if ( ParentDevicePath != NULL )
  {
    gBS->CloseProtocol(
          ControllerHandle,
          &gEfiDevicePathProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
  }

  return( Status );
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
UnixSnpDriverBindingStop(
  IN EFI_DRIVER_BINDING_PROTOCOL*    This,
  IN EFI_HANDLE            ControllerHandle,
  IN UINTN              NumberOfChildren,
  IN EFI_HANDLE*            ChildHandleBuffer
  )
{
  UNIX_SNP_PRIVATE_DATA*        Private = NULL;
  EFI_SIMPLE_NETWORK_PROTOCOL*    Snp;
  EFI_STATUS              Status;

  //
  // Get our context back.
  //
  Status = gBS->OpenProtocol(
        ControllerHandle,
        &gEfiSimpleNetworkProtocolGuid,
        ( VOID ** ) &Snp,
        This->DriverBindingHandle,
        ControllerHandle,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
        );
  if ( EFI_ERROR( Status ) )
  {
    return( EFI_UNSUPPORTED );
  }

  Private = UNIX_SNP_PRIVATE_DATA_FROM_SNP_THIS( Snp );

  Status = gBS->CloseProtocol(
        ControllerHandle,
        &gEfiUnixIoProtocolGuid,
        This->DriverBindingHandle,
        Private->DeviceHandle
        );

  Status = gBS->UninstallMultipleProtocolInterfaces(
        Private->DeviceHandle,
        &gEfiSimpleNetworkProtocolGuid,
        &Private->Snp,
        &gEfiDevicePathProtocolGuid,
        Private->DevicePath,
        NULL
        );

  FreePool( Private->InterfaceName );
  FreePool( Private->DevicePath );
  FreePool( Private );

  return( EFI_SUCCESS );
}


/**
  Changes the state of a network interface from "stopped" to "started".

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
UnixSnpStart(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This
  )
{
  STATIC struct bpf_insn        FilterInstructionTemplate[] =
  {
    // Load 4 bytes from the destination MAC address.
    BPF_STMT( BPF_LD + BPF_W + BPF_ABS, OFFSET_OF( EthernetHeader, DstAddr[ 0 ] ) ),

    // Compare to first 4 bytes of fake MAC address.
    BPF_JUMP( BPF_JMP + BPF_JEQ + BPF_K, 0x12345678, 0, 3 ),

    // Load remaining 2 bytes from the destination MAC address.
    BPF_STMT( BPF_LD + BPF_H + BPF_ABS, OFFSET_OF( EthernetHeader, DstAddr[ 4 ] ) ),

    // Compare to remaining 2 bytes of fake MAC address.
    BPF_JUMP( BPF_JMP + BPF_JEQ + BPF_K, 0x9ABC, 5, 0 ),

    // Load 4 bytes from the destination MAC address.
    BPF_STMT( BPF_LD + BPF_W + BPF_ABS, OFFSET_OF( EthernetHeader, DstAddr[ 0 ] ) ),

    // Compare to first 4 bytes of broadcast MAC address.
    BPF_JUMP( BPF_JMP + BPF_JEQ + BPF_K, 0xFFFFFFFF, 0, 2 ),

    // Load remaining 2 bytes from the destination MAC address.
    BPF_STMT( BPF_LD + BPF_H + BPF_ABS, OFFSET_OF( EthernetHeader, DstAddr[ 4 ] ) ),

    // Compare to remaining 2 bytes of broadcast MAC address.
    BPF_JUMP( BPF_JMP + BPF_JEQ + BPF_K, 0xFFFF, 1, 0 ),

    // Reject packet.
    BPF_STMT( BPF_RET + BPF_K, 0 ),

    // Receive entire packet.
    BPF_STMT( BPF_RET + BPF_K, -1 )
  };
  struct ifreq            BoundIf;
  struct bpf_program          BpfProgram;
  struct bpf_insn*          FilterProgram;
  UNIX_SNP_PRIVATE_DATA*        Private;
  EFI_STATUS              Status;
  UINT32                Temp32;
  INTN                Fd;
  INTN                Result;
  INTN                Value;
  UINT16                Temp16;

  Private = UNIX_SNP_PRIVATE_DATA_FROM_SNP_THIS( This );

  switch ( Private->Snp.Mode->State )
  {
    case EfiSimpleNetworkStopped:
      break;

    case EfiSimpleNetworkStarted:
    case EfiSimpleNetworkInitialized:
      return( EFI_ALREADY_STARTED );
      break;

    default:
      return( EFI_DEVICE_ERROR );
      break;
  }

  if ( Private->BpfFd == 0 )
  {
    Status = OpenBpfFileDescriptor( Private, &Fd );

    if ( EFI_ERROR( Status ) )
    {
      goto ErrorExit;
    }

    Private->BpfFd = Fd;

    //
    // Associate our interface with this BPF file descriptor.
    //
    AsciiStrCpy( BoundIf.ifr_name, Private->InterfaceName );
    Result = Private->UnixThunk->IoCtl( Private->BpfFd, BIOCSETIF, &BoundIf );

    if ( Result < 0 )
    {
      goto DeviceErrorExit;
    }

    //
    // Enable immediate mode and find out the buffer size.
    //
    Value = 1;
    Result = Private->UnixThunk->IoCtl( Private->BpfFd, BIOCIMMEDIATE, &Value );

    if ( Result < 0 )
    {
      goto DeviceErrorExit;
    }

    //
    // Enable non-blocking I/O.
    //

    Value = Private->UnixThunk->Fcntl( Private->BpfFd, F_GETFL, 0 );

    if ( Value == -1 )
    {
      goto DeviceErrorExit;
    }

    Value |= O_NONBLOCK;

    Result = Private->UnixThunk->Fcntl( Private->BpfFd, F_SETFL, (void *) Value );

    if ( Result == -1 )
    {
      goto DeviceErrorExit;
    }

    //
    // Disable "header complete" flag.  This means the supplied source MAC address is
    // what goes on the wire.
    //
    Value = 1;
    Result = Private->UnixThunk->IoCtl( Private->BpfFd, BIOCSHDRCMPLT, &Value );

    if ( Result < 0 )
    {
      goto DeviceErrorExit;
    }

    Result = Private->UnixThunk->IoCtl( Private->BpfFd, BIOCGBLEN, &Value );

    if ( Result < 0 )
    {
      goto DeviceErrorExit;
    }

    //
    // Allocate read buffer.
    //
    Private->ReadBufferSize = Value;
    Private->ReadBuffer = AllocateZeroPool( Private->ReadBufferSize );
    if ( Private->ReadBuffer == NULL )
    {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorExit;
    }

    Private->CurrentReadPointer = Private->EndReadPointer = Private->ReadBuffer;

    //
    // Install our packet filter: successful reads should only produce broadcast or unitcast
    // packets directed to our fake MAC address.
    //
    FilterProgram = AllocateCopyPool( sizeof( FilterInstructionTemplate ), &FilterInstructionTemplate );
    if ( FilterProgram == NULL )
    {
      goto ErrorExit;
    }

    //
    // Insert out fake MAC address into the filter.  The data has to be host endian.
    //
    CopyMem( &Temp32, &Private->Mode.CurrentAddress.Addr[ 0 ], sizeof( UINT32 ) );
    FilterProgram[ 1 ].k = NTOHL( Temp32 );
    CopyMem( &Temp16, &Private->Mode.CurrentAddress.Addr[ 4 ], sizeof( UINT16 ) );
    FilterProgram[ 3 ].k = NTOHS( Temp16 );

    BpfProgram.bf_len = sizeof( FilterInstructionTemplate ) / sizeof( struct bpf_insn );
    BpfProgram.bf_insns = FilterProgram;

    Result = Private->UnixThunk->IoCtl( Private->BpfFd, BIOCSETF, &BpfProgram );

    if ( Result < 0 )
    {
      goto DeviceErrorExit;
    }

    FreePool( FilterProgram );

    //
    // Enable promiscuous mode.
    //

    Result = Private->UnixThunk->IoCtl( Private->BpfFd, BIOCPROMISC, 0 );

    if ( Result < 0 )
    {
      goto DeviceErrorExit;
    }


    Private->Snp.Mode->State = EfiSimpleNetworkStarted;      
  }

  return( Status );

DeviceErrorExit:
  Status = EFI_DEVICE_ERROR;
ErrorExit:
  if ( Private->ReadBuffer != NULL )
  {
    FreePool( Private->ReadBuffer );
    Private->ReadBuffer = NULL;
  }
  return( Status );
}


/**
  Changes the state of a network interface from "started" to "stopped".

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
UnixSnpStop(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This
  )
{
  UNIX_SNP_PRIVATE_DATA*        Private = EFI_SUCCESS;
  EFI_STATUS              Status;

  Private = UNIX_SNP_PRIVATE_DATA_FROM_SNP_THIS( This );

  switch ( Private->Snp.Mode->State )
  {
    case EfiSimpleNetworkStarted:
      break;

    case EfiSimpleNetworkStopped:
      return( EFI_NOT_STARTED );
      break;

    default:
      return( EFI_DEVICE_ERROR );
      break;
  }

  if ( Private->BpfFd != 0 )
  {
    Private->UnixThunk->Close( Private->BpfFd );
    Private->BpfFd = 0;
  }

  if ( Private->ReadBuffer != NULL )
  {
    FreePool( Private->ReadBuffer );
    Private->CurrentReadPointer = Private->EndReadPointer = Private->ReadBuffer = NULL;
  }

  Private->Snp.Mode->State = EfiSimpleNetworkStopped;

  return( Status );
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
UnixSnpInitialize(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  IN UINTN              ExtraRxBufferSize OPTIONAL,
  IN UINTN              ExtraTxBufferSize OPTIONAL
  )
{
  UNIX_SNP_PRIVATE_DATA*        Private = EFI_SUCCESS;
  EFI_STATUS              Status;

  Private = UNIX_SNP_PRIVATE_DATA_FROM_SNP_THIS( This );

  switch ( Private->Snp.Mode->State )
  {
    case EfiSimpleNetworkStarted:
      break;

    case EfiSimpleNetworkStopped:
      return( EFI_NOT_STARTED );
      break;

    default:
      return( EFI_DEVICE_ERROR );
      break;
  }

#if 0
  Status = gBS->CreateEvent(
          EVT_NOTIFY_WAIT,
          TPL_NOTIFY,
          UnixSnpWaitForPacketNotify,
          Private,
          &Private->Snp.WaitForPacket
          );
#endif

  if ( !EFI_ERROR( Status ) )
  {
    Private->Mode.MCastFilterCount = 0;
    Private->Mode.ReceiveFilterSetting = 0;
    ZeroMem( Private->Mode.MCastFilter, sizeof( Private->Mode.MCastFilter ) );

    Private->Snp.Mode->State = EfiSimpleNetworkInitialized;
  }

  return( Status );
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
UnixSnpReset(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  IN BOOLEAN              ExtendedVerification
  )
{
  UNIX_SNP_PRIVATE_DATA*        Private;
  EFI_STATUS              Success = EFI_SUCCESS;

  Private = UNIX_SNP_PRIVATE_DATA_FROM_SNP_THIS( This );

  switch ( Private->Snp.Mode->State )
  {
    case EfiSimpleNetworkInitialized:
      break;

    case EfiSimpleNetworkStopped:
      return( EFI_NOT_STARTED );
      break;

    default:
      return( EFI_DEVICE_ERROR );
      break;
  }

  return( Success );
}

/**
  Resets a network adapter and leaves it in a state that is safe for 
  another driver to initialize.

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
UnixSnpShutdown(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This
  )
{
  UNIX_SNP_PRIVATE_DATA*        Private;
  EFI_STATUS              Success = EFI_SUCCESS;

  Private = UNIX_SNP_PRIVATE_DATA_FROM_SNP_THIS( This );

  switch ( Private->Snp.Mode->State )
  {
    case EfiSimpleNetworkInitialized:
      break;

    case EfiSimpleNetworkStopped:
      return( EFI_NOT_STARTED );
      break;

    default:
      return( EFI_DEVICE_ERROR );
      break;
  }

  Private->Snp.Mode->State = EfiSimpleNetworkStarted;

  Private->Mode.ReceiveFilterSetting = 0;
  Private->Mode.MCastFilterCount = 0;
  ZeroMem( Private->Mode.MCastFilter, sizeof( Private->Mode.MCastFilter ) );

  if ( Private->Snp.WaitForPacket != NULL )
  {
    gBS->CloseEvent( Private->Snp.WaitForPacket );
    Private->Snp.WaitForPacket = NULL;
  }

  if ( Private->BpfFd != 0 )
  {
    Private->UnixThunk->Close( Private->BpfFd );
    Private->BpfFd = 0;
  }

  if ( Private->ReadBuffer != NULL )
  {
    FreePool( Private->ReadBuffer );
    Private->CurrentReadPointer = Private->EndReadPointer = Private->ReadBuffer = NULL;
  }

  return( Success );
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
UnixSnpReceiveFilters(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  IN UINT32              EnableBits,
  IN UINT32              DisableBits,
  IN BOOLEAN              ResetMcastFilter,
  IN UINTN              McastFilterCount OPTIONAL,
  IN EFI_MAC_ADDRESS*          McastFilter OPTIONAL
  )
{
  UNIX_SNP_PRIVATE_DATA*        Private;

  Private = UNIX_SNP_PRIVATE_DATA_FROM_SNP_THIS( This );

//  ReturnValue = GlobalData->NtNetUtilityTable.SetReceiveFilter (
//                                                Instance->InterfaceInfo.InterfaceIndex,
//                                                EnableBits,
//                                                McastFilterCount,
//                                                McastFilter
//                                                );

  // For now, just succeed...
  return( EFI_SUCCESS );
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
UnixSnpStationAddress(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  IN BOOLEAN              Reset,
  IN EFI_MAC_ADDRESS*          NewMacAddr OPTIONAL
  )
{
  return( EFI_UNSUPPORTED );
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
UnixSnpStatistics(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  IN BOOLEAN              Reset,
  IN OUT UINTN*            StatisticsSize OPTIONAL,
  OUT EFI_NETWORK_STATISTICS*      StatisticsTable OPTIONAL
  )
{
  return( EFI_UNSUPPORTED );
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
UnixSnpMcastIptoMac(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  IN BOOLEAN              Ipv6,
  IN EFI_IP_ADDRESS*          Ip,
  OUT EFI_MAC_ADDRESS*        Mac
  )
{
  return( EFI_UNSUPPORTED );
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
UnixSnpNvdata(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  IN BOOLEAN              ReadOrWrite,
  IN UINTN              Offset,
  IN UINTN              BufferSize,
  IN OUT VOID*            Buffer
  )
{
  return( EFI_UNSUPPORTED );
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
UnixSnpGetStatus(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  OUT UINT32*              InterruptStatus,
  OUT VOID**              TxBuffer
  )
{
  if ( TxBuffer != NULL )
  {
    *( ( UINT8 ** ) TxBuffer ) = ( UINT8 * ) 1;
  }

  if ( InterruptStatus != NULL )
  {
    *InterruptStatus = EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT;
  }

  return( EFI_SUCCESS );
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
UnixSnpTransmit(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  IN UINTN              HeaderSize,
  IN UINTN              BufferSize,
  IN VOID*              Buffer,
  IN EFI_MAC_ADDRESS*          SrcAddr OPTIONAL,
  IN EFI_MAC_ADDRESS*          DestAddr OPTIONAL,
  IN UINT16*              Protocol OPTIONAL
  )
{
  UNIX_SNP_PRIVATE_DATA*        Private;
  EthernetHeader*            EnetHeader;
  INTN                Result;

  Private = UNIX_SNP_PRIVATE_DATA_FROM_SNP_THIS( This );

  if ( This->Mode->State < EfiSimpleNetworkStarted )
  {
    return( EFI_NOT_STARTED );
  }

  if ( HeaderSize != 0 )
  {
    if ( ( DestAddr == NULL ) || ( Protocol == NULL ) || ( HeaderSize != This->Mode->MediaHeaderSize ) )
    {
      return( EFI_INVALID_PARAMETER );
    }

    if ( SrcAddr == NULL )
    {
      SrcAddr = &This->Mode->CurrentAddress;
    }

    EnetHeader = ( EthernetHeader * ) Buffer;

    CopyMem( EnetHeader->DstAddr, DestAddr, NET_ETHER_ADDR_LEN );
    CopyMem( EnetHeader->SrcAddr, SrcAddr, NET_ETHER_ADDR_LEN );

    EnetHeader->Type = HTONS( *Protocol );
  }

  Result = Private->UnixThunk->Write( Private->BpfFd, Buffer, BufferSize );

  if ( Result < 0 )
  {
    return( EFI_DEVICE_ERROR );
  }
  else
  {
    return( EFI_SUCCESS );
  }
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
UnixSnpReceive(
  IN EFI_SIMPLE_NETWORK_PROTOCOL*    This,
  OUT UINTN*              HeaderSize OPTIONAL,
  IN OUT UINTN*            BuffSize,
  OUT VOID*              Buffer,
  OUT EFI_MAC_ADDRESS*        SourceAddr OPTIONAL,
  OUT EFI_MAC_ADDRESS*        DestinationAddr OPTIONAL,
  OUT UINT16*              Protocol OPTIONAL
  )
{
  UNIX_SNP_PRIVATE_DATA*        Private;
  struct bpf_hdr*            BpfHeader;
  EthernetHeader*            EnetHeader;
  EFI_STATUS              Status = EFI_SUCCESS;
  INTN                Result;

  if ( This->Mode->State < EfiSimpleNetworkStarted )
  {
    return( EFI_NOT_STARTED );
  }

  Private = UNIX_SNP_PRIVATE_DATA_FROM_SNP_THIS( This );

  //
  // Do we have any remaining packets from the previous read?
  //
  if ( Private->CurrentReadPointer >= Private->EndReadPointer )
  {
    Result = Private->UnixThunk->Read( Private->BpfFd, Private->ReadBuffer, Private->ReadBufferSize );

    if ( Result < 0 )
    {
      Result = Private->UnixThunk->GetErrno();

      //
      // EAGAIN means that there's no I/O outstanding against this file descriptor.
      //
      if ( Result == EAGAIN )
      {
        return( EFI_NOT_READY );
      }
      else
      {
        return( EFI_DEVICE_ERROR );
      }
    }

    if ( Result == 0 )
    {
      return( EFI_NOT_READY );
    }

    Private->CurrentReadPointer = Private->ReadBuffer;
    Private->EndReadPointer = Private->CurrentReadPointer + Result;
  }

  BpfHeader = Private->CurrentReadPointer;
  EnetHeader = Private->CurrentReadPointer + BpfHeader->bh_hdrlen;

  if ( BpfHeader->bh_caplen > *BuffSize )
  {
    *BuffSize = BpfHeader->bh_caplen;
    return( EFI_BUFFER_TOO_SMALL );
  }

  CopyMem( Buffer, EnetHeader, BpfHeader->bh_caplen );
  *BuffSize = BpfHeader->bh_caplen;

  if ( HeaderSize != NULL )
  {
    *HeaderSize = sizeof( EthernetHeader );
  }

  if ( DestinationAddr != NULL )
  {
    ZeroMem( DestinationAddr, sizeof( EFI_MAC_ADDRESS ) );
    CopyMem( DestinationAddr, EnetHeader->DstAddr, NET_ETHER_ADDR_LEN );
  }

  if ( SourceAddr != NULL )
  {
    ZeroMem( SourceAddr, sizeof( EFI_MAC_ADDRESS ) );
    CopyMem( SourceAddr, EnetHeader->SrcAddr, NET_ETHER_ADDR_LEN );
  }

  if ( Protocol != NULL )
  {
    *Protocol = NTOHS( EnetHeader->Type );
  }

  Private->CurrentReadPointer += BPF_WORDALIGN( BpfHeader->bh_hdrlen + BpfHeader->bh_caplen );

  return( Status );
}


VOID
EFIAPI
UnixSnpWaitForPacketNotify(
  IN EFI_EVENT            Event,
  IN VOID*              Context
  )
{
  UNIX_SNP_PRIVATE_DATA*        Private;

  Private = UNIX_SNP_PRIVATE_DATA_FROM_SNP_THIS( Context );

  if ( Private->Snp.Mode->State < EfiSimpleNetworkStarted )
  {
    return;
  }
}


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
InitializeUnixSnpDriver(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE*    SystemTable
  )
{
  EFI_STATUS            Status;

  //
  // Install the Driver Protocols
  //

  Status = EfiLibInstallDriverBindingComponentName2(
          ImageHandle,
          SystemTable,
          &gUnixSnpDriverBinding,
          ImageHandle,
          &gUnixSnpDriverComponentName,
          &gUnixSnpDriverComponentName2
          );

  return( Status );
}
