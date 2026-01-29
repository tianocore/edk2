/** @file
  IPMI FRU Driver.

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/IpmiCommandLib.h>
#include <IndustryStandard/Ipmi.h>

/*++

Routine Description:

  Initialize SM Redirection Fru Layer

Arguments:

  ImageHandle - ImageHandle of the loaded driver
  SystemTable - Pointer to the System Table

Returns:

  EFI_STATUS

--*/
EFI_STATUS
EFIAPI
InitializeFru (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                                 Status;
  IPMI_GET_DEVICE_ID_RESPONSE                ControllerInfo;
  IPMI_GET_FRU_INVENTORY_AREA_INFO_REQUEST   GetFruInventoryAreaInfoRequest;
  IPMI_GET_FRU_INVENTORY_AREA_INFO_RESPONSE  GetFruInventoryAreaInfoResponse;

  //
  //  Get all the SDR Records from BMC and retrieve the Record ID from the structure for future use.
  //
  Status = IpmiGetDeviceId (&ControllerInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "!!! IpmiFru  IpmiGetDeviceId Status=%x\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_ERROR, "!!! IpmiFru  FruInventorySupport %x\n", ControllerInfo.DeviceSupport.Bits.FruInventorySupport));

  if (ControllerInfo.DeviceSupport.Bits.FruInventorySupport) {
    GetFruInventoryAreaInfoRequest.DeviceId = 0;
    Status                                  = IpmiGetFruInventoryAreaInfo (&GetFruInventoryAreaInfoRequest, &GetFruInventoryAreaInfoResponse);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "!!! IpmiFru  IpmiGetFruInventoryAreaInfo Status=%x\n", Status));
      return Status;
    }

    DEBUG ((DEBUG_ERROR, "!!! IpmiFru  InventoryAreaSize=%x\n", GetFruInventoryAreaInfoResponse.InventoryAreaSize));
  }

  return EFI_SUCCESS;
}
