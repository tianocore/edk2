/** @file
  
  Template and Sample instance of PalCallLib.
  
  Copyright (c) 2006 - 2008, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Base.h>
#include <Library/PalCallLib.h>
#include <Library/DebugLib.h>

/**
  Makes a PAL procedure call.

  This is a wrapper function to make a PAL procedure call.
  This is just a template as simple instance. It does not
  make real PAL call. It directly reports error if called.

  @param  Index  The PAL procedure Index number.
  @param  Arg2   The 2nd parameter for PAL procedure calls.
  @param  Arg3   The 3rd parameter for PAL procedure calls.
  @param  Arg4   The 4th parameter for PAL procedure calls.

  @return Structure returned from the PAL Call procedure, including the status and return value.

**/
PAL_CALL_RETURN
EFIAPI
PalCall (
  IN UINT64                  Index,
  IN UINT64                  Arg2,
  IN UINT64                  Arg3,
  IN UINT64                  Arg4
  )
{
  PAL_CALL_RETURN Ret;

  Ret.Status = (UINT64) -1;
  ASSERT (!RETURN_ERROR (RETURN_UNSUPPORTED));
  return Ret;
}
