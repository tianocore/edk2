/** @file
  Data source for network testing.

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <errno.h>
#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <netinet/in.h>

#include <Protocol/ServiceBinding.h>
#include <Protocol/Tcp4.h>

#include <sys/EfiSysCall.h>
#include <sys/poll.h>
#include <sys/socket.h>


#define RANGE_SWITCH                2048  ///<  Switch display ranges
#define DATA_RATE_UPDATE_SHIFT      2     ///<  2n seconds between updates
#define AVERAGE_SHIFT_COUNT ( 6 - DATA_RATE_UPDATE_SHIFT )  ///<  2n samples in average

#define TPL_DATASOURCE      TPL_CALLBACK  ///<  Synchronization TPL

#define PACKET_SIZE                 1448  ///<  Size of data packets
#define DATA_BUFFER_SIZE    (( 65536 / PACKET_SIZE ) * PACKET_SIZE )  ///<  Buffer size in bytes


//
//  Socket Data
//
int Socket = -1;

//
//  TCP V4 Data
//
BOOLEAN bTcp4;                      ///<  TRUE if TCP4 is being used
BOOLEAN bTcp4Connected;             ///<  TRUE if connected to remote system
BOOLEAN bTcp4Connecting;            ///<  TRUE while connection in progress
UINTN Tcp4Index;                    ///<  Index into handle array
EFI_HANDLE Tcp4Controller;          ///<  Network controller handle
EFI_HANDLE Tcp4Handle;              ///<  TCP4 port handle
EFI_TCP4_PROTOCOL * pTcp4Protocol;  ///<  TCP4 protocol pointer
EFI_SERVICE_BINDING_PROTOCOL * pTcp4Service;  ///<  TCP4 Service binding
EFI_TCP4_CONFIG_DATA Tcp4ConfigData;///<  TCP4 configuration data
EFI_TCP4_OPTION Tcp4Option;         ///<  TCP4 port options
EFI_TCP4_CLOSE_TOKEN Tcp4CloseToken;///<  Close control
EFI_TCP4_CONNECTION_TOKEN Tcp4ConnectToken; ///<  Connection control
EFI_TCP4_LISTEN_TOKEN Tcp4ListenToken;      ///<  Listen control
EFI_TCP4_IO_TOKEN Tcp4TxToken;      ///<  Normal data token

//
//  Timer Data
//
volatile BOOLEAN bTick;
BOOLEAN bTimerRunning;
EFI_EVENT pTimer;

//
//  Remote IP Address Data
//
struct sockaddr_in RemoteHostAddress;
CHAR8 * pRemoteHost;

//
//  Traffic Data
//
UINT64 TotalBytesSent;
UINT64 PreviousBytes;
UINT64 AverageBytes;
UINT64 Samples;
UINT8 Buffer [ DATA_BUFFER_SIZE ];


//
//  Forward routine declarations
//
EFI_STATUS TimerStart ( UINTN Milliseconds );


/**
  Check for control C entered at console

  @retval  EFI_SUCCESS  Control C not entered
  @retval  EFI_ABORTED  Control C entered
**/
EFI_STATUS
ControlCCheck (
  )
{
  EFI_STATUS Status;

  //
  //  Assume no user intervention
  //
  Status = EFI_SUCCESS;

  //
  //  Display user stop request
  //
  if ( EFI_ERROR ( Status )) {
    DEBUG (( DEBUG_INFO,
              "User stop request!\r\n" ));
  }

  //
  //  Return the check status
  //
  return Status;
}


/**
  Get a digit

  @param [in] pDigit    The address of the next digit
  @param [out] pValue   The address to receive the value

  @return   Returns the address of the separator

**/
CHAR8 *
GetDigit (
  CHAR8 * pDigit,
  UINT32 * pValue
  )
{
  UINT32 Value;

  //
  //  Walk the digits
  //
  Value = 0;
  while (( '0' <= *pDigit ) && ( '9' >= *pDigit ))
  {
    //
    //  Make room for the new least significant digit
    //
    Value *= 10;

    //
    //  Convert the digit from ASCII to binary
    //
    Value += *pDigit - '0';

    //
    //  Set the next digit
    //
    pDigit += 1;
  }

  //
  //  Return the value
  //
  *pValue = Value;

  //
  //  Return the next separator
  //
  return pDigit;
}


