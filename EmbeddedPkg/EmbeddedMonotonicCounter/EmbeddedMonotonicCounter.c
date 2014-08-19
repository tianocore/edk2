/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/MonotonicCounter.h>

UINT64 gCurrentMonotonicCount = 0;

EFI_STATUS
EFIAPI
GetNextMonotonicCount (
  OUT UINT64  *Count
  )
{
  if (Count == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Count = gCurrentMonotonicCount++;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GetNextHighMonotonicCount (
  OUT UINT32  *HighCount
  )
{
  if (HighCount == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  gCurrentMonotonicCount += 0x0000000100000000ULL;

  *HighCount = (UINT32)RShiftU64 (gCurrentMonotonicCount, 32) & 0xFFFFFFFF;

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
MonotonicCounterDriverInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle = NULL;

  // Make sure the Monotonic Counter Architectural Protocol is not already installed in the system
  ASSERT_PROTOCOL_ALREADY_INSTALLED(NULL, &gEfiMonotonicCounterArchProtocolGuid);

  // Fill in the EFI Boot Services and EFI Runtime Services Monotonic Counter Fields
  gBS->GetNextMonotonicCount     = GetNextMonotonicCount;
  gRT->GetNextHighMonotonicCount = GetNextHighMonotonicCount;

  // Install the Monotonic Counter Architectural Protocol onto a new handle
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiMonotonicCounterArchProtocolGuid,    NULL,
                  NULL
                  );
  return Status;
}
