/** @file

  This file contains the implementation for a Platform Runtime Mechanism (PRM)
  loader driver.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PrmAcpiTable.h"

#include <Guid/ZeroGuid.h>
#include <IndustryStandard/Acpi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrmContextBufferLib.h>
#include <Library/PrmModuleDiscoveryLib.h>
#include <Library/PrmPeCoffLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/PrmConfig.h>

#include <PrmContextBuffer.h>
#include <PrmMmio.h>

#define _DBGMSGID_  "[PRMLOADER]"

UINTN  mPrmHandlerCount;
UINTN  mPrmModuleCount;

/**
  Processes a list of PRM context entries to build a PRM ACPI table.

  The ACPI table buffer is allocated and the table structure is built inside this function.

  @param[out]  PrmAcpiDescriptionTable    A pointer to a pointer to a buffer that is allocated within this function
                                          and will contain the PRM ACPI table. In case of an error in this function,
                                          *PrmAcpiDescriptorTable will be NULL.

  @retval EFI_SUCCESS                     All PRM Modules were processed to construct the PRM ACPI table successfully.
  @retval EFI_INVALID_PARAMETER           THe parameter PrmAcpiDescriptionTable is NULL.
  @retval EFI_OUT_OF_RESOURCES            Insufficient memory resources to allocate the PRM ACPI table boot services
                                          memory data buffer.

**/
EFI_STATUS
ProcessPrmModules (
  OUT PRM_ACPI_DESCRIPTION_TABLE  **PrmAcpiDescriptionTable
  )
{
  EFI_IMAGE_EXPORT_DIRECTORY           *CurrentImageExportDirectory;
  PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT  *CurrentExportDescriptorStruct;
  PRM_ACPI_DESCRIPTION_TABLE           *PrmAcpiTable;
  PRM_MODULE_IMAGE_CONTEXT             *CurrentPrmModuleImageContext;
  CONST CHAR8                          *CurrentExportDescriptorHandlerName;

  ACPI_PARAMETER_BUFFER_DESCRIPTOR  *CurrentModuleAcpiParamDescriptors;
  PRM_CONTEXT_BUFFER                *CurrentContextBuffer;
  PRM_MODULE_CONTEXT_BUFFERS        *CurrentModuleContextBuffers;
  PRM_MODULE_INFORMATION_STRUCT     *CurrentModuleInfoStruct;
  PRM_HANDLER_INFORMATION_STRUCT    *CurrentHandlerInfoStruct;

  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  CurrentImageAddress;
  UINTN                 AcpiParamIndex;
  UINTN                 HandlerIndex;
  UINT32                PrmAcpiDescriptionTableBufferSize;

  UINT64  HandlerPhysicalAddress;

  DEBUG ((DEBUG_INFO, "%a %a - Entry.\n", _DBGMSGID_, __func__));

  if (PrmAcpiDescriptionTable == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *PrmAcpiDescriptionTable = NULL;

  //
  // The platform DSC GUID must be set to a non-zero value
  //
  if (CompareGuid (&gEdkiiDscPlatformGuid, &gZeroGuid)) {
    DEBUG ((
      DEBUG_ERROR,
      "  %a %a: The Platform GUID in the DSC file must be set to a unique non-zero value.\n",
      _DBGMSGID_,
      __func__
      ));
    ASSERT (!CompareGuid (&gEdkiiDscPlatformGuid, &gZeroGuid));
  }

  DEBUG ((DEBUG_INFO, "  %a %a: %d total PRM modules to process.\n", _DBGMSGID_, __func__, mPrmModuleCount));
  DEBUG ((DEBUG_INFO, "  %a %a: %d total PRM handlers to process.\n", _DBGMSGID_, __func__, mPrmHandlerCount));

  PrmAcpiDescriptionTableBufferSize = (UINT32)(OFFSET_OF (PRM_ACPI_DESCRIPTION_TABLE, PrmModuleInfoStructure) +
                                               (OFFSET_OF (PRM_MODULE_INFORMATION_STRUCT, HandlerInfoStructure) *  mPrmModuleCount) +
                                               (sizeof (PRM_HANDLER_INFORMATION_STRUCT) * mPrmHandlerCount)
                                               );
  DEBUG ((DEBUG_INFO, "  %a %a: Total PRM ACPI table size: 0x%x.\n", _DBGMSGID_, __func__, PrmAcpiDescriptionTableBufferSize));

  PrmAcpiTable = AllocateZeroPool ((UINTN)PrmAcpiDescriptionTableBufferSize);
  if (PrmAcpiTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PrmAcpiTable->Header.Signature = PRM_TABLE_SIGNATURE;
  PrmAcpiTable->Header.Length    = PrmAcpiDescriptionTableBufferSize;
  PrmAcpiTable->Header.Revision  = PRM_TABLE_REVISION;
  PrmAcpiTable->Header.Checksum  = 0x0;
  CopyMem (&PrmAcpiTable->Header.OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (PrmAcpiTable->Header.OemId));
  PrmAcpiTable->Header.OemTableId      = PcdGet64 (PcdAcpiDefaultOemTableId);
  PrmAcpiTable->Header.OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
  PrmAcpiTable->Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  PrmAcpiTable->Header.CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);
  CopyGuid (&PrmAcpiTable->PrmPlatformGuid, &gEdkiiDscPlatformGuid);
  PrmAcpiTable->PrmModuleInfoOffset = OFFSET_OF (PRM_ACPI_DESCRIPTION_TABLE, PrmModuleInfoStructure);
  PrmAcpiTable->PrmModuleInfoCount  = (UINT32)mPrmModuleCount;

  //
  // Iterate across all PRM Modules on the list
  //
  CurrentModuleInfoStruct = &PrmAcpiTable->PrmModuleInfoStructure[0];
  for (
       CurrentPrmModuleImageContext = NULL, Status = GetNextPrmModuleEntry (&CurrentPrmModuleImageContext);
       !EFI_ERROR (Status);
       Status = GetNextPrmModuleEntry (&CurrentPrmModuleImageContext))
  {
    CurrentImageAddress               = CurrentPrmModuleImageContext->PeCoffImageContext.ImageAddress;
    CurrentImageExportDirectory       = CurrentPrmModuleImageContext->ExportDirectory;
    CurrentExportDescriptorStruct     = CurrentPrmModuleImageContext->ExportDescriptor;
    CurrentModuleAcpiParamDescriptors = NULL;

    DEBUG ((
      DEBUG_INFO,
      "  %a %a: PRM Module - %a with %d handlers.\n",
      _DBGMSGID_,
      __func__,
      (CHAR8 *)((UINTN)CurrentImageAddress + CurrentImageExportDirectory->Name),
      CurrentExportDescriptorStruct->Header.NumberPrmHandlers
      ));

    CurrentModuleInfoStruct->StructureRevision = PRM_MODULE_INFORMATION_STRUCT_REVISION;
    CurrentModuleInfoStruct->StructureLength   = (
                                                  OFFSET_OF (PRM_MODULE_INFORMATION_STRUCT, HandlerInfoStructure) +
                                                  (CurrentExportDescriptorStruct->Header.NumberPrmHandlers * sizeof (PRM_HANDLER_INFORMATION_STRUCT))
                                                  );
    CopyGuid (&CurrentModuleInfoStruct->Identifier, &CurrentExportDescriptorStruct->Header.ModuleGuid);
    CurrentModuleInfoStruct->HandlerCount      = (UINT32)CurrentExportDescriptorStruct->Header.NumberPrmHandlers;
    CurrentModuleInfoStruct->HandlerInfoOffset = OFFSET_OF (PRM_MODULE_INFORMATION_STRUCT, HandlerInfoStructure);

    CurrentModuleInfoStruct->MajorRevision = 0;
    CurrentModuleInfoStruct->MinorRevision = 0;
    Status                                 =  GetImageVersionInPeCoffImage (
                                                (VOID *)(UINTN)CurrentImageAddress,
                                                &CurrentPrmModuleImageContext->PeCoffImageContext,
                                                &CurrentModuleInfoStruct->MajorRevision,
                                                &CurrentModuleInfoStruct->MinorRevision
                                                );
    ASSERT_EFI_ERROR (Status);

    // It is currently valid for a PRM module not to use a context buffer
    Status = GetModuleContextBuffers (
               ByModuleGuid,
               &CurrentModuleInfoStruct->Identifier,
               (CONST PRM_MODULE_CONTEXT_BUFFERS **)&CurrentModuleContextBuffers
               );
    ASSERT (!EFI_ERROR (Status) || Status == EFI_NOT_FOUND);
    if (!EFI_ERROR (Status) && (CurrentModuleContextBuffers != NULL)) {
      CurrentModuleInfoStruct->RuntimeMmioRanges = (UINT64)(UINTN)CurrentModuleContextBuffers->RuntimeMmioRanges;
      CurrentModuleAcpiParamDescriptors          = CurrentModuleContextBuffers->AcpiParameterBufferDescriptors;
    }

    //
    // Iterate across all PRM handlers in the PRM Module
    //
    for (HandlerIndex = 0; HandlerIndex < CurrentExportDescriptorStruct->Header.NumberPrmHandlers; HandlerIndex++) {
      CurrentHandlerInfoStruct = &(CurrentModuleInfoStruct->HandlerInfoStructure[HandlerIndex]);

      CurrentHandlerInfoStruct->StructureRevision = PRM_HANDLER_INFORMATION_STRUCT_REVISION;
      CurrentHandlerInfoStruct->StructureLength   = sizeof (PRM_HANDLER_INFORMATION_STRUCT);
      CopyGuid (
        &CurrentHandlerInfoStruct->Identifier,
        &CurrentExportDescriptorStruct->PrmHandlerExportDescriptors[HandlerIndex].PrmHandlerGuid
        );

      CurrentExportDescriptorHandlerName = (CONST CHAR8 *)CurrentExportDescriptorStruct->PrmHandlerExportDescriptors[HandlerIndex].PrmHandlerName;

      Status =  GetContextBuffer (
                  &CurrentHandlerInfoStruct->Identifier,
                  CurrentModuleContextBuffers,
                  (CONST PRM_CONTEXT_BUFFER **)&CurrentContextBuffer
                  );
      if (!EFI_ERROR (Status)) {
        CurrentHandlerInfoStruct->StaticDataBuffer = (UINT64)(UINTN)CurrentContextBuffer->StaticDataBuffer;
      }

      Status =  GetExportEntryAddress (
                  CurrentExportDescriptorHandlerName,
                  CurrentImageAddress,
                  CurrentImageExportDirectory,
                  &HandlerPhysicalAddress
                  );
      ASSERT_EFI_ERROR (Status);
      if (!EFI_ERROR (Status)) {
        CurrentHandlerInfoStruct->PhysicalAddress = HandlerPhysicalAddress;
        DEBUG ((
          DEBUG_INFO,
          "    %a %a: Found %a handler physical address at 0x%016llx.\n",
          _DBGMSGID_,
          __func__,
          CurrentExportDescriptorHandlerName,
          CurrentHandlerInfoStruct->PhysicalAddress
          ));
      }

      //
      // Update the handler ACPI parameter buffer address if applicable
      //
      if (CurrentModuleAcpiParamDescriptors != NULL) {
        for (AcpiParamIndex = 0; AcpiParamIndex < CurrentModuleContextBuffers->AcpiParameterBufferDescriptorCount; AcpiParamIndex++) {
          if (CompareGuid (&CurrentModuleAcpiParamDescriptors[AcpiParamIndex].HandlerGuid, &CurrentHandlerInfoStruct->Identifier)) {
            CurrentHandlerInfoStruct->AcpiParameterBuffer = (UINT64)(UINTN)(
                                                                            CurrentModuleAcpiParamDescriptors[AcpiParamIndex].AcpiParameterBufferAddress
                                                                            );
          }
        }
      }
    }

    CurrentModuleInfoStruct = (PRM_MODULE_INFORMATION_STRUCT *)((UINTN)CurrentModuleInfoStruct + CurrentModuleInfoStruct->StructureLength);
  }

  *PrmAcpiDescriptionTable = PrmAcpiTable;

  return EFI_SUCCESS;
}

/**
  Publishes the PRM ACPI table (PRMT).

  @param[in]  PrmAcpiDescriptionTable     A pointer to a buffer with a completely populated and valid PRM
                                          ACPI description table.

  @retval EFI_SUCCESS                     The PRM ACPI was installed successfully.
  @retval EFI_INVALID_PARAMETER           THe parameter PrmAcpiDescriptionTable is NULL or the table signature
                                          in the table provided is invalid.
  @retval EFI_NOT_FOUND                   The protocol gEfiAcpiTableProtocolGuid could not be found.
  @retval EFI_OUT_OF_RESOURCES            Insufficient memory resources to allocate the PRM ACPI table buffer.

**/
EFI_STATUS
PublishPrmAcpiTable (
  IN  PRM_ACPI_DESCRIPTION_TABLE  *PrmAcpiDescriptionTable
  )
{
  EFI_STATUS               Status;
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTableProtocol;
  UINTN                    TableKey;

  if ((PrmAcpiDescriptionTable == NULL) || (PrmAcpiDescriptionTable->Header.Signature != PRM_TABLE_SIGNATURE)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTableProtocol);
  if (!EFI_ERROR (Status)) {
    TableKey = 0;
    //
    // Publish the PRM ACPI table. The table checksum will be computed during installation.
    //
    Status = AcpiTableProtocol->InstallAcpiTable (
                                  AcpiTableProtocol,
                                  PrmAcpiDescriptionTable,
                                  PrmAcpiDescriptionTable->Header.Length,
                                  &TableKey
                                  );
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "%a %a: The PRMT ACPI table was installed successfully.\n", _DBGMSGID_, __func__));
    }
  }

  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  The PRM Loader END_OF_DXE protocol notification event handler.

  All PRM Modules that are eligible for dispatch should have been loaded the DXE Dispatcher at the
  time of this function invocation.

  The main responsibilities of the PRM Loader are executed from this function which include 3 phases:
    1.) Disover PRM Modules - Find all PRM modules loaded during DXE dispatch and insert a PRM Module
        Context entry into a linked list to be handed off to phase 2.
    2.) Process PRM Modules - Build a GUID to PRM handler mapping for each module that is described in the
        PRM ACPI table so the OS can resolve a PRM Handler GUID to the corresponding PRM Handler physical address.
    3.) Publish PRM ACPI Table - Publish the PRM ACPI table with the information gathered in the phase 2.

  @param[in]  Event                       Event whose notification function is being invoked.
  @param[in]  Context                     The pointer to the notification function's context,
                                          which is implementation-dependent.

