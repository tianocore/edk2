/** @file
  LockBoxLib implementation using an APMC-triggered SMI.

  UefiPayloadPkg does not include the SMM LockBox infrastructure that is used
  by typical EDK2 platforms. For coreboot payloads that need OPAL secrets on
  S3 resume, provide a minimal in-DXE LockBox store and forward OPAL device
  LockBox updates to a coreboot SMM handler via an APMC-triggered SMI.

  Copyright (c) 2026, Star Labs Systems. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/LockBoxLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define OPAL_MAX_PASSWORD_SIZE  32

#define OPAL_DEVICE_LOCKBOX_GUID \
  { 0x56a77f0d, 0x6f05, 0x4d47, { 0xb9, 0x11, 0x4f, 0x0d, 0xec, 0x5c, 0x58, 0x61 } }

#define OPAL_S3_APM_CNT_SVC            0xEE
#define OPAL_S3_SMM_SUBCMD_SET_SECRET  0x01

#define OPAL_S3_SMM_CTX_SIGNATURE  SIGNATURE_32 ('O', 'P', 'S', '3')
#define OPAL_S3_SMM_CTX_VERSION    0x0001

#pragma pack(push, 1)
typedef struct {
  UINT16    Segment;
  UINT8     Bus;
  UINT8     Device;
  UINT8     Function;
  UINT8     Reserved;
} OPAL_PCI_DEVICE;

typedef struct {
  UINT32             Length;
  OPAL_PCI_DEVICE    Device;
  UINT8              PasswordLength;
  UINT8              Password[OPAL_MAX_PASSWORD_SIZE];
  UINT16             OpalBaseComId;
  UINT32             DevicePathLength;
  UINT8              DevicePath[0];
} OPAL_DEVICE_LOCKBOX_DATA;

typedef struct {
  UINT32    Signature;
  UINT16    Version;
  UINT16    Size;

  UINT8     Bus;
  UINT8     Device;
  UINT8     Function;
  UINT8     Reserved0;

  UINT16    OpalBaseComId;
  UINT16    Reserved1;

  UINT8     PasswordLength;
  UINT8     Reserved2[3];
  UINT8     Password[OPAL_MAX_PASSWORD_SIZE];
} OPAL_S3_SMM_CTX;
#pragma pack(pop)

typedef struct LOCK_BOX_ENTRY {
  struct LOCK_BOX_ENTRY    *Next;
  GUID                     Guid;
  UINT64                   Attributes;
  UINTN                    Length;
  VOID                     *Data;
} LOCK_BOX_ENTRY;

STATIC LOCK_BOX_ENTRY  *mLockBoxList;
STATIC CONST GUID      mOpalDeviceLockBoxGuid = OPAL_DEVICE_LOCKBOX_GUID;

#if defined (MDE_CPU_X64)
//
// Platform SMM ABI: set RAX/RBX then outb to APMC (0xB2).
//

/**
  Trigger an SMI by writing to the APMC port.

  @param[in]  Cmd    APMC command value.
  @param[in]  Arg    Argument passed to the SMM handler.
  @param[in]  Retry  Number of retries if the SMI is not handled.

  @retval Command-dependent result from SMM.
**/
UINTN
EFIAPI
LockBoxTriggerSmi (
  IN UINTN  Cmd,
  IN UINTN  Arg,
  IN UINTN  Retry
  );

#else

/**
  Stub for non-X64 builds.

  @param[in]  Cmd    APMC command value.
  @param[in]  Arg    Argument passed to the SMM handler.
  @param[in]  Retry  Number of retries if the SMI is not handled.

  @retval Command-dependent result from SMM.
**/
STATIC
UINTN
EFIAPI
LockBoxTriggerSmi (
  IN UINTN  Cmd,
  IN UINTN  Arg,
  IN UINTN  Retry
  )
{
  return Cmd;
}

#endif

/**
  Find the LockBox entry with a matching GUID.

  @param[in]  Guid  LockBox entry GUID.

  @retval Pointer to matching entry, or NULL if not found.
**/
STATIC
LOCK_BOX_ENTRY *
FindLockBoxEntry (
  IN GUID  *Guid
  )
{
  LOCK_BOX_ENTRY  *Entry;

  for (Entry = mLockBoxList; Entry != NULL; Entry = Entry->Next) {
    if (CompareGuid (&Entry->Guid, Guid)) {
      return Entry;
    }
  }

  return NULL;
}

