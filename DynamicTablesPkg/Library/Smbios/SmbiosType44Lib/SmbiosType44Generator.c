/** @file
  SMBIOS Type44 Table Generator.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SmbiosStringTableLib.h>

// Module specific include files.
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>
#include <IndustryStandard/SmBios.h>

#include "SmbiosType44Generator.h"

/**
  SMBIOS Type 44 Generator

  Requirements:
    The following Configuration Manager Object(s) are used by this Generator:
     - EArchCommonObjProcessorSpecificBlockInfo
     - EArmObjProcessorSpecificBlockInfo,
     - EArmObjProcessorSpecificSubDataArchInfo,
*/

/**
  This macro expands to a function that retrieves the Error source infomation
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjProcessorSpecificBlockInfo,
  CM_ARCH_COMMON_PROCESSOR_SPECIFIC_BLOCK_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjProcessorSpecificBlockInfo,
  CM_ARM_PROCESSOR_SPECIFIC_BLOCK_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjProcessorSpecificSubDataArchInfo,
  CM_ARM_PROCESSOR_SPECIFIC_SUB_DATA_ARCH_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjProcessorSpecificBlockInfo,
  CM_X64_PROCESSOR_SPECIFIC_BLOCK_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceRiscV,
  ERiscVObjProcessorSpecificBlockInfo,
  CM_RISCV_PROCESSOR_SPECIFIC_BLOCK_INFO
  );

/** Get Arm Processor Specific sub-data CM objects.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       Token                Arm Sub Data Token.
  @param [out]      ArmSubDataOps        Arm Sub Data Operation.

  @retval EFI_SUCCESS
  @retval Others                         Failed to initialise
**/
STATIC
EFI_STATUS
EFIAPI
GetArmSubDataArchCmObj (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN            CM_OBJECT_TOKEN                               Token,
  OUT           ARM_PROCESSOR_SUB_DATA_OPS                    *ArmSubDataOps
  )
{
  EFI_STATUS                                    Status;
  CM_ARM_PROCESSOR_SPECIFIC_SUB_DATA_ARCH_INFO  *Data;
  UINT32                                        DataCount;

  Status = GetEArmObjProcessorSpecificSubDataArchInfo (
             CfgMgrProtocol,
             Token,
             &Data,
             &DataCount
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get Arm processor sub-data arch info. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  if (DataCount != 1) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: SubData Token (%lx) should be unique. Count = %d\n",
      __func__,
      Token,
      DataCount
      ));
    return EFI_INVALID_PARAMETER;
  }

  ArmSubDataOps->CmObject = Data;

  return EFI_SUCCESS;
}

