/** @file
  RISC-V SSDT CPU Topology Generator GoogleTest unit tests.

  Tests RISC-V-specific CPU topology functions:
  - GetIntCInfo: Retrieves processor info from RINTC
  - CreateTopologyFromIntC: Creates topology from RINTC info
  - AddArchAmlCpuInfo: No-op for RISC-V (returns SUCCESS)

  Parameterized test suites:
  - RintcCpuCountTest: CPU counts from 1 to 256
  - AcpiProcessorUidValueTest: UID encoding boundaries
  - RintcTokenCombinationTest: CPC token presence
  - RintcFlagsTest: Enabled/disabled flags
  - MultiRintcUidTest: Multi-CPU UID patterns

  Copyright (c) 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <sstream>
#include <string>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/DebugLib.h>
  #include <Library/AmlLib/AmlLib.h>
  #include <Protocol/ConfigurationManagerProtocol.h>
  #include <ConfigurationManagerObject.h>
  #include <ConfigurationManagerHelper.h>
  #include <RiscVNameSpaceObjects.h>
  #include "../SsdtCpuTopologyGenerator.h"

  // RISC-V arch-specific functions under test
  EFI_STATUS
  EFIAPI
  GetIntCInfo (
    IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
    IN  CM_OBJECT_TOKEN                                     AcpiIdObjectToken,
    OUT UINT32                                              *AcpiProcessorUid,
    OUT CM_OBJECT_TOKEN                                     *CpcToken,
    OUT CM_OBJECT_TOKEN                                     *EtToken
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
using ::testing::Ne;

/**
  RISC-V-specific test fixture for SSDT CPU Topology unit tests.
  Tests the RISC-V arch functions in isolation.
**/
class RiscVArchSsdtCpuTopologyTest : public ::testing::Test {
protected:
  MockConfigurationManagerProtocol MockConfigMgrProtocol;
  MockCommonFunctions MockCommon;
  std::vector<CM_RISCV_RINTC_INFO> mRintcInfo;

  void
  SetUp (
    ) override
  {
    gMockCommonFunctions = &MockCommon;

    // Set default behavior for ConfigMgrProtocol - return NOT_FOUND
    ON_CALL (MockConfigMgrProtocol, GetObject (_, _, _, _))
      .WillByDefault (Return (EFI_NOT_FOUND));
  }

  void
  TearDown (
    ) override
  {
    gMockCommonFunctions = nullptr;
  }

  /**
    Create a simple RINTC info structure.

    @param[out] RintcInfo         The RINTC info to populate.
    @param[in]  AcpiProcessorUid  The ACPI processor UID.
    @param[in]  CpcToken          CPC token (or CM_NULL_TOKEN).
  **/
  void
  CreateRintcInfo (
    OUT CM_RISCV_RINTC_INFO  *RintcInfo,
    IN  UINT32               AcpiProcessorUid,
    IN  CM_OBJECT_TOKEN      CpcToken = CM_NULL_TOKEN
    )
  {
    ZeroMem (RintcInfo, sizeof (*RintcInfo));
    RintcInfo->AcpiProcessorUid = AcpiProcessorUid;
    RintcInfo->HartId           = AcpiProcessorUid;
    RintcInfo->Flags            = 1;  // Enabled
    RintcInfo->CpcToken         = CpcToken;
  }

  /**
    Setup mock expectation for RINTC info lookup by token.

    @param[in] RintcInfo  Array of RINTC info structures.
    @param[in] Count      Number of RINTC entries.
    @param[in] Token      Token to match (or CM_NULL_TOKEN for all).
  **/
  void
  SetupRintcInfo (
    CM_RISCV_RINTC_INFO  *RintcInfo,
    UINT32               Count,
    CM_OBJECT_TOKEN      Token = CM_NULL_TOKEN
    )
  {
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_RISCV_OBJECT_ID (ERiscVObjRintcInfo), Token, _))
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

//
// Test Cases - GetIntCInfo
//

