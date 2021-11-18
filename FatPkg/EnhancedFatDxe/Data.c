/** @file
  Global data in the FAT Filesystem driver.

Copyright (c) 2005 - 2013, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Fat.h"

//
// Globals
//
//
// FatFsLock - Global lock for synchronizing all requests.
//
EFI_LOCK  FatFsLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_CALLBACK);

EFI_LOCK  FatTaskLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_NOTIFY);

//
// Filesystem interface functions
//
EFI_FILE_PROTOCOL  FatFileInterface = {
  EFI_FILE_PROTOCOL_REVISION,
  FatOpen,
  FatClose,
  FatDelete,
  FatRead,
  FatWrite,
  FatGetPosition,
  FatSetPosition,
  FatGetInfo,
  FatSetInfo,
  FatFlush,
  FatOpenEx,
  FatReadEx,
  FatWriteEx,
  FatFlushEx
};
