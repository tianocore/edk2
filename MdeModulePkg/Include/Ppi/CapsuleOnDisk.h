/** @file
  This file declares Capsule On Disk PPI.  This PPI is used to find and load the
  capsule on files that are relocated into a temp file under rootdir.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PEI_CAPSULE_ON_DISK_PPI_H__
#define __PEI_CAPSULE_ON_DISK_PPI_H__

#define EDKII_PEI_CAPSULE_ON_DISK_PPI_GUID \
  { \
    0x71a9ea61, 0x5a35, 0x4a5d, {0xac, 0xef, 0x9c, 0xf8, 0x6d, 0x6d, 0x67, 0xe0 } \
  }

typedef struct _EDKII_PEI_CAPSULE_ON_DISK_PPI EDKII_PEI_CAPSULE_ON_DISK_PPI;

/**
  Loads a DXE capsule from some media into memory and updates the HOB table
  with the DXE firmware volume information.

  @param  PeiServices   General-purpose services that are available to every PEIM.
  @param  This          Indicates the EFI_PEI_RECOVERY_MODULE_PPI instance.

  @retval EFI_SUCCESS        The capsule was loaded correctly.
  @retval EFI_DEVICE_ERROR   A device error occurred.
  @retval EFI_NOT_FOUND      A recovery DXE capsule cannot be found.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_PEI_LOAD_CAPSULE_ON_DISK)(
  IN EFI_PEI_SERVICES               **PeiServices,
  IN EDKII_PEI_CAPSULE_ON_DISK_PPI  *This
  );

///
///  Finds and loads the recovery files.
///
struct _EDKII_PEI_CAPSULE_ON_DISK_PPI {
  EDKII_PEI_LOAD_CAPSULE_ON_DISK LoadCapsuleOnDisk;  ///< Loads a DXE binary capsule into memory.
};

extern EFI_GUID gEdkiiPeiCapsuleOnDiskPpiGuid;

#define EDKII_PEI_BOOT_IN_CAPSULE_ON_DISK_MODE_PPI \
  { \
    0xb08a11e4, 0xe2b7, 0x4b75, { 0xb5, 0x15, 0xaf, 0x61, 0x6, 0x68, 0xbf, 0xd1 } \
  }

extern EFI_GUID gEdkiiPeiBootInCapsuleOnDiskModePpiGuid;

#endif
