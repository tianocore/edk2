/**
  @file
  Display the system table

  Copyright (c) 2011-2012, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <WebServer.h>


/**
  Display the EFI Table Header

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pHeader       Address of the EFI_TABLE_HEADER structure

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
EfiTableHeader (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN EFI_TABLE_HEADER * pHeader
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );
  
  //
  //  Send the handles page
  //
  for ( ; ; ) {
    ///
    /// A 64-bit signature that identifies the type of table that follows.
    /// Unique signatures have been generated for the EFI System Table,
    /// the EFI Boot Services Table, and the EFI Runtime Services Table.
    ///
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "Hdr.Signature",
                           pHeader->Signature,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// The revision of the EFI Specification to which this table
    /// conforms. The upper 16 bits of this field contain the major
    /// revision value, and the lower 16 bits contain the minor revision
    /// value. The minor revision values are limited to the range of 00..99.
    ///
    Status = RowRevision ( SocketFD,
                           pPort,
                           "Hdr.Revision",
                           pHeader->Revision );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// The size, in bytes, of the entire table including the EFI_TABLE_HEADER.
    ///
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "Hdr.HeaderSize",
                               pHeader->HeaderSize );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// The 32-bit CRC for the entire table. This value is computed by
    /// setting this field to 0, and computing the 32-bit CRC for HeaderSize bytes.
    ///
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "Hdr.CRC",
                           pHeader->CRC32,
                           NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// Reserved field that must be set to 0.
    ///
    Status = RowHexValue ( SocketFD,
                           pPort,
                           "Hdr.Reserved",
                           pHeader->Reserved,
                           NULL );
    break;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Display a row containing a decimal value

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pName         Address of a zero terminated name string
  @param [in] Value         The value to display

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
RowDecimalValue (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST CHAR8 * pName,
  IN UINT64 Value
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<tr><td>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  pName );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</td><td><code>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendValue ( SocketFD,
                             pPort,
                             Value );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</code></td></tr>\r\n" );
    break;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Display a row containing a hex value

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pName         Address of a zero terminated name string
  @param [in] Value         The value to display
  @param [in] pWebPage      Address of a zero terminated web page name

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
RowHexValue (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST CHAR8 * pName,
  IN UINT64 Value,
  IN CONST CHAR16 * pWebPage
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<tr><td>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  pName );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</td><td><code>0x" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    if ( NULL != pWebPage ) {
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "<a target=\"_blank\" href=\"" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendUnicodeString ( SocketFD,
                                       pPort,
                                       pWebPage );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "\">" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }
    Status = HttpSendHexValue ( SocketFD,
                                pPort,
                                Value );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    if ( NULL != pWebPage ) {
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "</a>" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</code></td></tr>\r\n" );
    break;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Display a row containing a pointer

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pName         Address of a zero terminated name string
  @param [in] pAddress      The address to display
  @param [in] pWebPage      Address of a zero terminated web page name

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
RowPointer (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST CHAR8 * pName,
  IN CONST VOID * pAddress,
  IN CONST CHAR16 * pWebPage
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<tr><td>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  pName );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</td><td><code>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    if ( NULL != pWebPage ) {
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "<a target=\"_blank\" href=\"" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendUnicodeString ( SocketFD,
                                       pPort,
                                       pWebPage );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "\">" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "0x" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendHexBits ( SocketFD,
                               pPort,
                               sizeof ( pAddress ) * 8,
                               (UINT64)(UINTN)pAddress );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    if ( NULL != pWebPage ) {
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "</a>" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</code></td></tr>\r\n" );
    break;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Display a row containing a revision

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pName         Address of a zero terminated name string
  @param [in] Revision      The revision to display

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
RowRevision (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST CHAR8 * pName,
  IN UINT32 Revision
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<tr><td>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  pName );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</td><td><code>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendValue ( SocketFD,
                             pPort,
                             Revision >> 16 );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendByte ( SocketFD,
                            pPort,
                            '.' );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendValue ( SocketFD,
                             pPort,
                             Revision & 0xFFFF );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</code></td></tr>\r\n" );
    break;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Display a row containing a unicode string

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pName         Address of a zero terminated name string
  @param [in] pString       Address of a zero terminated unicode string

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
RowUnicodeString (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST CHAR8 * pName,
  IN CONST CHAR16 * pString
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<tr><td>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  pName );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</td><td>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendUnicodeString ( SocketFD,
                                     pPort,
                                     pString );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</td></tr>\r\n" );
    break;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Start the table page

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] pName         Address of a zero terminated name string
  @param [in] pTable        Address of the table

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
TableHeader (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN CONST CHAR16 * pName,
  IN CONST VOID * pTable
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
    //
    //  Send the page header
    //
    Status = HttpPageHeader ( SocketFD, pPort, pName );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    
    //
    //  Build the table header
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<h1>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendUnicodeString ( SocketFD,
                                     pPort,
                                     pName );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    if ( NULL != pTable ) {
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    ": 0x" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendHexBits ( SocketFD,
                                 pPort,
                                 sizeof ( pTable ) *  8,
                                 (UINT64)(UINTN)pTable );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</h1>\r\n"
                                  "<table border=\"1\">\r\n"
                                  "  <tr bgcolor=\"c0c0ff\"><th>Field Name</th><th>Value</th></tr>\r\n" );
    break;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  End the table page

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
TableTrailer (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN *pbDone
  )
{
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Use for/break instead of goto
  //
  for ( ; ; ) {
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


/**
  Respond with the system table

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
SystemTablePage (
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
    Status = TableHeader ( SocketFD, pPort, L"System Table", gST );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// The table header for the EFI System Table.
    ///
    Status = EfiTableHeader ( SocketFD,
                              pPort,
                              &gST->Hdr );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// A pointer to a null terminated string that identifies the vendor
    /// that produces the system firmware for the platform.
    ///
    Status = RowUnicodeString ( SocketFD,
                                pPort,
                                "FirmwareVendor",
                                gST->FirmwareVendor );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// A firmware vendor specific value that identifies the revision
    /// of the system firmware for the platform.
    ///
    Status = RowRevision ( SocketFD,
                           pPort,
                           "FirmwareRevision",
                           gST->FirmwareRevision );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// The handle for the active console input device. This handle must support
    /// EFI_SIMPLE_TEXT_INPUT_PROTOCOL and EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL.
    ///
    Status = RowPointer ( SocketFD,
                          pPort,
                          "ConsoleInHandle",
                          (VOID *)gST->ConsoleInHandle,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// A pointer to the EFI_SIMPLE_TEXT_INPUT_PROTOCOL interface that is
    /// associated with ConsoleInHandle.
    ///
    Status = RowPointer ( SocketFD,
                          pPort,
                          "ConIn",
                          (VOID *)gST->ConIn,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// The handle for the active console output device.
    ///
    Status = RowPointer ( SocketFD,
                          pPort,
                          "ConsoleOutHandle",
                          (VOID *)gST->ConsoleOutHandle,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// A pointer to the EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL interface
    /// that is associated with ConsoleOutHandle.
    ///
    Status = RowPointer ( SocketFD,
                          pPort,
                          "ConOut",
                          (VOID *)gST->ConOut,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// The handle for the active standard error console device.
    /// This handle must support the EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.
    ///
    Status = RowPointer ( SocketFD,
                          pPort,
                          "StandardErrorHandle",
                          (VOID *)gST->StandardErrorHandle,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// A pointer to the EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL interface
    /// that is associated with StandardErrorHandle.
    ///
    Status = RowPointer ( SocketFD,
                          pPort,
                          "StdErr",
                          (VOID *)gST->StdErr,
                          NULL );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// A pointer to the EFI Runtime Services Table.
    ///
    Status = RowPointer ( SocketFD,
                          pPort,
                          "RuntimeServices",
                          (VOID *)gST->RuntimeServices,
                          PAGE_RUNTIME_SERVICES_TABLE );

    ///
    /// A pointer to the EFI Boot Services Table.
    ///
    Status = RowPointer ( SocketFD,
                          pPort,
                          "BootServices",
                          (VOID *)gST->BootServices,
                          PAGE_BOOT_SERVICES_TABLE );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// The number of system configuration tables in the buffer ConfigurationTable.
    ///
    Status = RowDecimalValue ( SocketFD,
                               pPort,
                               "NumberOfTableEntries",
                               gST->NumberOfTableEntries );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    ///
    /// A pointer to the system configuration tables.
    /// The number of entries in the table is NumberOfTableEntries.
    ///
    Status = RowPointer ( SocketFD,
                          pPort,
                          "ConfigurationTable",
                          (VOID *)gST->ConfigurationTable,
                          PAGE_CONFIGURATION_TABLE );
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
