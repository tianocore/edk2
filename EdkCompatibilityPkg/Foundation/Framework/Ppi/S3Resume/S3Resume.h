/*++

  Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            
                                                                                            
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
  
Module Name:

  S3Resume.h
    
Abstract:

  S3 Resume PPI 

--*/

#ifndef _PEI_S3_RESUME_PPI_H
#define _PEI_S3_RESUME_PPI_H

#define PEI_S3_RESUME_PPI_GUID \
  { \
    0x4426CCB2, 0xE684, 0x4a8a, {0xAE, 0x40, 0x20, 0xD4, 0xB0, 0x25, 0xB7, 0x10} \
  }

EFI_FORWARD_DECLARATION (PEI_S3_RESUME_PPI);

typedef
EFI_STATUS
(EFIAPI *PEI_S3_RESUME_PPI_RESTORE_CONFIG) (
  IN EFI_PEI_SERVICES   **PeiServices
  );

struct _PEI_S3_RESUME_PPI {
  PEI_S3_RESUME_PPI_RESTORE_CONFIG  S3RestoreConfig;
};

extern EFI_GUID gPeiS3ResumePpiGuid;

#endif
