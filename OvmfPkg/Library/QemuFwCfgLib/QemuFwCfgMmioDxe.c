/** @file

  Stateful and implicitly initialized fw_cfg library implementation.

  Copyright (C) 2013 - 2014, Red Hat, Inc.
  Copyright (c) 2011 - 2013, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/FdtClient.h>

#include "QemuFwCfgLibMmioInternal.h"

STATIC UINTN  mFwCfgSelectorAddress;
STATIC UINTN  mFwCfgDataAddress;
STATIC UINTN  mFwCfgDmaAddress;

/**
  To get firmware configure selector address.

  @param VOID

  @retval  firmware configure selector address
**/
UINTN
EFIAPI
QemuGetFwCfgSelectorAddress (
  VOID
  )
{
  return mFwCfgSelectorAddress;
}

/**
  To get firmware configure Data address.

  @param VOID

  @retval  firmware configure data address
**/
UINTN
EFIAPI
QemuGetFwCfgDataAddress (
  VOID
  )
{
  return mFwCfgDataAddress;
}

/**
  To get firmware DMA address.

  @param VOID

  @retval  firmware DMA address
**/
UINTN
EFIAPI
QemuGetFwCfgDmaAddress (
  VOID
  )
{
  return mFwCfgDmaAddress;
}

RETURN_STATUS
EFIAPI
QemuFwCfgInitialize (
  VOID
  )
{
  EFI_STATUS            Status;
  FDT_CLIENT_PROTOCOL   *FdtClient;
  CONST UINT64          *Reg;
  UINT32                RegSize;
  UINTN                 AddressCells, SizeCells;
  UINT64                FwCfgSelectorAddress;
  UINT64                FwCfgSelectorSize;
  UINT64                FwCfgDataAddress;
  UINT64                FwCfgDataSize;
  UINT64                FwCfgDmaAddress;
  UINT64                FwCfgDmaSize;
  QEMU_FW_CFG_RESOURCE  *FwCfgResource;

  //
  // Check whether the Qemu firmware configure resources HOB has been created,
  // if so use the resources in the HOB.
  //
  FwCfgResource = QemuGetFwCfgResourceHob ();
  if (FwCfgResource != NULL) {
    mFwCfgSelectorAddress = FwCfgResource->FwCfgSelectorAddress;
    mFwCfgDataAddress     = FwCfgResource->FwCfgDataAddress;
    mFwCfgDmaAddress      = FwCfgResource->FwCfgDmaAddress;

    if (mFwCfgDmaAddress != 0) {
      InternalQemuFwCfgReadBytes  = DmaReadBytes;
      InternalQemuFwCfgWriteBytes = DmaWriteBytes;
      InternalQemuFwCfgSkipBytes  = DmaSkipBytes;
    }

    return RETURN_SUCCESS;
  }

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
