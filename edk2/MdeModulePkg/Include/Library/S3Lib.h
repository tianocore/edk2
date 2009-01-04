/** @file
  S3 library class defines a set of methods related do S3 boot mode.

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
  
  @retval EFI_SUCESS   Sucess to restore config from S3.
  @retval Others       Fail to restore config from S3.

**/
EFI_STATUS
EFIAPI
AcpiS3ResumeOs (
  VOID
  );

#endif

