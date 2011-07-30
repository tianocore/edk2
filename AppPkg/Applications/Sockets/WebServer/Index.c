/*++
  This file contains an 'Intel UEFI Application' and is        
  licensed for Intel CPUs and chipsets under the terms of your  
  license agreement with Intel or your vendor.  This file may   
  be modified by the user, subject to additional terms of the   
  license agreement                                             
--*/
/*++

Copyright (c)  2011 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

--*/

/** @file
  Generate the list of known pages.

**/

#include <WebServer.h>


/**
  Respond with the list of known pages

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
IndexPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  CONST DT_PAGE * pPage;
  CONST DT_PAGE * pPageEnd;
  EFI_STATUS Status;
  
  DBG_ENTER ( );
  
  //
  //  Send the index page
  //
  for ( ; ; ) {
    //
    //  Send the page header
    //
    Status = HttpPageHeader ( SocketFD, pPort, L"Index" );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Build the table header
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<h1>UEFI Web Server</h1>\r\n"
                                  "<table border=\"1\">\r\n"
                                  "  <tr bgcolor=\"c0c0ff\"><th>Page</th><th>Description</th></tr>\r\n" );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Walk the list of pages
    //  Skip the first page
    //
    pPage = &mPageList[0];
    pPageEnd = &pPage[mPageCount];
    pPage += 1;
    while ( pPageEnd > pPage ) {
      //
      //  Build the table entry for this page
      //
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "<tr><td><a target=\"_blank\" href=\"" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendUnicodeString ( SocketFD,
                                       pPort,
                                       &pPage->pPageName[1]);
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "\">" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendUnicodeString ( SocketFD,
                                       pPort,
                                       &pPage->pPageName[1]);
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "</a></td><td>" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendUnicodeString ( SocketFD,
                                       pPort,
                                       pPage->pDescription );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "</td></tr>\r\n" );
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Set the next page
      //
      pPage += 1;
    }
    if ( EFI_ERROR ( Status )) {
      break;
    }
  
    //
    //  Build the table trailer
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</table>\r\n" );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Send the page trailer
    //
    Status = HttpPageTrailer ( SocketFD, pPort, pbDone );
    break;
  }
    
  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}
