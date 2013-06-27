/** @file
  Implement the interface to the AX88772 Ethernet controller.

  This module implements the interface to the ASIX AX88772
  USB to Ethernet MAC with integrated 10/100 PHY.  Note that this implementation
  only supports the integrated PHY since no other test cases were available.

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
  Compute the CRC

  @param [in] pMacAddress      Address of a six byte buffer to containing the MAC address.

  @returns The CRC-32 value associated with this MAC address

**/
UINT32
Ax88772Crc (
  IN UINT8 * pMacAddress
  )
{
  UINT32 BitNumber;
  INT32 Carry;
  INT32 Crc;
  UINT32 Data;
  UINT8 * pEnd;

  DBG_ENTER ( );

  //
  //  Walk the MAC address
  //
  Crc = -1;
  pEnd = &pMacAddress[ PXE_HWADDR_LEN_ETHER ];
  while ( pEnd > pMacAddress ) {
    Data = *pMacAddress++;
    
    
    //
    //  CRC32: x32 + x26 + x23 + x22 + x16 + x12 + x11 + x10 + x8 + x7 + x5 + x4 + x2 + x + 1
    //
    //          1 0000 0100 1100 0001 0001 1101 1011 0111
    //
    for ( BitNumber = 0; 8 > BitNumber; BitNumber++ ) {
      Carry = (( Crc >> 31 ) & 1 ) ^ ( Data & 1 );
      Crc <<= 1;
      if ( 0 != Carry ) {
        Crc ^= 0x04c11db7;
      }
      Data >>= 1;
    }
  }

  //
  //  Return the CRC value
  //
  DBG_EXIT_HEX ( Crc );
  return (UINT32) Crc;
}


