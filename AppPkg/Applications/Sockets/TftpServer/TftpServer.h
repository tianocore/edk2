/** @file
  Definitions for the TFTP server.

  Copyright (c) 2011, 2012, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TFTP_SERVER_H_
#define _TFTP_SERVER_H_

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Uefi.h>

#include <Guid/EventGroup.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/BlockIo.h>

#include <netinet/in.h>
#include <netinet6/in6.h>

#include <sys/EfiSysCall.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------

#if defined(_MSC_VER)           /* Handle Microsoft VC++ compiler specifics. */
#define DBG_ENTER()             DEBUG (( DEBUG_ENTER_EXIT, "Entering " __FUNCTION__ "\n" )) ///<  Display routine entry
#define DBG_EXIT()              DEBUG (( DEBUG_ENTER_EXIT, "Exiting " __FUNCTION__ "\n" ))  ///<  Display routine exit
#define DBG_EXIT_DEC(Status)    DEBUG (( DEBUG_ENTER_EXIT, "Exiting " __FUNCTION__ ", Status: %d\n", Status ))      ///<  Display routine exit with decimal value
#define DBG_EXIT_HEX(Status)    DEBUG (( DEBUG_ENTER_EXIT, "Exiting " __FUNCTION__ ", Status: 0x%08x\n", Status ))  ///<  Display routine exit with hex value
#define DBG_EXIT_STATUS(Status) DEBUG (( DEBUG_ENTER_EXIT, "Exiting " __FUNCTION__ ", Status: %r\n", Status ))      ///<  Display routine exit with status value
#define DBG_EXIT_TF(Status)     DEBUG (( DEBUG_ENTER_EXIT, "Exiting " __FUNCTION__ ", returning %s\n", (FALSE == Status) ? L"FALSE" : L"TRUE" ))  ///<  Display routine with TRUE/FALSE value
#else   //  _MSC_VER
#define DBG_ENTER()
#define DBG_EXIT()
#define DBG_EXIT_DEC(Status)
#define DBG_EXIT_HEX(Status)
#define DBG_EXIT_STATUS(Status)
#define DBG_EXIT_TF(Status)
#endif  //  _MSC_VER

#define DIM(x)    ( sizeof ( x ) / sizeof ( x[0] ))   ///<  Compute the number of entries in an array

//------------------------------------------------------------------------------
//  Constants
//------------------------------------------------------------------------------

#define ACK_SHIFT               4           ///<  Number of samples in ACK average

#define DEBUG_WINDOW            0x00000001  ///<  Display the window messages
#define DEBUG_TX_PACKET         0x00000002  ///<  Display the transmit packet messages
#define DEBUG_FILE_BUFFER       0x00000004  ///<  Display the file buffer messages
#define DEBUG_SERVER_TIMER      0x00000008  ///<  Display the socket poll messages
#define DEBUG_TFTP_REQUEST      0x00000010  ///<  Display the TFTP request messages
#define DEBUG_PORT_WORK         0x00000020  ///<  Display the port work messages
#define DEBUG_SOCKET_POLL       0x00000040  ///<  Display the socket poll messages
#define DEBUG_TFTP_PORT         0x00000080  ///<  Display the TFTP port messages
#define DEBUG_TX                0x00000100  ///<  Display transmit messages
#define DEBUG_RX                0x00000200  ///<  Display receive messages
#define DEBUG_TFTP_ACK          0x00000400  ///<  Display the TFTP ACK messages
#define DEBUG_ENTER_EXIT        0x00000800  ///<  Display entry and exit messages

#define MAX_PACKETS             8           ///<  Maximum number of packets in the window

#define TFTP_PORT_POLL_DELAY  ( 2 * 1000 )  ///<  Delay in milliseconds for attempts to open the TFTP port
#define CLIENT_POLL_DELAY     50            ///<  Delay in milliseconds between client polls

#define TPL_TFTP_SERVER        TPL_CALLBACK ///<  TPL for routine synchronization

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

#define TFTP_SERVER_SIGNATURE       SIGNATURE_32('T','F','T','P') ///<  TSDT_TFTP_SERVER memory signature

//
//  See: http://www.rfc-editor.org/rfc/pdfrfc/rfc1350.txt.pdf
//
//  TFTP Operations
//

#define TFTP_OP_READ_REQUEST      1     ///<  Read request, zero terminated file name, zero terminated mode
#define TFTP_OP_WRITE_REQUEST     2     ///<  Write request, zero terminated file name, zero terminated mode
#define TFTP_OP_DATA              3     ///<  Data block, end-of-file indicated by short block
#define TFTP_OP_ACK               4     ///<  ACK block number
#define TFTP_OP_ERROR             5     ///<  Error number and explaination
#define TFTP_OP_OACK              6     ///<  ACK the options

#define TFTP_MAX_BLOCK_SIZE       4096  ///<  Maximum block size

