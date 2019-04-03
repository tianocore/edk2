/**
  @file
  Reboot the system

  Copyright (c) 2011-2012, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <WebServer.h>
#include <Library/UefiRuntimeServicesTableLib.h>


/**
  Page to reboot the system

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
RebootPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  EFI_STATUS Status;
  
  DBG_ENTER ( );
  
  //
  //  Send the Reboot page
  //
  for ( ; ; ) {
    //
    //  Send the page header
    //
    Status = HttpPageHeader ( SocketFD, pPort, L"Reboot" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
  
    //
    //  Send the page body
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<h1>Reboot</h1>\r\n"
                                  "<p>\r\n"
                                  "  Ouch!  The system is rebooting!\r\n" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
  
    //
    //  Send the page trailer
    //
    Status = HttpPageTrailer ( SocketFD, pPort, pbDone );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Deliver the data to the remote system by
    //  closing the socket
    //
    close ( SocketFD );

    //
    //  Attempt to reboot the system
    //
    DEBUG (( DEBUG_REQUEST, "Reseting System\r\n" ));
    gRT->ResetSystem ( EfiResetCold,
                       EFI_SUCCESS,
                       0,
                       NULL );
    break;
  }
    
  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}
