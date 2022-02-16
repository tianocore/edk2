/** @file
  Initialize Intel TDX support.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

/**
  In Tdx guest, some information need to be passed from host VMM to guest
  firmware. For example, the memory resource, etc. These information are
  prepared by host VMM and put in HobList which is described in TdxMetadata.

  Information in HobList is treated as external input. From the security
  perspective before it is consumed, it should be validated.

  @retval   EFI_SUCCESS   Successfully process the hoblist
  @retval   Others        Other error as indicated
**/
EFI_STATUS
EFIAPI
ProcessTdxHobList (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

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
