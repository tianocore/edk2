/** @file

  Confidential Compute Dxe driver. This driver installs protocols that are
  generic over confidential compute techonology.

  Copyright (c) 2022, Google LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/MemEncryptTdxLib.h>
#include <Protocol/MemoryAccept.h>

STATIC BOOLEAN  mAcceptAllMemoryAtEBS = TRUE;

STATIC EFI_EVENT  mAcceptAllMemoryEvent = NULL;

STATIC
EFI_STATUS
AcceptAllMemory (
  IN EDKII_MEMORY_ACCEPT_PROTOCOL  *AcceptMemory
  )
{
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *AllDescMap;
  UINTN                            NumEntries;
  UINTN                            Index;
  EFI_STATUS                       Status;

  DEBUG ((DEBUG_INFO, "Accepting all memory\n"));

  /*
   * Get a copy of the memory space map to iterate over while
   * changing the map.
   */
  Status = gDS->GetMemorySpaceMap (&NumEntries, &AllDescMap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < NumEntries; Index++) {
    CONST EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *Desc;

    Desc = &AllDescMap[Index];
    if (Desc->GcdMemoryType != EfiGcdMemoryTypeUnaccepted) {
      continue;
    }

    Status = AcceptMemory->AcceptMemory (
                             AcceptMemory,
                             Desc->BaseAddress,
                             Desc->Length
                             );
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = gDS->RemoveMemorySpace (Desc->BaseAddress, Desc->Length);
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeSystemMemory,
                    Desc->BaseAddress,
                    Desc->Length,
                    EFI_MEMORY_CPU_CRYPTO | EFI_MEMORY_XP | EFI_MEMORY_RO | EFI_MEMORY_RP
                    );
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  gBS->FreePool (AllDescMap);
  return Status;
}

VOID
EFIAPI
ResolveUnacceptedMemory (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EDKII_MEMORY_ACCEPT_PROTOCOL  *AcceptMemory;
  EFI_STATUS                    Status;

  if (!mAcceptAllMemoryAtEBS) {
    return;
  }

  Status = gBS->LocateProtocol (
                  &gEdkiiMemoryAcceptProtocolGuid,
                  NULL,
                  (VOID **)&AcceptMemory
                  );
  if (Status == EFI_NOT_FOUND) {
    return;
  }

  ASSERT_EFI_ERROR (Status);

  Status = AcceptAllMemory (AcceptMemory);
  ASSERT_EFI_ERROR (Status);
}

EFI_STATUS
EFIAPI
CocoDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Do nothing when confidential compute technologies that require memory
  // acceptance are not enabled.
  //
  if (!MemEncryptSevSnpIsEnabled () &&
      !MemEncryptTdxIsEnabled ())
  {
    return EFI_UNSUPPORTED;
  }

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  ResolveUnacceptedMemory,
                  NULL,
                  &gEfiEventBeforeExitBootServicesGuid,
                  &mAcceptAllMemoryEvent
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AllowUnacceptedMemory event creation for EventBeforeExitBootServices failed.\n"));
  }

  return EFI_SUCCESS;
}
