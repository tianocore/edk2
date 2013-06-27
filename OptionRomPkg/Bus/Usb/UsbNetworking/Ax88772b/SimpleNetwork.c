/** @file
  Provides the Simple Network functions.

  Copyright (c) 2011 - 2013, Intel Corporation
  All rights reserved. This program and the accompanying materials
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

  //
  // Set the MAC address
  //
  pNicDevice = DEV_FROM_SIMPLE_NETWORK ( pSimpleNetwork );
  pMode = pSimpleNetwork->Mode;

  //
  // Clear the multicast hash table
  //
  Ax88772MulticastClear ( pNicDevice );

  //
  // Load the multicast hash table
  //
  if ( 0 != ( pMode->ReceiveFilterSetting & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST )) {
      for ( Index = 0; Index < pMode->MCastFilterCount; Index++ ) {
        //
        // Enable the next multicast address
        //
        Ax88772MulticastSet ( pNicDevice,
                              &pMode->MCastFilter[ Index ].Addr[0]);
      }
  }

  Status = Ax88772RxControl ( pNicDevice, pMode->ReceiveFilterSetting );

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
  EFI_SIMPLE_NETWORK_MODE * pMode;
  NIC_DEVICE * pNicDevice;
  EFI_STATUS Status;
  BOOLEAN bFullDuplex;
  BOOLEAN bLinkUp;
  BOOLEAN bSpeed100;
  EFI_TPL TplPrevious;
 
  TplPrevious = gBS->RaiseTPL(TPL_CALLBACK);
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
    if ( EfiSimpleNetworkInitialized == pMode->State ) {

      if ( pNicDevice->LinkIdleCnt > MAX_LINKIDLE_THRESHOLD) {

          bLinkUp = pNicDevice->bLinkUp;
          bSpeed100 = pNicDevice->b100Mbps;
          bFullDuplex = pNicDevice->bFullDuplex;
          Status = Ax88772NegotiateLinkComplete ( pNicDevice,
                                            &pNicDevice->PollCount,
                                            &pNicDevice->bComplete,
                                            &pNicDevice->bLinkUp,
                                            &pNicDevice->b100Mbps,
                                            &pNicDevice->bFullDuplex );

          //
          // Determine if the autonegotiation is complete
          //
          if ( pNicDevice->bComplete ) {
              if ( pNicDevice->bLinkUp ) {
                  if (( bSpeed100 && ( !pNicDevice->b100Mbps ))
                      || (( !bSpeed100 ) && pNicDevice->b100Mbps )
                      || ( bFullDuplex && ( !pNicDevice->bFullDuplex ))
                      || (( !bFullDuplex ) && pNicDevice->bFullDuplex )) {
                          pNicDevice->PollCount = 0;
                          DEBUG (( EFI_D_INFO , "Reset to establish proper link setup: %d Mbps, %a duplex\r\n",
                                    pNicDevice->b100Mbps ? 100 : 10, pNicDevice->bFullDuplex ? "Full" : "Half"));
                          Status = SN_Reset ( &pNicDevice->SimpleNetwork, FALSE );
                  }
                  if (( !bLinkUp ) && pNicDevice->bLinkUp ) {
                      //
                      // Display the autonegotiation status
                      //
                      DEBUG (( EFI_D_INFO , "Link: Up, %d Mbps, %a duplex\r\n",
                                pNicDevice->b100Mbps ? 100 : 10, pNicDevice->bFullDuplex ? "Full" : "Half"));

                  }
                  pNicDevice->LinkIdleCnt = 0;
            }
        }
        //
        //  Update the link status
        //
        if ( bLinkUp && ( !pNicDevice->bLinkUp )) {
            DEBUG (( EFI_D_INFO , "Link: Down\r\n"));
        }
      }

      pMode->MediaPresent = pNicDevice->bLinkUp;
      //
      // Return the interrupt status
      //
      if ( NULL != pInterruptStatus ) {
        *pInterruptStatus = 0;
      }   
      Status = EFI_SUCCESS;
    }
    else {
      if ( EfiSimpleNetworkStarted == pMode->State ) {
        Status = EFI_DEVICE_ERROR;
      }
      else {
        Status = EFI_NOT_STARTED;
      }
    }
      
  }
  else {
    Status = EFI_INVALID_PARAMETER;
  }
  gBS->RestoreTPL(TplPrevious) ;

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
  UINT32  TmpState;
   EFI_TPL TplPrevious;
   
   TplPrevious = gBS->RaiseTPL (TPL_CALLBACK);
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
        TmpState = pMode->State;
        pMode->State = EfiSimpleNetworkInitialized;
        Status = SN_Reset ( pSimpleNetwork, FALSE );
        if ( EFI_ERROR ( Status )) {
          //
          // Update the network state
          //
          pMode->State = TmpState;
          DEBUG (( EFI_D_ERROR , "SN_reset failed\n"));
        }
      }
      else {
        DEBUG (( EFI_D_ERROR , "Increase ExtraRxBufferSize = %d ExtraTxBufferSize=%d\n", 
              ExtraRxBufferSize, ExtraTxBufferSize));
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
  gBS->RestoreTPL (TplPrevious);

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
  OUT EFI_MAC_ADDRESS * pMAC
  )
{
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  TplPrevious = gBS->RaiseTPL(TPL_CALLBACK);
  //
  // Get pointer to SNP driver instance for *this.
  //
  if (pSimpleNetwork == NULL) {
    gBS->RestoreTPL(TplPrevious);
    return EFI_INVALID_PARAMETER;
  }

  if (pIP == NULL || pMAC == NULL) {
    gBS->RestoreTPL(TplPrevious);
    return EFI_INVALID_PARAMETER;
  }

  if (bIPv6){
    Status = EFI_UNSUPPORTED;
  }  
  else {
      //
      // check if the ip given is a mcast IP
      //
      if ((pIP->v4.Addr[0] & 0xF0) != 0xE0) {
        gBS->RestoreTPL(TplPrevious);
        return EFI_INVALID_PARAMETER;
      }
      else {
        if (pSimpleNetwork->Mode->State == EfiSimpleNetworkInitialized)
        {
          pMAC->Addr[0] = 0x01;
          pMAC->Addr[1] = 0x00;
          pMAC->Addr[2] = 0x5e;
          pMAC->Addr[3] = (UINT8) (pIP->v4.Addr[1] & 0x7f);
          pMAC->Addr[4] = (UINT8) pIP->v4.Addr[2];
          pMAC->Addr[5] = (UINT8) pIP->v4.Addr[3];
          Status = EFI_SUCCESS;
        }
        else if (pSimpleNetwork->Mode->State == EfiSimpleNetworkStarted) {
          Status = EFI_DEVICE_ERROR;
        }
        else {
          Status = EFI_NOT_STARTED;
        }
        gBS->RestoreTPL(TplPrevious);
      }
  }
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
  //
  // This is not currently supported
  //
  Status = EFI_UNSUPPORTED;
  return Status;
}

VOID 
FillPkt2Queue (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * pSimpleNetwork,
  IN UINTN BufLength)
{

  UINT16 * pLength;
  UINT16 * pLengthBar;
  UINT8* pData;
  UINT32 offset;
  NIC_DEVICE * pNicDevice;
  
  pNicDevice = DEV_FROM_SIMPLE_NETWORK ( pSimpleNetwork);
  for ( offset = 0; offset < BufLength; ){
      pLength = (UINT16*) (pNicDevice->pBulkInBuff + offset);
      pLengthBar = (UINT16*) (pNicDevice->pBulkInBuff + offset +2);
      
      *pLength &= 0x7ff;
      *pLengthBar &= 0x7ff;
      *pLengthBar |= 0xf800;
      
      if ((*pLength ^ *pLengthBar ) != 0xFFFF) {
          DEBUG (( EFI_D_ERROR , "Pkt length error. BufLength = %d\n", BufLength));
          return;
      }          
      
      if (TRUE == pNicDevice->pNextFill->f_Used) {
        return;
      }
      else {
          pData = pNicDevice->pBulkInBuff + offset + 4;
          pNicDevice->pNextFill->f_Used = TRUE;
          pNicDevice->pNextFill->Length = *pLength;
          CopyMem (&pNicDevice->pNextFill->Data[0], pData, *pLength);
          
          pNicDevice->pNextFill = pNicDevice->pNextFill->pNext;
          offset += ((*pLength + HW_HDR_LENGTH - 1) &~3) + 1;
          pNicDevice->PktCntInQueue++;
      }
              
  }
}

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
  EFI_SIMPLE_NETWORK_MODE * pMode;
  NIC_DEVICE * pNicDevice;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;
  UINT16 Type;
  EFI_USB_IO_PROTOCOL *pUsbIo;
  UINTN LengthInBytes;
  UINT32 TransferStatus;
  RX_PKT * pFirstFill;
  TplPrevious = gBS->RaiseTPL (TPL_CALLBACK);
  
  //
  // Verify the parameters
  //
  if (( NULL != pSimpleNetwork ) && 
    ( NULL != pSimpleNetwork->Mode ) && 
    (NULL != pBufferSize) && 
    (NULL != pBuffer)) {
    //
    // The interface must be running
    //
    pMode = pSimpleNetwork->Mode;
    if ( EfiSimpleNetworkInitialized == pMode->State ) {
      //
      // Update the link status
      //
      pNicDevice = DEV_FROM_SIMPLE_NETWORK ( pSimpleNetwork );
      pNicDevice->LinkIdleCnt++;
      pMode->MediaPresent = pNicDevice->bLinkUp;
      
      if ( pMode->MediaPresent && pNicDevice->bComplete) {
      
      
        if (pNicDevice->PktCntInQueue != 0 ) {
            DEBUG (( EFI_D_INFO, "pNicDevice->PktCntInQueue = %d\n",
                pNicDevice->PktCntInQueue));
        }
        
        LengthInBytes = MAX_BULKIN_SIZE;
        if (pNicDevice->PktCntInQueue == 0 ){
            //
            // Attempt to do bulk in
            //
            SetMem (&pNicDevice->pBulkInBuff[0], 4, 0);
            pUsbIo = pNicDevice->pUsbIo;
            Status = pUsbIo->UsbBulkTransfer ( pUsbIo,
                                       USB_ENDPOINT_DIR_IN | BULK_IN_ENDPOINT,
                                       &pNicDevice->pBulkInBuff[0],
                                       &LengthInBytes,
                                       BULKIN_TIMEOUT,
                                       &TransferStatus );
                                       
            if (LengthInBytes != 0 && !EFI_ERROR(Status) && !EFI_ERROR(TransferStatus) ){
                FillPkt2Queue(pSimpleNetwork, LengthInBytes);
            }
        }
        
        pFirstFill = pNicDevice->pFirstFill;
         
        if (TRUE == pFirstFill->f_Used) {
            ETHERNET_HEADER * pHeader;
            pNicDevice->LinkIdleCnt = 0;
            CopyMem (pBuffer,  &pFirstFill->Data[0], pFirstFill->Length);
            pHeader = (ETHERNET_HEADER *) &pFirstFill->Data[0];
                     
            DEBUG (( EFI_D_INFO, "RX: %02x-%02x-%02x-%02x-%02x-%02x " 
                      "%02x-%02x-%02x-%02x-%02x-%02x  %02x-%02x  %d bytes\r\n",
                      pFirstFill->Data[0],
                      pFirstFill->Data[1],
                      pFirstFill->Data[2],
                      pFirstFill->Data[3],
                      pFirstFill->Data[4],
                      pFirstFill->Data[5],
                      pFirstFill->Data[6],
                      pFirstFill->Data[7],
                      pFirstFill->Data[8],
                      pFirstFill->Data[9],
                      pFirstFill->Data[10],
                      pFirstFill->Data[11],
                      pFirstFill->Data[12],
                      pFirstFill->Data[13],
                      pFirstFill->Length));   
            
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
            if (*pBufferSize < pFirstFill->Length) {
                  DEBUG (( EFI_D_ERROR, "RX: Buffer was too small"));
                  Status = EFI_BUFFER_TOO_SMALL;
            }
            *pBufferSize =  pFirstFill->Length;
            pFirstFill->f_Used = FALSE;
            pNicDevice->pFirstFill = pFirstFill->pNext;
            pNicDevice->PktCntInQueue--;
        }
        else {
            pNicDevice->LinkIdleCnt++;
            Status = EFI_NOT_READY;
        }
      }
      else {
        //
        //  Link no up
        //
        pNicDevice->LinkIdleCnt++;
        Status = EFI_NOT_READY; 
      }
      
    }
    else {
      if (EfiSimpleNetworkStarted == pMode->State) {
        Status = EFI_DEVICE_ERROR;
      }
      else {
        Status = EFI_NOT_STARTED;
      }
    }
  }
  else {
    Status = EFI_INVALID_PARAMETER;
  }                              
  gBS->RestoreTPL (TplPrevious);
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
/*
#define EFI_SIMPLE_NETWORK_RECEIVE_UNICAST               0x01
#define EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST             0x02
#define EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST             0x04
#define EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS           0x08
#define EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST 0x10
*/
  IN BOOLEAN bResetMCastFilter,
  IN UINTN MCastFilterCnt,
  IN EFI_MAC_ADDRESS * pMCastFilter
  )
{
  EFI_SIMPLE_NETWORK_MODE * pMode;
  EFI_STATUS Status = EFI_SUCCESS;   
  EFI_TPL TplPrevious; 
  NIC_DEVICE * pNicDevice;

  TplPrevious = gBS->RaiseTPL(TPL_CALLBACK);
  pNicDevice = DEV_FROM_SIMPLE_NETWORK ( pSimpleNetwork );
  pMode = pSimpleNetwork->Mode;

  if (pSimpleNetwork == NULL) {
    gBS->RestoreTPL(TplPrevious);
    return EFI_INVALID_PARAMETER;
  }

  switch (pMode->State) {
    case EfiSimpleNetworkInitialized:
      break;
    case EfiSimpleNetworkStopped:
      Status = EFI_NOT_STARTED;
      gBS->RestoreTPL(TplPrevious);
      return Status;
    default:
      Status = EFI_DEVICE_ERROR;
      gBS->RestoreTPL(TplPrevious);
      return Status;
  }

  //
  // check if we are asked to enable or disable something that the UNDI
  // does not even support!
  //
  if (((Enable &~pMode->ReceiveFilterMask) != 0) ||
    ((Disable &~pMode->ReceiveFilterMask) != 0)) {
    Status = EFI_INVALID_PARAMETER;
    gBS->RestoreTPL(TplPrevious);
    return Status;
  }
  
  if (bResetMCastFilter) {
    Disable |= (EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST & pMode->ReceiveFilterMask);
      pMode->MCastFilterCount = 0;
      if ( (0 == (pMode->ReceiveFilterSetting & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST)) 
            && Enable == 0 && Disable == 2) {
            gBS->RestoreTPL(TplPrevious);
            return EFI_SUCCESS;
      }
  } 
  else {
    if (MCastFilterCnt != 0) {
      UINTN i; 
      EFI_MAC_ADDRESS * pMulticastAddress;
      pMulticastAddress =  pMCastFilter;
      
      if ((MCastFilterCnt > pMode->MaxMCastFilterCount) ||
          (pMCastFilter == NULL)) {
        Status = EFI_INVALID_PARAMETER;
        gBS->RestoreTPL(TplPrevious);
        return Status;
      }
      
      for ( i = 0 ; i < MCastFilterCnt ; i++ ) {
          UINT8  tmp;
          tmp = pMulticastAddress->Addr[0];
          if ( (tmp & 0x01) != 0x01 ) {
            gBS->RestoreTPL(TplPrevious);
            return EFI_INVALID_PARAMETER;
          }
          pMulticastAddress++;
      }
      
      pMode->MCastFilterCount = (UINT32)MCastFilterCnt;
      CopyMem (&pMode->MCastFilter[0],
                     pMCastFilter,
                     MCastFilterCnt * sizeof ( EFI_MAC_ADDRESS));
    }
  }
  
  if (Enable == 0 && Disable == 0 && !bResetMCastFilter && MCastFilterCnt == 0) {
    Status = EFI_SUCCESS;
    gBS->RestoreTPL(TplPrevious);
    return Status;
  }

  if ((Enable & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST) != 0 && MCastFilterCnt == 0) {
    Status = EFI_INVALID_PARAMETER;
    gBS->RestoreTPL(TplPrevious);
    return Status;
  }
  
  pMode->ReceiveFilterSetting |= Enable;
  pMode->ReceiveFilterSetting &= ~Disable;
  Status = ReceiveFilterUpdate (pSimpleNetwork);
  
  if (EFI_DEVICE_ERROR == Status || EFI_INVALID_PARAMETER == Status)
      Status = EFI_SUCCESS;

  gBS->RestoreTPL(TplPrevious);
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
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  TplPrevious = gBS->RaiseTPL(TPL_CALLBACK);
  //
  //  Verify the parameters
  //
  if (( NULL != pSimpleNetwork ) && ( NULL != pSimpleNetwork->Mode )) {
		pMode = pSimpleNetwork->Mode;
		if ( EfiSimpleNetworkInitialized == pMode->State ) {
    	//
    	//  Update the device state
    	//
    	pNicDevice = DEV_FROM_SIMPLE_NETWORK ( pSimpleNetwork );
    	pNicDevice->bComplete = FALSE;
    	pNicDevice->bLinkUp = FALSE; 
    	pNicDevice->bHavePkt = FALSE;
    	pMode = pSimpleNetwork->Mode;
    	pMode->MediaPresent = FALSE;

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
   	}
   	else {
      if (EfiSimpleNetworkStarted == pMode->State) {
        Status = EFI_DEVICE_ERROR;
      }
      else {
        Status = EFI_NOT_STARTED;
      }
   	}  
  }
  else {
    Status = EFI_INVALID_PARAMETER;
  }
  gBS->RestoreTPL ( TplPrevious );
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
  RX_PKT * pCurr = NULL;
  RX_PKT * pPrev = NULL;

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
  pMode->MultipleTxSupported = FALSE;
  pMode->MediaPresentSupported = TRUE;
  pMode->MediaPresent = FALSE;
  pNicDevice->LinkIdleCnt = 0;
  //
  //  Read the MAC address
  //
  pNicDevice->PhyId = PHY_ID_INTERNAL;
  pNicDevice->b100Mbps = TRUE;
  pNicDevice->bFullDuplex = TRUE;
  
  Status = Ax88772MacAddressGet (
                pNicDevice,
                &pMode->PermanentAddress.Addr[0]);

  if ( !EFI_ERROR ( Status )) {
    int i; 
    //
    //  Use the hardware address as the current address
    //

    CopyMem ( &pMode->CurrentAddress,
              &pMode->PermanentAddress,
              PXE_HWADDR_LEN_ETHER );
              
    CopyMem ( &pNicDevice->MAC,
              &pMode->PermanentAddress,
              PXE_HWADDR_LEN_ETHER );
              
    pNicDevice->PktCntInQueue = 0;
    
    for ( i = 0 ; i < MAX_QUEUE_SIZE ; i++) {
        Status = gBS->AllocatePool ( EfiRuntimeServicesData, 
                                      sizeof (RX_PKT),
                                      (VOID **) &pCurr);
        if ( EFI_ERROR(Status)) {
            DEBUG (( EFI_D_ERROR, "Memory are not enough\n"));
            return Status;
        }                              
        pCurr->f_Used = FALSE;
        
        if ( i ) {
            pPrev->pNext = pCurr;
        }
        else {
            pNicDevice->QueueHead = pCurr;
        }
        
        if (MAX_QUEUE_SIZE - 1 == i) {
            pCurr->pNext = pNicDevice->QueueHead;
        }
        
        pPrev = pCurr;
    }
    
    pNicDevice->pNextFill = pNicDevice->QueueHead;
    pNicDevice->pFirstFill = pNicDevice->QueueHead;
    
    Status = gBS->AllocatePool (EfiRuntimeServicesData,
                                MAX_BULKIN_SIZE,
                                (VOID **) &pNicDevice->pBulkInBuff);
                                
    if (EFI_ERROR(Status)) {
        DEBUG (( EFI_D_ERROR, "gBS->AllocatePool for pBulkInBuff error. Status = %r\n",
              Status));
        return Status;
    }
  }
  else {
    DEBUG (( EFI_D_ERROR, "Ax88772MacAddressGet error. Status = %r\n", Status));
		return Status;
  }
  
  Status = gBS->AllocatePool ( EfiRuntimeServicesData,
                                   sizeof ( RX_TX_PACKET ),
                                   (VOID **) &pNicDevice->pRxTest );

  if (EFI_ERROR (Status)) {
    DEBUG (( EFI_D_ERROR, "gBS->AllocatePool:pNicDevice->pRxTest error. Status = %r\n",
              Status));
	  return Status;
  }
                                   
  Status = gBS->AllocatePool ( EfiRuntimeServicesData,
                                   sizeof ( RX_TX_PACKET ),
                                   (VOID **) &pNicDevice->pTxTest );

  if (EFI_ERROR (Status)) {
    DEBUG (( EFI_D_ERROR, "gBS->AllocatePool:pNicDevice->pTxTest error. Status = %r\n",
              Status));
	  gBS->FreePool (pNicDevice->pRxTest);
  }

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
  EFI_TPL TplPrevious;
  int i = 0;
  RX_PKT * pCurr = NULL;

  TplPrevious = gBS->RaiseTPL(TPL_CALLBACK);
  //
  // Verify the parameters
  //
  Status = EFI_INVALID_PARAMETER;
  if (( NULL != pSimpleNetwork ) && ( NULL != pSimpleNetwork->Mode )) {
    pMode = pSimpleNetwork->Mode;
    if ( EfiSimpleNetworkStopped == pMode->State ) {
      //
      // Initialize the mode structuref
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
      SetMem(&pMode->BroadcastAddress, PXE_HWADDR_LEN_ETHER, 0xff);
      pMode->IfType = EfiNetworkInterfaceUndi;
      pMode->MacAddressChangeable = TRUE;
      pMode->MultipleTxSupported = FALSE;
      pMode->MediaPresentSupported = TRUE;
      pMode->MediaPresent = FALSE; 
      pNicDevice->PktCntInQueue = 0;
      pNicDevice->pNextFill = pNicDevice->QueueHead;
      pNicDevice->pFirstFill = pNicDevice->QueueHead;
      pCurr = pNicDevice->QueueHead;
      
      for ( i = 0 ; i < MAX_QUEUE_SIZE ; i++) { 
        pCurr->f_Used = FALSE;
        pCurr = pCurr->pNext;
      }
      
    }
    else {
      Status = EFI_ALREADY_STARTED;
    }
  }
  gBS->RestoreTPL ( TplPrevious );
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
  EFI_TPL TplPrevious;
  
  TplPrevious = gBS->RaiseTPL(TPL_CALLBACK);
  //
  // Verify the parameters
  //
  if (( NULL != pSimpleNetwork )
    && ( NULL != pSimpleNetwork->Mode )
    && (( bReset ) || ( ( !bReset) && ( NULL != pNew )))) {
    //
    // Verify that the adapter is already started
    //
    pNicDevice = DEV_FROM_SIMPLE_NETWORK ( pSimpleNetwork );
    pMode = pSimpleNetwork->Mode;
    if ( EfiSimpleNetworkInitialized == pMode->State ) {
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
      if (EfiSimpleNetworkStarted == pMode->State) {
        Status = EFI_DEVICE_ERROR;
      }
      else {
        Status = EFI_NOT_STARTED;
      }
    }
  }
  else {
    Status = EFI_INVALID_PARAMETER;
  }
  gBS->RestoreTPL ( TplPrevious );
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

  typedef struct {
  UINT64 RxTotalFrames;
  UINT64 RxGoodFrames;
  UINT64 RxUndersizeFrames;
  UINT64 RxOversizeFrames;
  UINT64 RxDroppedFrames;
  UINT64 RxUnicastFrames;
  UINT64 RxBroadcastFrames;
  UINT64 RxMulticastFrames;
  UINT64 RxCrcErrorFrames;
  UINT64 RxTotalBytes;
  UINT64 TxTotalFrames;
  UINT64 TxGoodFrames;
  UINT64 TxUndersizeFrames;
  UINT64 TxOversizeFrames;
  UINT64 TxDroppedFrames;
  UINT64 TxUnicastFrames;
  UINT64 TxBroadcastFrames;
  UINT64 TxMulticastFrames;
  UINT64 TxCrcErrorFrames;
  UINT64 TxTotalBytes;
  UINT64 Collisions;
  UINT64 UnsupportedProtocol;
  } EFI_NETWORK_STATISTICS;
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
  EFI_SIMPLE_NETWORK_MODE * pMode;
  //
  // Verify the prarameters
  //
  if (( NULL != pSimpleNetwork ) && ( NULL != pSimpleNetwork->Mode )) {
    pMode = pSimpleNetwork->Mode;
    //
    // Determine if the interface is started 
    //
    if (EfiSimpleNetworkInitialized == pMode->State){
      //
      // Determine if the StatisticsSize is big enough
      //
      if (sizeof (EFI_NETWORK_STATISTICS) <= *pStatisticsSize){
        if (bReset) {
          Status = EFI_SUCCESS;
        } 
        else {
          Status = EFI_UNSUPPORTED;
        }
      }
      else {
        Status = EFI_BUFFER_TOO_SMALL;
      }
    }
    else{
      if (EfiSimpleNetworkStarted == pMode->State) {
        Status = EFI_DEVICE_ERROR;
      }
      else {
        Status = EFI_NOT_STARTED;
      }
    }
  }
  else {
  	Status = EFI_INVALID_PARAMETER;
  }

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
  EFI_TPL TplPrevious;
  
  TplPrevious = gBS->RaiseTPL(TPL_CALLBACK);
  //
  // Verify the parameters
  //
  if (( NULL != pSimpleNetwork ) && ( NULL != pSimpleNetwork->Mode )) {
    //
    // Determine if the interface is started
    //
    pMode = pSimpleNetwork->Mode;   
    if ( EfiSimpleNetworkStarted == pMode->State ) {
        pMode->State = EfiSimpleNetworkStopped;
        Status = EFI_SUCCESS; 
    }
    else {
        Status = EFI_NOT_STARTED;
    }
  } 
  else {
    Status = EFI_INVALID_PARAMETER;
  }
  
  gBS->RestoreTPL ( TplPrevious );
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
  EFI_TPL TplPrevious;
  
  TplPrevious = gBS->RaiseTPL(TPL_CALLBACK);
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
        // Update the network state
        //
        pMode->State = EfiSimpleNetworkStarted;
      }
      else if ( EFI_DEVICE_ERROR == Status ) {
      	pMode->State = EfiSimpleNetworkStopped;
      }
    }
    else {
      Status = EFI_NOT_STARTED;
    }
  }
  else {
    Status = EFI_INVALID_PARAMETER;
  }
  gBS->RestoreTPL ( TplPrevious );
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
  ETHERNET_HEADER * pHeader;
  EFI_SIMPLE_NETWORK_MODE * pMode;
  NIC_DEVICE * pNicDevice;
  EFI_USB_IO_PROTOCOL * pUsbIo;
  EFI_STATUS Status;
  UINTN TransferLength;
  UINT32 TransferStatus;
  UINT16 Type;
  EFI_TPL TplPrevious;

  TplPrevious = gBS->RaiseTPL(TPL_CALLBACK);

  // Verify the parameters
  //
  if (( NULL != pSimpleNetwork ) && 
      ( NULL != pSimpleNetwork->Mode ) && 
      ( NULL != pBuffer) && 
      ( (HeaderSize == 0) || ( (NULL != pDestAddr) && (NULL != pProtocol) ))) {
    //
    // The interface must be running
    //
    pMode = pSimpleNetwork->Mode;
    //
    // Verify parameter of HeaderSize
    //
    if ((HeaderSize == 0) || (HeaderSize == pMode->MediaHeaderSize)){
      //
      // Determine if BufferSize is big enough
      //
      if (BufferSize >= pMode->MediaHeaderSize){
        if ( EfiSimpleNetworkInitialized == pMode->State ) {
          //
          // Update the link status
          //
          pNicDevice = DEV_FROM_SIMPLE_NETWORK ( pSimpleNetwork );
          pMode->MediaPresent = pNicDevice->bLinkUp;

          //
          //  Release the synchronization with Ax88772Timer
          //      
          if ( pMode->MediaPresent && pNicDevice->bComplete) {
            //
            //  Copy the packet into the USB buffer
            //

            CopyMem ( &pNicDevice->pTxTest->Data[0], pBuffer, BufferSize ); 
            pNicDevice->pTxTest->Length = (UINT16) BufferSize;

            //
            //  Transmit the packet
            //
            pHeader = (ETHERNET_HEADER *) &pNicDevice->pTxTest->Data[0];
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
                Type = pNicDevice->pTxTest->Length;
              }
              Type = (UINT16)(( Type >> 8 ) | ( Type << 8 ));
              pHeader->type = Type;
            }
            if ( pNicDevice->pTxTest->Length < MIN_ETHERNET_PKT_SIZE ) {
              pNicDevice->pTxTest->Length = MIN_ETHERNET_PKT_SIZE;
              ZeroMem ( &pNicDevice->pTxTest->Data[ BufferSize ],
                        pNicDevice->pTxTest->Length - BufferSize );
            }
        
            DEBUG ((EFI_D_INFO, "TX: %02x-%02x-%02x-%02x-%02x-%02x  %02x-%02x-%02x-%02x-%02x-%02x"
                      "  %02x-%02x  %d bytes\r\n",
                      pNicDevice->pTxTest->Data[0],
                      pNicDevice->pTxTest->Data[1],
                      pNicDevice->pTxTest->Data[2],
                      pNicDevice->pTxTest->Data[3],
                      pNicDevice->pTxTest->Data[4],
                      pNicDevice->pTxTest->Data[5],
                      pNicDevice->pTxTest->Data[6],
                      pNicDevice->pTxTest->Data[7],
                      pNicDevice->pTxTest->Data[8],
                      pNicDevice->pTxTest->Data[9],
                      pNicDevice->pTxTest->Data[10],
                      pNicDevice->pTxTest->Data[11],
                      pNicDevice->pTxTest->Data[12],
                      pNicDevice->pTxTest->Data[13],
                      pNicDevice->pTxTest->Length ));

            pNicDevice->pTxTest->LengthBar = ~(pNicDevice->pTxTest->Length);
            TransferLength = sizeof ( pNicDevice->pTxTest->Length )
                           + sizeof ( pNicDevice->pTxTest->LengthBar )
                           + pNicDevice->pTxTest->Length;
                           
            if (TransferLength % 512 == 0 || TransferLength % 1024 == 0)
                TransferLength +=4;

            //
            //  Work around USB bus driver bug where a timeout set by receive
            //  succeeds but the timeout expires immediately after, causing the
            //  transmit operation to timeout.
            //
            pUsbIo = pNicDevice->pUsbIo;
            Status = pUsbIo->UsbBulkTransfer ( pUsbIo,
                                               BULK_OUT_ENDPOINT,
                                               &pNicDevice->pTxTest->Length,
                                               &TransferLength,
                                               0xfffffffe, 
                                               &TransferStatus );
            if ( !EFI_ERROR ( Status )) {
              Status = TransferStatus;
            }

            if ( !EFI_ERROR ( Status )) {
              pNicDevice->pTxBuffer = pBuffer;
            }
            else {
              if ((TransferLength != (UINTN)( pNicDevice->pTxTest->Length + 4 )) &&
                   (TransferLength != (UINTN)(( pNicDevice->pTxTest->Length + 4 ) + 4))) {
                DEBUG ((EFI_D_INFO, "TransferLength didn't match Packet Length\n"));
              }
              //
              //  Reset the controller to fix the error
              //
              if ( EFI_DEVICE_ERROR == Status ) {
                SN_Reset ( pSimpleNetwork, FALSE );
              }
              Status = EFI_NOT_READY;
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
          if (EfiSimpleNetworkStarted == pMode->State) {
            Status = EFI_DEVICE_ERROR;
          }
          else {
            Status = EFI_NOT_STARTED ;
          }
        }
      }
      else {
        Status = EFI_BUFFER_TOO_SMALL;
      }
    }
    else {
      Status = EFI_INVALID_PARAMETER;
    }
  }
  else {
    Status = EFI_INVALID_PARAMETER;
  }
  
  gBS->RestoreTPL (TplPrevious);

  return Status;
}
