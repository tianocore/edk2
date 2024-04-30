/** @file

  Stateful and implicitly initialized fw_cfg library implementation.

  Copyright (C) 2013 - 2014, Red Hat, Inc.
  Copyright (c) 2011 - 2013, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/FdtClient.h>

#include "QemuFwCfgLibMmioInternal.h"

UINTN  mFwCfgSelectorAddress;
UINTN  mFwCfgDataAddress;
UINTN  mFwCfgDmaAddress;

RETURN_STATUS
EFIAPI
QemuFwCfgInitialize (
  VOID
  )
{
  EFI_STATUS           Status;
  FDT_CLIENT_PROTOCOL  *FdtClient;
  CONST UINT64         *Reg;
  UINT32               RegSize;
  UINTN                AddressCells, SizeCells;
  UINT64               FwCfgSelectorAddress;
  UINT64               FwCfgSelectorSize;
  UINT64               FwCfgDataAddress;
  UINT64               FwCfgDataSize;
  UINT64               FwCfgDmaAddress;
  UINT64               FwCfgDmaSize;

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  ASSERT_EFI_ERROR (Status);

  Status = FdtClient->FindCompatibleNodeReg (
                        FdtClient,
                        "qemu,fw-cfg-mmio",
                        (CONST VOID **)&Reg,
                        &AddressCells,
                        &SizeCells,
                        &RegSize
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "%a: No 'qemu,fw-cfg-mmio' compatible DT node found (Status == %r)\n",
      __func__,
      Status
      ));
    return EFI_SUCCESS;
  }

  ASSERT (AddressCells == 2);
  ASSERT (SizeCells == 2);
  ASSERT (RegSize == 2 * sizeof (UINT64));

  FwCfgDataAddress     = SwapBytes64 (Reg[0]);
  FwCfgDataSize        = 8;
  FwCfgSelectorAddress = FwCfgDataAddress + FwCfgDataSize;
  FwCfgSelectorSize    = 2;

  //
  // The following ASSERT()s express
  //
  //   Address + Size - 1 <= MAX_UINTN
  //
  // for both registers, that is, that the last byte in each MMIO range is
  // expressible as a MAX_UINTN. The form below is mathematically
  // equivalent, and it also prevents any unsigned overflow before the
  // comparison.
  //
  ASSERT (FwCfgSelectorAddress <= MAX_UINTN - FwCfgSelectorSize + 1);
  ASSERT (FwCfgDataAddress     <= MAX_UINTN - FwCfgDataSize     + 1);

  mFwCfgSelectorAddress = FwCfgSelectorAddress;
  mFwCfgDataAddress     = FwCfgDataAddress;

  DEBUG ((
    DEBUG_INFO,
    "Found FwCfg @ 0x%Lx/0x%Lx\n",
    FwCfgSelectorAddress,
    FwCfgDataAddress
    ));

  if (SwapBytes64 (Reg[1]) >= 0x18) {
    FwCfgDmaAddress = FwCfgDataAddress + 0x10;
    FwCfgDmaSize    = 0x08;

    //
    // See explanation above.
    //
    ASSERT (FwCfgDmaAddress <= MAX_UINTN - FwCfgDmaSize + 1);

    DEBUG ((DEBUG_INFO, "Found FwCfg DMA @ 0x%Lx\n", FwCfgDmaAddress));
  } else {
    FwCfgDmaAddress = 0;
  }

  if (QemuFwCfgIsAvailable ()) {
    UINT32  Signature;

    QemuFwCfgSelectItem (QemuFwCfgItemSignature);
    Signature = QemuFwCfgRead32 ();
    if (Signature == SIGNATURE_32 ('Q', 'E', 'M', 'U')) {
      //
      // For DMA support, we require the DTB to advertise the register, and the
      // feature bitmap (which we read without DMA) to confirm the feature.
      //
      if (FwCfgDmaAddress != 0) {
        UINT32  Features;

        QemuFwCfgSelectItem (QemuFwCfgItemInterfaceVersion);
        Features = QemuFwCfgRead32 ();
        if ((Features & FW_CFG_F_DMA) != 0) {
          mFwCfgDmaAddress            = FwCfgDmaAddress;
          InternalQemuFwCfgReadBytes  = DmaReadBytes;
          InternalQemuFwCfgWriteBytes = DmaWriteBytes;
          InternalQemuFwCfgSkipBytes  = DmaSkipBytes;
        }
      }
    } else {
      mFwCfgSelectorAddress = 0;
      mFwCfgDataAddress     = 0;
    }
  }

  return RETURN_SUCCESS;
}
