/** @file
  Base ResetSystem library implementation.

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/QemuFwCfgLib.h>
#include "ResetSystemAcpiGed.h"

/**
  Get configuration item data by the firmware configuration file name.

  @param[in]  Name - Name of file to look up.

  @return    VOID*       The Pointer of Value of Firmware Configuration item read.
**/
STATIC
VOID *
GetFwCfgData (
  CONST CHAR8  *Name
  )
{
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  EFI_STATUS            Status;
  UINTN                 FwCfgSize;
  VOID                  *Data;

  Status = QemuFwCfgFindFile (Name, &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a %d read  %s error Status %d \n", __func__, __LINE__, Name, Status));
    return NULL;
  }

  Data = AllocatePool (FwCfgSize);
  if (Data == NULL) {
    return NULL;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (FwCfgSize, Data);

  return Data;
}

/**
  Find the power manager related info from ACPI table

  @retval EFI_SUCCESS     Successfully find out all the required information.
  @retval EFI_NOT_FOUND   Failed to find the required info.
**/
STATIC
EFI_STATUS
GetPowerManagerByParseAcpiInfo (
  VOID
  )
{
  EFI_ACPI_5_0_FIXED_ACPI_DESCRIPTION_TABLE     *Fadt       = NULL;
  EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp       = NULL;
  EFI_ACPI_DESCRIPTION_HEADER                   *Xsdt       = NULL;
  EFI_ACPI_DESCRIPTION_HEADER                   *Rsdt       = NULL;
  VOID                                          *AcpiTables = NULL;
  UINT32                                        *Entry32    = NULL;
  UINTN                                         Entry32Num;
  UINT32                                        *Signature = NULL;
  UINTN                                         Idx;

  Rsdp = GetFwCfgData ("etc/acpi/rsdp");
  if (Rsdp == NULL) {
    DEBUG ((DEBUG_ERROR, "%a %d read etc/acpi/rsdp error \n", __func__, __LINE__));
    return EFI_NOT_FOUND;
  }

  AcpiTables = GetFwCfgData ("etc/acpi/tables");
  if (AcpiTables == NULL) {
    DEBUG ((DEBUG_ERROR, "%a %d read etc/acpi/tables error \n", __func__, __LINE__));
    FreePool (Rsdp);
    return EFI_NOT_FOUND;
  }

  Rsdt       = (EFI_ACPI_DESCRIPTION_HEADER *)((UINTN)AcpiTables +  Rsdp->RsdtAddress);
  Entry32    = (UINT32 *)(Rsdt + 1);
  Entry32Num = (Rsdt->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) >> 2;
  for (Idx = 0; Idx < Entry32Num; Idx++) {
    Signature = (UINT32 *)((UINTN)Entry32[Idx] + (UINTN)AcpiTables);
    if (*Signature == EFI_ACPI_5_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
      Fadt = (EFI_ACPI_5_0_FIXED_ACPI_DESCRIPTION_TABLE *)Signature;
      DEBUG ((DEBUG_INFO, "Found Fadt in Rsdt\n"));
      goto Done;
    }
  }

  Xsdt       = (EFI_ACPI_DESCRIPTION_HEADER *)((UINTN)AcpiTables +  Rsdp->XsdtAddress);
  Entry32    = (UINT32 *)(Xsdt + 1);
  Entry32Num = (Xsdt->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) >> 2;
  for (Idx = 0; Idx < Entry32Num; Idx++) {
    Signature = (UINT32 *)((UINTN)Entry32[Idx] + (UINTN)AcpiTables);
    if (*Signature == EFI_ACPI_5_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
      Fadt = (EFI_ACPI_5_0_FIXED_ACPI_DESCRIPTION_TABLE *)Signature;
      DEBUG ((DEBUG_INFO, "Found Fadt in Xsdt\n"));
      goto Done;
    }
  }

  FreePool (Rsdp);
  FreePool (AcpiTables);
  DEBUG ((DEBUG_ERROR, " Fadt Not Found\n"));
  return EFI_NOT_FOUND;

Done:
  mPowerManager.ResetRegAddr        = Fadt->ResetReg.Address;
  mPowerManager.ResetValue          = Fadt->ResetValue;
  mPowerManager.SleepControlRegAddr = Fadt->SleepControlReg.Address;
  mPowerManager.SleepStatusRegAddr  = Fadt->SleepStatusReg.Address;

  FreePool (Rsdp);
  FreePool (AcpiTables);

  return EFI_SUCCESS;
}

/**
  The constructor function to initialize mPowerManager.

  @retval EFI_SUCCESS     Initialize mPowerManager success.
  @retval EFI_NOT_FOUND   Failed to initialize mPowerManager.
**/
EFI_STATUS
EFI_API
ResetSystemLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = GetPowerManagerByParseAcpiInfo ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a:%d\n", __func__, __LINE__));
  }

  ASSERT (mPowerManager.SleepControlRegAddr);
  ASSERT (mPowerManager.SleepStatusRegAddr);
  ASSERT (mPowerManager.ResetRegAddr);

  return Status;
}
