/** @file

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DmaProtection.h"

UINT64                           mVtdHostAddressWidthMask;
UINTN                            mVtdUnitNumber;
VTD_UNIT_INFORMATION             *mVtdUnitInformation;

BOOLEAN  mVtdEnabled;

/**
  Invalid VTd global IOTLB.

  @param[in]  VtdIndex              The index of VTd engine.

  @retval EFI_SUCCESS           VTd global IOTLB is invalidated.
  @retval EFI_DEVICE_ERROR      VTd global IOTLB is not invalidated.
**/
EFI_STATUS
InvalidateVtdIOTLBGlobal (
  IN UINTN  VtdIndex
  )
{
  UINT64  Reg64;
  UINT32  Reg32;

  if (!mVtdEnabled) {
    return EFI_SUCCESS;
  }

  DEBUG((DEBUG_VERBOSE, "InvalidateVtdIOTLBGlobal(%d)\n", VtdIndex));

  AsmWbinvd();

  //
  // Write Buffer Flush before invalidation
  //
  Reg32 = MmioRead32 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_CAP_REG);
  if ((Reg32 & B_CAP_REG_RWBF) != 0) {
    MmioWrite32 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_GCMD_REG, B_GMCD_REG_WBF);
  }

  //
  // Invalidate the context cache
  //
  Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_CCMD_REG);
  if ((Reg64 & B_CCMD_REG_ICC) != 0) {
    DEBUG ((DEBUG_ERROR,"ERROR: InvalidateVtdIOTLBGlobal: B_CCMD_REG_ICC is set for VTD(%d)\n",VtdIndex));
    return EFI_DEVICE_ERROR;
  }

  Reg64 &= ((~B_CCMD_REG_ICC) & (~B_CCMD_REG_CIRG_MASK));
  Reg64 |= (B_CCMD_REG_ICC | V_CCMD_REG_CIRG_GLOBAL);
  MmioWrite64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_CCMD_REG, Reg64);

  do {
    Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_CCMD_REG);
  } while ((Reg64 & B_CCMD_REG_ICC) != 0);

  //
  // Invalidate the IOTLB cache
  //

  Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + (mVtdUnitInformation[VtdIndex].ECapReg.Bits.IRO * 16) + R_IOTLB_REG);
  if ((Reg64 & B_IOTLB_REG_IVT) != 0) {
    DEBUG ((DEBUG_ERROR,"ERROR: InvalidateVtdIOTLBGlobal: B_IOTLB_REG_IVT is set for VTD(%d)\n", VtdIndex));
    return EFI_DEVICE_ERROR;
  }

  Reg64 &= ((~B_IOTLB_REG_IVT) & (~B_IOTLB_REG_IIRG_MASK));
  Reg64 |= (B_IOTLB_REG_IVT | V_IOTLB_REG_IIRG_GLOBAL);
  MmioWrite64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + (mVtdUnitInformation[VtdIndex].ECapReg.Bits.IRO * 16) + R_IOTLB_REG, Reg64);

  do {
    Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + (mVtdUnitInformation[VtdIndex].ECapReg.Bits.IRO * 16) + R_IOTLB_REG);
  } while ((Reg64 & B_IOTLB_REG_IVT) != 0);

  //
  // Disable VTd
  //
  MmioWrite32 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_GCMD_REG, B_GMCD_REG_SRTP);
  do {
    Reg32 = MmioRead32 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_GSTS_REG);
  } while((Reg32 & B_GSTS_REG_RTPS) == 0);

  //
  // Enable VTd
  //
  MmioWrite32 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_GCMD_REG, B_GMCD_REG_TE);
  do {
    Reg32 = MmioRead32 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_GSTS_REG);
  } while ((Reg32 & B_GSTS_REG_TE) == 0);

  return EFI_SUCCESS;
}