/** Get size of Arm processor arch sub-data.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CmObject             CM object of Processor Specific Data.
  @param [out]      Size                 Size of Processor Specific sub-data.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofArmSubDataArch (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST VOID                                          *CmObject,
  OUT     UINT32                                              *Size
  )
{
  CONST CM_ARM_PROCESSOR_SPECIFIC_SUB_DATA_ARCH_INFO  *SubData;

  SubData = CmObject;

  switch (SubData->Version) {
    case 1:
      *Size = sizeof (AARCH64_PROCESSOR_SPECIFIC_SUB_DATA_ARCH);
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Add Arm Processor Specific sub-data arch into ARM_PROCESSOR_SPECIFIC_BLOCK.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CmObject             CM object of Processor Specific Data.
  @param [out]      ProcBlock             Arm Processor Specific Block.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddArmSubDataArch (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST VOID                                          *CmObject,
  OUT     ARM_PROCESSOR_SPECIFIC_BLOCK                        *ProcBlock
  )
{
  CONST CM_ARM_PROCESSOR_SPECIFIC_SUB_DATA_ARCH_INFO  *SubData;
  AARCH64_PROCESSOR_SPECIFIC_SUB_DATA_ARCH            *ArchData;

  SubData  = CmObject;
  ArchData = (AARCH64_PROCESSOR_SPECIFIC_SUB_DATA_ARCH *)(ProcBlock + 1);

  ArchData->Version = SubData->Version;

  switch (SubData->Version) {
    case 1:
      ArchData->Length      = sizeof (AARCH64_PROCESSOR_SPECIFIC_SUB_DATA_ARCH);
      ArchData->IdAA64Afr0  = SubData->IdAA64Afr0;
      ArchData->IdAA64Afr1  = SubData->IdAA64Afr1;
      ArchData->IdAA64Dfr0  = SubData->IdAA64Dfr0;
      ArchData->IdAA64Dfr1  = SubData->IdAA64Dfr1;
      ArchData->IdAA64Dfr2  = SubData->IdAA64Dfr2;
      ArchData->IdAA64Fpfr0 = SubData->IdAA64Fpfr0;
      ArchData->IdAA64Isar0 = SubData->IdAA64Isar0;
      ArchData->IdAA64Isar1 = SubData->IdAA64Isar1;
      ArchData->IdAA64Isar2 = SubData->IdAA64Isar2;
      ArchData->IdAA64Isar3 = SubData->IdAA64Isar3;
      ArchData->IdAA64Mmfr0 = SubData->IdAA64Mmfr0;
      ArchData->IdAA64Mmfr1 = SubData->IdAA64Mmfr1;
      ArchData->IdAA64Mmfr2 = SubData->IdAA64Mmfr2;
      ArchData->IdAA64Mmfr3 = SubData->IdAA64Mmfr3;
      ArchData->IdAA64Mmfr4 = SubData->IdAA64Mmfr4;
      ArchData->IdAA64Pfr0  = SubData->IdAA64Pfr0;
      ArchData->IdAA64Pfr1  = SubData->IdAA64Pfr1;
      ArchData->IdAA64Pfr2  = SubData->IdAA64Pfr2;
      ArchData->IdAA64Smfr0 = SubData->IdAA64Smfr0;
      ArchData->IdAA64Zfr0  = SubData->IdAA64Zfr0;
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Operation table to handle Arm Processor specific sub data.
**/
STATIC ARM_PROCESSOR_SUB_DATA_OPS  mArmProcSubDataOps[] = {
  {
    ArmProcessorSpecificDataSubTypeArch,
    GetArmSubDataArchCmObj,
    GetSizeofArmSubDataArch,
    AddArmSubDataArch,
  },
};

