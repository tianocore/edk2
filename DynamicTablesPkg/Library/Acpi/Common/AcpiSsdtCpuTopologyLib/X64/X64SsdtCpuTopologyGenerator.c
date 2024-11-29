/** @file
  X64 SSDT Cpu Topology Table Generator Helpers.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

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
#include <Protocol/ConfigurationManagerProtocol.h>

#include "SsdtCpuTopologyGenerator.h"

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
    P-State information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjPstateInfo,
  CM_ARCH_COMMON_PSTATE_INFO
  );

/**
  Create the processor hierarchy AML tree from arch specific CM objects.

  @param [in] Generator        The SSDT Cpu Topology generator.
  @param [in] CfgMgrProtocol   Pointer to the Configuration Manager
                               Protocol Interface.
  @param [in] ScopeNode        Scope node handle ('\_SB' scope).

  @retval EFI_UNSUPPORTED      Not supported
**/
EFI_STATUS
EFIAPI
CreateTopologyFromIntC (
  IN        ACPI_CPU_TOPOLOGY_GENERATOR                   *Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        AML_OBJECT_NODE_HANDLE                        ScopeNode
  )
{
  EFI_STATUS                     Status;
  CM_X64_LOCAL_APIC_X2APIC_INFO  *LocalApicX2ApicInfo;
  UINT32                         LocalApicX2ApicCount;
  UINT32                         Index;
  AML_OBJECT_NODE_HANDLE         CpuNode;
  CM_ARCH_COMMON_CST_INFO        *CstInfo;
  CM_ARCH_COMMON_CSD_INFO        *CsdInfo;
  UINT32                         CsdNumEntries;
  CM_ARCH_COMMON_PSTATE_INFO     *PstateInfo;

  ASSERT (Generator != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (ScopeNode != NULL);

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
      break;
    }

    ///
    /// Check for optional tokens and add them to the CPU node.
    ///
    if (LocalApicX2ApicInfo[Index].CstToken != CM_NULL_TOKEN) {
      Status = GetEArchCommonObjCstInfo (
                 CfgMgrProtocol,
                 LocalApicX2ApicInfo[Index].CstToken,
                 &CstInfo,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }

      Status = AmlCreateCstNode (
                 CstInfo,
                 CpuNode,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }
    }

    if (LocalApicX2ApicInfo[Index].CsdToken != CM_NULL_TOKEN) {
      Status = GetEArchCommonObjCsdInfo (
                 CfgMgrProtocol,
                 LocalApicX2ApicInfo[Index].CsdToken,
                 &CsdInfo,
                 &CsdNumEntries
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }

      Status = AmlCreateCsdNode (
                 CsdInfo,
                 CsdNumEntries,
                 CpuNode,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }
    }

    ///
    /// Check for optional tokens and add them to the CPU node.
    ///
    if (LocalApicX2ApicInfo[Index].PstateToken != CM_NULL_TOKEN) {
      Status = GetEArchCommonObjPstateInfo (
                 CfgMgrProtocol,
                 LocalApicX2ApicInfo[Index].PstateToken,
                 &PstateInfo,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }

      Status = AmlCreatePctNode (
                 &PstateInfo->Pct,
                 CpuNode,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }

      Status = AmlCreatePssNode (
                 PstateInfo->Pss,
                 PstateInfo->PssPackagesCount,
                 CpuNode,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }

      Status = AmlCodeGenMethodRetInteger ("_PPC", PstateInfo->Ppc, 0, FALSE, 0, CpuNode, NULL);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }
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
  }

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
