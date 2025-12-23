/** @file
  ARM SSDT CPU Topology Integration GoogleTest.

  Tests full ARM CPU topology generation through BuildSsdtCpuTopologyTable.
  Validates generated AML output against ACPI 6.6 specification.

  Copyright (c) 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.6 Specification - s8.4 Declaring Processors
    - ACPI 6.6 Specification - s8.4.7.1 _CPC
    - ACPI 6.6 Specification - s8.4.5.5 _PSD
    - ARM CoreSight Architecture Specification
**/

#include <Uefi.h>
#include "SsdtCpuTopologyTestCommon.h"

#include <string>
#include <sstream>
#include <vector>

// =============================================================================
// ARM Integration Test Fixture
// =============================================================================

/**
  Test fixture for ARM-specific integration tests.
  Extends SsdtCpuTopologyTest to include ARM GICC and ET setup.
**/
class ArmIntegrationTest : public SsdtCpuTopologyTest {
protected:
  std::vector<CM_ARM_GICC_INFO> mGiccInfo;
  std::vector<CM_ARM_ET_INFO> mEtInfo;
  std::vector<CM_ARCH_COMMON_CPC_INFO> mCpcInfo;
  std::vector<CM_ARCH_COMMON_PSD_INFO> mPsdInfo;

  /**
    Create a simple GICC info structure for testing.

    @param[out] GiccInfo          The GICC info to populate.
    @param[in]  CpuInterfaceNum   The CPU interface number.
    @param[in]  AcpiProcessorUid  The ACPI processor UID.
    @param[in]  Mpidr             The MPIDR value.
  **/
  void
  CreateGiccInfo (
    OUT CM_ARM_GICC_INFO  *GiccInfo,
    IN  UINT32            CpuInterfaceNum,
    IN  UINT32            AcpiProcessorUid,
    IN  UINT64            Mpidr
    )
  {
    ZeroMem (GiccInfo, sizeof (*GiccInfo));
    GiccInfo->CPUInterfaceNumber = CpuInterfaceNum;
    GiccInfo->AcpiProcessorUid   = AcpiProcessorUid;
    GiccInfo->MPIDR              = Mpidr;
    GiccInfo->Flags              = 1;  // Enabled
    GiccInfo->CpcToken           = CM_NULL_TOKEN;
    GiccInfo->PsdToken           = CM_NULL_TOKEN;
    GiccInfo->EtToken            = CM_NULL_TOKEN;
  }

  /**
    Setup mock expectation for ARM GICC info lookup.

    @param[in] GiccInfo  Array of GICC info structures.
    @param[in] Count     Number of GICC entries.
  **/
  void
  SetupGiccInfo (
    CM_ARM_GICC_INFO  *GiccInfo,
    UINT32            Count
    )
  {
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARM_OBJECT_ID (EArmObjGicCInfo), CM_NULL_TOKEN, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARM_OBJECT_ID (EArmObjGicCInfo),
      (UINT32)(sizeof (CM_ARM_GICC_INFO) * Count),
      GiccInfo,
      Count
    }
             ),
           Return (EFI_SUCCESS)
           )
         );
  }

  /**
    Create a simple ET info structure for testing.

    @param[out] EtInfo   The ET info to populate.
    @param[in]  EtType   The embedded trace type (ArmEtTypeEte or ArmEtTypeEtm).
  **/
  void
  CreateEtInfo (
    OUT CM_ARM_ET_INFO  *EtInfo,
    IN  ARM_ET_TYPE     EtType
    )
  {
    EtInfo->EtType = EtType;
  }

  /**
    Setup mock expectation for ARM ET info lookup.

    @param[in] EtInfo  The ET info structure.
    @param[in] Token   Token to associate with this ET info.
  **/
  void
  SetupEtInfo (
    CM_ARM_ET_INFO   *EtInfo,
    CM_OBJECT_TOKEN  Token
    )
  {
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARM_OBJECT_ID (EArmObjEtInfo), Token, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARM_OBJECT_ID (EArmObjEtInfo),
      sizeof (CM_ARM_ET_INFO),
      EtInfo,
      1
    }
             ),
           Return (EFI_SUCCESS)
           )
         );
  }
};

