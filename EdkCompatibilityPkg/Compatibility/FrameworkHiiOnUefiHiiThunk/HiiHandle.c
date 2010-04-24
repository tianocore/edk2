/** @file
  This file is for functins related to assign and free Framework HII handle number.
  
Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "HiiHandle.h"

//
// FRAMEWORK_EFI_HII_HANDLE
//
UINT8 mHandle[1024 * 8] = {0};

/**

  Initialize the Framework Hii Handle database.

**/
VOID
InitHiiHandleDatabase (
  VOID
  )
{
  //
  // FRAMEWORK_EFI_HII_HANDLE 0 is reserved.
  // Set Bit 0 in mHandle[0] to 1.
  //
  mHandle[0] |= 1 << 0;
}

/**
  Allocate a new Framework HII handle. 

  @param  Handle Returns the new Framework HII Handle assigned.

  @retval EFI_SUCCESS         A new Framework HII Handle is assigned.
  @retval EFI_OUT_OF_RESOURCE The Framework HII Handle database is depleted.

**/
EFI_STATUS
AllocateHiiHandle (
  OUT FRAMEWORK_EFI_HII_HANDLE *Handle
  )
{
  UINTN       Index;

  for (Index = 0; Index < sizeof (mHandle) * 8; Index++) {
    if ((mHandle[Index / 8] & (1 << (Index % 8))) == 0) {
      mHandle[Index / 8] = (UINT8) (mHandle[Index / 8] | (1 << (Index % 8)));
      *Handle = (FRAMEWORK_EFI_HII_HANDLE) Index;
      ASSERT (*Handle != 0);
      return EFI_SUCCESS;
    }
  }
  
  return EFI_OUT_OF_RESOURCES;
}

/**
  Free Framework HII handle. 

  @param  Handle The Framework HII Handle to be freed.

**/
VOID
FreeHiiHandle (
  IN FRAMEWORK_EFI_HII_HANDLE Handle
  )
{
  UINT16 Num;

  Num = (UINT16) Handle;

  ASSERT ((mHandle [Num / 8] & (1 << (Num % 8))) != 0);
  mHandle [Num / 8] = (UINT8) (mHandle [Num / 8] & (~(1 << (Num % 8))));
}