/**
  Get the IP address

  @retval  EFI_SUCCESS  The IP address is valid
  @retval  Other        Failure to convert the IP address
**/
EFI_STATUS
IpAddress (
  )
{
  CHAR8 * pSeparator;
  INT32 RemoteAddress;
  EFI_STATUS Status;
  UINT32 Value1;
  UINT32 Value2;
  UINT32 Value3;
  UINT32 Value4;

  //
  //  Assume failure
  //
  Status = EFI_INVALID_PARAMETER;

  //
  //  Convert the IP address from a string to a numeric value
  //
  pSeparator = GetDigit ( pRemoteHost, &Value1 );
  if (( 255 >= Value1 ) && ( '.' == *pSeparator )) {
    pSeparator = GetDigit ( ++pSeparator, &Value2 );
    if (( 255 >= Value2 ) && ( '.' == *pSeparator )) {
      pSeparator = GetDigit ( ++pSeparator, &Value3 );
      if (( 255 >= Value3 ) && ( '.' == *pSeparator )) {
        pSeparator = GetDigit ( ++pSeparator, &Value4 );
        if (( 255 >= Value4 ) && ( 0 == *pSeparator )) {
          RemoteAddress = Value1
                        | ( Value2 << 8 )
                        | ( Value3 << 16 )
                        | ( Value4 << 24 );
          RemoteHostAddress.sin_addr.s_addr = (UINT32) RemoteAddress;
          Status = EFI_SUCCESS;
          DEBUG (( DEBUG_INFO,
                    "%d.%d.%d.%d: Remote host IP address\r\n",
                    Value1,
                    Value2,
                    Value3,
                    Value4 ));
        }
      }
    }
  }
  if ( EFI_ERROR ( Status )) {
    Print ( L"Invalid digit detected: %d\r\n", *pSeparator );
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Close the socket

  @retval  EFI_SUCCESS  The application is running normally
  @retval  Other        The user stopped the application
**/
EFI_STATUS
SocketClose (
  )
{
  int CloseStatus;
  EFI_STATUS Status;

  //
  //  Determine if the socket is open
  //
  Status = EFI_DEVICE_ERROR;
  if ( -1 != Socket ) {
    //
    //  Attempt to close the socket
    //
    CloseStatus = close ( Socket );
    if ( 0 == CloseStatus ) {
      DEBUG (( DEBUG_INFO,
                "0x%08x: Socket closed\r\n",
                Socket ));
      Socket = -1;
      Status = EFI_SUCCESS;
    }
    else {
      DEBUG (( DEBUG_ERROR,
                "ERROR: Failed to close socket, errno: %d\r\n",
                errno ));
    }
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Connect the socket

  @retval  EFI_SUCCESS  The application is running normally
  @retval  Other        The user stopped the application
**/
EFI_STATUS
SocketConnect (
  )
{
  int ConnectStatus;
  UINT32 RemoteAddress;
  EFI_STATUS Status;

  //
  //  Display the connecting message
  //
  RemoteAddress = RemoteHostAddress.sin_addr.s_addr;
  Print ( L"Connecting to remote system %d.%d.%d.%d:%d\r\n",
          RemoteAddress & 0xff,
          ( RemoteAddress >> 8 ) & 0xff,
          ( RemoteAddress >> 16 ) & 0xff,
          ( RemoteAddress >> 24 ) & 0xff,
          htons ( RemoteHostAddress.sin_port ));

  //
  //  Connect to the remote system
  //
  Status = EFI_SUCCESS;
  do {
    //
    //  Check for user stop request
    //
    while ( ! bTick ) {
      Status = ControlCCheck ( );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }
    bTick = FALSE;
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Connect to the remote system
    //
    ConnectStatus = connect ( Socket,
                              (struct sockaddr *) &RemoteHostAddress,
                              RemoteHostAddress.sin_len );
    if ( -1 != ConnectStatus ) {
      Print ( L"Connected to remote system %d.%d.%d.%d:%d\r\n",
              RemoteAddress & 0xff,
              ( RemoteAddress >> 8 ) & 0xff,
              ( RemoteAddress >> 16 ) & 0xff,
              ( RemoteAddress >> 24 ) & 0xff,
              htons ( RemoteHostAddress.sin_port ));
    }
    else {
      //
      //  Close the socket and try again
      //
      if ( EAGAIN != errno ) {
        Status = EFI_NOT_STARTED;
        break;
      }
    }
  } while ( -1 == ConnectStatus );

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Create the socket

  @retval  EFI_SUCCESS  The application is running normally
  @retval  Other        The user stopped the application
**/
EFI_STATUS
SocketNew (
  )
{
  EFI_STATUS Status;

  //
  //  Loop creating the socket
  //
  DEBUG (( DEBUG_INFO,
            "Creating the socket\r\n" ));
  do {
    //
    //  Check for user stop request
    //
    Status = ControlCCheck ( );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Attempt to create the socket
    //
    Socket = socket ( AF_INET,
                      SOCK_STREAM,
                      IPPROTO_TCP );
    if ( -1 != Socket ) {
      DEBUG (( DEBUG_INFO,
                "0x%08x: Socket created\r\n",
                Socket ));
      break;
    }
  } while ( -1 == Socket );

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Send data over the socket

  @retval  EFI_SUCCESS  The application is running normally
  @retval  Other        The user stopped the application
**/
EFI_STATUS
SocketSend (
  )
{
  size_t BytesSent;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  //
  //  Restart the timer
  //
  TimerStart ( 1000 << DATA_RATE_UPDATE_SHIFT );

  //
  //  Loop until the connection breaks or the user stops
  //
  do {
    //
    //  Check for user stop request
    //
    Status = ControlCCheck ( );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Send some bytes
    //
    BytesSent = write ( Socket, &Buffer[0], sizeof ( Buffer ));
    if ( -1 == BytesSent ) {
      DEBUG (( DEBUG_INFO,
                "ERROR: send failed, errno: %d\r\n",
                errno ));

      //
      //  Try again
      //
      Status = EFI_SUCCESS;

//
//  Exit now
//
Status = EFI_NOT_STARTED;
      break;
    }

    //
    //  Synchronize with the TimerCallback routine
    //
    TplPrevious = gBS->RaiseTPL ( TPL_DATASOURCE );

    //
    //  Account for the data sent
    //
    TotalBytesSent += BytesSent;

    //
    //  Release the TimerCallback routine synchronization
    //
    gBS->RestoreTPL ( TplPrevious );
  } while ( !EFI_ERROR ( Status ));

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Open the network connection and send the data.

  @retval EFI_SUCCESS   Continue looping
  @retval other         Stopped by user's Control-C input

**/
EFI_STATUS
SocketOpen (
  )
{
  EFI_STATUS Status;

  //
  //  Use do/while and break instead of goto
  //
  do
  {
    //
    //  Wait for the network layer to initialize
    //
    Status = SocketNew ( );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Wait for the remote network application to start
    //
    Status = SocketConnect ( );
    if ( EFI_NOT_STARTED == Status ) {
      Status = SocketClose ( );
      continue;
    }
    else if ( EFI_SUCCESS != Status ) {
      //
      //  Control-C
      //
      break;
    }

    //
    //  Send data until the connection breaks
    //
    Status = SocketSend ( );
    if ( EFI_ERROR ( Status )) {
      break;
    }
  } while ( FALSE );

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Close the TCP connection

  @retval  EFI_SUCCESS  The application is running normally
  @retval  Other        The user stopped the application
**/
EFI_STATUS
Tcp4Close (
  )
{
  UINTN Index;
  UINT8 * pIpAddress;
  EFI_STATUS Status;

  //
  //  Close the port
  //
  if ( bTcp4Connected ) {
    Tcp4CloseToken.AbortOnClose = TRUE;
    Status = pTcp4Protocol->Close ( pTcp4Protocol,
                                    &Tcp4CloseToken );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to start the TCP port close, Status: %r\r\n",
                Status ));
    }
    else {
      Status = gBS->WaitForEvent ( 1,
                                   &Tcp4CloseToken.CompletionToken.Event,
                                    &Index );
      if ( EFI_ERROR ( Status )) {
        DEBUG (( DEBUG_ERROR,
                  "ERROR - Failed to wait for close event, Status: %r\r\n",
                  Status ));
      }
      else {
        Status = Tcp4CloseToken.CompletionToken.Status;
        if ( EFI_ERROR ( Status )) {
          DEBUG (( DEBUG_ERROR,
                    "ERROR - Failed to close the TCP port, Status: %r\r\n",
                    Status ));
        }
        else {
          DEBUG (( DEBUG_INFO,
                    "0x%08x: TCP port closed\r\n",
                    pTcp4Protocol ));
          bTcp4Connected = FALSE;

          //
          //  Display the port closed message
          //
          pIpAddress = (UINT8 *)&RemoteHostAddress.sin_addr.s_addr;
          Print ( L"Closed connection to %d.%d.%d.%d:%d\r\n",
                  pIpAddress[0],
                  pIpAddress[1],
                  pIpAddress[2],
                  pIpAddress[3],
                  htons ( RemoteHostAddress.sin_port ));
        }
      }
    }
  }

  //
  //  Release the events
  //
  if ( NULL != Tcp4TxToken.CompletionToken.Event ) {
    Status = gBS->CloseEvent ( Tcp4TxToken.CompletionToken.Event );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_INFO,
                "0x%08x: TX event closed\r\n",
                Tcp4TxToken.CompletionToken.Event ));
      Tcp4TxToken.CompletionToken.Event = NULL;
    }
    else {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to close the Tcp4TxToken event, Status: %r\r\n",
                Status ));
    }
  }

  if ( NULL != Tcp4ListenToken.CompletionToken.Event ) {
    Status = gBS->CloseEvent ( Tcp4ListenToken.CompletionToken.Event );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_INFO,
                "0x%08x: Listen event closed\r\n",
                Tcp4ListenToken.CompletionToken.Event ));
      Tcp4ListenToken.CompletionToken.Event = NULL;
    }
    else {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to close the Tcp4ListenToken event, Status: %r\r\n",
                Status ));
    }
  }

  if ( NULL != Tcp4ConnectToken.CompletionToken.Event ) {
    Status = gBS->CloseEvent ( Tcp4ConnectToken.CompletionToken.Event );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_INFO,
                "0x%08x: Connect event closed\r\n",
                Tcp4ConnectToken.CompletionToken.Event ));
      Tcp4ConnectToken.CompletionToken.Event = NULL;
    }
    else {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to close the Tcp4ConnectToken event, Status: %r\r\n",
                Status ));
    }
  }

  if ( NULL != Tcp4CloseToken.CompletionToken.Event ) {
    Status = gBS->CloseEvent ( Tcp4CloseToken.CompletionToken.Event );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_INFO,
                "0x%08x: Close event closed\r\n",
                Tcp4CloseToken.CompletionToken.Event ));
      Tcp4CloseToken.CompletionToken.Event = NULL;
    }
    else {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to close the Tcp4CloseToken event, Status: %r\r\n",
                Status ));
    }
  }

  //
  //  Close the TCP protocol
  //
  if ( NULL != pTcp4Protocol ) {
    Status = gBS->CloseProtocol ( Tcp4Handle,
                                  &gEfiTcp4ProtocolGuid,
                                  gImageHandle,
                                  NULL );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to close the TCP protocol, Status: %r\r\n",
                Status ));
    }
    else {
      DEBUG (( DEBUG_INFO,
                "0x%08x: TCP4 protocol closed\r\n",
                pTcp4Protocol ));
      pTcp4Protocol = NULL;
    }
  }

  //
  //  Done with the TCP service
  //
  if ( NULL != Tcp4Handle ) {
    Status = pTcp4Service->DestroyChild ( pTcp4Service,
                                          Tcp4Handle );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to release TCP service handle, Status: %r\r\n",
                Status ));
    }
    else {
      DEBUG (( DEBUG_INFO,
                "Ox%08x: TCP service closed\r\n",
                Tcp4Handle ));
      Tcp4Handle = NULL;
    }
  }

  //
  //  Close the service protocol
  //
  if ( NULL != pTcp4Service ) {
    Status = gBS->CloseProtocol ( Tcp4Controller,
                                  &gEfiTcp4ServiceBindingProtocolGuid,
                                  gImageHandle,
                                  NULL );
    if ( !EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_INFO,
                "0x%08x: Controller closed gEfiTcp4ServiceBindingProtocolGuid protocol\r\n",
                Tcp4Controller ));
      pTcp4Service = NULL;
    }
    else {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to close the gEfiTcp4ServiceBindingProtocolGuid protocol, Status: %r\r\n",
                Status ));
    }
  }
  Tcp4Controller = NULL;
  bTcp4Connecting = TRUE;

  //
  //  Mark the connection as closed
  //
  Status = EFI_SUCCESS;

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Locate TCP protocol

  @retval EFI_SUCCESS   Protocol found
  @retval other         Protocl not found
