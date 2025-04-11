/** @file

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <IndustryStandard/QemuFwCfg.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HardwareInfoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "VirtMmCommunication.h"

EFI_STATUS
EFIAPI
VirtMmHwFind (
  VOID
  )
{
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  UINTN                 FwCfgSize;
  VOID                  *HardwareInfoBlob;
  LIST_ENTRY            HwInfoList;
  LIST_ENTRY            *HwLink;
  HARDWARE_INFO         *HwInfo;
  UINTN                 Count;
  EFI_STATUS            Status;

  Status = QemuFwCfgFindFile ("etc/hardware-info", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a: etc/hardware-info: exists\n", __func__));
  HardwareInfoBlob = AllocatePool (FwCfgSize);
  if (HardwareInfoBlob == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (FwCfgSize, HardwareInfoBlob);

  InitializeListHead (&HwInfoList);
  Status = CreateHardwareInfoList (
             HardwareInfoBlob,
             FwCfgSize,
             HardwareInfoTypeQemuUefiVars,
             &HwInfoList
             );
  Count = GetHardwareInfoCountByType (
            &HwInfoList,
            HardwareInfoTypeQemuUefiVars,
            sizeof (SIMPLE_INFO)
            );
  if (Count != 1) {
    FreeHardwareInfoList (&HwInfoList);
    FreePool (HardwareInfoBlob);
    return RETURN_NOT_FOUND;
  }

  HwLink = GetFirstHardwareInfoByType (
             &HwInfoList,
             HardwareInfoTypeQemuUefiVars,
             sizeof (SIMPLE_INFO)
             );
  HwInfo        = HARDWARE_INFO_FROM_LINK (HwLink);
  mUefiVarsAddr = HwInfo->Data.SimpleDevice->MmioAddress;
  DEBUG ((DEBUG_INFO, "%a: etc/hardware-info: uefi vars mmio @ 0x%lx\n", __func__, mUefiVarsAddr));

  FreeHardwareInfoList (&HwInfoList);
  FreePool (HardwareInfoBlob);
  return RETURN_SUCCESS;
}
