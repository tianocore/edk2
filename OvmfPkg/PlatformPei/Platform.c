/**@file
  Platform PEI driver

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2011, Andrei Warkentin <andreiw@motorola.com>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/QemuFwCfgS3Lib.h>
#include <Library/QemuFwCfgSimpleParserLib.h>
#include <Library/ResourcePublicationLib.h>
#include <Ppi/MasterBootMode.h>
#include <IndustryStandard/I440FxPiix4.h>
#include <IndustryStandard/Microvm.h>
#include <IndustryStandard/Pci22.h>
#include <IndustryStandard/Q35MchIch9.h>
#include <IndustryStandard/QemuCpuHotplug.h>
#include <Library/MemEncryptSevLib.h>
#include <OvmfPlatforms.h>
#include <Library/TdxHelperLib.h>

#include "Platform.h"
#include "PlatformId.h"

EFI_PEI_PPI_DESCRIPTOR  mPpiBootMode[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiMasterBootModePpiGuid,
    NULL
  }
};

VOID
MemMapInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  RETURN_STATUS  PcdStatus;

  PlatformMemMapInitialization (PlatformInfoHob);

  if (PlatformInfoHob->HostBridgeDevId == 0xffff /* microvm */) {
    return;
  }

  PcdStatus = PcdSet64S (PcdPciMmio32Base, PlatformInfoHob->PcdPciMmio32Base);
  ASSERT_RETURN_ERROR (PcdStatus);
  PcdStatus = PcdSet64S (PcdPciMmio32Size, PlatformInfoHob->PcdPciMmio32Size);
  ASSERT_RETURN_ERROR (PcdStatus);

  PcdStatus = PcdSet64S (PcdPciIoBase, PlatformInfoHob->PcdPciIoBase);
  ASSERT_RETURN_ERROR (PcdStatus);
  PcdStatus = PcdSet64S (PcdPciIoSize, PlatformInfoHob->PcdPciIoSize);
  ASSERT_RETURN_ERROR (PcdStatus);
}

STATIC
VOID
NoexecDxeInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  RETURN_STATUS  Status;

  Status = PlatformNoexecDxeInitialization (PlatformInfoHob);
  if (!RETURN_ERROR (Status)) {
    Status = PcdSetBoolS (PcdSetNxForStack, PlatformInfoHob->PcdSetNxForStack);
    ASSERT_RETURN_ERROR (Status);
  }
}

static const UINT8  EmptyFdt[] = {
  0xd0, 0x0d, 0xfe, 0xed, 0x00, 0x00, 0x00, 0x48,
  0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x48,
  0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x11,
  0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x09,
};

VOID
MicrovmInitialization (
  VOID
  )
{
  FIRMWARE_CONFIG_ITEM  FdtItem;
  UINTN                 FdtSize;
  UINTN                 FdtPages;
  EFI_STATUS            Status;
  UINT64                *FdtHobData;
  VOID                  *NewBase;

  Status = QemuFwCfgFindFile ("etc/fdt", &FdtItem, &FdtSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: no etc/fdt found in fw_cfg, using dummy\n", __func__));
    FdtItem = 0;
    FdtSize = sizeof (EmptyFdt);
  }

  FdtPages = EFI_SIZE_TO_PAGES (FdtSize);
  NewBase  = AllocatePages (FdtPages);
  if (NewBase == NULL) {
    DEBUG ((DEBUG_INFO, "%a: AllocatePages failed\n", __func__));
    return;
  }

  if (FdtItem) {
    QemuFwCfgSelectItem (FdtItem);
    QemuFwCfgReadBytes (FdtSize, NewBase);
  } else {
    CopyMem (NewBase, EmptyFdt, FdtSize);
  }

  FdtHobData = BuildGuidHob (&gFdtHobGuid, sizeof (*FdtHobData));
  if (FdtHobData == NULL) {
    DEBUG ((DEBUG_INFO, "%a: BuildGuidHob failed\n", __func__));
    return;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: fdt at 0x%x (size %d)\n",
    __func__,
    NewBase,
    FdtSize
    ));
  *FdtHobData = (UINTN)NewBase;
}