/**
  Send an OPAL S3 service request to coreboot SMM via APMC.

  @param[in]  Subcmd   Service subcommand.
  @param[in]  CtxPhys  Physical address of the SMM context buffer.

  @retval EFI_SUCCESS         The request was handled successfully.
  @retval EFI_DEVICE_ERROR    The SMI was not handled or returned an error.
**/
STATIC
EFI_STATUS
SendSmmSvc (
  IN UINT8                 Subcmd,
  IN EFI_PHYSICAL_ADDRESS  CtxPhys
  )
{
  UINTN  Cmd;
  UINTN  Result;

  Cmd = (UINTN)(((UINTN)Subcmd << 8) | OPAL_S3_APM_CNT_SVC);

  Result = LockBoxTriggerSmi (Cmd, (UINTN)CtxPhys, 5);
  if (Result == Cmd) {
    DEBUG ((DEBUG_VERBOSE, "%a(): no SMM response\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  if (Result != 0) {
    DEBUG ((DEBUG_VERBOSE, "%a(): SMM returned error: 0x%lx\n", __func__, (UINT64)Result));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Parse an OPAL device LockBox and forward each device secret to SMM.

  @param[in]  Buffer  Pointer to the LockBox payload.
  @param[in]  Length  Size of the payload in bytes.
**/
STATIC
VOID
ForwardOpalDeviceLockBoxToSmm (
  IN VOID   *Buffer,
  IN UINTN  Length
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  CtxPhys;
  OPAL_S3_SMM_CTX       *Ctx;
  UINT8                 *Ptr;
  UINT8                 *End;
  UINT32                MinRecLen;

  if ((Buffer == NULL) || (Length == 0)) {
    return;
  }

  MinRecLen = OFFSET_OF (OPAL_DEVICE_LOCKBOX_DATA, DevicePath);
  Ptr       = (UINT8 *)Buffer;
  End       = Ptr + Length;

  CtxPhys = 0xFFFFFFFF;
  Status  = gBS->AllocatePages (
                   AllocateMaxAddress,
                   EfiBootServicesData,
                   1,
                   &CtxPhys
                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_VERBOSE, "%a(): AllocatePages() failed: %r\n", __func__, Status));
    return;
  }

  Ctx = (OPAL_S3_SMM_CTX *)(UINTN)CtxPhys;
  ZeroMem (Ctx, EFI_PAGE_SIZE);

  while (Ptr + MinRecLen <= End) {
    OPAL_DEVICE_LOCKBOX_DATA  *Rec;

    Rec = (OPAL_DEVICE_LOCKBOX_DATA *)(VOID *)Ptr;
    if ((Rec->Length < MinRecLen) || (Ptr + Rec->Length > End)) {
      break;
    }

    if ((Rec->PasswordLength != 0) && (Rec->PasswordLength <= OPAL_MAX_PASSWORD_SIZE)) {
      ZeroMem (Ctx, sizeof (*Ctx));
      Ctx->Signature      = OPAL_S3_SMM_CTX_SIGNATURE;
      Ctx->Version        = OPAL_S3_SMM_CTX_VERSION;
      Ctx->Size           = sizeof (*Ctx);
      Ctx->Bus            = Rec->Device.Bus;
      Ctx->Device         = Rec->Device.Device;
      Ctx->Function       = Rec->Device.Function;
      Ctx->OpalBaseComId  = Rec->OpalBaseComId;
      Ctx->PasswordLength = Rec->PasswordLength;
      CopyMem (Ctx->Password, Rec->Password, Rec->PasswordLength);

      Status = SendSmmSvc (OPAL_S3_SMM_SUBCMD_SET_SECRET, CtxPhys);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_VERBOSE, "%a(): OPAL SMI handoff failed: %r\n", __func__, Status));
      }

      ZeroMem (Ctx->Password, sizeof (Ctx->Password));
      Ctx->PasswordLength = 0;
    }

    Ptr += Rec->Length;
  }

  ZeroMem (Ctx, EFI_PAGE_SIZE);
  gBS->FreePages (CtxPhys, 1);
}

