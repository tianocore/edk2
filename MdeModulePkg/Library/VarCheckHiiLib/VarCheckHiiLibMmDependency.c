/** @file
  VarCheckHiiLib Dependency library.
  It sends HII variable checking data to SMM via the MM Communication protocol.
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
#include "VarCheckHii.h"
#include <Library/UefiLib.h>
#include <Guid/EventGroup.h>

extern VAR_CHECK_HII_VARIABLE_HEADER  *mVarCheckHiiBin;
extern UINTN                          mVarCheckHiiBinSize;
EFI_GUID                              gVarCheckReceivedHiiBinHandlerGuid = VAR_CHECK_RECEIVED_HII_BIN_HANDLER_GUID;

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
VarCheckHiiLibSmmEndOfDxeNotify (
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

  DEBUG ((DEBUG_INFO, "%a starts.\n", __func__));
  VarCheckHiiGen ();
  if ((mVarCheckHiiBinSize == 0) || (mVarCheckHiiBin == NULL)) {
    DEBUG ((DEBUG_INFO, "%a: mVarCheckHiiBinSize = 0x%x, mVarCheckHiiBin = 0x%x \n", __func__, mVarCheckHiiBinSize, mVarCheckHiiBin));
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
    DEBUG ((DEBUG_ERROR, "%a: Failed to get PiSmmCommunicationRegionTable - %r\n", __func__, Status));
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
    DEBUG ((DEBUG_ERROR, "%a: Failed to find a suitable memory region for MM communication!\n", __func__));
    return;
  }

  //
  // Prepare the communication buffer
  //
  CommHeader     = (EFI_MM_COMMUNICATE_HEADER *)(UINTN)MmCommMemRegion->PhysicalStart;
  CommBufferSize = OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data) + mVarCheckHiiBinSize;
  ZeroMem (CommHeader, CommBufferSize);
  CopyGuid (&CommHeader->HeaderGuid, &gVarCheckReceivedHiiBinHandlerGuid);
  CommHeader->MessageLength = mVarCheckHiiBinSize;
  VarCheckHiiVariable       = (VAR_CHECK_HII_VARIABLE_HEADER *)(CommHeader->Data);
  CopyMem (VarCheckHiiVariable, mVarCheckHiiBin, mVarCheckHiiBinSize);
  //
  // Locate the MM Communication protocol and signal SMI
  //
  Status = gBS->LocateProtocol (&gEfiMmCommunicationProtocolGuid, NULL, (VOID **)&MmCommunication);

  if (!EFI_ERROR (Status)) {
    Status = MmCommunication->Communicate (MmCommunication, CommHeader, &CommBufferSize);
    DEBUG ((DEBUG_INFO, "%a: Communicate to smm environment = %r\n", __func__, Status));
  } else {
    DEBUG ((DEBUG_ERROR, "%a: Failed to locate MmCommunication protocol - %r\n", __func__, Status));
    return;
  }

  DEBUG ((DEBUG_INFO, "%a ends.\n", __func__));
  return;
}

/**
  Constructor function of the VarCheckHiiLibMmDependency.
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

  DEBUG ((DEBUG_INFO, "%a starts.\n", __func__));
  Status = gBS->CreateEventEx (EVT_NOTIFY_SIGNAL, TPL_NOTIFY, VarCheckHiiLibSmmEndOfDxeNotify, NULL, &gEfiEndOfDxeEventGroupGuid, &Event);
  ASSERT_EFI_ERROR (Status);
  DEBUG ((DEBUG_INFO, "%a ends.\n", __func__));
  return Status;
}
