/** @file
  Cache Maintenance Functions.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

typedef struct {
  UINT64                            Status;
  UINT64                            r9;
  UINT64                            r10;
  UINT64                            r11;
} PAL_PROC_RETURN;

PAL_PROC_RETURN
CallPalProcStatic (
  IN      UINT64                    Arg1,
  IN      UINT64                    Arg2,
  IN      UINT64                    Arg3,
  IN      UINT64                    Arg4
  );

VOID
EFIAPI
InvalidateInstructionCache (
  VOID
  )
{
  CallPalProcStatic (1, 1, 1, 0);
}

VOID
EFIAPI
WriteBackInvalidateDataCache (
  VOID
  )
{
  CallPalProcStatic (1, 2, 1, 0);
}

VOID *
EFIAPI
WriteBackInvalidateDataCacheRange (
  IN      VOID                      *Address,
  IN      UINTN                     Length
  )
{
  WriteBackInvalidateDataCache ();
  return Address;
}

VOID
EFIAPI
WriteBackDataCache (
  VOID
  )
{
  CallPalProcStatic (1, 2, 0, 0);
}

VOID *
EFIAPI
WriteBackDataCacheRange (
  IN      VOID                      *Address,
  IN      UINTN                     Length
  )
{
  WriteBackDataCache ();
  return Address;
}

VOID
EFIAPI
InvalidateDataCache (
  VOID
  )
{
}

VOID *
EFIAPI
InvalidateDataCacheRange (
  IN      VOID                      *Address,
  IN      UINTN                     Length
  )
{
  return Address;
}
