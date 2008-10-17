/**@file
  S3 Library. This library class defines a set of methods related do S3 mode

Copyright (c) 2006 - 2008 Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <FrameworkPei.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/DebugLib.h>

#include <Ppi/S3Resume.h>

/**
  This function is responsible for calling the S3 resume vector in the ACPI Tables.
  
  @retval EFI_SUCESS   Sucess to restore config from S3.
  @retval Others       Fail to restore config from S3.
**/
EFI_STATUS
EFIAPI
AcpiS3ResumeOs (
  VOID
  )
{
  EFI_STATUS              Status;
  EFI_PEI_S3_RESUME_PPI   *S3Resume;
  
  Status = PeiServicesLocatePpi (
             &gEfiPeiS3ResumePpiGuid,
             0,
             NULL,
             (VOID **)&S3Resume
             );
  ASSERT_EFI_ERROR (Status);

  return S3Resume->S3RestoreConfig ((EFI_PEI_SERVICES  **) GetPeiServicesTablePointer()); 
}

