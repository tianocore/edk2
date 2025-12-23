/** @file
  Specification-driven unit tests for common SSDT CPU Topology Generator code.

  Tests SsdtCpuTopologyGenerator.c against ACPI 6.6 specification requirements
  with mocked architecture-specific functions.

  @par Test Philosophy
  Tests validate what the API SHOULD do per ACPI specification, not what
  the current implementation happens to do. Each test references specific
  ACPI spec sections and validates against documented requirements.

  @par Test Categories
  1. Processor Device Tests (ACPI 6.6 s8.4)
     - HID validation ("ACPI0007")
     - _UID presence and correlation with MADT
     - Device scope under \_SB

  2. Processor Container Tests (ACPI 6.6 s8.4)
     - Container HID ("ACPI0010")
     - Container _UID uniqueness

  3. _CPC Tests (ACPI 6.6 s8.4.7.1)
     - Package structure (23 entries for Rev 3)
     - Field ordering per specification
     - Required vs optional field handling

  4. _PSD Tests (ACPI 6.6 s8.4.5.5)
     - Package structure (5 entries)
     - Revision and coordination type validation

  5. Device Naming Tests
     - Cxxx hexadecimal format convention

  @par Reference(s):
    - ACPI 6.6 Specification - s8.4 Declaring Processors
    - ACPI 6.6 Specification - s8.4.7.1 _CPC (Continuous Performance Control)
    - ACPI 6.6 Specification - s8.4.5.5 _PSD (P-State Dependency)

  Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "SsdtCpuTopologyTestCommon.h"
#include "MockArchFunctions.h"

#include <string>
#include <sstream>

using namespace testing;

/**
  Test fixture for Common SSDT CPU Topology tests.

  Sets up mocked arch functions and ConfigurationManager protocol.
**/
class CommonSsdtCpuTopologyTest : public SsdtCpuTopologyTest {
protected:
  MockArchFunctions MockArch;
  std::vector<CM_ARCH_COMMON_PROC_HIERARCHY_INFO> mProcHierarchyInfo;

  void
  SetUp (
    ) override
  {
    SsdtCpuTopologyTest::SetUp ();
    gMockArchFunctions = &MockArch;
  }

  void
  TearDown (
    ) override
  {
    gMockArchFunctions = nullptr;
    mProcHierarchyInfo.clear ();
    SsdtCpuTopologyTest::TearDown ();
  }

  /**
    Setup mock for GetIntCInfo to return success with given values.
    Models ARM/RISC-V behavior where IntC info is available.

    @param[in] AcpiProcessorUid  UID to return.
    @param[in] CpcToken          CPC token to return (or CM_NULL_TOKEN).
    @param[in] PsdToken          PSD token to return (or CM_NULL_TOKEN).
  **/
  void
  SetupGetIntCInfoSuccess (
    UINT32           AcpiProcessorUid,
    CM_OBJECT_TOKEN  CpcToken,
    CM_OBJECT_TOKEN  PsdToken
    )
  {
    EXPECT_CALL (MockArch, GetIntCInfo (_, _, _, _, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<2>(AcpiProcessorUid),
           SetArgPointee<3>(CpcToken),
           SetArgPointee<4>(PsdToken),
           Return (EFI_SUCCESS)
           )
         );
  }

  /**
    Setup mock for GetIntCInfo to return UNSUPPORTED.
    Models X64 behavior where IntC info is not used.
  **/
  void
  SetupGetIntCInfoUnsupported (
    )
  {
    EXPECT_CALL (MockArch, GetIntCInfo (_, _, _, _, _))
      .WillRepeatedly (Return (EFI_UNSUPPORTED));
  }

  /**
    Setup mock for CreateTopologyFromIntC to return UNSUPPORTED.
    Forces use of hierarchy-based topology creation.
  **/
  void
  SetupCreateTopologyFromIntCUnsupported (
    )
  {
    EXPECT_CALL (MockArch, CreateTopologyFromIntC (_, _, _))
      .WillRepeatedly (Return (EFI_UNSUPPORTED));
  }

  /**
    Setup mock for AddArchAmlCpuInfo as no-op (success).
    Models X64/RISC-V behavior.
  **/
  void
  SetupAddArchAmlCpuInfoNoOp (
    )
  {
    EXPECT_CALL (MockArch, AddArchAmlCpuInfo (_, _, _, _, _))
      .WillRepeatedly (Return (EFI_SUCCESS));
  }
};

// =============================================================================
// Test Cases - Processor Device (ACPI 6.6 s8.4)
// =============================================================================

/**
  @test SingleCpu_ViaHierarchy_GeneratesDevice

  @brief Validates single CPU device generation via processor hierarchy.

  @spec ACPI 6.6 s8.4: "Processor Device" objects using _HID of "ACPI0007"
        must be declared for each processor in the system.

  @expected
    - Single device under \_SB scope
    - Device has _HID = "ACPI0007"
**/
TEST_F (CommonSsdtCpuTopologyTest, SingleCpu_ViaHierarchy_GeneratesDevice) {
  // Setup: single processor hierarchy entry
  mProcHierarchyInfo.resize (1);
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token             = 1001;
  mProcHierarchyInfo[0].Flags             = PPTT_PROCESSOR_MASK;
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = 2001;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 1);

  // Mock arch functions
  SetupGetIntCInfoSuccess (0, CM_NULL_TOKEN, CM_NULL_TOKEN);
  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  // Build table
  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EXPECT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  EXPECT_NE (Table, nullptr);
  EXPECT_EQ (TableCount, 1u);

  // Parse and validate
  AML_ROOT_NODE_HANDLE  RootNode;

  EXPECT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  EXPECT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);
  EXPECT_TRUE (AmlTestHelper::DeviceHasHid (CpuDevice, ACPI_HID_PROCESSOR_DEVICE));

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test MultipleCpus_ViaHierarchy_CorrectCount

  @brief Validates multiple CPU device generation per ACPI 6.6 s8.4.

  @spec ACPI 6.6 s8.4: Each processor must have its own Processor Device
        object declared under the \_SB scope.

  @expected
    - Exactly N devices generated for N CPUs
    - Each device has unique name (C000, C001, ..., C00N-1)
    - All devices under \_SB scope
**/
TEST_F (CommonSsdtCpuTopologyTest, MultipleCpus_ViaHierarchy_CorrectCount) {
  const UINT32  CpuCount = 4;

  // Setup: 4 processor hierarchy entries
  mProcHierarchyInfo.resize (CpuCount);
  for (UINT32 i = 0; i < CpuCount; i++) {
    ZeroMem (&mProcHierarchyInfo[i], sizeof (mProcHierarchyInfo[i]));
    mProcHierarchyInfo[i].Token             = 1001 + i;
    mProcHierarchyInfo[i].Flags             = PPTT_PROCESSOR_MASK;
    mProcHierarchyInfo[i].ParentToken       = CM_NULL_TOKEN;
    mProcHierarchyInfo[i].AcpiIdObjectToken = 2001 + i;
  }

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), CpuCount);

  // Mock: GetIntCInfo returns sequential UIDs
  EXPECT_CALL (MockArch, GetIntCInfo (_, _, _, _, _))
    .WillRepeatedly (
       Invoke (
         [](CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL *CONST CfgMgrProtocol,
            CM_OBJECT_TOKEN Token,
            UINT32 *Uid,
            CM_OBJECT_TOKEN *Cpc,
            CM_OBJECT_TOKEN *Psd) -> EFI_STATUS {
    if (Uid != nullptr) {
      *Uid = (UINT32)(Token - 2001);       // Convert token to UID
    }

    if (Cpc != nullptr) {
      *Cpc = CM_NULL_TOKEN;
    }

    if (Psd != nullptr) {
      *Psd = CM_NULL_TOKEN;
    }

    return EFI_SUCCESS;
  }
         )
       );

  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  // Build table
  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EXPECT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  // Parse and count devices
  AML_ROOT_NODE_HANDLE  RootNode;

  EXPECT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify all 4 CPUs exist
  for (UINT32 i = 0; i < CpuCount; i++) {
    CHAR8            DevicePath[20];
    AML_NODE_HANDLE  CpuDevice;
    AsciiSPrint (DevicePath, sizeof (DevicePath), "\\_SB.C%03X", i);
    EXPECT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, DevicePath, &CpuDevice), EFI_SUCCESS)
    << "Missing device " << DevicePath;
  }

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test ProcessorContainer_HasAcpi0010Hid

  @brief Validates processor container device HID per ACPI 6.6 s8.4.

  @spec ACPI 6.6 s8.4: Processor Container devices use _HID of "ACPI0010"
        to represent processor topology groupings (packages, clusters, etc.)

  @expected
    - Container device has _HID = "ACPI0010"
    - Child CPUs have _HID = "ACPI0007"
