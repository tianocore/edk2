/** @file
  VarCheckHiiLib Dependency DXE Driver.
  This driver sends HII variable checking data to SMM at the end of DXE phase
  using the MM Communication protocol.
  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Protocol/MmCommunication.h>
#include <Guid/PiSmmCommunicationRegionTable.h>
#include "InternalVarCheckStructure.h"
#include "VarCheckHiiGen.h"
#include <Library/UefiLib.h>
#include <Guid/EventGroup.h>

extern EFI_GUID                       gEfiVariableCheckHiiCommunicationGuid;
extern VAR_CHECK_HII_VARIABLE_HEADER  *mVarCheckHiiBin;
extern UINTN                          mVarCheckHiiBinSize;

/**
  Sends HII variable checking data to SMM at the end of DXE phase.
  This function is triggered by the End of DXE. It locates a memory
  region for MM communication, prepares the communication buffer with HII variable
  checking data, and communicates with SMM using the MM Communication protocol.

  @param[in] Event      Event whose notification function is being invoked.
  @param[in] Context    The pointer to the notification function's context, which
                        is implementation-dependent.
**/
VOID
EFIAPI
SmmEndOfDxeNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                               Status;
  EFI_MM_COMMUNICATION_PROTOCOL            *MmCommunication;
  EFI_MM_COMMUNICATE_HEADER                *CommHeader;
  EDKII_PI_SMM_COMMUNICATION_REGION_TABLE  *PiSmmCommunicationRegionTable;
  EFI_MEMORY_DESCRIPTOR                    *MmCommMemRegion;
  UINTN                                    CommBufferSize;
  UINTN                                    Index;
  VAR_CHECK_HII_VARIABLE_HEADER            *VarCheckHiiVariable;

  DEBUG ((DEBUG_INFO, "SmmEndOfDxeNotify Start\n"));
  VarCheckHiiGen ();
  if ((mVarCheckHiiBinSize == 0) || (mVarCheckHiiBin == NULL)) {
    DEBUG ((DEBUG_INFO, "mVarCheckHiiBinSize = 0x%x, mVarCheckHiiBin = 0x%x \n", mVarCheckHiiBinSize, mVarCheckHiiBin));
    return;
  }

  //
  // Retrieve SMM Communication Region Table
  //
  Status = EfiGetSystemConfigurationTable (
             &gEdkiiPiSmmCommunicationRegionTableGuid,
             (VOID **)&PiSmmCommunicationRegionTable
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get PiSmmCommunicationRegionTable - %r\n", Status));
    return;
  }

  ASSERT (PiSmmCommunicationRegionTable != NULL);
  //
  // Find a memory region for MM communication
  //
  CommBufferSize  = 0;
  MmCommMemRegion = (EFI_MEMORY_DESCRIPTOR *)(PiSmmCommunicationRegionTable + 1);
  for (Index = 0; Index < PiSmmCommunicationRegionTable->NumberOfEntries; Index++) {
    if (MmCommMemRegion->Type == EfiConventionalMemory) {
      CommBufferSize = EFI_PAGES_TO_SIZE ((UINTN)MmCommMemRegion->NumberOfPages);
      if (CommBufferSize >= (sizeof (EFI_MM_COMMUNICATE_HEADER) + mVarCheckHiiBinSize)) {
        break;
      }
    }

    MmCommMemRegion = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MmCommMemRegion + PiSmmCommunicationRegionTable->DescriptorSize);
  }

  if (Index >= PiSmmCommunicationRegionTable->NumberOfEntries) {
    DEBUG ((DEBUG_ERROR, "SmmEndOfDxeNotify - Failed to find a suitable memory region for MM communication!\n"));
    return;
  }

  //
  // Prepare the communication buffer
  //
  CommHeader     = (EFI_MM_COMMUNICATE_HEADER *)(UINTN)MmCommMemRegion->PhysicalStart;
  CommBufferSize = OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data) + mVarCheckHiiBinSize;
  ZeroMem (CommHeader, CommBufferSize);
  CopyGuid (&CommHeader->HeaderGuid, &gEfiVariableCheckHiiCommunicationGuid);
  CommHeader->MessageLength = mVarCheckHiiBinSize;
  VarCheckHiiVariable       = (VAR_CHECK_HII_VARIABLE_HEADER *)(CommHeader->Data);
  CopyMem (VarCheckHiiVariable, mVarCheckHiiBin, mVarCheckHiiBinSize); // seperate
  //
  // Locate the MM Communication protocol and signal SMI
  //
  Status = gBS->LocateProtocol (&gEfiMmCommunicationProtocolGuid, NULL, (VOID **)&MmCommunication);

  if (!EFI_ERROR (Status)) {
    Status = MmCommunication->Communicate (MmCommunication, CommHeader, &CommBufferSize);
    DEBUG ((DEBUG_INFO, "SmmEndOfDxeNotify - Communicate to smm environment = %r\n", Status));
  } else {
    DEBUG ((DEBUG_ERROR, "SmmEndOfDxeNotify - Failed to locate MmCommunication protocol - %r\n", Status));
    return;
  }

  DEBUG ((DEBUG_INFO, "SmmEndOfDxeNotify Ends \n"));
  return;
}

/**
  Constructor function of the VarCheckHiiLibMmDependency DXE driver.
  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the Management mode System Table.
  @retval EFI_SUCCESS           The protocol was successfully installed into the DXE database.
**/
EFI_STATUS
EFIAPI
VarCheckHiiLibMmDependencyConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;

  DEBUG ((DEBUG_INFO, "VarCheckHiiLibMmDependencyConstructor Starts \n"));
  Status = gBS->CreateEventEx (EVT_NOTIFY_SIGNAL, TPL_NOTIFY, SmmEndOfDxeNotify, NULL, &gEfiEndOfDxeEventGroupGuid, &Event);
  ASSERT_EFI_ERROR (Status);
  DEBUG ((DEBUG_INFO, "VarCheckHiiLibMmDependencyConstructor Ends \n"));
  return Status;
}
