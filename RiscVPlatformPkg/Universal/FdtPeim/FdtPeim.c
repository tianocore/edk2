/** @file
The module to pass the device tree to DXE via HOB.

Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/RiscVFirmwareContextLib.h>

#include <libfdt.h>

#include <Guid/FdtHob.h>

/**
  The entrypoint of the module, it will pass the FDT via a HOB.

  @param  FileHandle             Handle of the file being invoked.
  @param  PeiServices            Describes the list of possible PEI Services.

  @retval EFI_SUCCESS            The address of FDT is passed in HOB.
          EFI_UNSUPPORTED        Can't locate FDT.
**/
EFI_STATUS
EFIAPI
PeimPassFdt (
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  VOID   *FdtPointer;
  VOID   *Base;
  VOID   *NewBase;
  UINTN  FdtSize;
  UINTN  FdtPages;
  UINT64 *FdtHobData;
  EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT *FirmwareContext;

  FirmwareContext = NULL;
  GetFirmwareContextPointer (&FirmwareContext);

  if (FirmwareContext == NULL) {
    DEBUG((DEBUG_ERROR, "%a: OpenSBI Firmware Context is NULL\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }
  FdtPointer = (VOID *)FirmwareContext->FlattenedDeviceTree;
  if (FdtPointer == NULL) {
    DEBUG((DEBUG_ERROR, "%a: Invalid FDT pointer\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }
  DEBUG((DEBUG_ERROR, "%a: Build FDT HOB - FDT at address: 0x%x \n", __FUNCTION__, FdtPointer));
  Base = FdtPointer;
  ASSERT (Base != NULL);
  ASSERT (fdt_check_header (Base) == 0);

  FdtSize = fdt_totalsize (Base);
  FdtPages = EFI_SIZE_TO_PAGES (FdtSize);
  NewBase = AllocatePages (FdtPages);
  ASSERT (NewBase != NULL);
  fdt_open_into (Base, NewBase, EFI_PAGES_TO_SIZE (FdtPages));

  FdtHobData = BuildGuidHob (&gFdtHobGuid, sizeof *FdtHobData);
  ASSERT (FdtHobData != NULL);
  *FdtHobData = (UINTN)NewBase;

  return EFI_SUCCESS;
}
