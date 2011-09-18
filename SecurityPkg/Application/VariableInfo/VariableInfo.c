/** @file
  If the Variable services have PcdVariableCollectStatistics set to TRUE then 
  this utility will print out the statistics information. You can use console 
  redirection to capture the data.
  
Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/BaseMemoryLib.h>

#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/SmmVariableCommon.h>
#include <Protocol/SmmCommunication.h>
#include <Protocol/SmmVariable.h>

extern EFI_GUID gEfiVariableGuid;
EFI_SMM_COMMUNICATION_PROTOCOL  *mSmmCommunication = NULL;

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
  IN OUT  EFI_SMM_COMMUNICATE_HEADER  *SmmCommunicateHeader,
  IN OUT  UINTN                       *SmmCommunicateSize
  )
{
  EFI_STATUS                          Status;
  SMM_VARIABLE_COMMUNICATE_HEADER     *SmmVariableFunctionHeader;

  CopyGuid (&SmmCommunicateHeader->HeaderGuid, &gEfiSmmVariableProtocolGuid);
  SmmCommunicateHeader->MessageLength = *SmmCommunicateSize - OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data);

  SmmVariableFunctionHeader = (SMM_VARIABLE_COMMUNICATE_HEADER *) &SmmCommunicateHeader->Data[0];
  SmmVariableFunctionHeader->Function = SMM_VARIABLE_FUNCTION_GET_STATISTICS;
  
  Status = mSmmCommunication->Communicate (mSmmCommunication, SmmCommunicateHeader, SmmCommunicateSize);
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
  EFI_STATUS                                     Status;
  VARIABLE_INFO_ENTRY                            *VariableInfo;
  EFI_SMM_COMMUNICATE_HEADER                     *CommBuffer;
  UINTN                                          RealCommSize;
  UINTN                                          CommSize;
  SMM_VARIABLE_COMMUNICATE_HEADER                *FunctionHeader;
  EFI_SMM_VARIABLE_PROTOCOL                      *Smmvariable;
  

  Status = gBS->LocateProtocol (&gEfiSmmVariableProtocolGuid, NULL, (VOID **) &Smmvariable);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->LocateProtocol (&gEfiSmmCommunicationProtocolGuid, NULL, (VOID **) &mSmmCommunication);
  if (EFI_ERROR (Status)) {
    return Status;
  }  

  CommSize  = SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE;
  RealCommSize = CommSize;
  CommBuffer = AllocateZeroPool (CommSize);
  ASSERT (CommBuffer != NULL);
  
  Print (L"Non-Volatile SMM Variables:\n");
  do {
    Status = GetVariableStatisticsData (CommBuffer, &CommSize);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      FreePool (CommBuffer);
      CommBuffer = AllocateZeroPool (CommSize);
      ASSERT (CommBuffer != NULL);
      RealCommSize = CommSize;
      Status = GetVariableStatisticsData (CommBuffer, &CommSize);
    }

    if (EFI_ERROR (Status) || (CommSize <= SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE)) { 
      break;
    }

    if (CommSize < RealCommSize) {
      CommSize = RealCommSize;
    }

    FunctionHeader = (SMM_VARIABLE_COMMUNICATE_HEADER *) CommBuffer->Data;
    VariableInfo   = (VARIABLE_INFO_ENTRY *) FunctionHeader->Data;

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
  
  Print (L"Volatile SMM Variables:\n");
  ZeroMem (CommBuffer, CommSize);
  do {
    Status = GetVariableStatisticsData (CommBuffer, &CommSize);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      FreePool (CommBuffer);
      CommBuffer = AllocateZeroPool (CommSize);
      ASSERT (CommBuffer != NULL);
      RealCommSize = CommSize;
      Status = GetVariableStatisticsData (CommBuffer, &CommSize);
    }

    if (EFI_ERROR (Status) || (CommSize <= SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE)) { 
      break;
    }

    if (CommSize < RealCommSize) {
      CommSize = RealCommSize;
    }

    FunctionHeader = (SMM_VARIABLE_COMMUNICATE_HEADER *) CommBuffer->Data;
    VariableInfo   = (VARIABLE_INFO_ENTRY *) FunctionHeader->Data;

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

  FreePool (CommBuffer);  
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
  EFI_STATUS            Status;
  VARIABLE_INFO_ENTRY   *VariableInfo;
  VARIABLE_INFO_ENTRY   *Entry;

  Status = EfiGetSystemConfigurationTable (&gEfiVariableGuid, (VOID **)&Entry);
  if (EFI_ERROR (Status) || (Entry == NULL)) {
    Status = EfiGetSystemConfigurationTable (&gEfiAuthenticatedVariableGuid, (VOID **)&Entry);
  }

  if (EFI_ERROR (Status) || (Entry == NULL)) {
    Status = PrintInfoFromSmm ();
    if (!EFI_ERROR (Status)) {
      return Status;
    }
  }  

  if (!EFI_ERROR (Status) && (Entry != NULL)) {
    Print (L"Non-Volatile EFI Variables:\n");
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

    Print (L"Volatile EFI Variables:\n");
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

  } else {
    Print (L"Warning: Variable Dxe driver doesn't enable the feature of statistical information!\n");
    Print (L"If you want to see this info, please:\n");
    Print (L"  1. Set PcdVariableCollectStatistics as TRUE\n");
    Print (L"  2. Rebuild Variable Dxe driver\n");
    Print (L"  3. Run \"VariableInfo\" cmd again\n");
  }

  return Status;
}