/**
  Get the MAC address

  This routine calls ::Ax88772UsbCommand to request the MAC
  address from the network adapter.

  @param [in] pNicDevice       Pointer to the NIC_DEVICE structure
  @param [out] pMacAddress      Address of a six byte buffer to receive the MAC address.

  @retval EFI_SUCCESS          The MAC address is available.
  @retval other                The MAC address is not valid.

**/
EFI_STATUS
Ax88772MacAddressGet (
  IN NIC_DEVICE * pNicDevice,
  OUT UINT8 * pMacAddress
  )
{
  USB_DEVICE_REQUEST SetupMsg;
  EFI_STATUS Status;
  
  DBG_ENTER ( );
  
  //
  //  Set the register address.
  //
  SetupMsg.RequestType = USB_ENDPOINT_DIR_IN
                       | USB_REQ_TYPE_VENDOR
                       | USB_TARGET_DEVICE;
  SetupMsg.Request = CMD_MAC_ADDRESS_READ;
  SetupMsg.Value = 0;
  SetupMsg.Index = 0;
  SetupMsg.Length = PXE_HWADDR_LEN_ETHER;

  //
  //  Read the PHY register
  //
  Status = Ax88772UsbCommand ( pNicDevice,
                               &SetupMsg,
                               pMacAddress );

  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Set the MAC address

  This routine calls ::Ax88772UsbCommand to set the MAC address
  in the network adapter.

  @param [in] pNicDevice       Pointer to the NIC_DEVICE structure
  @param [in] pMacAddress      Address of a six byte buffer to containing the new MAC address.

  @retval EFI_SUCCESS          The MAC address was set.
  @retval other                The MAC address was not set.

**/
EFI_STATUS
Ax88772MacAddressSet (
  IN NIC_DEVICE * pNicDevice,
  IN UINT8 * pMacAddress
  )
{
  USB_DEVICE_REQUEST SetupMsg;
  EFI_STATUS Status;
  
  DBG_ENTER ( );
  
  //
  //  Set the register address.
  //
  SetupMsg.RequestType = USB_REQ_TYPE_VENDOR
                       | USB_TARGET_DEVICE;
  SetupMsg.Request = CMD_MAC_ADDRESS_WRITE;
  SetupMsg.Value = 0;
  SetupMsg.Index = 0;
  SetupMsg.Length = PXE_HWADDR_LEN_ETHER;
  
  //
  //  Read the PHY register
  //
  Status = Ax88772UsbCommand ( pNicDevice,
                               &SetupMsg,
                               pMacAddress );
  
  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Clear the multicast hash table

  @param [in] pNicDevice       Pointer to the NIC_DEVICE structure

**/
VOID
Ax88772MulticastClear (
  IN NIC_DEVICE * pNicDevice
  )
{
  DBG_ENTER ( );

  //
  // Clear the multicast hash table
  //
  pNicDevice->MulticastHash[0] = 0;
  pNicDevice->MulticastHash[1] = 0;

  DBG_EXIT ( );
}


/**
  Enable a multicast address in the multicast hash table

  This routine calls ::Ax88772Crc to compute the hash bit for
  this MAC address.

  @param [in] pNicDevice       Pointer to the NIC_DEVICE structure
  @param [in] pMacAddress      Address of a six byte buffer to containing the MAC address.

**/
VOID
Ax88772MulticastSet (
  IN NIC_DEVICE * pNicDevice,
  IN UINT8 * pMacAddress
  )
{
  UINT32 BitNumber;
  UINT32 Crc;
  UINT32 Mask;

  DBG_ENTER ( );

  //
  //  Compute the CRC on the destination address
  //
  Crc = Ax88772Crc ( pMacAddress );

  //
  //  Set the bit corresponding to the destination address
  //
  BitNumber = Crc >> 26;
  if ( 32 > BitNumber ) {
    Mask = 1 << BitNumber;
    pNicDevice->MulticastHash[0] |= Mask;
  }
  else {
    Mask = 1 << ( BitNumber - 32 );
    pNicDevice->MulticastHash[1] |= Mask;
  }

  //
  //  Display the multicast address
  //
  DEBUG (( DEBUG_RX_MULTICAST | DEBUG_INFO,
            "Enable multicast: 0x%02x-%02x-%02x-%02x-%02x-%02x, CRC: 0x%08x, Bit number: 0x%02x\r\n",
            pMacAddress[0],
            pMacAddress[1],
            pMacAddress[2],
            pMacAddress[3],
            pMacAddress[4],
            pMacAddress[5],
            Crc,
            BitNumber ));

  DBG_EXIT ( );
}


/**
  Start the link negotiation

  This routine calls ::Ax88772PhyWrite to start the PHY's link
  negotiation.

  @param [in] pNicDevice       Pointer to the NIC_DEVICE structure

  @retval EFI_SUCCESS          The link negotiation was started.
  @retval other                Failed to start the link negotiation.

**/
EFI_STATUS
Ax88772NegotiateLinkStart (
  IN NIC_DEVICE * pNicDevice
  )
{
  UINT16 Control;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  // Set the supported capabilities.
  //
  Status = Ax88772PhyWrite ( pNicDevice,
                             PHY_ANAR,
                             AN_CSMA_CD
                             | AN_TX_FDX | AN_TX_HDX
                             | AN_10_FDX | AN_10_HDX );
  if ( !EFI_ERROR ( Status )) {
    //
    // Set the link speed and duplex
    //
    Control = BMCR_AUTONEGOTIATION_ENABLE
            | BMCR_RESTART_AUTONEGOTIATION;
    if ( pNicDevice->b100Mbps ) {
      Control |= BMCR_100MBPS;
    }
    if ( pNicDevice->bFullDuplex ) {
      Control |= BMCR_FULL_DUPLEX;
    }
    Status = Ax88772PhyWrite ( pNicDevice, PHY_BMCR, Control );
  }

  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Complete the negotiation of the PHY link

  This routine calls ::Ax88772PhyRead to determine if the
  link negotiation is complete.

  @param [in] pNicDevice       Pointer to the NIC_DEVICE structure
  @param [in, out] pPollCount  Address of number of times this routine was polled
  @param [out] pbComplete      Address of boolean to receive complate status.
  @param [out] pbLinkUp        Address of boolean to receive link status, TRUE=up.
  @param [out] pbHiSpeed       Address of boolean to receive link speed, TRUE=100Mbps.
  @param [out] pbFullDuplex    Address of boolean to receive link duplex, TRUE=full.

  @retval EFI_SUCCESS          The MAC address is available.
  @retval other                The MAC address is not valid.

**/
EFI_STATUS
Ax88772NegotiateLinkComplete (
  IN NIC_DEVICE * pNicDevice,
  IN OUT UINTN * pPollCount,
  OUT BOOLEAN * pbComplete,
  OUT BOOLEAN * pbLinkUp,
  OUT BOOLEAN * pbHiSpeed,
  OUT BOOLEAN * pbFullDuplex
  )
{
  UINT16 Mask;
  UINT16 PhyData;
  EFI_STATUS  Status;

  DBG_ENTER ( );
  
  //
  //  Determine if the link is up.
  //
  *pbComplete = FALSE;

  //
  //  Get the link status
  //
  Status = Ax88772PhyRead ( pNicDevice,
                            PHY_BMSR,
                            &PhyData );
  if ( !EFI_ERROR ( Status )) {
    //
    //  Determine if the autonegotiation is complete.
    //
    *pbLinkUp = (BOOLEAN)( 0 != ( PhyData & BMSR_LINKST ));
    *pbComplete = *pbLinkUp;
    if ( 0 != *pbComplete ) {
      //
      //  Get the partners capabilities.
      //
      Status = Ax88772PhyRead ( pNicDevice,
                                PHY_ANLPAR,
                                &PhyData );
      if ( !EFI_ERROR ( Status )) {
        //
        //  Autonegotiation is complete
        //  Determine the link speed.
        //
        *pbHiSpeed = (BOOLEAN)( 0 != ( PhyData & ( AN_TX_FDX | AN_TX_HDX )));

        //
        //  Determine the link duplex.
        //
        Mask = ( *pbHiSpeed ) ? AN_TX_FDX : AN_10_FDX;
        *pbFullDuplex = (BOOLEAN)( 0 != ( PhyData & Mask ));
      }
    }
  }

  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Read a register from the PHY

  This routine calls ::Ax88772UsbCommand to read a PHY register.

  @param [in] pNicDevice       Pointer to the NIC_DEVICE structure
  @param [in] RegisterAddress  Number of the register to read.
  @param [in, out] pPhyData    Address of a buffer to receive the PHY register value

  @retval EFI_SUCCESS          The PHY data is available.
  @retval other                The PHY data is not valid.

**/
EFI_STATUS
Ax88772PhyRead (
  IN NIC_DEVICE * pNicDevice,
  IN UINT8 RegisterAddress,
  IN OUT UINT16 * pPhyData
  )
{
  USB_DEVICE_REQUEST SetupMsg;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Request access to the PHY
  //
  SetupMsg.RequestType = USB_REQ_TYPE_VENDOR
                       | USB_TARGET_DEVICE;
  SetupMsg.Request = CMD_PHY_ACCESS_SOFTWARE;
  SetupMsg.Value = 0;
  SetupMsg.Index = 0;
  SetupMsg.Length = 0;
  Status = Ax88772UsbCommand ( pNicDevice,
                               &SetupMsg,
                               NULL );
  if ( !EFI_ERROR ( Status )) {
    //
    //  Read the PHY register address.
    //
    SetupMsg.RequestType = USB_ENDPOINT_DIR_IN
                         | USB_REQ_TYPE_VENDOR
                         | USB_TARGET_DEVICE;
    SetupMsg.Request = CMD_PHY_REG_READ;
    SetupMsg.Value = pNicDevice->PhyId;
    SetupMsg.Index = RegisterAddress;
    SetupMsg.Length = sizeof ( *pPhyData );
    Status = Ax88772UsbCommand ( pNicDevice,
                                 &SetupMsg,
                                 pPhyData );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_PHY | DEBUG_INFO,
                "PHY %d: 0x%02x --> 0x%04x\r\n",
                pNicDevice->PhyId,
                RegisterAddress,
                *pPhyData ));

      //
      //  Release the PHY to the hardware
      //
      SetupMsg.RequestType = USB_REQ_TYPE_VENDOR
                           | USB_TARGET_DEVICE;
      SetupMsg.Request = CMD_PHY_ACCESS_HARDWARE;
      SetupMsg.Value = 0;
      SetupMsg.Index = 0;
      SetupMsg.Length = 0;
      Status = Ax88772UsbCommand ( pNicDevice,
                                   &SetupMsg,
                                   NULL );
    }
  }

  //
  //  Return the operation status.
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Write to a PHY register

  This routine calls ::Ax88772UsbCommand to write a PHY register.

  @param [in] pNicDevice       Pointer to the NIC_DEVICE structure
  @param [in] RegisterAddress  Number of the register to read.
  @param [in] PhyData          Address of a buffer to receive the PHY register value

  @retval EFI_SUCCESS          The PHY data was written.
  @retval other                Failed to wwrite the PHY register.

**/
EFI_STATUS
Ax88772PhyWrite (
  IN NIC_DEVICE * pNicDevice,
  IN UINT8 RegisterAddress,
  IN UINT16 PhyData
  )
{
  USB_DEVICE_REQUEST SetupMsg;
  EFI_STATUS Status;
  
  DBG_ENTER ( );
  
  //
  //  Request access to the PHY
  //
  SetupMsg.RequestType = USB_REQ_TYPE_VENDOR
                       | USB_TARGET_DEVICE;
  SetupMsg.Request = CMD_PHY_ACCESS_SOFTWARE;
  SetupMsg.Value = 0;
  SetupMsg.Index = 0;
  SetupMsg.Length = 0;
  Status = Ax88772UsbCommand ( pNicDevice,
                               &SetupMsg,
                               NULL );
  if ( !EFI_ERROR ( Status )) {
    //
    //  Write the PHY register
    //
    SetupMsg.RequestType = USB_REQ_TYPE_VENDOR
                         | USB_TARGET_DEVICE;
    SetupMsg.Request = CMD_PHY_REG_WRITE;
    SetupMsg.Value = pNicDevice->PhyId;
    SetupMsg.Index = RegisterAddress;
    SetupMsg.Length = sizeof ( PhyData );
    Status = Ax88772UsbCommand ( pNicDevice,
                                 &SetupMsg,
                                 &PhyData );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_PHY | DEBUG_INFO,
                "PHY %d: 0x%02x <-- 0x%04x\r\n",
                pNicDevice->PhyId,
                RegisterAddress,
                PhyData ));

      //
      //  Release the PHY to the hardware
      //
      SetupMsg.RequestType = USB_REQ_TYPE_VENDOR
                           | USB_TARGET_DEVICE;
      SetupMsg.Request = CMD_PHY_ACCESS_HARDWARE;
      SetupMsg.Value = 0;
      SetupMsg.Index = 0;
      SetupMsg.Length = 0;
      Status = Ax88772UsbCommand ( pNicDevice,
                                   &SetupMsg,
                                   NULL );
    }
  }

  //
  //  Return the operation status.
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Reset the AX88772

  This routine uses ::Ax88772UsbCommand to reset the network
  adapter.  This routine also uses ::Ax88772PhyWrite to reset
  the PHY.

  @param [in] pNicDevice       Pointer to the NIC_DEVICE structure

  @retval EFI_SUCCESS          The MAC address is available.
  @retval other                The MAC address is not valid.

