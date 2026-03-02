/** @file
  Mock implementations for common SsdtCpuTopology functions.

  These implementations delegate to gMockCommonFunctions which allows
  Google Mock to intercept and control the behavior of these functions.

  Copyright (c) 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "MockCommonFunctions.h"

extern "C" {
  #include "../SsdtCpuTopologyGenerator.h"
}

// Global mock object
MockCommonFunctions  *gMockCommonFunctions = nullptr;

//
// C function implementations that delegate to the mock
//

extern "C" {
  EFI_STATUS
  EFIAPI
  WriteAslName (
    IN      CHAR8   LeadChar,
    IN      UINT32  Value,
    IN OUT  CHAR8   *AslName
    )
  {
    if (gMockCommonFunctions != nullptr) {
      return gMockCommonFunctions->WriteAslName (LeadChar, Value, AslName);
    }

    // Default implementation if mock not set
    if ((AslName == NULL) || (Value >= MAX_NODE_COUNT)) {
      return EFI_INVALID_PARAMETER;
    }

    // Convert nibbles to hex characters (0-9, A-F)
    UINT8  Nibble;

    AslName[0] = LeadChar;

    Nibble     = (UINT8)((Value >> 8) & 0xF);
    AslName[1] = (CHAR8)(Nibble < 10 ? '0' + Nibble : 'A' + Nibble - 10);

    Nibble     = (UINT8)((Value >> 4) & 0xF);
    AslName[2] = (CHAR8)(Nibble < 10 ? '0' + Nibble : 'A' + Nibble - 10);

    Nibble     = (UINT8)(Value & 0xF);
    AslName[3] = (CHAR8)(Nibble < 10 ? '0' + Nibble : 'A' + Nibble - 10);

    AslName[4] = '\0';
    return EFI_SUCCESS;
  }

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
    if (gMockCommonFunctions != nullptr) {
      return gMockCommonFunctions->CreateAmlCpu (
                                     Generator,
                                     ParentNode,
                                     AcpiProcessorUid,
                                     CpuName,
                                     CpuNodePtr
                                     );
    }

    return EFI_UNSUPPORTED;
  }

  EFI_STATUS
  EFIAPI
  CreateAmlCpcNode (
    IN  ACPI_CPU_TOPOLOGY_GENERATOR                         *Generator,
    IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
    IN  CM_OBJECT_TOKEN                                     CpcToken,
    IN  AML_OBJECT_NODE_HANDLE                              *Node
    )
  {
    if (gMockCommonFunctions != nullptr) {
      return gMockCommonFunctions->CreateAmlCpcNode (
                                     Generator,
                                     CfgMgrProtocol,
                                     CpcToken,
                                     Node
                                     );
    }

    return EFI_UNSUPPORTED;
  }

  EFI_STATUS
  EFIAPI
  CreateAmlPsdNode (
    IN  ACPI_CPU_TOPOLOGY_GENERATOR                         *Generator,
    IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
    IN  CM_OBJECT_TOKEN                                     PsdToken,
    IN  AML_OBJECT_NODE_HANDLE                              *Node
    )
  {
    if (gMockCommonFunctions != nullptr) {
      return gMockCommonFunctions->CreateAmlPsdNode (
                                     Generator,
                                     CfgMgrProtocol,
                                     PsdToken,
                                     Node
                                     );
    }

    return EFI_UNSUPPORTED;
  }
} // extern "C"