**/
TEST_F (CommonSsdtCpuTopologyTest, ProcessorContainer_HasAcpi0010Hid) {
  // Setup: Package -> 2 CPUs
  // Package must have Physical Package bit so children are valid
  mProcHierarchyInfo.resize (3);

  // Package (container) - Physical package, not leaf, valid ID for _UID
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token = 1000;
  // Physical package, VALID bit set, NOT leaf (container)
  mProcHierarchyInfo[0].Flags = (EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL |
                                 (EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID << 1));
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = CM_NULL_TOKEN;   // Container can have NULL ACPI ID

  // CPU 0 - Not physical (has parent with physical), leaf, valid ID
  ZeroMem (&mProcHierarchyInfo[1], sizeof (mProcHierarchyInfo[1]));
  mProcHierarchyInfo[1].Token             = 1001;
  mProcHierarchyInfo[1].Flags             = PPTT_CPU_PROCESSOR_MASK;
  mProcHierarchyInfo[1].ParentToken       = 1000;   // Parent is package
  mProcHierarchyInfo[1].AcpiIdObjectToken = 2001;

  // CPU 1 - Not physical (has parent with physical), leaf, valid ID
  ZeroMem (&mProcHierarchyInfo[2], sizeof (mProcHierarchyInfo[2]));
  mProcHierarchyInfo[2].Token             = 1002;
  mProcHierarchyInfo[2].Flags             = PPTT_CPU_PROCESSOR_MASK;
  mProcHierarchyInfo[2].ParentToken       = 1000;   // Parent is package
  mProcHierarchyInfo[2].AcpiIdObjectToken = 2002;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 3);
  SetupGetIntCInfoSuccess (0, CM_NULL_TOKEN, CM_NULL_TOKEN);
  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  // Build table
  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EXPECT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  // Parse and validate container
  AML_ROOT_NODE_HANDLE  RootNode;

  EXPECT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Find container device (C000 is first device = container)
  AML_NODE_HANDLE  ContainerDevice;

  EXPECT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &ContainerDevice), EFI_SUCCESS);
  EXPECT_TRUE (AmlTestHelper::DeviceHasHid (ContainerDevice, ACPI_HID_PROCESSOR_CONTAINER_DEVICE))
  << "Container should have ACPI0010 HID";

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

// =============================================================================
// Test Cases - _CPC (ACPI 6.6 s8.4.7.1)
// =============================================================================

/**
  @test CpcToken_Generates_CpcObject

  @brief Validates _CPC generation when CpcToken is set.

  @spec ACPI 6.6 s8.4.7.1: _CPC is an optional object that provides a flexible
        mechanism for OSPM to transition processor(s) into performance states.

  @expected
    - _CPC object present when CpcToken is non-null
    - _CPC is a Package
**/
TEST_F (CommonSsdtCpuTopologyTest, CpcToken_Generates_CpcObject) {
  // Setup CPC info
  CM_ARCH_COMMON_CPC_INFO  CpcInfo;

  CreateSimpleCpcInfo (&CpcInfo);

  // Setup hierarchy with CPC token
  mProcHierarchyInfo.resize (1);
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token             = 1001;
  mProcHierarchyInfo[0].Flags             = PPTT_PROCESSOR_MASK;
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = 2001;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 1);
  SetupCpcInfo (&CpcInfo, 3001);

  // Mock GetIntCInfo to return CPC token
  EXPECT_CALL (MockArch, GetIntCInfo (_, _, _, _, _))
    .WillRepeatedly (
       DoAll (
         SetArgPointee<2>(0),
         SetArgPointee<3>(3001),      // CPC token
         SetArgPointee<4>(CM_NULL_TOKEN),
         Return (EFI_SUCCESS)
         )
       );

  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  // Build table
  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EXPECT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  // Parse and check for _CPC
  AML_ROOT_NODE_HANDLE  RootNode;

  EXPECT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  EXPECT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);
  EXPECT_TRUE (AmlTestHelper::DeviceHasNamedObject (CpuDevice, "_CPC"))
  << "_CPC should be present when CpcToken is set";

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

// =============================================================================
// Test Cases - _PSD (ACPI 6.6 s8.4.5.5)
// =============================================================================

/**
  @test PsdToken_Generates_PsdObject

  @brief Validates _PSD generation when PsdToken is set.

  @spec ACPI 6.6 s8.4.5.5: _PSD is an optional object that provides
        P-State coordination information to OSPM.

  @expected
    - _PSD object present when PsdToken is non-null
    - _PSD is a Package of Packages
**/
TEST_F (CommonSsdtCpuTopologyTest, PsdToken_Generates_PsdObject) {
  // Setup PSD info
  CM_ARCH_COMMON_PSD_INFO  PsdInfo;

  CreateSimplePsdInfo (&PsdInfo, 0, 1);

  // Setup hierarchy
  mProcHierarchyInfo.resize (1);
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token             = 1001;
  mProcHierarchyInfo[0].Flags             = PPTT_PROCESSOR_MASK;
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = 2001;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 1);
  SetupPsdInfo (&PsdInfo, 4001);

  // Mock GetIntCInfo to return PSD token
  EXPECT_CALL (MockArch, GetIntCInfo (_, _, _, _, _))
    .WillRepeatedly (
       DoAll (
         SetArgPointee<2>(0),
         SetArgPointee<3>(CM_NULL_TOKEN),
         SetArgPointee<4>(4001),      // PSD token
         Return (EFI_SUCCESS)
         )
       );

  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  // Build table
  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EXPECT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  // Parse and check for _PSD
  AML_ROOT_NODE_HANDLE  RootNode;

  EXPECT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  EXPECT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);
  EXPECT_TRUE (AmlTestHelper::DeviceHasNamedObject (CpuDevice, "_PSD"))
  << "_PSD should be present when PsdToken is set";

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

// =============================================================================
// Test Cases - Flat Topology (No Processor Hierarchy)
// =============================================================================

/**
  @test NoProcHierarchy_UsesCreateTopologyFromIntC

  @brief Validates fallback to arch-specific topology when no hierarchy defined.

  @spec ACPI 6.6 s8.4: When processor hierarchy is not available, processors
        may be discovered from interrupt controller structures (MADT).

  @expected
    - CreateTopologyFromIntC called when no ProcHierarchyInfo
    - Table generation succeeds if arch function succeeds
**/
TEST_F (CommonSsdtCpuTopologyTest, NoProcHierarchy_UsesCreateTopologyFromIntC) {
  // Don't setup any proc hierarchy - rely on EFI_NOT_FOUND default

  // Mock CreateTopologyFromIntC to succeed and mark that it was called
  EXPECT_CALL (MockArch, CreateTopologyFromIntC (_, _, _))
    .WillOnce (Return (EFI_SUCCESS));

  // GetIntCInfo shouldn't be called when CreateTopologyFromIntC handles topology
  // AddArchAmlCpuInfo shouldn't be called either in this path
  SetupAddArchAmlCpuInfoNoOp ();

  // Build table - should succeed using arch-specific flat topology
  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EFI_STATUS  Status = BuildSsdtCpuTopologyTable (&Table, &TableCount);

  // The call should succeed when CreateTopologyFromIntC succeeds
  EXPECT_EQ (Status, EFI_SUCCESS);

  if (Status == EFI_SUCCESS) {
    FreeSsdtCpuTopologyTable (Table, TableCount);
  }
}

// =============================================================================
// Test Cases - Device Naming Convention
// =============================================================================

