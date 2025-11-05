/** @file
  Library providing 'acpiview' functionality to display and
  validate installed ACPI tables.

  Copyright (c) 2016 - 2020, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ACPI_VIEW_COMMAND_LIB_H_
#define ACPI_VIEW_COMMAND_LIB_H_

/**
  Dump a buffer to a file. Print error message if a file cannot be created.

  @param[in] FileName   The filename that shall be created to contain the buffer.
  @param[in] Buffer     Pointer to buffer that shall be dumped.
  @param[in] BufferSize The size of buffer to be dumped in bytes.

  @return The number of bytes that were written
**/
UINTN
EFIAPI
ShellDumpBufferToFile (
  IN CONST CHAR16  *FileNameBuffer,
  IN CONST VOID    *Buffer,
  IN CONST UINTN   BufferSize
  );

/**
  Display and validate ACPI tables.

  @param[in] ImageHandle  Handle to the Image (NULL if internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if internal).

  @retval SHELL_INVALID_PARAMETER The command line invocation could not be parsed.
  @retval SHELL_NOT_FOUND         The command failed.
  @retval SHELL_SUCCESS           The command was successful.
**/
SHELL_STATUS
EFIAPI
ShellCommandRunAcpiView (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

#endif // UEFI_SHELL_ACPIVIEW_COMMAND_LIB_H_
