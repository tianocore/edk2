/** @file
  Configuration Manager Dxe

  Copyright (c) 2021 - 2022, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#include <IndustryStandard/DebugPort2Table.h>
#include <IndustryStandard/IoRemappingTable.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>
#include <IndustryStandard/SerialPortConsoleRedirectionTable.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DynamicPlatRepoLib.h>
#include <Library/HobLib.h>
#include <Library/HwInfoParserLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/TableHelperLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "ConfigurationManager.h"

//
// The platform configuration repository information.
//
STATIC
EDKII_PLATFORM_REPOSITORY_INFO  mKvmtoolPlatRepositoryInfo = {
  //
  // Configuration Manager information
  //
  { CONFIGURATION_MANAGER_REVISION, CFG_MGR_OEM_ID },

  //
  // ACPI Table List
  //
  {
    //
    // FADT Table
    //
    {
      EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdFadt),
      NULL
    },
    //
    // GTDT Table
    //
    {
      EFI_ACPI_6_3_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_6_3_GENERIC_TIMER_DESCRIPTION_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdGtdt),
      NULL
    },
    //
    // MADT Table
    //
    {
      EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMadt),
      NULL
    },
    //
    // SPCR Table
    //
    {
      EFI_ACPI_6_3_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE,
      EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSpcr),
      NULL
    },
    //
    // DSDT Table
    //
    {
      EFI_ACPI_6_3_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
      0, // Unused
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdDsdt),
      (EFI_ACPI_DESCRIPTION_HEADER *)dsdt_aml_code
    },
    //
    // SSDT Cpu Hierarchy Table
    //
    {
      EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
      0, // Unused
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtCpuTopology),
      NULL
    },
    //
    // DBG2 Table
    //
    {
      EFI_ACPI_6_3_DEBUG_PORT_2_TABLE_SIGNATURE,
      EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdDbg2),
      NULL
    },
    //
    // PCI MCFG Table
    //
    {
      EFI_ACPI_6_3_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE,
      EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_REVISION,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMcfg),
      NULL
    },
    //
    // SSDT table describing the PCI root complex
    //
    {
      EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
      0, // Unused
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtPciExpress),
      NULL
    },
    //
    // IORT Table
    //
    {
      EFI_ACPI_6_3_IO_REMAPPING_TABLE_SIGNATURE,
      EFI_ACPI_IO_REMAPPING_TABLE_REVISION_00,
      CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdIort),
      NULL
    },
  },

  //
  // Power management profile information
  //
  { EFI_ACPI_6_3_PM_PROFILE_ENTERPRISE_SERVER },    // PowerManagement Profile

  //
  // ITS group node
  //
  {
    //
    // Reference token for this Iort node
    //
    REFERENCE_TOKEN (ItsGroupInfo),
    //
    // The number of ITS identifiers in the ITS node.
    //
    1,
    //
    // Reference token for the ITS identifier array
    //
    REFERENCE_TOKEN (ItsIdentifierArray)
  },

  //
  // ITS identifier array
  //
  {
    { 0 },                            // The ITS Identifier
  },

  //
  // Root Complex node info
  //
  {
    //
    // Reference token for this Iort node
    //
    REFERENCE_TOKEN (RootComplexInfo),
    //
    // Number of ID mappings
    //
    1,
    //
    // Reference token for the ID mapping array
    //
    REFERENCE_TOKEN (DeviceIdMapping[0]),
    //
    // Memory access properties : Cache coherent attributes
    //
    EFI_ACPI_IORT_MEM_ACCESS_PROP_CCA,
    //
    // Memory access properties : Allocation hints
    //
    0,
    //
    // Memory access properties : Memory access flags
    //
    0,
    //
    // ATS attributes
    //
    EFI_ACPI_IORT_ROOT_COMPLEX_ATS_UNSUPPORTED,
    //
    // PCI segment number
    //
    0,
    ///
    /// Memory address size limit
    ///
    MEMORY_ADDRESS_SIZE_LIMIT
  },

  //
  // Array of Device ID mappings
  //
  {
    //
    // Device ID mapping for Root complex node
    // RootComplex -> ITS Group
    //
    {
      //
      // Input base
      //
      0x0,
      //
      // Number of input IDs
      //
      0x0000FFFF,
      //
      // Output Base
      //
      0x0,
      //
      // Output reference
      //
      REFERENCE_TOKEN (ItsGroupInfo),
      //
      // Flags
      //
      0
    },
  },
};

/**
  A helper function for returning the Configuration Manager Objects.

  @param [in]       CmObjectId     The Configuration Manager Object ID.
  @param [in]       Object         Pointer to the Object(s).
  @param [in]       ObjectSize     Total size of the Object(s).
  @param [in]       ObjectCount    Number of Objects.
  @param [in, out]  CmObjectDesc   Pointer to the Configuration Manager Object
                                   descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
**/
STATIC
EFI_STATUS
EFIAPI
HandleCmObject (
  IN  CONST CM_OBJECT_ID                CmObjectId,
  IN        VOID                        *Object,
  IN  CONST UINTN                       ObjectSize,
  IN  CONST UINTN                       ObjectCount,
  IN  OUT   CM_OBJ_DESCRIPTOR   *CONST  CmObjectDesc
  )
{
  CmObjectDesc->ObjectId = CmObjectId;
  CmObjectDesc->Size     = ObjectSize;
  CmObjectDesc->Data     = Object;
  CmObjectDesc->Count    = ObjectCount;
  DEBUG ((
    DEBUG_INFO,
    "INFO: CmObjectId = " FMT_CM_OBJECT_ID ", "
                                           "Ptr = 0x%p, Size = %lu, Count = %lu\n",
    CmObjectId,
    CmObjectDesc->Data,
    CmObjectDesc->Size,
    CmObjectDesc->Count
    ));
  return EFI_SUCCESS;
}

