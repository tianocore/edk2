/** @file
  The abs, labs, and llabs functions compute the absolute value of an integer j.
  If the result cannot be represented, the behavior is undefined.

  The abs, labs, and llabs, functions return the absolute value of their
  parameter.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>

int
abs(int j)
{
  return(j < 0 ? -j : j);
}

long
labs(long j)
{
  return(j < 0 ? -j : j);
}

long long
llabs(long long j)
{
  return (j < 0 ? -j : j);
}