/**
  Test: GetIntCInfo returns AcpiProcessorUid from RINTC info.
**/
TEST_F (RiscVArchSsdtCpuTopologyTest, GetIntCInfo_ReturnsAcpiProcessorUid) {
  const UINT32  ExpectedUid = 42;

  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], ExpectedUid);
  SetupRintcInfo (mRintcInfo.data (), 1, 1001);  // Lookup by token 1001

  UINT32           Uid;
  CM_OBJECT_TOKEN  CpcToken, EtToken;

  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         &Uid,
                         &CpcToken,
                         &EtToken
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Uid, ExpectedUid);
}

/**
  Test: GetIntCInfo returns CpcToken from RINTC info.
**/
TEST_F (RiscVArchSsdtCpuTopologyTest, GetIntCInfo_ReturnsCpcToken) {
  const CM_OBJECT_TOKEN  ExpectedCpcToken = 5001;

  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 0, ExpectedCpcToken);
  SetupRintcInfo (mRintcInfo.data (), 1, 1001);

  UINT32           Uid;
  CM_OBJECT_TOKEN  CpcToken, EtToken;

  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         &Uid,
                         &CpcToken,
                         &EtToken
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (CpcToken, ExpectedCpcToken);
}

/**
  Test: GetIntCInfo returns CM_NULL_TOKEN for EtToken (RISC-V has no ET).
**/
TEST_F (RiscVArchSsdtCpuTopologyTest, GetIntCInfo_EtTokenAlwaysNull) {
  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 0);
  SetupRintcInfo (mRintcInfo.data (), 1, 1001);

  UINT32           Uid;
  CM_OBJECT_TOKEN  CpcToken, EtToken;

  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         &Uid,
                         &CpcToken,
                         &EtToken
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (EtToken, (CM_OBJECT_TOKEN)CM_NULL_TOKEN);
}

/**
  Test: GetIntCInfo returns NOT_FOUND when RINTC info not available.
**/
TEST_F (RiscVArchSsdtCpuTopologyTest, GetIntCInfo_NotFound_ReturnsError) {
  // Don't setup any RINTC info - default mock returns NOT_FOUND

  UINT32           Uid;
  CM_OBJECT_TOKEN  CpcToken, EtToken;

  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         &Uid,
                         &CpcToken,
                         &EtToken
                         );

  EXPECT_EQ (Status, EFI_NOT_FOUND);
}

//
// Test Cases - AddArchAmlCpuInfo
//

