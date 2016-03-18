/** @file 
  Wrapper routines that use a PXE-enabled NIC option ROM to 
  supply internal routines for an EFI SNI (Simple Network 
  Interface) Protocol.

  This file relies upon the existence of a PXE-compliant ROM
  in memory, as defined by the Preboot Execution Environment 
  Specification (PXE), Version 2.1, located at

  http://developer.intel.com/ial/wfm/wfmspecs.htm

Copyright (c) 1999 - 2010, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BiosSnp16.h"

/**
  PXE 
  START UNDI
  Op-Code: PXENV_START_UNDI (0000h)
  Input: Far pointer to a PXENV_START_UNDI_T parameter structure that has been initialized by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This service is used to pass the BIOS parameter registers to the UNDI driver. The UNDI driver is
  responsible for saving the information it needs to communicate with the hardware.
  This service is also responsible for hooking the Int 1Ah service routine
  Note: This API service must be called only once during UNDI Option ROM boot.
  The UNDI driver is responsible for saving this information and using it every time
  PXENV_UNDI_STARTUP is called.
  Service cannot be used in protected mode.
  typedef struct  {
      PXENV_STATUS Status;
      UINT16 AX;
      UINT16 BX;
      UINT16 DX;
      UINT16 DI;
      UINT16 ES;
  } PXENV_START_UNDI_T;
  Set before calling API service
  AX, BX, DX, DI, ES: BIOS initialization parameter registers. These
  fields should contain the same information passed to the option ROM
  initialization routine by the Host System BIOS. Information about the
  contents of these registers can be found in the [PnP], [PCI] and
  [BBS] specifications.
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.    

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                                
  @return Return value of PXE option ROM far call.                                
**/
EFI_STATUS
PxeStartUndi (
  IN     EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice,
  IN OUT PXENV_START_UNDI_T      *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_START_UNDI_T),
          PXENV_START_UNDI
          );
}

/**
  PXE 
  UNDI STARTUP    
  Op-Code: PXENV_UNDI_STARTUP (0001h)
  Input: Far pointer to a PXENV_UNDI_STARTUP_T parameter structure that has been initialized by the
  caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the
  PXENV_STATUS_xxx constants.
  Description: This API is responsible for initializing the contents of the UNDI code & data segment for proper
  operation. Information from the !PXE structure and the first PXENV_START_UNDI API call is used
  to complete this initialization. The rest of the UNDI APIs will not be available until this call has
  been completed.
  Note: PXENV_UNDI_STARTUP must not be called again without first calling
  PXENV_UNDI_SHUTDOWN.
  PXENV_UNDI_STARTUP and PXENV_UNDI_SHUTDOWN are no longer responsible for
  chaining interrupt 1Ah. This must be done by the PXENV_START_UNDI and
  PXENV_STOP_UNDI API calls.
  This service cannot be used in protected mode.
  typedef struct 
  {
      PXENV_STATUS Status;
  } PXENV_UNDI_STARTUP_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                                
  @return Return value of PXE option ROM far call.    
**/
EFI_STATUS
PxeUndiStartup (
  IN     EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_STARTUP_T    *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_STARTUP_T),
          PXENV_UNDI_STARTUP
          );
}

/**
  PXE 
  UNDI CLEANUP
  Op-Code: PXENV_UNDI_CLEANUP (0002h)
  Input: Far pointer to a PXENV_UNDI_CLEANUP_T parameter structure.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field
  in the parameter structure must be set to one of the values represented by the
  PXENV_STATUS_xxx constants.
  Description: This call will prepare the network adapter driver to be unloaded from memory. This call must be
  made just before unloading the Universal NIC Driver. The rest of the API will not be available
  after this call executes.
  This service cannot be used in protected mode.
  typedef struct {
      PXENX_STATUS Status;
  } PXENV_UNDI_CLEANUP_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                                
  @return Return value of PXE option ROM far call. 
**/
EFI_STATUS
PxeUndiCleanup (
  IN     EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_CLEANUP_T    *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_CLEANUP_T),
          PXENV_UNDI_CLEANUP
          );
}

