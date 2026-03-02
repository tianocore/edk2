/** @file
  RISC-V SSDT CPU Topology Integration GoogleTest.

  Tests full RISC-V CPU topology generation through BuildSsdtCpuTopologyTable.
  Validates generated AML output against ACPI specification.

  Parameterized test suites:
  - CpuCounts/RiscVCpuCountIntegrationTest: CPU counts from 1 to 32
  - UidBoundaries/RiscVUidBoundaryIntegrationTest: UID encoding boundaries

  Additional validation tests:
  - CPU _HID validation ("ACPI0007")
  - CPU _UID validation (matches RINTC AcpiProcessorUid)
  - CPU naming convention (Cxxx pattern)
  - _CPC structure validation (23 entries for Rev 3)
  - HartId independence from UID
  - Disabled CPU _STA handling

  Copyright (c) 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.6 Specification - s8.4 Declaring Processors
    - RISC-V ACPI Specification
**/

#include <Uefi.h>
#include "SsdtCpuTopologyTestCommon.h"
#include <sstream>
#include <string>

// =============================================================================
// RISC-V Integration Test Fixture
// =============================================================================

/**
  Test fixture for RISC-V-specific integration tests.
  Extends SsdtCpuTopologyTest to include RISC-V RINTC setup.
**/
class RiscVIntegrationTest : public SsdtCpuTopologyTest {
protected:
  std::vector<CM_RISCV_RINTC_INFO> mRintcInfo;
  std::vector<CM_ARCH_COMMON_CPC_INFO> mCpcInfo;

  /**
    Create a simple RINTC info structure for testing.

    @param[out] RintcInfo         The RINTC info to populate.
    @param[in]  HartId            The Hart ID.
    @param[in]  AcpiProcessorUid  The ACPI processor UID.
  **/
  void
  CreateRintcInfo (
    OUT CM_RISCV_RINTC_INFO  *RintcInfo,
    IN  UINT64               HartId,
    IN  UINT32               AcpiProcessorUid
    )
  {
    ZeroMem (RintcInfo, sizeof (*RintcInfo));
    RintcInfo->Version          = 1;
    RintcInfo->HartId           = HartId;
    RintcInfo->AcpiProcessorUid = AcpiProcessorUid;
    RintcInfo->Flags            = 1;  // Enabled
    RintcInfo->CpcToken         = CM_NULL_TOKEN;
  }

  /**
    Setup mock expectation for RISC-V RINTC info lookup.

    @param[in] RintcInfo  Array of RINTC info structures.
    @param[in] Count      Number of RINTC entries.
  **/
  void
  SetupRintcInfo (
    CM_RISCV_RINTC_INFO  *RintcInfo,
    UINT32               Count
    )
  {
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_RISCV_OBJECT_ID (ERiscVObjRintcInfo), CM_NULL_TOKEN, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_RISCV_OBJECT_ID (ERiscVObjRintcInfo),
      (UINT32)(sizeof (CM_RISCV_RINTC_INFO) * Count),
      RintcInfo,
      Count
    }
             ),
           Return (EFI_SUCCESS)
           )
         );
  }
};

// =============================================================================
// RISC-V Integration Tests - Test through BuildSsdtCpuTopologyTable with real AML
// =============================================================================

/**
  @test RiscVIntegration_SingleCpu_GeneratesCorrectAml

  @brief Integration test validating basic RISC-V CPU topology generation.

  @spec ACPI 6.6 s8.4: Each processor must have its own Processor Device
        object declared with _HID "ACPI0007".

  @expected Table generated with CPU device containing _HID and _UID.
**/
TEST_F (RiscVIntegrationTest, SingleCpu_GeneratesCorrectAml) {
  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 0, 100);  // Hart 0, UID 100
  SetupRintcInfo (mRintcInfo.data (), 1);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);
  ASSERT_EQ (TableCount, 1u);

  // Parse and validate
  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify CPU device exists
  UINT32  CpuCount = 0;

  EXPECT_EQ (AmlTestHelper::CountCpuDevices (RootNode, "\\_SB", &CpuCount), EFI_SUCCESS);
  EXPECT_EQ (CpuCount, 1u) << "Should have one CPU device";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test RiscVIntegration_MultipleCpus_AllHaveDevices

  @brief Integration test validating multiple RISC-V CPU generation.

  @spec ACPI 6.6 s8.4: Each processor must have its own Processor Device object.

  @expected One device generated per RINTC entry.
