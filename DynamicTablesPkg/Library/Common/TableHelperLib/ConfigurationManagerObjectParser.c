/** @file
  Configuration Manager Object parser.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <ConfigurationManagerObject.h>
#include "ConfigurationManagerObjectParser.h"

STATIC
VOID
EFIAPI
PrintOemId (
  CONST CHAR8* Format,
  UINT8* Ptr
  );

/** A parser for EArmObjBootArchInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmBootArchInfoParser[] = {
  {"BootArchFlags", 2, "0x%x", NULL}
};

/** A parser for EArmObjPowerManagementProfileInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmPowerManagementProfileInfoParser[] = {
  {"PowerManagementProfile", 1, "0x%x", NULL}
};

/** A parser for EArmObjGicCInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmGicCInfoParser[] = {
  {"CPUInterfaceNumber", 4, "0x%x", NULL},
  {"AcpiProcessorUid", 4, "0x%x", NULL},
  {"Flags", 4, "0x%x", NULL},
  {"ParkingProtocolVersion", 4, "0x%x", NULL},
  {"PerformanceInterruptGsiv", 4, "0x%x", NULL},
  {"ParkedAddress", 8, "0x%llx", NULL},
  {"PhysicalBaseAddress", 8, "0x%llx", NULL},
  {"GICV", 8, "0x%llx", NULL},
  {"GICH", 8, "0x%llx", NULL},
  {"VGICMaintenanceInterrupt", 4, "0x%x", NULL},
  {"GICRBaseAddress", 8, "0x%llx", NULL},
  {"MPIDR", 8, "0x%llx", NULL},
  {"ProcessorPowerEfficiencyClass", 1, "0x%x", NULL},
  {"SpeOverflowInterrupt", 2, "0x%x", NULL},
  {"ProximityDomain", 4, "0x%x", NULL},
  {"ClockDomain", 4, "0x%x", NULL},
  {"AffinityFlags", 4, "0x%x", NULL}
};

/** A parser for EArmObjGicDInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmGicDInfoParser[] = {
  {"PhysicalBaseAddress", 8, "0x%llx", NULL},
  {"SystemVectorBase", 4, "0x%x", NULL},
  {"GicVersion", 1, "0x%x", NULL},
};

/** A parser for EArmObjGicMsiFrameInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmGicMsiFrameInfoParser[] = {
  {"GicMsiFrameId", 4, "0x%x", NULL},
  {"PhysicalBaseAddress", 8, "0x%llx", NULL},
  {"Flags", 4, "0x%x", NULL},
  {"SPICount", 2, "0x%x", NULL},
  {"SPIBase", 2, "0x%x", NULL}
};

/** A parser for EArmObjGicRedistributorInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmGicRedistInfoParser[] = {
  {"DiscoveryRangeBaseAddress", 8, "0x%llx", NULL},
  {"DiscoveryRangeLength", 4, "0x%x", NULL}
};

/** A parser for EArmObjGicItsInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmGicItsInfoParser[] = {
  {"GicItsId", 4, "0x%x", NULL},
  {"PhysicalBaseAddress", 8, "0x%llx", NULL},
  {"ProximityDomain", 4, "0x%x", NULL}
};

/** A parser for EArmObjSerialConsolePortInfo,
    EArmObjSerialDebugPortInfo and EArmObjSerialPortInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmSerialPortInfoParser[] = {
  {"BaseAddress", 8, "0x%llx", NULL},
  {"Interrupt", 4, "0x%x", NULL},
  {"BaudRate", 8, "0x%llx", NULL},
  {"Clock", 4, "0x%x", NULL},
  {"PortSubtype", 2, "0x%x", NULL},
  {"BaseAddressLength", 8, "0x%llx", NULL},
  {"AccessSize", 1, "0x%d", NULL}
};

/** A parser for EArmObjGenericTimerInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmGenericTimerInfoParser[] = {
  {"CounterControlBaseAddress", 8, "0x%llx", NULL},
  {"CounterReadBaseAddress", 8, "0x%llx", NULL},
  {"SecurePL1TimerGSIV", 4, "0x%x", NULL},
  {"SecurePL1TimerFlags", 4, "0x%x", NULL},
  {"NonSecurePL1TimerGSIV", 4, "0x%x", NULL},
  {"NonSecurePL1TimerFlags", 4, "0x%x", NULL},
  {"VirtualTimerGSIV", 4, "0x%x", NULL},
  {"VirtualTimerFlags", 4, "0x%x", NULL},
  {"NonSecurePL2TimerGSIV", 4, "0x%x", NULL},
  {"NonSecurePL2TimerFlags", 4, "0x%x", NULL},
  {"VirtualPL2TimerGSIV", 4, "0x%x", NULL},
  {"VirtualPL2TimerFlags", 4, "0x%x", NULL}
};

/** A parser for EArmObjPlatformGTBlockInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmGTBlockTimerFrameInfoParser[] = {
  {"FrameNumber", 1, "0x%x", NULL},
  {"PhysicalAddressCntBase", 8, "0x%llx", NULL},
  {"PhysicalAddressCntEL0Base", 8, "0x%llx", NULL},
  {"PhysicalTimerGSIV", 4, "0x%x", NULL},
  {"PhysicalTimerFlags", 4, "0x%x", NULL},
  {"VirtualTimerGSIV", 4, "0x%x", NULL},
  {"VirtualTimerFlags", 4, "0x%x", NULL},
  {"CommonFlags", 4, "0x%x", NULL}
};

/** A parser for EArmObjGTBlockTimerFrameInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmGTBlockInfoParser[] = {
  {"GTBlockPhysicalAddress", 8, "0x%llx", NULL},
  {"GTBlockTimerFrameCount", 4, "0x%x", NULL},
  {"GTBlockTimerFrameToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL}
};

/** A parser for EArmObjPlatformGenericWatchdogInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmGenericWatchdogInfoParser[] = {
  {"ControlFrameAddress", 8, "0x%llx", NULL},
  {"RefreshFrameAddress", 8, "0x%llx", NULL},
  {"TimerGSIV", 4, "0x%x", NULL},
  {"Flags", 4, "0x%x", NULL}
};

/** A parser for EArmObjPciConfigSpaceInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmPciConfigSpaceInfoParser[] = {
  {"BaseAddress", 8, "0x%llx", NULL},
  {"PciSegmentGroupNumber", 2, "0x%x", NULL},
  {"StartBusNumber", 1, "0x%x", NULL},
  {"EndBusNumber", 1, "0x%x", NULL}
};

/** A parser for EArmObjHypervisorVendorIdentity.
*/
STATIC CONST CM_OBJ_PARSER CmArmHypervisorVendorIdParser[] = {
  {"HypervisorVendorId", 8, "0x%llx", NULL}
};