/**
  PXE 
  UNDI INITIALIZE
  Op-Code: PXENV_UNDI_INITIALIZE (0003h)
  Input: Far pointer to a PXENV_UNDI_INITIALIZE_T parameter structure that has been initialized by the
  caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call resets the adapter and programs it with default parameters. The default parameters used
  are those supplied to the most recent UNDI_STARTUP call. This routine does not enable the
  receive and transmit units of the network adapter to readily receive or transmit packets. The
  application must call PXENV_UNDI_OPEN to logically connect the network adapter to the network.
  This call must be made by an application to establish an interface to the network adapter driver.
  Note: When the PXE code makes this call to initialize the network adapter, it passes a NULL pointer for
  the Protocol field in the parameter structure.
  typedef struct {
    PXENV_STATUS Status;
    ADDR32 ProtocolIni;
    UINT8 reserved[8];
  } PXENV_UNDI_INITIALIZE_T;
  Set before calling API service
  ProtocolIni: Physical address of a memory copy of the driver
  module from the protocol.ini file obtained from the protocol manager
  driver (refer to the NDIS 2.0 specification). This parameter is
  supported for the universal NDIS driver to pass the information
  contained in the protocol.ini file to the NIC driver for any specific
  configuration of the NIC. (Note that the module identification in the
  protocol.ini file was done by NDIS.) This value can be NULL for any
  other application interfacing to the universal NIC driver
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.    
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                                
  @return Return value of PXE option ROM far call. 
**/
EFI_STATUS
PxeUndiInitialize (
  IN     EFI_SIMPLE_NETWORK_DEV   *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_INITIALIZE_T  *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_INITIALIZE_T),
          PXENV_UNDI_INITIALIZE
          );
}

/**
  Wrapper routine for reset adapter.
  
  PXE 
  UNDI RESET ADAPTER
  Op-Code: PXENV_UNDI_RESET_ADAPTER (0004h)
  Input: Far pointer to a PXENV_UNDI_RESET_ADAPTER_t parameter structure that has been initialized
  by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call resets and reinitializes the network adapter with the same set of parameters supplied to
  Initialize Routine. Unlike Initialize, this call opens the adapter that is, it connects logically to the
  network. This routine cannot be used to replace Initialize or Shutdown calls.
  typedef struct {
    PXENV_STATUS Status;
    PXENV_UNDI_MCAST_ADDRESS_t    R_Mcast_Buf;
  } PXENV_UNDI_RESET_T;

  #define MAXNUM_MCADDR 8

  typedef struct {
    UINT16 MCastAddrCount;
    MAC_ADDR McastAddr[MAXNUM_MCADDR];
  } PXENV_UNDI_MCAST_ADDRESS_t;

  Set before calling API service
  R_Mcast_Buf: This is a structure of MCastAddrCount and
  McastAddr.
  MCastAddrCount: Number of multicast MAC addresses in the
  buffer.
  McastAddr: List of up to MAXNUM_MCADDR multicast MAC
  addresses.
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.
  
  @param  SimpleNetworkDevice   Device instance.
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
  @param  RxFilter             Filter setting mask value for PXE recive .     
                               
  @return Return value of PXE option ROM far call. 
**/
EFI_STATUS
PxeUndiResetNic (
  IN     EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_RESET_T      *PxeUndiTable,
  IN     UINT16                  RxFilter
  )
{
  PXENV_UNDI_OPEN_T   Open;
  PXENV_UNDI_CLOSE_T  Close;
  UINTN               Status;

  Status = MakePxeCall (
            SimpleNetworkDevice,
            PxeUndiTable,
            sizeof (PXENV_UNDI_RESET_T),
            PXENV_UNDI_RESET_NIC
            );
  if (!EFI_ERROR(Status)) {
    return Status;
  }

  Close.Status = PXENV_STATUS_SUCCESS;

  Status = MakePxeCall (
            SimpleNetworkDevice,
            &Close,
            sizeof (Close),
            PXENV_UNDI_CLOSE
            );
  if (EFI_ERROR(Status)) {
    return EFI_DEVICE_ERROR;
  }

  Status = MakePxeCall (
            SimpleNetworkDevice,
            PxeUndiTable,
            sizeof (PXENV_UNDI_RESET_T),
            PXENV_UNDI_RESET_NIC
            );
  if (EFI_ERROR(Status)) {
    return EFI_DEVICE_ERROR;
  }

  Open.Status       = PXENV_STATUS_SUCCESS;
  Open.OpenFlag     = 0;
  Open.PktFilter    = RxFilter;
  CopyMem (
    &Open.McastBuffer,
    &PxeUndiTable->R_Mcast_Buf,
    sizeof (PXENV_UNDI_MCAST_ADDR_T)
    );      
  

  Status = MakePxeCall (
            SimpleNetworkDevice,
            &Open,
            sizeof (Open),
            PXENV_UNDI_OPEN
            );
  if (EFI_ERROR(Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  PXE 
  UNDI SHUTDOWN
  Op-Code: PXENV_UNDI_SHUTDOWN (0005h)
  Input: Far pointer to a PXENV_UNDI_SHUTDOWN_T parameter.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call resets the network adapter and leaves it in a safe state for another driver to program it.
  Note: The contents of the PXENV_UNDI_STARTUP parameter structure need to be saved by the
  Universal NIC Driver in case PXENV_UNDI_INITIALIZE is called again.
  typedef struct 
  {
    PXENV_STATUS Status;
  } PXENV_UNDI_SHUTDOWN_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                              
  @return Return value of PXE option ROM far call.   
**/
EFI_STATUS
PxeUndiShutdown (
  IN     EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_SHUTDOWN_T   *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_SHUTDOWN_T),
          PXENV_UNDI_SHUTDOWN
          );
}

/**
  PXE 
  UNDI OPEN
  Op-Code: PXENV_UNDI_OPEN (0006h)
  Input: Far pointer to a PXENV_UNDI_OPEN_T parameter structure that has been initialized by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call activates the adapter network connection and sets the adapter ready to accept packets
  for transmit and receive.
  typedef struct {
    PXENV_STATUS Status;
    UINT16 OpenFlag;
    UINT16 PktFilter;
      #define FLTR_DIRECTED 0x0001
      #define FLTR_BRDCST 0x0002
      #define FLTR_PRMSCS 0x0004
      #define FLTR_SRC_RTG 0x0008
    PXENV_UNDI_MCAST_ADDRESS_t R_Mcast_Buf;
  } PXENV_UNDI_OPEN_T;
  Set before calling API service
  OpenFlag: This is an adapter specific input parameter. This is
  supported for the universal NDIS 2.0 driver to pass in the open flags
  provided by the protocol driver. (See the NDIS 2.0 specification.)
  This can be zero.
  PktFilter: Filter for receiving packets. This can be one, or more, of
  the FLTR_xxx constants. Multiple values are arithmetically or-ed
  together.
  directed packets are packets that may come to your MAC address
  or the multicast MAC address.
  R_Mcast_Buf: See definition in UNDI RESET ADAPTER (0004h).
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                              
  @return Return value of PXE option ROM far call.  
**/
EFI_STATUS
PxeUndiOpen (
  IN     EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_OPEN_T       *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_OPEN_T),
          PXENV_UNDI_OPEN
          );
}

