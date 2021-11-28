/**@file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/ResourcePublicationLib.h>
#include <Ppi/MasterBootMode.h>
#include <IndustryStandard/Pci22.h>
#include <Uefi/UefiBaseType.h>
#include <Library/BaseMemoryLib.h>
#include <Uefi/UefiMultiPhase.h>
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>
#include <Library/HobLib.h>
#include <Library/TdxPlatformLib.h>
#include <Library/PciLib.h>
#include <IndustryStandard/Pci22.h>
#include <IndustryStandard/Q35MchIch9.h>
#include <IndustryStandard/I440FxPiix4.h>

//
// Host Bridge DID Address
//
#define HOSTBRIDGE_DID \
  PCI_LIB_ADDRESS (0, 0, 0, PCI_DEVICE_ID_OFFSET)

//
// Values we program into the PM base address registers
//
#define PIIX4_PMBA_VALUE   0xB000
#define ICH9_PMBASE_VALUE  0x0600

EFI_STATUS
GetNamedFwCfgBoolean (
  IN  CHAR8    *FwCfgFileName,
  OUT BOOLEAN  *Setting
  )
{
  EFI_STATUS            Status;
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  UINTN                 FwCfgSize;
  UINT8                 Value[3];

  Status = QemuFwCfgFindFile (FwCfgFileName, &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (FwCfgSize > sizeof Value) {
    return EFI_BAD_BUFFER_SIZE;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (FwCfgSize, Value);

  if ((FwCfgSize == 1) ||
      ((FwCfgSize == 2) && (Value[1] == '\n')) ||
      ((FwCfgSize == 3) && (Value[1] == '\r') && (Value[2] == '\n')))
  {
    switch (Value[0]) {
      case '0':
      case 'n':
      case 'N':
        *Setting = FALSE;
        return EFI_SUCCESS;

      case '1':
      case 'y':
      case 'Y':
        *Setting = TRUE;
        return EFI_SUCCESS;

      default:
        break;
    }
  }

  return EFI_PROTOCOL_ERROR;
}

VOID
PciExBarInitialization (
  VOID
  )
{
  union {
    UINT64    Uint64;
    UINT32    Uint32[2];
  } PciExBarBase;

  //
  // We only support the 256MB size for the MMCONFIG area:
  // 256 buses * 32 devices * 8 functions * 4096 bytes config space.
  //
  // The masks used below enforce the Q35 requirements that the MMCONFIG area
  // be (a) correctly aligned -- here at 256 MB --, (b) located under 64 GB.
  //
  // Note that (b) also ensures that the minimum address width we have
  // determined in AddressWidthInitialization(), i.e., 36 bits, will suffice
  // for DXE's page tables to cover the MMCONFIG area.
  //
  PciExBarBase.Uint64 = FixedPcdGet64 (PcdPciExpressBaseAddress);
  ASSERT ((PciExBarBase.Uint32[1] & MCH_PCIEXBAR_HIGHMASK) == 0);
  ASSERT ((PciExBarBase.Uint32[0] & MCH_PCIEXBAR_LOWMASK) == 0);

  //
  // Clear the PCIEXBAREN bit first, before programming the high register.
  //
  PciWrite32 (DRAMC_REGISTER_Q35 (MCH_PCIEXBAR_LOW), 0);

  //
  // Program the high register. Then program the low register, setting the
  // MMCONFIG area size and enabling decoding at once.
  //
  PciWrite32 (DRAMC_REGISTER_Q35 (MCH_PCIEXBAR_HIGH), PciExBarBase.Uint32[1]);
  PciWrite32 (
    DRAMC_REGISTER_Q35 (MCH_PCIEXBAR_LOW),
    PciExBarBase.Uint32[0] | MCH_PCIEXBAR_BUS_FF | MCH_PCIEXBAR_EN
    );
}

VOID
MiscInitialization (
  EFI_HOB_PLATFORM_INFO  *PlatformInfoHob,
  BOOLEAN                *CfgSysStateDefault
  )
{
  RETURN_STATUS         Status;
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  UINTN                 FwCfgSize;
  UINTN                 PmCmd;
  UINTN                 Pmba;
  UINT32                PmbaAndVal;
  UINT32                PmbaOrVal;
  UINTN                 AcpiCtlReg;
  UINT8                 AcpiEnBit;

  //
  // Disable A20 Mask
  //
  IoOr8 (0x92, BIT1);

  //
  // Determine platform type and save Host Bridge DID to PCD
  //
  switch (PlatformInfoHob->HostBridgePciDevId) {
    case INTEL_82441_DEVICE_ID:
      PmCmd      = POWER_MGMT_REGISTER_PIIX4 (PCI_COMMAND_OFFSET);
      Pmba       = POWER_MGMT_REGISTER_PIIX4 (PIIX4_PMBA);
      PmbaAndVal = ~(UINT32)PIIX4_PMBA_MASK;
      PmbaOrVal  = PIIX4_PMBA_VALUE;
      AcpiCtlReg = POWER_MGMT_REGISTER_PIIX4 (PIIX4_PMREGMISC);
      AcpiEnBit  = PIIX4_PMREGMISC_PMIOSE;
      break;
    case INTEL_Q35_MCH_DEVICE_ID:
      PmCmd      = POWER_MGMT_REGISTER_Q35 (PCI_COMMAND_OFFSET);
      Pmba       = POWER_MGMT_REGISTER_Q35 (ICH9_PMBASE);
      PmbaAndVal = ~(UINT32)ICH9_PMBASE_MASK;
      PmbaOrVal  = ICH9_PMBASE_VALUE;
      AcpiCtlReg = POWER_MGMT_REGISTER_Q35 (ICH9_ACPI_CNTL);
      AcpiEnBit  = ICH9_ACPI_CNTL_ACPI_EN;
      break;
    default:
      DEBUG ((
        DEBUG_ERROR,
        "%a: Unknown Host Bridge Device ID: 0x%04x\n",
        __FUNCTION__,
        PlatformInfoHob->HostBridgePciDevId
        ));
      ASSERT (FALSE);
      return;
  }

  //
  // If the appropriate IOspace enable bit is set, assume the ACPI PMBA
  // has been configured and skip the setup here.
  // This matches the logic in AcpiTimerLibConstructor ().
  //
  if ((PciRead8 (AcpiCtlReg) & AcpiEnBit) == 0) {
    //
    // The PEI phase should be exited with fully accessibe ACPI PM IO space:
    // 1. set PMBA
    //
    PciAndThenOr32 (Pmba, PmbaAndVal, PmbaOrVal);

    //
    // 2. set PCICMD/IOSE
    //
    PciOr8 (PmCmd, EFI_PCI_COMMAND_IO_SPACE);

    //
    // 3. set ACPI PM IO enable bit (PMREGMISC:PMIOSE or ACPI_CNTL:ACPI_EN)
    //
    PciOr8 (AcpiCtlReg, AcpiEnBit);
  }

  if (PlatformInfoHob->HostBridgePciDevId == INTEL_Q35_MCH_DEVICE_ID) {
    //
    // Set Root Complex Register Block BAR
    //
    PciWrite32 (
      POWER_MGMT_REGISTER_Q35 (ICH9_RCBA),
      ICH9_ROOT_COMPLEX_BASE | ICH9_RCBA_EN
      );

    //
    // Set PCI Express Register Range Base Address
    //
    PciExBarInitialization ();
  }

  //
  // check for overrides
  //
  Status = QemuFwCfgFindFile ("etc/system-states", &FwCfgItem, &FwCfgSize);
  if ((Status != RETURN_SUCCESS) || (FwCfgSize != sizeof PlatformInfoHob->SystemStates)) {
    DEBUG ((DEBUG_INFO, "ACPI using S3/S4 defaults\n"));
    *CfgSysStateDefault = TRUE;
    return;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (sizeof PlatformInfoHob->SystemStates, PlatformInfoHob->SystemStates);
}

/**
 * Perform Platform initialization.
 *
 * @param PlatformInfoHob       Pointer to the PlatformInfo Hob
 * @param CfgSysStateDefault    Indicate if using the default SysState
 * @param CfgNxForStackDefault  Indicate if using the default NxForStack
 * @return VOID
 */
