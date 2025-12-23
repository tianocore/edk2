/** @file
  ARM SSDT CPU Topology Generator GoogleTest unit tests.

  Tests ARM-specific CPU topology functions:
  - GetIntCInfo: Retrieves processor info from GICC
  - CreateTopologyFromIntC: Creates topology from GICC info
  - AddArchAmlCpuInfo: Adds ETE/ETM trace devices

  Copyright (c) 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.6 Specification - s8.4 Declaring Processors
    - ARM DEN0094 - CoreSight 1.2 Platform Design Document
**/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>
#include <sstream>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/DebugLib.h>
  #include <Library/AmlLib/AmlLib.h>
  #include <Protocol/ConfigurationManagerProtocol.h>
  #include <ConfigurationManagerObject.h>
  #include <ConfigurationManagerHelper.h>
  #include <ArmNameSpaceObjects.h>
  #include "../SsdtCpuTopologyGenerator.h"

  // ARM arch-specific functions under test
  EFI_STATUS
  EFIAPI
  GetIntCInfo (
    IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
    IN  CM_OBJECT_TOKEN                                     AcpiIdObjectToken,
    OUT UINT32                                              *AcpiProcessorUid,
    OUT CM_OBJECT_TOKEN                                     *CpcToken,
    OUT CM_OBJECT_TOKEN                                     *PsdToken
    );

  EFI_STATUS
  EFIAPI
  CreateTopologyFromIntC (
    IN        ACPI_CPU_TOPOLOGY_GENERATOR                   *Generator,
    IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
    IN        AML_OBJECT_NODE_HANDLE                        ScopeNode
    );

  EFI_STATUS
  EFIAPI
  AddArchAmlCpuInfo (
    IN  ACPI_CPU_TOPOLOGY_GENERATOR                         *Generator,
    IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
    IN  CM_OBJECT_TOKEN                                     AcpiIdObjectToken,
    IN  UINT32                                              CpuName,
    OUT  AML_OBJECT_NODE_HANDLE                             *CpuNode
    );
}

#include "MockCommonFunctions.h"
#include <GoogleTest/Protocol/MockConfigurationManagerProtocol.h>

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::Invoke;

/**
  ARM-specific test fixture for SSDT CPU Topology unit tests.
  Tests the ARM arch functions in isolation.
**/
class ArmArchSsdtCpuTopologyTest : public ::testing::Test {
protected:
  MockConfigurationManagerProtocol MockConfigMgrProtocol;
  MockCommonFunctions MockCommon;
  std::vector<CM_ARM_GICC_INFO> mGiccInfo;
  std::vector<CM_ARM_ET_INFO> mEtInfo;

  void
  SetUp (
    ) override
  {
    gMockCommonFunctions = &MockCommon;

    // Set default behavior for ConfigMgrProtocol - return NOT_FOUND
    ON_CALL (MockConfigMgrProtocol, GetObject (_, _, _, _))
      .WillByDefault (Return (EFI_NOT_FOUND));

    // Set default behavior for WriteAslName - populate the buffer to avoid ASAN errors
    // when GoogleMock logs uninteresting calls (it tries to print char* args as strings)
    ON_CALL (MockCommon, WriteAslName (_, _, _))
      .WillByDefault (
         Invoke (
           [](CHAR8 LeadChar, UINT32 Value, CHAR8 *AslName) -> EFI_STATUS {
      if (AslName == NULL) {
        return EFI_INVALID_PARAMETER;
      }

      AslName[0] = LeadChar;
      AslName[1] = '0' + ((Value >> 8) & 0xF);
      AslName[2] = '0' + ((Value >> 4) & 0xF);
      AslName[3] = '0' + (Value & 0xF);
      AslName[4] = '\0';
      return EFI_SUCCESS;
    }
           )
         );
  }

  void
  TearDown (
    ) override
  {
    gMockCommonFunctions = nullptr;
  }

  /**
    Create a simple GICC info structure.

    @param[out] GiccInfo          The GICC info to populate.
    @param[in]  AcpiProcessorUid  The ACPI processor UID.
    @param[in]  CpcToken          CPC token (or CM_NULL_TOKEN).
    @param[in]  PsdToken          PSD token (or CM_NULL_TOKEN).
    @param[in]  EtToken           ET token (or CM_NULL_TOKEN).
  **/
  void
  CreateGiccInfo (
    OUT CM_ARM_GICC_INFO  *GiccInfo,
    IN  UINT32            AcpiProcessorUid,
    IN  CM_OBJECT_TOKEN   CpcToken = CM_NULL_TOKEN,
    IN  CM_OBJECT_TOKEN   PsdToken = CM_NULL_TOKEN,
    IN  CM_OBJECT_TOKEN   EtToken  = CM_NULL_TOKEN
    )
  {
    ZeroMem (GiccInfo, sizeof (*GiccInfo));
    GiccInfo->AcpiProcessorUid   = AcpiProcessorUid;
    GiccInfo->CPUInterfaceNumber = AcpiProcessorUid;
    GiccInfo->Flags              = 1;  // Enabled
    GiccInfo->MPIDR              = AcpiProcessorUid;
    GiccInfo->CpcToken           = CpcToken;
    GiccInfo->PsdToken           = PsdToken;
    GiccInfo->EtToken            = EtToken;
  }

  /**
    Setup mock expectation for GICC info lookup by token.

    @param[in] GiccInfo  Array of GICC info structures.
    @param[in] Count     Number of GICC entries.
    @param[in] Token     Token to match (or CM_NULL_TOKEN for all).
  **/
  void
  SetupGiccInfo (
    CM_ARM_GICC_INFO  *GiccInfo,
    UINT32            Count,
    CM_OBJECT_TOKEN   Token = CM_NULL_TOKEN
    )
  {
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARM_OBJECT_ID (EArmObjGicCInfo), Token, _))
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
    Setup mock expectation for ET info lookup by token.

    @param[in] EtInfo  ET info structure.
    @param[in] Token   Token to match.
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
// Test Cases - GetIntCInfo
// =============================================================================

/**
  @test GetIntCInfo_ReturnsAcpiProcessorUid

  @brief Validates AcpiProcessorUid extraction from GICC info.

  @spec ACPI 6.6 s5.2.12.14: GICC structure contains AcpiProcessorUid field
        which correlates to the _UID of the processor device.

  @expected AcpiProcessorUid from GICC returned via output parameter
**/
TEST_F (ArmArchSsdtCpuTopologyTest, GetIntCInfo_ReturnsAcpiProcessorUid) {
  const UINT32  ExpectedUid = 42;

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], ExpectedUid);
  SetupGiccInfo (mGiccInfo.data (), 1, 1001);  // Lookup by token 1001

  UINT32           Uid;
  CM_OBJECT_TOKEN  CpcToken, PsdToken;

  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         &Uid,
                         &CpcToken,
                         &PsdToken
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Uid, ExpectedUid);
}

