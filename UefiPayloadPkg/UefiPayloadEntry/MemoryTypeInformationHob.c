/** @file

  Build the Memory Type Information HOB from the persisted variable store.

  Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"

#include <Guid/VariableFormat.h>
#include <Guid/SmmStoreInfoGuid.h>

#define SMMSTORE_RET_SUCCESS      0
#define SMMSTORE_RET_FAILURE      1
#define SMMSTORE_RET_UNSUPPORTED  2
#define SMMSTORE_CMD_RAW_READ     5

typedef struct {
  UINT32    BufSize;
  UINT32    BufOffset;
  UINT32    BlockId;
} SMM_STORE_PARAMS_READ;

typedef union {
  SMM_STORE_PARAMS_READ    Read;
} SMM_STORE_COM_BUF;

#if defined (MDE_CPU_X64)
UINTN
EFIAPI
TriggerSmi (
  IN UINTN  Cmd,
  IN UINTN  Arg,
  IN UINTN  Retry
  );

#endif

STATIC
BOOLEAN
ValidateMemoryTypeInfoVariable (
  IN EFI_MEMORY_TYPE_INFORMATION  *MemoryData,
  IN UINTN                        MemoryDataSize
  )
{
  UINTN  Count;
  UINTN  Index;

  if (MemoryData == NULL) {
    return FALSE;
  }

  Count = MemoryDataSize / sizeof (*MemoryData);
  if ((Count == 0) || (Count * sizeof (*MemoryData) != MemoryDataSize)) {
    return FALSE;
  }

  if (MemoryData[Count - 1].Type != EfiMaxMemoryType) {
    return FALSE;
  }

  for (Index = 0; Index < Count - 1; Index++) {
    if (MemoryData[Index].Type >= EfiMaxMemoryType) {
      return FALSE;
    }
  }

  return TRUE;
}

STATIC
VARIABLE_STORE_STATUS
GetVariableStoreStatus (
  IN VARIABLE_STORE_HEADER  *VariableStoreHeader
  )
{
  if ((CompareGuid (&VariableStoreHeader->Signature, &gEfiAuthenticatedVariableGuid) ||
       CompareGuid (&VariableStoreHeader->Signature, &gEfiVariableGuid)) &&
      (VariableStoreHeader->Format == VARIABLE_STORE_FORMATTED) &&
      (VariableStoreHeader->State == VARIABLE_STORE_HEALTHY))
  {
    return EfiValid;
  }

  return EfiInvalid;
}

STATIC
BOOLEAN
IsValidVariableHeader (
  IN VARIABLE_HEADER  *Variable,
  IN VARIABLE_HEADER  *VariableStoreEnd
  )
{
  return (BOOLEAN)(
                   (Variable != NULL) &&
                   (Variable < VariableStoreEnd) &&
                   (Variable->StartId == VARIABLE_DATA)
                   );
}

STATIC
UINTN
GetVariableHeaderSize (
  IN BOOLEAN  Authenticated
  )
{
  return Authenticated ? sizeof (AUTHENTICATED_VARIABLE_HEADER) : sizeof (VARIABLE_HEADER);
}

STATIC
UINTN
NameSizeOfVariable (
  IN VARIABLE_HEADER  *Variable,
  IN BOOLEAN          Authenticated
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthenticatedVariable;

  AuthenticatedVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (Authenticated) {
    if ((AuthenticatedVariable->State == (UINT8)-1) ||
        (AuthenticatedVariable->NameSize == MAX_UINT32) ||
        (AuthenticatedVariable->DataSize == MAX_UINT32) ||
        (AuthenticatedVariable->Attributes == MAX_UINT32))
    {
      return 0;
    }

    return AuthenticatedVariable->NameSize;
  }

  if ((Variable->State == (UINT8)-1) ||
      (Variable->NameSize == MAX_UINT32) ||
      (Variable->DataSize == MAX_UINT32) ||
      (Variable->Attributes == MAX_UINT32))
  {
    return 0;
  }

  return Variable->NameSize;
}

STATIC
UINTN
DataSizeOfVariable (
  IN VARIABLE_HEADER  *Variable,
  IN BOOLEAN          Authenticated
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthenticatedVariable;

  AuthenticatedVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (Authenticated) {
    if ((AuthenticatedVariable->State == (UINT8)-1) ||
        (AuthenticatedVariable->NameSize == MAX_UINT32) ||
        (AuthenticatedVariable->DataSize == MAX_UINT32) ||
        (AuthenticatedVariable->Attributes == MAX_UINT32))
    {
      return 0;
    }

    return AuthenticatedVariable->DataSize;
  }

  if ((Variable->State == (UINT8)-1) ||
      (Variable->NameSize == MAX_UINT32) ||
      (Variable->DataSize == MAX_UINT32) ||
      (Variable->Attributes == MAX_UINT32))
  {
    return 0;
  }

  return Variable->DataSize;
}

STATIC
CHAR16 *
GetVariableNamePtr (
  IN VARIABLE_HEADER  *Variable,
  IN BOOLEAN          Authenticated
  )
{
  return (CHAR16 *)((UINTN)Variable + GetVariableHeaderSize (Authenticated));
}

STATIC
EFI_GUID *
GetVendorGuidPtr (
  IN VARIABLE_HEADER  *Variable,
  IN BOOLEAN          Authenticated
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthenticatedVariable;

  AuthenticatedVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  return Authenticated ? &AuthenticatedVariable->VendorGuid : &Variable->VendorGuid;
}

STATIC
UINT8 *
GetVariableDataPtr (
  IN VARIABLE_HEADER  *Variable,
  IN BOOLEAN          Authenticated
  )
{
  UINTN  Offset;

  Offset  = (UINTN)GetVariableNamePtr (Variable, Authenticated);
  Offset += NameSizeOfVariable (Variable, Authenticated);
  Offset += GET_PAD_SIZE (NameSizeOfVariable (Variable, Authenticated));
  return (UINT8 *)Offset;
}

STATIC
VARIABLE_HEADER *
GetNextVariablePtr (
  IN VARIABLE_HEADER  *Variable,
  IN BOOLEAN          Authenticated
  )
{
  UINTN  Offset;

  Offset  = (UINTN)GetVariableDataPtr (Variable, Authenticated);
  Offset += DataSizeOfVariable (Variable, Authenticated);
  Offset += GET_PAD_SIZE (DataSizeOfVariable (Variable, Authenticated));
  return (VARIABLE_HEADER *)HEADER_ALIGN (Offset);
}

STATIC
VARIABLE_HEADER *
GetStartPointer (
  IN VARIABLE_STORE_HEADER  *VariableStoreHeader
  )
{
  return (VARIABLE_HEADER *)HEADER_ALIGN (VariableStoreHeader + 1);
}

STATIC
VARIABLE_HEADER *
GetEndPointer (
  IN VARIABLE_STORE_HEADER  *VariableStoreHeader
  )
{
  return (VARIABLE_HEADER *)HEADER_ALIGN ((UINTN)VariableStoreHeader + VariableStoreHeader->Size);
}

STATIC
BOOLEAN
FindVariableInStore (
  IN  VARIABLE_STORE_HEADER  *VariableStoreHeader,
  IN  CHAR16                 *VariableName,
  IN  EFI_GUID               *VendorGuid,
  OUT UINT8                  **VariableData,
  OUT UINTN                  *VariableDataSize
  )
{
  BOOLEAN          Authenticated;
  BOOLEAN          FoundAdded;
  UINTN            NameSize;
  VARIABLE_HEADER  *Candidate;
  VARIABLE_HEADER  *NextVariable;
  VARIABLE_HEADER  *Variable;
  VARIABLE_HEADER  *VariableStoreEnd;

  if ((VariableStoreHeader == NULL) ||
      (VariableName == NULL) ||
      (VendorGuid == NULL) ||
      (VariableData == NULL) ||
      (VariableDataSize == NULL))
  {
    return FALSE;
  }

  if (GetVariableStoreStatus (VariableStoreHeader) != EfiValid) {
    return FALSE;
  }

  Authenticated    = CompareGuid (&VariableStoreHeader->Signature, &gEfiAuthenticatedVariableGuid);
  NameSize         = StrSize (VariableName);
  Candidate        = NULL;
  FoundAdded       = FALSE;
  Variable         = GetStartPointer (VariableStoreHeader);
  VariableStoreEnd = GetEndPointer (VariableStoreHeader);

  while (IsValidVariableHeader (Variable, VariableStoreEnd)) {
    if ((NameSizeOfVariable (Variable, Authenticated) == NameSize) &&
        CompareGuid (GetVendorGuidPtr (Variable, Authenticated), VendorGuid) &&
        (CompareMem (GetVariableNamePtr (Variable, Authenticated), VariableName, NameSize) == 0))
    {
      if (Variable->State == VAR_ADDED) {
        Candidate  = Variable;
        FoundAdded = TRUE;
      } else if ((Variable->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) && !FoundAdded) {
        Candidate = Variable;
      }
    }

    NextVariable = GetNextVariablePtr (Variable, Authenticated);
    if ((NextVariable <= Variable) || (NextVariable > VariableStoreEnd)) {
      break;
    }

    Variable = NextVariable;
  }

  if (Candidate == NULL) {
    return FALSE;
  }

  *VariableData     = GetVariableDataPtr (Candidate, Authenticated);
  *VariableDataSize = DataSizeOfVariable (Candidate, Authenticated);
  return TRUE;
}

#if defined (MDE_CPU_X64)
STATIC
EFI_STATUS
SmmStoreRawRead (
  IN CONST SMMSTORE_INFO  *SmmStoreInfo,
  IN SMM_STORE_COM_BUF    *CommandBuffer
  )
{
  UINTN  Result;
  UINTN  Command;

  Command = ((UINTN)SMMSTORE_CMD_RAW_READ << 8) | SmmStoreInfo->ApmCmd;
  Result  = TriggerSmi (Command, (UINTN)CommandBuffer, 5);
  if (Result == Command) {
    return EFI_NO_RESPONSE;
  }

  if (Result == SMMSTORE_RET_SUCCESS) {
    return EFI_SUCCESS;
  }

  if (Result == SMMSTORE_RET_UNSUPPORTED) {
    return EFI_UNSUPPORTED;
  }

  return EFI_DEVICE_ERROR;
}

STATIC
EFI_STATUS
ReadSmmStoreBytes (
  IN     UINTN  Offset,
  IN OUT UINTN  *Size,
  OUT    VOID   *Buffer
  )
{
  UINTN              Block;
  UINTN              BlockOffset;
  UINT8              *ByteBuffer;
  UINTN              ChunkSize;
  SMM_STORE_COM_BUF  CommandBuffer;
  EFI_HOB_GUID_TYPE  *GuidHob;
  UINTN              RemainingSize;
  EFI_STATUS         Status;
  SMMSTORE_INFO      *SmmStoreInfo;
  UINTN              TotalReadSize;

  if ((Size == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  GuidHob = GetFirstGuidHob (&gEfiSmmStoreInfoHobGuid);
  if (GuidHob == NULL) {
    return EFI_NO_MEDIA;
  }

  SmmStoreInfo = (SMMSTORE_INFO *)GET_GUID_HOB_DATA (GuidHob);
  if ((SmmStoreInfo->BlockSize == 0) ||
      (SmmStoreInfo->NumBlocks == 0) ||
      (SmmStoreInfo->ComBufferSize == 0) ||
      (SmmStoreInfo->ComBuffer == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  RemainingSize = *Size;
  TotalReadSize = 0;
  ByteBuffer    = (UINT8 *)Buffer;

  while (RemainingSize > 0) {
    Block       = Offset / SmmStoreInfo->BlockSize;
    BlockOffset = Offset % SmmStoreInfo->BlockSize;
    if (Block >= SmmStoreInfo->NumBlocks) {
      *Size = TotalReadSize;
      return EFI_END_OF_MEDIA;
    }

    ChunkSize = MIN (RemainingSize, (UINTN)SmmStoreInfo->BlockSize - BlockOffset);
    if (BlockOffset >= SmmStoreInfo->ComBufferSize) {
      *Size = TotalReadSize;
      return EFI_BAD_BUFFER_SIZE;
    }

    ChunkSize = MIN (ChunkSize, (UINTN)SmmStoreInfo->ComBufferSize - BlockOffset);
    ZeroMem (&CommandBuffer, sizeof (CommandBuffer));
    CommandBuffer.Read.BufSize   = (UINT32)ChunkSize;
    CommandBuffer.Read.BufOffset = (UINT32)BlockOffset;
    CommandBuffer.Read.BlockId   = (UINT32)Block;

    Status = SmmStoreRawRead (SmmStoreInfo, &CommandBuffer);
    if (EFI_ERROR (Status)) {
      *Size = TotalReadSize;
      return Status;
    }

    CopyMem (ByteBuffer, (VOID *)(UINTN)(SmmStoreInfo->ComBuffer + BlockOffset), ChunkSize);

    ByteBuffer    += ChunkSize;
    Offset        += ChunkSize;
    RemainingSize -= ChunkSize;
    TotalReadSize += ChunkSize;
  }

  *Size = TotalReadSize;
  return EFI_SUCCESS;
}

#else
STATIC
EFI_STATUS
ReadSmmStoreBytes (
  IN     UINTN  Offset,
  IN OUT UINTN  *Size,
  OUT    VOID   *Buffer
  )
{
  return EFI_UNSUPPORTED;
}

#endif

VOID
BuildMemoryTypeInformationHob (
  IN EFI_MEMORY_TYPE_INFORMATION  *DefaultMemoryTypeInformation,
  IN UINTN                        DefaultMemoryTypeInformationSize
  )
{
  EFI_FIRMWARE_VOLUME_HEADER  FirmwareVolumeHeader;
  VARIABLE_STORE_HEADER       VariableStoreHeader;
  UINT8                       *VariableData;
  UINTN                       VariableDataSize;
  VARIABLE_STORE_HEADER       *VariableStore;
  UINTN                       VariableStoreSize;
  EFI_STATUS                  Status;

  if (GetFirstGuidHob (&gEfiMemoryTypeInformationGuid) != NULL) {
    return;
  }

  VariableDataSize = sizeof (FirmwareVolumeHeader);
  Status           = ReadSmmStoreBytes (0, &VariableDataSize, &FirmwareVolumeHeader);
  if (!EFI_ERROR (Status) &&
      (FirmwareVolumeHeader.Signature == EFI_FVH_SIGNATURE) &&
      (FirmwareVolumeHeader.HeaderLength >= sizeof (EFI_FIRMWARE_VOLUME_HEADER)))
  {
    VariableDataSize = sizeof (VariableStoreHeader);
    Status           = ReadSmmStoreBytes (
                         FirmwareVolumeHeader.HeaderLength,
                         &VariableDataSize,
                         &VariableStoreHeader
                         );
    if (!EFI_ERROR (Status) &&
        (GetVariableStoreStatus (&VariableStoreHeader) == EfiValid) &&
        (VariableStoreHeader.Size >= sizeof (VARIABLE_STORE_HEADER)) &&
        (VariableStoreHeader.Size <= SIZE_16MB))
    {
      VariableStoreSize = VariableStoreHeader.Size;
      VariableStore     = AllocatePool (VariableStoreSize);
      if (VariableStore != NULL) {
        Status = ReadSmmStoreBytes (
                   FirmwareVolumeHeader.HeaderLength,
                   &VariableStoreSize,
                   VariableStore
                   );
        if (!EFI_ERROR (Status) &&
            FindVariableInStore (
              VariableStore,
              EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME,
              &gEfiMemoryTypeInformationGuid,
              &VariableData,
              &VariableDataSize
              ) &&
            ValidateMemoryTypeInfoVariable ((EFI_MEMORY_TYPE_INFORMATION *)VariableData, VariableDataSize))
        {
          BuildGuidDataHob (
            &gEfiMemoryTypeInformationGuid,
            VariableData,
            VariableDataSize
            );
        }
      }
    }
  }

  if (GetFirstGuidHob (&gEfiMemoryTypeInformationGuid) == NULL) {
    BuildGuidDataHob (
      &gEfiMemoryTypeInformationGuid,
      DefaultMemoryTypeInformation,
      DefaultMemoryTypeInformationSize
      );
  }
}
