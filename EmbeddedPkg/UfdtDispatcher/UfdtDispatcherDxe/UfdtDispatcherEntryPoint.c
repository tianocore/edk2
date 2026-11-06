/** @file
  Locate the firmware device tree for the FDT dispatcher.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "UfdtDispatcherInternal.h"

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Guid/FdtHob.h>

/** Locate the firmware DTB and initialize FDT resource discovery.

  @param[in] ImageHandle  Dispatcher image retained for callback lifetime.
  @param[in] SystemTable  Firmware system table; unused.

  @retval EFI_NOT_FOUND         The FDT HOB is absent.
  @retval EFI_COMPROMISED_DATA  Invalid DTB.
  @retval EFI_SUCCESS           Success.
  @retval Other                 Dispatcher initialization failed.
**/
EFI_STATUS
EFIAPI
UfdtDispatcherEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  VOID  *Hob;

  Hob = GetFirstGuidHob (&gFdtHobGuid);
  if (Hob == NULL) {
    return EFI_NOT_FOUND;
  }

  if (GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (UINT64)) {
    return EFI_COMPROMISED_DATA;
  }

  return UfdtDispatcherInitialize (
           ImageHandle,
           (VOID *)(UINTN)ReadUnaligned64 (GET_GUID_HOB_DATA (Hob))
           );
}