**/
EFI_STATUS
Tcp4Locate (
  )
{
  UINTN HandleCount;
  EFI_HANDLE * pHandles;
  UINT8 * pIpAddress;
  EFI_STATUS Status;

  //
  //  Use do/while and break instead of goto
  //
  do {
    //
    //  Attempt to locate the next TCP adapter in the system
    //
    Status = gBS->LocateHandleBuffer ( ByProtocol,
                                       &gEfiTcp4ServiceBindingProtocolGuid,
                                       NULL,
                                       &HandleCount,
                                       &pHandles );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_WARN,
                "WARNING - No network controllers or TCP4 available, Status: %r\r\n",
                Status ));
      break;
    }

    //
    //  Wrap the index if necessary
    //
    if ( HandleCount <= Tcp4Index ) {
      Tcp4Index = 0;

      //
      //  Wait for the next timer tick
      //
      do {
      } while ( !bTick );
      bTick = FALSE;
    }

    //
    //  Display the connecting message
    //
    if ( bTcp4Connecting ) {
      pIpAddress = (UINT8 *)&RemoteHostAddress.sin_addr.s_addr;
      Print ( L"Connecting to %d.%d.%d.%d:%d\r\n",
              pIpAddress[0],
              pIpAddress[1],
              pIpAddress[2],
              pIpAddress[3],
              htons ( RemoteHostAddress.sin_port ));
      bTcp4Connecting = FALSE;
    }

    //
    //  Open the network controller's service protocol
    //
    Tcp4Controller = pHandles [ Tcp4Index++ ];
    Status = gBS->OpenProtocol (
                    Tcp4Controller,
                    &gEfiTcp4ServiceBindingProtocolGuid,
                    (VOID **) &pTcp4Service,
                    gImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to open gEfiTcp4ServiceBindingProtocolGuid on controller 0x%08x\r\n",
                Tcp4Controller ));
      Tcp4Controller = NULL;
      break;
    }
    DEBUG (( DEBUG_INFO,
              "0x%08x: Controller opened gEfiTcp4ServiceBindingProtocolGuid protocol\r\n",
              Tcp4Controller ));

    //
    //  Connect to the TCP service
    //
    Status = pTcp4Service->CreateChild ( pTcp4Service,
                                         &Tcp4Handle );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to open TCP service, Status: %r\r\n",
                Status ));
      Tcp4Handle = NULL;
      break;
    }
    DEBUG (( DEBUG_INFO,
              "Ox%08x: TCP service opened\r\n",
              Tcp4Handle ));

    //
    //  Locate the TCP protcol
    //
    Status = gBS->OpenProtocol ( Tcp4Handle,
                                 &gEfiTcp4ProtocolGuid,
                                 (VOID **)&pTcp4Protocol,
                                 gImageHandle,
                                 NULL,
                                 EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to open the TCP protocol, Status: %r\r\n",
                Status ));
      pTcp4Protocol = NULL;
      break;
    }
    DEBUG (( DEBUG_INFO,
              "0x%08x: TCP4 protocol opened\r\n",
              pTcp4Protocol ));
  }while ( FALSE );

  //
  //  Release the handle buffer
  //
  gBS->FreePool ( pHandles );

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Send data over the TCP4 connection

  @retval  EFI_SUCCESS  The application is running normally
  @retval  Other        The user stopped the application
