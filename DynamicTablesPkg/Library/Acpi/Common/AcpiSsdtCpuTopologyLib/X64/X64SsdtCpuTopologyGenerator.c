/** @file
  X64 SSDT Cpu Topology Table Generator Helpers.

  Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.3 Specification - January 2019 - s8.4 Declaring Processors
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/AcpiHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "SsdtCpuTopologyGenerator.h"

/** This macro defines the supported ACPI Processor Status bits.
    The following bits are supported:
    - ACPI_AML_STA_DEVICE_STATUS_PRESET
    - ACPI_AML_STA_DEVICE_STATUS_ENABLED
    - ACPI_AML_STA_DEVICE_STATUS_UI
    - ACPI_AML_STA_DEVICE_STATUS_FUNCTIONING
*/
#define ACPI_AML_STA_PROC_SUPPORTED  (    \
  ACPI_AML_STA_DEVICE_STATUS_PRESET |     \
  ACPI_AML_STA_DEVICE_STATUS_ENABLED |    \
  ACPI_AML_STA_DEVICE_STATUS_UI |         \
  ACPI_AML_STA_DEVICE_STATUS_FUNCTIONING)

/** This macro expands to a function that retrieves the
    Local APIC or X2APIC information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjLocalApicX2ApicInfo,
  CM_X64_LOCAL_APIC_X2APIC_INFO
  );

/** This macro expands to a function that retrieves the
    C-State information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjCstInfo,
  CM_ARCH_COMMON_CST_INFO
  );

/** This macro expands to a function that retrieves the
    C-State dependency information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjCsdInfo,
  CM_ARCH_COMMON_CSD_INFO
  );

/** This macro expands to a function that retrieves the
    P-State PCT information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjPctInfo,
  CM_ARCH_COMMON_PCT_INFO
  );

/** This macro expands to a function that retrieves the
    P-State PSS information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjPssInfo,
  CM_ARCH_COMMON_PSS_INFO
  );

/** This macro expands to a function that retrieves the
    P-State PPC information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjPpcInfo,
  CM_ARCH_COMMON_PPC_INFO
  );

/** This macro expands to a function that retrieves the
    _STA (Device Status) information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjStaInfo,
  CM_ARCH_COMMON_STA_INFO
  );

/**
  This macro expands to a function that retrieves the cross-CM-object-
  reference information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjCmRef,
  CM_ARCH_COMMON_OBJ_REF
  );

/** Add a new entry to the TokenTable and return its index.

  If an entry with Token is already available in the table,
  return its index without adding a new entry.

  @param [in]  TokenTable Pointer to the TokenTable to update.
  @param [in]  Token      New Token entry to add.

  @retval The index of the token entry in the TokenTable.
**/
STATIC
UINT32
EFIAPI
TokenTableAdd (
  IN  TOKEN_TABLE      *TokenTable,
  IN  CM_OBJECT_TOKEN  Token
  )
{
  UINT32  Index;

  if ((TokenTable == NULL) || (TokenTable->Table == NULL)) {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return 0;
  }

  // Search if there is already an entry with this Token.
  for (Index = 0; Index < TokenTable->LastIndex; Index++) {
    if (TokenTable->Table[Index] == Token) {
      return Index;
    }
  }

  // If no, create a new entry.
  TokenTable->Table[TokenTable->LastIndex] = Token;
  return TokenTable->LastIndex++;
}

/** Free the memory allocated for the TokenTable.

  @param [in]  TokenTable  Pointer to the TokenTable to free.
**/
STATIC
VOID
EFIAPI
TokenTableFree (
  IN  TOKEN_TABLE  *TokenTable
  )
{
  if (TokenTable == NULL) {
    return;
  }

  if (TokenTable->Table != NULL) {
    FreePool (TokenTable->Table);
  }

  TokenTable->Table     = NULL;
  TokenTable->LastIndex = 0;
}