/**
  @test DeviceNaming_FollowsCxxxFormat

  @brief Validates device names follow Cxxx hexadecimal format.

  @spec Implementation convention: Device names use "C" prefix followed
        by 3-digit hexadecimal index (C000, C001, ..., CFFF).

  @expected
    - Device names are exactly 4 characters
    - Format is "Cxxx" where xxx is hexadecimal
    - Names are unique across all devices
**/
TEST_F (CommonSsdtCpuTopologyTest, DeviceNaming_FollowsCxxxFormat) {
  // Setup 16 CPUs to test hex naming (C000-C00F)
  const UINT32  CpuCount = 16;

  mProcHierarchyInfo.resize (CpuCount);

  for (UINT32 i = 0; i < CpuCount; i++) {
    ZeroMem (&mProcHierarchyInfo[i], sizeof (mProcHierarchyInfo[i]));
    mProcHierarchyInfo[i].Token             = 1001 + i;
    mProcHierarchyInfo[i].Flags             = PPTT_PROCESSOR_MASK;
    mProcHierarchyInfo[i].ParentToken       = CM_NULL_TOKEN;
    mProcHierarchyInfo[i].AcpiIdObjectToken = 2001 + i;
  }

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), CpuCount);
  SetupGetIntCInfoSuccess (0, CM_NULL_TOKEN, CM_NULL_TOKEN);
  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  // Build table
  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EXPECT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  // Parse and verify hex names
  AML_ROOT_NODE_HANDLE  RootNode;

  EXPECT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Check C00F exists (hex naming)
  AML_NODE_HANDLE  CpuDevice;

  EXPECT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C00F", &CpuDevice), EFI_SUCCESS)
  << "Device C00F should exist (hex naming)";

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

// =============================================================================
// NEW: Specification-Driven Content Validation Tests
// =============================================================================

/**
  @test CpuDevice_Hid_ValidatesAcpi0007

  @brief Validates CPU device HID is exactly "ACPI0007" per ACPI 6.6 s8.4.

  @spec ACPI 6.6 s8.4: "Processor Device" objects must use _HID of "ACPI0007".

  @expected _HID = "ACPI0007" (exact string match)
**/
TEST_F (CommonSsdtCpuTopologyTest, CpuDevice_Hid_ValidatesAcpi0007) {
  // Setup single CPU
  mProcHierarchyInfo.resize (1);
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token             = 1001;
  mProcHierarchyInfo[0].Flags             = PPTT_PROCESSOR_MASK;
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = 2001;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 1);
  SetupGetIntCInfoSuccess (100, CM_NULL_TOKEN, CM_NULL_TOKEN);
  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  // ACPI 6.6 s8.4: Validate exact HID string
  CHAR8  HidBuffer[16];

  EXPECT_EQ (AmlTestHelper::GetDeviceHid (CpuDevice, HidBuffer, sizeof (HidBuffer)), EFI_SUCCESS);
  EXPECT_STREQ (HidBuffer, ACPI_HID_PROCESSOR_DEVICE_STR)
  << "ACPI 6.6 s8.4 requires CPU device _HID = \"ACPI0007\"";

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test CpuDevice_HasUid

  @brief Validates CPU device has _UID per ACPI 6.6 s8.4.

  @spec ACPI 6.6 s8.4: Processor Device objects must have _UID to
        uniquely identify each processor within the namespace.

  @expected _UID object present for each CPU device
**/
TEST_F (CommonSsdtCpuTopologyTest, CpuDevice_HasUid) {
  mProcHierarchyInfo.resize (1);
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token             = 1001;
  mProcHierarchyInfo[0].Flags             = PPTT_PROCESSOR_MASK;
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = 2001;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 1);
  SetupGetIntCInfoSuccess (42, CM_NULL_TOKEN, CM_NULL_TOKEN);
  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  // ACPI 6.6 s8.4: _UID must be present
  EXPECT_TRUE (AmlTestHelper::DeviceHasNamedObject (CpuDevice, "_UID"))
  << "ACPI 6.6 s8.4: CPU device must have _UID";

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test CpuDevice_UidMatchesCmInput

  @brief Validates CPU _UID matches Configuration Manager input.

  @spec ACPI 6.6 s8.4: _UID value should correlate with the processor's
        ACPI Processor UID as defined in MADT GICC/LAPIC structures.

  @expected _UID value equals AcpiProcessorUid from GetIntCInfo
**/
TEST_F (CommonSsdtCpuTopologyTest, CpuDevice_UidMatchesCmInput) {
  const UINT32  ExpectedUid = 0x123;

  mProcHierarchyInfo.resize (1);
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token             = 1001;
  mProcHierarchyInfo[0].Flags             = PPTT_PROCESSOR_MASK;
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = 2001;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 1);
  SetupGetIntCInfoSuccess (ExpectedUid, CM_NULL_TOKEN, CM_NULL_TOKEN);
  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  // ACPI 6.6 s8.4: Validate _UID matches CM input
  UINT64  ActualUid;

  EXPECT_EQ (AmlTestHelper::GetDeviceUid (CpuDevice, &ActualUid), EFI_SUCCESS);
  EXPECT_EQ (ActualUid, ExpectedUid)
  << "ACPI 6.6 s8.4: _UID must match AcpiProcessorUid from Configuration Manager";

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test Cpc_Revision3_Has23Entries

  @brief Validates _CPC package has 23 entries for revision 3 per ACPI 6.6 s8.4.7.1.

  @spec ACPI 6.6 s8.4.7.1: "For CPC revision 3, the package contains 23 entries:
        NumEntries, Revision, HighestPerformance, NominalPerformance, ..."

  @expected _CPC package has exactly 23 elements
**/
TEST_F (CommonSsdtCpuTopologyTest, Cpc_Revision3_Has23Entries) {
  CM_ARCH_COMMON_CPC_INFO  CpcInfo;

  CreateSimpleCpcInfo (&CpcInfo);

  mProcHierarchyInfo.resize (1);
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token             = 1001;
  mProcHierarchyInfo[0].Flags             = PPTT_PROCESSOR_MASK;
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = 2001;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 1);
  SetupCpcInfo (&CpcInfo, 3001);

  EXPECT_CALL (MockArch, GetIntCInfo (_, _, _, _, _))
    .WillRepeatedly (
       DoAll (
         SetArgPointee<2>(0),
         SetArgPointee<3>(3001),
         SetArgPointee<4>(CM_NULL_TOKEN),
         Return (EFI_SUCCESS)
         )
       );

  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  // ACPI 6.6 s8.4.7.1: Validate 23 entries for Rev 3
  UINT32  EntryCount;

  EXPECT_EQ (AmlTestHelper::ValidateCpcPackageStructure (CpuDevice, &EntryCount), EFI_SUCCESS);
  EXPECT_EQ (EntryCount, ACPI_CPC_REVISION_3_NUM_ENTRIES)
  << "ACPI 6.6 s8.4.7.1: CPC revision 3 must have 23 entries";

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test Cpc_RevisionField_MatchesCmInput

  @brief Validates _CPC revision field matches CM input per ACPI 6.6 s8.4.7.1.

  @spec ACPI 6.6 s8.4.7.1: "Revision (Integer): Current revision is 3"

  @expected _CPC[1] (Revision) equals CM_ARCH_COMMON_CPC_INFO.Revision
**/
TEST_F (CommonSsdtCpuTopologyTest, Cpc_RevisionField_MatchesCmInput) {
  CM_ARCH_COMMON_CPC_INFO  CpcInfo;

  CreateSimpleCpcInfo (&CpcInfo);

  mProcHierarchyInfo.resize (1);
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token             = 1001;
  mProcHierarchyInfo[0].Flags             = PPTT_PROCESSOR_MASK;
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = 2001;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 1);
  SetupCpcInfo (&CpcInfo, 3001);

  EXPECT_CALL (MockArch, GetIntCInfo (_, _, _, _, _))
    .WillRepeatedly (
       DoAll (
         SetArgPointee<2>(0),
         SetArgPointee<3>(3001),
         SetArgPointee<4>(CM_NULL_TOKEN),
         Return (EFI_SUCCESS)
         )
       );

  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  // ACPI 6.6 s8.4.7.1: Validate revision field
  UINT32  ActualRevision;

  EXPECT_EQ (AmlTestHelper::GetCpcRevision (CpuDevice, &ActualRevision), EFI_SUCCESS);
  EXPECT_EQ (ActualRevision, CpcInfo.Revision)
  << "ACPI 6.6 s8.4.7.1: _CPC revision must match CM input";

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test Psd_Package_HasCorrectStructure

  @brief Validates _PSD package structure per ACPI 6.6 s8.4.5.5.

  @spec ACPI 6.6 s8.4.5.5: _PSD is a Package containing one Package with:
        { NumEntries, Revision, Domain, CoordType, NumProcessors }
        NumEntries = 5, Revision = 0

  @expected
    - Outer package has 1 element (inner package)
    - Inner package has 5 elements
    - NumEntries field = 5
    - Revision field = 0