/**
  PXE 
  UNDI CLOSE
  Op-Code: PXENV_UNDI_CLOSE (0007h)
  Input: Far pointer to a PXENV_UNDI_CLOSE_T parameter.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call disconnects the network adapter from the network. Packets cannot be transmitted or
  received until the network adapter is open again.
  typedef struct {
    PXENV_STATUS Status;
  } PXENV_UNDI_CLOSE_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                              
  @return Return value of PXE option ROM far call.  
**/
EFI_STATUS
PxeUndiClose (
  IN     EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_CLOSE_T      *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_CLOSE_T),
          PXENV_UNDI_CLOSE
          );
}

/**
  PXE 
  UNDI TRANSMIT PACKET
  Op-Code: PXENV_UNDI_TRANSMIT (0008h)
  Input: Far pointer to a PXENV_UNDI_TRANSMIT_T parameter structure that
  has been initialized by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX.
  The status code must be set to one of the values represented by the
  PXENV_STATUS_xxx constants.
  Description: This call transmits a buffer to the network. The media header
  for the packet can be filled by the calling protocol, but it might not be.
  The network adapter driver will fill it if required by the values in the
  parameter block. The packet is buffered for transmission provided there is
  an available buffer, and the function returns PXENV_EXIT_SUCCESS. If no
  buffer is available the function returns PXENV_EXIT_FAILURE with a status
  code of PXE_UNDI_STATUS__OUT OF_RESOURCE. The number of buffers is
  implementation-dependent. An interrupt is generated on completion of the
  transmission of one or more packets. A call to PXENV_UNDI_TRANSMIT is
  permitted in the context of a transmit complete interrupt.

  typedef struct {
    PXENV_STATUS Status;
    UINT8 Protocol;
      #define P_UNKNOWN 0
      #define P_IP 1
      #define P_ARP 2
      #define P_RARP 3
    UINT8 XmitFlag;
      #define XMT_DESTADDR 0x0000
      #define XMT_BROADCAST 0x0001
    SEGOFF16 DestAddr;
    SEGOFF16 TBD;
    UINT32 Reserved[2];
  } t_PXENV_UNDI_TRANSMIT;

  #define MAX_DATA_BLKS 8

  typedef struct {
    UINT16 ImmedLength;
    SEGOFF16 Xmit;
    UINT16 DataBlkCount;
    struct DataBlk {
      UINT8 TDPtrType;
      UINT8 TDRsvdByte;
      UINT16 TDDataLen;
      SEGOFF16 TDDataPtr;
    } DataBlock[MAX_DATA_BLKS];
  } PXENV_UNDI_TBD_T

  Set before calling API service
  Protocol: This is the protocol of the upper layer that is calling UNDI
  TRANSMIT call. If the upper layer has filled the media header, this
  field must be P_UNKNOWN.
  XmitFlag: If this flag is XMT_DESTADDR, the NIC driver expects a
  pointer to the destination media address in the field DestAddr. If
  XMT_BROADCAST, the NIC driver fills the broadcast address for the
  destination.
  TBD: Segment:Offset address of the transmit buffer descriptor.
  ImmedLength: Length of the immediate transmit buffer: Xmit.
  Xmit: Segment:Offset of the immediate transmit buffer.
  DataBlkCount: Number of blocks in this transmit buffer.
  TDPtrType:
  0 => 32-bit physical address in TDDataPtr (not supported in this
  version of PXE)
  1 => segment:offset in TDDataPtr which can be a real mode or 16-bit
  protected mode pointer
  TDRsvdByte: Reserved must be zero.
  TDDatalen: Data block length in bytes.
  TDDataPtr: Segment:Offset of the transmit block.
  DataBlock: Array of transmit data blocks.
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants  
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                              
  @return Return value of PXE option ROM far call.  
**/
EFI_STATUS
PxeUndiTransmit (
  IN     EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_TRANSMIT_T   *PxeUndiTable
  )
{
  EFI_STATUS  Status;

  Status = MakePxeCall (
            SimpleNetworkDevice,
            PxeUndiTable,
            sizeof (PXENV_UNDI_TRANSMIT_T),
            PXENV_UNDI_TRANSMIT
            );
  if (Status == EFI_SUCCESS) {
    return EFI_SUCCESS;
  }

  switch (PxeUndiTable->Status) {
  case PXENV_STATUS_OUT_OF_RESOURCES:
    return EFI_NOT_READY;

  default:
    return EFI_DEVICE_ERROR;
  }
}

