/** @file
  RISC-V Flattened Device Tree DXE module

  The Linux booting protocol on RISC-V requires the id of the booting hart to
  be passed as a0. Therefore the EFISTUB needs to get this information. Because
  it runs in S-Mode, it cannot get this information from mhartid. Instead we
  insert the id into the device tree, that the EFIFSTUB can read from the config table.

  Copyright (c) 2021-2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <libfdt.h>

/**
  Fix up the device tree with booting hartid for the kernel

  @param  DtbBlob The device tree. Is extended to fit the hart id.
  @param  BootingHartId The boot hart ID.

  @retval EFI_SUCCESS           The device tree was success fixed up with the hart id.
  @retval EFI_OUT_OF_RESOURCES  There is not enough memory available to complete the operation.
**/
EFI_STATUS
EFIAPI
FixDtb (
  IN OUT VOID   *DtbBlob,
  IN     UINTN  BootingHartId
  )
{
  fdt32_t  Size;
  UINT32   ChosenOffset, Err;

  DEBUG ((
    DEBUG_INFO,
    "Fixing up device tree with boot hart id: %d\n",
    BootingHartId
    ));

  Size = fdt_totalsize (DtbBlob);
  Err  = fdt_open_into (DtbBlob, DtbBlob, Size + 32);
  if (Err < 0) {
    DEBUG ((
      DEBUG_ERROR,
      "Device Tree can't be expanded to accommodate new node\n",
      __FUNCTION__
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  ChosenOffset = fdt_path_offset (DtbBlob, "/chosen");
  fdt_setprop_u32 (DtbBlob, ChosenOffset, "boot-hartid", BootingHartId);

  return EFI_SUCCESS;
}

/**
  Install the FDT passed in HOB into EFI system configuration table.

  @retval EFI_SUCCESS          Successfully installed fixed up FDT in config table.
  @retval EFI_NOT_FOUND        Did not find FDT HOB.
  @retval EFI_OUT_OF_RESOURCES There is not enough memory available to complete the operation.
**/
EFI_STATUS
EFIAPI
InstallFdtFromHob (
  VOID
  )
{
  EFI_STATUS         Status;
  EFI_HOB_GUID_TYPE  *GuidHob;
  VOID               *DataInHob;
  UINTN              DataSize;

  GuidHob = GetFirstGuidHob (&gFdtHobGuid);
  if (GuidHob == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to find RISC-V DTB Hob\n",
      __FUNCTION__
      ));
    return EFI_NOT_FOUND;
  }

  DataInHob = (VOID *)*((UINTN *)GET_GUID_HOB_DATA (GuidHob));
  DataSize  = GET_GUID_HOB_DATA_SIZE (GuidHob);

  Status = FixDtb (DataInHob, PcdGet32 (PcdBootHartId));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->InstallConfigurationTable (&gFdtTableGuid, DataInHob);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: failed to install FDT configuration table\n",
      __FUNCTION__
      ));
  }

  return Status;
}

/**
  Install the FDT from the HOB into the EFI system configuration table.

  @param ImageHandle     Image handle of this driver.
  @param SystemTable     Pointer to the System Table.

  @retval EFI_SUCCESS    FDT successfully installed into config table.
  @retval EFI_NOT_FOUND  Did not find FDT HOB.
  @retval EFI_OUT_OF_RESOURCES There is not enough memory available to complete the operation.

**/
EFI_STATUS
EFIAPI
InstallFdt (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = InstallFdtFromHob ();

  return Status;
}
