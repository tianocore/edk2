/** @file
  This is an implementation of the ACPI S3 Save protocol.  This is defined in
  S3 boot path specification 0.9.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ACPI_S3_SAVE_H_
#define _ACPI_S3_SAVE_H_

/**
  Gets the buffer of legacy memory below 1 MB
  This function is to get the buffer in legacy memory below 1MB that is required during S3 resume.

  @param This           A pointer to the EFI_ACPI_S3_SAVE_PROTOCOL instance.
  @param Size           The returned size of legacy memory below 1 MB.

  @retval EFI_SUCCESS           Size is successfully returned.
  @retval EFI_INVALID_PARAMETER The pointer Size is NULL.

**/
EFI_STATUS
EFIAPI
LegacyGetS3MemorySize (
  IN  EFI_ACPI_S3_SAVE_PROTOCOL    * This,
  OUT UINTN                        * Size
  );

/**
  Prepares all information that is needed in the S3 resume boot path.

  Allocate the resources or prepare informations and save in ACPI variable set for S3 resume boot path

  @param This                 A pointer to the EFI_ACPI_S3_SAVE_PROTOCOL instance.
  @param LegacyMemoryAddress  The base address of legacy memory.

  @retval EFI_NOT_FOUND         Some necessary information cannot be found.
  @retval EFI_SUCCESS           All information was saved successfully.
  @retval EFI_OUT_OF_RESOURCES  Resources were insufficient to save all the information.
  @retval EFI_INVALID_PARAMETER The memory range is not located below 1 MB.

**/
EFI_STATUS
EFIAPI
S3Ready (
  IN EFI_ACPI_S3_SAVE_PROTOCOL     *This,
  IN VOID                          *LegacyMemoryAddress
  );
#endif