VOID
MiscInitializationForMicrovm (
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  RETURN_STATUS  PcdStatus;

  ASSERT (PlatformInfoHob->HostBridgeDevId == 0xffff);

  DEBUG ((DEBUG_INFO, "%a: microvm\n", __func__));
  //
  // Disable A20 Mask
  //
  IoOr8 (0x92, BIT1);

  //
  // Build the CPU HOB with guest RAM size dependent address width and 16-bits
  // of IO space. (Side note: unlike other HOBs, the CPU HOB is needed during
  // S3 resume as well, so we build it unconditionally.)
  //
  BuildCpuHob (PlatformInfoHob->PhysMemAddressWidth, 16);

  MicrovmInitialization ();
  PcdStatus = PcdSet16S (
                PcdOvmfHostBridgePciDevId,
                MICROVM_PSEUDO_DEVICE_ID
                );
  ASSERT_RETURN_ERROR (PcdStatus);
}

VOID
MiscInitialization (
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  RETURN_STATUS  PcdStatus;

  PlatformMiscInitialization (PlatformInfoHob);

  PcdStatus = PcdSet16S (PcdOvmfHostBridgePciDevId, PlatformInfoHob->HostBridgeDevId);
  ASSERT_RETURN_ERROR (PcdStatus);
}

VOID
BootModeInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  EFI_STATUS  Status;

  if (PlatformCmosRead8 (0xF) == 0xFE) {
    PlatformInfoHob->BootMode = BOOT_ON_S3_RESUME;
  }

  PlatformCmosWrite8 (0xF, 0x00);

  Status = PeiServicesSetBootMode (PlatformInfoHob->BootMode);
  ASSERT_EFI_ERROR (Status);

  Status = PeiServicesInstallPpi (mPpiBootMode);
  ASSERT_EFI_ERROR (Status);
}

VOID
ReserveEmuVariableNvStore (
  )
{
  EFI_PHYSICAL_ADDRESS  VariableStore;
  RETURN_STATUS         PcdStatus;

  VariableStore = (EFI_PHYSICAL_ADDRESS)(UINTN)PlatformReserveEmuVariableNvStore ();
  PcdStatus     = PcdSet64S (PcdEmuVariableNvStoreReserved, VariableStore);

  if (FeaturePcdGet (PcdSecureBootSupported)) {
    // restore emulated VarStore from pristine ROM copy
    PlatformInitEmuVariableNvStore ((VOID *)(UINTN)VariableStore);
  }

  ASSERT_RETURN_ERROR (PcdStatus);
}

STATIC
VOID
Q35BoardVerification (
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  if (PlatformInfoHob->HostBridgeDevId == INTEL_Q35_MCH_DEVICE_ID) {
    return;
  }

  DEBUG ((
    DEBUG_ERROR,
    "%a: no TSEG (SMRAM) on host bridge DID=0x%04x; "
    "only DID=0x%04x (Q35) is supported\n",
    __func__,
    PlatformInfoHob->HostBridgeDevId,
    INTEL_Q35_MCH_DEVICE_ID
    ));
  ASSERT (FALSE);
  CpuDeadLoop ();
}

/**
  Fetch the boot CPU count and the possible CPU count from QEMU, and expose
  them to UefiCpuPkg modules. Set the MaxCpuCount field in PlatformInfoHob.
**/
VOID
MaxCpuCountInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  RETURN_STATUS  PcdStatus;

  PlatformMaxCpuCountInitialization (PlatformInfoHob);

  PcdStatus = PcdSet32S (PcdCpuBootLogicalProcessorNumber, PlatformInfoHob->PcdCpuBootLogicalProcessorNumber);
  ASSERT_RETURN_ERROR (PcdStatus);
  PcdStatus = PcdSet32S (PcdCpuMaxLogicalProcessorNumber, PlatformInfoHob->PcdCpuMaxLogicalProcessorNumber);
  ASSERT_RETURN_ERROR (PcdStatus);
}

/**
 * @brief Builds PlatformInfo Hob
 */
