/** @file
  FDT client library for consumers of PCI related dynamic PCDs

  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/FdtClient.h>

RETURN_STATUS
EFIAPI
FdtPciPcdProducerLibConstructor (
  VOID
  )
{
  UINT64              PciExpressBaseAddress;
  FDT_CLIENT_PROTOCOL *FdtClient;
  CONST UINT64        *Reg;
  UINT32              RegElemSize, RegSize;
  EFI_STATUS          Status;

  PciExpressBaseAddress = PcdGet64 (PcdPciExpressBaseAddress);
  if (PciExpressBaseAddress != MAX_UINT64) {
    return EFI_SUCCESS;
  }

  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL,
                  (VOID **)&FdtClient);
  ASSERT_EFI_ERROR (Status);

  Status = FdtClient->FindCompatibleNodeReg (FdtClient,
                        "pci-host-ecam-generic", (CONST VOID **)&Reg,
                        &RegElemSize, &RegSize);

  if (EFI_ERROR (Status)) {
    PciExpressBaseAddress = 0;
  } else {
    ASSERT (RegElemSize == sizeof (UINT64));
    PciExpressBaseAddress = SwapBytes64 (*Reg);

    PcdSetBool (PcdPciDisableBusEnumeration, FALSE);
  }

  PcdSet64 (PcdPciExpressBaseAddress, PciExpressBaseAddress);

  return RETURN_SUCCESS;
}
