/** @file
  Initialize Intel TDX support.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/PlatformInitLib.h>

/**
  In Tdx guest, the system memory is passed in TdHob by host VMM. So
  the major task of PlatformTdxPublishRamRegions is to walk thru the
  TdHob list and transfer the ResourceDescriptorHob and MemoryAllocationHob
  to the hobs in DXE phase.

  MemoryAllocationHob should also be created for Mailbox and Ovmf work area.
**/
VOID
EFIAPI
PlatformTdxPublishRamRegions (
  VOID
  )
{
}

/**
  Find in TdHob and store first address (above 4G) not used
  in PlatformInfoHob->FirstNonAddress.

  @retval EFI_SUCCESS    Successfully found in TdHob.
  @retval EFI_NOT_FOUND  Not found in TdHob.
  @retval Others         Other errors as indicated.
**/
EFI_STATUS
PlatformTdxGetFirstNonAddressFromTdHob (
  IN OUT  EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Find in TdHob and store the low (below 4G) memory in
  PlatformInfoHob->LowMemory.

  @retval EFI_SUCCESS    Successfully found in TdHob.
  @retval EFI_NOT_FOUND  Not found in TdHob.
  @retval Others         Other errors as indicated.
**/
EFI_STATUS
PlatformTdxGetLowMemFromTdHob (
  IN OUT  EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  return EFI_UNSUPPORTED;
}
