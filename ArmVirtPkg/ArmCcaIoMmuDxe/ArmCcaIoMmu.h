/** @file
  The protocol provides support to allocate, free, map and umap a DMA buffer
  for bus master (e.g PciHostBridge). When the execution context is a Realm,
  the DMA operations must be performed on buffers that are shared with the HOST,
  hence the RAMP protocol is used to manage the sharing of the DMA buffers or in
  some cases bounce the buffers.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017, AMD Inc. All rights reserved.<BR>
  (C) Copyright 2017 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2022 - 2023, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ARM_CCA_IOMMU_H_
#define ARM_CCA_IOMMU_H_

#include <Protocol/IoMmu.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/RealmApertureManagementProtocol.h>

/**
  A macro defning the signature for the MAP_INFO structure.
*/
#define MAP_INFO_SIG  SIGNATURE_64 ('M', 'A', 'P', '_', 'I', 'N', 'F', 'O')

/** A structure describing the mapping for the buffers shared with the host.
*/
typedef struct {
  /// Signature.
  UINT64                   Signature;
  /// Linked List node entry.
  LIST_ENTRY               Link;
  /// IoMMU operation.
  EDKII_IOMMU_OPERATION    Operation;
  /// Number of bytes.
  UINTN                    NumberOfBytes;
  /// Number of pages.
  UINTN                    NumberOfPages;
  /// Address of the Host buffer.
  VOID                     *HostAddress;

  /// Address for the Bounce Buffer.
  EFI_PHYSICAL_ADDRESS     BbAddress;
  /// Handle to the Aperture.
  EFI_HANDLE               ApertureRef;
} MAP_INFO;

/**
  Install IOMMU protocol to provide the DMA support for PciHostBridge and
  RAMP.

**/
EFI_STATUS
EFIAPI
ArmCcaInstallIoMmuProtocol (
  VOID
  );

#endif