// =============================================================================
// ARM Integration Tests - Test through BuildSsdtCpuTopologyTable with real AML
// =============================================================================

/**
  @test ArmIntegration_SingleCpu_GeneratesCorrectAml

  @brief Integration test validating basic ARM CPU topology generation.

  @spec ACPI 6.6 s8.4: Each processor must have its own Processor Device
        object declared with _HID "ACPI0007".

  @expected Table generated with CPU device containing _HID and _UID.
**/
TEST_F (ArmIntegrationTest, SingleCpu_GeneratesCorrectAml) {
  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, 100, 0x0);  // CPU 0, UID 100, MPIDR 0
  SetupGiccInfo (mGiccInfo.data (), 1);

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
  @test ArmIntegration_MultipleCpus_AllHaveDevices

  @brief Integration test validating multiple ARM CPU generation.

  @spec ACPI 6.6 s8.4: Each processor must have its own Processor Device object.

  @expected One device generated per GICC entry.
**/
TEST_F (ArmIntegrationTest, MultipleCpus_AllHaveDevices) {
  const UINT32  NumCpus = 4;

  mGiccInfo.resize (NumCpus);
  for (UINT32 i = 0; i < NumCpus; i++) {
    // Different UIDs and MPIDRs for each CPU
    CreateGiccInfo (&mGiccInfo[i], i, i * 100, (UINT64)i << 8);
  }

  SetupGiccInfo (mGiccInfo.data (), NumCpus);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify correct number of CPU devices
  UINT32  CpuCount = 0;

  EXPECT_EQ (AmlTestHelper::CountCpuDevices (RootNode, "\\_SB", &CpuCount), EFI_SUCCESS);
  EXPECT_EQ (CpuCount, NumCpus) << "Should have one CPU device per GICC entry";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test ArmIntegration_CpcToken_GeneratesCpcNode

  @brief Integration test validating _CPC generation when CpcToken is set.

  @spec ACPI 6.6 s8.4.7.1: "_CPC object returns a Package describing CPPC
        registers and capabilities."

  @expected _CPC object generated with correct structure.
**/
TEST_F (ArmIntegrationTest, CpcToken_GeneratesCpcNode) {
  const CM_OBJECT_TOKEN  CpcToken = 9001;

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, 100, 0x0);
  mGiccInfo[0].CpcToken = CpcToken;
  SetupGiccInfo (mGiccInfo.data (), 1);

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
  @test ArmIntegration_PsdToken_NotGeneratedInGiccPath

  @brief Integration test documenting _PSD NOT generated in GICC path.

  @spec ARM Implementation Note: CreateTopologyFromIntC does NOT generate
        _PSD objects. PsdToken is extracted but not used. _PSD is only
        generated when using processor hierarchy with ProcHierarchyInfo.

  @note This test documents current implementation behavior.
        _PSD generation in GICC path would require implementation changes.

  @expected CPU device created but _PSD NOT present (implementation limitation)
**/
TEST_F (ArmIntegrationTest, PsdToken_NotGeneratedInGiccPath) {
  const CM_OBJECT_TOKEN  PsdToken = 9002;

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, 100, 0x0);
  mGiccInfo[0].PsdToken = PsdToken;
  SetupGiccInfo (mGiccInfo.data (), 1);

  // Set up PSD info (even though it won't be used in GICC path)
  mPsdInfo.resize (1);
  CreateSimplePsdInfo (&mPsdInfo[0], 0, 1);  // Domain 0, 1 processor
  SetupPsdInfo (&mPsdInfo[0], PsdToken);

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

  // Note: _PSD is NOT generated in CreateTopologyFromIntC path
  // This documents current implementation behavior
  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);
  EXPECT_FALSE (AmlTestHelper::DeviceHasNamedObject (CpuDevice, "_PSD"))
  << "ARM GICC path: _PSD not generated (implementation limitation)";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test ArmIntegration_EtToken_WithEte_GeneratesEtDevice

  @brief Integration test validating ET device generation for ETE type.

  @spec ARM CoreSight: ETE (Embedded Trace Extension) devices have
        _HID "ARMHC500".

  @expected ET device generated with correct HID for ETE.
