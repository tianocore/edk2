/**
  @file
  Display the memory map

  Copyright (c) 2012, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <WebServer.h>
#include <PiDxe.h>
#include <Library/DxeServicesTableLib.h>


CONST char * mpMemoryType[ ] = {
  "Non-existent",
  "Reserved",
  "System Memory",
  "Memory Mapped I/O"
};


/**
  Page to display the memory map

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
MemoryMapPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  UINT64 Attributes;
  BOOLEAN bSomethingDisplayed;
  UINTN Count;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR * pMemoryEnd;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR * pMemoryDescriptor;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR * pMemoryDescriptorStart;
  EFI_STATUS Status;
  
  DBG_ENTER ( );
  
  //
  //  Send the memory map page
  //
  pMemoryDescriptorStart = NULL;
  for ( ; ; ) {
    //
    //  Send the page header
    //
    Status = HttpPageHeader ( SocketFD, pPort, L"Memory Map" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
  
    //
    //  Start the table
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<h1>Memory Map</h1>\r\n"
                                  "<table>\r\n"
                                  "  <tr><th align=\"right\">Type</th><th align=\"right\">Start</th><th align=\"right\">End</th><th align=\"right\">Attributes</th></tr>\r\n" );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Get the memory map
    //
    Status = gDS->GetMemorySpaceMap ( &Count,
                                      &pMemoryDescriptor );
    if ( !EFI_ERROR ( Status )) {
      pMemoryDescriptorStart = pMemoryDescriptor;
      pMemoryEnd = &pMemoryDescriptor[ Count ];
      while ( pMemoryEnd > pMemoryDescriptor ) {
        //
        //  Display the type
        //
        Status = HttpSendAnsiString ( SocketFD, pPort, "<tr><td align=\"right\"><code>" );
        if ( EFI_ERROR ( Status )) {
          break;
        }
        if ( DIM ( mpMemoryType ) > pMemoryDescriptor->GcdMemoryType ) {
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        mpMemoryType[ pMemoryDescriptor->GcdMemoryType ]);
        }
        else {
          Status = HttpSendValue ( SocketFD,
                                   pPort,
                                   pMemoryDescriptor->GcdMemoryType );
        }
        if ( EFI_ERROR ( Status )) {
          break;
        }

        //
        //  Display the start address
        //
        Status = HttpSendAnsiString ( SocketFD, pPort, "</code></td><td align=\"right\"><code>0x" );
        if ( EFI_ERROR ( Status )) {
          break;
        }
        Status = HttpSendHexValue ( SocketFD,
                                    pPort,
                                    pMemoryDescriptor->BaseAddress );
        if ( EFI_ERROR ( Status )) {
          break;
        }

        //
        //  Display the end address
        //
        Status = HttpSendAnsiString ( SocketFD, pPort, "</code></td><td align=\"right\"><code>0x" );
        if ( EFI_ERROR ( Status )) {
          break;
        }
        Status = HttpSendHexValue ( SocketFD,
                                    pPort,
                                    pMemoryDescriptor->BaseAddress
                                    + pMemoryDescriptor->Length
                                    - 1 );
        if ( EFI_ERROR ( Status )) {
          break;
        }

        //
        //  Display the attributes
        //
        Status = HttpSendAnsiString ( SocketFD, pPort, "</code></td><td align=\"right\"><code>0x" );
        if ( EFI_ERROR ( Status )) {
          break;
        }
        Status = HttpSendHexValue ( SocketFD,
                                    pPort,
                                    pMemoryDescriptor->Attributes );
        if ( EFI_ERROR ( Status )) {
          break;
        }

        //
        //  Decode the attributes
        //
        Status = HttpSendAnsiString ( SocketFD, pPort, "</code></td><td>" );
        if ( EFI_ERROR ( Status )) {
          break;
        }
        bSomethingDisplayed = FALSE;
        Attributes = pMemoryDescriptor->Attributes;

        if ( 0 != ( Attributes & EFI_MEMORY_RUNTIME )) {
          bSomethingDisplayed = TRUE;
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "Runtime" );
          if ( EFI_ERROR ( Status )) {
            break;
          }
        }

        if ( 0 != ( Attributes & EFI_MEMORY_XP )) {
          if ( bSomethingDisplayed ) {
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          ", " );
            if ( EFI_ERROR ( Status )) {
              break;
            }
          }
          bSomethingDisplayed = TRUE;
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "No Execute" );
          if ( EFI_ERROR ( Status )) {
            break;
          }
        }

        if ( 0 != ( Attributes & EFI_MEMORY_RP )) {
          if ( bSomethingDisplayed ) {
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          ", " );
            if ( EFI_ERROR ( Status )) {
              break;
            }
          }
          bSomethingDisplayed = TRUE;
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "No Read" );
          if ( EFI_ERROR ( Status )) {
            break;
          }
        }

        if ( 0 != ( Attributes & EFI_MEMORY_WP )) {
          if ( bSomethingDisplayed ) {
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          ", " );
            if ( EFI_ERROR ( Status )) {
              break;
            }
          }
          bSomethingDisplayed = TRUE;
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "No Write" );
          if ( EFI_ERROR ( Status )) {
            break;
          }
        }

        if ( 0 != ( Attributes & EFI_MEMORY_UCE )) {
          if ( bSomethingDisplayed ) {
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          ", " );
            if ( EFI_ERROR ( Status )) {
              break;
            }
          }
          bSomethingDisplayed = TRUE;
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "UCE" );
          if ( EFI_ERROR ( Status )) {
            break;
          }
        }


        if ( 0 != ( Attributes & EFI_MEMORY_WB )) {
          if ( bSomethingDisplayed ) {
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          ", " );
            if ( EFI_ERROR ( Status )) {
              break;
            }
          }
          bSomethingDisplayed = TRUE;
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "Write Back" );
          if ( EFI_ERROR ( Status )) {
            break;
          }
        }

        if ( 0 != ( Attributes & EFI_MEMORY_WT )) {
          if ( bSomethingDisplayed ) {
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          ", " );
            if ( EFI_ERROR ( Status )) {
              break;
            }
          }
          bSomethingDisplayed = TRUE;
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "Write Through" );
          if ( EFI_ERROR ( Status )) {
            break;
          }
        }

        if ( 0 != ( Attributes & EFI_MEMORY_WC )) {
          if ( bSomethingDisplayed ) {
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          ", " );
            if ( EFI_ERROR ( Status )) {
              break;
            }
          }
          bSomethingDisplayed = TRUE;
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "Write Combining" );
          if ( EFI_ERROR ( Status )) {
            break;
          }
        }

        if ( 0 != ( Attributes & EFI_MEMORY_UC )) {
          if ( bSomethingDisplayed ) {
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          ", " );
            if ( EFI_ERROR ( Status )) {
              break;
            }
          }
          bSomethingDisplayed = TRUE;
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "Uncached" );
          if ( EFI_ERROR ( Status )) {
            break;
          }
        }

        //
        //  Finish the row
        //
        Status = HttpSendAnsiString ( SocketFD, pPort, "</td></tr>" );
        if ( EFI_ERROR ( Status )) {
          break;
        }

        //
        //  Set the next memory descriptor
        //
        pMemoryDescriptor += 1;
      }
    }

    //
    //  Finish the table
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
  //  Release the memory descriptors
  //
  if ( NULL != pMemoryDescriptorStart ) {
    FreePool ( pMemoryDescriptorStart );
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}