#define TFTP_ERROR_SEE_MSG          0   ///<  See the error message
#define TFTP_ERROR_NOT_FOUND        1   ///<  File not found
#define TFTP_ERROR_ACCESS_VIOLATION 2   ///<  Access violation
#define TFTP_ERROR_DISK_FULL        3   ///<  Disk full
#define TFTP_ERROR_ILLEGAL_OP       4   ///<  Illegal operation
#define TFTP_ERROR_UNKNOWN_XFER_ID  5   ///<  Unknown transfer ID
#define TFTP_ERROR_FILE_EXISTS      6   ///<  File already exists
#define TFTP_ERROR_NO_SUCH_USER     7   ///<  No such user

//------------------------------------------------------------------------------
//  Data Types
//------------------------------------------------------------------------------

/**
  Packet structure
**/
typedef struct _TFTP_PACKET TFTP_PACKET;
typedef struct _TFTP_PACKET {
  TFTP_PACKET * pNext;          ///<  Next packet in list
  UINT64    TxTime;             ///<  Time the transmit was performed
  ssize_t   TxBytes;            ///<  Bytes in the TX buffer
  UINT32    RetryCount;         ///<  Number of transmissions
  UINT16    BlockNumber;        ///<  Block number of this packet
  UINT8     TxBuffer[ 2 + 2 + TFTP_MAX_BLOCK_SIZE ];  ///<  Transmit buffer
} GCC_TFTP_PACKET;

/**
  Port control structure
**/
typedef struct _TSDT_CONNECTION_CONTEXT TSDT_CONNECTION_CONTEXT;
typedef struct _TSDT_CONNECTION_CONTEXT {
  //
  //  Remote connection management
  //
  TSDT_CONNECTION_CONTEXT * pNext;    ///<  Next context in the connection list
  struct sockaddr_in6 RemoteAddress;  ///<  Remote address
  int SocketFd;                       ///<  Socket file descriptor

  //
  //  File management parameters
  //
  FILE * File;                  ///<  NULL while file is closed
  UINT64 LengthInBytes;         ///<  Size of the file
  UINT64 BytesRemaining;        ///<  Number of bytes remaining to be sent
  UINT64 BytesToSend;           ///<  Number of bytes to send
  UINT64 ValidBytes;            ///<  Number of valid bytes in the buffer
  BOOLEAN bEofSent;             ///<  End of file sent
  UINT8 * pFill;                ///<  Next portion of the buffer to fill
  UINT8 * pBuffer;              ///<  Pointer into the file data
  UINT8 * pEnd;                 ///<  End of the file data
  UINT8 FileData[ 2 * MAX_PACKETS * TFTP_MAX_BLOCK_SIZE ];  ///<  File data to send
  UINT64 TimeStart;             ///<  Start of file transfer

  //
  //  TFTP management parameters
  //
  UINT16 BlockNumber;           ///<  Next block to be transmitted
  UINT32 BlockSize;             ///<  Negotiated block size

  //
  //  Window management
  //
  UINT32 AckCount;              ///<  Number of ACKs to receive before increasing the window
  UINT32 PacketsInWindow;       ///<  Number of packets in the window
  UINT32 Threshold;             ///<  Size of window when ACK count becomes logrithmic
  UINT32 WindowSize;            ///<  Size of the transmit window
  UINT64 MaxTimeout;            ///<  Maximum number of seconds to wait before retransmission
  UINT64 Rtt2x;                 ///<  Twice the average round trip time in nanoseconds

  //
  //  Buffer management
  //
  TFTP_PACKET * pFreeList;      ///<  List of free packets
  TFTP_PACKET * pTxHead;        ///<  First packet in the list of packets for transmission
  TFTP_PACKET * pTxTail;        ///<  Last packet in the list of packets for transmission
  TFTP_PACKET ErrorPacket;      ///<  Error packet
  TFTP_PACKET Tx[ MAX_PACKETS ];///<  Transmit packets
}GCC_TSDT_CONNECTION_CONTEXT;

/**
  TFTP server control structure
**/
typedef struct {
  UINTN Signature;              ///<  Structure identification

  //
  //  Image attributes
  //
  EFI_HANDLE ImageHandle;       ///<  Image handle

  //
  //  Performance management
  //
  UINT64 ClockFrequency;        ///<  Frequency of the clock
  UINT64 Time1;                 ///<  Clock value after rollover
  UINT64 Time2;                 ///<  Clock value before rollover
  UINT64 RxTime;                ///<  Time when the packet was recevied

  //
  //  TFTP port management
  //
  EFI_EVENT TimerEvent;         ///<  Timer to open TFTP port
  int Udpv4Index;               ///<  Entry for UDPv4
  int Udpv6Index;               ///<  Entry for UDPv6
  int Entries;                  ///<  Number of TFTP ports
  struct pollfd TftpPort [ 2 ]; ///<  Poll descriptor for the TFTP ports (UDP4, UDP6)

  //
  //  Request management
  //
  union {
    struct sockaddr_in v4;      ///<  UDP4 address
    struct sockaddr_in6 v6;     ///<  UDP6 address
  } RemoteAddress;              ///<  Remote address
  ssize_t RxBytes;              ///<  Receive data length in bytes
  UINT8 RxBuffer[ 2 + 2 + TFTP_MAX_BLOCK_SIZE ];  ///<  Receive buffer

  //
  //  Client port management
  //
  TSDT_CONNECTION_CONTEXT * pContextList; ///<  List of connection context structures
} TSDT_TFTP_SERVER;

