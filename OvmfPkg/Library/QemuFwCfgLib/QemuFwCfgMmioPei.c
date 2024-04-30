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
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/QemuFwCfgLib.h>

#include <libfdt.h>

#include "QemuFwCfgLibMmioInternal.h"

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
  QEMU_FW_CFG_RESOURCE  *FwCfgResource;

  FwCfgResource = QemuGetFwCfgResourceHob ();
  ASSERT (FwCfgResource != NULL);

  return FwCfgResource->FwCfgSelectorAddress;
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
  QEMU_FW_CFG_RESOURCE  *FwCfgResource;

  FwCfgResource = QemuGetFwCfgResourceHob ();
  ASSERT (FwCfgResource != NULL);

  return FwCfgResource->FwCfgDataAddress;
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
  QEMU_FW_CFG_RESOURCE  *FwCfgResource;

  FwCfgResource = QemuGetFwCfgResourceHob ();
  ASSERT (FwCfgResource != NULL);

  return FwCfgResource->FwCfgDmaAddress;
}

RETURN_STATUS
EFIAPI
QemuFwCfgInitialize (
  VOID
  )
{
  VOID                  *DeviceTreeBase;
  INT32                 Node;
  INT32                 Prev;
  UINT32                Signature;
  CONST CHAR8           *Type;
  INT32                 Len;
  CONST UINT64          *Reg;
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
    return RETURN_SUCCESS;
  }

  DeviceTreeBase = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);
  ASSERT (DeviceTreeBase != NULL);
  //
  // Make sure we have a valid device tree blob
  //
  ASSERT (fdt_check_header (DeviceTreeBase) == 0);

  //
  // Create resouce memory
  //
  FwCfgResource = AllocateZeroPool (sizeof (QEMU_FW_CFG_RESOURCE));
  ASSERT (FwCfgResource != NULL);

  for (Prev = 0; ; Prev = Node) {
    Node = fdt_next_node (DeviceTreeBase, Prev, NULL);
    if (Node < 0) {
      break;
    }

    //
    // Check for memory node
    //
    Type = fdt_getprop (DeviceTreeBase, Node, "compatible", &Len);
    if ((Type != NULL) &&
        (AsciiStrnCmp (Type, "qemu,fw-cfg-mmio", Len) == 0))
    {
      //
      // Get the 'reg' property of this node. For now, we will assume
      // two 8 byte quantities for base and size, respectively.
      //
      Reg = fdt_getprop (DeviceTreeBase, Node, "reg", &Len);
      if ((Reg != 0) && (Len == (2 * sizeof (UINT64)))) {
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

        FwCfgResource->FwCfgSelectorAddress = FwCfgSelectorAddress;
        FwCfgResource->FwCfgDataAddress     = FwCfgDataAddress;

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
          FwCfgResource->FwCfgDmaAddress = FwCfgDmaAddress;
        } else {
          FwCfgDmaAddress = 0;
        }

        if ((FwCfgSelectorAddress != 0) && (FwCfgDataAddress != 0)) {
          //
          // Select Item Signature
          //
          MmioWrite16 (FwCfgSelectorAddress, SwapBytes16 ((UINT16)QemuFwCfgItemSignature));

          //
          // Readout the Signature.
          //
          Signature = MmioRead32 (FwCfgDataAddress);

          if (Signature == SIGNATURE_32 ('Q', 'E', 'M', 'U')) {
            //
            // Build the firmware configure resource HOB.
            //
            QemuBuildFwCfgResourceHob (FwCfgResource);
          } else {
            FwCfgResource->FwCfgDataAddress     = 0;
            FwCfgResource->FwCfgSelectorAddress = 0;
            FwCfgResource->FwCfgDmaAddress      = 0;

            DEBUG ((
              DEBUG_ERROR,
              "%a: Signature dose not match QEMU!\n",
              __func__
              ));
            break;
          }
        }

        break;
      } else {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to parse FDT QemuCfg node\n",
          __func__
          ));
        break;
      }
    }
  }

  return RETURN_SUCCESS;
}
