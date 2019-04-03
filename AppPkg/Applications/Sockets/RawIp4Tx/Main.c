/** @file
  Raw IP4 transmit test application

  Copyright (c) 2011-2012, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RawIp4Tx.h"


/**
  Transmit raw IP4 packets to the remote system.

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
  int RetVal;

  //
  //  Run the application
  //
  RetVal = RawIp4Tx ( Argc, Argv );

  //
  //  Return the operation status
  //
  return RetVal;
}
