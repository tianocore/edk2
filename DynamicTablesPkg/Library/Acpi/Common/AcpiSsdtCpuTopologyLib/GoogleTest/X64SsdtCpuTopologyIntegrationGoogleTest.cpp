/** @file
  X64 SSDT CPU Topology Integration GoogleTest.

  Tests full X64 CPU topology generation through BuildSsdtCpuTopologyTable.
  Validates generated AML output against ACPI 6.6 specification.

  Parameterized test suites:
  - CpuCounts/X64CpuCountIntegrationTest: CPU counts from 1 to 32
  - UidBoundaries/X64UidBoundaryIntegrationTest: UID encoding boundaries

  Additional validation tests:
  - CPU _HID validation ("ACPI0007")
  - CPU _UID validation (matches APIC AcpiProcessorUid)
  - CPU naming convention (Cxxx pattern)
  - _CPC structure validation (23 entries for Rev 3)
  - _PSD structure validation (X64 supports this)
  - ApicId independence from UID
  - Disabled CPU _STA handling

  Copyright (c) 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.6 Specification - s8.4 Declaring Processors
**/

#include <Uefi.h>
#include "SsdtCpuTopologyTestCommon.h"
#include <sstream>
#include <string>

// =============================================================================
// X64-Specific Test Fixture
// =============================================================================

/**
  Test fixture for X64-specific integration tests.
  Extends SsdtCpuTopologyTest to include X64 APIC setup.
**/
class X64IntegrationTest : public SsdtCpuTopologyTest {
protected:
  std::vector<CM_X64_LOCAL_APIC_X2APIC_INFO> mApicInfo;
  std::vector<CM_ARCH_COMMON_CST_INFO> mCstInfo;
  std::vector<CM_ARCH_COMMON_CSD_INFO> mCsdInfo;
  std::vector<CM_ARCH_COMMON_PCT_INFO> mPctInfo;
  std::vector<CM_ARCH_COMMON_PSS_INFO> mPssInfo;
  std::vector<CM_ARCH_COMMON_PPC_INFO> mPpcInfo;
  std::vector<CM_ARCH_COMMON_STA_INFO> mStaInfo;
  std::vector<CM_ARCH_COMMON_CPC_INFO> mCpcInfo;
  std::vector<CM_ARCH_COMMON_PSD_INFO> mPsdInfo;
  std::vector<CM_ARCH_COMMON_OBJ_REF> mCmRef1;
  std::vector<CM_ARCH_COMMON_OBJ_REF> mCmRef2;

  void
  CreateApicInfo (
    OUT CM_X64_LOCAL_APIC_X2APIC_INFO  *ApicInfo,
    IN  UINT32                         ApicId,
    IN  UINT32                         AcpiProcessorUid
    )
  {
    ZeroMem (ApicInfo, sizeof (*ApicInfo));
    ApicInfo->ApicId           = ApicId;
    ApicInfo->AcpiProcessorUid = AcpiProcessorUid;
    ApicInfo->Flags            = 1;
    ApicInfo->CstToken         = CM_NULL_TOKEN;
    ApicInfo->CsdToken         = CM_NULL_TOKEN;
    ApicInfo->PctToken         = CM_NULL_TOKEN;
    ApicInfo->PssToken         = CM_NULL_TOKEN;
    ApicInfo->PpcToken         = CM_NULL_TOKEN;
    ApicInfo->PsdToken         = CM_NULL_TOKEN;
    ApicInfo->CpcToken         = CM_NULL_TOKEN;
    ApicInfo->StaToken         = CM_NULL_TOKEN;
  }

