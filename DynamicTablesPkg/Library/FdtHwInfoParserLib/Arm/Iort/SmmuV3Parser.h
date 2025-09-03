/** @file
  Arm SMMUv3 parser.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/iommu/arm,smmu-v3.yaml
**/

#ifndef SMMUV3_PARSER_H_
#define SMMUV3_PARSER_H_

/** CM_ARM_SMMUV3_NODE parser function.

  The following structure is populated:
  typedef struct CmArmSmmuV3Node {
    CM_OBJECT_TOKEN    Token;
    UINT32             IdMappingCount;          // {Populated}
    CM_OBJECT_TOKEN    IdMappingToken;          // {Populated}
    UINT64             BaseAddress;             // {Populated}
    UINT32             Flags;                   // {default = 0}
    UINT64             VatosAddress;            // {default = 0}
    UINT32             Model;                   // {Populated}
    UINT32             EventInterrupt;          // {Populated}
    UINT32             PriInterrupt;            // {Populated}
    UINT32             GerrInterrupt;           // {Populated}
    UINT32             SyncInterrupt;           // {Populated}
    UINT32             ProximityDomain;         // {default = 0}
    UINT32             DeviceIdMappingIndex;    // {Populated}
    UINT32             Identifier;              // {Populated}
    CM_OBJECT_TOKEN    ProximityDomainToken;    // {default = 0}
  } CM_ARM_SMMUV3_NODE;

  A parser parses a Device Tree to populate a specific CmObj type. None,
  one or many CmObj can be created by the parser.
  The created CmObj are then handed to the parser's caller through the
  HW_INFO_ADD_OBJECT interface.
  This can also be a dispatcher. I.e. a function that not parsing a
  Device Tree but calling other parsers.

  @param [in]  FdtParserHandle A handle to the parser instance.
  @param [in]  FdtBranch       When searching for DT node name, restrict
                               the search to this Device Tree branch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
EFI_STATUS
EFIAPI
ArmSmmuV3Parser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  );

#endif // SMMUV3_PARSER_H_
