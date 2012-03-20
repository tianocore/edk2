/**
  @file
  HTTP processing for the web server.

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
  Get a UTF-8 character from the buffer

  @param [in] pData     The address of the buffer containing the character
  @param [out] ppData   The address to receive the next character address

  @return     The character value

**/
INTN
HttpCharGet (
  IN UINT8 * pData,
  IN UINT8 ** ppData
  )
{
  INTN Data;
  INTN Character;
  INTN Control;
  INTN Mask;

  //
  //  Verify that there is some data left
  //
  if ( NULL == pData ) {
    //
    //  No data to return
    //
    pData = NULL;
    Character = 0;
  }
  else {
    //
    //  Get the first portion of the character
    //
    Character = *pData++;
    Control = Character;
    Mask = 0xc0;

    //
    //  Append the rest of the character
    //
    if ( 0 != ( Control & 0x80 )) {
      while ( 0 != ( Control & 0x40 )) {
        Character &= Mask;
        Mask <<= 5;
        Control <<= 1;
        Character <<= 6;
        Data = *pData++ & 0x3f;
        if ( 0x80 != ( Data & 0xc0 )) {
          //
          //  Invalid character
          //
          pData = NULL;
          Character = 0;
          break;
        }
        Character |= Data & 0x3f;
      }
    }
  }

  //
  //  Return the next character location and the character
  //
  *ppData = pData;
  return Character;
}


/**
  Transmit a portion of the HTTP response

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpFlush (
  IN int SocketFD,
  IN WSDT_PORT * pPort
  )
{
  INTN LengthInBytes;
  UINT8 * pBuffer;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;
  pBuffer = &pPort->TxBuffer[0];
  do {
    //
    //  Attempt to send the data
    //
    LengthInBytes = send ( SocketFD,
                           pBuffer,
                           pPort->TxBytes,
                           0 );
    if ( -1 != LengthInBytes ) {
      //
      //  Account for the data sent
      //
      pBuffer += LengthInBytes;
      pPort->TxBytes -= LengthInBytes;
    }
    else {
      //
      //  Transmit error
      //
      Status = EFI_DEVICE_ERROR;
      break;
    }
  } while ( 0 < pPort->TxBytes );

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Convert the ANSI character to lower case

  @param [in] Character The character to convert to lower case.

  @return   The lower case character

**/
INTN
HttpLowerCase (
  IN INTN Character
  )
{
  //
  //  Determine if the character is upper case
  //
  if (( 'A' <= Character ) && ( 'Z' >= Character )) {
    Character += 'a' - 'A';
  }

  //
  //  Return the lower case value of the character
  //
  return Character;
}


/**
  Match a Unicode string against a UTF-8 string

  @param [in] pString     A zero terminated Unicode string
  @param [in] pData       A zero terminated UTF-8 string
  @param [in] bIgnoreCase TRUE if case is to be ignored

  @return     The difference between the last two characters tested.
              Returns -1 for error.

**/
INTN
HttpMatch (
  IN UINT16 * pString,
  IN UINT8 * pData,
  IN BOOLEAN bIgnoreCase
  )
{
  INTN Character1;
  INTN Character2;
  INTN Difference;

  do {
    //
    //  Get the character from the comparison string
    //
    Character1 = *pString++;

    //
    //  Convert the character to lower case
    //
    if ( bIgnoreCase ) {
      Character1 = HttpLowerCase ( Character1 );
    }

    //
    //  Get the character from the request
    //
    Character2 = HttpCharGet ( pData, &pData );
    if ( NULL == pData ) {
       //
       // Error getting character
       //
       Difference = -1;
       break;
    }

    //
    //  Convert the character to lower case
    //
    if ( bIgnoreCase ) {
      Character2 = HttpLowerCase ( Character2 );
    }

    //
    //  Compare the characters
    //
    Difference = Character1 - Character2;
    if ( 0 != Difference ) {
      return Difference;
    }
  } while ( 0 != Character1 );

  //
  //  Return the difference
  //
  return Difference;
}


