/** @file
  DXE capsule process.
  Dummy function for runtime module, because CapsuleDxeRuntime
  does not need call ProcessCapsules().

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/CapsuleLib.h>

/**
  Function indicate the current completion progress of the firmware
  update. Platform may override with own specific progress function.

  @param[in]  Completion  A value between 1 and 100 indicating the current
                          completion progress of the firmware update

  @retval EFI_SUCESS             The capsule update progress was updated.
  @retval EFI_INVALID_PARAMETER  Completion is greater than 100%.
**/
EFI_STATUS
EFIAPI
UpdateImageProgress (
  IN UINTN  Completion
  )
{
  return EFI_SUCCESS;
}

/**

  This routine is called to process capsules.

  Caution: This function may receive untrusted input.

  The capsules reported in EFI_HOB_UEFI_CAPSULE are processed.
  If there is no EFI_HOB_UEFI_CAPSULE, this routine does nothing.

  This routine should be called twice in BDS.
  1) The first call must be before EndOfDxe. The system capsules is processed.
     If device capsule FMP protocols are exposted at this time and device FMP
     capsule has zero EmbeddedDriverCount, the device capsules are processed.
     Each individual capsule result is recorded in capsule record variable.
     System may reset in this function, if reset is required by capsule and
     all capsules are processed.
     If not all capsules are processed, reset will be defered to second call.

  2) The second call must be after EndOfDxe and after ConnectAll, so that all
     device capsule FMP protocols are exposed.
     The system capsules are skipped. If the device capsules are NOT processed
     in first call, they are processed here.
     Each individual capsule result is recorded in capsule record variable.
     System may reset in this function, if reset is required by capsule
     processed in first call and second call.

  @retval EFI_SUCCESS             There is no error when processing capsules.
  @retval EFI_OUT_OF_RESOURCES    No enough resource to process capsules.

**/
EFI_STATUS
EFIAPI
ProcessCapsules (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}
