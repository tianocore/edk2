/** @file

Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  SnpNt32.c

Abstract:

-**/

#include "SnpNt32.h"

EFI_DRIVER_BINDING_PROTOCOL gSnpNt32DriverBinding = {
  SnpNt32DriverBindingSupported,
  SnpNt32DriverBindingStart,
  SnpNt32DriverBindingStop,
  0xa,
  NULL,
  NULL
};

SNPNT32_GLOBAL_DATA         gSnpNt32GlobalData = {
  SNP_NT32_DRIVER_SIGNATURE,  //  Signature
  {
    NULL,
    NULL
  },                          //  InstanceList
  NULL,                       //  WinNtThunk
  NULL,                       //  NetworkLibraryHandle
  {
    0
  },                          //  NtNetUtilityTable
  {
    0,
    0,
    EfiLockUninitialized
  },                          //  Lock
  //
  //  Private functions
  //
  SnpNt32InitializeGlobalData,            //  InitializeGlobalData
  SnpNt32InitializeInstanceData,          //  InitializeInstanceData
  SnpNt32CloseInstance                    //  CloseInstance
};

/**
  Changes the state of a network interface from "stopped" to "started".

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
SnpNt32Start (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
  );
  
/**
  Changes the state of a network interface from "started" to "stopped".

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
SnpNt32Stop (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
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
SnpNt32Initialize (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINTN                       ExtraRxBufferSize OPTIONAL,
  IN UINTN                       ExtraTxBufferSize OPTIONAL
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
SnpNt32Reset (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                      ExtendedVerification
  );

/**
  Resets a network adapter and leaves it in a state that is safe for 
  another driver to initialize.

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
SnpNt32Shutdown (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
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
SnpNt32ReceiveFilters (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINT32                      EnableBits,
  IN UINT32                      DisableBits,
  IN BOOLEAN                     ResetMcastFilter,
  IN UINTN                       McastFilterCount OPTIONAL,
  IN EFI_MAC_ADDRESS             *McastFilter     OPTIONAL
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
SnpNt32StationAddress (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     Reset,
  IN EFI_MAC_ADDRESS             *NewMacAddr OPTIONAL
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
SnpNt32Statistics (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  * This,
  IN BOOLEAN                      Reset,
  IN OUT UINTN                    *StatisticsSize OPTIONAL,
  OUT EFI_NETWORK_STATISTICS      *StatisticsTable OPTIONAL
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
SnpNt32McastIptoMac (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     Ipv6,
  IN EFI_IP_ADDRESS              *Ip,
  OUT EFI_MAC_ADDRESS            *Mac
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
SnpNt32Nvdata (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     ReadOrWrite,
  IN UINTN                       Offset,
  IN UINTN                       BufferSize,
  IN OUT VOID                    *Buffer
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
SnpNt32GetStatus (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  OUT UINT32                     *InterruptStatus,
  OUT VOID                       **TxBuffer
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
SnpNt32Transmit (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINTN                       HeaderSize,
  IN UINTN                       BufferSize,
  IN VOID                        *Buffer,
  IN EFI_MAC_ADDRESS             *SrcAddr OPTIONAL,
  IN EFI_MAC_ADDRESS             *DestAddr OPTIONAL,
  IN UINT16                      *Protocol OPTIONAL
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
SnpNt32Receive (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  OUT UINTN                      *HeaderSize,
  IN OUT UINTN                   *BuffSize,
  OUT VOID                       *Buffer,
  OUT EFI_MAC_ADDRESS            *SourceAddr,
  OUT EFI_MAC_ADDRESS            *DestinationAddr,
  OUT UINT16                     *Protocol
  );

SNPNT32_INSTANCE_DATA gSnpNt32InstanceTemplate = {
  SNP_NT32_INSTANCE_SIGNATURE,            //  Signature
  {
    NULL,
    NULL
  },                                      //  Entry
  NULL,                                   //  GlobalData
  NULL,                                   //  DeviceHandle
  NULL,                                   //  DevicePath
  {                                       //  Snp
    EFI_SIMPLE_NETWORK_PROTOCOL_REVISION, //  Revision
    SnpNt32Start,                         //  Start
    SnpNt32Stop,                          //  Stop
    SnpNt32Initialize,                    //  Initialize
    SnpNt32Reset,                         //  Reset
    SnpNt32Shutdown,                      //  Shutdown
    SnpNt32ReceiveFilters,                //  ReceiveFilters
    SnpNt32StationAddress,                //  StationAddress
    SnpNt32Statistics,                    //  Statistics
    SnpNt32McastIptoMac,                  //  MCastIpToMac
    SnpNt32Nvdata,                        //  NvData
    SnpNt32GetStatus,                     //  GetStatus
    SnpNt32Transmit,                      //  Transmit
    SnpNt32Receive,                       //  Receive
    NULL,                                 //  WaitForPacket
    NULL                                  //  Mode
  },
  {                                       //  Mode
    EfiSimpleNetworkInitialized,          //  State
    NET_ETHER_ADDR_LEN,                   //  HwAddressSize
    NET_ETHER_HEADER_SIZE,                //  MediaHeaderSize
    1500,                                 //  MaxPacketSize
    0,                                    //  NvRamSize
    0,                                    //  NvRamAccessSize
    0,                                    //  ReceiveFilterMask
    0,                                    //  ReceiveFilterSetting
    MAX_MCAST_FILTER_CNT,                 //  MaxMCastFilterCount
    0,                                    //  MCastFilterCount
    {
      0
    },                                    //  MCastFilter
    {
      0
    },                                    //  CurrentAddress
    {
      0
    },                                    //  BroadcastAddress
    {
      0
    },                                    //  PermanentAddress
    NET_IFTYPE_ETHERNET,                  //  IfType
    FALSE,                                //  MacAddressChangeable
    FALSE,                                //  MultipleTxSupported
    TRUE,                                 //  MediaPresentSupported
    TRUE                                  //  MediaPresent
  },
  {
    0
  }                                       //  InterfaceInfo
};

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
SnpNt32DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * RemainingDevicePath OPTIONAL
  )
{

  SNPNT32_GLOBAL_DATA   *GlobalData;
  LIST_ENTRY            *Entry;
  SNPNT32_INSTANCE_DATA *Instance;

  GlobalData = &gSnpNt32GlobalData;

  NET_LIST_FOR_EACH (Entry, &GlobalData->InstanceList) {

    Instance = NET_LIST_USER_STRUCT_S (Entry, SNPNT32_INSTANCE_DATA, Entry, SNP_NT32_INSTANCE_SIGNATURE);

    if (Instance->DeviceHandle == ControllerHandle) {
      return EFI_SUCCESS;
    }

  }

  return EFI_UNSUPPORTED;
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
SnpNt32DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * RemainingDevicePath OPTIONAL
  )
{
  return EFI_SUCCESS;
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
SnpNt32DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  return EFI_SUCCESS;
}


/**
  Changes the state of a network interface from "stopped" to "started".

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
SnpNt32Start (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
  )
{
  return EFI_SUCCESS;
}


/**
  Changes the state of a network interface from "started" to "stopped".

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
SnpNt32Stop (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
  )
{
  return EFI_SUCCESS;
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
SnpNt32Initialize (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINTN                       ExtraRxBufferSize OPTIONAL,
  IN UINTN                       ExtraTxBufferSize OPTIONAL
  )
{
  return EFI_SUCCESS;
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
SnpNt32Reset (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                      ExtendedVerification
  )
{
  return EFI_SUCCESS;
}

/**
  Resets a network adapter and leaves it in a state that is safe for 
  another driver to initialize.

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           Always succeeds.

**/
EFI_STATUS
EFIAPI
SnpNt32Shutdown (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
  )
{
  return EFI_SUCCESS;
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
SnpNt32ReceiveFilters (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINT32                      EnableBits,
  IN UINT32                      DisableBits,
  IN BOOLEAN                     ResetMcastFilter,
  IN UINTN                       McastFilterCount OPTIONAL,
  IN EFI_MAC_ADDRESS             *McastFilter     OPTIONAL
  )
{
  SNPNT32_INSTANCE_DATA *Instance;
  SNPNT32_GLOBAL_DATA   *GlobalData;
  INT32                 ReturnValue;

  Instance    = SNP_NT32_INSTANCE_DATA_FROM_SNP_THIS (This);

  GlobalData  = Instance->GlobalData;

  if (EFI_ERROR (EfiAcquireLockOrFail (&GlobalData->Lock))) {
    return EFI_ACCESS_DENIED;
  }

  ReturnValue = GlobalData->NtNetUtilityTable.SetReceiveFilter (
                                                Instance->InterfaceInfo.InterfaceIndex,
                                                EnableBits,
                                                (UINT32)McastFilterCount,
                                                McastFilter
                                                );

  EfiReleaseLock (&GlobalData->Lock);

  if (ReturnValue <= 0) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
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
SnpNt32StationAddress (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     Reset,
  IN EFI_MAC_ADDRESS             *NewMacAddr OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
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
SnpNt32Statistics (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  * This,
  IN BOOLEAN                      Reset,
  IN OUT UINTN                    *StatisticsSize OPTIONAL,
  OUT EFI_NETWORK_STATISTICS      *StatisticsTable OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
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
SnpNt32McastIptoMac (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     Ipv6,
  IN EFI_IP_ADDRESS              *Ip,
  OUT EFI_MAC_ADDRESS            *Mac
  )
{
  return EFI_UNSUPPORTED;
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
SnpNt32Nvdata (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     ReadOrWrite,
  IN UINTN                       Offset,
  IN UINTN                       BufferSize,
  IN OUT VOID                    *Buffer
  )
{
  return EFI_UNSUPPORTED;
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
SnpNt32GetStatus (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  OUT UINT32                     *InterruptStatus,
  OUT VOID                       **TxBuffer
  )
{

  if (TxBuffer != NULL) {
    *((UINT8 **) TxBuffer) = (UINT8 *)(UINTN) 1;
  }

  if (InterruptStatus != NULL) {
    *InterruptStatus = EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT;
  }

  return EFI_SUCCESS;
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
  @retval EFI_ACCESS_DENIED     Error acquire global lock for operation.

**/
EFI_STATUS
EFIAPI
SnpNt32Transmit (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINTN                       HeaderSize,
  IN UINTN                       BufferSize,
  IN VOID                        *Buffer,
  IN EFI_MAC_ADDRESS             *SrcAddr OPTIONAL,
  IN EFI_MAC_ADDRESS             *DestAddr OPTIONAL,
  IN UINT16                      *Protocol OPTIONAL
  )
{
  SNPNT32_INSTANCE_DATA *Instance;
  SNPNT32_GLOBAL_DATA   *GlobalData;
  INT32                 ReturnValue;

  Instance    = SNP_NT32_INSTANCE_DATA_FROM_SNP_THIS (This);

  GlobalData  = Instance->GlobalData;

  if ((HeaderSize != 0) && (SrcAddr == NULL)) {
    SrcAddr = &Instance->Mode.CurrentAddress;
  }

  if (EFI_ERROR (EfiAcquireLockOrFail (&GlobalData->Lock))) {
    return EFI_ACCESS_DENIED;
  }

  ReturnValue = GlobalData->NtNetUtilityTable.Transmit (
                                                Instance->InterfaceInfo.InterfaceIndex,
                                                (UINT32)HeaderSize,
                                                (UINT32)BufferSize,
                                                Buffer,
                                                SrcAddr,
                                                DestAddr,
                                                Protocol
                                                );

  EfiReleaseLock (&GlobalData->Lock);

  if (ReturnValue < 0) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
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
SnpNt32Receive (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  OUT UINTN                      *HeaderSize,
  IN OUT UINTN                   *BuffSize,
  OUT VOID                       *Buffer,
  OUT EFI_MAC_ADDRESS            *SourceAddr,
  OUT EFI_MAC_ADDRESS            *DestinationAddr,
  OUT UINT16                     *Protocol
  )
{
  SNPNT32_INSTANCE_DATA *Instance;
  SNPNT32_GLOBAL_DATA   *GlobalData;
  INT32                 ReturnValue;
  UINTN                 BufSize;
  
  BufSize     = *BuffSize;

  Instance    = SNP_NT32_INSTANCE_DATA_FROM_SNP_THIS (This);

  GlobalData  = Instance->GlobalData;

  ASSERT (GlobalData->NtNetUtilityTable.Receive != NULL);

  if (EFI_ERROR (EfiAcquireLockOrFail (&GlobalData->Lock))) {
    return EFI_ACCESS_DENIED;
  }

  ReturnValue = GlobalData->NtNetUtilityTable.Receive (
                                                Instance->InterfaceInfo.InterfaceIndex,
                                                BuffSize,
                                                Buffer
                                                );

  EfiReleaseLock (&GlobalData->Lock);

  if (ReturnValue < 0) {
    if (ReturnValue == -100) {
      return EFI_BUFFER_TOO_SMALL;
    }

    return EFI_DEVICE_ERROR;
  } else if (ReturnValue == 0) {
    return EFI_NOT_READY;
  }

  if (HeaderSize != NULL) {
    *HeaderSize = 14;
  }

  if (SourceAddr != NULL) {
    ZeroMem (SourceAddr, sizeof (EFI_MAC_ADDRESS));
    CopyMem (SourceAddr, ((UINT8 *) Buffer) + 6, 6);
  }

  if (DestinationAddr != NULL) {
    ZeroMem (DestinationAddr, sizeof (EFI_MAC_ADDRESS));
    CopyMem (DestinationAddr, ((UINT8 *) Buffer), 6);
  }

  if (Protocol != NULL) {
    *Protocol = NTOHS (*((UINT16 *) (((UINT8 *) Buffer) + 12)));
  }

  return (*BuffSize <= BufSize) ? EFI_SUCCESS : EFI_BUFFER_TOO_SMALL; 
}

/**
  Initialize the driver's global data.

  @param  This                  Pointer to the global context data.

  @retval EFI_SUCCESS           The global data is initialized.
  @retval EFI_NOT_FOUND         The required DLL is not found.
  @retval EFI_DEVICE_ERROR      Error initialize network utility library.
  @retval EFI_OUT_OF_RESOURCES  Out of resource.
  @retval other                 Other errors.

**/
EFI_STATUS
SnpNt32InitializeGlobalData (
  IN OUT SNPNT32_GLOBAL_DATA *This
  )
{
  EFI_STATUS            Status;
  CHAR16                *DllFileNameU;
  UINT32                Index;
  INT32                 ReturnValue;
  BOOLEAN               NetUtilityLibInitDone;
  NT_NET_INTERFACE_INFO NetInterfaceInfoBuffer[MAX_INTERFACE_INFO_NUMBER];
  SNPNT32_INSTANCE_DATA *Instance;
  LIST_ENTRY            *Entry;
  UINT32                InterfaceCount;

  ASSERT (This != NULL);

  NetUtilityLibInitDone = FALSE;
  InterfaceCount        = MAX_INTERFACE_INFO_NUMBER;

  InitializeListHead (&This->InstanceList);
  EfiInitializeLock (&This->Lock, TPL_CALLBACK);

  //
  //  Get the WinNT thunk
  //
  Status = gBS->LocateProtocol (&gEfiWinNtThunkProtocolGuid, NULL, (VOID **)&This->WinNtThunk);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (This->WinNtThunk != NULL);

  DllFileNameU = NETWORK_LIBRARY_NAME_U;

  //
  //  Load network utility library
  //
  This->NetworkLibraryHandle = This->WinNtThunk->LoadLibraryEx (DllFileNameU, NULL, 0);

  if (NULL == This->NetworkLibraryHandle) {
    return EFI_NOT_FOUND;
  }

  This->NtNetUtilityTable.Initialize = (NT_NET_INITIALIZE) This->WinNtThunk->GetProcAddress (
                                                                               This->NetworkLibraryHandle,
                                                                               NETWORK_LIBRARY_INITIALIZE
                                                                               );

  if (NULL == This->NtNetUtilityTable.Initialize) {
    Status = EFI_NOT_FOUND;
    goto ErrorReturn;
  }

  This->NtNetUtilityTable.Finalize = (NT_NET_FINALIZE) This->WinNtThunk->GetProcAddress (
                                                                           This->NetworkLibraryHandle,
                                                                           NETWORK_LIBRARY_FINALIZE
                                                                           );

  if (NULL == This->NtNetUtilityTable.Finalize) {
    Status = EFI_NOT_FOUND;
    goto ErrorReturn;
  }

  This->NtNetUtilityTable.SetReceiveFilter = (NT_NET_SET_RECEIVE_FILTER) This->WinNtThunk->GetProcAddress (
                                                                                             This->NetworkLibraryHandle,
                                                                                             NETWORK_LIBRARY_SET_RCV_FILTER
                                                                                             );

  if (NULL == This->NtNetUtilityTable.SetReceiveFilter) {
    Status = EFI_NOT_FOUND;
    goto ErrorReturn;
  }

  This->NtNetUtilityTable.Receive = (NT_NET_RECEIVE) This->WinNtThunk->GetProcAddress (
                                                                         This->NetworkLibraryHandle,
                                                                         NETWORK_LIBRARY_RECEIVE
                                                                         );

  if (NULL == This->NtNetUtilityTable.Receive) {
    Status = EFI_NOT_FOUND;
    goto ErrorReturn;
  }

  This->NtNetUtilityTable.Transmit = (NT_NET_TRANSMIT) This->WinNtThunk->GetProcAddress (
                                                                           This->NetworkLibraryHandle,
                                                                           NETWORK_LIBRARY_TRANSMIT
                                                                           );

  if (NULL == This->NtNetUtilityTable.Transmit) {
    Status = EFI_NOT_FOUND;
    goto ErrorReturn;
  }
  //
  //  Initialize the network utility library
  //  And enumerate the interfaces in NT32 host
  //
  ReturnValue = This->NtNetUtilityTable.Initialize (&InterfaceCount, &NetInterfaceInfoBuffer[0]);
  if (ReturnValue <= 0) {
    Status = EFI_DEVICE_ERROR;
    goto ErrorReturn;
  }

  NetUtilityLibInitDone = TRUE;

  if (InterfaceCount == 0) {
    Status = EFI_NOT_FOUND;
    goto ErrorReturn;
  }
  //
  //  Create fake SNP instances
  //
  for (Index = 0; Index < InterfaceCount; Index++) {

    Instance = AllocatePool (sizeof (SNPNT32_INSTANCE_DATA));

    if (NULL == Instance) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorReturn;
    }
    //
    //  Copy the content from a template
    //
    CopyMem (Instance, &gSnpNt32InstanceTemplate, sizeof (SNPNT32_INSTANCE_DATA));

    //
    //  Set the interface information.
    //
    CopyMem (&Instance->InterfaceInfo, &NetInterfaceInfoBuffer[Index], sizeof(Instance->InterfaceInfo));
    //
    //  Initialize this instance
    //
    Status = This->InitializeInstanceData (This, Instance);
    if (EFI_ERROR (Status)) {

      gBS->FreePool (Instance);
      goto ErrorReturn;
    }
    //
    //  Insert this instance into the instance list
    //
    InsertTailList (&This->InstanceList, &Instance->Entry);
  }

  return EFI_SUCCESS;

ErrorReturn:

  while (!IsListEmpty (&This->InstanceList)) {

    Entry     = This->InstanceList.ForwardLink;

    Instance  = NET_LIST_USER_STRUCT_S (Entry, SNPNT32_INSTANCE_DATA, Entry, SNP_NT32_INSTANCE_SIGNATURE);

    RemoveEntryList (Entry);

    This->CloseInstance (This, Instance);
    gBS->FreePool (Instance);
  }

  if (NetUtilityLibInitDone) {

    ASSERT (This->WinNtThunk != NULL);

    if (This->NtNetUtilityTable.Finalize != NULL) {
      This->NtNetUtilityTable.Finalize ();
      This->NtNetUtilityTable.Finalize = NULL;
    }
  }

  return Status;
}


/**
  Initialize the snpnt32 driver instance.

  @param  This                  Pointer to the SnpNt32 global data.
  @param  Instance              Pointer to the instance context data.

  @retval EFI_SUCCESS           The driver instance is initialized.
  @retval other                 Initialization errors.

**/
EFI_STATUS
SnpNt32InitializeInstanceData (
  IN SNPNT32_GLOBAL_DATA        *This,
  IN OUT SNPNT32_INSTANCE_DATA  *Instance
  )
{
  EFI_STATUS    Status;
  EFI_DEV_PATH  EndNode;
  EFI_DEV_PATH  Node;

  Instance->GlobalData  = This;
  Instance->Snp.Mode    = &Instance->Mode;
  //
  //  Set broadcast address
  //
  SetMem (&Instance->Mode.BroadcastAddress, sizeof (EFI_MAC_ADDRESS), 0xFF);

  //
  //  Copy Current/PermanentAddress MAC address
  //
  CopyMem (&Instance->Mode.CurrentAddress, &Instance->InterfaceInfo.MacAddr, sizeof(Instance->Mode.CurrentAddress));
  CopyMem (&Instance->Mode.PermanentAddress, &Instance->InterfaceInfo.MacAddr, sizeof(Instance->Mode.PermanentAddress));

  //
  //  Since the fake SNP is based on a real NIC, to avoid conflict with the host
  //  NIC network stack, we use a different MAC address.
  //  So just change the last byte of the MAC address for the real NIC.
  //
  Instance->Mode.CurrentAddress.Addr[NET_ETHER_ADDR_LEN - 1]++;

  //
  //  Create a fake device path for the instance
  //
  ZeroMem (&Node, sizeof (Node));

  Node.DevPath.Type     = MESSAGING_DEVICE_PATH;
  Node.DevPath.SubType  = MSG_MAC_ADDR_DP;
  SetDevicePathNodeLength (&Node.DevPath, sizeof (MAC_ADDR_DEVICE_PATH));

  CopyMem (
    &Node.MacAddr.MacAddress,
    &Instance->Mode.CurrentAddress,
    NET_ETHER_ADDR_LEN
    );

  Node.MacAddr.IfType = Instance->Mode.IfType;

  SetDevicePathEndNode (&EndNode.DevPath);

  Instance->DevicePath = AppendDevicePathNode (
                           &EndNode.DevPath,
                           &Node.DevPath
                           );

  //
  //  Create a fake device handle for the fake SNP
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Instance->DeviceHandle,
                  &gEfiSimpleNetworkProtocolGuid,
                  &Instance->Snp,
                  &gEfiDevicePathProtocolGuid,
                  Instance->DevicePath,
                  NULL
                  );
  return Status;
}


/**
  Close the SnpNt32 driver instance.

  @param  This                  Pointer to the SnpNt32 global data.
  @param  Instance              Pointer to the instance context data.

  @retval EFI_SUCCESS           The instance is closed.

**/
EFI_STATUS
SnpNt32CloseInstance (
  IN SNPNT32_GLOBAL_DATA        *This,
  IN OUT SNPNT32_INSTANCE_DATA  *Instance
  )
{
  ASSERT (This != NULL);
  ASSERT (Instance != NULL);

  gBS->UninstallMultipleProtocolInterfaces (
         Instance->DeviceHandle,
         &gEfiSimpleNetworkProtocolGuid,
         &Instance->Snp,
         &gEfiDevicePathProtocolGuid,
         Instance->DevicePath,
         NULL
         );

  if (Instance->DevicePath != NULL) {
    gBS->FreePool (Instance->DevicePath);
  }

  return EFI_SUCCESS;
}

/**
  Unloads an image.

  @param  ImageHandle           Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
  @return Exit code from the image's unload handler

**/
EFI_STATUS
EFIAPI
SnpNt32Unload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS            Status;
  SNPNT32_GLOBAL_DATA   *This;
  LIST_ENTRY            *Entry;
  SNPNT32_INSTANCE_DATA *Instance;

  This    = &gSnpNt32GlobalData;

  Status  = NetLibDefaultUnload (ImageHandle);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (!IsListEmpty (&This->InstanceList)) {
    //
    //  Walkthrough the interfaces and remove all the SNP instance
    //
    Entry     = This->InstanceList.ForwardLink;

    Instance  = NET_LIST_USER_STRUCT_S (Entry, SNPNT32_INSTANCE_DATA, Entry, SNP_NT32_INSTANCE_SIGNATURE);

    RemoveEntryList (Entry);

    This->CloseInstance (This, Instance);
    gBS->FreePool (Instance);
  }

  if (This->NtNetUtilityTable.Finalize != NULL) {
    This->NtNetUtilityTable.Finalize ();
  }

  This->WinNtThunk->FreeLibrary (This->NetworkLibraryHandle);

  return EFI_SUCCESS;
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
InitializeSnpNt32Driver (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{

  EFI_STATUS  Status;

  //
  // Install the Driver Protocols
  //

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gSnpNt32DriverBinding,
             ImageHandle,
             &gSnpNt32DriverComponentName,
             &gSnpNt32DriverComponentName2
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  //  Initialize the global data
  //
  Status = SnpNt32InitializeGlobalData (&gSnpNt32GlobalData);
  if (EFI_ERROR (Status)) {
    SnpNt32Unload (ImageHandle);
  }

  return Status;
}