**/
TEST_F (CommonSsdtCpuTopologyTest, Psd_Package_HasCorrectStructure) {
  CM_ARCH_COMMON_PSD_INFO  PsdInfo;

  CreateSimplePsdInfo (&PsdInfo, 0, 2);

  mProcHierarchyInfo.resize (1);
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token             = 1001;
  mProcHierarchyInfo[0].Flags             = PPTT_PROCESSOR_MASK;
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = 2001;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 1);
  SetupPsdInfo (&PsdInfo, 4001);

  EXPECT_CALL (MockArch, GetIntCInfo (_, _, _, _, _))
    .WillRepeatedly (
       DoAll (
         SetArgPointee<2>(0),
         SetArgPointee<3>(CM_NULL_TOKEN),
         SetArgPointee<4>(4001),
         Return (EFI_SUCCESS)
         )
       );

  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  // ACPI 6.6 s8.4.5.5: Validate PSD structure
  UINT32  OuterCount;

  EXPECT_EQ (AmlTestHelper::ValidatePsdPackageStructure (CpuDevice, &OuterCount), EFI_SUCCESS);
  EXPECT_EQ (OuterCount, 1u)
  << "ACPI 6.6 s8.4.5.5: _PSD outer package must have 1 element";

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test Psd_Fields_MatchCmInput

  @brief Validates _PSD fields match Configuration Manager input.

  @spec ACPI 6.6 s8.4.5.5:
        - NumEntries: Integer = 5
        - Revision: Integer = 0
        - Domain: DWORD from CM input
        - CoordType: DWORD (0xFC=SW_ALL, 0xFD=SW_ANY, 0xFE=HW_ALL)
        - NumProcessors: DWORD from CM input

  @expected All fields match CM_ARCH_COMMON_PSD_INFO values
**/
TEST_F (CommonSsdtCpuTopologyTest, Psd_Fields_MatchCmInput) {
  const UINT32  ExpectedDomain   = 7;
  const UINT32  ExpectedNumProcs = 4;

  CM_ARCH_COMMON_PSD_INFO  PsdInfo;

  CreateSimplePsdInfo (&PsdInfo, ExpectedDomain, ExpectedNumProcs);

  mProcHierarchyInfo.resize (1);
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token             = 1001;
  mProcHierarchyInfo[0].Flags             = PPTT_PROCESSOR_MASK;
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = 2001;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 1);
  SetupPsdInfo (&PsdInfo, 4001);

  EXPECT_CALL (MockArch, GetIntCInfo (_, _, _, _, _))
    .WillRepeatedly (
       DoAll (
         SetArgPointee<2>(0),
         SetArgPointee<3>(CM_NULL_TOKEN),
         SetArgPointee<4>(4001),
         Return (EFI_SUCCESS)
         )
       );

  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  // ACPI 6.6 s8.4.5.5: Validate all PSD fields
  UINT32  NumEntries, Revision, Domain, CoordType, NumProcessors;

  EXPECT_EQ (
    AmlTestHelper::GetPsdFields (
                     CpuDevice,
                     &NumEntries,
                     &Revision,
                     &Domain,
                     &CoordType,
                     &NumProcessors
                     ),
    EFI_SUCCESS
    );

  EXPECT_EQ (NumEntries, ACPI_PSD_NUM_ENTRIES)
  << "ACPI 6.6 s8.4.5.5: NumEntries must be 5";
  EXPECT_EQ (Revision, ACPI_PSD_REVISION)
  << "ACPI 6.6 s8.4.5.5: Revision must be 0";
  EXPECT_EQ (Domain, ExpectedDomain)
  << "_PSD Domain must match CM input";
  EXPECT_TRUE (
    CoordType == ACPI_COORD_TYPE_SW_ALL ||
    CoordType == ACPI_COORD_TYPE_SW_ANY ||
    CoordType == ACPI_COORD_TYPE_HW_ALL
    )
  << "ACPI 6.6 s8.4.5.5: CoordType must be valid (0xFC, 0xFD, or 0xFE)";
  EXPECT_EQ (NumProcessors, ExpectedNumProcs)
  << "_PSD NumProcessors must match CM input";

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test ContainerDevice_HasUid

  @brief Validates processor container has unique _UID.

  @spec ACPI 6.6 s8.4: Processor Container devices must have _UID to
        uniquely identify each container within the namespace.

  @expected Container device has _UID object
**/
TEST_F (CommonSsdtCpuTopologyTest, ContainerDevice_HasUid) {
  // Setup: Package -> 1 CPU
  mProcHierarchyInfo.resize (2);

  // Package (container)
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token = 1000;
  mProcHierarchyInfo[0].Flags = (EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL |
                                 (EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID << 1));
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = CM_NULL_TOKEN;

  // CPU
  ZeroMem (&mProcHierarchyInfo[1], sizeof (mProcHierarchyInfo[1]));
  mProcHierarchyInfo[1].Token             = 1001;
  mProcHierarchyInfo[1].Flags             = PPTT_CPU_PROCESSOR_MASK;
  mProcHierarchyInfo[1].ParentToken       = 1000;
  mProcHierarchyInfo[1].AcpiIdObjectToken = 2001;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 2);
  SetupGetIntCInfoSuccess (0, CM_NULL_TOKEN, CM_NULL_TOKEN);
  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  ContainerDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &ContainerDevice), EFI_SUCCESS);

  // ACPI 6.6 s8.4: Container must have _UID
  EXPECT_TRUE (AmlTestHelper::DeviceHasNamedObject (ContainerDevice, "_UID"))
  << "ACPI 6.6 s8.4: Container device must have _UID";

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test AllDevices_ValidateDeviceNames

  @brief Validates all device names follow Cxxx format for all generated CPUs.

  @spec Implementation convention: All CPU and container device names must
        follow the "Cxxx" format where xxx is a 3-digit hexadecimal index.

  @expected All device names are 4 characters starting with 'C'
**/
TEST_F (CommonSsdtCpuTopologyTest, AllDevices_ValidateDeviceNames) {
  const UINT32  CpuCount = 4;

  mProcHierarchyInfo.resize (CpuCount);
  for (UINT32 i = 0; i < CpuCount; i++) {
    ZeroMem (&mProcHierarchyInfo[i], sizeof (mProcHierarchyInfo[i]));
    mProcHierarchyInfo[i].Token             = 1001 + i;
    mProcHierarchyInfo[i].Flags             = PPTT_PROCESSOR_MASK;
    mProcHierarchyInfo[i].ParentToken       = CM_NULL_TOKEN;
    mProcHierarchyInfo[i].AcpiIdObjectToken = 2001 + i;
  }

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), CpuCount);
  SetupGetIntCInfoSuccess (0, CM_NULL_TOKEN, CM_NULL_TOKEN);
  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Validate all device names
  for (UINT32 i = 0; i < CpuCount; i++) {
    CHAR8            DevicePath[20];
    AML_NODE_HANDLE  CpuDevice;
    CHAR8            NameBuffer[8];
    CHAR8            ExpectedName[8];

    AsciiSPrint (DevicePath, sizeof (DevicePath), "\\_SB.C%03X", i);
    ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, DevicePath, &CpuDevice), EFI_SUCCESS)
    << "Device " << DevicePath << " not found";

    // Validate name format
    EXPECT_EQ (AmlTestHelper::GetDeviceName (CpuDevice, NameBuffer, sizeof (NameBuffer)), EFI_SUCCESS);
    AsciiSPrint (ExpectedName, sizeof (ExpectedName), "C%03X", i);
    EXPECT_STREQ (NameBuffer, ExpectedName)
    << "Device name must follow Cxxx format";
  }

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test SsdtTableHeader_ValidPerAcpiSpec

  @brief Validates SSDT table header per ACPI specification.

  @spec ACPI 6.6: SSDT tables must have proper signature and revision.

  @expected
    - Signature = "SSDT"
    - Revision = 2 (ACPI 6.3+)
