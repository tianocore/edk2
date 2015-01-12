/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

  AcpiPlatformHooks.c

Abstract:

  ACPI Platform Driver Hooks

--*/

//
// Statements that include other files.
//
#include "AcpiPlatform.h"
#include "AcpiPlatformHooks.h"
#include "Platform.h"

//
// Prototypes of the various hook functions.
//
#include "AcpiPlatformHooksLib.h"

extern EFI_GLOBAL_NVS_AREA_PROTOCOL  mGlobalNvsArea;
extern SYSTEM_CONFIGURATION             mSystemConfiguration;

ENHANCED_SPEEDSTEP_PROTOCOL             *mEistProtocol  = NULL;

EFI_CPU_ID_MAP              mCpuApicIdAcpiIdMapTable[MAX_CPU_NUM];

EFI_STATUS
AppendCpuMapTableEntry (
  IN EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC_STRUCTURE   *AcpiLocalApic
  )
{
  BOOLEAN Added;
  UINTN   Index;

  for (Index = 0; Index < MAX_CPU_NUM; Index++) {
    if ((mCpuApicIdAcpiIdMapTable[Index].ApicId == AcpiLocalApic->ApicId) && mCpuApicIdAcpiIdMapTable[Index].Flags) {
      return EFI_SUCCESS;
    }
  }

  Added = FALSE;
  for (Index = 0; Index < MAX_CPU_NUM; Index++) {
      if (!mCpuApicIdAcpiIdMapTable[Index].Flags) {
        mCpuApicIdAcpiIdMapTable[Index].Flags           = 1;
        mCpuApicIdAcpiIdMapTable[Index].ApicId          = AcpiLocalApic->ApicId;
        mCpuApicIdAcpiIdMapTable[Index].AcpiProcessorId = AcpiLocalApic->AcpiProcessorId;
      Added = TRUE;
      break;
    }
  }

  ASSERT (Added);
  return EFI_SUCCESS;
}

UINT32
ProcessorId2ApicId (
  UINT32  AcpiProcessorId
  )
{
  UINTN Index;

  ASSERT (AcpiProcessorId < MAX_CPU_NUM);
  for (Index = 0; Index < MAX_CPU_NUM; Index++) {
    if (mCpuApicIdAcpiIdMapTable[Index].Flags && (mCpuApicIdAcpiIdMapTable[Index].AcpiProcessorId == AcpiProcessorId)) {
      return mCpuApicIdAcpiIdMapTable[Index].ApicId;
    }
  }

  return (UINT32) -1;
}

UINT8
GetProcNumberInPackage (
  IN UINT8  Package
  )
{
  UINTN Index;
  UINT8 Number;

  Number = 0;
  for (Index = 0; Index < MAX_CPU_NUM; Index++) {
    if (mCpuApicIdAcpiIdMapTable[Index].Flags && (((mCpuApicIdAcpiIdMapTable[Index].ApicId >> 0x04) & 0x01) == Package)) {
      Number++;
    }
  }

  return Number;
}

EFI_STATUS
LocateCpuEistProtocol (
  IN UINT32                           CpuIndex,
  OUT ENHANCED_SPEEDSTEP_PROTOCOL     **EistProtocol
  )
{
  UINTN                       HandleCount;
  EFI_HANDLE                  *HandleBuffer;
  ENHANCED_SPEEDSTEP_PROTOCOL *EistProt;
  UINTN                       Index;
  UINT32                      ApicId;
  EFI_STATUS                  Status;

  HandleCount = 0;
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEnhancedSpeedstepProtocolGuid,
         NULL,
         &HandleCount,
         &HandleBuffer
         );

  Index     = 0;
  EistProt  = NULL;
  Status    = EFI_NOT_FOUND;
  while (Index < HandleCount) {
    gBS->HandleProtocol (
           HandleBuffer[Index],
           &gEnhancedSpeedstepProtocolGuid,
          (VOID **) &EistProt
           );
    //
    // Adjust the CpuIndex by +1 due to the AcpiProcessorId is 1 based.
    //
    ApicId = ProcessorId2ApicId (CpuIndex+1);
    if (ApicId == (UINT32) -1) {
      break;
    }

    if (EistProt->ProcApicId == ApicId) {
      Status = EFI_SUCCESS;
      break;
    }

    Index++;
  }

  if (HandleBuffer != NULL) {
    gBS->FreePool (HandleBuffer);
  }

  if (!EFI_ERROR (Status)) {
    *EistProtocol = EistProt;
  } else {
    *EistProtocol = NULL;
  }

  return Status;
}

