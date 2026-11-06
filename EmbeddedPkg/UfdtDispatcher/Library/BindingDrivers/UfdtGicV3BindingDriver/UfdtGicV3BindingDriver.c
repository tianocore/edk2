/** @file
  GICv3 dispatcher.

  Copyright (c) 2015 - 2016, Linaro Ltd. All rights reserved.<BR>
  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic-v3.yaml
**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UfdtDispatcherLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MapMmioLib.h>
#include <Library/PcdLib.h>
#include <Protocol/UfdtResourceInfo.h>
#include <Protocol/UfdtGenericIntC.h>

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
UfdtGicV3MapMemory (
  IN UFDT_RESOURCE_PROTOCOL  *This
  )
{
  UFDT_GIC_V3_INFO  *Info;
  EFI_STATUS        Status;

  Info = This->Info;
  if (Info->Common.MmioMapped) {
    return EFI_SUCCESS;
  }

  Status = MapMmioMemory (Info->Distributor.BaseAddress, Info->Distributor.Length, EFI_MEMORY_UC | EFI_MEMORY_XP);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = MapMmioMemory (Info->Redistributors.BaseAddress, Info->Redistributors.Length, EFI_MEMORY_UC | EFI_MEMORY_XP);
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
UfdtGicV3SetDevicePcds (
  IN CONST VOID  *Resources
  )
{
  CONST UFDT_GIC_V3_INFO  *Info;
  EFI_STATUS              Status;

  Info = Resources;

  Status = PcdSet64S (PcdGicDistributorBase, Info->Distributor.BaseAddress);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return PcdSet64S (PcdGicRedistributorsBase, Info->Redistributors.BaseAddress);
}

/** Parse one device, retaining its snapshot and mapping callback for selection.

  @param[in,out] Context  Device identity and output snapshot.

  @retval EFI_SUCCESS           Success.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval Other                 Parsing or resource validation errors.
**/
EFI_STATUS
EFIAPI
UfdtGicV3Configure (
  IN OUT UFDT_BINDING_DEVICE  *Context
  )
{
  UFDT_DEVICE_NODE        *Device;
  UFDT_GIC_V3_INFO        Info;
  UFDT_RESOURCE_PROTOCOL  Resource;
  EFI_STATUS              Status;
  INT32                   Size;
  CONST VOID              *Property;

  Device = Context->Device;
  ZeroMem (&Info, sizeof (Info));

  // ArmVirt assumes that the first GICR region contains the boot CPU.
  // Additional regions may describe other CPUs and can be ignored.
  Property = UfdtNodeGetProperty (Device, "#redistributor-regions", &Size);
  if ((Property != NULL) && ((Size != sizeof (UINT32)) || (SwapBytes32 (ReadUnaligned32 (Property)) == 0))) {
    return EFI_UNSUPPORTED;
  }

  // ArmGicDxe determines the redistributor stride from hardware.
  if (UfdtNodeGetProperty (Device, "redistributor-stride", NULL) != NULL) {
    return EFI_UNSUPPORTED;
  }

  Status = UfdtGetTranslatedReg (Device, 0, &Info.Distributor.BaseAddress, &Info.Distributor.Length);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = UfdtGetTranslatedReg (Device, 1, &Info.Redistributors.BaseAddress, &Info.Redistributors.Length);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Resource.Info = AllocateCopyPool (sizeof (Info), &Info);
  if (Resource.Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Resource.MapMmio  = UfdtGicV3MapMemory;
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
UfdtGicV3SetPcds (
  IN UFDT_BINDING_DRIVER  *Driver
  )
{
  return UfdtSelectResource (Driver, UfdtGicV3SetDevicePcds);
}

STATIC CONST CHAR8 *CONST  mGicV3Compatible[] = { "arm,gic-v3", NULL };

///
/// Binding for GICv3 interrupt controller nodes.
///
STATIC UFDT_BINDING_DRIVER  mGicV3BindingDriver = {
  .Phase           = UfdtPhaseInterruptController,
  .Name            = "GicV3",
  .Compatible      = mGicV3Compatible,
  .Configure       = UfdtGicV3Configure,
  .SetPcds         = UfdtGicV3SetPcds,
  .Flags           = UFDT_BINDING_FLAG_SINGLE_CLASS_DEVICE,
  .PresentProtocol = &gEdkiiUfdtGicV3InfoProtocolGuid,
  .AbsentProtocol  = &gEdkiiUfdtGicV3AbsentProtocolGuid,
  .GenericClass    = &gEdkiiUfdtGenericIntCProtocolGuid,
};

/** Register the GIC adapter without running discovery.

  @param[in] ImageHandle  Owning dispatcher image; unused by registration.
  @param[in] SystemTable  Firmware system table; unused by registration.

  @retval EFI_SUCCESS  Success.
  @retval EFI_INVALID_PARAMETER Invalid parameter
**/
EFI_STATUS
EFIAPI
UfdtGicV3BindingDriverConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UfdtRegisterDriver (&mGicV3BindingDriver);
}