/**
  @test GetIntCInfo_ReturnsCpcToken

  @brief Validates CpcToken extraction from GICC info.

  @spec ARM Configuration Manager: GICC info includes optional CpcToken
        for _CPC (CPPC) support when available.

  @expected CpcToken from CM_ARM_GICC_INFO returned via output parameter
**/
TEST_F (ArmArchSsdtCpuTopologyTest, GetIntCInfo_ReturnsCpcToken) {
  const CM_OBJECT_TOKEN  ExpectedCpcToken = 5001;

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, ExpectedCpcToken);
  SetupGiccInfo (mGiccInfo.data (), 1, 1001);

  UINT32           Uid;
  CM_OBJECT_TOKEN  CpcToken, PsdToken;

  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         &Uid,
                         &CpcToken,
                         &PsdToken
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (CpcToken, ExpectedCpcToken);
}

/**
  @test GetIntCInfo_ReturnsPsdToken

  @brief Validates PsdToken extraction from GICC info.

  @spec ARM Configuration Manager: GICC info includes optional PsdToken
        for _PSD (P-state dependency) support when available.

  @expected PsdToken from CM_ARM_GICC_INFO returned via output parameter
**/
TEST_F (ArmArchSsdtCpuTopologyTest, GetIntCInfo_ReturnsPsdToken) {
  const CM_OBJECT_TOKEN  ExpectedPsdToken = 6001;

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, CM_NULL_TOKEN, ExpectedPsdToken);
  SetupGiccInfo (mGiccInfo.data (), 1, 1001);

  UINT32           Uid;
  CM_OBJECT_TOKEN  CpcToken, PsdToken;

  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         &Uid,
                         &CpcToken,
                         &PsdToken
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (PsdToken, ExpectedPsdToken);
}

/**
  @test GetIntCInfo_NotFound_ReturnsError

  @brief Validates error return when GICC info not found.

  @spec API Contract: Configuration Manager lookup failures must be
        propagated as errors to caller.

  @expected EFI_NOT_FOUND when GICC info unavailable
**/
TEST_F (ArmArchSsdtCpuTopologyTest, GetIntCInfo_NotFound_ReturnsError) {
  // Don't setup any GICC info - default mock returns NOT_FOUND

  UINT32           Uid;
  CM_OBJECT_TOKEN  CpcToken, PsdToken;

  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         &Uid,
                         &CpcToken,
                         &PsdToken
                         );

  EXPECT_EQ (Status, EFI_NOT_FOUND);
}

/**
  @test GetIntCInfo_NullOutputs_Succeeds

  @brief Validates NULL output parameters handled gracefully.

  @spec API Contract: Functions should handle NULL output parameters
        gracefully when caller doesn't need all values.

  @expected EFI_SUCCESS (not crash) with NULL outputs
**/
TEST_F (ArmArchSsdtCpuTopologyTest, GetIntCInfo_NullOutputs_Succeeds) {
  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 100, 200, 300);
  SetupGiccInfo (mGiccInfo.data (), 1, 1001);

  // Call with all NULL outputs - should succeed without crashing
  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         NULL,
                         NULL,
                         NULL
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
}

// =============================================================================
// Test Cases - CreateTopologyFromIntC
// =============================================================================

/**
  @test CreateTopologyFromIntC_CreatesOneCpuPerGicc

  @brief Validates CPU device created for each GICC entry.

  @spec ACPI 6.6 s8.4: Each processor must have its own Processor Device
        object. ARM discovers processors from GICC structures in MADT.

  @expected CreateAmlCpu called once per GICC entry
**/
TEST_F (ArmArchSsdtCpuTopologyTest, CreateTopologyFromIntC_CreatesOneCpuPerGicc) {
  const UINT32  CpuCount = 3;

  mGiccInfo.resize (CpuCount);
  for (UINT32 i = 0; i < CpuCount; i++) {
    CreateGiccInfo (&mGiccInfo[i], i);
  }

  SetupGiccInfo (mGiccInfo.data (), CpuCount, CM_NULL_TOKEN);

  // Expect CreateAmlCpu to be called once per CPU
  EXPECT_CALL (MockCommon, CreateAmlCpu (_, _, _, _, _))
    .Times (CpuCount)
    .WillRepeatedly (Return (EFI_SUCCESS));

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator = { };

  EFI_STATUS  Status = CreateTopologyFromIntC (
                         &Generator,
                         gConfigurationManagerProtocol,
                         (AML_OBJECT_NODE_HANDLE)0x1000  // Mock scope node
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
}

/**
  @test CreateTopologyFromIntC_CreatesCpcWhenTokenSet

  @brief Validates _CPC object generated when CpcToken is set.

  @spec ACPI 6.6 s8.4.7.1: _CPC provides CPPC information when available.

  @expected CreateAmlCpcNode called when CpcToken != CM_NULL_TOKEN
**/
TEST_F (ArmArchSsdtCpuTopologyTest, CreateTopologyFromIntC_CreatesCpcWhenTokenSet) {
  const CM_OBJECT_TOKEN  CpcToken = 5001;

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, CpcToken);
  SetupGiccInfo (mGiccInfo.data (), 1, CM_NULL_TOKEN);

  // Expect CreateAmlCpu to be called
  EXPECT_CALL (MockCommon, CreateAmlCpu (_, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));

  // Expect CreateAmlCpcNode to be called with the CpcToken
  EXPECT_CALL (MockCommon, CreateAmlCpcNode (_, _, CpcToken, _))
    .WillOnce (Return (EFI_SUCCESS));

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator = { };

  EFI_STATUS  Status = CreateTopologyFromIntC (
                         &Generator,
                         gConfigurationManagerProtocol,
                         (AML_OBJECT_NODE_HANDLE)0x1000
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
}

/**
  @test CreateTopologyFromIntC_NoCpcWhenNoToken

  @brief Validates _CPC not generated when CpcToken is null.

  @spec ACPI 6.6: _CPC is optional. When not configured, it should not be
        generated.

  @expected CreateAmlCpcNode NOT called when CpcToken is CM_NULL_TOKEN
**/
TEST_F (ArmArchSsdtCpuTopologyTest, CreateTopologyFromIntC_NoCpcWhenNoToken) {
  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0);  // No CPC token
  SetupGiccInfo (mGiccInfo.data (), 1, CM_NULL_TOKEN);

  EXPECT_CALL (MockCommon, CreateAmlCpu (_, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));

  // CreateAmlCpcNode should NOT be called
  EXPECT_CALL (MockCommon, CreateAmlCpcNode (_, _, _, _))
    .Times (0);

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator = { };

  EFI_STATUS  Status = CreateTopologyFromIntC (
                         &Generator,
                         gConfigurationManagerProtocol,
                         (AML_OBJECT_NODE_HANDLE)0x1000
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
}

/**
  @test CreateTopologyFromIntC_PropagatesCpuError

  @brief Validates error propagation when CreateAmlCpu fails.

  @spec API Contract: Errors from lower-level functions must be propagated.

  @expected ASSERT or error return when CreateAmlCpu fails
**/
TEST_F (ArmArchSsdtCpuTopologyTest, CreateTopologyFromIntC_PropagatesCpuError) {
  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0);
  SetupGiccInfo (mGiccInfo.data (), 1, CM_NULL_TOKEN);

  // CreateAmlCpu fails
  EXPECT_CALL (MockCommon, CreateAmlCpu (_, _, _, _, _))
    .WillOnce (Return (EFI_OUT_OF_RESOURCES));

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator = { };

  // Should ASSERT on error - use EXPECT_ANY_THROW
  EXPECT_ANY_THROW (
    CreateTopologyFromIntC (
      &Generator,
      gConfigurationManagerProtocol,
      (AML_OBJECT_NODE_HANDLE)0x1000
      )
    );
}