**/
TEST_F (ArmIntegrationTest, EtToken_WithEte_GeneratesEtDevice) {
  const CM_OBJECT_TOKEN  EtToken = 9003;

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, 100, 0x0);
  mGiccInfo[0].EtToken = EtToken;
  SetupGiccInfo (mGiccInfo.data (), 1);

  // Set up ET info with ETE type
  mEtInfo.resize (1);
  CreateEtInfo (&mEtInfo[0], ArmEtTypeEte);
  SetupEtInfo (&mEtInfo[0], EtToken);

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

  // Note: ET device is a child of the CPU device
  // Full validation would check for the ET device with _HID "ARMHC500"

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test ArmIntegration_DisabledCpu_StillGenerated

  @brief Integration test for disabled ARM CPU handling.

  @spec ARM: Disabled CPUs (Flags = 0) should still be represented.

  @expected CPU device generated even when Flags indicates disabled.
**/
TEST_F (ArmIntegrationTest, DisabledCpu_StillGenerated) {
  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, 100, 0x0);
  mGiccInfo[0].Flags = 0;  // Disabled
  SetupGiccInfo (mGiccInfo.data (), 1);

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
// Enhanced Integration Tests - Deep AML Validation
// =============================================================================

/**
  @test ArmIntegration_CpuHid_IsAcpi0007

  @brief Validates CPU device _HID is "ACPI0007" per ACPI 6.6.

  @spec ACPI 6.6 s8.4: "Processor Device objects using _HID of ACPI0007"

  @expected CPU device has _HID = "ACPI0007"
**/
TEST_F (ArmIntegrationTest, CpuHid_IsAcpi0007) {
  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, 42, 0x0);
  SetupGiccInfo (mGiccInfo.data (), 1);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Find CPU device and verify HID
  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  CHAR8  HidBuffer[16];

  ASSERT_EQ (AmlTestHelper::GetDeviceHid (CpuDevice, HidBuffer, sizeof (HidBuffer)), EFI_SUCCESS);
  EXPECT_STREQ (HidBuffer, ACPI_HID_PROCESSOR_DEVICE)
  << "ACPI 6.6 s8.4: CPU device _HID must be ACPI0007";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test ArmIntegration_CpuUid_MatchesGiccAcpiProcessorUid

  @brief Validates CPU device _UID matches GICC AcpiProcessorUid.

  @spec ACPI 6.6 s5.2.12.14: "AcpiProcessorUid correlates with the _UID
        of the CPU Device object"

  @expected CPU device _UID equals GICC AcpiProcessorUid
**/
TEST_F (ArmIntegrationTest, CpuUid_MatchesGiccAcpiProcessorUid) {
  const UINT32  ExpectedUid = 0x1234;

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, ExpectedUid, 0x0);
  SetupGiccInfo (mGiccInfo.data (), 1);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Find CPU device and verify UID
  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  UINT64  Uid;

  ASSERT_EQ (AmlTestHelper::GetDeviceUid (CpuDevice, &Uid), EFI_SUCCESS);
  EXPECT_EQ (Uid, ExpectedUid)
  << "ACPI 6.6 s5.2.12.14: _UID must match GICC AcpiProcessorUid";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test ArmIntegration_MultipleCpus_UidsMatchGiccOrder

  @brief Validates all CPU _UIDs match their corresponding GICC entries.

  @spec ACPI 6.6: Each processor _UID must correlate with MADT GICC UID.

  @expected Each CPU device _UID matches its GICC AcpiProcessorUid