**/
EFI_STATUS
Ax88772Reset (
  IN NIC_DEVICE * pNicDevice
  )
{
  USB_DEVICE_REQUEST SetupMsg;
  EFI_STATUS Status;
  
  DBG_ENTER ( );

  //
  //  Turn off the MAC
  //
  SetupMsg.RequestType = USB_REQ_TYPE_VENDOR
                       | USB_TARGET_DEVICE;
  SetupMsg.Request = CMD_RX_CONTROL_WRITE;
  SetupMsg.Value = 0;
  SetupMsg.Index = 0;
  SetupMsg.Length = 0;
  Status = Ax88772UsbCommand ( pNicDevice,
                               &SetupMsg,
                               NULL );
  if ( !EFI_ERROR ( Status )) {
    DEBUG (( DEBUG_PHY | DEBUG_RX_BROADCAST | DEBUG_RX_MULTICAST
              | DEBUG_RX_UNICAST | DEBUG_TX | DEBUG_INFO,
              "MAC reset\r\n" ));

    //
    //  The link is now idle
    //
    pNicDevice->bLinkIdle = TRUE;

    //
    //  Delay for a bit
    //
    gBS->Stall ( RESET_MSEC );

    //
    //  Select the internal PHY
    //
    SetupMsg.RequestType = USB_REQ_TYPE_VENDOR
                         | USB_TARGET_DEVICE;
    SetupMsg.Request = CMD_PHY_SELECT;
    SetupMsg.Value = SPHY_PSEL;
    SetupMsg.Index = 0;
    SetupMsg.Length = 0;
    Status = Ax88772UsbCommand ( pNicDevice,
                                 &SetupMsg,
                                 NULL );
    if ( !EFI_ERROR ( Status )) {
      //
      //  Delay for a bit
      //
      gBS->Stall ( PHY_RESET_MSEC );

      //
      //  Clear the internal PHY reset
      //
      SetupMsg.Request = CMD_RESET;
      SetupMsg.Value = SRR_IPRL | SRR_PRL;
      Status = Ax88772UsbCommand ( pNicDevice,
                                   &SetupMsg,
                                   NULL );
      if ( !EFI_ERROR ( Status )) {
        //
        //  Reset the PHY
        //
        Status = Ax88772PhyWrite ( pNicDevice,
                                   PHY_BMCR,
                                   BMCR_RESET );
        if ( !EFI_ERROR ( Status )) {
          //
          //  Set the gaps
          //
          SetupMsg.RequestType = USB_REQ_TYPE_VENDOR
                               | USB_TARGET_DEVICE;
          SetupMsg.Request = CMD_GAPS_WRITE;
          SetupMsg.Value = 0x0c15;
          SetupMsg.Index = 0x0e;
          SetupMsg.Length = 0;
          Status = Ax88772UsbCommand ( pNicDevice,
                                       &SetupMsg,
                                       NULL );
        }
      }
    }
  }

  //
  //  Return the operation status.
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


VOID 
FillPkt2Queue (
  IN NIC_DEVICE * pNicDevice,
  IN UINTN BufLength)
{

  UINT16 * pLength;
  UINT16 * pLengthBar;
  UINT8* pData;
  UINT32 offset;
  RX_TX_PACKET * pRxPacket;
  UINTN LengthInBytes;
  EFI_STATUS Status;
  
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
      
    pRxPacket = pNicDevice->pRxFree;
    LengthInBytes = sizeof ( *pRxPacket ) - sizeof ( pRxPacket->pNext );
    if ( NULL == pRxPacket ) {
      Status = gBS->AllocatePool ( EfiRuntimeServicesData,
                                   sizeof( RX_TX_PACKET ),
                                   (VOID **) &pRxPacket );
      if ( !EFI_ERROR ( Status )) {
        //
        //  Add this packet to the free packet list
        //
        pNicDevice->pRxFree = pRxPacket;
        pRxPacket->pNext = NULL;
      }
      else {
        //
        //  Use the discard packet buffer
        //
        //pRxPacket = &Packet;
      }
    }
      

    pData = pNicDevice->pBulkInBuff + offset + 4;
    pRxPacket->Length = *pLength;
    pRxPacket->LengthBar = *(UINT16*) (pNicDevice->pBulkInBuff + offset +2);
    CopyMem (&pRxPacket->Data[0], pData, *pLength);
    //DEBUG((DEBUG_INFO, "Packet [%d]\n", *pLength));
    
    pNicDevice->pRxFree = pRxPacket->pNext;
    pRxPacket->pNext = NULL;
    
    if ( NULL == pNicDevice->pRxTail ) {
      pNicDevice->pRxHead = pRxPacket;
    }
    else {
      pNicDevice->pRxTail->pNext = pRxPacket;
    }
    pNicDevice->pRxTail = pRxPacket;
    offset += (*pLength + 4);
              
  }
}