/**
  Buffer the HTTP page header

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pTitle        A zero terminated Unicode title string

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpPageHeader (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST CHAR16 * pTitle
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Build the page header
  //
  for ( ; ; ) {
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<!DOCTYPE "
                                  "HTML "
                                  "PUBLIC "
                                  "\"-//W3C//DTD HTML 4.01 Transitional//EN\" "
                                  "\"http://www.w3.org/TR/html4/loose.dtd\">\r\n" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD, pPort, "<html lang=\"en-US\">\r\n" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    if ( NULL != pTitle ) {
      Status = HttpSendAnsiString ( SocketFD, pPort, "  <head>\r\n" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendAnsiString ( SocketFD, pPort, "    <title>" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendUnicodeString ( SocketFD, pPort, pTitle );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendAnsiString ( SocketFD, pPort, "</title>\r\n" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendAnsiString ( SocketFD, pPort, "  </head>\r\n" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }
    Status = HttpSendAnsiString ( SocketFD, pPort, "  <body>\r\n" );
    break;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Respond with an error indicating that the page was not found

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpPageNotFound (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN BOOLEAN * pbDone
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Send the page not found
  //
  for ( ; ; ) {
    //
    //  Send the page header
    //
    Status = HttpPageHeader ( SocketFD, pPort, L"404 Not found" );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Send the page body
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "ERROR <b>404</b><br />"
                                  "Requested page is not available\r\n" );
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


/**
  Buffer and send the HTTP page trailer

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpPageTrailer (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN BOOLEAN * pbDone
  )
{
  int RetVal;
  EFI_STATUS Status;
  socklen_t LengthInBytes;
  struct sockaddr_in6 LocalAddress;
  struct sockaddr_in6 RemoteAddress;

  DBG_ENTER ( );

  //
  //  Build the page header
  //
  for ( ; ; ) {
    LengthInBytes = sizeof ( LocalAddress );
    RetVal = getsockname ( SocketFD, (struct sockaddr *)&LocalAddress, &LengthInBytes );
    if ( 0 == RetVal ) {
      LengthInBytes = sizeof ( LocalAddress );
      RetVal = getpeername ( SocketFD, (struct sockaddr *)&RemoteAddress, &LengthInBytes );
      if ( 0 == RetVal ) {
        //
        //  Seperate the body from the trailer
        //
        Status = HttpSendAnsiString ( SocketFD, pPort, "  <hr>\r\n<code>" );
        if ( EFI_ERROR ( Status )) {
          break;
        }

        //
        //  Display the system addresses and the page transfer direction
        //
        Status = HttpSendIpAddress ( SocketFD, pPort, &LocalAddress );
        if ( EFI_ERROR ( Status )) {
          break;
        }
        Status = HttpSendAnsiString ( SocketFD, pPort, "  -->  " );
        if ( EFI_ERROR ( Status )) {
          break;
        }
        Status = HttpSendIpAddress ( SocketFD, pPort, &RemoteAddress );
        if ( EFI_ERROR ( Status )) {
          break;
        }
        Status = HttpSendAnsiString ( SocketFD, pPort, "</code>\r\n" );
        if ( EFI_ERROR ( Status )) {
          break;
        }
      }
    }

    //
    //  Terminate the page
    //
    Status = HttpSendAnsiString ( SocketFD, pPort, "  </body>\r\n" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD, pPort, "  </html>\r\n" );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Send the page trailer
    //
    Status = HttpFlush ( SocketFD, pPort );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Mark the page as complete
    //
    *pbDone = TRUE;
    break;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Replace a space with a zero

  @param [in] pData     The request buffer address
  @param [in] pEnd      End of buffer address

  @return     The next character location

**/
UINT8 *
HttpReplaceSpace (
  IN UINT8 * pData,
  IN UINT8 * pEnd
  )
{
  INTN Character;
  UINT8 * pSpace;

  pSpace = pData;
  while ( pEnd > pData ) {
    //
    //  Get the character from the request
    //
    Character = HttpCharGet ( pData, &pData );
    if ( ' ' == Character ) {
      break;
    }
    pSpace = pData;
  }

  //
  //  Replace the space character with zero
  //
  ZeroMem ( pSpace, pData - pSpace );

  //
  //  Return the next character location
  //
  return pData;
}


