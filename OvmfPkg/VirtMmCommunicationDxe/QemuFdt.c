/** @file

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/QemuUefiVars.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/FdtClient.h>

#include "VirtMmCommunication.h"

/*
  Qemu hooks up the uefi vars communication device like this:

        platform-bus@c000000 {
                interrupt-parent = <0x8002>;
                ranges = <0x00 0x00 0xc000000 0x2000000>;
                #address-cells = <0x01>;
                #size-cells = <0x01>;
                compatible = "qemu,platform", "simple-bus";

                qemu-uefi-vars@0 {
                        reg = <0x00 0x10>;
                        compatible = "qemu,uefi-vars";
                };
        };

  So we have to lookup both platform bus and our device to figure where it
  actually is mapped in mmio space.

  The code assumes the uefi-vars device is connected to the platform bus without
  checking that this is actually the case due to FdtClientDxe API limitations.
*/
STATIC
UINT64
VirtMmGetValue (
  CONST VOID  *Data,
  UINT32      Size,
  UINT32      *Pos,
  UINT32      Cells
  )
{
  UINT32  *Ptr32;
  UINT64  *Ptr64;
  UINT64  Value;

  switch (Cells) {
    case 1:
      ASSERT (*Pos + 4 <= Size);
      Ptr32 = (UINT32 *)(Data + *Pos);
      *Pos += 4;
      Value = SwapBytes32 (*Ptr32);
      break;
    case 2:
      ASSERT (*Pos + 8 <= Size);
      Ptr64 = (UINT64 *)(Data + *Pos);
      *Pos += 8;
      Value = SwapBytes64 (ReadUnaligned64 (Ptr64));
      break;
    default:
      ASSERT (!"unsupported cell size");
      Value = 0;
  }

  return Value;
}

STATIC
EFI_STATUS
EFIAPI
VirtMmGetProp (
  IN  FDT_CLIENT_PROTOCOL  *FdtClient,
  IN  CHAR8                *Compatible,
  IN  CHAR8                *Property,
  OUT CONST VOID           **Values,
  OUT UINT32               *ValSize
  )
{
  EFI_STATUS  Status;

  Status = FdtClient->FindCompatibleNodeProperty (
                        FdtClient,
                        Compatible,
                        Property,
                        Values,
                        ValSize
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: node compatible=\"%a/%a\" not found (%r)\n",
      __func__,
      Compatible,
      Property,
      Status
      ));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_VERBOSE, "%a: %a/%a: ok\n", __func__, Compatible, Property));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtMmHwFind (
  VOID
  )
{
  FDT_CLIENT_PROTOCOL  *FdtClient;
  EFI_STATUS           Status;
  UINT32               Pos;
  CONST VOID           *Ranges;
  UINT32               RangesSize;
  CONST VOID           *Reg;
  UINT32               RegSize;

  UINT64  DevAddr;
  UINT64  DevSize;
  UINT64  BusChildAddr;
  UINT64  BusParentAddr;
  UINT64  BusSize;

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  ASSERT_EFI_ERROR (Status);

  VirtMmGetProp (
    FdtClient,
    "qemu,platform",
    "ranges",
    &Ranges,
    &RangesSize
    );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  VirtMmGetProp (
    FdtClient,
    UEFI_VARS_FDT_COMPAT,
    "reg",
    &Reg,
    &RegSize
    );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  if (RegSize == 2 * sizeof (UINT32)) {
    ASSERT (RangesSize == RegSize + sizeof (UINT64));
    Pos           = 0;
    BusChildAddr  = VirtMmGetValue (Ranges, RangesSize, &Pos, 1);
    BusParentAddr = VirtMmGetValue (Ranges, RangesSize, &Pos, 2);
    BusSize       = VirtMmGetValue (Ranges, RangesSize, &Pos, 1);
    Pos           = 0;
    DevAddr       = VirtMmGetValue (Reg, RegSize, &Pos, 1);
    DevSize       = VirtMmGetValue (Reg, RegSize, &Pos, 1);
  } else {
    DEBUG ((DEBUG_ERROR, "%a: unexpected regsize\n", __func__));
    return RETURN_UNSUPPORTED;
  }

  DEBUG ((DEBUG_VERBOSE, "%a: %a\n", __func__, "qemu,platform"));
  DEBUG ((DEBUG_VERBOSE, "%a:   bus child  %8lx\n", __func__, BusChildAddr));
  DEBUG ((DEBUG_VERBOSE, "%a:   bus parent %8lx\n", __func__, BusParentAddr));
  DEBUG ((DEBUG_VERBOSE, "%a:   bus size   %8lx\n", __func__, BusSize));
  DEBUG ((DEBUG_VERBOSE, "%a: %a\n", __func__, UEFI_VARS_FDT_COMPAT));
  DEBUG ((DEBUG_VERBOSE, "%a:   dev addr   %8lx\n", __func__, DevAddr));
  DEBUG ((DEBUG_VERBOSE, "%a:   dev size   %8lx\n", __func__, DevSize));

  mUefiVarsAddr = DevAddr - BusChildAddr + BusParentAddr;
  DEBUG ((DEBUG_VERBOSE, "%a:   -> mmio at %8lx\n", __func__, mUefiVarsAddr));
  return RETURN_SUCCESS;
}
