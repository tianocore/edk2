/**
  @file
  Display the firmware

  Copyright (c) 2011-2012, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <WebServer.h>


/**
  Respond with the firmware status

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
FirmwarePage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );
  
  //
  //  Send the system table page
  //
  for ( ; ; ) {
    //
    //  Send the page and table header
    //
    Status = TableHeader ( SocketFD, pPort, L"Firmware", NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the firmware vendor and revision
    //
    Status = RowUnicodeString ( SocketFD,
                                pPort,
                                "Vendor",
                                gST->FirmwareVendor );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    
    Status = RowRevision ( SocketFD,
                           pPort,
                           "Revision",
                           gST->FirmwareRevision );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the UEFI version
    //
    Status = RowRevision ( SocketFD,
                           pPort,
                           "UEFI",
                           gST->Hdr.Revision );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Build the table trailer
    //
    Status = TableTrailer ( SocketFD,
                            pPort,
                            pbDone );
    break;
  }
    
  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}