/**
  Invalid VTd IOTLB domain.

  @param[in]  VtdIndex              The index of VTd engine.
  @param[in]  DomainIdentifier      The domain ID of the source.

  @retval EFI_SUCCESS           VTd IOTLB domain is invalidated.
  @retval EFI_DEVICE_ERROR      VTd IOTLB domain is not invalidated.
**/
EFI_STATUS
InvalidateVtdIOTLBDomain (
  IN UINTN  VtdIndex,
  IN UINT16 DomainIdentifier
  )
{
  UINT64  Reg64;

  if (!mVtdEnabled) {
    return EFI_SUCCESS;
  }

  DEBUG((DEBUG_VERBOSE, "InvalidateVtdIOTLBDomain(%d): 0x%016lx (0x%04x)\n", VtdIndex, DomainIdentifier));

  //
  // Invalidate the context cache
  //
  Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_CCMD_REG);
  if ((Reg64 & B_CCMD_REG_ICC) != 0) {
    DEBUG ((DEBUG_ERROR,"ERROR: InvalidateVtdIOTLBDomain: B_CCMD_REG_ICC is set for VTD(%d)\n",VtdIndex));
    return EFI_DEVICE_ERROR;
  }

  Reg64 &= ((~B_CCMD_REG_ICC) & (~B_CCMD_REG_CIRG_MASK));
  Reg64 |= (B_CCMD_REG_ICC | V_CCMD_REG_CIRG_DOMAIN);
  Reg64 |= (B_CCMD_REG_ICC | V_CCMD_REG_CIRG_DOMAIN);
  MmioWrite64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_CCMD_REG, Reg64);

  do {
    Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_CCMD_REG);
  } while ((Reg64 & B_CCMD_REG_ICC) != 0);

  //
  // Invalidate the IOTLB cache
  //

  Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + (mVtdUnitInformation[VtdIndex].ECapReg.Bits.IRO * 16) + R_IOTLB_REG);
  if ((Reg64 & B_IOTLB_REG_IVT) != 0) {
    DEBUG ((DEBUG_ERROR,"ERROR: InvalidateVtdIOTLBDomain: B_IOTLB_REG_IVT is set for VTD(%d)\n", VtdIndex));
    return EFI_DEVICE_ERROR;
  }

  Reg64 &= ((~B_IOTLB_REG_IVT) & (~B_IOTLB_REG_IIRG_MASK));
  Reg64 |= (B_IOTLB_REG_IVT | V_IOTLB_REG_IIRG_DOMAIN);
  Reg64 |= DomainIdentifier;
  MmioWrite64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + (mVtdUnitInformation[VtdIndex].ECapReg.Bits.IRO * 16) + R_IOTLB_REG, Reg64);

  do {
    Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + (mVtdUnitInformation[VtdIndex].ECapReg.Bits.IRO * 16) + R_IOTLB_REG);
  } while ((Reg64 & B_IOTLB_REG_IVT) != 0);

  return EFI_SUCCESS;
}

/**
  Invalid VTd IOTLB page.

  @param[in]  VtdIndex              The index of VTd engine.
  @param[in]  Address               The address of IOTLB page.
  @param[in]  AddressMode           The address mode of IOTLB page.
  @param[in]  DomainIdentifier      The domain ID of the source.

  @retval EFI_SUCCESS           VTd IOTLB page is invalidated.
  @retval EFI_DEVICE_ERROR      VTd IOTLB page is not invalidated.
**/
EFI_STATUS
InvalidateVtdIOTLBPage (
  IN UINTN  VtdIndex,
  IN UINT64 Address,
  IN UINT8  AddressMode,
  IN UINT16 DomainIdentifier
  )
{
  UINT64  Reg64;
  UINT64  Data64;

  if (!mVtdEnabled) {
    return EFI_SUCCESS;
  }

  DEBUG((DEBUG_VERBOSE, "InvalidateVtdIOTLBPage(%d): 0x%016lx (0x%02x)\n", VtdIndex, Address, AddressMode));

  if (mVtdUnitInformation[VtdIndex].CapReg.Bits.PSI != 0) {
    Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + (mVtdUnitInformation[VtdIndex].ECapReg.Bits.IRO * 16) + R_IOTLB_REG);
    if ((Reg64 & B_IOTLB_REG_IVT) != 0) {
      DEBUG ((DEBUG_ERROR,"ERROR: InvalidateVtdIOTLBPage: B_IOTLB_REG_IVT is set for VTD(%d)\n", VtdIndex));
      return EFI_DEVICE_ERROR;
    }

    Data64 = Address | AddressMode;
    MmioWrite64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + (mVtdUnitInformation[VtdIndex].ECapReg.Bits.IRO * 16) + R_IVA_REG, Data64);

    Reg64 &= ((~B_IOTLB_REG_IVT) & (~B_IOTLB_REG_IIRG_MASK));
    Reg64 |= (B_IOTLB_REG_IVT | V_IOTLB_REG_IIRG_PAGE);
    Reg64 |= LShiftU64 (DomainIdentifier, 32);
    MmioWrite64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + (mVtdUnitInformation[VtdIndex].ECapReg.Bits.IRO * 16) + R_IOTLB_REG, Reg64);

    do {
      Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + (mVtdUnitInformation[VtdIndex].ECapReg.Bits.IRO * 16) + R_IOTLB_REG);
    } while ((Reg64 & B_IOTLB_REG_IVT) != 0);
  } else {
    InvalidateVtdIOTLBGlobal (VtdIndex);
  }

  return EFI_SUCCESS;
}

