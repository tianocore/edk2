/** @file
  Return the current FPU rounding mode.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

extern int internal_FPU_rmode( void );

static INT8  rmode[] = { 1, 3, 2, 0 };

int
__flt_rounds ( void )
{
  return rmode[ internal_FPU_rmode() ];
}
