/** @file
  Mock implementations for common SsdtCpuTopology functions.

  These mocks allow arch-specific code to be unit tested in isolation
  without linking the common SsdtCpuTopologyGenerator.c code.

  Copyright (c) 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_COMMON_FUNCTIONS_H_
#define MOCK_COMMON_FUNCTIONS_H_

#include <gmock/gmock.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/AmlLib/AmlLib.h>
  #include <Protocol/ConfigurationManagerProtocol.h>
}

// Forward declare the generator struct (defined in SsdtCpuTopologyGenerator.h)
typedef struct AcpiCpuTopologyGenerator ACPI_CPU_TOPOLOGY_GENERATOR;

/**
  Mock class for common SsdtCpuTopology functions.

  These functions are implemented in SsdtCpuTopologyGenerator.c and called
  by architecture-specific code in Arm/, X64/, RiscV/ subdirectories.
**/
class MockCommonFunctions {
public:
  virtual ~MockCommonFunctions (
                                )
  {
  }

  /**
    Mock for WriteAslName - generates ASL name strings like "C000", "L001"
  **/
  MOCK_METHOD3 (
    WriteAslName,
    EFI_STATUS (
                CHAR8   LeadChar,
                UINT32  Value,
                CHAR8   *AslName
                )
    );

  /**
    Mock for CreateAmlCpu - creates a CPU device in AML tree
  **/
  MOCK_METHOD5 (
    CreateAmlCpu,
    EFI_STATUS (
                ACPI_CPU_TOPOLOGY_GENERATOR  *Generator,
                AML_NODE_HANDLE              ParentNode,
                UINT32                       AcpiProcessorUid,
                UINT32                       CpuName,
                AML_OBJECT_NODE_HANDLE       *CpuNodePtr
                )
    );

  /**
    Mock for CreateAmlCpcNode - creates _CPC object for CPPC support
  **/
  MOCK_METHOD4 (
    CreateAmlCpcNode,
    EFI_STATUS (
                ACPI_CPU_TOPOLOGY_GENERATOR                         *Generator,
                CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
                CM_OBJECT_TOKEN                                     CpcToken,
                AML_OBJECT_NODE_HANDLE                              *Node
                )
    );

  /**
    Mock for CreateAmlPsdNode - creates _PSD object for P-state dependencies
  **/
  MOCK_METHOD4 (
    CreateAmlPsdNode,
    EFI_STATUS (
                ACPI_CPU_TOPOLOGY_GENERATOR                         *Generator,
                CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
                CM_OBJECT_TOKEN                                     PsdToken,
                AML_OBJECT_NODE_HANDLE                              *Node
                )
    );
};

// Global mock object - set this in test SetUp()
extern MockCommonFunctions  *gMockCommonFunctions;

#endif // MOCK_COMMON_FUNCTIONS_H_
