/** @file
  Global data used in memory service

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeMain.h"


//
// MemoryLock - synchronizes access to the memory map and pool lists
//
EFI_LOCK          gMemoryLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_NOTIFY);

//
// MemoryMap - the current memory map
//
LIST_ENTRY        gMemoryMap  = INITIALIZE_LIST_HEAD_VARIABLE (gMemoryMap);
