/**
  @file
  Display the handles in the system

  Copyright (c) 2011-2012, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <WebServer.h>


/**
  Respond with the handles in the system

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HandlePage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  INTN Digit;
  INTN Entries;
  INTN Index;
  UINTN GuidCount;
  UINTN LengthInBytes;
  UINT8 * pDigit;
  EFI_HANDLE * pHandleArray;
  EFI_HANDLE * pHandle;
  EFI_HANDLE * pHandleEnd;
  EFI_GUID ** ppGuidArray;
  EFI_GUID ** ppGuid;
  EFI_GUID ** ppGuidEnd;
  INTN Shift;
  EFI_STATUS Status;
  UINTN Value;
  CONST UINTN cDigit [] = {
    3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15 };
  
  DBG_ENTER ( );
  
  //
  //  Send the handles page
  //
  for ( ; ; ) {
    //
    //  Send the page header
    //
    Status = HttpPageHeader ( SocketFD, pPort, L"Handle Database" );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Build the table header
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<h1>Handle Database</h1>\r\n"
                                  "<table border=\"1\">\r\n"
                                  "  <tr bgcolor=\"c0c0ff\"><th>Handle</th><th>Protocol Guids</th></tr>\r\n" );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Determine the number of handles in the database
    //
    LengthInBytes = 0;
    Status = gBS->LocateHandle ( AllHandles,
                                 NULL,
                                 NULL,
                                 &LengthInBytes,
                                 NULL );
    if ( EFI_BUFFER_TOO_SMALL == Status ) {
      //
      //  Allocate space for the handles
      //
      Status = gBS->AllocatePool ( EfiRuntimeServicesData,
                                   LengthInBytes,
                                   (VOID **) &pHandleArray );
      if ( !EFI_ERROR ( Status )) {
        //
        //  Get the list of handles
        //
        Status = gBS->LocateHandle ( AllHandles,
                                     NULL,
                                     NULL,
                                     &LengthInBytes,
                                     pHandleArray );
        if ( !EFI_ERROR ( Status )) {
          Entries = LengthInBytes / sizeof ( *pHandleArray );
          pHandle = pHandleArray;
          pHandleEnd = &pHandle [ Entries ];
          while ( pHandleEnd > pHandle ) {
            //
            //  Build the table entry for this page
            //
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          "<tr><td><code>0x" );
            if ( EFI_ERROR ( Status )) {
              break;
            }
            Value = (UINTN) *pHandle;
            for ( Shift = ( sizeof ( Shift ) << 3 ) - 4; 0 <= Shift; Shift -= 4 ) {
              //
              //  Convert the next address nibble to ANSI hex
              //
              Digit = (( Value >> Shift ) & 0xf ) | '0';
              if ( '9' < Digit ) {
                Digit += 'a' - '0' - 10;
              }

              //
              //  Display the address digit
              //
              Status = HttpSendByte ( SocketFD,
                                      pPort,
                                      (UINT8) Digit );
              if ( EFI_ERROR ( Status )) {
                break;
              }
            }
            if ( EFI_ERROR ( Status )) {
              break;
            }

            //
            //  Start the second column
            //
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          "</code></td><td><code>\r\n" );
            if ( EFI_ERROR ( Status )) {
              break;
            }

            //
            //  Determine the number of protocols connected to this handle
            //
            Status = gBS->ProtocolsPerHandle ( *pHandle,
                                               &ppGuidArray,
                                               &GuidCount );
            if ( EFI_ERROR ( Status )) {
              break;
            }
            ppGuid = ppGuidArray;
            ppGuidEnd = &ppGuid [ GuidCount ];
            while ( ppGuidEnd > ppGuid ) {
              //
              //  Display the guid
              //
              pDigit = (UINT8 *) *ppGuid;
              for ( Index = 0; 16 > Index; Index++ ) {
                //
                //  Separate the portions of the GUID
                //  99E87DCF-6162-40c5-9FA1-32111F5197F7
                //
                if (( 4 == Index )
                  || ( 6 == Index )
                  || ( 8 == Index )
                  || ( 10 == Index )) {
                  Status = HttpSendByte ( SocketFD,
                                          pPort,
                                          '-' );
                  if ( EFI_ERROR ( Status )) {
                    break;
                  }
                }

                //
                //  Display the GUID digits
                //
                Value = pDigit [ cDigit [ Index ]];
                for ( Shift = 4; 0 <= Shift; Shift -= 4 ) {
                  //
                  //  Convert the next address nibble to ANSI hex
                  //
                  Digit = (( Value >> Shift ) & 0xf ) | '0';
                  if ( '9' < Digit ) {
                    Digit += 'a' - '0' - 10;
                  }
                
                  //
                  //  Display the address digit
                  //
                  Status = HttpSendByte ( SocketFD,
                                          pPort,
                                          (UINT8) Digit );
                  if ( EFI_ERROR ( Status )) {
                    break;
                  }
                }
                if ( EFI_ERROR ( Status )) {
                  break;
                }
              }

              //
              //  Separate each GUID
              //
              Status = HttpSendAnsiString ( SocketFD,
                                            pPort,
                                            "<br/>\r\n" );
              if ( EFI_ERROR ( Status )) {
                break;
              }

              //
              //  Set the next protocol
              //
              ppGuid+= 1;
            }

            //
            //  Free the GUID array
            //
            gBS->FreePool ( ppGuidArray );
            if ( EFI_ERROR ( Status )) {
              break;
            }

            //
            //  End the row
            //
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          "</code></td></tr>\r\n" );
            if ( EFI_ERROR ( Status )) {
              break;
            }

            //
            //  Set the next handle
            //
            pHandle += 1;
          }
        }

        //
        //  Done with the handle array
        //
        gBS->FreePool ( pHandleArray );
      }
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
