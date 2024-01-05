/** @file
  Configuration Manager Object parser.

  Copyright (c) 2021 - 2023, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <ConfigurationManagerObject.h>
#include "ConfigurationManagerObjectParser.h"

STATIC
VOID
EFIAPI
PrintString (
  CONST CHAR8  *Format,
  UINT8        *Ptr
  );

STATIC
VOID
EFIAPI
PrintStringPtr (
  CONST CHAR8  *Format,
  UINT8        *Ptr
  );

STATIC
VOID
EFIAPI
PrintChar4 (
  CONST CHAR8  *Format,
  UINT8        *Ptr
  );

STATIC
VOID
EFIAPI
PrintChar6 (
  CONST CHAR8  *Format,
  UINT8        *Ptr
  );

STATIC
VOID
EFIAPI
PrintChar8 (
  CONST CHAR8  *Format,
  UINT8        *Ptr
  );

/** A parser for EArchObjBootArchInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchBootArchInfoParser[] = {
  { "BootArchFlags", 2, "0x%x", NULL }
};

/** A parser for EArchObjPowerManagementProfileInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchPowerManagementProfileInfoParser[] = {
  { "PowerManagementProfile", 1, "0x%x", NULL }
};

/** A parser for EArchObjGicCInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchGicCInfoParser[] = {
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
  { "EtToken",                       sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL }
};

/** A parser for EArchObjGicDInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchGicDInfoParser[] = {
  { "PhysicalBaseAddress", 8, "0x%llx", NULL },
  { "SystemVectorBase",    4, "0x%x",   NULL },
  { "GicVersion",          1, "0x%x",   NULL },
};

/** A parser for EArchObjGicMsiFrameInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchGicMsiFrameInfoParser[] = {
  { "GicMsiFrameId",       4, "0x%x",   NULL },
  { "PhysicalBaseAddress", 8, "0x%llx", NULL },
  { "Flags",               4, "0x%x",   NULL },
  { "SPICount",            2, "0x%x",   NULL },
  { "SPIBase",             2, "0x%x",   NULL }
};

/** A parser for EArchObjGicRedistributorInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchGicRedistInfoParser[] = {
  { "DiscoveryRangeBaseAddress", 8, "0x%llx", NULL },
  { "DiscoveryRangeLength",      4, "0x%x",   NULL }
};

/** A parser for EArchObjGicItsInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchGicItsInfoParser[] = {
  { "GicItsId",            4, "0x%x",   NULL },
  { "PhysicalBaseAddress", 8, "0x%llx", NULL },
  { "ProximityDomain",     4, "0x%x",   NULL }
};

/** A parser for EArchObjSerialConsolePortInfo,
    EArchObjSerialDebugPortInfo and EArchObjSerialPortInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchSerialPortInfoParser[] = {
  { "BaseAddress",       8, "0x%llx", NULL },
  { "Interrupt",         4, "0x%x",   NULL },
  { "BaudRate",          8, "0x%llx", NULL },
  { "Clock",             4, "0x%x",   NULL },
  { "PortSubtype",       2, "0x%x",   NULL },
  { "BaseAddressLength", 8, "0x%llx", NULL },
  { "AccessSize",        1, "0x%d",   NULL }
};

/** A parser for EArchObjGenericTimerInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchGenericTimerInfoParser[] = {
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

/** A parser for EArchObjPlatformGTBlockInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchGTBlockInfoParser[] = {
  { "GTBlockPhysicalAddress", 8,                        "0x%llx", NULL },
  { "GTBlockTimerFrameCount", 4,                        "0x%x",   NULL },
  { "GTBlockTimerFrameToken", sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL }
};

/** A parser for EArchObjGTBlockTimerFrameInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchGTBlockTimerFrameInfoParser[] = {
  { "FrameNumber",               1, "0x%x",   NULL },
  { "PhysicalAddressCntBase",    8, "0x%llx", NULL },
  { "PhysicalAddressCntEL0Base", 8, "0x%llx", NULL },
  { "PhysicalTimerGSIV",         4, "0x%x",   NULL },
  { "PhysicalTimerFlags",        4, "0x%x",   NULL },
  { "VirtualTimerGSIV",          4, "0x%x",   NULL },
  { "VirtualTimerFlags",         4, "0x%x",   NULL },
  { "CommonFlags",               4, "0x%x",   NULL }
};

/** A parser for EArchObjPlatformGenericWatchdogInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchGenericWatchdogInfoParser[] = {
  { "ControlFrameAddress", 8, "0x%llx", NULL },
  { "RefreshFrameAddress", 8, "0x%llx", NULL },
  { "TimerGSIV",           4, "0x%x",   NULL },
  { "Flags",               4, "0x%x",   NULL }
};

/** A parser for EArchObjPciConfigSpaceInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchPciConfigSpaceInfoParser[] = {
  { "BaseAddress",           8,                        "0x%llx", NULL },
  { "PciSegmentGroupNumber", 2,                        "0x%x",   NULL },
  { "StartBusNumber",        1,                        "0x%x",   NULL },
  { "EndBusNumber",          1,                        "0x%x",   NULL },
  { "AddressMapToken",       sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
  { "InterruptMapToken",     sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
};

/** A parser for EArchObjHypervisorVendorIdentity.
*/
STATIC CONST CM_OBJ_PARSER  CmArchHypervisorVendorIdParser[] = {
  { "HypervisorVendorId", 8, "0x%llx", NULL }
};