// =============================================================================
// Test Cases - AddArchAmlCpuInfo (ETE/ETM support)
// =============================================================================

// Note: ETE/ETM device creation with EtToken requires AML infrastructure and
// is tested in ArmSsdtCpuTopologyIntegrationGoogleTest.cpp (ArmEtTypeIntegrationTest).

/**
  @test AddArchAmlCpuInfo_NoEtToken_ReturnsSuccess

  @brief Validates SUCCESS when no EtToken (no trace device needed).

  @spec ARM Configuration Manager: EtToken is optional. When not configured,
        no trace device is created and function returns success.

  @expected EFI_SUCCESS with no trace device creation
**/
TEST_F (ArmArchSsdtCpuTopologyTest, AddArchAmlCpuInfo_NoEtToken_ReturnsSuccess) {
  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0);  // No ET token
  SetupGiccInfo (mGiccInfo.data (), 1, 1001);

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator   = { };
  AML_OBJECT_NODE_HANDLE       CpuNode     = (AML_OBJECT_NODE_HANDLE)0x2000;
  AML_OBJECT_NODE_HANDLE       *CpuNodePtr = &CpuNode;

  EFI_STATUS  Status = AddArchAmlCpuInfo (
                         &Generator,
                         gConfigurationManagerProtocol,
                         1001,
                         0,
                         CpuNodePtr
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
}

/**
  @test AddArchAmlCpuInfo_GiccNotFound_ReturnsError

  @brief Validates error when GICC lookup fails.

  @spec API Contract: Configuration Manager lookup failures must be propagated.

  @expected EFI_NOT_FOUND when GICC info unavailable
**/
TEST_F (ArmArchSsdtCpuTopologyTest, AddArchAmlCpuInfo_GiccNotFound_ReturnsError) {
  // Don't setup GICC info - lookup will fail

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator   = { };
  AML_OBJECT_NODE_HANDLE       CpuNode     = (AML_OBJECT_NODE_HANDLE)0x2000;
  AML_OBJECT_NODE_HANDLE       *CpuNodePtr = &CpuNode;

  EFI_STATUS  Status = AddArchAmlCpuInfo (
                         &Generator,
                         gConfigurationManagerProtocol,
                         1001,
                         0,
                         CpuNodePtr
                         );

  EXPECT_EQ (Status, EFI_NOT_FOUND);
}

// =============================================================================
// NEW: Specification-Driven ARM Tests (ACPI 6.6 + CoreSight)
// =============================================================================

/**
  @test EteDevice_UsesArmhc500Hid

  @brief Validates ETE (Embedded Trace Extension) device uses correct HID.

  @spec ARM CoreSight Architecture: ETE devices use _HID "ARMHC500" per
        ARM DEN0094 CoreSight Platform Design Document.

  @expected ETE device _HID = "ARMHC500"
**/
TEST_F (ArmArchSsdtCpuTopologyTest, EteDevice_UsesArmhc500Hid) {
  // This test verifies the expected HID value is defined correctly
  // Full validation requires integration tests with real AML parsing
  // Here we verify the specification constant is correct

  // From ARM CoreSight spec: ARMHC500 is the standard HID for ETE devices
  EXPECT_STREQ (ACPI_HID_ET_DEVICE, "ARMHC500")
  << "ARM CoreSight: ETE device _HID must be ARMHC500";
}

TEST_F (ArmArchSsdtCpuTopologyTest, GiccFlags_EnabledBit_AffectsGeneration) {
  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0);
  mGiccInfo[0].Flags = 0;  // Disabled
  SetupGiccInfo (mGiccInfo.data (), 1, CM_NULL_TOKEN);

  // Even disabled CPUs should be created (with _STA indicating status)
  EXPECT_CALL (MockCommon, CreateAmlCpu (_, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator = { };

  EFI_STATUS  Status = CreateTopologyFromIntC (
                         &Generator,
                         gConfigurationManagerProtocol,
                         (AML_OBJECT_NODE_HANDLE)0x1000
                         );

  EXPECT_EQ (Status, EFI_SUCCESS)
  << "ACPI 6.6 s5.2.12.14: Disabled GICCs should still generate devices";
}

/**
  @test AcpiProcessorUid_CorrelatesWithMadt

  @brief Validates _UID correlates with MADT GICC AcpiProcessorUid.

  @spec ACPI 6.6 s8.4: Processor Device _UID must correlate with the
        processor's ACPI Processor UID defined in MADT GICC structures.

  @expected GetIntCInfo returns AcpiProcessorUid from GICC for _UID
**/
TEST_F (ArmArchSsdtCpuTopologyTest, AcpiProcessorUid_CorrelatesWithMadt) {
  const UINT32  ExpectedUids[] = { 0x100, 0x101, 0x102, 0x103 };
  const UINT32  CpuCount       = sizeof (ExpectedUids) / sizeof (ExpectedUids[0]);

  mGiccInfo.resize (CpuCount);
  for (UINT32 i = 0; i < CpuCount; i++) {
    CreateGiccInfo (&mGiccInfo[i], ExpectedUids[i]);
  }

  // Setup individual token lookups
  for (UINT32 i = 0; i < CpuCount; i++) {
    CM_OBJECT_TOKEN  Token = 1001 + i;

    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARM_OBJECT_ID (EArmObjGicCInfo), Token, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARM_OBJECT_ID (EArmObjGicCInfo),
      sizeof (CM_ARM_GICC_INFO),
      &mGiccInfo[i],
      1
    }
             ),
           Return (EFI_SUCCESS)
           )
         );

    // Verify GetIntCInfo returns correct UID for each
    UINT32           Uid;
    CM_OBJECT_TOKEN  CpcToken, PsdToken;

    EFI_STATUS  Status = GetIntCInfo (
                           gConfigurationManagerProtocol,
                           Token,
                           &Uid,
                           &CpcToken,
                           &PsdToken
                           );

    EXPECT_EQ (Status, EFI_SUCCESS);
    EXPECT_EQ (Uid, ExpectedUids[i])
    << "ACPI 6.6 s8.4: _UID must match MADT GICC AcpiProcessorUid for CPU " << i;
  }
}