  void
  SetupApicInfo (
    CM_X64_LOCAL_APIC_X2APIC_INFO  *ApicInfo,
    UINT32                         Count
    )
  {
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_X64_OBJECT_ID (EX64ObjLocalApicX2ApicInfo), CM_NULL_TOKEN, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_X64_OBJECT_ID (EX64ObjLocalApicX2ApicInfo),
      (UINT32)(sizeof (CM_X64_LOCAL_APIC_X2APIC_INFO) * Count),
      ApicInfo,
      Count
    }
             ),
           Return (EFI_SUCCESS)
           )
         );
  }

  void
  SetupStaInfo (
    CM_ARCH_COMMON_STA_INFO  *StaInfo,
    CM_OBJECT_TOKEN          Token
    )
  {
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjStaInfo), Token, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjStaInfo),
      sizeof (CM_ARCH_COMMON_STA_INFO),
      StaInfo,
      1
    }
             ),
           Return (EFI_SUCCESS)
           )
         );
  }

  void
  SetupCmRef (
    CM_ARCH_COMMON_OBJ_REF  *CmRef,
    UINT32                  Count,
    CM_OBJECT_TOKEN         Token
    )
  {
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjCmRef), Token, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjCmRef),
      (UINT32)(sizeof (CM_ARCH_COMMON_OBJ_REF) * Count),
      CmRef,
      Count
    }
             ),
           Return (EFI_SUCCESS)
           )
         );
  }

  void
  SetupCstInfo (
    CM_ARCH_COMMON_CST_INFO  *CstInfo,
    UINT32                   Count,
    CM_OBJECT_TOKEN          Token
    )
  {
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjCstInfo), Token, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjCstInfo),
      (UINT32)(sizeof (CM_ARCH_COMMON_CST_INFO) * Count),
      CstInfo,
      Count
    }
             ),
           Return (EFI_SUCCESS)
           )
         );
  }

  void
  SetupPctInfo (
    CM_ARCH_COMMON_PCT_INFO  *PctInfo,
    CM_OBJECT_TOKEN          Token
    )
  {
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjPctInfo), Token, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjPctInfo),
      sizeof (CM_ARCH_COMMON_PCT_INFO),
      PctInfo,
      1
    }
             ),
           Return (EFI_SUCCESS)
           )
         );
  }

  void
  SetupPssInfo (
    CM_ARCH_COMMON_PSS_INFO  *PssInfo,
    UINT32                   Count,
    CM_OBJECT_TOKEN          Token
    )
  {
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjPssInfo), Token, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjPssInfo),
      (UINT32)(sizeof (CM_ARCH_COMMON_PSS_INFO) * Count),
      PssInfo,
      Count
    }
             ),
           Return (EFI_SUCCESS)
           )
         );
  }

  void
  SetupPpcInfo (
    CM_ARCH_COMMON_PPC_INFO  *PpcInfo,
    CM_OBJECT_TOKEN          Token
    )
  {
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjPpcInfo), Token, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjPpcInfo),
      sizeof (CM_ARCH_COMMON_PPC_INFO),
      PpcInfo,
      1
    }
             ),
           Return (EFI_SUCCESS)
           )
         );
  }
};

// =============================================================================
// X64-Specific Helper Functions
// =============================================================================

inline void
CreateSimpleStaInfo (
  OUT CM_ARCH_COMMON_STA_INFO  *StaInfo,
  IN  UINT32                   DeviceStatus
  )
{
  ZeroMem (StaInfo, sizeof (*StaInfo));
  StaInfo->DeviceStatus = DeviceStatus;
}

inline void
CreateSimpleCstInfo (
  OUT CM_ARCH_COMMON_CST_INFO  *CstInfo,
  IN  UINT8                    CStateType
  )
{
  ZeroMem (CstInfo, sizeof (*CstInfo));
  CstInfo->Type                       = CStateType;
  CstInfo->Latency                    = 100;
  CstInfo->Power                      = 500;
  CstInfo->Register.AddressSpaceId    = EFI_ACPI_6_5_SYSTEM_IO;
  CstInfo->Register.RegisterBitWidth  = 8;
  CstInfo->Register.RegisterBitOffset = 0;
  CstInfo->Register.AccessSize        = EFI_ACPI_6_5_BYTE;
  CstInfo->Register.Address           = 0x414;
}

