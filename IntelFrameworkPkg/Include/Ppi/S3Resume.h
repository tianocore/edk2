/** @file
  This file declares S3 Resume PPI which accomplishes the firmware S3 resume boot path
  and transfers control to OS.

  This PPI is published by the S3 resume PEIM and can be used on the S3 resume boot path to
  restore the platform to its preboot configuration and transfer control to OS. The information that is
  required for an S3 resume can be saved during the normal boot path using
  EFI_ACPI_S3_SAVE_PROTOCOL. This presaved information can then be restored in the S3
  resume boot path using EFI_PEI_S3_RESUME_PPI. Architecturally, the S3 resume PEIM is the
  last PEIM to be dispatched in the S3 resume boot path.
  Before using this PPI, the caller must ensure the necessary information for the S3 resume, such as
  the following, is available for the S3 resume boot path:
  - EFI_ACPI_S3_RESUME_SCRIPT_TABLE script table. Type
    EFI_ACPI_S3_RESUME_SCRIPT_TABLE is defined in the Intel Platform Innovation
    Framework for EFI Boot Script Specification.
  - OS waking vector.
  - The reserved memory range to be used for the S3 resume.
  Otherwise, the S3 resume boot path may fail.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  This PPI is defined in Framework for EFI S3 Resume Boot Path spec.
  Version 0.9.

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

  @param  PeiServices    The pointer to the PEI Services Table

  @retval EFI_ABORTED           Execution of the S3 resume boot script table failed.
  @retval EFI_NOT_FOUND         Could not be locate some necessary information that
                                is used for the S3 resume boot path d.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_S3_RESUME_PPI_RESTORE_CONFIG)(
  IN EFI_PEI_SERVICES   **PeiServices
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
