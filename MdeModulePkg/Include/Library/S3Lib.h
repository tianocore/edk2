/** @file
  S3 library class defines a set of methods related to S3 boot mode.
  This library class is no longer used and modules using this library should
  directly locate EFI_PEI_S3_RESUME_PPI defined in PI 1.2 specification.

Copyright (c) 2005 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __S3_LIB_H__
#define __S3_LIB_H__

/**
  This function is responsible for calling the S3 resume vector in the ACPI Tables.
  
  @retval EFI_SUCESS   Successfully restored config from S3.
  @retval Others       Failed to restore config from S3.

**/
EFI_STATUS
EFIAPI
AcpiS3ResumeOs (
  VOID
  );

#endif

