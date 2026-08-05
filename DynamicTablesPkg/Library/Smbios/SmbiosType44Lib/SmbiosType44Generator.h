/** @file

  Copyright (c) 2026, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard
**/

#pragma once

#if !defined (MDE_CPU_AARCH64)
#define   ARM_SMBIOS_TYPE44_RECORD_UNSUPPORTED  TRUE
#else
#define   ARM_SMBIOS_TYPE44_RECORD_UNSUPPORTED  FALSE
#endif

#if !defined (MDE_CPU_X64) && !defined (MDE_CPU_IA32)
#define   X86_SMBIOS_TYPE44_RECORD_UNSUPPORTED  TRUE
#else
#define   X86_SMBIOS_TYPE44_RECORD_UNSUPPORTED  FALSE
#endif

#if !defined (MDE_CPU_RISCV64)
#define   RISCV_SMBIOS_TYPE44_RECORD_UNSUPPORTED  TRUE
#else
#define   RISCV_SMBIOS_TYPE44_RECORD_UNSUPPORTED  FALSE
#endif

typedef struct ArmProcessorSubDataOps     ARM_PROCESSOR_SUB_DATA_OPS;
typedef struct ProcessorSpecificBlockOps  PROCESSOR_SPECIFIC_BLOCK_OPS;

/** Get Arm Processor Specific sub-data CM objects.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       Token                Arm Sub Data Token.
  @param [out]      ArmSubDataOps        Arm Sub Data Operation.

  @retval EFI_SUCCESS
  @retval Others                         Failed to initialise
**/
typedef
EFI_STATUS
(EFIAPI *GET_ARM_PROCESSOR_SUB_DATA_CM_OBJ)(
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN            CM_OBJECT_TOKEN                                        Token,
  OUT           ARM_PROCESSOR_SUB_DATA_OPS                             *ArmSubDataOps
  );

/** Get size of Arm Processor Specific sub-data.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CmObject             CM object of Processor Specific Data.
  @param [out]      Size                 Size of Processor Specific Data or SubData.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
typedef
EFI_STATUS
(EFIAPI *GET_SIZE_OF_ARM_PROCESSOR_SUB_DATA)(
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      CONST VOID                                                   *CmObject,
  OUT     UINT32                                                       *Size
  );

/** Add Arm Processor Specific sub-data into ARM_PROCESSOR_SPECIFIC_DATA.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CmObject             CM object of Processor Specific sub-data.
  @param [out]      ProcBlock             Arm Processor Specific Block.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
typedef
EFI_STATUS
(EFIAPI *ADD_ARM_PROCESSOR_SUB_DATA)(
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      CONST VOID                                                   *CmObject,
  OUT     ARM_PROCESSOR_SPECIFIC_BLOCK                                 *ProcBlock
  );

/** Get Architecture Processor Specific Data CM objects.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       Token                Processor Specific Block Token.
  @param [out]      ProcBlockOps         Process Specific Block Operation.

  @retval EFI_SUCCESS
  @retval Others                         Failed to initialise
**/
typedef
EFI_STATUS
(EFIAPI *GET_PROCESSOR_SPECIFIC_BLOCK_CM_OBJ)(
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN            CM_OBJECT_TOKEN                                        Token,
  OUT           PROCESSOR_SPECIFIC_BLOCK_OPS                           *ProcBlockOps
  );

/** Get size of Processor Specific Block.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CmObject             CM object of Processor Specific Data.
  @param [out]      Size                 Size of Processor Specific Data.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
typedef
EFI_STATUS
(EFIAPI *GET_SIZE_OF_PROCESSOR_SPECIFIC_BLOCK)(
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      CONST VOID                                                   *CmObject,
  OUT     UINT32                                                       *Size
  );

/** Add Processor Specific Block into SMBIOS record.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CmObject             CM object of Processor Specific Data.
  @param [out]      SmbiosRecord         Type 44 Smbios Record.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
typedef
EFI_STATUS
(EFIAPI *ADD_PROCESSOR_SPECIFIC_BLOCK)(
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      CONST VOID                                                   *CmObject,
  OUT     SMBIOS_TABLE_TYPE44                                          *SmbiosRecord
  );

/** A structure that describes operation relevant error source.
*/
typedef struct ArmProcessorSubDataOps {
  /// Processor Specific Block Arch Type.
  ARM_PROCESSOR_SPECIFIC_DATA_SUB_TYPE    SubDataType;

  /// Get CM objects for Process Specific Data.
  GET_ARM_PROCESSOR_SUB_DATA_CM_OBJ       GetProcSubDataCmObj;

  /// Get  size of Process Specific Data.
  GET_SIZE_OF_ARM_PROCESSOR_SUB_DATA      GetSizeofProcSubData;

  /// Add Error source to HEST
  ADD_ARM_PROCESSOR_SUB_DATA              AddProcSubData;

  /// Architecture Processor Specific Data CM object.
  VOID                                    *CmObject;
} ARM_PROCESSOR_SUB_DATA_OPS;

/** A structure that describes operation relevant error source.
*/
typedef struct ProcessorSpecificBlockOps {
  /// Processor Specific Block Arch Type.
  PROCESSOR_SPECIFIC_BLOCK_ARCH_TYPE      ArchType;

  /// Get CM objects for Process Specific Data.
  GET_PROCESSOR_SPECIFIC_BLOCK_CM_OBJ     GetProcBlockCmObj;

  /// Get  size of Process Specific Data.
  GET_SIZE_OF_PROCESSOR_SPECIFIC_BLOCK    GetSizeofProcBlock;

  /// Add Error source to HEST
  ADD_PROCESSOR_SPECIFIC_BLOCK            AddProcBlock;

  /// If FALSE, generate sub-tables for this error source type.
  BOOLEAN                                 Unsupported;

  /// Architecture Processor Specific Data CM object.
  VOID                                    *CmObject;
} PROCESSOR_SPECIFIC_BLOCK_OPS;
