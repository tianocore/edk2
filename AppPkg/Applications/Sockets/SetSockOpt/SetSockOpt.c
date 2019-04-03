/** @file
  Set the socket options

  Copyright (c) 2011-2012, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <Uefi.h>
#include <unistd.h>

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>

#include <sys/socket.h>
#include <sys/time.h>

typedef enum _DATA_TYPE {
  DATA_TYPE_UNKNOWN = 0,
  DATA_TYPE_INT32_DECIMAL,
  DATA_TYPE_SOCKET_TYPE,
  DATA_TYPE_TIMEVAL
} DATA_TYPE;

typedef struct {
  char * pOptionName;
  int OptionValue;
  int OptionLevel;
  BOOLEAN bSetAllowed;
  DATA_TYPE DataType;
} OPTIONS;

CONST OPTIONS mOptions[] = {
  { "SO_ACCEPTCONN", SO_ACCEPTCONN, SOL_SOCKET, FALSE, DATA_TYPE_UNKNOWN },
  { "SO_BROADCAST", SO_BROADCAST, SOL_SOCKET, TRUE, DATA_TYPE_UNKNOWN },
  { "SO_DEBUG", SO_DEBUG, SOL_SOCKET, TRUE, DATA_TYPE_UNKNOWN },
  { "SO_DONTROUTE", SO_DONTROUTE, SOL_SOCKET, TRUE, DATA_TYPE_UNKNOWN },
  { "SO_ERROR", SO_ERROR, SOL_SOCKET, FALSE, DATA_TYPE_UNKNOWN },
  { "SO_KEEPALIVE", SO_KEEPALIVE, SOL_SOCKET, TRUE, DATA_TYPE_UNKNOWN },
  { "SO_OOBINLINE", SO_OOBINLINE, SOL_SOCKET, TRUE, DATA_TYPE_UNKNOWN },
  { "SO_OVERFLOWED", SO_OVERFLOWED, SOL_SOCKET, TRUE, DATA_TYPE_UNKNOWN },
  { "SO_RCVBUF", SO_RCVBUF, SOL_SOCKET, TRUE, DATA_TYPE_INT32_DECIMAL },
  { "SO_RCVLOWAT", SO_RCVLOWAT, SOL_SOCKET, TRUE, DATA_TYPE_UNKNOWN },
  { "SO_RCVTIMEO", SO_RCVTIMEO, SOL_SOCKET, TRUE, DATA_TYPE_TIMEVAL },
  { "SO_REUSEADDR", SO_REUSEADDR, SOL_SOCKET, TRUE, DATA_TYPE_UNKNOWN },
  { "SO_REUSEPORT", SO_REUSEPORT, SOL_SOCKET, TRUE, DATA_TYPE_UNKNOWN },
  { "SO_SNDBUF", SO_SNDBUF, SOL_SOCKET, TRUE, DATA_TYPE_INT32_DECIMAL },
  { "SO_SNDLOWAT", SO_SNDLOWAT, SOL_SOCKET, TRUE, DATA_TYPE_UNKNOWN },
  { "SO_SNDTIMEO", SO_SNDTIMEO, SOL_SOCKET, TRUE, DATA_TYPE_UNKNOWN },
  { "SO_TIMESTAMP", SO_TIMESTAMP, SOL_SOCKET, TRUE, DATA_TYPE_UNKNOWN },
  { "SO_TYPE", SO_TYPE, SOL_SOCKET, FALSE, DATA_TYPE_SOCKET_TYPE },
  { "SO_USELOOPBACK", SO_USELOOPBACK, SOL_SOCKET, TRUE, DATA_TYPE_UNKNOWN }
};


UINT8 mBuffer[ 65536 ];
UINT8 mValue[ 65536 ];
char * mSocketType[] = {
  "SOCK_STREAM",
  "SOCK_DGRAM",
  "SOCK_RAW",
  "SOCK_RDM",
  "SOCK_SEQPACKET"
};

void
DisplayOption (
  CONST OPTIONS * pOption,
  socklen_t LengthInBytes,
  BOOLEAN bDisplayUpdate,
  BOOLEAN bDisplayCrLf
  )
{
  UINT8 * pEnd;
  char * pString;
  union {
    UINT8 * u8;
    INT32 * i32;
    struct timeval * TimeVal;
  } Value;

  //
  //  Display the value length
  //
  if ( !bDisplayUpdate ) {
    Print ( L"LengthInBytes: %d\r\n", LengthInBytes );
    Print ( L"%a: ", pOption->pOptionName );
  }
  else {
    Print ( L" --> " );
  }

  //
  //  Display the value
  //
  Value.u8 = &mBuffer[0];
  switch ( pOption->DataType ) {
  case DATA_TYPE_UNKNOWN:
    Print ( L"%a:", pOption->pOptionName );
    pEnd = &Value.u8[ LengthInBytes ];
    while ( pEnd > Value.u8 ) {
      Print ( L" %02x", *Value.u8 );
      Value.u8 += 1;
    }
    break;
    
  case DATA_TYPE_INT32_DECIMAL:
    if ( 4 == LengthInBytes ) {
      Print ( L"%d", *Value.i32 );
    }
    else {
      errno = ( 4 > LengthInBytes ) ? EBUFSIZE : ERANGE;
      Print ( L"\r\nERROR - Invalid length, errno: %d\r\n", errno );
    }
    break;

  case DATA_TYPE_SOCKET_TYPE:
    if ( 4 == LengthInBytes ) {
      if (( SOCK_STREAM <= *Value.i32 ) && ( SOCK_SEQPACKET >= *Value.i32 )) {
        pString = mSocketType[ *Value.i32 - SOCK_STREAM ];
        Print ( L"%a", pString );
      }
      else {
        Print ( L"%08x (unknown type)", *Value.i32 );
      }
    }
    else {
      errno = ( 4 > LengthInBytes ) ? EBUFSIZE : ERANGE;
      Print ( L"\r\nERROR - Invalid length, errno: %d\r\n", errno );
    }
    break;

  case DATA_TYPE_TIMEVAL:
    if ( sizeof ( *Value.TimeVal ) == LengthInBytes ) {
      if (( 0 == Value.TimeVal->tv_sec )
        && ( 0 == Value.TimeVal->tv_usec )) {
        Print ( L"Infinite" );
      }
      else {
        Print ( L"%d.%06d sec",
                Value.TimeVal->tv_sec,
                Value.TimeVal->tv_usec );
      }
    }
    else {
      errno = ( 4 > LengthInBytes ) ? EBUFSIZE : ERANGE;
      Print ( L"\r\nERROR - Invalid length, errno: %d\r\n", errno );
    }
    break;
  }

  //
  //  Terminate the line
  //
  if ( bDisplayCrLf ) {
    Print ( L"\r\n" );
  }
}

socklen_t
GetOptionValue (
  CONST OPTIONS * pOption,
  char * pValue
  )
{
  socklen_t BytesToWrite;
  union {
    UINT8 * u8;
    INT32 * i32;
    struct timeval * TimeVal;
  } Value;
  int Values;

  //
  //  Assume failure
  //
  errno = EINVAL;
  BytesToWrite = 0;

  //
  //  Determine the type of parameter
  //
  if ( pOption->bSetAllowed ) {
    Value.u8 = &mValue[0];
    switch ( pOption->DataType ) {
    default:
      break;

    case DATA_TYPE_INT32_DECIMAL:
      Values = sscanf ( pValue, "%d", Value.i32 );
      if ( 1 == Values ) {
        BytesToWrite = sizeof ( *Value.i32);
        errno = 0;
      }
      break;

    case DATA_TYPE_TIMEVAL:
      Values = sscanf ( pValue, "%d.%d",
                        &Value.TimeVal->tv_sec,
                        &Value.TimeVal->tv_usec );
      if (( 2 == Values )
        && ( 0 <= Value.TimeVal->tv_sec )
        && ( 0 <= Value.TimeVal->tv_usec )
        && ( 1000000 > Value.TimeVal->tv_usec )){
        BytesToWrite = sizeof ( *Value.TimeVal );
        errno = 0;
      }
    }
  }

  //
  //  Display the error
  //
  if ( 0 == BytesToWrite ) {
    Print ( L"ERROR - Invalid value!\r\n" );
  }

  //
  //  Return the number of bytes to be written
  //
  return BytesToWrite;
}


/**
  Set the socket options

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
  socklen_t BytesToWrite;
  socklen_t LengthInBytes;
  CONST OPTIONS * pEnd;
  CONST OPTIONS * pOption;
  int s;
  int Status;

  DEBUG (( DEBUG_INFO,
            "%a starting\r\n",
            Argv[0]));

  //
  //  Parse the socket option
  //
  pOption = &mOptions[0];
  pEnd = &pOption[sizeof ( mOptions ) / sizeof ( mOptions[0])];
  if ( 2 <= Argc ) {
    while ( pEnd > pOption ) {
      if ( 0 == strcmp ( Argv[1], pOption->pOptionName )) {
        break;
      }
      pOption += 1;
    }
    if ( pEnd <= pOption ) {
      Print ( L"ERROR: Invalid option: %a\r\n", Argv[1]);
      Argc = 1;
    }
  }

  //
  //  Display the help if necessary
  //
  if (( 2 > Argc ) || ( 3 < Argc )) {
    Print ( L"%a <option>\r\n", Argv[0]);
    Print ( L"\r\n" );
    Print ( L"Option one of:\r\n" );
    pOption = &mOptions[0];
    while ( pEnd > pOption ) {
      Print ( L"   %a: %a\r\n",
              pOption->pOptionName,
              pOption->bSetAllowed ? "get/set" : "get" );
      pOption += 1;
    }
    errno = EINVAL;
  }
  else {
    //
    //  Determine if the value is to be set
    //
    BytesToWrite = 0;
    if (( 3 > Argc )
      || ( 0 < ( BytesToWrite = GetOptionValue ( pOption, Argv[2])))) {
      //
      //  Get the socket
      //
      s = socket ( AF_INET, 0, 0 );
      if ( -1 == s ) {
        Print ( L"ERROR - Unable to open the socket, errno: %d\r\n", errno );
      }
      else {
        //
        //  Display the option value
        //
        LengthInBytes = sizeof ( mBuffer );
        Status = getsockopt ( s,
                              pOption->OptionLevel,
                              pOption->OptionValue,
                              &mBuffer,
                              &LengthInBytes );
        if ( -1 == Status ) {
          Print ( L"ERROR - getsockopt failed, errno: %d\r\n", errno );
        }
        else {
          DisplayOption ( pOption,
                          LengthInBytes,
                          FALSE,
                          (BOOLEAN)( 0 == BytesToWrite ));

          //
          //  Determine if the value is to be set
          //
          if (( 0 < BytesToWrite )
              && ( BytesToWrite == LengthInBytes )) {
            //
            //  Set the option value
            //
            Status = setsockopt ( s,
                                  pOption->OptionLevel,
                                  pOption->OptionValue,
                                  &mValue,
                                  BytesToWrite );
            if ( -1 == Status ) {
              Print ( L"ERROR - setsockopt failed, errno: %d\r\n", errno );
            }
            else {
              //
              //  Display the updated option value
              //
              Status = getsockopt ( s,
                                    pOption->OptionLevel,
                                    pOption->OptionValue,
                                    &mBuffer,
                                    &LengthInBytes );
              if ( -1 == Status ) {
                Print ( L"ERROR - getsockopt failed, errno: %d\r\n", errno );
              }
              else {
                DisplayOption ( pOption,
                                LengthInBytes,
                                TRUE,
                                TRUE );
              }
            }
          }
        }

        //
        //  Done with the socket
        //
        close ( s );
      }
    }
  }

  //
  //  All done
  //
  DEBUG (( DEBUG_INFO,
            "%a exiting, errno: %d\r\n",
            Argv[0],
            errno ));
  return errno;
}
