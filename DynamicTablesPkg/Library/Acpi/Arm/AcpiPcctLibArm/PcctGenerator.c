/** @file
  PCCT Table Generator

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.4 Specification - January 2021
    s14 PLATFORM COMMUNICATIONS CHANNEL (PCC)

**/

#include <Library/AcpiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include "PcctGenerator.h"

/** ARM standard PCCT Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArmObjPccSubspaceType0Info
  - EArmObjPccSubspaceType1Info
  - EArmObjPccSubspaceType2Info
  - EArmObjPccSubspaceType3Info
  - EArmObjPccSubspaceType4Info
  - EArmObjPccSubspaceType5Info
*/

/** This macro expands to a function that retrieves the PCC
    Subspace of Type 0 Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjPccSubspaceType0Info,
  CM_ARM_PCC_SUBSPACE_TYPE0_INFO
  );

/** This macro expands to a function that retrieves the PCC
    Subspace of Type 1 Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjPccSubspaceType1Info,
  CM_ARM_PCC_SUBSPACE_TYPE1_INFO
  );

/** This macro expands to a function that retrieves the PCC
    Subspace of Type 2 Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjPccSubspaceType2Info,
  CM_ARM_PCC_SUBSPACE_TYPE2_INFO
  );

/** This macro expands to a function that retrieves the PCC
    Subspace of Type 3 Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjPccSubspaceType3Info,
  CM_ARM_PCC_SUBSPACE_TYPE3_INFO
  );

/** This macro expands to a function that retrieves the PCC
    Subspace of Type 4 Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjPccSubspaceType4Info,
  CM_ARM_PCC_SUBSPACE_TYPE4_INFO
  );

/** This macro expands to a function that retrieves the PCC
    Subspace of Type 5 Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjPccSubspaceType5Info,
  CM_ARM_PCC_SUBSPACE_TYPE5_INFO
  );

/** The Platform is capable of generating an interrupt
    to indicate completion of a command.

  Cf: s14.1.1 Platform Communications Channel Global Flags
  Platform Interrupt flag
  and s14.1.6 Extended PCC subspaces (types 3 and 4)
    If a responder subspace is included in the PCCT,
    then the global Platform Interrupt flag must be set to 1

  Set this variable and populate the PCCT flag accordingly if either:
   - One of the PCCT Subspace uses interrupts.
   - A PCC Subspace of type 4 is used.
*/
STATIC BOOLEAN  mHasPlatformInterrupt;

/** Initialize the MappingTable.

  @param [in] MappingTable  The mapping table structure.
  @param [in] Count         Number of entries to allocate in the
                            MappingTable.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
MappingTableInitialize (
  IN  MAPPING_TABLE  *MappingTable,
  IN  UINT32         Count
  )
{
  VOID  **Table;

  if ((MappingTable == NULL)  ||
      (Count == 0))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Table = AllocateZeroPool (sizeof (*Table) * Count);
  if (Table == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  MappingTable->Table    = Table;
  MappingTable->MaxIndex = Count;

  return EFI_SUCCESS;
}

/** Free the MappingTable.

  @param [in, out]  MappingTable  The mapping table structure.
**/
STATIC
VOID
EFIAPI
MappingTableFree (
  IN  OUT MAPPING_TABLE  *MappingTable
  )
{
  ASSERT (MappingTable != NULL);
  ASSERT (MappingTable->Table != NULL);

  if (MappingTable->Table != NULL) {
    FreePool (MappingTable->Table);
  }
}

