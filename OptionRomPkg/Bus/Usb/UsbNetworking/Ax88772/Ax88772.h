/** @file
  Definitions for ASIX AX88772 Ethernet adapter.

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _AX88772_H_
#define _AX88772_H_

#include <Uefi.h>

#include <Guid/EventGroup.h>
#include <Guid/NicIp4ConfigNvData.h>

#include <IndustryStandard/Pci.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/NetworkInterfaceIdentifier.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/UsbIo.h>

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
//
//Too many output debug info hangs system in Debug tip
//
//#if defined(_MSC_VER)           /* Handle Microsoft VC++ compiler specifics. */
//#define DBG_ENTER()             DEBUG (( DEBUG_INFO, "Entering " __FUNCTION__ "\n" )) ///<  Display routine entry
//#define DBG_EXIT()              DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ "\n" ))  ///<  Display routine exit
//#define DBG_EXIT_DEC(Status)    DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ ", Status: %d\n", Status ))      ///<  Display routine exit with decimal value
//#define DBG_EXIT_HEX(Status)    DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ ", Status: 0x%08x\n", Status ))  ///<  Display routine exit with hex value
//#define DBG_EXIT_STATUS(Status) DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ ", Status: %r\n", Status ))      ///<  Display routine exit with status value
//#define DBG_EXIT_TF(Status)     DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ ", returning %s\n", (FALSE == Status) ? L"FALSE" : L"TRUE" ))  ///<  Display routine with TRUE/FALSE value
//#else   //  _MSC_VER
#define DBG_ENTER()               ///<  Display routine entry
#define DBG_EXIT()                ///<  Display routine exit
#define DBG_EXIT_DEC(Status)      ///<  Display routine exit with decimal value
#define DBG_EXIT_HEX(Status)      ///<  Display routine exit with hex value
#define DBG_EXIT_STATUS(Status)   ///<  Display routine exit with status value
#define DBG_EXIT_TF(Status)       ///<  Display routine with TRUE/FALSE value
//#endif  //  _MSC_VER

#define USB_IS_IN_ENDPOINT(EndPointAddr)      (((EndPointAddr) & BIT7) != 0)  ///<  Return TRUE/FALSE for IN direction
#define USB_IS_OUT_ENDPOINT(EndPointAddr)     (((EndPointAddr) & BIT7) == 0)  ///<  Return TRUE/FALSE for OUT direction
#define USB_IS_BULK_ENDPOINT(Attribute)       (((Attribute) & (BIT0 | BIT1)) == USB_ENDPOINT_BULK)      ///<  Return TRUE/FALSE for BULK type
#define USB_IS_INTERRUPT_ENDPOINT(Attribute)  (((Attribute) & (BIT0 | BIT1)) == USB_ENDPOINT_INTERRUPT) ///<  Return TRUE/FALSE for INTERRUPT type

//------------------------------------------------------------------------------
//  Constants
//------------------------------------------------------------------------------

#define DEBUG_RX_BROADCAST  0x40000000  ///<  Display RX broadcast messages
#define DEBUG_RX_MULTICAST  0x20000000  ///<  Display RX multicast messages
#define DEBUG_RX_UNICAST    0x10000000  ///<  Display RX unicast messages
#define DEBUG_MAC_ADDRESS   0x08000000  ///<  Display the MAC address
#define DEBUG_LINK          0x04000000  ///<  Display the link status
#define DEBUG_TX            0x02000000  ///<  Display the TX messages
#define DEBUG_PHY           0x01000000  ///<  Display the PHY register values
#define DEBUG_SROM          0x00800000  ///<  Display the SROM contents
#define DEBUG_TIMER         0x00400000  ///<  Display the timer routine entry/exit
#define DEBUG_TPL           0x00200000  ///<  Display the timer routine entry/exit

#define AX88772_MAX_PKT_SIZE  ( 2048 - 4 )  ///< Maximum packet size
#define ETHERNET_HEADER_SIZE  sizeof ( ETHERNET_HEADER )  ///<  Size in bytes of the Ethernet header
#define MIN_ETHERNET_PKT_SIZE 60    ///<  Minimum packet size including Ethernet header
#define MAX_ETHERNET_PKT_SIZE 1500  ///<  Ethernet spec 3.1.1: Minimum packet size
#define MAX_BULKIN_SIZE       2048  ///<  Maximum size of one UsbBulk 


#define USB_NETWORK_CLASS   0x09    ///<  USB Network class code
#define USB_BUS_TIMEOUT     1000    ///<  USB timeout in milliseconds

#define TIMER_MSEC          20              ///<  Polling interval for the NIC
#define TPL_AX88772         TPL_CALLBACK    ///<  TPL for routine synchronization

