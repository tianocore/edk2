/** @file
  If the Variable services have PcdVariableCollectStatistics set to TRUE then
  this utility will print out the statistics information. You can use console
  redirection to capture the data.

  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Guid/VariableFormat.h>
#include <Guid/SmmVariableCommon.h>
#include <Guid/PiSmmCommunicationRegionTable.h>
#include <Protocol/MmCommunication2.h>
#include <Protocol/SmmVariable.h>

EFI_MM_COMMUNICATION2_PROTOCOL  *mMmCommunication2 = NULL;

/**
  This function get the variable statistics data from SMM variable driver.

  @param[in, out] SmmCommunicateHeader In input, a pointer to a collection of data that will
                                       be passed into an SMM environment. In output, a pointer
                                       to a collection of data that comes from an SMM environment.
  @param[in, out] SmmCommunicateSize   The size of the SmmCommunicateHeader.

  @retval EFI_SUCCESS               Get the statistics data information.
  @retval EFI_NOT_FOUND             Not found.
  @retval EFI_BUFFER_TO_SMALL       DataSize is too small for the result.

**/
EFI_STATUS
EFIAPI
GetVariableStatisticsData (
  IN OUT  EFI_MM_COMMUNICATE_HEADER  *SmmCommunicateHeader,
  IN OUT  UINTN                      *SmmCommunicateSize
  )
{
  EFI_STATUS                       Status;
  SMM_VARIABLE_COMMUNICATE_HEADER  *SmmVariableFunctionHeader;

  CopyGuid (&SmmCommunicateHeader->HeaderGuid, &gEfiSmmVariableProtocolGuid);
  SmmCommunicateHeader->MessageLength = *SmmCommunicateSize - OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data);

  SmmVariableFunctionHeader           = (SMM_VARIABLE_COMMUNICATE_HEADER *)&SmmCommunicateHeader->Data[0];
  SmmVariableFunctionHeader->Function = SMM_VARIABLE_FUNCTION_GET_STATISTICS;

  Status = mMmCommunication2->Communicate (
                                mMmCommunication2,
                                SmmCommunicateHeader,
                                SmmCommunicateHeader,
                                SmmCommunicateSize
                                );
  ASSERT_EFI_ERROR (Status);

  Status = SmmVariableFunctionHeader->ReturnStatus;
  return Status;
}