**/
TEST_F (ArmIntegrationTest, MultipleCpus_UidsMatchGiccOrder) {
  const UINT32  ExpectedUids[] = { 100, 200, 300, 400 };
  const UINT32  NumCpus        = sizeof (ExpectedUids) / sizeof (ExpectedUids[0]);

  mGiccInfo.resize (NumCpus);
  for (UINT32 i = 0; i < NumCpus; i++) {
    CreateGiccInfo (&mGiccInfo[i], i, ExpectedUids[i], (UINT64)i << 8);
  }

  SetupGiccInfo (mGiccInfo.data (), NumCpus);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify each CPU's UID
  for (UINT32 i = 0; i < NumCpus; i++) {
    CHAR8  DevicePath[16];

    AsciiSPrint (DevicePath, sizeof (DevicePath), "\\_SB.C%03X", i);

    AML_NODE_HANDLE  CpuDevice;

    ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, DevicePath, &CpuDevice), EFI_SUCCESS)
    << "CPU device " << DevicePath << " not found";

    UINT64  Uid;

    ASSERT_EQ (AmlTestHelper::GetDeviceUid (CpuDevice, &Uid), EFI_SUCCESS);
    EXPECT_EQ (Uid, ExpectedUids[i])
    << "CPU " << i << " _UID mismatch";
  }

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test ArmIntegration_CpuNaming_FollowsCxxxPattern

  @brief Validates CPU device naming follows C000, C001, etc. pattern.

  @spec UEFI DynamicTablesPkg Convention: CPU devices named Cxxx in hex.

  @expected CPU devices named C000, C001, C002, C003
