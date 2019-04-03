/**
  @file
  Display the memory type range registers

  Copyright (c) 2012, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <WebServer.h>
#include <Library/MtrrLib.h>
#include <Register/Msr.h>

#define VARIABLE_MTRR_VALID     0x800

CONST char * mMemoryType [ ] = {
  "Uncached",
  "Write Combining",
  "Reserved",
  "Reserved",
  "Write Through",
  "Write Protected",
  "Writeback"
};


/**
  Display a fixed MTRR row

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [in] Start         Start address for the region
  @param [in] End           End address for the region
  @param [in] Type          Memory type

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
MtrrDisplayFixedRow (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  IN UINT64 Start,
  IN UINT64 End,
  IN UINT64 Type
  )
{
  EFI_STATUS Status;

  //
  //  Use break instead of goto
  //
  for ( ; ; ) {
    //
    //  Start the row
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "  <tr><td align=\"right\"><code>0x" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    
    //
    //  Start
    //
    Status = HttpSendHexValue ( SocketFD,
                                pPort,
                                Start );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    
    //
    //  End
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</code></td><td align=\"right\"><code>0x" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Status = HttpSendHexValue ( SocketFD,
                                pPort,
                                End - 1 );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    
    //
    //  Type
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</code></td><td>" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    Type &= 0xff;
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  ( DIM ( mMemoryType ) > Type )
                                  ? mMemoryType [ Type ]
                                  : "Reserved" );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    
    //
    //  End of row
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "</td></tr>\r\n" );
    break;
  }

  //
  //  Return the final status
  //
  return Status;
}


/**
  Display the memory type registers

  @param [in] SocketFD      The socket's file descriptor to add to the list.
  @param [in] pPort         The WSDT_PORT structure address
  @param [out] pbDone       Address to receive the request completion status

  @retval EFI_SUCCESS       The request was successfully processed

**/
EFI_STATUS
MemoryTypeRegistersPage (
  IN int SocketFD,
  IN WSDT_PORT * pPort,
  OUT BOOLEAN * pbDone
  )
{
  UINT64 Addr;
  BOOLEAN bValid;
  MSR_IA32_MTRRCAP_REGISTER Capabilities;
  UINTN Count;
  MSR_IA32_MTRR_DEF_TYPE_REGISTER DefType;
  UINTN Index;
  UINT64 Mask;

  CONST UINT64 mFixedAddresses [( 8 * MTRR_NUMBER_OF_FIXED_MTRR ) + 1 ] = {
           0ULL,
     0x10000ULL,
     0x20000ULL,
     0x30000ULL,
     0x40000ULL,
     0x50000ULL,
     0x60000ULL,
     0x70000ULL,

     0x80000ULL,
     0x84000ULL,
     0x88000ULL,
     0x8c000ULL,
     0x90000ULL,
     0x94000ULL,
     0x98000ULL,
     0x9c000ULL,

     0xa0000ULL,
     0xa4000ULL,
     0xa8000ULL,
     0xac000ULL,
     0xb0000ULL,
     0xb4000ULL,
     0xb8000ULL,
     0xbc000ULL,

     0xc0000ULL,
     0xc1000ULL,
     0xc2000ULL,
     0xc3000ULL,
     0xc4000ULL,
     0xc5000ULL,
     0xc6000ULL,
     0xc7000ULL,

     0xc8000ULL,
     0xc9000ULL,
     0xca000ULL,
     0xcb000ULL,
     0xcc000ULL,
     0xcd000ULL,
     0xce000ULL,
     0xcf000ULL,

     0xd0000ULL,
     0xd1000ULL,
     0xd2000ULL,
     0xd3000ULL,
     0xd4000ULL,
     0xd5000ULL,
     0xd6000ULL,
     0xd7000ULL,

     0xd8000ULL,
     0xd9000ULL,
     0xda000ULL,
     0xdb000ULL,
     0xdc000ULL,
     0xdd000ULL,
     0xde000ULL,
     0xdf000ULL,

     0xe0000ULL,
     0xe1000ULL,
     0xe2000ULL,
     0xe3000ULL,
     0xe4000ULL,
     0xe5000ULL,
     0xe6000ULL,
     0xe7000ULL,

     0xe8000ULL,
     0xe9000ULL,
     0xea000ULL,
     0xeb000ULL,
     0xec000ULL,
     0xed000ULL,
     0xee000ULL,
     0xef000ULL,

     0xf0000ULL,
     0xf1000ULL,
     0xf2000ULL,
     0xf3000ULL,
     0xf4000ULL,
     0xf5000ULL,
     0xf6000ULL,
     0xf7000ULL,

     0xf8000ULL,
     0xf9000ULL,
     0xfa000ULL,
     0xfb000ULL,
     0xfc000ULL,
     0xfd000ULL,
     0xfe000ULL,
     0xff000ULL,

    0x100000ULL
  };
  MTRR_SETTINGS Mtrr;
  CONST UINT64 * pMemEnd;
  CONST UINT64 * pMemStart;
  UINT64 PreviousType;
  UINT64 ShiftCount;
  EFI_STATUS Status;
  UINT64 Type;
  INT64 Value;
  
  DBG_ENTER ( );
  
  //
  //  Send the Memory Type Registers page
  //
  for ( ; ; ) {
    //
    //  Send the page header
    //
    Status = HttpPageHeader ( SocketFD, pPort, L"Memory Type Range Registers" );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Send the header
    //
    Status = HttpSendAnsiString ( SocketFD,
                                  pPort,
                                  "<h1>Memory Type Range Registers</h1>\r\n" );
    if ( EFI_ERROR ( Status )) {
      break;
    }

    //
    //  Determine if MTRRs are supported
    //
    if ( !IsMtrrSupported ( )) {
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "<p>Memory Type Range Registers are not supported!\r\n" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
    }
    else {
      //
      //  Get the capabilities
      //
      Capabilities.Uint64 = AsmReadMsr64 ( MSR_IA32_MTRRCAP );
      DefType.Uint64 =  AsmReadMsr64 ( MSR_IA32_MTRR_DEF_TYPE );

      //
      //  Display the capabilities
      //
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "<p>Capabilities: " );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendHexValue ( SocketFD,
                                  pPort,
                                  Capabilities.Uint64 );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "<br>\r\n" );
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Display the default type
      //
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "Def Type: " );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendHexValue ( SocketFD,
                                  pPort,
                                  DefType.Uint64);
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    ", MTRRs " );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    ( 0 != DefType.Bits.E )
                                    ? "Enabled"
                                    : "Disabled" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    ", Fixed MTRRs " );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    ( 0 != DefType.Bits.FE )
                                    ? "Enabled"
                                    : "Disabled" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    ", " );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Type = DefType.Uint64 & 0xff;
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    ( DIM ( mMemoryType ) > Type )
                                    ? mMemoryType [ Type ]
                                    : "Reserved" );
      if ( EFI_ERROR ( Status )) {
        break;
      }
      Status = HttpSendAnsiString ( SocketFD,
                                    pPort,
                                    "</p>\r\n" );
      if ( EFI_ERROR ( Status )) {
        break;
      }

      //
      //  Determine if MTRRs are enabled
      //
      if ( 0 == DefType.Bits.E ) {
        Status = HttpSendAnsiString ( SocketFD,
                                      pPort,
                                      "<p>All memory is uncached!</p>\r\n" );
        if ( EFI_ERROR ( Status )) {
          break;
        }
      }
      else {
        //
        //  Get the MTRRs
        //
        MtrrGetAllMtrrs ( &Mtrr );

        //
        //  Determine if the fixed MTRRs are supported
        //
        if (( 0 != Capabilities.Bits.FIX )
          && ( 0 != DefType.Bits.FE)) {

          //
          //  Beginning of table
          //
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "<h2>Fixed MTRRs</h2>\r\n"
                                        "<table>\r\n"
                                        "  <tr><th>Index</th><th align=\"right\">Value</th><th align=\"right\">Start</th><th align=\"right\">End</th></tr>\r\n" );
          if ( EFI_ERROR ( Status )) {
            break;
          }

          //
          //  Display the fixed MTRRs
          //
          pMemStart = &mFixedAddresses[ 0 ];
          for ( Count = 0; DIM ( Mtrr.Fixed.Mtrr ) > Count; Count++ ) {
            //
            //  Start the row
            //
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          "  <tr><td>" );
            if ( EFI_ERROR ( Status )) {
              break;
            }
            
            //
            //  Index
            //
            Status = HttpSendValue ( SocketFD,
                                     pPort,
                                     Count );
            if ( EFI_ERROR ( Status )) {
              break;
            }
            
            //
            //  Value
            //
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          "</td><td align=\"right\"><code>0x" );
            if ( EFI_ERROR ( Status )) {
              break;
            }
            Status = HttpSendHexValue ( SocketFD,
                                        pPort,
                                        Mtrr.Fixed.Mtrr[ Count ]);
            if ( EFI_ERROR ( Status )) {
              break;
            }

            //
            //  Start
            //
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          "</code></td><td align=\"right\"><code>0x" );
            if ( EFI_ERROR ( Status )) {
              break;
            }
            Status = HttpSendHexValue ( SocketFD,
                                        pPort,
                                        *pMemStart );
            if ( EFI_ERROR ( Status )) {
              break;
            }
            pMemStart += 8;

            //
            //  Value
            //
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          "</code></td><td align=\"right\"><code>0x" );
            if ( EFI_ERROR ( Status )) {
              break;
            }
            Status = HttpSendHexValue ( SocketFD,
                                        pPort,
                                        *pMemStart - 1 );
            if ( EFI_ERROR ( Status )) {
              break;
            }

            //
            //  End of row
            //
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          "</code></td></tr>\r\n" );
            if ( EFI_ERROR ( Status )) {
              break;
            }
          }
          if ( EFI_ERROR ( Status )) {
            break;
          }

          //
          //  End of table
          //
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "</table>\r\n" );
          if ( EFI_ERROR ( Status )) {
            break;
          }

          //
          //  Beginning of table
          //
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "<table>\r\n"
                                        "  <tr><th align=\"right\">Start</th><th align=\"right\">End</th><th align=\"left\">Type</th></tr>\r\n" );
          if ( EFI_ERROR ( Status )) {
            break;
          }

          //
          //  Decode the fixed MTRRs
          //
          PreviousType = Mtrr.Fixed.Mtrr[ 0 ] & 0xff;
          pMemStart = &mFixedAddresses[ 0 ];
          pMemEnd = pMemStart;
          for ( Count = 0; DIM ( Mtrr.Fixed.Mtrr ) > Count; Count++ ) {
            //
            //  Get the memory types
            //
            Type = Mtrr.Fixed.Mtrr[ Count ];

            //
            //  Walk the memory range
            //
            for ( Index = 0; 8 > Index; Index++ ) {
              //
              //  Determine if this is the same memory type
              //
              if ( PreviousType != ( Type & 0xff )) {
                //
                //  Display the row
                //
                Status = MtrrDisplayFixedRow ( SocketFD,
                                               pPort,
                                               *pMemStart,
                                               *pMemEnd,
                                               PreviousType );
                if ( EFI_ERROR ( Status )) {
                  break;
                }

                //
                //  Start the next range of addresses
                //
                pMemStart = pMemEnd;
                PreviousType = Type & 0xff;
              }

              //
              //  Set the next memory range and type
              //
              Type >>= 8;
              pMemEnd += 1;
            }
            if ( EFI_ERROR ( Status )) {
              break;
            }
          }
          if ( EFI_ERROR ( Status )) {
            break;
          }

          //
          //  Display the final row
          //
          Status = MtrrDisplayFixedRow ( SocketFD,
                                         pPort,
                                         *pMemStart,
                                         *pMemEnd,
                                         PreviousType );
          if ( EFI_ERROR ( Status )) {
            break;
          }

          //
          //  End of table 
          //
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "</table>\r\n" );
          if ( EFI_ERROR ( Status )) {
            break;
          }
        }

        //
        //  Determine if the variable MTRRs are supported
        //
        if ( 0 < Capabilities.Bits.VCNT ) {
          //
          //  Beginning of table
          //
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "<h2>Variable MTRRs</h2>\r\n"
                                        "<table>\r\n"
                                        "  <tr><th>Index</th><th align=\"right\">Base</th><th align=\"right\">Mask</th><th align=\"right\">Start</th><th align=\"right\">End</th></tr>\r\n" );
          if ( EFI_ERROR ( Status )) {
            break;
          }

          //
          //  Display the variable MTRRs
          //
          for ( Count = 0; Capabilities.Bits.VCNT > Count; Count++ ) {
            //
            //  Start the row
            //
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          "  <tr><td>" );
            if ( EFI_ERROR ( Status )) {
              break;
            }
            
            //
            //  Index
            //
            Status = HttpSendValue ( SocketFD,
                                     pPort,
                                     Count );
            if ( EFI_ERROR ( Status )) {
              break;
            }
            
            //
            //  Base
            //
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          "</td><td align=\"right\"><code>0x" );
            if ( EFI_ERROR ( Status )) {
              break;
            }
            Status = HttpSendHexValue ( SocketFD,
                                        pPort,
                                        Mtrr.Variables.Mtrr[ Count ].Base );
            if ( EFI_ERROR ( Status )) {
              break;
            }

            //
            //  Mask
            //
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          "</td><td align=\"right\"><code>0x" );
            if ( EFI_ERROR ( Status )) {
              break;
            }
            Status = HttpSendHexValue ( SocketFD,
                                        pPort,
                                        Mtrr.Variables.Mtrr[ Count ].Mask );
            if ( EFI_ERROR ( Status )) {
              break;
            }

            //
            //  Determine if the entry is valid
            //
            bValid = ( Mtrr.Variables.Mtrr[ Count ].Mask & VARIABLE_MTRR_VALID ) ? TRUE : FALSE;

            //
            //  Start
            //
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          "</code></td><td align=\"right\"><code>" );
            if ( EFI_ERROR ( Status )) {
              break;
            }
            Addr = Mtrr.Variables.Mtrr[ Count ].Base & 0xfffffffffffff000ULL;
            if ( bValid ) {
              Status = HttpSendAnsiString ( SocketFD,
                                            pPort,
                                            "0x" );
              if ( EFI_ERROR ( Status )) {
                break;
              }
              Status = HttpSendHexValue ( SocketFD,
                                          pPort,
                                          Addr );
            }
            else {
              Status = HttpSendAnsiString ( SocketFD,
                                            pPort,
                                            "Invalid" );
            }
            if ( EFI_ERROR ( Status )) {
              break;
            }

            //
            //  End
            //
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          "</code></td><td align=\"right\"><code>" );
            if ( EFI_ERROR ( Status )) {
              break;
            }
            if ( bValid ) {
              //
              //  Determine the end address
              //
              Mask = Mtrr.Variables.Mtrr[ Count ].Mask;
              Value = Mask;
              ShiftCount = 0;
              while ( 0 < Value ) {
                Value <<= 1;
                ShiftCount += 1;
              }
              Value = 1;
              Value <<= 64 - ShiftCount;
              Value -= 1;
              Value = ~Value;
              Value |= Mask;
              Value &= ~VARIABLE_MTRR_VALID;
              Value = ~Value;

              Status = HttpSendAnsiString ( SocketFD,
                                            pPort,
                                            "0x" );
              if ( EFI_ERROR ( Status )) {
                break;
              }
              Status = HttpSendHexValue ( SocketFD,
                                          pPort,
                                          Addr + Value );
            }
            if ( EFI_ERROR ( Status )) {
              break;
            }

            //
            //  Type
            //
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          "</code></td><td>" );
            if ( EFI_ERROR ( Status )) {
              break;
            }
            if ( bValid ) {
              Type = Mtrr.Variables.Mtrr[ Count ].Base & 0xFF;
              Status = HttpSendAnsiString ( SocketFD,
                                            pPort,
                                            ( DIM ( mMemoryType ) > Type )
                                            ? mMemoryType [ Type ]
                                            : "Reserved" );
            }
            if ( EFI_ERROR ( Status )) {
              break;
            }

            //
            //  End of row
            //
            Status = HttpSendAnsiString ( SocketFD,
                                          pPort,
                                          "</td></tr>\r\n" );
            if ( EFI_ERROR ( Status )) {
              break;
            }
          }
          if ( EFI_ERROR ( Status )) {
            break;
          }

          //
          //  End of table 
          //
          Status = HttpSendAnsiString ( SocketFD,
                                        pPort,
                                        "</table>\r\n" );
          if ( EFI_ERROR ( Status )) {
            break;
          }
        }
      }
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
