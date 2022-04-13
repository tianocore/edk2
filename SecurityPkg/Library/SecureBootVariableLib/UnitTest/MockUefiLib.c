/** @file
  The UEFI Library provides functions and macros that simplify the development of
  UEFI Drivers and UEFI Applications.  These functions and macros help manage EFI
  events, build simple locks utilizing EFI Task Priority Levels (TPLs), install
  EFI Driver Model related protocols, manage Unicode string tables for UEFI Drivers,
  and print messages on the console output and standard error devices.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

/**
  Returns the status whether get the variable success. The function retrieves
  variable  through the UEFI Runtime Service GetVariable().  The
  returned buffer is allocated using AllocatePool().  The caller is responsible
  for freeing this buffer with FreePool().

  If Name  is NULL, then ASSERT().
  If Guid  is NULL, then ASSERT().
  If Value is NULL, then ASSERT().

  @param[in]  Name  The pointer to a Null-terminated Unicode string.
  @param[in]  Guid  The pointer to an EFI_GUID structure
  @param[out] Value The buffer point saved the variable info.
  @param[out] Size  The buffer size of the variable.

  @return EFI_OUT_OF_RESOURCES      Allocate buffer failed.
  @return EFI_SUCCESS               Find the specified variable.
  @return Others Errors             Return errors from call to gRT->GetVariable.

**/
EFI_STATUS
EFIAPI
GetVariable2 (
  IN CONST CHAR16    *Name,
  IN CONST EFI_GUID  *Guid,
  OUT VOID           **Value,
  OUT UINTN          *Size OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;

  ASSERT (Name != NULL && Guid != NULL && Value != NULL);

  //
  // Try to get the variable size.
  //
  BufferSize = 0;
  *Value     = NULL;
  if (Size != NULL) {
    *Size = 0;
  }

  Status = gRT->GetVariable ((CHAR16 *)Name, (EFI_GUID *)Guid, NULL, &BufferSize, *Value);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  //
  // Allocate buffer to get the variable.
  //
  *Value = AllocatePool (BufferSize);
  ASSERT (*Value != NULL);
  if (*Value == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get the variable data.
  //
  Status = gRT->GetVariable ((CHAR16 *)Name, (EFI_GUID *)Guid, NULL, &BufferSize, *Value);
  if (EFI_ERROR (Status)) {
    FreePool (*Value);
    *Value = NULL;
  }

  if (Size != NULL) {
    *Size = BufferSize;
  }

  return Status;
}

/** Return the attributes of the variable.

  Returns the status whether get the variable success. The function retrieves
  variable  through the UEFI Runtime Service GetVariable().  The
  returned buffer is allocated using AllocatePool().  The caller is responsible
  for freeing this buffer with FreePool().  The attributes are returned if
  the caller provides a valid Attribute parameter.

  If Name  is NULL, then ASSERT().
  If Guid  is NULL, then ASSERT().
  If Value is NULL, then ASSERT().

  @param[in]  Name  The pointer to a Null-terminated Unicode string.
  @param[in]  Guid  The pointer to an EFI_GUID structure
  @param[out] Value The buffer point saved the variable info.
  @param[out] Size  The buffer size of the variable.
  @param[out] Attr  The pointer to the variable attributes as found in var store

  @retval EFI_OUT_OF_RESOURCES      Allocate buffer failed.
  @retval EFI_SUCCESS               Find the specified variable.
  @retval Others Errors             Return errors from call to gRT->GetVariable.

**/
EFI_STATUS
EFIAPI
GetVariable3 (
  IN CONST CHAR16    *Name,
  IN CONST EFI_GUID  *Guid,
  OUT VOID           **Value,
  OUT UINTN          *Size OPTIONAL,
  OUT UINT32         *Attr OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;

  ASSERT (Name != NULL && Guid != NULL && Value != NULL);

  //
  // Try to get the variable size.
  //
  BufferSize = 0;
  *Value     = NULL;
  if (Size != NULL) {
    *Size = 0;
  }

  if (Attr != NULL) {
    *Attr = 0;
  }

  Status = gRT->GetVariable ((CHAR16 *)Name, (EFI_GUID *)Guid, Attr, &BufferSize, *Value);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  //
  // Allocate buffer to get the variable.
  //
  *Value = AllocatePool (BufferSize);
  ASSERT (*Value != NULL);
  if (*Value == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get the variable data.
  //
  Status = gRT->GetVariable ((CHAR16 *)Name, (EFI_GUID *)Guid, Attr, &BufferSize, *Value);
  if (EFI_ERROR (Status)) {
    FreePool (*Value);
    *Value = NULL;
  }

  if (Size != NULL) {
    *Size = BufferSize;
  }

  return Status;
}

/**
  Returns a pointer to an allocated buffer that contains the contents of a
  variable retrieved through the UEFI Runtime Service GetVariable().  This
  function always uses the EFI_GLOBAL_VARIABLE GUID to retrieve variables.
  The returned buffer is allocated using AllocatePool().  The caller is
  responsible for freeing this buffer with FreePool().

  If Name is NULL, then ASSERT().
  If Value is NULL, then ASSERT().

  @param[in]  Name  The pointer to a Null-terminated Unicode string.
  @param[out] Value The buffer point saved the variable info.
  @param[out] Size  The buffer size of the variable.

  @return EFI_OUT_OF_RESOURCES      Allocate buffer failed.
  @return EFI_SUCCESS               Find the specified variable.
  @return Others Errors             Return errors from call to gRT->GetVariable.

**/
EFI_STATUS
EFIAPI
GetEfiGlobalVariable2 (
  IN CONST CHAR16  *Name,
  OUT VOID         **Value,
  OUT UINTN        *Size OPTIONAL
  )
{
  return GetVariable2 (Name, &gEfiGlobalVariableGuid, Value, Size);
}
