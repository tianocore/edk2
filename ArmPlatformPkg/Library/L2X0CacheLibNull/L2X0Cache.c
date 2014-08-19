/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Uefi.h>
#include <Drivers/PL310L2Cache.h>

// Initialize L2X0 Cache Controller
VOID
L2x0CacheInit (
  IN  UINTN   L2x0Base,
  IN  UINT32  L2x0TagLatencies,
  IN  UINT32  L2x0DataLatencies,
  IN  UINT32  L2x0AuxValue,
  IN  UINT32  L2x0AuxMask,
  IN  BOOLEAN CacheEnabled
  )
{
    //No implementation
}