/**
  PXE 
  UNDI SET MULTICAST ADDRESS
  Op-Code: PXENV_UNDI_SET_MCAST_ADDRESS (0009h)
  Input: Far pointer to a PXENV_TFTP_SET_MCAST_ADDRESS_t parameter structure that has been
  initialized by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call changes the current list of multicast addresses to the input list and resets the network
  adapter to accept it. If the number of multicast addresses is zero, multicast is disabled.
  typedef struct {
    PXENV_STATUS Status;
    PXENV_UNDI_MCAST_ADDRESS_t R_Mcast_Buf;
  } PXENV_UNDI_SET_MCAST_ADDR_T;
  Set before calling API service
  R_Mcast_Buf: See description in the UNDI RESET ADAPTER
  (0004h) API.
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants        
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                              
  @return Return value of PXE option ROM far call.  
**/
EFI_STATUS
PxeUndiSetMcastAddr (
  IN     EFI_SIMPLE_NETWORK_DEV       *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_SET_MCAST_ADDR_T  *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_SET_MCAST_ADDR_T),
          PXENV_UNDI_SET_MCAST_ADDR
          );
}

/**
  PXE 
  UNDI SET STATION ADDRESS
  Op-Code: PXENV_UNDI_SET_STATION_ADDRESS (000Ah)
  Input: Far pointer to a PXENV_UNDI_SET_STATION_ADDRESS_t parameter structure that has been
  initialized by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call sets the MAC address to be the input value and is called before opening the network
  adapter. Later, the open call uses this variable as a temporary MAC address to program the
  adapter individual address registers.
  typedef struct {
    PXENV_STATUS Status;
    MAC_ADDR StationAddress;
  } PXENV_UNDI_SET_STATION_ADDR_T;
  Set before calling API service
  StationAddress: Temporary MAC address to be used for
  transmit and receive.
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.     
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                              
  @return Return value of PXE option ROM far call.  
**/
EFI_STATUS
PxeUndiSetStationAddr (
  IN     EFI_SIMPLE_NETWORK_DEV         *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_SET_STATION_ADDR_T  *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_SET_STATION_ADDR_T),
          PXENV_UNDI_SET_STATION_ADDR
          );
}

