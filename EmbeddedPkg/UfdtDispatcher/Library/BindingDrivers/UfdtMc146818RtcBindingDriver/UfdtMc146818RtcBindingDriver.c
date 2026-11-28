/** @file
  MC146818 RTC dispatcher.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/rtc/rtc-cmos.yaml
**/

#include <PiDxe.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UfdtDispatcherLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MapMmioLib.h>
#include <Library/PcdLib.h>
#include <Protocol/UfdtResourceInfo.h>
#include <Protocol/UfdtGenericRtc.h>

#include <Library/UefiBootServicesTableLib.h>

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
UfdtMc146818RtcMapMemory (
  IN UFDT_RESOURCE_PROTOCOL  *This
  )
{
  UFDT_MC146818_RTC_INFO  *Info;
  EFI_STATUS              Status;

  Info = This->Info;
  if (Info->Common.MmioMapped) {
    return EFI_SUCCESS;
  }

  Status = MapMmioMemory (Info->Registers.BaseAddress, Info->Registers.Length, EFI_MEMORY_UC | EFI_MEMORY_XP | EFI_MEMORY_RUNTIME);
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
UfdtMc146818RtcSetDevicePcds (
  IN CONST VOID  *Resources
  )
{
  CONST UFDT_MC146818_RTC_INFO  *Info;
  EFI_STATUS                    Status;

  Info = Resources;

  Status = PcdSet64S (PcdRtcIndexRegister64, Info->IndexRegister);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PcdSet64S (PcdRtcTargetRegister64, Info->TargetRegister);
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
UfdtMc146818RtcConfigure (
  IN OUT UFDT_BINDING_DEVICE  *Context
  )
{
  UFDT_DEVICE_NODE        *Device;
  UFDT_MC146818_RTC_INFO  Info;
  UFDT_RESOURCE_PROTOCOL  Resource;
  EFI_STATUS              Status;

  Device = Context->Device;
  ZeroMem (&Info, sizeof (Info));

  Status = UfdtGetTranslatedReg (Device, 0, &Info.Registers.BaseAddress, &Info.Registers.Length);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Info.Registers.Length < 2) {
    return EFI_UNSUPPORTED;
  }

  Info.IndexRegister  = Info.Registers.BaseAddress;
  Info.TargetRegister = Info.Registers.BaseAddress + 1;

  Resource.Info = AllocateCopyPool (sizeof (Info), &Info);
  if (Resource.Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Resource.MapMmio  = UfdtMc146818RtcMapMemory;
  Context->Resource = Resource;
  return EFI_SUCCESS;
}

/** Select a prepared device, set its PCDs and publish readiness.

  Select the first prepared device unless a device of this class is already
  selected. Preserve an existing selection without repeating PCD writes.
  Abort on PCD setup or publication failure; candidates remain connected.

  @param[in] Driver  Binding descriptor and complete list of prepared devices.

  @retval EFI_SUCCESS            Success.
  @retval EFI_NOT_FOUND          No prepared device is available.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval Other                  Protocol lookup, PCD setup or publication error.
**/
STATIC
EFI_STATUS
EFIAPI
UfdtMc146818RtcSetPcds (
  IN UFDT_BINDING_DRIVER  *Driver
  )
{
  return UfdtSelectResource (Driver, UfdtMc146818RtcSetDevicePcds);
}

STATIC CONST CHAR8 *CONST  mMc146818RtcCompatible[] = { "motorola,mc146818", NULL };

///
/// Binding for MC146818 RTC nodes.
///
STATIC UFDT_BINDING_DRIVER  mMc146818RtcBindingDriver = {
  .Phase           = UfdtPhaseDevice,
  .Name            = "Mc146818Rtc",
  .Compatible      = mMc146818RtcCompatible,
  .Configure       = UfdtMc146818RtcConfigure,
  .SetPcds         = UfdtMc146818RtcSetPcds,
  .Flags           = UFDT_BINDING_FLAG_RESERVE_NODE | UFDT_BINDING_FLAG_SINGLE_CLASS_DEVICE,
  .PresentProtocol = &gEdkiiUfdtMc146818RtcInfoProtocolGuid,
  .AbsentProtocol  = &gEdkiiUfdtMc146818RtcAbsentProtocolGuid,
  .GenericClass    = &gEdkiiUfdtGenericRtcProtocolGuid,
};

/** Register the MC146818 RTC adapter without running discovery.

  @param[in] ImageHandle  Owning dispatcher image; unused by registration.
  @param[in] SystemTable  Firmware system table; unused by registration.

  @retval EFI_SUCCESS  Success.
  @retval EFI_INVALID_PARAMETER Invalid parameter
**/
EFI_STATUS
EFIAPI
UfdtMc146818RtcBindingDriverConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UfdtRegisterDriver (&mMc146818RtcBindingDriver);
}