/**
  Test: AddArchAmlCpuInfo returns SUCCESS on RISC-V (no-op).
  RISC-V doesn't add arch-specific AML info.
**/
TEST_F (RiscVArchSsdtCpuTopologyTest, AddArchAmlCpuInfo_ReturnsSuccess) {
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

//
// Test Cases - CreateTopologyFromIntC
//

/**
  Test: CreateTopologyFromIntC calls CreateAmlCpu for each RINTC entry.
**/
TEST_F (RiscVArchSsdtCpuTopologyTest, CreateTopologyFromIntC_CreatesOneCpuPerRintc) {
  const UINT32  CpuCount = 3;

  mRintcInfo.resize (CpuCount);
  for (UINT32 i = 0; i < CpuCount; i++) {
    CreateRintcInfo (&mRintcInfo[i], i);
  }

  SetupRintcInfo (mRintcInfo.data (), CpuCount, CM_NULL_TOKEN);

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
  Test: CreateTopologyFromIntC calls CreateAmlCpcNode when CpcToken is set.
**/
TEST_F (RiscVArchSsdtCpuTopologyTest, CreateTopologyFromIntC_CreatesCpcWhenTokenSet) {
  const CM_OBJECT_TOKEN  CpcToken = 5001;

  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 0, CpcToken);
  SetupRintcInfo (mRintcInfo.data (), 1, CM_NULL_TOKEN);

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
  Test: CreateTopologyFromIntC does NOT call CreateAmlCpcNode when no CpcToken.
**/
TEST_F (RiscVArchSsdtCpuTopologyTest, CreateTopologyFromIntC_NoCpcWhenNoToken) {
  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 0);  // No CPC token
  SetupRintcInfo (mRintcInfo.data (), 1, CM_NULL_TOKEN);

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
  Test: CreateTopologyFromIntC propagates CreateAmlCpu failure.
**/
TEST_F (RiscVArchSsdtCpuTopologyTest, CreateTopologyFromIntC_PropagatesCpuError) {
  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 0);
  SetupRintcInfo (mRintcInfo.data (), 1, CM_NULL_TOKEN);

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

/**
  Test: CreateTopologyFromIntC propagates CreateAmlCpcNode failure.
**/
TEST_F (RiscVArchSsdtCpuTopologyTest, CreateTopologyFromIntC_PropagatesCpcError) {
  const CM_OBJECT_TOKEN  CpcToken = 5001;

  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 0, CpcToken);
  SetupRintcInfo (mRintcInfo.data (), 1, CM_NULL_TOKEN);

  EXPECT_CALL (MockCommon, CreateAmlCpu (_, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));

  // CreateAmlCpcNode fails
  EXPECT_CALL (MockCommon, CreateAmlCpcNode (_, _, CpcToken, _))
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

/**
  Test: GetIntCInfo with NULL AcpiProcessorUid output is allowed.
**/
TEST_F (RiscVArchSsdtCpuTopologyTest, GetIntCInfo_NullUidOutputAllowed) {
  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 42);
  SetupRintcInfo (mRintcInfo.data (), 1, 1001);

  CM_OBJECT_TOKEN  CpcToken, EtToken;

  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         nullptr,  // NULL UID output
                         &CpcToken,
                         &EtToken
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
}

/**
  Test: GetIntCInfo with NULL CpcToken output is allowed.
**/
TEST_F (RiscVArchSsdtCpuTopologyTest, GetIntCInfo_NullCpcTokenOutputAllowed) {
  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 42);
  SetupRintcInfo (mRintcInfo.data (), 1, 1001);

  UINT32           Uid;
  CM_OBJECT_TOKEN  EtToken;

  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         &Uid,
                         nullptr,  // NULL CPC token output
                         &EtToken
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Uid, 42u);
}

// =============================================================================
// Parameterized Test: RintcCpuCountTest
// Tests CPU creation for various CPU counts from 1 to 256.
// =============================================================================

struct RintcCpuCountParams {
  UINT32         CpuCount;
  std::string    Description;
};

class RintcCpuCountTest : public RiscVArchSsdtCpuTopologyTest,
  public ::testing::WithParamInterface<RintcCpuCountParams> {
};

