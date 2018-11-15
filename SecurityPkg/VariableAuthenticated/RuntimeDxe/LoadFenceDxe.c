/** @file
  Serialize operation on all load-from-memory instructions (DXE version).

Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Variable.h"

/**
  This service is consumed by the variable modules to perform a serializing
  operation on all load-from-memory instructions that were issued prior to the
  call of this function.

**/
VOID
MemoryLoadFence (
  VOID
  )
{
  //
  // Do nothing.
  //
}
