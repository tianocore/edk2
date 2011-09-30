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

#include <sys/EfiSysCall.h>
#include <sys/poll.h>
#include <sys/socket.h>


#define MAX_CONNECTIONS       ( 1 + 16 )  ///<  Maximum number of client connections
#define RANGE_SWITCH                2048  ///<  Switch display ranges
#define DATA_RATE_UPDATE_SHIFT      2     ///<  2n seconds between updates
#define AVERAGE_SHIFT_COUNT ( 6 - DATA_RATE_UPDATE_SHIFT )  ///<  2n samples in average

#define TPL_DATASINK        TPL_CALLBACK  ///<  Synchronization TPL

#define PACKET_SIZE                 1448  ///<  Size of data packets
#define DATA_BUFFER_SIZE    (( 65536 / PACKET_SIZE ) * PACKET_SIZE )  ///<  Buffer size in bytes

typedef struct _DT_PORT {
  UINT64 BytesAverage;
  UINT64 BytesPrevious;
  UINT64 BytesTotal;
  struct sockaddr_in RemoteAddress;
  UINT64 Samples;
} DT_PORT;

volatile BOOLEAN bTick;
BOOLEAN bTimerRunning;
struct sockaddr_in LocalAddress;
EFI_EVENT pTimer;
int ListenSocket;
UINT8 Buffer[ DATA_BUFFER_SIZE ];
struct pollfd PollFd[ MAX_CONNECTIONS ];
DT_PORT Port[ MAX_CONNECTIONS ];
nfds_t MaxPort;


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
  Accept a socket connection

  @retval  EFI_SUCCESS      The application is running normally
  @retval  EFI_NOT_STARTED  Error with the listen socket
  @retval  Other            The user stopped the application
