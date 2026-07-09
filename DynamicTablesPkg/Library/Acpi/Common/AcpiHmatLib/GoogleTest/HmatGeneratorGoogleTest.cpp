/** @file
  Unit tests for HMAT Generator

  Copyright (c) 2026, Google LLC. All rights reserved. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Base.h"
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
  #include <Library/TableHelperLib.h>
  #include <Library/MetadataObjLib.h>
  #include <AcpiTableGenerator.h>
  #include <ConfigurationManagerObject.h>
  #include <ConfigurationManagerHelper.h>
  #include <StandardNameSpaceObjects.h>
  #include <IndustryStandard/Acpi64.h>
  #include "GoogleTest/Protocol/MockConfigurationManagerProtocol.h"
  #include "Library/Common/MetadataHandlerLib/MetadataHandler.h"

  EFI_STATUS
  EFIAPI
  AcpiHmatLibConstructor (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    );

  EFI_STATUS
  EFIAPI
  AcpiHmatLibDestructor (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    );

  // Global generator instance
  static ACPI_TABLE_GENERATOR  *gHmatGenerator = NULL;

  static METADATA_ROOT_HANDLE  mMetadataRoot;

  // C++ wrapper functions for C linkage functions
  EFI_STATUS
  RegisterAcpiTableGenerator (
    IN  CONST ACPI_TABLE_GENERATOR  *CONST  TableGenerator
    )
  {
    if (TableGenerator == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    gHmatGenerator = const_cast<ACPI_TABLE_GENERATOR *>(TableGenerator);
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
    gHmatGenerator = NULL;
    return EFI_SUCCESS;
  }

  METADATA_ROOT_HANDLE
  EFIAPI
  GetMetadataRoot (
    VOID
    )
  {
    return mMetadataRoot;
  }
}

using namespace testing;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::AtLeast;

class HmatGeneratorTest : public ::testing::Test {
protected:
  MockConfigurationManagerProtocol MockConfigMgrProtocol;
  CM_STD_OBJ_CONFIGURATION_MANAGER_INFO CfgMgrInfo = { 0 };

  void
  ValidateMemProximityDomainAttrInfo (
    const EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES  *Expected,
    const EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES  *Actual
    )
  {
    EXPECT_EQ (Actual->Type, Expected->Type);
    EXPECT_EQ (Actual->Length, sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES));
    EXPECT_EQ (Actual->Flags.InitiatorProximityDomainValid, Expected->Flags.InitiatorProximityDomainValid);
    EXPECT_EQ (Actual->InitiatorProximityDomain, Expected->InitiatorProximityDomain);
    EXPECT_EQ (Actual->MemoryProximityDomain, Expected->MemoryProximityDomain);
  }

  void
  ValidateSSLBI (
    const EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO  *ExpectedBase,
    const UINT32                                                                  *ExpectedInitiatorDomains,
    const UINT32                                                                  *ExpectedTargetDomains,
    const UINT16                                                                  *ExpectedLatencyMatrix,
    const EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO  *Actual
    )
  {
    EXPECT_EQ (Actual->Type, ExpectedBase->Type);
    EXPECT_EQ (Actual->Length, ExpectedBase->Length);
    EXPECT_EQ (Actual->Flags.AccessAttributes, ExpectedBase->Flags.AccessAttributes);
    EXPECT_EQ (Actual->Flags.MemoryHierarchy, ExpectedBase->Flags.MemoryHierarchy);
    EXPECT_EQ (Actual->DataType, ExpectedBase->DataType);
    EXPECT_EQ (Actual->NumberOfInitiatorProximityDomains, ExpectedBase->NumberOfInitiatorProximityDomains);
    EXPECT_EQ (Actual->NumberOfTargetProximityDomains, ExpectedBase->NumberOfTargetProximityDomains);
    EXPECT_EQ (Actual->EntryBaseUnit, ExpectedBase->EntryBaseUnit);

    UINT8   *Ptr                    = (UINT8 *)Actual;
    UINT32  *ActualInitiatorDomains = (UINT32 *)(Ptr + sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO));

    for (UINT32 i = 0; i < ExpectedBase->NumberOfInitiatorProximityDomains; i++) {
      EXPECT_EQ (ActualInitiatorDomains[i], ExpectedInitiatorDomains[i]);
    }

    UINT32  *ActualTargetDomains = (UINT32 *)(ActualInitiatorDomains + ExpectedBase->NumberOfInitiatorProximityDomains);

    for (UINT32 i = 0; i < ExpectedBase->NumberOfTargetProximityDomains; i++) {
      EXPECT_EQ (ActualTargetDomains[i], ExpectedTargetDomains[i]);
    }

    UINT16  *ActualLatencyMatrix = (UINT16 *)(ActualTargetDomains + ExpectedBase->NumberOfTargetProximityDomains);

    for (UINT32 i = 0; i < ExpectedBase->NumberOfInitiatorProximityDomains * ExpectedBase->NumberOfTargetProximityDomains; i++) {
      EXPECT_EQ (ActualLatencyMatrix[i], ExpectedLatencyMatrix[i]);
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
    CfgMgrInfo.OemId[4] = 'M';
    CfgMgrInfo.OemId[5] = 'E';

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

    EXPECT_CALL (
      MockConfigMgrProtocol,
      GetObject (
        _,
        CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjProximityDomainInfo),
        _,
        _
        )
      )
      .WillRepeatedly (
         Invoke (WRAP_ACCESSOR (GetProximityDomainInfo))
         );

    EXPECT_CALL (
      MockConfigMgrProtocol,
      GetObject (
        _,
        CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjMemoryProximityDomainAttrInfo),
        CM_NULL_TOKEN,
        _
        )
      )
      .WillRepeatedly (
         Invoke (WRAP_ACCESSOR (GetMemProximityDomainAttrInfo))
         );

    EXPECT_CALL (
      MockConfigMgrProtocol,
      GetObject (
        _,
        CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjMemoryLatBwInfo),
        CM_NULL_TOKEN,
        _
        )
      )
      .WillRepeatedly (
         Invoke (WRAP_ACCESSOR (GetMemLatBwInfo))
         );

    EXPECT_CALL (
      MockConfigMgrProtocol,
      GetObject (
        _,
        CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjProximityDomainRelationInfo),
        _,
        _
        )
      )
      .WillRepeatedly (
         Invoke (WRAP_ACCESSOR (GetProximityDomainRelationInfo))
         );

    // Initialize the HMAT library with our mock protocol
    EXPECT_EQ (AcpiHmatLibConstructor (NULL, NULL), EFI_SUCCESS);

    // Setup common test data with proper initialization
    mAcpiTableInfo.TableGeneratorId   = CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdHmat);
    mAcpiTableInfo.AcpiTableSignature = EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_SIGNATURE;
    mAcpiTableInfo.AcpiTableRevision  = EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_REVISION;

    MetadataInitializeHandle (&mMetadataRoot);
  }

  void
  TearDown (
    ) override
  {
    if (mTableUnderTest != nullptr) {
      gHmatGenerator->FreeTableResources (
                        gHmatGenerator,
                        &mAcpiTableInfo,
                        gConfigurationManagerProtocol,
                        &mTableUnderTest
                        );
    }

    // Clean up the HMAT library
    AcpiHmatLibDestructor (NULL, NULL);
    MetadataFreeHandle (mMetadataRoot);
  }

  void
  SetupProximityDomainInfo (
    UINT64  ProximityDomainInfoCount
    )
  {
    mProximityDomainInfo.resize (ProximityDomainInfoCount);
    for (UINT64 i = 0; i < ProximityDomainInfoCount; i++) {
      mProximityDomainInfo[i].GenerateDomainId = FALSE;
      mProximityDomainInfo[i].DomainId         = 100 + (UINT32)i;
    }
  }

  void
  SetupMemProximityDomainAttrInfo (
    CM_ARCH_COMMON_MEMORY_PROXIMITY_DOMAIN_ATTR_INFO  *MemProximityDomainAttrInfo,
    UINT64                                            MemProximityDomainAttrInfoCount
    )
  {
    mMemProximityDomainAttrInfo.resize (MemProximityDomainAttrInfoCount);
    for (UINT64 i = 0; i < MemProximityDomainAttrInfoCount; i++) {
      mMemProximityDomainAttrInfo[i] = MemProximityDomainAttrInfo[i];
    }
  }

  void
  SetupMemLatBwInfo (
    CM_ARCH_COMMON_MEMORY_LAT_BW_INFO  *MemLatBwInfo,
    UINT64                             MemLatBwInfoCount
    )
  {
    mMemLatBwInfo.resize (MemLatBwInfoCount);
    for (UINT64 i = 0; i < MemLatBwInfoCount; i++) {
      mMemLatBwInfo[i] = MemLatBwInfo[i];
    }
  }

  void
  SetupProximityDomainRelationInfo (
    CM_OBJECT_TOKEN                                Token,
    CM_ARCH_COMMON_PROXIMITY_DOMAIN_RELATION_INFO  *Matrix,
    UINT64                                         MatrixCount
    )
  {
    mProximityDomainRelationInfo[Token].resize (MatrixCount);
    for (UINT64 i = 0; i < MatrixCount; i++) {
      mProximityDomainRelationInfo[Token][i] = Matrix[i];
    }
  }

  EFI_STATUS
  GetProximityDomainInfo (
    IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *This,
    IN  CONST CM_OBJECT_ID                          CmObjectId,
    IN  CONST CM_OBJECT_TOKEN                       Token,
    IN  OUT   CM_OBJ_DESCRIPTOR                     *CmObject
    )
  {
    UINT32  Index;

    if (CmObject == nullptr) {
      return EFI_INVALID_PARAMETER;
    }

    if (Token == CM_NULL_TOKEN) {
      *CmObject = CM_OBJ_DESCRIPTOR {
        CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjProximityDomainInfo),
        static_cast<UINT32>(sizeof (CM_ARCH_COMMON_PROXIMITY_DOMAIN_INFO) * mProximityDomainInfo.size ()),
        mProximityDomainInfo.data (),
        static_cast<UINT32>(mProximityDomainInfo.size ())
      };

      return EFI_SUCCESS;
    }

    Index = (UINT32)Token - 1;
    if (Index >= mProximityDomainInfo.size ()) {
      return EFI_NOT_FOUND;
    }

    *CmObject = CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjProximityDomainInfo),
      static_cast<UINT32>(sizeof (CM_ARCH_COMMON_PROXIMITY_DOMAIN_INFO)),
      &mProximityDomainInfo[Index],
      1
    };

    return EFI_SUCCESS;
  }

  EFI_STATUS
  GetMemProximityDomainAttrInfo (
    IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *This,
    IN  CONST CM_OBJECT_ID                          CmObjectId,
    IN  CONST CM_OBJECT_TOKEN                       Token,
    IN  OUT   CM_OBJ_DESCRIPTOR                     *CmObject
    )
  {
    if (CmObject == nullptr) {
      return EFI_INVALID_PARAMETER;
    }

    *CmObject = CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjMemoryProximityDomainAttrInfo),
      static_cast<UINT32>(sizeof (CM_ARCH_COMMON_MEMORY_PROXIMITY_DOMAIN_ATTR_INFO) * mMemProximityDomainAttrInfo.size ()),
      mMemProximityDomainAttrInfo.data (),
      static_cast<UINT32>(mMemProximityDomainAttrInfo.size ())
    };

    return EFI_SUCCESS;
  }

  EFI_STATUS
  GetMemLatBwInfo (
    IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *This,
    IN  CONST CM_OBJECT_ID                          CmObjectId,
    IN  CONST CM_OBJECT_TOKEN                       Token,
    IN  OUT   CM_OBJ_DESCRIPTOR                     *CmObject
    )
  {
    if (CmObject == nullptr) {
      return EFI_INVALID_PARAMETER;
    }

    *CmObject = CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjMemoryLatBwInfo),
      static_cast<UINT32>(sizeof (CM_ARCH_COMMON_MEMORY_LAT_BW_INFO) * mMemLatBwInfo.size ()),
      mMemLatBwInfo.data (),
      static_cast<UINT32>(mMemLatBwInfo.size ())
    };

    return EFI_SUCCESS;
  }

  EFI_STATUS
  GetProximityDomainRelationInfo (
    IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *This,
    IN  CONST CM_OBJECT_ID                          CmObjectId,
    IN  CONST CM_OBJECT_TOKEN                       Token,
    IN  OUT   CM_OBJ_DESCRIPTOR                     *CmObject
    )
  {
    if (CmObject == nullptr) {
      return EFI_INVALID_PARAMETER;
    }

    auto  it = mProximityDomainRelationInfo.find (Token);

    if (it == mProximityDomainRelationInfo.end ()) {
      return EFI_NOT_FOUND;
    }

    *CmObject = CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjProximityDomainRelationInfo),
      static_cast<UINT32>(sizeof (CM_ARCH_COMMON_PROXIMITY_DOMAIN_RELATION_INFO) * it->second.size ()),
      it->second.data (),
      static_cast<UINT32>(it->second.size ())
    };

    return EFI_SUCCESS;
  }

  CM_STD_OBJ_ACPI_TABLE_INFO mAcpiTableInfo;
  std::vector<CM_ARCH_COMMON_PROXIMITY_DOMAIN_INFO> mProximityDomainInfo;
  std::vector<CM_ARCH_COMMON_MEMORY_PROXIMITY_DOMAIN_ATTR_INFO> mMemProximityDomainAttrInfo;
  std::vector<CM_ARCH_COMMON_MEMORY_LAT_BW_INFO> mMemLatBwInfo;
  std::map<CM_OBJECT_TOKEN, std::vector<CM_ARCH_COMMON_PROXIMITY_DOMAIN_RELATION_INFO> > mProximityDomainRelationInfo;
  EFI_ACPI_DESCRIPTION_HEADER *mTableUnderTest = nullptr;

  CM_ARCH_COMMON_PROXIMITY_DOMAIN_INFO mDefaultProximityDomains[10];

  CM_ARCH_COMMON_MEMORY_PROXIMITY_DOMAIN_ATTR_INFO mDefaultMemProximity[2] = {
    {
      .Flags                    = 0,
      .InitiatorProximityDomain = 1,
      .MemoryProximityDomain    = 2
    },
    {
      .Flags                    = 1,
      .InitiatorProximityDomain = 3,
      .MemoryProximityDomain    = 4
    }
  };

  CM_ARCH_COMMON_MEMORY_LAT_BW_INFO mDefaultMemLatBw[2] = {
    {
      .Flags                 = 1,
      .DataType              = 3,
      .MinTransferSize       = 24,
      .EntryBaseUnit         = 2,
      .RelativeDistanceArray = 1
    },
    {
      .Flags                 = 1,
      .DataType              = 3,
      .MinTransferSize       = 24,
      .EntryBaseUnit         = 2,
      .RelativeDistanceArray = 2
    }
  };

  CM_ARCH_COMMON_PROXIMITY_DOMAIN_RELATION_INFO mDefaultMatrix1[4] = {
    { .FirstDomainToken = 1, .SecondDomainToken = 2, .Relation = 100 },
    { .FirstDomainToken = 1, .SecondDomainToken = 4, .Relation = 200 },
    { .FirstDomainToken = 3, .SecondDomainToken = 2, .Relation = 300 },
    { .FirstDomainToken = 3, .SecondDomainToken = 4, .Relation = 400 }
  };

  CM_ARCH_COMMON_PROXIMITY_DOMAIN_RELATION_INFO mDefaultMatrix2[2] = {
    { .FirstDomainToken = 1, .SecondDomainToken = 5, .Relation = 1000 },
    { .FirstDomainToken = 1, .SecondDomainToken = 6, .Relation = 2000 },
  };

  // Invalid relation value, should be in UINT16 range
  CM_ARCH_COMMON_PROXIMITY_DOMAIN_RELATION_INFO mInvalidRelMatrix[2] = {
    { .FirstDomainToken = 1, .SecondDomainToken = 2, .Relation = 301000 },
    { .FirstDomainToken = 1, .SecondDomainToken = 2, .Relation = 302000 },
  };
};

STATIC
constexpr UINT32
GetSLLBISize (
  UINT32  InitiatorCount,
  UINT32  TargetCount
  )
{
  return sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO) +
         InitiatorCount * sizeof (UINT32) +
         TargetCount * sizeof (UINT32) +
         InitiatorCount * TargetCount * sizeof (UINT16);
}

constexpr EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES  kExpectedMpda[2] = {
  {
    .Type   = 0,
    .Length = 40,
    .Flags  = {
      .InitiatorProximityDomainValid = 0
    },
    .InitiatorProximityDomain = 100,
    .MemoryProximityDomain    = 101,
  },
  {
    .Type   = 0,
    .Length = 40,
    .Flags  = {
      .InitiatorProximityDomainValid = 1
    },
    .InitiatorProximityDomain = 102,
    .MemoryProximityDomain    = 103
  }
};

constexpr EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO  kExpectedSllbiBase[2] = {
  {
    .Type   = 1,
    .Length = GetSLLBISize (2, 2),
    .Flags  = {
      .MemoryHierarchy = 1
    },
    .DataType                          = 3,
    .MinTransferSize                   = 24,
    .NumberOfInitiatorProximityDomains = 2,
    .NumberOfTargetProximityDomains    = 2,
    .EntryBaseUnit                     = 2
  },
  {
    .Type   = 1,
    .Length = GetSLLBISize (1, 2),
    .Flags  = {
      .MemoryHierarchy = 1
    },
    .DataType                          = 3,
    .MinTransferSize                   = 24,
    .NumberOfInitiatorProximityDomains = 1,
    .NumberOfTargetProximityDomains    = 2,
    .EntryBaseUnit                     = 2
  }
};

constexpr UINT32  kExpectedSllbiInitiators0[2] = {
  100, 102
};

constexpr UINT32  kExpectedSllbiInitiators1[1] = {
  100
};

constexpr UINT32  kExpectedSllbiTargets0[2] = {
  101, 103
};

constexpr UINT32  kExpectedSllbiTargets1[2] = {
  104, 105
};

constexpr UINT16  kExpectedSllbiMatrix0[4] = { 100, 200, 300, 400
};

constexpr UINT16  kExpectedSllbiMatrix1[2] = { 1000, 2000
};

TEST_F (HmatGeneratorTest, NoProximityDomainsTest) {
  SetupProximityDomainInfo (4);
  SetupMemLatBwInfo (mDefaultMemLatBw, 1);
  SetupProximityDomainRelationInfo ((CM_OBJECT_TOKEN)1, mDefaultMatrix1, 4);
  EXPECT_EQ (
    gHmatGenerator->BuildAcpiTable (
                      gHmatGenerator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &mTableUnderTest
                      ),
    EFI_SUCCESS
    );
  UINT32  expectedSize = sizeof (EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_HEADER) +
                         GetSLLBISize (2, 2);

  EXPECT_EQ (mTableUnderTest->Length, expectedSize);

  UINT8  *mTableUnderTestPtr = (UINT8 *)mTableUnderTest;

  ValidateSSLBI (
    &kExpectedSllbiBase[0],
    kExpectedSllbiInitiators0,
    kExpectedSllbiTargets0,
    kExpectedSllbiMatrix0,
    (EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO *)&mTableUnderTestPtr[40]
    );
}

TEST_F (HmatGeneratorTest, NoLatencyInfoTest) {
  SetupProximityDomainInfo (4);
  SetupMemProximityDomainAttrInfo (mDefaultMemProximity, 2);
  EXPECT_EQ (
    gHmatGenerator->BuildAcpiTable (
                      gHmatGenerator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &mTableUnderTest
                      ),
    EFI_SUCCESS
    );
  UINT32  expectedSize = sizeof (EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_HEADER) +
                         2 * sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES);

  EXPECT_EQ (mTableUnderTest->Length, expectedSize);
  UINT8  *tableUnderTestPtr = (UINT8 *)mTableUnderTest;

  UINT32  Idx = sizeof (EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_HEADER);

  ValidateMemProximityDomainAttrInfo (&kExpectedMpda[0], (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES *)&tableUnderTestPtr[Idx]);

  Idx = sizeof (EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_HEADER) +
        sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES);
  ValidateMemProximityDomainAttrInfo (&kExpectedMpda[1], (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES *)&tableUnderTestPtr[Idx]);
}

TEST_F (HmatGeneratorTest, MismatchedMatrixSizeTest) {
  SetupProximityDomainInfo (4);
  SetupMemProximityDomainAttrInfo (mDefaultMemProximity, 2);
  SetupMemLatBwInfo (mDefaultMemLatBw, 1);
  SetupProximityDomainRelationInfo ((CM_OBJECT_TOKEN)1, &mDefaultMatrix1[0], 3); // Should be 4
  EXPECT_EQ (
    gHmatGenerator->BuildAcpiTable (
                      gHmatGenerator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &mTableUnderTest
                      ),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (HmatGeneratorTest, InvalidRelationValueTest) {
  SetupProximityDomainInfo (4);
  SetupMemProximityDomainAttrInfo (mDefaultMemProximity, 2);
  SetupMemLatBwInfo (mDefaultMemLatBw, 1);
  SetupProximityDomainRelationInfo ((CM_OBJECT_TOKEN)1, &mInvalidRelMatrix[0], 2);
  EXPECT_EQ (
    gHmatGenerator->BuildAcpiTable (
                      gHmatGenerator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &mTableUnderTest
                      ),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (HmatGeneratorTest, ValidTableTest) {
  SetupProximityDomainInfo (7);
  SetupMemProximityDomainAttrInfo (mDefaultMemProximity, 2);
  SetupMemLatBwInfo (mDefaultMemLatBw, 2);
  SetupProximityDomainRelationInfo ((CM_OBJECT_TOKEN)1, mDefaultMatrix1, 4);
  SetupProximityDomainRelationInfo ((CM_OBJECT_TOKEN)2, mDefaultMatrix2, 2);

  EXPECT_EQ (
    gHmatGenerator->BuildAcpiTable (
                      gHmatGenerator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &mTableUnderTest
                      ),
    EFI_SUCCESS
    );

  EXPECT_NE (mTableUnderTest, nullptr);
  UINT32  expectedSize = sizeof (EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_HEADER) +
                         2 * sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES) +
                         GetSLLBISize (2, 2) +
                         GetSLLBISize (2, 1);

  EXPECT_EQ (mTableUnderTest->Length, expectedSize);
  UINT8  *tableUnderTestPtr = (UINT8 *)mTableUnderTest;

  UINT32  Idx = sizeof (EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_HEADER);

  ValidateMemProximityDomainAttrInfo (&kExpectedMpda[0], (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES *)&tableUnderTestPtr[Idx]);

  Idx = sizeof (EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_HEADER) +
        sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES);
  ValidateMemProximityDomainAttrInfo (&kExpectedMpda[1], (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES *)&tableUnderTestPtr[Idx]);

  Idx = sizeof (EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_HEADER) +
        2 * sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES);
  ValidateSSLBI (
    &kExpectedSllbiBase[0],
    kExpectedSllbiInitiators0,
    kExpectedSllbiTargets0,
    kExpectedSllbiMatrix0,
    (EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO *)&tableUnderTestPtr[Idx]
    );

  Idx = sizeof (EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_HEADER) +
        2 * sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES) +
        GetSLLBISize (2, 2);
  ValidateSSLBI (
    &kExpectedSllbiBase[1],
    kExpectedSllbiInitiators1,
    kExpectedSllbiTargets1,
    kExpectedSllbiMatrix1,
    (EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO *)&tableUnderTestPtr[Idx]
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