/**
  Prepare VTD configuration.
**/
VOID
PrepareVtdConfig (
  VOID
  )
{
  UINTN         Index;
  UINTN         DomainNumber;

  for (Index = 0; Index < mVtdUnitNumber; Index++) {
    DEBUG ((DEBUG_INFO, "Dump VTd Capability (%d)\n", Index));
    mVtdUnitInformation[Index].CapReg.Uint64 = MmioRead64 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_CAP_REG);
    DumpVtdCapRegs (&mVtdUnitInformation[Index].CapReg);
    mVtdUnitInformation[Index].ECapReg.Uint64 = MmioRead64 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_ECAP_REG);
    DumpVtdECapRegs (&mVtdUnitInformation[Index].ECapReg);

    if ((mVtdUnitInformation[Index].CapReg.Bits.SLLPS & BIT0) == 0) {
      DEBUG((DEBUG_WARN, "!!!! 2MB super page is not supported on VTD %d !!!!\n", Index));
    }
    if ((mVtdUnitInformation[Index].CapReg.Bits.SAGAW & BIT2) == 0) {
      DEBUG((DEBUG_ERROR, "!!!! 4-level page-table is not supported on VTD %d !!!!\n", Index));
      return ;
    }

    DomainNumber = (UINTN)1 << (UINT8)((UINTN)mVtdUnitInformation[Index].CapReg.Bits.ND * 2 + 4);
    if (mVtdUnitInformation[Index].PciDeviceInfo.PciDescriptorNumber >= DomainNumber) {
      DEBUG((DEBUG_ERROR, "!!!! Pci device Number(0x%x) >= DomainNumber(0x%x) !!!!\n", mVtdUnitInformation[Index].PciDeviceInfo.PciDescriptorNumber, DomainNumber));
      return ;
    }
  }
  return ;
}

