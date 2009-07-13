/** @file
  This PPI produces functions to interpret and execute the PI boot script table.
  
  This PPI is published by a PEIM and provides for the restoration of the platform's
  configuration when resuming from the ACPI S3 power state. The ability to execute 
  the boot script may depend on the availability of other PPIs. For example, if 
  the boot script includes an SMBus command, this PEIM looks for the relevant PPI 
  that is able to execute that command.  
  
  Copyright (c) 2007 - 2009, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  This PPI is defined in UEFI Platform Initialization Specification 1.2 Volume 5: 
  Standards

**/

#ifndef __PEI_S3_RESUME_PPI_H__
#define __PEI_S3_RESUME_PPI_H__

///
/// Global ID for EFI_PEI_S3_RESUME_PPI
///
#define EFI_PEI_S3_RESUME_PPI_GUID \
  { \
    0x4426CCB2, 0xE684, 0x4a8a, {0xAE, 0x40, 0x20, 0xD4, 0xB0, 0x25, 0xB7, 0x10 } \
  }

///
/// Forward declaration for EFI_PEI_S3_RESUME_PPI
///
typedef struct _EFI_PEI_S3_RESUME_PPI  EFI_PEI_S3_RESUME_PPI;

/**
  Restores the platform to its preboot configuration for an S3 resume and
  jumps to the OS waking vector.

  This function will restore the platform to its pre-boot configuration that was 
  pre-stored in the boot script table and transfer control to OS waking vector.
  Upon invocation, this function is responsible for locating the following 
  information before jumping to OS waking vector:
    - ACPI tables
    - boot script table
    - any other information that it needs
    
  The S3RestoreConfig() function then executes the pre-stored boot script table 
  and transitions the platform to the pre-boot state. The boot script is recorded 
  during regular boot using the EFI_S3_SAVE_STATE_PROTOCOL.Write() and
  EFI_S3_SMM_SAVE_STATE_PROTOCOL.Write() functions.  Finally, this function 
  transfers control to the OS waking vector. If the OS supports only a real-mode 
  waking vector, this function will switch from flat mode to real mode before 
  jumping to the waking vector.  If all platform pre-boot configurations are 
  successfully restored and all other necessary information is ready, this 
  function will never return and instead will directly jump to the OS waking 
  vector. If this function returns, it indicates that the attempt to resume 
  from the ACPI S3 sleep state failed.  
  
  @param[in] PeiServices   Pointer to the PEI Services Table

  @retval EFI_ABORTED     Execution of the S3 resume boot script table failed.
  @retval EFI_NOT_FOUND   Some necessary information that is used for the S3 
                          resume boot path could not be located.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_S3_RESUME_PPI_RESTORE_CONFIG)(
  IN EFI_PEI_SERVICES  **PeiServices
  );

/**
  EFI_PEI_S3_RESUME_PPI accomplishes the firmware S3 resume boot
  path and transfers control to OS.
**/
struct _EFI_PEI_S3_RESUME_PPI {
  ///
  /// Restores the platform to its preboot configuration for an S3 resume and
  /// jumps to the OS waking vector.
  ///
  EFI_PEI_S3_RESUME_PPI_RESTORE_CONFIG  S3RestoreConfig;
};

extern EFI_GUID gEfiPeiS3ResumePpiGuid;

#endif
