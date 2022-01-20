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
#include <Library/PlatformInitLib.h>
#include <OvmfPlatforms.h>

#include "Platform.h"

EFI_PEI_PPI_DESCRIPTOR  mPpiBootMode[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiMasterBootModePpiGuid,
    NULL
  }
};

UINT16  mHostBridgeDevId;

EFI_BOOT_MODE  mBootMode = BOOT_WITH_FULL_CONFIGURATION;

BOOLEAN  mS3Supported = FALSE;

UINT32  mMaxCpuCount;

extern UINT32  mLowerMemorySize;

VOID
MemMapInitialization (
  VOID
  )
{
  UINT64         PciIoBase;
  UINT64         PciIoSize;
  RETURN_STATUS  PcdStatus;
  UINT32         PciBase;
  UINT32         PciSize;

  PlatformMemMapInitialization (mHostBridgeDevId, mQemuUc32Base, &PciBase, &PciSize, &PciIoBase, &PciIoSize);
  if (mHostBridgeDevId == 0xffff /* microvm */) {
    return;
  }

  PcdStatus = PcdSet64S (PcdPciMmio32Base, PciBase);
  ASSERT_RETURN_ERROR (PcdStatus);
  PcdStatus = PcdSet64S (PcdPciMmio32Size, PciSize);
  ASSERT_RETURN_ERROR (PcdStatus);

  PcdStatus = PcdSet64S (PcdPciIoBase, PciIoBase);
  ASSERT_RETURN_ERROR (PcdStatus);
  PcdStatus = PcdSet64S (PcdPciIoSize, PciIoSize);
  ASSERT_RETURN_ERROR (PcdStatus);
}

#define UPDATE_BOOLEAN_PCD_FROM_FW_CFG(TokenName)                   \
          do {                                                      \
            BOOLEAN       Setting;                                  \
            RETURN_STATUS PcdStatus;                                \
                                                                    \
            if (!RETURN_ERROR (QemuFwCfgParseBool (                 \
                              "opt/ovmf/" #TokenName, &Setting))) { \
              PcdStatus = PcdSetBoolS (TokenName, Setting);         \
              ASSERT_RETURN_ERROR (PcdStatus);                      \
            }                                                       \
          } while (0)

VOID
NoexecDxeInitialization (
  VOID
  )
{
  UPDATE_BOOLEAN_PCD_FROM_FW_CFG (PcdSetNxForStack);
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
    DEBUG ((DEBUG_INFO, "%a: no etc/fdt found in fw_cfg, using dummy\n", __FUNCTION__));
    FdtItem = 0;
    FdtSize = sizeof (EmptyFdt);
  }

  FdtPages = EFI_SIZE_TO_PAGES (FdtSize);
  NewBase  = AllocatePages (FdtPages);
  if (NewBase == NULL) {
    DEBUG ((DEBUG_INFO, "%a: AllocatePages failed\n", __FUNCTION__));
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
    DEBUG ((DEBUG_INFO, "%a: BuildGuidHob failed\n", __FUNCTION__));
    return;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: fdt at 0x%x (size %d)\n",
    __FUNCTION__,
    NewBase,
    FdtSize
    ));
  *FdtHobData = (UINTN)NewBase;
}

/**
 * Misc initialization for Microvm and Cloud-Hypervisor
 *
 * @return VOID
 */
VOID
MiscInitialization2 (
  VOID
  )
{
  RETURN_STATUS  PcdStatus;

  if ((mHostBridgeDevId != 0xffff) && (mHostBridgeDevId != CLOUDHV_DEVICE_ID)) {
    ASSERT (FALSE);
    return;
  }

  //
  // Disable A20 Mask
  //
  IoOr8 (0x92, BIT1);

  //
  // Build the CPU HOB with guest RAM size dependent address width and 16-bits
  // of IO space. (Side note: unlike other HOBs, the CPU HOB is needed during
  // S3 resume as well, so we build it unconditionally.)
  //
  BuildCpuHob (mPhysMemAddressWidth, 16);

  switch (mHostBridgeDevId) {
    case 0xffff: /* microvm */
      DEBUG ((DEBUG_INFO, "%a: microvm\n", __FUNCTION__));
      MicrovmInitialization ();
      PcdStatus = PcdSet16S (
                    PcdOvmfHostBridgePciDevId,
                    MICROVM_PSEUDO_DEVICE_ID
                    );
      ASSERT_RETURN_ERROR (PcdStatus);
      return;
    case CLOUDHV_DEVICE_ID:
      DEBUG ((DEBUG_INFO, "%a: Cloud Hypervisor host bridge\n", __FUNCTION__));
      PcdStatus = PcdSet16S (
                    PcdOvmfHostBridgePciDevId,
                    CLOUDHV_DEVICE_ID
                    );
      ASSERT_RETURN_ERROR (PcdStatus);
      return;
    default:
      DEBUG ((
        DEBUG_ERROR,
        "%a: Unknown Host Bridge Device ID: 0x%04x\n",
        __FUNCTION__,
        mHostBridgeDevId
        ));
      ASSERT (FALSE);
      return;
  }
}

VOID
MiscInitialization (
  VOID
  )
{
  RETURN_STATUS  PcdStatus;

  if ((mHostBridgeDevId == 0xffff) || (mHostBridgeDevId == CLOUDHV_DEVICE_ID)) {
    MiscInitialization2 ();
    return;
  }

  PlatformMiscInitialization (mHostBridgeDevId, mPhysMemAddressWidth);

  PcdStatus = PcdSet16S (PcdOvmfHostBridgePciDevId, mHostBridgeDevId);
  ASSERT_RETURN_ERROR (PcdStatus);
}