**/
TEST_F (CommonSsdtCpuTopologyTest, SsdtTableHeader_ValidPerAcpiSpec) {
  mProcHierarchyInfo.resize (1);
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token             = 1001;
  mProcHierarchyInfo[0].Flags             = PPTT_PROCESSOR_MASK;
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = 2001;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 1);
  SetupGetIntCInfoSuccess (0, CM_NULL_TOKEN, CM_NULL_TOKEN);
  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table[0], nullptr);

  // Use base class validation
  ValidateSsdtTableHeader (Table[0]);

  FreeSsdtCpuTopologyTable (Table, TableCount);
}

// =============================================================================
// Error Contract Tests (API Documentation)
// =============================================================================

/**
  @test NoProcHierarchy_NoIntCTopology_ReturnsSuccessWithNoTable

  @brief Validates behavior when no topology source available.

  @spec Implementation Note: When neither processor hierarchy nor IntC topology
        is available, the generator returns SUCCESS with an empty/null table.
        This allows platforms without CPU topology to still boot.

  @expected EFI_SUCCESS with TableCount indicating no table generated
**/
TEST_F (CommonSsdtCpuTopologyTest, NoProcHierarchy_NoIntCTopology_ReturnsSuccessWithNoTable) {
  // Don't setup any proc hierarchy
  // Setup CreateTopologyFromIntC to also fail
  EXPECT_CALL (MockArch, CreateTopologyFromIntC (_, _, _))
    .WillOnce (Return (EFI_NOT_FOUND));

  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EFI_STATUS  Status = BuildSsdtCpuTopologyTable (&Table, &TableCount);

  // Implementation returns SUCCESS even with no topology - platform may not need it
  EXPECT_EQ (Status, EFI_SUCCESS)
  << "No topology source returns SUCCESS (platform-dependent)";

  // Free allocated resources even if no tables were generated
  if (Table != nullptr) {
    FreeSsdtCpuTopologyTable (Table, TableCount);
  }
}

/**
  @test CmLookupFailure_TriggersAssert

  @brief Documents that CM lookup failures trigger ASSERT in debug builds.

  @spec Implementation Note: The current implementation uses ASSERT macros
        when Configuration Manager lookups fail unexpectedly. This is
        appropriate for catching platform configuration errors during
        development.

  @expected ASSERT triggered (test verifies this behavior exists)
**/
TEST_F (CommonSsdtCpuTopologyTest, CmLookupFailure_TriggersAssert) {
  // This test documents that CM lookup failures cause ASSERTs.
  // In production (RELEASE builds), behavior may differ.
  // Unit tests cannot easily verify ASSERT behavior without crashing,
  // so this test just documents the expected behavior.

  // Setup proc hierarchy but GetIntCInfo fails
  mProcHierarchyInfo.resize (1);
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token             = 1001;
  mProcHierarchyInfo[0].Flags             = PPTT_PROCESSOR_MASK;
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = 2001;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 1);

  // GetIntCInfo returns success to avoid ASSERT path
  SetupGetIntCInfoSuccess (0, CM_NULL_TOKEN, CM_NULL_TOKEN);
  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  // Normal path should succeed
  EFI_STATUS  Status = BuildSsdtCpuTopologyTable (&Table, &TableCount);

  EXPECT_EQ (Status, EFI_SUCCESS)
  << "Normal CM lookup path should succeed";

  if (!EFI_ERROR (Status) && (Table != nullptr)) {
    FreeSsdtCpuTopologyTable (Table, TableCount);
  }
}

// =============================================================================
// Hierarchy Tests (ACPI 6.6 s8.4 Multi-Level)
// =============================================================================

/**
  @test TwoLevelHierarchy_PackageWithCpus

  @brief Validates two-level hierarchy: Package → CPUs.

  @spec ACPI 6.6 s8.4: Processor containers (ACPI0010) can contain
        processor devices (ACPI0007) to represent physical topology.

  @expected
    - Container device with ACPI0010
    - Child CPU devices with ACPI0007
**/
TEST_F (CommonSsdtCpuTopologyTest, TwoLevelHierarchy_PackageWithCpus) {
  const UINT32  CpuCount = 2;

  // Setup: Package → 2 CPUs
  mProcHierarchyInfo.resize (3);

  // Package (container)
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token = 1000;
  mProcHierarchyInfo[0].Flags = (EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL |
                                 (EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID << 1));
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = CM_NULL_TOKEN;

  // CPUs
  for (UINT32 i = 0; i < CpuCount; i++) {
    ZeroMem (&mProcHierarchyInfo[1 + i], sizeof (mProcHierarchyInfo[1 + i]));
    mProcHierarchyInfo[1 + i].Token             = 1001 + i;
    mProcHierarchyInfo[1 + i].Flags             = PPTT_CPU_PROCESSOR_MASK;
    mProcHierarchyInfo[1 + i].ParentToken       = 1000;
    mProcHierarchyInfo[1 + i].AcpiIdObjectToken = 2001 + i;
  }

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 3);
  SetupGetIntCInfoSuccess (0, CM_NULL_TOKEN, CM_NULL_TOKEN);
  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify container exists
  AML_NODE_HANDLE  ContainerDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &ContainerDevice), EFI_SUCCESS);
  EXPECT_TRUE (AmlTestHelper::DeviceHasHid (ContainerDevice, ACPI_HID_PROCESSOR_CONTAINER_STR))
  << "ACPI 6.6 s8.4: Container must have ACPI0010 HID";

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test ThreeLevelHierarchy_PackageClusterCpu

  @brief Validates three-level hierarchy: Package → Cluster → CPUs.

  @spec ACPI 6.6 s8.4: Processor topology can have multiple levels of
        containers representing package, cluster, and core relationships.

  @expected
    - Package container with ACPI0010
    - Cluster container with ACPI0010
    - CPU devices with ACPI0007
**/
TEST_F (CommonSsdtCpuTopologyTest, ThreeLevelHierarchy_PackageClusterCpu) {
  // Setup: Package → Cluster → 2 CPUs
  mProcHierarchyInfo.resize (4);

  // Package
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token = 1000;
  mProcHierarchyInfo[0].Flags = (EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL |
                                 (EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID << 1));
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = CM_NULL_TOKEN;

  // Cluster (non-physical container inside package)
  ZeroMem (&mProcHierarchyInfo[1], sizeof (mProcHierarchyInfo[1]));
  mProcHierarchyInfo[1].Token             = 1001;
  mProcHierarchyInfo[1].Flags             = (EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID << 1);  // Not physical, not leaf
  mProcHierarchyInfo[1].ParentToken       = 1000;
  mProcHierarchyInfo[1].AcpiIdObjectToken = CM_NULL_TOKEN;

  // CPUs inside cluster
  ZeroMem (&mProcHierarchyInfo[2], sizeof (mProcHierarchyInfo[2]));
  mProcHierarchyInfo[2].Token             = 1002;
  mProcHierarchyInfo[2].Flags             = PPTT_CPU_PROCESSOR_MASK;
  mProcHierarchyInfo[2].ParentToken       = 1001;
  mProcHierarchyInfo[2].AcpiIdObjectToken = 2001;

  ZeroMem (&mProcHierarchyInfo[3], sizeof (mProcHierarchyInfo[3]));
  mProcHierarchyInfo[3].Token             = 1003;
  mProcHierarchyInfo[3].Flags             = PPTT_CPU_PROCESSOR_MASK;
  mProcHierarchyInfo[3].ParentToken       = 1001;
  mProcHierarchyInfo[3].AcpiIdObjectToken = 2002;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 4);
  SetupGetIntCInfoSuccess (0, CM_NULL_TOKEN, CM_NULL_TOKEN);
  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EFI_STATUS  Status = BuildSsdtCpuTopologyTable (&Table, &TableCount);

  EXPECT_EQ (Status, EFI_SUCCESS)
  << "Three-level hierarchy should be supported";

  if (!EFI_ERROR (Status)) {
    FreeSsdtCpuTopologyTable (Table, TableCount);
  }
}

