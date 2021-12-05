/** @file

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/FspCommonLib.h>

/**

  Switch the current stack to the previous saved stack.

  @param[in]  NewStack         The new stack to be switched.

  @return OldStack         After switching to the saved stack,
                           this value will be saved in eax before returning.


**/
UINT32
SwapStack (
  IN  UINT32  NewStack
  )
{
  FSP_GLOBAL_DATA  *FspData;
  UINT32           OldStack;

  FspData            = GetFspGlobalDataPointer ();
  OldStack           = FspData->CoreStack;
  FspData->CoreStack = NewStack;
  return OldStack;
}
