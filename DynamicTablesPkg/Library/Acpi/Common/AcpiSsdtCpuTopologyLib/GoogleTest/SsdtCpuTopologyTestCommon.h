/** @file
  Common test fixtures and utilities for SSDT CPU Topology GoogleTest.

  Provides shared test infrastructure for both ARM and X64 CPU topology tests.

  Copyright (c) 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef SSDT_CPU_TOPOLOGY_TEST_COMMON_H_
#define SSDT_CPU_TOPOLOGY_TEST_COMMON_H_

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <Library/GoogleTestLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/ConfigurationManagerProtocol.h>
  #include <Library/FunctionMockLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/DebugLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include <Library/PrintLib.h>
  #include <Library/TableHelperLib.h>
  #include <AcpiTableGenerator.h>
  #include <ConfigurationManagerObject.h>
  #include <ConfigurationManagerHelper.h>
  #include <StandardNameSpaceObjects.h>
  #include <ArchCommonNameSpaceObjects.h>
  #include <ArmNameSpaceObjects.h>
  #include <X64NameSpaceObjects.h>
  #include <RiscVNameSpaceObjects.h>
  #include "GoogleTest/Protocol/MockConfigurationManagerProtocol.h"
  #include "../SsdtCpuTopologyGenerator.h"
}

#include "AmlTestHelper.h"

using namespace testing;
using ::testing::_;
using ::testing::Ne;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::AtLeast;

/**
  External declarations for library constructor/destructor.
  These are called to register/deregister the ACPI table generator.
**/
extern "C" {
  EFI_STATUS
  EFIAPI
  AcpiSsdtCpuTopologyLibConstructor (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    );

  EFI_STATUS
  EFIAPI
  AcpiSsdtCpuTopologyLibDestructor (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    );

  // Global generator instance captured during registration
  static ACPI_TABLE_GENERATOR  *gCpuTopologyGenerator = NULL;

  /**
    C++ wrapper functions for generator registration.
    These capture the generator pointer for use in tests.
  **/
  EFI_STATUS
  RegisterAcpiTableGenerator (
    IN  CONST ACPI_TABLE_GENERATOR  *CONST  TableGenerator
    )
  {
    if (TableGenerator == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    gCpuTopologyGenerator = const_cast<ACPI_TABLE_GENERATOR *>(TableGenerator);
    return EFI_SUCCESS;
  }

  EFI_STATUS
  DeregisterAcpiTableGenerator (
    IN  CONST ACPI_TABLE_GENERATOR  *CONST  TableGenerator
    )
  {
    if (TableGenerator == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    gCpuTopologyGenerator = NULL;
    return EFI_SUCCESS;
  }
}

/**
  Base class for SSDT CPU Topology tests.
  Provides common setup/teardown and validation utilities.
**/
class SsdtCpuTopologyTestBase {
protected:

  /**
    Validate the SSDT table header against ACPI 6.6 requirements.

    @param[in] SsdtTable  Pointer to the SSDT table.
  **/
  void
  ValidateSsdtTableHeader (
    IN EFI_ACPI_DESCRIPTION_HEADER  *SsdtTable
    )
  {
    EXPECT_NE (SsdtTable, nullptr);
    EXPECT_EQ (SsdtTable->Signature, (UINT32)EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE);
    EXPECT_EQ (SsdtTable->Revision, EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_REVISION);
  }

  /**
    Validate that a CPU device has correct HID per ACPI 6.6 s8.4.

    @param[in] DeviceNode  The CPU device node.
  **/
  void
  ValidateCpuDeviceHid (
    IN AML_NODE_HANDLE  DeviceNode
    )
  {
    CHAR8       HidBuffer[16];
    EFI_STATUS  Status;

    Status = AmlTestHelper::GetDeviceHid (DeviceNode, HidBuffer, sizeof (HidBuffer));
    EXPECT_EQ (Status, EFI_SUCCESS) << "Failed to get _HID from CPU device";
    EXPECT_STREQ (HidBuffer, ACPI_HID_PROCESSOR_DEVICE_STR)
    << "CPU device _HID must be ACPI0007 per ACPI 6.6 s8.4";
  }

  /**
    Validate that a processor container has correct HID per ACPI 6.6 s8.4.

    @param[in] DeviceNode  The container device node.
  **/
  void
  ValidateContainerDeviceHid (
    IN AML_NODE_HANDLE  DeviceNode
    )
  {
    CHAR8       HidBuffer[16];
    EFI_STATUS  Status;

    Status = AmlTestHelper::GetDeviceHid (DeviceNode, HidBuffer, sizeof (HidBuffer));
    EXPECT_EQ (Status, EFI_SUCCESS) << "Failed to get _HID from container device";
    EXPECT_STREQ (HidBuffer, ACPI_HID_PROCESSOR_CONTAINER_STR)
    << "Processor container _HID must be ACPI0010 per ACPI 6.6 s8.4";
  }

  /**
    Validate CPU device _UID matches expected value.

    @param[in] DeviceNode   The CPU device node.
    @param[in] ExpectedUid  The expected _UID value.
  **/
  void
  ValidateCpuDeviceUid (
    IN AML_NODE_HANDLE  DeviceNode,
    IN UINT64           ExpectedUid
    )
  {
    UINT64      Uid;
    EFI_STATUS  Status;

    Status = AmlTestHelper::GetDeviceUid (DeviceNode, &Uid);
    EXPECT_EQ (Status, EFI_SUCCESS) << "Failed to get _UID from CPU device";
    EXPECT_EQ (Uid, ExpectedUid) << "_UID must match AcpiProcessorUid from Configuration Manager";
  }

  /**
    Validate device name follows Cxxx pattern per library convention.

    @param[in] DeviceNode  The device node.
    @param[in] Index       Expected index (xxx in Cxxx).
  **/
  void
  ValidateDeviceName (
    IN AML_NODE_HANDLE  DeviceNode,
    IN UINT32           Index
    )
  {
    CHAR8       NameBuffer[8];
    CHAR8       ExpectedName[8];
    EFI_STATUS  Status;

    Status = AmlTestHelper::GetDeviceName (DeviceNode, NameBuffer, sizeof (NameBuffer));
    EXPECT_EQ (Status, EFI_SUCCESS) << "Failed to get device name";

    // Device names are in format "Cxxx" where xxx is hex index
    AsciiSPrint (ExpectedName, sizeof (ExpectedName), "C%03X", Index);
    EXPECT_STREQ (NameBuffer, ExpectedName)
    << "Device name must follow Cxxx pattern";
  }
};

/**
  Test fixture for SSDT CPU Topology tests.
  Provides mock Configuration Manager Protocol and test data management.
**/
class SsdtCpuTopologyTest : public ::testing::Test, public SsdtCpuTopologyTestBase {
protected:
  MockConfigurationManagerProtocol MockConfigMgrProtocol;
  CM_STD_OBJ_CONFIGURATION_MANAGER_INFO CfgMgrInfo = { 0 };
  CM_STD_OBJ_ACPI_TABLE_INFO mAcpiTableInfo;

  void
  SetUp (
    ) override
  {
    // Set up default behavior for GetObject
    ON_CALL (MockConfigMgrProtocol, GetObject (_, _, _, _))
      .WillByDefault (Return (EFI_NOT_FOUND));

    // Set up configuration manager info
    CfgMgrInfo.Revision = CREATE_REVISION (1, 0);
    CfgMgrInfo.OemId[0] = 'T';
    CfgMgrInfo.OemId[1] = 'E';
    CfgMgrInfo.OemId[2] = 'S';
    CfgMgrInfo.OemId[3] = 'T';
    CfgMgrInfo.OemId[4] = 'I';
    CfgMgrInfo.OemId[5] = 'D';

    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_STD_OBJECT_ID (EStdObjCfgMgrInfo), CM_NULL_TOKEN, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_STD_OBJECT_ID (EStdObjCfgMgrInfo),
      sizeof (CM_STD_OBJ_CONFIGURATION_MANAGER_INFO),
      &CfgMgrInfo,
      1
    }
             ),
           Return (EFI_SUCCESS)
           )
         );

    // Set up expectation for optional ProcHierarchyInfo - return NOT_FOUND by default
    // Tests that need proc hierarchy should call SetupProcHierarchyInfo() to override this
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjProcHierarchyInfo), CM_NULL_TOKEN, _))
      .WillRepeatedly (Return (EFI_NOT_FOUND));

    // Initialize the CPU Topology library with our mock protocol
    EXPECT_EQ (AcpiSsdtCpuTopologyLibConstructor (NULL, NULL), EFI_SUCCESS);
    EXPECT_NE (gCpuTopologyGenerator, nullptr) << "Generator should be registered";

    // Setup common test data
    mAcpiTableInfo.TableGeneratorId   = CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtCpuTopology);
    mAcpiTableInfo.AcpiTableSignature = EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE;
    mAcpiTableInfo.AcpiTableRevision  = EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_REVISION;
  }

  void
  TearDown (
    ) override
  {
    // Clean up the CPU Topology library
    AcpiSsdtCpuTopologyLibDestructor (NULL, NULL);
  }

  /**
    Setup mock expectation for processor hierarchy info.
    Follows Dbg2 pattern with individual EXPECT_CALLs per token.

    @param[in] ProcHierarchyInfo  Array of processor hierarchy info.
    @param[in] Count              Number of entries.
  **/
  void
  SetupProcHierarchyInfo (
    CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *ProcHierarchyInfo,
    UINT32                              Count
    )
  {
    // Set up expectation for CM_NULL_TOKEN - return entire array
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjProcHierarchyInfo), CM_NULL_TOKEN, _))
      .WillOnce (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjProcHierarchyInfo),
      (UINT32)(sizeof (CM_ARCH_COMMON_PROC_HIERARCHY_INFO) * Count),
      ProcHierarchyInfo,
      Count
    }
             ),
           Return (EFI_SUCCESS)
           )
         );

    // Set up individual expectations for each token - return single entry
    for (UINT32 i = 0; i < Count; i++) {
      EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjProcHierarchyInfo), ProcHierarchyInfo[i].Token, _))
        .WillRepeatedly (
           DoAll (
             SetArgPointee<3>(
               CM_OBJ_DESCRIPTOR {
        CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjProcHierarchyInfo),
        sizeof (CM_ARCH_COMMON_PROC_HIERARCHY_INFO),
        &ProcHierarchyInfo[i],
        1
      }
               ),
             Return (EFI_SUCCESS)
             )
           );
    }
  }

  /**
    Setup mock expectation for CPC info.

    @param[in] CpcInfo  The CPC info structure.
    @param[in] Token    Token to associate with this CPC info.
  **/
  void
  SetupCpcInfo (
    CM_ARCH_COMMON_CPC_INFO  *CpcInfo,
    CM_OBJECT_TOKEN          Token
    )
  {
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjCpcInfo), Token, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjCpcInfo),
      sizeof (CM_ARCH_COMMON_CPC_INFO),
      CpcInfo,
      1
    }
             ),
           Return (EFI_SUCCESS)
           )
         );
  }

  /**
    Setup mock expectation for PSD info.

    @param[in] PsdInfo  The PSD info structure.
    @param[in] Token    Token to associate with this PSD info.
  **/
  void
  SetupPsdInfo (
    CM_ARCH_COMMON_PSD_INFO  *PsdInfo,
    CM_OBJECT_TOKEN          Token
    )
  {
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjPsdInfo), Token, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjPsdInfo),
      sizeof (CM_ARCH_COMMON_PSD_INFO),
      PsdInfo,
      1
    }
             ),
           Return (EFI_SUCCESS)
           )
         );
  }

  /**
    Build the SSDT CPU Topology table using the generator.

    @param[out] Table       On success, the generated table.
    @param[out] TableCount  Number of tables generated (should be 1).

    @retval EFI_SUCCESS  Table was generated successfully.
    @retval Other        An error occurred.
  **/
  EFI_STATUS
  BuildSsdtCpuTopologyTable (
    OUT EFI_ACPI_DESCRIPTION_HEADER  ***Table,
    OUT UINTN                        *TableCount
    )
  {
    EFI_STATUS  Status;

    if (gCpuTopologyGenerator == NULL) {
      return EFI_NOT_READY;
    }

    // Generator uses old-style BuildAcpiTable (single table), not BuildAcpiTableEx
    // Allocate array to hold single table
    EFI_ACPI_DESCRIPTION_HEADER  **TableArray =
      (EFI_ACPI_DESCRIPTION_HEADER **)AllocateZeroPool (sizeof (EFI_ACPI_DESCRIPTION_HEADER *));

    if (TableArray == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = gCpuTopologyGenerator->BuildAcpiTable (
                                      gCpuTopologyGenerator,
                                      &mAcpiTableInfo,
                                      gConfigurationManagerProtocol,
                                      &TableArray[0]
                                      );

    DEBUG ((DEBUG_ERROR, "%a: BuildAcpiTable returned Status=%r, TableArray[0]=%p\n", __func__, Status, TableArray[0]));

    if (EFI_ERROR (Status)) {
      FreePool (TableArray);
      return Status;
    }

      *Table      = TableArray;
      *TableCount = 1;
    return EFI_SUCCESS;
  }

  /**
    Free table resources after test.

    @param[in] Table       The table array to free.
    @param[in] TableCount  Number of tables.
  **/
  void
  FreeSsdtCpuTopologyTable (
    IN EFI_ACPI_DESCRIPTION_HEADER  **Table,
    IN UINTN                        TableCount
    )
  {
    if (Table == NULL) {
      return;
    }

    // Free each table using the generator's FreeTableResources function
    if ((gCpuTopologyGenerator != NULL) &&
        (gCpuTopologyGenerator->FreeTableResources != NULL))
    {
      for (UINTN Index = 0; Index < TableCount; Index++) {
        if (Table[Index] != NULL) {
          gCpuTopologyGenerator->FreeTableResources (
                                   gCpuTopologyGenerator,
                                   &mAcpiTableInfo,
                                   gConfigurationManagerProtocol,
                                   &Table[Index]
                                   );
        }
      }
    }

    // Free the table array itself
    FreePool (Table);
  }
};

