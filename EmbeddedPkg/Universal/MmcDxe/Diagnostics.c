/** @file
  Diagnostics Protocol implementation for the MMC DXE driver

  Copyright (c) 2011-2014, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>

#include "Mmc.h"

#define DIAGNOSTIC_LOGBUFFER_MAXCHAR  1024

CHAR16* mLogBuffer = NULL;
UINTN   mLogRemainChar = 0;

CHAR16*
DiagnosticInitLog (
  UINTN MaxBufferChar
  )
{
  mLogRemainChar = MaxBufferChar;
  mLogBuffer = AllocatePool ((UINTN)MaxBufferChar * sizeof (CHAR16));
  return mLogBuffer;
}

UINTN
DiagnosticLog (
  CONST CHAR16* Str
  )
{
  UINTN len = StrLen (Str);
  if (len < mLogRemainChar) {
    StrCpyS (mLogBuffer, mLogRemainChar, Str);
    mLogRemainChar -= len;
    mLogBuffer += len;
    return len;
  } else {
    return 0;
  }
}

VOID
GenerateRandomBuffer (
  VOID* Buffer,
  UINTN BufferSize
  )
{
  UINT64  i;
  UINT64* Buffer64 = (UINT64*)Buffer;

  for (i = 0; i < (BufferSize >> 3); i++) {
    *Buffer64 = i | (~i << 32);
    Buffer64++;
  }
}

BOOLEAN
CompareBuffer (
  VOID  *BufferA,
  VOID  *BufferB,
  UINTN BufferSize
  )
{
  UINTN i;
  UINT64* BufferA64 = (UINT64*)BufferA;
  UINT64* BufferB64 = (UINT64*)BufferB;

  for (i = 0; i < (BufferSize >> 3); i++) {
    if (*BufferA64 != *BufferB64) {
      DEBUG ((EFI_D_ERROR, "CompareBuffer: Error at %i", i));
      DEBUG ((EFI_D_ERROR, "(0x%lX) != (0x%lX)\n", *BufferA64, *BufferB64));
      return FALSE;
    }
    BufferA64++;
    BufferB64++;
  }
  return TRUE;
}

EFI_STATUS
MmcReadWriteDataTest (
  MMC_HOST_INSTANCE *MmcHostInstance,
  EFI_LBA           Lba,
  UINTN             BufferSize
  )
{
  VOID                        *BackBuffer;
  VOID                        *WriteBuffer;
  VOID                        *ReadBuffer;
  EFI_STATUS                  Status;

  // Check if a Media is Present
  if (!MmcHostInstance->BlockIo.Media->MediaPresent) {
    DiagnosticLog (L"ERROR: No Media Present\n");
    return EFI_NO_MEDIA;
  }

  if (MmcHostInstance->State != MmcTransferState) {
    DiagnosticLog (L"ERROR: Not ready for Transfer state\n");
    return EFI_NOT_READY;
  }

  BackBuffer = AllocatePool (BufferSize);
  WriteBuffer = AllocatePool (BufferSize);
  ReadBuffer = AllocatePool (BufferSize);

  // Read (and save) buffer at a specific location
  Status = MmcReadBlocks (&(MmcHostInstance->BlockIo), MmcHostInstance->BlockIo.Media->MediaId,Lba,BufferSize,BackBuffer);
  if (Status != EFI_SUCCESS) {
    DiagnosticLog (L"ERROR: Fail to Read Block (1)\n");
    return Status;
  }

  // Write buffer at the same location
  GenerateRandomBuffer (WriteBuffer,BufferSize);
  Status = MmcWriteBlocks (&(MmcHostInstance->BlockIo), MmcHostInstance->BlockIo.Media->MediaId,Lba,BufferSize,WriteBuffer);
  if (Status != EFI_SUCCESS) {
    DiagnosticLog (L"ERROR: Fail to Write Block (1)\n");
    return Status;
  }

  // Read the buffer at the same location
  Status = MmcReadBlocks (&(MmcHostInstance->BlockIo), MmcHostInstance->BlockIo.Media->MediaId,Lba,BufferSize,ReadBuffer);
  if (Status != EFI_SUCCESS) {
    DiagnosticLog (L"ERROR: Fail to Read Block (2)\n");
    return Status;
  }

  // Check that is conform
  if (!CompareBuffer (ReadBuffer,WriteBuffer,BufferSize)) {
    DiagnosticLog (L"ERROR: Fail to Read/Write Block (1)\n");
    return EFI_INVALID_PARAMETER;
  }

  // Restore content at the original location
  Status = MmcWriteBlocks (&(MmcHostInstance->BlockIo), MmcHostInstance->BlockIo.Media->MediaId,Lba,BufferSize,BackBuffer);
  if (Status != EFI_SUCCESS) {
    DiagnosticLog (L"ERROR: Fail to Write Block (2)\n");
    return Status;
  }

  // Read the restored content
  Status = MmcReadBlocks (&(MmcHostInstance->BlockIo), MmcHostInstance->BlockIo.Media->MediaId,Lba,BufferSize,ReadBuffer);
  if (Status != EFI_SUCCESS) {
    DiagnosticLog (L"ERROR: Fail to Read Block (3)\n");
    return Status;
  }

  // Check the content is correct
  if (!CompareBuffer (ReadBuffer,BackBuffer,BufferSize)) {
    DiagnosticLog (L"ERROR: Fail to Read/Write Block (2)\n");
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MmcDriverDiagnosticsRunDiagnostics (
  IN  EFI_DRIVER_DIAGNOSTICS_PROTOCOL               *This,
  IN  EFI_HANDLE                                    ControllerHandle,
  IN  EFI_HANDLE                                    ChildHandle  OPTIONAL,
  IN  EFI_DRIVER_DIAGNOSTIC_TYPE                    DiagnosticType,
  IN  CHAR8                                         *Language,
  OUT EFI_GUID                                      **ErrorType,
  OUT UINTN                                         *BufferSize,
  OUT CHAR16                                        **Buffer
  )
{
  LIST_ENTRY              *CurrentLink;
  MMC_HOST_INSTANCE       *MmcHostInstance;
  EFI_STATUS              Status;

  if ((Language         == NULL) ||
      (ErrorType        == NULL) ||
      (Buffer           == NULL) ||
      (ControllerHandle == NULL) ||
      (BufferSize       == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Check Language is supported (i.e. is "en-*" - only English is supported)
  if (AsciiStrnCmp (Language, "en", 2) != 0) {
    return EFI_UNSUPPORTED;
  }

  Status = EFI_SUCCESS;
  *ErrorType  = NULL;
  *BufferSize = DIAGNOSTIC_LOGBUFFER_MAXCHAR;
  *Buffer = DiagnosticInitLog (DIAGNOSTIC_LOGBUFFER_MAXCHAR);

  DiagnosticLog (L"MMC Driver Diagnostics\n");

  // Find the MMC Host instance on which we have been asked to run diagnostics
  MmcHostInstance = NULL;
  CurrentLink = mMmcHostPool.ForwardLink;
  while (CurrentLink != NULL && CurrentLink != &mMmcHostPool && (Status == EFI_SUCCESS)) {
    MmcHostInstance = MMC_HOST_INSTANCE_FROM_LINK(CurrentLink);
    ASSERT(MmcHostInstance != NULL);
    if (MmcHostInstance->MmcHandle == ControllerHandle) {
      break;
    }
    CurrentLink = CurrentLink->ForwardLink;
  }

  // If we didn't find the controller, return EFI_UNSUPPORTED
  if ((MmcHostInstance == NULL)
      || (MmcHostInstance->MmcHandle != ControllerHandle)) {
    return EFI_UNSUPPORTED;
  }

  // LBA=1 Size=BlockSize
  DiagnosticLog (L"MMC Driver Diagnostics - Test: First Block\n");
  Status = MmcReadWriteDataTest (MmcHostInstance, 1, MmcHostInstance->BlockIo.Media->BlockSize);

  // LBA=2 Size=BlockSize
  DiagnosticLog (L"MMC Driver Diagnostics - Test: Second Block\n");
  Status = MmcReadWriteDataTest (MmcHostInstance, 2, MmcHostInstance->BlockIo.Media->BlockSize);

  // LBA=10 Size=BlockSize
  DiagnosticLog (L"MMC Driver Diagnostics - Test: Any Block\n");
  Status = MmcReadWriteDataTest (MmcHostInstance, MmcHostInstance->BlockIo.Media->LastBlock >> 1, MmcHostInstance->BlockIo.Media->BlockSize);

  // LBA=LastBlock Size=BlockSize
  DiagnosticLog (L"MMC Driver Diagnostics - Test: Last Block\n");
  Status = MmcReadWriteDataTest (MmcHostInstance, MmcHostInstance->BlockIo.Media->LastBlock, MmcHostInstance->BlockIo.Media->BlockSize);

  // LBA=1 Size=2*BlockSize
  DiagnosticLog (L"MMC Driver Diagnostics - Test: First Block / 2 BlockSSize\n");
  Status = MmcReadWriteDataTest (MmcHostInstance, 1, 2 * MmcHostInstance->BlockIo.Media->BlockSize);

  return Status;
}

//
// EFI Driver Diagnostics 2 Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_DRIVER_DIAGNOSTICS2_PROTOCOL gMmcDriverDiagnostics2 = {
  (EFI_DRIVER_DIAGNOSTICS2_RUN_DIAGNOSTICS) MmcDriverDiagnosticsRunDiagnostics,
  "en"
};