/**
  A helper function for returning the Configuration Manager Objects that
  match the token.

  @param [in]  This               Pointer to the Configuration Manager Protocol.
  @param [in]  CmObjectId         The Configuration Manager Object ID.
  @param [in]  Object             Pointer to the Object(s).
  @param [in]  ObjectSize         Total size of the Object(s).
  @param [in]  ObjectCount        Number of Objects.
  @param [in]  Token              A token identifying the object.
  @param [in]  HandlerProc        A handler function to search the object
                                  referenced by the token.
  @param [in, out]  CmObjectDesc  Pointer to the Configuration Manager Object
                                  descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
STATIC
EFI_STATUS
EFIAPI
HandleCmObjectRefByToken (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN        VOID                                          *Object,
  IN  CONST UINTN                                         ObjectSize,
  IN  CONST UINTN                                         ObjectCount,
  IN  CONST CM_OBJECT_TOKEN                               Token,
  IN  CONST CM_OBJECT_HANDLER_PROC                        HandlerProc,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObjectDesc
  )
{
  EFI_STATUS  Status;

  CmObjectDesc->ObjectId = CmObjectId;
  if (Token == CM_NULL_TOKEN) {
    CmObjectDesc->Size  = ObjectSize;
    CmObjectDesc->Data  = Object;
    CmObjectDesc->Count = ObjectCount;
    Status              = EFI_SUCCESS;
  } else {
    Status = HandlerProc (This, CmObjectId, Token, CmObjectDesc);
  }

  DEBUG ((
    DEBUG_INFO,
    "INFO: Token = 0x%p, CmObjectId = " FMT_CM_OBJECT_ID ", "
                                                         "Ptr = 0x%p, Size = %lu, Count = %lu\n",
    (VOID *)Token,
    CmObjectId,
    CmObjectDesc->Data,
    CmObjectDesc->Size,
    CmObjectDesc->Count
    ));
  return Status;
}

/**
  Return an ITS identifier array.

  @param [in]  This        Pointer to the Configuration Manager Protocol.
  @param [in]  CmObjectId  The Configuration Manager Object ID.
  @param [in]  Token       A token for identifying the object
  @param [out] CmObject    Pointer to the Configuration Manager Object
                           descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetItsIdentifierArray (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token,
  OUT       CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PlatformRepo = This->PlatRepoInfo;

  if (Token != (CM_OBJECT_TOKEN)&PlatformRepo->ItsIdentifierArray) {
    return EFI_NOT_FOUND;
  }

  CmObject->ObjectId = CmObjectId;
  CmObject->Size     = sizeof (PlatformRepo->ItsIdentifierArray);
  CmObject->Data     = (VOID *)&PlatformRepo->ItsIdentifierArray;
  CmObject->Count    = ARRAY_SIZE (PlatformRepo->ItsIdentifierArray);
  return EFI_SUCCESS;
}

/**
  Return a device Id mapping array.

  @param [in]  This        Pointer to the Configuration Manager Protocol.
  @param [in]  CmObjectId  The Configuration Manager Object ID.
  @param [in]  Token       A token for identifying the object
  @param [out] CmObject    Pointer to the Configuration Manager Object
                           descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetDeviceIdMappingArray (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token,
  OUT       CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PlatformRepo = This->PlatRepoInfo;

  if (Token != (CM_OBJECT_TOKEN)&PlatformRepo->DeviceIdMapping[0]) {
    return EFI_NOT_FOUND;
  }

  CmObject->ObjectId = CmObjectId;
  CmObject->Size     = sizeof (CM_ARM_ID_MAPPING);
  CmObject->Data     = (VOID *)Token;
  CmObject->Count    = 1;
  return EFI_SUCCESS;
}

/**
  Function pointer called by the parser to add information.

  Callback function that the parser can use to add new
  CmObj. This function must copy the CmObj data and not rely on
  the parser preserving the CmObj memory.
  This function is responsible of the Token allocation.

  @param  [in]  ParserHandle  A handle to the parser instance.
  @param  [in]  Context       A pointer to the caller's context provided in
                              HwInfoParserInit ().
  @param  [in]  CmObjDesc     CM_OBJ_DESCRIPTOR containing the CmObj(s) to add.
  @param  [out] Token         If provided and success, contain the token
                              generated for the CmObj.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
HwInfoAdd (
  IN        HW_INFO_PARSER_HANDLE  ParserHandle,
  IN        VOID                   *Context,
  IN  CONST CM_OBJ_DESCRIPTOR      *CmObjDesc,
  OUT       CM_OBJECT_TOKEN        *Token OPTIONAL
  )
{
  EFI_STATUS                      Status;
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;

  if ((ParserHandle == NULL)  ||
      (Context == NULL)       ||
      (CmObjDesc == NULL))
  {
    ASSERT (ParserHandle != NULL);
    ASSERT (Context != NULL);
    ASSERT (CmObjDesc != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PlatformRepo = (EDKII_PLATFORM_REPOSITORY_INFO *)Context;

  DEBUG_CODE_BEGIN ();
  //
  // Print the received objects.
  //
  ParseCmObjDesc (CmObjDesc);
  DEBUG_CODE_END ();

  Status = DynPlatRepoAddObject (
             PlatformRepo->DynamicPlatformRepo,
             CmObjDesc,
             Token
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/**
  Cleanup the platform configuration repository.

  @param [in]  This        Pointer to the Configuration Manager Protocol.

  @retval EFI_SUCCESS             Success
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
CleanupPlatformRepository (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This
  )
{
  EFI_STATUS                      Status;
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;

  if (This == NULL) {
    ASSERT (This != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PlatformRepo = This->PlatRepoInfo;

  //
  // Shutdown the dynamic repo and free all objects.
  //
  Status = DynamicPlatRepoShutdown (PlatformRepo->DynamicPlatformRepo);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  //
  // Shutdown parser.
  //
  Status = HwInfoParserShutdown (PlatformRepo->FdtParserHandle);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/**
  Initialize the platform configuration repository.

  @param [in]  This        Pointer to the Configuration Manager Protocol.

  @retval EFI_SUCCESS             Success
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES    An allocation has failed.
**/
STATIC
EFI_STATUS
EFIAPI
InitializePlatformRepository (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This
  )
{
  EFI_STATUS                      Status;
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;
  VOID                            *Hob;

  if (This == NULL) {
    ASSERT (This != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Hob = GetFirstGuidHob (&gFdtHobGuid);
  if ((Hob == NULL) || (GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (UINT64))) {
    ASSERT (FALSE);
    ASSERT (GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (UINT64));
    return EFI_NOT_FOUND;
  }

  PlatformRepo          = This->PlatRepoInfo;
  PlatformRepo->FdtBase = (VOID *)*(UINTN *)GET_GUID_HOB_DATA (Hob);

  //
  // Initialise the dynamic platform repository.
  //
  Status = DynamicPlatRepoInit (&PlatformRepo->DynamicPlatformRepo);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  //
  // Initialise the FDT parser
  //
  Status = HwInfoParserInit (
             PlatformRepo->FdtBase,
             PlatformRepo,
             HwInfoAdd,
             &PlatformRepo->FdtParserHandle
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ErrorHandler;
  }

  Status = HwInfoParse (PlatformRepo->FdtParserHandle);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ErrorHandler;
  }

  Status = DynamicPlatRepoFinalise (PlatformRepo->DynamicPlatformRepo);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ErrorHandler;
  }

  return EFI_SUCCESS;

ErrorHandler:
  CleanupPlatformRepository (This);
  return Status;
}

