/** @file
  Raw IP4 transmit test application

  Copyright (c) 2011-2012, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