/**
  @test CpcAndPsd_IndependentTokens

  @brief Validates CPC and PSD tokens are independent.

  @spec ACPI 6.6: _CPC (s8.4.7.1) and _PSD (s8.4.5.5) are independent
        optional objects. A CPU may have CPC without PSD or vice versa.

  @expected GetIntCInfo returns each token independently
**/
TEST_F (ArmArchSsdtCpuTopologyTest, CpcAndPsd_IndependentTokens) {
  const CM_OBJECT_TOKEN  CpcOnly = 5001;
  const CM_OBJECT_TOKEN  PsdOnly = 6001;

  // Test 1: CPC only
  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, CpcOnly, CM_NULL_TOKEN);
  SetupGiccInfo (mGiccInfo.data (), 1, 1001);

  UINT32           Uid;
  CM_OBJECT_TOKEN  CpcToken, PsdToken;

  EXPECT_EQ (GetIntCInfo (gConfigurationManagerProtocol, 1001, &Uid, &CpcToken, &PsdToken), EFI_SUCCESS);
  EXPECT_EQ (CpcToken, CpcOnly) << "CPC token should be returned";
  EXPECT_EQ (PsdToken, (CM_OBJECT_TOKEN)CM_NULL_TOKEN) << "PSD token should be NULL";

  // Test 2: PSD only
  CreateGiccInfo (&mGiccInfo[0], 0, CM_NULL_TOKEN, PsdOnly);
  SetupGiccInfo (mGiccInfo.data (), 1, 1002);

  EXPECT_EQ (GetIntCInfo (gConfigurationManagerProtocol, 1002, &Uid, &CpcToken, &PsdToken), EFI_SUCCESS);
  EXPECT_EQ (CpcToken, (CM_OBJECT_TOKEN)CM_NULL_TOKEN) << "CPC token should be NULL";
  EXPECT_EQ (PsdToken, PsdOnly) << "PSD token should be returned";
}

// =============================================================================
// Parameterized Tests - GiccCpuCountTest
// =============================================================================

/**
  Parameterized test fixture for GICC CPU count variations.

  Tests CreateTopologyFromIntC with varying numbers of GICC entries
  to validate correct CPU device generation.
**/
class GiccCpuCountTest : public ArmArchSsdtCpuTopologyTest,
  public ::testing::WithParamInterface<UINT32> {
};

/**
  @test GiccCpuCountTest_CreatesCorrectCpuCount

  @brief Validates correct number of CPUs created for each GICC count.

  @spec ACPI 6.6 s8.4: Each processor must have its own Processor Device
        object. ARM discovers processors from GICC structures in MADT.

  @expected CreateAmlCpu called exactly CpuCount times
**/
TEST_P (GiccCpuCountTest, CreatesCorrectCpuCount) {
  const UINT32  CpuCount = GetParam ();

  mGiccInfo.resize (CpuCount);
  for (UINT32 i = 0; i < CpuCount; i++) {
    CreateGiccInfo (&mGiccInfo[i], i);
  }

  SetupGiccInfo (mGiccInfo.data (), CpuCount, CM_NULL_TOKEN);

  // Expect CreateAmlCpu to be called once per CPU
  EXPECT_CALL (MockCommon, CreateAmlCpu (_, _, _, _, _))
    .Times (CpuCount)
    .WillRepeatedly (Return (EFI_SUCCESS));

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator = { };

  EFI_STATUS  Status = CreateTopologyFromIntC (
                         &Generator,
                         gConfigurationManagerProtocol,
                         (AML_OBJECT_NODE_HANDLE)0x1000
                         );

  EXPECT_EQ (Status, EFI_SUCCESS)
  << "CreateTopologyFromIntC should succeed for " << CpuCount << " CPUs";
}

/**
  Generate descriptive test name for GiccCpuCountTest.
**/
std::string
GiccCpuCountTestName (
  const ::testing::TestParamInfo<UINT32>  &info
  )
{
  std::ostringstream  oss;

  oss << info.param << "Cpus";
  return oss.str ();
}

INSTANTIATE_TEST_SUITE_P (
  CpuCounts,
  GiccCpuCountTest,
  ::testing::Values (1, 2, 4, 8, 16, 32, 64, 128, 256),
  GiccCpuCountTestName
  );

// =============================================================================
// Parameterized Tests - AcpiProcessorUidValueTest
// =============================================================================

/**
  Parameterized test fixture for ACPI Processor UID value variations.

  Tests GetIntCInfo UID extraction with values at AML encoding boundaries.
**/
class AcpiProcessorUidValueTest : public ArmArchSsdtCpuTopologyTest,
  public ::testing::WithParamInterface<UINT32> {
};

/**
  @test AcpiProcessorUidValueTest_ExtractsCorrectUid

  @brief Validates UID extraction from GICC at AML encoding boundaries.

  @spec ACPI 6.6 s5.2.12.14: GICC AcpiProcessorUid correlates with the
        _UID of the processor device. Values must be preserved exactly.

  @expected GetIntCInfo returns exact UID from GICC structure
**/
TEST_P (AcpiProcessorUidValueTest, ExtractsCorrectUid) {
  const UINT32  ExpectedUid = GetParam ();

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], ExpectedUid);
  SetupGiccInfo (mGiccInfo.data (), 1, 1001);

  UINT32           Uid;
  CM_OBJECT_TOKEN  CpcToken, PsdToken;

  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         &Uid,
                         &CpcToken,
                         &PsdToken
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Uid, ExpectedUid)
  << "GICC AcpiProcessorUid must be returned exactly";
}

/**
  Generate descriptive test name for AcpiProcessorUidValueTest.
**/
std::string
AcpiProcessorUidValueTestName (
  const ::testing::TestParamInfo<UINT32>  &info
  )
{
  std::ostringstream  oss;

  oss << "Uid" << info.param;
  return oss.str ();
}

