/** @file
  Header file for the ISA BUS driver.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef _ISA_BUS_H_
#define _ISA_BUS_H_

#include <Uefi.h>
#include <Protocol/IsaHc.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/ServiceBinding.h>

typedef struct {
  UINT32                          Signature;
  EFI_SERVICE_BINDING_PROTOCOL    ServiceBinding;
  EFI_ISA_HC_PROTOCOL             *IsaHc;       ///< ISA HC protocol produced by the ISA Host Controller driver
  EFI_HANDLE                      IsaHcHandle;  ///< ISA HC handle created by the ISA Host Controller driver
} ISA_BUS_PRIVATE_DATA;
#define ISA_BUS_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('_', 'i', 's', 'b')
#define ISA_BUS_PRIVATE_DATA_FROM_THIS(a)  CR (a, ISA_BUS_PRIVATE_DATA, ServiceBinding, ISA_BUS_PRIVATE_DATA_SIGNATURE)

typedef struct {
  UINT32     Signature;
  BOOLEAN    InDestroying;                      ///< Flag to avoid DestroyChild() re-entry.
} ISA_BUS_CHILD_PRIVATE_DATA;
#define ISA_BUS_CHILD_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('_', 'i', 's', 'c')

extern EFI_DRIVER_BINDING_PROTOCOL  gIsaBusDriverBinding;

#endif
