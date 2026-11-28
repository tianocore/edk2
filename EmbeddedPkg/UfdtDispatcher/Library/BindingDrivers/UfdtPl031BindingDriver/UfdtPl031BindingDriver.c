/** @file
  PL031 RTC dispatcher.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/rtc/arm,pl031.yaml
**/

#include <PiDxe.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UfdtDispatcherLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/MapMmioLib.h>
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
UfdtPl031MapMemory (
  IN UFDT_RESOURCE_PROTOCOL  *This
  )
{
  UFDT_PL031_RTC_INFO  *Info;
  EFI_STATUS           Status;

  Info = This->Info;
  if (Info->Common.MmioMapped) {
    return EFI_SUCCESS;
  }

  if (Info->Registers.BaseAddress > MAX_UINT32) {
    return EFI_UNSUPPORTED;
  }

  // Match the runtime window used by PL031RealTimeClockLib.
  Status = MapMmioMemory (Info->Registers.BaseAddress, SIZE_4KB, EFI_MEMORY_UC | EFI_MEMORY_XP | EFI_MEMORY_RUNTIME);
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
UfdtPl031SetDevicePcds (
  IN CONST VOID  *Resources
  )
{
  CONST UFDT_PL031_RTC_INFO  *Info;
  EFI_STATUS                 Status;

  Info = Resources;

  // This restriction belongs to PcdPL031RtcBase, not to DT reg decoding.
  if (Info->Registers.BaseAddress > MAX_UINT32) {
    return EFI_UNSUPPORTED;
  }

  Status = PcdSet32S (PcdPL031RtcBase, (UINT32)Info->Registers.BaseAddress);
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
UfdtPl031Configure (
  IN OUT UFDT_BINDING_DEVICE  *Context
  )
{
  UFDT_DEVICE_NODE        *Device;
  UFDT_PL031_RTC_INFO     Info;
  UFDT_RESOURCE_PROTOCOL  Resource;
  EFI_STATUS              Status;

  Device = Context->Device;
  ZeroMem (&Info, sizeof (Info));

  Status = UfdtGetTranslatedReg (Device, 0, &Info.Registers.BaseAddress, &Info.Registers.Length);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Info.Registers.Length < SIZE_4KB) || (Info.Registers.BaseAddress > MAX_UINT32)) {
    return EFI_UNSUPPORTED;
  }

  Resource.Info = AllocateCopyPool (sizeof (Info), &Info);
  if (Resource.Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Resource.MapMmio  = UfdtPl031MapMemory;
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
UfdtPl031SetPcds (
  IN UFDT_BINDING_DRIVER  *Driver
  )
{
  return UfdtSelectResource (Driver, UfdtPl031SetDevicePcds);
}

STATIC CONST CHAR8 *CONST  mPl031Compatible[] = { "arm,pl031", NULL };

///
/// Binding for PL031 RTC nodes.
///
STATIC UFDT_BINDING_DRIVER  mPl031BindingDriver = {
  .Phase           = UfdtPhaseDevice,
  .Name            = "Pl031",
  .Compatible      = mPl031Compatible,
  .Configure       = UfdtPl031Configure,
  .SetPcds         = UfdtPl031SetPcds,
  .Flags           = UFDT_BINDING_FLAG_RESERVE_NODE | UFDT_BINDING_FLAG_SINGLE_CLASS_DEVICE,
  .PresentProtocol = &gEdkiiUfdtPl031RtcInfoProtocolGuid,
  .AbsentProtocol  = &gEdkiiUfdtPl031RtcAbsentProtocolGuid,
  .GenericClass    = &gEdkiiUfdtGenericRtcProtocolGuid,
};

/** Register the PL031 RTC adapter without running discovery.

  @param[in] ImageHandle  Owning dispatcher image; unused by registration.
  @param[in] SystemTable  Firmware system table; unused by registration.

  @retval EFI_SUCCESS  Success.
  @retval EFI_INVALID_PARAMETER Invalid parameter
**/
EFI_STATUS
EFIAPI
UfdtPl031BindingDriverConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UfdtRegisterDriver (&mPl031BindingDriver);
}
