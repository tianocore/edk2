/** @file
  Hest Table Generator

  Copyright (c) 2026, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.6 specification
  (https://uefi.org/specs/ACPI/6.6/18_Platform_Error_Interfaces.html)

  @par Glossary:
  - Cm or CM   - Configuration Manager
  - Obj or OBJ - Object
**/

#include <Library/AcpiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/AcpiHelperLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "HestGenerator.h"

/**
  Hest Generator

  Requirements:
    The following Configuration Manager Object(s) are used by this Generator:
    - EObjNameSpaceArchCommon,
    - EArchCommonObjErrSourcePciRootPortInfo
    - EArchCommonObjErrSourcePciBridgeInfo
    - EArchCommonObjErrSourcePciDeviceInfo
    - EArchCommonObjErrSourceGenericHwInfo
    - EArchCommonObjErrSourceGenericHwVer2Info
    - EX64ObjIa32MachineCheckBankInfo
    - EX64ObjErrSourceIa32MachineCheckExceptionInfo
    - EX64ObjErrSourceIa32CorrectedMachineCheckInfo
    - EX64ObjErrSourceIa32DeferredMachineCheckInfo
    - EX64ObjErrSourceIa32NmiInfo
*/

/**
  This macro expands to a function that retrieves the Processor Hierarchy
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjCmRef,
  CM_ARCH_COMMON_OBJ_REF
  );

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjErrSourcePciRootPortInfo,
  CM_ARCH_COMMON_ERROR_SOURCE_PCI_ROOT_PORT_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjErrSourcePciBridgeInfo,
  CM_ARCH_COMMON_ERROR_SOURCE_PCI_BRIDGE_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjErrSourcePciDeviceInfo,
  CM_ARCH_COMMON_ERROR_SOURCE_PCI_DEVICE_INFO
  )

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjErrSourceGenericHwInfo,
  CM_ARCH_COMMON_ERROR_SOURCE_GENERIC_HW_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjErrSourceGenericHwVer2Info,
  CM_ARCH_COMMON_ERROR_SOURCE_GENERIC_HW_VERSION2_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjIa32MachineCheckBankInfo,
  CM_X64_IA32_MACHINE_CHECK_BANK_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjErrSourceIa32MachineCheckExceptionInfo,
  CM_X64_ERROR_SOURCE_IA32_MACHINE_CHECK_EXCEPTION_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjErrSourceIa32CorrectedMachineCheckInfo,
  CM_X64_ERROR_SOURCE_IA32_CORRECTED_MACHINE_CHECK_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjErrSourceIa32DeferredMachineCheckInfo,
  CM_X64_ERROR_SOURCE_IA32_DEFERRED_MACHINE_CHECK_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjErrSourceIa32NmiInfo,
  CM_X64_ERROR_SOURCE_IA32_NMI_INFO
  );

STATIC UINT16  mNextSourceId = 0;

/** Find Firmware First source indexer by Token.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       Token                Error source token.

  @retval NULL                           Not found.
  @retval Others                         Firmware first source indexer
                                         associated with Token.
**/
STATIC
HEST_SOURCE_INDEXER *
EFIAPI
FindFirmwareFirstSourceIndexer (
  IN  ACPI_HEST_GENERATOR  *Generator,
  IN  CM_OBJECT_TOKEN      Token
  )
{
  UINT32  Idx;

  for (Idx = 0; Generator->FirmwareFirstSourceCount; Idx++) {
    if (Generator->FirmwareFirstIndexer[Idx].Token == Token) {
      return &Generator->FirmwareFirstIndexer[Idx];
    }
  }

  return NULL;
}

/** Find GHES assist source indexer by Token.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       Token                Error source token.

  @retval NULL                           Not found.
  @retval Others                         GHES assist source indexer
                                         associated with Token.
**/
STATIC
HEST_SOURCE_INDEXER *
EFIAPI
FindGhesAssistSourceIndexer (
  IN  ACPI_HEST_GENERATOR  *Generator,
  IN  CM_OBJECT_TOKEN      Token
  )
{
  UINT32  Idx;

  for (Idx = 0; Generator->GhesAssistSourceCount; Idx++) {
    if (Generator->GhesAssistIndexer[Idx].Token == Token) {
      return &Generator->GhesAssistIndexer[Idx];
    }
  }

  return NULL;
}