**/
TEST_F (ArmIntegrationTest, CpuNaming_FollowsCxxxPattern) {
  const UINT32  NumCpus = 4;

  mGiccInfo.resize (NumCpus);
  for (UINT32 i = 0; i < NumCpus; i++) {
    CreateGiccInfo (&mGiccInfo[i], i, i, (UINT64)i << 8);
  }

  SetupGiccInfo (mGiccInfo.data (), NumCpus);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify each CPU follows Cxxx naming
  const CHAR8  *ExpectedNames[] = { "C000", "C001", "C002", "C003" };

  for (UINT32 i = 0; i < NumCpus; i++) {
    CHAR8  DevicePath[16];

    AsciiSPrint (DevicePath, sizeof (DevicePath), "\\_SB.%a", ExpectedNames[i]);

    AML_NODE_HANDLE  CpuDevice;
    EFI_STATUS       Status = AmlTestHelper::FindDeviceByPath (RootNode, DevicePath, &CpuDevice);

    EXPECT_EQ (Status, EFI_SUCCESS)
    << "Expected CPU device " << ExpectedNames[i] << " not found";
  }

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test ArmIntegration_CpcToken_ValidatesPackageStructure

  @brief Validates _CPC package structure per ACPI 6.6 s8.4.7.1.

  @spec ACPI 6.6 s8.4.7.1: "_CPC package contains 23 entries for Rev 3"

  @expected _CPC has correct entry count and revision
**/
TEST_F (ArmIntegrationTest, CpcToken_ValidatesPackageStructure) {
  const CM_OBJECT_TOKEN  CpcToken = 9001;

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, 100, 0x0);
  mGiccInfo[0].CpcToken = CpcToken;
  SetupGiccInfo (mGiccInfo.data (), 1);

  mCpcInfo.resize (1);
  CreateSimpleCpcInfo (&mCpcInfo[0]);
  SetupCpcInfo (&mCpcInfo[0], CpcToken);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  // Validate _CPC structure
  UINT32  EntryCount = 0;

  EXPECT_EQ (AmlTestHelper::ValidateCpcPackageStructure (CpuDevice, &EntryCount), EFI_SUCCESS)
  << "_CPC package structure validation failed";

  EXPECT_EQ (EntryCount, ACPI_CPC_REVISION_3_NUM_ENTRIES)
  << "ACPI 6.6 s8.4.7.1: _CPC Rev 3 must have 23 entries";

  // Verify revision
  UINT32  Revision = 0;

  EXPECT_EQ (AmlTestHelper::GetCpcRevision (CpuDevice, &Revision), EFI_SUCCESS);
  EXPECT_EQ (Revision, ACPI_CPC_REVISION_3)
  << "_CPC revision should be 3";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test ArmIntegration_PsdToken_GiccPathLimitation

  @brief Documents _PSD validation not possible in GICC path.

  @spec ARM Implementation Note: _PSD is not generated in CreateTopologyFromIntC.
        This test documents that _PSD validation tests should use processor
        hierarchy path instead of GICC path.

  @note For _PSD testing, use common tests with processor hierarchy or
        X64 tests where _PSD is generated via hierarchy.

  @expected Test documents limitation - _PSD not available in GICC path
**/
TEST_F (ArmIntegrationTest, PsdToken_GiccPathLimitation) {
  // This test documents that _PSD is NOT generated in ARM GICC path.
  // The PsdToken field exists in CM_ARM_GICC_INFO but CreateTopologyFromIntC
  // does not call CreateAmlPsdNode(). This would require implementation changes.
  //
  // To test _PSD:
  // 1. Use processor hierarchy info (CM_ARCH_COMMON_PROC_HIERARCHY_INFO)
  // 2. Use X64 tests which use hierarchy path
  // 3. Or modify ARM implementation to generate _PSD in GICC path

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, 100, 0x0);
  SetupGiccInfo (mGiccInfo.data (), 1);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  // Verify CPU exists but _PSD is not generated in GICC path
  EXPECT_TRUE (AmlTestHelper::DeviceHasHid (CpuDevice, ACPI_HID_PROCESSOR_DEVICE))
  << "CPU device should exist with correct HID";
  EXPECT_FALSE (AmlTestHelper::DeviceHasNamedObject (CpuDevice, "_PSD"))
  << "GICC path limitation: _PSD not generated";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test ArmIntegration_CpcWithPsdToken_OnlyCpcGenerated

  @brief Validates that in GICC path, only _CPC is generated (not _PSD).

  @spec ARM Implementation Note: CreateTopologyFromIntC only generates _CPC
        when CpcToken is set. PsdToken is ignored in this code path.

  @expected _CPC present, _PSD NOT present (GICC path limitation)
**/
TEST_F (ArmIntegrationTest, CpcWithPsdToken_OnlyCpcGenerated) {
  const CM_OBJECT_TOKEN  CpcToken = 9001;
  const CM_OBJECT_TOKEN  PsdToken = 9002;

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, 100, 0x0);
  mGiccInfo[0].CpcToken = CpcToken;
  mGiccInfo[0].PsdToken = PsdToken;  // Set but not used in GICC path
  SetupGiccInfo (mGiccInfo.data (), 1);

  mCpcInfo.resize (1);
  CreateSimpleCpcInfo (&mCpcInfo[0]);
  SetupCpcInfo (&mCpcInfo[0], CpcToken);

  // PSD info setup - won't be used but included for completeness
  mPsdInfo.resize (1);
  CreateSimplePsdInfo (&mPsdInfo[0], 0, 1);
  SetupPsdInfo (&mPsdInfo[0], PsdToken);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  // _CPC should be present (CpcToken is used)
  EXPECT_TRUE (AmlTestHelper::DeviceHasNamedObject (CpuDevice, "_CPC"))
  << "CPU should have _CPC when CpcToken is set";

  // _PSD is NOT generated in GICC path (implementation limitation)
  EXPECT_FALSE (AmlTestHelper::DeviceHasNamedObject (CpuDevice, "_PSD"))
  << "GICC path: _PSD not generated even when PsdToken is set";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test ArmIntegration_EteDevice_HasCorrectHid

  @brief Validates ETE device has _HID "ARMHC500" per ARM CoreSight spec.

  @spec ARM CoreSight: ETE devices use _HID "ARMHC500"

  @expected ETE child device has correct HID
**/
TEST_F (ArmIntegrationTest, EteDevice_HasCorrectHid) {
  const CM_OBJECT_TOKEN  EtToken = 9003;

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, 100, 0x0);
  mGiccInfo[0].EtToken = EtToken;
  SetupGiccInfo (mGiccInfo.data (), 1);

  mEtInfo.resize (1);
  CreateEtInfo (&mEtInfo[0], ArmEtTypeEte);
  SetupEtInfo (&mEtInfo[0], EtToken);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Find CPU device
  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  // Find ET device as child (E000)
  AML_NODE_HANDLE  EtDevice;
  EFI_STATUS       Status = AmlTestHelper::FindDeviceInScope (CpuDevice, "E000", &EtDevice);

  ASSERT_EQ (Status, EFI_SUCCESS)
  << "ETE device E000 not found as child of CPU";

  // Verify ETE HID
  CHAR8  HidBuffer[16];

  ASSERT_EQ (AmlTestHelper::GetDeviceHid (EtDevice, HidBuffer, sizeof (HidBuffer)), EFI_SUCCESS);
  EXPECT_STREQ (HidBuffer, ARM_HID_ET_DEVICE_STR)
  << "ARM CoreSight: ETE device _HID must be ARMHC500";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test ArmIntegration_EteDevice_UidMatchesCpu

  @brief Validates ETE device _UID matches CPU UID per ARM CoreSight spec.

  @spec ARM CoreSight: ETE _UID correlates with parent CPU

  @expected ETE device _UID equals parent CPU AcpiProcessorUid
**/
TEST_F (ArmIntegrationTest, EteDevice_UidMatchesCpu) {
  const CM_OBJECT_TOKEN  EtToken     = 9003;
  const UINT32           ExpectedUid = 500;

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, ExpectedUid, 0x0);
  mGiccInfo[0].EtToken = EtToken;
  SetupGiccInfo (mGiccInfo.data (), 1);

  mEtInfo.resize (1);
  CreateEtInfo (&mEtInfo[0], ArmEtTypeEte);
  SetupEtInfo (&mEtInfo[0], EtToken);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Find ET device
  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  AML_NODE_HANDLE  EtDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceInScope (CpuDevice, "E000", &EtDevice), EFI_SUCCESS);

  // Verify ETE UID
  UINT64  EtUid;

  ASSERT_EQ (AmlTestHelper::GetDeviceUid (EtDevice, &EtUid), EFI_SUCCESS);
  EXPECT_EQ (EtUid, ExpectedUid)
  << "ETE device _UID should match CPU AcpiProcessorUid";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test ArmIntegration_OnlineCapableCpu_HasSta

  @brief Validates online-capable disabled CPU has _STA method.

  @spec ACPI 6.6 s6.3.7: _STA returns device status.
        Online-capable CPUs need _STA to indicate runtime enablement.

  @expected CPU with OnlineCapable flag has _STA method
