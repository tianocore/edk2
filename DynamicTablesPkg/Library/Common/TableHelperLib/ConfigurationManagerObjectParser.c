/** @file
  Configuration Manager Object parser.

  Copyright (c) 2021 - 2023, ARM Limited. All rights reserved.<BR>
  Copyright (C) 2024 - 2025 Advanced Micro Devices, Inc. All rights reserved.
  Copyright (c) 2024 - 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <ConfigurationManagerObject.h>
#include "ArchCommonNameSpaceObjects.h"
#include "ConfigurationManagerObjectParser.h"

STATIC
VOID
EFIAPI
PrintString (
  CONST CHAR8  *Format,
  UINT8        *Ptr,
  UINT32       Length
  );

STATIC
VOID
EFIAPI
PrintStringPtr (
  CONST CHAR8  *Format,
  UINT8        *Ptr,
  UINT32       Length
  );

STATIC
VOID
EFIAPI
PrintChars (
  CONST CHAR8  *Format,
  UINT8        *Ptr,
  UINT32       Length
  );

STATIC
VOID
EFIAPI
HexDump (
  CONST CHAR8  *Format,
  UINT8        *Ptr,
  UINT32       Length
  );

/** A parser for EArmObjBootArchInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArmBootArchInfoParser[] = {
  { "BootArchFlags", 2, "0x%x", NULL }
};

/** A parser for EArchCommonObjPowerManagementProfileInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonPowerManagementProfileInfoParser[] = {
  { "PowerManagementProfile", 1, "0x%x", NULL }
};

/** A parser for EArmObjGicCInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArmGicCInfoParser[] = {
  { "CPUInterfaceNumber",            4,                        "0x%x",   NULL },
  { "AcpiProcessorUid",              4,                        "0x%x",   NULL },
  { "Flags",                         4,                        "0x%x",   NULL },
  { "ParkingProtocolVersion",        4,                        "0x%x",   NULL },
  { "PerformanceInterruptGsiv",      4,                        "0x%x",   NULL },
  { "ParkedAddress",                 8,                        "0x%llx", NULL },
  { "PhysicalBaseAddress",           8,                        "0x%llx", NULL },
  { "GICV",                          8,                        "0x%llx", NULL },
  { "GICH",                          8,                        "0x%llx", NULL },
  { "VGICMaintenanceInterrupt",      4,                        "0x%x",   NULL },
  { "GICRBaseAddress",               8,                        "0x%llx", NULL },
  { "MPIDR",                         8,                        "0x%llx", NULL },
  { "ProcessorPowerEfficiencyClass", 1,                        "0x%x",   NULL },
  { "SpeOverflowInterrupt",          2,                        "0x%x",   NULL },
  { "ProximityDomain",               4,                        "0x%x",   NULL },
  { "ClockDomain",                   4,                        "0x%x",   NULL },
  { "AffinityFlags",                 4,                        "0x%x",   NULL },
  { "CpcToken",                      sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
  { "TRBEInterrupt",                 2,                        "0x%x",   NULL },
  { "EtToken",                       sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
  { "PsdToken",                      sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
};

/** A parser for EArmObjGicDInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArmGicDInfoParser[] = {
  { "PhysicalBaseAddress", 8, "0x%llx", NULL },
  { "SystemVectorBase",    4, "0x%x",   NULL },
  { "GicVersion",          1, "0x%x",   NULL },
};

/** A parser for EArmObjGicMsiFrameInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArmGicMsiFrameInfoParser[] = {
  { "GicMsiFrameId",       4, "0x%x",   NULL },
  { "PhysicalBaseAddress", 8, "0x%llx", NULL },
  { "Flags",               4, "0x%x",   NULL },
  { "SPICount",            2, "0x%x",   NULL },
  { "SPIBase",             2, "0x%x",   NULL }
};

/** A parser for EArmObjGicRedistributorInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArmGicRedistInfoParser[] = {
  { "DiscoveryRangeBaseAddress", 8, "0x%llx", NULL },
  { "DiscoveryRangeLength",      4, "0x%x",   NULL }
};

/** A parser for EArmObjGicItsInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArmGicItsInfoParser[] = {
  { "GicItsId",            4, "0x%x",   NULL },
  { "PhysicalBaseAddress", 8, "0x%llx", NULL },
  { "ProximityDomain",     4, "0x%x",   NULL }
};

/** A parser for EArchCommonObjConsolePortInfo,
    EArchCommonObjSerialDebugPortInfo and EArchCommonObjSerialPortInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonSerialPortInfoParser[] = {
  { "BaseAddress",       8, "0x%llx", NULL },
  { "Interrupt",         4, "0x%x",   NULL },
  { "BaudRate",          8, "0x%llx", NULL },
  { "Clock",             4, "0x%x",   NULL },
  { "PortSubtype",       2, "0x%x",   NULL },
  { "BaseAddressLength", 8, "0x%llx", NULL },
  { "AccessSize",        1, "0x%d",   NULL }
};

/** A parser for EArmObjGenericTimerInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArmGenericTimerInfoParser[] = {
  { "CounterControlBaseAddress", 8, "0x%llx", NULL },
  { "CounterReadBaseAddress",    8, "0x%llx", NULL },
  { "SecurePL1TimerGSIV",        4, "0x%x",   NULL },
  { "SecurePL1TimerFlags",       4, "0x%x",   NULL },
  { "NonSecurePL1TimerGSIV",     4, "0x%x",   NULL },
  { "NonSecurePL1TimerFlags",    4, "0x%x",   NULL },
  { "VirtualTimerGSIV",          4, "0x%x",   NULL },
  { "VirtualTimerFlags",         4, "0x%x",   NULL },
  { "NonSecurePL2TimerGSIV",     4, "0x%x",   NULL },
  { "NonSecurePL2TimerFlags",    4, "0x%x",   NULL },
  { "VirtualPL2TimerGSIV",       4, "0x%x",   NULL },
  { "VirtualPL2TimerFlags",      4, "0x%x",   NULL }
};

/** A parser for EArmObjPlatformGTBlockInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArmGTBlockInfoParser[] = {
  { "GTBlockPhysicalAddress", 8,                        "0x%llx", NULL },
  { "GTBlockTimerFrameCount", 4,                        "0x%x",   NULL },
  { "GTBlockTimerFrameToken", sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL }
};

/** A parser for EArmObjGTBlockTimerFrameInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArmGTBlockTimerFrameInfoParser[] = {
  { "FrameNumber",               1, "0x%x",   NULL },
  { "PhysicalAddressCntBase",    8, "0x%llx", NULL },
  { "PhysicalAddressCntEL0Base", 8, "0x%llx", NULL },
  { "PhysicalTimerGSIV",         4, "0x%x",   NULL },
  { "PhysicalTimerFlags",        4, "0x%x",   NULL },
  { "VirtualTimerGSIV",          4, "0x%x",   NULL },
  { "VirtualTimerFlags",         4, "0x%x",   NULL },
  { "CommonFlags",               4, "0x%x",   NULL }
};

/** A parser for EArmObjPlatformGenericWatchdogInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArmGenericWatchdogInfoParser[] = {
  { "ControlFrameAddress", 8, "0x%llx", NULL },
  { "RefreshFrameAddress", 8, "0x%llx", NULL },
  { "TimerGSIV",           4, "0x%x",   NULL },
  { "Flags",               4, "0x%x",   NULL }
};

/** A parser for EArchCommonObjPciConfigSpaceInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonPciConfigSpaceInfoParser[] = {
  { "BaseAddress",           8,                        "0x%llx", NULL },
  { "PciSegmentGroupNumber", 2,                        "0x%x",   NULL },
  { "StartBusNumber",        1,                        "0x%x",   NULL },
  { "EndBusNumber",          1,                        "0x%x",   NULL },
  { "AddressMapToken",       sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
  { "InterruptMapToken",     sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
};

/** A parser for EArchCommonObjHypervisorVendorIdentity.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonHypervisorVendorIdentityParser[] = {
  { "HypervisorVendorId", 8, "0x%llx", NULL }
};

/** A parser for EArchCommonObjFixedFeatureFlags.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonFixedFeatureFlagsParser[] = {
  { "Flags", 4, "0x%x", NULL }
};

/** A parser for EArmObjItsGroup.
*/
STATIC CONST CM_OBJ_PARSER  CmArmItsGroupNodeParser[] = {
  { "Token",      sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "ItsIdCount", 4,                        "0x%x", NULL },
  { "ItsIdToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "Identifier", 4,                        "0x%x", NULL },
};

/** A parser for EArmObjNamedComponent.
*/
STATIC CONST CM_OBJ_PARSER  CmArmNamedComponentNodeParser[] = {
  { "Token",             sizeof (CM_OBJECT_TOKEN), "0x%p", NULL           },
  { "IdMappingCount",    4,                        "0x%x", NULL           },
  { "IdMappingToken",    sizeof (CM_OBJECT_TOKEN), "0x%p", NULL           },
  { "Flags",             4,                        "0x%x", NULL           },
  { "CacheCoherent",     4,                        "0x%x", NULL           },
  { "AllocationHints",   1,                        "0x%x", NULL           },
  { "MemoryAccessFlags", 1,                        "0x%x", NULL           },
  { "AddressSizeLimit",  1,                        "0x%x", NULL           },
  { "ObjectName",        sizeof (CHAR8 *),         NULL,   PrintStringPtr },
  { "Identifier",        4,                        "0x%x", NULL           },
};

/** A parser for EArmObjRootComplex.
*/
STATIC CONST CM_OBJ_PARSER  CmArmRootComplexNodeParser[] = {
  { "Token",             sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "IdMappingCount",    4,                        "0x%x", NULL },
  { "IdMappingToken",    sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "CacheCoherent",     4,                        "0x%x", NULL },
  { "AllocationHints",   1,                        "0x%x", NULL },
  { "MemoryAccessFlags", 1,                        "0x%x", NULL },
  { "AtsAttribute",      4,                        "0x%x", NULL },
  { "PciSegmentNumber",  4,                        "0x%x", NULL },
  { "MemoryAddressSize", 1,                        "0x%x", NULL },
  { "PasidCapabilities", 2,                        "0x%x", NULL },
  { "Flags",             4,                        "0x%x", NULL },
  { "Identifier",        4,                        "0x%x", NULL },
};

/** A parser for EArmObjSmmuV1SmmuV2.
*/
STATIC CONST CM_OBJ_PARSER  CmArmSmmuV1SmmuV2NodeParser[] = {
  { "Token",                 sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
  { "IdMappingCount",        4,                        "0x%x",   NULL },
  { "IdMappingToken",        sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
  { "BaseAddress",           8,                        "0x%llx", NULL },
  { "Span",                  8,                        "0x%llx", NULL },
  { "Model",                 4,                        "0x%x",   NULL },
  { "Flags",                 4,                        "0x%x",   NULL },
  { "ContextInterruptCount", 4,                        "0x%x",   NULL },
  { "ContextInterruptToken", sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
  { "PmuInterruptCount",     4,                        "0x%x",   NULL },
  { "PmuInterruptToken",     sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
  { "SMMU_NSgIrpt",          4,                        "0x%x",   NULL },
  { "SMMU_NSgIrptFlags",     4,                        "0x%x",   NULL },
  { "SMMU_NSgCfgIrpt",       4,                        "0x%x",   NULL },
  { "SMMU_NSgCfgIrptFlags",  4,                        "0x%x",   NULL },
  { "Identifier",            4,                        "0x%x",   NULL },
};

/** A parser for EArmObjSmmuV3.
*/
STATIC CONST CM_OBJ_PARSER  CmArmSmmuV3NodeParser[] = {
  { "Token",                sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
  { "IdMappingCount",       4,                        "0x%x",   NULL },
  { "IdMappingToken",       sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
  { "BaseAddress",          8,                        "0x%llx", NULL },
  { "Flags",                4,                        "0x%x",   NULL },
  { "VatosAddress",         8,                        "0x%llx", NULL },
  { "Model",                4,                        "0x%x",   NULL },
  { "EventInterrupt",       4,                        "0x%x",   NULL },
  { "PriInterrupt",         4,                        "0x%x",   NULL },
  { "GerrInterrupt",        4,                        "0x%x",   NULL },
  { "SyncInterrupt",        4,                        "0x%x",   NULL },
  { "ProximityDomain",      4,                        "0x%x",   NULL },
  { "DeviceIdMappingIndex", 4,                        "0x%x",   NULL },
  { "Identifier",           4,                        "0x%x",   NULL },
};

/** A parser for EArmObjPmcg.
*/
STATIC CONST CM_OBJ_PARSER  CmArmPmcgNodeParser[] = {
  { "Token",             sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
  { "IdMappingCount",    4,                        "0x%x",   NULL },
  { "IdMappingToken",    sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
  { "BaseAddress",       8,                        "0x%llx", NULL },
  { "OverflowInterrupt", 4,                        "0x%x",   NULL },
  { "Page1BaseAddress",  8,                        "0x%llx", NULL },
  { "ReferenceToken",    sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
  { "Identifier",        4,                        "0x%x",   NULL },
};

/** A parser for EArmObjGicItsIdentifierArray.
*/
STATIC CONST CM_OBJ_PARSER  CmArmGicItsIdentifierParser[] = {
  { "ItsId", 4, "0x%x", NULL }
};

/** A parser for EArmObjIdMappingArray.
*/
STATIC CONST CM_OBJ_PARSER  CmArmIdMappingParser[] = {
  { "InputBase",            4,                        "0x%x", NULL },
  { "NumIds",               4,                        "0x%x", NULL },
  { "OutputBase",           4,                        "0x%x", NULL },
  { "OutputReferenceToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "Flags",                4,                        "0x%x", NULL }
};

/** A parser for EArmObjSmmuInterruptArray.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonGenericInterruptParser[] = {
  { "Interrupt", 4, "0x%x", NULL },
  { "Flags",     4, "0x%x", NULL }
};

/** A parser for EArchCommonObjProcHierarchyInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonProcHierarchyInfoParser[] = {
  { "Token",                      sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "Flags",                      4,                        "0x%x", NULL },
  { "ParentToken",                sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "AcpiIdObjectToken",          sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "NoOfPrivateResources",       4,                        "0x%x", NULL },
  { "PrivateResourcesArrayToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "LpiToken",                   sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "OverrideNameUidEnabled",     1,                        "%d",   NULL },
  { "OverrideName",               2,                        "0x%x", NULL },
  { "OverrideUid",                4,                        "0x%x", NULL }
};

/** A parser for EArchCommonObjCacheInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonCacheInfoParser[] = {
  { "Token",                 sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "NextLevelOfCacheToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "Size",                  4,                        "0x%x", NULL },
  { "NumberOfSets",          4,                        "0x%x", NULL },
  { "Associativity",         4,                        "0x%x", NULL },
  { "Attributes",            1,                        "0x%x", NULL },
  { "LineSize",              2,                        "0x%x", NULL },
  { "CacheId",               4,                        "0x%x", NULL },
};

/** A parser for EArchCommonObjCmRef.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonObjRefParser[] = {
  { "ReferenceToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL }
};

/** A parser for EArchCommonObjMemoryAffinityInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonMemoryAffinityInfoParser[] = {
  { "ProximityDomain", 4, "0x%x",   NULL },
  { "BaseAddress",     8, "0x%llx", NULL },
  { "Length",          8, "0x%llx", NULL },
  { "Flags",           4, "0x%x",   NULL }
};

/** A parser for EArchCommonObjDeviceHandleAcpi.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonDeviceHandleAcpiParser[] = {
  { "Hid", 8, "0x%llx", NULL },
  { "Uid", 4, "0x%x",   NULL }
};

/** A parser for EArchCommonObjDeviceHandlePci.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonDeviceHandlePciParser[] = {
  { "SegmentNumber",  2, "0x%x", NULL },
  { "BusNumber",      1, "0x%x", NULL },
  { "DeviceNumber",   1, "0x%x", NULL },
  { "FunctionNumber", 1, "0x%x", NULL }
};

/** A parser for EArchCommonObjGenericInitiatorAffinityInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonGenericInitiatorAffinityInfoParser[] = {
  { "ProximityDomain",   4,                        "0x%x", NULL },
  { "Flags",             4,                        "0x%x", NULL },
  { "DeviceHandleType",  1,                        "0x%x", NULL },
  { "DeviceHandleToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL }
};

/** A parser for EArmObjCmn600Info.
*/
STATIC CONST CM_OBJ_PARSER  CmArmCmn600InfoParser[] = {
  { "PeriphBaseAddress",       8, "0x%llx", NULL },
  { "PeriphBaseAddressLength", 8, "0x%llx", NULL },
  { "RootNodeBaseAddress",     8, "0x%llx", NULL },
  { "DtcCount",                1, "0x%x",   NULL },
  { "DtcInterrupt[0]",         4, "0x%x",   NULL },
  { "DtcFlags[0]",             4, "0x%x",   NULL },
  { "DtcInterrupt[1]",         4, "0x%x",   NULL },
  { "DtcFlags[1]",             4, "0x%x",   NULL },
  { "DtcInterrupt[2]",         4, "0x%x",   NULL },
  { "DtcFlags[2]",             4, "0x%x",   NULL },
  { "DtcInterrupt[3]",         4, "0x%x",   NULL },
  { "DtcFlags[3]",             4, "0x%x",   NULL }
};

/** A parser for the EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE structure.
*/
STATIC CONST CM_OBJ_PARSER  AcpiGenericAddressParser[] = {
  { "AddressSpaceId",    1, "%d",     NULL },
  { "RegisterBitWidth",  1, "%d",     NULL },
  { "RegisterBitOffset", 1, "%d",     NULL },
  { "AccessSize",        1, "%d",     NULL },
  { "Address",           8, "0x%llx", NULL },
};

/** A parser for EArchCommonObjLpiInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonLpiInfoParser[] = {
  { "MinResidency",             4,                                               "0x%x",   NULL        },
  { "WorstCaseWakeLatency",     4,                                               "0x%x",   NULL        },
  { "Flags",                    4,                                               "0x%x",   NULL        },
  { "ArchFlags",                4,                                               "0x%x",   NULL        },
  { "ResCntFreq",               4,                                               "0x%x",   NULL        },
  { "EnableParentState",        4,                                               "0x%x",   NULL        },
  { "IsInteger",                1,                                               "%d",     NULL        },
  { "IntegerEntryMethod",       8,                                               "0x%llx", NULL        },
  { "RegisterEntryMethod",      sizeof (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "ResidencyCounterRegister", sizeof (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "UsageCounterRegister",     sizeof (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "StateName",                16,                                              NULL,     PrintString },
};

/** A parser for EArchCommonObjPciAddressMapInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonPciAddressMapInfoParser[] = {
  { "SpaceCode",   1, "%d",     NULL },
  { "PciAddress",  8, "0x%llx", NULL },
  { "CpuAddress",  8, "0x%llx", NULL },
  { "AddressSize", 8, "0x%llx", NULL },
};

/** A parser for EArchCommonObjPciInterruptMapInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonPciInterruptMapInfoParser[] = {
  { "PciBus",        1,                                         "0x%x", NULL },
  { "PciDevice",     1,                                         "0x%x", NULL },
  { "PciInterrupt",  1,                                         "0x%x", NULL },
  { "IntcInterrupt", sizeof (CM_ARCH_COMMON_GENERIC_INTERRUPT),
    NULL, NULL, CmArchCommonGenericInterruptParser,
    ARRAY_SIZE (CmArchCommonGenericInterruptParser) },
};

/** A parser for EArmObjRmr.
*/
STATIC CONST CM_OBJ_PARSER  CmArmRmrInfoParser[] = {
  { "Token",             sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "IdMappingCount",    4,                        "0x%x", NULL },
  { "IdMappingToken",    sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "Identifier",        4,                        "0x%x", NULL },
  { "Flags",             4,                        "0x%x", NULL },
  { "MemRangeDescCount", 4,                        "0x%x", NULL },
  { "MemRangeDescToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
};

/** A parser for EArmObjMemoryRangeDescriptor.
*/
STATIC CONST CM_OBJ_PARSER  CmArmMemoryRangeDescriptorInfoParser[] = {
  { "BaseAddress", 8, "0x%llx", NULL },
  { "Length",      8, "0x%llx", NULL },
};

/** A parser for EArchCommonObjCpcInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonCpcInfoParser[] = {
  { "Revision",                              4,                                               "0x%lx", NULL },
  { "HighestPerformanceBuffer",              sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "HighestPerformanceInteger",             4,                                               "0x%lx", NULL },
  { "NominalPerformanceBuffer",              sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "NominalPerformanceInteger",             4,                                               "0x%lx", NULL },
  { "LowestNonlinearPerformanceBuffer",      sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "LowestNonlinearPerformanceInteger",     4,                                               "0x%lx", NULL },
  { "LowestPerformanceBuffer",               sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "LowestPerformanceInteger",              4,                                               "0x%lx", NULL },
  { "GuaranteedPerformanceRegister",         sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "DesiredPerformanceRegister",            sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "MinimumPerformanceRegister",            sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "MaximumPerformanceRegister",            sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "PerformanceReductionToleranceRegister", sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "TimeWindowRegister",                    sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "CounterWraparoundTimeBuffer",           sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "CounterWraparoundTimeInteger",          4,                                               "0x%lx", NULL },
  { "ReferencePerformanceCounterRegister",   sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "DeliveredPerformanceCounterRegister",   sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "PerformanceLimitedRegister",            sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "CPPCEnableRegister",                    sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "AutonomousSelectionEnableBuffer",       sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "AutonomousSelectionEnableInteger",      4,                                               "0x%lx", NULL },
  { "AutonomousActivityWindowRegister",      sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "EnergyPerformancePreferenceRegister",   sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "ReferencePerformanceBuffer",            sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "ReferencePerformanceInteger",           4,                                               "0x%lx", NULL },
  { "LowestFrequencyBuffer",                 sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "LowestFrequencyInteger",                4,                                               "0x%lx", NULL },
  { "NominalFrequencyBuffer",                sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "NominalFrequencyInteger",               4,                                               "0x%lx", NULL },
};

/** A parser for the PCC_MAILBOX_REGISTER_INFO struct.
*/
STATIC CONST CM_OBJ_PARSER  CmArmMailboxRegisterInfoParser[] = {
  { "Register",     sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE), NULL,     NULL,
    AcpiGenericAddressParser, ARRAY_SIZE (AcpiGenericAddressParser) },
  { "PreserveMask", 8,                                               "0x%llx", NULL },
  { "WriteMask",    8,                                               "0x%llx", NULL },
};

/** A parser for the PCC_SUBSPACE_CHANNEL_TIMING_INFO struct.
*/
STATIC CONST CM_OBJ_PARSER  CmArmPccSubspaceChannelTimingInfoParser[] = {
  { "NominalLatency",           4, "0x%x", NULL },
  { "MaxPeriodicAccessRate",    4, "0x%x", NULL },
  { "MinRequestTurnaroundTime", 2, "0x%x", NULL },
};

/** A parser for EArchCommonObjPccSubspaceType0Info.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonPccSubspaceType0InfoParser[] = {
  { "SubspaceId",    1,                                         "0x%x",   NULL },
  { "Type",          1,                                         "0x%x",   NULL },
  { "BaseAddress",   8,                                         "0x%llx", NULL },
  { "AddressLength", 8,                                         "0x%llx", NULL },
  { "DoorbellReg",   sizeof (PCC_MAILBOX_REGISTER_INFO),
    NULL, NULL, CmArmMailboxRegisterInfoParser,
    ARRAY_SIZE (CmArmMailboxRegisterInfoParser) },
  { "ChannelTiming", sizeof (PCC_SUBSPACE_CHANNEL_TIMING_INFO),
    NULL, NULL, CmArmPccSubspaceChannelTimingInfoParser,
    ARRAY_SIZE (CmArmPccSubspaceChannelTimingInfoParser) },
};

/** A parser for EArchCommonObjPccSubspaceType1Info.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonPccSubspaceType1InfoParser[] = {
  { "GenericPccInfo", sizeof (PCC_SUBSPACE_GENERIC_INFO),
    NULL, NULL, CmArchCommonPccSubspaceType0InfoParser,
    ARRAY_SIZE (CmArchCommonPccSubspaceType0InfoParser) },
  { "PlatIrq",        sizeof (CM_ARCH_COMMON_GENERIC_INTERRUPT),
    NULL, NULL, CmArchCommonGenericInterruptParser,
    ARRAY_SIZE (CmArchCommonGenericInterruptParser) },
};

/** A parser for EArchCommonObjPccSubspaceType2Info.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonPccSubspaceType2InfoParser[] = {
  { "GenericPccInfo", sizeof (PCC_SUBSPACE_GENERIC_INFO),
    NULL, NULL, CmArchCommonPccSubspaceType0InfoParser,
    ARRAY_SIZE (CmArchCommonPccSubspaceType0InfoParser) },
  { "PlatIrq",        sizeof (CM_ARCH_COMMON_GENERIC_INTERRUPT),NULL,NULL,
    CmArchCommonGenericInterruptParser, ARRAY_SIZE (CmArchCommonGenericInterruptParser) },
  { "PlatIrqAckReg",  sizeof (PCC_MAILBOX_REGISTER_INFO),
    NULL, NULL, CmArmMailboxRegisterInfoParser,
    ARRAY_SIZE (CmArmMailboxRegisterInfoParser) },
};

/** A parser for EArchCommonObjPccSubspaceType3Info or EArchCommonObjPccSubspaceType4Info.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonPccSubspaceType34InfoParser[] = {
  { "GenericPccInfo",       sizeof (PCC_SUBSPACE_GENERIC_INFO),
    NULL, NULL, CmArchCommonPccSubspaceType0InfoParser,
    ARRAY_SIZE (CmArchCommonPccSubspaceType0InfoParser) },
  { "PlatIrq",              sizeof (CM_ARCH_COMMON_GENERIC_INTERRUPT),NULL,NULL,
    CmArchCommonGenericInterruptParser, ARRAY_SIZE (CmArchCommonGenericInterruptParser) },
  { "PlatIrqAckReg",        sizeof (PCC_MAILBOX_REGISTER_INFO),
    NULL, NULL, CmArmMailboxRegisterInfoParser,
    ARRAY_SIZE (CmArmMailboxRegisterInfoParser) },
  { "CmdCompleteCheckReg",  sizeof (PCC_MAILBOX_REGISTER_INFO),
    NULL, NULL, CmArmMailboxRegisterInfoParser,
    ARRAY_SIZE (CmArmMailboxRegisterInfoParser) },
  { "CmdCompleteUpdateReg", sizeof (PCC_MAILBOX_REGISTER_INFO),
    NULL, NULL, CmArmMailboxRegisterInfoParser,
    ARRAY_SIZE (CmArmMailboxRegisterInfoParser) },
  { "ErrorStatusReg",       sizeof (PCC_MAILBOX_REGISTER_INFO),
    NULL, NULL, CmArmMailboxRegisterInfoParser,
    ARRAY_SIZE (CmArmMailboxRegisterInfoParser) },
};

/** A parser for EArchCommonObjPccSubspaceType5Info.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonPccSubspaceType5InfoParser[] = {
  { "GenericPccInfo",      sizeof (PCC_SUBSPACE_GENERIC_INFO),
    NULL, NULL, CmArchCommonPccSubspaceType0InfoParser,
    ARRAY_SIZE (CmArchCommonPccSubspaceType0InfoParser) },
  { "Version",             2,                                        "0x%x",NULL },
  { "PlatIrq",             sizeof (CM_ARCH_COMMON_GENERIC_INTERRUPT),NULL,  NULL,
    CmArchCommonGenericInterruptParser, ARRAY_SIZE (CmArchCommonGenericInterruptParser) },
  { "CmdCompleteCheckReg", sizeof (PCC_MAILBOX_REGISTER_INFO),
    NULL, NULL, CmArmMailboxRegisterInfoParser,
    ARRAY_SIZE (CmArmMailboxRegisterInfoParser) },
  { "ErrorStatusReg",      sizeof (PCC_MAILBOX_REGISTER_INFO),
    NULL, NULL, CmArmMailboxRegisterInfoParser,
    ARRAY_SIZE (CmArmMailboxRegisterInfoParser) },
};

/** A parser for EArmObjEtInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArmEtInfo[] = {
  { "EtType", sizeof (ARM_ET_TYPE), "0x%x", NULL }
};

/** A parser for EArchCommonObjPsdInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonPsdInfoParser[] = {
  { "Revision",  1, "0x%x", NULL },
  { "DomainId",  4, "0x%x", NULL },
  { "CoordType", 4, "0x%x", NULL },
  { "NumProc",   4, "0x%x", NULL },
};

/** A parser for EArchCommonObjTpm2InterfaceInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonTpm2InterfaceInfo[] = {
  { "PlatformClass",             sizeof (UINT16),                                               "0x%x",   NULL    },
  { "AddressOfControlArea",      sizeof (UINT64),                                               "0x%llx", NULL    },
  { "StartMethod",               sizeof (UINT32),                                               "0x%x",   NULL    },
  { "StartMethodParametersSize", sizeof (UINT8),                                                "0x%x",   NULL    },
  { "StartMethodParameters",     EFI_TPM2_ACPI_TABLE_START_METHOD_SPECIFIC_PARAMETERS_MAX_SIZE, NULL,     HexDump },
  { "Laml",                      sizeof (UINT32),                                               "0x%x",   NULL    },
  { "Lasa",                      sizeof (UINT64),                                               "0x%llx", NULL    },
};

/** A parser for EArchCommonObjSpmiInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonSpmiInterfaceInfoParser[] = {
  { "InterfaceType", sizeof (UINT8),                                  "0x%x", NULL },
  { "BaseAddress",   sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser, ARRAY_SIZE (AcpiGenericAddressParser) },
};

/** A parser for EArchCommonObjSpmiInterruptDeviceInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonSpmiInterruptDeviceInfoParser[] = {
  { "InterruptType",         sizeof (UINT8),  "0x%x", NULL },
  { "GPE",                   sizeof (UINT8),  "0x%x", NULL },
  { "PciDeviceFlag",         sizeof (UINT8),  "0x%x", NULL },
  { "GlobalSystemInterrupt", sizeof (UINT32), "0x%x", NULL },
  { "DeviceId",              sizeof (UINT32), "0x%x", NULL }
};

STATIC CONST CM_OBJ_PARSER  CmArchCommonCstInfoParser[] = {
  { "Register",
    sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE),
    NULL,
    NULL,
    AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "Type",    1,  "0x%x", NULL },
  { "Latency", 2,  "0x%x", NULL },
  { "Power",   4,  "0x%x", NULL }
};

/** A parser for EArchCommonObjCsdInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonCsdInfoParser[] = {
  { "Revision",       1,                        "0x%x", NULL },
  { "Domain",         4,                        "0x%x", NULL },
  { "CoordType",      4,                        "0x%x", NULL },
  { "NumProcessors",  4,                        "0x%x", NULL },
  { "CstPkgRefToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL }
};

/** A parser for EArchCommonObjPctInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonPctInfoParser[] = {
  { "ControlRegister",
    sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE),
    NULL,
    NULL,
    AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "StatusRegister",
    sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE),
    NULL,
    NULL,
    AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) }
};

/** A parser for EArchCommonObjPssInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonPssInfoParser[] = {
  { "CoreFrequency",    4, "0x%x", NULL },
  { "Power",            4, "0x%x", NULL },
  { "Latency",          4, "0x%x", NULL },
  { "BusMasterLatency", 4, "0x%x", NULL },
  { "Control",          4, "0x%x", NULL },
  { "Status",           4, "0x%x", NULL }
};

/** A parser for EArchCommonObjPpcInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonPpcInfoParser[] = {
  { "PstateCount", 4, "0x%x", NULL }
};

/** A parser for EArchCommonObjStaInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonStaInfoParser[] = {
  { "DeviceStatus", 4, "0x%x", NULL }
};

/** A parser for EArchCommonObjMemoryRangeDescriptor.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonObjMemoryRangeDescriptor[] = {
  { "BaseAddress", sizeof (UINT64), "0x%llx", NULL },
  { "Length",      sizeof (UINT64), "0x%llx", NULL },
};

/** A parser for EArchCommonObjGenericDbg2DeviceInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonObjDbg2DeviceInfo[] = {
  { "AddressResourceToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL        },
  { "PortType",             sizeof (UINT16),          "0x%x", NULL        },
  { "PortSubtype",          sizeof (UINT16),          "0x%x", NULL        },
  { "AccessSize",           sizeof (UINT8),           "0x%x", NULL        },
  { "ObjectName",           AML_NAME_SEG_SIZE + 1,    NULL,   PrintString }
};

/** A parser for EArchCommonObjCxlHostBridgeInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonObjCxlHostBridgeInfo[] = {
  { "Token",                 sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
  { "Uid",                   sizeof (UINT32),          "0x%x",   NULL },
  { "Version",               sizeof (UINT32),          "0x%x",   NULL },
  { "ComponentRegisterBase", sizeof (UINT64),          "0x%llx", NULL },
};

STATIC CONST CM_OBJ_PARSER  CmArchCommonObjInterleaveTargetTokenParser[] = {
  { "InterleaveTargetToken[0]",  sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "InterleaveTargetToken[1]",  sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "InterleaveTargetToken[2]",  sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "InterleaveTargetToken[3]",  sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "InterleaveTargetToken[4]",  sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "InterleaveTargetToken[5]",  sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "InterleaveTargetToken[6]",  sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "InterleaveTargetToken[7]",  sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "InterleaveTargetToken[8]",  sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "InterleaveTargetToken[9]",  sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "InterleaveTargetToken[10]", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "InterleaveTargetToken[11]", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "InterleaveTargetToken[12]", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "InterleaveTargetToken[13]", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "InterleaveTargetToken[14]", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "InterleaveTargetToken[15]", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
};

/** A parser for EArchCommonObjCxlFixedMemoryWindowInfo
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonObjCxlFixedMemoryWindowInfo[] = {
  { "BaseHostPhysicalAddress",         sizeof (UINT64), "0x%llx", NULL },
  { "WindowSizeBytes",                 sizeof (UINT64), "0x%llx", NULL },
  { "NumberOfInterleaveWays",          sizeof (UINT8),  "%d",     NULL },
  { "InterleaveArithmetic",            sizeof (UINT8),  "%d",     NULL },
  { "HostBridgeInterleaveGranularity", sizeof (UINT32), "0x%x",   NULL },
  { "WindowRestrictions",              sizeof (UINT16), "0x%x",   NULL },
  { "QtgId",                           sizeof (UINT16), "%d",     NULL },
  { "InterleaveTargetTokens",
    sizeof (CM_OBJECT_TOKEN) * CFMWS_MAX_INTERLEAVE_WAYS,
    NULL,
    NULL,
    CmArchCommonObjInterleaveTargetTokenParser,
    ARRAY_SIZE (CmArchCommonObjInterleaveTargetTokenParser) },
};

/** A parser for EArchCommonObjGenericDeviceInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCommonGenericDeviceInfoParser[] = {
  { "Name",                   AML_NAME_SEG_SIZE + 1,    NULL,   PrintString },
  { "Hid",                    8+1,                      NULL,   PrintString },
  { "CidValid",               1,                        "%d",   NULL        },
  { "Cid",                    8+1,                      NULL,   PrintString },
  { "Uid",                    4,                        "0x%x", NULL        },
  { "HrvValid",               1,                        "%d",   NULL        },
  { "Hrv",                    4,                        "0x%x", NULL        },
  { "Cca",                    1,                        "%d",   NULL        },
  { "AddressResourceToken",   sizeof (CM_OBJECT_TOKEN), "0x%p", NULL        },
  { "InterruptResourceToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL        }
};

/** A parser for Arch Common namespace objects.
*/
STATIC CONST CM_OBJ_PARSER_ARRAY  ArchCommonNamespaceObjectParser[] = {
  CM_PARSER_ADD_OBJECT_RESERVED (EArchCommonObjReserved),
  CM_PARSER_ADD_OBJECT (EArchCommonObjPowerManagementProfileInfo,  CmArchCommonPowerManagementProfileInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjSerialPortInfo,              CmArchCommonSerialPortInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjConsolePortInfo,             CmArchCommonSerialPortInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjSerialDebugPortInfo,         CmArchCommonSerialPortInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjHypervisorVendorIdentity,    CmArchCommonHypervisorVendorIdentityParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjFixedFeatureFlags,           CmArchCommonFixedFeatureFlagsParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjCmRef,                       CmArchCommonObjRefParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjPciConfigSpaceInfo,          CmArchCommonPciConfigSpaceInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjPciAddressMapInfo,           CmArchCommonPciAddressMapInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjPciInterruptMapInfo,         CmArchCommonPciInterruptMapInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjMemoryAffinityInfo,          CmArchCommonMemoryAffinityInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjDeviceHandleAcpi,            CmArchCommonDeviceHandleAcpiParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjDeviceHandlePci,             CmArchCommonDeviceHandlePciParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjGenericInitiatorAffinityInfo,CmArchCommonGenericInitiatorAffinityInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjLpiInfo,                     CmArchCommonLpiInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjProcHierarchyInfo,           CmArchCommonProcHierarchyInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjCacheInfo,                   CmArchCommonCacheInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjCpcInfo,                     CmArchCommonCpcInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjPccSubspaceType0Info,        CmArchCommonPccSubspaceType0InfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjPccSubspaceType1Info,        CmArchCommonPccSubspaceType1InfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjPccSubspaceType2Info,        CmArchCommonPccSubspaceType2InfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjPccSubspaceType3Info,        CmArchCommonPccSubspaceType34InfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjPccSubspaceType4Info,        CmArchCommonPccSubspaceType34InfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjPccSubspaceType5Info,        CmArchCommonPccSubspaceType5InfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjPsdInfo,                     CmArchCommonPsdInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjTpm2InterfaceInfo,           CmArchCommonTpm2InterfaceInfo),
  CM_PARSER_ADD_OBJECT (EArchCommonObjSpmiInterfaceInfo,           CmArchCommonSpmiInterfaceInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjSpmiInterruptDeviceInfo,     CmArchCommonSpmiInterruptDeviceInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjCstInfo,                     CmArchCommonCstInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjCsdInfo,                     CmArchCommonCsdInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjPctInfo,                     CmArchCommonPctInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjPssInfo,                     CmArchCommonPssInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjPpcInfo,                     CmArchCommonPpcInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjStaInfo,                     CmArchCommonStaInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjMemoryRangeDescriptor,       CmArchCommonObjMemoryRangeDescriptor),
  CM_PARSER_ADD_OBJECT (EArchCommonObjGenericDbg2DeviceInfo,       CmArchCommonObjDbg2DeviceInfo),
  CM_PARSER_ADD_OBJECT (EArchCommonObjCxlHostBridgeInfo,           CmArchCommonObjCxlHostBridgeInfo),
  CM_PARSER_ADD_OBJECT (EArchCommonObjCxlFixedMemoryWindowInfo,    CmArchCommonObjCxlFixedMemoryWindowInfo),
  CM_PARSER_ADD_OBJECT (EArchCommonObjGenericDeviceInfo,           CmArchCommonGenericDeviceInfoParser),
  CM_PARSER_ADD_OBJECT (EArchCommonObjGenericInterrupt,            CmArchCommonGenericInterruptParser),
  CM_PARSER_ADD_OBJECT_RESERVED (EArchCommonObjMax)
};

/** A parser for Arm namespace objects.
*/
STATIC CONST CM_OBJ_PARSER_ARRAY  ArmNamespaceObjectParser[] = {
  CM_PARSER_ADD_OBJECT_RESERVED (EArmObjReserved),
  CM_PARSER_ADD_OBJECT (EArmObjBootArchInfo,               CmArmBootArchInfoParser),
  CM_PARSER_ADD_OBJECT (EArmObjGicCInfo,                   CmArmGicCInfoParser),
  CM_PARSER_ADD_OBJECT (EArmObjGicDInfo,                   CmArmGicDInfoParser),
  CM_PARSER_ADD_OBJECT (EArmObjGicMsiFrameInfo,            CmArmGicMsiFrameInfoParser),
  CM_PARSER_ADD_OBJECT (EArmObjGicRedistributorInfo,       CmArmGicRedistInfoParser),
  CM_PARSER_ADD_OBJECT (EArmObjGicItsInfo,                 CmArmGicItsInfoParser),
  CM_PARSER_ADD_OBJECT (EArmObjGenericTimerInfo,           CmArmGenericTimerInfoParser),
  CM_PARSER_ADD_OBJECT (EArmObjPlatformGTBlockInfo,        CmArmGTBlockInfoParser),
  CM_PARSER_ADD_OBJECT (EArmObjGTBlockTimerFrameInfo,      CmArmGTBlockTimerFrameInfoParser),
  CM_PARSER_ADD_OBJECT (EArmObjPlatformGenericWatchdogInfo,CmArmGenericWatchdogInfoParser),
  CM_PARSER_ADD_OBJECT (EArmObjItsGroup,                   CmArmItsGroupNodeParser),
  CM_PARSER_ADD_OBJECT (EArmObjNamedComponent,             CmArmNamedComponentNodeParser),
  CM_PARSER_ADD_OBJECT (EArmObjRootComplex,                CmArmRootComplexNodeParser),
  CM_PARSER_ADD_OBJECT (EArmObjSmmuV1SmmuV2,               CmArmSmmuV1SmmuV2NodeParser),
  CM_PARSER_ADD_OBJECT (EArmObjSmmuV3,                     CmArmSmmuV3NodeParser),
  CM_PARSER_ADD_OBJECT (EArmObjPmcg,                       CmArmPmcgNodeParser),
  CM_PARSER_ADD_OBJECT (EArmObjGicItsIdentifierArray,      CmArmGicItsIdentifierParser),
  CM_PARSER_ADD_OBJECT (EArmObjIdMappingArray,             CmArmIdMappingParser),
  CM_PARSER_ADD_OBJECT (EArmObjSmmuInterruptArray,         CmArchCommonGenericInterruptParser),
  CM_PARSER_ADD_OBJECT (EArmObjCmn600Info,                 CmArmCmn600InfoParser),
  CM_PARSER_ADD_OBJECT (EArmObjRmr,                        CmArmRmrInfoParser),
  CM_PARSER_ADD_OBJECT (EArmObjMemoryRangeDescriptor,      CmArmMemoryRangeDescriptorInfoParser),
  CM_PARSER_ADD_OBJECT (EArmObjEtInfo,                     CmArmEtInfo),
  CM_PARSER_ADD_OBJECT_RESERVED (EArmObjMax)
};

/** A parser for EX64ObjFadtSciInterrupt.
*/
STATIC CONST CM_OBJ_PARSER  CmX64ObjFadtSciInterruptParser[] = {
  { "SciInterrupt", 2, "0x%x", NULL }
};

/** A parser for EX64ObjFadtSciCmdInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmX64ObjFadtSciCmdInfoParser[] = {
  { "SciCmd",      4, "0x%x", NULL },
  { "AcpiEnable",  1, "0x%x", NULL },
  { "AcpiDisable", 1, "0x%x", NULL },
  { "S4BiosReq",   1, "0x%x", NULL },
  { "PstateCnt",   1, "0x%x", NULL },
  { "CstCnt",      1, "0x%x", NULL }
};

/** A parser for EX64ObjFadtPmBlockInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmX64ObjFadtPmBlockInfoParser[] = {
  { "Pm1aEvtBlk", 4, "0x%x", NULL },
  { "Pm1bEvtBlk", 4, "0x%x", NULL },
  { "Pm1aCntBlk", 4, "0x%x", NULL },
  { "Pm1bCntBlk", 4, "0x%x", NULL },
  { "Pm2CntBlk",  4, "0x%x", NULL },
  { "PmTmrBlk",   4, "0x%x", NULL },
  { "Pm1EvtLen",  1, "0x%x", NULL },
  { "Pm1CntLen",  1, "0x%x", NULL },
  { "Pm2CntLen",  1, "0x%x", NULL },
  { "PmTmrLen",   1, "0x%x", NULL }
};

/** A parser for EX64ObjFadtGpeBlockInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmX64ObjFadtGpeBlockInfoParser[] = {
  { "Gpe0Blk",    4, "0x%x", NULL },
  { "Gpe1Blk",    4, "0x%x", NULL },
  { "Gpe0BlkLen", 1, "0x%x", NULL },
  { "Gpe1BlkLen", 1, "0x%x", NULL },
  { "Gpe1Base",   1, "0x%x", NULL }
};

/** A parser for EX64ObjFadtXpmBlockInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmX64ObjFadtXpmBlockInfoParser[] = {
  { "XPm1aEvtBlk", sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "XPm1bEvtBlk", sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "XPm1aCntBlk", sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "XPm1bCntBlk", sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "XPm2CntBlk",  sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "XPmTmrBlk",   sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) }
};

/** A parser for EX64ObjFadtXgpeBlockInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmX64ObjFadtXgpeBlockInfoParser[] = {
  { "XGpe0Blk", sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "XGpe1Blk", sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) }
};

/** A parser for EX64ObjFadtSleepBlockInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmX64ObjFadtSleepBlockInfoParser[] = {
  { "SleepControlReg", sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "SleepStatusReg",  sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) }
};

/** A parser for EX64ObjFadtResetBlockInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmX64ObjFadtResetBlockInfoParser[] = {
  { "ResetReg",   sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser) },
  { "ResetValue", 1,                                              "0x%x",NULL }
};

/** A parser for EX64ObjFadtMiscInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmX64ObjFadtMiscInfoParser[] = {
  { "PLvl2Lat",    2, "0x%x", NULL },
  { "PLvl3Lat",    2, "0x%x", NULL },
  { "FlushSize",   2, "0x%x", NULL },
  { "FlushStride", 2, "0x%x", NULL },
  { "DutyOffset",  1, "0x%x", NULL },
  { "DutyWidth",   1, "0x%x", NULL },
  { "DayAlrm",     1, "0x%x", NULL },
  { "MonAlrm",     1, "0x%x", NULL },
  { "Century",     1, "0x%x", NULL }
};

/** A parser for EX64ObjWsmtFlagsInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmX64ObjWsmtFlagsInfoParser[] = {
  { "WsmtFlags", 4, "0x%x", NULL }
};

/** A parser for EX64ObjHpetInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmX64ObjHpetInfoParser[] = {
  { "BaseAddressLower32Bit",                     4, "0x%x", NULL },
  { "MainCounterMinimumClockTickInPeriodicMode", 2, "0x%x", NULL },
  { "PageProtectionAndOemAttribute",             1, "0x%x", NULL }
};

/** A parser for EX64ObjMadtInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmX64ObjMadtInfoParser[] = {
  { "LocalApicAddress", 4,                        "0x%x", NULL },
  { "Flags",            4,                        "0x%x", NULL },
  { "ApicMode",         sizeof (LOCAL_APIC_MODE), "0x%x", NULL }
};

/** A parser for CmArchCommonLocalApicX2ApicInfoParser.
*/
STATIC CONST CM_OBJ_PARSER  CmX64ObjLocalApicX2ApicInfoParser[] = {
  { "ApicId",           4,                        "0x%x", NULL },
  { "Flags",            4,                        "0x%x", NULL },
  { "AcpiProcessorUid", 4,                        "0x%x", NULL },
  { "CstToken",         sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "CsdToken",         sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "PctToken",         sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "PssToken",         sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "PpcToken",         sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "PsdToken",         sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "CpcToken",         sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "StaToken",         sizeof (CM_OBJECT_TOKEN), "0x%p", NULL }
};

/** A parser for CmX64IoApicInfoParser.
*/
STATIC CONST CM_OBJ_PARSER  CmX64IoApicInfoParser[] = {
  { "IoApicId",                  1, "0x%x", NULL },
  { "IoApicAddress",             4, "0x%x", NULL },
  { "GlobalSystemInterruptBase", 4, "0x%x", NULL }
};

/** A parser for CmX64IntrSourceOverrideInfoParser.
*/
STATIC CONST CM_OBJ_PARSER  CmX64IntrSourceOverrideInfoParser[] = {
  { "Bus",                   1, "0x%x", NULL },
  { "Source",                1, "0x%x", NULL },
  { "GlobalSystemInterrupt", 4, "0x%x", NULL },
  { "Flags",                 2, "0x%x", NULL }
};

/** A parser for CmX64LocalApicNmiInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmX64LocalApicNmiInfo[] = {
  { "Flags",            2, "0x%x", NULL },
  { "AcpiProcessorUid", 4, "0x%x", NULL },
  { "LocalApicLint",    1, "0x%x", NULL }
};

/** A parser for EX64ObjFacsInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmX64ObjFacsInfoParser[] = {
  { "FirmwareWakingVector",  4, "0x%x",   NULL },
  { "Flags",                 4, "0x%x",   NULL },
  { "XFirmwareWakingVector", 8, "0x%llx", NULL },
  { "OspmFlags",             4, "0x%x",   NULL }
};

/** A parser for CmX64LocalApicX2ApicAffinityInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmX64LocalApicX2ApicAffinityInfo[] = {
  { "ApicMode",        sizeof (LOCAL_APIC_MODE), "0x%x", NULL },
  { "ApicId",          4,                        "0x%x", NULL },
  { "ProximityDomain", 4,                        "0x%x", NULL },
  { "Flags",           4,                        "0x%x", NULL },
  { "ClockDomain",     4,                        "0x%x", NULL }
};

/** A parser for X64 namespace objects.
*/
STATIC CONST CM_OBJ_PARSER_ARRAY  X64NamespaceObjectParser[] = {
  CM_PARSER_ADD_OBJECT_RESERVED (EX64ObjReserved),
  CM_PARSER_ADD_OBJECT (EX64ObjFadtSciInterrupt,           CmX64ObjFadtSciInterruptParser),
  CM_PARSER_ADD_OBJECT (EX64ObjFadtSciCmdInfo,             CmX64ObjFadtSciCmdInfoParser),
  CM_PARSER_ADD_OBJECT (EX64ObjFadtPmBlockInfo,            CmX64ObjFadtPmBlockInfoParser),
  CM_PARSER_ADD_OBJECT (EX64ObjFadtGpeBlockInfo,           CmX64ObjFadtGpeBlockInfoParser),
  CM_PARSER_ADD_OBJECT (EX64ObjFadtXpmBlockInfo,           CmX64ObjFadtXpmBlockInfoParser),
  CM_PARSER_ADD_OBJECT (EX64ObjFadtXgpeBlockInfo,          CmX64ObjFadtXgpeBlockInfoParser),
  CM_PARSER_ADD_OBJECT (EX64ObjFadtSleepBlockInfo,         CmX64ObjFadtSleepBlockInfoParser),
  CM_PARSER_ADD_OBJECT (EX64ObjFadtResetBlockInfo,         CmX64ObjFadtResetBlockInfoParser),
  CM_PARSER_ADD_OBJECT (EX64ObjFadtMiscInfo,               CmX64ObjFadtMiscInfoParser),
  CM_PARSER_ADD_OBJECT (EX64ObjWsmtFlagsInfo,              CmX64ObjWsmtFlagsInfoParser),
  CM_PARSER_ADD_OBJECT (EX64ObjHpetInfo,                   CmX64ObjHpetInfoParser),
  CM_PARSER_ADD_OBJECT (EX64ObjMadtInfo,                   CmX64ObjMadtInfoParser),
  CM_PARSER_ADD_OBJECT (EX64ObjLocalApicX2ApicInfo,        CmX64ObjLocalApicX2ApicInfoParser),
  CM_PARSER_ADD_OBJECT (EX64ObjIoApicInfo,                 CmX64IoApicInfoParser),
  CM_PARSER_ADD_OBJECT (EX64ObjIntrSourceOverrideInfo,     CmX64IntrSourceOverrideInfoParser),
  CM_PARSER_ADD_OBJECT (EX64ObjLocalApicX2ApicNmiInfo,     CmX64LocalApicNmiInfo),
  CM_PARSER_ADD_OBJECT (EX64ObjFacsInfo,                   CmX64ObjFacsInfoParser),
  CM_PARSER_ADD_OBJECT (EX64ObjLocalApicX2ApicAffinityInfo,CmX64LocalApicX2ApicAffinityInfo),
  CM_PARSER_ADD_OBJECT_RESERVED (EX64ObjMax)
};

/** A parser for EStdObjCfgMgrInfo.
*/
STATIC CONST CM_OBJ_PARSER  StdObjCfgMgrInfoParser[] = {
  { "Revision", 4, "0x%x", NULL       },
  { "OemId[6]", 6, NULL,   PrintChars }
};

/** A parser for EStdObjAcpiTableList.
*/
STATIC CONST CM_OBJ_PARSER  StdObjAcpiTableInfoParser[] = {
  { "AcpiTableSignature", 4,                                      NULL,   PrintChars },
  { "AcpiTableRevision",  1,                                      "%d",   NULL       },
  { "TableGeneratorId",   sizeof (ACPI_TABLE_GENERATOR_ID),       "0x%x", NULL       },
  { "AcpiTableData",      sizeof (EFI_ACPI_DESCRIPTION_HEADER *), "0x%p", NULL       },
  { "OemTableId",         8,                                      NULL,   PrintChars },
  { "OemRevision",        4,                                      "0x%x", NULL       },
  { "MinorRevision",      1,                                      "0x%x", NULL       },
};

/** A parser for EStdObjSmbiosTableList.
*/
STATIC CONST CM_OBJ_PARSER  StdObjSmbiosTableInfoParser[] = {
  { "TableGeneratorId", sizeof (SMBIOS_TABLE_GENERATOR_ID), "0x%x", NULL },
  { "SmbiosTableData",  sizeof (SMBIOS_STRUCTURE *),        "0x%p", NULL }
};

/** A parser for Standard namespace objects.
*/
STATIC CONST CM_OBJ_PARSER_ARRAY  StdNamespaceObjectParser[] = {
  CM_PARSER_ADD_OBJECT (EStdObjCfgMgrInfo,      StdObjCfgMgrInfoParser),
  CM_PARSER_ADD_OBJECT (EStdObjAcpiTableList,   StdObjAcpiTableInfoParser),
  CM_PARSER_ADD_OBJECT (EStdObjSmbiosTableList, StdObjSmbiosTableInfoParser),
  CM_PARSER_ADD_OBJECT_RESERVED (EStdObjMax)
};

/** Print string data.

  The string must be NULL terminated.

  @param [in]  Format  Format to print the Ptr.
  @param [in]  Ptr     Pointer to the string.
  @param [in]  Length  Length of the field
**/
STATIC
VOID
EFIAPI
PrintString (
  IN CONST CHAR8  *Format,
  IN UINT8        *Ptr,
  IN UINT32       Length
  )
{
  if (Ptr == NULL) {
    ASSERT (0);
    return;
  }

  DEBUG ((DEBUG_INFO, "%a", Ptr));
}

/** Print string from pointer.

  The string must be NULL terminated.

  @param [in]  Format      Format to print the string.
  @param [in]  Ptr         Pointer to the string pointer.
  @param [in]  Length      Length of the field
**/
STATIC
VOID
EFIAPI
PrintStringPtr (
  IN CONST CHAR8  *Format,
  IN UINT8        *Ptr,
  IN UINT32       Length
  )
{
  UINT8  *String;

  if (Ptr == NULL) {
    ASSERT (0);
    return;
  }

  String = *(UINT8 **)Ptr;

  if (String == NULL) {
    String = (UINT8 *)"(NULLPTR)";
  }

  PrintString (Format, String, Length);
}

/** Print characters.

  @param [in]  Format  Format to print the Ptr.
  @param [in]  Ptr     Pointer to the characters.
  @param [in]  Length  Length of the field
**/
STATIC
VOID
EFIAPI
PrintChars (
  IN  CONST CHAR8  *Format,
  IN  UINT8        *Ptr,
  IN  UINT32       Length
  )
{
  UINT32  Index;

  for (Index = 0; Index < Length; Index++) {
    DEBUG ((DEBUG_INFO, "%c", Ptr[Index]));
  }
}

/** Dump data in Hex format

  @param [in]  Format  Format to print the Ptr.
  @param [in]  Ptr     Pointer to the string.
  @param [in]  Length  Length of the field
**/
STATIC
VOID
EFIAPI
HexDump (
  IN CONST CHAR8  *Format,
  IN UINT8        *Ptr,
  IN UINT32       Length
  )
{
  UINT32  Index;

  for (Index = 0; Index < Length; Index++) {
    DEBUG ((DEBUG_INFO, "0x%02x ", *Ptr++));
  }
}

/** Print fields of the objects.

  @param [in]  Data           Pointer to the object to print.
  @param [in]  Parser         Parser containing the object fields.
  @param [in]  ItemCount      Number of entries/fields in the Parser.
  @param [in]  RemainingSize  Parse at most *RemainingSize bytes.
                              This function decrements the value
                              from the number bytes consumed.
  @param [in]  IndentLevel    Indentation to use when printing.
**/
STATIC
VOID
PrintCmObjDesc (
  IN        VOID           *Data,
  IN  CONST CM_OBJ_PARSER  *Parser,
  IN        UINTN          ItemCount,
  IN        INTN           *RemainingSize,
  IN        UINT32         IndentLevel
  )
{
  UINT32  Index;
  UINT32  IndentIndex;
  INTN    SubStructSize;

  if ((Data == NULL)    ||
      (Parser == NULL)  ||
      (ItemCount == 0)  ||
      (RemainingSize == NULL))
  {
    ASSERT (0);
    return;
  }

  // Print each field.
  for (Index = 0; Index < ItemCount; Index++) {
    // Check there is enough space in left.
    *RemainingSize -= Parser[Index].Length;
    if (*RemainingSize < 0) {
      DEBUG ((
        DEBUG_ERROR,
        "\nERROR: %a: Buffer overrun\n",
        Parser[Index].NameStr
        ));
      ASSERT (0);
      return;
    }

    // Indentation
    for (IndentIndex = 0; IndentIndex < IndentLevel; IndentIndex++) {
      DEBUG ((DEBUG_INFO, "  "));
    }

    DEBUG ((
      DEBUG_INFO,
      "%-*a :",
      OUTPUT_FIELD_COLUMN_WIDTH - 2 * IndentLevel,
      Parser[Index].NameStr
      ));
    if (Parser[Index].PrintFormatter != NULL) {
      Parser[Index].PrintFormatter (Parser[Index].Format, Data, Parser[Index].Length);
    } else if (Parser[Index].Format != NULL) {
      switch (Parser[Index].Length) {
        case 1:
          DEBUG ((DEBUG_INFO, Parser[Index].Format, *(UINT8 *)Data));
          break;
        case 2:
          DEBUG ((DEBUG_INFO, Parser[Index].Format, *(UINT16 *)Data));
          break;
        case 4:
          DEBUG ((DEBUG_INFO, Parser[Index].Format, *(UINT32 *)Data));
          break;
        case 8:
          DEBUG ((DEBUG_INFO, Parser[Index].Format, ReadUnaligned64 (Data)));
          break;
        default:
          DEBUG ((
            DEBUG_INFO,
            "\nERROR: %a: CANNOT PARSE THIS FIELD, Field Length = %d\n",
            Parser[Index].NameStr,
            Parser[Index].Length
            ));
      } // switch
    } else if (Parser[Index].SubObjParser != NULL) {
      SubStructSize = Parser[Index].Length;

      DEBUG ((DEBUG_INFO, "\n"));
      PrintCmObjDesc (
        Data,
        Parser[Index].SubObjParser,
        Parser[Index].SubObjItemCount,
        &SubStructSize,
        IndentLevel + 1
        );
    } else {
      ASSERT (0);
      DEBUG ((
        DEBUG_INFO,
        "\nERROR: %a: CANNOT PARSE THIS FIELD, Field Length = %d\n",
        Parser[Index].NameStr,
        Parser[Index].Length
        ));
    }

    DEBUG ((DEBUG_INFO, "\n"));
    Data = (UINT8 *)Data + Parser[Index].Length;
  } // for
}

/** Parse and print a CmObjDesc.

  @param [in]  CmObjDesc  The CmObjDesc to parse and print.
**/
VOID
EFIAPI
ParseCmObjDesc (
  IN  CONST CM_OBJ_DESCRIPTOR  *CmObjDesc
  )
{
  UINTN                       ObjId;
  UINTN                       NameSpaceId;
  UINT32                      ObjIndex;
  UINT32                      ObjectCount;
  INTN                        RemainingSize;
  INTN                        Offset;
  CONST  CM_OBJ_PARSER_ARRAY  *ParserArray;

  if ((CmObjDesc == NULL) || (CmObjDesc->Data == NULL)) {
    return;
  }

  NameSpaceId = GET_CM_NAMESPACE_ID (CmObjDesc->ObjectId);
  ObjId       = GET_CM_OBJECT_ID (CmObjDesc->ObjectId);

  switch (NameSpaceId) {
    case EObjNameSpaceStandard:
      if (ObjId >= EStdObjMax) {
        ASSERT (0);
        return;
      }

      if (ObjId >= ARRAY_SIZE (StdNamespaceObjectParser)) {
        DEBUG ((DEBUG_ERROR, "ObjId 0x%x is missing from the StdNamespaceObjectParser array\n", ObjId));
        ASSERT (0);
        return;
      }

      ParserArray = &StdNamespaceObjectParser[ObjId];
      break;
    case EObjNameSpaceArm:
      if (ObjId >= EArmObjMax) {
        ASSERT (0);
        return;
      }

      if (ObjId >= ARRAY_SIZE (ArmNamespaceObjectParser)) {
        DEBUG ((DEBUG_ERROR, "ObjId 0x%x is missing from the ArmNamespaceObjectParser array\n", ObjId));
        ASSERT (0);
        return;
      }

      ParserArray = &ArmNamespaceObjectParser[ObjId];
      break;

    case EObjNameSpaceArchCommon:
      if (ObjId >= EArchCommonObjMax) {
        ASSERT (0);
        return;
      }

      if (ObjId >= ARRAY_SIZE (ArchCommonNamespaceObjectParser)) {
        DEBUG ((DEBUG_ERROR, "ObjId 0x%x is missing from the ArchCommonNamespaceObjectParser array\n", ObjId));
        ASSERT (0);
        return;
      }

      ParserArray = &ArchCommonNamespaceObjectParser[ObjId];
      break;

    case EObjNameSpaceX64:
      if (ObjId >= EX64ObjMax) {
        ASSERT (0);
        return;
      }

      if (ObjId >= ARRAY_SIZE (X64NamespaceObjectParser)) {
        DEBUG ((DEBUG_ERROR, "ObjId 0x%x is missing from the X64NamespaceObjectParser array\n", ObjId));
        ASSERT (0);
        return;
      }

      ParserArray = &X64NamespaceObjectParser[ObjId];
      break;

    default:
      // Not supported
      DEBUG ((DEBUG_ERROR, "NameSpaceId 0x%x, ObjId 0x%x is not supported by the parser\n", NameSpaceId, ObjId));
      ASSERT (0);
      return;
  } // switch

  ObjectCount   = CmObjDesc->Count;
  RemainingSize = CmObjDesc->Size;
  Offset        = 0;

  for (ObjIndex = 0; ObjIndex < ObjectCount; ObjIndex++) {
    DEBUG ((
      DEBUG_INFO,
      "\n%-*a [%d/%d]:\n",
      OUTPUT_FIELD_COLUMN_WIDTH,
      ParserArray->ObjectName,
      ObjIndex + 1,
      ObjectCount
      ));

    ASSERT (ObjId == ParserArray->ObjectId);

    if (ParserArray->Parser == NULL) {
      DEBUG ((DEBUG_ERROR, "Parser not implemented\n"));
      RemainingSize = 0;
    } else {
      PrintCmObjDesc (
        (VOID *)((UINTN)CmObjDesc->Data + Offset),
        ParserArray->Parser,
        ParserArray->ItemCount,
        &RemainingSize,
        1
        );
      if ((RemainingSize > (INTN)CmObjDesc->Size) ||
          (RemainingSize < 0))
      {
        ASSERT (0);
        return;
      }

      Offset = CmObjDesc->Size - RemainingSize;
    }
  } // for

  ASSERT (RemainingSize == 0);
}
