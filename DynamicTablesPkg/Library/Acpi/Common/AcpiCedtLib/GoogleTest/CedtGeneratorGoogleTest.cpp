/** @file
  Unit tests for CEDT Generator

  Copyright (c) 2025, Google, Inc. All rights reserved. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/ConfigurationManagerProtocol.h>
  #include <Library/FunctionMockLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/DebugLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include <Library/TableHelperLib.h>
  #include <AcpiTableGenerator.h>
  #include <ConfigurationManagerObject.h>
  #include <ConfigurationManagerHelper.h>
  #include <StandardNameSpaceObjects.h>
  #include <IndustryStandard/Cxl.h>
  #include <IndustryStandard/Acpi64.h>
  #include "GoogleTest/Protocol/MockConfigurationManagerProtocol.h"

  EFI_STATUS
  EFIAPI
  AcpiCedtLibConstructor (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    );

  EFI_STATUS
  EFIAPI
  AcpiCedtLibDestructor (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    );

  // Global generator instance
  static ACPI_TABLE_GENERATOR  *gCedtGenerator = NULL;

  // C++ wrapper functions for C linkage functions
  EFI_STATUS
  RegisterAcpiTableGenerator (
    IN  CONST ACPI_TABLE_GENERATOR  *CONST  TableGenerator
    )
  {
    if (TableGenerator == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    gCedtGenerator = const_cast<ACPI_TABLE_GENERATOR *>(TableGenerator);
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

    // Clear the stored generator
    gCedtGenerator = NULL;
    return EFI_SUCCESS;
  }
}

using namespace testing;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::AtLeast;

#define WRAP_ACCESSOR(accessor) \
  [this] \
  (IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL *This, \
   IN  CONST CM_OBJECT_ID                         CmObjectId, \
   IN  CONST CM_OBJECT_TOKEN                      Token, \
   IN  OUT   CM_OBJ_DESCRIPTOR                    *CmObject \
  ) \
  { \
    return this->accessor(This, CmObjectId, Token, CmObject); \
  }

class CedtGeneratorTest : public ::testing::Test {
protected:
  MockConfigurationManagerProtocol MockConfigMgrProtocol;
  CM_STD_OBJ_CONFIGURATION_MANAGER_INFO CfgMgrInfo = { 0 };

  void
  ValidateChbs (
    EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE  *Expected,
    EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE  *Actual
    )
  {
    // Fields that should never change.
    EXPECT_EQ (
      Actual->Type,
      EFI_ACPI_6_4_CEDT_STRUCTURE_TYPE_CXL_HOST_BRIDGE_STRUCTURE
      );
    EXPECT_EQ (Actual->Reserved0, 0U);
    EXPECT_EQ (
      Actual->RecordLength,
      sizeof (EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE)
      );
    EXPECT_EQ (Actual->Reserved1, 0U);

    // Fields that can change.
    EXPECT_EQ (Actual->Uid, Expected->Uid);
    EXPECT_EQ (Actual->CxlVersion, Expected->CxlVersion);
    EXPECT_EQ (Actual->Base, Expected->Base);
    EXPECT_EQ (Actual->Length, Expected->Length);
  }

  void
  ValidateCfmws (
    CONST EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE  *Expected,
    EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE        *Actual,
    UINT32                                                     *ExpectedInterleaveTargets,
    UINT32                                                     ExpectedInterleaveTargetCount
    )
  {
    EXPECT_EQ (
      Actual->Type,
      EFI_ACPI_6_4_CEDT_STRUCTURE_TYPE_CXL_FIXED_MEMORY_WINDOW_STRUCTURE
      );
    EXPECT_EQ (Actual->Reserved0, 0U);
    EXPECT_EQ (Actual->Reserved1, 0U);
    EXPECT_EQ (Actual->Reserved2, 0U);

    EXPECT_EQ (Actual->RecordLength, Expected->RecordLength);
    EXPECT_EQ (Actual->BaseHpa, Expected->BaseHpa);
    EXPECT_EQ (Actual->WindowSize, Expected->WindowSize);
    EXPECT_EQ (
      Actual->EncodedNumberOfInterleaveWays,
      Expected->EncodedNumberOfInterleaveWays
      );
    EXPECT_EQ (Actual->InterleaveArithmetic, Expected->InterleaveArithmetic);
    EXPECT_EQ (
      Actual->HostBridgeInterleaveGranularity,
      Expected->HostBridgeInterleaveGranularity
      );
    EXPECT_EQ (Actual->WindowRestrictions, Expected->WindowRestrictions);
    EXPECT_EQ (Actual->QtgId, Expected->QtgId);

    for (UINT32 Index = 0; Index < ExpectedInterleaveTargetCount; Index++) {
      EXPECT_EQ (Actual->InterleaveTargetList[Index], ExpectedInterleaveTargets[Index]);
    }
  }

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

    // Set up call to get all host bridges.
    EXPECT_CALL (
      MockConfigMgrProtocol,
      GetObject (
        _,
        CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjCxlHostBridgeInfo),
        CM_NULL_TOKEN,
        _
        )
      )
      .WillRepeatedly (
         Invoke (WRAP_ACCESSOR (GetAllHostBridges))
         );

    // Set up call to get a single host bridge.
    EXPECT_CALL (
      MockConfigMgrProtocol,
      GetObject (
        _,
        CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjCxlHostBridgeInfo),
        Ne ((UINTN)CM_NULL_TOKEN),
        _
        )
      )
      .WillRepeatedly (
         Invoke (WRAP_ACCESSOR (GetHostBridgeByToken))
         );

    // Setup call to get fixed memory windows.
    EXPECT_CALL (
      MockConfigMgrProtocol,
      GetObject (
        _,
        CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjCxlFixedMemoryWindowInfo),
        CM_NULL_TOKEN,
        _
        )
      )
      .WillRepeatedly (
         Invoke (WRAP_ACCESSOR (GetAllWindows))
         );

    // Initialize the CEDT library with our mock protocol
    EXPECT_EQ (AcpiCedtLibConstructor (NULL, NULL), EFI_SUCCESS);

    // Setup common test data with proper initialization
    mAcpiTableInfo.TableGeneratorId   = CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdCedt);
    mAcpiTableInfo.AcpiTableSignature = EFI_ACPI_6_4_CXL_EARLY_DISCOVERY_TABLE_SIGNATURE;
    mAcpiTableInfo.AcpiTableRevision  = EFI_ACPI_6_4_CEDT_CXL_EARLY_DISCOVERY_TABLE_REVISION_01;
  }

  void
  TearDown (
    ) override
  {
    if (mTableUnderTest != nullptr) {
      gCedtGenerator->FreeTableResources (
                        gCedtGenerator,
                        &mAcpiTableInfo,
                        gConfigurationManagerProtocol,
                        &mTableUnderTest
                        );
    }

    // Clean up the CEDT library
    AcpiCedtLibDestructor (NULL, NULL);
  }

  // Setup the mocks for each type of object that can be requested from
  // ConfigurationManager.

  void
  SetupHostBridgeInfo (
    CM_ARCH_COMMON_CXL_HOST_BRIDGE_INFO  *HostBridges,
    UINT64                               HostBridgesCount
    )
  {
    // Copy config into test vector.
    mHostBridgeInfo.resize (HostBridgesCount);
    for (UINT64 i = 0; i < HostBridgesCount; i++) {
      mHostBridgeInfo[i] = HostBridges[i];
    }
  }

  void
  SetupFixedMemoryWindowInfo (
    CM_ARCH_COMMON_CXL_FIXED_MEMORY_WINDOW_INFO  *Windows,
    UINT64                                       WindowsCount
    )
  {
    // Copy config into test vector.
    mWindowInfo.resize (WindowsCount);
    for (UINT64 i = 0; i < WindowsCount; i++) {
      mWindowInfo[i] = Windows[i];
    }
  }

  EFI_STATUS
  GetHostBridgeByToken (
    IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *This,
    IN  CONST CM_OBJECT_ID                          CmObjectId,
    IN  CONST CM_OBJECT_TOKEN                       Token,
    IN  OUT   CM_OBJ_DESCRIPTOR                     *CmObject
    )
  {
    (void)This;
    (void)CmObjectId;

    if (CmObject == nullptr) {
      return EFI_INVALID_PARAMETER;
    }

    for (CM_ARCH_COMMON_CXL_HOST_BRIDGE_INFO &HostBridge : mHostBridgeInfo) {
      if ((UINTN)HostBridge.Token == Token) {
        *CmObject = CM_OBJ_DESCRIPTOR {
          CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjCxlHostBridgeInfo),
          sizeof (CM_ARCH_COMMON_CXL_HOST_BRIDGE_INFO),
          &HostBridge,
          1
        };
        return EFI_SUCCESS;
      }
    }

    return EFI_NOT_FOUND;
  }

  EFI_STATUS
  GetAllHostBridges (
    IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *This,
    IN  CONST CM_OBJECT_ID                          CmObjectId,
    IN  CONST CM_OBJECT_TOKEN                       Token,
    IN  OUT   CM_OBJ_DESCRIPTOR                     *CmObject
    )
  {
    (void)This;
    (void)CmObjectId;

    if (CmObject == nullptr) {
      return EFI_INVALID_PARAMETER;
    }

    *CmObject = CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjCxlHostBridgeInfo),
      static_cast<UINT32>(sizeof (CM_ARCH_COMMON_CXL_HOST_BRIDGE_INFO) * mHostBridgeInfo.size ()),
      mHostBridgeInfo.data (),
      static_cast<UINT32>(mHostBridgeInfo.size ())
    };

    return EFI_SUCCESS;
  }

  EFI_STATUS
  GetAllWindows (
    IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *This,
    IN  CONST CM_OBJECT_ID                          CmObjectId,
    IN  CONST CM_OBJECT_TOKEN                       Token,
    IN  OUT   CM_OBJ_DESCRIPTOR                     *CmObject
    )
  {
    (void)This;
    (void)CmObjectId;

    if (CmObject == nullptr) {
      return EFI_INVALID_PARAMETER;
    }

    *CmObject = CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjCxlFixedMemoryWindowInfo),
      static_cast<UINT32>(sizeof (CM_ARCH_COMMON_CXL_FIXED_MEMORY_WINDOW_INFO) * mWindowInfo.size ()),
      mWindowInfo.data (),
      static_cast<UINT32>(mWindowInfo.size ())
    };

    return EFI_SUCCESS;
  }

  CM_STD_OBJ_ACPI_TABLE_INFO mAcpiTableInfo;
  std::vector<CM_ARCH_COMMON_CXL_HOST_BRIDGE_INFO> mHostBridgeInfo;
  std::vector<CM_ARCH_COMMON_CXL_FIXED_MEMORY_WINDOW_INFO> mWindowInfo;
  EFI_ACPI_DESCRIPTION_HEADER *mTableUnderTest = nullptr;

  CM_ARCH_COMMON_CXL_HOST_BRIDGE_INFO mDefaultHb[2] = {
    {
      42,
      5,
      1,
      0x100000000
    },
    {
      71,
      2,
      0,
      0xF00000000
    },
  };

  EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE mDefaultHbExpected[2] = {
    {
      0,
      0,
      32,
      5,
      1,
      0,
      0x100000000,
      0x10000
    },
    {
      0,
      0,
      32,
      2,
      0,
      0,
      0xF00000000,
      0x2000
    }
  };

  CM_ARCH_COMMON_CXL_FIXED_MEMORY_WINDOW_INFO mDefaultWindows[2] = {
    {
      0x40000000,
      0xA0000000,
      2,
      0,
      1024,
      EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE_WINDOW_RESTRICTIONS_HOST_ONLY_COHERENT,
      7,
      {
        42, 71
      },
    },
    {
      0xF0000000,
      0xC0000000,
      2,
      0,
      16384,
      EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE_WINDOW_RESTRICTIONS_HOST_ONLY_COHERENT,
      7,
      {
        71, 42
      },
    }
  };
};

TEST_F (CedtGeneratorTest, SmokeTest) {
  EXPECT_EQ (
    gCedtGenerator->BuildAcpiTable (
                      gCedtGenerator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &mTableUnderTest
                      ),
    EFI_SUCCESS
    );

  EXPECT_EQ (mTableUnderTest->Length, 36U);
}

TEST_F (CedtGeneratorTest, ChbsOnlyTest) {
  SetupHostBridgeInfo (&mDefaultHb[0], 2);

  EXPECT_EQ (
    gCedtGenerator->BuildAcpiTable (
                      gCedtGenerator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &mTableUnderTest
                      ),
    EFI_SUCCESS
    );

  EXPECT_EQ (mTableUnderTest->Length, 100U);

  EFI_ACPI_6_4_CXL_EARLY_DISCOVERY_TABLE  *Cedt =
    (EFI_ACPI_6_4_CXL_EARLY_DISCOVERY_TABLE *)mTableUnderTest;
  EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE  *Chbs =
    (EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE *)(&Cedt->CedtStructure[0]);

  ValidateChbs (&mDefaultHbExpected[0], &Chbs[0]);
  ValidateChbs (&mDefaultHbExpected[1], &Chbs[1]);
}

TEST_F (CedtGeneratorTest, CfmwsWithNoInterleavingTest) {
  SetupHostBridgeInfo (&mDefaultHb[0], 1);

  CM_ARCH_COMMON_CXL_FIXED_MEMORY_WINDOW_INFO  Window = mDefaultWindows[0];

  Window.NumberOfInterleaveWays          = 1;
  Window.HostBridgeInterleaveGranularity = 256;
  SetupFixedMemoryWindowInfo (&Window, 1);

  EXPECT_EQ (
    gCedtGenerator->BuildAcpiTable (
                      gCedtGenerator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &mTableUnderTest
                      ),
    EFI_SUCCESS
    );

  EXPECT_EQ (mTableUnderTest->Length, 108U);
  EFI_ACPI_6_4_CXL_EARLY_DISCOVERY_TABLE  *Cedt =
    (EFI_ACPI_6_4_CXL_EARLY_DISCOVERY_TABLE *)mTableUnderTest;
  EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE  *Chbs =
    (EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE *)(&Cedt->CedtStructure[0]);

  EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE  *Cfmws =
    (EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE *)
    (((EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE *)&Cedt->CedtStructure[0]) + 1);

  CONST EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE  ExpectedCfmws = {
    1,
    0,
    40,
    0,
    0x40000000,
    0xA0000000,
    0,
    0,
    0,
    0,
    EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE_WINDOW_RESTRICTIONS_HOST_ONLY_COHERENT,
    7,
  };

  UINT32  ExpectedInterleaveTargetList[1] = { 5 };

  ValidateChbs (&mDefaultHbExpected[0], &Chbs[0]);
  ValidateCfmws (&ExpectedCfmws, Cfmws, &ExpectedInterleaveTargetList[0], 1);
}

TEST_F (CedtGeneratorTest, CfmwsWith2WayInterleavingTest) {
  SetupHostBridgeInfo (&mDefaultHb[0], 2);
  SetupFixedMemoryWindowInfo (&mDefaultWindows[0], 2);

  EXPECT_EQ (
    gCedtGenerator->BuildAcpiTable (
                      gCedtGenerator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &mTableUnderTest
                      ),
    EFI_SUCCESS
    );

  EXPECT_EQ (mTableUnderTest->Length, 188U);
  EFI_ACPI_6_4_CXL_EARLY_DISCOVERY_TABLE  *Cedt =
    (EFI_ACPI_6_4_CXL_EARLY_DISCOVERY_TABLE *)mTableUnderTest;
  EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE  *Chbs =
    (EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE *)(&Cedt->CedtStructure[0]);

  EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE  *Cfmws0 =
    (EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE *)
    (((EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE *)&Cedt->CedtStructure[0]) + 2);

  EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE  *Cfmws1 =
    (EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE *)
    (((UINT8 *)(Cfmws0 + 1)) + 2 * sizeof (UINT32));

  EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE  Cfmws0Expected = {
    1,
    0,
    44,
    0,
    0x40000000,
    0xA0000000,
    1,
    0,
    0,
    2,
    EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE_WINDOW_RESTRICTIONS_HOST_ONLY_COHERENT,
    7,
  };

  EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE  Cfmws1Expected = {
    1,
    0,
    44,
    0,
    0xF0000000,
    0xC0000000,
    1,
    0,
    0,
    6,
    EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE_WINDOW_RESTRICTIONS_HOST_ONLY_COHERENT,
    7,
  };

  UINT32  InterleaveTargetListExpected[2][2] = {
    { 5, 2 },
    { 2, 5 },
  };

  ValidateChbs (&mDefaultHbExpected[0], &Chbs[0]);
  ValidateChbs (&mDefaultHbExpected[1], &Chbs[1]);
  ValidateCfmws (&Cfmws0Expected, Cfmws0, &InterleaveTargetListExpected[0][0], 2);
  ValidateCfmws (&Cfmws1Expected, Cfmws1, &InterleaveTargetListExpected[1][0], 2);
}

TEST_F (CedtGeneratorTest, InvalidInterleaveWaysTest) {
  SetupHostBridgeInfo (&mDefaultHb[0], 1);

  CM_ARCH_COMMON_CXL_FIXED_MEMORY_WINDOW_INFO  Window = mDefaultWindows[0];

  Window.NumberOfInterleaveWays = 7;

  SetupFixedMemoryWindowInfo (&Window, 1);

  EXPECT_EQ (
    gCedtGenerator->BuildAcpiTable (
                      gCedtGenerator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &mTableUnderTest
                      ),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (CedtGeneratorTest, InvalidInterleaveGranularityTest) {
  SetupHostBridgeInfo (&mDefaultHb[0], 1);

  CM_ARCH_COMMON_CXL_FIXED_MEMORY_WINDOW_INFO  Window = mDefaultWindows[0];

  Window.HostBridgeInterleaveGranularity = 42;

  SetupFixedMemoryWindowInfo (&Window, 1);

  EXPECT_EQ (
    gCedtGenerator->BuildAcpiTable (
                      gCedtGenerator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &mTableUnderTest
                      ),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (CedtGeneratorTest, InvalidHpaAlignmentTest) {
  SetupHostBridgeInfo (&mDefaultHb[0], 1);

  CM_ARCH_COMMON_CXL_FIXED_MEMORY_WINDOW_INFO  Window = mDefaultWindows[0];

  Window.BaseHostPhysicalAddress = 0x04000000;

  SetupFixedMemoryWindowInfo (&Window, 1);

  EXPECT_EQ (
    gCedtGenerator->BuildAcpiTable (
                      gCedtGenerator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &mTableUnderTest
                      ),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (CedtGeneratorTest, InvalidWindowSizeTest) {
  SetupHostBridgeInfo (&mDefaultHb[0], 1);

  CM_ARCH_COMMON_CXL_FIXED_MEMORY_WINDOW_INFO  Window = mDefaultWindows[0];

  Window.WindowSizeBytes = 0xA1000000;
  SetupFixedMemoryWindowInfo (&Window, 1);

  EXPECT_EQ (
    gCedtGenerator->BuildAcpiTable (
                      gCedtGenerator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &mTableUnderTest
                      ),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (CedtGeneratorTest, InvalidWindowSizeWithInterleavingTest) {
  SetupHostBridgeInfo (&mDefaultHb[0], 2);

  CM_ARCH_COMMON_CXL_FIXED_MEMORY_WINDOW_INFO  Windows[2] = {
    mDefaultWindows[0],
    mDefaultWindows[1],
  };

  // Window size should be a multiple of 512MB.
  Windows[0].WindowSizeBytes = 0x30000000;
  SetupFixedMemoryWindowInfo (&Windows[0], 2);

  EXPECT_EQ (
    gCedtGenerator->BuildAcpiTable (
                      gCedtGenerator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &mTableUnderTest
                      ),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (CedtGeneratorTest, InvalidWindowRestrictionsTest) {
  SetupHostBridgeInfo (&mDefaultHb[0], 1);

  CM_ARCH_COMMON_CXL_FIXED_MEMORY_WINDOW_INFO  Window = mDefaultWindows[0];

  Window.WindowRestrictions = 0xF0;
  SetupFixedMemoryWindowInfo (&Window, 1);

  EXPECT_EQ (
    gCedtGenerator->BuildAcpiTable (
                      gCedtGenerator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &mTableUnderTest
                      ),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (CedtGeneratorTest, InvalidInterleaveArithmeticTest) {
  SetupHostBridgeInfo (&mDefaultHb[0], 1);

  CM_ARCH_COMMON_CXL_FIXED_MEMORY_WINDOW_INFO  Window = mDefaultWindows[0];

  Window.InterleaveArithmetic = 2;
  SetupFixedMemoryWindowInfo (&Window, 1);

  EXPECT_EQ (
    gCedtGenerator->BuildAcpiTable (
                      gCedtGenerator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &mTableUnderTest
                      ),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (CedtGeneratorTest, InvalidInterleaveTargetTest) {
  SetupHostBridgeInfo (&mDefaultHb[0], 1);

  CM_ARCH_COMMON_CXL_FIXED_MEMORY_WINDOW_INFO  Window = mDefaultWindows[0];

  Window.InterleaveTargetTokens[0] = 41;
  Window.NumberOfInterleaveWays    = 1;
  SetupFixedMemoryWindowInfo (&Window, 1);

  EXPECT_EQ (
    gCedtGenerator->BuildAcpiTable (
                      gCedtGenerator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &mTableUnderTest
                      ),
    EFI_NOT_FOUND
    );
}

TEST_F (CedtGeneratorTest, InvalidAbsentChbsForCfmwsTest) {
  CM_ARCH_COMMON_CXL_FIXED_MEMORY_WINDOW_INFO  Window = mDefaultWindows[0];

  Window.NumberOfInterleaveWays = 1;
  SetupFixedMemoryWindowInfo (&Window, 1);

  EXPECT_EQ (
    gCedtGenerator->BuildAcpiTable (
                      gCedtGenerator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &mTableUnderTest
                      ),
    EFI_NOT_FOUND
    );
}

int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