/**
  Helper to create a simple CPC info structure for testing.
  Uses integer values (not buffer registers) for simplicity.
**/
inline void
CreateSimpleCpcInfo (
  OUT CM_ARCH_COMMON_CPC_INFO  *CpcInfo
  )
{
  ZeroMem (CpcInfo, sizeof (*CpcInfo));
  CpcInfo->Revision                          = ACPI_CPC_REVISION_3;
  CpcInfo->HighestPerformanceInteger         = 1000;
  CpcInfo->NominalPerformanceInteger         = 800;
  CpcInfo->LowestNonlinearPerformanceInteger = 400;
  CpcInfo->LowestPerformanceInteger          = 200;

  // Required register fields (must be non-null per ACPI spec)
  // Using FFH (Functional Fixed Hardware) address space type
  CpcInfo->PerformanceLimitedRegister.AddressSpaceId    = EFI_ACPI_6_4_FUNCTIONAL_FIXED_HARDWARE;
  CpcInfo->PerformanceLimitedRegister.RegisterBitWidth  = 8;
  CpcInfo->PerformanceLimitedRegister.RegisterBitOffset = 0;
  CpcInfo->PerformanceLimitedRegister.AccessSize        = EFI_ACPI_6_4_BYTE;
  CpcInfo->PerformanceLimitedRegister.Address           = 1;

  CpcInfo->ReferencePerformanceCounterRegister.AddressSpaceId    = EFI_ACPI_6_4_FUNCTIONAL_FIXED_HARDWARE;
  CpcInfo->ReferencePerformanceCounterRegister.RegisterBitWidth  = 64;
  CpcInfo->ReferencePerformanceCounterRegister.RegisterBitOffset = 0;
  CpcInfo->ReferencePerformanceCounterRegister.AccessSize        = EFI_ACPI_6_4_QWORD;
  CpcInfo->ReferencePerformanceCounterRegister.Address           = 2;

  CpcInfo->DeliveredPerformanceCounterRegister.AddressSpaceId    = EFI_ACPI_6_4_FUNCTIONAL_FIXED_HARDWARE;
  CpcInfo->DeliveredPerformanceCounterRegister.RegisterBitWidth  = 64;
  CpcInfo->DeliveredPerformanceCounterRegister.RegisterBitOffset = 0;
  CpcInfo->DeliveredPerformanceCounterRegister.AccessSize        = EFI_ACPI_6_4_QWORD;
  CpcInfo->DeliveredPerformanceCounterRegister.Address           = 3;

  // DesiredPerformanceRegister is also required
  CpcInfo->DesiredPerformanceRegister.AddressSpaceId    = EFI_ACPI_6_4_FUNCTIONAL_FIXED_HARDWARE;
  CpcInfo->DesiredPerformanceRegister.RegisterBitWidth  = 32;
  CpcInfo->DesiredPerformanceRegister.RegisterBitOffset = 0;
  CpcInfo->DesiredPerformanceRegister.AccessSize        = EFI_ACPI_6_4_DWORD;
  CpcInfo->DesiredPerformanceRegister.Address           = 4;
}

/**
  Helper to create a simple PSD info structure for testing.
**/
inline void
CreateSimplePsdInfo (
  OUT CM_ARCH_COMMON_PSD_INFO  *PsdInfo,
  IN  UINT32                   Domain,
  IN  UINT32                   NumProc
  )
{
  ZeroMem (PsdInfo, sizeof (*PsdInfo));
  PsdInfo->Revision  = 0;  // Revision 0 per ACPI spec
  PsdInfo->Domain    = Domain;
  PsdInfo->CoordType = 0xFD;  // SW_ANY coordination
  PsdInfo->NumProc   = NumProc;
}

#endif // SSDT_CPU_TOPOLOGY_TEST_COMMON_H_
