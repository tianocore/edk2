/** @file
  This module installs the ACPI Hardware Error Source Table (HEST).

  @par Glossary:
    - HEST - Hardware Error Source Table
    - RAS  - Reliability, Availability, and Serviceability
    - MPXY - Message Proxy extension in the RISC-V SBI specification

  Copyright (c) 2026, Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Uefi.h>

#include <IndustryStandard/Acpi.h>

#include <Protocol/AcpiTable.h>

#include <Guid/EventGroup.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/SafeIntLib.h>
#include <Library/BaseRiscVSbiLib.h>

#include <Library/DxeRiscvMpxy.h>
#include <Library/DxeRasAgentClient.h>

EFI_EVENT  mHestReadyToBootEvent;
UINTN      mHestTableKey = 0;

#define STATUS_BLOCK_SIZE  1024

#define HEST_TO_BASE_ERROR_STRUCTURE(_table)                            \
  (EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE *)    \
    ((UINT8 *)(_table) +                                                \
     sizeof (EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER));

//
// ACPI Hardware Error Source Table template
//
EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER  mHestTemplate = {
  {
    EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_SIGNATURE,
    sizeof (EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER),
    EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_REVISION, // Revision
    0x00,                                              // Checksum will be updated at runtime
    //
    // It is expected that these values will be updated at EntryPoint.
    //
    { 0x00 },   // OEM ID is a 6 bytes long field
    0x00,       // OEM Table ID(8 bytes long)
    0x00,       // OEM Revision
    0x00,       // Creator ID
    0x00,       // Creator Revision
  },
  0             // Number of error source
};

/**
  Close all currently opened RAS agent channels.

  @param[in] RasAgentChannelIds  Array of RAS agent channel IDs.
  @param[in] NumChannelIds       Number of valid channel IDs in RasAgentChannelIds.

  @return  This function does not return a value.
**/
STATIC
VOID
CloseRasChannels (
  IN UINT32  *RasAgentChannelIds,
  IN UINT32  NumChannelIds
  )
{
  UINT32  ChannelIndex;

  for (ChannelIndex = 0; ChannelIndex < NumChannelIds; ChannelIndex++) {
    RacCloseRasAgentChannel (RasAgentChannelIds[ChannelIndex]);
  }
}

