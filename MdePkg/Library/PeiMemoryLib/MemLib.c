/** @file
  Base Memory Library.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "MemLibInternals.h"

VOID *
EFIAPI
InternalMemCopyMem (
  OUT     VOID                      *Destination,
  IN      CONST VOID                *Source,
  IN      UINTN                     Length
  )
{
  (*GetPeiServicesTablePointer ())->CopyMem (
                                      Destination,
                                      (VOID*)Source,
                                      Length
                                      );
  return Destination;
}

VOID *
EFIAPI
InternalMemSetMem (
  OUT     VOID                      *Buffer,
  IN      UINTN                     Size,
  IN      UINT8                     Value
  )
{
  (*GetPeiServicesTablePointer ())->SetMem (
                                      Buffer,
                                      Size,
                                      Value
                                      );
  return Buffer;
}