/**
  Receive a frame from the network.

  This routine polls the USB receive interface for a packet.  If a packet
  is available, this routine adds the receive packet to the list of
  pending receive packets.

  This routine calls ::Ax88772NegotiateLinkComplete to verify
  that the link is up.  This routine also calls ::SN_Reset to
  reset the network adapter when necessary.  Finally this
  routine attempts to receive one or more packets from the
  network adapter.

  @param [in] pNicDevice  Pointer to the NIC_DEVICE structure
  @param [in] bUpdateLink TRUE = Update link status

**/
VOID
Ax88772Rx (
  IN NIC_DEVICE * pNicDevice,
  IN BOOLEAN bUpdateLink
  )
{
  BOOLEAN bFullDuplex;
  BOOLEAN bLinkUp;
  BOOLEAN bRxPacket;
  BOOLEAN bSpeed100;
  UINTN LengthInBytes;
  RX_TX_PACKET Packet;
  RX_TX_PACKET * pRxPacket;
  EFI_USB_IO_PROTOCOL *pUsbIo;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;
  UINT32 TransferStatus;

  //
  //  Synchronize with Ax88772Timer
  //
  VERIFY_TPL ( TPL_AX88772 );
  TplPrevious = gBS->RaiseTPL ( TPL_AX88772 );
  DEBUG (( DEBUG_TPL | DEBUG_INFO,
            "%d: TPL\r\n",
            TPL_AX88772 ));

  //
  //  Get the link status
  //
  if ( bUpdateLink ) {
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
          DEBUG (( DEBUG_LINK | DEBUG_INFO,
                    "Reset to establish proper link setup: %d Mbps, %s duplex\r\n",
                    pNicDevice->b100Mbps ? 100 : 10,
                    pNicDevice->bFullDuplex ? L"Full" : L"Half" ));
          Status = SN_Reset ( &pNicDevice->SimpleNetwork, FALSE );
        }
        if (( !bLinkUp ) && pNicDevice->bLinkUp ) {
          //
          // Display the autonegotiation status
          //
          DEBUG (( DEBUG_LINK | DEBUG_INFO,
                    "Link: Up, %d Mbps, %s duplex\r\n",
                    pNicDevice->b100Mbps ? 100 : 10,
                    pNicDevice->bFullDuplex ? L"Full" : L"Half" ));
        }
      }
    }

    //
    //  Update the link status
    //
    if ( bLinkUp && ( !pNicDevice->bLinkUp )) {
      DEBUG (( DEBUG_LINK | DEBUG_INFO, "Link: Down\r\n" ));
    }
  }

  //
  //  Loop until all the packets are emptied from the receiver
  //
  do {
    bRxPacket = FALSE;

    //
    //  Locate a packet for use
    //
    pRxPacket = pNicDevice->pRxFree;
    LengthInBytes = MAX_BULKIN_SIZE;
    if ( NULL == pRxPacket ) {
      Status = gBS->AllocatePool ( EfiRuntimeServicesData,
                                   sizeof ( *pRxPacket ),
                                   (VOID **) &pRxPacket );
      if ( !EFI_ERROR ( Status )) {
        //
        //  Add this packet to the free packet list
        //
        pNicDevice->pRxFree = pRxPacket;
        pRxPacket->pNext = NULL;
      }
      else {
        //
        //  Use the discard packet buffer
        //
        pRxPacket = &Packet;
      }
    }

    //
    //  Attempt to receive a packet
    //
    SetMem (&pNicDevice->pBulkInBuff[0], MAX_BULKIN_SIZE, 0);
    pUsbIo = pNicDevice->pUsbIo;
    Status = pUsbIo->UsbBulkTransfer ( pUsbIo,
                                       USB_ENDPOINT_DIR_IN | BULK_IN_ENDPOINT,
                                       &pNicDevice->pBulkInBuff[0],
                                       &LengthInBytes,
                                       2,
                                       &TransferStatus );
    if ( LengthInBytes > 0 ) {
      FillPkt2Queue(pNicDevice, LengthInBytes);
    }
    pRxPacket = pNicDevice->pRxHead;
    if (( !EFI_ERROR ( Status ))
      && ( 0 < pRxPacket->Length )
      && ( pRxPacket->Length <= sizeof ( pRxPacket->Data ))
      && ( LengthInBytes > 0)) {

      //
      //  Determine if the packet should be received
      //
      bRxPacket = TRUE;
      LengthInBytes = pRxPacket->Length;
      pNicDevice->bLinkIdle = FALSE;
      if ( pNicDevice->pRxFree == pRxPacket ) {
        //
        //  Display the received packet
        //
        if ( 0 != ( pRxPacket->Data[0] & 1 )) {
          if (( 0xff == pRxPacket->Data[0])
            && ( 0xff == pRxPacket->Data[1])
            && ( 0xff == pRxPacket->Data[2])
            && ( 0xff == pRxPacket->Data[3])
            && ( 0xff == pRxPacket->Data[4])
            && ( 0xff == pRxPacket->Data[5])) {
            DEBUG (( DEBUG_RX_BROADCAST | DEBUG_INFO,
                      "RX: %02x-%02x-%02x-%02x-%02x-%02x  %02x-%02x-%02x-%02x-%02x-%02x  %02x-%02x  %d bytes\r\n",
                      pRxPacket->Data[0],
                      pRxPacket->Data[1],
                      pRxPacket->Data[2],
                      pRxPacket->Data[3],
                      pRxPacket->Data[4],
                      pRxPacket->Data[5],
                      pRxPacket->Data[6],
                      pRxPacket->Data[7],
                      pRxPacket->Data[8],
                      pRxPacket->Data[9],
                      pRxPacket->Data[10],
                      pRxPacket->Data[11],
                      pRxPacket->Data[12],
                      pRxPacket->Data[13],
                      LengthInBytes ));
          }
          else {
            DEBUG (( DEBUG_RX_MULTICAST | DEBUG_INFO,
                      "RX: %02x-%02x-%02x-%02x-%02x-%02x  %02x-%02x-%02x-%02x-%02x-%02x  %02x-%02x  %d bytes\r\n",
                      pRxPacket->Data[0],
                      pRxPacket->Data[1],
                      pRxPacket->Data[2],
                      pRxPacket->Data[3],
                      pRxPacket->Data[4],
                      pRxPacket->Data[5],
                      pRxPacket->Data[6],
                      pRxPacket->Data[7],
                      pRxPacket->Data[8],
                      pRxPacket->Data[9],
                      pRxPacket->Data[10],
                      pRxPacket->Data[11],
                      pRxPacket->Data[12],
                      pRxPacket->Data[13],
                      LengthInBytes ));
          }
        }
        else {
          DEBUG (( DEBUG_RX_UNICAST | DEBUG_INFO,
                    "RX: %02x-%02x-%02x-%02x-%02x-%02x  %02x-%02x-%02x-%02x-%02x-%02x  %02x-%02x  %d bytes\r\n",
                    pRxPacket->Data[0],
                    pRxPacket->Data[1],
                    pRxPacket->Data[2],
                    pRxPacket->Data[3],
                    pRxPacket->Data[4],
                    pRxPacket->Data[5],
                    pRxPacket->Data[6],
                    pRxPacket->Data[7],
                    pRxPacket->Data[8],
                    pRxPacket->Data[9],
                    pRxPacket->Data[10],
                    pRxPacket->Data[11],
                    pRxPacket->Data[12],
                    pRxPacket->Data[13],
                    LengthInBytes ));
        }
        
      }
      else {
        //
        //  Error, not enough buffers for this packet, discard packet
        //
        DEBUG (( DEBUG_WARN | DEBUG_INFO,
                  "WARNING - No buffer, discarding RX packet: %02x-%02x-%02x-%02x-%02x-%02x  %02x-%02x-%02x-%02x-%02x-%02x  %02x-%02x  %d bytes\r\n",
                  pRxPacket->Data[0],
                  pRxPacket->Data[1],
                  pRxPacket->Data[2],
                  pRxPacket->Data[3],
                  pRxPacket->Data[4],
                  pRxPacket->Data[5],
                  pRxPacket->Data[6],
                  pRxPacket->Data[7],
                  pRxPacket->Data[8],
                  pRxPacket->Data[9],
                  pRxPacket->Data[10],
                  pRxPacket->Data[11],
                  pRxPacket->Data[12],
                  pRxPacket->Data[13],
                  LengthInBytes ));
      }
    }
  }while ( bRxPacket );

  //
  //  Release the synchronization withhe Ax88772Timer
  //
  gBS->RestoreTPL ( TplPrevious );
  DEBUG (( DEBUG_TPL | DEBUG_INFO,
            "%d: TPL\r\n",
            TplPrevious ));
}