INSTANTIATE_TEST_SUITE_P (
  UidBoundaries,
  AcpiProcessorUidValueTest,
  ::testing::Values (
               0u,         // Zero
               63u,        // Max 6-bit
               64u,        // Min 7-bit
               127u,       // Max 7-bit (ByteData boundary)
               128u,       // Min 8-bit signed overflow
               255u,       // Max 8-bit
               256u,       // Min 9-bit (WordData boundary)
               32767u,     // Max 15-bit signed
               65535u,     // Max 16-bit (WordData max)
               65536u,     // Min 17-bit (DWordData boundary)
               0xFFFFFFFFu // Max 32-bit
               ),
  AcpiProcessorUidValueTestName
  );

// =============================================================================
// Parameterized Tests - GiccTokenCombinationTest
// =============================================================================

/**
  Configuration for GICC token combination testing.
**/
struct GiccTokenConfig {
  bool          HasCpc;
  bool          HasPsd;
  bool          HasEt;
  const char    *Description;
};

/**
  Parameterized test fixture for GICC token combinations.

  Tests all combinations of CPC/PSD/ET tokens being present or absent.
**/
class GiccTokenCombinationTest : public ArmArchSsdtCpuTopologyTest,
  public ::testing::WithParamInterface<GiccTokenConfig> {
};

/**
  @test GiccTokenCombinationTest_ReturnsCorrectTokens

  @brief Validates correct token extraction for all combinations.

  @spec ARM Configuration Manager: GICC info includes optional CpcToken,
        PsdToken, and EtToken. Each is independent and should be
        correctly extracted regardless of other tokens.

  @expected GetIntCInfo returns correct token values for each combination
**/
TEST_P (GiccTokenCombinationTest, ReturnsCorrectTokens) {
  const GiccTokenConfig  &config = GetParam ();

  const CM_OBJECT_TOKEN  ExpectedCpcToken = config.HasCpc ? 5001 : CM_NULL_TOKEN;
  const CM_OBJECT_TOKEN  ExpectedPsdToken = config.HasPsd ? 6001 : CM_NULL_TOKEN;
  const CM_OBJECT_TOKEN  ExpectedEtToken  = config.HasEt ? 7001 : CM_NULL_TOKEN;

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 42, ExpectedCpcToken, ExpectedPsdToken, ExpectedEtToken);
  SetupGiccInfo (mGiccInfo.data (), 1, 1001);

  UINT32           Uid;
  CM_OBJECT_TOKEN  CpcToken, PsdToken;

  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         &Uid,
                         &CpcToken,
                         &PsdToken
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (CpcToken, ExpectedCpcToken)
  << "CpcToken mismatch for config: " << config.Description;
  EXPECT_EQ (PsdToken, ExpectedPsdToken)
  << "PsdToken mismatch for config: " << config.Description;
}

/**
  Generate descriptive test name for GiccTokenCombinationTest.
**/
std::string
GiccTokenCombinationTestName (
  const ::testing::TestParamInfo<GiccTokenConfig>  &info
  )
{
  return std::string (info.param.Description);
}

INSTANTIATE_TEST_SUITE_P (
  TokenCombinations,
  GiccTokenCombinationTest,
  ::testing::Values (
               GiccTokenConfig { false, false, false, "NoTokens" },
               GiccTokenConfig { true, false, false, "CpcOnly" },
               GiccTokenConfig { false, true, false, "PsdOnly" },
               GiccTokenConfig { false, false, true, "EtOnly" },
               GiccTokenConfig { true, true, false, "CpcAndPsd" },
               GiccTokenConfig { true, false, true, "CpcAndEt" },
               GiccTokenConfig { false, true, true, "PsdAndEt" },
               GiccTokenConfig { true, true, true, "AllTokens" }
               ),
  GiccTokenCombinationTestName
  );

// =============================================================================
// Parameterized Tests - EtTypeTest
// =============================================================================

// Note: ET type parameterized testing (ETE vs ETM device creation) requires
// AML infrastructure and is covered by ArmEtTypeIntegrationTest in
// ArmSsdtCpuTopologyIntegrationGoogleTest.cpp.

// =============================================================================
// Parameterized Tests - GiccFlagsTest
// =============================================================================

/**
  Configuration for GICC flags testing.

  @note GICC Flags (per ACPI 6.6 s5.2.12.14):
        Bit 0: Enabled - processor is usable
        Bit 1: Performance Interrupt Mode - edge triggered
        Bit 2: VGIC Maintenance Interrupt Mode - edge triggered
        Bit 3: Online Capable - can be enabled at runtime
**/
struct GiccFlagsConfig {
  UINT32        Flags;
  bool          ExpectCpuCreated;
  const char    *Description;
};

/**
  Parameterized test fixture for GICC flag variations.

  Tests CPU device creation behavior based on GICC flags.
**/
class GiccFlagsTest : public ArmArchSsdtCpuTopologyTest,
  public ::testing::WithParamInterface<GiccFlagsConfig> {
};

/**
  @test GiccFlagsTest_CreatesCpuBasedOnFlags

  @brief Validates CPU device creation based on GICC flags.

  @spec ACPI 6.6 s5.2.12.14: GICC Flags field indicates processor status.
        Bit 0 (Enabled) indicates if processor is usable.
        Bit 3 (Online Capable) indicates if can be enabled at runtime.
        Disabled but Online Capable CPUs should still have devices.

  @expected CPU devices created for all GICC entries (status via _STA)
**/
TEST_P (GiccFlagsTest, CreatesCpuBasedOnFlags) {
  const GiccFlagsConfig  &config = GetParam ();

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0);
  mGiccInfo[0].Flags = config.Flags;
  SetupGiccInfo (mGiccInfo.data (), 1, CM_NULL_TOKEN);

  if (config.ExpectCpuCreated) {
    EXPECT_CALL (MockCommon, CreateAmlCpu (_, _, _, _, _))
      .Times (1)
      .WillOnce (Return (EFI_SUCCESS));
  } else {
    EXPECT_CALL (MockCommon, CreateAmlCpu (_, _, _, _, _))
      .Times (0);
  }

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator = { };

  EFI_STATUS  Status = CreateTopologyFromIntC (
                         &Generator,
                         gConfigurationManagerProtocol,
                         (AML_OBJECT_NODE_HANDLE)0x1000
                         );

  EXPECT_EQ (Status, EFI_SUCCESS)
  << "CreateTopologyFromIntC should succeed for flags: " << config.Description;
}

/**
  Generate descriptive test name for GiccFlagsTest.
**/
std::string
GiccFlagsTestName (
  const ::testing::TestParamInfo<GiccFlagsConfig>  &info
  )
{
  return std::string (info.param.Description);
}