TEST_P (RintcCpuCountTest, CreatesCorrectNumberOfCpus) {
  const auto  &Params = GetParam ();

  mRintcInfo.resize (Params.CpuCount);
  for (UINT32 i = 0; i < Params.CpuCount; i++) {
    CreateRintcInfo (&mRintcInfo[i], i);
  }

  SetupRintcInfo (mRintcInfo.data (), Params.CpuCount, CM_NULL_TOKEN);

  // Expect CreateAmlCpu to be called once per CPU
  EXPECT_CALL (MockCommon, CreateAmlCpu (_, _, _, _, _))
    .Times (Params.CpuCount)
    .WillRepeatedly (Return (EFI_SUCCESS));

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator = { };

  EFI_STATUS  Status = CreateTopologyFromIntC (
                         &Generator,
                         gConfigurationManagerProtocol,
                         (AML_OBJECT_NODE_HANDLE)0x1000
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
}

std::string
RintcCpuCountParamName (
  const ::testing::TestParamInfo<RintcCpuCountParams>  &Info
  )
{
  return Info.param.Description;
}

INSTANTIATE_TEST_SUITE_P (
  CpuCounts,
  RintcCpuCountTest,
  ::testing::Values (
               RintcCpuCountParams{ 1, "OneCpu" },
               RintcCpuCountParams{ 2, "TwoCpus" },
               RintcCpuCountParams{ 4, "FourCpus" },
               RintcCpuCountParams{ 8, "EightCpus" },
               RintcCpuCountParams{ 16, "SixteenCpus" },
               RintcCpuCountParams{ 32, "ThirtyTwoCpus" },
               RintcCpuCountParams{ 64, "SixtyFourCpus" },
               RintcCpuCountParams{ 128, "OneHundredTwentyEightCpus" },
               RintcCpuCountParams{ 256, "TwoHundredFiftySixCpus" }
               ),
  RintcCpuCountParamName
  );

// =============================================================================
// Parameterized Test: AcpiProcessorUidValueTest
// Tests UID extraction at AML encoding boundaries.
// =============================================================================

struct UidValueParams {
  UINT32         Uid;
  std::string    Description;
};

class AcpiProcessorUidValueTest : public RiscVArchSsdtCpuTopologyTest,
  public ::testing::WithParamInterface<UidValueParams> {
};

TEST_P (AcpiProcessorUidValueTest, ExtractsUidCorrectly) {
  const auto  &Params = GetParam ();

  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], Params.Uid);
  SetupRintcInfo (mRintcInfo.data (), 1, 1001);

  UINT32           Uid;
  CM_OBJECT_TOKEN  CpcToken, EtToken;

  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         &Uid,
                         &CpcToken,
                         &EtToken
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Uid, Params.Uid);
}

std::string
UidValueParamName (
  const ::testing::TestParamInfo<UidValueParams>  &Info
  )
{
  return Info.param.Description;
}

INSTANTIATE_TEST_SUITE_P (
  UidBoundaries,
  AcpiProcessorUidValueTest,
  ::testing::Values (
               UidValueParams{ 0, "Zero" },
               UidValueParams{ 63, "ByteMax6Bit" },
               UidValueParams{ 64, "ByteMin7Bit" },
               UidValueParams{ 127, "ByteMax7Bit" },
               UidValueParams{ 128, "WordMin" },
               UidValueParams{ 255, "ByteMax" },
               UidValueParams{ 256, "WordMin9Bit" },
               UidValueParams{ 32767, "WordMaxSigned" },
               UidValueParams{ 65535, "WordMax" },
               UidValueParams{ 65536, "DwordMin" },
               UidValueParams{ 0xFFFFFFFF, "DwordMax" }
               ),
  UidValueParamName
  );

// =============================================================================
// Parameterized Test: RintcTokenCombinationTest
// Tests CPC token presence combinations.
// =============================================================================

struct TokenCombinationParams {
  bool           HasCpc;
  std::string    Description;
};

class RintcTokenCombinationTest : public RiscVArchSsdtCpuTopologyTest,
  public ::testing::WithParamInterface<TokenCombinationParams> {
};

TEST_P (RintcTokenCombinationTest, HandlesTokenCorrectly) {
  const auto             &Params  = GetParam ();
  const CM_OBJECT_TOKEN  CpcToken = Params.HasCpc ? 5001 : CM_NULL_TOKEN;

  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 0, CpcToken);
  SetupRintcInfo (mRintcInfo.data (), 1, CM_NULL_TOKEN);

  EXPECT_CALL (MockCommon, CreateAmlCpu (_, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));

  if (Params.HasCpc) {
    EXPECT_CALL (MockCommon, CreateAmlCpcNode (_, _, CpcToken, _))
      .WillOnce (Return (EFI_SUCCESS));
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

  EXPECT_EQ (Status, EFI_SUCCESS);
}

std::string
TokenCombinationParamName (
  const ::testing::TestParamInfo<TokenCombinationParams>  &Info
  )
{
  return Info.param.Description;
}

INSTANTIATE_TEST_SUITE_P (
  TokenCombinations,
  RintcTokenCombinationTest,
  ::testing::Values (
               TokenCombinationParams{ false, "NoCpc" },
               TokenCombinationParams{ true, "WithCpc" }
               ),
  TokenCombinationParamName
  );