/**
  Enable or disable the receiver

  This routine calls ::Ax88772UsbCommand to update the
  receiver state.  This routine also calls ::Ax88772MacAddressSet
  to establish the MAC address for the network adapter.

  @param [in] pNicDevice       Pointer to the NIC_DEVICE structure
  @param [in] RxFilter         Simple network RX filter mask value

  @retval EFI_SUCCESS          The MAC address was set.
  @retval other                The MAC address was not set.

**/
EFI_STATUS
Ax88772RxControl (
  IN NIC_DEVICE * pNicDevice,
  IN UINT32 RxFilter
  )
{
  UINT16 MediumStatus;
  INT32 MulticastHash[2];
  UINT16 RxControl;
  USB_DEVICE_REQUEST SetupMsg;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Disable all multicast
  //
  MulticastHash[0] = 0;
  MulticastHash[1] = 0;

  //
  // Enable the receiver if something is to be received
  //
  Status = EFI_SUCCESS;
  RxControl = RXC_SO | RXC_MFB_16384;
  if ( 0 != RxFilter ) {
    //
    //  Enable the receiver
    //
    SetupMsg.RequestType = USB_ENDPOINT_DIR_IN
                         | USB_REQ_TYPE_VENDOR
                         | USB_TARGET_DEVICE;
    SetupMsg.Request = CMD_MEDIUM_STATUS_READ;
    SetupMsg.Value = 0;
    SetupMsg.Index = 0;
    SetupMsg.Length = sizeof ( MediumStatus );
    Status = Ax88772UsbCommand ( pNicDevice,
                                 &SetupMsg,
                                 &MediumStatus );
    if ( !EFI_ERROR ( Status )) {
      if ( 0 == ( MediumStatus & MS_RE )) {
        MediumStatus |= MS_RE | MS_ONE;
        if ( pNicDevice->bFullDuplex ) {
          MediumStatus |= MS_TFC | MS_RFC;
        }
        SetupMsg.RequestType = USB_REQ_TYPE_VENDOR
                             | USB_TARGET_DEVICE;
        SetupMsg.Request = CMD_MEDIUM_STATUS_WRITE;
        SetupMsg.Value = MediumStatus;
        SetupMsg.Index = 0;
        SetupMsg.Length = 0;
        Status = Ax88772UsbCommand ( pNicDevice,
                                     &SetupMsg,
                                     NULL );
        if ( EFI_ERROR ( Status )) {
          DEBUG (( DEBUG_ERROR | DEBUG_INFO,
                    "ERROR - Failed to enable receiver, Status: %r\r\n",
                    Status ));
        }
      }
    }
    else {
      DEBUG (( DEBUG_ERROR | DEBUG_INFO,
                "ERROR - Failed to read receiver status, Status: %r\r\n",
                Status ));
    }

    //
    //  Enable multicast if requested
    //
    if ( 0 != ( RxFilter & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST )) {
      RxControl |= RXC_AM;
      MulticastHash[0] = pNicDevice->MulticastHash[0];
      MulticastHash[1] = pNicDevice->MulticastHash[1];
    }

    //
    //  Enable all multicast if requested
    //
    if ( 0 != ( RxFilter & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST )) {
      RxControl |= RXC_AMALL;
      MulticastHash[0] = -1;
      MulticastHash[1] = -1;
    }

    //
    //  Enable broadcast if requested
    //
    if ( 0 != ( RxFilter & EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST )) {
      RxControl |= RXC_AB;
    }

    //
    //  Enable promiscuous mode if requested
    //
    if ( 0 != ( RxFilter & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS )) {
      RxControl |= RXC_PRO;
      MulticastHash[0] = -1;
      MulticastHash[1] = -1;
    }
  }

  //
  //  Update the MAC address
  //
  if ( !EFI_ERROR ( Status )) {
    Status = Ax88772MacAddressSet ( pNicDevice, &pNicDevice->SimpleNetworkData.CurrentAddress.Addr[0]);
  }

  //
  //  Update the receiver control
  //
  if ( !EFI_ERROR ( Status )) {
    SetupMsg.RequestType = USB_REQ_TYPE_VENDOR
                         | USB_TARGET_DEVICE;
    SetupMsg.Request = CMD_RX_CONTROL_WRITE;
    SetupMsg.Value = RxControl;
    SetupMsg.Index = 0;
    SetupMsg.Length = 0;
    Status = Ax88772UsbCommand ( pNicDevice,
                                 &SetupMsg,
                                 NULL );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_RX_BROADCAST | DEBUG_RX_MULTICAST | DEBUG_RX_UNICAST | DEBUG_INFO,
                "RxControl: 0x%04x\r\n",
                RxControl ));

      //
      //  Update the multicast hash table
      //
      SetupMsg.RequestType = USB_REQ_TYPE_VENDOR
                           | USB_TARGET_DEVICE;
      SetupMsg.Request = CMD_MULTICAST_HASH_WRITE;
      SetupMsg.Value = 0;
      SetupMsg.Index = 0;
      SetupMsg.Length = sizeof ( pNicDevice ->MulticastHash );
      Status = Ax88772UsbCommand ( pNicDevice,
                                   &SetupMsg,
                                   &pNicDevice->MulticastHash );
      if ( !EFI_ERROR ( Status )) {
        DEBUG (( DEBUG_RX_MULTICAST | DEBUG_INFO,
                  "Multicast Hash: 0x%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                  (UINT8) MulticastHash[0],
                  (UINT8)( MulticastHash[0] >> 8 ),
                  (UINT8)( MulticastHash[0] >> 16 ),
                  (UINT8)( MulticastHash[0] >> 24 ),
                  (UINT8) MulticastHash[1],
                  (UINT8)( MulticastHash[1] >> 8 ),
                  (UINT8)( MulticastHash[1] >> 16 ),
                  (UINT8)( MulticastHash[1] >> 24 )));
      }
      else {
        DEBUG (( DEBUG_ERROR | DEBUG_INFO,
                  "ERROR - Failed to update multicast hash table, Status: %r\r\n",
                  Status ));
      }
    }
    else {
      DEBUG (( DEBUG_ERROR | DEBUG_INFO,
                "ERROR - Failed to set receiver control, Status: %r\r\n",
                Status ));
    }
  }

  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Read an SROM location

  This routine calls ::Ax88772UsbCommand to read data from the
  SROM.

  @param [in] pNicDevice       Pointer to the NIC_DEVICE structure
  @param [in] Address          SROM address
  @param [out] pData           Buffer to receive the data

  @retval EFI_SUCCESS          The read was successful
  @retval other                The read failed

