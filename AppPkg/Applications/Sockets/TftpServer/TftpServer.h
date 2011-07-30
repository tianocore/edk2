/** @file
  Definitions for the TFTP server.

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _TFTP_SERVER_H_
#define _TFTP_SERVER_H_

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <Uefi.h>

#include <Guid/EventGroup.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/BlockIo.h>

#include <netinet/in.h>

#include <sys/EfiSysCall.h>
#include <sys/poll.h>
#include <sys/socket.h>

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------

#if defined(_MSC_VER)           /* Handle Microsoft VC++ compiler specifics. */
#define DBG_ENTER()             DEBUG (( DEBUG_INFO, "Entering " __FUNCTION__ "\n" )) ///<  Display routine entry
#define DBG_EXIT()              DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ "\n" ))  ///<  Display routine exit
#define DBG_EXIT_DEC(Status)    DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ ", Status: %d\n", Status ))      ///<  Display routine exit with decimal value
#define DBG_EXIT_HEX(Status)    DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ ", Status: 0x%08x\n", Status ))  ///<  Display routine exit with hex value
#define DBG_EXIT_STATUS(Status) DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ ", Status: %r\n", Status ))      ///<  Display routine exit with status value
#define DBG_EXIT_TF(Status)     DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ ", returning %s\n", (FALSE == Status) ? L"FALSE" : L"TRUE" ))  ///<  Display routine with TRUE/FALSE value
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

#define DEBUG_SOCKET_POLL       0x40000000  ///<  Display the socket poll messages
#define DEBUG_PORT_WORK         0x20000000  ///<  Display the port work messages
#define DEBUG_SERVER_TIMER      0x10000000  ///<  Display the socket poll messages
#define DEBUG_TFTP_PORT         0x08000000  ///<  Display the TFTP port messages
#define DEBUG_TFTP_REQUEST      0x04000000  ///<  Display the TFTP request messages
#define DEBUG_TX                0x02000000  ///<  Display transmit messages
#define DEBUG_RX                0x01000000  ///<  Display receive messages
#define DEBUG_TFTP_ACK          0x00800000  ///<  Display the TFTP ACK messages

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

//------------------------------------------------------------------------------
//  Data Types
//------------------------------------------------------------------------------

/**
  Port control structure
**/
typedef struct _TSDT_CONNECTION_CONTEXT TSDT_CONNECTION_CONTEXT;
typedef struct _TSDT_CONNECTION_CONTEXT {
  //
  //  Remote connection management
  //
  TSDT_CONNECTION_CONTEXT * pNext;  ///<  Next context in the connection list
  struct sockaddr_in RemoteAddress; ///<  Remote address

  //
  //  TFTP management parameters
  //
  UINT16 AckNext;               ///<  Next block to be received
  BOOLEAN bExpectAck;           ///<  TRUE for read, FALSE for write
  UINT32 BlockSize;             ///<  Negotiated block size
  UINT32 Timeout;               ///<  Number of seconds to wait before retransmission

  //
  //  File management parameters
  //
  EFI_HANDLE File;              ///<  NULL while file is closed
  UINT64 LengthInBytes;         ///<  Size of the file
  UINT64 MaxTransferSize;       ///<  Maximum transfer size
  BOOLEAN bEofSent;             ///<  End of file sent
  UINT8 * pBuffer;              ///<  Pointer into the file data
  UINT8 * pEnd;                 ///<  End of the file data
  UINT8 FileData [ 64 * TFTP_MAX_BLOCK_SIZE ];  ///<  File data to send

  //
  //  Buffer management
  //
  ssize_t   TxBytes;            ///<  Bytes in the TX buffer
  UINT8     TxBuffer [ 2 + 2 + TFTP_MAX_BLOCK_SIZE ]; ///<  Transmit buffer
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
  //  TFTP port management
  //
  BOOLEAN   bTimerRunning;      ///<  Port creation timer status
  EFI_EVENT TimerEvent;         ///<  Timer to open TFTP port
  struct pollfd TftpPort;       ///<  Poll descriptor for the TFTP port
  struct sockaddr_in TftpServerAddress; ///<  Address of the local TFTP server

  //
  //  Request management
  //
  struct sockaddr_in RemoteAddress; ///<  Remote address
  ssize_t   RxBytes;                ///<  Receive data length in bytes
  UINT8     RxBuffer [ 2 + 2 + TFTP_MAX_BLOCK_SIZE ]; ///<  Receive buffer

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

**/
VOID
TftpProcessRequest (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN TSDT_CONNECTION_CONTEXT * pContext
  );

/**
  Build and send an error packet

  @param [in] pTftpServer The TFTP server control structure address.
  @param [in] pContext    The context structure address.
  @param [in] Error       Error number for the packet
  @param [in] pError      Zero terminated error string address

  @retval EFI_SUCCESS     Message processed successfully

**/
EFI_STATUS
TftpSendError (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN TSDT_CONNECTION_CONTEXT * pContext,
  IN UINT16 Error,
  IN UINT8 * pError
  );

/**
  Send the next block of file system data

  @param [in] pTftpServer The TFTP server control structure address.
  @param [in] pContext    The context structure address.

  @retval EFI_SUCCESS   Message processed successfully

**/
EFI_STATUS
TftpSendNextBlock (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN TSDT_CONNECTION_CONTEXT * pContext
  );

/**
  TFTP port creation timer routine

  This routine polls the socket layer waiting for the initial network connection
  which will enable the creation of the TFTP port.  The socket layer will manage
  the coming and going of the network connections after that until the last network
  connection is broken.

  @param [in] pTftpServer  The TFTP server control structure address.

**/
VOID
TftpServerTimer (
  IN TSDT_TFTP_SERVER * pTftpServer
  );

/**
  Start the TFTP server port creation timer

  @param [in] pTftpServer  The TFTP server control structure address.

  @retval EFI_SUCCESS         The timer was successfully started.
  @retval EFI_ALREADY_STARTED The timer is already running.
  @retval Other               The timer failed to start.

**/
EFI_STATUS
TftpServerTimerStart (
  IN TSDT_TFTP_SERVER * pTftpServer
  );

/**
  Stop the TFTP server port creation timer

  @param [in] pTftpServer  The TFTP server control structure address.

  @retval EFI_SUCCESS   The TFTP port timer is stopped
  @retval Other         Failed to stop the TFTP port timer

**/
EFI_STATUS
TftpServerTimerStop (
  IN TSDT_TFTP_SERVER * pTftpServer
  );

/**
  Send the next TFTP packet

  @param [in] pTftpServer   The TFTP server control structure address.
  @param [in] pContext      The context structure address.

  @retval EFI_SUCCESS   Message processed successfully

**/
EFI_STATUS
TftpTxPacket (
  IN TSDT_TFTP_SERVER * pTftpServer,
  IN TSDT_CONNECTION_CONTEXT * pContext
  );

//------------------------------------------------------------------------------

#endif  //  _TFTP_SERVER_H_