/**

  This function get and print the variable statistics data from SMM variable driver.

  @retval EFI_SUCCESS               Print the statistics information successfully.
  @retval EFI_NOT_FOUND             Not found the statistics information.

**/
EFI_STATUS
PrintInfoFromSmm (
  VOID
  )
{
  EFI_STATUS                               Status;
  VARIABLE_INFO_ENTRY                      *VariableInfo;
  EFI_MM_COMMUNICATE_HEADER                *CommBuffer;
  UINTN                                    RealCommSize;
  UINTN                                    CommSize;
  SMM_VARIABLE_COMMUNICATE_HEADER          *FunctionHeader;
  EFI_SMM_VARIABLE_PROTOCOL                *Smmvariable;
  EDKII_PI_SMM_COMMUNICATION_REGION_TABLE  *PiSmmCommunicationRegionTable;
  UINT32                                   Index;
  EFI_MEMORY_DESCRIPTOR                    *Entry;
  UINTN                                    Size;
  UINTN                                    MaxSize;

  Status = gBS->LocateProtocol (&gEfiSmmVariableProtocolGuid, NULL, (VOID **)&Smmvariable);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->LocateProtocol (&gEfiMmCommunication2ProtocolGuid, NULL, (VOID **)&mMmCommunication2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CommBuffer   = NULL;
  RealCommSize = 0;
  Status       = EfiGetSystemConfigurationTable (
                   &gEdkiiPiSmmCommunicationRegionTableGuid,
                   (VOID **)&PiSmmCommunicationRegionTable
                   );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (PiSmmCommunicationRegionTable != NULL);
  Entry   = (EFI_MEMORY_DESCRIPTOR *)(PiSmmCommunicationRegionTable + 1);
  Size    = 0;
  MaxSize = 0;
  for (Index = 0; Index < PiSmmCommunicationRegionTable->NumberOfEntries; Index++) {
    if (Entry->Type == EfiConventionalMemory) {
      Size = EFI_PAGES_TO_SIZE ((UINTN)Entry->NumberOfPages);
      if (Size > (SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + sizeof (VARIABLE_INFO_ENTRY))) {
        if (Size > MaxSize) {
          MaxSize      = Size;
          RealCommSize = MaxSize;
          CommBuffer   = (EFI_MM_COMMUNICATE_HEADER *)(UINTN)Entry->PhysicalStart;
        }
      }
    }

    Entry = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)Entry + PiSmmCommunicationRegionTable->DescriptorSize);
  }

  if (CommBuffer == NULL) {
    Print (L"Warning: No SMM communication buffer found!\n");
    ASSERT (CommBuffer != NULL);
    return EFI_NOT_FOUND;
  }

  ZeroMem (CommBuffer, RealCommSize);

  Print (L"SMM Driver Non-Volatile Variables:\n");
  do {
    CommSize = RealCommSize;
    Status   = GetVariableStatisticsData (CommBuffer, &CommSize);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Print (L"The generic SMM communication buffer provided by SmmCommunicationRegionTable is too small\n");
      return Status;
    }

    if (EFI_ERROR (Status) || (CommSize <= SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE)) {
      break;
    }

    FunctionHeader = (SMM_VARIABLE_COMMUNICATE_HEADER *)CommBuffer->Data;
    VariableInfo   = (VARIABLE_INFO_ENTRY *)FunctionHeader->Data;

    if (!VariableInfo->Volatile) {
      Print (
        L"%g R%03d(%03d) W%03d D%03d:%s\n",
        &VariableInfo->VendorGuid,
        VariableInfo->ReadCount,
        VariableInfo->CacheCount,
        VariableInfo->WriteCount,
        VariableInfo->DeleteCount,
        (CHAR16 *)(VariableInfo + 1)
        );
    }
  } while (TRUE);

  Print (L"SMM Driver Volatile Variables:\n");
  ZeroMem (CommBuffer, RealCommSize);
  do {
    CommSize = RealCommSize;
    Status   = GetVariableStatisticsData (CommBuffer, &CommSize);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Print (L"The generic SMM communication buffer provided by SmmCommunicationRegionTable is too small\n");
      return Status;
    }

    if (EFI_ERROR (Status) || (CommSize <= SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE)) {
      break;
    }

    FunctionHeader = (SMM_VARIABLE_COMMUNICATE_HEADER *)CommBuffer->Data;
    VariableInfo   = (VARIABLE_INFO_ENTRY *)FunctionHeader->Data;

    if (VariableInfo->Volatile) {
      Print (
        L"%g R%03d(%03d) W%03d D%03d:%s\n",
        &VariableInfo->VendorGuid,
        VariableInfo->ReadCount,
        VariableInfo->CacheCount,
        VariableInfo->WriteCount,
        VariableInfo->DeleteCount,
        (CHAR16 *)(VariableInfo + 1)
        );
    }
  } while (TRUE);

  return Status;
}

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the image goes into a library that calls this
  function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS           RuntimeDxeStatus;
  EFI_STATUS           SmmStatus;
  VARIABLE_INFO_ENTRY  *VariableInfo;
  VARIABLE_INFO_ENTRY  *Entry;

  RuntimeDxeStatus = EfiGetSystemConfigurationTable (&gEfiVariableGuid, (VOID **)&Entry);
  if (EFI_ERROR (RuntimeDxeStatus) || (Entry == NULL)) {
    RuntimeDxeStatus = EfiGetSystemConfigurationTable (&gEfiAuthenticatedVariableGuid, (VOID **)&Entry);
  }

  if (!EFI_ERROR (RuntimeDxeStatus) && (Entry != NULL)) {
    Print (L"Runtime DXE Driver Non-Volatile EFI Variables:\n");
    VariableInfo = Entry;
    do {
      if (!VariableInfo->Volatile) {
        Print (
          L"%g R%03d(%03d) W%03d D%03d:%s\n",
          &VariableInfo->VendorGuid,
          VariableInfo->ReadCount,
          VariableInfo->CacheCount,
          VariableInfo->WriteCount,
          VariableInfo->DeleteCount,
          VariableInfo->Name
          );
      }

      VariableInfo = VariableInfo->Next;
    } while (VariableInfo != NULL);

    Print (L"Runtime DXE Driver Volatile EFI Variables:\n");
    VariableInfo = Entry;
    do {
      if (VariableInfo->Volatile) {
        Print (
          L"%g R%03d(%03d) W%03d D%03d:%s\n",
          &VariableInfo->VendorGuid,
          VariableInfo->ReadCount,
          VariableInfo->CacheCount,
          VariableInfo->WriteCount,
          VariableInfo->DeleteCount,
          VariableInfo->Name
          );
      }

      VariableInfo = VariableInfo->Next;
    } while (VariableInfo != NULL);
  }

  SmmStatus = PrintInfoFromSmm ();

  if (EFI_ERROR (RuntimeDxeStatus) && EFI_ERROR (SmmStatus)) {
    Print (L"Warning: Variable Dxe/Smm driver doesn't enable the feature of statistical information!\n");
    Print (L"If you want to see this info, please:\n");
    Print (L"  1. Set PcdVariableCollectStatistics as TRUE\n");
    Print (L"  2. Rebuild Variable Dxe/Smm driver\n");
    Print (L"  3. Run \"VariableInfo\" cmd again\n");

    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}