**/
EFI_STATUS
Tcp4Send (
  )
{
  UINTN Index;
  EFI_TCP4_TRANSMIT_DATA Packet;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  //
  //  Restart the timer
  //
  TimerStart ( 1000 << DATA_RATE_UPDATE_SHIFT );

  //
  //  Initialize the packet
  //
  Packet.DataLength = sizeof ( Buffer );
  Packet.FragmentCount = 1;
  Packet.Push = FALSE;
  Packet.Urgent = FALSE;
  Packet.FragmentTable[0].FragmentBuffer = &Buffer[0];
  Packet.FragmentTable[0].FragmentLength = sizeof ( Buffer );
  Tcp4TxToken.Packet.TxData = &Packet;

  //
  //  Loop until the connection breaks or the user stops
  //
  do {
    //
    //  Check for user stop request
    //
    Status = ControlCCheck ( );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Send some bytes
    //
    Status = pTcp4Protocol->Transmit ( pTcp4Protocol,
                                       &Tcp4TxToken );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to start the transmit, Status: %r\r\n",
                Status ));

      //
      //  Try again
      //
      Status = EFI_SUCCESS;
      break;
    }

    //
    //  Wait for the transmit to complete
    //
    Status = gBS->WaitForEvent ( 1,
                                 &Tcp4TxToken.CompletionToken.Event,
                                 &Index );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to wait for transmit completion, Status: %r\r\n",
                Status ));

      //
      //  Try again
      //
      Status = EFI_SUCCESS;
      break;
    }

    //
    //  Get the transmit status
    //
    Status = Tcp4TxToken.CompletionToken.Status;
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_WARN,
                "WARNING - Failed the transmission, Status: %r\r\n",
                Status ));

      //
      //  Try again
      //
      Status = EFI_SUCCESS;

