/** @file
  Mock implementations for architecture-specific SSDT CPU Topology functions.

  These implementations delegate to the GoogleMock framework, allowing
  tests to set expectations and return values for arch-specific functions.

  Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "MockArchFunctions.h"

// Global mock instance pointer - initialized by test fixtures
MockArchFunctions  *gMockArchFunctions = nullptr;

extern "C" {
  /**
    Mock implementation of GetIntCInfo.
    Delegates to gMockArchFunctions if set, otherwise returns EFI_UNSUPPORTED.
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
    if (gMockArchFunctions != nullptr) {
      return gMockArchFunctions->GetIntCInfo (
                                   CfgMgrProtocol,
                                   AcpiIdObjectToken,
                                   AcpiProcessorUid,
                                   CpcToken,
                                   PsdToken
                                   );
    }

    // Default: return UNSUPPORTED (X64-like behavior)
    return EFI_UNSUPPORTED;
  }

  /**
    Mock implementation of CreateTopologyFromIntC.
    Delegates to gMockArchFunctions if set, otherwise returns EFI_UNSUPPORTED.
  **/
  EFI_STATUS
  EFIAPI
  CreateTopologyFromIntC (
    IN        ACPI_CPU_TOPOLOGY_GENERATOR                   *Generator,
    IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
    IN        AML_OBJECT_NODE_HANDLE                        ScopeNode
    )
  {
    if (gMockArchFunctions != nullptr) {
      return gMockArchFunctions->CreateTopologyFromIntC (
                                   Generator,
                                   CfgMgrProtocol,
                                   ScopeNode
                                   );
    }

    // Default: return UNSUPPORTED
    return EFI_UNSUPPORTED;
  }

  /**
    Mock implementation of AddArchAmlCpuInfo.
    Delegates to gMockArchFunctions if set, otherwise returns EFI_SUCCESS (no-op).
  **/
  EFI_STATUS
  EFIAPI
  AddArchAmlCpuInfo (
    IN  ACPI_CPU_TOPOLOGY_GENERATOR                         *Generator,
    IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
    IN  CM_OBJECT_TOKEN                                     AcpiIdObjectToken,
    IN  UINT32                                              CpuName,
    OUT AML_OBJECT_NODE_HANDLE                              *CpuNode
    )
  {
    if (gMockArchFunctions != nullptr) {
      return gMockArchFunctions->AddArchAmlCpuInfo (
                                   Generator,
                                   CfgMgrProtocol,
                                   AcpiIdObjectToken,
                                   CpuName,
                                   CpuNode
                                   );
    }

    // Default: return SUCCESS (no-op, like X64/RISC-V)
    return EFI_SUCCESS;
  }
} // extern "C"