/** A parser for EArmObjFixedFeatureFlags.
*/
STATIC CONST CM_OBJ_PARSER CmArmFixedFeatureFlagsParser[] = {
  {"Flags", 4, "0x%x", NULL}
};

/** A parser for EArmObjItsGroup.
*/
STATIC CONST CM_OBJ_PARSER CmArmItsGroupNodeParser[] = {
  {"Token", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"ItsIdCount", 4, "0x%x", NULL},
  {"ItsIdToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL}
};

/** A parser for EArmObjNamedComponent.
*/
STATIC CONST CM_OBJ_PARSER CmArmNamedComponentNodeParser[] = {
  {"Token", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"IdMappingCount", 4, "0x%x", NULL},
  {"IdMappingToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"Flags", 4, "0x%x", NULL},
  {"CacheCoherent", 4, "0x%x", NULL},
  {"AllocationHints", 1, "0x%x", NULL},
  {"MemoryAccessFlags", 1, "0x%x", NULL},
  {"AddressSizeLimit", 1, "0x%x", NULL},
  {"ObjectName", sizeof (CHAR8*), "%a", NULL}
};

/** A parser for EArmObjRootComplex.
*/
STATIC CONST CM_OBJ_PARSER CmArmRootComplexNodeParser[] = {
  {"Token", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"IdMappingCount", 4, "0x%x", NULL},
  {"IdMappingToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"CacheCoherent", 4, "0x%x", NULL},
  {"AllocationHints", 1, "0x%x", NULL},
  {"MemoryAccessFlags", 1, "0x%x", NULL},
  {"AtsAttribute", 4, "0x%x", NULL},
  {"PciSegmentNumber", 4, "0x%x", NULL},
  {"MemoryAddressSize", 1, "0x%x", NULL}
};

/** A parser for EArmObjSmmuV1SmmuV2.
*/
STATIC CONST CM_OBJ_PARSER CmArmSmmuV1SmmuV2NodeParser[] = {
  {"Token", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"IdMappingCount", 4, "0x%x", NULL},
  {"IdMappingToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"BaseAddress", 8, "0x%llx", NULL},
  {"Span", 8, "0x%llx", NULL},
  {"Model", 4, "0x%x", NULL},
  {"Flags", 4, "0x%x", NULL},
  {"ContextInterruptCount", 4, "0x%x", NULL},
  {"ContextInterruptToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"PmuInterruptCount", 4, "0x%x", NULL},
  {"PmuInterruptToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"SMMU_NSgIrpt", 4, "0x%x", NULL},
  {"SMMU_NSgIrptFlags", 4, "0x%x", NULL},
  {"SMMU_NSgCfgIrpt", 4, "0x%x", NULL},
  {"SMMU_NSgCfgIrptFlags", 4, "0x%x", NULL}
};

/** A parser for EArmObjSmmuV3.
*/
STATIC CONST CM_OBJ_PARSER CmArmSmmuV3NodeParser[] = {
  {"Token", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"IdMappingCount", 4, "0x%x", NULL},
  {"IdMappingToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"BaseAddress", 8, "0x%llx", NULL},
  {"Flags", 4, "0x%x", NULL},
  {"VatosAddress", 8, "0x%llx", NULL},
  {"Model", 4, "0x%x", NULL},
  {"EventInterrupt", 4, "0x%x", NULL},
  {"PriInterrupt", 4, "0x%x", NULL},
  {"GerrInterrupt", 4, "0x%x", NULL},
  {"SyncInterrupt", 4, "0x%x", NULL},
  {"ProximityDomain", 4, "0x%x", NULL},
  {"DeviceIdMappingIndex", 4, "0x%x", NULL}
};

/** A parser for EArmObjPmcg.
*/
STATIC CONST CM_OBJ_PARSER CmArmPmcgNodeParser[] = {
  {"Token", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"IdMappingCount", 4, "0x%x", NULL},
  {"IdMappingToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"BaseAddress", 8, "0x%llx", NULL},
  {"OverflowInterrupt", 4, "0x%x", NULL},
  {"Page1BaseAddress", 8, "0x%llx", NULL},
  {"ReferenceToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL}
};

/** A parser for EArmObjGicItsIdentifierArray.
*/
STATIC CONST CM_OBJ_PARSER CmArmGicItsIdentifierParser[] = {
  {"ItsId", 4, "0x%x", NULL}
};

/** A parser for EArmObjIdMappingArray.
*/
STATIC CONST CM_OBJ_PARSER CmArmIdMappingParser[] = {
  {"InputBase", 4, "0x%x", NULL},
  {"NumIds", 4, "0x%x", NULL},
  {"OutputBase", 4, "0x%x", NULL},
  {"OutputReferenceToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"Flags", 4, "0x%x", NULL}
};

/** A parser for EArmObjSmmuInterruptArray.
*/
STATIC CONST CM_OBJ_PARSER CmArmGenericInterruptParser[] = {
  {"Interrupt", 4, "0x%x", NULL},
  {"Flags", 4, "0x%x", NULL}
};

/** A parser for EArmObjProcHierarchyInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmProcHierarchyInfoParser[] = {
  {"Token", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"Flags", 4, "0x%x", NULL},
  {"ParentToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"GicCToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"NoOfPrivateResources", 4, "0x%x", NULL},
  {"PrivateResourcesArrayToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL}
};

/** A parser for EArmObjCacheInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmCacheInfoParser[] = {
  {"Token", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"NextLevelOfCacheToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"Size", 4, "0x%x", NULL},
  {"NumberOfSets", 4, "0x%x", NULL},
  {"Associativity", 4, "0x%x", NULL},
  {"Attributes", 1, "0x%x", NULL},
  {"LineSize", 2, "0x%x", NULL}
};

/** A parser for EArmObjProcNodeIdInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmProcNodeIdInfoParser[] = {
  {"Token", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL},
  {"VendorId", 4, "0x%p", NULL},
  {"Level1Id", 8, "0x%x", NULL},
  {"Level2Id", 8, "0x%x", NULL},
  {"MajorRev", 2, "0x%x", NULL},
  {"MinorRev", 2, "0x%x", NULL},
  {"SpinRev", 2, "0x%x", NULL}
};

/** A parser for EArmObjCmRef.
*/
STATIC CONST CM_OBJ_PARSER CmArmObjRefParser[] = {
  {"ReferenceToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL}
};

/** A parser for EArmObjMemoryAffinityInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmMemoryAffinityInfoParser[] = {
  {"ProximityDomain", 4, "0x%x", NULL},
  {"BaseAddress", 8, "0x%llx", NULL},
  {"Length", 8, "0x%llx", NULL},
  {"Flags", 4, "0x%x", NULL}
};

/** A parser for EArmObjDeviceHandleAcpi.
*/
STATIC CONST CM_OBJ_PARSER CmArmDeviceHandleAcpiParser[] = {
  {"Hid", 8, "0x%llx", NULL},
  {"Uid", 4, "0x%x", NULL}
};

/** A parser for EArmObjDeviceHandlePci.
*/
STATIC CONST CM_OBJ_PARSER CmArmDeviceHandlePciParser[] = {
  {"SegmentNumber", 2, "0x%x", NULL},
  {"BusNumber", 1, "0x%x", NULL},
  {"DeviceNumber", 1, "0x%x", NULL},
  {"FunctionNumber", 1, "0x%x", NULL}
};

/** A parser for EArmObjGenericInitiatorAffinityInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmGenericInitiatorAffinityInfoParser[] = {
  {"ProximityDomain", 4, "0x%x", NULL},
  {"Flags", 4, "0x%x", NULL},
  {"DeviceHandleType", 1, "0x%x", NULL},
  {"DeviceHandleToken", sizeof (CM_OBJECT_TOKEN), "0x%p", NULL}
};

/** A parser for EArmObjCmn600Info.
*/
STATIC CONST CM_OBJ_PARSER CmArmCmn600InfoParser[] = {
  {"PeriphBaseAddress", 8, "0x%llx", NULL},
  {"PeriphBaseAddressLength", 8, "0x%llx", NULL},
  {"RootNodeBaseAddress", 8, "0x%llx", NULL},
  {"DtcCount", 1, "0x%x", NULL},
  {"DtcInterrupt[0]", 4, "0x%x", NULL},
  {"DtcFlags[0]", 4, "0x%x", NULL},
  {"DtcInterrupt[1]", 4, "0x%x", NULL},
  {"DtcFlags[1]", 4, "0x%x", NULL},
  {"DtcInterrupt[2]", 4, "0x%x", NULL},
  {"DtcFlags[2]", 4, "0x%x", NULL},
  {"DtcInterrupt[3]", 4, "0x%x", NULL},
  {"DtcFlags[3]", 4, "0x%x", NULL}
};

/** A parser for the EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE structure.
*/
STATIC CONST CM_OBJ_PARSER AcpiGenericAddressParser[] = {
  {"AddressSpaceId", 1, "%d", NULL},
  {"RegisterBitWidth", 1, "%d", NULL},
  {"RegisterBitOffset", 1, "%d", NULL},
  {"AccessSize", 1, "%d", NULL},
  {"Address", 8, "0x%llx", NULL},
};

/** A parser for EArmObjLpiInfo.
*/
STATIC CONST CM_OBJ_PARSER CmArmLpiInfoParser[] = {
  {"MinResidency", 4, "0x%llx", NULL},
  {"WorstCaseWakeLatency", 4, "0x%llx", NULL},
  {"Flags", 4, "0x%llx", NULL},
  {"ArchFlags", 4, "0x%llx", NULL},
  {"ResCntFreq", 4, "0x%llx", NULL},
  {"EnableParentState", 4, "0x%llx", NULL},
  {"IsInteger", 1, "%d", NULL},
  {"IntegerEntryMethod", 8, "0x%llx", NULL},
  {"RegisterEntryMethod", sizeof (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser)},
  {"ResidencyCounterRegister", sizeof (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser)},
  {"UsageCounterRegister", sizeof (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE),
    NULL, NULL, AcpiGenericAddressParser,
    ARRAY_SIZE (AcpiGenericAddressParser)},
  {"StateName", 16, "0x%a", NULL},
};

/** A parser for Arm namespace objects.
*/
STATIC CONST CM_OBJ_PARSER_ARRAY ArmNamespaceObjectParser[] = {
  {"EArmObjReserved", NULL, 0},
  {"EArmObjBootArchInfo", CmArmBootArchInfoParser,
    ARRAY_SIZE (CmArmBootArchInfoParser)},
  {"EArmObjCpuInfo", NULL, 0},
  {"EArmObjPowerManagementProfileInfo", CmArmPowerManagementProfileInfoParser,
    ARRAY_SIZE (CmArmPowerManagementProfileInfoParser)},
  {"EArmObjGicCInfo", CmArmGicCInfoParser, ARRAY_SIZE (CmArmGicCInfoParser)},
  {"EArmObjGicDInfo", CmArmGicDInfoParser, ARRAY_SIZE (CmArmGicDInfoParser)},
  {"EArmObjGicMsiFrameInfo", CmArmGicMsiFrameInfoParser,
    ARRAY_SIZE (CmArmGicMsiFrameInfoParser)},
  {"EArmObjGicRedistributorInfo", CmArmGicRedistInfoParser,
    ARRAY_SIZE (CmArmGicRedistInfoParser)},
  {"EArmObjGicItsInfo", CmArmGicItsInfoParser,
    ARRAY_SIZE (CmArmGicItsInfoParser)},
  {"EArmObjSerialConsolePortInfo", CmArmSerialPortInfoParser,
    ARRAY_SIZE (CmArmSerialPortInfoParser)},
  {"EArmObjSerialDebugPortInfo", CmArmSerialPortInfoParser,
    ARRAY_SIZE (CmArmSerialPortInfoParser)},
  {"EArmObjGenericTimerInfo", CmArmGenericTimerInfoParser,
    ARRAY_SIZE (CmArmGenericTimerInfoParser)},
  {"EArmObjPlatformGTBlockInfo", CmArmGTBlockTimerFrameInfoParser,
    ARRAY_SIZE (CmArmGTBlockTimerFrameInfoParser)},
  {"EArmObjGTBlockTimerFrameInfo", CmArmGTBlockInfoParser,
    ARRAY_SIZE (CmArmGTBlockInfoParser)},
  {"EArmObjPlatformGenericWatchdogInfo", CmArmGenericWatchdogInfoParser,
    ARRAY_SIZE (CmArmGenericWatchdogInfoParser)},
  {"EArmObjPciConfigSpaceInfo", CmArmPciConfigSpaceInfoParser,
    ARRAY_SIZE (CmArmPciConfigSpaceInfoParser)},
  {"EArmObjHypervisorVendorIdentity", CmArmHypervisorVendorIdParser,
    ARRAY_SIZE (CmArmHypervisorVendorIdParser)},
  {"EArmObjFixedFeatureFlags", CmArmFixedFeatureFlagsParser,
    ARRAY_SIZE (CmArmFixedFeatureFlagsParser)},
  {"EArmObjItsGroup", CmArmItsGroupNodeParser,
    ARRAY_SIZE (CmArmItsGroupNodeParser)},
  {"EArmObjNamedComponent", CmArmNamedComponentNodeParser,
    ARRAY_SIZE (CmArmNamedComponentNodeParser)},
  {"EArmObjRootComplex", CmArmRootComplexNodeParser,
    ARRAY_SIZE (CmArmRootComplexNodeParser)},
  {"EArmObjSmmuV1SmmuV2", CmArmSmmuV1SmmuV2NodeParser,
    ARRAY_SIZE (CmArmSmmuV1SmmuV2NodeParser)},
  {"EArmObjSmmuV3", CmArmSmmuV3NodeParser,
    ARRAY_SIZE (CmArmSmmuV3NodeParser)},
  {"EArmObjPmcg", CmArmPmcgNodeParser, ARRAY_SIZE (CmArmPmcgNodeParser)},
  {"EArmObjGicItsIdentifierArray", CmArmGicItsIdentifierParser,
    ARRAY_SIZE (CmArmGicItsIdentifierParser)},
  {"EArmObjIdMappingArray", CmArmIdMappingParser,
    ARRAY_SIZE (CmArmIdMappingParser)},
  {"EArmObjSmmuInterruptArray", CmArmGenericInterruptParser,
    ARRAY_SIZE (CmArmGenericInterruptParser)},
  {"EArmObjProcHierarchyInfo", CmArmProcHierarchyInfoParser,
    ARRAY_SIZE (CmArmProcHierarchyInfoParser)},
  {"EArmObjCacheInfo", CmArmCacheInfoParser,
    ARRAY_SIZE (CmArmCacheInfoParser)},
  {"EArmObjProcNodeIdInfo", CmArmProcNodeIdInfoParser,
    ARRAY_SIZE (CmArmProcNodeIdInfoParser)},
  {"EArmObjCmRef", CmArmObjRefParser, ARRAY_SIZE (CmArmObjRefParser)},
  {"EArmObjMemoryAffinityInfo", CmArmMemoryAffinityInfoParser,
    ARRAY_SIZE (CmArmMemoryAffinityInfoParser)},
  {"EArmObjDeviceHandleAcpi", CmArmDeviceHandleAcpiParser,
    ARRAY_SIZE (CmArmDeviceHandleAcpiParser)},
  {"EArmObjDeviceHandlePci", CmArmDeviceHandlePciParser,
    ARRAY_SIZE (CmArmDeviceHandlePciParser)},
  {"EArmObjGenericInitiatorAffinityInfo",
    CmArmGenericInitiatorAffinityInfoParser,
    ARRAY_SIZE (CmArmGenericInitiatorAffinityInfoParser)},
  {"EArmObjSerialPortInfo", CmArmSerialPortInfoParser,
    ARRAY_SIZE (CmArmSerialPortInfoParser)},
  {"EArmObjCmn600Info", CmArmCmn600InfoParser,
    ARRAY_SIZE (CmArmCmn600InfoParser)},
  {"EArmObjLpiInfo", CmArmLpiInfoParser,
      ARRAY_SIZE (CmArmLpiInfoParser)},
  {"EArmObjMax", NULL, 0},
};

/** A parser for EStdObjCfgMgrInfo.
*/
STATIC CONST CM_OBJ_PARSER StdObjCfgMgrInfoParser[] = {
  {"Revision", 4, "0x%x", NULL},
  {"OemId[6]", 6, "%C%C%C%C%C%C", PrintOemId}
};

/** A parser for EStdObjAcpiTableList.
*/
STATIC CONST CM_OBJ_PARSER StdObjAcpiTableInfoParser[] = {
  {"AcpiTableSignature", 4, "0x%x", NULL},
  {"AcpiTableRevision", 1, "%d", NULL},
  {"TableGeneratorId", sizeof (ACPI_TABLE_GENERATOR_ID), "0x%x", NULL},
  {"AcpiTableData", sizeof (EFI_ACPI_DESCRIPTION_HEADER*), "0x%p", NULL},
  {"OemTableId", 8, "0x%LLX", NULL},
  {"OemRevision", 4, "0x%x", NULL}
};

/** A parser for EStdObjSmbiosTableList.
*/
STATIC CONST CM_OBJ_PARSER StdObjSmbiosTableInfoParser[] = {
  {"TableGeneratorId", sizeof (SMBIOS_TABLE_GENERATOR_ID), "0x%x", NULL},
  {"SmbiosTableData", sizeof (SMBIOS_STRUCTURE*), "0x%p", NULL}
};

/** A parser for Standard namespace objects.
*/
STATIC CONST CM_OBJ_PARSER_ARRAY StdNamespaceObjectParser[] = {
  {"EStdObjCfgMgrInfo", StdObjCfgMgrInfoParser,
    ARRAY_SIZE (StdObjCfgMgrInfoParser)},
  {"EStdObjAcpiTableList", StdObjAcpiTableInfoParser,
    ARRAY_SIZE (StdObjAcpiTableInfoParser)},
  {"EStdObjSmbiosTableList", StdObjSmbiosTableInfoParser,
    ARRAY_SIZE (StdObjSmbiosTableInfoParser)},
};

/** Print OEM Id.

  @param [in]  Format  Format to print the Ptr.
  @param [in]  Ptr     Pointer to the OEM Id.
**/
STATIC
VOID
EFIAPI
PrintOemId (
  IN  CONST CHAR8  * Format,
  IN  UINT8        * Ptr
  )
{
  DEBUG ((
    DEBUG_ERROR,
    (Format != NULL) ? Format : "%C%C%C%C%C%C",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3],
    Ptr[4],
    Ptr[5]
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
  IN        VOID            *Data,
  IN  CONST CM_OBJ_PARSER   *Parser,
  IN        UINTN           ItemCount,
  IN        INTN            *RemainingSize,
  IN        UINT32          IndentLevel
  )
{
  UINT32  Index;
  UINT32  IndentIndex;
  INTN    SubStructSize;

  if ((Data == NULL)    ||
      (Parser == NULL)  ||
      (ItemCount == 0)  ||
      (RemainingSize == NULL)) {
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
      DEBUG ((DEBUG_ERROR, "  "));
    }

    DEBUG ((
      DEBUG_ERROR,
      "%-*a :",
      OUTPUT_FIELD_COLUMN_WIDTH - 2 * IndentLevel,
      Parser[Index].NameStr
      ));
    if (Parser[Index].PrintFormatter != NULL) {
      Parser[Index].PrintFormatter (Parser[Index].Format, Data);
    } else if (Parser[Index].Format != NULL) {
      switch (Parser[Index].Length) {
        case 1:
          DEBUG ((DEBUG_ERROR, Parser[Index].Format, *(UINT8*)Data));
          break;
        case 2:
          DEBUG ((DEBUG_ERROR, Parser[Index].Format, *(UINT16*)Data));
          break;
        case 4:
          DEBUG ((DEBUG_ERROR, Parser[Index].Format, *(UINT32*)Data));
          break;
        case 8:
          DEBUG ((DEBUG_ERROR, Parser[Index].Format, ReadUnaligned64(Data)));
          break;
        default:
          DEBUG ((
            DEBUG_ERROR,
            "\nERROR: %a: CANNOT PARSE THIS FIELD, Field Length = %d\n",
            Parser[Index].NameStr,
            Parser[Index].Length
            ));
      } // switch
    } else if (Parser[Index].SubObjParser != NULL) {
      SubStructSize = Parser[Index].Length;

      DEBUG ((DEBUG_ERROR, "\n"));
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
        DEBUG_ERROR,
        "\nERROR: %a: CANNOT PARSE THIS FIELD, Field Length = %d\n",
        Parser[Index].NameStr,
        Parser[Index].Length
        ));
    }
    DEBUG ((DEBUG_ERROR, "\n"));
    Data = (UINT8*)Data + Parser[Index].Length;
  } // for
}

/** Parse and print a CmObjDesc.

  @param [in]  CmObjDesc  The CmObjDesc to parse and print.
**/
VOID
EFIAPI
ParseCmObjDesc (
  IN  CONST CM_OBJ_DESCRIPTOR * CmObjDesc
  )
{
  UINTN   ObjId;
  UINTN   NameSpaceId;
  UINT32  ObjIndex;
  UINT32  ObjectCount;
  INTN    RemainingSize;
  CONST  CM_OBJ_PARSER_ARRAY * ParserArray;

  if ((CmObjDesc == NULL) || (CmObjDesc->Data == NULL)) {
    return;
  }

  NameSpaceId = GET_CM_NAMESPACE_ID (CmObjDesc->ObjectId);
  ObjId = GET_CM_OBJECT_ID (CmObjDesc->ObjectId);

  switch (NameSpaceId) {
    case EObjNameSpaceStandard:
      if (ObjId >= EStdObjMax) {
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
      ParserArray = &ArmNamespaceObjectParser[ObjId];
      break;
    default:
      // Not supported
      ASSERT (0);
      return;
  } // switch

  ObjectCount = CmObjDesc->Count;
  RemainingSize = CmObjDesc->Size;

  for (ObjIndex = 0; ObjIndex < ObjectCount; ObjIndex++) {
    DEBUG ((
      DEBUG_ERROR,
      "\n%-*a [%d/%d]:\n",
      OUTPUT_FIELD_COLUMN_WIDTH,
      ParserArray->ObjectName,
      ObjIndex + 1,
      ObjectCount
      ));
    PrintCmObjDesc (
      CmObjDesc->Data,
      ParserArray->Parser,
      ParserArray->ItemCount,
      &RemainingSize,
      1
      );
  } // for
}
