/** @file

  The PRM Module Discovery library provides functionality to discover PRM modules installed by platform firmware.

  Copyright (c) Microsoft Corporation
  Copyright (c) 2020 - 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>
#include <Protocol/MmAccess.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrmModuleDiscoveryLib.h>
#include <Library/PrmPeCoffLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/LoadedImage.h>

#include "PrmModuleDiscovery.h"

#define _DBGMSGID_  "[PRMMODULEDISCOVERYLIB]"

LIST_ENTRY  mPrmModuleList;

/**
  Gets the next PRM module discovered after the given PRM module.

  @param[in,out]  ModuleImageContext      A pointer to a pointer to a PRM module image context structure.
                                          ModuleImageContext should point to a pointer that points to NULL to
                                          get the first PRM module discovered.

  @retval EFI_SUCCESS                     The next PRM module was found successfully.
  @retval EFI_INVALID_PARAMETER           The given ModuleImageContext structure is invalid or the pointer is NULL.
  @retval EFI_NOT_FOUND                   The next PRM module was not found.

**/
EFI_STATUS
EFIAPI
GetNextPrmModuleEntry (
  IN OUT  PRM_MODULE_IMAGE_CONTEXT  **ModuleImageContext
  )
{
  LIST_ENTRY                           *CurrentLink;
  LIST_ENTRY                           *ForwardLink;
  PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY  *CurrentListEntry;
  PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY  *ForwardListEntry;

  DEBUG ((DEBUG_INFO, "%a %a - Entry.\n", _DBGMSGID_, __FUNCTION__));

  if (ModuleImageContext == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (*ModuleImageContext == NULL) {
    ForwardLink = GetFirstNode (&mPrmModuleList);
  } else {
    CurrentListEntry = NULL;
    CurrentListEntry = CR (*ModuleImageContext, PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY, Context, PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY_SIGNATURE);
    if ((CurrentListEntry == NULL) || (CurrentListEntry->Signature != PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY_SIGNATURE)) {
      return EFI_INVALID_PARAMETER;
    }

    CurrentLink = &CurrentListEntry->Link;
    ForwardLink = GetNextNode (&mPrmModuleList, CurrentLink);

    if (ForwardLink == &mPrmModuleList) {
      return EFI_NOT_FOUND;
    }
  }

  ForwardListEntry = BASE_CR (ForwardLink, PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY, Link);
  if (ForwardListEntry->Signature == PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY_SIGNATURE) {
    *ModuleImageContext = &ForwardListEntry->Context;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Creates a new PRM Module Image Context linked list entry.

  @retval PrmModuleImageContextListEntry  If successful, a pointer a PRM Module Image Context linked list entry
                                          otherwise, NULL is returned.

**/
PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY *
CreateNewPrmModuleImageContextListEntry (
  VOID
  )
{
  PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY  *PrmModuleImageContextListEntry;

  DEBUG ((DEBUG_INFO, "%a %a - Entry.\n", _DBGMSGID_, __FUNCTION__));

  PrmModuleImageContextListEntry = AllocateZeroPool (sizeof (*PrmModuleImageContextListEntry));
  if (PrmModuleImageContextListEntry == NULL) {
    return NULL;
  }

  DEBUG ((
    DEBUG_INFO,
    "  %a %a: Allocated PrmModuleImageContextListEntry at 0x%x of size 0x%x bytes.\n",
    _DBGMSGID_,
    __FUNCTION__,
    (UINTN)PrmModuleImageContextListEntry,
    sizeof (*PrmModuleImageContextListEntry)
    ));

  PrmModuleImageContextListEntry->Signature = PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY_SIGNATURE;

  return PrmModuleImageContextListEntry;
}

/**
  Check whether the address is within any of the MMRAM ranges.

  @param[in]  Address           The address to be checked.
  @param[in]  MmramRanges       Pointer to MMRAM descriptor.
  @param[in]  MmramRangeCount   MMRAM range count.

  @retval     TRUE     The address is in MMRAM ranges.
  @retval     FALSE    The address is out of MMRAM ranges.
**/
BOOLEAN
EFIAPI
IsAddressInMmram (
  IN EFI_PHYSICAL_ADDRESS  Address,
  IN EFI_MMRAM_DESCRIPTOR  *MmramRanges,
  IN UINTN                 MmramRangeCount
  )
{
  UINTN  Index;

  for (Index = 0; Index < MmramRangeCount; Index++) {
    if ((Address >= MmramRanges[Index].CpuStart) &&
        (Address < (MmramRanges[Index].CpuStart + MmramRanges[Index].PhysicalSize)))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Discovers all PRM Modules loaded during boot.

  Each PRM Module discovered is placed into a linked list so the list can br processsed in the future.

  @param[out]   ModuleCount               An optional pointer parameter that, if provided, is set to the number
                                          of PRM modules discovered.
  @param[out]   HandlerCount              An optional pointer parameter that, if provided, is set to the number
                                          of PRM handlers discovered.

  @retval EFI_SUCCESS                     All PRM Modules were discovered successfully.
  @retval EFI_INVALID_PARAMETER           An actual pointer parameter was passed as NULL.
  @retval EFI_NOT_FOUND                   The gEfiLoadedImageProtocolGuid protocol could not be found.
  @retval EFI_OUT_OF_RESOURCES            Insufficient memory resources to allocate the new PRM Context
                                          linked list nodes.
  @retval EFI_ALREADY_STARTED             The function was called previously and already discovered the PRM modules
                                          loaded on this boot.

**/
EFI_STATUS
EFIAPI
DiscoverPrmModules (
  OUT UINTN  *ModuleCount    OPTIONAL,
  OUT UINTN  *HandlerCount   OPTIONAL
  )
{
  EFI_STATUS                           Status;
  PRM_MODULE_IMAGE_CONTEXT             TempPrmModuleImageContext;
  PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY  *PrmModuleImageContextListEntry;
  EFI_LOADED_IMAGE_PROTOCOL            *LoadedImageProtocol;
  EFI_HANDLE                           *HandleBuffer;
  UINTN                                HandleCount;
  UINTN                                Index;
  UINTN                                PrmHandlerCount;
  UINTN                                PrmModuleCount;
  EFI_MM_ACCESS_PROTOCOL               *MmAccess;
  UINTN                                Size;
  EFI_MMRAM_DESCRIPTOR                 *MmramRanges;
  UINTN                                MmramRangeCount;

  DEBUG ((DEBUG_INFO, "%a %a - Entry.\n", _DBGMSGID_, __FUNCTION__));

  PrmHandlerCount = 0;
  PrmModuleCount  = 0;

  if (!IsListEmpty (&mPrmModuleList)) {
    return EFI_ALREADY_STARTED;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiLoadedImageProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status) && (HandleCount == 0)) {
    DEBUG ((DEBUG_ERROR, "%a %a: No LoadedImageProtocol instances found!\n", _DBGMSGID_, __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  MmramRanges     = NULL;
  MmramRangeCount = 0;
  Status          = gBS->LocateProtocol (
                           &gEfiMmAccessProtocolGuid,
                           NULL,
                           (VOID **)&MmAccess
                           );
  if (Status == EFI_SUCCESS) {
    //
    // Get MMRAM range information
    //
    Size   = 0;
    Status = MmAccess->GetCapabilities (MmAccess, &Size, NULL);
    if ((Status == EFI_BUFFER_TOO_SMALL) && (Size != 0)) {
      MmramRanges = (EFI_MMRAM_DESCRIPTOR *)AllocatePool (Size);
      if (MmramRanges != NULL) {
        Status = MmAccess->GetCapabilities (MmAccess, &Size, MmramRanges);
        if (Status == EFI_SUCCESS) {
          MmramRangeCount = Size / sizeof (EFI_MMRAM_DESCRIPTOR);
        }
      }
    }
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **)&LoadedImageProtocol
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (IsAddressInMmram ((EFI_PHYSICAL_ADDRESS)(UINTN)(LoadedImageProtocol->ImageBase), MmramRanges, MmramRangeCount)) {
      continue;
    }

    ZeroMem (&TempPrmModuleImageContext, sizeof (TempPrmModuleImageContext));
    TempPrmModuleImageContext.PeCoffImageContext.Handle    = LoadedImageProtocol->ImageBase;
    TempPrmModuleImageContext.PeCoffImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;

    Status = PeCoffLoaderGetImageInfo (&TempPrmModuleImageContext.PeCoffImageContext);
    if (EFI_ERROR (Status) || (TempPrmModuleImageContext.PeCoffImageContext.ImageError != IMAGE_ERROR_SUCCESS)) {
      DEBUG ((
        DEBUG_WARN,
        "%a %a: ImageHandle 0x%016lx is not a valid PE/COFF image. It cannot be considered a PRM module.\n",
        _DBGMSGID_,
        __FUNCTION__,
        (EFI_PHYSICAL_ADDRESS)(UINTN)LoadedImageProtocol->ImageBase
        ));
      continue;
    }

    if (TempPrmModuleImageContext.PeCoffImageContext.IsTeImage) {
      // A PRM Module is not allowed to be a TE image
      continue;
    }

    // Attempt to find an export table in this image
    Status =  GetExportDirectoryInPeCoffImage (
                LoadedImageProtocol->ImageBase,
                &TempPrmModuleImageContext.PeCoffImageContext,
                &TempPrmModuleImageContext.ExportDirectory
                );
    if (EFI_ERROR (Status)) {
      continue;
    }

    // Attempt to find the PRM Module Export Descriptor in the export table
    Status = GetPrmModuleExportDescriptorTable (
               TempPrmModuleImageContext.ExportDirectory,
               &TempPrmModuleImageContext.PeCoffImageContext,
               &TempPrmModuleImageContext.ExportDescriptor
               );
    if (EFI_ERROR (Status) || (TempPrmModuleImageContext.ExportDescriptor == NULL)) {
      continue;
    }

    // A PRM Module Export Descriptor was successfully found, this is considered a PRM Module.

    //
    // Create a new PRM Module image context node
    //
    PrmModuleImageContextListEntry = CreateNewPrmModuleImageContextListEntry ();
    if (PrmModuleImageContextListEntry == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (
      &PrmModuleImageContextListEntry->Context,
      &TempPrmModuleImageContext,
      sizeof (PrmModuleImageContextListEntry->Context)
      );
    InsertTailList (&mPrmModuleList, &PrmModuleImageContextListEntry->Link);
    PrmHandlerCount += TempPrmModuleImageContext.ExportDescriptor->Header.NumberPrmHandlers;
    PrmModuleCount++;
    DEBUG ((DEBUG_INFO, "%a %a: New PRM Module inserted into list to be processed.\n", _DBGMSGID_, __FUNCTION__));
  }

  if (HandlerCount != NULL) {
    *HandlerCount = PrmHandlerCount;
  }

  if (ModuleCount != NULL) {
    *ModuleCount = PrmModuleCount;
  }

  if (MmramRanges != NULL) {
    FreePool (MmramRanges);
  }

  return EFI_SUCCESS;
}

/**
  The destructor function for this library instance.

  Frees global resources allocated by this library instance.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The destructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
PrmModuleDiscoveryLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  LIST_ENTRY                           *Link;
  LIST_ENTRY                           *NextLink;
  PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY  *ListEntry;

  if (IsListEmpty (&mPrmModuleList)) {
    return EFI_SUCCESS;
  }

  Link = GetFirstNode (&mPrmModuleList);
  while (!IsNull (&mPrmModuleList, Link)) {
    ListEntry = CR (Link, PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY, Link, PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY_SIGNATURE);
    NextLink  = GetNextNode (&mPrmModuleList, Link);

    RemoveEntryList (Link);
    FreePool (ListEntry);

    Link = NextLink;
  }

  return EFI_SUCCESS;
}

/**
  The constructor function for this library instance.

  Internally initializes data structures used later during library execution.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
PrmModuleDiscoveryLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  InitializeListHead (&mPrmModuleList);

  return EFI_SUCCESS;
}