/**
  PXE 
  UNDI SET PACKET FILTER
  Op-Code: PXENV_UNDI_SET_PACKET_FILTER (000Bh)
  Input: Far pointer to a PXENV_UNDI_SET_PACKET_FILTER_T parameter structure that has been
  initialized by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call resets the adapter's receive unit to accept a new filter, different from the one provided with
  the open call.
  typedef struct {
    PXENV_STATUS Status;
    UINT8 filter;
  } PXENV_UNDI_SET_PACKET_FILTER_T;
  Set before calling API service
  Filter: See the receive filter values in the UNDI OPEN
  (0006h) API description.
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.   
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                              
  @return Return value of PXE option ROM far call.  
**/
EFI_STATUS
PxeUndiSetPacketFilter (
  IN     EFI_SIMPLE_NETWORK_DEV          *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_SET_PACKET_FILTER_T  *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_SET_PACKET_FILTER_T),
          PXENV_UNDI_SET_PACKET_FILTER
          );
}

/**
  PXE 
  UNDI GET INFORMATION
  Op-Code: PXENV_UNDI_GET_INFORMATION (000Ch)
  Input: Far pointer to a PXENV_UNDI_GET_INFORMATION_T parameter structure that has been
  initialized by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the
  PXENV_STATUS_xxx constants.
  Description: This call copies the network adapter variables, including the MAC address, into the input buffer.
  Note: The PermNodeAddress field must be valid after PXENV_START_UNDI and
  PXENV_UNDI_STARTUP have been issued. All other fields must be valid after
  PXENV_START_UNDI, PXENV_UNDI_STARTUP and PXENV_UNDI_INITIALIZE have been
  called.
  typedef struct {
    PXENV_STATUS Status;
    UINT16 BaseIo;
    UINT16 IntNumber;
    UINT16 MaxTranUnit;
    UINT16 HwType;
      #define ETHER_TYPE 1
      #define EXP_ETHER_TYPE 2
      #define IEEE_TYPE 6
      #define ARCNET_TYPE 7
    UINT16 HwAddrLen;
    MAC_ADDR CurrentNodeAddress;
    MAC_ADDR PermNodeAddress;
    SEGSEL ROMAddress;
    UINT16 RxBufCt;
    UINT16 TxBufCt;
  } PXENV_UNDI_GET_INFORMATION_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.
  BaseIO: Adapter base I/O address.
  IntNumber: Adapter IRQ number.
  MaxTranUnit: Adapter maximum transmit unit.
  HWType: Type of protocol at the hardware level.
  HWAddrLen: Length of the hardware address.
  CurrentNodeAddress: Current hardware address.
  PermNodeAddress: Permanent hardware address.
  ROMAddress: Real mode ROM segment address.
  RxBufCnt: Receive queue length.
  TxBufCnt: Transmit queue length.  
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                              
  @return Return value of PXE option ROM far call.  
**/
EFI_STATUS
PxeUndiGetInformation (
  IN     EFI_SIMPLE_NETWORK_DEV        *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_GET_INFORMATION_T  *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_GET_INFORMATION_T),
          PXENV_UNDI_GET_INFORMATION
          );
}

/**
  PXE 
  UNDI GET STATISTICS
  Op-Code: PXENV_UNDI_GET_STATISTICS (000Dh)
  Input: Far pointer to a PXENV_UNDI_GET_STATISTICS_T parameter structure that has been initialized
  by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call reads statistical information from the network adapter, and returns.
  typedef struct {
    PXENV_STATUS Status;
    UINT32 XmtGoodFrames;
    UINT32 RcvGoodFrames;
    UINT32 RcvCRCErrors;
    UINT32 RcvResourceErrors;
  } PXENV_UNDI_GET_STATISTICS_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.
  XmtGoodFrames: Number of successful transmissions.
  RcvGoodFrames: Number of good frames received.
  RcvCRCErrors: Number of frames received with CRC
  error.
  RcvResourceErrors: Number of frames discarded
  because receive queue was full.
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                              
  @return Return value of PXE option ROM far call.  
**/
EFI_STATUS
PxeUndiGetStatistics (
  IN     EFI_SIMPLE_NETWORK_DEV       *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_GET_STATISTICS_T  *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_GET_STATISTICS_T),
          PXENV_UNDI_GET_STATISTICS
          );
}

/**
  PXE 
  UNDI CLEAR STATISTICS
  Op-Code: PXENV_UNDI_CLEAR_STATISTICS (000Eh)
  Input: Far pointer to a PXENV_UNDI_CLEAR_STATISTICS_T parameter.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the
  PXENV_STATUS_xxx constants.
  Description: This call clears the statistical information from the network adapter.
  typedef struct {
    PXENV_STATUS Status;
  } PXENV_UNDI_CLEAR_STATISTICS_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                              
  @return Return value of PXE option ROM far call.  
**/
EFI_STATUS
PxeUndiClearStatistics (
  IN     EFI_SIMPLE_NETWORK_DEV         *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_CLEAR_STATISTICS_T  *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_CLEAR_STATISTICS_T),
          PXENV_UNDI_CLEAR_STATISTICS
          );
}