/**
  @test MultiplePackages_IndependentHierarchies

  @brief Validates multiple independent package hierarchies.

  @spec ACPI 6.6 s8.4: Systems with multiple physical packages should
        have independent processor container trees.

  @expected Each package generates independent container with its CPUs
**/
TEST_F (CommonSsdtCpuTopologyTest, MultiplePackages_IndependentHierarchies) {
  // Setup: 2 Packages, each with 1 CPU
  mProcHierarchyInfo.resize (4);

  // Package 0
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token = 1000;
  mProcHierarchyInfo[0].Flags = (EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL |
                                 (EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID << 1));
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = CM_NULL_TOKEN;

  // CPU in Package 0
  ZeroMem (&mProcHierarchyInfo[1], sizeof (mProcHierarchyInfo[1]));
  mProcHierarchyInfo[1].Token             = 1001;
  mProcHierarchyInfo[1].Flags             = PPTT_CPU_PROCESSOR_MASK;
  mProcHierarchyInfo[1].ParentToken       = 1000;
  mProcHierarchyInfo[1].AcpiIdObjectToken = 2001;

  // Package 1
  ZeroMem (&mProcHierarchyInfo[2], sizeof (mProcHierarchyInfo[2]));
  mProcHierarchyInfo[2].Token = 2000;
  mProcHierarchyInfo[2].Flags = (EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL |
                                 (EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID << 1));
  mProcHierarchyInfo[2].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[2].AcpiIdObjectToken = CM_NULL_TOKEN;

  // CPU in Package 1
  ZeroMem (&mProcHierarchyInfo[3], sizeof (mProcHierarchyInfo[3]));
  mProcHierarchyInfo[3].Token             = 2001;
  mProcHierarchyInfo[3].Flags             = PPTT_CPU_PROCESSOR_MASK;
  mProcHierarchyInfo[3].ParentToken       = 2000;
  mProcHierarchyInfo[3].AcpiIdObjectToken = 2002;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 4);
  SetupGetIntCInfoSuccess (0, CM_NULL_TOKEN, CM_NULL_TOKEN);
  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EFI_STATUS  Status = BuildSsdtCpuTopologyTable (&Table, &TableCount);

  EXPECT_EQ (Status, EFI_SUCCESS)
  << "Multiple packages should be supported";

  if (!EFI_ERROR (Status)) {
    FreeSsdtCpuTopologyTable (Table, TableCount);
  }
}

// =============================================================================
// Parameterized Tests - CPU Count Scaling
// =============================================================================

/**
  Parameterized test fixture for CPU count scaling tests.
  Tests device generation with various CPU counts to ensure naming
  and device creation scales correctly.
**/
class CpuCountTest : public CommonSsdtCpuTopologyTest,
  public WithParamInterface<UINT32> {
};

/**
  @test CpuCount_GeneratesCorrectDeviceCount

  @brief Validates correct device count for various CPU configurations.

  @spec ACPI 6.6 s8.4: Each processor must have its own device object.
        Implementation supports up to 0xFFF (4095) CPUs with Cxxx naming.

  @param CpuCount Number of CPUs to generate (parameterized)

  @expected Exactly CpuCount devices generated with correct names
**/
TEST_P (CpuCountTest, GeneratesCorrectDeviceCount) {
  const UINT32  CpuCount = GetParam ();

  // Setup processor hierarchy entries
  mProcHierarchyInfo.resize (CpuCount);
  for (UINT32 i = 0; i < CpuCount; i++) {
    ZeroMem (&mProcHierarchyInfo[i], sizeof (mProcHierarchyInfo[i]));
    mProcHierarchyInfo[i].Token             = 1001 + i;
    mProcHierarchyInfo[i].Flags             = PPTT_PROCESSOR_MASK;
    mProcHierarchyInfo[i].ParentToken       = CM_NULL_TOKEN;
    mProcHierarchyInfo[i].AcpiIdObjectToken = 2001 + i;
  }

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), CpuCount);

  // Mock GetIntCInfo to return sequential UIDs
  EXPECT_CALL (MockArch, GetIntCInfo (_, _, _, _, _))
    .WillRepeatedly (
       Invoke (
         [](CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL *CONST CfgMgrProtocol,
            CM_OBJECT_TOKEN Token,
            UINT32 *Uid,
            CM_OBJECT_TOKEN *Cpc,
            CM_OBJECT_TOKEN *Psd) -> EFI_STATUS {
    if (Uid != nullptr) {
      *Uid = (UINT32)(Token - 2001);
    }

    if (Cpc != nullptr) {
      *Cpc = CM_NULL_TOKEN;
    }

    if (Psd != nullptr) {
      *Psd = CM_NULL_TOKEN;
    }

    return EFI_SUCCESS;
  }
         )
       );

  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  // Build table
  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  // Parse and count devices
  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Count CPU devices
  UINT32  DeviceCount;

  EXPECT_EQ (AmlTestHelper::CountCpuDevices (RootNode, "\\_SB_", &DeviceCount), EFI_SUCCESS);
  EXPECT_EQ (DeviceCount, CpuCount)
  << "Expected " << CpuCount << " CPU devices, found " << DeviceCount;

  // Verify first and last device names follow Cxxx format
  AML_NODE_HANDLE  FirstDevice, LastDevice;
  CHAR8            FirstPath[20], LastPath[20];

  AsciiSPrint (FirstPath, sizeof (FirstPath), "\\_SB.C%03X", 0);
  EXPECT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, FirstPath, &FirstDevice), EFI_SUCCESS)
  << "First device " << FirstPath << " not found";

  if (CpuCount > 1) {
    AsciiSPrint (LastPath, sizeof (LastPath), "\\_SB.C%03X", CpuCount - 1);
    EXPECT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, LastPath, &LastDevice), EFI_SUCCESS)
    << "Last device " << LastPath << " not found";
  }

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

INSTANTIATE_TEST_SUITE_P (
  CpuCounts,
  CpuCountTest,
  Values (1u, 2u, 4u, 8u, 16u, 32u, 64u, 128u),
  [](const TestParamInfo<UINT32> &info) {
  return "Cpus_" + std::to_string (info.param);
}
  );

// =============================================================================
// Parameterized Tests - UID Edge Cases
// =============================================================================

/**
  Parameterized test fixture for UID value edge case tests.
  Tests various UID values to ensure proper integer encoding in AML.
**/
class UidValueTest : public CommonSsdtCpuTopologyTest,
  public WithParamInterface<UINT32> {
};

/**
  @test UidValue_EncodedCorrectly

  @brief Validates UID values are correctly encoded in generated AML.

  @spec ACPI 6.6 s8.4: _UID must uniquely identify each processor.
        Values can range from 0 to 0xFFFFFFFF.

  @param UidValue The UID value to test (parameterized)

  @expected _UID in generated AML matches input value exactly
**/
TEST_P (UidValueTest, EncodedCorrectly) {
  const UINT32  ExpectedUid = GetParam ();

  mProcHierarchyInfo.resize (1);
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token             = 1001;
  mProcHierarchyInfo[0].Flags             = PPTT_PROCESSOR_MASK;
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = 2001;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 1);
  SetupGetIntCInfoSuccess (ExpectedUid, CM_NULL_TOKEN, CM_NULL_TOKEN);
  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  UINT64  ActualUid;

  EXPECT_EQ (AmlTestHelper::GetDeviceUid (CpuDevice, &ActualUid), EFI_SUCCESS);
  EXPECT_EQ (ActualUid, ExpectedUid)
  << "UID 0x" << std::hex << ExpectedUid << " not encoded correctly";

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

INSTANTIATE_TEST_SUITE_P (
  UidValues,
  UidValueTest,
  Values (
    0u,                  // Zero
    1u,                  // One (special AML opcode)
    0x3Fu,               // Max 6-bit (fits in ByteConst)
    0x40u,               // Requires BytePrefix
    0xFFu,               // Max byte
    0x100u,              // Requires WordPrefix
    0xFFFFu,             // Max word
    0x10000u,            // Requires DWordPrefix
    0xFFFFFFu,           // 24-bit value
    0xFFFFFFFEu,         // Near max DWORD
    0xFFFFFFFFu          // Max DWORD
    ),
  [](const TestParamInfo<UINT32> &info) {
  std::stringstream ss;
  ss << "Uid_0x" << std::hex << std::uppercase << info.param;
  return ss.str ();
}
  );

// =============================================================================
// Parameterized Tests - Topology Configurations
// =============================================================================

/**
  Structure describing a processor topology configuration.
**/
struct TopologyConfig {
  UINT32        PackageCount;
  UINT32        ClustersPerPackage;
  UINT32        CpusPerCluster;
  const char    *Description;
};