EFI_HOB_PLATFORM_INFO *
BuildPlatformInfoHob (
  VOID
  )
{
  EFI_HOB_PLATFORM_INFO  PlatformInfoHob;
  EFI_HOB_GUID_TYPE      *GuidHob;

  ZeroMem (&PlatformInfoHob, sizeof PlatformInfoHob);
  BuildGuidDataHob (&gUefiOvmfPkgPlatformInfoGuid, &PlatformInfoHob, sizeof (EFI_HOB_PLATFORM_INFO));
  GuidHob = GetFirstGuidHob (&gUefiOvmfPkgPlatformInfoGuid);
  return (EFI_HOB_PLATFORM_INFO *)GET_GUID_HOB_DATA (GuidHob);
}

/**
  Perform Platform PEI initialization.

  @param  FileHandle      Handle of the file being invoked.
  @param  PeiServices     Describes the list of possible PEI Services.

  @return EFI_SUCCESS     The PEIM initialized successfully.

**/
EFI_STATUS
EFIAPI
InitializePlatform (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_HOB_PLATFORM_INFO  *PlatformInfoHob;
  EFI_STATUS             Status;

  DEBUG ((DEBUG_INFO, "Platform PEIM Loaded\n"));
  PlatformInfoHob = BuildPlatformInfoHob ();

  if (TdIsEnabled ()) {
    TdxHelperBuildGuidHobForTdxMeasurement ();
  }

  PlatformInfoHob->SmmSmramRequire     = FeaturePcdGet (PcdSmmSmramRequire);
  PlatformInfoHob->SevEsIsEnabled      = MemEncryptSevEsIsEnabled ();
  PlatformInfoHob->PcdPciMmio64Size    = PcdGet64 (PcdPciMmio64Size);
  PlatformInfoHob->DefaultMaxCpuNumber = PcdGet32 (PcdCpuMaxLogicalProcessorNumber);

  PlatformDebugDumpCmos ();

  if (QemuFwCfgS3Enabled ()) {
    DEBUG ((DEBUG_INFO, "S3 support was detected on QEMU\n"));
    PlatformInfoHob->S3Supported = TRUE;
    Status                       = PcdSetBoolS (PcdAcpiS3Enable, TRUE);
    ASSERT_EFI_ERROR (Status);
  }

  BootModeInitialization (PlatformInfoHob);

  //
  // Query Host Bridge DID
  //
  PlatformInfoHob->HostBridgeDevId = PciRead16 (OVMF_HOSTBRIDGE_DID);
  AddressWidthInitialization (PlatformInfoHob);

  MaxCpuCountInitialization (PlatformInfoHob);

  if (PlatformInfoHob->SmmSmramRequire) {
    Q35BoardVerification (PlatformInfoHob);
    Q35TsegMbytesInitialization (PlatformInfoHob);
    Q35SmramAtDefaultSmbaseInitialization (PlatformInfoHob);
  }

  PublishPeiMemory (PlatformInfoHob);

  PlatformQemuUc32BaseInitialization (PlatformInfoHob);

  InitializeRamRegions (PlatformInfoHob);

  if (PlatformInfoHob->BootMode != BOOT_ON_S3_RESUME) {
    if (!PlatformInfoHob->SmmSmramRequire) {
      ReserveEmuVariableNvStore ();
    }

    PeiFvInitialization (PlatformInfoHob);
    MemTypeInfoInitialization (PlatformInfoHob);
    MemMapInitialization (PlatformInfoHob);
    NoexecDxeInitialization (PlatformInfoHob);
  }

  InstallClearCacheCallback ();
  AmdSevInitialize (PlatformInfoHob);
  if (PlatformInfoHob->HostBridgeDevId == 0xffff) {
    MiscInitializationForMicrovm (PlatformInfoHob);
  } else {
    MiscInitialization (PlatformInfoHob);
    PlatformIdInitialization (PeiServices);
  }

  IntelTdxInitialize ();
  InstallFeatureControlCallback (PlatformInfoHob);
  if (PlatformInfoHob->SmmSmramRequire) {
    RelocateSmBase ();
  }

  return EFI_SUCCESS;
}