/**
  PXE 
  UNDI INITIATE DIAGS
  Op-Code: PXENV_UNDI_INITIATE_DIAGS (000Fh)
  Input: Far pointer to a PXENV_UNDI_INITIATE_DIAGS_T parameter.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the
  PXENV_STATUS_xxx constants.
  Description: This call can be used to initiate the run-time diagnostics. It causes the network adapter to run
  hardware diagnostics and to update its status information.
  typedef struct {
    PXENV_STATUS Status;
  } PXENV_UNDI_INITIATE_DIAGS_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.    
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                              
  @return Return value of PXE option ROM far call.  
**/
EFI_STATUS
PxeUndiInitiateDiags (
  IN     EFI_SIMPLE_NETWORK_DEV       *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_INITIATE_DIAGS_T  *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_INITIATE_DIAGS_T),
          PXENV_UNDI_INITIATE_DIAGS
          );
}

/**
  PXE 
  UNDI FORCE INTERRUPT
  Op-Code: PXENV_UNDI_FORCE_INTERRUPT (0010h)
  Input: Far pointer to a PXENV_UNDI_FORCE_INTERRUPT_T parameter structure that has been
  initialized by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call forces the network adapter to generate an interrupt. When a receive interrupt occurs, the
  network adapter driver usually queues the packet and calls the application's callback receive
  routine with a pointer to the packet received. Then, the callback routine either can copy the packet
  to its buffer or can decide to delay the copy to a later time. If the packet is not immediately copied,
  the network adapter driver does not remove it from the input queue. When the application wants to
  copy the packet, it can call the PXENV_UNDI_FORCE_INTERRUPT routine to simulate the receive
  interrupt.
  typedef struct {
    PXENV_STATUS Status;
  } PXENV_UNDI_FORCE_INTERRUPT_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.  
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                              
  @return Return value of PXE option ROM far call.  
**/
EFI_STATUS
PxeUndiForceInterrupt (
  IN     EFI_SIMPLE_NETWORK_DEV        *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_FORCE_INTERRUPT_T  *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_FORCE_INTERRUPT_T),
          PXENV_UNDI_FORCE_INTERRUPT
          );
}

/**
  PXE 
  UNDI GET MULTICAST ADDRESS
  Op-Code: PXENV_UNDI_GET_MCAST_ADDRESS (0011h)
  Input: Far pointer to a PXENV_GET_MCAST_ADDRESS_t parameter structure that has been initialized
  by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call converts the given IP multicast address to a hardware multicast address.
  typedef struct  {
    PXENV_STATUS Status;
    IP4 InetAddr;
    MAC_ADDR MediaAddr;
  } PXENV_UNDI_GET_MCAST_ADDR_T;
  Set before calling API service
  InetAddr: IP multicast address.
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.
  MediaAddr: MAC multicast address.
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                              
  @return Return value of PXE option ROM far call.  
**/
EFI_STATUS
PxeUndiGetMcastAddr (
  IN     EFI_SIMPLE_NETWORK_DEV       *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_GET_MCAST_ADDR_T  *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_GET_MCAST_ADDR_T),
          PXENV_UNDI_GET_MCAST_ADDR
          );
}