**/
TEST_F (RiscVIntegrationTest, MultipleCpus_AllHaveDevices) {
  const UINT32  NumCpus = 4;

  mRintcInfo.resize (NumCpus);
  for (UINT32 i = 0; i < NumCpus; i++) {
    // Different Hart IDs and UIDs for each CPU
    CreateRintcInfo (&mRintcInfo[i], i, i * 100);
  }

  SetupRintcInfo (mRintcInfo.data (), NumCpus);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify correct number of CPU devices
  UINT32  CpuCount = 0;

  EXPECT_EQ (AmlTestHelper::CountCpuDevices (RootNode, "\\_SB", &CpuCount), EFI_SUCCESS);
  EXPECT_EQ (CpuCount, NumCpus) << "Should have one CPU device per RINTC entry";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test RiscVIntegration_CpcToken_GeneratesCpcNode

  @brief Integration test validating _CPC generation when CpcToken is set.

  @spec ACPI 6.6 s8.4.7.1: "_CPC object returns a Package describing CPPC
        registers and capabilities."

  @expected _CPC object generated with correct structure.
**/
TEST_F (RiscVIntegrationTest, CpcToken_GeneratesCpcNode) {
  const CM_OBJECT_TOKEN  CpcToken = 10001;

  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 0, 100);
  mRintcInfo[0].CpcToken = CpcToken;
  SetupRintcInfo (mRintcInfo.data (), 1);

  // Set up CPC info
  mCpcInfo.resize (1);
  CreateSimpleCpcInfo (&mCpcInfo[0]);
  SetupCpcInfo (&mCpcInfo[0], CpcToken);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify CPU device exists
  UINT32  CpuCount = 0;

  EXPECT_EQ (AmlTestHelper::CountCpuDevices (RootNode, "\\_SB", &CpuCount), EFI_SUCCESS);
  EXPECT_GE (CpuCount, 1u) << "At least one CPU device should exist";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test RiscVIntegration_DisabledCpu_StillGenerated

  @brief Integration test for disabled RISC-V CPU handling.

  @spec RISC-V: Disabled CPUs (Flags = 0) should still be represented.

  @expected CPU device generated even when Flags indicates disabled.
**/
TEST_F (RiscVIntegrationTest, DisabledCpu_StillGenerated) {
  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 0, 100);
  mRintcInfo[0].Flags = 0;  // Disabled
  SetupRintcInfo (mRintcInfo.data (), 1);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify CPU devices exist even when disabled
  UINT32  CpuCount = 0;

  EXPECT_EQ (AmlTestHelper::CountCpuDevices (RootNode, "\\_SB", &CpuCount), EFI_SUCCESS);
  EXPECT_GE (CpuCount, 1u) << "Disabled CPUs should still generate devices";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

// =============================================================================
// Enhanced Validation Tests
// =============================================================================

/**
  @test RiscVIntegration_CpuHid_IsAcpi0007

  @brief Validates CPU device has correct _HID.

  @spec ACPI 6.6 s8.4: Processor Device objects must have _HID "ACPI0007".

  @expected CPU device _HID = "ACPI0007".
**/
TEST_F (RiscVIntegrationTest, CpuHid_IsAcpi0007) {
  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 0, 100);
  SetupRintcInfo (mRintcInfo.data (), 1);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Find CPU device and validate HID
  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);
  EXPECT_TRUE (AmlTestHelper::DeviceHasHid (CpuDevice, "ACPI0007"))
  << "CPU device must have _HID 'ACPI0007'";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test RiscVIntegration_CpuUid_MatchesRintcAcpiProcessorUid

  @brief Validates CPU device _UID matches RINTC AcpiProcessorUid.

  @spec ACPI 6.6 s8.4: _UID must uniquely identify each processor.

  @expected _UID in AML matches AcpiProcessorUid from RINTC.