/** A parser for EArchObjFixedFeatureFlags.
*/
STATIC CONST CM_OBJ_PARSER  CmArchFixedFeatureFlagsParser[] = {
  { "Flags", 4, "0x%x", NULL }
};

/** A parser for EArchObjItsGroup.
*/
STATIC CONST CM_OBJ_PARSER  CmArchItsGroupNodeParser[] = {
  { "Token",      sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "ItsIdCount", 4,                        "0x%x", NULL },
  { "ItsIdToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "Identifier", 4,                        "0x%x", NULL },
};

/** A parser for EArchObjNamedComponent.
*/
STATIC CONST CM_OBJ_PARSER  CmArchNamedComponentNodeParser[] = {
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

/** A parser for EArchObjRootComplex.
*/
STATIC CONST CM_OBJ_PARSER  CmArchRootComplexNodeParser[] = {
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

/** A parser for EArchObjSmmuV1SmmuV2.
*/
STATIC CONST CM_OBJ_PARSER  CmArchSmmuV1SmmuV2NodeParser[] = {
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

/** A parser for EArchObjSmmuV3.
*/
STATIC CONST CM_OBJ_PARSER  CmArchSmmuV3NodeParser[] = {
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

/** A parser for EArchObjPmcg.
*/
STATIC CONST CM_OBJ_PARSER  CmArchPmcgNodeParser[] = {
  { "Token",             sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
  { "IdMappingCount",    4,                        "0x%x",   NULL },
  { "IdMappingToken",    sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
  { "BaseAddress",       8,                        "0x%llx", NULL },
  { "OverflowInterrupt", 4,                        "0x%x",   NULL },
  { "Page1BaseAddress",  8,                        "0x%llx", NULL },
  { "ReferenceToken",    sizeof (CM_OBJECT_TOKEN), "0x%p",   NULL },
  { "Identifier",        4,                        "0x%x",   NULL },
};

/** A parser for EArchObjGicItsIdentifierArray.
*/
STATIC CONST CM_OBJ_PARSER  CmArchGicItsIdentifierParser[] = {
  { "ItsId", 4, "0x%x", NULL }
};

/** A parser for EArchObjIdMappingArray.
*/
STATIC CONST CM_OBJ_PARSER  CmArchIdMappingParser[] = {
  { "InputBase",            4,                        "0x%x", NULL },
  { "NumIds",               4,                        "0x%x", NULL },
  { "OutputBase",           4,                        "0x%x", NULL },
  { "OutputReferenceToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "Flags",                4,                        "0x%x", NULL }
};

/** A parser for EArchObjSmmuInterruptArray.
*/
STATIC CONST CM_OBJ_PARSER  CmArchGenericInterruptParser[] = {
  { "Interrupt", 4, "0x%x", NULL },
  { "Flags",     4, "0x%x", NULL }
};

/** A parser for EArchObjProcHierarchyInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchProcHierarchyInfoParser[] = {
  { "Token",                      sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "Flags",                      4,                        "0x%x", NULL },
  { "ParentToken",                sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "GicCToken",                  sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "NoOfPrivateResources",       4,                        "0x%x", NULL },
  { "PrivateResourcesArrayToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "LpiToken",                   sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "OverrideNameUidEnabled",     1,                        "%d",   NULL },
  { "OverrideName",               2,                        "0x%x", NULL },
  { "OverrideUid",                4,                        "0x%x", NULL }
};

/** A parser for EArchObjCacheInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCacheInfoParser[] = {
  { "Token",                 sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "NextLevelOfCacheToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "Size",                  4,                        "0x%x", NULL },
  { "NumberOfSets",          4,                        "0x%x", NULL },
  { "Associativity",         4,                        "0x%x", NULL },
  { "Attributes",            1,                        "0x%x", NULL },
  { "LineSize",              2,                        "0x%x", NULL },
  { "CacheId",               4,                        "0x%x", NULL },
};

/** A parser for EArchObjProcNodeIdInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchProcNodeIdInfoParser[] = {
  { "Token",    sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "VendorId", 4,                        "0x%p", NULL },
  { "Level1Id", 8,                        "0x%x", NULL },
  { "Level2Id", 8,                        "0x%x", NULL },
  { "MajorRev", 2,                        "0x%x", NULL },
  { "MinorRev", 2,                        "0x%x", NULL },
  { "SpinRev",  2,                        "0x%x", NULL }
};

/** A parser for EArchObjCmRef.
*/
STATIC CONST CM_OBJ_PARSER  CmArchObjRefParser[] = {
  { "ReferenceToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL }
};

/** A parser for EArchObjMemoryAffinityInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchMemoryAffinityInfoParser[] = {
  { "ProximityDomain", 4, "0x%x",   NULL },
  { "BaseAddress",     8, "0x%llx", NULL },
  { "Length",          8, "0x%llx", NULL },
  { "Flags",           4, "0x%x",   NULL }
};

/** A parser for EArchObjDeviceHandleAcpi.
*/
STATIC CONST CM_OBJ_PARSER  CmArchDeviceHandleAcpiParser[] = {
  { "Hid", 8, "0x%llx", NULL },
  { "Uid", 4, "0x%x",   NULL }
};

/** A parser for EArchObjDeviceHandlePci.
*/
STATIC CONST CM_OBJ_PARSER  CmArchDeviceHandlePciParser[] = {
  { "SegmentNumber",  2, "0x%x", NULL },
  { "BusNumber",      1, "0x%x", NULL },
  { "DeviceNumber",   1, "0x%x", NULL },
  { "FunctionNumber", 1, "0x%x", NULL }
};

/** A parser for EArchObjGenericInitiatorAffinityInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchGenericInitiatorAffinityInfoParser[] = {
  { "ProximityDomain",   4,                        "0x%x", NULL },
  { "Flags",             4,                        "0x%x", NULL },
  { "DeviceHandleType",  1,                        "0x%x", NULL },
  { "DeviceHandleToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL }
};

/** A parser for EArchObjCmn600Info.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCmn600InfoParser[] = {
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

/** A parser for EArchObjLpiInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchLpiInfoParser[] = {
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

/** A parser for EArchObjPciAddressMapInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchPciAddressMapInfoParser[] = {
  { "SpaceCode",   1, "%d",     NULL },
  { "PciAddress",  8, "0x%llx", NULL },
  { "CpuAddress",  8, "0x%llx", NULL },
  { "AddressSize", 8, "0x%llx", NULL },
};

/** A parser for EArchObjPciInterruptMapInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmPciInterruptMapInfoParser[] = {
  { "PciBus",        1,                                  "0x%x", NULL },
  { "PciDevice",     1,                                  "0x%x", NULL },
  { "PciInterrupt",  1,                                  "0x%x", NULL },
  { "IntcInterrupt", sizeof (CM_ARCH_GENERIC_INTERRUPT),
    NULL, NULL, CmArchGenericInterruptParser,
    ARRAY_SIZE (CmArchGenericInterruptParser) },
};

/** A parser for EArchObjRmr.
*/
STATIC CONST CM_OBJ_PARSER  CmArchRmrInfoParser[] = {
  { "Token",             sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "IdMappingCount",    4,                        "0x%x", NULL },
  { "IdMappingToken",    sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
  { "Identifier",        4,                        "0x%x", NULL },
  { "Flags",             4,                        "0x%x", NULL },
  { "MemRangeDescCount", 4,                        "0x%x", NULL },
  { "MemRangeDescToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL },
};

/** A parser for EArchObjMemoryRangeDescriptor.
*/
STATIC CONST CM_OBJ_PARSER  CmArchMemoryRangeDescriptorInfoParser[] = {
  { "BaseAddress", 8, "0x%llx", NULL },
  { "Length",      8, "0x%llx", NULL },
};

/** A parser for EArchObjCpcInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchCpcInfoParser[] = {
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
STATIC CONST CM_OBJ_PARSER  CmArchMailboxRegisterInfoParser[] = {
  { "Register",     sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE), NULL,     NULL,
    AcpiGenericAddressParser, ARRAY_SIZE (AcpiGenericAddressParser) },
  { "PreserveMask", 8,                                               "0x%llx", NULL },
  { "WriteMask",    8,                                               "0x%llx", NULL },
};

/** A parser for the PCC_SUBSPACE_CHANNEL_TIMING_INFO struct.
*/
STATIC CONST CM_OBJ_PARSER  CmArchPccSubspaceChannelTimingInfoParser[] = {
  { "NominalLatency",           4, "0x%x", NULL },
  { "MaxPeriodicAccessRate",    4, "0x%x", NULL },
  { "MinRequestTurnaroundTime", 2, "0x%x", NULL },
};

/** A parser for EArchObjPccSubspaceType0Info.
*/
STATIC CONST CM_OBJ_PARSER  CmArchPccSubspaceType0InfoParser[] = {
  { "SubspaceId",    1,                                         "0x%x",   NULL },
  { "Type",          1,                                         "0x%x",   NULL },
  { "BaseAddress",   8,                                         "0x%llx", NULL },
  { "AddressLength", 8,                                         "0x%llx", NULL },
  { "DoorbellReg",   sizeof (PCC_MAILBOX_REGISTER_INFO),
    NULL, NULL, CmArchMailboxRegisterInfoParser,
    ARRAY_SIZE (CmArchMailboxRegisterInfoParser) },
  { "ChannelTiming", sizeof (PCC_SUBSPACE_CHANNEL_TIMING_INFO),
    NULL, NULL, CmArchPccSubspaceChannelTimingInfoParser,
    ARRAY_SIZE (CmArchPccSubspaceChannelTimingInfoParser) },
};

/** A parser for EArchObjPccSubspaceType1Info.
*/
STATIC CONST CM_OBJ_PARSER  CmArchPccSubspaceType1InfoParser[] = {
  { "GenericPccInfo", sizeof (PCC_SUBSPACE_GENERIC_INFO),
    NULL, NULL, CmArchPccSubspaceType0InfoParser,
    ARRAY_SIZE (CmArchPccSubspaceType0InfoParser) },
  { "PlatIrq",        sizeof (CM_ARCH_GENERIC_INTERRUPT),
    NULL, NULL, CmArchGenericInterruptParser,
    ARRAY_SIZE (CmArchGenericInterruptParser) },
};

/** A parser for EArchObjPccSubspaceType2Info.
*/
STATIC CONST CM_OBJ_PARSER  CmArchPccSubspaceType2InfoParser[] = {
  { "GenericPccInfo", sizeof (PCC_SUBSPACE_GENERIC_INFO),
    NULL, NULL, CmArchPccSubspaceType0InfoParser,
    ARRAY_SIZE (CmArchPccSubspaceType0InfoParser) },
  { "PlatIrq",        sizeof (CM_ARCH_GENERIC_INTERRUPT),NULL,NULL,
    CmArchGenericInterruptParser, ARRAY_SIZE (CmArchGenericInterruptParser) },
  { "PlatIrqAckReg",  sizeof (PCC_MAILBOX_REGISTER_INFO),
    NULL, NULL, CmArchMailboxRegisterInfoParser,
    ARRAY_SIZE (CmArchMailboxRegisterInfoParser) },
};

/** A parser for EArchObjPccSubspaceType3Info or EArchObjPccSubspaceType4Info.
*/
STATIC CONST CM_OBJ_PARSER  CmArchPccSubspaceType34InfoParser[] = {
  { "GenericPccInfo",       sizeof (PCC_SUBSPACE_GENERIC_INFO),
    NULL, NULL, CmArchPccSubspaceType0InfoParser,
    ARRAY_SIZE (CmArchPccSubspaceType0InfoParser) },
  { "PlatIrq",              sizeof (CM_ARCH_GENERIC_INTERRUPT),NULL,NULL,
    CmArchGenericInterruptParser, ARRAY_SIZE (CmArchGenericInterruptParser) },
  { "PlatIrqAckReg",        sizeof (PCC_MAILBOX_REGISTER_INFO),
    NULL, NULL, CmArchMailboxRegisterInfoParser,
    ARRAY_SIZE (CmArchMailboxRegisterInfoParser) },
  { "CmdCompleteCheckReg",  sizeof (PCC_MAILBOX_REGISTER_INFO),
    NULL, NULL, CmArchMailboxRegisterInfoParser,
    ARRAY_SIZE (CmArchMailboxRegisterInfoParser) },
  { "CmdCompleteUpdateReg", sizeof (PCC_MAILBOX_REGISTER_INFO),
    NULL, NULL, CmArchMailboxRegisterInfoParser,
    ARRAY_SIZE (CmArchMailboxRegisterInfoParser) },
  { "ErrorStatusReg",       sizeof (PCC_MAILBOX_REGISTER_INFO),
    NULL, NULL, CmArchMailboxRegisterInfoParser,
    ARRAY_SIZE (CmArchMailboxRegisterInfoParser) },
};

/** A parser for EArchObjPccSubspaceType5Info.
*/
STATIC CONST CM_OBJ_PARSER  CmArchPccSubspaceType5InfoParser[] = {
  { "GenericPccInfo",      sizeof (PCC_SUBSPACE_GENERIC_INFO),
    NULL, NULL, CmArchPccSubspaceType0InfoParser,
    ARRAY_SIZE (CmArchPccSubspaceType0InfoParser) },
  { "Version",             2,                                 "0x%x",NULL },
  { "PlatIrq",             sizeof (CM_ARCH_GENERIC_INTERRUPT),NULL,  NULL,
    CmArchGenericInterruptParser, ARRAY_SIZE (CmArchGenericInterruptParser) },
  { "CmdCompleteCheckReg", sizeof (PCC_MAILBOX_REGISTER_INFO),
    NULL, NULL, CmArchMailboxRegisterInfoParser,
    ARRAY_SIZE (CmArchMailboxRegisterInfoParser) },
  { "ErrorStatusReg",      sizeof (PCC_MAILBOX_REGISTER_INFO),
    NULL, NULL, CmArchMailboxRegisterInfoParser,
    ARRAY_SIZE (CmArchMailboxRegisterInfoParser) },
};

/** A parser for EArchObjEtInfo.
*/
STATIC CONST CM_OBJ_PARSER  CmArchEtInfo[] = {
  { "EtType", sizeof (ARM_ET_TYPE), "0x%x", NULL }
};

/** A parser for Arm namespace objects.
*/
STATIC CONST CM_OBJ_PARSER_ARRAY  ArmNamespaceObjectParser[] = {
  { "EArchObjReserved",                     NULL,                                   0                                 },
  { "EArchObjBootArchInfo",                 CmArchBootArchInfoParser,
    ARRAY_SIZE (CmArchBootArchInfoParser) },
  { "EArchObjCpuInfo",                      NULL,                                   0                                 },
  { "EArchObjPowerManagementProfileInfo",   CmArchPowerManagementProfileInfoParser,
    ARRAY_SIZE (CmArchPowerManagementProfileInfoParser) },
  { "EArchObjGicCInfo",                     CmArchGicCInfoParser,                   ARRAY_SIZE (CmArchGicCInfoParser) },
  { "EArchObjGicDInfo",                     CmArchGicDInfoParser,                   ARRAY_SIZE (CmArchGicDInfoParser) },
  { "EArchObjGicMsiFrameInfo",              CmArchGicMsiFrameInfoParser,
    ARRAY_SIZE (CmArchGicMsiFrameInfoParser) },
  { "EArchObjGicRedistributorInfo",         CmArchGicRedistInfoParser,
    ARRAY_SIZE (CmArchGicRedistInfoParser) },
  { "EArchObjGicItsInfo",                   CmArchGicItsInfoParser,
    ARRAY_SIZE (CmArchGicItsInfoParser) },
  { "EArchObjSerialConsolePortInfo",        CmArchSerialPortInfoParser,
    ARRAY_SIZE (CmArchSerialPortInfoParser) },
  { "EArchObjSerialDebugPortInfo",          CmArchSerialPortInfoParser,
    ARRAY_SIZE (CmArchSerialPortInfoParser) },
  { "EArchObjGenericTimerInfo",             CmArchGenericTimerInfoParser,
    ARRAY_SIZE (CmArchGenericTimerInfoParser) },
  { "EArchObjPlatformGTBlockInfo",          CmArchGTBlockInfoParser,
    ARRAY_SIZE (CmArchGTBlockInfoParser) },
  { "EArchObjGTBlockTimerFrameInfo",        CmArchGTBlockTimerFrameInfoParser,
    ARRAY_SIZE (CmArchGTBlockTimerFrameInfoParser) },
  { "EArchObjPlatformGenericWatchdogInfo",  CmArchGenericWatchdogInfoParser,
    ARRAY_SIZE (CmArchGenericWatchdogInfoParser) },
  { "EArchObjPciConfigSpaceInfo",           CmArchPciConfigSpaceInfoParser,
    ARRAY_SIZE (CmArchPciConfigSpaceInfoParser) },
  { "EArchObjHypervisorVendorIdentity",     CmArchHypervisorVendorIdParser,
    ARRAY_SIZE (CmArchHypervisorVendorIdParser) },
  { "EArchObjFixedFeatureFlags",            CmArchFixedFeatureFlagsParser,
    ARRAY_SIZE (CmArchFixedFeatureFlagsParser) },
  { "EArchObjItsGroup",                     CmArchItsGroupNodeParser,
    ARRAY_SIZE (CmArchItsGroupNodeParser) },
  { "EArchObjNamedComponent",               CmArchNamedComponentNodeParser,
    ARRAY_SIZE (CmArchNamedComponentNodeParser) },
  { "EArchObjRootComplex",                  CmArchRootComplexNodeParser,
    ARRAY_SIZE (CmArchRootComplexNodeParser) },
  { "EArchObjSmmuV1SmmuV2",                 CmArchSmmuV1SmmuV2NodeParser,
    ARRAY_SIZE (CmArchSmmuV1SmmuV2NodeParser) },
  { "EArchObjSmmuV3",                       CmArchSmmuV3NodeParser,
    ARRAY_SIZE (CmArchSmmuV3NodeParser) },
  { "EArchObjPmcg",                         CmArchPmcgNodeParser,                   ARRAY_SIZE (CmArchPmcgNodeParser) },
  { "EArchObjGicItsIdentifierArray",        CmArchGicItsIdentifierParser,
    ARRAY_SIZE (CmArchGicItsIdentifierParser) },
  { "EArchObjIdMappingArray",               CmArchIdMappingParser,
    ARRAY_SIZE (CmArchIdMappingParser) },
  { "EArchObjSmmuInterruptArray",           CmArchGenericInterruptParser,
    ARRAY_SIZE (CmArchGenericInterruptParser) },
  { "EArchObjProcHierarchyInfo",            CmArchProcHierarchyInfoParser,
    ARRAY_SIZE (CmArchProcHierarchyInfoParser) },
  { "EArchObjCacheInfo",                    CmArchCacheInfoParser,
    ARRAY_SIZE (CmArchCacheInfoParser) },
  { "EArchObjProcNodeIdInfo",               CmArchProcNodeIdInfoParser,
    ARRAY_SIZE (CmArchProcNodeIdInfoParser) },
  { "EArchObjCmRef",                        CmArchObjRefParser,                     ARRAY_SIZE (CmArchObjRefParser)   },
  { "EArchObjMemoryAffinityInfo",           CmArchMemoryAffinityInfoParser,
    ARRAY_SIZE (CmArchMemoryAffinityInfoParser) },
  { "EArchObjDeviceHandleAcpi",             CmArchDeviceHandleAcpiParser,
    ARRAY_SIZE (CmArchDeviceHandleAcpiParser) },
  { "EArchObjDeviceHandlePci",              CmArchDeviceHandlePciParser,
    ARRAY_SIZE (CmArchDeviceHandlePciParser) },
  { "EArchObjGenericInitiatorAffinityInfo",
    CmArchGenericInitiatorAffinityInfoParser,
    ARRAY_SIZE (CmArchGenericInitiatorAffinityInfoParser) },
  { "EArchObjSerialPortInfo",               CmArchSerialPortInfoParser,
    ARRAY_SIZE (CmArchSerialPortInfoParser) },
  { "EArchObjCmn600Info",                   CmArchCmn600InfoParser,
    ARRAY_SIZE (CmArchCmn600InfoParser) },
  { "EArchObjLpiInfo",                      CmArchLpiInfoParser,
    ARRAY_SIZE (CmArchLpiInfoParser) },
  { "EArchObjPciAddressMapInfo",            CmArchPciAddressMapInfoParser,
    ARRAY_SIZE (CmArchPciAddressMapInfoParser) },
  { "EArchObjPciInterruptMapInfo",          CmPciInterruptMapInfoParser,
    ARRAY_SIZE (CmPciInterruptMapInfoParser) },
  { "EArchObjRmr",                          CmArchRmrInfoParser,
    ARRAY_SIZE (CmArchRmrInfoParser) },
  { "EArchObjMemoryRangeDescriptor",        CmArchMemoryRangeDescriptorInfoParser,
    ARRAY_SIZE (CmArchMemoryRangeDescriptorInfoParser) },
  { "EArchObjCpcInfo",                      CmArchCpcInfoParser,
    ARRAY_SIZE (CmArchCpcInfoParser) },
  { "EArchObjPccSubspaceType0Info",         CmArchPccSubspaceType0InfoParser,
    ARRAY_SIZE (CmArchPccSubspaceType0InfoParser) },
  { "EArchObjPccSubspaceType1Info",         CmArchPccSubspaceType1InfoParser,
    ARRAY_SIZE (CmArchPccSubspaceType1InfoParser) },
  { "EArchObjPccSubspaceType2Info",         CmArchPccSubspaceType2InfoParser,
    ARRAY_SIZE (CmArchPccSubspaceType2InfoParser) },
  { "EArchObjPccSubspaceType3Info",         CmArchPccSubspaceType34InfoParser,
    ARRAY_SIZE (CmArchPccSubspaceType34InfoParser) },
  { "EArchObjPccSubspaceType4Info",         CmArchPccSubspaceType34InfoParser,
    ARRAY_SIZE (CmArchPccSubspaceType34InfoParser) },
  { "EArchObjPccSubspaceType5Info",         CmArchPccSubspaceType5InfoParser,
    ARRAY_SIZE (CmArchPccSubspaceType5InfoParser) },
  { "EArchObjEtInfo",                       CmArchEtInfo,
    ARRAY_SIZE (CmArchEtInfo) },
  { "EArchObjMax",                          NULL,                                   0                                 },
};

/** A parser for EStdObjCfgMgrInfo.
*/
STATIC CONST CM_OBJ_PARSER  StdObjCfgMgrInfoParser[] = {
  { "Revision", 4, "0x%x",         NULL       },
  { "OemId[6]", 6, "%c%c%c%c%c%c", PrintChar6 }
};

/** A parser for EStdObjAcpiTableList.
*/
STATIC CONST CM_OBJ_PARSER  StdObjAcpiTableInfoParser[] = {
  { "AcpiTableSignature", 4,                                      "%c%c%c%c",         PrintChar4 },
  { "AcpiTableRevision",  1,                                      "%d",               NULL       },
  { "TableGeneratorId",   sizeof (ACPI_TABLE_GENERATOR_ID),       "0x%x",             NULL       },
  { "AcpiTableData",      sizeof (EFI_ACPI_DESCRIPTION_HEADER *), "0x%p",             NULL       },
  { "OemTableId",         8,                                      "%c%c%c%c%c%c%c%c", PrintChar8 },
  { "OemRevision",        4,                                      "0x%x",             NULL       },
  { "MinorRevision",      1,                                      "0x%x",             NULL       },
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
  { "EStdObjCfgMgrInfo",      StdObjCfgMgrInfoParser,
    ARRAY_SIZE (StdObjCfgMgrInfoParser) },
  { "EStdObjAcpiTableList",   StdObjAcpiTableInfoParser,
    ARRAY_SIZE (StdObjAcpiTableInfoParser) },
  { "EStdObjSmbiosTableList", StdObjSmbiosTableInfoParser,
    ARRAY_SIZE (StdObjSmbiosTableInfoParser) },
  { "EStdObjMax",             NULL,                       0}
};

/** Print string data.

  The string must be NULL terminated.

  @param [in]  Format  Format to print the Ptr.
  @param [in]  Ptr     Pointer to the string.
**/
STATIC
VOID
EFIAPI
PrintString (
  IN CONST CHAR8  *Format,
  IN UINT8        *Ptr
  )
{
  if (Ptr == NULL) {
    ASSERT (0);
    return;
  }

  DEBUG ((DEBUG_ERROR, "%a", Ptr));
}

/** Print string from pointer.

  The string must be NULL terminated.

  @param [in]  Format      Format to print the string.
  @param [in]  Ptr         Pointer to the string pointer.
**/
STATIC
VOID
EFIAPI
PrintStringPtr (
  IN CONST CHAR8  *Format,
  IN UINT8        *Ptr
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

  PrintString (Format, String);
}

/** Print 4 characters.

  @param [in]  Format  Format to print the Ptr.
  @param [in]  Ptr     Pointer to the characters.
**/
STATIC
VOID
EFIAPI
PrintChar4 (
  IN  CONST CHAR8  *Format,
  IN  UINT8        *Ptr
  )
{
  DEBUG ((
    DEBUG_ERROR,
    (Format != NULL) ? Format : "%c%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3]
    ));
}

/** Print 6 characters.

  @param [in]  Format  Format to print the Ptr.
  @param [in]  Ptr     Pointer to the characters.
**/
STATIC
VOID
EFIAPI
PrintChar6 (
  IN  CONST CHAR8  *Format,
  IN  UINT8        *Ptr
  )
{
  DEBUG ((
    DEBUG_ERROR,
    (Format != NULL) ? Format : "%c%c%c%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3],
    Ptr[4],
    Ptr[5]
    ));
}

/** Print 8 characters.

  @param [in]  Format  Format to print the Ptr.
  @param [in]  Ptr     Pointer to the characters.
**/
STATIC
VOID
EFIAPI
PrintChar8 (
  IN  CONST CHAR8  *Format,
  IN  UINT8        *Ptr
  )
{
  DEBUG ((
    DEBUG_ERROR,
    (Format != NULL) ? Format : "%c%c%c%c%c%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3],
    Ptr[4],
    Ptr[5],
    Ptr[6],
    Ptr[7]
    ));
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
        DEBUG_INFO,
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
      Parser[Index].PrintFormatter (Parser[Index].Format, Data);
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
      if (ObjId >= EArchObjMax) {
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
