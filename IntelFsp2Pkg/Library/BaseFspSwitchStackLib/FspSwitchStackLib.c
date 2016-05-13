/** @file

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  IN  UINT32 NewStack
  )
{
  FSP_GLOBAL_DATA  *FspData;
  UINT32         OldStack;

  FspData  = GetFspGlobalDataPointer ();
  OldStack = FspData->CoreStack;
  FspData->CoreStack = NewStack;
  return OldStack;
}