/** Update the TokenTable.
  Allocate memory for the TokenTable and add the CstToken entries.

  @param [in]  TokenTable Pointer to the TokenTable to initialize.
  @param [in]  LocalApic  Pointer to the Local APIC or X2APIC information.
  @param [in]  Count      Number of entries to allocate in the TokenTable.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
TokenTableUpdate (
  IN  TOKEN_TABLE                    *TokenTable,
  IN  CM_X64_LOCAL_APIC_X2APIC_INFO  *LocalApic,
  IN  UINT32                         Count
  )
{
  UINT32  Index;

  if ((TokenTable == NULL) ||
      (LocalApic == NULL)   ||
      (Count == 0)        ||
      (Count >= MAX_NODE_COUNT))
  {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  TokenTable->Table = AllocateZeroPool (sizeof (CM_OBJECT_TOKEN) * Count);
  if (TokenTable->Table == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  TokenTable->LastIndex = 0;

  for (Index = 0; Index < Count; Index++) {
    if (LocalApic[Index].CstToken != CM_NULL_TOKEN) {
      TokenTableAdd (TokenTable, LocalApic[Index].CstToken);
    }
  }

  return EFI_SUCCESS;
}

/** Generate the name for a C-State node in the format Cxxx,
  where xxx represents the C-state Index:
  CSTx  - for C-state indices less than 0xF.
  CSxx  - for C-state indices between 0xF and 0xFF.
  Cxxx  - for C-state indices greater than 0xFF.

  @param [in, out] AslName     Buffer to store the generated name.
  @param [in]      AslNameSize Size of the buffer.
  @param [in]      Index       Index of the C-State in the TokenTable.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
GenerateCstAslName (
  IN OUT CHAR8  *AslName,
  IN UINTN      AslNameSize,
  IN UINT32     Index
  )
{
  if ((AslName == NULL) || (AslNameSize < sizeof (CSTS_SCOPE))) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (AslName, CSTS_SCOPE, AslNameSize);

  AslName[3] = AsciiFromHex (Index & 0xF);

  if (Index > 0xF) {
    AslName[2] = AsciiFromHex ((Index >> 4) & 0xF);
  }

  if (Index > 0xFF) {
    AslName[1] = AsciiFromHex ((Index >> 8) & 0xF);
  }

  return EFI_SUCCESS;
}

/** Generate all the C-States under the '_SB_.CSTS' scope.

  This function generates the following ASL code:
  Scope (\_SB) {
    Scope(CSTS) {
      Name (CST0, Package() {
        X, // Count
        Package() {
          [An CST state]
        },
        Package() {
          [Another CST state]
        },
      } // Name CST0

      Name (CST1, Package() {
        ...
      } // Name CST1
    } // Scope CSTS
    ...
  } // Scope /_SB

  The c-states are fetched from the Configuration Manager.
  The names of the c-states are generated from the TokenTable.

  @param [in]  CstTokenTable    Pointer to the TokenTable containing
                                the C-State tokens.
  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager
                                Protocol Interface.
  @param [in] ScopeNode         Scope node handle ('CSTS' scope).

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
GenerateCstStates (
  IN  TOKEN_TABLE                                         *CstTokenTable,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        AML_OBJECT_NODE_HANDLE                        ScopeNode
  )
{
  AML_OBJECT_NODE_HANDLE   CstNode;
  CHAR8                    AslName[AML_NAME_SEG_SIZE + 1];
  CM_ARCH_COMMON_CST_INFO  *CstInfo;
  CM_ARCH_COMMON_OBJ_REF   *CstPkgRefInfo;
  CM_ARCH_COMMON_OBJ_REF   *CstRefInfo;
  EFI_STATUS               Status;
  UINT32                   CstPkgRefIndex;
  UINT32                   CstPkgRefInfoCount;
  UINT32                   CstRefIndex;
  UINT32                   CstRefInfoCount;
  UINT32                   Index;

  if ((CstTokenTable == NULL) || (CfgMgrProtocol == NULL) || (ScopeNode == NULL)) {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < CstTokenTable->LastIndex; Index++) {
    Status = GenerateCstAslName (AslName, AML_NAME_SEG_SIZE + 1, Index);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    Status = AmlCreateCstNode (AslName, ScopeNode, &CstNode);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    Status = GetEArchCommonObjCmRef (
               CfgMgrProtocol,
               CstTokenTable->Table[Index],
               &CstRefInfo,
               &CstRefInfoCount
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    for (CstRefIndex = 0; CstRefIndex < CstRefInfoCount; CstRefIndex++) {
      /// Each CST object contains the references to the CST packages object
      Status = GetEArchCommonObjCmRef (
                 CfgMgrProtocol,
                 CstRefInfo[CstRefIndex].ReferenceToken,
                 &CstPkgRefInfo,
                 &CstPkgRefInfoCount
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }

      for (CstPkgRefIndex = 0; CstPkgRefIndex < CstPkgRefInfoCount; CstPkgRefIndex++) {
        Status = GetEArchCommonObjCstInfo (
                   CfgMgrProtocol,
                   CstPkgRefInfo[CstPkgRefIndex].ReferenceToken,
                   &CstInfo,
                   NULL
                   );
        if (EFI_ERROR (Status)) {
          ASSERT_EFI_ERROR (Status);
          return Status;
        }

        Status = AmlAddCstState (
                   CstInfo,
                   CstNode
                   );
        if (EFI_ERROR (Status)) {
          ASSERT_EFI_ERROR (Status);
          return Status;
        }
      }
    } // for CstRefIndex
  } // for Index

  return EFI_SUCCESS;
}

/** Create and add an _CST method to cpu node.

  For instance, transform an AML node from:
  Device (C002)
  {
      Name (_UID, 2)
      Name (_HID, "ACPI0007")
  }

  To:
  Device (C002)
  {
      Name (_UID, 2)
      Name (_HID, "ACPI0007")
      Method (_CST, 0, NotSerialized)
      {
          Return (\_SB.CSTS.CST1)
      }
  }

  @param [in]  CstTokenTable    Pointer to the TokenTable containing the C-State tokens.
  @param [in]  CstToken         C-State token to add to the CPU node.
  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager
                                Protocol Interface.
  @param [in]  Node             CPU node handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
CreateAmlCstMethod (
  IN  TOKEN_TABLE                                         *CstTokenTable,
  IN  CM_OBJECT_TOKEN                                     CstToken,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  AML_OBJECT_NODE_HANDLE                              *Node
  )
{
  CHAR8       AslName[AML_NAME_SEG_SIZE + 1];
  CHAR8       AslNameFull[CSTS_SCOPE_PREFIX_SIZE + AML_NAME_SEG_SIZE + 1];
  EFI_STATUS  Status;
  UINT32      Index;

  if ((CstTokenTable == NULL) || (CfgMgrProtocol == NULL) || (Node == NULL)) {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < CstTokenTable->LastIndex; Index++) {
    if (CstTokenTable->Table[Index] == CstToken) {
      break;
    }
  }

  if (Index == CstTokenTable->LastIndex) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GenerateCstAslName (AslName, AML_NAME_SEG_SIZE + 1, Index);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  CopyMem (AslNameFull, CSTS_SCOPE_PREFIX, CSTS_SCOPE_PREFIX_SIZE);
  CopyMem (&AslNameFull[CSTS_SCOPE_PREFIX_SIZE - 1], AslName, AML_NAME_SEG_SIZE);
  AslNameFull[CSTS_SCOPE_PREFIX_SIZE + AML_NAME_SEG_SIZE - 1] = '\0';
  // ASL:
  // Method (_CST, 0) {
  //   Return ([AslName])
  // }
  Status = AmlCodeGenMethodRetNameString (
             "_CST",
             AslNameFull,
             0,
             FALSE,
             0,
             Node,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/**
  Create a C-State Dependency (CSD) object.

  @param [in]  CstTokenTable    Pointer to the TokenTable containing the C-State tokens.
  @param [in]  CsdToken         C-State Dependency token to add to the CPU node.
  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager
                                Protocol Interface.
  @param [in]  Node             CPU node handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_NOT_FOUND           No CSD information found.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
CreateAmlCsdObject (
  IN  TOKEN_TABLE                                         *CstTokenTable,
  IN  CM_OBJECT_TOKEN                                     CsdToken,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  AML_OBJECT_NODE_HANDLE                              *Node
  )
{
  AML_CSD_INFO             *CsdInfo;
  CM_ARCH_COMMON_CSD_INFO  *CmCsdInfo;
  CM_ARCH_COMMON_OBJ_REF   *CstPkgRefInfo;
  CM_ARCH_COMMON_OBJ_REF   *CstRefInfo;
  EFI_STATUS               Status;
  UINT32                   CsdIndex;
  UINT32                   CsdNumEntries;
  UINT32                   CstIndex;
  UINT32                   CstPkgIndex;
  UINT32                   CstPkgRefIndex;
  UINT32                   CstPkgRefInfoCount;
  UINT32                   CstRefInfoCount;

  Status = GetEArchCommonObjCsdInfo (
             CfgMgrProtocol,
             CsdToken,
             &CmCsdInfo,
             &CsdNumEntries
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  if (CsdNumEntries == 0) {
    ASSERT_EFI_ERROR (EFI_NOT_FOUND);
    return EFI_NOT_FOUND;
  }

  CsdInfo = AllocateZeroPool (sizeof (AML_CSD_INFO) * CsdNumEntries);
  if (CsdInfo == NULL) {
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    Status = EFI_OUT_OF_RESOURCES;
    return Status;
  }

  for (CsdIndex = 0; CsdIndex < CsdNumEntries; CsdIndex++) {
    CsdInfo[CsdIndex].Revision      = CmCsdInfo[CsdIndex].Revision;
    CsdInfo[CsdIndex].Domain        = CmCsdInfo[CsdIndex].Domain;
    CsdInfo[CsdIndex].CoordType     = CmCsdInfo[CsdIndex].CoordType;
    CsdInfo[CsdIndex].NumProcessors = CmCsdInfo[CsdIndex].NumProcessors;
    if (CmCsdInfo[CsdIndex].CstPkgRefToken == CM_NULL_TOKEN) {
      ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
      Status = EFI_INVALID_PARAMETER;
      goto return_handler;
    }

    /// Initialize to maximum value until the reference token is found
    CsdInfo[CsdIndex].Index = MAX_UINT32;
    for (
         CstIndex = 0;
         ((CstIndex < CstTokenTable->LastIndex) && (CsdInfo[CsdIndex].Index == MAX_UINT32));
         CstIndex++
         )
    {
      Status = GetEArchCommonObjCmRef (
                 CfgMgrProtocol,
                 CstTokenTable->Table[CstIndex],
                 &CstRefInfo,
                 &CstRefInfoCount
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto return_handler;
      }

      for (
           CstPkgIndex = 0;
           ((CstPkgIndex < CstRefInfoCount) && (CsdInfo[CsdIndex].Index == MAX_UINT32));
           CstPkgIndex++
           )
      {
        Status = GetEArchCommonObjCmRef (
                   CfgMgrProtocol,
                   CstRefInfo[CstPkgIndex].ReferenceToken,
                   &CstPkgRefInfo,
                   &CstPkgRefInfoCount
                   );
        if (EFI_ERROR (Status)) {
          ASSERT_EFI_ERROR (Status);
          goto return_handler;
        }

        for (
             CstPkgRefIndex = 0;
             ((CstPkgRefIndex < CstPkgRefInfoCount) && (CsdInfo[CsdIndex].Index == MAX_UINT32));
             CstPkgRefIndex++
             )
        {
          if (CstPkgRefInfo[CstPkgRefIndex].ReferenceToken == CmCsdInfo[CsdIndex].CstPkgRefToken) {
            CsdInfo[CsdIndex].Index = CstPkgIndex;
            break;
          }
        }
      }
    }

    if (CsdInfo[CsdIndex].Index == MAX_UINT32) {
      ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
      Status = EFI_INVALID_PARAMETER;
      goto return_handler;
    }
  }

  Status = AmlCreateCsdNode (
             CsdInfo,
             CsdNumEntries,
             Node,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
  }

return_handler:
  if (CsdInfo != NULL) {
    FreePool (CsdInfo);
  }

  return Status;
}

/**
  Create a standard ACPI CPU node.
  Collect the information from the Configuration Manager.
  Optionally creates a C-State, P-State and _STA method.

  @param [in]  Generator          The SSDT Cpu Topology generator.
  @param [in]  CfgMgrProtocol     Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in]  ScopeNode          Scope node handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
CreateTopologyFromIntC (
  IN        ACPI_CPU_TOPOLOGY_GENERATOR                   *Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        AML_OBJECT_NODE_HANDLE                        ScopeNode
  )
{
  AML_OBJECT_NODE_HANDLE         CpuNode;
  AML_OBJECT_NODE_HANDLE         CstsScopeNode;
  CM_ARCH_COMMON_PCT_INFO        *PctInfo;
  CM_ARCH_COMMON_PPC_INFO        *PpcInfo;
  CM_ARCH_COMMON_PSS_INFO        *PssInfo;
  CM_ARCH_COMMON_STA_INFO        *StaInfo;
  CM_X64_LOCAL_APIC_X2APIC_INFO  *LocalApicX2ApicInfo;
  EFI_STATUS                     Status;
  TOKEN_TABLE                    CstTokenTable;
  UINT32                         Index;
  UINT32                         LocalApicX2ApicCount;
  UINT32                         PssNumEntries;

  if ((Generator == NULL) || (CfgMgrProtocol == NULL) || (ScopeNode == NULL)) {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  LocalApicX2ApicCount = 0;
  Status               = GetEX64ObjLocalApicX2ApicInfo (
                           CfgMgrProtocol,
                           CM_NULL_TOKEN,
                           &LocalApicX2ApicInfo,
                           &LocalApicX2ApicCount
                           );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = TokenTableUpdate (
             &CstTokenTable,
             LocalApicX2ApicInfo,
             LocalApicX2ApicCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  /// if C-State are provided, create the CSTS scope and C-states
  if (CstTokenTable.LastIndex > 0) {
    Status = AmlCodeGenScope (CSTS_SCOPE, ScopeNode, &CstsScopeNode);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto return_handler;
    }

    Status = GenerateCstStates (&CstTokenTable, CfgMgrProtocol, CstsScopeNode);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto return_handler;
    }
  }

  /// for each processor object, create an AML node.
  for (Index = 0; Index < LocalApicX2ApicCount; Index++) {
    Status = CreateAmlCpu (
               Generator,
               ScopeNode,
               LocalApicX2ApicInfo[Index].AcpiProcessorUid,
               Index,
               &CpuNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto return_handler;
    }

    ///
    /// Check for optional tokens and add them to the CPU node.
    ///
    if (LocalApicX2ApicInfo[Index].CstToken != CM_NULL_TOKEN) {
      Status = CreateAmlCstMethod (
                 &CstTokenTable,
                 LocalApicX2ApicInfo[Index].CstToken,
                 CfgMgrProtocol,
                 CpuNode
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto return_handler;
      }

      if (LocalApicX2ApicInfo[Index].CsdToken != CM_NULL_TOKEN) {
        Status = CreateAmlCsdObject (
                   &CstTokenTable,
                   LocalApicX2ApicInfo[Index].CsdToken,
                   CfgMgrProtocol,
                   CpuNode
                   );
        if (EFI_ERROR (Status)) {
          ASSERT_EFI_ERROR (Status);
          goto return_handler;
        }
      }
    }

    ///
    /// Check for optional tokens and add them to the CPU node.
    ///
    if ((LocalApicX2ApicInfo[Index].PctToken != CM_NULL_TOKEN) &&
        (LocalApicX2ApicInfo[Index].PssToken != CM_NULL_TOKEN) &&
        (LocalApicX2ApicInfo[Index].PpcToken != CM_NULL_TOKEN))
    {
      Status = GetEArchCommonObjPctInfo (
                 CfgMgrProtocol,
                 LocalApicX2ApicInfo[Index].PctToken,
                 &PctInfo,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto return_handler;
      }

      Status = GetEArchCommonObjPssInfo (
                 CfgMgrProtocol,
                 LocalApicX2ApicInfo[Index].PssToken,
                 &PssInfo,
                 &PssNumEntries
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto return_handler;
      }

      Status = GetEArchCommonObjPpcInfo (
                 CfgMgrProtocol,
                 LocalApicX2ApicInfo[Index].PpcToken,
                 &PpcInfo,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto return_handler;
      }

      Status = AmlCreatePctNode (
                 PctInfo,
                 CpuNode,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto return_handler;
      }

      Status = AmlCreatePssNode (
                 PssInfo,
                 PssNumEntries,
                 CpuNode,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto return_handler;
      }

      Status = AmlCodeGenMethodRetInteger (
                 "_PPC",
                 PpcInfo->PstateCount,
                 0,
                 FALSE,
                 0,
                 CpuNode,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto return_handler;
      }
    } else if ((LocalApicX2ApicInfo[Index].PctToken != CM_NULL_TOKEN) ||
               (LocalApicX2ApicInfo[Index].PssToken != CM_NULL_TOKEN) ||
               (LocalApicX2ApicInfo[Index].PpcToken != CM_NULL_TOKEN))
    {
      DEBUG ((
        DEBUG_WARN,
        "Missing PCT, PSS or PPC token for processor %d\n",
        LocalApicX2ApicInfo[Index].AcpiProcessorUid
        ));
    }

    if (LocalApicX2ApicInfo[Index].PsdToken != CM_NULL_TOKEN) {
      Status = CreateAmlPsdNode (Generator, CfgMgrProtocol, LocalApicX2ApicInfo[Index].PsdToken, CpuNode);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }
    }

    if (LocalApicX2ApicInfo[Index].CpcToken != CM_NULL_TOKEN) {
      Status = CreateAmlCpcNode (Generator, CfgMgrProtocol, LocalApicX2ApicInfo[Index].CpcToken, CpuNode);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }
    }

    if (LocalApicX2ApicInfo[Index].StaToken != CM_NULL_TOKEN) {
      Status = GetEArchCommonObjStaInfo (
                 CfgMgrProtocol,
                 LocalApicX2ApicInfo[Index].StaToken,
                 &StaInfo,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }

      /// check STA bits
      if ((StaInfo->DeviceStatus & ~(ACPI_AML_STA_PROC_SUPPORTED)) != 0) {
        DEBUG ((
          DEBUG_ERROR,
          "Unsupported STA bits set for processor %d\n",
          LocalApicX2ApicInfo[Index].AcpiProcessorUid
          ));
        return EFI_UNSUPPORTED;
      }

      Status = AmlCodeGenMethodRetInteger ("_STA", StaInfo->DeviceStatus, 0, FALSE, 0, CpuNode, NULL);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }
    }
  }

return_handler:
  TokenTableFree (&CstTokenTable);
  return Status;
}

/** Add arch specific information to a CPU node in the asl description.

  @param [in]  Generator          The SSDT Cpu Topology generator.
  @param [in]  CfgMgrProtocol     Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in]  AcpiIdObjectToken  AcpiIdObjectToken identifying the CPU to fetch the
                                  other fields from.
  @param [in]  CpuName            Value used to generate the CPU node name.
  @param [out] CpuNode            CPU Node to which the ET device node is
                                  attached.

  @retval EFI_UNSUPPORTED         Not supported
**/
EFI_STATUS
EFIAPI
AddArchAmlCpuInfo (
  IN  ACPI_CPU_TOPOLOGY_GENERATOR                         *Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CM_OBJECT_TOKEN                                     AcpiIdObjectToken,
  IN  UINT32                                              CpuName,
  OUT  AML_OBJECT_NODE_HANDLE                             *CpuNode
  )
{
  return EFI_UNSUPPORTED;
}

/** Get generic interrupt information from arch specific CM objects.

  @param [in]  CfgMgrProtocol     Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in]  AcpiIdObjectToken  AcpiIdObjectToken identifying the CPU to fetch the
                                  other fields from.
  @param [out] AcpiProcessorUid   AcpiProcessorUid of the CPU identified by
                                  the AcpiIdObjectToken.
  @param [out] CpcToken           CpcToken of the CPU identified by
                                  the AcpiIdObjectToken.
  @param [out] PsdToken           PsdToken of the CPU identified by
                                  the AcpiIdObjectToken.

  @retval EFI_UNSUPPORTED         Not supported
**/
EFI_STATUS
EFIAPI
GetIntCInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CM_OBJECT_TOKEN                                     AcpiIdObjectToken,
  OUT UINT32                                              *AcpiProcessorUid,
  OUT CM_OBJECT_TOKEN                                     *CpcToken,
  OUT CM_OBJECT_TOKEN                                     *PsdToken
  )
{
  return EFI_UNSUPPORTED;
}
