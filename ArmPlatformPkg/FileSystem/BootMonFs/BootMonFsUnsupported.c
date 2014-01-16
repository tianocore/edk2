/** @file
*
*  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
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

#include "BootMonFsInternal.h"

EFIAPI
EFI_STATUS
BootMonFsSetPositionUnsupported (
  IN EFI_FILE_PROTOCOL  *This,
  IN UINT64             Position
  )
{
  ASSERT(0);
  return EFI_UNSUPPORTED;
}

EFIAPI
EFI_STATUS
BootMonFsGetPositionUnsupported (
  IN EFI_FILE_PROTOCOL  *This,
  OUT UINT64            *Position
  )
{
  ASSERT(0);
  return EFI_UNSUPPORTED;
}