VOID
EFIAPI
TdxPlatformInitialize (
  EFI_HOB_PLATFORM_INFO  *PlatformInfoHob,
  BOOLEAN                *CfgSysStateDefault,
  BOOLEAN                *CfgNxForStackDefault
  )
{
  RETURN_STATUS  Status;

  PlatformInfoHob->HostBridgePciDevId = PciRead16 (HOSTBRIDGE_DID);

  if (PlatformInfoHob->HostBridgePciDevId == INTEL_Q35_MCH_DEVICE_ID) {
    BuildResourceDescriptorHob (
      EFI_RESOURCE_IO,
      EFI_RESOURCE_ATTRIBUTE_PRESENT     |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED,
      0x6000,
      0xa000
      );
  } else {
    BuildResourceDescriptorHob (
      EFI_RESOURCE_IO,
      EFI_RESOURCE_ATTRIBUTE_PRESENT     |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED,
      0xc000,
      0x4000
      );
  }

  MiscInitialization (PlatformInfoHob, CfgSysStateDefault);

  Status = GetNamedFwCfgBoolean ("opt/ovmf/PcdSetNxForStack", &PlatformInfoHob->SetNxForStack);
  if (Status != RETURN_SUCCESS) {
    DEBUG ((DEBUG_INFO, "NxForStack using defaults\n"));
    *CfgNxForStackDefault = TRUE;
  }
}
