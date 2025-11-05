/** @file
  Capsule Runtime Driver produces two UEFI capsule runtime services.
  (UpdateCapsule, QueryCapsuleCapabilities)
  It installs the Capsule Architectural Protocol defined in PI1.0a to signify
  the capsule runtime services are ready.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2018, Linaro, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CAPSULE_SERVICE_H_
#define _CAPSULE_SERVICE_H_

#include <Uefi.h>

#include <Protocol/Capsule.h>
#include <Guid/CapsuleVendor.h>
#include <Guid/FmpCapsule.h>

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/CapsuleLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>

/**
  Create the variable to save the base address of page table and stack
  for transferring into long mode in IA32 PEI.
**/
VOID
SaveLongModeContext (
  VOID
  );

/**
  Whether the platform supports capsules that persist across reset. Note that
  some platforms only support such capsules at boot time.

  @return TRUE  if a PersistAcrossReset capsule may be passed to UpdateCapsule()
                at this time
          FALSE otherwise
**/
BOOLEAN
IsPersistAcrossResetCapsuleSupported (
  VOID
  );

/**
  Writes Back a range of data cache lines covering a set of capsules in memory.

  Writes Back the data cache lines specified by ScatterGatherList.

  @param  ScatterGatherList Physical address of the data structure that
                            describes a set of capsules in memory

**/
VOID
CapsuleCacheWriteBack (
  IN  EFI_PHYSICAL_ADDRESS  ScatterGatherList
  );

#endif
