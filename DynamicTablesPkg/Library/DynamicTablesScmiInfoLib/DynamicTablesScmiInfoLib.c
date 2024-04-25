/** @file
  Arm SCMI Info Library.

  Copyright (c) 2022 - 2023, Arm Limited. All rights reserved.<BR>

  Arm Functional Fixed Hardware Specification:
  - https://developer.arm.com/documentation/den0048/latest/

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/AcpiLib.h>
#include <Library/DynamicTablesScmiInfoLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/ArmScmi.h>
#include <Protocol/ArmScmiPerformanceProtocol.h>

/** Arm FFH registers

  Cf. Arm Functional Fixed Hardware Specification
  s3.2 Performance management and Collaborative Processor Performance Control
*/
#define ARM_FFH_DELIVERED_PERF_COUNTER_REGISTER  0x0
#define ARM_FFH_REFERENCE_PERF_COUNTER_REGISTER  0x1

/// Arm SCMI performance protocol.
STATIC SCMI_PERFORMANCE_PROTOCOL  *ScmiPerfProtocol;

/** Arm SCMI Info Library constructor.

  @param  ImageHandle   Image of the loaded driver.
  @param  SystemTable   Pointer to the System Table.

  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Device error.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not Found
  @retval EFI_TIMEOUT             Timeout.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
EFI_STATUS
EFIAPI
DynamicTablesScmiInfoLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINT32      Version;

  Status = gBS->LocateProtocol (
                  &gArmScmiPerformanceProtocolGuid,
                  NULL,
                  (VOID **)&ScmiPerfProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ScmiPerfProtocol->GetVersion (ScmiPerfProtocol, &Version);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // FastChannels were added in SCMI v2.0 spec.
  if (Version < PERFORMANCE_PROTOCOL_VERSION_V2) {
    DEBUG ((
      DEBUG_ERROR,
      "DynamicTablesScmiInfoLib requires SCMI version > 2.0\n"
      ));
    return EFI_UNSUPPORTED;
  }

  return Status;
}

/** Get the OPPs/performance states of a power domain.

  This function is a wrapper around the SCMI PERFORMANCE_DESCRIBE_LEVELS
  command. The list of discrete performance states is returned in a buffer
  that must be freed by the caller.

  @param[in]  DomainId        Identifier for the performance domain.
  @param[out] LevelArray      If success, pointer to the list of list of
                              performance state. This memory must be freed by
                              the caller.
  @param[out] LevelArrayCount If success, contains the number of states in
                              LevelArray.

  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Device error.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_TIMEOUT             Time out.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
STATIC
EFI_STATUS
EFIAPI
DynamicTablesScmiInfoDescribeLevels (
  IN  UINT32                  DomainId,
  OUT SCMI_PERFORMANCE_LEVEL  **LevelArray,
  OUT UINT32                  *LevelArrayCount
  )
{
  EFI_STATUS              Status;
  SCMI_PERFORMANCE_LEVEL  *Array;
  UINT32                  Count;
  UINT32                  Size;

  if ((ScmiPerfProtocol == NULL)  ||
      (LevelArray == NULL)  ||
      (LevelArrayCount == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  // First call to get the number of levels.
  Size   = 0;
  Status = ScmiPerfProtocol->DescribeLevels (
                               ScmiPerfProtocol,
                               DomainId,
                               &Count,
                               &Size,
                               NULL
                               );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    // EFI_SUCCESS is not a valid option.
    if (Status == EFI_SUCCESS) {
      return EFI_INVALID_PARAMETER;
    } else {
      return Status;
    }
  }

  Array = AllocateZeroPool (Size);
  if (Array == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Second call to get the descriptions of the levels.
  Status = ScmiPerfProtocol->DescribeLevels (
                               ScmiPerfProtocol,
                               DomainId,
                               &Count,
                               &Size,
                               Array
                               );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *LevelArray      = Array;
  *LevelArrayCount = Count;

  return Status;
}

/** Populate a AML_CPC_INFO object based on SCMI information.

  @param[in]  DomainId    Identifier for the performance domain.
  @param[out] CpcInfo     If success, this structure was populated from
                          information queried to the SCP.

  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Device error.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_TIMEOUT             Time out.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
EFI_STATUS
EFIAPI
DynamicTablesScmiInfoGetFastChannel (
  IN  UINT32        DomainId,
  OUT AML_CPC_INFO  *CpcInfo
  )
{
  EFI_STATUS                          Status;
  SCMI_PERFORMANCE_FASTCHANNEL        FcLevelGet;
  SCMI_PERFORMANCE_FASTCHANNEL        FcLimitsSet;
  SCMI_PERFORMANCE_DOMAIN_ATTRIBUTES  DomainAttributes;

  SCMI_PERFORMANCE_LEVEL  *LevelArray;
  UINT32                  LevelCount;

  UINT64  FcLevelGetAddr;
  UINT64  FcLimitsMaxSetAddr;
  UINT64  FcLimitsMinSetAddr;

  if ((ScmiPerfProtocol == NULL)  ||
      (CpcInfo == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = ScmiPerfProtocol->DescribeFastchannel (
                               ScmiPerfProtocol,
                               DomainId,
                               ScmiMessageIdPerformanceLevelSet,
                               &FcLevelGet
                               );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ScmiPerfProtocol->DescribeFastchannel (
                               ScmiPerfProtocol,
                               DomainId,
                               ScmiMessageIdPerformanceLimitsSet,
                               &FcLimitsSet
                               );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ScmiPerfProtocol->GetDomainAttributes (
                               ScmiPerfProtocol,
                               DomainId,
                               &DomainAttributes
                               );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynamicTablesScmiInfoDescribeLevels (DomainId, &LevelArray, &LevelCount);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  /* Do some safety checks.
     Only support FastChannels (and not doorbells) as this is
     the only mechanism supported by SCP.
     FcLimits[Get|Set] require 2 UINT32 values (max, then min) and
     FcLimits[Get|Set] require 1 UINT32 value (level).
  */
  if ((FcLevelGet.ChanSize != sizeof (UINT32))  ||
      ((FcLevelGet.Attributes & SCMI_PERF_FC_ATTRIB_HAS_DOORBELL) ==
       SCMI_PERF_FC_ATTRIB_HAS_DOORBELL) ||
      (FcLimitsSet.ChanSize != 2 * sizeof (UINT32)) ||
      ((FcLimitsSet.Attributes & SCMI_PERF_FC_ATTRIB_HAS_DOORBELL) ==
       SCMI_PERF_FC_ATTRIB_HAS_DOORBELL))
  {
    Status = EFI_INVALID_PARAMETER;
    goto exit_handler;
  }

  FcLevelGetAddr = ((UINT64)FcLevelGet.ChanAddrHigh << 32) |
                   FcLevelGet.ChanAddrLow;
  FcLimitsMaxSetAddr = ((UINT64)FcLimitsSet.ChanAddrHigh << 32) |
                       FcLimitsSet.ChanAddrLow;
  FcLimitsMinSetAddr = FcLimitsMaxSetAddr + 0x4;

  CpcInfo->Revision                          = EFI_ACPI_6_5_AML_CPC_REVISION;
  CpcInfo->HighestPerformanceInteger         = LevelArray[LevelCount - 1].Level;
  CpcInfo->NominalPerformanceInteger         = DomainAttributes.SustainedPerfLevel;
  CpcInfo->LowestNonlinearPerformanceInteger = LevelArray[0].Level;
  CpcInfo->LowestPerformanceInteger          = LevelArray[0].Level;

  CpcInfo->DesiredPerformanceRegister.AddressSpaceId    = EFI_ACPI_6_5_SYSTEM_MEMORY;
  CpcInfo->DesiredPerformanceRegister.RegisterBitWidth  = 32;
  CpcInfo->DesiredPerformanceRegister.RegisterBitOffset = 0;
  CpcInfo->DesiredPerformanceRegister.AccessSize        = EFI_ACPI_6_5_DWORD;
  CpcInfo->DesiredPerformanceRegister.Address           = FcLevelGetAddr;

  CpcInfo->MinimumPerformanceRegister.AddressSpaceId    = EFI_ACPI_6_5_SYSTEM_MEMORY;
  CpcInfo->MinimumPerformanceRegister.RegisterBitWidth  = 32;
  CpcInfo->MinimumPerformanceRegister.RegisterBitOffset = 0;
  CpcInfo->MinimumPerformanceRegister.AccessSize        = EFI_ACPI_6_5_DWORD;
  CpcInfo->MinimumPerformanceRegister.Address           = FcLimitsMinSetAddr;

  CpcInfo->MaximumPerformanceRegister.AddressSpaceId    = EFI_ACPI_6_5_SYSTEM_MEMORY;
  CpcInfo->MaximumPerformanceRegister.RegisterBitWidth  = 32;
  CpcInfo->MaximumPerformanceRegister.RegisterBitOffset = 0;
  CpcInfo->MaximumPerformanceRegister.AccessSize        = EFI_ACPI_6_5_DWORD;
  CpcInfo->MaximumPerformanceRegister.Address           = FcLimitsMaxSetAddr;

  CpcInfo->ReferencePerformanceCounterRegister.AddressSpaceId    = EFI_ACPI_6_5_FUNCTIONAL_FIXED_HARDWARE;
  CpcInfo->ReferencePerformanceCounterRegister.RegisterBitWidth  = 0x40;
  CpcInfo->ReferencePerformanceCounterRegister.RegisterBitOffset = 0;
  CpcInfo->ReferencePerformanceCounterRegister.AccessSize        = ARM_FFH_REFERENCE_PERF_COUNTER_REGISTER;
  CpcInfo->ReferencePerformanceCounterRegister.Address           = 0x4;

  CpcInfo->DeliveredPerformanceCounterRegister.AddressSpaceId    = EFI_ACPI_6_5_FUNCTIONAL_FIXED_HARDWARE;
  CpcInfo->DeliveredPerformanceCounterRegister.RegisterBitWidth  = 0x40;
  CpcInfo->DeliveredPerformanceCounterRegister.RegisterBitOffset = 0;
  CpcInfo->DeliveredPerformanceCounterRegister.AccessSize        = ARM_FFH_DELIVERED_PERF_COUNTER_REGISTER;
  CpcInfo->DeliveredPerformanceCounterRegister.Address           = 0x4;

  // SCMI should advertise performance values on a unified scale. So frequency
  // values are not available. LowestFrequencyInteger and
  // NominalFrequencyInteger are populated in the ConfigurationManager.

exit_handler:
  FreePool (LevelArray);
  return Status;
}
