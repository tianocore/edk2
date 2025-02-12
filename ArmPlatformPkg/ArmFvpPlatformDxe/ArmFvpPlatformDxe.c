/** @file

  The ArmFvpPlatformDxe performs the platform specific initialization like:
  - It captures the Transfer List HOB and installs the DeviceTree if it's present.

  Copyright (c) 2024, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/FdtClient.h>
#include <Library/ArmTransferListLib.h>
#include <Guid/TlHob.h>
#include <Guid/FdtHob.h>
#include <libfdt.h>

/** Install the appropriate protocol interface.

  @param [in]  ImageHandle  Handle for this image.
  @param [in]  DeviceTreeBase  Pointer to the Device Tree base.

  @retval EFI_SUCCESS             Success.
  @retval EFI_OUT_OF_RESOURCES    There was not enough memory to install the
                                  protocols.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.

**/
STATIC
EFI_STATUS
PlatformHasValidDt (
  IN EFI_HANDLE  ImageHandle
  )
{
  // Expose the Device Tree.
  return gBS->InstallProtocolInterface (
                                        &ImageHandle,
                                        &gEdkiiPlatformHasDeviceTreeGuid,
                                        EFI_NATIVE_INTERFACE,
                                        NULL
                                        );
}

/** Entry point for ArmFvpPlatform Dxe

  @param [in]  ImageHandle  Handle for this image.
  @param [in]  SystemTable  Pointer to the EFI system table.

  @retval EFI_SUCCESS             Success.
  @retval EFI_OUT_OF_RESOURCES    There was not enough memory to install the
                                  protocols.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.

**/
EFI_STATUS
EFIAPI
ArmFvpPlatformDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  VOID        *Hob;
  VOID        *TransferListBase;
  VOID        *DeviceTreeBase;

  Hob = GetFirstGuidHob (&gTlHobGuid);
  if ((Hob == NULL) || (GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (UINT64))) {
    return EFI_NOT_FOUND;
  }

  TransferListBase = (VOID *)(UINTN)*(UINT64 *)GET_GUID_HOB_DATA (Hob);
  if (TlCheckHeader (TransferListBase) != TL_OPS_INVALID) {
    DEBUG ((DEBUG_INFO|DEBUG_LOAD, "Valid TL found\n"));
  } else {
    DEBUG (
           (
            DEBUG_ERROR,
            "%a: No Transfer List found @ 0x%p\n",
            __func__,
            TransferListBase
           )
           );
  }

  Hob = GetFirstGuidHob (&gFdtHobGuid);
  if ((Hob == NULL) || (GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (UINT64))) {
    return EFI_NOT_FOUND;
  }

  DeviceTreeBase = (VOID *)(UINTN)*(UINT64 *)GET_GUID_HOB_DATA (Hob);
  if ((DeviceTreeBase == NULL) || (fdt_check_header (DeviceTreeBase) != 0)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get Device Tree data\n", __func__));
    return EFI_NOT_FOUND;
  }

  Status = PlatformHasValidDt (ImageHandle);

  return Status;
}