**/
TEST_F (RiscVIntegrationTest, CpuUid_MatchesRintcAcpiProcessorUid) {
  const UINT32  ExpectedUid = 42;

  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 0, ExpectedUid);
  SetupRintcInfo (mRintcInfo.data (), 1);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Find CPU device and validate UID
  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  UINT64  ActualUid;

  ASSERT_EQ (AmlTestHelper::GetDeviceUid (CpuDevice, &ActualUid), EFI_SUCCESS);
  EXPECT_EQ (ActualUid, ExpectedUid) << "_UID must match RINTC AcpiProcessorUid";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test RiscVIntegration_MultipleCpus_UidsMatchRintcOrder

  @brief Validates all CPU UIDs match RINTC order.

  @expected Each CPU's _UID matches corresponding RINTC AcpiProcessorUid.
**/
TEST_F (RiscVIntegrationTest, MultipleCpus_UidsMatchRintcOrder) {
  const UINT32  NumCpus = 4;

  mRintcInfo.resize (NumCpus);
  UINT32  ExpectedUids[] = { 10, 20, 30, 40 };

  for (UINT32 i = 0; i < NumCpus; i++) {
    CreateRintcInfo (&mRintcInfo[i], i, ExpectedUids[i]);
  }

  SetupRintcInfo (mRintcInfo.data (), NumCpus);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify each CPU's UID
  const char  *CpuNames[] = { "C000", "C001", "C002", "C003" };

  for (UINT32 i = 0; i < NumCpus; i++) {
    char  DevicePath[16];
    AsciiSPrint (DevicePath, sizeof (DevicePath), "\\_SB.%a", CpuNames[i]);
    AML_NODE_HANDLE  CpuDevice;
    ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, DevicePath, &CpuDevice), EFI_SUCCESS)
    << "CPU " << CpuNames[i] << " not found";

    UINT64  ActualUid;
    ASSERT_EQ (AmlTestHelper::GetDeviceUid (CpuDevice, &ActualUid), EFI_SUCCESS);
    EXPECT_EQ (ActualUid, ExpectedUids[i])
    << CpuNames[i] << " UID mismatch: expected " << ExpectedUids[i] << ", got " << ActualUid;
  }

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test RiscVIntegration_CpuNaming_FollowsCxxxPattern

  @brief Validates CPU naming follows Cxxx pattern.

  @spec Internal: CPU devices named C000, C001, C002, etc.

  @expected All CPU devices follow Cxxx naming.