/**
  PXE 
  UNDI GET NIC TYPE
  Op-Code: PXENV_UNDI_GET_NIC_TYPE (0012h)
  Input: Far pointer to a PXENV_UNDI_GET_NIC_TYPE parameter structure that has been initialized by
  the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants. If the PXENV_EXIT_SUCCESS is returned the parameter structure must contain the
  NIC information.
  Description: This call, if successful, provides the NIC-specific information necessary to identify the network
  adapter that is used to boot the system.
  Note: The application first gets the DHCPDISCOVER packet using GET_CACHED_INFO and checks if
  the UNDI is supported before making this call. If the UNDI is not supported, the NIC-specific
  information can be obtained from the DHCPDISCOVER packet itself.
  PXENV_START_UNDI, PXENV_UNDI_STARTUP and PXENV_UNDI_INITIALIZE must be called
  before the information provided is valid.
  typedef {
    PXENV_STATUS Status;
    UINT8 NicType;
      #define PCI_NIC 2
      #define PnP_NIC 3
      #define CardBus_NIC 4
    Union {
      Struct {
        UINT16 Vendor_ID;
        UINT16 Dev_ID;
        UINT8 Base_Class;
        UINT8 Sub_Class;
        UINT8 Prog_Intf;
        UINT8 Rev;
        UINT16 BusDevFunc;
        UINT16 SubVendor_ID;
        UINT16 SubDevice_ID;
      } pci, cardbus;
      struct {
        UINT32 EISA_Dev_ID;
        UINT8 Base_Class;
        UINT8 Sub_Class;
        UINT8 Prog_Intf;
        UINT16 CardSelNum;
      } pnp;
    } info;
  } PXENV_UNDI_GET_NIC_TYPE_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.
  NICType: Type of NIC information stored in the parameter
  structure.
  Info: Information about the fields in this union can be found
  in the [PnP] and [PCI] specifications    
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                              
  @return Return value of PXE option ROM far call.  
**/
EFI_STATUS
PxeUndiGetNicType (
  IN     EFI_SIMPLE_NETWORK_DEV     *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_GET_NIC_TYPE_T  *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_GET_NIC_TYPE_T),
          PXENV_UNDI_GET_NIC_TYPE
          );
}

/**
  PXE 
  UNDI GET IFACE INFO
  Op-Code: PXENV_UNDI_GET_IFACE_INFO (0013h)
  Input: Far pointer to a PXENV_UNDI_GET_IFACE_INFO_t parameter structure that has been initialized
  by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants. If the PXENV_EXIT_SUCCESS is returned, the parameter structure must contain the
  interface specific information.
  Description: This call, if successful, provides the network interface specific information such as the interface
  type at the link layer (Ethernet, Tokenring) and the link speed. This information can be used in the
  universal drivers such as NDIS or Miniport to communicate to the upper protocol modules.
  Note: UNDI follows the NDIS2 specification in giving this information. It is the responsibility of the
  universal driver to translate/convert this information into a format that is required in its specification
  or to suit the expectation of the upper level protocol modules.
  PXENV_START_UNDI, PXENV_UNDI_STARTUP and PXENV_UNDI_INITIALIZE must be called
  before the information provided is valid.
  typedef struct {
    PXENV_STATUS Status
    UINT8 IfaceType[16];
    UINT32 LinkSpeed;
    UINT32 ServiceFlags;
    UINT32 Reserved[4];
  } PXENV_UNDI_GET_NDIS_INFO_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.
  IfaceType: Name of MAC type in ASCIIZ format. This is
  used by the universal NDIS driver to specify its driver type
  to the protocol driver.
  LinkSpeed: Defined in the NDIS 2.0 specification.
  ServiceFlags: Defined in the NDIS 2.0 specification.
  Reserved: Must be zero.       
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                              
  @return Return value of PXE option ROM far call.  
**/
EFI_STATUS
PxeUndiGetNdisInfo (
  IN     EFI_SIMPLE_NETWORK_DEV      *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_GET_NDIS_INFO_T  *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_GET_NDIS_INFO_T),
          PXENV_UNDI_GET_NDIS_INFO
          );
}