/**
  Enable DMAR translation.

  @retval EFI_SUCCESS           DMAR translation is enabled.
  @retval EFI_DEVICE_ERROR      DMAR translation is not enabled.
**/
EFI_STATUS
EnableDmar (
  VOID
  )
{
  UINTN     Index;
  UINT64    Reg64;
  UINT32    Reg32;

  AsmWbinvd();

  for (Index = 0; Index < mVtdUnitNumber; Index++) {
    DEBUG((DEBUG_INFO, ">>>>>>EnableDmar() for engine [%d] \n", Index));

    if (mVtdUnitInformation[Index].ExtRootEntryTable != NULL) {
      DEBUG((DEBUG_INFO, "ExtRootEntryTable 0x%x \n", mVtdUnitInformation[Index].ExtRootEntryTable));
      MmioWrite64 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_RTADDR_REG, (UINT64)(UINTN)mVtdUnitInformation[Index].ExtRootEntryTable | BIT11);
    } else {
      DEBUG((DEBUG_INFO, "RootEntryTable 0x%x \n", mVtdUnitInformation[Index].RootEntryTable));
      MmioWrite64 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_RTADDR_REG, (UINT64)(UINTN)mVtdUnitInformation[Index].RootEntryTable);
    }

    MmioWrite32 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_GCMD_REG, B_GMCD_REG_SRTP);

    DEBUG((DEBUG_INFO, "EnableDmar: waiting for RTPS bit to be set... \n"));
    do {
      Reg32 = MmioRead32 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_GSTS_REG);
    } while((Reg32 & B_GSTS_REG_RTPS) == 0);

    //
    // Init DMAr Fault Event and Data registers
    //
    Reg32 = MmioRead32 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_FEDATA_REG);

    //
    // Write Buffer Flush before invalidation
    //
    Reg32 = MmioRead32 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_CAP_REG);
    if ((Reg32 & B_CAP_REG_RWBF) != 0) {
      MmioWrite32 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_GCMD_REG, B_GMCD_REG_WBF);
    }

    //
    // Invalidate the context cache
    //
    Reg64 = MmioRead64 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_CCMD_REG);
    if ((Reg64 & B_CCMD_REG_ICC) != 0) {
      DEBUG ((DEBUG_INFO,"ERROR: EnableDmar: B_CCMD_REG_ICC is set for VTD(%d)\n",Index));
      return EFI_DEVICE_ERROR;
    }

    Reg64 &= ((~B_CCMD_REG_ICC) & (~B_CCMD_REG_CIRG_MASK));
    Reg64 |= (B_CCMD_REG_ICC | V_CCMD_REG_CIRG_GLOBAL);
    MmioWrite64 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_CCMD_REG, Reg64);

    DEBUG((DEBUG_INFO, "EnableDmar: Waiting B_CCMD_REG_ICC ...\n"));
    do {
      Reg64 = MmioRead64 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_CCMD_REG);
    } while ((Reg64 & B_CCMD_REG_ICC) != 0);

    //
    // Invalidate the IOTLB cache
    //
    DEBUG((DEBUG_INFO, "EnableDmar: IRO 0x%x\n", mVtdUnitInformation[Index].ECapReg.Bits.IRO));

    Reg64 = MmioRead64 (mVtdUnitInformation[Index].VtdUnitBaseAddress + (mVtdUnitInformation[Index].ECapReg.Bits.IRO * 16) + R_IOTLB_REG);
    if ((Reg64 & B_IOTLB_REG_IVT) != 0) {
      DEBUG ((DEBUG_INFO,"ERROR: EnableDmar: B_IOTLB_REG_IVT is set for VTD(%d)\n", Index));
      return EFI_DEVICE_ERROR;
    }

    Reg64 &= ((~B_IOTLB_REG_IVT) & (~B_IOTLB_REG_IIRG_MASK));
    Reg64 |= (B_IOTLB_REG_IVT | V_IOTLB_REG_IIRG_GLOBAL);
    MmioWrite64 (mVtdUnitInformation[Index].VtdUnitBaseAddress + (mVtdUnitInformation[Index].ECapReg.Bits.IRO * 16) + R_IOTLB_REG, Reg64);

    DEBUG((DEBUG_INFO, "EnableDmar: Waiting B_IOTLB_REG_IVT ...\n"));
    do {
      Reg64 = MmioRead64 (mVtdUnitInformation[Index].VtdUnitBaseAddress + (mVtdUnitInformation[Index].ECapReg.Bits.IRO * 16) + R_IOTLB_REG);
    } while ((Reg64 & B_IOTLB_REG_IVT) != 0);

    //
    // Enable VTd
    //
    MmioWrite32 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_GCMD_REG, B_GMCD_REG_TE);
    DEBUG((DEBUG_INFO, "EnableDmar: Waiting B_GSTS_REG_TE ...\n"));
    do {
      Reg32 = MmioRead32 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_GSTS_REG);
    } while ((Reg32 & B_GSTS_REG_TE) == 0);

    DEBUG ((DEBUG_INFO,"VTD (%d) enabled!<<<<<<\n",Index));
  }

  mVtdEnabled = TRUE;

  return EFI_SUCCESS;
}