/**
  Return a standard namespace object.

  @param [in]      This        Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId  The Configuration Manager Object ID.
  @param [in]      Token       An optional token identifying the object. If
                               unused this must be CM_NULL_TOKEN.
  @param [in, out] CmObject    Pointer to the Configuration Manager Object
                               descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetStandardNameSpaceObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EFI_STATUS                      Status;
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;
  UINTN                           AcpiTableCount;
  CM_OBJ_DESCRIPTOR               CmObjDesc;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status       = EFI_NOT_FOUND;
  PlatformRepo = This->PlatRepoInfo;

  switch (GET_CM_OBJECT_ID (CmObjectId)) {
    case EStdObjCfgMgrInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->CmInfo,
                 sizeof (PlatformRepo->CmInfo),
                 1,
                 CmObject
                 );
      break;

    case EStdObjAcpiTableList:
      AcpiTableCount = ARRAY_SIZE (PlatformRepo->CmAcpiTableList);

      //
      // Get Pci config space information.
      //
      Status = DynamicPlatRepoGetObject (
                 PlatformRepo->DynamicPlatformRepo,
                 CREATE_CM_ARM_OBJECT_ID (EArmObjPciConfigSpaceInfo),
                 CM_NULL_TOKEN,
                 &CmObjDesc
                 );
      if (Status == EFI_NOT_FOUND) {
        //
        // The last 3 tables are for PCIe. If PCIe information is not
        // present, Kvmtool was launched without the PCIe option.
        // Therefore, reduce the table count by 3.
        //
        AcpiTableCount -= 3;
      } else if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }

      //
      // Get the Gic version.
      //
      Status = DynamicPlatRepoGetObject (
                 PlatformRepo->DynamicPlatformRepo,
                 CREATE_CM_ARM_OBJECT_ID (EArmObjGicDInfo),
                 CM_NULL_TOKEN,
                 &CmObjDesc
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }

      if (((CM_ARM_GICD_INFO *)CmObjDesc.Data)->GicVersion < 3) {
        //
        // IORT is only required for GicV3/4
        //
        AcpiTableCount -= 1;
      }

      Status = HandleCmObject (
                 CmObjectId,
                 PlatformRepo->CmAcpiTableList,
                 (sizeof (PlatformRepo->CmAcpiTableList[0]) * AcpiTableCount),
                 AcpiTableCount,
                 CmObject
                 );
      break;

    default:
      Status = EFI_NOT_FOUND;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: CmObjectId " FMT_CM_OBJECT_ID ". Status = %r\n",
        CmObjectId,
        Status
        ));
      break;
  }

  return Status;
}

/**
  Return an ARM namespace object.

  @param [in]      This        Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId  The Configuration Manager Object ID.
  @param [in]      Token       An optional token identifying the object. If
                               unused this must be CM_NULL_TOKEN.
  @param [in, out] CmObject    Pointer to the Configuration Manager Object
                               descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetArmNameSpaceObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EFI_STATUS                      Status;
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status       = EFI_NOT_FOUND;
  PlatformRepo = This->PlatRepoInfo;

  //
  // First check among the static objects.
  //
  switch (GET_CM_OBJECT_ID (CmObjectId)) {
    case EArmObjPowerManagementProfileInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->PmProfileInfo,
                 sizeof (PlatformRepo->PmProfileInfo),
                 1,
                 CmObject
                 );
      break;

    case EArmObjItsGroup:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->ItsGroupInfo,
                 sizeof (PlatformRepo->ItsGroupInfo),
                 1,
                 CmObject
                 );
      break;

    case EArmObjGicItsIdentifierArray:
      Status = HandleCmObjectRefByToken (
                 This,
                 CmObjectId,
                 PlatformRepo->ItsIdentifierArray,
                 sizeof (PlatformRepo->ItsIdentifierArray),
                 ARRAY_SIZE (PlatformRepo->ItsIdentifierArray),
                 Token,
                 GetItsIdentifierArray,
                 CmObject
                 );
      break;

    case EArmObjRootComplex:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->RootComplexInfo,
                 sizeof (PlatformRepo->RootComplexInfo),
                 1,
                 CmObject
                 );
      break;

    case EArmObjIdMappingArray:
      Status = HandleCmObjectRefByToken (
                 This,
                 CmObjectId,
                 PlatformRepo->DeviceIdMapping,
                 sizeof (PlatformRepo->DeviceIdMapping),
                 ARRAY_SIZE (PlatformRepo->DeviceIdMapping),
                 Token,
                 GetDeviceIdMappingArray,
                 CmObject
                 );
      break;

    default:
      //
      // No match found among the static objects.
      // Check the dynamic objects.
      //
      Status = DynamicPlatRepoGetObject (
                 PlatformRepo->DynamicPlatformRepo,
                 CmObjectId,
                 Token,
                 CmObject
                 );
      break;
  } // switch

  if (Status == EFI_NOT_FOUND) {
    DEBUG ((
      DEBUG_INFO,
      "INFO: CmObjectId " FMT_CM_OBJECT_ID ". Status = %r\n",
      CmObjectId,
      Status
      ));
  } else {
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/**
  Return an OEM namespace object.

  @param [in]      This        Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId  The Configuration Manager Object ID.
  @param [in]      Token       An optional token identifying the object. If
                               unused this must be CM_NULL_TOKEN.
  @param [in, out] CmObject    Pointer to the Configuration Manager Object
                               descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetOemNameSpaceObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;
  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  switch (GET_CM_OBJECT_ID (CmObjectId)) {
    default:
      Status = EFI_NOT_FOUND;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: CmObjectId " FMT_CM_OBJECT_ID ". Status = %r\n",
        CmObjectId,
        Status
        ));
      break;
  }

  return Status;
}

/**
  The GetObject function defines the interface implemented by the
  Configuration Manager Protocol for returning the Configuration
  Manager Objects.

  @param [in]      This        Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId  The Configuration Manager Object ID.
  @param [in]      Token       An optional token identifying the object. If
                               unused this must be CM_NULL_TOKEN.
  @param [in, out] CmObject    Pointer to the Configuration Manager Object
                               descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
EFI_STATUS
EFIAPI
ArmKvmtoolPlatformGetObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EFI_STATUS  Status;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  switch (GET_CM_NAMESPACE_ID (CmObjectId)) {
    case EObjNameSpaceStandard:
      Status = GetStandardNameSpaceObject (This, CmObjectId, Token, CmObject);
      break;
    case EObjNameSpaceArm:
      Status = GetArmNameSpaceObject (This, CmObjectId, Token, CmObject);
      break;
    case EObjNameSpaceOem:
      Status = GetOemNameSpaceObject (This, CmObjectId, Token, CmObject);
      break;
    default:
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Unknown Namespace CmObjectId " FMT_CM_OBJECT_ID ". "
                                                                "Status = %r\n",
        CmObjectId,
        Status
        ));
      break;
  }

  return Status;
}

/**
  The SetObject function defines the interface implemented by the
  Configuration Manager Protocol for updating the Configuration
  Manager Objects.

  @param [in]      This        Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId  The Configuration Manager Object ID.
  @param [in]      Token       An optional token identifying the object. If
                               unused this must be CM_NULL_TOKEN.
  @param [in]      CmObject    Pointer to the Configuration Manager Object
                               descriptor describing the Object.

  @retval EFI_UNSUPPORTED  This operation is not supported.
**/
EFI_STATUS
EFIAPI
ArmKvmtoolPlatformSetObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN        CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  return EFI_UNSUPPORTED;
}

