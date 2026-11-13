/** @file
  Fixed-clock dispatcher.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/clock/fixed-clock.yaml
**/
#include <Library/BaseLib.h>
#include <Library/UfdtDispatcherLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/UfdtResourceInfo.h>

STATIC UFDT_BINDING_DRIVER  mClockBindingDriver;

/** Parse a "fixed-clock" compatible node.

  @param[in,out] Context  Device identity and output snapshot.

  @retval EFI_SUCCESS           Success.
  @retval EFI_COMPROMISED_DATA  Invalid DTB.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval Other                 Resource-publication errors.
**/
STATIC
EFI_STATUS
EFIAPI
UfdtClockConfigure (
  IN OUT UFDT_BINDING_DEVICE  *Context
  )
{
  UFDT_DEVICE_NODE        *Device;
  EFI_HANDLE              DeviceHandle;
  UFDT_CLOCK_INFO         Info;
  UFDT_RESOURCE_PROTOCOL  Resource;
  EFI_STATUS              Status;
  CONST VOID              *Property;
  INT32                   Size;

  Device       = Context->Device;
  DeviceHandle = Context->Handle;

  // Per specification, #clock-cells = <0>;
  Property = UfdtNodeGetProperty (Device, "#clock-cells", &Size);
  if ((Property == NULL) || (Size != sizeof (UINT32)) || (ReadUnaligned32 (Property) != 0)) {
    return EFI_COMPROMISED_DATA;
  }

  Property = UfdtNodeGetProperty (Device, "clock-frequency", &Size);
  if ((Property == NULL) || (Size != sizeof (UINT32))) {
    return EFI_COMPROMISED_DATA;
  }

  Info.Frequency = SwapBytes32 (ReadUnaligned32 (Property));
  if (Info.Frequency == 0) {
    return EFI_COMPROMISED_DATA;
  }

  Info.Common.MmioMapped = FALSE;
  Resource.MapMmio       = NULL;
  Resource.Info          = AllocateCopyPool (sizeof (Info), &Info);
  if (Resource.Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = UfdtPublishResource (&mClockBindingDriver, DeviceHandle, &Resource);
  if (EFI_ERROR (Status)) {
    FreePool (Resource.Info);
    return Status;
  }

  Context->Resource = Resource;
  return Status;
}

STATIC CONST CHAR8 *CONST  mClockCompatible[] = { "fixed-clock", NULL };

///
/// Binding for fixed-clock nodes.
///
STATIC UFDT_BINDING_DRIVER  mClockBindingDriver = {
  .Phase           = UfdtPhaseClock,
  .Name            = "Clock",
  .Compatible      = mClockCompatible,
  .Configure       = UfdtClockConfigure,
  .Flags           = 0,
  .PresentProtocol = &gEdkiiUfdtClockInfoProtocolGuid,
  .AbsentProtocol  = &gEdkiiUfdtClockAbsentProtocolGuid,
};

/** Register the fixed-clock adapter without running discovery.

  @param[in] ImageHandle  Owning dispatcher image; unused by registration.
  @param[in] SystemTable  Firmware system table; unused by registration.

  @retval EFI_SUCCESS  Success.
  @retval EFI_INVALID_PARAMETER Invalid parameter
**/
EFI_STATUS
EFIAPI
UfdtFixedClockBindingDriverConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UfdtRegisterDriver (&mClockBindingDriver);
}
