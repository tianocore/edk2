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
    - EX64ObjIa32MachineCheckBankInfo
    - EX64ObjErrSourceIa32MachineCheckExceptionInfo
    - EX64ObjErrSourceIa32CorrectedMachineCheckInfo
    - EX64ObjErrSourceIa32DeferredMachineCheckInfo
    - EX64ObjErrSourceIa32NmiInfo
    - EArchCommonObjErrSourcePciRootPortInfo
    - EArchCommonObjErrSourcePciBridgeInfo
    - EArchCommonObjErrSourcePciDeviceInfo
    - EArchCommonObjErrSourceGenericHwInfo
    - EArchCommonObjErrSourceGenericHwVer2Info
*/

/**
  This macro expands to a function that retrieves the Error source infomation
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjCmRef,
  CM_ARCH_COMMON_OBJ_REF
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
  CM_ARCH_COMMON_ERROR_SOURCE_GENERIC_HW_VERSION_2_INFO
  );

/** Find source indexer by Token in specified type.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       Token                Error source token.

  @retval NULL                           Not found.
  @retval Others                         Firmware first source indexer
                                         associated with Token.
**/
STATIC
HEST_SOURCE_INDEXER *
EFIAPI
FindSourceIndexer (
  IN  ACPI_HEST_GENERATOR  *Generator,
  IN  CM_OBJECT_TOKEN      Token
  )
{
  UINT32                    Idx;
  HEST_SOURCE_INDEXER_INFO  *IndexerInfo;

  IndexerInfo = &Generator->SourceIndexerInfos;

  for (Idx = 0; Idx < IndexerInfo->SourceIndexerCount; Idx++) {
    if (IndexerInfo->SourceIndexer[Idx].Token == Token) {
      return &IndexerInfo->SourceIndexer[Idx];
    }
  }

  return NULL;
}

/** Validate common field of error source.

  @param [in]       ErrSourceOps      Error Source Ops.
  @param [in]       ErrSourceCommon   Common information for error source.

  @retval EFI_SUCCESS                 Valid.
  @retval EFI_INVALID_PARAMETER       Invalid.
**/
STATIC
EFI_STATUS
EFIAPI
ValidateErrSourceCommonInfo (
  IN  CONST ERR_SOURCE_OPS           *ErrSourceOps,
  IN CONST ERROR_SOURCE_COMMON_INFO  *ErrSourceCommon
  )
{
  UINT8    Flags;
  BOOLEAN  FirmwareFirst;
  BOOLEAN  GhesAssist;
  BOOLEAN  Global;

  if ((ErrSourceCommon->Enabled > 1) ||
      (ErrSourceCommon->NumberOfRecordsToPreAllocate == 0) ||
      (ErrSourceCommon->MaxSectionsPerRecord == 0))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HEST: %s Error source has invalid information.\n",
      ErrSourceOps->ErrSourceName
      ));

    return EFI_INVALID_PARAMETER;
  }

  Flags = ErrSourceCommon->Flags;

  if ((((ErrSourceOps->Flags & EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST) == 0) &&
       ((Flags & EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST) != 0)) ||
      (((ErrSourceOps->Flags & EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GHES_ASSIST) == 0) &&
       ((Flags & EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GHES_ASSIST) != 0)) ||
      (((ErrSourceOps->Flags & EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GLOBAL) == 0) &&
       ((Flags & EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GLOBAL) != 0)))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HEST: %s Error source type has invalid flags set.\n",
      ErrSourceOps->ErrSourceName
      ));
    return EFI_INVALID_PARAMETER;
  }

  FirmwareFirst = ((Flags & EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST) != 0);
  GhesAssist    = ((Flags & EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GHES_ASSIST) != 0);

  if ((ErrSourceOps->ErrSourceType == EFI_ACPI_6_6_IA32_ARCHITECTURE_NMI_ERROR) ||
      ((Flags & EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GLOBAL) != 0))
  {
    Global = TRUE;
  } else {
    Global = FALSE;
  }

  if (FirmwareFirst && GhesAssist) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HEST: %s Error Source has both FirmwareFirst and GhesAssist.\n",
      ErrSourceOps->ErrSourceName
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (Global && (ErrSourceOps->ErrSourceCmObjCount > 1)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HEST: Cannot have more than one Error Source Info (%d) type: %d \n",
      ErrSourceOps->ErrSourceCmObjCount,
      ErrSourceOps->ErrSourceType
      ));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Set Machine Check error source common field.

  @param [in]       ErrSourceType        Error source Type.
  @param [in]       SourceId             Error source id.
  @param [in]       Flags                Error source flags.
  @param [in]       MachineCheckCommon   Common information for Machine Check error source.
  @param [out]      ErrSource            Error source entry.

**/
STATIC
VOID
EFIAPI
SetMachineCheckCommonErrSourceInfo (
  IN UINT16                                        ErrSourceType,
  IN UINT16                                        SourceId,
  IN UINT8                                         Flags,
  IN CONST MACHINE_CHECK_ERROR_SOURCE_COMMON_INFO  *MachineCheckCommon,
  OUT VOID                                         *ErrSource
  )
{
  EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_EXCEPTION_STRUCTURE  *MachineCheckErrSource;

  MachineCheckErrSource = ErrSource;

  MachineCheckErrSource->Type                         = ErrSourceType;
  MachineCheckErrSource->SourceId                     = SourceId;
  MachineCheckErrSource->Flags                        = Flags;
  MachineCheckErrSource->Enabled                      = MachineCheckCommon->Common.Enabled;
  MachineCheckErrSource->NumberOfRecordsToPreAllocate = MachineCheckCommon->Common.NumberOfRecordsToPreAllocate;
  MachineCheckErrSource->MaxSectionsPerRecord         = MachineCheckCommon->Common.MaxSectionsPerRecord;
}