//
//  Exit now
//
Status = EFI_NOT_STARTED;
      break;
    }

    //
    //  Synchronize with the TimerCallback routine
    //
    TplPrevious = gBS->RaiseTPL ( TPL_DATASOURCE );

    //
    //  Account for the data sent
    //
    TotalBytesSent += Packet.DataLength;

    //
    //  Release the TimerCallback routine synchronization
    //
    gBS->RestoreTPL ( TplPrevious );
  } while ( !EFI_ERROR ( Status ));

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Open the network connection and send the data.

  @retval EFI_SUCCESS   Continue looping
  @retval other         Stopped by user's Control-C input

**/
EFI_STATUS
Tcp4Open (
  )
{
  UINTN Index;
  UINT8 * pIpAddress;
  EFI_STATUS Status;

  //
  //  Use do/while and break instead of goto
  //
  do {
    //
    //  Locate the TCP protocol
    //
    Status = Tcp4Locate ( );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Create the necessary events
    //
    Status = gBS->CreateEvent ( 0,
                                TPL_CALLBACK,
                                NULL,
                                NULL,
                                &Tcp4CloseToken.CompletionToken.Event );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to create the close event, Status: %r\r\n",
                Status ));
      Tcp4CloseToken.CompletionToken.Event = NULL;
      break;
    }
    DEBUG (( DEBUG_INFO,
              "0x%08x: Close event open\r\n",
              Tcp4CloseToken.CompletionToken.Event ));

    Status = gBS->CreateEvent ( 0,
                                TPL_CALLBACK,
                                NULL,
                                NULL,
                                &Tcp4ConnectToken.CompletionToken.Event );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to create the connect event, Status: %r\r\n",
                Status ));
      Tcp4ConnectToken.CompletionToken.Event = NULL;
      break;
    }
    DEBUG (( DEBUG_INFO,
              "0x%08x: Connect event open\r\n",
              Tcp4ConnectToken.CompletionToken.Event ));

    Status = gBS->CreateEvent ( 0,
                                TPL_CALLBACK,
                                NULL,
                                NULL,
                                &Tcp4ListenToken.CompletionToken.Event );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to create the listen event, Status: %r\r\n",
                Status ));
      Tcp4ListenToken.CompletionToken.Event = NULL;
      break;
    }
    DEBUG (( DEBUG_INFO,
              "0x%08x: Listen event open\r\n",
              Tcp4ListenToken.CompletionToken.Event ));

    Status = gBS->CreateEvent ( 0,
                                TPL_CALLBACK,
                                NULL,
                                NULL,
                                &Tcp4TxToken.CompletionToken.Event );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to create the TX event, Status: %r\r\n",
                Status ));
      Tcp4TxToken.CompletionToken.Event = NULL;
      break;
    }
    DEBUG (( DEBUG_INFO,
              "0x%08x: TX event open\r\n",
              Tcp4TxToken.CompletionToken.Event ));

    //
    //  Configure the local TCP port
    //
    Tcp4ConfigData.TimeToLive = 255;
    Tcp4ConfigData.TypeOfService = 0;
    Tcp4ConfigData.ControlOption = NULL;
    Tcp4ConfigData.AccessPoint.ActiveFlag = TRUE;
    Tcp4ConfigData.AccessPoint.StationAddress.Addr[0] = 0;
    Tcp4ConfigData.AccessPoint.StationAddress.Addr[1] = 0;
    Tcp4ConfigData.AccessPoint.StationAddress.Addr[2] = 0;
    Tcp4ConfigData.AccessPoint.StationAddress.Addr[3] = 0;
    Tcp4ConfigData.AccessPoint.StationPort = 0;
    Tcp4ConfigData.AccessPoint.RemoteAddress.Addr[0] = (UINT8)  RemoteHostAddress.sin_addr.s_addr;
    Tcp4ConfigData.AccessPoint.RemoteAddress.Addr[1] = (UINT8)( RemoteHostAddress.sin_addr.s_addr >> 8 );
    Tcp4ConfigData.AccessPoint.RemoteAddress.Addr[2] = (UINT8)( RemoteHostAddress.sin_addr.s_addr >> 16 );
    Tcp4ConfigData.AccessPoint.RemoteAddress.Addr[3] = (UINT8)( RemoteHostAddress.sin_addr.s_addr >> 24 );
    Tcp4ConfigData.AccessPoint.RemotePort = RemoteHostAddress.sin_port;
    Tcp4ConfigData.AccessPoint.UseDefaultAddress = TRUE;
    Tcp4ConfigData.AccessPoint.SubnetMask.Addr[0] = 0;
    Tcp4ConfigData.AccessPoint.SubnetMask.Addr[1] = 0;
    Tcp4ConfigData.AccessPoint.SubnetMask.Addr[2] = 0;
    Tcp4ConfigData.AccessPoint.SubnetMask.Addr[3] = 0;
    Status = pTcp4Protocol->Configure ( pTcp4Protocol,
                                        &Tcp4ConfigData );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to configure TCP port, Status: %r\r\n",
                Status ));
      break;
    }
    DEBUG (( DEBUG_INFO,
              "0x%08x: TCP4 port configured\r\n",
              pTcp4Protocol ));

    //
    //  Connect to the remote TCP port
    //
    Status = pTcp4Protocol->Connect ( pTcp4Protocol,
                                      &Tcp4ConnectToken );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to start the connection to the remote system, Status: %r\r\n",
                Status ));
      break;
    }
    Status = gBS->WaitForEvent ( 1,
                                 &Tcp4ConnectToken.CompletionToken.Event,
                                 &Index );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to wait for the connection, Status: %r\r\n",
                Status ));
      break;
    }
    Status = Tcp4ConnectToken.CompletionToken.Status;
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_WARN,
                "WARNING - Failed to connect to the remote system, Status: %r\r\n",
                Status ));
      break;
    }
    DEBUG (( DEBUG_INFO,
              "0x%08x: TCP4 port connected\r\n",
              pTcp4Protocol ));
    bTcp4Connected = TRUE;

    //
    //  Display the connection
    //
    pIpAddress = (UINT8 *)&RemoteHostAddress.sin_addr.s_addr;
    Print ( L"Connected to %d.%d.%d.%d:%d\r\n",
            pIpAddress[0],
            pIpAddress[1],
            pIpAddress[2],
            pIpAddress[3],
            htons ( RemoteHostAddress.sin_port ));
  } while ( 0 );

  if ( EFI_ERROR ( Status )) {
    //
    //  Try again
    //
    Status = EFI_SUCCESS;
  }
  else {
    //
    //  Semd data until the connection breaks
    //
    Status = Tcp4Send ( );
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Handle the timer callback

  @param [in] Event     Event that caused this callback
  @param [in] pContext  Context for this routine
**/
VOID
TimerCallback (
  IN EFI_EVENT Event,
  IN VOID * pContext
  )
{
  UINT64 BytesSent;
  UINT64 DeltaBytes;
  UINT32 Delta;
  UINT64 Average;

  //
  //  Notify the other code of the timer tick
  //
  bTick = TRUE;

  //
  //  Update the average bytes per second
  //
  BytesSent = TotalBytesSent;
  if ( 0 != BytesSent ) {
    DeltaBytes = AverageBytes >> AVERAGE_SHIFT_COUNT;
    AverageBytes -= DeltaBytes;
    DeltaBytes = BytesSent - PreviousBytes;
    PreviousBytes = BytesSent;
    AverageBytes += DeltaBytes;

    //
    //  Separate the samples
    //
    if (( 2 << AVERAGE_SHIFT_COUNT ) == Samples ) {
      Print ( L"---------- Stable average ----------\r\n" );
    }
    Samples += 1;

    //
    //  Display the data rate
    //
    Delta = (UINT32)( DeltaBytes >> DATA_RATE_UPDATE_SHIFT );
    Average = AverageBytes >> ( AVERAGE_SHIFT_COUNT + DATA_RATE_UPDATE_SHIFT );
    if ( Average < RANGE_SWITCH ) {
      Print ( L"%d Bytes/sec, Ave: %d Bytes/Sec\r\n",
              Delta,
              (UINT32) Average );
    }
    else {
      Average >>= 10;
      if ( Average < RANGE_SWITCH ) {
        Print ( L"%d Bytes/sec, Ave: %d KiBytes/Sec\r\n",
                Delta,
                (UINT32) Average );
      }
      else {
        Average >>= 10;
        if ( Average < RANGE_SWITCH ) {
          Print ( L"%d Bytes/sec, Ave: %d MiBytes/Sec\r\n",
                  Delta,
                  (UINT32) Average );
        }
        else {
          Average >>= 10;
          if ( Average < RANGE_SWITCH ) {
            Print ( L"%d Bytes/sec, Ave: %d GiBytes/Sec\r\n",
                    Delta,
                    (UINT32) Average );
          }
          else {
            Average >>= 10;
            if ( Average < RANGE_SWITCH ) {
              Print ( L"%d Bytes/sec, Ave: %d TiBytes/Sec\r\n",
                      Delta,
                      Average );
            }
            else {
              Average >>= 10;
              Print ( L"%d Bytes/sec, Ave: %d PiBytes/Sec\r\n",
                      Delta,
                      (UINT32) Average );
            }
          }
        }
      }
    }
  }
}


/**
  Create the timer

  @retval  EFI_SUCCESS  The timer was successfully created
  @retval  Other        Timer initialization failed
**/
EFI_STATUS
TimerCreate (
  )
{
  EFI_STATUS Status;

  //
  //  Create the timer
  //
  Status = gBS->CreateEvent ( EVT_TIMER | EVT_NOTIFY_SIGNAL,
                              TPL_DATASOURCE,
                              TimerCallback,
                              NULL,
                              &pTimer );
  if ( EFI_ERROR ( Status )) {
    DEBUG (( DEBUG_ERROR,
              "ERROR - Failed to allocate the timer event, Status: %r\r\n",
              Status ));
  }
  else {
    DEBUG (( DEBUG_INFO,
              "0x%08x: Timer created\r\n",
              pTimer ));
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Stop the timer

  @retval  EFI_SUCCESS  The timer was stopped successfully
  @retval  Other        The timer failed to stop
**/
EFI_STATUS
TimerStop (
  )
{
  EFI_STATUS Status;

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Determine if the timer is running
  //
  if ( bTimerRunning ) {
    //
    //  Stop the timer
    //
    Status = gBS->SetTimer ( pTimer,
                             TimerCancel,
                             0 );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to stop the timer, Status: %r\r\n",
                Status ));
    }
    else {
      //
      //  Timer timer is now stopped
      //
      bTimerRunning = FALSE;
      DEBUG (( DEBUG_INFO,
                "0x%08x: Timer stopped\r\n",
                pTimer ));
    }
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Start the timer

  @param [in] Milliseconds  The number of milliseconds between timer callbacks

  @retval  EFI_SUCCESS  The timer was successfully created
  @retval  Other        Timer initialization failed
**/
EFI_STATUS
TimerStart (
  UINTN Milliseconds
  )
{
  EFI_STATUS Status;
  UINT64 TimeDelay;

  //
  //  Stop the timer if necessary
  //
  Status = EFI_SUCCESS;
  if ( bTimerRunning ) {
    Status = TimerStop ( );
  }
  if ( !EFI_ERROR ( Status )) {
    //
    //  Compute the new delay
    //
    TimeDelay = Milliseconds;
    TimeDelay *= 1000 * 10;

    //
    //  Start the timer
    //
    Status = gBS->SetTimer ( pTimer,
                             TimerPeriodic,
                             TimeDelay );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to start the timer, Status: %r\r\n",
                Status ));
    }
    else {
      //
      //  The timer is now running
      //
      bTimerRunning = TRUE;
      DEBUG (( DEBUG_INFO,
        "0x%08x: Timer running\r\n",
        pTimer ));
    }
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Destroy the timer

  @retval  EFI_SUCCESS  The timer was destroyed successfully
  @retval  Other        Failed to destroy the timer
**/
EFI_STATUS
TimerDestroy (
  )
{
  EFI_STATUS Status;

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Determine if the timer is running
  //
  if ( bTimerRunning ) {
    //
    //  Stop the timer
    //
    Status = TimerStop ( );
  }
  if (( !EFI_ERROR ( Status )) && ( NULL != pTimer )) {
    //
    //  Done with this timer
    //
    Status = gBS->CloseEvent ( pTimer );
    if ( EFI_ERROR ( Status )) {
      DEBUG (( DEBUG_ERROR,
                "ERROR - Failed to free the timer event, Status: %r\r\n",
                Status ));
    }
    else {
      DEBUG (( DEBUG_INFO,
                "0x%08x: Timer Destroyed\r\n",
                pTimer ));
      pTimer = NULL;
    }
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Send data to the DataSink program to test a network's bandwidth.

  @param [in] Argc  The number of arguments
  @param [in] Argv  The argument value array

  @retval  0        The application exited normally.
  @retval  Other    An error occurred.
**/
int
main (
  IN int Argc,
  IN char **Argv
  )
{
  EFI_STATUS (* pClose) ();
  EFI_STATUS (* pOpen) ();
  EFI_STATUS Status;

  DEBUG (( DEBUG_INFO,
            "DataSource starting\r\n" ));

  //
  //  Validate the command line
  //
  if ( 2 != Argc ) {
    Print ( L"%s  <remote IP address>\r\n", Argv[0] );
    return -1;
  }

bTcp4 = TRUE;

  //
  //  Determine the support routines
  //
  if ( bTcp4 ) {
    pOpen = Tcp4Open;
    pClose = Tcp4Close;
    bTcp4Connecting = TRUE;
  }
  else {
    pOpen = SocketOpen;
    pClose = SocketClose;
  }

  //
  //  Use for/break instead of goto
  //
  for ( ; ; )
  {
    //
    //  No bytes sent so far
    //
    TotalBytesSent = 0;
    AverageBytes = 0;
    PreviousBytes = 0;
    Samples = 0;

    //
    //  Get the port number
    //
    ZeroMem ( &RemoteHostAddress, sizeof ( RemoteHostAddress ));
    RemoteHostAddress.sin_len = sizeof ( RemoteHostAddress );
    RemoteHostAddress.sin_family = AF_INET;
    RemoteHostAddress.sin_port = htons ( PcdGet16 ( DataSource_Port ));

Print ( L"Argc: %d\r\n", Argc);
Print ( L"Argv[0]: %a\r\n", Argv[0]);
Print ( L"Argv[1]: %a\r\n", Argv[1]);

    //
    //  Get the IP address
    //
    pRemoteHost = Argv [1];
    Status = IpAddress ( );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Create the timer
    //
    bTick = TRUE;
    Status = TimerCreate ( );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Loop forever abusing the specified system
    //
    do {
      //
      //  Start a timer to perform connection polling and display updates
      //
      Status = TimerStart ( 2 * 1000 );
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Open the network connection and send the data
      //
      Status = pOpen ( );
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Done with the network connection
      //
      Status = pClose ( );
    } while ( !EFI_ERROR ( Status ));

    //
    //  Close the network connection if necessary
    //
    pClose ( );

    //
    //  All done
    //
    break;
  }

  //
  //  Stop the timer if necessary
  //
  TimerStop ( );
  TimerDestroy ( );

  //
  //  Return the operation status
  //
  DEBUG (( DEBUG_INFO,
            "DataSource exiting, Status: %r\r\n",
            Status ));
  return Status;
}
