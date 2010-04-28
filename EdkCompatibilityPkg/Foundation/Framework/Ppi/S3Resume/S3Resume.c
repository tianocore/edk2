/*++

  Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            
                                                                                            
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
  
Module Name:
  
  S3Resume.c

Abstract:

  Boot Script Executer PPI GUID as defined in Tiano

--*/

#include "Tiano.h"
#include "PeiBind.h"
#include "PeiApi.h"
#include EFI_PPI_DEFINITION (S3Resume)

EFI_GUID  gPeiS3ResumePpiGuid = PEI_S3_RESUME_PPI_GUID;

EFI_GUID_STRING(&gPeiS3ResumePpiGuid, "S3Resume", "S3 Resume PPI");
