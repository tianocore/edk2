/** @file
  Implements functions to write firmware file

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"
#include "FwVolDriver.h"

/**
  Writes one or more files to the firmware volume.

  @param  This                   Indicates the calling context.
  @param  NumberOfFiles          Number of files.
  @param  WritePolicy            WritePolicy indicates the level of reliability
                                 for the write in the event of a power failure or
                                 other system failure during the write operation.
  @param  FileData               FileData is an pointer to an array of
                                 EFI_FV_WRITE_DATA. Each element of array
                                 FileData represents a file to be written.

  @retval EFI_SUCCESS            Files successfully written to firmware volume
  @retval EFI_OUT_OF_RESOURCES   Not enough buffer to be allocated.
  @retval EFI_DEVICE_ERROR       Device error.
  @retval EFI_WRITE_PROTECTED    Write protected.
  @retval EFI_NOT_FOUND          Not found.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_UNSUPPORTED        This function not supported.

**/
EFI_STATUS
EFIAPI
FvWriteFile (
  IN CONST EFI_FIRMWARE_VOLUME2_PROTOCOL  *This,
  IN       UINT32                         NumberOfFiles,
  IN       EFI_FV_WRITE_POLICY            WritePolicy,
  IN       EFI_FV_WRITE_FILE_DATA         *FileData
  )
{
  return EFI_UNSUPPORTED;
}