//
// A structure describing the configuration manager protocol interface.
//
STATIC
CONST
EDKII_CONFIGURATION_MANAGER_PROTOCOL  mKvmtoolPlatformConfigManagerProtocol = {
  CREATE_REVISION (1,          0),
  ArmKvmtoolPlatformGetObject,
  ArmKvmtoolPlatformSetObject,
  &mKvmtoolPlatRepositoryInfo
};

/**
  Entrypoint of Configuration Manager Dxe.

  @param  ImageHandle
  @param  SystemTable

  @retval EFI_SUCCESS
  @retval EFI_LOAD_ERROR
  @retval EFI_OUT_OF_RESOURCES
**/
EFI_STATUS
EFIAPI
ConfigurationManagerDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEdkiiConfigurationManagerProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID *)&mKvmtoolPlatformConfigManagerProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get Install Configuration Manager Protocol." \
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = InitializePlatformRepository (
             &mKvmtoolPlatformConfigManagerProtocol
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to initialize the Platform Configuration Repository." \
      " Status = %r\n",
      Status
      ));
    goto ErrorHandler;
  }

  return Status;

ErrorHandler:
  gBS->UninstallProtocolInterface (
         &ImageHandle,
         &gEdkiiConfigurationManagerProtocolGuid,
         (VOID *)&mKvmtoolPlatformConfigManagerProtocol
         );
  return Status;
}

/**
  Unload function for this image.

  @param ImageHandle   Handle for the image of this driver.

  @retval EFI_SUCCESS  Driver unloaded successfully.
  @retval other        Driver can not unloaded.
**/
EFI_STATUS
EFIAPI
ConfigurationManagerDxeUnloadImage (
  IN EFI_HANDLE  ImageHandle
  )
{
  return CleanupPlatformRepository (&mKvmtoolPlatformConfigManagerProtocol);
}