/** Get the total size of banks related for Machine Check Error source entry.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       BankInfoArrayToken   Count of Machine Check Error source.
  @param [out]      BankCount            Count of the banks.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetNumberofMachineCheckBankInfo (
  IN            ACPI_HEST_GENERATOR                           *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN            CM_ARCH_COMMON_OBJ_REF                        BankInfoArrayToken,
  OUT           UINT32                                        *BankCount
  )
{
  EFI_STATUS                           Status;
  CM_ARCH_COMMON_OBJ_REF               *BankRefList;
  UINT32                               BankRefCount;
  UINT32                               BankIdx;
  CM_X64_IA32_MACHINE_CHECK_BANK_INFO  *BankInfo;
  UINT32                               BankInfoCount;

  *BankCount = 0;

  Status = GetEArchCommonObjCmRef (
             CfgMgrProtocol,
             BankInfoArrayToken.ReferenceToken,
             &BankRefList,
             &BankRefCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HEST: Failed to get Bank array for Machine Check Error Source: "
      "Token = %p, Status = %r\n",
      BankInfoArrayToken,
      Status
      ));
    goto ErrorHandler;
  }

  for (BankIdx = 0; BankIdx < BankRefCount; BankIdx++) {
    Status = GetEX64ObjIa32MachineCheckBankInfo (
               CfgMgrProtocol,
               BankRefList[BankIdx].ReferenceToken,
               &BankInfo,
               &BankInfoCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Couldn't find Machine Bank Info: "
        "Token = %p, Status = %r\n",
        BankRefList[BankIdx].ReferenceToken,
        Status
        ));
      goto ErrorHandler;
    }
  }

  *BankCount = BankRefCount;

  return EFI_SUCCESS;

ErrorHandler:
  *BankCount = 0;
  return Status;
}

/** Get the total size required for Machine Check Exception Error source entry
    and Update the Indexers.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       StartOffset          Offset from the start of HEST where
                                         Machine Check Exception Error source starts.
  @param [in]       ErrSourceList        Point to Machine Check Exception Error source list.
  @param [in]       ErrSourceCount       Count of Machine Check Exception Error source.
  @param [out]      TotalSize            Total size of the Machine Check Exception Error source.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofMachineCheckExceptionErrSource (
  IN            ACPI_HEST_GENERATOR                                    *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      CONST UINT32                                                 StartOffset,
  IN      CONST CM_X64_ERROR_SOURCE_IA32_MACHINE_CHECK_EXCEPTION_INFO  *ErrSourceList,
  IN            UINT32                                                 ErrSourceCount,
  OUT           UINT32                                                 *TotalSize
  )
{
  EFI_STATUS           Status;
  UINT32               Idx;
  UINT32               ErrSourceOffset;
  UINT32               ErrSourceSize;
  UINT32               BankCount;
  HEST_SOURCE_INDEXER  *FirmwareFirstIndexer;
  UINT32               FirmwareFirstIdx;
  HEST_SOURCE_INDEXER  *GhesAssistIndexer;
  UINT32               GhesAssistIdx;

  Status               = EFI_INVALID_PARAMETER;
  ErrSourceOffset      = StartOffset;
  *TotalSize           = 0;
  FirmwareFirstIdx     = Generator->FirmwareFirstSourceCount;
  FirmwareFirstIndexer = &Generator->FirmwareFirstIndexer[FirmwareFirstIdx];
  GhesAssistIdx        = Generator->GhesAssistSourceCount;
  GhesAssistIndexer    = &Generator->GhesAssistIndexer[GhesAssistIdx];

  for (Idx = 0; Idx < ErrSourceCount; Idx++) {
    ErrSourceSize = sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_EXCEPTION_STRUCTURE);

    if (ErrSourceList[Idx].Common.FirmwareFirst &&
        ErrSourceList[Idx].Common.GhesAssist)
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Machine Check Exception Error Source[%d] has both FirmwareFirst and GhesAssist.\n",
        Idx
        ));
      goto ErrorHandler;
    }

    if (ErrSourceList[Idx].Common.MachineBankInfoTokenArray.ReferenceToken != CM_NULL_TOKEN) {
      Status = GetNumberofMachineCheckBankInfo (
                 Generator,
                 CfgMgrProtocol,
                 ErrSourceList[Idx].Common.MachineBankInfoTokenArray,
                 &BankCount
                 );
      if (EFI_ERROR (Status)) {
        goto ErrorHandler;
      }
    } else {
      BankCount = 0;
    }

    if (ErrSourceList[Idx].Common.FirmwareFirst) {
      FirmwareFirstIndexer->Token  = ErrSourceList[Idx].Token;
      FirmwareFirstIndexer->Object = (VOID *)&ErrSourceList[Idx];
      FirmwareFirstIndexer->Offset = ErrSourceOffset;
      FirmwareFirstIndexer++;
      Generator->FirmwareFirstSourceCount++;
    }

    if (ErrSourceList[Idx].Common.GhesAssist) {
      GhesAssistIndexer->Token  = ErrSourceList[Idx].Token;
      GhesAssistIndexer->Object = (VOID *)&ErrSourceList[Idx];
      GhesAssistIndexer->Offset = ErrSourceOffset;
      GhesAssistIndexer++;
      Generator->GhesAssistSourceCount++;
    }

    ErrSourceSize   += BankCount * sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_BANK_STRUCTURE);
    ErrSourceOffset += ErrSourceSize;
    *TotalSize      += ErrSourceSize;
  }

  return EFI_SUCCESS;

ErrorHandler:
  *TotalSize = 0;
  return Status;
}

/** Get the total size required for Corrected Machine Check Error source entry
    and Update the Indexers.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       StartOffset          Offset from the start of HEST where
                                         Corrected Machine Check Error source starts.
  @param [in]       ErrSourceList        Point to Corrected Machine Check Error source list.
  @param [in]       ErrSourceCount       Count of Corrected Machine Check Error source.
  @param [out]      TotalSize            Total size of the Corrected Machine Check Error source.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofCorrectedMachineCheckErrSource (
  IN            ACPI_HEST_GENERATOR                                    *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      CONST UINT32                                                 StartOffset,
  IN      CONST CM_X64_ERROR_SOURCE_IA32_CORRECTED_MACHINE_CHECK_INFO  *ErrSourceList,
  IN            UINT32                                                 ErrSourceCount,
  OUT           UINT32                                                 *TotalSize
  )
{
  EFI_STATUS           Status;
  UINT32               Idx;
  UINT32               ErrSourceOffset;
  UINT32               ErrSourceSize;
  UINT32               BankCount;
  HEST_SOURCE_INDEXER  *FirmwareFirstIndexer;
  UINT32               FirmwareFirstIdx;
  HEST_SOURCE_INDEXER  *GhesAssistIndexer;
  UINT32               GhesAssistIdx;

  Status               = EFI_INVALID_PARAMETER;
  ErrSourceOffset      = StartOffset;
  *TotalSize           = 0;
  FirmwareFirstIdx     = Generator->FirmwareFirstSourceCount;
  FirmwareFirstIndexer = &Generator->FirmwareFirstIndexer[FirmwareFirstIdx];
  GhesAssistIdx        = Generator->GhesAssistSourceCount;
  GhesAssistIndexer    = &Generator->GhesAssistIndexer[GhesAssistIdx];

  for (Idx = 0; Idx < ErrSourceCount; Idx++) {
    ErrSourceSize = sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_CORRECTED_MACHINE_CHECK_STRUCTURE);

    if (ErrSourceList[Idx].Common.FirmwareFirst &&
        ErrSourceList[Idx].Common.GhesAssist)
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Corrected Machine Check Error Source[%d] has both FirmwareFirst and GhesAssist.\n",
        Idx
        ));
      goto ErrorHandler;
    }

    if (ErrSourceList[Idx].Common.MachineBankInfoTokenArray.ReferenceToken != CM_NULL_TOKEN) {
      Status = GetNumberofMachineCheckBankInfo (
                 Generator,
                 CfgMgrProtocol,
                 ErrSourceList[Idx].Common.MachineBankInfoTokenArray,
                 &BankCount
                 );
      if (EFI_ERROR (Status)) {
        goto ErrorHandler;
      }
    } else {
      BankCount = 0;
    }

    if (ErrSourceList[Idx].Common.FirmwareFirst) {
      FirmwareFirstIndexer->Token  = ErrSourceList[Idx].Token;
      FirmwareFirstIndexer->Object = (VOID *)&ErrSourceList[Idx];
      FirmwareFirstIndexer->Offset = ErrSourceOffset;
      FirmwareFirstIndexer++;
      Generator->FirmwareFirstSourceCount++;
    }

    if (ErrSourceList[Idx].Common.GhesAssist) {
      GhesAssistIndexer->Token  = ErrSourceList[Idx].Token;
      GhesAssistIndexer->Object = (VOID *)&ErrSourceList[Idx];
      GhesAssistIndexer->Offset = ErrSourceOffset;
      GhesAssistIndexer++;
      Generator->GhesAssistSourceCount++;
    }

    ErrSourceSize   += BankCount * sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_BANK_STRUCTURE);
    ErrSourceOffset += ErrSourceSize;
    *TotalSize      += ErrSourceSize;
  }

  return EFI_SUCCESS;

ErrorHandler:
  *TotalSize = 0;
  return Status;
}

/** Get the total size required for Deferred Machine Check Error source entry
    and Update the Indexers.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       StartOffset          Offset from the start of HEST where
                                         Deferred Machine Check Error source starts.
  @param [in]       ErrSourceList        Point to Deferred Machine Check Error source list.
  @param [in]       ErrSourceCount       Count of Deferred Machine Check Error source.
  @param [out]      TotalSize            Total size of the Deferred Machine Check Error source.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofDeferredMachineCheckErrSource (
  IN            ACPI_HEST_GENERATOR                                   *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST          CfgMgrProtocol,
  IN      CONST UINT32                                                StartOffset,
  IN      CONST CM_X64_ERROR_SOURCE_IA32_DEFERRED_MACHINE_CHECK_INFO  *ErrSourceList,
  IN            UINT32                                                ErrSourceCount,
  OUT           UINT32                                                *TotalSize
  )
{
  EFI_STATUS           Status;
  UINT32               Idx;
  UINT32               ErrSourceOffset;
  UINT32               ErrSourceSize;
  UINT32               BankCount;
  HEST_SOURCE_INDEXER  *FirmwareFirstIndexer;
  UINT32               FirmwareFirstIdx;
  HEST_SOURCE_INDEXER  *GhesAssistIndexer;
  UINT32               GhesAssistIdx;

  Status               = EFI_INVALID_PARAMETER;
  ErrSourceOffset      = StartOffset;
  *TotalSize           = 0;
  FirmwareFirstIdx     = Generator->FirmwareFirstSourceCount;
  FirmwareFirstIndexer = &Generator->FirmwareFirstIndexer[FirmwareFirstIdx];
  GhesAssistIdx        = Generator->GhesAssistSourceCount;
  GhesAssistIndexer    = &Generator->GhesAssistIndexer[GhesAssistIdx];

  for (Idx = 0; Idx < ErrSourceCount; Idx++) {
    ErrSourceSize = sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_DEFERRED_MACHINE_CHECK_STRUCTURE);

    if (ErrSourceList[Idx].Common.FirmwareFirst &&
        ErrSourceList[Idx].Common.GhesAssist)
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Deferred Machine Check Error Source[%d] has both FirmwareFirst and GhesAssist.\n",
        Idx
        ));
      goto ErrorHandler;
    }

    if (ErrSourceList[Idx].Common.MachineBankInfoTokenArray.ReferenceToken != CM_NULL_TOKEN) {
      Status = GetNumberofMachineCheckBankInfo (
                 Generator,
                 CfgMgrProtocol,
                 ErrSourceList[Idx].Common.MachineBankInfoTokenArray,
                 &BankCount
                 );
      if (EFI_ERROR (Status)) {
        goto ErrorHandler;
      }
    } else {
      BankCount = 0;
    }

    if (ErrSourceList[Idx].Common.FirmwareFirst) {
      FirmwareFirstIndexer->Token  = ErrSourceList[Idx].Token;
      FirmwareFirstIndexer->Object = (VOID *)&ErrSourceList[Idx];
      FirmwareFirstIndexer->Offset = ErrSourceOffset;
      FirmwareFirstIndexer++;
      Generator->FirmwareFirstSourceCount++;
    }

    if (ErrSourceList[Idx].Common.GhesAssist) {
      GhesAssistIndexer->Token  = ErrSourceList[Idx].Token;
      GhesAssistIndexer->Object = (VOID *)&ErrSourceList[Idx];
      GhesAssistIndexer->Offset = ErrSourceOffset;
      GhesAssistIndexer++;
      Generator->GhesAssistSourceCount++;
    }

    ErrSourceSize   += BankCount * sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_BANK_STRUCTURE);
    ErrSourceOffset += ErrSourceSize;
    *TotalSize      += ErrSourceSize;
  }

  return EFI_SUCCESS;

ErrorHandler:
  *TotalSize = 0;
  return Status;
}

/** Get the total size required for PCI Root Port error source entry and
    Update the Indexers.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       StartOffset          Offset from the start of HEST where
                                         Machine Check Error source starts.
  @param [in]       ErrSourceList        Point to PCI Root Port error source list.
  @param [in]       ErrSourceCount       Count of Error source.
  @param [out]      TotalSize            Total size of the PCI Root Port error source.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofPciRootPortErrSource (
  IN            ACPI_HEST_GENERATOR                             *Generator,
  IN      CONST UINT32                                          StartOffset,
  IN      CONST CM_ARCH_COMMON_ERROR_SOURCE_PCI_ROOT_PORT_INFO  *ErrSourceList,
  IN            UINT32                                          ErrSourceCount,
  OUT           UINT32                                          *TotalSize
  )
{
  EFI_STATUS           Status;
  UINT32               Idx;
  UINT32               ErrSourceSize;
  UINT32               ErrSourceOffset;
  HEST_SOURCE_INDEXER  *FirmwareFirstIndexer;
  UINT32               FirmwareFirstIdx;

  Status               = EFI_INVALID_PARAMETER;
  *TotalSize           = 0;
  ErrSourceOffset      = StartOffset;
  FirmwareFirstIdx     = Generator->FirmwareFirstSourceCount;
  FirmwareFirstIndexer = &Generator->FirmwareFirstIndexer[FirmwareFirstIdx];
  ErrSourceSize        = sizeof (EFI_ACPI_6_6_PCI_EXPRESS_ROOT_PORT_AER_STRUCTURE);

  for (Idx = 0; Idx < ErrSourceCount; Idx++) {
    if (ErrSourceList[Idx].Common.Global && (ErrSourceCount > 0)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: PCI Root Port error source type(%d) should have "
        "only one entry when Global is set.\n",
        Idx
        ));
      goto ErrorHandler;
    }

    if (ErrSourceList[Idx].Common.FirmwareFirst) {
      FirmwareFirstIndexer->Token  = ErrSourceList[Idx].Token;
      FirmwareFirstIndexer->Object = (VOID *)&ErrSourceList[Idx];
      FirmwareFirstIndexer->Offset = ErrSourceOffset;
      FirmwareFirstIndexer++;
      Generator->FirmwareFirstSourceCount++;
    }

    ErrSourceOffset += ErrSourceSize;
    *TotalSize      += ErrSourceSize;
  }

  return EFI_SUCCESS;

ErrorHandler:
  *TotalSize = 0;
  return Status;
}

/** Get the total size required for PCI Bridge error source entry and
    Update the Indexers.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       StartOffset          Offset from the start of HEST where
                                         Machine Check Error source starts.
  @param [in]       ErrSourceList        Point to PCI Bridge error source list.
  @param [in]       ErrSourceCount       Count of Error source.
  @param [out]      TotalSize            Total size of the PCI Bridge error source.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofPciBridgeErrSource (
  IN            ACPI_HEST_GENERATOR                          *Generator,
  IN      CONST UINT32                                       StartOffset,
  IN      CONST CM_ARCH_COMMON_ERROR_SOURCE_PCI_BRIDGE_INFO  *ErrSourceList,
  IN            UINT32                                       ErrSourceCount,
  OUT           UINT32                                       *TotalSize
  )
{
  EFI_STATUS           Status;
  UINT32               Idx;
  UINT32               ErrSourceSize;
  UINT32               ErrSourceOffset;
  HEST_SOURCE_INDEXER  *FirmwareFirstIndexer;
  UINT32               FirmwareFirstIdx;

  Status               = EFI_INVALID_PARAMETER;
  *TotalSize           = 0;
  ErrSourceOffset      = StartOffset;
  FirmwareFirstIdx     = Generator->FirmwareFirstSourceCount;
  FirmwareFirstIndexer = &Generator->FirmwareFirstIndexer[FirmwareFirstIdx];
  ErrSourceSize        = sizeof (EFI_ACPI_6_6_PCI_EXPRESS_BRIDGE_AER_STRUCTURE);

  for (Idx = 0; Idx < ErrSourceCount; Idx++) {
    if (ErrSourceList[Idx].Common.Global && (ErrSourceCount > 0)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: PCI Bridge error source type(%d) should have only one entry "
        "when Global is set.\n",
        Idx
        ));
      goto ErrorHandler;
    }

    if (ErrSourceList[Idx].Common.FirmwareFirst) {
      FirmwareFirstIndexer->Token  = ErrSourceList[Idx].Token;
      FirmwareFirstIndexer->Object = (VOID *)&ErrSourceList[Idx];
      FirmwareFirstIndexer->Offset = ErrSourceOffset;
      FirmwareFirstIndexer++;
      Generator->FirmwareFirstSourceCount++;
    }

    ErrSourceOffset += ErrSourceSize;
    *TotalSize      += ErrSourceSize;
  }

  return EFI_SUCCESS;

ErrorHandler:
  *TotalSize = 0;
  return Status;
}

/** Get the total size required for PCI Device error source entry and
    Update the Indexers.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       StartOffset          Offset from the start of HEST where
                                         Machine Check Error source starts.
  @param [in]       ErrSourceList        Point to PCI Device error source list.
  @param [in]       ErrSourceCount       Count of Error source.
  @param [out]      TotalSize            Total size of the PCI Device error source.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofPciDeviceErrSource (
  IN            ACPI_HEST_GENERATOR                          *Generator,
  IN      CONST UINT32                                       StartOffset,
  IN      CONST CM_ARCH_COMMON_ERROR_SOURCE_PCI_DEVICE_INFO  *ErrSourceList,
  IN            UINT32                                       ErrSourceCount,
  OUT           UINT32                                       *TotalSize
  )
{
  EFI_STATUS           Status;
  UINT32               Idx;
  UINT32               ErrSourceSize;
  UINT32               ErrSourceOffset;
  HEST_SOURCE_INDEXER  *FirmwareFirstIndexer;
  UINT32               FirmwareFirstIdx;

  Status               = EFI_INVALID_PARAMETER;
  *TotalSize           = 0;
  ErrSourceOffset      = StartOffset;
  FirmwareFirstIdx     = Generator->FirmwareFirstSourceCount;
  FirmwareFirstIndexer = &Generator->FirmwareFirstIndexer[FirmwareFirstIdx];
  ErrSourceSize        = sizeof (EFI_ACPI_6_6_PCI_EXPRESS_DEVICE_AER_STRUCTURE);

  for (Idx = 0; Idx < ErrSourceCount; Idx++) {
    if (ErrSourceList[Idx].Common.Global && (ErrSourceCount > 0)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: PCI Device error source type(%d) should have only one entry "
        "when Global is set.\n",
        Idx
        ));
      goto ErrorHandler;
    }

    if (ErrSourceList[Idx].Common.FirmwareFirst) {
      FirmwareFirstIndexer->Token  = ErrSourceList[Idx].Token;
      FirmwareFirstIndexer->Object = (VOID *)&ErrSourceList[Idx];
      FirmwareFirstIndexer->Offset = ErrSourceOffset;
      FirmwareFirstIndexer++;
      Generator->FirmwareFirstSourceCount++;
    }

    ErrSourceOffset += ErrSourceSize;
    *TotalSize      += ErrSourceSize;
  }

  return EFI_SUCCESS;

ErrorHandler:
  *TotalSize = 0;
  return Status;
}

/** Get the total size required for Ghes

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       StartOffset          Offset from the start of HEST where
                                         Machine Check Error source starts.
  @param [in]       ErrSourceList        Point to GHES list.
  @param [in]       ErrSourceCount       Count of GHES.
  @param [out]      TotalSize            Total size of the GHES.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofGhes (
  IN            ACPI_HEST_GENERATOR                          *Generator,
  IN      CONST UINT32                                       StartOffset,
  IN      CONST CM_ARCH_COMMON_ERROR_SOURCE_GENERIC_HW_INFO  *ErrSourceList,
  IN            UINT32                                       ErrSourceCount,
  OUT           UINT32                                       *TotalSize
  )
{
  UINT32               Idx;
  UINT32               ErrSourceOffset;
  UINT32               ErrSourceSize;
  HEST_SOURCE_INDEXER  *FirmwareFirstIndexer;
  HEST_SOURCE_INDEXER  *GhesAssistIndexer;

  ErrSourceOffset = StartOffset;
  ErrSourceSize   = sizeof (EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_SOURCE_STRUCTURE);
  *TotalSize      = 0;

  for (Idx = 0; Idx < ErrSourceCount; Idx++) {
    if (ErrSourceList[Idx].Common.RelatedSourceToken != CM_NULL_TOKEN) {
      FirmwareFirstIndexer = FindFirmwareFirstSourceIndexer (
                               Generator,
                               ErrSourceList[Idx].Common.RelatedSourceToken
                               );
      GhesAssistIndexer = FindGhesAssistSourceIndexer (
                            Generator,
                            ErrSourceList[Idx].Common.RelatedSourceToken
                            );
      if ((FirmwareFirstIndexer == NULL) && (GhesAssistIndexer == NULL)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: HEST: Invalid Related source for GHES[%d]. Token = %p\n",
          Idx,
          ErrSourceList[Idx].Common.RelatedSourceToken
          ));
        goto ErrorHandler;
      }

      if (FirmwareFirstIndexer != NULL) {
        FirmwareFirstIndexer->GhesFound = TRUE;
      }

      if (GhesAssistIndexer != NULL) {
        GhesAssistIndexer->GhesFound = TRUE;
      }
    }

    ErrSourceOffset += ErrSourceSize;
    *TotalSize      += ErrSourceSize;
  }

  return EFI_SUCCESS;

ErrorHandler:
  *TotalSize = 0;
  return EFI_INVALID_PARAMETER;
}

/** Get the total size required for Ghes version 2

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       StartOffset          Offset from the start of HEST where
                                         GHESv2 starts.
  @param [in]       ErrSourceList        Point to GHESv2 list.
  @param [in]       ErrSourceCount       Count of GHESv2.
  @param [out]      TotalSize            Total size of the GHESv2.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofGhesV2 (
  IN            ACPI_HEST_GENERATOR                                   *Generator,
  IN      CONST UINT32                                                StartOffset,
  IN      CONST CM_ARCH_COMMON_ERROR_SOURCE_GENERIC_HW_VERSION2_INFO  *ErrSourceList,
  IN            UINT32                                                ErrSourceCount,
  OUT           UINT32                                                *TotalSize
  )
{
  UINT32               Idx;
  UINT32               ErrSourceOffset;
  UINT32               ErrSourceSize;
  HEST_SOURCE_INDEXER  *FirmwareFirstIndexer;
  HEST_SOURCE_INDEXER  *GhesAssistIndexer;

  ErrSourceOffset = StartOffset;
  ErrSourceSize   = sizeof (EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE);
  *TotalSize      = 0;

  for (Idx = 0; Idx < ErrSourceCount; Idx++) {
    if (ErrSourceList[Idx].Common.RelatedSourceToken != CM_NULL_TOKEN) {
      FirmwareFirstIndexer = FindFirmwareFirstSourceIndexer (
                               Generator,
                               ErrSourceList[Idx].Common.RelatedSourceToken
                               );
      GhesAssistIndexer = FindGhesAssistSourceIndexer (
                            Generator,
                            ErrSourceList[Idx].Common.RelatedSourceToken
                            );
      if ((FirmwareFirstIndexer == NULL) && (GhesAssistIndexer == NULL)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: HEST: Invalid Related source for GHESv2[%d]. Token = %p\n",
          Idx,
          ErrSourceList[Idx].Common.RelatedSourceToken
          ));
        goto ErrorHandler;
      }

      if (FirmwareFirstIndexer != NULL) {
        FirmwareFirstIndexer->GhesFound = TRUE;
      }

      if (GhesAssistIndexer != NULL) {
        GhesAssistIndexer->GhesFound = TRUE;
      }
    }

    ErrSourceOffset += ErrSourceSize;
    *TotalSize      += ErrSourceSize;
  }

  return EFI_SUCCESS;

ErrorHandler:
  *TotalSize = 0;
  return EFI_INVALID_PARAMETER;
}

/** Add banks to related for Machine Check Error source entry in HEST table.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       BankInfoArrayToken   Count of Machine Check Error source.
  @param [in]       StartOffset          Offset from the start of HEST where
                                         bank starts.
  @param [out]      Hest                 HEST Table.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddMachineCheckBankInfo (
  IN            ACPI_HEST_GENERATOR                              *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST     CfgMgrProtocol,
  IN            CM_ARCH_COMMON_OBJ_REF                           BankInfoArrayToken,
  IN      CONST UINT32                                           StartOffset,
  OUT           EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *Hest
  )
{
  EFI_STATUS                                                         Status;
  CM_ARCH_COMMON_OBJ_REF                                             *BankRefList;
  UINT32                                                             BankRefCount;
  UINT32                                                             BankIdx;
  CM_X64_IA32_MACHINE_CHECK_BANK_INFO                                *BankInfo;
  UINT32                                                             BankInfoCount;
  UINT32                                                             BankOffset;
  UINT8                                                              *HestTable;
  EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_BANK_STRUCTURE  Bank;

  BankOffset = StartOffset;
  HestTable  = (UINT8 *)Hest;

  Status = GetEArchCommonObjCmRef (
             CfgMgrProtocol,
             BankInfoArrayToken.ReferenceToken,
             &BankRefList,
             &BankRefCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HEST: Failed to get Bank array for Machine Check Error Source: "
      "Token = %p, Status = %r\n",
      BankInfoArrayToken,
      Status
      ));
    goto ErrorHandler;
  }

  for (BankIdx = 0; BankIdx < BankRefCount; BankIdx++) {
    Status = GetEX64ObjIa32MachineCheckBankInfo (
               CfgMgrProtocol,
               BankRefList[BankIdx].ReferenceToken,
               &BankInfo,
               &BankInfoCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Couldn't find Machine Bank Info: "
        "Token = %p, Status = %r\n",
        BankRefList[BankIdx].ReferenceToken,
        Status
        ));
      goto ErrorHandler;
    }

    Bank.BankNumber                  = (UINT8)BankIdx;
    Bank.ClearStatusOnInitialization = BankInfo->ClearOnInit;
    Bank.StatusDataFormat            = BankInfo->StatusDataFormat;
    Bank.ControlRegisterMsrAddress   = BankInfo->ControlMSRegAddress;
    Bank.ControlInitData             = BankInfo->ControlInitData;
    Bank.StatusRegisterMsrAddress    = BankInfo->StatusMSRegAddress;
    Bank.AddressRegisterMsrAddress   = BankInfo->AddrMSRegAddress;
    Bank.MiscRegisterMsrAddress      = BankInfo->MiscMSRegAddress;

    CopyMem (
      HestTable + BankOffset,
      &Bank,
      sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_BANK_STRUCTURE)
      );

    BankOffset += sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_BANK_STRUCTURE);
  }

  return EFI_SUCCESS;

ErrorHandler:
  return Status;
}

/** Add Machine Check Exception Error source entry into HEST table.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       StartOffset          Offset from the start of HEST where
                                         Machine Check Exception Error source starts.
  @param [in]       ErrSourceList        Point to Machine Check Exception Error source list.
  @param [in]       ErrSourceCount       Count of Machine Check Exception Error source.
  @param [out]      Hest                 HEST Table.

  @retval EFI_SUCCESS                    Machine Check Exception Error source generated successfully.
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddMachineCheckExceptionErrSource (
  IN            ACPI_HEST_GENERATOR                                    *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      CONST UINT32                                                 StartOffset,
  IN      CONST CM_X64_ERROR_SOURCE_IA32_MACHINE_CHECK_EXCEPTION_INFO  *ErrSourceList,
  IN            UINT32                                                 ErrSourceCount,
  OUT           EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER        *Hest
  )
{
  EFI_STATUS                                                        Status;
  UINT32                                                            Idx;
  UINT32                                                            ErrSourceOffset;
  UINT32                                                            ErrSourceSize;
  UINT8                                                             *HestTable;
  HEST_SOURCE_INDEXER                                               *FirmwareFirstIndexer;
  HEST_SOURCE_INDEXER                                               *GhesAssistIndexer;
  UINT16                                                            SourceId;
  UINT8                                                             Flags;
  UINT32                                                            BankCount;
  EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_EXCEPTION_STRUCTURE  Exception;

  Status          = EFI_INVALID_PARAMETER;
  ErrSourceSize   = sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_EXCEPTION_STRUCTURE);
  ErrSourceOffset = StartOffset;
  HestTable       = (UINT8 *)Hest;

  for (Idx = 0; Idx < ErrSourceCount; Idx++) {
    SourceId = mNextSourceId++;
    Flags    = 0;

    if (ErrSourceList[Idx].Common.MachineBankInfoTokenArray.ReferenceToken != CM_NULL_TOKEN) {
      Status = GetNumberofMachineCheckBankInfo (
                 Generator,
                 CfgMgrProtocol,
                 ErrSourceList[Idx].Common.MachineBankInfoTokenArray,
                 &BankCount
                 );
      if (EFI_ERROR (Status)) {
        goto ErrorHandler;
      }
    } else {
      BankCount = 0;
    }

    if (ErrSourceList[Idx].Common.FirmwareFirst) {
      Flags |= EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST;
    }

    if (ErrSourceList[Idx].Common.GhesAssist) {
      Flags |= EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GHES_ASSIST;
    }

    Exception.Type                         = EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_EXCEPTION;
    Exception.SourceId                     = SourceId;
    Exception.Flags                        = Flags;
    Exception.Enabled                      = ErrSourceList[Idx].Common.Enabled;
    Exception.NumberOfRecordsToPreAllocate = ErrSourceList[Idx].Common.NumberOfRecordsToPreAllocate;
    Exception.MaxSectionsPerRecord         = ErrSourceList[Idx].Common.MaxSectionsPerRecord;
    Exception.GlobalCapabilityInitData     = ErrSourceList[Idx].GlobalCapInitData;
    Exception.GlobalControlInitData        = ErrSourceList[Idx].GlobalControlInitData;
    Exception.NumberOfHardwareBanks        = (UINT8)BankCount;

    CopyMem (HestTable + ErrSourceOffset, &Exception, ErrSourceSize);
    ErrSourceOffset += ErrSourceSize;

    if (BankCount != 0) {
      Status = AddMachineCheckBankInfo (
                 Generator,
                 CfgMgrProtocol,
                 ErrSourceList[Idx].Common.MachineBankInfoTokenArray,
                 ErrSourceOffset,
                 Hest
                 );
      if (EFI_ERROR (Status)) {
        goto ErrorHandler;
      }

      ErrSourceOffset += BankCount * sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_BANK_STRUCTURE);
    }

    if (ErrSourceList[Idx].Common.FirmwareFirst) {
      FirmwareFirstIndexer = FindFirmwareFirstSourceIndexer (
                               Generator,
                               ErrSourceList[Idx].Token
                               );
      if (FirmwareFirstIndexer == NULL) {
        ASSERT (0);
        goto ErrorHandler;
      }

      FirmwareFirstIndexer->SourceId = SourceId;
    }

    if (ErrSourceList[Idx].Common.GhesAssist) {
      GhesAssistIndexer = FindGhesAssistSourceIndexer (
                            Generator,
                            ErrSourceList[Idx].Token
                            );
      if (GhesAssistIndexer == NULL) {
        ASSERT (0);
        goto ErrorHandler;
      }

      GhesAssistIndexer->SourceId = SourceId;
    }

    Hest->ErrorSourceCount++;
  }

  return EFI_SUCCESS;

ErrorHandler:
  return Status;
}

/** Add Corrected Machine Check Error source entry into HEST table.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       StartOffset          Offset from the start of HEST where
                                         Corrected Machine Check Error source starts.
  @param [in]       ErrSourceList        Point to Corrected Machine Check Error source list.
  @param [in]       ErrSourceCount       Count of Corrected Machine Check Error source.
  @param [out]      Hest                 HEST Table.

  @retval EFI_SUCCESS                    Corrected Machine Check Error source generated successfully.
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddCorrectedMachineCheckErrSource (
  IN            ACPI_HEST_GENERATOR                                    *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      CONST UINT32                                                 StartOffset,
  IN      CONST CM_X64_ERROR_SOURCE_IA32_CORRECTED_MACHINE_CHECK_INFO  *ErrSourceList,
  IN            UINT32                                                 ErrSourceCount,
  OUT           EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER        *Hest
  )
{
  EFI_STATUS                                                        Status;
  UINT32                                                            Idx;
  UINT32                                                            ErrSourceOffset;
  UINT32                                                            ErrSourceSize;
  UINT8                                                             *HestTable;
  HEST_SOURCE_INDEXER                                               *FirmwareFirstIndexer;
  HEST_SOURCE_INDEXER                                               *GhesAssistIndexer;
  UINT16                                                            SourceId;
  UINT8                                                             Flags;
  UINT32                                                            BankCount;
  EFI_ACPI_6_6_IA32_ARCHITECTURE_CORRECTED_MACHINE_CHECK_STRUCTURE  Corrected;

  Status          = EFI_INVALID_PARAMETER;
  ErrSourceSize   = sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_CORRECTED_MACHINE_CHECK_STRUCTURE);
  ErrSourceOffset = StartOffset;
  HestTable       = (UINT8 *)Hest;

  for (Idx = 0; Idx < ErrSourceCount; Idx++) {
    SourceId = mNextSourceId++;
    Flags    = 0;

    if (ErrSourceList[Idx].Common.MachineBankInfoTokenArray.ReferenceToken != CM_NULL_TOKEN) {
      Status = GetNumberofMachineCheckBankInfo (
                 Generator,
                 CfgMgrProtocol,
                 ErrSourceList[Idx].Common.MachineBankInfoTokenArray,
                 &BankCount
                 );
      if (EFI_ERROR (Status)) {
        goto ErrorHandler;
      }
    } else {
      BankCount = 0;
    }

    if (ErrSourceList[Idx].Common.FirmwareFirst) {
      Flags |= EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST;
    }

    if (ErrSourceList[Idx].Common.GhesAssist) {
      Flags |= EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GHES_ASSIST;
    }

    Corrected.Type                         = EFI_ACPI_6_6_IA32_ARCHITECTURE_CORRECTED_MACHINE_CHECK;
    Corrected.SourceId                     = SourceId;
    Corrected.Flags                        = Flags;
    Corrected.Enabled                      = ErrSourceList[Idx].Common.Enabled;
    Corrected.NumberOfRecordsToPreAllocate = ErrSourceList[Idx].Common.NumberOfRecordsToPreAllocate;
    Corrected.MaxSectionsPerRecord         = ErrSourceList[Idx].Common.MaxSectionsPerRecord;
    Corrected.NumberOfHardwareBanks        = (UINT8)BankCount;

    CopyMem (
      &Corrected.NotificationStructure,
      &ErrSourceList[Idx].NotificationInfo,
      sizeof (EFI_ACPI_6_6_HARDWARE_ERROR_NOTIFICATION_STRUCTURE)
      );

    CopyMem (HestTable + ErrSourceOffset, &Corrected, ErrSourceSize);
    ErrSourceOffset += ErrSourceSize;

    if (BankCount != 0) {
      Status = AddMachineCheckBankInfo (
                 Generator,
                 CfgMgrProtocol,
                 ErrSourceList[Idx].Common.MachineBankInfoTokenArray,
                 ErrSourceOffset,
                 Hest
                 );
      if (EFI_ERROR (Status)) {
        goto ErrorHandler;
      }

      ErrSourceOffset += BankCount * sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_BANK_STRUCTURE);
    }

    if (ErrSourceList[Idx].Common.FirmwareFirst) {
      FirmwareFirstIndexer = FindFirmwareFirstSourceIndexer (
                               Generator,
                               ErrSourceList[Idx].Token
                               );
      if (FirmwareFirstIndexer == NULL) {
        ASSERT (0);
        goto ErrorHandler;
      }

      FirmwareFirstIndexer->SourceId = SourceId;
    }

    if (ErrSourceList[Idx].Common.GhesAssist) {
      GhesAssistIndexer = FindGhesAssistSourceIndexer (
                            Generator,
                            ErrSourceList[Idx].Token
                            );
      if (GhesAssistIndexer == NULL) {
        ASSERT (0);
        goto ErrorHandler;
      }

      GhesAssistIndexer->SourceId = SourceId;
    }

    Hest->ErrorSourceCount++;
  }

  return EFI_SUCCESS;

ErrorHandler:
  return Status;
}

/** Add Machine Deferred Check Error source entry into HEST table.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       StartOffset          Offset from the start of HEST where
                                         Machine Check Error source starts.
  @param [in]       ErrSourceList        Point to Deferred Machine Check Error source list.
  @param [in]       ErrSourceCount       Count of Deferred Machine Check Error source.
  @param [out]      Hest                 HEST Table.

  @retval EFI_SUCCESS                    Deferred Machine Check Error source generated successfully.
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddDeferredMachineCheckErrSource (
  IN            ACPI_HEST_GENERATOR                                   *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST          CfgMgrProtocol,
  IN      CONST UINT32                                                StartOffset,
  IN      CONST CM_X64_ERROR_SOURCE_IA32_DEFERRED_MACHINE_CHECK_INFO  *ErrSourceList,
  IN            UINT32                                                ErrSourceCount,
  OUT           EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER       *Hest
  )
{
  EFI_STATUS                                                       Status;
  UINT32                                                           Idx;
  UINT32                                                           ErrSourceOffset;
  UINT32                                                           ErrSourceSize;
  UINT8                                                            *HestTable;
  HEST_SOURCE_INDEXER                                              *FirmwareFirstIndexer;
  HEST_SOURCE_INDEXER                                              *GhesAssistIndexer;
  UINT16                                                           SourceId;
  UINT8                                                            Flags;
  UINT32                                                           BankCount;
  EFI_ACPI_6_6_IA32_ARCHITECTURE_DEFERRED_MACHINE_CHECK_STRUCTURE  Deferred;

  Status          = EFI_INVALID_PARAMETER;
  ErrSourceOffset = StartOffset;
  ErrSourceSize   = sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_DEFERRED_MACHINE_CHECK_STRUCTURE);
  HestTable       = (UINT8 *)Hest;

  for (Idx = 0; Idx < ErrSourceCount; Idx++) {
    SourceId = mNextSourceId++;
    Flags    = 0;

    if (ErrSourceList[Idx].Common.MachineBankInfoTokenArray.ReferenceToken != CM_NULL_TOKEN) {
      Status = GetNumberofMachineCheckBankInfo (
                 Generator,
                 CfgMgrProtocol,
                 ErrSourceList[Idx].Common.MachineBankInfoTokenArray,
                 &BankCount
                 );
      if (EFI_ERROR (Status)) {
        goto ErrorHandler;
      }
    } else {
      BankCount = 0;
    }

    if (ErrSourceList[Idx].Common.FirmwareFirst) {
      Flags |= EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST;
    }

    if (ErrSourceList[Idx].Common.GhesAssist) {
      Flags |= EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GHES_ASSIST;
    }

    Deferred.Type                         = EFI_ACPI_6_6_IA32_ARCHITECTURE_DEFERRED_MACHINE_CHECK;
    Deferred.SourceId                     = SourceId;
    Deferred.Flags                        = Flags;
    Deferred.Enabled                      = ErrSourceList[Idx].Common.Enabled;
    Deferred.NumberOfRecordsToPreAllocate = ErrSourceList[Idx].Common.NumberOfRecordsToPreAllocate;
    Deferred.MaxSectionsPerRecord         = ErrSourceList[Idx].Common.MaxSectionsPerRecord;
    Deferred.NumberOfHardwareBanks        = (UINT8)BankCount;

    CopyMem (
      &Deferred.NotificationStructure,
      &ErrSourceList[Idx].NotificationInfo,
      sizeof (EFI_ACPI_6_6_HARDWARE_ERROR_NOTIFICATION_STRUCTURE)
      );

    CopyMem (HestTable + ErrSourceOffset, &Deferred, ErrSourceSize);
    ErrSourceOffset += ErrSourceSize;

    if (BankCount != 0) {
      Status = AddMachineCheckBankInfo (
                 Generator,
                 CfgMgrProtocol,
                 ErrSourceList[Idx].Common.MachineBankInfoTokenArray,
                 ErrSourceOffset,
                 Hest
                 );
      if (EFI_ERROR (Status)) {
        goto ErrorHandler;
      }

      ErrSourceOffset += BankCount * sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_BANK_STRUCTURE);
    }

    if (ErrSourceList[Idx].Common.FirmwareFirst) {
      FirmwareFirstIndexer = FindFirmwareFirstSourceIndexer (
                               Generator,
                               ErrSourceList[Idx].Token
                               );
      if (FirmwareFirstIndexer == NULL) {
        ASSERT (0);
        goto ErrorHandler;
      }

      FirmwareFirstIndexer->SourceId = SourceId;
    }

    if (ErrSourceList[Idx].Common.GhesAssist) {
      GhesAssistIndexer = FindGhesAssistSourceIndexer (
                            Generator,
                            ErrSourceList[Idx].Token
                            );
      if (GhesAssistIndexer == NULL) {
        ASSERT (0);
        goto ErrorHandler;
      }

      GhesAssistIndexer->SourceId = SourceId;
    }

    Hest->ErrorSourceCount++;
  }

  return EFI_SUCCESS;

ErrorHandler:
  return Status;
}

/** Add NMI Error source entry into HEST table.

  @param [in]       StartOffset          Offset from the start of HEST where
                                         Machine Check Error source starts.
  @param [in]       ErrSourceList        Point to NMI Error source list.
  @param [in]       ErrSourceCount       Count of NMI Error source.
  @param [out]      Hest                 HEST Table.

  @retval EFI_SUCCESS                    Machine Check Error source generated successfully.
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddNmiErrSource (
  IN      CONST UINT32                                           StartOffset,
  IN      CONST CM_X64_ERROR_SOURCE_IA32_NMI_INFO                *ErrSourceList,
  IN            UINT32                                           ErrSourceCount,
  OUT           EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *Hest
  )
{
  UINT32                                              Idx;
  UINT32                                              ErrSourceOffset;
  UINT32                                              ErrSourceSize;
  UINT8                                               *HestTable;
  EFI_ACPI_6_6_IA32_ARCHITECTURE_NMI_ERROR_STRUCTURE  Nmi;

  ErrSourceOffset = StartOffset;
  HestTable       = (UINT8 *)Hest;

  if (ErrSourceCount != 1) {
    return EFI_INVALID_PARAMETER;
  }

  for (Idx = 0; Idx < ErrSourceCount; Idx++) {
    ErrSourceSize                    = sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_NMI_ERROR_STRUCTURE);
    Nmi.Type                         = EFI_ACPI_6_6_IA32_ARCHITECTURE_NMI_ERROR;
    Nmi.SourceId                     = mNextSourceId++;
    Nmi.NumberOfRecordsToPreAllocate = ErrSourceList[Idx].NumberOfRecordsToPreAllocate;
    Nmi.MaxSectionsPerRecord         = ErrSourceList[Idx].MaxSectionsPerRecord;
    Nmi.MaxRawDataLength             = ErrSourceList[Idx].MaxSectionsPerRecord;

    CopyMem (HestTable + ErrSourceOffset, &Nmi, ErrSourceSize);

    ErrSourceOffset += ErrSourceSize;
    Hest->ErrorSourceCount++;
  }

  return EFI_SUCCESS;
}

/** Add PCI Root PortError source entry into HEST table.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       StartOffset          Offset from the start of HEST where
                                         PCI Root Port Error source starts.
  @param [in]       ErrSourceList        Point to PCI Root Port Error source list.
  @param [in]       ErrSourceCount       Count of Error source.
  @param [out]      Hest                 HEST Table.

  @retval EFI_SUCCESS                    PCI Root Port Error source generated successfully.
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddPciRootPortErrSource (
  IN            ACPI_HEST_GENERATOR                              *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST     CfgMgrProtocol,
  IN      CONST UINT32                                           StartOffset,
  IN      CONST CM_ARCH_COMMON_ERROR_SOURCE_PCI_ROOT_PORT_INFO   *ErrSourceList,
  IN            UINT32                                           ErrSourceCount,
  OUT           EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *Hest
  )
{
  UINT32                                            Idx;
  UINT32                                            ErrSourceOffset;
  UINT32                                            ErrSourceSize;
  UINT8                                             *HestTable;
  HEST_SOURCE_INDEXER                               *FirmwareFirstIndexer;
  UINT16                                            SourceId;
  UINT8                                             Flags;
  EFI_ACPI_6_6_PCI_EXPRESS_ROOT_PORT_AER_STRUCTURE  RootPort;

  ErrSourceOffset = StartOffset;
  ErrSourceSize   = sizeof (EFI_ACPI_6_6_PCI_EXPRESS_ROOT_PORT_AER_STRUCTURE);
  HestTable       = (UINT8 *)Hest;

  for (Idx = 0; Idx < ErrSourceCount; Idx++) {
    SourceId = mNextSourceId++;
    Flags    = 0;

    if (ErrSourceList[Idx].Common.FirmwareFirst) {
      Flags |= EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST;
    }

    if (ErrSourceList[Idx].Common.Global) {
      Flags |= EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GLOBAL;
    }

    RootPort.Type                                = EFI_ACPI_6_6_PCI_EXPRESS_ROOT_PORT_AER;
    RootPort.SourceId                            = SourceId;
    RootPort.Flags                               = Flags;
    RootPort.Enabled                             = ErrSourceList[Idx].Common.Enabled;
    RootPort.NumberOfRecordsToPreAllocate        = ErrSourceList[Idx].Common.NumberOfRecordsToPreAllocate;
    RootPort.MaxSectionsPerRecord                = ErrSourceList[Idx].Common.MaxSectionsPerRecord;
    RootPort.Bus                                 = ErrSourceList[Idx].Common.Bus;
    RootPort.Device                              = ErrSourceList[Idx].Common.Device;
    RootPort.Function                            = ErrSourceList[Idx].Common.Function;
    RootPort.DeviceControl                       = ErrSourceList[Idx].Common.DeviceControl;
    RootPort.UncorrectableErrorMask              = ErrSourceList[Idx].Common.UncorrectableErrMask;
    RootPort.UncorrectableErrorSeverity          = ErrSourceList[Idx].Common.UncorrectableErrSeverity;
    RootPort.CorrectableErrorMask                = ErrSourceList[Idx].Common.CorrectableErrMask;
    RootPort.AdvancedErrorCapabilitiesAndControl = ErrSourceList[Idx].Common.AdvancedErrCapAndControl;
    RootPort.RootErrorCommand                    = ErrSourceList[Idx].RootErrorCmd;

    CopyMem (HestTable + ErrSourceOffset, &RootPort, ErrSourceSize);
    ErrSourceOffset += ErrSourceSize;

    if (ErrSourceList[Idx].Common.FirmwareFirst) {
      FirmwareFirstIndexer = FindFirmwareFirstSourceIndexer (
                               Generator,
                               ErrSourceList[Idx].Token
                               );
      if (FirmwareFirstIndexer == NULL) {
        ASSERT (0);
        goto ErrorHandler;
      }

      FirmwareFirstIndexer->SourceId = SourceId;
    }

    Hest->ErrorSourceCount++;
  }

  return EFI_SUCCESS;

ErrorHandler:
  return EFI_INVALID_PARAMETER;
}

/** Add PCI Bridge Error source entry into HEST table.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       StartOffset          Offset from the start of HEST where
                                         PCI Bridge Error source starts.
  @param [in]       ErrSourceList        Point to PCI Bridge Error source list.
  @param [in]       ErrSourceCount       Count of Error source.
  @param [out]      Hest                 HEST Table.

  @retval EFI_SUCCESS                    PCI Bridge Error source generated successfully.
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddPciBridgeErrSource (
  IN            ACPI_HEST_GENERATOR                              *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST     CfgMgrProtocol,
  IN      CONST UINT32                                           StartOffset,
  IN      CONST CM_ARCH_COMMON_ERROR_SOURCE_PCI_BRIDGE_INFO      *ErrSourceList,
  IN            UINT32                                           ErrSourceCount,
  OUT           EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *Hest
  )
{
  UINT32                                         Idx;
  UINT32                                         ErrSourceOffset;
  UINT32                                         ErrSourceSize;
  UINT8                                          *HestTable;
  HEST_SOURCE_INDEXER                            *FirmwareFirstIndexer;
  UINT16                                         SourceId;
  UINT8                                          Flags;
  EFI_ACPI_6_6_PCI_EXPRESS_BRIDGE_AER_STRUCTURE  Bridge;

  ErrSourceOffset = StartOffset;
  ErrSourceSize   = sizeof (EFI_ACPI_6_6_PCI_EXPRESS_BRIDGE_AER_STRUCTURE);
  HestTable       = (UINT8 *)Hest;

  for (Idx = 0; Idx < ErrSourceCount; Idx++) {
    SourceId = mNextSourceId++;
    Flags    = 0;

    if (ErrSourceList[Idx].Common.FirmwareFirst) {
      Flags |= EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST;
    }

    if (ErrSourceList[Idx].Common.Global) {
      Flags |= EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GLOBAL;
    }

    Bridge.Type                                         = EFI_ACPI_6_6_PCI_EXPRESS_BRIDGE_AER;
    Bridge.SourceId                                     = SourceId;
    Bridge.Flags                                        = Flags;
    Bridge.Enabled                                      = ErrSourceList[Idx].Common.Enabled;
    Bridge.NumberOfRecordsToPreAllocate                 = ErrSourceList[Idx].Common.NumberOfRecordsToPreAllocate;
    Bridge.MaxSectionsPerRecord                         = ErrSourceList[Idx].Common.MaxSectionsPerRecord;
    Bridge.Bus                                          = ErrSourceList[Idx].Common.Bus;
    Bridge.Device                                       = ErrSourceList[Idx].Common.Device;
    Bridge.Function                                     = ErrSourceList[Idx].Common.Function;
    Bridge.DeviceControl                                = ErrSourceList[Idx].Common.DeviceControl;
    Bridge.UncorrectableErrorMask                       = ErrSourceList[Idx].Common.UncorrectableErrMask;
    Bridge.UncorrectableErrorSeverity                   = ErrSourceList[Idx].Common.UncorrectableErrSeverity;
    Bridge.CorrectableErrorMask                         = ErrSourceList[Idx].Common.CorrectableErrMask;
    Bridge.AdvancedErrorCapabilitiesAndControl          = ErrSourceList[Idx].Common.AdvancedErrCapAndControl;
    Bridge.SecondaryUncorrectableErrorMask              = ErrSourceList[Idx].SecondaryUncorrectableErrMask;
    Bridge.SecondaryUncorrectableErrorSeverity          = ErrSourceList[Idx].SecondaryUncorrectableErrSeverity;
    Bridge.SecondaryAdvancedErrorCapabilitiesAndControl = ErrSourceList[Idx].SecondaryAdvancedCapAndControl;

    CopyMem (HestTable + ErrSourceOffset, &Bridge, ErrSourceSize);
    ErrSourceOffset += ErrSourceSize;

    if (ErrSourceList[Idx].Common.FirmwareFirst) {
      FirmwareFirstIndexer = FindFirmwareFirstSourceIndexer (
                               Generator,
                               ErrSourceList[Idx].Token
                               );
      if (FirmwareFirstIndexer == NULL) {
        ASSERT (0);
        goto ErrorHandler;
      }

      FirmwareFirstIndexer->SourceId = SourceId;
    }

    Hest->ErrorSourceCount++;
  }

  return EFI_SUCCESS;

ErrorHandler:
  return EFI_INVALID_PARAMETER;
}

/** Add PCI Device Error source entry into HEST table.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       StartOffset          Offset from the start of HEST where
                                         PCI Device Error source starts.
  @param [in]       ErrSourceList        Point to PCI Device Error source list.
  @param [in]       ErrSourceCount       Count of Error source.
  @param [out]      Hest                 HEST Table.

  @retval EFI_SUCCESS                    PCI Device Error source generated successfully.
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddPciDeviceErrSource (
  IN            ACPI_HEST_GENERATOR                              *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST     CfgMgrProtocol,
  IN      CONST UINT32                                           StartOffset,
  IN      CONST CM_ARCH_COMMON_ERROR_SOURCE_PCI_DEVICE_INFO      *ErrSourceList,
  IN            UINT32                                           ErrSourceCount,
  OUT           EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *Hest
  )
{
  UINT32                                         Idx;
  UINT32                                         ErrSourceOffset;
  UINT32                                         ErrSourceSize;
  UINT8                                          *HestTable;
  HEST_SOURCE_INDEXER                            *FirmwareFirstIndexer;
  UINT16                                         SourceId;
  UINT8                                          Flags;
  EFI_ACPI_6_6_PCI_EXPRESS_DEVICE_AER_STRUCTURE  Device;

  ErrSourceOffset = StartOffset;
  ErrSourceSize   = sizeof (EFI_ACPI_6_6_PCI_EXPRESS_DEVICE_AER_STRUCTURE);
  HestTable       = (UINT8 *)Hest;

  for (Idx = 0; Idx < ErrSourceCount; Idx++) {
    SourceId = mNextSourceId++;
    Flags    = 0;

    if (ErrSourceList[Idx].Common.FirmwareFirst) {
      Flags |= EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST;
    }

    if (ErrSourceList[Idx].Common.Global) {
      Flags |= EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GLOBAL;
    }

    Device.Type                                = EFI_ACPI_6_6_PCI_EXPRESS_BRIDGE_AER;
    Device.SourceId                            = SourceId;
    Device.Flags                               = Flags;
    Device.Enabled                             = ErrSourceList[Idx].Common.Enabled;
    Device.NumberOfRecordsToPreAllocate        = ErrSourceList[Idx].Common.NumberOfRecordsToPreAllocate;
    Device.MaxSectionsPerRecord                = ErrSourceList[Idx].Common.MaxSectionsPerRecord;
    Device.Bus                                 = ErrSourceList[Idx].Common.Bus;
    Device.Device                              = ErrSourceList[Idx].Common.Device;
    Device.Function                            = ErrSourceList[Idx].Common.Function;
    Device.DeviceControl                       = ErrSourceList[Idx].Common.DeviceControl;
    Device.UncorrectableErrorMask              = ErrSourceList[Idx].Common.UncorrectableErrMask;
    Device.UncorrectableErrorSeverity          = ErrSourceList[Idx].Common.UncorrectableErrSeverity;
    Device.CorrectableErrorMask                = ErrSourceList[Idx].Common.CorrectableErrMask;
    Device.AdvancedErrorCapabilitiesAndControl = ErrSourceList[Idx].Common.AdvancedErrCapAndControl;

    CopyMem (HestTable + ErrSourceOffset, &Device, ErrSourceSize);
    ErrSourceOffset += ErrSourceSize;

    if (ErrSourceList[Idx].Common.FirmwareFirst) {
      FirmwareFirstIndexer = FindFirmwareFirstSourceIndexer (
                               Generator,
                               ErrSourceList[Idx].Token
                               );
      if (FirmwareFirstIndexer == NULL) {
        ASSERT (0);
        goto ErrorHandler;
      }

      FirmwareFirstIndexer->SourceId = SourceId;
    }

    Hest->ErrorSourceCount++;
  }

  return EFI_SUCCESS;

ErrorHandler:
  return EFI_INVALID_PARAMETER;
}

/** Add GHES entry into HEST table.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       StartOffset          Offset from the start of HEST where
                                         Machine Check Error source starts.
  @param [in]       ErrSourceList        Point to GHES list.
  @param [in]       ErrSourceCount       Count of GHES.
  @param [out]      Hest                 HEST Table.

  @retval EFI_SUCCESS                    GHES generated successfully.
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddGhes (
  IN            ACPI_HEST_GENERATOR                              *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST     CfgMgrProtocol,
  IN      CONST UINT32                                           StartOffset,
  IN      CONST CM_ARCH_COMMON_ERROR_SOURCE_GENERIC_HW_INFO      *ErrSourceList,
  IN            UINT32                                           ErrSourceCount,
  OUT           EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *Hest
  )
{
  UINT32                                                Idx;
  UINT32                                                ErrSourceOffset;
  UINT32                                                ErrSourceSize;
  UINT8                                                 *HestTable;
  HEST_SOURCE_INDEXER                                   *FirmwareFirstIndexer;
  HEST_SOURCE_INDEXER                                   *GhesAssistIndexer;
  UINT16                                                SourceId;
  UINT16                                                RelatedSourceId;
  EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_SOURCE_STRUCTURE  Ghes;

  ErrSourceOffset = StartOffset;
  ErrSourceSize   = sizeof (EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_SOURCE_STRUCTURE);
  HestTable       = (UINT8 *)Hest;

  for (Idx = 0; Idx < ErrSourceCount; Idx++) {
    SourceId = mNextSourceId++;

    if (ErrSourceList[Idx].Common.RelatedSourceToken == CM_NULL_TOKEN) {
      RelatedSourceId = 0xFFFF;
    } else {
      FirmwareFirstIndexer = FindFirmwareFirstSourceIndexer (
                               Generator,
                               ErrSourceList[Idx].Common.RelatedSourceToken
                               );
      GhesAssistIndexer = FindGhesAssistSourceIndexer (
                            Generator,
                            ErrSourceList[Idx].Common.RelatedSourceToken
                            );
      if ((FirmwareFirstIndexer == NULL) && (GhesAssistIndexer == NULL)) {
        ASSERT (0);
        goto ErrorHandler;
      }

      if (FirmwareFirstIndexer != NULL) {
        RelatedSourceId = FirmwareFirstIndexer->SourceId;
      } else {
        RelatedSourceId = GhesAssistIndexer->SourceId;
      }
    }

    Ghes.Type                         = EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR;
    Ghes.SourceId                     = SourceId;
    Ghes.RelatedSourceId              = RelatedSourceId;
    Ghes.Enabled                      = ErrSourceList[Idx].Common.Enabled;
    Ghes.NumberOfRecordsToPreAllocate = ErrSourceList[Idx].Common.NumberOfRecordsToPreAllocate;
    Ghes.MaxSectionsPerRecord         = ErrSourceList[Idx].Common.MaxSectionsPerRecord;
    Ghes.MaxRawDataLength             = ErrSourceList[Idx].Common.MaxRawDataLength;
    Ghes.ErrorStatusBlockLength       = ErrSourceList[Idx].Common.ErrorStatusBlockLength;

    CopyMem (
      &Ghes.ErrorStatusAddress,
      &ErrSourceList[Idx].Common.ErrorStatusAddress,
      sizeof (EFI_ACPI_6_6_GENERIC_ADDRESS_STRUCTURE)
      );

    CopyMem (
      &Ghes.NotificationStructure,
      &ErrSourceList[Idx].Common.NotificationStructure,
      sizeof (EFI_ACPI_6_6_HARDWARE_ERROR_NOTIFICATION_STRUCTURE)
      );

    CopyMem (HestTable + ErrSourceOffset, &Ghes, ErrSourceSize);
    ErrSourceOffset += ErrSourceSize;

    Hest->ErrorSourceCount++;
  }

  return EFI_SUCCESS;

ErrorHandler:
  return EFI_INVALID_PARAMETER;
}

/** Add GHESv2 entry into HEST table.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       StartOffset          Offset from the start of HEST where
                                         Machine Check Error source starts.
  @param [in]       ErrSourceList        Point to GHESv2 list.
  @param [in]       ErrSourceCount       Count of GHESv2.
  @param [out]      Hest                 HEST Table.

  @retval EFI_SUCCESS                    GHESv2 generated successfully.
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddGhesV2 (
  IN            ACPI_HEST_GENERATOR                                   *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST          CfgMgrProtocol,
  IN      CONST UINT32                                                StartOffset,
  IN      CONST CM_ARCH_COMMON_ERROR_SOURCE_GENERIC_HW_VERSION2_INFO  *ErrSourceList,
  IN            UINT32                                                ErrSourceCount,
  OUT           EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER       *Hest
  )
{
  UINT32                                                          Idx;
  UINT32                                                          ErrSourceOffset;
  UINT32                                                          ErrSourceSize;
  UINT8                                                           *HestTable;
  HEST_SOURCE_INDEXER                                             *FirmwareFirstIndexer;
  HEST_SOURCE_INDEXER                                             *GhesAssistIndexer;
  UINT16                                                          SourceId;
  UINT16                                                          RelatedSourceId;
  EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE  GhesVer2;

  ErrSourceOffset = StartOffset;
  ErrSourceSize   = sizeof (EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE);
  HestTable       = (UINT8 *)Hest;

  for (Idx = 0; Idx < ErrSourceCount; Idx++) {
    SourceId = mNextSourceId++;

    if (ErrSourceList[Idx].Common.RelatedSourceToken == CM_NULL_TOKEN) {
      RelatedSourceId = 0xFFFF;
    } else {
      FirmwareFirstIndexer = FindFirmwareFirstSourceIndexer (
                               Generator,
                               ErrSourceList[Idx].Common.RelatedSourceToken
                               );
      GhesAssistIndexer = FindGhesAssistSourceIndexer (
                            Generator,
                            ErrSourceList[Idx].Common.RelatedSourceToken
                            );
      if ((FirmwareFirstIndexer == NULL) && (GhesAssistIndexer == NULL)) {
        ASSERT (0);
        goto ErrorHandler;
      }

      if (FirmwareFirstIndexer != NULL) {
        RelatedSourceId = FirmwareFirstIndexer->SourceId;
      } else {
        RelatedSourceId = GhesAssistIndexer->SourceId;
      }
    }

    GhesVer2.Type                         = EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_VERSION_2;
    GhesVer2.SourceId                     = SourceId;
    GhesVer2.RelatedSourceId              = RelatedSourceId;
    GhesVer2.Enabled                      = ErrSourceList[Idx].Common.Enabled;
    GhesVer2.NumberOfRecordsToPreAllocate = ErrSourceList[Idx].Common.NumberOfRecordsToPreAllocate;
    GhesVer2.MaxSectionsPerRecord         = ErrSourceList[Idx].Common.MaxSectionsPerRecord;
    GhesVer2.MaxRawDataLength             = ErrSourceList[Idx].Common.MaxRawDataLength;
    GhesVer2.ErrorStatusBlockLength       = ErrSourceList[Idx].Common.ErrorStatusBlockLength;
    GhesVer2.ReadAckPreserve              = ErrSourceList[Idx].ReadAckPreserve;
    GhesVer2.ReadAckWrite                 = ErrSourceList[Idx].ReadAckWrite;

    CopyMem (
      &GhesVer2.ErrorStatusAddress,
      &ErrSourceList[Idx].Common.ErrorStatusAddress,
      sizeof (EFI_ACPI_6_6_GENERIC_ADDRESS_STRUCTURE)
      );

    CopyMem (
      &GhesVer2.NotificationStructure,
      &ErrSourceList[Idx].Common.NotificationStructure,
      sizeof (EFI_ACPI_6_6_HARDWARE_ERROR_NOTIFICATION_STRUCTURE)
      );

    CopyMem (
      &GhesVer2.ReadAckRegister,
      &ErrSourceList[Idx].ReadAckRegister,
      sizeof (EFI_ACPI_6_6_GENERIC_ADDRESS_STRUCTURE)
      );

    CopyMem (HestTable + ErrSourceOffset, &GhesVer2, ErrSourceSize);
    ErrSourceOffset += ErrSourceSize;

    Hest->ErrorSourceCount++;
  }

  return EFI_SUCCESS;

ErrorHandler:
  return EFI_INVALID_PARAMETER;
}

/** Construct the HEST ACPI table.

  Called by the Dynamic Table Manager, this function invokes the
  Configuration Manager protocol interface to get the required hardware
  information for generating the ACPI table.

  If this function allocates any resources then they must be freed
  in the FreeTpm2TableResources function.

  @param [in]  This           Pointer to the table generator.
  @param [in]  AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                              Protocol Interface.
  @param [out] Table          Pointer to a list of generated ACPI Table.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object was not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
STATIC
EFI_STATUS
EFIAPI
BuildHestTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS                                             Status;
  UINT32                                                 Idx;
  UINT32                                                 TableSize;
  UINT32                                                 ErrSourceSize;
  ACPI_HEST_GENERATOR                                    *Generator;
  EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER        *Hest;
  CM_X64_ERROR_SOURCE_IA32_MACHINE_CHECK_EXCEPTION_INFO  *MachineCheckExceptionErrSourceList;
  UINT32                                                 MachineCheckExceptionErrSourceCount;
  UINT32                                                 MachineCheckExceptionErrSourceOffset;
  CM_X64_ERROR_SOURCE_IA32_CORRECTED_MACHINE_CHECK_INFO  *CorrectedMachineCheckErrSourceList;
  UINT32                                                 CorrectedMachineCheckErrSourceCount;
  UINT32                                                 CorrectedMachineCheckErrSourceOffset;
  CM_X64_ERROR_SOURCE_IA32_DEFERRED_MACHINE_CHECK_INFO   *DeferredMachineCheckErrSourceList;
  UINT32                                                 DeferredMachineCheckErrSourceCount;
  UINT32                                                 DeferredMachineCheckErrSourceOffset;
  UINT32                                                 MachineCheckErrSourceCount;
  CM_X64_ERROR_SOURCE_IA32_NMI_INFO                      *NmiErrSourceList;
  UINT32                                                 NmiErrSourceCount;
  UINT32                                                 NmiErrSourceOffset;
  CM_ARCH_COMMON_ERROR_SOURCE_PCI_ROOT_PORT_INFO         *PciRootPortErrSourceList;
  UINT32                                                 PciRootPortErrSourceCount;
  UINT32                                                 PciRootPortErrSourceOffset;
  CM_ARCH_COMMON_ERROR_SOURCE_PCI_BRIDGE_INFO            *PciBridgeErrSourceList;
  UINT32                                                 PciBridgeErrSourceCount;
  UINT32                                                 PciBridgeErrSourceOffset;
  CM_ARCH_COMMON_ERROR_SOURCE_PCI_DEVICE_INFO            *PciDeviceErrSourceList;
  UINT32                                                 PciDeviceErrSourceCount;
  UINT32                                                 PciDeviceErrSourceOffset;
  UINT32                                                 PciErrSourceCount;
  CM_ARCH_COMMON_ERROR_SOURCE_GENERIC_HW_INFO            *GhesList;
  UINT32                                                 GhesCount;
  UINT32                                                 GhesOffset;
  CM_ARCH_COMMON_ERROR_SOURCE_GENERIC_HW_VERSION2_INFO   *GhesVer2List;
  UINT32                                                 GhesVer2Count;
  UINT32                                                 GhesVer2Offset;

  ASSERT (
    (This != NULL) &&
    (AcpiTableInfo != NULL) &&
    (CfgMgrProtocol != NULL) &&
    (Table != NULL) &&
    (AcpiTableInfo->TableGeneratorId == This->GeneratorID) &&
    (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature)
    );

  if ((AcpiTableInfo->AcpiTableRevision < This->MinAcpiTableRevision) ||
      (AcpiTableInfo->AcpiTableRevision > This->AcpiTableRevision))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HEST: Requested table revision = %d is not supported. "
      "Supported table revisions: Minimum = %d. Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table                               = NULL;
  Generator                            = (ACPI_HEST_GENERATOR *)This;
  MachineCheckExceptionErrSourceOffset = 0;
  CorrectedMachineCheckErrSourceOffset = 0;
  DeferredMachineCheckErrSourceOffset  = 0;
  MachineCheckErrSourceCount           = 0;
  NmiErrSourceOffset                   = 0;
  PciRootPortErrSourceOffset           = 0;
  PciBridgeErrSourceOffset             = 0;
  PciDeviceErrSourceOffset             = 0;
  PciErrSourceCount                    = 0;
  GhesOffset                           = 0;
  GhesVer2Offset                       = 0;

  // Get IA-32 Machine Check Exception Error Source info.
  Status = GetEX64ObjErrSourceIa32MachineCheckExceptionInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &MachineCheckExceptionErrSourceList,
             &MachineCheckExceptionErrSourceCount
             );
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_NOT_FOUND) || (Status == EFI_INVALID_PARAMETER)) {
      MachineCheckExceptionErrSourceList  = NULL;
      MachineCheckExceptionErrSourceCount = 0;
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get Machine Check Exception Error Source Info. Status = %r\n",
        Status
        ));
      return Status;
    }
  }

  MachineCheckErrSourceCount += MachineCheckExceptionErrSourceCount;

  // Get IA-32 Corrected Machine Check Error Source info.
  Status = GetEX64ObjErrSourceIa32CorrectedMachineCheckInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &CorrectedMachineCheckErrSourceList,
             &CorrectedMachineCheckErrSourceCount
             );
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_NOT_FOUND) || (Status == EFI_INVALID_PARAMETER)) {
      CorrectedMachineCheckErrSourceList  = NULL;
      CorrectedMachineCheckErrSourceCount = 0;
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get Corrected Machine Check Error Source Info. Status = %r\n",
        Status
        ));
      return Status;
    }
  }

  MachineCheckErrSourceCount += CorrectedMachineCheckErrSourceCount;

  // Get IA-32 Deferred Machine Check Error Source info.
  Status = GetEX64ObjErrSourceIa32DeferredMachineCheckInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &DeferredMachineCheckErrSourceList,
             &DeferredMachineCheckErrSourceCount
             );
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_NOT_FOUND) || (Status == EFI_INVALID_PARAMETER)) {
      DeferredMachineCheckErrSourceList  = NULL;
      DeferredMachineCheckErrSourceCount = 0;
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get Deferred Machine Check Error Source Info. Status = %r\n",
        Status
        ));
      return Status;
    }
  }

  MachineCheckErrSourceCount += DeferredMachineCheckErrSourceCount;

  // Get IA-32 Nmi Error Source info.
  Status = GetEX64ObjErrSourceIa32NmiInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &NmiErrSourceList,
             &NmiErrSourceCount
             );
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_NOT_FOUND) || (Status == EFI_INVALID_PARAMETER)) {
      NmiErrSourceList  = NULL;
      NmiErrSourceCount = 0;
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get Nmi Error Source Info. Status = %r\n",
        Status
        ));
      return Status;
    }
  }

  if (NmiErrSourceCount > 1) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HEST: Having Nmi Error Source Info more than one\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  // Get Pci Root Port Error Source info.
  Status = GetEArchCommonObjErrSourcePciRootPortInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &PciRootPortErrSourceList,
             &PciRootPortErrSourceCount
             );
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_NOT_FOUND) || (Status == EFI_INVALID_PARAMETER)) {
      PciRootPortErrSourceList  = NULL;
      PciRootPortErrSourceCount = 0;
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get PCI Root Port Error Source Info. Status = %r\n",
        Status
        ));
      return Status;
    }
  }

  PciErrSourceCount += PciRootPortErrSourceCount;

  // Get Pci Bridge Error Source info.
  Status = GetEArchCommonObjErrSourcePciBridgeInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &PciBridgeErrSourceList,
             &PciBridgeErrSourceCount
             );
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_NOT_FOUND) || (Status == EFI_INVALID_PARAMETER)) {
      PciBridgeErrSourceList  = NULL;
      PciBridgeErrSourceCount = 0;
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get PCI Bridge Error Source Info. Status = %r\n",
        Status
        ));
      return Status;
    }
  }

  PciErrSourceCount += PciBridgeErrSourceCount;

  // Get Pci Device Error Source info.
  Status = GetEArchCommonObjErrSourcePciDeviceInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &PciDeviceErrSourceList,
             &PciDeviceErrSourceCount
             );
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_NOT_FOUND) || (Status == EFI_INVALID_PARAMETER)) {
      PciDeviceErrSourceList  = NULL;
      PciDeviceErrSourceCount = 0;
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get PCI Device Error Source Info. Status = %r\n",
        Status
        ));
      return Status;
    }
  }

  PciErrSourceCount += PciDeviceErrSourceCount;

  // Get GHES info.
  Status = GetEArchCommonObjErrSourceGenericHwInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &GhesList,
             &GhesCount
             );
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_NOT_FOUND) || (Status == EFI_INVALID_PARAMETER)) {
      GhesList  = NULL;
      GhesCount = 0;
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get GHES Info. Status = %r\n",
        Status
        ));
      return Status;
    }
  }

  // Get GHES version 2 info.
  Status = GetEArchCommonObjErrSourceGenericHwVer2Info (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &GhesVer2List,
             &GhesVer2Count
             );
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_NOT_FOUND) || (Status == EFI_INVALID_PARAMETER)) {
      GhesVer2List  = NULL;
      GhesVer2Count = 0;
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get GHES Info. Status = %r\n",
        Status
        ));
      return Status;
    }
  }

  if ((MachineCheckErrSourceCount + NmiErrSourceCount + PciErrSourceCount +
       GhesCount + GhesVer2Count) == 0)
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HEST: No Error Source info\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((MachineCheckErrSourceCount + PciErrSourceCount) != 0) {
    Generator->FirmwareFirstIndexer = AllocateZeroPool (
                                        (sizeof (HEST_SOURCE_INDEXER) *
                                         (MachineCheckErrSourceCount +
                                          PciErrSourceCount))
                                        );
    if (Generator->FirmwareFirstIndexer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to allocate for FirmwareFirstIndexer. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }
  }

  if (MachineCheckErrSourceCount != 0) {
    Generator->GhesAssistIndexer = AllocateZeroPool (
                                     (sizeof (HEST_SOURCE_INDEXER) *
                                      MachineCheckErrSourceCount)
                                     );
    if (Generator->GhesAssistIndexer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to allocate for GhesAssistIndexer. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }
  }

  // Calculate the size of the HEST Table.
  TableSize = sizeof (EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER);

  if (MachineCheckExceptionErrSourceCount != 0) {
    MachineCheckExceptionErrSourceOffset = TableSize;
    Status                               = GetSizeofMachineCheckExceptionErrSource (
                                             Generator,
                                             CfgMgrProtocol,
                                             MachineCheckExceptionErrSourceOffset,
                                             MachineCheckExceptionErrSourceList,
                                             MachineCheckExceptionErrSourceCount,
                                             &ErrSourceSize
                                             );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get size of Machine Check Exception Error Source. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }

    TableSize += ErrSourceSize;
  }

  if (CorrectedMachineCheckErrSourceCount != 0) {
    CorrectedMachineCheckErrSourceOffset = TableSize;
    Status                               = GetSizeofCorrectedMachineCheckErrSource (
                                             Generator,
                                             CfgMgrProtocol,
                                             CorrectedMachineCheckErrSourceOffset,
                                             CorrectedMachineCheckErrSourceList,
                                             CorrectedMachineCheckErrSourceCount,
                                             &ErrSourceSize
                                             );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get size of Corrected Machine Check Error Source. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }

    TableSize += ErrSourceSize;
  }

  if (DeferredMachineCheckErrSourceCount != 0) {
    DeferredMachineCheckErrSourceOffset = TableSize;
    Status                              = GetSizeofDeferredMachineCheckErrSource (
                                            Generator,
                                            CfgMgrProtocol,
                                            DeferredMachineCheckErrSourceOffset,
                                            DeferredMachineCheckErrSourceList,
                                            DeferredMachineCheckErrSourceCount,
                                            &ErrSourceSize
                                            );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get size of Deferred Machine Check Error Source. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }

    TableSize += ErrSourceSize;
  }

  if (NmiErrSourceCount != 0) {
    NmiErrSourceOffset = TableSize;
    TableSize         += NmiErrSourceCount * sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_NMI_ERROR_STRUCTURE);
  }

  if (PciRootPortErrSourceCount != 0) {
    PciRootPortErrSourceOffset = TableSize;
    Status                     = GetSizeofPciRootPortErrSource (
                                   Generator,
                                   PciRootPortErrSourceOffset,
                                   PciRootPortErrSourceList,
                                   PciRootPortErrSourceCount,
                                   &ErrSourceSize
                                   );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get size of PCI Root Port Error Source. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }

    TableSize += ErrSourceSize;
  }

  if (PciBridgeErrSourceCount != 0) {
    PciBridgeErrSourceOffset = TableSize;
    Status                   = GetSizeofPciBridgeErrSource (
                                 Generator,
                                 PciBridgeErrSourceOffset,
                                 PciBridgeErrSourceList,
                                 PciBridgeErrSourceCount,
                                 &ErrSourceSize
                                 );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get size of PCI Bridge Error Source. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }

    TableSize += ErrSourceSize;
  }

  if (PciDeviceErrSourceCount != 0) {
    PciDeviceErrSourceOffset = TableSize;
    Status                   = GetSizeofPciDeviceErrSource (
                                 Generator,
                                 PciDeviceErrSourceOffset,
                                 PciDeviceErrSourceList,
                                 PciDeviceErrSourceCount,
                                 &ErrSourceSize
                                 );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get size of PCI Bridge Error Source. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }

    TableSize += ErrSourceSize;
  }

  if (GhesCount != 0) {
    GhesOffset = TableSize;
    Status     = GetSizeofGhes (
                   Generator,
                   GhesOffset,
                   GhesList,
                   GhesCount,
                   &ErrSourceSize
                   );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get size of GHES. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }

    TableSize += ErrSourceSize;
  }

  if (GhesVer2Count != 0) {
    GhesVer2Offset = TableSize;
    Status         = GetSizeofGhesV2 (
                       Generator,
                       GhesVer2Offset,
                       GhesVer2List,
                       GhesVer2Count,
                       &ErrSourceSize
                       );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get size of GHES version 2. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }

    TableSize += ErrSourceSize;
  }

  // Validate related sources.
  for (Idx = 0; Idx < Generator->FirmwareFirstSourceCount; Idx++) {
    if (!Generator->FirmwareFirstIndexer[Idx].GhesFound) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: no GHES for firmware first source - Token: %p.\n",
        Generator->FirmwareFirstIndexer[Idx].Token
        ));

      goto ErrorHandler;
    }
  }

  for (Idx = 0; Idx < Generator->GhesAssistSourceCount; Idx++) {
    if (!Generator->GhesAssistIndexer[Idx].GhesFound) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: no GHES for GHES assist source - Token: %p.\n",
        Generator->FirmwareFirstIndexer[Idx].Token
        ));

      goto ErrorHandler;
    }
  }

  *Table = AllocateZeroPool (TableSize);
  if (*Table == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HEST: Failed to allocate memory for HEST Table. Size = %d," \
      " Status = %r\n",
      TableSize,
      Status
      ));
    goto ErrorHandler;
  }

  Hest = (EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER *)*Table;

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Hest->Header,
             AcpiTableInfo,
             (UINT32)TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HEST: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    goto ErrorHandler;
  }

  // Update HEST table.
  if (MachineCheckExceptionErrSourceOffset != 0) {
    Status = AddMachineCheckExceptionErrSource (
               Generator,
               CfgMgrProtocol,
               MachineCheckExceptionErrSourceOffset,
               MachineCheckExceptionErrSourceList,
               MachineCheckExceptionErrSourceCount,
               Hest
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to add Machine Check Exception Error source. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }
  }

  if (CorrectedMachineCheckErrSourceOffset != 0) {
    Status = AddCorrectedMachineCheckErrSource (
               Generator,
               CfgMgrProtocol,
               CorrectedMachineCheckErrSourceOffset,
               CorrectedMachineCheckErrSourceList,
               CorrectedMachineCheckErrSourceCount,
               Hest
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to add Corrected Machine Check Error source. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }
  }

  if (DeferredMachineCheckErrSourceOffset != 0) {
    Status = AddDeferredMachineCheckErrSource (
               Generator,
               CfgMgrProtocol,
               DeferredMachineCheckErrSourceOffset,
               DeferredMachineCheckErrSourceList,
               DeferredMachineCheckErrSourceCount,
               Hest
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to add Deferred Machine Check Error source. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }
  }

  if (NmiErrSourceOffset != 0) {
    Status = AddNmiErrSource (
               NmiErrSourceOffset,
               NmiErrSourceList,
               NmiErrSourceCount,
               Hest
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to add NMI Error source. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }
  }

  if (PciRootPortErrSourceOffset != 0) {
    Status = AddPciRootPortErrSource (
               Generator,
               CfgMgrProtocol,
               PciRootPortErrSourceOffset,
               PciRootPortErrSourceList,
               PciRootPortErrSourceCount,
               Hest
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to add PCI Root Port Error source. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }
  }

  if (PciBridgeErrSourceOffset != 0) {
    Status = AddPciBridgeErrSource (
               Generator,
               CfgMgrProtocol,
               PciBridgeErrSourceOffset,
               PciBridgeErrSourceList,
               PciBridgeErrSourceCount,
               Hest
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to add PCI Bridge Error source. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }
  }

  if (PciDeviceErrSourceOffset != 0) {
    Status = AddPciDeviceErrSource (
               Generator,
               CfgMgrProtocol,
               PciDeviceErrSourceOffset,
               PciDeviceErrSourceList,
               PciDeviceErrSourceCount,
               Hest
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to add PCI Device Error source. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }
  }

  if (GhesOffset != 0) {
    Status = AddGhes (
               Generator,
               CfgMgrProtocol,
               GhesOffset,
               GhesList,
               GhesCount,
               Hest
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to add GHES. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }
  }

  if (GhesVer2Offset != 0) {
    Status = AddGhesV2 (
               Generator,
               CfgMgrProtocol,
               GhesVer2Offset,
               GhesVer2List,
               GhesVer2Count,
               Hest
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to add GHES version 2. Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }
  }

  return EFI_SUCCESS;

ErrorHandler:
  if (*Table != NULL) {
    FreePool (*Table);
    *Table = NULL;
  }

  if (Generator->FirmwareFirstIndexer) {
    FreePool (Generator->FirmwareFirstIndexer);
    Generator->FirmwareFirstIndexer = NULL;
  }

  if (Generator->GhesAssistIndexer) {
    FreePool (Generator->GhesAssistIndexer);
    Generator->GhesAssistIndexer = NULL;
  }

  return Status;
}

/** Free any resources allocated for constructing the HEST.

  @param [in]      This           Pointer to the table generator.
  @param [in]      AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in, out] Table          Pointer to an array of pointers
                                  to ACPI Table.


  @retval EFI_SUCCESS           The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER The table pointer is NULL or invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeHestTableResources (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  ACPI_HEST_GENERATOR  *Generator;

  ASSERT (
    (This != NULL) &&
    (AcpiTableInfo != NULL) &&
    (CfgMgrProtocol != NULL) &&
    (AcpiTableInfo->TableGeneratorId == This->GeneratorID) &&
    (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature)
    );

  Generator = (ACPI_HEST_GENERATOR *)This;

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: HEST: Invalid Table Pointer\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (Generator->FirmwareFirstIndexer) {
    FreePool (Generator->FirmwareFirstIndexer);
    Generator->FirmwareFirstIndexer = NULL;
  }

  if (Generator->GhesAssistIndexer) {
    FreePool (Generator->GhesAssistIndexer);
    Generator->GhesAssistIndexer = NULL;
  }

  FreePool (*Table);
  *Table = NULL;

  return EFI_SUCCESS;
}

/** The HEST Table Generator revision.
*/
#define HEST_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the TPM2 Table Generator.
*/
STATIC
CONST
ACPI_HEST_GENERATOR  HestGenerator = {
  {
    // Generator ID
    CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdHest),
    // Generator Description
    L"ACPI.STD.HEST.GENERATOR",
    // ACPI Table Signature
    EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_SIGNATURE,
    // ACPI Table Revision supported by this Generator
    EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_REVISION,
    // Minimum supported ACPI Table Revision
    EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_REVISION,
    // Creator ID
    TABLE_GENERATOR_CREATOR_ID,
    // Creator Revision
    HEST_GENERATOR_REVISION,
    // Build Table function
    BuildHestTable,
    // Free Resource function
    FreeHestTableResources,
    // Extended build function not needed
    NULL,
    // Extended build function not implemented by the generator.
    // Hence extended free resource function is not required.
    NULL
  },
  NULL,
  0,
  NULL,
  0,
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
AcpiHestLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&HestGenerator.Header);
  DEBUG ((DEBUG_INFO, "HEST: Register Generator. Status = %r\n", Status));
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
AcpiHestLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&HestGenerator.Header);
  DEBUG ((DEBUG_INFO, "HEST: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