/**
  Process an HTTP request

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpRequest (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  UINT8 * pData;
  UINT8 * pEnd;
  CONST DT_PAGE * pPage;
  CONST DT_PAGE * pPageEnd;
  UINT8 * pVerb;
  UINT8 * pVersion;
  UINT8 * pWebPage;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Assume the request is not finished
  //
  *pbDone = FALSE;
  Status = EFI_SUCCESS;
  for ( ; ; ) {

    //
    //  Attempt to parse the command
    //
    pData = &pPort->Request[0];
    pEnd = &pData[ pPort->RequestLength ];
    pVerb = pData;
    pWebPage = HttpReplaceSpace ( pVerb, pEnd );
    if ( pEnd <= pWebPage ) {
      break;
    }
    pVersion = HttpReplaceSpace ( pWebPage, pEnd );
    if ( pEnd <= pVersion ) {
      break;
    }

    //
    //  Validate the request
    //
    if ( 0 != HttpMatch ( L"GET", pVerb, TRUE )) {
      //
      //  Invalid request type
      //
      DEBUG (( DEBUG_REQUEST,
                "HTTP: Invalid verb\r\n" ));
      Status = EFI_NOT_FOUND;
      break;
    }

    //
    //  Walk the page table
    //
    pPage = &mPageList[0];
    pPageEnd = &pPage[ mPageCount ];
    while ( pPageEnd > pPage ) {
      //
      //  Determine if the page was located
      //
      if ( 0 == HttpMatch ( pPage->pPageName, pWebPage, FALSE )) {
        break;
      }

      //
      //  Set the next page
      //
      pPage += 1;
    }
    if ( pPageEnd <= pPage ) {
      //
      //  The page was not found
      //
      DEBUG (( DEBUG_REQUEST,
                "HTTP: Page not found in page table\r\n" ));
      Status = EFI_NOT_FOUND;
      break;
    }

    //
    //  Respond with the page contents
    //
    Status = pPage->pfnResponse ( SocketFD, pPort, pbDone );
    break;
  }

  //
  //  Return page not found if necessary
  //
  if ( EFI_NOT_FOUND == Status ) {
    Status = HttpPageNotFound ( SocketFD, pPort, pbDone );
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Buffer data for sending

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] LengthInBytes Length of valid data in the buffer
  @param [in] pBuffer       Buffer of data to send

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpSend (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN size_t LengthInBytes,
  IN CONST UINT8 * pBuffer
  )
{
  size_t DataBytes;
  size_t MaxBytes;
  EFI_STATUS Status;

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;
  do {
    //
    //  Determine how much data fits into the buffer
    //
    MaxBytes = sizeof ( pPort->TxBuffer );
    DataBytes = MaxBytes - pPort->TxBytes;
    if ( DataBytes > LengthInBytes ) {
      DataBytes = LengthInBytes;
    }

    //
    //  Copy the data into the buffer
    //
    CopyMem ( &pPort->TxBuffer[ pPort->TxBytes ],
              pBuffer,
              DataBytes );

    //
    //  Account for the data copied
    //
    pPort->TxBytes += DataBytes;
    LengthInBytes -= DataBytes;

    //
    //  Transmit the buffer if it is full
    //
    if ( MaxBytes <= pPort->TxBytes ) {
      Status = HttpFlush ( SocketFD, pPort );
    }
  } while (( EFI_SUCCESS == Status ) && ( 0 < LengthInBytes ));

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Send an ANSI string

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pString       A zero terminated Unicode string

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpSendAnsiString (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST char * pString
  )
{
  CONST char * pData;
  EFI_STATUS Status;

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Walk the characters in he string
  //
  pData = pString;
  while ( 0 != *pData ) {
    pData += 1;
  }

  //
  //  Send the string
  //
  Status = HttpSend ( SocketFD,
                      pPort,
                      pData - pString,
                      (CONST UINT8 *)pString );

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Buffer a single byte

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] Data          The data byte to send

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpSendByte (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN UINT8 Data
  )
{
  EFI_STATUS Status;

  //
  //  Send the data byte
  //
  Status = HttpSend ( SocketFD,
                      pPort,
                      1,
                      &Data );

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Display a character

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] Character     Character to display
  @param [in] pReplacement  Replacement character string

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpSendCharacter (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CHAR8 Character,
  IN CHAR8 * pReplacement
  )
{
  EFI_STATUS Status;

  //
  //  Determine if this is a printable character
  //
  if (( 0x20 <= Character ) && ( 0x7f > Character )) {
    if ( '<' == Character ) {
      //
      //  Replace with HTML equivalent
      //
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "&lt;" );
    }
    else if ( '>' == Character ) {
      //
      //  Replace with HTML equivalent
      //
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "&gt;" );
    }
    else if ( '&' == Character ) {
      //
      //  Replace with HTML equivalent
      //
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "&amp;" );
    }
    else if ( '\"' == Character ) {
      //
      //  Replace with HTML equivalent
      //
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "&quot;" );
    }
    else {
      //
      //  Display the character
      //
      Status = HttpSendByte ( SocketFD,
                              pPort,
                              Character );
    }
  }
  else {
    //
    //  Not a displayable character
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  pReplacement );
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Send a buffer dump

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] ByteCount     The number of bytes to display
  @param [in] pData         Address of the byte array

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpSendDump (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN UINTN ByteCount,
  IN CONST UINT8 * pData
  )
{
  INTN BytesToDisplay;
  UINT8 Character;
  INTN Index;
  INTN InitialSpaces;
  CONST UINT8 * pDataEnd;
  CONST UINT8 * pEnd;
  CONST UINT8 * pTemp;
  EFI_STATUS Status;

  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
    //
    //  Start the field value
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<code>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Walk the bytes to be displayed
    //
    pEnd = &pData[ ByteCount ];
    while ( pEnd > pData ) {
      //
      //  Display the address
      //
      Status = HttpSendHexBits ( SocketFD,
                                 pPort,
                                 sizeof ( pData ) * 8,
                                 (UINT64)(UINTN)pData );
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Separate the address and data
      //
      Status = HttpSendByte ( SocketFD, pPort, ':' );
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Position the starting data correctly
      //
      InitialSpaces = (UINTN)pData;
      InitialSpaces &= BYTES_ON_A_LINE - 1;
      for ( Index = SPACES_ADDRESS_TO_DATA
                  + (( 2 + SPACES_BETWEEN_BYTES )
                        * InitialSpaces );
            0 < Index; Index-- ) {
        Status = HttpSendAnsiString ( SocketFD,
                                      pPort,
                                      "&nbsp;" );
        if ( EFI_ERROR ( Status )) {
          break;
        }
      }
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Display the data
      //
      BytesToDisplay = pEnd - pData;
      if (( BYTES_ON_A_LINE - InitialSpaces ) < BytesToDisplay ) {
        BytesToDisplay = BYTES_ON_A_LINE - InitialSpaces;
      }
      pDataEnd = &pData[ BytesToDisplay ];
      pTemp = pData;
      while ( pDataEnd > pTemp ) {
        Status = HttpSendHexBits ( SocketFD,
                                   pPort,
                                   8,
                                   *pTemp++ );
        if ( EFI_ERROR ( Status )) {
          break;
        }

        //
        //  Separate the data bytes
        //
        for ( Index = SPACES_BETWEEN_BYTES; 0 < Index; Index-- ) {
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "&nbsp;" );
          if ( EFI_ERROR ( Status )) {
            break;
          }
        }
        if ( EFI_ERROR ( Status )) {
          break;
        }
      }
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Separate the data from the ASCII display
      //
      for ( Index = (( 2 + SPACES_BETWEEN_BYTES )
                       * ( BYTES_ON_A_LINE - BytesToDisplay - InitialSpaces ))
                  - SPACES_BETWEEN_BYTES
                  + SPACES_DATA_TO_ASCII
                  + InitialSpaces;
            0 < Index; Index-- ) {
        Status = HttpSendAnsiString ( SocketFD,
                                      pPort,
                                      "&nbsp;" );
        if ( EFI_ERROR ( Status )) {
          break;
        }
      }
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Display the ASCII data
      //
      while ( pDataEnd > pData ) {
        Character = *pData++;
        Status = HttpSendCharacter ( SocketFD,
                                     pPort,
                                     Character,
                                     "." );
        if ( EFI_ERROR ( Status )) {
          break;
        }
      }
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Terminate the line
      //
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "<br/>\r\n" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }

    //
    //  Terminate the field value and row
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</code>\r\n" );
    break;
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Display a row containing a GUID value

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pGuid         Address of the GUID to display

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpSendGuid (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST EFI_GUID * pGuid
  )
{
  UINT32 Index;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
    //
    //  Display the GUID in a form found in the code
    //
    //  E.g. 0xca16005f, 0x11ec, 0x4bdc, { 0x99, 0x97, 0x27, 0x2c, 0xa9, 0xba, 0x15, 0xe5 }
    //

    //
    //  Display the first 32 bits
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "0x" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendHexBits ( SocketFD,
                               pPort,
                               32,
                               pGuid->Data1 );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the second 16 bits
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  ", 0x" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendHexBits ( SocketFD,
                               pPort,
                               16,
                               pGuid->Data2 );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the thrid 16 bits
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  ", 0x" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendHexBits ( SocketFD,
                               pPort,
                               16,
                               pGuid->Data3 );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Place the last 64 bits in braces
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  ", { 0x" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    for ( Index = 0; 7 >= Index; Index++ ) {
      //
      //  Display the next 8 bits
      //
      Status = HttpSendHexBits ( SocketFD,
                                 pPort,
                                 8,
                                 pGuid->Data4[ Index ]);
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Separate the bytes
      //
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    ( 7 != Index ) ? ", 0x" : " }" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }
    break;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Output a hex value to the HTML page

  @param [in] SocketFD    Socket file descriptor
  @param [in] pPort       The WSDT_PORT structure address
  @param [in] Bits        Number of bits to display
  @param [in] Value       Value to display

  @retval EFI_SUCCESS Successfully displayed the address
**/
EFI_STATUS
HttpSendHexBits (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN INT32 Bits,
  IN UINT64 Value
  )
{
  UINT32 Digit;
  INT32 Shift;
  EFI_STATUS Status;

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Walk the list of divisors
  //
  Shift = (( Bits + 3 ) & ( ~3 )) - 4;
  while ( 0 <= Shift ) {
    //
    //  Determine the next digit
    //
    Digit = (UINT32)(( Value >> Shift ) & 0xf );
    if ( 10 <= Digit ) {
      Digit += 'a' - '0' - 10;
    }

    //
    //  Display the digit
    //
    Status = HttpSendByte ( SocketFD, pPort, (UINT8)( '0' + Digit ));
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Set the next shift
    //
    Shift -= 4;
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Output a hex value to the HTML page

  @param [in] SocketFD    Socket file descriptor
  @param [in] pPort       The WSDT_PORT structure address
  @param [in] Value       Value to display

  @retval EFI_SUCCESS Successfully displayed the address
**/
EFI_STATUS
HttpSendHexValue (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN UINT64 Value
  )
{
  BOOLEAN bDisplayZeros;
  UINT32 Digit;
  INT32 Shift;
  EFI_STATUS Status;

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Walk the list of divisors
  //
  bDisplayZeros = FALSE;
  Shift = 60;
  do {
    //
    //  Determine the next digit
    //
    Digit = (UINT32)(( Value >> Shift ) & 0xf );
    if ( 10 <= Digit ) {
      Digit += 'a' - '0' - 10;
    }

    //
    //  Suppress leading zeros
    //
    if (( 0 != Digit ) || bDisplayZeros || ( 0 == Shift )) {
      bDisplayZeros = TRUE;

      //
      //  Display the digit
      //
      Status = HttpSendByte ( SocketFD, pPort, (UINT8)( '0' + Digit ));
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }

    //
    //  Set the next shift
    //
    Shift -= 4;
  } while ( 0 <= Shift );

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Output an IP6 address value to the HTML page

  @param [in] SocketFD          Socket file descriptor
  @param [in] pPort             The WSDT_PORT structure address
  @param [in] Value             Value to display
  @param [in] bFirstValue       TRUE if first value
  @param [in] bLastValue        TRUE if last value
  @param [in] bZeroSuppression  TRUE while zeros are being suppressed
  @param [in] pbZeroSuppression Address to receive TRUE when zero suppression
                                has started, use NULL if next colon value not
                                needed.

  @retval EFI_SUCCESS Successfully displayed the address
**/
EFI_STATUS
HttpSendIp6Value (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN UINT16 Value,
  IN BOOLEAN bFirstValue,
  IN BOOLEAN bLastValue,
  IN BOOLEAN bZeroSuppression,
  IN BOOLEAN * pbZeroSuppression
  )
{
  BOOLEAN bZeroSuppressionStarting;
  UINT32 Digit;
  EFI_STATUS Status;

  //
  //  Use break instead of goto
  //
  bZeroSuppressionStarting = FALSE;
  Status = EFI_SUCCESS;
  for ( ; ; ) {
    //
    //  Display the leading colon if necessary
    //
    if ( bZeroSuppression && ( bLastValue || ( 0 != Value ))) {
      Status = HttpSendByte ( SocketFD, pPort, ':' );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }

    //
    //  Skip over a series of zero values
    //
    bZeroSuppressionStarting = (BOOLEAN)( 0 == Value );
    if ( !bZeroSuppressionStarting ) {
      //
      //  Display the value
      //
      Digit = ( Value >> 4 ) & 0xf;
      Status = HttpSendHexValue ( SocketFD,
                                  pPort,
                                  Digit );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Digit = Value & 0xf;
      Status = HttpSendHexValue ( SocketFD,
                                  pPort,
                                  Digit );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Digit = ( Value >> 12 ) & 0xf;
      Status = HttpSendHexValue ( SocketFD,
                                  pPort,
                                  Digit );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Digit = ( Value >> 8 ) & 0xf;
      Status = HttpSendHexValue ( SocketFD,
                                  pPort,
                                  Digit );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }

    //
    //  Display the trailing colon if necessary
    //
    if (( !bLastValue ) && ( bFirstValue || ( 0 != Value ))) {
      Status = HttpSendByte ( SocketFD, pPort, ':' );
    }
    break;
  }

  //
  //  Return the next colon display
  if ( NULL != pbZeroSuppression ) {
    *pbZeroSuppression = bZeroSuppressionStarting;
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Output an IP address to the HTML page

  @param [in] SocketFD    Socket file descriptor
  @param [in] pPort       The WSDT_PORT structure address
  @param [in] pAddress    Address of the socket address

  @retval EFI_SUCCESS Successfully displayed the address
**/
EFI_STATUS
HttpSendIpAddress (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN struct sockaddr_in6 * pAddress
  )
{
  BOOLEAN bZeroSuppression;
  UINT32 Index;
  struct sockaddr_in * pIpv4;
  struct sockaddr_in6 * pIpv6;
  UINT16 PortNumber;
  EFI_STATUS Status;

  //
  //  Use break instead of goto
  //
  for ( ; ; ) {
    //
    //  Determine the type of address
    //
    if ( AF_INET6 == pAddress->sin6_family ) {
      pIpv6 = pAddress;

      //
      //  Display the address in RFC2732 format
      //
      bZeroSuppression = FALSE;
      Status = HttpSendByte ( SocketFD, pPort, '[' );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      for ( Index = 0; 8 > Index; Index++ ) {
        Status = HttpSendIp6Value ( SocketFD,
                                    pPort,
                                    pIpv6->sin6_addr.__u6_addr.__u6_addr16[ Index ],
                                    (BOOLEAN)( 0 == Index ),
                                    (BOOLEAN)( 7 == Index ),
                                    bZeroSuppression,
                                    &bZeroSuppression );
        if ( EFI_ERROR ( Status )) {
          break;
        }
      }
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Separate the port number
      //
      Status = HttpSendByte ( SocketFD, pPort, ']' );

      //
      //  Get the port number
      //
      PortNumber = pIpv6->sin6_port;
    }
    else {
      //
      //  Output the IPv4 address
      //
      pIpv4 = (struct sockaddr_in *)pAddress;
      Status = HttpSendValue ( SocketFD, pPort, (UINT8)pIpv4->sin_addr.s_addr );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendByte ( SocketFD, pPort, '.' );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendValue ( SocketFD, pPort, (UINT8)( pIpv4->sin_addr.s_addr >> 8 ));
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendByte ( SocketFD, pPort, '.' );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendValue ( SocketFD, pPort, (UINT8)( pIpv4->sin_addr.s_addr >> 16 ));
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendByte ( SocketFD, pPort, '.' );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendValue ( SocketFD, pPort, (UINT8)( pIpv4->sin_addr.s_addr >> 24 ));

      //
      //  Get the port number
      //
      PortNumber = pIpv4->sin_port;
    }
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Display the port number
    //
    Status = HttpSendByte ( SocketFD, pPort, ':' );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendValue ( SocketFD, pPort, htons ( PortNumber ));
    break;
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Send a Unicode string

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pString       A zero terminated Unicode string

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
HttpSendUnicodeString (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST UINT16 * pString
  )
{
  UINT8 Data;
  UINT16 Character;
  EFI_STATUS Status;

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Walk the characters in he string
  //
  while ( 0 != ( Character = *pString++ )) {
    //
    //  Convert the character to UTF-8
    //
    if ( 0 != ( Character & 0xf800 )) {
      //
      //  Send the upper 4 bits
      //
      Data = (UINT8)(( Character >> 12 ) & 0xf );
      Data |= 0xe0;
      Status = HttpSendByte ( SocketFD,
                              pPort,
                              Data );
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Send the next 6 bits
      //
      Data = (UINT8)(( Character >> 6 ) & 0x3f );
      Data |= 0x80;
      Status = HttpSendByte ( SocketFD,
                              pPort,
                              Data );
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Send the last 6 bits
      //
      Data = (UINT8)( Character & 0x3f );
      Data |= 0x80;
    }
    else if ( 0 != ( Character & 0x0780 )) {
      //
      //  Send the upper 5 bits
      //
      Data = (UINT8)(( Character >> 6 ) & 0x1f );
      Data |= 0xc0;
      Status = HttpSendByte ( SocketFD,
                              pPort,
                              Data );
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Send the last 6 bits
      //
      Data = (UINT8)( Character & 0x3f );
      Data |= 0x80;
    }
    else {
      Data = (UINT8)( Character & 0x7f );
    }

    //
    //  Send the last data byte
    //
    Status = HttpSendByte ( SocketFD,
                            pPort,
                            Data );
    if ( EFI_ERROR ( Status )) {
      break;
    }
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Output a value to the HTML page

  @param [in] SocketFD    Socket file descriptor
  @param [in] pPort       The WSDT_PORT structure address
  @param [in] Value       Value to display

  @retval EFI_SUCCESS Successfully displayed the address
**/
EFI_STATUS
HttpSendValue (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN UINT64 Value
  )
{
  BOOLEAN bDisplayZeros;
  UINT64 Digit;
  CONST UINT64 * pEnd;
  CONST UINT64 * pDivisor;
  CONST UINT64 pDivisors[ ] = {
     10000000000000000000ULL,
      1000000000000000000ULL,
       100000000000000000ULL,
        10000000000000000ULL,
         1000000000000000ULL,
          100000000000000ULL,
           10000000000000ULL,
            1000000000000ULL,
             100000000000ULL,
              10000000000ULL,
               1000000000ULL,
                100000000ULL,
                 10000000ULL,
                  1000000ULL,
                   100000ULL,
                    10000ULL,
                     1000ULL,
                      100ULL,
                       10ULL
  };
  EFI_STATUS Status;
  UINT64 Temp;

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Walk the list of divisors
  //
  bDisplayZeros = FALSE;
  pDivisor = &pDivisors[0];
  pEnd = &pDivisor[ sizeof ( pDivisors ) / sizeof ( pDivisors[0])];
  while ( pEnd > pDivisor ) {
    //
    //  Determine the next digit
    //
    Digit = Value / *pDivisor;

    //
    //  Suppress leading zeros
    //
    if (( 0 != Digit ) || bDisplayZeros ) {
      bDisplayZeros = TRUE;

      //
      //  Display the digit
      //
      Status = HttpSendByte ( SocketFD, pPort, (UINT8)( '0' + Digit ));
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Determine the remainder
      //
      Temp = *pDivisor * Digit;
      Value -= Temp;
    }

    //
    //  Set the next divisor
    //
    pDivisor += 1;
  }

  //
  //  Display the final digit
  //
  if ( !EFI_ERROR ( Status )) {
    Status = HttpSendByte ( SocketFD, pPort, (UINT8)( '0' + Value ));
  }

  //
  //  Return the operation status
  //
  return Status;
}