/**
  Disable DMAR translation.

  @retval EFI_SUCCESS           DMAR translation is disabled.
  @retval EFI_DEVICE_ERROR      DMAR translation is not disabled.
**/
EFI_STATUS
DisableDmar (
  VOID
  )
{
  UINTN     Index;
  UINT32    Reg32;

  AsmWbinvd();

  for (Index = 0; Index < mVtdUnitNumber; Index++) {
    DEBUG((DEBUG_INFO, ">>>>>>DisableDmar() for engine [%d] \n", Index));

    //
    // Write Buffer Flush before invalidation
    //
    Reg32 = MmioRead32 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_CAP_REG);
    if ((Reg32 & B_CAP_REG_RWBF) != 0) {
      MmioWrite32 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_GCMD_REG, B_GMCD_REG_WBF);
    }

    //
    // Disable VTd
    //
    MmioWrite32 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_GCMD_REG, B_GMCD_REG_SRTP);
    do {
      Reg32 = MmioRead32 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_GSTS_REG);
    } while((Reg32 & B_GSTS_REG_RTPS) == 0);

    Reg32 = MmioRead32 (mVtdUnitInformation[Index].VtdUnitBaseAddress + R_GSTS_REG);
    DEBUG((DEBUG_INFO, "DisableDmar: GSTS_REG - 0x%08x\n", Reg32));

    DEBUG ((DEBUG_INFO,"VTD (%d) Disabled!<<<<<<\n",Index));
  }

  mVtdEnabled = FALSE;

  return EFI_SUCCESS;
}

/**
  Dump VTd capability registers.

  @param[in]  CapReg              The capability register.
**/
VOID
DumpVtdCapRegs (
  IN VTD_CAP_REG *CapReg
  )
{
  DEBUG((DEBUG_INFO, "  CapReg:\n", CapReg->Uint64));
  DEBUG((DEBUG_INFO, "    ND     - 0x%x\n", CapReg->Bits.ND));
  DEBUG((DEBUG_INFO, "    AFL    - 0x%x\n", CapReg->Bits.AFL));
  DEBUG((DEBUG_INFO, "    RWBF   - 0x%x\n", CapReg->Bits.RWBF));
  DEBUG((DEBUG_INFO, "    PLMR   - 0x%x\n", CapReg->Bits.PLMR));
  DEBUG((DEBUG_INFO, "    PHMR   - 0x%x\n", CapReg->Bits.PHMR));
  DEBUG((DEBUG_INFO, "    CM     - 0x%x\n", CapReg->Bits.CM));
  DEBUG((DEBUG_INFO, "    SAGAW  - 0x%x\n", CapReg->Bits.SAGAW));
  DEBUG((DEBUG_INFO, "    MGAW   - 0x%x\n", CapReg->Bits.MGAW));
  DEBUG((DEBUG_INFO, "    ZLR    - 0x%x\n", CapReg->Bits.ZLR));
  DEBUG((DEBUG_INFO, "    FRO    - 0x%x\n", CapReg->Bits.FRO));
  DEBUG((DEBUG_INFO, "    SLLPS  - 0x%x\n", CapReg->Bits.SLLPS));
  DEBUG((DEBUG_INFO, "    PSI    - 0x%x\n", CapReg->Bits.PSI));
  DEBUG((DEBUG_INFO, "    NFR    - 0x%x\n", CapReg->Bits.NFR));
  DEBUG((DEBUG_INFO, "    MAMV   - 0x%x\n", CapReg->Bits.MAMV));
  DEBUG((DEBUG_INFO, "    DWD    - 0x%x\n", CapReg->Bits.DWD));
  DEBUG((DEBUG_INFO, "    DRD    - 0x%x\n", CapReg->Bits.DRD));
  DEBUG((DEBUG_INFO, "    FL1GP  - 0x%x\n", CapReg->Bits.FL1GP));
  DEBUG((DEBUG_INFO, "    PI     - 0x%x\n", CapReg->Bits.PI));
}