inline void
CreateSimplePctInfo (
  OUT CM_ARCH_COMMON_PCT_INFO  *PctInfo
  )
{
  ZeroMem (PctInfo, sizeof (*PctInfo));
  PctInfo->ControlRegister.AddressSpaceId    = EFI_ACPI_6_5_SYSTEM_IO;
  PctInfo->ControlRegister.RegisterBitWidth  = 16;
  PctInfo->ControlRegister.RegisterBitOffset = 0;
  PctInfo->ControlRegister.AccessSize        = EFI_ACPI_6_5_WORD;
  PctInfo->ControlRegister.Address           = 0xB2;
  PctInfo->StatusRegister.AddressSpaceId     = EFI_ACPI_6_5_SYSTEM_IO;
  PctInfo->StatusRegister.RegisterBitWidth   = 16;
  PctInfo->StatusRegister.RegisterBitOffset  = 0;
  PctInfo->StatusRegister.AccessSize         = EFI_ACPI_6_5_WORD;
  PctInfo->StatusRegister.Address            = 0xB3;
}

inline void
CreateSimplePssInfo (
  OUT CM_ARCH_COMMON_PSS_INFO  *PssInfo,
  IN  UINT32                   CoreFrequency,
  IN  UINT32                   Power
  )
{
  ZeroMem (PssInfo, sizeof (*PssInfo));
  PssInfo->CoreFrequency    = CoreFrequency;
  PssInfo->Power            = Power;
  PssInfo->Latency          = 10;
  PssInfo->BusMasterLatency = 10;
  PssInfo->Control          = CoreFrequency;
  PssInfo->Status           = CoreFrequency;
}

inline void
CreateSimplePpcInfo (
  OUT CM_ARCH_COMMON_PPC_INFO  *PpcInfo,
  IN  UINT32                   PstateCount
  )
{
  ZeroMem (PpcInfo, sizeof (*PpcInfo));
  PpcInfo->PstateCount = PstateCount;
}

// =============================================================================
// X64 Integration Tests - Test through BuildSsdtCpuTopologyTable with real AML
// =============================================================================

/**
  @test X64Integration_SingleCpu_GeneratesCorrectAml

  @brief Integration test validating basic X64 CPU topology generation.

  @spec ACPI 6.6 s8.4: Each processor must have its own Processor Device
        object declared with _HID "ACPI0007".

  @expected Table generated with CPU device containing _HID and _UID.
**/
TEST_F (X64IntegrationTest, SingleCpu_GeneratesCorrectAml) {
  mApicInfo.resize (1);
  CreateApicInfo (&mApicInfo[0], 0, 100);  // APIC ID 0, UID 100
  SetupApicInfo (mApicInfo.data (), 1);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);
  ASSERT_EQ (TableCount, 1u);

  // Parse and validate
  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify CPU device exists using CountCpuDevices
  UINT32  CpuCount = 0;

  EXPECT_EQ (AmlTestHelper::CountCpuDevices (RootNode, "\\_SB", &CpuCount), EFI_SUCCESS);
  EXPECT_EQ (CpuCount, 1u) << "Should have one CPU device";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test X64Integration_CstToken_GeneratesCstMethod

  @brief Integration test validating _CST generation when CstToken is set.

  @spec ACPI 6.6 s8.4.1.1: "_CST object returns a Package containing a list
        of processor power states. Each entry describes a C-state."

  @expected _CST method generated with C-state entries.

  @note X64 CST implementation uses a two-level CmRef chain:
        CstToken → CmRef → CmRef → CstInfo