/**
  PXE 
  UNDI ISR
  Op-Code: PXENV_UNDI_ISR (0014h)
  Input: Far pointer to a PXENV_UNDI_ISR_T parameter structure that has been initialized by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This API function will be called at different levels of processing the interrupt. The FuncFlag field in
  the parameter block indicates the operation to be performed for the call. This field is filled with the
  status of that operation on return.
  Note: Interrupt Service Routine Operation:
  In this design the UNDI does not hook the interrupt for the Network Interface. Instead, the
  application or the protocol driver hooks the interrupt and calls UNDI with the PXENV_UNDI_ISR
  API call for interrupt verification (PXENV_UNDI_ISR_IN_START) and processing
  (PXENV_UNDI_ISR_IN_PROCESS and PXENV_UNDI_ISR_GET_NEXT).
  When the Network Interface HW generates an interrupt the protocol driver interrupt service
  routine (ISR) gets control and takes care of the interrupt processing at the PIC level. The ISR then
  calls the UNDI using the PXENV_UNDI_ISR API with the value PXENV_UNDI_ISR_IN_START for
  the FuncFlag parameter. At this time UNDI must disable the interrupts at the Network Interface
  level and read any status values required to further process the interrupt. UNDI must return as
  quickly as possible with one of the two values, PXENV_UNDI_ISR_OUT_OURS or
  PXENV_UNDI_ISR_OUT_NOT_OURS, for the parameter FuncFlag depending on whether the
  interrupt was generated by this particular Network Interface or not.
  If the value returned in FuncFlag is PXENV_UNDI_ISR_OUT_NOT_OURS, then the interrupt was
  not generated by our NIC, and interrupt processing is complete.
  If the value returned in FuncFlag is PXENV_UNDI_ISR_OUT_OURS, the protocol driver must start
  a handler thread and send an end-of-interrupt (EOI) command to the PIC. Interrupt processing is
  now complete.
  The protocol driver strategy routine will call UNDI using this same API with FuncFlag equal to
  PXENV_UNDI_ISR_IN_PROCESS. At this time UNDI must find the cause of this interrupt and
  return the status in the FuncFlag. It first checks if there is a frame received and if so it returns the
  first buffer pointer of that frame in the parameter block.
  The protocol driver calls UNDI repeatedly with the FuncFlag equal to
  PXENV_UNDI_ISR_IN_GET_NEXT to get all the buffers in a frame and also all the received
  frames in the queue. On this call, UNDI must remember the previous buffer given to the protoco,l
  remove it from the receive queue and recycle it. In case of a multi-buffered frame, if the previous
  buffer is not the last buffer in the frame it must return the next buffer in the frame in the parameter
  block. Otherwise it must return the first buffer in the next frame.
  If there is no received frame pending to be processed, UNDI processes the transmit completes and
  if there is no other interrupt status to be processed, UNDI re-enables the interrupt at the
  NETWORK INTERFACE level and returns PXENV_UNDI_ISR_OUT_DONE in the FuncFlag.
  IMPORTANT: It is possible for the protocol driver to be interrupted again while in the
  strategy routine when the UNDI re-enables interrupts.   
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                              
  @return Return value of PXE option ROM far call.  
**/
EFI_STATUS
PxeUndiIsr (
  IN     EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_ISR_T        *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_ISR_T),
          PXENV_UNDI_ISR
          );
}

/**
  PXE 
  STOP UNDI
  Op-Code: PXENV_STOP_UNDI (0015h)
  Input: Far pointer to a PXENV_STOP_UNDI_T parameter structure that has been initialized by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This routine is responsible for unhooking the Int 1Ah service routine.
  Note: This API service must be called only once at the end of UNDI Option ROM boot. One of the valid
  status codes is PXENV_STATUS_KEEP. If this status is returned, UNDI must not be removed from
  base memory. Also, UNDI must not be removed from base memory if BC is not removed from base
  memory.
  Service cannot be used in protected mode.
  typedef struct {
    PXENV_STATUS Status;
  } PXENV_STOP_UNDI_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.      
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                              
  @return Return value of PXE option ROM far call.  
**/
EFI_STATUS
PxeUndiStop (
  IN     EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice,
  IN OUT PXENV_STOP_UNDI_T       *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_STOP_UNDI_T),
          PXENV_STOP_UNDI
          );
}

/**
  PXE 
  UNDI GET STATE
  Op-Code: PXENV_UNDI_GET_STATE (0015h)
  Input: Far pointer to a PXENV_UNDI_GET_STATE_T parameter.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants. The UNDI_STATE field in the parameter structure must be set to one of the valid state
  constants
  Description: This call can be used to obtain state of the UNDI engine in order to avoid issuing adverse call
  sequences
  typedef struct {
    #define PXE_UNDI_GET_STATE_STARTED 1
    #define PXE_UNDI_GET_STATE_INITIALIZED 2
    #define PXE_UNDI_GET_STATE_OPENED 3
    PXENV_STATUS Status;
    UINT8 UNDIstate;
  } PXENV_UNDI_GET_STATE_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.
  State: See definitions of the state constants.
  Note. UNDI implementation is responsible for maintaining
  internal state machine.
  UNDI ISR
  Op-Code: PXENV_UNDI_ISR (0014h)
  Input: Far pointer to a t_PXENV_UNDI_ISR parameter structure that has been initialized by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This API function will be called at different levels of processing the interrupt. The FuncFlag field in
  the parameter block indicates the operation to be performed for the call. This field is filled with the
  status of that operation on return.     
  
  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold paramter and return value 
                                for option ROM call.
                              
  @return Return value of PXE option ROM far call.  
**/
EFI_STATUS
PxeUndiGetState (
  IN     EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_GET_STATE_T  *PxeUndiTable
  )
{
  return MakePxeCall (
          SimpleNetworkDevice,
          PxeUndiTable,
          sizeof (PXENV_UNDI_GET_STATE_T),
          PXENV_UNDI_GET_STATE
          );
}
