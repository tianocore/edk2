/** @file
  S3 library class defines a set of methods related to S3 boot mode.
  This library class is no longer used and modules using this library should
  directly locate EFI_PEI_S3_RESUME_PPI, defined in the PI 1.2 specification.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __S3_LIB_H__
#define __S3_LIB_H__

/**
  This function is responsible for calling the S3 resume vector in the ACPI Tables.

  @retval EFI_SUCCESS   Successfully restored the configuration from S3.
  @retval Others       Failed to restore the configuration from S3.

**/
EFI_STATUS
EFIAPI
AcpiS3ResumeOs (
  VOID
  );

#endif