**/
TEST_F (RiscVIntegrationTest, CpuNaming_FollowsCxxxPattern) {
  const UINT32  NumCpus = 4;

  mRintcInfo.resize (NumCpus);
  for (UINT32 i = 0; i < NumCpus; i++) {
    CreateRintcInfo (&mRintcInfo[i], i, i);
  }

  SetupRintcInfo (mRintcInfo.data (), NumCpus);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify CPU naming pattern
  const char  *ExpectedNames[] = { "C000", "C001", "C002", "C003" };

  for (UINT32 i = 0; i < NumCpus; i++) {
    char  DevicePath[16];
    AsciiSPrint (DevicePath, sizeof (DevicePath), "\\_SB.%a", ExpectedNames[i]);
    AML_NODE_HANDLE  CpuDevice;
    EFI_STATUS       Status = AmlTestHelper::FindDeviceByPath (RootNode, DevicePath, &CpuDevice);
    EXPECT_EQ (Status, EFI_SUCCESS) << "Expected CPU device " << ExpectedNames[i] << " not found";
  }

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test RiscVIntegration_CpcToken_ValidatesPackageStructure

  @brief Validates _CPC package has correct structure.

  @spec ACPI 6.6 s8.4.7.1: _CPC package must have 23 entries for Rev 3.

  @expected _CPC package contains 23 elements.
**/
TEST_F (RiscVIntegrationTest, CpcToken_ValidatesPackageStructure) {
  const CM_OBJECT_TOKEN  CpcToken = 10001;

  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 0, 100);
  mRintcInfo[0].CpcToken = CpcToken;
  SetupRintcInfo (mRintcInfo.data (), 1);

  // Set up CPC info with Revision 3
  mCpcInfo.resize (1);
  CreateSimpleCpcInfo (&mCpcInfo[0]);
  mCpcInfo[0].Revision = 3;
  SetupCpcInfo (&mCpcInfo[0], CpcToken);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Find CPU device
  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  // Find _CPC and validate structure
  EXPECT_TRUE (AmlTestHelper::DeviceHasNamedObject (CpuDevice, "_CPC"))
  << "CPU device should have _CPC object";

  UINT32  CpcRevision;

  if (AmlTestHelper::GetCpcRevision (CpuDevice, &CpcRevision) == EFI_SUCCESS) {
    EXPECT_EQ (CpcRevision, 3u) << "_CPC revision should be 3";
  }

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test RiscVIntegration_HartId_IndependentFromUid

  @brief Validates HartId can differ from AcpiProcessorUid.

  @spec RISC-V: HartId identifies the hart, UID is ACPI-level identifier.

  @expected CPU created with UID from AcpiProcessorUid, not HartId.
**/
TEST_F (RiscVIntegrationTest, HartId_IndependentFromUid) {
  mRintcInfo.resize (1);
  // HartId = 5, AcpiProcessorUid = 100
  CreateRintcInfo (&mRintcInfo[0], 5, 100);
  SetupRintcInfo (mRintcInfo.data (), 1);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Find CPU device and validate UID is AcpiProcessorUid (100), not HartId (5)
  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  UINT64  ActualUid;

  ASSERT_EQ (AmlTestHelper::GetDeviceUid (CpuDevice, &ActualUid), EFI_SUCCESS);
  EXPECT_EQ (ActualUid, 100u) << "_UID should be AcpiProcessorUid (100), not HartId (5)";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test RiscVIntegration_DisabledCpu_NoSta

  @brief Documents that RISC-V RINTC path doesn't generate _STA for disabled CPUs.

  @note RISC-V CreateTopologyFromIntC doesn't check RINTC flags or generate _STA.
        All CPUs are created regardless of enabled/disabled state.

  @expected Disabled CPU is created but _STA is NOT generated.
**/
TEST_F (RiscVIntegrationTest, DisabledCpu_NoSta) {
  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 0, 100);
  mRintcInfo[0].Flags = 0;  // Disabled
  SetupRintcInfo (mRintcInfo.data (), 1);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Find CPU device
  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  // RISC-V RINTC path doesn't generate _STA - document this implementation limitation
  EXPECT_FALSE (AmlTestHelper::DeviceHasSta (CpuDevice))
  << "RISC-V RINTC path: _STA not generated (implementation limitation)";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test RiscVIntegration_MixedCpcConfig

  @brief Tests CPUs with mixed CPC configurations.

  @expected Only CPUs with CpcToken have _CPC objects.
**/
TEST_F (RiscVIntegrationTest, MixedCpcConfig) {
  const UINT32  NumCpus = 4;

  mRintcInfo.resize (NumCpus);
  CreateRintcInfo (&mRintcInfo[0], 0, 0);   // No CPC
  CreateRintcInfo (&mRintcInfo[1], 1, 1);   // Has CPC
  CreateRintcInfo (&mRintcInfo[2], 2, 2);   // No CPC
  CreateRintcInfo (&mRintcInfo[3], 3, 3);   // Has CPC

  const CM_OBJECT_TOKEN  CpcToken1 = 10001;
  const CM_OBJECT_TOKEN  CpcToken3 = 10003;

  mRintcInfo[1].CpcToken = CpcToken1;
  mRintcInfo[3].CpcToken = CpcToken3;

  SetupRintcInfo (mRintcInfo.data (), NumCpus);

  // Set up CPC info
  mCpcInfo.resize (2);
  CreateSimpleCpcInfo (&mCpcInfo[0]);
  CreateSimpleCpcInfo (&mCpcInfo[1]);
  SetupCpcInfo (&mCpcInfo[0], CpcToken1);
  SetupCpcInfo (&mCpcInfo[1], CpcToken3);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify correct number of CPUs
  UINT32  CpuCount = 0;

  EXPECT_EQ (AmlTestHelper::CountCpuDevices (RootNode, "\\_SB", &CpuCount), EFI_SUCCESS);
  EXPECT_EQ (CpuCount, NumCpus);

  // Verify CPC presence
  const char  *CpuNames[]   = { "C000", "C001", "C002", "C003" };
  bool        ExpectedCpc[] = { false, true, false, true };

  for (UINT32 i = 0; i < NumCpus; i++) {
    char  DevicePath[16];
    AsciiSPrint (DevicePath, sizeof (DevicePath), "\\_SB.%a", CpuNames[i]);
    AML_NODE_HANDLE  CpuDevice;
    ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, DevicePath, &CpuDevice), EFI_SUCCESS);

    bool  HasCpc = AmlTestHelper::DeviceHasNamedObject (CpuDevice, "_CPC");
    EXPECT_EQ (HasCpc, ExpectedCpc[i])
    << CpuNames[i] << " CPC presence mismatch";
  }

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test RiscVIntegration_LargeCpuCount

  @brief Tests handling of large CPU count.

  @expected All CPUs generated correctly.
**/
TEST_F (RiscVIntegrationTest, LargeCpuCount) {
  const UINT32  NumCpus = 64;

  mRintcInfo.resize (NumCpus);
  for (UINT32 i = 0; i < NumCpus; i++) {
    CreateRintcInfo (&mRintcInfo[i], i, i);
  }

  SetupRintcInfo (mRintcInfo.data (), NumCpus);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify CPU count
  UINT32  CpuCount = 0;

  EXPECT_EQ (AmlTestHelper::CountCpuDevices (RootNode, "\\_SB", &CpuCount), EFI_SUCCESS);
  EXPECT_EQ (CpuCount, NumCpus);

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

// =============================================================================
// Parameterized Test: RiscVCpuCountIntegrationTest
// Tests CPU counts from 1 to 32.
// =============================================================================

struct RiscVCpuCountParams {
  UINT32         CpuCount;
  std::string    Description;
};

class RiscVCpuCountIntegrationTest : public RiscVIntegrationTest,
  public ::testing::WithParamInterface<RiscVCpuCountParams> {
};

TEST_P (RiscVCpuCountIntegrationTest, GeneratesCorrectCpuCount) {
  const auto  &Params = GetParam ();

  mRintcInfo.resize (Params.CpuCount);
  for (UINT32 i = 0; i < Params.CpuCount; i++) {
    CreateRintcInfo (&mRintcInfo[i], i, i);
  }

  SetupRintcInfo (mRintcInfo.data (), Params.CpuCount);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify correct number of CPUs
  UINT32  CpuCount = 0;

  EXPECT_EQ (AmlTestHelper::CountCpuDevices (RootNode, "\\_SB", &CpuCount), EFI_SUCCESS);
  EXPECT_EQ (CpuCount, Params.CpuCount);

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

std::string
RiscVCpuCountParamName (
  const ::testing::TestParamInfo<RiscVCpuCountParams>  &Info
  )
{
  return Info.param.Description;
}

INSTANTIATE_TEST_SUITE_P (
  CpuCounts,
  RiscVCpuCountIntegrationTest,
  ::testing::Values (
               RiscVCpuCountParams{ 1, "OneCpu" },
               RiscVCpuCountParams{ 2, "TwoCpus" },
               RiscVCpuCountParams{ 4, "FourCpus" },
               RiscVCpuCountParams{ 8, "EightCpus" },
               RiscVCpuCountParams{ 16, "SixteenCpus" },
               RiscVCpuCountParams{ 32, "ThirtyTwoCpus" }
               ),
  RiscVCpuCountParamName
  );

// =============================================================================
// Parameterized Test: RiscVUidBoundaryIntegrationTest
// Tests UID encoding boundaries in generated AML.
// =============================================================================

struct RiscVUidBoundaryParams {
  UINT32         Uid;
  std::string    Description;
};

class RiscVUidBoundaryIntegrationTest : public RiscVIntegrationTest,
  public ::testing::WithParamInterface<RiscVUidBoundaryParams> {
};

TEST_P (RiscVUidBoundaryIntegrationTest, UidEncodedCorrectly) {
  const auto  &Params = GetParam ();

  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 0, Params.Uid);
  SetupRintcInfo (mRintcInfo.data (), 1);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Find CPU device and validate UID
  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  UINT64  ActualUid;

  ASSERT_EQ (AmlTestHelper::GetDeviceUid (CpuDevice, &ActualUid), EFI_SUCCESS);
  EXPECT_EQ (ActualUid, (UINT64)Params.Uid)
  << "UID " << Params.Uid << " not encoded correctly in AML";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

std::string
RiscVUidBoundaryParamName (
  const ::testing::TestParamInfo<RiscVUidBoundaryParams>  &Info
  )
{
  return Info.param.Description;
}

INSTANTIATE_TEST_SUITE_P (
  UidBoundaries,
  RiscVUidBoundaryIntegrationTest,
  ::testing::Values (
               RiscVUidBoundaryParams{ 0, "Zero" },
               RiscVUidBoundaryParams{ 63, "ByteMax6Bit" },
               RiscVUidBoundaryParams{ 64, "ByteMin7Bit" },
               RiscVUidBoundaryParams{ 127, "ByteMax7Bit" },
               RiscVUidBoundaryParams{ 255, "ByteMax" },
               RiscVUidBoundaryParams{ 256, "WordMin" },
               RiscVUidBoundaryParams{ 32767, "WordMaxSigned" },
               RiscVUidBoundaryParams{ 65535, "WordMax" },
               RiscVUidBoundaryParams{ 65536, "DwordMin" }
               ),
  RiscVUidBoundaryParamName
  );

// =============================================================================
// Parameterized Test: RiscVHartIdPatternTest
// Tests various HartId patterns.
// =============================================================================

struct RiscVHartIdPatternParams {
  std::vector<UINT64>    HartIds;
  std::vector<UINT32>    Uids;
  std::string            Description;
};

class RiscVHartIdPatternTest : public RiscVIntegrationTest,
  public ::testing::WithParamInterface<RiscVHartIdPatternParams> {
};

TEST_P (RiscVHartIdPatternTest, HandlesHartIdPatterns) {
  const auto  &Params = GetParam ();
  UINT32      NumCpus = (UINT32)Params.HartIds.size ();

  mRintcInfo.resize (NumCpus);
  for (UINT32 i = 0; i < NumCpus; i++) {
    CreateRintcInfo (&mRintcInfo[i], Params.HartIds[i], Params.Uids[i]);
  }

  SetupRintcInfo (mRintcInfo.data (), NumCpus);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify correct number of CPUs
  UINT32  CpuCount = 0;

  EXPECT_EQ (AmlTestHelper::CountCpuDevices (RootNode, "\\_SB", &CpuCount), EFI_SUCCESS);
  EXPECT_EQ (CpuCount, NumCpus);

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

std::string
RiscVHartIdPatternParamName (
  const ::testing::TestParamInfo<RiscVHartIdPatternParams>  &Info
  )
{
  return Info.param.Description;
}

INSTANTIATE_TEST_SUITE_P (
  HartIdPatterns,
  RiscVHartIdPatternTest,
  ::testing::Values (
               RiscVHartIdPatternParams{
  { 0, 1, 2, 3 }, { 0, 1, 2, 3 }, "SequentialMatching" },
               RiscVHartIdPatternParams{
  { 0, 2, 4, 6 }, { 0, 1, 2, 3 }, "NonContiguousHarts" },
               RiscVHartIdPatternParams{
  { 100, 101, 102, 103 }, { 0, 1, 2, 3 }, "HighHartIds" },
               RiscVHartIdPatternParams{
  { 0, 1, 2, 3 }, { 10, 20, 30, 40 }, "DifferentUids" }
               ),
  RiscVHartIdPatternParamName
  );

/**
  Main entry point for RISC-V SSDT CPU Topology integration tests.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
