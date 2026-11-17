/** @file
  SBSA generic watchdog dispatcher.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/watchdog/arm,sbsa-gwdt.yaml
**/

#include <PiDxe.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UfdtDispatcherLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MapMmioLib.h>
#include <Library/PcdLib.h>
#include <Protocol/UfdtResourceInfo.h>

/** Map the MMIO regions required by the device consumer.

  Return immediately if already mapped.
  Record success only after all required regions have been mapped.

  @param[in] This  Resource interface containing the parsed device resources.

  @retval EFI_SUCCESS  Success.
  @retval Other        Errors from MapMmioMemory().
**/
STATIC
EFI_STATUS
EFIAPI
UfdtWatchdogMapMemory (
  IN UFDT_RESOURCE_PROTOCOL  *This
  )
{
  UFDT_WATCHDOG_INFO  *Info;
  EFI_STATUS          Status;

  Info = This->Info;
  if (Info->Common.MmioMapped) {
    return EFI_SUCCESS;
  }

  Status = MapMmioMemory (Info->ControlFrame.BaseAddress, Info->ControlFrame.Length, EFI_MEMORY_UC | EFI_MEMORY_XP);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = MapMmioMemory (Info->RefreshFrame.BaseAddress, Info->RefreshFrame.Length, EFI_MEMORY_UC | EFI_MEMORY_XP);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Info->Common.MmioMapped = TRUE;
  return EFI_SUCCESS;
}

/** Set the legacy PCDs from the device snapshot.

  @param[in] Resources  Parsed device resources.

  @retval EFI_SUCCESS  Success.
  @retval Other        PCD write errors.
**/
STATIC
EFI_STATUS
EFIAPI
UfdtWatchdogSetDevicePcds (
  IN CONST VOID  *Resources
  )
{
  CONST UFDT_WATCHDOG_INFO  *Info;
  EFI_STATUS                Status;

  Info = Resources;

  Status = PcdSet64S (PcdGenericWatchdogControlBase, Info->ControlFrame.BaseAddress);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PcdSet64S (PcdGenericWatchdogRefreshBase, Info->RefreshFrame.BaseAddress);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PcdSet32S (PcdGenericWatchdogEl2IntrNum, Info->Interrupt);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/** Parse one device, retaining its snapshot and mapping callback for selection.

  @param[in,out] Context  Device identity and output snapshot.

  @retval EFI_SUCCESS           Success.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval Other                 Parsing or resource validation errors.
**/
EFI_STATUS
EFIAPI
UfdtWatchdogConfigure (
  IN OUT UFDT_BINDING_DEVICE  *Context
  )
{
  UFDT_DEVICE_NODE        *Device;
  UFDT_WATCHDOG_INFO      Info;
  UFDT_RESOURCE_PROTOCOL  Resource;
  EFI_STATUS              Status;

  Device = Context->Device;
  ZeroMem (&Info, sizeof (Info));

  Status = UfdtGetTranslatedReg (Device, 0, &Info.ControlFrame.BaseAddress, &Info.ControlFrame.Length);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = UfdtGetTranslatedReg (Device, 1, &Info.RefreshFrame.BaseAddress, &Info.RefreshFrame.Length);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Info.ControlFrame.Length < SIZE_4KB) || (Info.RefreshFrame.Length < SIZE_4KB)) {
    return EFI_COMPROMISED_DATA;
  }

  Status = UfdtGetInterruptId (Device, 0, &Info.Interrupt);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // The SBSA watchdog interrupt must be an SPI.
  if ((Info.Interrupt < FDT_GIC_SPI_BASE) ||
      (Info.Interrupt >= FDT_GIC_SPI_BASE + FDT_GIC_SPI_COUNT))
  {
    return EFI_COMPROMISED_DATA;
  }

  Resource.Info = AllocateCopyPool (sizeof (Info), &Info);
  if (Resource.Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Resource.MapMmio  = UfdtWatchdogMapMemory;
  Context->Resource = Resource;
  return EFI_SUCCESS;
}

/** Select a prepared device, set its PCDs and publish readiness.

  Select the first prepared device. Preserve an existing selection without
  repeating PCD writes. Abort on PCD setup or publication failure without
  trying later candidates. Devices remain connected.

  @param[in] Driver  Binding descriptor and complete list of prepared devices.

  @retval EFI_SUCCESS            Success.
  @retval EFI_NOT_FOUND          No prepared device is available.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval Other                  Protocol lookup, PCD setup or publication error.
**/
STATIC
EFI_STATUS
EFIAPI
UfdtWatchdogSetPcds (
  IN UFDT_BINDING_DRIVER  *Driver
  )
{
  return UfdtSelectResource (Driver, UfdtWatchdogSetDevicePcds);
}

STATIC CONST CHAR8 *CONST  mWatchdogCompatible[] = { "arm,sbsa-gwdt", NULL };

///
/// Binding for SBSA generic watchdog nodes.
///
STATIC UFDT_BINDING_DRIVER  mWatchdogBindingDriver = {
  .Phase           = UfdtPhaseDevice,
  .Name            = "Watchdog",
  .Compatible      = mWatchdogCompatible,
  .Configure       = UfdtWatchdogConfigure,
  .SetPcds         = UfdtWatchdogSetPcds,
  .Flags           = UFDT_BINDING_FLAG_SINGLE_DEVICE,
  .PresentProtocol = &gEdkiiUfdtWatchdogInfoProtocolGuid,
  .AbsentProtocol  = &gEdkiiUfdtWatchdogAbsentProtocolGuid,
};

/** Register the SBSA watchdog adapter without running discovery.

  @param[in] ImageHandle  Owning dispatcher image; unused by registration.
  @param[in] SystemTable  Firmware system table; unused by registration.

  @retval EFI_SUCCESS  Success.
  @retval EFI_INVALID_PARAMETER Invalid parameter
**/
EFI_STATUS
EFIAPI
UfdtWatchdogBindingDriverConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UfdtRegisterDriver (&mWatchdogBindingDriver);
}
