/** @file
  ArmGicArchLib library class implementation for DT based virt platforms

  Copyright (c) 2015 - 2016, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>

#include <Library/ArmGicLib.h>
#include <Library/ArmGicArchLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/FdtClient.h>

STATIC ARM_GIC_ARCH_REVISION        mGicArchRevision;

RETURN_STATUS
EFIAPI
ArmVirtGicArchLibConstructor (
  VOID
  )
{
  UINT32                IccSre;
  FDT_CLIENT_PROTOCOL   *FdtClient;
  CONST UINT64          *Reg;
  UINT32                RegSize;
  UINTN                 AddressCells, SizeCells;
  UINTN                 GicRevision;
  EFI_STATUS            Status;
  UINT64                DistBase, CpuBase, RedistBase;
  RETURN_STATUS         PcdStatus;

  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL,
                  (VOID **)&FdtClient);
  ASSERT_EFI_ERROR (Status);

  GicRevision = 2;
  Status = FdtClient->FindCompatibleNodeReg (FdtClient, "arm,cortex-a15-gic",
                        (CONST VOID **)&Reg, &AddressCells, &SizeCells,
                        &RegSize);
  if (Status == EFI_NOT_FOUND) {
    GicRevision = 3;
    Status = FdtClient->FindCompatibleNodeReg (FdtClient, "arm,gic-v3",
                          (CONST VOID **)&Reg, &AddressCells, &SizeCells,
                          &RegSize);
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }

  switch (GicRevision) {

  case 3:
    //
    // The GIC v3 DT binding describes a series of at least 3 physical (base
    // addresses, size) pairs: the distributor interface (GICD), at least one
    // redistributor region (GICR) containing dedicated redistributor
    // interfaces for all individual CPUs, and the CPU interface (GICC).
    // Under virtualization, we assume that the first redistributor region
    // listed covers the boot CPU. Also, our GICv3 driver only supports the
    // system register CPU interface, so we can safely ignore the MMIO version
    // which is listed after the sequence of redistributor interfaces.
    // This means we are only interested in the first two memory regions
    // supplied, and ignore everything else.
    //
    ASSERT (RegSize >= 32);

    // RegProp[0..1] == { GICD base, GICD size }
    DistBase = SwapBytes64 (Reg[0]);
    ASSERT (DistBase < MAX_UINTN);

    // RegProp[2..3] == { GICR base, GICR size }
    RedistBase = SwapBytes64 (Reg[2]);
    ASSERT (RedistBase < MAX_UINTN);

    PcdStatus = PcdSet64S (PcdGicDistributorBase, DistBase);
    ASSERT_RETURN_ERROR (PcdStatus);
    PcdStatus = PcdSet64S (PcdGicRedistributorsBase, RedistBase);
    ASSERT_RETURN_ERROR (PcdStatus);

    DEBUG ((EFI_D_INFO, "Found GIC v3 (re)distributor @ 0x%Lx (0x%Lx)\n",
      DistBase, RedistBase));

    //
    // The default implementation of ArmGicArchLib is responsible for enabling
    // the system register interface on the GICv3 if one is found. So let's do
    // the same here.
    //
    IccSre = ArmGicV3GetControlSystemRegisterEnable ();
    if (!(IccSre & ICC_SRE_EL2_SRE)) {
      ArmGicV3SetControlSystemRegisterEnable (IccSre | ICC_SRE_EL2_SRE);
      IccSre = ArmGicV3GetControlSystemRegisterEnable ();
    }

    //
    // Unlike the default implementation, there is no fall through to GICv2
    // mode if this GICv3 cannot be driven in native mode due to the fact
    // that the System Register interface is unavailable.
    //
    ASSERT (IccSre & ICC_SRE_EL2_SRE);

    mGicArchRevision = ARM_GIC_ARCH_REVISION_3;
    break;

  case 2:
    //
    // When the GICv2 is emulated with virtualization=on, it adds a virtual
    // set of control registers. This means the register property can be
    // either 32 or 64 bytes in size.
    //
    ASSERT ((RegSize == 32) || (RegSize == 64));

    DistBase = SwapBytes64 (Reg[0]);
    CpuBase  = SwapBytes64 (Reg[2]);
    ASSERT (DistBase < MAX_UINTN);
    ASSERT (CpuBase < MAX_UINTN);

    PcdStatus = PcdSet64S (PcdGicDistributorBase, DistBase);
    ASSERT_RETURN_ERROR (PcdStatus);
    PcdStatus = PcdSet64S (PcdGicInterruptInterfaceBase, CpuBase);
    ASSERT_RETURN_ERROR (PcdStatus);

    DEBUG ((EFI_D_INFO, "Found GIC @ 0x%Lx/0x%Lx\n", DistBase, CpuBase));

    mGicArchRevision = ARM_GIC_ARCH_REVISION_2;
    break;

  default:
    DEBUG ((EFI_D_ERROR, "%a: No GIC revision specified!\n", __FUNCTION__));
    return RETURN_NOT_FOUND;
  }
  return RETURN_SUCCESS;
}

ARM_GIC_ARCH_REVISION
EFIAPI
ArmGicGetSupportedArchRevision (
  VOID
  )
{
  return mGicArchRevision;
}
