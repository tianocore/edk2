/** @file
  ARM SSDT Cpu Topology Table Generator Helpers.

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

/** ARM SSDT Cpu Topology Table Generator.

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArmObjGicCInfo
  - EArmObjEtInfo (OPTIONAL)
*/

/** This macro expands to a function that retrieves the GIC
    CPU interface Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjGicCInfo,
  CM_ARM_GICC_INFO
  );

/**
  This macro expands to a function that retrieves the ET device
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjEtInfo,
  CM_ARM_ET_INFO
  );

/** Create an embedded trace device and add it to the Cpu Node in the
    AML namespace.

  This generates the following ASL code:
  Device (E002)
  {
      Name (_UID, 2)
      Name (_HID, "ARMHC500")
  }

  Note: Currently we only support generating ETE nodes. Unlike ETM,
  ETE has a system register interface and therefore does not need
  the MMIO range to be described.

  @param [in]  Generator            The SSDT Cpu Topology generator.
  @param [in]  ParentNode           Parent node to attach the Cpu node to.
  @param [in]  AcpiProcessorUid     ACPI Processor UID of the CPU.
  @param [in]  CpuName              Value used to generate the node name.
  @param [out] EtNodePtr            If not NULL, return the created Cpu node.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
CreateAmlEtd (
  IN   ACPI_CPU_TOPOLOGY_GENERATOR  *Generator,
  IN   AML_NODE_HANDLE              ParentNode,
  IN   UINT32                       AcpiProcessorUid,
  IN   UINT32                       CpuName,
  OUT  AML_OBJECT_NODE_HANDLE       *EtNodePtr OPTIONAL
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  EtNode;
  CHAR8                   AslName[AML_NAME_SEG_SIZE + 1];

  ASSERT (Generator != NULL);
  ASSERT (ParentNode != NULL);

  Status = WriteAslName ('E', CpuName, AslName);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenDevice (AslName, ParentNode, &EtNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenNameInteger (
             "_UID",
             AcpiProcessorUid,
             EtNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenNameString (
             "_HID",
             ACPI_HID_ET_DEVICE,
             EtNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // If requested, return the handle to the EtNode.
  if (EtNodePtr != NULL) {
    *EtNodePtr = EtNode;
  }

  return Status;
}

/** Create and add an Embedded trace device to the Cpu Node.

  @param [in]  Generator              The SSDT Cpu Topology generator.
  @param [in]  CfgMgrProtocol         Pointer to the Configuration Manager
                                      Protocol Interface.
  @param [in]  AcpiProcessorUid       ACPI processor Uid of the local intc (gicc, other)
                                      describing the Cpu.
  @param [in]  EtToken                Embedded Trace Token of the CPU.
  @param [in]  CpuName                Value used to generate the CPU node name.
  @param [in]  CpuNode                CPU Node to which the ET device node is
                                      attached.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_UNSUPPORTED         Feature Unsupported.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
CreateAmlEtNode (
  IN  ACPI_CPU_TOPOLOGY_GENERATOR                         *Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  UINT32                                              AcpiProcessorUid,
  IN  CM_OBJECT_TOKEN                                     EtToken,
  IN  UINT32                                              CpuName,
  IN  AML_OBJECT_NODE_HANDLE                              *CpuNode
  )
{
  EFI_STATUS      Status;
  CM_ARM_ET_INFO  *EtInfo;

  Status = GetEArmObjEtInfo (
             CfgMgrProtocol,
             EtToken,
             &EtInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Currently we only support creation of a ETE Node.
  if (EtInfo->EtType != ArmEtTypeEte) {
    return EFI_UNSUPPORTED;
  }

  Status = CreateAmlEtd (
             Generator,
             CpuNode,
             AcpiProcessorUid,
             CpuName,
             NULL
             );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Create the processor hierarchy AML tree from arch specific CM objects.

  The Arm architecture will use the CM_ARM_GICC_INFO CM objects for instance.
  A processor container is by extension any non-leave device in the cpu topology.

  @param [in] Generator        The SSDT Cpu Topology generator.
  @param [in] CfgMgrProtocol   Pointer to the Configuration Manager
                               Protocol Interface.
  @param [in] ScopeNode        Scope node handle ('\_SB' scope).

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
  EFI_STATUS              Status;
  CM_ARM_GICC_INFO        *GicCInfo;
  UINT32                  GicCInfoCount;
  UINT32                  Index;
  AML_OBJECT_NODE_HANDLE  CpuNode;

  ASSERT (Generator != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (ScopeNode != NULL);

  Status = GetEArmObjGicCInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &GicCInfo,
             &GicCInfoCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // For each CM_ARM_GICC_INFO object, create an AML node.
  for (Index = 0; Index < GicCInfoCount; Index++) {
    Status = CreateAmlCpu (
               Generator,
               ScopeNode,
               GicCInfo[Index].AcpiProcessorUid,
               Index,
               &CpuNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      break;
    }

    // If a CPC info is associated with the
    // GicCinfo, create an _CPC method returning them.
    if (GicCInfo[Index].CpcToken != CM_NULL_TOKEN) {
      Status = CreateAmlCpcNode (Generator, CfgMgrProtocol, GicCInfo[Index].CpcToken, CpuNode);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        break;
      }
    }

    if (GicCInfo[Index].EtToken != CM_NULL_TOKEN) {
      Status = CreateAmlEtNode (
                 Generator,
                 CfgMgrProtocol,
                 GicCInfo[Index].AcpiProcessorUid,
                 GicCInfo[Index].EtToken,
                 Index,
                 CpuNode
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }
    }
  } // for

  return Status;
}

/** Get generic interrupt information from arch specific CM objects.

  The AcpiProcessorUid, CpcToken, etc. are held in arch specific CM objects,
  in the CM_ARM_GICC_INFO CM object for Arm for instance.
  This wrapper allows to get this information from each arch object.

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

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
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
  EFI_STATUS        Status;
  CM_ARM_GICC_INFO  *GicCInfo;

  Status = GetEArmObjGicCInfo (
             CfgMgrProtocol,
             AcpiIdObjectToken,
             &GicCInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (AcpiProcessorUid != NULL) {
    *AcpiProcessorUid = GicCInfo->AcpiProcessorUid;
  }

  if (CpcToken != NULL) {
    *CpcToken = GicCInfo->CpcToken;
  }

  if (PsdToken != NULL) {
    *PsdToken = GicCInfo->PsdToken;
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

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Feature Unsupported.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
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
  EFI_STATUS        Status;
  CM_ARM_GICC_INFO  *GicCInfo;

  Status = GetEArmObjGicCInfo (
             CfgMgrProtocol,
             AcpiIdObjectToken,
             &GicCInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Add an Embedded Trace node if present.
  if (GicCInfo->EtToken != CM_NULL_TOKEN) {
    Status = CreateAmlEtNode (
               Generator,
               CfgMgrProtocol,
               GicCInfo->AcpiProcessorUid,
               GicCInfo->EtToken,
               CpuName,
               CpuNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  return Status;
}
