/** @file
  Architectural timer dispatcher.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/timer/arm,arch_timer.yaml
**/

#include <PiDxe.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UfdtDispatcherLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Protocol/UfdtResourceInfo.h>

/** Set the legacy PCDs from the device snapshot.

  @param[in] Resources  Parsed device resources.

  @retval EFI_SUCCESS  Success.
  @retval Other        PCD write errors.
**/
STATIC
EFI_STATUS
EFIAPI
UfdtTimerSetDevicePcds (
  IN CONST VOID  *Resources
  )
{
  CONST UFDT_TIMER_INFO  *Info;
  EFI_STATUS             Status;

  Info = Resources;

  Status = PcdSet32S (PcdArmArchTimerSecIntrNum, Info->SecurePhysicalInterrupt);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PcdSet32S (PcdArmArchTimerIntrNum, Info->NonSecurePhysicalInterrupt);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PcdSet32S (PcdArmArchTimerVirtIntrNum, Info->VirtualInterrupt);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PcdSet32S (PcdArmArchTimerHypIntrNum, Info->HypervisorPhysicalInterrupt);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PcdSet32S (PcdArmArchTimerHypVirtIntrNum, Info->HypervisorVirtualInterrupt);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/** Parse timer interrupts and retain their snapshot for deferred selection.

  @param[in,out] Context  Device identity and output snapshot.

  @retval EFI_SUCCESS           Success.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval Other                 Interrupt parsing errors.
**/
EFI_STATUS
EFIAPI
UfdtTimerConfigure (
  IN OUT UFDT_BINDING_DEVICE  *Context
  )
{
  UFDT_DEVICE_NODE        *Device;
  UFDT_TIMER_INFO         Info;
  UFDT_RESOURCE_PROTOCOL  Resource;
  EFI_STATUS              Status;
  UINT32                  Index;
  UINT32                  *Interrupts[5];

  Device = Context->Device;
  ZeroMem (&Info, sizeof (Info));
  Interrupts[0] = &Info.SecurePhysicalInterrupt;
  Interrupts[1] = &Info.NonSecurePhysicalInterrupt;
  Interrupts[2] = &Info.VirtualInterrupt;
  Interrupts[3] = &Info.HypervisorPhysicalInterrupt;
  Interrupts[4] = &Info.HypervisorVirtualInterrupt;

  for (Index = 0; Index < ARRAY_SIZE (Interrupts); Index++) {
    Status = UfdtGetInterruptId (Device, Index, Interrupts[Index]);
    if ((Index >= 3) && (Status == EFI_NOT_FOUND)) {
      continue;
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Architectural timer interrupts must be PPIs.
    if ((*Interrupts[Index] < FDT_GIC_PPI_BASE) ||
        (*Interrupts[Index] >= FDT_GIC_PPI_BASE + FDT_GIC_PPI_COUNT))
    {
      return EFI_COMPROMISED_DATA;
    }
  }

  Resource.MapMmio = NULL;
  Resource.Info    = AllocateCopyPool (sizeof (Info), &Info);
  if (Resource.Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

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
UfdtTimerSetPcds (
  IN UFDT_BINDING_DRIVER  *Driver
  )
{
  return UfdtSelectResource (Driver, UfdtTimerSetDevicePcds);
}

STATIC CONST CHAR8 *CONST  mTimerCompatible[] = { "arm,armv8-timer", "arm,armv7-timer", NULL };

///
/// Binding for Arm architectural timer nodes.
///
STATIC UFDT_BINDING_DRIVER  mTimerBindingDriver = {
  .Phase           = UfdtPhaseDevice,
  .Name            = "Timer",
  .Compatible      = mTimerCompatible,
  .Configure       = UfdtTimerConfigure,
  .SetPcds         = UfdtTimerSetPcds,
  .Flags           = UFDT_BINDING_FLAG_SINGLE_DEVICE,
  .PresentProtocol = &gEdkiiUfdtTimerInfoProtocolGuid,
  .AbsentProtocol  = &gEdkiiUfdtTimerAbsentProtocolGuid,
};

/** Register the architectural timer adapter without running discovery.

  @param[in] ImageHandle  Owning dispatcher image; unused by registration.
  @param[in] SystemTable  Firmware system table; unused by registration.

  @retval EFI_SUCCESS  Success.
  @retval EFI_INVALID_PARAMETER Invalid parameter
**/
EFI_STATUS
EFIAPI
UfdtTimerBindingDriverConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UfdtRegisterDriver (&mTimerBindingDriver);
}