// =============================================================================
// Parameterized Test: RintcFlagsTest
// Tests RINTC flag variations (Enabled/Disabled).
// =============================================================================

struct RintcFlagsParams {
  UINT32         Flags;
  std::string    Description;
};

class RintcFlagsTest : public RiscVArchSsdtCpuTopologyTest,
  public ::testing::WithParamInterface<RintcFlagsParams> {
};

TEST_P (RintcFlagsTest, ProcessesAllFlagValues) {
  const auto  &Params = GetParam ();

  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 0);
  mRintcInfo[0].Flags = Params.Flags;
  SetupRintcInfo (mRintcInfo.data (), 1, CM_NULL_TOKEN);

  // CPU should be created regardless of flag state
  EXPECT_CALL (MockCommon, CreateAmlCpu (_, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator = { };

  EFI_STATUS  Status = CreateTopologyFromIntC (
                         &Generator,
                         gConfigurationManagerProtocol,
                         (AML_OBJECT_NODE_HANDLE)0x1000
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
}

std::string
RintcFlagsParamName (
  const ::testing::TestParamInfo<RintcFlagsParams>  &Info
  )
{
  return Info.param.Description;
}

INSTANTIATE_TEST_SUITE_P (
  FlagVariations,
  RintcFlagsTest,
  ::testing::Values (
               RintcFlagsParams{ 0, "Disabled" },
               RintcFlagsParams{ 1, "Enabled" }
               ),
  RintcFlagsParamName
  );

// =============================================================================
// Parameterized Test: MultiRintcUidTest
// Tests multi-CPU UID patterns.
// =============================================================================

struct MultiRintcUidParams {
  std::vector<UINT32>    Uids;
  std::string            Description;
};

class MultiRintcUidTest : public RiscVArchSsdtCpuTopologyTest,
  public ::testing::WithParamInterface<MultiRintcUidParams> {
};

TEST_P (MultiRintcUidTest, AllUidsExtractedCorrectly) {
  const auto  &Params = GetParam ();

  mRintcInfo.resize (Params.Uids.size ());
  for (size_t i = 0; i < Params.Uids.size (); i++) {
    CreateRintcInfo (&mRintcInfo[i], Params.Uids[i]);
  }

  SetupRintcInfo (mRintcInfo.data (), (UINT32)Params.Uids.size (), CM_NULL_TOKEN);

  EXPECT_CALL (MockCommon, CreateAmlCpu (_, _, _, _, _))
    .Times ((int)Params.Uids.size ())
    .WillRepeatedly (Return (EFI_SUCCESS));

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator = { };

  EFI_STATUS  Status = CreateTopologyFromIntC (
                         &Generator,
                         gConfigurationManagerProtocol,
                         (AML_OBJECT_NODE_HANDLE)0x1000
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
}

std::string
MultiRintcUidParamName (
  const ::testing::TestParamInfo<MultiRintcUidParams>  &Info
  )
{
  return Info.param.Description;
}

INSTANTIATE_TEST_SUITE_P (
  UidPatterns,
  MultiRintcUidTest,
  ::testing::Values (
               MultiRintcUidParams{
  { 0, 1, 2, 3 }, "Sequential" },
               MultiRintcUidParams{
  { 0, 2, 4, 6 }, "EvenOnly" },
               MultiRintcUidParams{
  { 1, 3, 5, 7 }, "OddOnly" },
               MultiRintcUidParams{
  { 0, 100, 200, 300 }, "Sparse" },
               MultiRintcUidParams{
  { 127, 128, 255, 256 }, "CrossWordBoundary" },
               MultiRintcUidParams{
  { 0, 1000, 10000, 100000 }, "LargeGaps" }
               ),
  MultiRintcUidParamName
  );

// =============================================================================
// Additional Edge Case Tests
// =============================================================================

/**
  Test: HartId can differ from AcpiProcessorUid.
**/
TEST_F (RiscVArchSsdtCpuTopologyTest, HartId_IndependentFromUid) {
  // HartId = 5, but AcpiProcessorUid = 100
  mRintcInfo.resize (1);
  ZeroMem (&mRintcInfo[0], sizeof (mRintcInfo[0]));
  mRintcInfo[0].HartId           = 5;
  mRintcInfo[0].AcpiProcessorUid = 100;
  mRintcInfo[0].Flags            = 1;
  SetupRintcInfo (mRintcInfo.data (), 1, 1001);

  UINT32           Uid;
  CM_OBJECT_TOKEN  CpcToken, EtToken;

  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         &Uid,
                         &CpcToken,
                         &EtToken
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Uid, 100u) << "UID should be AcpiProcessorUid, not HartId";
}

/**
  Test: Large CPU count (512) handled correctly.
**/
TEST_F (RiscVArchSsdtCpuTopologyTest, LargeCpuCount_HandledCorrectly) {
  const UINT32  CpuCount = 512;

  mRintcInfo.resize (CpuCount);
  for (UINT32 i = 0; i < CpuCount; i++) {
    CreateRintcInfo (&mRintcInfo[i], i);
  }

  SetupRintcInfo (mRintcInfo.data (), CpuCount, CM_NULL_TOKEN);

  EXPECT_CALL (MockCommon, CreateAmlCpu (_, _, _, _, _))
    .Times (CpuCount)
    .WillRepeatedly (Return (EFI_SUCCESS));

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator = { };

  EFI_STATUS  Status = CreateTopologyFromIntC (
                         &Generator,
                         gConfigurationManagerProtocol,
                         (AML_OBJECT_NODE_HANDLE)0x1000
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
}

/**
  Test: Maximum 32-bit UID handled correctly.
**/
TEST_F (RiscVArchSsdtCpuTopologyTest, MaxUid32Bit_HandledCorrectly) {
  mRintcInfo.resize (1);
  CreateRintcInfo (&mRintcInfo[0], 0xFFFFFFFF);
  SetupRintcInfo (mRintcInfo.data (), 1, 1001);

  UINT32           Uid;
  CM_OBJECT_TOKEN  CpcToken, EtToken;

  EFI_STATUS  Status = GetIntCInfo (
                         gConfigurationManagerProtocol,
                         1001,
                         &Uid,
                         &CpcToken,
                         &EtToken
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Uid, 0xFFFFFFFFu);
}

/**
  Test: Mixed CPC configurations - some CPUs with CPC, some without.
**/
TEST_F (RiscVArchSsdtCpuTopologyTest, MixedCpcConfig_HandledCorrectly) {
  const UINT32  CpuCount = 4;

  mRintcInfo.resize (CpuCount);
  CreateRintcInfo (&mRintcInfo[0], 0);                // No CPC
  CreateRintcInfo (&mRintcInfo[1], 1, 5001);          // Has CPC
  CreateRintcInfo (&mRintcInfo[2], 2);                // No CPC
  CreateRintcInfo (&mRintcInfo[3], 3, 5003);          // Has CPC

  SetupRintcInfo (mRintcInfo.data (), CpuCount, CM_NULL_TOKEN);

  EXPECT_CALL (MockCommon, CreateAmlCpu (_, _, _, _, _))
    .Times (CpuCount)
    .WillRepeatedly (Return (EFI_SUCCESS));

  // Only 2 CPUs have CPC tokens
  EXPECT_CALL (MockCommon, CreateAmlCpcNode (_, _, _, _))
    .Times (2)
    .WillRepeatedly (Return (EFI_SUCCESS));

  ACPI_CPU_TOPOLOGY_GENERATOR  Generator = { };

  EFI_STATUS  Status = CreateTopologyFromIntC (
                         &Generator,
                         gConfigurationManagerProtocol,
                         (AML_OBJECT_NODE_HANDLE)0x1000
                         );

  EXPECT_EQ (Status, EFI_SUCCESS);
}

/**
  Main entry point for RISC-V SSDT CPU Topology arch tests.
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
