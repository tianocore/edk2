/** @file
  EFI_FILE_PROTOCOL.SetInfo() member function for the Virtio Filesystem driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Guid/FileSystemInfo.h>            // gEfiFileSystemInfoGuid
#include <Guid/FileSystemVolumeLabelInfo.h> // gEfiFileSystemVolumeLabelInfo...
#include <Library/BaseLib.h>                // StrCmp()
#include <Library/BaseMemoryLib.h>          // CompareGuid()

#include "VirtioFsDxe.h"

/**
  Validate a buffer that the EFI_FILE_PROTOCOL.SetInfo() caller passes in for a
  particular InformationType GUID.

  The structure to be validated is supposed to end with a variable-length,
  NUL-terminated CHAR16 Name string.

  @param[in] SizeByProtocolCaller  The BufferSize parameter as provided by the
                                   EFI_FILE_PROTOCOL.SetInfo() caller.

  @param[in] MinimumStructSize     The minimum structure size that is required
                                   for the given InformationType GUID,
                                   including a single CHAR16 element from the
                                   trailing Name field.

  @param[in] IsSizeByInfoPresent   TRUE if and only if the expected structure
                                   starts with a UINT64 Size field that reports
                                   the actual structure size.

  @param[in] Buffer                The Buffer parameter as provided by the
                                   EFI_FILE_PROTOCOL.SetInfo() caller.

  @retval EFI_SUCCESS            Validation successful, Buffer is well-formed.

  @retval EFI_BAD_BUFFER_SIZE    The EFI_FILE_PROTOCOL.SetInfo()
                                 caller provided a BufferSize that is smaller
                                 than the minimum structure size required for
                                 the given InformationType GUID.

  @retval EFI_INVALID_PARAMETER  IsSizeByInfoPresent is TRUE, and the leading
                                 UINT64 Size field does not match the
                                 EFI_FILE_PROTOCOL.SetInfo() caller-provided
                                 BufferSize.

  @retval EFI_INVALID_PARAMETER  The trailing Name field does not consist of a
                                 whole multiple of CHAR16 elements.

  @retval EFI_INVALID_PARAMETER  The trailing Name field is not NUL-terminated.
**/
STATIC
EFI_STATUS
ValidateInfoStructure (
  IN UINTN   SizeByProtocolCaller,
  IN UINTN   MinimumStructSize,
  IN BOOLEAN IsSizeByInfoPresent,
  IN VOID    *Buffer
  )
{
  UINTN  NameFieldByteOffset;
  UINTN  NameFieldBytes;
  UINTN  NameFieldChar16s;
  CHAR16 *NameField;

  //
  // Make sure the internal function asking for validation passes in sane
  // values.
  //
  ASSERT (MinimumStructSize >= sizeof (CHAR16));
  NameFieldByteOffset = MinimumStructSize - sizeof (CHAR16);

  if (IsSizeByInfoPresent) {
    ASSERT (MinimumStructSize >= sizeof (UINT64) + sizeof (CHAR16));
    ASSERT (NameFieldByteOffset >= sizeof (UINT64));
  }

  //
  // Check whether the protocol caller provided enough bytes for the minimum
  // size of this info structure.
  //
  if (SizeByProtocolCaller < MinimumStructSize) {
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // If the info structure starts with a UINT64 Size field, check if that
  // agrees with the protocol caller-provided size.
  //
  if (IsSizeByInfoPresent) {
    UINT64 *SizeByInfo;

    SizeByInfo = Buffer;
    if (*SizeByInfo != SizeByProtocolCaller) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // The CHAR16 Name field at the end of the structure must have an even number
  // of bytes.
  //
  // The subtraction below cannot underflow, and yields at least
  // sizeof(CHAR16).
  //
  ASSERT (SizeByProtocolCaller >= NameFieldByteOffset);
  NameFieldBytes = SizeByProtocolCaller - NameFieldByteOffset;
  ASSERT (NameFieldBytes >= sizeof (CHAR16));
  if (NameFieldBytes % sizeof (CHAR16) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The CHAR16 Name field at the end of the structure must be NUL-terminated.
  //
  NameFieldChar16s = NameFieldBytes / sizeof (CHAR16);
  ASSERT (NameFieldChar16s >= 1);

  NameField = (CHAR16 *)((UINT8 *)Buffer + NameFieldByteOffset);
  if (NameField[NameFieldChar16s - 1] != L'\0') {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Process an EFI_FILE_SYSTEM_INFO setting request.
**/
STATIC
EFI_STATUS
SetFileSystemInfo (
  IN EFI_FILE_PROTOCOL *This,
  IN UINTN             BufferSize,
  IN VOID              *Buffer
  )
{
  VIRTIO_FS_FILE       *VirtioFsFile;
  VIRTIO_FS            *VirtioFs;
  EFI_STATUS           Status;
  EFI_FILE_SYSTEM_INFO *FileSystemInfo;

  VirtioFsFile = VIRTIO_FS_FILE_FROM_SIMPLE_FILE (This);
  VirtioFs     = VirtioFsFile->OwnerFs;

  //
  // Validate if Buffer passes as EFI_FILE_SYSTEM_INFO.
  //
  Status = ValidateInfoStructure (
             BufferSize,                       // SizeByProtocolCaller
             OFFSET_OF (EFI_FILE_SYSTEM_INFO,
               VolumeLabel) + sizeof (CHAR16), // MinimumStructSize
             TRUE,                             // IsSizeByInfoPresent
             Buffer
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  FileSystemInfo = Buffer;

  //
  // EFI_FILE_SYSTEM_INFO fields other than VolumeLabel cannot be changed, per
  // spec.
  //
  // If the label is being changed to its current value, report success;
  // otherwise, reject the request, as the Virtio Filesystem device does not
  // support changing the label.
  //
  if (StrCmp (FileSystemInfo->VolumeLabel, VirtioFs->Label) == 0) {
    return EFI_SUCCESS;
  }
  return EFI_WRITE_PROTECTED;
}

/**
  Process an EFI_FILE_SYSTEM_VOLUME_LABEL setting request.
**/
STATIC
EFI_STATUS
SetFileSystemVolumeLabelInfo (
  IN EFI_FILE_PROTOCOL *This,
  IN UINTN             BufferSize,
  IN VOID              *Buffer
  )
{
  VIRTIO_FS_FILE               *VirtioFsFile;
  VIRTIO_FS                    *VirtioFs;
  EFI_STATUS                   Status;
  EFI_FILE_SYSTEM_VOLUME_LABEL *FileSystemVolumeLabel;

  VirtioFsFile = VIRTIO_FS_FILE_FROM_SIMPLE_FILE (This);
  VirtioFs     = VirtioFsFile->OwnerFs;

  //
  // Validate if Buffer passes as EFI_FILE_SYSTEM_VOLUME_LABEL.
  //
  Status = ValidateInfoStructure (
             BufferSize,                              // SizeByProtocolCaller
             OFFSET_OF (EFI_FILE_SYSTEM_VOLUME_LABEL,
               VolumeLabel) + sizeof (CHAR16),        // MinimumStructSize
             FALSE,                                   // IsSizeByInfoPresent
             Buffer
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  FileSystemVolumeLabel = Buffer;

  //
  // If the label is being changed to its current value, report success;
  // otherwise, reject the request, as the Virtio Filesystem device does not
  // support changing the label.
  //
  if (StrCmp (FileSystemVolumeLabel->VolumeLabel, VirtioFs->Label) == 0) {
    return EFI_SUCCESS;
  }
  return EFI_WRITE_PROTECTED;
}

EFI_STATUS
EFIAPI
VirtioFsSimpleFileSetInfo (
  IN EFI_FILE_PROTOCOL *This,
  IN EFI_GUID          *InformationType,
  IN UINTN             BufferSize,
  IN VOID              *Buffer
  )
{
  if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    return EFI_UNSUPPORTED;
  }

  if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {
    return SetFileSystemInfo (This, BufferSize, Buffer);
  }

  if (CompareGuid (InformationType, &gEfiFileSystemVolumeLabelInfoIdGuid)) {
    return SetFileSystemVolumeLabelInfo (This, BufferSize, Buffer);
  }

  return EFI_UNSUPPORTED;
}