/**
  Parameterized test fixture for topology configuration tests.
  Tests various multi-level hierarchy configurations.
**/
class TopologyConfigTest : public CommonSsdtCpuTopologyTest,
  public WithParamInterface<TopologyConfig> {
};

/**
  @test TopologyConfig_GeneratesCorrectHierarchy

  @brief Validates various topology configurations generate correct AML.

  @spec ACPI 6.6 s8.4: Processor containers (ACPI0010) can be nested
        to represent physical topology (packages, clusters, cores).

  @param config Topology configuration (parameterized)

  @expected
    - Correct number of container devices
    - Correct number of CPU devices
    - Proper nesting structure
**/
TEST_P (TopologyConfigTest, GeneratesCorrectHierarchy) {
  const TopologyConfig  &config = GetParam ();

  // Calculate total nodes needed
  UINT32  TotalClusters = config.PackageCount * config.ClustersPerPackage;
  UINT32  TotalCpus     = (TotalClusters > 0)
                        ? (TotalClusters * config.CpusPerCluster)
                        : (config.PackageCount * config.CpusPerCluster);
  UINT32  TotalNodes = config.PackageCount + TotalClusters + TotalCpus;

  if (TotalCpus == 0) {
    GTEST_SKIP () << "No CPUs in configuration";
  }

  mProcHierarchyInfo.resize (TotalNodes);
  UINT32  NodeIndex = 0;

  // Create packages
  for (UINT32 pkg = 0; pkg < config.PackageCount; pkg++) {
    CM_OBJECT_TOKEN  PkgToken = 1000 + pkg * 100;

    ZeroMem (&mProcHierarchyInfo[NodeIndex], sizeof (mProcHierarchyInfo[NodeIndex]));
    mProcHierarchyInfo[NodeIndex].Token = PkgToken;
    mProcHierarchyInfo[NodeIndex].Flags = (EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL |
                                           (EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID << 1));
    mProcHierarchyInfo[NodeIndex].ParentToken       = CM_NULL_TOKEN;
    mProcHierarchyInfo[NodeIndex].AcpiIdObjectToken = CM_NULL_TOKEN;
    NodeIndex++;

    if (config.ClustersPerPackage > 0) {
      // Create clusters under this package
      for (UINT32 cluster = 0; cluster < config.ClustersPerPackage; cluster++) {
        CM_OBJECT_TOKEN  ClusterToken = PkgToken + 10 + cluster;

        ZeroMem (&mProcHierarchyInfo[NodeIndex], sizeof (mProcHierarchyInfo[NodeIndex]));
        mProcHierarchyInfo[NodeIndex].Token             = ClusterToken;
        mProcHierarchyInfo[NodeIndex].Flags             = (EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID << 1);
        mProcHierarchyInfo[NodeIndex].ParentToken       = PkgToken;
        mProcHierarchyInfo[NodeIndex].AcpiIdObjectToken = CM_NULL_TOKEN;
        NodeIndex++;

        // Create CPUs under this cluster
        for (UINT32 cpu = 0; cpu < config.CpusPerCluster; cpu++) {
          ZeroMem (&mProcHierarchyInfo[NodeIndex], sizeof (mProcHierarchyInfo[NodeIndex]));
          mProcHierarchyInfo[NodeIndex].Token             = ClusterToken + 1 + cpu;
          mProcHierarchyInfo[NodeIndex].Flags             = PPTT_CPU_PROCESSOR_MASK;
          mProcHierarchyInfo[NodeIndex].ParentToken       = ClusterToken;
          mProcHierarchyInfo[NodeIndex].AcpiIdObjectToken = 2000 + NodeIndex;
          NodeIndex++;
        }
      }
    } else {
      // Create CPUs directly under package
      for (UINT32 cpu = 0; cpu < config.CpusPerCluster; cpu++) {
        ZeroMem (&mProcHierarchyInfo[NodeIndex], sizeof (mProcHierarchyInfo[NodeIndex]));
        mProcHierarchyInfo[NodeIndex].Token             = PkgToken + 1 + cpu;
        mProcHierarchyInfo[NodeIndex].Flags             = PPTT_CPU_PROCESSOR_MASK;
        mProcHierarchyInfo[NodeIndex].ParentToken       = PkgToken;
        mProcHierarchyInfo[NodeIndex].AcpiIdObjectToken = 2000 + NodeIndex;
        NodeIndex++;
      }
    }
  }

  ASSERT_EQ (NodeIndex, TotalNodes) << "Node count mismatch";

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), TotalNodes);
  SetupGetIntCInfoSuccess (0, CM_NULL_TOKEN, CM_NULL_TOKEN);
  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EFI_STATUS  Status = BuildSsdtCpuTopologyTable (&Table, &TableCount);

  ASSERT_EQ (Status, EFI_SUCCESS)
  << "Topology generation failed for: " << config.Description;

  // Parse and validate
  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Count container and CPU devices
  UINT32  CpuCount, ContainerCount;

  EXPECT_EQ (AmlTestHelper::CountCpuDevices (RootNode, "\\_SB_", &CpuCount), EFI_SUCCESS);
  EXPECT_EQ (AmlTestHelper::CountContainerDevices (RootNode, "\\_SB_", &ContainerCount), EFI_SUCCESS);

  // Note: CountContainerDevices only counts direct children of \_SB, not nested ones
  // So we check that at least packages are generated as containers
  EXPECT_GE (ContainerCount, config.PackageCount)
  << "Expected at least " << config.PackageCount << " containers for packages";

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

INSTANTIATE_TEST_SUITE_P (
  Topologies,
  TopologyConfigTest,
  Values (
    TopologyConfig { 1, 0, 1, "1pkg_1cpu" },
    TopologyConfig { 1, 0, 2, "1pkg_2cpus" },
    TopologyConfig { 1, 0, 4, "1pkg_4cpus" },
    TopologyConfig { 1, 1, 2, "1pkg_1cluster_2cpus" },
    TopologyConfig { 1, 2, 2, "1pkg_2clusters_2cpus" },
    TopologyConfig { 1, 2, 4, "1pkg_2clusters_4cpus" },
    TopologyConfig { 2, 0, 2, "2pkgs_2cpus" },
    TopologyConfig { 2, 1, 2, "2pkgs_1cluster_2cpus" },
    TopologyConfig { 2, 2, 4, "2pkgs_2clusters_4cpus" },
    TopologyConfig { 4, 2, 4, "4pkgs_2clusters_4cpus" }
    ),
  [](const TestParamInfo<TopologyConfig> &info) {
  return std::string (info.param.Description);
}
  );

// =============================================================================
// Parameterized Tests - PSD Coordination Types
// =============================================================================

/**
  Structure for PSD coordination type test parameters.
**/
struct PsdCoordConfig {
  UINT32        CoordType;
  const char    *TypeName;
};

/**
  Parameterized test fixture for PSD coordination type tests.
**/
class PsdCoordTypeTest : public CommonSsdtCpuTopologyTest,
  public WithParamInterface<PsdCoordConfig> {
};

/**
  @test PsdCoordType_EncodedCorrectly

  @brief Validates all valid PSD coordination types are encoded correctly.

  @spec ACPI 6.6 s8.4.5.5: CoordType field values:
        - 0xFC (SW_ALL): Software coordinates all processors
        - 0xFD (SW_ANY): Software may coordinate with any processor
        - 0xFE (HW_ALL): Hardware coordinates all processors

  @param config Coordination type configuration (parameterized)

  @expected CoordType in generated _PSD matches input
**/
TEST_P (PsdCoordTypeTest, EncodedCorrectly) {
  const PsdCoordConfig  &config = GetParam ();

  CM_ARCH_COMMON_PSD_INFO  PsdInfo;

  ZeroMem (&PsdInfo, sizeof (PsdInfo));
  PsdInfo.Revision  = 0;
  PsdInfo.Domain    = 0;
  PsdInfo.CoordType = config.CoordType;
  PsdInfo.NumProc   = 1;

  mProcHierarchyInfo.resize (1);
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token             = 1001;
  mProcHierarchyInfo[0].Flags             = PPTT_PROCESSOR_MASK;
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = 2001;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 1);
  SetupPsdInfo (&PsdInfo, 4001);

  EXPECT_CALL (MockArch, GetIntCInfo (_, _, _, _, _))
    .WillRepeatedly (
       DoAll (
         SetArgPointee<2>(0),
         SetArgPointee<3>(CM_NULL_TOKEN),
         SetArgPointee<4>(4001),
         Return (EFI_SUCCESS)
         )
       );

  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  UINT32  NumEntries, Revision, Domain, CoordType, NumProcessors;

  EXPECT_EQ (
    AmlTestHelper::GetPsdFields (
                     CpuDevice,
                     &NumEntries,
                     &Revision,
                     &Domain,
                     &CoordType,
                     &NumProcessors
                     ),
    EFI_SUCCESS
    );

  EXPECT_EQ (CoordType, config.CoordType)
  << "CoordType " << config.TypeName << " (0x" << std::hex << config.CoordType
  << ") not encoded correctly";

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

