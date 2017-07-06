/** @file

  The protocol provides support to allocate, free, map and umap a DMA buffer for
  bus master (e.g PciHostBridge). When SEV is enabled, the DMA operations must
  be performed on unencrypted buffer hence protocol clear the encryption bit
  from the DMA buffer.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017, AMD Inc. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __AMD_SEV_IOMMU_H_
#define __AMD_SEV_IOMMU_H

#include <Protocol/IoMmu.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemEncryptSevLib.h>

/**
  Install IOMMU protocol to provide the DMA support for PciHostBridge and
  MemEncryptSevLib.

**/
VOID
EFIAPI
AmdSevInstallIoMmuProtocol (
  VOID
  );

#endif
