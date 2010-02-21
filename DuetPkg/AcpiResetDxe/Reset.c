/*++ @file
  Reset Architectural Protocol implementation.

Copyright (c) 2006 - 2010, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

--*/

#include "Reset.h"

/**
  Use ACPI method to reset the sytem. If fail, use legacy 8042 method to reset the system

  @param[in] AcpiDescription   Global variable to record reset info

**/
VOID
SystemReset (
  IN EFI_ACPI_DESCRIPTION *AcpiDescription
  )
{
  UINT8   Dev;
  UINT8   Func;
  UINT8   Register;

  if ((AcpiDescription->RESET_REG.Address != 0) &&
      ((AcpiDescription->RESET_REG.AddressSpaceId == EFI_ACPI_3_0_SYSTEM_IO) ||
       (AcpiDescription->RESET_REG.AddressSpaceId == EFI_ACPI_3_0_SYSTEM_MEMORY) ||
       (AcpiDescription->RESET_REG.AddressSpaceId == EFI_ACPI_3_0_PCI_CONFIGURATION_SPACE))) {
    //
    // Use ACPI System Reset
    //
    switch (AcpiDescription->RESET_REG.AddressSpaceId) {
    case EFI_ACPI_3_0_SYSTEM_IO:
      IoWrite8 ((UINTN) AcpiDescription->RESET_REG.Address, AcpiDescription->RESET_VALUE);
      break;
    case EFI_ACPI_3_0_SYSTEM_MEMORY:
      MmioWrite8 ((UINTN) AcpiDescription->RESET_REG.Address, AcpiDescription->RESET_VALUE);
      break;
    case EFI_ACPI_3_0_PCI_CONFIGURATION_SPACE:
      Register = (UINT8) AcpiDescription->RESET_REG.Address;
      Func     = (UINT8) (RShiftU64 (AcpiDescription->RESET_REG.Address, 16) & 0x7);
      Dev      = (UINT8) (RShiftU64 (AcpiDescription->RESET_REG.Address, 32) & 0x1F);
      PciWrite8 (PCI_LIB_ADDRESS (0, Dev, Func, Register), AcpiDescription->RESET_VALUE);
      break;
    }
  }

  //
  // If system comes here, means ACPI reset fail, do Legacy System Reset, assume 8042 available
  //
  Register = 0xfe;
  IoWrite8 (0x64, Register);

  //
  // System should reset now
  //

  return ;
}

/**
  Use ACPI method to shutdown the sytem

  @param[in] AcpiDescription   Global variable to record reset info

  @retval EFI_UNSUPPORTED      Shutdown fails

**/
EFI_STATUS
SystemShutdown (
  IN EFI_ACPI_DESCRIPTION *AcpiDescription
  )
{
  UINT16  Value;

  //
  // 1. Write SLP_TYPa
  //
  if ((AcpiDescription->PM1a_CNT_BLK.Address != 0) && (AcpiDescription->SLP_TYPa != 0)) {
    switch (AcpiDescription->PM1a_CNT_BLK.AddressSpaceId) {
    case EFI_ACPI_3_0_SYSTEM_IO:
      Value = IoRead16 ((UINTN) AcpiDescription->PM1a_CNT_BLK.Address);
      Value = (Value & 0xc3ff) | 0x2000 | (AcpiDescription->SLP_TYPa << 10);
      IoWrite16 ((UINTN) AcpiDescription->PM1a_CNT_BLK.Address, Value);
      break;
    case EFI_ACPI_3_0_SYSTEM_MEMORY:
      Value = MmioRead16 ((UINTN) AcpiDescription->PM1a_CNT_BLK.Address);
      Value = (Value & 0xc3ff) | 0x2000 | (AcpiDescription->SLP_TYPa << 10);
      MmioWrite16 ((UINTN) AcpiDescription->PM1a_CNT_BLK.Address, Value);
      break;
    }
  }

  //
  // 2. Write SLP_TYPb
  //
  if ((AcpiDescription->PM1b_CNT_BLK.Address != 0) && (AcpiDescription->SLP_TYPb != 0)) {
    switch (AcpiDescription->PM1b_CNT_BLK.AddressSpaceId) {
    case EFI_ACPI_3_0_SYSTEM_IO:
      Value = IoRead16 ((UINTN) AcpiDescription->PM1b_CNT_BLK.Address);
      Value = (Value & 0xc3ff) | 0x2000 | (AcpiDescription->SLP_TYPb << 10);
      IoWrite16 ((UINTN) AcpiDescription->PM1b_CNT_BLK.Address, Value);
      break;
    case EFI_ACPI_3_0_SYSTEM_MEMORY:
      Value = MmioRead16 ((UINTN) AcpiDescription->PM1b_CNT_BLK.Address);
      Value = (Value & 0xc3ff) | 0x2000 | (AcpiDescription->SLP_TYPb << 10);
      MmioWrite16 ((UINTN) AcpiDescription->PM1b_CNT_BLK.Address, Value);
      break;
    }
  }

  //
  // Done, if code runs here, mean not shutdown correctly
  //
  return EFI_UNSUPPORTED;
}

/**
  Reset the system.

  @param[in] ResetType       Warm or cold
  @param[in] ResetStatus     Possible cause of reset
  @param[in] DataSize        Size of ResetData in bytes
  @param[in] ResetData       Optional Unicode string
  @param[in] AcpiDescription Global variable to record reset info

**/
VOID
EFIAPI
AcpiResetSystem (
  IN EFI_RESET_TYPE   ResetType,
  IN EFI_STATUS       ResetStatus,
  IN UINTN            DataSize,
  IN CHAR16           *ResetData, OPTIONAL
  IN EFI_ACPI_DESCRIPTION *AcpiDescription
  )
{
  EFI_STATUS  Status;

  switch (ResetType) {
  case EfiResetWarm:
  case EfiResetCold:
    SystemReset (AcpiDescription);
    break;

  case EfiResetShutdown:
    Status = SystemShutdown (AcpiDescription);
    if (EFI_ERROR (Status)) {
      SystemReset (AcpiDescription);
    }
    break;

  default:
    return ;
  }

  //
  // Given we should have reset getting here would be bad
  //
  ASSERT (FALSE);
}

BOOLEAN
GetAcpiDescription (
  IN EFI_ACPI_DESCRIPTION *AcpiDescription
  )
{
  EFI_HOB_GUID_TYPE       *HobAcpiDescription;
  //
  // Get AcpiDescription Hob
  //
  HobAcpiDescription = GetFirstGuidHob (&gEfiAcpiDescriptionGuid);
  if (HobAcpiDescription == NULL) {
    return FALSE;
  }

  //
  // Copy it to Runtime Memory
  //
  ASSERT (sizeof (EFI_ACPI_DESCRIPTION) == GET_GUID_HOB_DATA_SIZE (HobAcpiDescription));
  CopyMem (AcpiDescription, GET_GUID_HOB_DATA (HobAcpiDescription), sizeof (EFI_ACPI_DESCRIPTION));

  DEBUG ((EFI_D_ERROR, "ACPI Reset Base - %lx\n", AcpiDescription->RESET_REG.Address));
  DEBUG ((EFI_D_ERROR, "ACPI Reset Value - %02x\n", (UINTN)AcpiDescription->RESET_VALUE));
  DEBUG ((EFI_D_ERROR, "IAPC support - %x\n", (UINTN)(AcpiDescription->IAPC_BOOT_ARCH)));
  return TRUE;
}
