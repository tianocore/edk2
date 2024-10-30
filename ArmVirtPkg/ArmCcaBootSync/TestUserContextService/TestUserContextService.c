/** @file
  Host based unit test User Context service.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <stdio.h>

/**
  Entrypoint for the Test User Context service.

  @param[in] argc         Number of input arguments.
  @param[in] argv         Input arguments.

  @retval    0             Success.
            -1             Failure
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  printf ("Test User Context Service.\n");

  return 0;
}