INSTANTIATE_TEST_SUITE_P (
  FlagVariations,
  GiccFlagsTest,
  ::testing::Values (
               // Bit 0: Enabled
               GiccFlagsConfig { 0x1, true, "Enabled" },
               // Bit 0 clear: Disabled
               GiccFlagsConfig { 0x0, true, "Disabled" },
               // Bit 3: Online Capable (but currently disabled)
               GiccFlagsConfig { 0x8, true, "OnlineCapable" },
               // Bit 0 + Bit 3: Enabled and Online Capable
               GiccFlagsConfig { 0x9, true, "EnabledAndOnlineCapable" }
               ),
  GiccFlagsTestName
  );

// =============================================================================
// Parameterized Tests - MultiGiccUidTest
// =============================================================================

/**
  Configuration for testing multiple GICCs with specific UIDs.
**/
struct MultiGiccUidConfig {
  std::vector<UINT32>    Uids;
  const char             *Description;
};

/**
  Parameterized test fixture for multiple GICC entries with specific UIDs.

  Validates that all UIDs in a multi-CPU system are correctly extracted.
**/
class MultiGiccUidTest : public ArmArchSsdtCpuTopologyTest,
  public ::testing::WithParamInterface<MultiGiccUidConfig> {
};

/**
  @test MultiGiccUidTest_ExtractsAllUids

  @brief Validates all UIDs correctly extracted from multiple GICC entries.

  @spec ACPI 6.6 s8.4: Each processor must have unique _UID correlating
        with MADT GICC AcpiProcessorUid.

  @expected Each GICC's AcpiProcessorUid correctly extracted
**/
TEST_P (MultiGiccUidTest, ExtractsAllUids) {
  const MultiGiccUidConfig  &config  = GetParam ();
  const UINT32              CpuCount = static_cast<UINT32>(config.Uids.size ());

  mGiccInfo.resize (CpuCount);
  for (UINT32 i = 0; i < CpuCount; i++) {
    CreateGiccInfo (&mGiccInfo[i], config.Uids[i]);
  }

  // Setup individual token lookups for each GICC
  for (UINT32 i = 0; i < CpuCount; i++) {
    CM_OBJECT_TOKEN  Token = 1001 + i;

    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARM_OBJECT_ID (EArmObjGicCInfo), Token, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARM_OBJECT_ID (EArmObjGicCInfo),
      sizeof (CM_ARM_GICC_INFO),
      &mGiccInfo[i],
      1
    }
             ),
           Return (EFI_SUCCESS)
           )
         );
  }

  // Verify each UID is extracted correctly
  for (UINT32 i = 0; i < CpuCount; i++) {
    UINT32           Uid;
    CM_OBJECT_TOKEN  CpcToken, PsdToken;

    EFI_STATUS  Status = GetIntCInfo (
                           gConfigurationManagerProtocol,
                           1001 + i,
                           &Uid,
                           &CpcToken,
                           &PsdToken
                           );

    EXPECT_EQ (Status, EFI_SUCCESS);
    EXPECT_EQ (Uid, config.Uids[i])
    << "UID mismatch for CPU " << i << " in config: " << config.Description;
  }
}

/**
  Generate descriptive test name for MultiGiccUidTest.
**/
std::string
MultiGiccUidTestName (
  const ::testing::TestParamInfo<MultiGiccUidConfig>  &info
  )
{
  return std::string (info.param.Description);
}

INSTANTIATE_TEST_SUITE_P (
  MultiCpuUidPatterns,
  MultiGiccUidTest,
  ::testing::Values (
               MultiGiccUidConfig {
  { 0, 1, 2, 3 }, "Sequential_0_3" },
               MultiGiccUidConfig {
  { 0x100, 0x101, 0x102, 0x103 }, "Sequential_0x100" },
               MultiGiccUidConfig {
  { 0, 2, 4, 6 }, "EvenOnly" },
               MultiGiccUidConfig {
  { 1, 3, 5, 7 }, "OddOnly" },
               MultiGiccUidConfig {
  { 0, 0x100, 0x200, 0x300 }, "Sparse_0x100_stride" },
               MultiGiccUidConfig {
  { 0xFFFF, 0x10000, 0x10001 }, "CrossWordBoundary" }
               ),
  MultiGiccUidTestName
  );

// =============================================================================
// Additional Tests - ETM Unsupported, Mixed Configs, Error Propagation
// =============================================================================

/**
  @test EtmType_ExplicitlyReturnsUnsupported

  @brief Validates that ETM type explicitly returns EFI_UNSUPPORTED.

  @spec ARM CoreSight Architecture: Currently only ETE (Embedded Trace Extension)
        is supported. ETM (Embedded Trace Module) requires MMIO range description
        which is not currently implemented.

  @expected AddArchAmlCpuInfo returns EFI_UNSUPPORTED for ETM type
**/
TEST_F (ArmArchSsdtCpuTopologyTest, EtmType_ExplicitlyReturnsUnsupported) {
  const CM_OBJECT_TOKEN  EtToken = 7001;

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 42, CM_NULL_TOKEN, CM_NULL_TOKEN, EtToken);
  SetupGiccInfo (mGiccInfo.data (), 1, 1001);

  // Setup ET info with ETM type (not supported)
  mEtInfo.resize (1);
  mEtInfo[0].EtType = ArmEtTypeEtm;

  EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARM_OBJECT_ID (EArmObjEtInfo), EtToken, _))
    .WillOnce (
       DoAll (
         SetArgPointee<3>(
           CM_OBJ_DESCRIPTOR {
    CREATE_CM_ARM_OBJECT_ID (EArmObjEtInfo),
    sizeof (CM_ARM_ET_INFO),
    &mEtInfo[0],
    1
  }
           ),
         Return (EFI_SUCCESS)
         )
       );

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator   = { };
  AML_OBJECT_NODE_HANDLE       CpuNode     = (AML_OBJECT_NODE_HANDLE)0x2000;
  AML_OBJECT_NODE_HANDLE       *CpuNodePtr = &CpuNode;

  // ETM type should return EFI_UNSUPPORTED (may ASSERT in debug builds)
  EFI_STATUS  Status = EFI_SUCCESS;

  try {
    Status = AddArchAmlCpuInfo (
               &Generator,
               gConfigurationManagerProtocol,
               1001,
               0,
               CpuNodePtr
               );
    // If no exception, verify the expected return value
    EXPECT_EQ (Status, EFI_UNSUPPORTED)
    << "ETM type should return EFI_UNSUPPORTED";
  }
  catch (...) {
    // ASSERT from ASSERT_EFI_ERROR is acceptable
  }
}