/**
  Dump VTd extended capability registers.

  @param[in]  ECapReg              The extended capability register.
**/
VOID
DumpVtdECapRegs (
  IN VTD_ECAP_REG *ECapReg
  )
{
  DEBUG((DEBUG_INFO, "  ECapReg:\n", ECapReg->Uint64));
  DEBUG((DEBUG_INFO, "    C      - 0x%x\n", ECapReg->Bits.C));
  DEBUG((DEBUG_INFO, "    QI     - 0x%x\n", ECapReg->Bits.QI));
  DEBUG((DEBUG_INFO, "    DT     - 0x%x\n", ECapReg->Bits.DT));
  DEBUG((DEBUG_INFO, "    IR     - 0x%x\n", ECapReg->Bits.IR));
  DEBUG((DEBUG_INFO, "    EIM    - 0x%x\n", ECapReg->Bits.EIM));
  DEBUG((DEBUG_INFO, "    PT     - 0x%x\n", ECapReg->Bits.PT));
  DEBUG((DEBUG_INFO, "    SC     - 0x%x\n", ECapReg->Bits.SC));
  DEBUG((DEBUG_INFO, "    IRO    - 0x%x\n", ECapReg->Bits.IRO));
  DEBUG((DEBUG_INFO, "    MHMV   - 0x%x\n", ECapReg->Bits.MHMV));
  DEBUG((DEBUG_INFO, "    ECS    - 0x%x\n", ECapReg->Bits.ECS));
  DEBUG((DEBUG_INFO, "    MTS    - 0x%x\n", ECapReg->Bits.MTS));
  DEBUG((DEBUG_INFO, "    NEST   - 0x%x\n", ECapReg->Bits.NEST));
  DEBUG((DEBUG_INFO, "    DIS    - 0x%x\n", ECapReg->Bits.DIS));
  DEBUG((DEBUG_INFO, "    PASID  - 0x%x\n", ECapReg->Bits.PASID));
  DEBUG((DEBUG_INFO, "    PRS    - 0x%x\n", ECapReg->Bits.PRS));
  DEBUG((DEBUG_INFO, "    ERS    - 0x%x\n", ECapReg->Bits.ERS));
  DEBUG((DEBUG_INFO, "    SRS    - 0x%x\n", ECapReg->Bits.SRS));
  DEBUG((DEBUG_INFO, "    NWFS   - 0x%x\n", ECapReg->Bits.NWFS));
  DEBUG((DEBUG_INFO, "    EAFS   - 0x%x\n", ECapReg->Bits.EAFS));
  DEBUG((DEBUG_INFO, "    PSS    - 0x%x\n", ECapReg->Bits.PSS));
}

/**
  Dump VTd registers.

  @param[in]  VtdIndex              The index of VTd engine.
**/
VOID
DumpVtdRegs (
  IN UINTN  VtdIndex
  )
{
  UINTN         Index;
  UINT64        Reg64;
  VTD_FRCD_REG  FrcdReg;
  VTD_CAP_REG   CapReg;
  UINT32        Reg32;
  VTD_SOURCE_ID SourceId;

  DEBUG((DEBUG_INFO, "#### DumpVtdRegs(%d) Begin ####\n", VtdIndex));

  Reg32 = MmioRead32 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_VER_REG);
  DEBUG((DEBUG_INFO, "  VER_REG     - 0x%08x\n", Reg32));

  CapReg.Uint64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_CAP_REG);
  DEBUG((DEBUG_INFO, "  CAP_REG     - 0x%016lx\n", CapReg.Uint64));

  Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_ECAP_REG);
  DEBUG((DEBUG_INFO, "  ECAP_REG    - 0x%016lx\n", Reg64));

  Reg32 = MmioRead32 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_GSTS_REG);
  DEBUG((DEBUG_INFO, "  GSTS_REG    - 0x%08x \n", Reg32));

  Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_RTADDR_REG);
  DEBUG((DEBUG_INFO, "  RTADDR_REG  - 0x%016lx\n", Reg64));

  Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_CCMD_REG);
  DEBUG((DEBUG_INFO, "  CCMD_REG    - 0x%016lx\n", Reg64));

  Reg32 = MmioRead32 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_FSTS_REG);
  DEBUG((DEBUG_INFO, "  FSTS_REG    - 0x%08x\n", Reg32));

  Reg32 = MmioRead32 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_FECTL_REG);
  DEBUG((DEBUG_INFO, "  FECTL_REG   - 0x%08x\n", Reg32));

  Reg32 = MmioRead32 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_FEDATA_REG);
  DEBUG((DEBUG_INFO, "  FEDATA_REG  - 0x%08x\n", Reg32));

  Reg32 = MmioRead32 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_FEADDR_REG);
  DEBUG((DEBUG_INFO, "  FEADDR_REG  - 0x%08x\n",Reg32));

  Reg32 = MmioRead32 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + R_FEUADDR_REG);
  DEBUG((DEBUG_INFO, "  FEUADDR_REG - 0x%08x\n",Reg32));

  for (Index = 0; Index < (UINTN)CapReg.Bits.NFR + 1; Index++) {
    FrcdReg.Uint64[0] = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + ((CapReg.Bits.FRO * 16) + (Index * 16) + R_FRCD_REG));
    FrcdReg.Uint64[1] = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + ((CapReg.Bits.FRO * 16) + (Index * 16) + R_FRCD_REG + sizeof(UINT64)));
    DEBUG((DEBUG_INFO, "  FRCD_REG[%d] - 0x%016lx %016lx\n", Index, FrcdReg.Uint64[1], FrcdReg.Uint64[0]));
    if (FrcdReg.Uint64[1] != 0 || FrcdReg.Uint64[0] != 0) {
      DEBUG((DEBUG_INFO, "    Fault Info - 0x%016lx\n", LShiftU64(FrcdReg.Bits.FI, 12)));
      SourceId.Uint16 = (UINT16)FrcdReg.Bits.SID;
      DEBUG((DEBUG_INFO, "    Source - B%02x D%02x F%02x\n", SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function));
      DEBUG((DEBUG_INFO, "    Type - %x (%a)\n", FrcdReg.Bits.T, FrcdReg.Bits.T ? "read" : "write"));
      DEBUG((DEBUG_INFO, "    Reason - %x\n", FrcdReg.Bits.FR));
    }
  }

  Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + (mVtdUnitInformation[VtdIndex].ECapReg.Bits.IRO * 16) + R_IVA_REG);
  DEBUG((DEBUG_INFO, "  IVA_REG     - 0x%016lx\n",Reg64));

  Reg64 = MmioRead64 (mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress + (mVtdUnitInformation[VtdIndex].ECapReg.Bits.IRO * 16) + R_IOTLB_REG);
  DEBUG((DEBUG_INFO, "  IOTLB_REG   - 0x%016lx\n",Reg64));

  DEBUG((DEBUG_INFO, "#### DumpVtdRegs(%d) End ####\n", VtdIndex));
}

