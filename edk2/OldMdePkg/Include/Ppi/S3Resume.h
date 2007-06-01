/** @file
  This file declares S3 Resume PPI.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  S3Resume.h

  @par Revision Reference:
  This PPI is defined in Framework of EFI S3 Resume Boot Path spec.
  Version 0.9

**/

#ifndef __PEI_S3_RESUME_PPI_H__
#define __PEI_S3_RESUME_PPI_H__

#define EFI_PEI_S3_RESUME_PPI_GUID \
  { \
    0x4426CCB2, 0xE684, 0x4a8a, {0xAE, 0x40, 0x20, 0xD4, 0xB0, 0x25, 0xB7, 0x10 } \
  }

typedef struct _EFI_PEI_S3_RESUME_PPI   EFI_PEI_S3_RESUME_PPI;

/**
  Restores the platform to its preboot configuration for an S3 resume and 
  jumps to the OS waking vector.

  @param  PeiServices    Pointer to the PEI Services Table

  @retval EFI_ABORTED           Execution of the S3 resume boot script table failed.
  @retval EFI_NOT_FOUND         Some necessary information that is used for
                                the S3 resume boot path could not be located.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_S3_RESUME_PPI_RESTORE_CONFIG) (
  IN EFI_PEI_SERVICES   **PeiServices
  );

/**
  @par Ppi Description:
  EFI_PEI_S3_RESUME_PPI accomplishes the firmware S3 resume boot 
  path and transfers control to OS.

  @param S3RestoreConfig
  Restores the platform to its preboot configuration for an S3 resume and 
  jumps to the OS waking vector.

**/
struct _EFI_PEI_S3_RESUME_PPI {
  EFI_PEI_S3_RESUME_PPI_RESTORE_CONFIG  S3RestoreConfig;
};

extern EFI_GUID gEfiPeiS3ResumePpiGuid;

#endif
