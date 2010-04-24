/** @file
  This file is for functins related to assign and free Framework HII handle number.
  
Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HII_HANDLE_H_
#define _HII_HANDLE_H_

#include <FrameworkDxe.h>
#include <Protocol/FrameworkHii.h>
#include <Library/DebugLib.h>

/**

  Initialize the Framework Hii Handle database.

**/
VOID
InitHiiHandleDatabase (
  VOID
  );

/**
  Allocate a new Framework HII handle. 

  @param  Handle Returns the new Framework HII Handle assigned.

  @retval EFI_SUCCESS         A new Framework HII Handle is assigned.
  @retval EFI_OUT_OF_RESOURCE The Framework HII Handle database is depleted.

**/
EFI_STATUS
AllocateHiiHandle (
  OUT FRAMEWORK_EFI_HII_HANDLE *Handle
  );
  

/**
  Free Framework HII handle. 

  @param  Handle The Framework HII Handle to be freed.

**/
VOID
FreeHiiHandle (
  IN FRAMEWORK_EFI_HII_HANDLE Handle
  );


#endif

