/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    AcpiS3Save.c

Abstract:

  Tiano Tiano S3 Save Protocol

--*/

#include "Tiano.h"

#include EFI_PROTOCOL_DEFINITION (AcpiS3Save)

EFI_GUID  gEfiAcpiS3SaveGuid = EFI_ACPI_S3_SAVE_GUID;

EFI_GUID_STRING(&gEfiAcpiS3SaveGuid, "EFI Acpi S3 Save Protocol", "Tiano Acpi S3 Save Protocol");