/** Add a new entry for CmArmPccSubspace at Index.

  @param [in] MappingTable      The mapping table structure.
  @param [in] CmArmPccSubspace  Pointer to a CM_ARM_PCC_SUBSPACE_TYPE[X]_INFO.
  @param [in] Index             Index at which CmArmPccSubspace must be added.
                                This is the Subspace Id.

  @retval EFI_SUCCESS            Success.
  @retval EFI_BUFFER_TOO_SMALL   Buffer too small.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
MappingTableAdd (
  IN  MAPPING_TABLE  *MappingTable,
  IN  VOID           *CmArmPccSubspace,
  IN  UINT32         Index
  )
{
  if ((MappingTable == NULL)        ||
      (MappingTable->Table == NULL) ||
      (CmArmPccSubspace == NULL))
  {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  if ((Index >= MappingTable->MaxIndex) ||
      (MappingTable->Table[Index] != 0))
  {
    ASSERT_EFI_ERROR (EFI_BUFFER_TOO_SMALL);
    return EFI_BUFFER_TOO_SMALL;
  }

  // Just map the Pcc Subspace in the Table.
  MappingTable->Table[Index] = CmArmPccSubspace;
  return EFI_SUCCESS;
}

/** Parse the CmPccArray objects and add them to the MappingTable.

  @param [in] MappingTable     The mapping table structure.
  @param [in] CmPccArray       Pointer to an array of CM_ARM_PCC_SUBSPACE_TYPE[X]_INFO.
  @param [in] CmPccCount       Count of objects in CmPccArray.

  @retval EFI_SUCCESS            Success.
  @retval EFI_BUFFER_TOO_SMALL   Buffer too small.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
MapPccSubspaceId (
  IN  MAPPING_TABLE  *MappingTable,
  IN  VOID           *CmPccArray,
  IN  UINT32         CmPccCount
  )
{
  EFI_STATUS                 Status;
  UINT8                      *PccBuffer;
  UINT32                     Index;
  UINT32                     CmObjSize;
  PCC_SUBSPACE_GENERIC_INFO  *GenericPcc;

  if (CmPccCount == 0) {
    return EFI_SUCCESS;
  }

  if ((CmPccArray == NULL) || (MappingTable == NULL)) {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  GenericPcc = (PCC_SUBSPACE_GENERIC_INFO *)CmPccArray;

  switch (GenericPcc->Type) {
    case EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_GENERIC:
      CmObjSize = sizeof (CM_ARM_PCC_SUBSPACE_TYPE0_INFO);
      break;

    case EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_1_HW_REDUCED_COMMUNICATIONS:
      CmObjSize = sizeof (CM_ARM_PCC_SUBSPACE_TYPE1_INFO);
      break;

    case EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_2_HW_REDUCED_COMMUNICATIONS:
      CmObjSize = sizeof (CM_ARM_PCC_SUBSPACE_TYPE2_INFO);
      break;

    case EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_3_EXTENDED_PCC:
      CmObjSize = sizeof (CM_ARM_PCC_SUBSPACE_TYPE3_INFO);
      break;

    case EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_4_EXTENDED_PCC:
      CmObjSize = sizeof (CM_ARM_PCC_SUBSPACE_TYPE4_INFO);
      break;

    case EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_5_HW_REGISTERS_COMMUNICATIONS:
      CmObjSize = sizeof (CM_ARM_PCC_SUBSPACE_TYPE5_INFO);
      break;

    default:
      ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
      return EFI_INVALID_PARAMETER;
  }

  PccBuffer = (UINT8 *)CmPccArray;

  // Map the Pcc channel to their Subspace Id.
  for (Index = 0; Index < CmPccCount; Index++) {
    GenericPcc = (PCC_SUBSPACE_GENERIC_INFO *)PccBuffer;

    Status = MappingTableAdd (
               MappingTable,
               PccBuffer,
               GenericPcc->SubspaceId
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    PccBuffer += CmObjSize;
  }

  return EFI_SUCCESS;
}

/** Add one PCCT Subspace structure of Type 0 (Generic).

  @param [in]  PccCmObj   Pointer to a CmObj PCCT Subspace info structure.
  @param [in]  PccAcpi    Pointer to the ACPI PCCT Subspace structure to populate.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddSubspaceStructType0 (
  IN  CM_ARM_PCC_SUBSPACE_TYPE0_INFO      *PccCmObj,
  IN  EFI_ACPI_6_4_PCCT_SUBSPACE_GENERIC  *PccAcpi
  )
{
  PCC_MAILBOX_REGISTER_INFO         *Doorbell;
  PCC_SUBSPACE_CHANNEL_TIMING_INFO  *ChannelTiming;

  if ((PccCmObj == NULL) ||
      (PccCmObj->Type != EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_GENERIC)  ||
      (PccAcpi == NULL))
  {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  Doorbell      = &PccCmObj->DoorbellReg;
  ChannelTiming = &PccCmObj->ChannelTiming;

  PccAcpi->Type                    = PccCmObj->Type;
  PccAcpi->Length                  = sizeof (EFI_ACPI_6_4_PCCT_SUBSPACE_GENERIC);
  *(UINT32 *)&PccAcpi->Reserved[0] = EFI_ACPI_RESERVED_DWORD;
  *(UINT16 *)&PccAcpi->Reserved[4] = EFI_ACPI_RESERVED_WORD;
  PccAcpi->BaseAddress             = PccCmObj->BaseAddress;
  PccAcpi->AddressLength           = PccCmObj->AddressLength;

  CopyMem (
    &PccAcpi->DoorbellRegister,
    &Doorbell->Register,
    sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE)
    );
  PccAcpi->DoorbellPreserve = Doorbell->PreserveMask;
  PccAcpi->DoorbellWrite    = Doorbell->WriteMask;

  PccAcpi->NominalLatency               = ChannelTiming->NominalLatency;
  PccAcpi->MaximumPeriodicAccessRate    = ChannelTiming->MaxPeriodicAccessRate;
  PccAcpi->MinimumRequestTurnaroundTime = ChannelTiming->MinRequestTurnaroundTime;

  return EFI_SUCCESS;
}

/** Add one PCCT subspace structure of Type 1 (HW-Reduced).

  @param [in]  PccCmObj   Pointer to a CmObj PCCT Subspace info structure.
  @param [in]  PccAcpi    Pointer to the ACPI PCCT Subspace structure to populate.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddSubspaceStructType1 (
  IN  CM_ARM_PCC_SUBSPACE_TYPE1_INFO                          *PccCmObj,
  IN  EFI_ACPI_6_4_PCCT_SUBSPACE_1_HW_REDUCED_COMMUNICATIONS  *PccAcpi
  )
{
  CM_ARM_PCC_SUBSPACE_TYPE0_INFO    *GenericPccCmObj;
  PCC_MAILBOX_REGISTER_INFO         *Doorbell;
  PCC_SUBSPACE_CHANNEL_TIMING_INFO  *ChannelTiming;

  GenericPccCmObj = (CM_ARM_PCC_SUBSPACE_TYPE0_INFO *)PccCmObj;

  if ((PccCmObj == NULL) ||
      (GenericPccCmObj->Type != EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_1_HW_REDUCED_COMMUNICATIONS)  ||
      (PccAcpi == NULL))
  {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  Doorbell      = &GenericPccCmObj->DoorbellReg;
  ChannelTiming = &GenericPccCmObj->ChannelTiming;

  PccAcpi->Type                   = GenericPccCmObj->Type;
  PccAcpi->Length                 = sizeof (EFI_ACPI_6_4_PCCT_SUBSPACE_1_HW_REDUCED_COMMUNICATIONS);
  PccAcpi->PlatformInterrupt      = PccCmObj->PlatIrq.Interrupt;
  PccAcpi->PlatformInterruptFlags = PccCmObj->PlatIrq.Flags;
  PccAcpi->Reserved               = EFI_ACPI_RESERVED_BYTE;
  PccAcpi->BaseAddress            = GenericPccCmObj->BaseAddress;
  PccAcpi->AddressLength          = GenericPccCmObj->AddressLength;

  CopyMem (
    &PccAcpi->DoorbellRegister,
    &Doorbell->Register,
    sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE)
    );
  PccAcpi->DoorbellPreserve = Doorbell->PreserveMask;
  PccAcpi->DoorbellWrite    = Doorbell->WriteMask;

  PccAcpi->NominalLatency               = ChannelTiming->NominalLatency;
  PccAcpi->MaximumPeriodicAccessRate    = ChannelTiming->MaxPeriodicAccessRate;
  PccAcpi->MinimumRequestTurnaroundTime = ChannelTiming->MinRequestTurnaroundTime;

  if ((PccCmObj->PlatIrq.Interrupt != 0)) {
    mHasPlatformInterrupt = TRUE;
  }

  return EFI_SUCCESS;
}

/** Add one PCCT subspace structure of Type 2 (HW-Reduced).

  @param [in]  PccCmObj   Pointer to a CmObj PCCT Subspace info structure.
  @param [in]  PccAcpi    Pointer to the ACPI PCCT Subspace structure to populate.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddSubspaceStructType2 (
  IN  CM_ARM_PCC_SUBSPACE_TYPE2_INFO                          *PccCmObj,
  IN  EFI_ACPI_6_4_PCCT_SUBSPACE_2_HW_REDUCED_COMMUNICATIONS  *PccAcpi
  )
{
  CM_ARM_PCC_SUBSPACE_TYPE0_INFO    *GenericPccCmObj;
  PCC_MAILBOX_REGISTER_INFO         *Doorbell;
  PCC_MAILBOX_REGISTER_INFO         *PlatIrqAck;
  PCC_SUBSPACE_CHANNEL_TIMING_INFO  *ChannelTiming;

  GenericPccCmObj = (CM_ARM_PCC_SUBSPACE_TYPE0_INFO *)PccCmObj;

  if ((PccCmObj == NULL) ||
      (GenericPccCmObj->Type != EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_2_HW_REDUCED_COMMUNICATIONS)  ||
      (PccAcpi == NULL))
  {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  Doorbell      = &GenericPccCmObj->DoorbellReg;
  PlatIrqAck    = &PccCmObj->PlatIrqAckReg;
  ChannelTiming = &GenericPccCmObj->ChannelTiming;

  PccAcpi->Type                   = GenericPccCmObj->Type;
  PccAcpi->Length                 = sizeof (EFI_ACPI_6_4_PCCT_SUBSPACE_2_HW_REDUCED_COMMUNICATIONS);
  PccAcpi->PlatformInterrupt      = PccCmObj->PlatIrq.Interrupt;
  PccAcpi->PlatformInterruptFlags = PccCmObj->PlatIrq.Flags;
  PccAcpi->BaseAddress            = GenericPccCmObj->BaseAddress;
  PccAcpi->Reserved               = EFI_ACPI_RESERVED_BYTE;
  PccAcpi->BaseAddress            = GenericPccCmObj->BaseAddress;
  PccAcpi->AddressLength          = GenericPccCmObj->AddressLength;

  CopyMem (
    &PccAcpi->DoorbellRegister,
    &Doorbell->Register,
    sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE)
    );
  PccAcpi->DoorbellPreserve = Doorbell->PreserveMask;
  PccAcpi->DoorbellWrite    = Doorbell->WriteMask;

  PccAcpi->NominalLatency               = ChannelTiming->NominalLatency;
  PccAcpi->MaximumPeriodicAccessRate    = ChannelTiming->MaxPeriodicAccessRate;
  PccAcpi->MinimumRequestTurnaroundTime = ChannelTiming->MinRequestTurnaroundTime;

  CopyMem (
    &PccAcpi->PlatformInterruptAckRegister,
    &PlatIrqAck->Register,
    sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE)
    );
  PccAcpi->PlatformInterruptAckPreserve = PlatIrqAck->PreserveMask;
  PccAcpi->PlatformInterruptAckWrite    = PlatIrqAck->WriteMask;

  if ((PccCmObj->PlatIrq.Interrupt != 0)) {
    mHasPlatformInterrupt = TRUE;
  }

  return EFI_SUCCESS;
}

/** Add one PCCT subspace structure of Type 3 or 4 (Extended).

  @param [in]  PccCmObj   Pointer to a CmObj PCCT Subspace info structure.
  @param [in]  PccAcpi    Pointer to the ACPI PCCT Subspace structure to populate.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddSubspaceStructType34 (
  IN  CM_ARM_PCC_SUBSPACE_TYPE3_INFO             *PccCmObj,
  IN  EFI_ACPI_6_4_PCCT_SUBSPACE_3_EXTENDED_PCC  *PccAcpi
  )
{
  CM_ARM_PCC_SUBSPACE_TYPE0_INFO    *GenericPccCmObj;
  PCC_MAILBOX_REGISTER_INFO         *Doorbell;
  PCC_MAILBOX_REGISTER_INFO         *PlatIrqAck;
  PCC_MAILBOX_REGISTER_INFO         *CmdCompleteCheck;
  PCC_MAILBOX_REGISTER_INFO         *CmdCompleteUpdate;
  PCC_MAILBOX_REGISTER_INFO         *ErrorStatus;
  PCC_SUBSPACE_CHANNEL_TIMING_INFO  *ChannelTiming;

  GenericPccCmObj = (CM_ARM_PCC_SUBSPACE_TYPE0_INFO *)PccCmObj;

  if ((PccCmObj == NULL) ||
      ((GenericPccCmObj->Type != EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_3_EXTENDED_PCC) &&
       (GenericPccCmObj->Type != EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_4_EXTENDED_PCC)) ||
      (PccAcpi == NULL))
  {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  Doorbell          = &GenericPccCmObj->DoorbellReg;
  PlatIrqAck        = &PccCmObj->PlatIrqAckReg;
  CmdCompleteCheck  = &PccCmObj->CmdCompleteCheckReg;
  CmdCompleteUpdate = &PccCmObj->CmdCompleteUpdateReg;
  ErrorStatus       = &PccCmObj->ErrorStatusReg;
  ChannelTiming     = &GenericPccCmObj->ChannelTiming;

  PccAcpi->Type                   = GenericPccCmObj->Type;
  PccAcpi->Length                 = sizeof (EFI_ACPI_6_4_PCCT_SUBSPACE_3_EXTENDED_PCC);
  PccAcpi->PlatformInterrupt      = PccCmObj->PlatIrq.Interrupt;
  PccAcpi->PlatformInterruptFlags = PccCmObj->PlatIrq.Flags;
  PccAcpi->Reserved               = EFI_ACPI_RESERVED_BYTE;
  PccAcpi->BaseAddress            = GenericPccCmObj->BaseAddress;
  PccAcpi->AddressLength          = GenericPccCmObj->AddressLength;

  CopyMem (
    &PccAcpi->DoorbellRegister,
    &Doorbell->Register,
    sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE)
    );
  PccAcpi->DoorbellPreserve = Doorbell->PreserveMask;
  PccAcpi->DoorbellWrite    = Doorbell->WriteMask;

  PccAcpi->NominalLatency               = ChannelTiming->NominalLatency;
  PccAcpi->MaximumPeriodicAccessRate    = ChannelTiming->MaxPeriodicAccessRate;
  PccAcpi->MinimumRequestTurnaroundTime = ChannelTiming->MinRequestTurnaroundTime;

  CopyMem (
    &PccAcpi->PlatformInterruptAckRegister,
    &PlatIrqAck->Register,
    sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE)
    );
  PccAcpi->PlatformInterruptAckPreserve = PlatIrqAck->PreserveMask;
  PccAcpi->PlatformInterruptAckSet      = PlatIrqAck->WriteMask;

  PccAcpi->Reserved1[0] = EFI_ACPI_RESERVED_BYTE;
  PccAcpi->Reserved1[1] = EFI_ACPI_RESERVED_BYTE;
  PccAcpi->Reserved1[1] = EFI_ACPI_RESERVED_BYTE;
  PccAcpi->Reserved1[3] = EFI_ACPI_RESERVED_BYTE;
  PccAcpi->Reserved1[4] = EFI_ACPI_RESERVED_BYTE;
  PccAcpi->Reserved1[5] = EFI_ACPI_RESERVED_BYTE;
  PccAcpi->Reserved1[6] = EFI_ACPI_RESERVED_BYTE;
  PccAcpi->Reserved1[7] = EFI_ACPI_RESERVED_BYTE;

  CopyMem (
    &PccAcpi->CommandCompleteCheckRegister,
    &CmdCompleteCheck->Register,
    sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE)
    );
  PccAcpi->CommandCompleteCheckMask = CmdCompleteCheck->PreserveMask;
  // No Write mask.

  CopyMem (
    &PccAcpi->CommandCompleteUpdateRegister,
    &CmdCompleteUpdate->Register,
    sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE)
    );
  PccAcpi->CommandCompleteUpdatePreserve = CmdCompleteUpdate->PreserveMask;
  PccAcpi->CommandCompleteUpdateSet      = CmdCompleteUpdate->WriteMask;

  CopyMem (
    &PccAcpi->ErrorStatusRegister,
    &ErrorStatus->Register,
    sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE)
    );
  PccAcpi->ErrorStatusMask = ErrorStatus->PreserveMask;
  // No Write mask.

  if (GenericPccCmObj->Type == EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_4_EXTENDED_PCC) {
    mHasPlatformInterrupt = TRUE;
  } else if ((PccCmObj->PlatIrq.Interrupt != 0)) {
    mHasPlatformInterrupt = TRUE;
  }

  return EFI_SUCCESS;
}

/** Add one PCCT subspace structure of Type 5 (HW-Registers).

  @param [in]  PccCmObj   Pointer to a CmObj PCCT Subspace info structure.
  @param [in]  PccAcpi    Pointer to the ACPI PCCT Subspace structure to populate.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
AddSubspaceStructType5 (
  IN  CM_ARM_PCC_SUBSPACE_TYPE5_INFO                            *PccCmObj,
  IN  EFI_ACPI_6_4_PCCT_SUBSPACE_5_HW_REGISTERS_COMMUNICATIONS  *PccAcpi
  )
{
  CM_ARM_PCC_SUBSPACE_TYPE0_INFO    *GenericPccCmObj;
  PCC_MAILBOX_REGISTER_INFO         *Doorbell;
  PCC_MAILBOX_REGISTER_INFO         *CmdCompleteCheck;
  PCC_MAILBOX_REGISTER_INFO         *ErrorStatus;
  PCC_SUBSPACE_CHANNEL_TIMING_INFO  *ChannelTiming;

  GenericPccCmObj = (CM_ARM_PCC_SUBSPACE_TYPE0_INFO *)PccCmObj;

  if ((PccCmObj == NULL) ||
      (GenericPccCmObj->Type != EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_5_HW_REGISTERS_COMMUNICATIONS)  ||
      (PccAcpi == NULL))
  {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  Doorbell         = &GenericPccCmObj->DoorbellReg;
  CmdCompleteCheck = &PccCmObj->CmdCompleteCheckReg;
  ErrorStatus      = &PccCmObj->ErrorStatusReg;
  ChannelTiming    = &GenericPccCmObj->ChannelTiming;

  PccAcpi->Type                    = GenericPccCmObj->Type;
  PccAcpi->Length                  = sizeof (EFI_ACPI_6_4_PCCT_SUBSPACE_5_HW_REGISTERS_COMMUNICATIONS);
  PccAcpi->Version                 = PccCmObj->Version;
  PccAcpi->BaseAddress             = GenericPccCmObj->BaseAddress;
  PccAcpi->SharedMemoryRangeLength = GenericPccCmObj->AddressLength;

  CopyMem (
    &PccAcpi->DoorbellRegister,
    &Doorbell->Register,
    sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE)
    );
  PccAcpi->DoorbellPreserve = Doorbell->PreserveMask;
  PccAcpi->DoorbellWrite    = Doorbell->WriteMask;

  CopyMem (
    &PccAcpi->CommandCompleteCheckRegister,
    &CmdCompleteCheck->Register,
    sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE)
    );
  PccAcpi->CommandCompleteCheckMask = CmdCompleteCheck->PreserveMask;
  // No Write mask.

  CopyMem (
    &PccAcpi->ErrorStatusRegister,
    &ErrorStatus->Register,
    sizeof (EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE)
    );
  PccAcpi->ErrorStatusMask = ErrorStatus->PreserveMask;
  // No Write mask.

  PccAcpi->NominalLatency = ChannelTiming->NominalLatency;
  // No MaximumPeriodicAccessRate.
  PccAcpi->MinimumRequestTurnaroundTime = ChannelTiming->MinRequestTurnaroundTime;

  return EFI_SUCCESS;
}

/** Populate the PCCT table using the MappingTable.

  @param [in] MappingTable  The mapping table structure.
  @param [in] Pcc           Pointer to an array of Pcc Subpace structures.
  @param [in] Size          Size of the Pcc array.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_BUFFER_TOO_SMALL  Buffer too small.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
PopulatePcctTable (
  IN  MAPPING_TABLE  *MappingTable,
  IN  VOID           *Pcc,
  IN  UINT32         Size
  )
{
  EFI_STATUS  Status;
  UINT8       *PccBuffer;
  UINT32      CmObjSize;
  UINT32      Index;
  UINT32      MaxIndex;
  VOID        **Table;
  VOID        *CurrentPccSubspace;

  ASSERT (MappingTable != NULL);
  ASSERT (MappingTable->Table != NULL);

  PccBuffer = Pcc;
  MaxIndex  = MappingTable->MaxIndex;
  Table     = MappingTable->Table;

  for (Index = 0; Index < MaxIndex; Index++) {
    CurrentPccSubspace = Table[Index];
    if (CurrentPccSubspace == NULL) {
      ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
      return EFI_INVALID_PARAMETER;
    }

    switch (((PCC_SUBSPACE_GENERIC_INFO *)CurrentPccSubspace)->Type) {
      case EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_GENERIC:
        Status = AddSubspaceStructType0 (
                   (CM_ARM_PCC_SUBSPACE_TYPE0_INFO *)CurrentPccSubspace,
                   (EFI_ACPI_6_4_PCCT_SUBSPACE_GENERIC *)PccBuffer
                   );

        CmObjSize = sizeof (EFI_ACPI_6_4_PCCT_SUBSPACE_GENERIC);
        break;

      case EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_1_HW_REDUCED_COMMUNICATIONS:
        Status = AddSubspaceStructType1 (
                   (CM_ARM_PCC_SUBSPACE_TYPE1_INFO *)CurrentPccSubspace,
                   (EFI_ACPI_6_4_PCCT_SUBSPACE_1_HW_REDUCED_COMMUNICATIONS *)PccBuffer
                   );

        CmObjSize = sizeof (EFI_ACPI_6_4_PCCT_SUBSPACE_1_HW_REDUCED_COMMUNICATIONS);
        break;

      case EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_2_HW_REDUCED_COMMUNICATIONS:
        Status = AddSubspaceStructType2 (
                   (CM_ARM_PCC_SUBSPACE_TYPE2_INFO *)CurrentPccSubspace,
                   (EFI_ACPI_6_4_PCCT_SUBSPACE_2_HW_REDUCED_COMMUNICATIONS *)PccBuffer
                   );

        CmObjSize = sizeof (EFI_ACPI_6_4_PCCT_SUBSPACE_2_HW_REDUCED_COMMUNICATIONS);
        break;

      case EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_3_EXTENDED_PCC:
        Status = AddSubspaceStructType34 (
                   (CM_ARM_PCC_SUBSPACE_TYPE3_INFO *)CurrentPccSubspace,
                   (EFI_ACPI_6_4_PCCT_SUBSPACE_3_EXTENDED_PCC *)PccBuffer
                   );

        CmObjSize = sizeof (EFI_ACPI_6_4_PCCT_SUBSPACE_3_EXTENDED_PCC);
        break;

      case EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_4_EXTENDED_PCC:
        Status = AddSubspaceStructType34 (
                   (CM_ARM_PCC_SUBSPACE_TYPE4_INFO *)CurrentPccSubspace,
                   (EFI_ACPI_6_4_PCCT_SUBSPACE_4_EXTENDED_PCC *)PccBuffer
                   );

        CmObjSize = sizeof (EFI_ACPI_6_4_PCCT_SUBSPACE_4_EXTENDED_PCC);
        break;

      case EFI_ACPI_6_4_PCCT_SUBSPACE_TYPE_5_HW_REGISTERS_COMMUNICATIONS:
        Status = AddSubspaceStructType5 (
                   (CM_ARM_PCC_SUBSPACE_TYPE5_INFO *)CurrentPccSubspace,
                   (EFI_ACPI_6_4_PCCT_SUBSPACE_5_HW_REGISTERS_COMMUNICATIONS *)PccBuffer
                   );

        CmObjSize = sizeof (EFI_ACPI_6_4_PCCT_SUBSPACE_5_HW_REGISTERS_COMMUNICATIONS);
        break;

      default:
        ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
        return EFI_INVALID_PARAMETER;
    } // switch

    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    if (Size < CmObjSize) {
      ASSERT_EFI_ERROR (EFI_BUFFER_TOO_SMALL);
      return EFI_BUFFER_TOO_SMALL;
    }

    PccBuffer += CmObjSize;
    Size      -= CmObjSize;
  } // for

  return EFI_SUCCESS;
}

/** Construct the PCCT ACPI table.

  Called by the Dynamic Table Manager, this function invokes the
  Configuration Manager protocol interface to get the required hardware
  information for generating the ACPI table.

  If this function allocates any resources then they must be freed
  in the FreeXXXXTableResources function.

  @param [in]  This           Pointer to the table generator.
  @param [in]  AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                              Protocol Interface.
  @param [out] Table          Pointer to the constructed ACPI Table.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object was not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
  @retval EFI_BUFFER_TOO_SMALL   Buffer too small.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
STATIC
EFI_STATUS
EFIAPI
BuildPcctTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS                                                Status;
  ACPI_PCCT_GENERATOR                                       *Generator;
  UINT32                                                    TableSize;
  EFI_ACPI_6_4_PLATFORM_COMMUNICATION_CHANNEL_TABLE_HEADER  *Pcct;
  UINT8                                                     *Buffer;

  MAPPING_TABLE  *MappingTable;
  UINT32         MappingTableCount;

  CM_ARM_PCC_SUBSPACE_TYPE0_INFO  *PccType0;
  UINT32                          PccType0Count;
  CM_ARM_PCC_SUBSPACE_TYPE1_INFO  *PccType1;
  UINT32                          PccType1Count;
  CM_ARM_PCC_SUBSPACE_TYPE2_INFO  *PccType2;
  UINT32                          PccType2Count;
  CM_ARM_PCC_SUBSPACE_TYPE3_INFO  *PccType3;
  UINT32                          PccType3Count;
  CM_ARM_PCC_SUBSPACE_TYPE4_INFO  *PccType4;
  UINT32                          PccType4Count;
  CM_ARM_PCC_SUBSPACE_TYPE5_INFO  *PccType5;
  UINT32                          PccType5Count;

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
      "ERROR: PCCT: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  Generator    = (ACPI_PCCT_GENERATOR *)This;
  MappingTable = &Generator->MappingTable;
  *Table       = NULL;

  // First get all the Pcc Subpace CmObj of type X.

  Status = GetEArmObjPccSubspaceType0Info (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &PccType0,
             &PccType0Count
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  Status = GetEArmObjPccSubspaceType1Info (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &PccType1,
             &PccType1Count
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  Status = GetEArmObjPccSubspaceType2Info (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &PccType2,
             &PccType2Count
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  Status = GetEArmObjPccSubspaceType3Info (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &PccType3,
             &PccType3Count
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  Status = GetEArmObjPccSubspaceType4Info (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &PccType4,
             &PccType4Count
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  Status = GetEArmObjPccSubspaceType5Info (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &PccType5,
             &PccType5Count
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  // Count the number of Pcc Subspaces.
  MappingTableCount  = PccType0Count;
  MappingTableCount += PccType1Count;
  MappingTableCount += PccType2Count;
  MappingTableCount += PccType3Count;
  MappingTableCount += PccType4Count;
  MappingTableCount += PccType5Count;

  Status = MappingTableInitialize (MappingTable, MappingTableCount);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  // Map the Subspace Ids for all types.

  Status = MapPccSubspaceId (MappingTable, PccType0, PccType0Count);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  Status = MapPccSubspaceId (MappingTable, PccType1, PccType1Count);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  Status = MapPccSubspaceId (MappingTable, PccType2, PccType2Count);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  Status = MapPccSubspaceId (MappingTable, PccType3, PccType3Count);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  Status = MapPccSubspaceId (MappingTable, PccType4, PccType4Count);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  Status = MapPccSubspaceId (MappingTable, PccType5, PccType5Count);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  // Compute the size of the PCCT table.
  TableSize  = sizeof (EFI_ACPI_6_4_PLATFORM_COMMUNICATION_CHANNEL_TABLE_HEADER);
  TableSize += PccType0Count * sizeof (EFI_ACPI_6_4_PCCT_SUBSPACE_GENERIC);
  TableSize += PccType1Count * sizeof (EFI_ACPI_6_4_PCCT_SUBSPACE_1_HW_REDUCED_COMMUNICATIONS);
  TableSize += PccType2Count * sizeof (EFI_ACPI_6_4_PCCT_SUBSPACE_2_HW_REDUCED_COMMUNICATIONS);
  TableSize += PccType3Count * sizeof (EFI_ACPI_6_4_PCCT_SUBSPACE_3_EXTENDED_PCC);
  TableSize += PccType4Count * sizeof (EFI_ACPI_6_4_PCCT_SUBSPACE_4_EXTENDED_PCC);
  TableSize += PccType5Count * sizeof (EFI_ACPI_6_4_PCCT_SUBSPACE_5_HW_REGISTERS_COMMUNICATIONS);

  // Allocate a Buffer for the PCCT table.
  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)AllocateZeroPool (TableSize);
  if (*Table == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  Pcct = (EFI_ACPI_6_4_PLATFORM_COMMUNICATION_CHANNEL_TABLE_HEADER *)*Table;

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Pcct->Header,
             AcpiTableInfo,
             TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: PCCT: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  Buffer     = (UINT8 *)Pcct;
  Buffer    += sizeof (EFI_ACPI_6_4_PLATFORM_COMMUNICATION_CHANNEL_TABLE_HEADER);
  TableSize -= sizeof (EFI_ACPI_6_4_PLATFORM_COMMUNICATION_CHANNEL_TABLE_HEADER);

  // Populate the PCCT table by following the Subspace Id mapping.
  Status = PopulatePcctTable (MappingTable, Buffer, TableSize);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  // Setup the Reserved fields once mHasPlatformInterrupt hase been populated.
  Pcct->Flags    = mHasPlatformInterrupt;
  Pcct->Reserved = EFI_ACPI_RESERVED_QWORD;

  MappingTableFree (MappingTable);

  return Status;

error_handler:
  DEBUG ((
    DEBUG_ERROR,
    "ERROR: PCCT: Failed to install table. Status = %r\n",
    Status
    ));

  if (*Table != NULL) {
    FreePool (*Table);
    *Table = NULL;
  }

  MappingTableFree (MappingTable);

  return Status;
}

/** Free any resources allocated for constructing the PCCT.

  @param [in]      This           Pointer to the table generator.
  @param [in]      AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in, out] Table          Pointer to the ACPI Table.

  @retval EFI_SUCCESS           The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER The table pointer is NULL or invalid.
**/
STATIC
EFI_STATUS
FreePcctTableResources (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: PCCT: Invalid Table Pointer\n"));
    ASSERT ((Table != NULL) && (*Table != NULL));
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;
  return EFI_SUCCESS;
}