/**
  @test CpuInterfaceNumber_IndependentFromAcpiProcessorUid

  @brief Validates CPUInterfaceNumber and AcpiProcessorUid are independent.

  @spec ACPI 6.6 s5.2.12.14: GICC structure has both CPUInterfaceNumber
        and AcpiProcessorUid fields. CPUInterfaceNumber is the GIC CPU
        interface number, while AcpiProcessorUid correlates with _UID.

  @expected GetIntCInfo returns AcpiProcessorUid (not CPUInterfaceNumber)
**/
TEST_F (ArmArchSsdtCpuTopologyTest, CpuInterfaceNumber_IndependentFromAcpiProcessorUid) {
  const UINT32  CpuInterfaceNumber = 100;
  const UINT32  AcpiProcessorUid   = 42;

  mGiccInfo.resize (1);
  ZeroMem (&mGiccInfo[0], sizeof (mGiccInfo[0]));
  mGiccInfo[0].CPUInterfaceNumber = CpuInterfaceNumber;
  mGiccInfo[0].AcpiProcessorUid   = AcpiProcessorUid;
  mGiccInfo[0].Flags              = 1;  // Enabled
  SetupGiccInfo (mGiccInfo.data (), 1, 1001);

  UINT32           Uid;
  CM_OBJECT_TOKEN  CpcToken, PsdToken;

  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         &Uid,
                         &CpcToken,
                         &PsdToken
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Uid, AcpiProcessorUid)
  << "GetIntCInfo should return AcpiProcessorUid, not CPUInterfaceNumber";
  EXPECT_NE (Uid, CpuInterfaceNumber)
  << "AcpiProcessorUid should differ from CPUInterfaceNumber in this test";
}

// =============================================================================
// Parameterized Tests - MixedCpuConfigTest
// =============================================================================

/**
  Configuration for mixed CPU capability testing.

  Represents a single CPU's token configuration.
**/
struct CpuTokenSetup {
  bool    HasCpc;
  bool    HasEt;
};

/**
  Configuration for testing multiple CPUs with different capabilities.
**/
struct MixedCpuConfig {
  std::vector<CpuTokenSetup>    CpuConfigs;
  const char                    *Description;
};

/**
  Parameterized test fixture for mixed CPU configurations.

  Tests that CreateTopologyFromIntC correctly handles a mix of
  CPUs with different CPC/ET token presence.
**/
class MixedCpuConfigTest : public ArmArchSsdtCpuTopologyTest,
  public ::testing::WithParamInterface<MixedCpuConfig> {
};

/**
  @test MixedCpuConfigTest_HandlesVariousConfigurations

  @brief Validates correct handling of mixed CPU capabilities.

  @spec ACPI 6.6 s8.4: Each processor can have different optional
        ACPI objects (_CPC, _PSD, etc.) based on platform capabilities.
        The generator must handle heterogeneous configurations.

  @expected CreateAmlCpu called for each CPU, CreateAmlCpcNode called
            only for CPUs with CpcToken
**/
TEST_P (MixedCpuConfigTest, HandlesVariousConfigurations) {
  const MixedCpuConfig  &config  = GetParam ();
  const UINT32          CpuCount = static_cast<UINT32>(config.CpuConfigs.size ());

  mGiccInfo.resize (CpuCount);
  mEtInfo.resize (CpuCount);

  UINT32  ExpectedCpcCalls = 0;

  for (UINT32 i = 0; i < CpuCount; i++) {
    CM_OBJECT_TOKEN  CpcToken = config.CpuConfigs[i].HasCpc ? (5001 + i) : CM_NULL_TOKEN;
    CM_OBJECT_TOKEN  EtToken  = config.CpuConfigs[i].HasEt ? (7001 + i) : CM_NULL_TOKEN;

    CreateGiccInfo (&mGiccInfo[i], i, CpcToken, CM_NULL_TOKEN, EtToken);

    if (config.CpuConfigs[i].HasCpc) {
      ExpectedCpcCalls++;
    }

    // Setup ET info for CPUs that have it (use ETE type for success)
    if (config.CpuConfigs[i].HasEt) {
      mEtInfo[i].EtType = ArmEtTypeEte;
    }
  }

  SetupGiccInfo (mGiccInfo.data (), CpuCount, CM_NULL_TOKEN);

  // Expect CreateAmlCpu called for each CPU
  EXPECT_CALL (MockCommon, CreateAmlCpu (_, _, _, _, _))
    .Times (CpuCount)
    .WillRepeatedly (Return (EFI_SUCCESS));

  // Expect CreateAmlCpcNode called only for CPUs with CpcToken
  if (ExpectedCpcCalls > 0) {
    EXPECT_CALL (MockCommon, CreateAmlCpcNode (_, _, _, _))
      .Times (ExpectedCpcCalls)
      .WillRepeatedly (Return (EFI_SUCCESS));
  } else {
    EXPECT_CALL (MockCommon, CreateAmlCpcNode (_, _, _, _))
      .Times (0);
  }

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator = { };

  EFI_STATUS  Status = CreateTopologyFromIntC (
                         &Generator,
                         gConfigurationManagerProtocol,
                         (AML_OBJECT_NODE_HANDLE)0x1000
                         );

  EXPECT_EQ (Status, EFI_SUCCESS)
  << "CreateTopologyFromIntC should succeed for config: " << config.Description;
}

/**
  Generate descriptive test name for MixedCpuConfigTest.
**/
std::string
MixedCpuConfigTestName (
  const ::testing::TestParamInfo<MixedCpuConfig>  &info
  )
{
  return std::string (info.param.Description);
}

INSTANTIATE_TEST_SUITE_P (
  MixedConfigurations,
  MixedCpuConfigTest,
  ::testing::Values (
               // All CPUs have CPC
               MixedCpuConfig {
  {
    { true, false }, { true, false }, { true, false }, { true, false }
  },
  "AllCpc"
},
               // No CPUs have CPC
               MixedCpuConfig {
  {
    { false, false }, { false, false }, { false, false }, { false, false }
  },
  "NoCpc"
},
               // Alternating CPC (big.LITTLE style)
               MixedCpuConfig {
  {
    { true, false }, { false, false }, { true, false }, { false, false }
  },
  "AlternatingCpc"
},
               // First half has CPC
               MixedCpuConfig {
  {
    { true, false }, { true, false }, { false, false }, { false, false }
  },
  "FirstHalfCpc"
},
               // Single CPU with CPC among many
               MixedCpuConfig {
  {
    { false, false }, { false, false }, { true, false }, { false, false }
  },
  "SingleCpcAmongMany"
},
               // All but one have CPC
               MixedCpuConfig {
  {
    { true, false }, { true, false }, { true, false }, { false, false }
  },
  "AllButOneCpc"
}
               ),
  MixedCpuConfigTestName
  );

// =============================================================================
// Error Propagation Tests
// =============================================================================