//#define SERVER_FROM_SERVICE(a) CR(a, TSDT_TFTP_SERVER, ServiceBinding, TFTP_SERVER_SIGNATURE) ///< Locate DT_LAYER from service binding

extern TSDT_TFTP_SERVER mTftpServer;

//------------------------------------------------------------------------------
// Support routines
//------------------------------------------------------------------------------

/**
  Queue data packets for transmission

  @param [in] pContext    Connection context structure address

  @retval TRUE if a read error occurred

**/
BOOLEAN
PacketFill (
  IN TSDT_CONNECTION_CONTEXT * pContext
  );

/**
  Free the packet

  @param [in] pContext    Address of a ::TSDT_CONNECTION_CONTEXT structure
  @param [in] pPacket     Address of a ::TFTP_PACKET structure

**/
VOID
PacketFree(
  IN TSDT_CONNECTION_CONTEXT * pContext,
  IN TFTP_PACKET * pPacket
  );

/**
  Get a packet for transmission

  @param [in] pContext    Address of a ::TSDT_CONNECTION_CONTEXT structure

  @retval Address of a ::TFTP_PACKET structure

**/
TFTP_PACKET *
PacketGet (
  IN TSDT_CONNECTION_CONTEXT * pContext
  );

/**
  Queue the packet for transmission

  @param [in] pContext    Address of a ::TSDT_CONNECTION_CONTEXT structure
  @param [in] pPacket     Address of a ::TFTP_PACKET structure

  @retval TRUE if a transmission error has occurred

**/
BOOLEAN
PacketQueue (
  IN TSDT_CONNECTION_CONTEXT * pContext,
  IN TFTP_PACKET * pPacket
  );

/**
  Transmit the packet

  @param [in] pContext    Address of a ::TSDT_CONNECTION_CONTEXT structure
  @param [in] pPacket     Address of a ::TFTP_PACKET structure

  @retval EFI_SUCCESS   Message processed successfully

**/
EFI_STATUS
PacketTx (
  IN TSDT_CONNECTION_CONTEXT * pContext,
  IN TFTP_PACKET * pPacket
  );

/**
  Build and send an error packet

  @param [in] pContext    The context structure address.
  @param [in] Error       Error number for the packet
  @param [in] pError      Zero terminated error string address

  @retval EFI_SUCCESS     Message processed successfully

**/
EFI_STATUS
SendError (
  IN TSDT_CONNECTION_CONTEXT * pContext,
  IN UINT16 Error,
  IN UINT8 * pError
  );

/**
  Process the TFTP request

  @param [in] pOption   Address of the first zero terminated option string
  @param [in] pValue    Address to receive the value

  @retval EFI_SUCCESS   Option translated into a value

**/
EFI_STATUS
TftpOptionValue (
  IN UINT8 * pOption,
  IN INT32 * pValue
  );

/**
  Process the TFTP request

  @param [in] pTftpServer The TFTP server control structure address.
  @param [in] pContext    Connection context structure address
  @param [in] SocketFd    Socket file descriptor

**/
VOID
TftpProcessRequest (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN TSDT_CONNECTION_CONTEXT * pContext,
  IN int SocketFd
  );

/**
  Process the read request

  @param [in] pTftpServer Address of the ::TSDT_TFTP_SERVER structure
  @param [in] pContext    Connection context structure address
  @param [in] SocketFd    Socket file descriptor

  @retval TRUE if the context should be closed

**/
BOOLEAN
TftpRead (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN TSDT_CONNECTION_CONTEXT * pContext,
  IN int SocketFd
  );

/**
  Update the window due to the ACK

  @param [in] pTftpServer Address of the ::TSDT_TFTP_SERVER structure
  @param [in] pContext    Address of a ::TSDT_CONNECTION_CONTEXT structure
  @param [in] pPacket     Address of a ::TFTP_PACKET structure

**/
VOID
WindowAck (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN TSDT_CONNECTION_CONTEXT * pContext,
  IN TFTP_PACKET * pPacket
  );

/**
  A timeout has occurred, close the window

  @param [in] pContext    Address of a ::TSDT_CONNECTION_CONTEXT structure

**/
VOID
WindowTimeout (
  IN TSDT_CONNECTION_CONTEXT * pContext
  );

//------------------------------------------------------------------------------

#endif  //  _TFTP_SERVER_H_