/** This macro defines the PCCT Table Generator revision.
*/
#define PCCT_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the PCCT Table Generator.
*/
STATIC
ACPI_PCCT_GENERATOR  PcctGenerator = {
  // ACPI table generator header
  {
    // Generator ID
    CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdPcct),
    // Generator Description
    L"ACPI.STD.PCCT.GENERATOR",
    // ACPI Table Signature
    EFI_ACPI_6_4_PLATFORM_COMMUNICATIONS_CHANNEL_TABLE_SIGNATURE,
    // ACPI Table Revision supported by this Generator
    EFI_ACPI_6_4_PLATFORM_COMMUNICATION_CHANNEL_TABLE_REVISION,
    // Minimum ACPI Table Revision supported by this Generator
    EFI_ACPI_6_4_PLATFORM_COMMUNICATION_CHANNEL_TABLE_REVISION,
    // Creator ID
    TABLE_GENERATOR_CREATOR_ID_ARM,
    // Creator Revision
    PCCT_GENERATOR_REVISION,
    // Build Table function
    BuildPcctTable,
    // Free Resource function
    FreePcctTableResources,
    // Extended build function not needed
    NULL,
    // Extended build function not implemented by the generator.
    // Hence extended free resource function is not required.
    NULL
  },

  // Private fields are defined from here.

  // Mapping Table
  {
    // Table
    NULL,
    // MaxIndex
    0,
  },
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
AcpiPcctLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&PcctGenerator.Header);
  DEBUG ((DEBUG_INFO, "PCCT: Register Generator. Status = %r\n", Status));
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
AcpiPcctLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&PcctGenerator.Header);
  DEBUG ((DEBUG_INFO, "PCCT: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