/**
  @test CpcCreationError_PropagatesFromCreateTopologyFromIntC

  @brief Validates error propagation when CPC creation fails.

  @spec API Contract: Errors from CreateAmlCpcNode must be propagated
        to the caller of CreateTopologyFromIntC.

  @expected CreateTopologyFromIntC returns error or ASSERTs
**/
TEST_F (ArmArchSsdtCpuTopologyTest, CpcCreationError_PropagatesFromCreateTopologyFromIntC) {
  const CM_OBJECT_TOKEN  CpcToken = 5001;

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 0, CpcToken);
  SetupGiccInfo (mGiccInfo.data (), 1, CM_NULL_TOKEN);

  // CreateAmlCpu succeeds
  EXPECT_CALL (MockCommon, CreateAmlCpu (_, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));

  // CreateAmlCpcNode fails
  EXPECT_CALL (MockCommon, CreateAmlCpcNode (_, _, CpcToken, _))
    .WillOnce (Return (EFI_OUT_OF_RESOURCES));

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator = { };

  // Should ASSERT or return error
  EXPECT_ANY_THROW (
    CreateTopologyFromIntC (
      &Generator,
      gConfigurationManagerProtocol,
      (AML_OBJECT_NODE_HANDLE)0x1000
      )
    );
}

/**
  @test EtInfoNotFound_ReturnsError

  @brief Validates error when ET info lookup fails.

  @spec API Contract: Configuration Manager lookup failures must be propagated.

  @expected AddArchAmlCpuInfo returns error when ET info not found
**/
TEST_F (ArmArchSsdtCpuTopologyTest, EtInfoNotFound_ReturnsError) {
  const CM_OBJECT_TOKEN  EtToken = 7001;

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], 42, CM_NULL_TOKEN, CM_NULL_TOKEN, EtToken);
  SetupGiccInfo (mGiccInfo.data (), 1, 1001);

  // Don't setup ET info - it will return NOT_FOUND
  EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARM_OBJECT_ID (EArmObjEtInfo), EtToken, _))
    .WillOnce (Return (EFI_NOT_FOUND));

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator   = { };
  AML_OBJECT_NODE_HANDLE       CpuNode     = (AML_OBJECT_NODE_HANDLE)0x2000;
  AML_OBJECT_NODE_HANDLE       *CpuNodePtr = &CpuNode;

  // Should ASSERT or return error
  EXPECT_ANY_THROW (
    AddArchAmlCpuInfo (
      &Generator,
      gConfigurationManagerProtocol,
      1001,
      0,
      CpuNodePtr
      )
    );
}

// =============================================================================
// Large Index Tests
// =============================================================================

/**
  @test LargeCpuCount_HandledCorrectly

  @brief Validates large CPU count handling.

  @spec ACPI 6.6: Systems can have many processors. The generator must
        handle large numbers of CPUs without overflow in naming (Cxxx).

  @note CPU device names use format Cxxx where xxx is hex. This supports
        up to 4095 (0xFFF) CPUs per scope with 3-digit names.

  @expected CreateAmlCpu called for all CPUs without errors
**/
TEST_F (ArmArchSsdtCpuTopologyTest, LargeCpuCount_HandledCorrectly) {
  const UINT32  CpuCount = 512;  // Large but reasonable count

  mGiccInfo.resize (CpuCount);
  for (UINT32 i = 0; i < CpuCount; i++) {
    CreateGiccInfo (&mGiccInfo[i], i);
  }

  SetupGiccInfo (mGiccInfo.data (), CpuCount, CM_NULL_TOKEN);

  // Expect CreateAmlCpu called for each CPU
  EXPECT_CALL (MockCommon, CreateAmlCpu (_, _, _, _, _))
    .Times (CpuCount)
    .WillRepeatedly (Return (EFI_SUCCESS));

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator = { };

  EFI_STATUS  Status = CreateTopologyFromIntC (
                         &Generator,
                         gConfigurationManagerProtocol,
                         (AML_OBJECT_NODE_HANDLE)0x1000
                         );

  EXPECT_EQ (Status, EFI_SUCCESS)
  << "CreateTopologyFromIntC should handle " << CpuCount << " CPUs";
}

/**
  @test MaxUid32Bit_HandledCorrectly

  @brief Validates maximum 32-bit UID handling.

  @spec ACPI 6.6 s5.2.12.14: AcpiProcessorUid is a 32-bit field.
        The generator must handle the full range.

  @expected GetIntCInfo correctly extracts max 32-bit UID
**/
TEST_F (ArmArchSsdtCpuTopologyTest, MaxUid32Bit_HandledCorrectly) {
  const UINT32  MaxUid = 0xFFFFFFFF;

  mGiccInfo.resize (1);
  CreateGiccInfo (&mGiccInfo[0], MaxUid);
  SetupGiccInfo (mGiccInfo.data (), 1, 1001);

  UINT32           Uid;
  CM_OBJECT_TOKEN  CpcToken, PsdToken;

  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         &Uid,
                         &CpcToken,
                         &PsdToken
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Uid, MaxUid)
  << "GetIntCInfo should handle maximum 32-bit UID";
}

/**
  @test AllGiccFields_ExtractedCorrectly

  @brief Validates all relevant GICC fields are accessible.

  @spec ACPI 6.6 s5.2.12.14: GICC structure contains multiple fields.
        The generator should correctly access all relevant fields.

  @expected All token fields correctly extracted
**/
TEST_F (ArmArchSsdtCpuTopologyTest, AllGiccFields_ExtractedCorrectly) {
  const UINT32           ExpectedUid      = 123;
  const CM_OBJECT_TOKEN  ExpectedCpcToken = 5001;
  const CM_OBJECT_TOKEN  ExpectedPsdToken = 6001;
  const CM_OBJECT_TOKEN  ExpectedEtToken  = 7001;

  mGiccInfo.resize (1);
  ZeroMem (&mGiccInfo[0], sizeof (mGiccInfo[0]));
  mGiccInfo[0].AcpiProcessorUid   = ExpectedUid;
  mGiccInfo[0].CPUInterfaceNumber = 99;  // Different from UID
  mGiccInfo[0].Flags              = 0x9; // Enabled + Online Capable
  mGiccInfo[0].MPIDR              = 0x80000100;
  mGiccInfo[0].CpcToken           = ExpectedCpcToken;
  mGiccInfo[0].PsdToken           = ExpectedPsdToken;
  mGiccInfo[0].EtToken            = ExpectedEtToken;
  SetupGiccInfo (mGiccInfo.data (), 1, 1001);

  UINT32           Uid;
  CM_OBJECT_TOKEN  CpcToken, PsdToken;

  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         &Uid,
                         &CpcToken,
                         &PsdToken
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Uid, ExpectedUid);
  EXPECT_EQ (CpcToken, ExpectedCpcToken);
  EXPECT_EQ (PsdToken, ExpectedPsdToken);
}

/**
  Main entry point for ARM SSDT CPU Topology arch tests.
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