/** Get Arm Processor Specific Block CM objects.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       Token                Processor Specific Block Token.
  @param [out]      ProcBlockOps         Process Specific Block Operation.

  @retval EFI_SUCCESS
  @retval Others                         Failed to initialise
**/
STATIC
EFI_STATUS
EFIAPI
GetArmProcBlockCmObj (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN            CM_OBJECT_TOKEN                               Token,
  OUT           PROCESSOR_SPECIFIC_BLOCK_OPS                  *ProcBlockOps
  )
{
  EFI_STATUS                            Status;
  CM_ARM_PROCESSOR_SPECIFIC_BLOCK_INFO  *Block;
  UINT32                                BlockCount;

  Status = GetEArmObjProcessorSpecificBlockInfo (
             CfgMgrProtocol,
             Token,
             &Block,
             &BlockCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get Arm processor data info. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  ProcBlockOps->CmObject = Block;

  return EFI_SUCCESS;
}

/** Get x86 (x64) Processor Specific Block CM objects.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       Token                Processor Specific Block Token.
  @param [out]      ProcBlockOps         Process Specific Block Operation.

  @retval EFI_SUCCESS
  @retval Others                         Failed to initialise
**/
STATIC
EFI_STATUS
EFIAPI
GetX64ProcBlockCmObj (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN            CM_OBJECT_TOKEN                               Token,
  OUT           PROCESSOR_SPECIFIC_BLOCK_OPS                  *ProcBlockOps
  )
{
  EFI_STATUS                            Status;
  CM_X64_PROCESSOR_SPECIFIC_BLOCK_INFO  *Block;
  UINT32                                BlockCount;

  Status = GetEX64ObjProcessorSpecificBlockInfo (
             CfgMgrProtocol,
             Token,
             &Block,
             &BlockCount
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get X64 processor data info. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  ProcBlockOps->CmObject = Block;

  return EFI_SUCCESS;
}

/** Get RiscV Processor Specific Block CM objects.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       Token                Processor Specific Block Token.
  @param [out]      ProcBlockOps         Process Specific Block Operation.

  @retval EFI_SUCCESS
  @retval Others                         Failed to initialise
**/
STATIC
EFI_STATUS
EFIAPI
GetRiscVProcBlockCmObj (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN            CM_OBJECT_TOKEN                               Token,
  OUT           PROCESSOR_SPECIFIC_BLOCK_OPS                  *ProcBlockOps
  )
{
  EFI_STATUS                              Status;
  CM_RISCV_PROCESSOR_SPECIFIC_BLOCK_INFO  *Block;
  UINT32                                  BlockCount;

  Status = GetERiscVObjProcessorSpecificBlockInfo (
             CfgMgrProtocol,
             Token,
             &Block,
             &BlockCount
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get risc-v processor data info. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  ProcBlockOps->CmObject = Block;

  return EFI_SUCCESS;
}

/** Get size of Arm Processor Specific Block.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CmObject             CM object of Processor Specific Block.
  @param [out]      Size                 Size of Processor Specific Block.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofArmProcBlock (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST VOID                                          *CmObject,
  OUT     UINT32                                              *Size
  )
{
  EFI_STATUS                                  Status;
  CONST CM_ARM_PROCESSOR_SPECIFIC_BLOCK_INFO  *Block;
  UINT32                                      BlockSize;
  ARM_PROCESSOR_SUB_DATA_OPS                  *SubDataOps;
  UINT32                                      SubDataSize;

  Block     = CmObject;
  *Size     = 0;
  BlockSize = 0;

  if (Block->SubType >= ArmProcessorSpecificDataSubTypeMax) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid sub-data type\n",
      __func__
      ));
    return EFI_INVALID_PARAMETER;
  }

  switch (Block->Revision) {
    case PROCESSOR_SPECIFIC_VERSION_INFO (1, 0):
      BlockSize += sizeof (ARM_PROCESSOR_SPECIFIC_BLOCK);
      break;
    default:
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid revision for ARM_PROCESSOR_SPECIFIC_BLOCK: 0x%x\n",
        __func__,
        Block->Revision
        ));
      return EFI_INVALID_PARAMETER;
  }

  SubDataOps = &mArmProcSubDataOps[Block->SubType];

  Status = SubDataOps->GetProcSubDataCmObj (
                         CfgMgrProtocol,
                         Block->SubDataToken,
                         SubDataOps
                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to Get sub-data cm object: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Status = SubDataOps->GetSizeofProcSubData (
                         CfgMgrProtocol,
                         SubDataOps->CmObject,
                         &SubDataSize
                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to Get sub-data size: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  BlockSize += SubDataSize;
  *Size      = BlockSize;

  return EFI_SUCCESS;
}

/** Get size of x86 (x64) Processor Specific Block.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CmObject             CM object of Processor Specific Block.
  @param [out]      Size                 Size of Processor Specific Block.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofX64ProcBlock (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST VOID                                          *CmObject,
  OUT     UINT32                                              *Size
  )
{
  CONST CM_X64_PROCESSOR_SPECIFIC_BLOCK_INFO  *Block;

  Block = CmObject;
  *Size = 0;

  if (Block->BlockIdentifier == X86_PROCESSOR_BLOCK_IDENTIFIER_USE_CONDITION_DATA) {
    switch (Block->Revision) {
      case PROCESSOR_SPECIFIC_VERSION_INFO (1, 0):
        *Size = sizeof (X86_PROCESSOR_SPECIFIC_BLOCK);
        break;
      default:
        DEBUG ((
          DEBUG_ERROR,
          "%a: Invalid revision for USE_CONDITION_DATA block.: 0x%x\n",
          __func__,
          Block->Revision
          ));
        return EFI_INVALID_PARAMETER;
    }
  } else {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid Block Identifier: 0x%x\n",
      __func__,
      Block->BlockIdentifier
      ));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Get size of RiscV Processor Specific Block.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CmObject             CM object of Processor Specific Block.
  @param [out]      Size                 Size of Processor Specific Block.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofRiscVProcBlock (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST VOID                                          *CmObject,
  OUT     UINT32                                              *Size
  )
{
  CONST CM_RISCV_PROCESSOR_SPECIFIC_BLOCK_INFO  *Block;

  Block = CmObject;
  *Size = 0;

  switch (Block->Revision) {
    case PROCESSOR_SPECIFIC_VERSION_INFO (1, 0):
      *Size = sizeof (RISCV_PROCESSOR_SPECIFIC_BLOCK);
      break;
    default:
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid revision for risc-v block.: 0x%x\n",
        __func__,
        Block->Revision
        ));
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Add Arm Processor Specific Block into SMBIOS record.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CmObject             CM object of Processor Specific Block.
  @param [out]      SmbiosRecord         Type 44 Smbios Record.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddArmProcBlock (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST VOID                                          *CmObject,
  OUT     SMBIOS_TABLE_TYPE44                                 *SmbiosRecord
  )
{
  EFI_STATUS                                  Status;
  ARM_PROCESSOR_SPECIFIC_BLOCK                *ProcBlock;
  CONST CM_ARM_PROCESSOR_SPECIFIC_BLOCK_INFO  *Block;
  ARM_PROCESSOR_SUB_DATA_OPS                  *SubDataOps;
  UINT32                                      SubDataSize;

  Block      = CmObject;
  SubDataOps = &mArmProcSubDataOps[Block->SubType];
  ProcBlock  = (ARM_PROCESSOR_SPECIFIC_BLOCK *)(SmbiosRecord + 1);

  Status = SubDataOps->GetSizeofProcSubData (
                         CfgMgrProtocol,
                         SubDataOps->CmObject,
                         &SubDataSize
                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to Get sub-data size: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  ProcBlock->Revision = Block->Revision;
  ProcBlock->Length   = sizeof (ARM_PROCESSOR_SPECIFIC_BLOCK) + SubDataSize;
  ProcBlock->VendorId = Block->VendorId;
  ProcBlock->SubType  = Block->SubType;

  return SubDataOps->AddProcSubData (
                       CfgMgrProtocol,
                       SubDataOps->CmObject,
                       ProcBlock
                       );
}

/** Add x86 (x64) Processor Specific Block into SMBIOS record.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CmObject             CM object of Processor Specific Block.
  @param [out]      SmbiosRecord         Type 44 Smbios Record.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddX64ProcBlock (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST VOID                                          *CmObject,
  OUT     SMBIOS_TABLE_TYPE44                                 *SmbiosRecord
  )
{
  X86_PROCESSOR_SPECIFIC_BLOCK                *ProcBlock;
  CONST CM_X64_PROCESSOR_SPECIFIC_BLOCK_INFO  *Block;

  Block     = CmObject;
  ProcBlock = (X86_PROCESSOR_SPECIFIC_BLOCK *)(SmbiosRecord + 1);

  ProcBlock->BlockIdentifier        = Block->BlockIdentifier;
  ProcBlock->BlockLength            = sizeof (X86_PROCESSOR_SPECIFIC_BLOCK);
  ProcBlock->Revision               = Block->Revision;
  ProcBlock->UseConditionAttributes = Block->UseConditionAttributes;

  return EFI_SUCCESS;
}

/** Add risc-v Processor Specific Block into SMBIOS record.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CmObject             CM object of Processor Specific Block.
  @param [out]      SmbiosRecord         Type 44 Smbios Record.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddRiscVProcBlock (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST VOID                                          *CmObject,
  OUT     SMBIOS_TABLE_TYPE44                                 *SmbiosRecord
  )
{
  RISCV_PROCESSOR_SPECIFIC_BLOCK                *ProcBlock;
  CONST CM_RISCV_PROCESSOR_SPECIFIC_BLOCK_INFO  *Block;

  Block     = CmObject;
  ProcBlock = (RISCV_PROCESSOR_SPECIFIC_BLOCK *)(SmbiosRecord + 1);

  ProcBlock->Revision = Block->Revision;
  ProcBlock->HartId   = Block->HartId;
  ProcBlock->VendorId = Block->VendorId;
  ProcBlock->ArchId   = Block->ArchId;
  ProcBlock->ImplId   = Block->ImplId;

  return EFI_SUCCESS;
}

/** Operation table to handle Processor Specific Block to generate
    Smbios Type 44 record.
**/
STATIC PROCESSOR_SPECIFIC_BLOCK_OPS  mProcSpecificBlockOps[] = {
  {
    ProcessorSpecificBlockArchTypeReserved,
    NULL,
    NULL,
    NULL,
    TRUE,
  },
  {
    ProcessorSpecificBlockArchTypeIa32,
    GetX64ProcBlockCmObj,
    GetSizeofX64ProcBlock,
    AddX64ProcBlock,
    X86_SMBIOS_TYPE44_RECORD_UNSUPPORTED,
  },
  {
    ProcessorSpecificBlockArchTypeX64,
    GetX64ProcBlockCmObj,
    GetSizeofX64ProcBlock,
    AddX64ProcBlock,
    X86_SMBIOS_TYPE44_RECORD_UNSUPPORTED,
  },
  {
    ProcessorSpecificBlockArchTypeItanium,
    GetX64ProcBlockCmObj,
    GetSizeofX64ProcBlock,
    AddX64ProcBlock,
    X86_SMBIOS_TYPE44_RECORD_UNSUPPORTED,
  },
  {
    ProcessorSpecificBlockArchTypeAarch32,
    GetArmProcBlockCmObj,
    GetSizeofArmProcBlock,
    AddArmProcBlock,
    ARM_SMBIOS_TYPE44_RECORD_UNSUPPORTED,
  },
  {
    ProcessorSpecificBlockArchTypeAarch64,
    GetArmProcBlockCmObj,
    GetSizeofArmProcBlock,
    AddArmProcBlock,
    ARM_SMBIOS_TYPE44_RECORD_UNSUPPORTED,
  },
  {
    ProcessorSpecificBlockArchTypeRiscVRV32,
    GetRiscVProcBlockCmObj,
    GetSizeofRiscVProcBlock,
    AddRiscVProcBlock,
    RISCV_SMBIOS_TYPE44_RECORD_UNSUPPORTED,
  },
  {
    ProcessorSpecificBlockArchTypeRiscVRV64,
    GetRiscVProcBlockCmObj,
    GetSizeofRiscVProcBlock,
    AddRiscVProcBlock,
    RISCV_SMBIOS_TYPE44_RECORD_UNSUPPORTED,
  },
  {
    ProcessorSpecificBlockArchTypeRiscVRV128,
    GetRiscVProcBlockCmObj,
    GetSizeofRiscVProcBlock,
    AddRiscVProcBlock,
    RISCV_SMBIOS_TYPE44_RECORD_UNSUPPORTED,
  },
};

/** Free any resources allocated when installing SMBIOS Type44 table.

 @param [in]  This                 Pointer to the SMBIOS table generator.
 @param [in]  TableFactoryProtocol Pointer to the SMBIOS Table Factory
                                   Protocol interface.
 @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table information.
 @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                   Protocol interface.
 @param [in]  Table                Pointer to the SMBIOS table.
 @param [in]  CmObjectToken        Pointer to the CM ObjectToken Array.
 @param [in]  TableCount           Number of SMBIOS tables.

 @retval EFI_SUCCESS            Table generated successfully.
 @retval EFI_BAD_BUFFER_SIZE    The size returned by the Configuration
                                Manager is less than the Object size for
                                the requested object.
 @retval EFI_INVALID_PARAMETER  A parameter is invalid.
 @retval EFI_NOT_FOUND          Could not find information.
 @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
 @retval EFI_UNSUPPORTED        Unsupported configuration.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSmbiosType44TableEx (
  IN      CONST SMBIOS_TABLE_GENERATOR                    *CONST   This,
  IN      CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL      *CONST   TableFactoryProtocol,
  IN      CONST CM_STD_OBJ_SMBIOS_TABLE_INFO              *CONST   SmbiosTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL      *CONST   CfgMgrProtocol,
  IN      SMBIOS_STRUCTURE                               ***CONST  Table,
  IN      CM_OBJECT_TOKEN                                          **CmObjectToken,
  IN      CONST UINTN                                              TableCount
  )
{
  UINTN             Index;
  SMBIOS_STRUCTURE  **TableList;

  TableList = *Table;
  for (Index = 0; Index < TableCount; Index++) {
    if (TableList[Index] != NULL) {
      FreePool (TableList[Index]);
    }
  }

  if (TableList != NULL) {
    FreePool (TableList);
  }

  return EFI_SUCCESS;
}

/** Construct SMBIOS Type 44 Table describing Processor Specific Block.

  If this function allocates any resources then they must be freed
  in the FreeXXXXTableResources function.

 @param [in]  This                 Pointer to the SMBIOS table generator.
 @param [in]  TableFactoryProtocol Pointer to the SMBIOS Table Factory
                                   Protocol interface.
 @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table information.
 @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                   Protocol interface.
 @param [out] Table                Pointer to the SMBIOS table.
 @param [out] CmObjectToken        Pointer to the CM Object Token Array.
 @param [out] TableCount           Number of tables installed.

 @retval EFI_SUCCESS            Table generated successfully.
 @retval EFI_BAD_BUFFER_SIZE    The size returned by the Configuration
                                Manager is less than the Object size for
                                the requested object.
 @retval EFI_INVALID_PARAMETER  A parameter is invalid.
 @retval EFI_NOT_FOUND          Could not find information.
 @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
 @retval EFI_UNSUPPORTED        Unsupported configuration.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSmbiosType44TableEx (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               ***Table,
  OUT       CM_OBJECT_TOKEN                                **CmObjectToken,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                                    Status;
  SMBIOS_STRUCTURE                              **TableList;
  SMBIOS_TABLE_TYPE44                           *SmbiosRecord;
  UINTN                                         SmbiosRecordSize;
  UINT16                                        Type4Handle;
  UINTN                                         Index;
  CM_ARCH_COMMON_PROCESSOR_SPECIFIC_BLOCK_INFO  *ProcSpecificBlockList;
  UINT32                                        ProcSpecificBlockCount;
  PROCESSOR_SPECIFIC_BLOCK_OPS                  *ProcBlockOps;
  UINT32                                        ProcBlockSize;

  ASSERT (This != NULL);
  ASSERT (SmbiosTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (CmObjectToken != NULL);
  ASSERT (TableCount != NULL);
  ASSERT (SmbiosTableInfo->TableGeneratorId == This->GeneratorID);

  if ((This == NULL) || (SmbiosTableInfo == NULL) || (CfgMgrProtocol == NULL) ||
      (Table == NULL) || (TableCount == NULL) || (CmObjectToken == NULL) ||
      (SmbiosTableInfo->TableGeneratorId != This->GeneratorID))
  {
    DEBUG ((DEBUG_ERROR, "%a:Invalid Paramater\n ", __func__));
    return EFI_INVALID_PARAMETER;
  }

  TableList   = NULL;
  *Table      = NULL;
  *TableCount = 0;

  Status = GetEArchCommonObjProcessorSpecificBlockInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &ProcSpecificBlockList,
             &ProcSpecificBlockCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get processor hierarchy info. Status = %r\n",
      __func__,
      Status
      ));
    return EFI_INVALID_PARAMETER;
  }

  TableList = (SMBIOS_STRUCTURE **)AllocateZeroPool (sizeof (SMBIOS_STRUCTURE *) * ProcSpecificBlockCount);
  if (TableList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to alloc memory for %u devices table\n",
      __func__,
      ProcSpecificBlockCount
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorHandler;
  }

  for (Index = 0; Index < ProcSpecificBlockCount; Index++) {
    if (ProcSpecificBlockList[Index].ProcArchType >= ProcessorSpecificBlockArchTypeLoongArch32) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Unsupported Type: 0x%x\n",
        __func__,
        ProcSpecificBlockList[Index].ProcArchType
        ));
      Status = EFI_INVALID_PARAMETER;
      goto ErrorHandler;
    }

    ProcBlockOps = &mProcSpecificBlockOps[ProcSpecificBlockList[Index].ProcArchType];
    if (ProcBlockOps->Unsupported) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Unsupported Type: 0x%x\n",
        __func__,
        ProcSpecificBlockList[Index].ProcArchType
        ));
      Status = EFI_INVALID_PARAMETER;
      goto ErrorHandler;
    }

    Type4Handle = FindSmbiosHandleEx (
                    CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType04),
                    ProcSpecificBlockList[Index].ProcSocketToken
                    );
    if (Type4Handle == SMBIOS_HANDLE_INVALID) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to get related Type4 Handle. Token: %lx\n",
        __func__,
        ProcSpecificBlockList[Index].ProcSocketToken
        ));
      Status = EFI_INVALID_PARAMETER;
      goto ErrorHandler;
    }

    Status = ProcBlockOps->GetProcBlockCmObj (
                             CfgMgrProtocol,
                             ProcSpecificBlockList[Index].ArchProcessorSpecificDataToken,
                             ProcBlockOps
                             );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to get Processor specific data CM object. Token: %lx\n",
        __func__,
        ProcSpecificBlockList[Index].ArchProcessorSpecificDataToken
        ));
      goto ErrorHandler;
    }

    Status = ProcBlockOps->GetSizeofProcBlock (
                             CfgMgrProtocol,
                             ProcBlockOps->CmObject,
                             &ProcBlockSize
                             );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to get Processor specific data size.\n",
        __func__
        ));
      goto ErrorHandler;
    }

    // Include the empty string table.
    SmbiosRecordSize = sizeof (SMBIOS_TABLE_TYPE44) + ProcBlockSize + 2;

    SmbiosRecord = AllocateZeroPool (SmbiosRecordSize);
    if (SmbiosRecord == NULL) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to allocate SmbiosRecord.\n",
        __func__
        ));
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorHandler;
    }

    // Set up the header
    SmbiosRecord->Hdr.Type                                 = EFI_SMBIOS_TYPE_PROCESSOR_ADDITIONAL_INFORMATION;
    SmbiosRecord->Hdr.Length                               = SmbiosRecordSize - 2;
    SmbiosRecord->RefHandle                                = Type4Handle;
    SmbiosRecord->ProcessorSpecificBlock.ProcessorArchType = ProcSpecificBlockList[Index].ProcArchType;
    SmbiosRecord->ProcessorSpecificBlock.Length            = ProcBlockSize;

    Status = ProcBlockOps->AddProcBlock (
                             CfgMgrProtocol,
                             ProcBlockOps->CmObject,
                             SmbiosRecord
                             );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to add Processor Specific Data into SmbiosRecord Status=%r.\n",
        __func__,
        Status
        ));
      FreePool (SmbiosRecord);
      goto ErrorHandler;
    }

    TableList[Index] = (SMBIOS_STRUCTURE *)SmbiosRecord;
  }

  ASSERT (Index == ProcSpecificBlockCount);

  *Table         = TableList;
  *CmObjectToken = NULL;
  *TableCount    = ProcSpecificBlockCount;

  return EFI_SUCCESS;

ErrorHandler:
  if (TableList != NULL) {
    while (Index-- != 0) {
      if (TableList[Index] != NULL) {
        FreePool (TableList[Index]);
      }
    }

    FreePool (TableList);
  }

  return Status;
}

/** The interface for the SMBIOS Type4 Table Generator.
*/
STATIC
CONST
SMBIOS_TABLE_GENERATOR  SmbiosType44Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType44),
  // Generator Description
  L"SMBIOS.TYPE44.GENERATOR",
  // SMBIOS Table Type
  EFI_SMBIOS_TYPE_PROCESSOR_ADDITIONAL_INFORMATION,
  NULL,
  NULL,
  // Build table function.
  BuildSmbiosType44TableEx,
  // Free function.
  FreeSmbiosType44TableEx,
};

/** Register the Generator with the SMBIOS Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID
                                is already registered.
**/
EFI_STATUS
EFIAPI
SmbiosType44LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType44Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 44: Register Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Deregister the Generator from the SMBIOS Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is deregistered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The Generator is not registered.
**/
EFI_STATUS
EFIAPI
SmbiosType44LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType44Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 44: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
