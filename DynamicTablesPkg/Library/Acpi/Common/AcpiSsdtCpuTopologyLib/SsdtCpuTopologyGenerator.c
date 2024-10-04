/** @file
  SSDT Cpu Topology Table Generator.

  Copyright (c) 2021 - 2023, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.3 Specification - January 2019 - s8.4 Declaring Processors
    - ACPI for CoreSight version 1.2 Platform Design Document
      (https://developer.arm.com/documentation/den0067/a/?lang=en)

  @par Glossary:
    - ETE - Embedded Trace Extension.
    - ETM - Embedded Trace Macrocell.
**/

#include <Library/AcpiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/AcpiHelperLib.h>
#include <Library/TableHelperLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "SsdtCpuTopologyGenerator.h"

/** SSDT Cpu Topology Table Generator.

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjProcHierarchyInfo (OPTIONAL) along with
  - EArchCommonObjCmRef (OPTIONAL)
  - EArchCommonObjLpiInfo (OPTIONAL)
  - EArchCommonObjPsdInfo (OPTIONAL)
*/

/**
  This macro expands to a function that retrieves the Processor Hierarchy
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjProcHierarchyInfo,
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO
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

/**
  This macro expands to a function that retrieves the Lpi
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjLpiInfo,
  CM_ARCH_COMMON_LPI_INFO
  );

/**
  This macro expands to a function that retrieves the CPC
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjCpcInfo,
  CM_ARCH_COMMON_CPC_INFO
  );

/**
  This macro expands to a function that retrieves the PSD
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjPsdInfo,
  CM_ARCH_COMMON_PSD_INFO
  );

/** Initialize the TokenTable.

  One entry should be allocated for each CM_ARCH_COMMON_PROC_HIERARCHY_INFO
  structure of the platform. The TokenTable allows to have a mapping:
  Index <-> CM_OBJECT_TOKEN (to CM_ARCH_COMMON_LPI_INFO structures).

  There will always be less sets of Lpi states (CM_ARCH_COMMON_OBJ_REF)
  than the number of cpus/clusters (CM_ARCH_COMMON_PROC_HIERARCHY_INFO).

  @param [in]  Generator  The SSDT Cpu Topology generator.
  @param [in]  Count      Number of entries to allocate in the TokenTable.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
TokenTableInitialize (
  IN  ACPI_CPU_TOPOLOGY_GENERATOR  *Generator,
  IN  UINT32                       Count
  )
{
  CM_OBJECT_TOKEN  *Table;

  if ((Generator == NULL) ||
      (Count == 0)        ||
      (Count >= MAX_NODE_COUNT))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Table = AllocateZeroPool (sizeof (CM_OBJECT_TOKEN) * Count);
  if (Table == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  Generator->TokenTable.Table = Table;

  return EFI_SUCCESS;
}

/** Free the TokenTable.

  @param [in]  Generator    The SSDT Cpu Topology generator.
**/
STATIC
VOID
EFIAPI
TokenTableFree (
  IN  ACPI_CPU_TOPOLOGY_GENERATOR  *Generator
  )
{
  ASSERT (Generator != NULL);
  ASSERT (Generator->TokenTable.Table != NULL);

  if (Generator->TokenTable.Table != NULL) {
    FreePool (Generator->TokenTable.Table);
  }
}

/** Add a new entry to the TokenTable and return its index.

  If an entry with Token is already available in the table,
  return its index without adding a new entry.

  @param [in]  Generator  The SSDT Cpu Topology generator.
  @param [in]  Token      New Token entry to add.

  @retval The index of the token entry in the TokenTable.
**/
STATIC
UINT32
EFIAPI
TokenTableAdd (
  IN  ACPI_CPU_TOPOLOGY_GENERATOR  *Generator,
  IN  CM_OBJECT_TOKEN              Token
  )
{
  CM_OBJECT_TOKEN  *Table;
  UINT32           Index;
  UINT32           LastIndex;

  ASSERT (Generator != NULL);
  ASSERT (Generator->TokenTable.Table != NULL);

  Table     = Generator->TokenTable.Table;
  LastIndex = Generator->TokenTable.LastIndex;

  // Search if there is already an entry with this Token.
  for (Index = 0; Index < LastIndex; Index++) {
    if (Table[Index] == Token) {
      return Index;
    }
  }

  ASSERT (LastIndex < MAX_NODE_COUNT);
  ASSERT (LastIndex < Generator->ProcNodeCount);

  // If no, create a new entry.
  Table[LastIndex] = Token;

  return Generator->TokenTable.LastIndex++;
}

