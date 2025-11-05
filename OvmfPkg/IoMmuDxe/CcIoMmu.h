/** @file

  The protocol provides support to allocate, free, map and umap a DMA buffer
  for bus master (e.g PciHostBridge). When SEV is enabled, the DMA operations
  must be performed on unencrypted buffer hence protocol clear the encryption
  bit from the DMA buffer.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017, AMD Inc. All rights reserved.<BR>
  (C) Copyright 2017 Hewlett Packard Enterprise Development LP<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _AMD_SEV_IOMMU_H_
#define _AMD_SEV_IOMMU_H_

#include <Protocol/IoMmu.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/MemEncryptTdxLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  Install IOMMU protocol to provide the DMA support for PciHostBridge.

**/
EFI_STATUS
EFIAPI
InstallIoMmuProtocol (
  VOID
  );

#endif