/**
  Dump VTd registers for all VTd engine.
**/
VOID
DumpVtdRegsAll (
  VOID
  )
{
  UINTN       Num;

  for (Num = 0; Num < mVtdUnitNumber; Num++) {
    DumpVtdRegs (Num);
  }
}

/**
  Dump VTd registers if there is error.
**/
VOID
DumpVtdIfError (
  VOID
  )
{
  UINTN         Num;
  UINTN         Index;
  VTD_FRCD_REG  FrcdReg;
  VTD_CAP_REG   CapReg;
  UINT32        Reg32;
  BOOLEAN       HasError;

  for (Num = 0; Num < mVtdUnitNumber; Num++) {
    HasError = FALSE;
    Reg32 = MmioRead32 (mVtdUnitInformation[Num].VtdUnitBaseAddress + R_FSTS_REG);
    if (Reg32 != 0) {
      HasError = TRUE;
    }
    Reg32 = MmioRead32 (mVtdUnitInformation[Num].VtdUnitBaseAddress + R_FECTL_REG);
    if ((Reg32 & BIT30) != 0) {
      HasError = TRUE;
    }

    CapReg.Uint64 = MmioRead64 (mVtdUnitInformation[Num].VtdUnitBaseAddress + R_CAP_REG);
    for (Index = 0; Index < (UINTN)CapReg.Bits.NFR + 1; Index++) {
      FrcdReg.Uint64[0] = MmioRead64 (mVtdUnitInformation[Num].VtdUnitBaseAddress + ((CapReg.Bits.FRO * 16) + (Index * 16) + R_FRCD_REG));
      FrcdReg.Uint64[1] = MmioRead64 (mVtdUnitInformation[Num].VtdUnitBaseAddress + ((CapReg.Bits.FRO * 16) + (Index * 16) + R_FRCD_REG + sizeof(UINT64)));
      if ((FrcdReg.Uint64[0] != 0) || (FrcdReg.Uint64[1] != 0)) {
        HasError = TRUE;
      }
    }

    if (HasError) {
      DEBUG((DEBUG_INFO, "#### ERROR ####\n"));
      DumpVtdRegs (Num);
      DEBUG((DEBUG_INFO, "#### ERROR ####\n"));
    }
  }
}
