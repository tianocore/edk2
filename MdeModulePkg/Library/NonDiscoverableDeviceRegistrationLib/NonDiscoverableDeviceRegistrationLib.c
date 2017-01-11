/** @file
  Copyright (c) 2016, Linaro, Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Guid/NonDiscoverableDevice.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/NonDiscoverableDevice.h>

/**
  Get Guid form the type of non-discoverable device.

  @param[in]  Type    The type of non-discoverable device.

  @retval   Return the Guid.

**/
STATIC
CONST EFI_GUID *
GetGuidFromType (
  IN  NON_DISCOVERABLE_DEVICE_TYPE  Type
  )
{
  switch (Type) {
  case NonDiscoverableDeviceTypeAhci:
    return &gEdkiiNonDiscoverableAhciDeviceGuid;

  case NonDiscoverableDeviceTypeAmba:
    return &gEdkiiNonDiscoverableAmbaDeviceGuid;

  case NonDiscoverableDeviceTypeEhci:
    return &gEdkiiNonDiscoverableEhciDeviceGuid;

  case NonDiscoverableDeviceTypeNvme:
    return &gEdkiiNonDiscoverableNvmeDeviceGuid;

  case NonDiscoverableDeviceTypeOhci:
    return &gEdkiiNonDiscoverableOhciDeviceGuid;

  case NonDiscoverableDeviceTypeSdhci:
    return &gEdkiiNonDiscoverableSdhciDeviceGuid;

  case NonDiscoverableDeviceTypeUfs:
    return &gEdkiiNonDiscoverableUfsDeviceGuid;

  case NonDiscoverableDeviceTypeUhci:
    return &gEdkiiNonDiscoverableUhciDeviceGuid;

  case NonDiscoverableDeviceTypeXhci:
    return &gEdkiiNonDiscoverableXhciDeviceGuid;

  default:
    return NULL;
  }
}

#pragma pack (1)
typedef struct {
  VENDOR_DEVICE_PATH                  Vendor;
  UINT64                              BaseAddress;
  UINT8                               ResourceType;
  EFI_DEVICE_PATH_PROTOCOL            End;
} NON_DISCOVERABLE_DEVICE_PATH;
#pragma pack ()

/**
  Register a non-discoverable MMIO device.

  @param[in]      Type                The type of non-discoverable device
  @param[in]      DmaType             Whether the device is DMA coherent
  @param[in]      InitFunc            Initialization routine to be invoked when
                                      the device is enabled
  @param[in,out]  Handle              The handle onto which to install the
                                      non-discoverable device protocol.
                                      If Handle is NULL or *Handle is NULL, a
                                      new handle will be allocated.
  @param[in]      NumMmioResources    The number of UINTN base/size pairs that
                                      follow, each describing an MMIO region
                                      owned by the device
  @param[in]  ...                     The variable argument list which contains the
                                      info about MmioResources.

  @retval EFI_SUCCESS                 The registration succeeded.
  @retval EFI_INVALID_PARAMETER       An invalid argument was given
  @retval Other                       The registration failed.

**/
EFI_STATUS
EFIAPI
RegisterNonDiscoverableMmioDevice (
  IN      NON_DISCOVERABLE_DEVICE_TYPE      Type,
  IN      NON_DISCOVERABLE_DEVICE_DMA_TYPE  DmaType,
  IN      NON_DISCOVERABLE_DEVICE_INIT      InitFunc,
  IN OUT  EFI_HANDLE                        *Handle OPTIONAL,
  IN      UINTN                             NumMmioResources,
  ...
  )
{
  NON_DISCOVERABLE_DEVICE             *Device;
  NON_DISCOVERABLE_DEVICE_PATH        *DevicePath;
  EFI_HANDLE                          LocalHandle;
  EFI_STATUS                          Status;
  UINTN                               AllocSize;
  UINTN                               Index;
  VA_LIST                             Args;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR   *Desc;
  EFI_ACPI_END_TAG_DESCRIPTOR         *End;
  UINTN                               Base, Size;

  if (Type >= NonDiscoverableDeviceTypeMax ||
      DmaType >= NonDiscoverableDeviceDmaTypeMax ||
      NumMmioResources == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (Handle == NULL) {
    Handle = &LocalHandle;
    LocalHandle = NULL;
  }

  AllocSize = sizeof *Device +
              NumMmioResources * sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) +
              sizeof (EFI_ACPI_END_TAG_DESCRIPTOR);
  Device = (NON_DISCOVERABLE_DEVICE *)AllocateZeroPool (AllocSize);
  if (Device == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Device->Type = GetGuidFromType (Type);
  ASSERT (Device->Type != NULL);

  Device->DmaType = DmaType;
  Device->Initialize = InitFunc;
  Device->Resources = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)(Device + 1);

  VA_START (Args, NumMmioResources);
  for (Index = 0; Index < NumMmioResources; Index++) {
    Desc = &Device->Resources [Index];
    Base = VA_ARG (Args, UINTN);
    Size = VA_ARG (Args, UINTN);

    Desc->Desc                  = ACPI_ADDRESS_SPACE_DESCRIPTOR;
    Desc->Len                   = sizeof *Desc - 3;
    Desc->AddrRangeMin          = Base;
    Desc->AddrLen               = Size;
    Desc->AddrRangeMax          = Base + Size - 1;
    Desc->ResType               = ACPI_ADDRESS_SPACE_TYPE_MEM;
    Desc->AddrSpaceGranularity  = ((EFI_PHYSICAL_ADDRESS)Base + Size > SIZE_4GB) ? 64 : 32;
    Desc->AddrTranslationOffset = 0;
  }
  VA_END (Args);

  End = (EFI_ACPI_END_TAG_DESCRIPTOR *)&Device->Resources [NumMmioResources];

  End->Desc     = ACPI_END_TAG_DESCRIPTOR;
  End->Checksum = 0;

  DevicePath = (NON_DISCOVERABLE_DEVICE_PATH *)CreateDeviceNode (
                                                 HARDWARE_DEVICE_PATH,
                                                 HW_VENDOR_DP,
                                                 sizeof (*DevicePath));
  if (DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeDevice;
  }

  CopyGuid (&DevicePath->Vendor.Guid, &gEdkiiNonDiscoverableDeviceProtocolGuid);

  //
  // Use the base address and type of the first region to
  // make the device path unique
  //
  DevicePath->BaseAddress = Device->Resources [0].AddrRangeMin;
  DevicePath->ResourceType = Device->Resources [0].ResType;

  SetDevicePathNodeLength (&DevicePath->Vendor,
    sizeof (*DevicePath) - sizeof (DevicePath->End));
  SetDevicePathEndNode (&DevicePath->End);

  Status = gBS->InstallMultipleProtocolInterfaces (Handle,
                  &gEdkiiNonDiscoverableDeviceProtocolGuid, Device,
                  &gEfiDevicePathProtocolGuid, DevicePath,
                  NULL);
  if (EFI_ERROR (Status)) {
    goto FreeDevicePath;
  }
  return EFI_SUCCESS;

FreeDevicePath:
  FreePool (DevicePath);

FreeDevice:
  FreePool (Device);

  return Status;
}