**/
VOID
EFIAPI
PrmLoaderEndOfDxeNotification (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS                  Status;
  PRM_ACPI_DESCRIPTION_TABLE  *PrmAcpiDescriptionTable;

  DEBUG ((DEBUG_INFO, "%a %a - Entry.\n", _DBGMSGID_, __func__));

  Status = DiscoverPrmModules (&mPrmModuleCount, &mPrmHandlerCount);
  ASSERT_EFI_ERROR (Status);

  Status = ProcessPrmModules (&PrmAcpiDescriptionTable);
  ASSERT_EFI_ERROR (Status);

  Status = PublishPrmAcpiTable (PrmAcpiDescriptionTable);
  ASSERT_EFI_ERROR (Status);

  if (PrmAcpiDescriptionTable != NULL) {
    FreePool (PrmAcpiDescriptionTable);
  }

  gBS->CloseEvent (Event);
}

/**
  The entry point for this module.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Others         An error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
PrmLoaderEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   EndOfDxeEvent;

  DEBUG ((DEBUG_INFO, "%a %a - Entry.\n", _DBGMSGID_, __func__));

  //
  // Discover and process installed PRM modules at the End of DXE
  // The PRM ACPI table is published if one or PRM modules are discovered
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  PrmLoaderEndOfDxeNotification,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &EndOfDxeEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a %a: EndOfDxe callback registration failed! %r.\n", _DBGMSGID_, __func__, Status));
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}