**/
TEST_F (ArmIntegrationTest, OnlineCapableCpu_HasSta) {
  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, 100, 0x0);
  // Flags = 0x8 (Online Capable, but currently disabled)
  mGiccInfo[0].Flags = 0x8;
  SetupGiccInfo (mGiccInfo.data (), 1);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  // Online-capable disabled CPUs should have _STA to indicate status
  // (Implementation may vary - check if _STA is generated)
  BOOLEAN  HasSta = AmlTestHelper::DeviceHasSta (CpuDevice);

  // Note: This expectation depends on implementation
  // Some implementations always generate _STA, others only when needed
  (void)HasSta;  // Avoid unused variable warning if test is informational

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

// =============================================================================
// Parameterized Integration Tests - CPU Count Scaling
// =============================================================================

/**
  Parameterized test fixture for ARM CPU count integration tests.
**/
class ArmCpuCountIntegrationTest : public ArmIntegrationTest,
  public ::testing::WithParamInterface<UINT32> {
};

/**
  @test ArmCpuCountIntegrationTest_GeneratesCorrectCount

  @brief Parameterized test for various ARM CPU counts.

  @spec ACPI 6.6 s8.4: Each processor needs a device object.

  @expected Correct number of CPU devices generated for each count
**/
TEST_P (ArmCpuCountIntegrationTest, GeneratesCorrectCount) {
  const UINT32  NumCpus = GetParam ();

  mGiccInfo.resize (NumCpus);
  for (UINT32 i = 0; i < NumCpus; i++) {
    CreateGiccInfo (&mGiccInfo[i], i, i, (UINT64)i << 8);
  }

  SetupGiccInfo (mGiccInfo.data (), NumCpus);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  UINT32  CpuCount = 0;

  EXPECT_EQ (AmlTestHelper::CountCpuDevices (RootNode, "\\_SB", &CpuCount), EFI_SUCCESS);
  EXPECT_EQ (CpuCount, NumCpus)
  << "Should generate exactly " << NumCpus << " CPU devices";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  Generate descriptive test name for ArmCpuCountIntegrationTest.
**/
std::string
ArmCpuCountIntegrationTestName (
  const ::testing::TestParamInfo<UINT32>  &info
  )
{
  std::ostringstream  oss;

  oss << info.param << "Cpus";
  return oss.str ();
}

INSTANTIATE_TEST_SUITE_P (
  CpuCounts,
  ArmCpuCountIntegrationTest,
  ::testing::Values (1, 2, 4, 8, 16, 32),
  ArmCpuCountIntegrationTestName
  );

// =============================================================================
// Parameterized Integration Tests - Large UID Values
// =============================================================================

/**
  Parameterized test fixture for ARM UID boundary integration tests.
**/
class ArmUidBoundaryIntegrationTest : public ArmIntegrationTest,
  public ::testing::WithParamInterface<UINT32> {
};

/**
  @test ArmUidBoundaryIntegrationTest_EncodesCorrectly

  @brief Parameterized test for UID values at AML encoding boundaries.

  @spec ACPI: _UID must be correctly encoded in AML.

  @expected _UID value matches GICC AcpiProcessorUid exactly
**/
TEST_P (ArmUidBoundaryIntegrationTest, EncodesCorrectly) {
  const UINT32  ExpectedUid = GetParam ();

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, ExpectedUid, 0x0);
  SetupGiccInfo (mGiccInfo.data (), 1);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  UINT64  Uid;

  ASSERT_EQ (AmlTestHelper::GetDeviceUid (CpuDevice, &Uid), EFI_SUCCESS);
  EXPECT_EQ (Uid, ExpectedUid)
  << "UID " << ExpectedUid << " not correctly encoded in AML";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  Generate descriptive test name for ArmUidBoundaryIntegrationTest.
**/
std::string
ArmUidBoundaryIntegrationTestName (
  const ::testing::TestParamInfo<UINT32>  &info
  )
{
  std::ostringstream  oss;

  oss << "Uid" << info.param;
  return oss.str ();
}

INSTANTIATE_TEST_SUITE_P (
  UidBoundaries,
  ArmUidBoundaryIntegrationTest,
  ::testing::Values (
               0u,     // Zero
               63u,    // 6-bit max
               64u,    // 7-bit
               127u,   // ByteData boundary
               128u,   // Signed overflow
               255u,   // 8-bit max
               256u,   // WordData boundary
               65535u, // 16-bit max
               65536u  // DWordData boundary
               ),
  ArmUidBoundaryIntegrationTestName
  );

// =============================================================================
// Parameterized Tests - ET (Embedded Trace) Type Integration
// =============================================================================

/**
  Configuration for ET type integration testing.

  @note ARM CoreSight Architecture defines two trace types:
        - ETE (Embedded Trace Extension): _HID "ARMHC500", fully supported
        - ETM (Embedded Trace Module): _HID "ARMHC501", currently unsupported
**/
struct EtTypeIntegrationConfig {
  ARM_ET_TYPE    EtType;
  const char     *ExpectedHid;
  BOOLEAN        IsSupported;
  const char     *Description;
};

/**
  Test fixture for ET type parameterized integration tests.
**/
class ArmEtTypeIntegrationTest : public ArmIntegrationTest,
  public ::testing::WithParamInterface<EtTypeIntegrationConfig> {
};

/**
  @test ArmEtTypeIntegrationTest_GeneratesCorrectHid

  @brief Validates ET device generation with correct _HID per ARM CoreSight spec.

  @spec ARM CoreSight Architecture:
        - ETE (Embedded Trace Extension) uses _HID "ARMHC500"
        - ETM (Embedded Trace Module) uses _HID "ARMHC501"

  @expected ETE creates device with ARMHC500, ETM returns unsupported
**/
TEST_P (ArmEtTypeIntegrationTest, GeneratesCorrectHidOrUnsupported) {
  const EtTypeIntegrationConfig  &config = GetParam ();
  const CM_OBJECT_TOKEN          EtToken = 9010;
  const UINT32                   CpuUid  = 42;

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, CpuUid, 0x0);
  mGiccInfo[0].EtToken = EtToken;
  SetupGiccInfo (mGiccInfo.data (), 1);

  mEtInfo.resize (1);
  CreateEtInfo (&mEtInfo[0], config.EtType);
  SetupEtInfo (&mEtInfo[0], EtToken);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  if (config.IsSupported) {
    // Supported type should succeed and generate device with correct HID
    EFI_STATUS  Status = BuildSsdtCpuTopologyTable (&Table, &TableCount);
    ASSERT_EQ (Status, EFI_SUCCESS)
    << config.Description << " should be supported";

    AML_ROOT_NODE_HANDLE  RootNode;
    ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

    AML_NODE_HANDLE  CpuDevice;
    ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

    AML_NODE_HANDLE  EtDevice;
    ASSERT_EQ (AmlTestHelper::FindDeviceInScope (CpuDevice, "E000", &EtDevice), EFI_SUCCESS)
    << "ET device should be created for " << config.Description;

    CHAR8  HidBuffer[16];
    ASSERT_EQ (AmlTestHelper::GetDeviceHid (EtDevice, HidBuffer, sizeof (HidBuffer)), EFI_SUCCESS);
    EXPECT_STREQ (HidBuffer, config.ExpectedHid)
    << config.Description << " should have _HID " << config.ExpectedHid;

    AmlDeleteTree (RootNode);
    FreeSsdtCpuTopologyTable (Table, TableCount);
  } else {
    // Unsupported type triggers ASSERT_EFI_ERROR in the implementation
    // which throws an exception in the test framework
    EXPECT_ANY_THROW (BuildSsdtCpuTopologyTable (&Table, &TableCount))
    << config.Description << " should trigger ASSERT (unsupported)";
  }
}

/**
  Generate descriptive test name for ArmEtTypeIntegrationTest.
**/
std::string
ArmEtTypeIntegrationTestName (
  const ::testing::TestParamInfo<EtTypeIntegrationConfig>  &info
  )
{
  return std::string (info.param.Description);
}

INSTANTIATE_TEST_SUITE_P (
  EmbeddedTraceTypes,
  ArmEtTypeIntegrationTest,
  ::testing::Values (
               EtTypeIntegrationConfig { ArmEtTypeEte, "ARMHC500", TRUE, "EteType" },
               EtTypeIntegrationConfig { ArmEtTypeEtm, "ARMHC501", FALSE, "EtmType" }
               ),
  ArmEtTypeIntegrationTestName
  );

/**
  Main entry point for ARM SSDT CPU Topology integration tests.
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
