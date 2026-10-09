/** @file
  Publish prepared FDT resources.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UfdtDispatcherLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/UfdtResource.h>

/** Check that publication satisfies the resource and class singleton policies.

  SINGLE_DEVICE checks the resource GUID. SINGLE_CLASS_DEVICE checks the
  generic class protocol, preventing selection across different resource GUIDs.

  @param[in] Driver  Binding descriptor with publication policy and resource GUID.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_UNSUPPORTED        A single-device class already has an instance.
  @retval Other                  Errors returned by LocateProtocol().
**/
STATIC
EFI_STATUS
EFIAPI
UfdtCheckSingleDevice (
  IN UFDT_BINDING_DRIVER  *Driver
  )
{
  VOID        *Existing;
  EFI_STATUS  Status;

  if ((Driver == NULL) || (Driver->PresentProtocol == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Driver->Flags & UFDT_BINDING_FLAG_SINGLE_CLASS_DEVICE) != 0) {
    if (Driver->GenericClass == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    Status = gBS->LocateProtocol (Driver->GenericClass, NULL, &Existing);
    if (!EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }

    if (Status != EFI_NOT_FOUND) {
      return Status;
    }
  }

  if ((Driver->Flags & UFDT_BINDING_FLAG_SINGLE_DEVICE) == 0) {
    return EFI_SUCCESS;
  }

  Status = gBS->LocateProtocol (Driver->PresentProtocol, NULL, &Existing);
  if (!EFI_ERROR (Status)) {
    // Found an installed protocol when none is supposed to be installed.
    return EFI_UNSUPPORTED;
  }

  return (Status == EFI_NOT_FOUND) ? EFI_SUCCESS : Status;
}

/** Publish a device's prepared resource interface using its binding policy.

  Shallow-copy Protocol and publish it after any required PCD setup. Apply
  SINGLE_DEVICE and SINGLE_CLASS_DEVICE policies. The dispatcher reserves
  selected nodes at ReadyToBoot only when RESERVE_NODE is set.
  The caller owns Protocol->Info and must retain it while published.

  @param[in]     Driver        Binding descriptor with publication policy and resource GUID.
  @param[in]     DeviceHandle  Existing device on which to install the interface.
  @param[in]     Protocol      Prepared interface with non-NULL Info.
                               Info must remain valid until ExitBootServices() on success.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_UNSUPPORTED        A single-device class already has an instance.
  @retval EFI_OUT_OF_RESOURCES   Out of memory.
  @retval Other                  Protocol-lookup or protocol-publication errors.
**/
EFI_STATUS
EFIAPI
UfdtPublishResource (
  IN     UFDT_BINDING_DRIVER           *Driver,
  IN     EFI_HANDLE                    DeviceHandle,
  IN     CONST UFDT_RESOURCE_PROTOCOL  *Protocol
  )
{
  UFDT_RESOURCE_PROTOCOL  *Resource;
  EFI_STATUS              Status;

  if ((Driver == NULL) || (DeviceHandle == NULL) ||
      (Protocol == NULL) || (Protocol->Info == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = UfdtCheckSingleDevice (Driver);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Resource = AllocateCopyPool (sizeof (*Resource), Protocol);
  if (Resource == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &DeviceHandle,
                  Driver->PresentProtocol,
                  Resource,
                  Driver->GenericClass,
                  NULL,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Resource);
  }

  return Status;
}

/** Select a prepared device and publish its resource interface.

  Select the first prepared device. Keep an existing SINGLE_DEVICE or
  SINGLE_CLASS_DEVICE selection without repeating PCD writes or publication.
  Abort selection on PCD setup or publication failure. Devices remain
  connected; PCD changes are not rolled back.

  @param[in] Driver   Binding descriptor and complete list of prepared devices.
  @param[in] SetPcds  Callback that sets PCDs from one resource snapshot; must not be NULL.

  @retval EFI_SUCCESS            Success.
  @retval EFI_NOT_FOUND          No prepared device is available.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval Other                  Protocol lookup, PCD setup or publication error.
**/
EFI_STATUS
EFIAPI
UfdtSelectResource (
  IN UFDT_BINDING_DRIVER   *Driver,
  IN UFDT_SET_DEVICE_PCDS  SetPcds
  )
{
  LIST_ENTRY           *Link;
  UFDT_BINDING_DEVICE  *BindingDevice;
  EFI_STATUS           Status;

  Status = UfdtCheckSingleDevice (Driver);
  if (Status == EFI_UNSUPPORTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Link = GetFirstNode (&Driver->Devices);
       !IsNull (&Driver->Devices, Link);
       Link = GetNextNode (&Driver->Devices, Link))
  {
    BindingDevice = BASE_CR (Link, UFDT_BINDING_DEVICE, Link);
    Status        = SetPcds (BindingDevice->Resource.Info);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "UfdtDriver: %a setup failed on %a: %r\n", Driver->Name, BindingDevice->Device->Path, Status));
      return Status;
    }

    Status = UfdtPublishResource (
               Driver,
               BindingDevice->Handle,
               &BindingDevice->Resource
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "UfdtDriver: %a publication failed on %a: %r\n", Driver->Name, BindingDevice->Device->Path, Status));
      return Status;
    }

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}