**/
EFI_STATUS
SocketAccept (
  )
{
  INT32 SocketStatus;
  EFI_STATUS Status;
  INTN Index;

  //
  //  Assume failure
  //
  Status = EFI_DEVICE_ERROR;

  //
  //  Bind to the local address
  //
  SocketStatus = bind ( ListenSocket,
                        (struct sockaddr *) &LocalAddress,
                        LocalAddress.sin_len );
  if ( 0 == SocketStatus ) {
    //
    //  Start listening on the local socket
    //
    SocketStatus = listen ( ListenSocket, 5 );
    if ( 0 == SocketStatus ) {
      //
      //  Local socket in the listen state
      //
      Status = EFI_SUCCESS;

      //
      //  Allocate a port
      //
      Index = MaxPort++;
      PollFd[ Index ].fd = ListenSocket;
      PollFd[ Index ].events = POLLRDNORM | POLLHUP;
      PollFd[ Index ].revents = 0;
      Port[ Index ].BytesAverage = 0;
      Port[ Index ].BytesPrevious = 0;
      Port[ Index ].BytesTotal = 0;
      Port[ Index ].Samples = 0;
      Port[ Index ].RemoteAddress.sin_len = 0;
      Port[ Index ].RemoteAddress.sin_family = 0;
      Port[ Index ].RemoteAddress.sin_port = 0;
      Port[ Index ].RemoteAddress.sin_addr.s_addr= 0;
    }
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
  INT32 CloseStatus;
  EFI_STATUS Status;

  //
  //  Determine if the socket is open
  //
  Status = EFI_DEVICE_ERROR;
  if ( -1 != ListenSocket ) {
    //
    //  Attempt to close the socket
    //
    CloseStatus = close ( ListenSocket );
    if ( 0 == CloseStatus ) {
      DEBUG (( DEBUG_INFO,
                "0x%08x: Socket closed\r\n",
                ListenSocket ));
      ListenSocket = -1;
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
  //  Get the port number
  //
  ZeroMem ( &LocalAddress, sizeof ( LocalAddress ));
  LocalAddress.sin_len = sizeof ( LocalAddress );
  LocalAddress.sin_family = AF_INET;
  LocalAddress.sin_port = htons ( PcdGet16 ( DataSource_Port ));
  
  //
  //  Loop creating the socket
  //
  DEBUG (( DEBUG_INFO,
            "Creating the socket\r\n" ));

  //
  //  Check for user stop request
  //
  Status = ControlCCheck ( );
  if ( !EFI_ERROR ( Status )) {
    //
    //  Attempt to create the socket
    //
    ListenSocket = socket ( AF_INET,
                            SOCK_STREAM,
                            IPPROTO_TCP );
    if ( -1 != ListenSocket ) {
      DEBUG (( DEBUG_INFO,
                "0x%08x: Socket created\r\n",
                ListenSocket ));
    }
    else {
      Status = EFI_NOT_STARTED;
    }
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Poll the socket for more work

  @retval  EFI_SUCCESS      The application is running normally
  @retval  EFI_NOT_STARTED  Listen socket error
  @retval  Other            The user stopped the application
**/
EFI_STATUS
SocketPoll (
  )
{
  BOOLEAN bRemoveSocket;
  BOOLEAN bListenError;
  size_t BytesReceived;
  int CloseStatus;
  nfds_t Entry;
  INTN EntryPrevious;
  int FdCount;
  nfds_t Index;
  socklen_t LengthInBytes;
  struct sockaddr_in RemoteAddress;
  int Socket;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  //
  //  Check for control-C
  //
  bListenError = FALSE;
  Status = ControlCCheck ( );
  if ( !EFI_ERROR ( Status )) {
    //
    //  Poll the sockets
    //
    FdCount = poll ( &PollFd[0],
                     MaxPort,
                     0 );
    if ( -1 == FdCount ) {
      //
      //  Poll error
      //
      DEBUG (( DEBUG_ERROR,
                "ERROR - Poll error, errno: %d\r\n",
                errno ));
      Status = EFI_DEVICE_ERROR;
    }
    else {
      //
      //  Process the poll output
      //
      Index = 0;
      while ( FdCount ) {
        bRemoveSocket = FALSE;

        //
        //  Account for this descriptor
        //
        if ( 0 != PollFd[ Index ].revents ) {
          FdCount -= 1;
        }

        //
        //  Check for a broken connection
        //
        if ( 0 != ( PollFd[ Index ].revents & POLLHUP )) {
          bRemoveSocket = TRUE;
          if ( ListenSocket == PollFd[ Index ].fd ) {
            bListenError = TRUE;
            DEBUG (( DEBUG_ERROR,
                      "ERROR - Network closed on listen socket, errno: %d\r\n",
                      errno ));
          }
          else {
            DEBUG (( DEBUG_ERROR,
                      "ERROR - Network closed on socket %d.%d.%d.%d:%d, errno: %d\r\n",
                      Port[ Index ].RemoteAddress.sin_addr.s_addr & 0xff,
                      ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 8 ) & 0xff,
                      ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 16 ) & 0xff,
                      ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 24 ) & 0xff,
                      htons ( Port[ Index ].RemoteAddress.sin_port ),
                      errno ));

            //
            //  Close the socket
            //
            CloseStatus = close ( PollFd[ Index ].fd );
            if ( 0 == CloseStatus ) {
              bRemoveSocket = TRUE;
              DEBUG (( DEBUG_INFO,
                        "0x%08x: Socket closed for %d.%d.%d.%d:%d\r\n",
                        PollFd[ Index ].fd,
                        Port[ Index ].RemoteAddress.sin_addr.s_addr & 0xff,
                        ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 8 ) & 0xff,
                        ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 16 ) & 0xff,
                        ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 24 ) & 0xff,
                        htons ( Port[ Index ].RemoteAddress.sin_port )));
            }
            else {
              DEBUG (( DEBUG_ERROR,
                        "ERROR - Failed to close socket 0x%08x for %d.%d.%d.%d:%d, errno: %d\r\n",
                        PollFd[ Index ].fd,
                        Port[ Index ].RemoteAddress.sin_addr.s_addr & 0xff,
                        ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 8 ) & 0xff,
                        ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 16 ) & 0xff,
                        ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 24 ) & 0xff,
                        htons ( Port[ Index ].RemoteAddress.sin_port ),
                        errno ));
            }
          }
        }
        
        //
        //  Check for a connection or read data
        //
        if ( 0 != ( PollFd[ Index ].revents & POLLRDNORM )) {
          //
          //  Check for a connection
          //
          if ( ListenSocket == PollFd[ Index ].fd ) {
            //
            //  Another client connection was received
            //
            LengthInBytes = sizeof ( RemoteAddress );
            Socket = accept ( ListenSocket,
                              (struct sockaddr *) &RemoteAddress,
                              &LengthInBytes );
            if ( -1 == Socket ) {
              //
              //  Listen socket error
              //
              bListenError = TRUE;
              bRemoveSocket = TRUE;
              DEBUG (( DEBUG_ERROR,
                        "ERROR - Listen socket failure, errno: %d\r\n",
                        errno ));
            }
            else {
              //
              //  Determine if there is room for this connection
              //
              if (( MAX_CONNECTIONS <= MaxPort )
                || ((( MAX_CONNECTIONS - 1 ) == MaxPort ) && ( -1 == ListenSocket ))) {
                //
                //  Display the connection
                //
                Print ( L"Rejecting connection to remote system %d.%d.%d.%d:%d\r\n",
                        RemoteAddress.sin_addr.s_addr & 0xff,
                        ( RemoteAddress.sin_addr.s_addr >> 8 ) & 0xff,
                        ( RemoteAddress.sin_addr.s_addr >> 16 ) & 0xff,
                        ( RemoteAddress.sin_addr.s_addr >> 24 ) & 0xff,
                        htons ( RemoteAddress.sin_port ));

                //
                //  No room for this connection
                //  Close the connection
                //
                CloseStatus = close ( Socket );
                if ( 0 == CloseStatus ) {
                  bRemoveSocket = TRUE;
                  DEBUG (( DEBUG_INFO,
                            "0x%08x: Socket closed for %d.%d.%d.%d:%d\r\n",
                            PollFd[ Index ].fd,
                            RemoteAddress.sin_addr.s_addr & 0xff,
                            ( RemoteAddress.sin_addr.s_addr >> 8 ) & 0xff,
                            ( RemoteAddress.sin_addr.s_addr >> 16 ) & 0xff,
                            ( RemoteAddress.sin_addr.s_addr >> 24 ) & 0xff,
                            htons ( RemoteAddress.sin_port )));
                }
                else {
                  DEBUG (( DEBUG_ERROR,
                            "ERROR - Failed to close socket 0x%08x, errno: %d\r\n",
                            PollFd[ Index ].fd,
                            errno ));
                }

                //
                //  Keep the application running
                //  No issue with the listen socket
                //
                Status = EFI_SUCCESS;
              }
              else {
                //
                //  Display the connection
                //
                Print ( L"Connected to remote system %d.%d.%d.%d:%d\r\n",
                        RemoteAddress.sin_addr.s_addr & 0xff,
                        ( RemoteAddress.sin_addr.s_addr >> 8 ) & 0xff,
                        ( RemoteAddress.sin_addr.s_addr >> 16 ) & 0xff,
                        ( RemoteAddress.sin_addr.s_addr >> 24 ) & 0xff,
                        htons ( RemoteAddress.sin_port ));

                //
                //  Allocate the client connection
                //
                Index = MaxPort++;
                Port[ Index ].BytesAverage = 0;
                Port[ Index ].BytesPrevious = 0;
                Port[ Index ].BytesTotal = 0;
                Port[ Index ].Samples = 0;
                Port[ Index ].RemoteAddress.sin_len = RemoteAddress.sin_len;
                Port[ Index ].RemoteAddress.sin_family = RemoteAddress.sin_family;
                Port[ Index ].RemoteAddress.sin_port = RemoteAddress.sin_port;
                Port[ Index ].RemoteAddress.sin_addr = RemoteAddress.sin_addr;
                PollFd[ Index ].fd = Socket;
                PollFd[ Index ].events = POLLRDNORM | POLLHUP;
                PollFd[ Index ].revents = 0;
              }
            }
          }
          else {
            //
            //  Data received
            //
            BytesReceived = read ( PollFd[ Index ].fd,
                                   &Buffer,
                                   sizeof ( Buffer ));
            if ( 0 < BytesReceived ) {
              //
              //  Display the amount of data received
              //
              DEBUG (( DEBUG_INFO,
                        "0x%08x: Socket received 0x%08x bytes from %d.%d.%d.%d:%d\r\n",
                        PollFd[ Index ].fd,
                        BytesReceived,
                        Port[ Index ].RemoteAddress.sin_addr.s_addr & 0xff,
                        ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 8 ) & 0xff,
                        ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 16 ) & 0xff,
                        ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 24 ) & 0xff,
                        htons ( Port[ Index ].RemoteAddress.sin_port )));

              //
              //  Synchronize with the TimerCallback routine
              //
              TplPrevious = gBS->RaiseTPL ( TPL_DATASINK );

              //
              //  Account for the data received
              //
              Port[ Index ].BytesTotal += BytesReceived;

              //
              //  Release the synchronization with the TimerCallback routine
              //
              gBS->RestoreTPL ( TplPrevious );
            }
            else if ( -1 == BytesReceived ) {
              //
              //  Close the socket
              //
              DEBUG (( DEBUG_INFO,
                        "ERROR - Receive failure for %d.%d.%d.%d:%d, errno: %d\r\n",
                        Port[ Index ].RemoteAddress.sin_addr.s_addr & 0xff,
                        ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 8 ) & 0xff,
                        ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 16 ) & 0xff,
                        ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 24 ) & 0xff,
                        htons ( Port[ Index ].RemoteAddress.sin_port ),
                        errno ));
              CloseStatus = close ( PollFd[ Index ].fd );
              if ( 0 == CloseStatus ) {
                bRemoveSocket = TRUE;
                DEBUG (( DEBUG_INFO,
                          "0x%08x: Socket closed for %d.%d.%d.%d:%d\r\n",
                          PollFd[ Index ].fd,
                          Port[ Index ].RemoteAddress.sin_addr.s_addr & 0xff,
                          ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 8 ) & 0xff,
                          ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 16 ) & 0xff,
                          ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 24 ) & 0xff,
                          htons ( Port[ Index ].RemoteAddress.sin_port )));
              }
              else {
                DEBUG (( DEBUG_ERROR,
                          "ERROR - Failed to close socket 0x%08x for %d.%d.%d.%d:%d, errno: %d\r\n",
                          PollFd[ Index ].fd,
                          Port[ Index ].RemoteAddress.sin_addr.s_addr & 0xff,
                          ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 8 ) & 0xff,
                          ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 16 ) & 0xff,
                          ( Port[ Index ].RemoteAddress.sin_addr.s_addr >> 24 ) & 0xff,
                          htons ( Port[ Index ].RemoteAddress.sin_port ),
                          errno ));
              }
            }

            //
            //  Keep the application running
            //  No issue with the listen socket
            //
            Status = EFI_SUCCESS;
          }
        }

        //
        //  Remove the socket if necessary
        //
        if ( bRemoveSocket ) {
          DEBUG (( DEBUG_INFO,
                    "0x%08x: Socket removed from polling\r\n",
                    PollFd[ Index ].fd ));
          MaxPort -= 1;
          for ( Entry = Index + 1; MaxPort >= Entry; Entry++ ) {
            EntryPrevious = Entry;
            Port[ EntryPrevious ].BytesAverage = Port[ Entry ].BytesAverage;
            Port[ EntryPrevious ].BytesPrevious = Port[ Entry ].BytesPrevious;
            Port[ EntryPrevious ].BytesTotal = Port[ Entry ].BytesTotal;
            Port[ EntryPrevious ].RemoteAddress.sin_len = Port[ Entry ].RemoteAddress.sin_len;
            Port[ EntryPrevious ].RemoteAddress.sin_family = Port[ Entry ].RemoteAddress.sin_family;
            Port[ EntryPrevious ].RemoteAddress.sin_port = Port[ Entry ].RemoteAddress.sin_port;
            Port[ EntryPrevious ].RemoteAddress.sin_addr.s_addr = Port[ Entry ].RemoteAddress.sin_addr.s_addr;
            Port[ EntryPrevious ].Samples = Port[ Entry ].Samples;
            PollFd[ EntryPrevious ].events = PollFd[ Entry ].events;
            PollFd[ EntryPrevious ].fd = PollFd[ Entry ].fd;
            PollFd[ EntryPrevious ].revents = PollFd[ Entry ].revents;
          }
          PollFd[ MaxPort ].fd = -1;
          Index -= 1;
        }

        //
        //  Account for this socket
        //
        Index += 1;
      }
    }
  }

  //
  //  Return the listen failure if necessary
  //
  if (( !EFI_ERROR ( Status )) && bListenError ) {
    Status = EFI_NOT_STARTED;
  }

  //
  //  Return the poll status
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
  UINT64 Average;
  UINT64 BytesReceived;
  UINT32 Delta;
  UINT64 DeltaBytes;
  nfds_t Index;

  //
  //  Notify the other code of the timer tick
  //
  bTick = TRUE;

  //
  //  Walk the list of ports
  //
  for ( Index = 0; MaxPort > Index; Index++ ) {
    //
    //  Determine if any data was received
    //
    BytesReceived = Port[ Index ].BytesTotal;
    if (( ListenSocket != PollFd[ Index ].fd )
      && ( 0 != BytesReceived )) {
      //
      //  Update the average bytes per second
      //
      DeltaBytes = Port[ Index ].BytesAverage >> AVERAGE_SHIFT_COUNT;
      Port[ Index ].BytesAverage -= DeltaBytes;
      DeltaBytes = BytesReceived - Port[ Index ].BytesPrevious;
      Port[ Index ].BytesPrevious = BytesReceived;
      Port[ Index ].BytesAverage += DeltaBytes;

      //
      //  Separate the samples
      //
      if (( 2 << AVERAGE_SHIFT_COUNT ) == Port[ Index ].Samples ) {
        Print ( L"---------- Stable average ----------\r\n" );
      }
      Port[ Index ].Samples += 1;

      //
      //  Display the data rate
      //
      Delta = (UINT32)( DeltaBytes >> DATA_RATE_UPDATE_SHIFT );
      Average = Port[ Index ].BytesAverage >> ( AVERAGE_SHIFT_COUNT + DATA_RATE_UPDATE_SHIFT );
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
                              TPL_DATASINK,
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
  Receive data from the DataSource program to test a network's bandwidth.

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
  EFI_STATUS Status;

  DEBUG (( DEBUG_INFO,
            "DataSink starting\r\n" ));

  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
    //
    //  Create the timer
    //
    bTick = TRUE;
    Status = TimerCreate ( );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Start a timer to perform network polling and display updates
    //
    Status = TimerStart ( 1 * 1000 );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Loop forever waiting for abuse
    //
    do {
      ListenSocket = -1;
      do {
        //
        //  Complete any client operations
        //
        Status = SocketPoll ( );
        if ( EFI_ERROR ( Status )) {
          //
          //  Control-C
          //
          break;
        }
      
        //
        //  Wait for a while
        //
      } while ( !bTick );
      if ( EFI_ERROR ( Status )) {
        //
        //  Control-C
        //
        break;
      }
      
      //
      //  Wait for the network layer to initialize
      //
      Status = SocketNew ( );
      if ( EFI_ERROR ( Status )) {
        continue;
      }

      //
      //  Wait for the remote network application to start
      //
      Status = SocketAccept ( );
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
      do {
        Status = SocketPoll ( );
      } while ( !EFI_ERROR ( Status ));

      //
      //  Done with the socket
      //
      Status = SocketClose ( );
    } while ( !EFI_ERROR ( Status ));

    //
    //  Close the socket if necessary
    //
    SocketClose ( );

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
            "DataSink exiting, Status: %r\r\n",
            Status ));
  return Status;
}
