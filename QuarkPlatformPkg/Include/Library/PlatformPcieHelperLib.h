/** @file
PlatformPcieHelperLib function prototype definitions.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PLATFORM_PCIE_HELPER_LIB_H__
#define __PLATFORM_PCIE_HELPER_LIB_H__

#include "Platform.h"

//
// Function prototypes for routines exported by this library.
//

/**
  Platform assert PCI express PERST# signal.

  @param   PlatformType     See EFI_PLATFORM_TYPE enum definitions.

**/
VOID
EFIAPI
PlatformPERSTAssert (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  );

/**
  Platform de assert PCI express PERST# signal.

  @param   PlatformType     See EFI_PLATFORM_TYPE enum definitions.

**/
VOID
EFIAPI
PlatformPERSTDeAssert (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  );

/** Early initialisation of the PCIe controller.

  @param   PlatformType     See EFI_PLATFORM_TYPE enum definitions.

  @retval   EFI_SUCCESS               Operation success.

**/
EFI_STATUS
EFIAPI
PlatformPciExpressEarlyInit (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  );

#endif // #ifndef __PLATFORM_PCIE_HELPER_LIB_H__
