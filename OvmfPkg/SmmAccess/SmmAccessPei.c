/** @file

  A PEIM with the following responsibilities:

  - verify & configure the Q35 TSEG in the entry point,
  - provide SMRAM access by producing PEI_SMM_ACCESS_PPI,
  - set aside the SMM_S3_RESUME_STATE object at the bottom of TSEG, and expose
    it via the gEfiAcpiVariableGuid GUID HOB.

  This PEIM runs from RAM, so we can write to variables with static storage
  duration.

  Copyright (C) 2013, 2015, Red Hat, Inc.<BR>
  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/AcpiS3Context.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/PeiServicesLib.h>
#include <Ppi/SmmAccess.h>

#include <OvmfPlatforms.h>

#include "SmramInternal.h"

//
// PEI_SMM_ACCESS_PPI implementation.
//

/**
  Opens the SMRAM area to be accessible by a PEIM driver.

  This function "opens" SMRAM so that it is visible while not inside of SMM.
  The function should return EFI_UNSUPPORTED if the hardware does not support
  hiding of SMRAM. The function should return EFI_DEVICE_ERROR if the SMRAM
  configuration is locked.

  @param  PeiServices            General purpose services available to every
                                 PEIM.
  @param  This                   The pointer to the SMM Access Interface.
  @param  DescriptorIndex        The region of SMRAM to Open.

  @retval EFI_SUCCESS            The region was successfully opened.
  @retval EFI_DEVICE_ERROR       The region could not be opened because locked
                                 by chipset.
  @retval EFI_INVALID_PARAMETER  The descriptor index was out of bounds.

**/
STATIC
EFI_STATUS
EFIAPI
SmmAccessPeiOpen (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN PEI_SMM_ACCESS_PPI              *This,
  IN UINTN                           DescriptorIndex
  )
{
  if (DescriptorIndex >= DescIdxCount) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // According to current practice, DescriptorIndex is not considered at all,
  // beyond validating it.
  //
  return SmramAccessOpen (&This->LockState, &This->OpenState);
}

/**
  Inhibits access to the SMRAM.

  This function "closes" SMRAM so that it is not visible while outside of SMM.
  The function should return EFI_UNSUPPORTED if the hardware does not support
  hiding of SMRAM.

  @param  PeiServices              General purpose services available to every
                                   PEIM.
  @param  This                     The pointer to the SMM Access Interface.
  @param  DescriptorIndex          The region of SMRAM to Close.

  @retval EFI_SUCCESS              The region was successfully closed.
  @retval EFI_DEVICE_ERROR         The region could not be closed because
                                   locked by chipset.
  @retval EFI_INVALID_PARAMETER    The descriptor index was out of bounds.

**/
STATIC
EFI_STATUS
EFIAPI
SmmAccessPeiClose (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN PEI_SMM_ACCESS_PPI              *This,
  IN UINTN                           DescriptorIndex
  )
{
  if (DescriptorIndex >= DescIdxCount) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // According to current practice, DescriptorIndex is not considered at all,
  // beyond validating it.
  //
  return SmramAccessClose (&This->LockState, &This->OpenState);
}

/**
  Inhibits access to the SMRAM.

  This function prohibits access to the SMRAM region.  This function is usually
  implemented such that it is a write-once operation.

  @param  PeiServices              General purpose services available to every
                                   PEIM.
  @param  This                     The pointer to the SMM Access Interface.
  @param  DescriptorIndex          The region of SMRAM to Close.

  @retval EFI_SUCCESS            The region was successfully locked.
  @retval EFI_DEVICE_ERROR       The region could not be locked because at
                                 least one range is still open.
  @retval EFI_INVALID_PARAMETER  The descriptor index was out of bounds.

**/
STATIC
EFI_STATUS
EFIAPI
SmmAccessPeiLock (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN PEI_SMM_ACCESS_PPI              *This,
  IN UINTN                           DescriptorIndex
  )
{
  if (DescriptorIndex >= DescIdxCount) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // According to current practice, DescriptorIndex is not considered at all,
  // beyond validating it.
  //
  return SmramAccessLock (&This->LockState, &This->OpenState);
}