VOID
BootModeInitialization (
  VOID
  )
{
  EFI_STATUS  Status;

  if (PlatformCmosRead8 (0xF) == 0xFE) {
    mBootMode = BOOT_ON_S3_RESUME;
  }

  PlatformCmosWrite8 (0xF, 0x00);

  Status = PeiServicesSetBootMode (mBootMode);
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

  //
  // Allocate storage for NV variables early on so it will be
  // at a consistent address.  Since VM memory is preserved
  // across reboots, this allows the NV variable storage to survive
  // a VM reboot.
  //
  VariableStore =
    (EFI_PHYSICAL_ADDRESS)(UINTN)
    AllocateRuntimePages (
      EFI_SIZE_TO_PAGES (2 * PcdGet32 (PcdFlashNvStorageFtwSpareSize))
      );
  DEBUG ((
    DEBUG_INFO,
    "Reserved variable store memory: 0x%lX; size: %dkb\n",
    VariableStore,
    (2 * PcdGet32 (PcdFlashNvStorageFtwSpareSize)) / 1024
    ));
  PcdStatus = PcdSet64S (PcdEmuVariableNvStoreReserved, VariableStore);
  ASSERT_RETURN_ERROR (PcdStatus);
}

VOID
S3Verification (
  VOID
  )
{
 #if defined (MDE_CPU_X64)
  if (FeaturePcdGet (PcdSmmSmramRequire) && mS3Supported) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: S3Resume2Pei doesn't support X64 PEI + SMM yet.\n",
      __FUNCTION__
      ));
    DEBUG ((
      DEBUG_ERROR,
      "%a: Please disable S3 on the QEMU command line (see the README),\n",
      __FUNCTION__
      ));
    DEBUG ((
      DEBUG_ERROR,
      "%a: or build OVMF with \"OvmfPkgIa32X64.dsc\".\n",
      __FUNCTION__
      ));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

 #endif
}

VOID
Q35BoardVerification (
  VOID
  )
{
  if (mHostBridgeDevId == INTEL_Q35_MCH_DEVICE_ID) {
    return;
  }

  DEBUG ((
    DEBUG_ERROR,
    "%a: no TSEG (SMRAM) on host bridge DID=0x%04x; "
    "only DID=0x%04x (Q35) is supported\n",
    __FUNCTION__,
    mHostBridgeDevId,
    INTEL_Q35_MCH_DEVICE_ID
    ));
  ASSERT (FALSE);
  CpuDeadLoop ();
}

/**
  Fetch the boot CPU count and the possible CPU count from QEMU, and expose
  them to UefiCpuPkg modules. Set the mMaxCpuCount variable.
**/
VOID
MaxCpuCountInitialization (
  VOID
  )
{
  UINT16         BootCpuCount;
  RETURN_STATUS  PcdStatus;

  PlatformMaxCpuCountInitialization (
    mHostBridgeDevId,
    PcdGet32 (PcdCpuMaxLogicalProcessorNumber),
    &mMaxCpuCount,
    &BootCpuCount
    );

  PcdStatus = PcdSet32S (PcdCpuBootLogicalProcessorNumber, BootCpuCount);
  ASSERT_RETURN_ERROR (PcdStatus);
  PcdStatus = PcdSet32S (PcdCpuMaxLogicalProcessorNumber, mMaxCpuCount);
  ASSERT_RETURN_ERROR (PcdStatus);
}

/**
 * @brief Builds PlatformInfo Hob
 */
VOID
BuildPlatformInfoHob (
  VOID
  )
{
  EFI_HOB_PLATFORM_INFO  PlatformInfoHob;

  ZeroMem (&PlatformInfoHob, sizeof (PlatformInfoHob));
  PlatformInfoHob.HostBridgePciDevId = mHostBridgeDevId;

  BuildGuidDataHob (&gUefiOvmfPkgPlatformInfoGuid, &PlatformInfoHob, sizeof (EFI_HOB_PLATFORM_INFO));
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
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "Platform PEIM Loaded\n"));

  PlatformDebugDumpCmos ();

  if (QemuFwCfgS3Enabled ()) {
    DEBUG ((DEBUG_INFO, "S3 support was detected on QEMU\n"));
    mS3Supported = TRUE;
    Status       = PcdSetBoolS (PcdAcpiS3Enable, TRUE);
    ASSERT_EFI_ERROR (Status);
  }

  S3Verification ();
  BootModeInitialization ();
  AddressWidthInitialization ();

  //
  // Query Host Bridge DID
  //
  mHostBridgeDevId = PciRead16 (OVMF_HOSTBRIDGE_DID);

  MaxCpuCountInitialization ();

  if (FeaturePcdGet (PcdSmmSmramRequire)) {
    Q35BoardVerification ();
    Q35TsegMbytesInitialization ();
    Q35SmramAtDefaultSmbaseInitialization ();
  }

  //
  // Fetch the lower memory size (Below 4G)
  //
  mLowerMemorySize = PlatformGetSystemMemorySizeBelow4gb (mHostBridgeDevId);

  PublishPeiMemory ();

  QemuUc32BaseInitialization ();

  InitializeRamRegions ();

  if (mBootMode != BOOT_ON_S3_RESUME) {
    if (!FeaturePcdGet (PcdSmmSmramRequire)) {
      ReserveEmuVariableNvStore ();
    }

    PeiFvInitialization ();
    MemTypeInfoInitialization ();
    MemMapInitialization ();
    NoexecDxeInitialization ();
  }

  InstallClearCacheCallback ();
  AmdSevInitialize ();
  IntelTdxInitialize ();
  MiscInitialization ();
  InstallFeatureControlCallback ();
  BuildPlatformInfoHob ();

  return EFI_SUCCESS;
}