EFI_STATUS
PlatformHookInit (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (
                  &gEnhancedSpeedstepProtocolGuid,
                  NULL,
                  (VOID **) &mEistProtocol
                  );

  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Called for every ACPI table found in the BIOS flash.
  Returns whether a table is active or not. Inactive tables
  are not published in the ACPI table list.

  This hook can be used to implement optional SSDT tables or
  enabling/disabling specific functionality (e.g. SPCR table)
  based on a setup switch or platform preference. In case of
  optional SSDT tables,the platform flash will include all the
  SSDT tables but will return EFI_SUCCESS only for those tables
  that need to be published.

  @param[in]  *Table         Pointer to the active table.

  @retval  EFI_SUCCESS       if the table is active.
  @retval  EFI_UNSUPPORTED   if the table is not active.

**/
EFI_STATUS
AcpiPlatformHooksIsActiveTable (
  IN OUT EFI_ACPI_COMMON_HEADER     *Table
  )
{
  EFI_ACPI_DESCRIPTION_HEADER *TableHeader;

  TableHeader = (EFI_ACPI_DESCRIPTION_HEADER *) Table;

  if (TableHeader->Signature == EFI_ACPI_2_0_STATIC_RESOURCE_AFFINITY_TABLE_SIGNATURE) {

  }

  if ((mSystemConfiguration.ENDBG2 == 0) && (CompareMem (&TableHeader->OemTableId, "INTLDBG2", 8) == 0)) {
    return EFI_UNSUPPORTED;
  }
  return EFI_SUCCESS;
}

/**
    Update the GV3 SSDT table.

    @param[in][out]  *TableHeader   The table to be set.

    @retval  EFI_SUCCESS            Returns Success.

**/
EFI_STATUS
PatchGv3SsdtTable (
  IN OUT   EFI_ACPI_DESCRIPTION_HEADER  *TableHeader
  )
{
  EFI_STATUS                  Status;
  UINT8                       *CurrPtr;
  UINT8                       *SsdtPointer;
  UINT32                      Signature;
  UINT32                      CpuFixes;
  UINT32                      NpssFixes;
  UINT32                      SpssFixes;
  UINT32                      CpuIndex;
  UINT32                      PackageSize;
  UINT32                      NewPackageSize;
  UINT32                      AdjustSize;
  UINTN                       EntryIndex;
  UINTN                       TableIndex;
  EFI_ACPI_NAME_COMMAND       *PssTable;
  EFI_PSS_PACKAGE             *PssTableItemPtr;
  ENHANCED_SPEEDSTEP_PROTOCOL *EistProt;
  EIST_INFORMATION            *EistInfo;
  EFI_ACPI_CPU_PSS_STATE      *PssState;
  EFI_ACPI_NAMEPACK_DWORD     *NamePtr;
  //
  // Loop through the ASL looking for values that we must fix up.
  //
  NpssFixes = 0;
  SpssFixes = 0;
  CpuFixes  = 0;
  CpuIndex  = 0;
  CurrPtr   = (UINT8 *) TableHeader;

  EistProt  = NULL;
  for (SsdtPointer = CurrPtr; SsdtPointer <= (CurrPtr + ((EFI_ACPI_COMMON_HEADER *) CurrPtr)->Length); SsdtPointer++) {
    Signature = *(UINT32 *) SsdtPointer;
    switch (Signature) {

    case SIGNATURE_32 ('_', 'P', 'R', '_'):
      //
      // _CPUX ('0' to '0xF')
      //
      CpuIndex = *(SsdtPointer + 7);
      if (CpuIndex >= '0' && CpuIndex <= '9') {
        CpuIndex -= '0';
      } else {
        if (CpuIndex > '9') {
          CpuIndex -= '7';
        }
      }

      CpuFixes++;
      LocateCpuEistProtocol (CpuIndex, &EistProt);
      break;

    case SIGNATURE_32 ('D', 'O', 'M', 'N'):

      NamePtr = ACPI_NAME_COMMAND_FROM_NAMEPACK_STR (SsdtPointer);
      if (NamePtr->StartByte != AML_NAME_OP) {
        continue;
      }

      if (NamePtr->Size != AML_NAME_DWORD_SIZE) {
        continue;
      }

      NamePtr->Value = 0;

        if (mCpuApicIdAcpiIdMapTable[CpuIndex].Flags) {
          NamePtr->Value = (mCpuApicIdAcpiIdMapTable[CpuIndex].ApicId >> 0x04) & 0x01;
      }
      break;

    case SIGNATURE_32 ('N', 'C', 'P', 'U'):

      NamePtr = ACPI_NAME_COMMAND_FROM_NAMEPACK_STR (SsdtPointer);
      if (NamePtr->StartByte != AML_NAME_OP) {
        continue;
      }

      if (NamePtr->Size != AML_NAME_DWORD_SIZE) {
        continue;
      }

        NamePtr->Value = 0;
        if (mCpuApicIdAcpiIdMapTable[CpuIndex].Flags) {
          NamePtr->Value = GetProcNumberInPackage ((mCpuApicIdAcpiIdMapTable[CpuIndex].ApicId >> 0x04) & 0x01);
      }
      break;

    case SIGNATURE_32 ('N', 'P', 'S', 'S'):
    case SIGNATURE_32 ('S', 'P', 'S', 'S'):
      if (EistProt == NULL) {
        continue;
      }

      PssTable = ACPI_NAME_COMMAND_FROM_NAME_STR (SsdtPointer);
      if (PssTable->StartByte != AML_NAME_OP) {
        continue;
      }

      Status      = EistProt->GetEistTable (EistProt, &EistInfo, (VOID **) &PssState);

      AdjustSize  = PssTable->NumEntries * sizeof (EFI_PSS_PACKAGE);
      AdjustSize -= EistInfo->NumStates * sizeof (EFI_PSS_PACKAGE);
      PackageSize     = (PssTable->Size & 0xF) + ((PssTable->Size & 0xFF00) >> 4);
      NewPackageSize  = PackageSize - AdjustSize;
      PssTable->Size  = (UINT16) ((NewPackageSize & 0xF) + ((NewPackageSize & 0x0FF0) << 4));

      //
      // Set most significant two bits of byte zero to 01, meaning two bytes used.
      //
      PssTable->Size |= 0x40;

      //
      // Set unused table to Noop Code.
      //
      SetMem( (UINT8 *) PssTable + NewPackageSize + AML_NAME_PREFIX_SIZE, AdjustSize, AML_NOOP_OP);
      PssTable->NumEntries  = (UINT8) EistInfo->NumStates;
      PssTableItemPtr       = (EFI_PSS_PACKAGE *) ((UINT8 *) PssTable + sizeof (EFI_ACPI_NAME_COMMAND));

      //
      // Update the size.
      //
      for (TableIndex = 0; TableIndex < EistInfo->NumStates; TableIndex++) {
        EntryIndex                = EistInfo->NumStates - TableIndex - 1;
        PssTableItemPtr->CoreFreq = PssState[EntryIndex].CoreFrequency * PssState[EntryIndex].Control;
        PssTableItemPtr->Power    = PssState[EntryIndex].Power * 1000;
        if (PssTable->NameStr == SIGNATURE_32 ('N', 'P', 'S', 'S')) {
          PssTableItemPtr->BMLatency    = PssState[EntryIndex].BusMasterLatency;
          PssTableItemPtr->TransLatency = PssState[EntryIndex].TransitionLatency;
        } else {
          //
          // This method should be supported by SMM PPM Handler.
          //
          PssTableItemPtr->BMLatency    = PssState[EntryIndex].BusMasterLatency * 2;
          PssTableItemPtr->TransLatency = PssState[EntryIndex].TransitionLatency * 10;
        }

        PssTableItemPtr->Control  = PssState[EntryIndex].Control;
        PssTableItemPtr->Status   = PssState[EntryIndex].Status;
        PssTableItemPtr++;
      }

      if (PssTable->NameStr == SIGNATURE_32 ('N', 'P', 'S', 'S')) {
        NpssFixes++;
      } else {
        SpssFixes++;
      }

      SsdtPointer = (UINT8 *) PssTable + PackageSize;
      break;
    }
  }

  //
  // N fixes together currently.
  //
  ASSERT (CpuFixes == (UINT32) MAX_CPU_NUM);
  ASSERT (SpssFixes == NpssFixes);
  ASSERT (CpuFixes >= SpssFixes);

  return EFI_SUCCESS;
}

/**
    Update the DSDT table.

    @param[in][out]  *TableHeader   The table to be set.

    @retval  EFI_SUCCESS            Returns EFI_SUCCESS.

**/
EFI_STATUS
PatchDsdtTable (
  IN OUT   EFI_ACPI_DESCRIPTION_HEADER  *TableHeader
  )
{

  UINT8                              *CurrPtr;
  UINT8                              *DsdtPointer;
  UINT32                             *Signature;
  UINT8                              *EndPtr;
  UINT8   *Operation;
  UINT32  *Address;
  UINT16  *Size;

  //
  // Fix PCI32 resource "FIX0" -- PSYS system status area
  //
  CurrPtr = (UINT8*) &((EFI_ACPI_DESCRIPTION_HEADER*) TableHeader)[0];
  EndPtr = (UINT8*) TableHeader;
  EndPtr = EndPtr + TableHeader->Length;
  while (CurrPtr < (EndPtr-2)) {
    //
    // Removed the _S3 tag to indicate that we do not support S3. The 4th byte is blank space
    // since there are only 3 char "_S3".
    //
    if (mSystemConfiguration.AcpiSuspendState == 0) {
      //
      // For iasl compiler version 20061109.
      //
      if ((CurrPtr[0] == '_') && (CurrPtr[1] == 'S') && (CurrPtr[2] == '3') && (CurrPtr[3] == '_')) {
        break;
      }
      //
      // For iasl compiler version 20040527.
      //
      if ((CurrPtr[0] == '\\') && (CurrPtr[1] == '_') && (CurrPtr[2] == 'S') && (CurrPtr[3] == '3')) {
        break;
      }
    }
    CurrPtr++;
  }
  CurrPtr = (UINT8*) &((EFI_ACPI_DESCRIPTION_HEADER*) TableHeader)[0];
  EndPtr = (UINT8*) TableHeader;
  EndPtr = EndPtr + TableHeader->Length;
  while (CurrPtr < (EndPtr-2)) {
    //
    // For mipi dsi port select _DEP.
    //
    if (mSystemConfiguration.MipiDsi== 1) {
      //
      // For iasl compiler version 20061109.
      //
      if ((CurrPtr[0] == 'N') && (CurrPtr[1] == 'D') && (CurrPtr[2] == 'E') && (CurrPtr[3] == 'P')) {
        CurrPtr[0] = '_';
        break;
      }

    } else {
      if ((CurrPtr[0] == 'P') && (CurrPtr[1] == 'D') && (CurrPtr[2] == 'E') && (CurrPtr[3] == 'P')) {
        CurrPtr[0] = '_';
        break;
      }

    }
    CurrPtr++;
  }
  //
  // Loop through the ASL looking for values that we must fix up.
  //
  CurrPtr = (UINT8 *) TableHeader;
  for (DsdtPointer = CurrPtr; DsdtPointer <= (CurrPtr + ((EFI_ACPI_COMMON_HEADER *) CurrPtr)->Length); DsdtPointer++) {
    Signature = (UINT32 *) DsdtPointer;

    switch (*Signature) {
    //
    // GNVS operation region.
    //
    case (SIGNATURE_32 ('G', 'N', 'V', 'S')):
      //
      // Conditional match.  For Region Objects, the Operator will always be the
      // byte immediately before the specific name.  Therefore, subtract 1 to check
      // the Operator.
      //
      Operation = DsdtPointer - 1;
      if (*Operation == AML_OPREGION_OP) {
        Address   = (UINT32 *) (DsdtPointer + 6);
        *Address  = (UINT32) (UINTN) mGlobalNvsArea.Area;
        Size      = (UINT16 *) (DsdtPointer + 11);
        *Size     = sizeof (EFI_GLOBAL_NVS_AREA);
      }
      break;
    default:
      break;
    }
  }
  return EFI_SUCCESS;
}

