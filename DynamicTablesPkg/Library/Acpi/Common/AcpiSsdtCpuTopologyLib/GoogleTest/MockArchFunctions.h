/** @file
  Mock declarations for architecture-specific SSDT CPU Topology functions.

  These mocks allow testing SsdtCpuTopologyGenerator.c (common code) in isolation
  from architecture-specific implementations (ARM, X64, RISC-V).

  Copyright (c) 2021-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_ARCH_FUNCTIONS_H_
#define MOCK_ARCH_FUNCTIONS_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
#include <gmock/gmock.h>

extern "C" {
  #include <Uefi.h>
  #include <ConfigurationManagerObject.h>
  #include <Protocol/ConfigurationManagerProtocol.h>
  #include <Library/AmlLib/AmlLib.h>
  #include "../SsdtCpuTopologyGenerator.h"
}

/**
  Mock class for architecture-specific CPU topology functions.

  This provides mockable versions of:
  - GetIntCInfo: Gets CPU info from arch-specific interrupt controller
  - CreateTopologyFromIntC: Creates flat topology from arch-specific CM objects
  - AddArchAmlCpuInfo: Adds arch-specific info to CPU nodes (e.g., ETE/ETM)
**/
struct MockArchFunctions {
  MOCK_METHOD (
    EFI_STATUS,
    GetIntCInfo,
    (CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL *CONST CfgMgrProtocol,
     CM_OBJECT_TOKEN AcpiIdObjectToken,
     UINT32 *AcpiProcessorUid,
     CM_OBJECT_TOKEN *CpcToken,
     CM_OBJECT_TOKEN *PsdToken)
    );

  MOCK_METHOD (
    EFI_STATUS,
    CreateTopologyFromIntC,
    (ACPI_CPU_TOPOLOGY_GENERATOR *Generator,
     CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL *CONST CfgMgrProtocol,
     AML_OBJECT_NODE_HANDLE ScopeNode)
    );

  MOCK_METHOD (
    EFI_STATUS,
    AddArchAmlCpuInfo,
    (ACPI_CPU_TOPOLOGY_GENERATOR *Generator,
     CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL *CONST CfgMgrProtocol,
     CM_OBJECT_TOKEN AcpiIdObjectToken,
     UINT32 CpuName,
     AML_OBJECT_NODE_HANDLE *CpuNode)
    );
};

// Global mock instance - set by test fixture
extern MockArchFunctions  *gMockArchFunctions;

#endif // MOCK_ARCH_FUNCTIONS_H_