/** Write a string 'Xxxx\0' in AslName (5 bytes long),
  with 'X' being the leading char of the name, and
  with 'xxx' being Value in hexadecimal.

  As 'xxx' in hexadecimal represents a number on 12 bits,
  we have Value < (1 << 12).

  @param [in]       LeadChar  Leading char of the name.
  @param [in]       Value     Hex value of the name.
                              Must be lower than (2 << 12).
  @param [in, out]  AslName   Pointer to write the 'Xxxx' string to.
                              Must be at least 5 bytes long.

  @retval EFI_SUCCESS               Success.
  @retval EFI_INVALID_PARAMETER     Invalid parameter.
**/
EFI_STATUS
EFIAPI
WriteAslName (
  IN      CHAR8   LeadChar,
  IN      UINT32  Value,
  IN OUT  CHAR8   *AslName
  )
{
  UINT8  Index;

  if ((Value >= MAX_NODE_COUNT)  ||
      (AslName == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  AslName[0]                 = LeadChar;
  AslName[AML_NAME_SEG_SIZE] = '\0';

  for (Index = 0; Index < AML_NAME_SEG_SIZE - 1; Index++) {
    AslName[AML_NAME_SEG_SIZE - Index - 1] =
      AsciiFromHex (((Value >> (4 * Index)) & 0xF));
  }

  return EFI_SUCCESS;
}

/** Create and add an _PSD Node to Cpu Node.

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
      Name (_PSD, Package()
      {
        NumEntries,      // Integer
        Revision,        // Integer
        Domain,          // Integer
        CoordType,       // Integer
        NumProcessors,   // Integer
      })
  }

  @param [in]  Generator              The SSDT Cpu Topology generator.
  @param [in]  CfgMgrProtocol         Pointer to the Configuration Manager
                                      Protocol Interface.
  @param [in]  PsdToken               Token to identify the Psd information.
  @param [in]  Node                   CPU Node to which the _CPC node is
                                      attached.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
CreateAmlPsdNode (
  IN  ACPI_CPU_TOPOLOGY_GENERATOR                         *Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CM_OBJECT_TOKEN                                     PsdToken,
  IN  AML_OBJECT_NODE_HANDLE                              *Node
  )
{
  EFI_STATUS               Status;
  CM_ARCH_COMMON_PSD_INFO  *PsdInfo;

  Status = GetEArchCommonObjPsdInfo (
             CfgMgrProtocol,
             PsdToken,
             &PsdInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = AmlCreatePsdNode (
             PsdInfo,
             Node,
             NULL
             );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Create and add an _CPC Node to Cpu Node.

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
      Name(_CPC, Package()
      {
        NumEntries,                              // Integer
        Revision,                                // Integer
        HighestPerformance,                      // Integer or Buffer (Resource Descriptor)
        NominalPerformance,                      // Integer or Buffer (Resource Descriptor)
        LowestNonlinearPerformance,              // Integer or Buffer (Resource Descriptor)
        LowestPerformance,                       // Integer or Buffer (Resource Descriptor)
        GuaranteedPerformanceRegister,           // Buffer (Resource Descriptor)
        DesiredPerformanceRegister ,             // Buffer (Resource Descriptor)
        MinimumPerformanceRegister ,             // Buffer (Resource Descriptor)
        MaximumPerformanceRegister ,             // Buffer (Resource Descriptor)
        PerformanceReductionToleranceRegister,   // Buffer (Resource Descriptor)
        TimeWindowRegister,                      // Buffer (Resource Descriptor)
        CounterWraparoundTime,                   // Integer or Buffer (Resource Descriptor)
        ReferencePerformanceCounterRegister,     // Buffer (Resource Descriptor)
        DeliveredPerformanceCounterRegister,     // Buffer (Resource Descriptor)
        PerformanceLimitedRegister,              // Buffer (Resource Descriptor)
        CPPCEnableRegister                       // Buffer (Resource Descriptor)
        AutonomousSelectionEnable,               // Integer or Buffer (Resource Descriptor)
        AutonomousActivityWindowRegister,        // Buffer (Resource Descriptor)
        EnergyPerformancePreferenceRegister,     // Buffer (Resource Descriptor)
        ReferencePerformance                     // Integer or Buffer (Resource Descriptor)
        LowestFrequency,                         // Integer or Buffer (Resource Descriptor)
        NominalFrequency                         // Integer or Buffer (Resource Descriptor)
      })
  }

  @param [in]  Generator              The SSDT Cpu Topology generator.
  @param [in]  CfgMgrProtocol         Pointer to the Configuration Manager
                                      Protocol Interface.
  @param [in]  CpcToken               CPC token of the INTC info
                                      describing the Cpu.
  @param [in]  Node                   CPU Node to which the _CPC node is
                                      attached.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
CreateAmlCpcNode (
  IN  ACPI_CPU_TOPOLOGY_GENERATOR                         *Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CM_OBJECT_TOKEN                                     CpcToken,
  IN  AML_OBJECT_NODE_HANDLE                              *Node
  )
{
  EFI_STATUS               Status;
  CM_ARCH_COMMON_CPC_INFO  *CpcInfo;

  Status = GetEArchCommonObjCpcInfo (
             CfgMgrProtocol,
             CpcToken,
             &CpcInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCreateCpcNode (
             CpcInfo,
             Node,
             NULL
             );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Create and add an _LPI method to Cpu/Cluster Node.

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
      Method (_LPI, 0, NotSerialized)
      {
          Return (\_SB.L003)
      }
  }

  @param [in]  Generator              The SSDT Cpu Topology generator.
  @param [in]  ProcHierarchyNodeInfo  CM_ARCH_COMMON_PROC_HIERARCHY_INFO
                                       describing the Cpu.
  @param [in]  Node                   Node to which the _LPI method is
                                      attached. Can represent a Cpu or a
                                      Cluster.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
CreateAmlLpiMethod (
  IN  ACPI_CPU_TOPOLOGY_GENERATOR         *Generator,
  IN  CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *ProcHierarchyNodeInfo,
  IN  AML_OBJECT_NODE_HANDLE              *Node
  )
{
  EFI_STATUS  Status;
  UINT32      TokenIndex;
  CHAR8       AslName[SB_SCOPE_PREFIX_SIZE + AML_NAME_SEG_SIZE];

  ASSERT (Generator != NULL);
  ASSERT (ProcHierarchyNodeInfo != NULL);
  ASSERT (ProcHierarchyNodeInfo->LpiToken != CM_NULL_TOKEN);
  ASSERT (Node != NULL);

  TokenIndex = TokenTableAdd (Generator, ProcHierarchyNodeInfo->LpiToken);

  CopyMem (AslName, SB_SCOPE_PREFIX, SB_SCOPE_PREFIX_SIZE);

  Status = WriteAslName (
             'L',
             TokenIndex,
             AslName + SB_SCOPE_PREFIX_SIZE - 1
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // ASL:
  // Method (_LPI, 0) {
  //   Return ([AslName])
  // }
  Status = AmlCodeGenMethodRetNameString (
             "_LPI",
             AslName,
             0,
             FALSE,
             0,
             Node,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
  }

  return Status;
}

/** Generate all the Lpi states under the '_SB' scope.

  This function generates the following ASL code:
  Scope (\_SB) {
    Name (L000, Package() {
      0, // Version
      0, // Level Index
      X, // Count
      Package() {
        [An Lpi state]
      },
      Package() {
        [Another Lpi state]
      },
    } // Name L000

    Name (L001, Package() {
      ...
    } // Name L001

    ...
  } // Scope /_SB

  The Lpi states are fetched from the Configuration Manager.
  The names of the Lpi states are generated from the TokenTable.

  @param [in]  Generator        The SSDT Cpu Topology generator.
  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager
                                Protocol Interface.
  @param [in] ScopeNode         Scope node handle ('\_SB' scope).

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
GenerateLpiStates (
  IN        ACPI_CPU_TOPOLOGY_GENERATOR                   *Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        AML_OBJECT_NODE_HANDLE                        ScopeNode
  )
{
  EFI_STATUS  Status;

  UINT32  Index;
  UINT32  LastIndex;

  AML_OBJECT_NODE_HANDLE   LpiNode;
  CM_ARCH_COMMON_OBJ_REF   *LpiRefInfo;
  UINT32                   LpiRefInfoCount;
  UINT32                   LpiRefIndex;
  CM_ARCH_COMMON_LPI_INFO  *LpiInfo;
  CHAR8                    AslName[AML_NAME_SEG_SIZE + 1];

  ASSERT (Generator != NULL);
  ASSERT (Generator->TokenTable.Table != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (ScopeNode != NULL);

  LastIndex = Generator->TokenTable.LastIndex;

  // For each entry in the TokenTable, create a name in the AML namespace
  // under SB_SCOPE, to store the Lpi states associated with the LpiToken.
  for (Index = 0; Index < LastIndex; Index++) {
    Status = WriteAslName ('L', Index, AslName);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    // We do not support the LevelId field for now, let it to 0.
    Status = AmlCreateLpiNode (AslName, 0, 0, ScopeNode, &LpiNode);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    // Fetch the LPI objects referenced by the token.
    Status = GetEArchCommonObjCmRef (
               CfgMgrProtocol,
               Generator->TokenTable.Table[Index],
               &LpiRefInfo,
               &LpiRefInfoCount
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    for (LpiRefIndex = 0; LpiRefIndex < LpiRefInfoCount; LpiRefIndex++) {
      // For each CM_ARCH_COMMON_LPI_INFO referenced by the token,
      // add an Lpi state.
      Status = GetEArchCommonObjLpiInfo (
                 CfgMgrProtocol,
                 LpiRefInfo[LpiRefIndex].ReferenceToken,
                 &LpiInfo,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        return Status;
      }

      Status = AmlAddLpiState (
                 LpiInfo->MinResidency,
                 LpiInfo->WorstCaseWakeLatency,
                 LpiInfo->Flags,
                 LpiInfo->ArchFlags,
                 LpiInfo->ResCntFreq,
                 LpiInfo->EnableParentState,
                 LpiInfo->IsInteger ?
                 NULL :
                 &LpiInfo->RegisterEntryMethod,
                 LpiInfo->IsInteger ?
                 LpiInfo->IntegerEntryMethod :
                 0,
                 &LpiInfo->ResidencyCounterRegister,
                 &LpiInfo->UsageCounterRegister,
                 LpiInfo->StateName,
                 LpiNode
                 );
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        return Status;
      }
    } // for LpiRefIndex
  } // for Index

  return EFI_SUCCESS;
}

/** Create a Cpu in the AML namespace.

  This generates the following ASL code:
  Device (C002)
  {
      Name (_UID, 2)
      Name (_HID, "ACPI0007")
  }

  @param [in]  Generator         The SSDT Cpu Topology generator.
  @param [in]  ParentNode        Parent node to attach the Cpu node to.
  @param [in]  AcpiProcessorUid  ACPI processor UID of the CPU.
  @param [in]  CpuName           Value used to generate the node name.
  @param [out] CpuNodePtr        If not NULL, return the created Cpu node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
CreateAmlCpu (
  IN   ACPI_CPU_TOPOLOGY_GENERATOR  *Generator,
  IN   AML_NODE_HANDLE              ParentNode,
  IN   UINT32                       AcpiProcessorUid,
  IN   UINT32                       CpuName,
  OUT  AML_OBJECT_NODE_HANDLE       *CpuNodePtr OPTIONAL
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  CpuNode;
  CHAR8                   AslName[AML_NAME_SEG_SIZE + 1];

  ASSERT (Generator != NULL);
  ASSERT (ParentNode != NULL);

  Status = WriteAslName ('C', CpuName, AslName);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenDevice (AslName, ParentNode, &CpuNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenNameInteger (
             "_UID",
             AcpiProcessorUid,
             CpuNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenNameString (
             "_HID",
             ACPI_HID_PROCESSOR_DEVICE,
             CpuNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // If requested, return the handle to the CpuNode.
  if (CpuNodePtr != NULL) {
    *CpuNodePtr = CpuNode;
  }

  return Status;
}

/** Create a Cpu in the AML namespace from a CM_ARCH_COMMON_PROC_HIERARCHY_INFO
    CM object.

  @param [in]  Generator              The SSDT Cpu Topology generator.
  @param [in]  CfgMgrProtocol         Pointer to the Configuration Manager
                                      Protocol Interface.
  @param [in]  ParentNode             Parent node to attach the Cpu node to.
  @param [in]  CpuName                Value used to generate the node name.
  @param [in]  ProcHierarchyNodeInfo  CM_ARCH_COMMON_PROC_HIERARCHY_INFO
                                       describing the Cpu.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
CreateAmlCpuFromProcHierarchy (
  IN        ACPI_CPU_TOPOLOGY_GENERATOR                   *Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        AML_NODE_HANDLE                               ParentNode,
  IN        UINT32                                        CpuName,
  IN        CM_ARCH_COMMON_PROC_HIERARCHY_INFO            *ProcHierarchyNodeInfo
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  CpuNode;
  UINT32                  AcpiProcessorUid;
  CM_OBJECT_TOKEN         CpcToken;
  CM_OBJECT_TOKEN         PsdToken;

  ASSERT (Generator != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (ParentNode != NULL);
  ASSERT (ProcHierarchyNodeInfo != NULL);
  ASSERT (ProcHierarchyNodeInfo->AcpiIdObjectToken != CM_NULL_TOKEN);

  Status = GetIntCInfo (
             CfgMgrProtocol,
             ProcHierarchyNodeInfo->AcpiIdObjectToken,
             &AcpiProcessorUid,
             &CpcToken,
             &PsdToken
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = CreateAmlCpu (Generator, ParentNode, AcpiProcessorUid, CpuName, &CpuNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // If a set of Lpi states is associated with the
  // CM_ARCH_COMMON_PROC_HIERARCHY_INFO, create an _LPI method returning them.
  if (ProcHierarchyNodeInfo->LpiToken != CM_NULL_TOKEN) {
    Status = CreateAmlLpiMethod (Generator, ProcHierarchyNodeInfo, CpuNode);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  if (PsdToken != CM_NULL_TOKEN) {
    Status = CreateAmlPsdNode (Generator, CfgMgrProtocol, PsdToken, CpuNode);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  // If a CPC info is associated with the
  // IntcInfo, create an _CPC method returning them.
  if (CpcToken != CM_NULL_TOKEN) {
    Status = CreateAmlCpcNode (Generator, CfgMgrProtocol, CpcToken, CpuNode);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  // Add arch specific information if necessary.
  Status = AddArchAmlCpuInfo (
             Generator,
             CfgMgrProtocol,
             ProcHierarchyNodeInfo->AcpiIdObjectToken,
             CpuName,
             CpuNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  return Status;
}

/** Create a Processor Container in the AML namespace.

  Any CM_ARCH_COMMON_PROC_HIERARCHY_INFO object with the following flags is
  assumed to be a processor container:
   - EFI_ACPI_6_3_PPTT_PACKAGE_NOT_PHYSICAL
   - EFI_ACPI_6_3_PPTT_PROCESSOR_ID_INVALID
   - EFI_ACPI_6_3_PPTT_NODE_IS_NOT_LEAF

  This generates the following ASL code:
  Device (C002)
  {
      Name (_UID, 2)
      Name (_HID, "ACPI0010")
  }

  @param [in]  Generator              The SSDT Cpu Topology generator.
  @param [in]  CfgMgrProtocol         Pointer to the Configuration Manager
                                      Protocol Interface.
  @param [in]  ParentNode             Parent node to attach the processor
                                      container node to.
  @param [in]  ProcHierarchyNodeInfo  CM_ARCH_COMMON_PROC_HIERARCHY_INFO object
                                      used to create the node.
  @param [in]  ProcContainerName      Name of the processor container.
  @param [in]  ProcContainerUid       Uid of the processor container.
  @param [out] ProcContainerNodePtr   If success, contains the created processor
                                      container node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
CreateAmlProcessorContainer (
  IN        ACPI_CPU_TOPOLOGY_GENERATOR                   *Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        AML_NODE_HANDLE                               ParentNode,
  IN        CM_ARCH_COMMON_PROC_HIERARCHY_INFO            *ProcHierarchyNodeInfo,
  IN        UINT16                                        ProcContainerName,
  IN        UINT32                                        ProcContainerUid,
  OUT       AML_OBJECT_NODE_HANDLE                        *ProcContainerNodePtr
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  ProcContainerNode;
  CHAR8                   AslNameProcContainer[AML_NAME_SEG_SIZE + 1];

  ASSERT (Generator != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (ParentNode != NULL);
  ASSERT (ProcHierarchyNodeInfo != NULL);
  ASSERT (ProcContainerNodePtr != NULL);

  Status = WriteAslName ('C', ProcContainerName, AslNameProcContainer);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenDevice (AslNameProcContainer, ParentNode, &ProcContainerNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Use the ProcContainerIndex for the _UID value as there is no AcpiProcessorUid
  // and EFI_ACPI_6_3_PPTT_PROCESSOR_ID_INVALID is set for non-Cpus.
  Status = AmlCodeGenNameInteger (
             "_UID",
             ProcContainerUid,
             ProcContainerNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenNameString (
             "_HID",
             ACPI_HID_PROCESSOR_CONTAINER_DEVICE,
             ProcContainerNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // If a set of Lpi states are associated with the
  // CM_ARCH_COMMON_PROC_HIERARCHY_INFO, create an _LPI method returning them.
  if (ProcHierarchyNodeInfo->LpiToken != CM_NULL_TOKEN) {
    Status = CreateAmlLpiMethod (
               Generator,
               ProcHierarchyNodeInfo,
               ProcContainerNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  }

  *ProcContainerNodePtr = ProcContainerNode;

  return Status;
}

/** Check flags and topology of a ProcNode.

  @param [in]  NodeFlags        Flags of the ProcNode to check.
  @param [in]  IsLeaf           The ProcNode is a leaf.
  @param [in]  NodeToken        NodeToken of the ProcNode.
  @param [in]  ParentNodeToken  Parent NodeToken of the ProcNode.
  @param [in]  PackageNodeSeen  A parent of the ProcNode has the physical package flag set.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
CheckProcNode (
  UINT32           NodeFlags,
  BOOLEAN          IsLeaf,
  CM_OBJECT_TOKEN  NodeToken,
  CM_OBJECT_TOKEN  ParentNodeToken,
  BOOLEAN          PackageNodeSeen
  )
{
  BOOLEAN  InvalidFlags;
  BOOLEAN  HasPhysicalPackageBit;

  HasPhysicalPackageBit = (NodeFlags & EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL) ==
                          EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL;

  // Only one Physical Package flag is allowed in the hierarchy
  InvalidFlags = HasPhysicalPackageBit && PackageNodeSeen;

  // Check Leaf specific flags.
  if (IsLeaf) {
    InvalidFlags |= ((NodeFlags & PPTT_LEAF_MASK) != PPTT_LEAF_MASK);
    // Must have Physical Package flag somewhere in the hierarchy
    InvalidFlags |= !(HasPhysicalPackageBit || PackageNodeSeen);
  } else {
    InvalidFlags |= ((NodeFlags & PPTT_LEAF_MASK) != 0);
  }

  if (InvalidFlags) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-CPU-TOPOLOGY: Invalid flags for ProcNode: 0x%p.\n",
      (VOID *)NodeToken
      ));
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Create an AML representation of the Cpu topology.

  A processor container is by extension any non-leave device in the cpu topology.

  @param [in] Generator               The SSDT Cpu Topology generator.
  @param [in] CfgMgrProtocol          Pointer to the Configuration Manager
                                      Protocol Interface.
  @param [in] NodeToken               Token of the CM_ARCH_COMMON_PROC_HIERARCHY_INFO currently handled.
  @param [in] ParentNode              Parent node to attach the created
                                      node to.
  @param [in,out] ProcContainerIndex  Pointer to the current processor container
                                      index to be used as UID.
  @param [in]  PackageNodeSeen        A parent of the ProcNode has the physical package flag set.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
CreateAmlCpuTopologyTree (
  IN        ACPI_CPU_TOPOLOGY_GENERATOR                   *Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        CM_OBJECT_TOKEN                               NodeToken,
  IN        AML_NODE_HANDLE                               ParentNode,
  IN OUT    UINT32                                        *ProcContainerIndex,
  IN        BOOLEAN                                       PackageNodeSeen
  )
{
  EFI_STATUS              Status;
  UINT32                  Index;
  UINT32                  CpuIndex;
  UINT32                  ProcContainerName;
  AML_OBJECT_NODE_HANDLE  ProcContainerNode;
  UINT32                  Uid;
  UINT16                  Name;
  BOOLEAN                 HasPhysicalPackageBit;

  ASSERT (Generator != NULL);
  ASSERT (Generator->ProcNodeList != NULL);
  ASSERT (Generator->ProcNodeCount != 0);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (ParentNode != NULL);
  ASSERT (ProcContainerIndex != NULL);

  CpuIndex          = 0;
  ProcContainerName = 0;

  for (Index = 0; Index < Generator->ProcNodeCount; Index++) {
    // Find the children of the CM_ARCH_COMMON_PROC_HIERARCHY_INFO
    // currently being handled (i.e. ParentToken == NodeToken).
    if (Generator->ProcNodeList[Index].ParentToken == NodeToken) {
      // Only Cpus (leaf nodes in this tree) have a AcpiIdObjectToken.
      // Create a Cpu node.
      if (Generator->ProcNodeList[Index].AcpiIdObjectToken != CM_NULL_TOKEN) {
        Status = CheckProcNode (
                   Generator->ProcNodeList[Index].Flags,
                   TRUE,
                   Generator->ProcNodeList[Index].Token,
                   NodeToken,
                   PackageNodeSeen
                   );
        if (EFI_ERROR (Status)) {
          ASSERT (0);
          return Status;
        }

        if (Generator->ProcNodeList[Index].OverrideNameUidEnabled) {
          Name = Generator->ProcNodeList[Index].OverrideName;
        } else {
          ASSERT ((CpuIndex & ~MAX_UINT16) == 0);
          Name = (UINT16)CpuIndex;
        }

        Status = CreateAmlCpuFromProcHierarchy (
                   Generator,
                   CfgMgrProtocol,
                   ParentNode,
                   Name,
                   &Generator->ProcNodeList[Index]
                   );
        if (EFI_ERROR (Status)) {
          ASSERT (0);
          return Status;
        }

        CpuIndex++;
      } else {
        // If this is not a Cpu, then this is a processor container.

        Status = CheckProcNode (
                   Generator->ProcNodeList[Index].Flags,
                   FALSE,
                   Generator->ProcNodeList[Index].Token,
                   NodeToken,
                   PackageNodeSeen
                   );
        if (EFI_ERROR (Status)) {
          ASSERT (0);
          return Status;
        }

        if (Generator->ProcNodeList[Index].OverrideNameUidEnabled) {
          Name = Generator->ProcNodeList[Index].OverrideName;
          Uid  = Generator->ProcNodeList[Index].OverrideUid;
        } else {
          ASSERT ((ProcContainerName & ~MAX_UINT16) == 0);
          Name = (UINT16)ProcContainerName;
          Uid  = *ProcContainerIndex;
        }

        Status = CreateAmlProcessorContainer (
                   Generator,
                   CfgMgrProtocol,
                   ParentNode,
                   &Generator->ProcNodeList[Index],
                   Name,
                   Uid,
                   &ProcContainerNode
                   );
        if (EFI_ERROR (Status)) {
          ASSERT (0);
          return Status;
        }

        // Nodes must have a unique name in the ASL namespace.
        // Reset the Cpu index whenever we create a new processor container.
        (*ProcContainerIndex)++;
        CpuIndex = 0;

        // And reset the cluster name whenever there is a package.
        if (NodeToken == CM_NULL_TOKEN) {
          ProcContainerName = 0;
        } else {
          ProcContainerName++;
        }

        HasPhysicalPackageBit = (Generator->ProcNodeList[Index].Flags & EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL) ==
                                EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL;

        // Recursively continue creating an AML tree.
        Status = CreateAmlCpuTopologyTree (
                   Generator,
                   CfgMgrProtocol,
                   Generator->ProcNodeList[Index].Token,
                   ProcContainerNode,
                   ProcContainerIndex,
                   (PackageNodeSeen || HasPhysicalPackageBit)
                   );
        if (EFI_ERROR (Status)) {
          ASSERT (0);
          return Status;
        }
      }
    } // if ParentToken == NodeToken
  } // for

  return EFI_SUCCESS;
}

/** Create the processor hierarchy AML tree from
    CM_ARCH_COMMON_PROC_HIERARCHY_INFO CM objects.

  @param [in] Generator        The SSDT Cpu Topology generator.
  @param [in] CfgMgrProtocol   Pointer to the Configuration Manager
                               Protocol Interface.
  @param [in] ScopeNode        Scope node handle ('\_SB' scope).

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
CreateTopologyFromProcHierarchy (
  IN        ACPI_CPU_TOPOLOGY_GENERATOR                   *Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        AML_OBJECT_NODE_HANDLE                        ScopeNode
  )
{
  EFI_STATUS  Status;
  UINT32      ProcContainerIndex;

  ASSERT (Generator != NULL);
  ASSERT (Generator->ProcNodeCount != 0);
  ASSERT (Generator->ProcNodeList != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (ScopeNode != NULL);

  ProcContainerIndex = 0;

  Status = TokenTableInitialize (Generator, Generator->ProcNodeCount);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = CreateAmlCpuTopologyTree (
             Generator,
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             ScopeNode,
             &ProcContainerIndex,
             FALSE
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  Status = GenerateLpiStates (Generator, CfgMgrProtocol, ScopeNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

exit_handler:
  TokenTableFree (Generator);
  return Status;
}

/** Construct the SSDT Cpu Topology ACPI table.

  This function invokes the Configuration Manager protocol interface
  to get the required hardware information for generating the ACPI
  table.

  If this function allocates any resources then they must be freed
  in the FreeXXXXTableResources function.

  @param [in]  This           Pointer to the table generator.
  @param [in]  AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                              Protocol Interface.
  @param [out] Table          Pointer to the constructed ACPI Table.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object was not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSsdtCpuTopologyTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS                          Status;
  AML_ROOT_NODE_HANDLE                RootNode;
  AML_OBJECT_NODE_HANDLE              ScopeNode;
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *ProcHierarchyNodeList;
  UINT32                              ProcHierarchyNodeCount;
  ACPI_CPU_TOPOLOGY_GENERATOR         *Generator;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  Generator = (ACPI_CPU_TOPOLOGY_GENERATOR *)This;

  Status = AddSsdtAcpiHeader (
             CfgMgrProtocol,
             This,
             AcpiTableInfo,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AmlCodeGenScope (SB_SCOPE, RootNode, &ScopeNode);
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  // Get the processor hierarchy info and update the processor topology
  // structure count with Processor Hierarchy Nodes (Type 0)
  Status = GetEArchCommonObjProcHierarchyInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &ProcHierarchyNodeList,
             &ProcHierarchyNodeCount
             );
  if (EFI_ERROR (Status) &&
      (Status != EFI_NOT_FOUND))
  {
    goto exit_handler;
  }

  if (Status == EFI_NOT_FOUND) {
    // If hierarchy information is not found generate a flat topology.
    Status = CreateTopologyFromIntC (
               Generator,
               CfgMgrProtocol,
               ScopeNode
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }
  } else {
    // Generate the topology from CM_ARCH_COMMON_PROC_HIERARCHY_INFO objects.
    Generator->ProcNodeList  = ProcHierarchyNodeList;
    Generator->ProcNodeCount = ProcHierarchyNodeCount;

    Status = CreateTopologyFromProcHierarchy (
               Generator,
               CfgMgrProtocol,
               ScopeNode
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }
  }

  Status = AmlSerializeDefinitionBlock (
             RootNode,
             Table
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-CPU-TOPOLOGY: Failed to Serialize SSDT Table Data."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

exit_handler:
  // Delete the RootNode and its attached children.
  return AmlDeleteTree (RootNode);
}

/** Free any resources allocated for constructing the
    SSDT Cpu Topology ACPI table.

  @param [in]      This           Pointer to the table generator.
  @param [in]      AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in, out] Table          Pointer to the ACPI Table.

  @retval EFI_SUCCESS           The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER The table pointer is NULL or invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSsdtCpuTopologyTableResources (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: SSDT-CPU-TOPOLOGY: Invalid Table Pointer\n"));
    ASSERT ((Table != NULL) && (*Table != NULL));
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;
  return EFI_SUCCESS;
}

/** This macro defines the SSDT Cpu Topology Table Generator revision.
*/
#define SSDT_CPU_TOPOLOGY_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the SSDT Cpu Topology Table Generator.
*/
STATIC
ACPI_CPU_TOPOLOGY_GENERATOR  SsdtCpuTopologyGenerator = {
  // ACPI table generator header
  {
    // Generator ID
    CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtCpuTopology),
    // Generator Description
    L"ACPI.STD.SSDT.CPU.TOPOLOGY.GENERATOR",
    // ACPI Table Signature
    EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
    // ACPI Table Revision - Unused
    0,
    // Minimum ACPI Table Revision - Unused
    0,
    // Creator ID
    TABLE_GENERATOR_CREATOR_ID,
    // Creator Revision
    SSDT_CPU_TOPOLOGY_GENERATOR_REVISION,
    // Build Table function
    BuildSsdtCpuTopologyTable,
    // Free Resource function
    FreeSsdtCpuTopologyTableResources,
    // Extended build function not needed
    NULL,
    // Extended build function not implemented by the generator.
    // Hence extended free resource function is not required.
    NULL
  },

  // Private fields are defined from here.

  // TokenTable
  {
    // Table
    NULL,
    // LastIndex
    0
  },
  // ProcNodeList
  NULL,
  // ProcNodeCount
  0
};

/** Register the Generator with the ACPI Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID
                                is already registered.
**/
EFI_STATUS
EFIAPI
AcpiSsdtCpuTopologyLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SsdtCpuTopologyGenerator.Header);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-CPU-TOPOLOGY: Register Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Deregister the Generator from the ACPI Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is deregistered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The Generator is not registered.
**/
EFI_STATUS
EFIAPI
AcpiSsdtCpuTopologyLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SsdtCpuTopologyGenerator.Header);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-CPU-TOPOLOGY: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