/**
  Verify new TPL value

  This macro which is enabled when debug is enabled verifies that
  the new TPL value is >= the current TPL value.
**/
#ifdef VERIFY_TPL
#undef VERIFY_TPL
#endif  //  VERIFY_TPL

#if !defined(MDEPKG_NDEBUG)

#define VERIFY_TPL(tpl)                           \
{                                                 \
  EFI_TPL PreviousTpl;                            \
                                                  \
  PreviousTpl = gBS->RaiseTPL ( TPL_HIGH_LEVEL ); \
  gBS->RestoreTPL ( PreviousTpl );                \
  if ( PreviousTpl > tpl ) {                      \
    DEBUG (( DEBUG_ERROR, "Current TPL: %d, New TPL: %d\r\n", PreviousTpl, tpl ));  \
    ASSERT ( PreviousTpl <= tpl );                \
  }                                               \
}

#else   //  MDEPKG_NDEBUG

#define VERIFY_TPL(tpl)

#endif  //  MDEPKG_NDEBUG

//------------------------------------------------------------------------------
//  Hardware Definition
//------------------------------------------------------------------------------

#define DEV_SIGNATURE     SIGNATURE_32 ('A','X','8','8')  ///<  Signature of data structures in memory

#define VENDOR_ID         0x0b95  ///<  Vendor ID for Asix
#define PRODUCT_ID        0x7720  ///<  Product ID for the AX88772 USB 10/100 Ethernet controller

#define RESET_MSEC        1000    ///<  Reset duration
#define PHY_RESET_MSEC     500    ///<  PHY reset duration

//
//  RX Control register
//

#define RXC_PRO           0x0001  ///<  Receive all packets
#define RXC_AMALL         0x0002  ///<  Receive all multicast packets
#define RXC_SEP           0x0004  ///<  Save error packets
#define RXC_AB            0x0008  ///<  Receive broadcast packets
#define RXC_AM            0x0010  ///<  Use multicast destination address hash table
#define RXC_AP            0x0020  ///<  Accept physical address from Multicast Filter
#define RXC_SO            0x0080  ///<  Start operation
#define RXC_MFB           0x0300  ///<  Maximum frame burst
#define RXC_MFB_2048      0       ///<  Maximum frame size:  2048 bytes
#define RXC_MFB_4096      0x0100  ///<  Maximum frame size:  4096 bytes
#define RXC_MFB_8192      0x0200  ///<  Maximum frame size:  8192 bytes
#define RXC_MFB_16384     0x0300  ///<  Maximum frame size: 16384 bytes

//
//  Medium Status register
//

#define MS_FD             0x0002  ///<  Full duplex
#define MS_ONE            0x0004  ///<  Must be one
#define MS_RFC            0x0010  ///<  RX flow control enable
#define MS_TFC            0x0020  ///<  TX flow control enable
#define MS_PF             0x0080  ///<  Pause frame enable
#define MS_RE             0x0100  ///<  Receive enable
#define MS_PS             0x0200  ///<  Port speed 1=100, 0=10 Mbps
#define MS_SBP            0x0800  ///<  Stop back pressure
#define MS_SM             0x1000  ///<  Super MAC support

//
//  Software PHY Select register
//

#define SPHY_PSEL         0x01    ///<  Select internal PHY
#define SPHY_ASEL         0x02    ///<  1=Auto select, 0=Manual select

//
//  Software Reset register
//

#define SRR_RR            0x01    ///<  Clear receive frame length error
#define SRR_RT            0x02    ///<  Clear transmit frame length error
#define SRR_PRTE          0x04    ///<  External PHY reset pin tri-state enable
#define SRR_PRL           0x08    ///<  External PHY reset pin level
#define SRR_BZ            0x10    ///<  Force Bulk to return zero length packet
#define SRR_IPRL          0x20    ///<  Internal PHY reset control
#define SRR_IPPD          0x40    ///<  Internal PHY power down

//
//  PHY ID values
//

#define PHY_ID_INTERNAL   0x0010  ///<  Internal PHY

//
//  USB Commands
//

#define CMD_PHY_ACCESS_SOFTWARE   0x06  ///<  Software in control of PHY
#define CMD_PHY_REG_READ          0x07  ///<  Read PHY register, Value: PHY, Index: Register, Data: Register value
#define CMD_PHY_REG_WRITE         0x08  ///<  Write PHY register, Value: PHY, Index: Register, Data: New 16-bit value
#define CMD_PHY_ACCESS_HARDWARE   0x0a  ///<  Hardware in control of PHY
#define CMD_SROM_READ             0x0b  ///<  Read SROM register: Value: Address, Data: Value
#define CMD_RX_CONTROL_WRITE      0x10  ///<  Set the RX control register, Value: New value
#define CMD_GAPS_WRITE            0x12  ///<  Write the gaps register, Value: New value
#define CMD_MAC_ADDRESS_READ      0x13  ///<  Read the MAC address, Data: 6 byte MAC address
#define CMD_MAC_ADDRESS_WRITE     0x14  ///<  Set the MAC address, Data: New 6 byte MAC address
#define CMD_MULTICAST_HASH_WRITE  0x16  ///<  Write the multicast hash table, Data: New 8 byte value
#define CMD_MEDIUM_STATUS_READ    0x1a  ///<  Read medium status register, Data: Register value
#define CMD_MEDIUM_STATUS_WRITE   0x1b  ///<  Write medium status register, Value: New value
#define CMD_RESET                 0x20  ///<  Reset register, Value: New value
#define CMD_PHY_SELECT            0x22  ///<  PHY select register, Value: New value

