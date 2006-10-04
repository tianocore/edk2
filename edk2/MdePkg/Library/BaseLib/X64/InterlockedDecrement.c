/** @file
  InterlockedDecrement function

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#if _MSC_EXTENSIONS

//
// Microsoft Visual Studio 7.1 Function Prototypes for I/O Intrinsics
//
long _InterlockedDecrement(
   long * lpAddend
);

#pragma intrinsic(_InterlockedDecrement)

UINT32
EFIAPI
InternalSyncDecrement (
  IN      UINT32                    *Value
  )
{
  return _InterlockedDecrement (Value);
}

#endif