/**
  Initialize RAS agent channels and compute total number of error sources.

  This initializes the RAS client library, discovers RAS agent channel IDs,
  opens each channel, and accumulates the global error source count into
  mHestTemplate.ErrorSourceCount.

  @param[out] RasAgentChannelIds  Buffer receiving discovered channel IDs.
  @param[out] NumChannelIds       Number of channel IDs discovered.

  @retval EFI_SUCCESS  Initialization and channel probing completed successfully.
  @retval Others       Error returned by RacInit(), RacGetRasAgentMpxyChannelId(),
                       RacOpenRasAgentChannel(), or RacGetNumberErrorSources().
**/
STATIC
EFI_STATUS
InitializeRasChannels (
  OUT UINT32  *RasAgentChannelIds,
  OUT UINT32  *NumChannelIds
  )
{
  EFI_STATUS  Status;
  UINT32      ChannelIndex;
  UINT32      NumSources;

  Status = RacInit ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = RacGetRasAgentMpxyChannelId (
             MAX_RAS_AGENTS,
             RasAgentChannelIds,
             NumChannelIds
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mHestTemplate.ErrorSourceCount = 0;
  for (ChannelIndex = 0; ChannelIndex < *NumChannelIds; ChannelIndex++) {
    Status = RacOpenRasAgentChannel (RasAgentChannelIds[ChannelIndex]);
    if (EFI_ERROR (Status)) {
      CloseRasChannels (RasAgentChannelIds, ChannelIndex);
      return Status;
    }

    Status = RacGetNumberErrorSources (RasAgentChannelIds[ChannelIndex], &NumSources);
    if (EFI_ERROR (Status)) {
      CloseRasChannels (RasAgentChannelIds, ChannelIndex + 1);
      return Status;
    }

    mHestTemplate.ErrorSourceCount += NumSources;
  }

  return EFI_SUCCESS;
}

/**
  Populate HEST generic hardware error source structures from RAS descriptors.

  For each discovered RAS channel, this function fetches source IDs and then
  source descriptors, copying each descriptor into the destination HEST array.

  @param[in]  RasAgentChannelIds    Array of RAS agent channel IDs.
  @param[in]  NumChannelIds         Number of valid entries in RasAgentChannelIds.
  @param[out] BaseErrSrcStructure   Destination buffer for GHESv2 structures.

  @retval EFI_SUCCESS  All descriptors were fetched and copied successfully.
  @retval Others       Error returned by RacGetErrorSourceIDList() or
                       RacGetErrorSourceDescriptor().
**/
STATIC
EFI_STATUS
PopulateErrorSourceStructures (
  IN  UINT32                                                          *RasAgentChannelIds,
  IN  UINT32                                                          NumChannelIds,
  OUT EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE  *BaseErrSrcStructure
  )
{
  EFI_STATUS                                                      Status;
  UINT32                                                          NumSources;
  UINT32                                                          *ErrSources;
  UINT32                                                          ErrDescSize;
  UINTN                                                           DescriptorType;
  VOID                                                            *ErrDesc;
  UINT32                                                          ChannelIndex;
  UINT32                                                          Index;
  EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE  *TmpErrSrc;

  TmpErrSrc = BaseErrSrcStructure;

  for (ChannelIndex = 0; ChannelIndex < NumChannelIds; ChannelIndex++) {
    Status = RacGetErrorSourceIDList (
               RasAgentChannelIds[ChannelIndex],
               &ErrSources,
               &NumSources
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    for (Index = 0; Index < NumSources; Index++) {
      Status = RacGetErrorSourceDescriptor (
                 RasAgentChannelIds[ChannelIndex],
                 ErrSources[Index],
                 &DescriptorType,
                 &ErrDesc,
                 &ErrDescSize
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      ASSERT (DescriptorType == DtGhesV2);
      CopyMem (TmpErrSrc, ErrDesc, ErrDescSize);
      TmpErrSrc++;
    }
  }

  return EFI_SUCCESS;
}

/**
  Finalize and install the HEST ACPI table.

  This function updates the HEST header fields (Length and Checksum) and
  publishes the table through EFI_ACPI_TABLE_PROTOCOL.

  @param[in] AcpiTableProtocol  Pointer to installed ACPI table protocol.
  @param[in] HestTable          Pointer to HEST buffer to install.
  @param[in] HestTableSize      Size in bytes of HestTable.

  @retval EFI_SUCCESS  The ACPI table was installed successfully.
  @retval Others       Error returned by InstallAcpiTable().
**/
STATIC
EFI_STATUS
InstallHestTable (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTableProtocol,
  IN VOID                     *HestTable,
  IN UINT32                   HestTableSize
  )
{
  EFI_STATUS                   Status;
  EFI_ACPI_DESCRIPTION_HEADER  *Header;

  Header           = &((EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER *)HestTable)->Header;
  Header->Length   = HestTableSize;
  Header->Checksum = 0;
  Header->Checksum = CalculateCheckSum8 ((UINT8 *)HestTable, HestTableSize);

  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                HestTable,
                                HestTableSize,
                                &mHestTableKey
                                );

  return Status;
}

/**
  Notify function for event group EFI_EVENT_GROUP_READY_TO_BOOT. This is used to
  install the Hardware Error Source Table.

  @param[in]  Event   The Event that is being processed.
  @param[in]  Context The Event Context.

**/
VOID
EFIAPI
HestReadyToBootEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                                                      Status;
  EFI_ACPI_TABLE_PROTOCOL                                         *AcpiTableProtocol;
  VOID                                                            *HestTable;
  UINTN                                                           HestPages;
  UINT32                                                          HestTableSize;
  UINT32                                                          RasAgentChannelIds[MAX_RAS_AGENTS];
  UINT32                                                          NumChannelIds;
  EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE  *BaseErrSrcStructure;

  NumChannelIds = 0;
  HestTable     = NULL;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  Status = InitializeRasChannels (RasAgentChannelIds, &NumChannelIds);
  if (EFI_ERROR (Status)) {
    return;
  }

  DEBUG ((DEBUG_INFO, "Number of error sources across all RAS agents: %u\n", mHestTemplate.ErrorSourceCount));

  HestTableSize = sizeof (mHestTemplate) +
                  sizeof (EFI_ACPI_6_6_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE)
                  * mHestTemplate.ErrorSourceCount;
  HestPages = EFI_SIZE_TO_PAGES (HestTableSize);
  HestTable = AllocateAlignedPages (HestPages, SIZE_4KB);

  if (HestTable == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  CopyMem (HestTable, &mHestTemplate, sizeof (mHestTemplate));
  BaseErrSrcStructure = HEST_TO_BASE_ERROR_STRUCTURE (HestTable);

  Status = PopulateErrorSourceStructures (
             RasAgentChannelIds,
             NumChannelIds,
             BaseErrSrcStructure
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = InstallHestTable (AcpiTableProtocol, HestTable, HestTableSize);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

Exit:
  if (HestTable != NULL) {
    FreeAlignedPages (HestTable, HestPages);
  }

  CloseRasChannels (RasAgentChannelIds, NumChannelIds);
}

/**
  The module Entry Point of the Hardware Error Source Table DXE driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
HardwareErrorSourceDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                   Status;
  EFI_ACPI_DESCRIPTION_HEADER  *Header;

  //
  // Update Header fields of HEST
  //
  Header = &mHestTemplate.Header;
  ZeroMem (Header->OemId, sizeof (Header->OemId));
  CopyMem (
    Header->OemId,
    PcdGetPtr (PcdAcpiDefaultOemId),
    MIN (PcdGetSize (PcdAcpiDefaultOemId), sizeof (Header->OemId))
    );

  Header->OemTableId      = PcdGet64 (PcdAcpiDefaultOemTableId);
  Header->OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
  Header->CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  Header->CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);

  //
  // Register notify function to install HEST on ReadyToBoot Event.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  HestReadyToBootEventNotify,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &mHestReadyToBootEvent
                  );

  ASSERT_EFI_ERROR (Status);

  return Status;
}