//------------------------------
//  USB Endpoints
//------------------------------

#define CONTROL_ENDPOINT                0       ///<  Control endpoint
#define INTERRUPT_ENDPOINT              1       ///<  Interrupt endpoint
#define BULK_IN_ENDPOINT                2       ///<  Receive endpoint
#define BULK_OUT_ENDPOINT               3       ///<  Transmit endpoint

//------------------------------
//  PHY Registers
//------------------------------

#define PHY_BMCR                        0       ///<  Control register
#define PHY_BMSR                        1       ///<  Status register
#define PHY_ANAR                        4       ///<  Autonegotiation advertisement register
#define PHY_ANLPAR                      5       ///<  Autonegotiation link parter ability register
#define PHY_ANER                        6       ///<  Autonegotiation expansion register

//  BMCR - Register 0

#define BMCR_RESET                      0x8000  ///<  1 = Reset the PHY, bit clears after reset
#define BMCR_LOOPBACK                   0x4000  ///<  1 = Loopback enabled
#define BMCR_100MBPS                    0x2000  ///<  100 Mbits/Sec
#define BMCR_10MBPS                     0       ///<  10 Mbits/Sec
#define BMCR_AUTONEGOTIATION_ENABLE     0x1000  ///<  1 = Enable autonegotiation
#define BMCR_POWER_DOWN                 0x0800  ///<  1 = Power down
#define BMCR_ISOLATE                    0x0400  ///<  0 = Isolate PHY
#define BMCR_RESTART_AUTONEGOTIATION    0x0200  ///<  1 = Restart autonegotiation
#define BMCR_FULL_DUPLEX                0x0100  ///<  Full duplex operation
#define BMCR_HALF_DUPLEX                0       ///<  Half duplex operation
#define BMCR_COLLISION_TEST             0x0080  ///<  1 = Collision test enabled

//  BSMR - Register 1

#define BMSR_100BASET4                  0x8000  ///<  1 = 100BASE-T4 mode
#define BMSR_100BASETX_FDX              0x4000  ///<  1 = 100BASE-TX full duplex
#define BMSR_100BASETX_HDX              0x2000  ///<  1 = 100BASE-TX half duplex
#define BMSR_10BASET_FDX                0x1000  ///<  1 = 10BASE-T full duplex
#define BMSR_10BASET_HDX                0x0800  ///<  1 = 10BASE-T half duplex
#define BMSR_MF                         0x0040  ///<  1 = PHY accepts frames with preamble suppressed
#define BMSR_AUTONEG_CMPLT              0x0020  ///<  1 = Autonegotiation complete
#define BMSR_RF                         0x0010  ///<  1 = Remote fault
#define BMSR_AUTONEG                    0x0008  ///<  1 = Able to perform autonegotiation
#define BMSR_LINKST                     0x0004  ///<  1 = Link up
#define BMSR_JABBER_DETECT              0x0002  ///<  1 = jabber condition detected
#define BMSR_EXTENDED_CAPABILITY        0x0001  ///<  1 = Extended register capable

//  ANAR and ANLPAR Registers 4, 5

#define AN_NP                           0x8000  ///<  1 = Next page available
#define AN_ACK                          0x4000  ///<  1 = Link partner acknowledged
#define AN_RF                           0x2000  ///<  1 = Remote fault indicated by link partner
#define AN_FCS                          0x0400  ///<  1 = Flow control ability
#define AN_T4                           0x0200  ///<  1 = 100BASE-T4 support
#define AN_TX_FDX                       0x0100  ///<  1 = 100BASE-TX Full duplex
#define AN_TX_HDX                       0x0080  ///<  1 = 100BASE-TX support
#define AN_10_FDX                       0x0040  ///<  1 = 10BASE-T Full duplex
#define AN_10_HDX                       0x0020  ///<  1 = 10BASE-T support
#define AN_CSMA_CD                      0x0001  ///<  1 = IEEE 802.3 CSMA/CD support

//------------------------------------------------------------------------------
//  Data Types
//------------------------------------------------------------------------------

