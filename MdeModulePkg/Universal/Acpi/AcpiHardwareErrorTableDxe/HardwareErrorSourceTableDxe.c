/** @file
  This module installs ACPI Hardware Error Source Table (HEST)

  Copyright (c) 2024, Ventana Micro Systems, Inc.
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
UINTN      mHestTableKey          = 0;
BOOLEAN    mAcpiHestInstalled     = FALSE;
BOOLEAN    mAcpiHestStatusChanged = FALSE;
BOOLEAN    mAcpiHestBufferChanged = FALSE;

#define STATUS_BLOCK_SIZE  1024
#define MPXY_SHMEM_SIZE    4096

//
// ACPI Hardware Error Source Table template
//
EFI_ACPI_6_5_HARDWARE_ERROR_SOURCE_TABLE_HEADER  mHestTemplate = {
  {
    EFI_ACPI_6_5_HARDWARE_ERROR_SOURCE_TABLE_SIGNATURE,
    sizeof (EFI_ACPI_6_5_HARDWARE_ERROR_SOURCE_TABLE_HEADER),
    EFI_ACPI_6_5_HARDWARE_ERROR_SOURCE_TABLE_REVISION, // Revision
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
  EFI_ACPI_DESCRIPTION_HEADER                                     *Header;
  VOID                                                            *ErrDesc;
  UINT32                                                          NumSources, *ErrSources, ErrDescSize;
  INTN                                                            i;
  VOID                                                            *HestTable;
  UINT32                                                          HestPages, HestTableSize;
  UINTN                                                           DescriptorType;
  EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE  *BaseErrSrcStructure, *tESS;

  #define HEST_TO_BASE_ERROR_STRUCTURE(_table)                            \
  (EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE *)    \
    ((UINT8 *)_table +                                                  \
     sizeof(typeof(EFI_ACPI_6_5_HARDWARE_ERROR_SOURCE_TABLE_HEADER)));

  Header = &mHestTemplate.Header;

  //
  // Get ACPI Table protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Check if HEST is already installed.
  //
  if (mAcpiHestInstalled) {
    Status = AcpiTableProtocol->UninstallAcpiTable (
                                  AcpiTableProtocol,
                                  mHestTableKey
                                  );
    if (EFI_ERROR (Status)) {
      return;
    }
  }

  // Initialize the RAS agent client library.
  Status = RacInit ();
  if (EFI_ERROR (Status)) {
    return;
  }

  // Fetch the number of hardware error sources available
  Status = RacGetNumberErrorSources (&NumSources);
  if (EFI_ERROR (Status)) {
    return;
  }

  // Fetch the unique source ID for each error source.
  Status = RacGetErrorSourceIDList (&ErrSources, &NumSources);
  if (EFI_ERROR (Status)) {
    return;
  }

  mHestTemplate.ErrorSourceCount = NumSources;

  // Allocate memory for all the error source descriptors
  HestTableSize = sizeof (mHestTemplate) +
                  sizeof (EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE)
                  * NumSources;
  HestPages = EFI_SIZE_TO_PAGES (HestTableSize);
  HestTable = AllocateAlignedPages (HestPages, 4096);

  if (HestTable == NULL) {
    return;
  }

  CopyMem (HestTable, &mHestTemplate, sizeof (mHestTemplate));

  tESS = BaseErrSrcStructure = HEST_TO_BASE_ERROR_STRUCTURE (HestTable);

  for (i = 0; i < NumSources; i++) {
    Status = RacGetErrorSourceDescriptor (
               ErrSources[i],
               &DescriptorType,
               &ErrDesc,
               &ErrDescSize
               );
    if (EFI_ERROR (Status)) {
      return;
    }

    ASSERT (DescriptorType == DT_GHESV2);

    CopyMem (tESS, ErrDesc, ErrDescSize);
    tESS++;
  }

  Header         = &((typeof(mHestTemplate) *) HestTable)->Header;
  Header->Length = HestTableSize;
  //
  // Update Checksum in Hest Table
  //
  Header->Checksum = 0;
  Header->Checksum =
    CalculateCheckSum8 (
      (UINT8 *)&HestTable,
      HestTableSize
      );

  //
  // Publish Boot Graphics Resource Table.
  //
  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                HestTable,
                                HestTableSize,
                                &mHestTableKey
                                );
  if (EFI_ERROR (Status)) {
    return;
  }

  mAcpiHestInstalled = TRUE;
}

/**
  The module Entry Point of the Boot Graphics Resource Table DXE driver.

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

  WriteUnaligned64 (&Header->OemTableId, PcdGet64 (PcdAcpiDefaultOemTableId));
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