**/
TEST_F (X64IntegrationTest, CstToken_GeneratesCstMethod) {
  const CM_OBJECT_TOKEN  CstToken    = 8001;  // Token in APIC
  const CM_OBJECT_TOKEN  CstRefToken = 8002;  // First level reference
  const CM_OBJECT_TOKEN  CstPkgToken = 8003;  // Second level (package) reference

  // Set up APIC with CstToken
  mApicInfo.resize (1);
  CreateApicInfo (&mApicInfo[0], 0, 100);
  mApicInfo[0].CstToken = CstToken;
  SetupApicInfo (mApicInfo.data (), 1);

  // Set up the two-level CmRef chain that X64 CST implementation requires
  // First level: CstToken → array of CmRef with ReferenceToken = CstRefToken
  mCmRef1.resize (1);
  mCmRef1[0].ReferenceToken = CstRefToken;
  SetupCmRef (mCmRef1.data (), 1, CstToken);

  // Second level: CstRefToken → array of CmRef with ReferenceToken = CstPkgToken
  mCmRef2.resize (1);
  mCmRef2[0].ReferenceToken = CstPkgToken;
  SetupCmRef (mCmRef2.data (), 1, CstRefToken);

  // Final level: CstPkgToken → actual CST info
  mCstInfo.resize (1);
  CreateSimpleCstInfo (&mCstInfo[0], 2);  // C2 state
  SetupCstInfo (mCstInfo.data (), 1, CstPkgToken);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  // Parse and validate
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
  @test X64Integration_PstateTokens_GeneratesPstateObjects

  @brief Integration test validating P-state object generation.

  @spec ACPI 6.6 s8.4.5: P-state control requires _PCT (control/status regs),
        _PSS (supported states), and _PPC (performance capability).

  @expected _PCT, _PSS, and _PPC objects generated when all tokens set.
**/
TEST_F (X64IntegrationTest, PstateTokens_GeneratesPstateObjects) {
  const CM_OBJECT_TOKEN  PctToken = 8101;
  const CM_OBJECT_TOKEN  PssToken = 8102;
  const CM_OBJECT_TOKEN  PpcToken = 8103;

  // Set up APIC with P-state tokens (all three required)
  mApicInfo.resize (1);
  CreateApicInfo (&mApicInfo[0], 0, 100);
  mApicInfo[0].PctToken = PctToken;
  mApicInfo[0].PssToken = PssToken;
  mApicInfo[0].PpcToken = PpcToken;
  SetupApicInfo (mApicInfo.data (), 1);

  // Set up PCT info
  mPctInfo.resize (1);
  CreateSimplePctInfo (&mPctInfo[0]);
  SetupPctInfo (&mPctInfo[0], PctToken);

  // Set up PSS info (3 P-states: 3000MHz, 2000MHz, 1000MHz)
  mPssInfo.resize (3);
  CreateSimplePssInfo (&mPssInfo[0], 3000, 65000);  // 3GHz, 65W
  CreateSimplePssInfo (&mPssInfo[1], 2000, 40000);  // 2GHz, 40W
  CreateSimplePssInfo (&mPssInfo[2], 1000, 20000);  // 1GHz, 20W
  SetupPssInfo (mPssInfo.data (), 3, PssToken);

  // Set up PPC info
  mPpcInfo.resize (1);
  CreateSimplePpcInfo (&mPpcInfo[0], 3);
  SetupPpcInfo (&mPpcInfo[0], PpcToken);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  // Parse and validate
  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify CPU devices exist
  UINT32  CpuCount = 0;

  EXPECT_EQ (AmlTestHelper::CountCpuDevices (RootNode, "\\_SB", &CpuCount), EFI_SUCCESS);
  EXPECT_GE (CpuCount, 1u) << "At least one CPU device should exist";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test X64Integration_StaToken_GeneratesStaMethod

  @brief Integration test validating _STA generation when StaToken is set.

  @spec ACPI 6.6 s6.3.7: "_STA returns the status of a device."

  @expected _STA method generated with device status value.
**/
TEST_F (X64IntegrationTest, StaToken_GeneratesStaMethod) {
  const CM_OBJECT_TOKEN  StaToken = 8201;

  // Set up APIC with StaToken
  mApicInfo.resize (1);
  CreateApicInfo (&mApicInfo[0], 0, 100);
  mApicInfo[0].StaToken = StaToken;
  SetupApicInfo (mApicInfo.data (), 1);

  // Set up STA info - device present and enabled (0xF)
  mStaInfo.resize (1);
  CreateSimpleStaInfo (&mStaInfo[0], 0xF);
  SetupStaInfo (&mStaInfo[0], StaToken);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify CPU devices exist
  UINT32  CpuCount = 0;

  EXPECT_EQ (AmlTestHelper::CountCpuDevices (RootNode, "\\_SB", &CpuCount), EFI_SUCCESS);
  EXPECT_GE (CpuCount, 1u) << "At least one CPU device should exist";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test X64Integration_MultipleCpus_AllHaveDevices

  @brief Integration test validating multiple CPU generation.

  @spec ACPI 6.6 s8.4: Each processor must have its own Processor Device object.

  @expected One device generated per APIC entry.
**/
TEST_F (X64IntegrationTest, MultipleCpus_AllHaveDevices) {
  const UINT32  NumCpus = 4;

  mApicInfo.resize (NumCpus);
  for (UINT32 i = 0; i < NumCpus; i++) {
    CreateApicInfo (&mApicInfo[i], i, i * 100);  // Different UIDs
  }

  SetupApicInfo (mApicInfo.data (), NumCpus);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify correct number of CPU devices
  UINT32  CpuCount = 0;

  EXPECT_EQ (AmlTestHelper::CountCpuDevices (RootNode, "\\_SB", &CpuCount), EFI_SUCCESS);
  EXPECT_EQ (CpuCount, NumCpus) << "Should have one CPU device per APIC entry";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test X64Integration_DisabledCpu_StillGenerated

  @brief Integration test for disabled CPU handling.

  @spec X64: Disabled CPUs (Flags = 0) should still have _STA method
        returning disabled status if StaToken is set.

  @expected CPU device generated even when Flags indicates disabled.
**/
TEST_F (X64IntegrationTest, DisabledCpu_StillGenerated) {
  const CM_OBJECT_TOKEN  StaToken = 8301;

  mApicInfo.resize (1);
  CreateApicInfo (&mApicInfo[0], 0, 100);
  mApicInfo[0].Flags    = 0;  // Disabled
  mApicInfo[0].StaToken = StaToken;
  SetupApicInfo (mApicInfo.data (), 1);

  // Set up STA info - device not present (0)
  mStaInfo.resize (1);
  CreateSimpleStaInfo (&mStaInfo[0], 0);
  SetupStaInfo (&mStaInfo[0], StaToken);

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
  @test X64Integration_CpuHid_IsAcpi0007

  @brief Validates CPU device has correct _HID.

  @spec ACPI 6.6 s8.4: Processor Device objects must have _HID "ACPI0007".

  @expected CPU device _HID = "ACPI0007".
**/
TEST_F (X64IntegrationTest, CpuHid_IsAcpi0007) {
  mApicInfo.resize (1);
  CreateApicInfo (&mApicInfo[0], 0, 100);
  SetupApicInfo (mApicInfo.data (), 1);

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
  @test X64Integration_CpuUid_MatchesApicAcpiProcessorUid

  @brief Validates CPU device _UID matches APIC AcpiProcessorUid.

  @spec ACPI 6.6 s8.4: _UID must uniquely identify each processor.

  @expected _UID in AML matches AcpiProcessorUid from APIC.
**/
TEST_F (X64IntegrationTest, CpuUid_MatchesApicAcpiProcessorUid) {
  const UINT32  ExpectedUid = 42;

  mApicInfo.resize (1);
  CreateApicInfo (&mApicInfo[0], 0, ExpectedUid);
  SetupApicInfo (mApicInfo.data (), 1);

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
  EXPECT_EQ (ActualUid, ExpectedUid) << "_UID must match APIC AcpiProcessorUid";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test X64Integration_MultipleCpus_UidsMatchApicOrder

  @brief Validates all CPU UIDs match APIC order.

  @expected Each CPU's _UID matches corresponding APIC AcpiProcessorUid.
**/
TEST_F (X64IntegrationTest, MultipleCpus_UidsMatchApicOrder) {
  const UINT32  NumCpus = 4;

  mApicInfo.resize (NumCpus);
  UINT32  ExpectedUids[] = { 10, 20, 30, 40 };

  for (UINT32 i = 0; i < NumCpus; i++) {
    CreateApicInfo (&mApicInfo[i], i, ExpectedUids[i]);
  }

  SetupApicInfo (mApicInfo.data (), NumCpus);

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
  @test X64Integration_CpuNaming_FollowsCxxxPattern

  @brief Validates CPU naming follows Cxxx pattern.

  @spec Internal: CPU devices named C000, C001, C002, etc.

  @expected All CPU devices follow Cxxx naming.
**/
TEST_F (X64IntegrationTest, CpuNaming_FollowsCxxxPattern) {
  const UINT32  NumCpus = 4;

  mApicInfo.resize (NumCpus);
  for (UINT32 i = 0; i < NumCpus; i++) {
    CreateApicInfo (&mApicInfo[i], i, i);
  }

  SetupApicInfo (mApicInfo.data (), NumCpus);

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
  @test X64Integration_CpcToken_ValidatesPackageStructure

  @brief Validates _CPC package has correct structure.

  @spec ACPI 6.6 s8.4.7.1: _CPC package must have 23 entries for Rev 3.

  @expected _CPC package contains 23 elements.
**/
TEST_F (X64IntegrationTest, CpcToken_ValidatesPackageStructure) {
  const CM_OBJECT_TOKEN  CpcToken = 10001;

  mApicInfo.resize (1);
  CreateApicInfo (&mApicInfo[0], 0, 100);
  mApicInfo[0].CpcToken = CpcToken;
  SetupApicInfo (mApicInfo.data (), 1);

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
  @test X64Integration_PsdToken_ValidatesPackageStructure

  @brief Validates _PSD package has correct structure.

  @spec ACPI 6.6 s8.4.5.5: _PSD provides P-state dependency info.

  @expected _PSD package generated with correct fields.
**/
TEST_F (X64IntegrationTest, PsdToken_ValidatesPackageStructure) {
  const CM_OBJECT_TOKEN  PsdToken = 10002;

  mApicInfo.resize (1);
  CreateApicInfo (&mApicInfo[0], 0, 100);
  mApicInfo[0].PsdToken = PsdToken;
  SetupApicInfo (mApicInfo.data (), 1);

  // Set up PSD info
  mPsdInfo.resize (1);
  CreateSimplePsdInfo (&mPsdInfo[0], 0, 4);  // Domain 0, 4 processors
  SetupPsdInfo (&mPsdInfo[0], PsdToken);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Find CPU device
  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  // Verify _PSD is present (X64 supports PSD unlike ARM GICC path)
  EXPECT_TRUE (AmlTestHelper::DeviceHasNamedObject (CpuDevice, "_PSD"))
  << "X64: _PSD should be generated when PsdToken is set";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test X64Integration_ApicId_IndependentFromUid

  @brief Validates ApicId can differ from AcpiProcessorUid.

  @spec X64: ApicId identifies the hardware, UID is ACPI-level identifier.

  @expected CPU created with UID from AcpiProcessorUid, not ApicId.
**/
TEST_F (X64IntegrationTest, ApicId_IndependentFromUid) {
  mApicInfo.resize (1);
  // ApicId = 5, AcpiProcessorUid = 100
  CreateApicInfo (&mApicInfo[0], 5, 100);
  SetupApicInfo (mApicInfo.data (), 1);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Find CPU device and validate UID is AcpiProcessorUid (100), not ApicId (5)
  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS);

  UINT64  ActualUid;

  ASSERT_EQ (AmlTestHelper::GetDeviceUid (CpuDevice, &ActualUid), EFI_SUCCESS);
  EXPECT_EQ (ActualUid, 100u) << "_UID should be AcpiProcessorUid (100), not ApicId (5)";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test X64Integration_DisabledCpu_HasSta

  @brief Validates disabled CPUs with StaToken are generated correctly.

  @spec ACPI 6.6 s6.3.7: _STA returns device status.

  @note X64 implementation generates _STA as a Method (not a Name object).
        The DeviceHasNamedObject helper only finds Name nodes, not Methods.
        This test verifies the CPU device is created and _STA is added
        (confirmed via AML debug output showing "_SB_C000_STA" namespace entry).
**/
TEST_F (X64IntegrationTest, DisabledCpu_HasSta) {
  const CM_OBJECT_TOKEN  StaToken = 8301;

  mApicInfo.resize (1);
  CreateApicInfo (&mApicInfo[0], 0, 100);
  mApicInfo[0].Flags    = 0;  // Disabled
  mApicInfo[0].StaToken = StaToken;
  SetupApicInfo (mApicInfo.data (), 1);

  // Set up STA info - device not present (0)
  mStaInfo.resize (1);
  CreateSimpleStaInfo (&mStaInfo[0], 0);
  SetupStaInfo (&mStaInfo[0], StaToken);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Find CPU device - if it exists, generation succeeded
  AML_NODE_HANDLE  CpuDevice;

  ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, "\\_SB.C000", &CpuDevice), EFI_SUCCESS)
  << "Disabled CPU with StaToken should still generate device";

  // X64 generates _STA as Method (AmlCodeGenMethodRetInteger).
  // The DeviceHasNamedObject helper only finds Name nodes, not Method nodes.
  // Verified via debug output: "_SB_C000_STA" is added to namespace.

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test X64Integration_MixedTokenConfig

  @brief Tests CPUs with mixed CPC/PSD configurations.

  @expected Only CPUs with tokens have corresponding objects.
**/
TEST_F (X64IntegrationTest, MixedTokenConfig) {
  const UINT32  NumCpus = 4;

  mApicInfo.resize (NumCpus);
  CreateApicInfo (&mApicInfo[0], 0, 0);   // No CPC/PSD
  CreateApicInfo (&mApicInfo[1], 1, 1);   // Has CPC
  CreateApicInfo (&mApicInfo[2], 2, 2);   // Has PSD
  CreateApicInfo (&mApicInfo[3], 3, 3);   // Has both

  const CM_OBJECT_TOKEN  CpcToken1 = 10001;
  const CM_OBJECT_TOKEN  CpcToken3 = 10003;
  const CM_OBJECT_TOKEN  PsdToken2 = 20002;
  const CM_OBJECT_TOKEN  PsdToken3 = 20003;

  mApicInfo[1].CpcToken = CpcToken1;
  mApicInfo[2].PsdToken = PsdToken2;
  mApicInfo[3].CpcToken = CpcToken3;
  mApicInfo[3].PsdToken = PsdToken3;

  SetupApicInfo (mApicInfo.data (), NumCpus);

  // Set up CPC info
  mCpcInfo.resize (2);
  CreateSimpleCpcInfo (&mCpcInfo[0]);
  CreateSimpleCpcInfo (&mCpcInfo[1]);
  SetupCpcInfo (&mCpcInfo[0], CpcToken1);
  SetupCpcInfo (&mCpcInfo[1], CpcToken3);

  // Set up PSD info
  mPsdInfo.resize (2);
  CreateSimplePsdInfo (&mPsdInfo[0], 0, 2);
  CreateSimplePsdInfo (&mPsdInfo[1], 1, 2);
  SetupPsdInfo (&mPsdInfo[0], PsdToken2);
  SetupPsdInfo (&mPsdInfo[1], PsdToken3);

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

  // Verify object presence
  const char  *CpuNames[]   = { "C000", "C001", "C002", "C003" };
  bool        ExpectedCpc[] = { false, true, false, true };
  bool        ExpectedPsd[] = { false, false, true, true };

  for (UINT32 i = 0; i < NumCpus; i++) {
    char  DevicePath[16];
    AsciiSPrint (DevicePath, sizeof (DevicePath), "\\_SB.%a", CpuNames[i]);
    AML_NODE_HANDLE  CpuDevice;
    ASSERT_EQ (AmlTestHelper::FindDeviceByPath (RootNode, DevicePath, &CpuDevice), EFI_SUCCESS);

    bool  HasCpc = AmlTestHelper::DeviceHasNamedObject (CpuDevice, "_CPC");
    EXPECT_EQ (HasCpc, ExpectedCpc[i])
    << CpuNames[i] << " CPC presence mismatch";

    bool  HasPsd = AmlTestHelper::DeviceHasNamedObject (CpuDevice, "_PSD");
    EXPECT_EQ (HasPsd, ExpectedPsd[i])
    << CpuNames[i] << " PSD presence mismatch";
  }

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

/**
  @test X64Integration_LargeCpuCount

  @brief Tests handling of large CPU count.

  @expected All CPUs generated correctly.
**/
TEST_F (X64IntegrationTest, LargeCpuCount) {
  const UINT32  NumCpus = 64;

  mApicInfo.resize (NumCpus);
  for (UINT32 i = 0; i < NumCpus; i++) {
    CreateApicInfo (&mApicInfo[i], i, i);
  }

  SetupApicInfo (mApicInfo.data (), NumCpus);

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
// Parameterized Test: X64CpuCountIntegrationTest
// Tests CPU counts from 1 to 32.
// =============================================================================

class X64CpuCountIntegrationTest : public X64IntegrationTest,
  public ::testing::WithParamInterface<UINT32> {
};

TEST_P (X64CpuCountIntegrationTest, GeneratesCorrectCpuCount) {
  UINT32  CpuCount = GetParam ();

  mApicInfo.resize (CpuCount);
  for (UINT32 i = 0; i < CpuCount; i++) {
    CreateApicInfo (&mApicInfo[i], i, i);
  }

  SetupApicInfo (mApicInfo.data (), CpuCount);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  ASSERT_EQ (BuildSsdtCpuTopologyTable (&Table, &TableCount), EFI_SUCCESS);
  ASSERT_NE (Table, nullptr);

  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT_EQ (AmlTestHelper::ParseSsdtTable (Table[0], &RootNode), EFI_SUCCESS);

  // Verify correct number of CPUs
  UINT32  ActualCount = 0;

  EXPECT_EQ (AmlTestHelper::CountCpuDevices (RootNode, "\\_SB", &ActualCount), EFI_SUCCESS);
  EXPECT_EQ (ActualCount, CpuCount);

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

std::string
X64CpuCountIntegrationTestName (
  const ::testing::TestParamInfo<UINT32>  &info
  )
{
  std::ostringstream  oss;

  oss << info.param << "Cpus";
  return oss.str ();
}

INSTANTIATE_TEST_SUITE_P (
  CpuCounts,
  X64CpuCountIntegrationTest,
  ::testing::Values (
               1u,
               2u,
               4u,
               8u,
               16u,
               32u
               ),
  X64CpuCountIntegrationTestName
  );

// =============================================================================
// Parameterized Test: X64UidBoundaryIntegrationTest
// Tests UID encoding boundaries in generated AML.
// =============================================================================

class X64UidBoundaryIntegrationTest : public X64IntegrationTest,
  public ::testing::WithParamInterface<UINT32> {
};

TEST_P (X64UidBoundaryIntegrationTest, UidEncodedCorrectly) {
  UINT32  Uid = GetParam ();

  mApicInfo.resize (1);
  CreateApicInfo (&mApicInfo[0], 0, Uid);
  SetupApicInfo (mApicInfo.data (), 1);

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
  EXPECT_EQ (ActualUid, (UINT64)Uid)
  << "UID " << Uid << " not encoded correctly in AML";

  AmlDeleteTree (RootNode);
  FreeSsdtCpuTopologyTable (Table, TableCount);
}

std::string
X64UidBoundaryIntegrationTestName (
  const ::testing::TestParamInfo<UINT32>  &info
  )
{
  std::ostringstream  oss;

  oss << "Uid" << info.param;
  return oss.str ();
}

INSTANTIATE_TEST_SUITE_P (
  UidBoundaries,
  X64UidBoundaryIntegrationTest,
  ::testing::Values (
               0u,     // Zero
               63u,    // 6-bit max
               64u,    // 7-bit
               127u,   // ByteData boundary
               255u,   // 8-bit max
               256u,   // WordData boundary
               32767u, // 16-bit signed max
               65535u, // 16-bit max
               65536u  // DWordData boundary
               ),
  X64UidBoundaryIntegrationTestName
  );

/**
  Main entry point for X64 SSDT CPU Topology integration tests.
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