**/
EFI_STATUS
Ax88772SromRead (
  IN NIC_DEVICE * pNicDevice,
  IN UINT32 Address,
  OUT UINT16 * pData
  )
{
  USB_DEVICE_REQUEST SetupMsg;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Read a value from the SROM
  //
  SetupMsg.RequestType = USB_ENDPOINT_DIR_IN
                       | USB_REQ_TYPE_VENDOR
                       | USB_TARGET_DEVICE;
  SetupMsg.Request = CMD_SROM_READ;
  SetupMsg.Value = (UINT16) Address;
  SetupMsg.Index = 0;
  SetupMsg.Length = sizeof ( *pData );
  Status = Ax88772UsbCommand ( pNicDevice,
                               &SetupMsg,
                               pData );

  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  This routine is called at a regular interval to poll for
  receive packets.

  This routine polls the link state and gets any receive packets
  by calling ::Ax88772Rx.

  @param [in] Event            Timer event
  @param [in] pNicDevice       Pointer to the NIC_DEVICE structure

**/
VOID
Ax88772Timer (
  IN EFI_EVENT Event,
  IN NIC_DEVICE * pNicDevice
  )
{
  //
  //  Use explicit DEBUG messages since the output frequency is too
  //  high for DEBUG_INFO to keep up and have spare cycles for the
  //  shell
  //
  DEBUG (( DEBUG_TIMER, "Entering Ax88772Timer\r\n" ));

  //
  //  Poll the link state and get any receive packets
  //
  Ax88772Rx ( pNicDevice, FALSE );

  DEBUG (( DEBUG_TIMER, "Exiting Ax88772Timer\r\n" ));
}


/**
  Send a command to the USB device.

  @param [in] pNicDevice       Pointer to the NIC_DEVICE structure
  @param [in] pRequest         Pointer to the request structure
  @param [in, out] pBuffer     Data buffer address

  @retval EFI_SUCCESS          The USB transfer was successful
  @retval other                The USB transfer failed

**/
EFI_STATUS
Ax88772UsbCommand (
  IN NIC_DEVICE * pNicDevice,
  IN USB_DEVICE_REQUEST * pRequest,
  IN OUT VOID * pBuffer
  )
{
  UINT32 CmdStatus;
  EFI_USB_DATA_DIRECTION Direction;
  EFI_USB_IO_PROTOCOL * pUsbIo;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  // Determine the transfer direction
  //
  Direction = EfiUsbNoData;
  if ( 0 != pRequest->Length ) {
    Direction = ( 0 != ( pRequest->RequestType & USB_ENDPOINT_DIR_IN ))
              ? EfiUsbDataIn : EfiUsbDataOut;
  }

  //
  // Issue the command
  //
  pUsbIo = pNicDevice->pUsbIo;
  Status = pUsbIo->UsbControlTransfer ( pUsbIo,
                                        pRequest,
                                        Direction,
                                        USB_BUS_TIMEOUT,
                                        pBuffer,
                                        pRequest->Length,
                                        &CmdStatus );

  //
  // Determine the operation status
  //
  if ( !EFI_ERROR ( Status )) {
    Status = CmdStatus;
  }
  else {
    //
    // Display any errors
    //
    DEBUG (( DEBUG_INFO,
              "Ax88772UsbCommand - Status: %r\n",
              Status ));

    //
    // Only use status values associated with the Simple Network protocol
    //
    if ( EFI_TIMEOUT == Status ) {
      Status = EFI_DEVICE_ERROR;
    }
  }

  //
  // Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}
