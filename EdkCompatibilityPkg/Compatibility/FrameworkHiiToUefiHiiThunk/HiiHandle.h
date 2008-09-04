/**@file
  This file is for functins related to assign and free Framework HII handle number.
  
Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HII_HANDLE_H
#define _HII_HANDLE_H

#include <FrameworkDxe.h>
#include <Protocol/FrameworkHii.h>
#include <Library/DebugLib.h>

VOID
InitHiiHandleDatabase (
  VOID
  );

EFI_STATUS
AllocateHiiHandle (
  FRAMEWORK_EFI_HII_HANDLE *Handle
);
  

VOID
FreeHiiHandle (
  FRAMEWORK_EFI_HII_HANDLE Handle
);


#endif