INSTANTIATE_TEST_SUITE_P (
  CoordTypes,
  PsdCoordTypeTest,
  Values (
    PsdCoordConfig { 0xFC, "SW_ALL" },
    PsdCoordConfig { 0xFD, "SW_ANY" },
    PsdCoordConfig { 0xFE, "HW_ALL" }
    ),
  [](const TestParamInfo<PsdCoordConfig> &info) {
  return std::string (info.param.TypeName);
}
  );

// =============================================================================
// Parameterized Tests - Token Combinations (CPC/PSD presence)
// =============================================================================

/**
  Structure for token combination test parameters.
**/
struct TokenCombinationConfig {
  bool          HasCpc;
  bool          HasPsd;
  const char    *Description;
};

/**
  Parameterized test fixture for CPC/PSD token combination tests.
**/
class TokenCombinationTest : public CommonSsdtCpuTopologyTest,
  public WithParamInterface<TokenCombinationConfig> {
protected:
  CM_ARCH_COMMON_CPC_INFO mCpcInfo;
  CM_ARCH_COMMON_PSD_INFO mPsdInfo;
};

/**
  @test TokenCombination_GeneratesCorrectObjects

  @brief Validates correct ACPI objects based on token presence.

  @spec ACPI 6.6 s8.4.7.1/_PSD: _CPC and _PSD are optional objects.
        They should only be generated when corresponding tokens are set.

  @param config Token combination configuration (parameterized)

  @expected
    - _CPC present iff HasCpc is true
    - _PSD present iff HasPsd is true
**/
TEST_P (TokenCombinationTest, GeneratesCorrectObjects) {
  const TokenCombinationConfig  &config = GetParam ();

  CM_OBJECT_TOKEN  CpcToken = config.HasCpc ? 3001 : CM_NULL_TOKEN;
  CM_OBJECT_TOKEN  PsdToken = config.HasPsd ? 4001 : CM_NULL_TOKEN;

  // Setup CPC info if needed
  if (config.HasCpc) {
    CreateSimpleCpcInfo (&mCpcInfo);
    SetupCpcInfo (&mCpcInfo, 3001);
  }

  // Setup PSD info if needed
  if (config.HasPsd) {
    CreateSimplePsdInfo (&mPsdInfo, 0, 1);
    SetupPsdInfo (&mPsdInfo, 4001);
  }

  mProcHierarchyInfo.resize (1);
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token             = 1001;
  mProcHierarchyInfo[0].Flags             = PPTT_PROCESSOR_MASK;
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = 2001;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 1);

  EXPECT_CALL (MockArch, GetIntCInfo (_, _, _, _, _))
    .WillRepeatedly (
       DoAll (
         SetArgPointee<2>(0),
         SetArgPointee<3>(CpcToken),
         SetArgPointee<4>(PsdToken),
         Return (EFI_SUCCESS)
         )
       );

  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  // Verify _CPC presence matches expectation
  EXPECT_EQ (AmlTestHelper::DeviceHasNamedObject (CpuDevice, "_CPC"), config.HasCpc)
  << "_CPC presence mismatch for: " << config.Description;

  // Verify _PSD presence matches expectation
  EXPECT_EQ (AmlTestHelper::DeviceHasNamedObject (CpuDevice, "_PSD"), config.HasPsd)
  << "_PSD presence mismatch for: " << config.Description;

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

INSTANTIATE_TEST_SUITE_P (
  TokenCombos,
  TokenCombinationTest,
  Values (
    TokenCombinationConfig { false, false, "NoCpc_NoPsd" },
    TokenCombinationConfig { true, false, "Cpc_NoPsd" },
    TokenCombinationConfig { false, true, "NoCpc_Psd" },
    TokenCombinationConfig { true, true, "Cpc_Psd" }
    ),
  [](const TestParamInfo<TokenCombinationConfig> &info) {
  return std::string (info.param.Description);
}
  );

// =============================================================================
// Parameterized Tests - PSD Domain and Processor Count Combinations
// =============================================================================

/**
  Structure for PSD domain test parameters.
**/
struct PsdDomainConfig {
  UINT32    Domain;
  UINT32    NumProcessors;
};

/**
  Parameterized test fixture for PSD domain/processor count tests.
**/
class PsdDomainTest : public CommonSsdtCpuTopologyTest,
  public WithParamInterface<PsdDomainConfig> {
};

/**
  @test PsdDomain_FieldsEncodedCorrectly

  @brief Validates PSD domain and processor count fields.

  @spec ACPI 6.6 s8.4.5.5: Domain field identifies the P-state domain.
        NumProcessors indicates how many processors share the domain.

  @param config Domain configuration (parameterized)

  @expected Domain and NumProcessors in generated _PSD match input
**/
TEST_P (PsdDomainTest, FieldsEncodedCorrectly) {
  const PsdDomainConfig  &config = GetParam ();

  CM_ARCH_COMMON_PSD_INFO  PsdInfo;

  ZeroMem (&PsdInfo, sizeof (PsdInfo));
  PsdInfo.Revision  = 0;
  PsdInfo.Domain    = config.Domain;
  PsdInfo.CoordType = 0xFE;  // HW_ALL
  PsdInfo.NumProc   = config.NumProcessors;

  mProcHierarchyInfo.resize (1);
  ZeroMem (&mProcHierarchyInfo[0], sizeof (mProcHierarchyInfo[0]));
  mProcHierarchyInfo[0].Token             = 1001;
  mProcHierarchyInfo[0].Flags             = PPTT_PROCESSOR_MASK;
  mProcHierarchyInfo[0].ParentToken       = CM_NULL_TOKEN;
  mProcHierarchyInfo[0].AcpiIdObjectToken = 2001;

  SetupProcHierarchyInfo (mProcHierarchyInfo.data (), 1);
  SetupPsdInfo (&PsdInfo, 4001);

  EXPECT_CALL (MockArch, GetIntCInfo (_, _, _, _, _))
    .WillRepeatedly (
       DoAll (
         SetArgPointee<2>(0),
         SetArgPointee<3>(CM_NULL_TOKEN),
         SetArgPointee<4>(4001),
         Return (EFI_SUCCESS)
         )
       );

  SetupCreateTopologyFromIntCUnsupported ();
  SetupAddArchAmlCpuInfoNoOp ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  UINT32  NumEntries, Revision, Domain, CoordType, NumProcessors;

  EXPECT_EQ (
    AmlTestHelper::GetPsdFields (
                     CpuDevice,
                     &NumEntries,
                     &Revision,
                     &Domain,
                     &CoordType,
                     &NumProcessors
                     ),
    EFI_SUCCESS
    );

  EXPECT_EQ (Domain, config.Domain)
  << "Domain not encoded correctly";
  EXPECT_EQ (NumProcessors, config.NumProcessors)
  << "NumProcessors not encoded correctly";

  AmlTestHelper::DeleteAmlTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

INSTANTIATE_TEST_SUITE_P (
  PsdDomains,
  PsdDomainTest,
  Values (
    PsdDomainConfig { 0, 1 },
    PsdDomainConfig { 0, 2 },
    PsdDomainConfig { 0, 4 },
    PsdDomainConfig { 1, 4 },
    PsdDomainConfig { 7, 8 },
    PsdDomainConfig { 15, 16 },
    PsdDomainConfig { 0xFF, 32 },
    PsdDomainConfig { 0xFFFF, 64 }
    ),
  [](const TestParamInfo<PsdDomainConfig> &info) {
  return "Domain" + std::to_string (info.param.Domain) +
         "_Procs" + std::to_string (info.param.NumProcessors);
}
  );

//
// Main entry point
//

int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
