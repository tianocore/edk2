/** @file
  This library provides API to invoke the S3 resume vector in the APCI Table in S3 resume mode. 

  This library instance is no longer used and module using this library
  class should update to directly locate EFI_PEI_S3_RESUME_PPI defined
  in PI 1.2 specification. 

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/DebugLib.h>

#include <Ppi/S3Resume.h>

/**
  This function is responsible for calling the S3 resume vector in the ACPI Tables.
  
  @retval EFI_SUCESS   Success to restore config from S3.
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