/**
  Queries the memory controller for the possible regions that will support
  SMRAM.

  @param  PeiServices           General purpose services available to every
                                PEIM.
  @param This                   The pointer to the SmmAccessPpi Interface.
  @param SmramMapSize           The pointer to the variable containing size of
                                the buffer to contain the description
                                information.
  @param SmramMap               The buffer containing the data describing the
                                Smram region descriptors.

  @retval EFI_BUFFER_TOO_SMALL  The user did not provide a sufficient buffer.
  @retval EFI_SUCCESS           The user provided a sufficiently-sized buffer.

**/
STATIC
EFI_STATUS
EFIAPI
SmmAccessPeiGetCapabilities (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN PEI_SMM_ACCESS_PPI              *This,
  IN OUT UINTN                       *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR        *SmramMap
  )
{
  return SmramAccessGetCapabilities (This->LockState, This->OpenState,
           SmramMapSize, SmramMap);
}

//
// LockState and OpenState will be filled in by the entry point.
//
STATIC PEI_SMM_ACCESS_PPI mAccess = {
  &SmmAccessPeiOpen,
  &SmmAccessPeiClose,
  &SmmAccessPeiLock,
  &SmmAccessPeiGetCapabilities
};


STATIC EFI_PEI_PPI_DESCRIPTOR mPpiList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gPeiSmmAccessPpiGuid, &mAccess
  }
};


//
// Utility functions.
//
STATIC
UINT8
CmosRead8 (
  IN UINT8 Index
  )
{
  IoWrite8 (0x70, Index);
  return IoRead8 (0x71);
}

STATIC
UINT32
GetSystemMemorySizeBelow4gb (
  VOID
  )
{
  UINT32 Cmos0x34;
  UINT32 Cmos0x35;

  Cmos0x34 = CmosRead8 (0x34);
  Cmos0x35 = CmosRead8 (0x35);

  return ((Cmos0x35 << 8 | Cmos0x34) << 16) + SIZE_16MB;
}


