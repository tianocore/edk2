/** @file
  MMIO mapping helper for UFDT resource consumer constructors.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>

/**
  Map the selected resource's MMIO regions, if a mapping callback is present.

  The calling library's DEPEX must require ResourceGuid before its constructor
  calls this function. The resource is a singleton platform selection.
  Call only while boot services are available.

  @param[in] ResourceGuid  Protocol identifying the selected resource.

  @retval EFI_SUCCESS  The regions were mapped or no mapping was required.
  @return              An error returned by LocateProtocol() or MapMmio().
**/
EFI_STATUS
EFIAPI
UfdtDepexMapMmio (
  IN EFI_GUID  *ResourceGuid
  );