/** Set PCI error source common field.

  @param [in]       ErrSourceType        Error source Type.
  @param [in]       SourceId             Error source id.
  @param [in]       Flags                Error source flags.
  @param [in]       PciCommon            Common information for PCI error source.
  @param [out]      ErrSource            Error source entry.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
SetPciCommonErrSourceInfo (
  IN UINT16                              ErrSourceType,
  IN UINT16                              SourceId,
  IN UINT8                               Flags,
  IN CONST PCI_ERROR_SOURCE_COMMON_INFO  *PciCommon,
  OUT VOID                               *ErrSource
  )
{
  EFI_ACPI_6_6_PCI_EXPRESS_DEVICE_AER_STRUCTURE  *PciErrSource;

  PciErrSource = ErrSource;

  PciErrSource->Type                         = ErrSourceType;
  PciErrSource->SourceId                     = SourceId;
  PciErrSource->Flags                        = Flags;
  PciErrSource->Enabled                      = PciCommon->Common.Enabled;
  PciErrSource->NumberOfRecordsToPreAllocate = PciCommon->Common.NumberOfRecordsToPreAllocate;
  PciErrSource->MaxSectionsPerRecord         = PciCommon->Common.MaxSectionsPerRecord;

  if ((Flags & EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GLOBAL) == 0) {
    PciErrSource->Bus      = PciCommon->Bus;
    PciErrSource->Device   = PciCommon->Device;
    PciErrSource->Function = PciCommon->Function;
  } else if ((PciCommon->Bus != 0) ||
             (PciCommon->Device != 0) ||
             (PciCommon->Function != 0))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HEST: Should not set PCI bdf when GLOBAL Flag is set.\n"
      ));
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  PciErrSource->DeviceControl                       = PciCommon->DeviceControl;
  PciErrSource->UncorrectableErrorMask              = PciCommon->UncorrectableErrMask;
  PciErrSource->UncorrectableErrorSeverity          = PciCommon->UncorrectableErrSeverity;
  PciErrSource->CorrectableErrorMask                = PciCommon->CorrectableErrMask;
  PciErrSource->AdvancedErrorCapabilitiesAndControl = PciCommon->AdvancedErrCapAndControl;

  return EFI_SUCCESS;
}

/** Set Notification structure.

  @param [in]   SourceNotif   Notification structure to use as input.
  @param [out]  TargetNotif   Notification structure to set.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
SetNotificationStructure (
  IN  CONST EFI_ACPI_6_6_HARDWARE_ERROR_NOTIFICATION_STRUCTURE  *SourceNotif,
  OUT       EFI_ACPI_6_6_HARDWARE_ERROR_NOTIFICATION_STRUCTURE  *TargetNotif
  )
{
  if ((SourceNotif->Type > EFI_ACPI_6_6_HARDWARE_ERROR_NOTIFICATION_SOFTWARE_DELEGATED_EXCEPTION)  ||
      (SourceNotif->Length != sizeof (EFI_ACPI_6_6_HARDWARE_ERROR_NOTIFICATION_STRUCTURE))         ||
      (SourceNotif->ConfigurationWriteEnable.Reserved != 0))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HEST: Invalid Notification Structure.\n"
      ));
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (
    TargetNotif,
    SourceNotif,
    sizeof (EFI_ACPI_6_6_HARDWARE_ERROR_NOTIFICATION_STRUCTURE)
    );

  return EFI_SUCCESS;
}

/** Set GHES common field.

  @param [in]       ErrSourceType        Error source Type.
  @param [in]       SourceId             Error source id.
  @param [in]       RelatedSourceId      Relevant Source id with GHES.
  @param [in]       GhesCommon           Common information for GHES.
  @param [out]      ErrSource            Error source entry.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
SetGhesCommonInfo (
  IN UINT16                  ErrSourceType,
  IN UINT16                  SourceId,
  UINT16                     RelatedSourceId,
  IN CONST GHES_COMMON_INFO  *GhesCommon,
  OUT VOID                   *ErrSource
  )
{
  EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_SOURCE_STRUCTURE  *Ghes;

  Ghes = ErrSource;

  Ghes->Type                         = ErrSourceType;
  Ghes->SourceId                     = SourceId;
  Ghes->RelatedSourceId              = RelatedSourceId;
  Ghes->Enabled                      = GhesCommon->Common.Enabled;
  Ghes->NumberOfRecordsToPreAllocate = GhesCommon->Common.NumberOfRecordsToPreAllocate;
  Ghes->MaxSectionsPerRecord         = GhesCommon->Common.MaxSectionsPerRecord;
  Ghes->MaxRawDataLength             = GhesCommon->MaxRawDataLength;
  Ghes->ErrorStatusBlockLength       = GhesCommon->ErrorStatusBlockLength;

  CopyMem (
    &Ghes->ErrorStatusAddress,
    &GhesCommon->ErrorStatusAddress,
    sizeof (EFI_ACPI_6_6_GENERIC_ADDRESS_STRUCTURE)
    );

  return SetNotificationStructure (&GhesCommon->NotificationStructure, &Ghes->NotificationStructure);
}

/** Get Machine Check Error source ops.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [out]      ErrSourceOps         Error Source Operation.

  @retval EFI_SUCCESS                    GHES generated successfully.
  @retval Others                         Failed to initialise
**/
STATIC
EFI_STATUS
EFIAPI
GetMachineCheckErrSourceCmObj (
  IN            ACPI_HEST_GENERATOR                           *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT     ERR_SOURCE_OPS                                      *ErrSourceOps
  )
{
  EFI_STATUS  Status;

  switch (ErrSourceOps->ErrSourceType) {
    case EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_EXCEPTION:
      // Get IA-32 Machine Check Exception Error Source info.
      Status = GetEX64ObjErrSourceIa32MachineCheckExceptionInfo (
                 CfgMgrProtocol,
                 CM_NULL_TOKEN,
                 (CM_X64_ERROR_SOURCE_IA32_MACHINE_CHECK_EXCEPTION_INFO **)
                 &ErrSourceOps->ErrSourceCmObjList,
                 &ErrSourceOps->ErrSourceCmObjCount
                 );
      break;
    case EFI_ACPI_6_6_IA32_ARCHITECTURE_CORRECTED_MACHINE_CHECK:
      // Get IA-32 Corrected Machine Check Error Source info.
      Status = GetEX64ObjErrSourceIa32CorrectedMachineCheckInfo (
                 CfgMgrProtocol,
                 CM_NULL_TOKEN,
                 (CM_X64_ERROR_SOURCE_IA32_CORRECTED_MACHINE_CHECK_INFO **)
                 &ErrSourceOps->ErrSourceCmObjList,
                 &ErrSourceOps->ErrSourceCmObjCount
                 );
      break;
    case EFI_ACPI_6_6_IA32_ARCHITECTURE_DEFERRED_MACHINE_CHECK:
      // Get IA-32 Deferred Machine Check Error Source info.
      Status = GetEX64ObjErrSourceIa32DeferredMachineCheckInfo (
                 CfgMgrProtocol,
                 CM_NULL_TOKEN,
                 (CM_X64_ERROR_SOURCE_IA32_DEFERRED_MACHINE_CHECK_INFO **)
                 &ErrSourceOps->ErrSourceCmObjList,
                 &ErrSourceOps->ErrSourceCmObjCount
                 );
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  return Status;
}

/** Get NMI Error source ops.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [out]      ErrSourceOps         Error Source Operation.

  @retval EFI_SUCCESS                    GHES generated successfully.
  @retval Others                         Failed to initialise
**/
STATIC
EFI_STATUS
EFIAPI
GetNmiErrSourceCmObj (
  IN            ACPI_HEST_GENERATOR                           *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT     ERR_SOURCE_OPS                                      *ErrSourceOps
  )
{
  // Get IA-32 Nmi Error Source info.
  return GetEX64ObjErrSourceIa32NmiInfo (
           CfgMgrProtocol,
           CM_NULL_TOKEN,
           (CM_X64_ERROR_SOURCE_IA32_NMI_INFO **)
           &ErrSourceOps->ErrSourceCmObjList,
           &ErrSourceOps->ErrSourceCmObjCount
           );
}

/** Get PCI Error source CM objects.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [out]      ErrSourceOps         Error Source Operation.

  @retval EFI_SUCCESS                    GHES generated successfully.
  @retval Others                         Failed to initialise
**/
STATIC
EFI_STATUS
EFIAPI
GetPciErrSourceCmObj (
  IN            ACPI_HEST_GENERATOR                           *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT     ERR_SOURCE_OPS                                      *ErrSourceOps
  )
{
  EFI_STATUS  Status;

  switch (ErrSourceOps->ErrSourceType) {
    case EFI_ACPI_6_6_PCI_EXPRESS_ROOT_PORT_AER:
      // Get Pci Root Port Error Source info.
      Status = GetEArchCommonObjErrSourcePciRootPortInfo (
                 CfgMgrProtocol,
                 CM_NULL_TOKEN,
                 (CM_ARCH_COMMON_ERROR_SOURCE_PCI_ROOT_PORT_INFO **)
                 &ErrSourceOps->ErrSourceCmObjList,
                 &ErrSourceOps->ErrSourceCmObjCount
                 );
      break;
    case EFI_ACPI_6_6_PCI_EXPRESS_BRIDGE_AER:
      // Get Pci Bridge Error Source info.
      Status = GetEArchCommonObjErrSourcePciBridgeInfo (
                 CfgMgrProtocol,
                 CM_NULL_TOKEN,
                 (CM_ARCH_COMMON_ERROR_SOURCE_PCI_BRIDGE_INFO **)
                 &ErrSourceOps->ErrSourceCmObjList,
                 &ErrSourceOps->ErrSourceCmObjCount
                 );
      break;
    case EFI_ACPI_6_6_PCI_EXPRESS_DEVICE_AER:
      // Get Pci Device Error Source info.
      Status = GetEArchCommonObjErrSourcePciDeviceInfo (
                 CfgMgrProtocol,
                 CM_NULL_TOKEN,
                 (CM_ARCH_COMMON_ERROR_SOURCE_PCI_DEVICE_INFO **)
                 &ErrSourceOps->ErrSourceCmObjList,
                 &ErrSourceOps->ErrSourceCmObjCount
                 );
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  return Status;
}

/** Get GHES CmObj.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [out]      ErrSourceOps         Error Source Operation.

  @retval EFI_SUCCESS                    GHES generated successfully.
  @retval Others                         Failed to initialise
**/
STATIC
EFI_STATUS
EFIAPI
GetGhesCmObj (
  IN            ACPI_HEST_GENERATOR                           *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT           ERR_SOURCE_OPS                                *ErrSourceOps
  )
{
  EFI_STATUS  Status;

  switch (ErrSourceOps->ErrSourceType) {
    case EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR:
      // Get GHES info.
      Status = GetEArchCommonObjErrSourceGenericHwInfo (
                 CfgMgrProtocol,
                 CM_NULL_TOKEN,
                 (CM_ARCH_COMMON_ERROR_SOURCE_GENERIC_HW_INFO **)
                 &ErrSourceOps->ErrSourceCmObjList,
                 &ErrSourceOps->ErrSourceCmObjCount
                 );
      break;
    case EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_VERSION_2:
      // Get GHES version 2 info.
      Status = GetEArchCommonObjErrSourceGenericHwVer2Info (
                 CfgMgrProtocol,
                 CM_NULL_TOKEN,
                 (CM_ARCH_COMMON_ERROR_SOURCE_GENERIC_HW_VERSION_2_INFO **)
                 &ErrSourceOps->ErrSourceCmObjList,
                 &ErrSourceOps->ErrSourceCmObjCount
                 );
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  return Status;
}

/** Get the total size required for Error source entry and Update the Indexers.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       ErrSourceOps         Error Source Operation.
  @param [out]      TotalSize            Total size of the Check Exception Error source.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSizeofErrSource (
  IN            ACPI_HEST_GENERATOR                           *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST ERR_SOURCE_OPS                                *ErrSourceOps,
  OUT           UINT32                                        *TotalSize
  )
{
  EFI_STATUS                      Status;
  UINT32                          Idx;
  UINT8                           *ErrSourceList;
  UINT32                          ErrSourceOffset;
  UINT32                          ErrSourceSize;
  CONST ERROR_SOURCE_COMMON_INFO  *ErrSourceCommonInfo;
  UINT32                          Step;
  UINT8                           Flags;
  UINT32                          AdditionalSize;
  HEST_SOURCE_INDEXER_INFO        *IndexerInfo;
  HEST_SOURCE_INDEXER             *Indexer;
  UINT32                          IndexerIdx;

  Status      = EFI_INVALID_PARAMETER;
  *TotalSize  = 0;
  IndexerInfo = &Generator->SourceIndexerInfos;
  IndexerIdx  = IndexerInfo->SourceIndexerCount;
  Indexer     = &IndexerInfo->SourceIndexer[IndexerIdx];

  ErrSourceList   = ErrSourceOps->ErrSourceCmObjList;
  Step            = ErrSourceOps->ErrSourceCmObjSize;
  ErrSourceOffset = ErrSourceOps->ErrSourceOffset;

  for (Idx = 0; Idx < ErrSourceOps->ErrSourceCmObjCount; Idx++) {
    ErrSourceCommonInfo = (CONST ERROR_SOURCE_COMMON_INFO *)(ErrSourceList + (Idx * Step));
    ErrSourceSize       = ErrSourceOps->ErrSourceSize;
    AdditionalSize      = 0;
    Flags               = ErrSourceCommonInfo->Flags;

    Status = ValidateErrSourceCommonInfo (ErrSourceOps, ErrSourceCommonInfo);
    if (EFI_ERROR (Status)) {
      goto ErrorHandler;
    }

    if (ErrSourceOps->GetAdditionalSizeofErrSource != NULL) {
      Status = ErrSourceOps->GetAdditionalSizeofErrSource (
                               Generator,
                               CfgMgrProtocol,
                               ErrSourceCommonInfo,
                               &AdditionalSize
                               );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: HEST: Failed to get additional size of %s Error Source[%d].\n",
          ErrSourceOps->ErrSourceName,
          Idx
          ));
        goto ErrorHandler;
      }
    }

    if ((Flags & (EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST | EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GHES_ASSIST)) != 0) {
      Indexer->Token  = ErrSourceCommonInfo->Token;
      Indexer->Object = (VOID *)ErrSourceCommonInfo;
      Indexer->Offset = ErrSourceOffset;
      Indexer++;
      IndexerInfo->SourceIndexerCount++;
    }

    ErrSourceSize   += AdditionalSize;
    ErrSourceOffset += ErrSourceSize;
    *TotalSize      += ErrSourceSize;
  }

  return EFI_SUCCESS;

ErrorHandler:
  *TotalSize = 0;
  return Status;
}

/** Get the total size required for Machine Check Error source entry.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       ErrSourceCmObj       Error Source CM Object.
  @param [out]      Size                 Additional size for the Machine Check Error source.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetAdditionalSizeofMachineCheckErrSource (
  IN            ACPI_HEST_GENERATOR                           *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST ERROR_SOURCE_COMMON_INFO                      *ErrSourceCmObj,
  OUT           UINT32                                        *Size
  )
{
  EFI_STATUS                                    Status;
  CM_ARCH_COMMON_OBJ_REF                        *BankRefList;
  UINT32                                        BankCount;
  CONST MACHINE_CHECK_ERROR_SOURCE_COMMON_INFO  *MachineCheckCommon;

  BankCount = 0;

  MachineCheckCommon = (CONST MACHINE_CHECK_ERROR_SOURCE_COMMON_INFO *)ErrSourceCmObj;

  if (MachineCheckCommon->MachineBankInfoTokenArray.ReferenceToken != CM_NULL_TOKEN) {
    Status = GetEArchCommonObjCmRef (
               CfgMgrProtocol,
               MachineCheckCommon->MachineBankInfoTokenArray.ReferenceToken,
               &BankRefList,
               &BankCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get Bank array for Machine Check Error Source: "
        "Token = %p, Status = %r\n",
        MachineCheckCommon->MachineBankInfoTokenArray.ReferenceToken,
        Status
        ));
      return Status;
    }

    if (BankCount > MAX_UINT8) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: BankCount(%d) couldn't be over MAX_UINT8.\n",
        BankCount
        ));
      return EFI_INVALID_PARAMETER;
    }
  }

  *Size = BankCount * sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_BANK_STRUCTURE);

  return EFI_SUCCESS;
}

/** Get the Additional size for GHES(vX) and additional validation check.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       ErrSourceCmObj       Error Source CM Object.
  @param [out]      Size                 Additional size for GHES.


  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetAdditionalSizeofGhes (
  IN            ACPI_HEST_GENERATOR                           *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST ERROR_SOURCE_COMMON_INFO                      *ErrSourceCmObj,
  OUT           UINT32                                        *Size
  )
{
  CONST GHES_COMMON_INFO  *GhesCommon;
  HEST_SOURCE_INDEXER     *Indexer;

  *Size = 0;

  GhesCommon = (CONST GHES_COMMON_INFO *)ErrSourceCmObj;

  if (GhesCommon->RelatedSourceToken != CM_NULL_TOKEN) {
    Indexer = FindSourceIndexer (
                Generator,
                GhesCommon->RelatedSourceToken
                );
    if (Indexer == NULL) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Invalid Related source for GHES. Token = %p\n",
        GhesCommon->RelatedSourceToken
        ));
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/** Add Error source entry into HEST table.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       ErrSourceOps         Error Source Operation.
  @param [out]      Hest                 HEST Table.

  @retval EFI_SUCCESS                    Machine Check Exception Error source generated successfully.
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddErrSource (
  IN            ACPI_HEST_GENERATOR                              *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST     CfgMgrProtocol,
  IN            ERR_SOURCE_OPS                                   *ErrSourceOps,
  OUT           EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *Hest
  )
{
  EFI_STATUS                      Status;
  UINT32                          Idx;
  UINT8                           *ErrSourceList;
  UINT32                          ErrSourceSize;
  UINT32                          AdditionalSize;
  UINT32                          Step;
  HEST_SOURCE_INDEXER             *Indexer;
  CONST ERROR_SOURCE_COMMON_INFO  *ErrSourceCommonInfo;
  UINT16                          SourceId;
  UINT8                           Flags;

  Status        = EFI_INVALID_PARAMETER;
  ErrSourceList = ErrSourceOps->ErrSourceCmObjList;
  Step          = ErrSourceOps->ErrSourceCmObjSize;

  for (Idx = 0; Idx < ErrSourceOps->ErrSourceCmObjCount; Idx++) {
    ErrSourceCommonInfo = (ERROR_SOURCE_COMMON_INFO *)(ErrSourceList + (Idx * Step));
    SourceId            = Generator->NextSourceId++;
    Flags               = ErrSourceCommonInfo->Flags;

    Status = ErrSourceOps->AddErrSource (
                             Generator,
                             CfgMgrProtocol,
                             SourceId,
                             ErrSourceOps,
                             ErrSourceCommonInfo,
                             Hest
                             );
    if (EFI_ERROR (Status)) {
      goto ErrorHandler;
    }

    if ((Flags & (EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST | EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GHES_ASSIST)) != 0) {
      Indexer = FindSourceIndexer (
                  Generator,
                  ErrSourceCommonInfo->Token
                  );
      if (Indexer == NULL) {
        ASSERT (0);
        goto ErrorHandler;
      }

      Indexer->SourceId = SourceId;
    }

    ErrSourceSize = ErrSourceOps->ErrSourceSize;
    if (ErrSourceOps->GetAdditionalSizeofErrSource != NULL) {
      Status = ErrSourceOps->GetAdditionalSizeofErrSource (
                               Generator,
                               CfgMgrProtocol,
                               ErrSourceCommonInfo,
                               &AdditionalSize
                               );
      if (EFI_ERROR (Status)) {
        goto ErrorHandler;
      }

      ErrSourceSize += AdditionalSize;
    }

    ErrSourceOps->ErrSourceOffset += ErrSourceSize;
    Hest->ErrorSourceCount++;
  }

  return EFI_SUCCESS;

ErrorHandler:
  return Status;
}

/** Add banks to related for Machine Check Error source entry in HEST table.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       BankRefList          List of Bank reference token.
  @param [in]       BankRefCount         Count of Bank reference token.
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
  IN      CONST CM_ARCH_COMMON_OBJ_REF                           *BankRefList,
  IN      CONST UINT32                                           BankRefCount,
  IN      CONST UINT32                                           StartOffset,
  OUT           EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *Hest
  )
{
  EFI_STATUS                                                         Status;
  UINT32                                                             BankIdx;
  CM_X64_IA32_MACHINE_CHECK_BANK_INFO                                *BankInfo;
  UINT32                                                             BankInfoCount;
  UINT32                                                             BankOffset;
  UINT8                                                              *HestTable;
  EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_BANK_STRUCTURE  *Bank;

  BankOffset = StartOffset;
  HestTable  = (UINT8 *)Hest;

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

    Bank = (EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_BANK_STRUCTURE *)(HestTable + BankOffset);

    Bank->BankNumber                  = (UINT8)BankIdx;
    Bank->ClearStatusOnInitialization = BankInfo->ClearOnInit;
    Bank->StatusDataFormat            = BankInfo->StatusDataFormat;
    Bank->ControlRegisterMsrAddress   = BankInfo->ControlMSRegAddress;
    Bank->ControlInitData             = BankInfo->ControlInitData;
    Bank->StatusRegisterMsrAddress    = BankInfo->StatusMSRegAddress;
    Bank->AddressRegisterMsrAddress   = BankInfo->AddrMSRegAddress;
    Bank->MiscRegisterMsrAddress      = BankInfo->MiscMSRegAddress;

    BankOffset += sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_BANK_STRUCTURE);
  }

  return EFI_SUCCESS;

ErrorHandler:
  return Status;
}

/** Add Machine Check Error source entry into HEST table.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       SourceId             Error Source Id.
  @param [in]       ErrSourceOps         Error Source Operation.
  @param [in]       ErrSourceCmObj       Error Source CM Object.
  @param [out]      Hest                 HEST Table.

  @retval EFI_SUCCESS                    Machine Check Exception Error source generated successfully.
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddMachineCheckErrSource (
  IN            ACPI_HEST_GENERATOR                              *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST     CfgMgrProtocol,
  IN            UINT16                                           SourceId,
  IN      CONST ERR_SOURCE_OPS                                   *ErrSourceOps,
  IN      CONST ERROR_SOURCE_COMMON_INFO                         *ErrSourceCmObj,
  OUT           EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *Hest
  )
{
  EFI_STATUS                                                        Status;
  UINT32                                                            ErrSourceOffset;
  UINT8                                                             *HestTable;
  UINT8                                                             Flags;
  CM_ARCH_COMMON_OBJ_REF                                            *BankRefList;
  UINT32                                                            BankCount;
  CONST MACHINE_CHECK_ERROR_SOURCE_COMMON_INFO                      *MachineCheckCommon;
  CONST CM_X64_ERROR_SOURCE_IA32_MACHINE_CHECK_EXCEPTION_INFO       *ExceptionInfo;
  CONST CM_X64_ERROR_SOURCE_IA32_CORRECTED_MACHINE_CHECK_INFO       *CorrectedInfo;
  CONST CM_X64_ERROR_SOURCE_IA32_DEFERRED_MACHINE_CHECK_INFO        *DeferredInfo;
  EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_EXCEPTION_STRUCTURE  *Exception;
  EFI_ACPI_6_6_IA32_ARCHITECTURE_CORRECTED_MACHINE_CHECK_STRUCTURE  *Corrected;
  EFI_ACPI_6_6_IA32_ARCHITECTURE_DEFERRED_MACHINE_CHECK_STRUCTURE   *Deferred;
  VOID                                                              *ErrSource;

  Status             = EFI_INVALID_PARAMETER;
  HestTable          = (UINT8 *)Hest;
  MachineCheckCommon = (CONST MACHINE_CHECK_ERROR_SOURCE_COMMON_INFO *)ErrSourceCmObj;
  Flags              = MachineCheckCommon->Common.Flags;
  Flags             &= (EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST |
                        EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GHES_ASSIST);
  ErrSourceOffset = ErrSourceOps->ErrSourceOffset;
  ErrSource       = (VOID *)(HestTable + ErrSourceOffset);

  if (MachineCheckCommon->MachineBankInfoTokenArray.ReferenceToken != CM_NULL_TOKEN) {
    Status = GetEArchCommonObjCmRef (
               CfgMgrProtocol,
               MachineCheckCommon->MachineBankInfoTokenArray.ReferenceToken,
               &BankRefList,
               &BankCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get Bank array for Machine Check Error Source: "
        "Token = %p, Status = %r\n",
        MachineCheckCommon->MachineBankInfoTokenArray.ReferenceToken,
        Status
        ));
      return Status;
    }

    if (BankCount > MAX_UINT8) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: BankCount(%d) couldn't be over MAX_UINT8.\n",
        BankCount
        ));
      return EFI_INVALID_PARAMETER;
    }
  } else {
    BankCount = 0;
  }

  SetMachineCheckCommonErrSourceInfo (
    ErrSourceOps->ErrSourceType,
    SourceId,
    Flags,
    MachineCheckCommon,
    ErrSource
    );

  switch (ErrSourceOps->ErrSourceType) {
    case EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_EXCEPTION:
      ExceptionInfo                       = (CONST CM_X64_ERROR_SOURCE_IA32_MACHINE_CHECK_EXCEPTION_INFO *)MachineCheckCommon;
      Exception                           = (EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_EXCEPTION_STRUCTURE *)ErrSource;
      Exception->GlobalCapabilityInitData = ExceptionInfo->GlobalCapInitData;
      Exception->GlobalControlInitData    = ExceptionInfo->GlobalControlInitData;
      Exception->NumberOfHardwareBanks    = (UINT8)BankCount;
      break;
    case EFI_ACPI_6_6_IA32_ARCHITECTURE_CORRECTED_MACHINE_CHECK:
      CorrectedInfo                    = (CONST CM_X64_ERROR_SOURCE_IA32_CORRECTED_MACHINE_CHECK_INFO *)MachineCheckCommon;
      Corrected                        = (EFI_ACPI_6_6_IA32_ARCHITECTURE_CORRECTED_MACHINE_CHECK_STRUCTURE *)ErrSource;
      Corrected->NumberOfHardwareBanks = (UINT8)BankCount;

      Status = SetNotificationStructure (&CorrectedInfo->NotificationInfo, &Corrected->NotificationStructure);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      break;
    case EFI_ACPI_6_6_IA32_ARCHITECTURE_DEFERRED_MACHINE_CHECK:
      DeferredInfo                    =  (CONST CM_X64_ERROR_SOURCE_IA32_DEFERRED_MACHINE_CHECK_INFO *)MachineCheckCommon;
      Deferred                        =  (EFI_ACPI_6_6_IA32_ARCHITECTURE_DEFERRED_MACHINE_CHECK_STRUCTURE *)ErrSource;
      Deferred->NumberOfHardwareBanks = (UINT8)BankCount;

      Status = SetNotificationStructure (&DeferredInfo->NotificationInfo, &Deferred->NotificationStructure);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  ErrSourceOffset += ErrSourceOps->ErrSourceSize;

  if (BankCount != 0) {
    Status = AddMachineCheckBankInfo (
               Generator,
               CfgMgrProtocol,
               BankRefList,
               BankCount,
               ErrSourceOffset,
               Hest
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/** Add NMI Error source entry into HEST table.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       SourceId             Error Source Id.
  @param [in]       ErrSourceOps         Error Source Operation.
  @param [in]       ErrSourceCmObj       Error Source CM Object.
  @param [out]      Hest                 HEST Table.

  @retval EFI_SUCCESS                    Machine Check Error source generated successfully.
**/
STATIC
EFI_STATUS
EFIAPI
AddNmiErrSource (
  IN            ACPI_HEST_GENERATOR                              *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST     CfgMgrProtocol,
  IN            UINT16                                           SourceId,
  IN      CONST ERR_SOURCE_OPS                                   *ErrSourceOps,
  IN      CONST ERROR_SOURCE_COMMON_INFO                         *ErrSourceCmObj,
  OUT           EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *Hest
  )
{
  UINT8                                               *HestTable;
  CONST CM_X64_ERROR_SOURCE_IA32_NMI_INFO             *NmiInfo;
  EFI_ACPI_6_6_IA32_ARCHITECTURE_NMI_ERROR_STRUCTURE  *Nmi;

  HestTable = (UINT8 *)Hest;
  NmiInfo   = (CONST CM_X64_ERROR_SOURCE_IA32_NMI_INFO *)ErrSourceCmObj;
  Nmi       = (EFI_ACPI_6_6_IA32_ARCHITECTURE_NMI_ERROR_STRUCTURE *)
              (HestTable + ErrSourceOps->ErrSourceOffset);

  Nmi->Type                         = ErrSourceOps->ErrSourceType;
  Nmi->SourceId                     = SourceId;
  Nmi->NumberOfRecordsToPreAllocate = NmiInfo->Common.NumberOfRecordsToPreAllocate;
  Nmi->MaxSectionsPerRecord         = NmiInfo->Common.MaxSectionsPerRecord;
  Nmi->MaxRawDataLength             = NmiInfo->MaxRawDataLength;

  return EFI_SUCCESS;
}

/** Add PCI Error source entry into HEST table.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       SourceId             Error Source Id.
  @param [in]       ErrSourceOps         Error Source Operation.
  @param [in]       ErrSourceCmObj       Error Source CM Object.
  @param [out]      Hest                 HEST Table.

  @retval EFI_SUCCESS                    PCI Root Port Error source generated successfully.
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddPciErrSource (
  IN            ACPI_HEST_GENERATOR                              *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST     CfgMgrProtocol,
  IN            UINT16                                           SourceId,
  IN      CONST ERR_SOURCE_OPS                                   *ErrSourceOps,
  IN      CONST ERROR_SOURCE_COMMON_INFO                         *ErrSourceCmObj,
  OUT           EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *Hest
  )
{
  UINT8                                                 *HestTable;
  EFI_STATUS                                            Status;
  UINT8                                                 Flags;
  CONST PCI_ERROR_SOURCE_COMMON_INFO                    *PciCommon;
  CONST CM_ARCH_COMMON_ERROR_SOURCE_PCI_ROOT_PORT_INFO  *RootPortInfo;
  CONST CM_ARCH_COMMON_ERROR_SOURCE_PCI_BRIDGE_INFO     *BridgeInfo;
  EFI_ACPI_6_6_PCI_EXPRESS_ROOT_PORT_AER_STRUCTURE      *RootPort;
  EFI_ACPI_6_6_PCI_EXPRESS_BRIDGE_AER_STRUCTURE         *Bridge;
  VOID                                                  *ErrSource;

  HestTable = (UINT8 *)Hest;
  ErrSource = (VOID *)(HestTable + ErrSourceOps->ErrSourceOffset);

  PciCommon = (CONST PCI_ERROR_SOURCE_COMMON_INFO *)ErrSourceCmObj;
  Flags     = PciCommon->Common.Flags;
  Flags    &= (EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST |
               EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GLOBAL);

  Status = SetPciCommonErrSourceInfo (
             ErrSourceOps->ErrSourceType,
             SourceId,
             Flags,
             PciCommon,
             ErrSource
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  switch (ErrSourceOps->ErrSourceType) {
    case EFI_ACPI_6_6_PCI_EXPRESS_ROOT_PORT_AER:
      RootPortInfo               = (CONST CM_ARCH_COMMON_ERROR_SOURCE_PCI_ROOT_PORT_INFO *)PciCommon;
      RootPort                   = (EFI_ACPI_6_6_PCI_EXPRESS_ROOT_PORT_AER_STRUCTURE *)ErrSource;
      RootPort->RootErrorCommand = RootPortInfo->RootErrorCmd;
      break;
    case EFI_ACPI_6_6_PCI_EXPRESS_BRIDGE_AER:
      BridgeInfo                                           = (CONST CM_ARCH_COMMON_ERROR_SOURCE_PCI_BRIDGE_INFO *)PciCommon;
      Bridge                                               = (EFI_ACPI_6_6_PCI_EXPRESS_BRIDGE_AER_STRUCTURE *)ErrSource;
      Bridge->SecondaryUncorrectableErrorMask              = BridgeInfo->SecondaryUncorrectableErrMask;
      Bridge->SecondaryUncorrectableErrorSeverity          = BridgeInfo->SecondaryUncorrectableErrSeverity;
      Bridge->SecondaryAdvancedErrorCapabilitiesAndControl = BridgeInfo->SecondaryAdvancedCapAndControl;
      break;
    case EFI_ACPI_6_6_PCI_EXPRESS_DEVICE_AER:
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Add GHES(v2) entry into HEST table.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       SourceId             Error Source Id.
  @param [in]       ErrSourceOps         Error Source Operation.
  @param [in]       ErrSourceCmObj       Error Source CM Object.
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
  IN            UINT16                                           SourceId,
  IN      CONST ERR_SOURCE_OPS                                   *ErrSourceOps,
  IN      CONST ERROR_SOURCE_COMMON_INFO                         *ErrSourceCmObj,
  OUT           EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *Hest
  )
{
  EFI_STATUS                                                      Status;
  UINT8                                                           *HestTable;
  HEST_SOURCE_INDEXER                                             *Indexer;
  UINT16                                                          RelatedSourceId;
  CONST GHES_COMMON_INFO                                          *GhesCommon;
  CONST CM_ARCH_COMMON_ERROR_SOURCE_GENERIC_HW_VERSION_2_INFO     *GhesV2Info;
  EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE  *GhesVer2;
  VOID                                                            *ErrSource;

  HestTable  = (UINT8 *)Hest;
  GhesCommon = (CONST GHES_COMMON_INFO *)ErrSourceCmObj;
  ErrSource  = (VOID *)(HestTable + ErrSourceOps->ErrSourceOffset);

  if (GhesCommon->RelatedSourceToken == CM_NULL_TOKEN) {
    RelatedSourceId = 0xFFFF;
  } else {
    Indexer = FindSourceIndexer (
                Generator,
                GhesCommon->RelatedSourceToken
                );
    if (Indexer == NULL) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    RelatedSourceId = Indexer->SourceId;
  }

  Status = SetGhesCommonInfo (
             ErrSourceOps->ErrSourceType,
             SourceId,
             RelatedSourceId,
             GhesCommon,
             ErrSource
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  switch (ErrSourceOps->ErrSourceType) {
    case EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR:
      break;
    case EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_VERSION_2:
      GhesV2Info                = (CONST CM_ARCH_COMMON_ERROR_SOURCE_GENERIC_HW_VERSION_2_INFO *)GhesCommon;
      GhesVer2                  = (EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE *)ErrSource;
      GhesVer2->ReadAckPreserve = GhesV2Info->ReadAckPreserve;
      GhesVer2->ReadAckWrite    = GhesV2Info->ReadAckWrite;

      CopyMem (
        &GhesVer2->ReadAckRegister,
        &GhesV2Info->ReadAckRegister,
        sizeof (EFI_ACPI_6_6_GENERIC_ADDRESS_STRUCTURE)
        );

      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

STATIC ERR_SOURCE_OPS  HestErrSourceOps[] = {
  {
    EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_EXCEPTION,
    L"IA-32 Machine Check Exception",
    sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_MACHINE_CHECK_EXCEPTION_STRUCTURE),
    sizeof (CM_X64_ERROR_SOURCE_IA32_MACHINE_CHECK_EXCEPTION_INFO),
    EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST | EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GHES_ASSIST,
    GetMachineCheckErrSourceCmObj,
    GetAdditionalSizeofMachineCheckErrSource,
    AddMachineCheckErrSource,
    HEST_IA32_SUBTABLE_UNSUPPORTED,
  },
  {
    EFI_ACPI_6_6_IA32_ARCHITECTURE_CORRECTED_MACHINE_CHECK,
    L"IA-32 Corrected Machine Check",
    sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_CORRECTED_MACHINE_CHECK_STRUCTURE),
    sizeof (CM_X64_ERROR_SOURCE_IA32_CORRECTED_MACHINE_CHECK_INFO),
    EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST | EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GHES_ASSIST,
    GetMachineCheckErrSourceCmObj,
    GetAdditionalSizeofMachineCheckErrSource,
    AddMachineCheckErrSource,
    HEST_IA32_SUBTABLE_UNSUPPORTED,
  },
  {
    EFI_ACPI_6_6_IA32_ARCHITECTURE_DEFERRED_MACHINE_CHECK,
    L"IA-32 Deferred Machine Check",
    sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_DEFERRED_MACHINE_CHECK_STRUCTURE),
    sizeof (CM_X64_ERROR_SOURCE_IA32_DEFERRED_MACHINE_CHECK_INFO),
    EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST | EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GHES_ASSIST,
    GetMachineCheckErrSourceCmObj,
    GetAdditionalSizeofMachineCheckErrSource,
    AddMachineCheckErrSource,
    HEST_IA32_SUBTABLE_UNSUPPORTED,
  },
  {
    EFI_ACPI_6_6_IA32_ARCHITECTURE_NMI_ERROR,
    L"IA-32 NMI",
    sizeof (EFI_ACPI_6_6_IA32_ARCHITECTURE_NMI_ERROR_STRUCTURE),
    sizeof (CM_X64_ERROR_SOURCE_IA32_NMI_INFO),
    0,
    GetNmiErrSourceCmObj,
    NULL,
    AddNmiErrSource,
    HEST_IA32_SUBTABLE_UNSUPPORTED,
  },
  {
    EFI_ACPI_6_6_PCI_EXPRESS_ROOT_PORT_AER,
    L"PCI Root Port",
    sizeof (EFI_ACPI_6_6_PCI_EXPRESS_ROOT_PORT_AER_STRUCTURE),
    sizeof (CM_ARCH_COMMON_ERROR_SOURCE_PCI_ROOT_PORT_INFO),
    EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST | EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GLOBAL,
    GetPciErrSourceCmObj,
    NULL,
    AddPciErrSource,
  },
  {
    EFI_ACPI_6_6_PCI_EXPRESS_BRIDGE_AER,
    L"PCI Bridge",
    sizeof (EFI_ACPI_6_6_PCI_EXPRESS_BRIDGE_AER_STRUCTURE),
    sizeof (CM_ARCH_COMMON_ERROR_SOURCE_PCI_BRIDGE_INFO),
    EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST | EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GLOBAL,
    GetPciErrSourceCmObj,
    NULL,
    AddPciErrSource,
  },
  {
    EFI_ACPI_6_6_PCI_EXPRESS_DEVICE_AER,
    L"PCI Device",
    sizeof (EFI_ACPI_6_6_PCI_EXPRESS_DEVICE_AER_STRUCTURE),
    sizeof (CM_ARCH_COMMON_ERROR_SOURCE_PCI_DEVICE_INFO),
    EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST | EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GLOBAL,
    GetPciErrSourceCmObj,
    NULL,
    AddPciErrSource,
  },
  {
    EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR,
    L"GHES",
    sizeof (EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_SOURCE_STRUCTURE),
    sizeof (CM_ARCH_COMMON_ERROR_SOURCE_GENERIC_HW_INFO),
    0,
    GetGhesCmObj,
    GetAdditionalSizeofGhes,
    AddGhes,
  },
  {
    EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_VERSION_2,
    L"GHESv2",
    sizeof (EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE),
    sizeof (CM_ARCH_COMMON_ERROR_SOURCE_GENERIC_HW_VERSION_2_INFO),
    0,
    GetGhesCmObj,
    GetAdditionalSizeofGhes,
    AddGhes,
  },
};

/** Free any resources allocated for constructing the HEST.

  @param [in] Generator      Pointer to the HEST Generator.
  @param [in] Table          Pointer to an array of pointers
                             to ACPI Table.

  @retval EFI_SUCCESS           The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER The table pointer is NULL or invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeResources (
  IN  ACPI_HEST_GENERATOR          *Generator,
  IN  EFI_ACPI_DESCRIPTION_HEADER  **Table
  )
{
  Generator->NextSourceId = 0;

  Generator->SourceIndexerInfos.SourceIndexerCount = 0;
  if (Generator->SourceIndexerInfos.SourceIndexer != NULL) {
    FreePool (Generator->SourceIndexerInfos.SourceIndexer);
    Generator->SourceIndexerInfos.SourceIndexer = NULL;
  }

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: HEST: Invalid Table Pointer\n"));
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;

  return EFI_SUCCESS;
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
  EFI_STATUS                                       Status;
  UINT32                                           Idx;
  UINT32                                           ErrSourceCount;
  UINT32                                           TableSize;
  UINT32                                           ErrSourceSize;
  ACPI_HEST_GENERATOR                              *Generator;
  HEST_SOURCE_INDEXER_INFO                         *IndexerInfo;
  EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *Hest;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

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

  *Table         = NULL;
  Generator      = (ACPI_HEST_GENERATOR *)This;
  IndexerInfo    = &Generator->SourceIndexerInfos;
  ErrSourceSize  = 0;
  ErrSourceCount = 0;

  // Get error source CM objects
  for (Idx = 0; Idx < ARRAY_SIZE (HestErrSourceOps); Idx++) {
    if (HestErrSourceOps[Idx].Unsupported) {
      continue;
    }

    Status = HestErrSourceOps[Idx].GetErrSourceCmObj (
                                     Generator,
                                     CfgMgrProtocol,
                                     &HestErrSourceOps[Idx]
                                     );
    if (EFI_ERROR (Status)) {
      if (Status == EFI_NOT_FOUND) {
        Status                                    = EFI_SUCCESS;
        HestErrSourceOps[Idx].ErrSourceCmObjList  = NULL;
        HestErrSourceOps[Idx].ErrSourceCmObjCount = 0;
      } else {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: HEST: Failed to get %s Error Source CM objects. Status = %r\n",
          HestErrSourceOps[Idx].ErrSourceName,
          Status
          ));
        goto ErrorHandler;
      }
    }

    ErrSourceCount += HestErrSourceOps[Idx].ErrSourceCmObjCount;

    if ((HestErrSourceOps[Idx].Flags &
         (EFI_ACPI_6_6_ERROR_SOURCE_FLAG_FIRMWARE_FIRST |
          EFI_ACPI_6_6_ERROR_SOURCE_FLAG_GHES_ASSIST)) != 0)
    {
      IndexerInfo->SourceIndexerCount += HestErrSourceOps[Idx].ErrSourceCmObjCount;
    }
  }

  if (ErrSourceCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HEST: No Error Source info\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (IndexerInfo->SourceIndexerCount) {
    IndexerInfo->SourceIndexer = AllocateZeroPool (
                                   (sizeof (HEST_SOURCE_INDEXER) *
                                    IndexerInfo->SourceIndexerCount)
                                   );
    if (IndexerInfo->SourceIndexer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to allocate for Indexer[%d]. Status = %r\n",
        Idx,
        Status
        ));
      goto ErrorHandler;
    }

    IndexerInfo->SourceIndexerCount = 0;
  }

  // Calculate the size of the HEST Table.
  TableSize = sizeof (EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER);

  for (Idx = 0; Idx < ARRAY_SIZE (HestErrSourceOps); Idx++) {
    if (HestErrSourceOps[Idx].Unsupported ||
        (HestErrSourceOps[Idx].ErrSourceCmObjCount == 0))
    {
      continue;
    }

    HestErrSourceOps[Idx].ErrSourceOffset = TableSize;

    Status = GetSizeofErrSource (
               Generator,
               CfgMgrProtocol,
               &HestErrSourceOps[Idx],
               &ErrSourceSize
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to get size of %s Error Source. Status = %r\n",
        HestErrSourceOps[Idx].ErrSourceName,
        Status
        ));
      goto ErrorHandler;
    }

    TableSize += ErrSourceSize;
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
  for (Idx = 0; Idx < ARRAY_SIZE (HestErrSourceOps); Idx++) {
    if (HestErrSourceOps[Idx].Unsupported ||
        (HestErrSourceOps[Idx].ErrSourceOffset == 0))
    {
      continue;
    }

    Status = AddErrSource (
               Generator,
               CfgMgrProtocol,
               &HestErrSourceOps[Idx],
               Hest
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: HEST: Failed to add %s Error source. Status = %r\n",
        HestErrSourceOps[Idx].ErrSourceName,
        Status
        ));
      goto ErrorHandler;
    }
  }

  return EFI_SUCCESS;

ErrorHandler:
  FreeResources ((ACPI_HEST_GENERATOR *)This, Table);
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
  ASSERT (
    (This != NULL) &&
    (AcpiTableInfo != NULL) &&
    (CfgMgrProtocol != NULL) &&
    (AcpiTableInfo->TableGeneratorId == This->GeneratorID) &&
    (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature)
    );

  return FreeResources ((ACPI_HEST_GENERATOR *)This, Table);
}

/** The HEST Table Generator revision.
*/
#define HEST_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the TPM2 Table Generator.
*/
STATIC
// CONST TODO: The Indexer is modified, so not CONST
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

  // Next source id.
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