//
// Entry point of this driver.
//
EFI_STATUS
EFIAPI
SmmAccessPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  UINT16               HostBridgeDevId;
  UINT8                EsmramcVal;
  UINT8                RegMask8;
  UINT32               TopOfLowRam, TopOfLowRamMb;
  EFI_STATUS           Status;
  UINTN                SmramMapSize;
  EFI_SMRAM_DESCRIPTOR SmramMap[DescIdxCount];
  VOID                 *GuidHob;

  //
  // This module should only be included if SMRAM support is required.
  //
  ASSERT (FeaturePcdGet (PcdSmmSmramRequire));

  //
  // Verify if we're running on a Q35 machine type.
  //
  HostBridgeDevId = PciRead16 (OVMF_HOSTBRIDGE_DID);
  if (HostBridgeDevId != INTEL_Q35_MCH_DEVICE_ID) {
    DEBUG ((EFI_D_ERROR, "%a: no SMRAM with host bridge DID=0x%04x; only "
      "DID=0x%04x (Q35) is supported\n", __FUNCTION__, HostBridgeDevId,
      INTEL_Q35_MCH_DEVICE_ID));
    goto WrongConfig;
  }

  //
  // Confirm if QEMU supports SMRAM.
  //
  // With no support for it, the ESMRAMC (Extended System Management RAM
  // Control) register reads as zero. If there is support, the cache-enable
  // bits are hard-coded as 1 by QEMU.
  //
  EsmramcVal = PciRead8 (DRAMC_REGISTER_Q35 (MCH_ESMRAMC));
  RegMask8 = MCH_ESMRAMC_SM_CACHE | MCH_ESMRAMC_SM_L1 | MCH_ESMRAMC_SM_L2;
  if ((EsmramcVal & RegMask8) != RegMask8) {
    DEBUG ((EFI_D_ERROR, "%a: this Q35 implementation lacks SMRAM\n",
      __FUNCTION__));
    goto WrongConfig;
  }

  TopOfLowRam = GetSystemMemorySizeBelow4gb ();
  ASSERT ((TopOfLowRam & (SIZE_1MB - 1)) == 0);
  TopOfLowRamMb = TopOfLowRam >> 20;

  //
  // Some of the following registers are no-ops for QEMU at the moment, but it
  // is recommended to set them correctly, since the ESMRAMC that we ultimately
  // care about is in the same set of registers.
  //
  // First, we disable the integrated VGA, and set both the GTT Graphics Memory
  // Size and the Graphics Mode Select memory pre-allocation fields to zero.
  // This takes just one write to the Graphics Control Register.
  //
  PciWrite16 (DRAMC_REGISTER_Q35 (MCH_GGC), MCH_GGC_IVD);

  //
  // Set Top of Low Usable DRAM.
  //
  PciWrite16 (DRAMC_REGISTER_Q35 (MCH_TOLUD),
    (UINT16)(TopOfLowRamMb << MCH_TOLUD_MB_SHIFT));

  //
  // Given the zero graphics memory sizes configured above, set the
  // graphics-related stolen memory bases to the same as TOLUD.
  //
  PciWrite32 (DRAMC_REGISTER_Q35 (MCH_GBSM),
    TopOfLowRamMb << MCH_GBSM_MB_SHIFT);
  PciWrite32 (DRAMC_REGISTER_Q35 (MCH_BGSM),
    TopOfLowRamMb << MCH_BGSM_MB_SHIFT);

  //
  // Set TSEG Memory Base.
  //
  InitQ35TsegMbytes ();
  PciWrite32 (DRAMC_REGISTER_Q35 (MCH_TSEGMB),
    (TopOfLowRamMb - mQ35TsegMbytes) << MCH_TSEGMB_MB_SHIFT);

  //
  // Set TSEG size, and disable TSEG visibility outside of SMM. Note that the
  // T_EN bit has inverse meaning; when T_EN is set, then TSEG visibility is
  // *restricted* to SMM.
  //
  EsmramcVal &= ~(UINT32)MCH_ESMRAMC_TSEG_MASK;
  EsmramcVal |= mQ35TsegMbytes == 8 ? MCH_ESMRAMC_TSEG_8MB :
                mQ35TsegMbytes == 2 ? MCH_ESMRAMC_TSEG_2MB :
                mQ35TsegMbytes == 1 ? MCH_ESMRAMC_TSEG_1MB :
                MCH_ESMRAMC_TSEG_EXT;
  EsmramcVal |= MCH_ESMRAMC_T_EN;
  PciWrite8 (DRAMC_REGISTER_Q35 (MCH_ESMRAMC), EsmramcVal);

  //
  // TSEG should be closed (see above), but unlocked, initially. Set G_SMRAME
  // (Global SMRAM Enable) too, as both D_LCK and T_EN depend on it.
  //
  PciAndThenOr8 (DRAMC_REGISTER_Q35 (MCH_SMRAM),
    (UINT8)((~(UINT32)MCH_SMRAM_D_LCK) & 0xff), MCH_SMRAM_G_SMRAME);

  //
  // Create the GUID HOB and point it to the first SMRAM range.
  //
  GetStates (&mAccess.LockState, &mAccess.OpenState);
  SmramMapSize = sizeof SmramMap;
  Status = SmramAccessGetCapabilities (mAccess.LockState, mAccess.OpenState,
             &SmramMapSize, SmramMap);
  ASSERT_EFI_ERROR (Status);

  DEBUG_CODE_BEGIN ();
  {
    UINTN Count;
    UINTN Idx;

    Count = SmramMapSize / sizeof SmramMap[0];
    DEBUG ((EFI_D_VERBOSE, "%a: SMRAM map follows, %d entries\n", __FUNCTION__,
      (INT32)Count));
    DEBUG ((EFI_D_VERBOSE, "% 20a % 20a % 20a % 20a\n", "PhysicalStart(0x)",
      "PhysicalSize(0x)", "CpuStart(0x)", "RegionState(0x)"));
    for (Idx = 0; Idx < Count; ++Idx) {
      DEBUG ((EFI_D_VERBOSE, "% 20Lx % 20Lx % 20Lx % 20Lx\n",
        SmramMap[Idx].PhysicalStart, SmramMap[Idx].PhysicalSize,
        SmramMap[Idx].CpuStart, SmramMap[Idx].RegionState));
    }
  }
  DEBUG_CODE_END ();

  GuidHob = BuildGuidHob (&gEfiAcpiVariableGuid,
    sizeof SmramMap[DescIdxSmmS3ResumeState]);
  if (GuidHob == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (GuidHob, &SmramMap[DescIdxSmmS3ResumeState],
    sizeof SmramMap[DescIdxSmmS3ResumeState]);

  //
  // We're done. The next step should succeed, but even if it fails, we can't
  // roll back the above BuildGuidHob() allocation, because PEI doesn't support
  // releasing memory.
  //
  return PeiServicesInstallPpi (mPpiList);

WrongConfig:
  //
  // We really don't want to continue in this case.
  //
  ASSERT (FALSE);
  CpuDeadLoop ();
  return EFI_UNSUPPORTED;
}