/**
  Ethernet header layout

  IEEE 802.3-2002 Part 3 specification, section 3.1.1.
**/
#pragma pack(1)
typedef struct {
  UINT8 dest_addr[PXE_HWADDR_LEN_ETHER];  ///<  Destination LAN address
  UINT8 src_addr[PXE_HWADDR_LEN_ETHER];   ///<  Source LAN address
  UINT16 type;                            ///<  Protocol or length
} ETHERNET_HEADER;
#pragma pack()

/**
  Receive and Transmit packet structure
**/
#pragma pack(1)
typedef struct _RX_TX_PACKET {
  struct _RX_TX_PACKET * pNext;       ///<  Next receive packet
  UINT16 Length;                      ///<  Packet length
  UINT16 LengthBar;                   ///<  Complement of the length
  UINT8 Data[ AX88772_MAX_PKT_SIZE ]; ///<  Received packet data
} RX_TX_PACKET;
#pragma pack()

/**
  AX88772 control structure

  The driver uses this structure to manage the Asix AX88772 10/100
  Ethernet controller.
**/
typedef struct {
  UINTN Signature;          ///<  Structure identification

  //
  //  USB data
  //
  EFI_HANDLE Controller;        ///<  Controller handle
  EFI_USB_IO_PROTOCOL * pUsbIo; ///<  USB driver interface

  //
  //  Simple network protocol data
  //
  EFI_SIMPLE_NETWORK_PROTOCOL SimpleNetwork;  ///<  Driver's network stack interface
  EFI_SIMPLE_NETWORK_MODE SimpleNetworkData;  ///<  Data for simple network

  //
  // Ethernet controller data
  //
  BOOLEAN bInitialized;     ///<  Controller initialized
  VOID * pTxBuffer;         ///<  Last transmit buffer
  UINT16 PhyId;             ///<  PHY ID

  //
  //  Link state
  //
  BOOLEAN b100Mbps;         ///<  Current link speed, FALSE = 10 Mbps
  BOOLEAN bComplete;        ///<  Current state of auto-negotiation
  BOOLEAN bFullDuplex;      ///<  Current duplex
  BOOLEAN bLinkUp;          ///<  Current link state
  BOOLEAN bLinkIdle;        ///<  TRUE = No received traffic
  EFI_EVENT Timer;          ///<  Timer to monitor link state and receive packets
  UINTN PollCount;          ///<  Number of times the autonegotiation status was polled

  //
  //  Receive buffer list
  //
  RX_TX_PACKET * pRxHead;   ///<  Head of receive packet list
  RX_TX_PACKET * pRxTail;   ///<  Tail of receive packet list
  RX_TX_PACKET * pRxFree;   ///<  Free packet list
  INT32 MulticastHash[2];   ///<  Hash table for multicast destination addresses
  UINT8 * pBulkInBuff;      ///<  Buffer for Usb Bulk
} NIC_DEVICE;

#define DEV_FROM_SIMPLE_NETWORK(a)  CR (a, NIC_DEVICE, SimpleNetwork, DEV_SIGNATURE)  ///< Locate NIC_DEVICE from Simple Network Protocol

//------------------------------------------------------------------------------
// Simple Network Protocol
//------------------------------------------------------------------------------

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

//------------------------------------------------------------------------------
// Support Routines
//------------------------------------------------------------------------------

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
  );

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
  );

/**
  Clear the multicast hash table

  @param [in] pNicDevice       Pointer to the NIC_DEVICE structure

**/
VOID
Ax88772MulticastClear (
  IN NIC_DEVICE * pNicDevice
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

//------------------------------------------------------------------------------
// EFI Component Name Protocol Support
//------------------------------------------------------------------------------

extern EFI_COMPONENT_NAME_PROTOCOL   gComponentName;  ///<  Component name protocol declaration
extern EFI_COMPONENT_NAME2_PROTOCOL  gComponentName2; ///<  Component name 2 protocol declaration

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param [in] pThis             A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param [in] pLanguage         A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 3066 or ISO 639-2 language code format.
  @param [out] ppDriverName     A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER DriverName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
GetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL * pThis,
  IN  CHAR8 * pLanguage,
  OUT CHAR16 ** ppDriverName
  );


/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param [in] pThis             A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param [in] ControllerHandle  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.
  @param [in] ChildHandle       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.
  @param [in] pLanguage         A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 3066 or ISO 639-2 language code format.
  @param [out] ppControllerName A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.
  @retval EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER ControllerName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
GetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL * pThis,
  IN  EFI_HANDLE ControllerHandle,
  IN OPTIONAL EFI_HANDLE ChildHandle,
  IN  CHAR8 * pLanguage,
  OUT CHAR16 ** ppControllerName
  );

//------------------------------------------------------------------------------

#endif  //  _AX88772_H_
