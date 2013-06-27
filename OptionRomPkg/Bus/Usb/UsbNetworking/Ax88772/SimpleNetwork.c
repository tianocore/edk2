/** @file
  Provides the Simple Network functions.

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ax88772.h"

/**
  This function updates the filtering on the receiver.

  This support routine calls ::Ax88772MacAddressSet to update
  the MAC address.  This routine then rebuilds the multicast
  hash by calling ::Ax88772MulticastClear and ::Ax88772MulticastSet.
  Finally this routine enables the receiver by calling
  ::Ax88772RxControl.

  @param [in] pSimpleNetwork    Simple network mode pointer

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_INVALID_PARAMETER pSimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
ReceiveFilterUpdate (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * pSimpleNetwork
  )
{
  EFI_SIMPLE_NETWORK_MODE * pMode;
  NIC_DEVICE * pNicDevice;
  EFI_STATUS Status;
  UINT32 Index;

  DBG_ENTER ( );

  //
  // Set the MAC address
  //
  pNicDevice = DEV_FROM_SIMPLE_NETWORK ( pSimpleNetwork );
  pMode = pSimpleNetwork->Mode;
  Status = Ax88772MacAddressSet ( pNicDevice,
                                  &pMode->CurrentAddress.Addr[0]);
  if ( !EFI_ERROR ( Status )) {
    //
    // Clear the multicast hash table
    //
    Ax88772MulticastClear ( pNicDevice );

    //
    // Load the multicast hash table
    //
    if ( 0 != ( pMode->ReceiveFilterSetting & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST )) {
      for ( Index = 0;
            ( !EFI_ERROR ( Status )) && ( Index < pMode->MCastFilterCount );
            Index++ ) {
        //
        // Enable the next multicast address
        //
        Ax88772MulticastSet ( pNicDevice,
                              &pMode->MCastFilter[ Index ].Addr[0]);
      }
    }

    //
    // Enable the receiver
    //
    if ( !EFI_ERROR ( Status )) {
      Status = Ax88772RxControl ( pNicDevice, pMode->ReceiveFilterSetting );
    }
  }

  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  This function updates the SNP driver status.
  
  This function gets the current interrupt and recycled transmit
  buffer status from the network interface.  The interrupt status
  and the media status are returned as a bit mask in InterruptStatus.
  If InterruptStatus is NULL, the interrupt status will not be read.
  Upon successful return of the media status, the MediaPresent field
  of EFI_SIMPLE_NETWORK_MODE will be updated to reflect any change
  of media status.  If TxBuf is not NULL, a recycled transmit buffer
  address will be retrived.  If a recycled transmit buffer address
  is returned in TxBuf, then the buffer has been successfully
  transmitted, and the status for that buffer is cleared.

  This function calls ::Ax88772Rx to update the media status and
  queue any receive packets.

  @param [in] pSimpleNetwork    Protocol instance pointer
  @param [in] pInterruptStatus  A pointer to the bit mask of the current active interrupts.
                                If this is NULL, the interrupt status will not be read from
                                the device.  If this is not NULL, the interrupt status will
                                be read from teh device.  When the interrupt status is read,
                                it will also be cleared.  Clearing the transmit interrupt
                                does not empty the recycled transmit buffer array.
  @param [out] ppTxBuf          Recycled transmit buffer address.  The network interface will
                                not transmit if its internal recycled transmit buffer array is
                                full.  Reading the transmit buffer does not clear the transmit
                                interrupt.  If this is NULL, then the transmit buffer status
                                will not be read.  If there are not transmit buffers to recycle
                                and TxBuf is not NULL, *TxBuf will be set to NULL.

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_INVALID_PARAMETER pSimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.

**/
EFI_STATUS
EFIAPI
SN_GetStatus (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * pSimpleNetwork,
  OUT UINT32 * pInterruptStatus,
  OUT VOID ** ppTxBuf
  )
{
  BOOLEAN bLinkIdle;
  EFI_SIMPLE_NETWORK_MODE * pMode;
  NIC_DEVICE * pNicDevice;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  // Verify the parameters
  //
  if (( NULL != pSimpleNetwork ) && ( NULL != pSimpleNetwork->Mode )) {
    //
    // Return the transmit buffer
    //
    pNicDevice = DEV_FROM_SIMPLE_NETWORK ( pSimpleNetwork );
    if (( NULL != ppTxBuf ) && ( NULL != pNicDevice->pTxBuffer )) {
      *ppTxBuf = pNicDevice->pTxBuffer;
      pNicDevice->pTxBuffer = NULL;
    }

    //
    // Determine if interface is running
    //
    pMode = pSimpleNetwork->Mode;
    if ( EfiSimpleNetworkStopped != pMode->State ) {
      //
      //  Synchronize with Ax88772Timer
      //
      VERIFY_TPL ( TPL_AX88772 );
      TplPrevious = gBS->RaiseTPL ( TPL_AX88772 );

      //
      // Update the link status
      //
      bLinkIdle = pNicDevice->bLinkIdle;
      pNicDevice->bLinkIdle = TRUE;
      Ax88772Rx ( pNicDevice, bLinkIdle );
      pMode->MediaPresent = pNicDevice->bLinkUp;

      //
      //  Release the synchronization with Ax88772Timer
      //
      gBS->RestoreTPL ( TplPrevious );

      //
      // Return the interrupt status
      //
      if ( NULL != pInterruptStatus ) {
        *pInterruptStatus = 0;
      }
      Status = EFI_SUCCESS;
    }
    else {
      Status = EFI_NOT_STARTED;
    }
  }
  else {
    Status = EFI_INVALID_PARAMETER;
  }

  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Resets the network adapter and allocates the transmit and receive buffers
  required by the network interface; optionally, also requests allocation of
  additional transmit and receive buffers.  This routine must be called before
  any other routine in the Simple Network protocol is called.

  @param [in] pSimpleNetwork    Protocol instance pointer
  @param [in] ExtraRxBufferSize Size in bytes to add to the receive buffer allocation
  @param [in] ExtraTxBufferSize Size in bytes to add to the transmit buffer allocation

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_OUT_OF_RESORUCES  There was not enough memory for the transmit and receive buffers
  @retval EFI_INVALID_PARAMETER pSimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
EFIAPI
SN_Initialize (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * pSimpleNetwork,
  IN UINTN ExtraRxBufferSize,
  IN UINTN ExtraTxBufferSize
  )
{
  EFI_SIMPLE_NETWORK_MODE * pMode;
  EFI_STATUS Status;

  DBG_ENTER ( );
  
  //
  // Verify the parameters
  //
  if (( NULL != pSimpleNetwork ) && ( NULL != pSimpleNetwork->Mode )) {
    //
    // Determine if the interface is already started
    //
    pMode = pSimpleNetwork->Mode;
    if ( EfiSimpleNetworkStarted == pMode->State ) {
      if (( 0 == ExtraRxBufferSize ) && ( 0 == ExtraTxBufferSize )) {
        //
        // Start the adapter
        //
        Status = SN_Reset ( pSimpleNetwork, FALSE );
        if ( !EFI_ERROR ( Status )) {
          //
          // Update the network state
          //
          pMode->State = EfiSimpleNetworkInitialized;
        }
      }
      else {
        Status = EFI_UNSUPPORTED;
      }
    }
    else {
      Status = EFI_NOT_STARTED;
    }
  }
  else {
    Status = EFI_INVALID_PARAMETER;
  }
  
  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  This function converts a multicast IP address to a multicast HW MAC address
  for all packet transactions.

  @param [in] pSimpleNetwork    Protocol instance pointer
  @param [in] bIPv6             Set to TRUE if the multicast IP address is IPv6 [RFC2460].
                                Set to FALSE if the multicast IP address is IPv4 [RFC 791].
  @param [in] pIP               The multicast IP address that is to be converted to a
                                multicast HW MAC address.
  @param [in] pMAC              The multicast HW MAC address that is to be generated from IP.

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_INVALID_PARAMETER pSimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
EFIAPI
SN_MCastIPtoMAC (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * pSimpleNetwork,
  IN BOOLEAN bIPv6,
  IN EFI_IP_ADDRESS * pIP,
  IN EFI_MAC_ADDRESS * pMAC
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  // This is not currently supported
  //
  Status = EFI_UNSUPPORTED;

  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  This function performs read and write operations on the NVRAM device
  attached to a network interface.

  @param [in] pSimpleNetwork    Protocol instance pointer
  @param [in] ReadWrite         TRUE for read operations, FALSE for write operations.
  @param [in] Offset            Byte offset in the NVRAM device at which to start the
                                read or write operation.  This must be a multiple of
                                NvRamAccessSize and less than NvRamSize.
  @param [in] BufferSize        The number of bytes to read or write from the NVRAM device.
                                This must also be a multiple of NvramAccessSize.
  @param [in, out] pBuffer      A pointer to the data buffer.

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_INVALID_PARAMETER pSimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
EFIAPI
SN_NvData (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * pSimpleNetwork,
  IN BOOLEAN ReadWrite,
  IN UINTN Offset,
  IN UINTN BufferSize,
  IN OUT VOID * pBuffer
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  // This is not currently supported
  //
  Status = EFI_UNSUPPORTED;

  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Attempt to receive a packet from the network adapter.

  This function retrieves one packet from the receive queue of the network
  interface.  If there are no packets on the receive queue, then EFI_NOT_READY
  will be returned.  If there is a packet on the receive queue, and the size
  of the packet is smaller than BufferSize, then the contents of the packet
  will be placed in Buffer, and BufferSize will be udpated with the actual
  size of the packet.  In addition, if SrcAddr, DestAddr, and Protocol are
  not NULL, then these values will be extracted from the media header and
  returned.  If BufferSize is smaller than the received packet, then the
  size of the receive packet will be placed in BufferSize and
  EFI_BUFFER_TOO_SMALL will be returned.

  This routine calls ::Ax88772Rx to update the media status and
  empty the network adapter of receive packets.

  @param [in] pSimpleNetwork    Protocol instance pointer
  @param [out] pHeaderSize      The size, in bytes, of the media header to be filled in by
                                the Transmit() function.  If HeaderSize is non-zero, then
                                it must be equal to SimpleNetwork->Mode->MediaHeaderSize
                                and DestAddr and Protocol parameters must not be NULL.
  @param [out] pBufferSize      The size, in bytes, of the entire packet (media header and
                                data) to be transmitted through the network interface.
  @param [out] pBuffer          A pointer to the packet (media header followed by data) to
                                to be transmitted.  This parameter can not be NULL.  If
                                HeaderSize is zero, then the media header is Buffer must
                                already be filled in by the caller.  If HeaderSize is nonzero,
                                then the media header will be filled in by the Transmit()
                                function.
  @param [out] pSrcAddr         The source HW MAC address.  If HeaderSize is zero, then
                                this parameter is ignored.  If HeaderSize is nonzero and
                                SrcAddr is NULL, then SimpleNetwork->Mode->CurrentAddress
                                is used for the source HW MAC address.
  @param [out] pDestAddr        The destination HW MAC address.  If HeaderSize is zero, then
                                this parameter is ignored.
  @param [out] pProtocol        The type of header to build.  If HeaderSize is zero, then
                                this parameter is ignored.

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_NOT_READY         No packets have been received on the network interface.
  @retval EFI_BUFFER_TOO_SMALL  The packet is larger than BufferSize bytes.
  @retval EFI_INVALID_PARAMETER pSimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.

**/
EFI_STATUS
EFIAPI
SN_Receive (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * pSimpleNetwork,
  OUT UINTN                      * pHeaderSize,
  OUT UINTN                      * pBufferSize,
  OUT VOID                       * pBuffer,
  OUT EFI_MAC_ADDRESS            * pSrcAddr,
  OUT EFI_MAC_ADDRESS            * pDestAddr,
  OUT UINT16                     * pProtocol
  )
{
  ETHERNET_HEADER * pHeader;
  EFI_SIMPLE_NETWORK_MODE * pMode;
  NIC_DEVICE * pNicDevice;
  RX_TX_PACKET * pRxPacket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;
  UINT16 Type;

  DBG_ENTER ( );

  //
  // Verify the parameters
  //
  if (( NULL != pSimpleNetwork ) && ( NULL != pSimpleNetwork->Mode )) {
    //
    // The interface must be running
    //
    pMode = pSimpleNetwork->Mode;
    if ( EfiSimpleNetworkInitialized == pMode->State ) {
      //
      //  Synchronize with Ax88772Timer
      //
      VERIFY_TPL ( TPL_AX88772 );
      TplPrevious = gBS->RaiseTPL ( TPL_AX88772 );

      //
      // Update the link status
      //
      pNicDevice = DEV_FROM_SIMPLE_NETWORK ( pSimpleNetwork );
      Ax88772Rx ( pNicDevice, FALSE );
      pMode->MediaPresent = pNicDevice->bLinkUp;
      if ( pMode->MediaPresent ) {
        //
        //  Attempt to receive a packet
        //
        pRxPacket = pNicDevice->pRxHead;
        if ( NULL != pRxPacket ) {
          pNicDevice->pRxHead = pRxPacket->pNext;
          if ( NULL == pNicDevice->pRxHead ) {
            pNicDevice->pRxTail = NULL;
          }

          //
          // Copy the received packet into the receive buffer
          //
          *pBufferSize = pRxPacket->Length;
          CopyMem ( pBuffer, &pRxPacket->Data[0], pRxPacket->Length );
          pHeader = (ETHERNET_HEADER *) &pRxPacket->Data[0];
          if ( NULL != pHeaderSize ) {
            *pHeaderSize = sizeof ( *pHeader );
          }
          if ( NULL != pDestAddr ) {
            CopyMem ( pDestAddr, &pHeader->dest_addr, PXE_HWADDR_LEN_ETHER );
          }
          if ( NULL != pSrcAddr ) {
            CopyMem ( pSrcAddr, &pHeader->src_addr, PXE_HWADDR_LEN_ETHER );
          }
          if ( NULL != pProtocol ) {
            Type = pHeader->type;
            Type = (UINT16)(( Type >> 8 ) | ( Type << 8 ));
            *pProtocol = Type;
          }
          Status = EFI_SUCCESS;
        }
        else {
          //
          //  No receive packets available
          //
          Status = EFI_NOT_READY;
        }
      }
      else {
        //
        //  Link no up
        //
        Status = EFI_NOT_READY;
      }

      //
      //  Release the synchronization with Ax88772Timer
      //
      gBS->RestoreTPL ( TplPrevious );
    }
    else {
      Status = EFI_NOT_STARTED;
    }
  }
  else {
    Status = EFI_INVALID_PARAMETER;
  }

  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  This function is used to enable and disable the hardware and software receive
  filters for the underlying network device.

  The receive filter change is broken down into three steps:

    1.  The filter mask bits that are set (ON) in the Enable parameter
        are added to the current receive filter settings.

    2.  The filter mask bits that are set (ON) in the Disable parameter
        are subtracted from the updated receive filter settins.

    3.  If the resulting filter settigns is not supported by the hardware
        a more liberal setting is selected.

  If the same bits are set in the Enable and Disable parameters, then the bits
  in the Disable parameter takes precedence.

  If the ResetMCastFilter parameter is TRUE, then the multicast address list
  filter is disabled (irregardless of what other multicast bits are set in
  the enable and Disable parameters).  The SNP->Mode->MCastFilterCount field
  is set to zero.  The SNP->Mode->MCastFilter contents are undefined.

  After enableing or disabling receive filter settings, software should
  verify the new settings by checking the SNP->Mode->ReceeiveFilterSettings,
  SNP->Mode->MCastFilterCount and SNP->Mode->MCastFilter fields.

  Note: Some network drivers and/or devices will automatically promote
  receive filter settings if the requested setting can not be honored.
  For example, if a request for four multicast addresses is made and
  the underlying hardware only supports two multicast addresses the
  driver might set the promiscuous or promiscuous multicast receive filters
  instead.  The receiving software is responsible for discarding any extra
  packets that get through the hardware receive filters.

  If ResetMCastFilter is TRUE, then the multicast receive filter list
  on the network interface will be reset to the default multicast receive
  filter list.  If ResetMCastFilter is FALSE, and this network interface
  allows the multicast receive filter list to be modified, then the
  MCastFilterCnt and MCastFilter are used to update the current multicast
  receive filter list.  The modified receive filter list settings can be
  found in the MCastFilter field of EFI_SIMPLE_NETWORK_MODE.

  This routine calls ::ReceiveFilterUpdate to update the receive
  state in the network adapter.

  @param [in] pSimpleNetwork    Protocol instance pointer
  @param [in] Enable            A bit mask of receive filters to enable on the network interface.
  @param [in] Disable           A bit mask of receive filters to disable on the network interface.
                                For backward compatibility with EFI 1.1 platforms, the
                                EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST bit must be set
                                when the ResetMCastFilter parameter is TRUE.
  @param [in] bResetMCastFilter Set to TRUE to reset the contents of the multicast receive
                                filters on the network interface to their default values.
  @param [in] MCastFilterCnt    Number of multicast HW MAC address in the new MCastFilter list.
                                This value must be less than or equal to the MaxMCastFilterCnt
                                field of EFI_SIMPLE_NETWORK_MODE.  This field is optional if
                                ResetMCastFilter is TRUE.
  @param [in] pMCastFilter      A pointer to a list of new multicast receive filter HW MAC
                                addresses.  This list will replace any existing multicast
                                HW MAC address list.  This field is optional if ResetMCastFilter
                                is TRUE.

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_INVALID_PARAMETER pSimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
EFIAPI
SN_ReceiveFilters (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * pSimpleNetwork,
  IN UINT32 Enable,
  IN UINT32 Disable,
  IN BOOLEAN bResetMCastFilter,
  IN UINTN MCastFilterCnt,
  IN EFI_MAC_ADDRESS * pMCastFilter
  )
{
  EFI_SIMPLE_NETWORK_MODE * pMode;
  EFI_MAC_ADDRESS * pMulticastAddress;
  EFI_MAC_ADDRESS * pTableEnd;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  // Verify the parameters
  //
  Status = EFI_INVALID_PARAMETER;
  if (( NULL != pSimpleNetwork ) && ( NULL != pSimpleNetwork->Mode )) {
    pMode = pSimpleNetwork->Mode;

    //
    //  Update the multicast list if necessary
    //
    if ( !bResetMCastFilter ) {
      if ( 0 != MCastFilterCnt ) {
        if (( MAX_MCAST_FILTER_CNT >= MCastFilterCnt )
          && ( NULL != pMCastFilter )) {
          //
          // Verify the multicast addresses
          //
          pMulticastAddress = pMCastFilter;
          pTableEnd = pMulticastAddress + MCastFilterCnt;
          while ( pTableEnd > pMulticastAddress ) {
            //
            // The first digit of the multicast address must have the LSB set
            //
            if ( 0 == ( pMulticastAddress->Addr[0] & 1 )) {
              //
              // Invalid multicast address
              //
              break;
            }
            pMulticastAddress += 1;
          }
          if ( pTableEnd == pMulticastAddress ) {
            //
            // Update the multicast filter list.
            //
            CopyMem (&pMode->MCastFilter[0],
                     pMCastFilter,
                     MCastFilterCnt * sizeof ( *pMCastFilter ));
            Status = EFI_SUCCESS;
          }
        }
      }
      else {
        Status = EFI_SUCCESS;
      }
    }
    else {
      //
      // No multicast address list is specified
      //
      MCastFilterCnt = 0;
      Status = EFI_SUCCESS;
    }
    if ( !EFI_ERROR ( Status )) {
      //
      // The parameters are valid!
      //
      pMode->ReceiveFilterSetting |= Enable;
      pMode->ReceiveFilterSetting &= ~Disable;
      pMode->MCastFilterCount = (UINT32)MCastFilterCnt;

      //
      // Update the receive filters in the adapter
      //
      Status = ReceiveFilterUpdate ( pSimpleNetwork );
    }
  }

  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Reset the network adapter.

  Resets a network adapter and reinitializes it with the parameters that
  were provided in the previous call to Initialize ().  The transmit and
  receive queues are cleared.  Receive filters, the station address, the
  statistics, and the multicast-IP-to-HW MAC addresses are not reset by
  this call.

  This routine calls ::Ax88772Reset to perform the adapter specific
  reset operation.  This routine also starts the link negotiation
  by calling ::Ax88772NegotiateLinkStart.

  @param [in] pSimpleNetwork    Protocol instance pointer
  @param [in] bExtendedVerification  Indicates that the driver may perform a more
                                exhaustive verification operation of the device
                                during reset.

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_INVALID_PARAMETER pSimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
EFIAPI
SN_Reset (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * pSimpleNetwork,
  IN BOOLEAN bExtendedVerification
  )
{
  EFI_SIMPLE_NETWORK_MODE * pMode;
  NIC_DEVICE * pNicDevice;
  RX_TX_PACKET * pRxPacket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Verify the parameters
  //
  if (( NULL != pSimpleNetwork ) && ( NULL != pSimpleNetwork->Mode )) {
    //
    //  Synchronize with Ax88772Timer
    //
    VERIFY_TPL ( TPL_AX88772 );
    TplPrevious = gBS->RaiseTPL ( TPL_AX88772 );

    //
    //  Update the device state
    //
    pNicDevice = DEV_FROM_SIMPLE_NETWORK ( pSimpleNetwork );
    pNicDevice->bComplete = FALSE;
    pNicDevice->bLinkUp = FALSE;

    pMode = pSimpleNetwork->Mode;
    pMode->MediaPresent = FALSE;

    //
    //  Discard any received packets
    //
    while ( NULL != pNicDevice->pRxHead ) {
      //
      //  Remove the packet from the received packet list
      //
      pRxPacket = pNicDevice->pRxHead;
      pNicDevice->pRxHead = pRxPacket->pNext;

      //
      //  Queue the packet to the free list
      //
      pRxPacket->pNext = pNicDevice->pRxFree;
      pNicDevice->pRxFree = pRxPacket;
    }
    pNicDevice->pRxTail = NULL;

    //
    //  Reset the device
    //
    Status = Ax88772Reset ( pNicDevice );
    if ( !EFI_ERROR ( Status )) {
      //
      //  Update the receive filters in the adapter
      //
      Status = ReceiveFilterUpdate ( pSimpleNetwork );

      //
      //  Try to get a connection to the network
      //
      if ( !EFI_ERROR ( Status )) {
        //
        //  Start the autonegotiation
        //
        Status = Ax88772NegotiateLinkStart ( pNicDevice );
      }
    }

    //
    //  Release the synchronization with Ax88772Timer
    //
    gBS->RestoreTPL ( TplPrevious );
  }
  else {
    Status = EFI_INVALID_PARAMETER;
  }

  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Initialize the simple network protocol.

  This routine calls ::Ax88772MacAddressGet to obtain the
  MAC address.

  @param [in] pNicDevice       NIC_DEVICE_INSTANCE pointer

  @retval EFI_SUCCESS     Setup was successful

**/
EFI_STATUS
SN_Setup (
  IN NIC_DEVICE * pNicDevice
  )
{
  EFI_SIMPLE_NETWORK_MODE * pMode;
  EFI_SIMPLE_NETWORK_PROTOCOL * pSimpleNetwork;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  // Initialize the simple network protocol
  //
  pSimpleNetwork = &pNicDevice->SimpleNetwork;
  pSimpleNetwork->Revision = EFI_SIMPLE_NETWORK_PROTOCOL_REVISION;
  pSimpleNetwork->Start = (EFI_SIMPLE_NETWORK_START)SN_Start;
  pSimpleNetwork->Stop = (EFI_SIMPLE_NETWORK_STOP)SN_Stop;
  pSimpleNetwork->Initialize = (EFI_SIMPLE_NETWORK_INITIALIZE)SN_Initialize;
  pSimpleNetwork->Reset = (EFI_SIMPLE_NETWORK_RESET)SN_Reset;
  pSimpleNetwork->Shutdown = (EFI_SIMPLE_NETWORK_SHUTDOWN)SN_Shutdown;
  pSimpleNetwork->ReceiveFilters = (EFI_SIMPLE_NETWORK_RECEIVE_FILTERS)SN_ReceiveFilters;
  pSimpleNetwork->StationAddress = (EFI_SIMPLE_NETWORK_STATION_ADDRESS)SN_StationAddress;
  pSimpleNetwork->Statistics = (EFI_SIMPLE_NETWORK_STATISTICS)SN_Statistics;
  pSimpleNetwork->MCastIpToMac = (EFI_SIMPLE_NETWORK_MCAST_IP_TO_MAC)SN_MCastIPtoMAC;
  pSimpleNetwork->NvData = (EFI_SIMPLE_NETWORK_NVDATA)SN_NvData;
  pSimpleNetwork->GetStatus = (EFI_SIMPLE_NETWORK_GET_STATUS)SN_GetStatus;
  pSimpleNetwork->Transmit = (EFI_SIMPLE_NETWORK_TRANSMIT)SN_Transmit;
  pSimpleNetwork->Receive = (EFI_SIMPLE_NETWORK_RECEIVE)SN_Receive;
  pSimpleNetwork->WaitForPacket = NULL;
  pMode = &pNicDevice->SimpleNetworkData;
  pSimpleNetwork->Mode = pMode;

  pMode->State = EfiSimpleNetworkStopped;
  pMode->HwAddressSize = PXE_HWADDR_LEN_ETHER;
  pMode->MediaHeaderSize = sizeof ( ETHERNET_HEADER );
  pMode->MaxPacketSize = MAX_ETHERNET_PKT_SIZE;
  pMode->NvRamSize = 0;
  pMode->NvRamAccessSize = 0;
  pMode->ReceiveFilterMask = EFI_SIMPLE_NETWORK_RECEIVE_UNICAST
                           | EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST
                           | EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST
                           | EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS
                           | EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST;
  pMode->ReceiveFilterSetting = EFI_SIMPLE_NETWORK_RECEIVE_UNICAST
                              | EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST;
  pMode->MaxMCastFilterCount = MAX_MCAST_FILTER_CNT;
  pMode->MCastFilterCount = 0;
  SetMem ( &pMode->BroadcastAddress,
           PXE_HWADDR_LEN_ETHER,
           0xff );
  pMode->IfType = EfiNetworkInterfaceUndi;
  pMode->MacAddressChangeable = TRUE;
  pMode->MultipleTxSupported = TRUE;
  pMode->MediaPresentSupported = TRUE;
  pMode->MediaPresent = FALSE;

  //
  //  Read the MAC address
  //
  pNicDevice->PhyId = PHY_ID_INTERNAL;
  pNicDevice->b100Mbps = TRUE;
  pNicDevice->bFullDuplex = TRUE;

  Status = gBS->AllocatePool ( EfiRuntimeServicesData, 
                               MAX_BULKIN_SIZE,
                               (VOID **) &pNicDevice->pBulkInBuff);
  if ( EFI_ERROR(Status)) {
    DEBUG (( EFI_D_ERROR, "Memory are not enough\n"));
    return Status;
  }
        
  Status = Ax88772MacAddressGet (
                pNicDevice,
                &pMode->PermanentAddress.Addr[0]);
  if ( !EFI_ERROR ( Status )) {
    //
    //  Display the MAC address
    //
    DEBUG (( DEBUG_MAC_ADDRESS | DEBUG_INFO,
              "MAC: %02x-%02x-%02x-%02x-%02x-%02x\n",
              pMode->PermanentAddress.Addr[0],
              pMode->PermanentAddress.Addr[1],
              pMode->PermanentAddress.Addr[2],
              pMode->PermanentAddress.Addr[3],
              pMode->PermanentAddress.Addr[4],
              pMode->PermanentAddress.Addr[5]));

    //
    //  Use the hardware address as the current address
    //
    CopyMem ( &pMode->CurrentAddress,
              &pMode->PermanentAddress,
              PXE_HWADDR_LEN_ETHER );
  }

  //
  //  Return the setup status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  This routine starts the network interface.

  @param [in] pSimpleNetwork    Protocol instance pointer

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_ALREADY_STARTED   The network interface was already started.
  @retval EFI_INVALID_PARAMETER pSimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
EFIAPI
SN_Start (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * pSimpleNetwork
  )
{
  NIC_DEVICE * pNicDevice;
  EFI_SIMPLE_NETWORK_MODE * pMode;
  EFI_STATUS Status;
  
  DBG_ENTER ( );
  
  //
  // Verify the parameters
  //
  Status = EFI_INVALID_PARAMETER;
  if (( NULL != pSimpleNetwork ) && ( NULL != pSimpleNetwork->Mode )) {
    pMode = pSimpleNetwork->Mode;
    if ( EfiSimpleNetworkStopped == pMode->State ) {
      //
      // Initialize the mode structure
      // NVRAM access is not supported
      //
      ZeroMem ( pMode, sizeof ( *pMode ));
  
      pMode->State = EfiSimpleNetworkStarted;
      pMode->HwAddressSize = PXE_HWADDR_LEN_ETHER;
      pMode->MediaHeaderSize = sizeof ( ETHERNET_HEADER );
      pMode->MaxPacketSize = MAX_ETHERNET_PKT_SIZE;
      pMode->ReceiveFilterMask = EFI_SIMPLE_NETWORK_RECEIVE_UNICAST
                               | EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST
                               | EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST
                               | EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS
                               | EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST;
      pMode->ReceiveFilterSetting = EFI_SIMPLE_NETWORK_RECEIVE_UNICAST;
      pMode->MaxMCastFilterCount = MAX_MCAST_FILTER_CNT;
      pNicDevice = DEV_FROM_SIMPLE_NETWORK ( pSimpleNetwork );
      Status = Ax88772MacAddressGet ( pNicDevice, &pMode->PermanentAddress.Addr[0]);
      CopyMem ( &pMode->CurrentAddress,
                &pMode->PermanentAddress,
                sizeof ( pMode->CurrentAddress ));
      pMode->BroadcastAddress.Addr[0] = 0xff;
      pMode->BroadcastAddress.Addr[1] = 0xff;
      pMode->BroadcastAddress.Addr[2] = 0xff;
      pMode->BroadcastAddress.Addr[3] = 0xff;
      pMode->BroadcastAddress.Addr[4] = 0xff;
      pMode->BroadcastAddress.Addr[5] = 0xff;
      pMode->IfType = 1;
      pMode->MacAddressChangeable = TRUE;
      pMode->MultipleTxSupported = TRUE;
      pMode->MediaPresentSupported = TRUE;
      pMode->MediaPresent = FALSE;
    }
    else {
      Status = EFI_ALREADY_STARTED;
    }
  }
  
  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Set the MAC address.
  
  This function modifies or resets the current station address of a
  network interface.  If Reset is TRUE, then the current station address
  is set ot the network interface's permanent address.  If Reset if FALSE
  then the current station address is changed to the address specified by
  pNew.

  This routine calls ::Ax88772MacAddressSet to update the MAC address
  in the network adapter.

  @param [in] pSimpleNetwork    Protocol instance pointer
  @param [in] bReset            Flag used to reset the station address to the
                                network interface's permanent address.
  @param [in] pNew              New station address to be used for the network
                                interface.

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_INVALID_PARAMETER pSimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
EFIAPI
SN_StationAddress (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * pSimpleNetwork,
  IN BOOLEAN bReset,
  IN EFI_MAC_ADDRESS * pNew
  )
{
  NIC_DEVICE * pNicDevice;
  EFI_SIMPLE_NETWORK_MODE * pMode;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  // Verify the parameters
  //
  if (( NULL != pSimpleNetwork )
    && ( NULL != pSimpleNetwork->Mode )
    && (( !bReset ) || ( bReset && ( NULL != pNew )))) {
    //
    // Verify that the adapter is already started
    //
    pNicDevice = DEV_FROM_SIMPLE_NETWORK ( pSimpleNetwork );
    pMode = pSimpleNetwork->Mode;
    if ( EfiSimpleNetworkStarted == pMode->State ) {
      //
      // Determine the adapter MAC address
      //
      if ( bReset ) {
        //
        // Use the permanent address
        //
        CopyMem ( &pMode->CurrentAddress,
                  &pMode->PermanentAddress,
                  sizeof ( pMode->CurrentAddress ));
      }
      else {
        //
        // Use the specified address
        //
        CopyMem ( &pMode->CurrentAddress,
                  pNew,
                  sizeof ( pMode->CurrentAddress ));
      }

      //
      // Update the address on the adapter
      //
      Status = Ax88772MacAddressSet ( pNicDevice, &pMode->CurrentAddress.Addr[0]);
    }
    else {
      Status = EFI_NOT_STARTED;
    }
  }
  else {
    Status = EFI_INVALID_PARAMETER;
  }

  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  This function resets or collects the statistics on a network interface.
  If the size of the statistics table specified by StatisticsSize is not
  big enough for all of the statistics that are collected by the network
  interface, then a partial buffer of statistics is returned in
  StatisticsTable.

  @param [in] pSimpleNetwork    Protocol instance pointer
  @param [in] bReset            Set to TRUE to reset the statistics for the network interface.
  @param [in, out] pStatisticsSize  On input the size, in bytes, of StatisticsTable.  On output
                                the size, in bytes, of the resulting table of statistics.
  @param [out] pStatisticsTable A pointer to the EFI_NETWORK_STATISTICS structure that
                                conains the statistics.

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_BUFFER_TOO_SMALL  The pStatisticsTable is NULL or the buffer is too small.
  @retval EFI_INVALID_PARAMETER pSimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
EFIAPI
SN_Statistics (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * pSimpleNetwork,
  IN BOOLEAN bReset,
  IN OUT UINTN * pStatisticsSize,
  OUT EFI_NETWORK_STATISTICS * pStatisticsTable
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  // This is not currently supported
  //
  Status = EFI_UNSUPPORTED;

  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  This function stops a network interface.  This call is only valid
  if the network interface is in the started state.

  @param [in] pSimpleNetwork    Protocol instance pointer

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_INVALID_PARAMETER pSimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
EFIAPI
SN_Stop (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * pSimpleNetwork
  )
{
  EFI_SIMPLE_NETWORK_MODE * pMode;
  EFI_STATUS Status;
  
  DBG_ENTER ( );
  
  //
  // Verify the parameters
  //
  if (( NULL != pSimpleNetwork ) && ( NULL != pSimpleNetwork->Mode )) {
    //
    // Determine if the interface is started
    //
    pMode = pSimpleNetwork->Mode;
    if ( EfiSimpleNetworkStopped != pMode->State ) {
      if ( EfiSimpleNetworkStarted == pMode->State ) {
        //
        //  Release the resources acquired in SN_Start
        //

        //
        //  Mark the adapter as stopped
        //
        pMode->State = EfiSimpleNetworkStopped;
        Status = EFI_SUCCESS;
      }
      else {
        Status = EFI_UNSUPPORTED;
      }
    }
    else {
      Status = EFI_NOT_STARTED;
    }
  }
  else {
    Status = EFI_INVALID_PARAMETER;
  }
  
  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  This function releases the memory buffers assigned in the Initialize() call.
  Pending transmits and receives are lost, and interrupts are cleared and disabled.
  After this call, only Initialize() and Stop() calls may be used.

  @param [in] pSimpleNetwork    Protocol instance pointer

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_INVALID_PARAMETER pSimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
EFIAPI
SN_Shutdown (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * pSimpleNetwork
  )
{
  EFI_SIMPLE_NETWORK_MODE * pMode;
  UINT32 RxFilter;
  EFI_STATUS Status;
  
  DBG_ENTER ( );
  
  //
  // Verify the parameters
  //
  if (( NULL != pSimpleNetwork ) && ( NULL != pSimpleNetwork->Mode )) {
    //
    // Determine if the interface is already started
    //
    pMode = pSimpleNetwork->Mode;
    if ( EfiSimpleNetworkInitialized == pMode->State ) {
      //
      // Stop the adapter
      //
      RxFilter = pMode->ReceiveFilterSetting;
      pMode->ReceiveFilterSetting = 0;
      Status = SN_Reset ( pSimpleNetwork, FALSE );
      pMode->ReceiveFilterSetting = RxFilter;
      if ( !EFI_ERROR ( Status )) {
        //
        // Release the resources acquired by SN_Initialize
        //

        //
        // Update the network state
        //
        pMode->State = EfiSimpleNetworkStarted;
      }
    }
    else {
      Status = EFI_NOT_STARTED;
    }
  }
  else {
    Status = EFI_INVALID_PARAMETER;
  }
  
  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Send a packet over the network.

  This function places the packet specified by Header and Buffer on
  the transmit queue.  This function performs a non-blocking transmit
  operation.  When the transmit is complete, the buffer is returned
  via the GetStatus() call.

  This routine calls ::Ax88772Rx to empty the network adapter of
  receive packets.  The routine then passes the transmit packet
  to the network adapter.

  @param [in] pSimpleNetwork    Protocol instance pointer
  @param [in] HeaderSize        The size, in bytes, of the media header to be filled in by
                                the Transmit() function.  If HeaderSize is non-zero, then
                                it must be equal to SimpleNetwork->Mode->MediaHeaderSize
                                and DestAddr and Protocol parameters must not be NULL.
  @param [in] BufferSize        The size, in bytes, of the entire packet (media header and
                                data) to be transmitted through the network interface.
  @param [in] pBuffer           A pointer to the packet (media header followed by data) to
                                to be transmitted.  This parameter can not be NULL.  If
                                HeaderSize is zero, then the media header is Buffer must
                                already be filled in by the caller.  If HeaderSize is nonzero,
                                then the media header will be filled in by the Transmit()
                                function.
  @param [in] pSrcAddr          The source HW MAC address.  If HeaderSize is zero, then
                                this parameter is ignored.  If HeaderSize is nonzero and
                                SrcAddr is NULL, then SimpleNetwork->Mode->CurrentAddress
                                is used for the source HW MAC address.
  @param [in] pDestAddr         The destination HW MAC address.  If HeaderSize is zero, then
                                this parameter is ignored.
  @param [in] pProtocol         The type of header to build.  If HeaderSize is zero, then
                                this parameter is ignored.

  @retval EFI_SUCCESS           This operation was successful.
  @retval EFI_NOT_STARTED       The network interface was not started.
  @retval EFI_NOT_READY         The network interface is too busy to accept this transmit request.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize parameter is too small.
  @retval EFI_INVALID_PARAMETER pSimpleNetwork parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.

**/
EFI_STATUS
EFIAPI
SN_Transmit (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * pSimpleNetwork,
  IN UINTN HeaderSize,
  IN UINTN BufferSize,
  IN VOID * pBuffer,
  IN EFI_MAC_ADDRESS * pSrcAddr,
  IN EFI_MAC_ADDRESS * pDestAddr,
  IN UINT16 * pProtocol
  )
{
  RX_TX_PACKET Packet;
  ETHERNET_HEADER * pHeader;
  EFI_SIMPLE_NETWORK_MODE * pMode;
  NIC_DEVICE * pNicDevice;
  EFI_USB_IO_PROTOCOL * pUsbIo;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;
  UINTN TransferLength;
  UINT32 TransferStatus;
  UINT16 Type;

  DBG_ENTER ( );

  //
  // Verify the parameters
  //
  if (( NULL != pSimpleNetwork ) && ( NULL != pSimpleNetwork->Mode )) {
    //
    // The interface must be running
    //
    pMode = pSimpleNetwork->Mode;
    if ( EfiSimpleNetworkInitialized == pMode->State ) {
      //
      //  Synchronize with Ax88772Timer
      //
      VERIFY_TPL ( TPL_AX88772 );
      TplPrevious = gBS->RaiseTPL ( TPL_AX88772 );

      //
      // Update the link status
      //
      pNicDevice = DEV_FROM_SIMPLE_NETWORK ( pSimpleNetwork );

      //
      //No need to call receive to receive packet
      //
      //Ax88772Rx ( pNicDevice, FALSE );
      pMode->MediaPresent = pNicDevice->bLinkUp;

      //
      //  Release the synchronization with Ax88772Timer
      //
      gBS->RestoreTPL ( TplPrevious );
      if ( pMode->MediaPresent ) {
        //
        //  Copy the packet into the USB buffer
        //
        CopyMem ( &Packet.Data[0], pBuffer, BufferSize );
        Packet.Length = (UINT16) BufferSize;

        //
        //  Transmit the packet
        //
        pHeader = (ETHERNET_HEADER *) &Packet.Data[0];
        if ( 0 != HeaderSize ) {
          if ( NULL != pDestAddr ) {
            CopyMem ( &pHeader->dest_addr, pDestAddr, PXE_HWADDR_LEN_ETHER );
          }
          if ( NULL != pSrcAddr ) {
            CopyMem ( &pHeader->src_addr, pSrcAddr, PXE_HWADDR_LEN_ETHER );
          }
          else {
            CopyMem ( &pHeader->src_addr, &pMode->CurrentAddress.Addr[0], PXE_HWADDR_LEN_ETHER );
          }
          if ( NULL != pProtocol ) {
            Type = *pProtocol;
          }
          else {
            Type = Packet.Length;
          }
          Type = (UINT16)(( Type >> 8 ) | ( Type << 8 ));
          pHeader->type = Type;
        }
        if ( Packet.Length < MIN_ETHERNET_PKT_SIZE ) {
          Packet.Length = MIN_ETHERNET_PKT_SIZE;
          ZeroMem ( &Packet.Data[ BufferSize ],
                    Packet.Length - BufferSize );
        }
        DEBUG (( DEBUG_TX | DEBUG_INFO,
                  "TX: %02x-%02x-%02x-%02x-%02x-%02x  %02x-%02x-%02x-%02x-%02x-%02x  %02x-%02x  %d bytes\r\n",
                  Packet.Data[0],
                  Packet.Data[1],
                  Packet.Data[2],
                  Packet.Data[3],
                  Packet.Data[4],
                  Packet.Data[5],
                  Packet.Data[6],
                  Packet.Data[7],
                  Packet.Data[8],
                  Packet.Data[9],
                  Packet.Data[10],
                  Packet.Data[11],
                  Packet.Data[12],
                  Packet.Data[13],
                  Packet.Length ));
        Packet.LengthBar = ~Packet.Length;
        TransferLength = sizeof ( Packet.Length )
                       + sizeof ( Packet.LengthBar )
                       + Packet.Length;

        //
        //  Work around USB bus driver bug where a timeout set by receive
        //  succeeds but the timeout expires immediately after, causing the
        //  transmit operation to timeout.
        //
        pUsbIo = pNicDevice->pUsbIo;
        Status = pUsbIo->UsbBulkTransfer ( pUsbIo,
                                           BULK_OUT_ENDPOINT,
                                           &Packet.Length,
                                           &TransferLength,
                                           0xfffffffe,
                                           &TransferStatus );
        if ( !EFI_ERROR ( Status )) {
          Status = TransferStatus;
        }
        if (( !EFI_ERROR ( Status ))
          && ( TransferLength != (UINTN)( Packet.Length + 4 ))) {
          Status = EFI_WARN_WRITE_FAILURE;
        }
        if ( EFI_SUCCESS == Status ) {
          pNicDevice->pTxBuffer = pBuffer;
        }
        else {
          DEBUG (( DEBUG_ERROR | DEBUG_INFO,
                    "Ax88772 USB transmit error, TransferLength: %d, Status: %r\r\n",
                    sizeof ( Packet.Length ) + Packet.Length,
                    Status ));
          //
          //  Reset the controller to fix the error
          //
          if ( EFI_DEVICE_ERROR == Status ) {
            SN_Reset ( pSimpleNetwork, FALSE );
          }
        }
      }
      else {
        //
        // No packets available.
        //
        Status = EFI_NOT_READY;
      }
    }
    else {
      Status = EFI_NOT_STARTED;
    }
  }
  else {
    DEBUG (( DEBUG_ERROR | DEBUG_INFO,
              "Ax88772 invalid transmit parameter\r\n"
              "  0x%08x: HeaderSize\r\n"
              "  0x%08x: BufferSize\r\n"
              "  0x%08x: Buffer\r\n"
              "  0x%08x: SrcAddr\r\n"
              "  0x%08x: DestAddr\r\n"
              "  0x%04x:     Protocol\r\n",
              HeaderSize,
              BufferSize,
              pBuffer,
              pSrcAddr,
              pDestAddr,
              pProtocol ));
    Status = EFI_INVALID_PARAMETER;
  }

  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}