/**
  Save a LockBox entry.

  @param[in]  Guid    LockBox GUID.
  @param[in]  Buffer  Buffer containing the data to save.
  @param[in]  Length  Size of the data in bytes.

  @retval RETURN_SUCCESS            The entry was created.
  @retval RETURN_INVALID_PARAMETER  Input parameters are invalid.
  @retval RETURN_ALREADY_STARTED    An entry already exists for Guid.
  @retval RETURN_OUT_OF_RESOURCES   Allocation failed.
**/
RETURN_STATUS
EFIAPI
SaveLockBox (
  IN  GUID   *Guid,
  IN  VOID   *Buffer,
  IN  UINTN  Length
  )
{
  LOCK_BOX_ENTRY  *Entry;

  if ((Guid == NULL) || (Buffer == NULL) || (Length == 0)) {
    return RETURN_INVALID_PARAMETER;
  }

  Entry = FindLockBoxEntry (Guid);
  if (Entry != NULL) {
    return RETURN_ALREADY_STARTED;
  }

  Entry = AllocateZeroPool (sizeof (*Entry));
  if (Entry == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  Entry->Data = AllocatePool (Length);
  if (Entry->Data == NULL) {
    FreePool (Entry);
    return RETURN_OUT_OF_RESOURCES;
  }

  CopyMem (&Entry->Guid, Guid, sizeof (*Guid));
  CopyMem (Entry->Data, Buffer, Length);
  Entry->Length = Length;

  Entry->Next  = mLockBoxList;
  mLockBoxList = Entry;

  if (CompareGuid (Guid, &mOpalDeviceLockBoxGuid)) {
    ForwardOpalDeviceLockBoxToSmm (Entry->Data, Entry->Length);
  }

  return RETURN_SUCCESS;
}

/**
  Set attributes for an existing LockBox entry.

  @param[in]  Guid        LockBox GUID.
  @param[in]  Attributes  Attributes to set.

  @retval RETURN_SUCCESS            Attributes were updated.
  @retval RETURN_INVALID_PARAMETER  Guid is NULL.
  @retval RETURN_NOT_FOUND          No entry exists for Guid.
**/
RETURN_STATUS
EFIAPI
SetLockBoxAttributes (
  IN  GUID    *Guid,
  IN  UINT64  Attributes
  )
{
  LOCK_BOX_ENTRY  *Entry;

  if (Guid == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  Entry = FindLockBoxEntry (Guid);
  if (Entry == NULL) {
    return RETURN_NOT_FOUND;
  }

  Entry->Attributes = Attributes;
  return RETURN_SUCCESS;
}

/**
  Update an existing LockBox entry at an offset.

  @param[in]  Guid    LockBox GUID.
  @param[in]  Offset  Offset within the entry to update.
  @param[in]  Buffer  Buffer containing the new data.
  @param[in]  Length  Size of the new data in bytes.

  @retval RETURN_SUCCESS            The entry was updated.
  @retval RETURN_INVALID_PARAMETER  Input parameters are invalid.
  @retval RETURN_NOT_FOUND          No entry exists for Guid.
  @retval RETURN_OUT_OF_RESOURCES   Allocation failed.
**/
RETURN_STATUS
EFIAPI
UpdateLockBox (
  IN  GUID   *Guid,
  IN  UINTN  Offset,
  IN  VOID   *Buffer,
  IN  UINTN  Length
  )
{
  LOCK_BOX_ENTRY  *Entry;
  VOID            *NewData;
  UINTN           NewLength;

  if ((Guid == NULL) || (Buffer == NULL) || (Length == 0)) {
    return RETURN_INVALID_PARAMETER;
  }

  Entry = FindLockBoxEntry (Guid);
  if (Entry == NULL) {
    return RETURN_NOT_FOUND;
  }

  if (Offset > MAX_UINTN - Length) {
    return RETURN_INVALID_PARAMETER;
  }

  NewLength = Offset + Length;
  if (NewLength > Entry->Length) {
    NewData = AllocateZeroPool (NewLength);
    if (NewData == NULL) {
      return RETURN_OUT_OF_RESOURCES;
    }

    CopyMem (NewData, Entry->Data, Entry->Length);
    FreePool (Entry->Data);
    Entry->Data   = NewData;
    Entry->Length = NewLength;
  }

  CopyMem ((UINT8 *)Entry->Data + Offset, Buffer, Length);

  if (CompareGuid (Guid, &mOpalDeviceLockBoxGuid)) {
    ForwardOpalDeviceLockBoxToSmm (Entry->Data, Entry->Length);
  }

  return RETURN_SUCCESS;
}

/**
  Restore a LockBox entry into a caller-provided buffer.

  @param[in]      Guid    LockBox GUID.
  @param[in]      Buffer  Destination buffer (optional).
  @param[in, out] Length  On input, buffer size; on output, data size (optional).

  @retval RETURN_SUCCESS            Data restored successfully.
  @retval RETURN_INVALID_PARAMETER  Input parameters are invalid.
  @retval RETURN_NOT_FOUND          No entry exists for Guid.
  @retval RETURN_BUFFER_TOO_SMALL   Buffer is too small; Length updated.
  @retval RETURN_WRITE_PROTECTED    Buffer/Length not provided.
**/
RETURN_STATUS
EFIAPI
RestoreLockBox (
  IN  GUID       *Guid,
  IN  VOID       *Buffer  OPTIONAL,
  IN  OUT UINTN  *Length  OPTIONAL
  )
{
  LOCK_BOX_ENTRY  *Entry;

  if (Guid == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if ((Buffer == NULL) != (Length == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  if ((Buffer == NULL) && (Length == NULL)) {
    return RETURN_WRITE_PROTECTED;
  }

  Entry = FindLockBoxEntry (Guid);
  if (Entry == NULL) {
    return RETURN_NOT_FOUND;
  }

  if (*Length < Entry->Length) {
    *Length = Entry->Length;
    return RETURN_BUFFER_TOO_SMALL;
  }

  CopyMem (Buffer, Entry->Data, Entry->Length);
  *Length = Entry->Length;
  return RETURN_SUCCESS;
}

/**
  Restore all LockBox entries in-place.

  This implementation does not support restoring all entries in-place.

  @retval RETURN_UNSUPPORTED  This operation is not supported.
**/
RETURN_STATUS
EFIAPI
RestoreAllLockBoxInPlace (
  VOID
  )
{
  return RETURN_UNSUPPORTED;
}
