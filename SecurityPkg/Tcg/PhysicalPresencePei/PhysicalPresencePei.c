/** @file
  This driver produces PEI_LOCK_PHYSICAL_PRESENCE_PPI to indicate
  whether TPM need be locked or not. It can be replaced by a platform
  specific driver.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Ppi/LockPhysicalPresence.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Guid/PhysicalPresenceData.h>
#include <Library/PcdLib.h>
#include <Library/PeiServicesLib.h>

/**
  This interface returns whether TPM physical presence needs be locked or not.

  @param[in]  PeiServices       The pointer to the PEI Services Table.

  @retval     TRUE              The TPM physical presence should be locked.
  @retval     FALSE             The TPM physical presence cannot be locked.

**/
BOOLEAN
EFIAPI
LockTpmPhysicalPresence (
  IN CONST  EFI_PEI_SERVICES             **PeiServices
  );

//
// Gobal defintions for lock physical presence PPI and its descriptor.
//
PEI_LOCK_PHYSICAL_PRESENCE_PPI    mLockPhysicalPresencePpi = {
  LockTpmPhysicalPresence
};

EFI_PEI_PPI_DESCRIPTOR       mLockPhysicalPresencePpiList = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gPeiLockPhysicalPresencePpiGuid,
  &mLockPhysicalPresencePpi
};

/**
  This interface returns whether TPM physical presence needs be locked or not.

  @param[in]  PeiServices       The pointer to the PEI Services Table.

  @retval     TRUE              The TPM physical presence should be locked.
  @retval     FALSE             The TPM physical presence cannot be locked.

**/
BOOLEAN
EFIAPI
LockTpmPhysicalPresence (
  IN CONST  EFI_PEI_SERVICES             **PeiServices
  )
{
  EFI_STATUS                         Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI    *Variable;
  UINTN                              DataSize;
  EFI_PHYSICAL_PRESENCE              TcgPpData;

  //
  // The CRTM has sensed the physical presence assertion of the user. For example,
  // the user has pressed the startup button or inserted a USB dongle. The details
  // of the implementation are vendor-specific. Here we read a PCD value to indicate
  // whether operator physical presence.
  //
  if (!PcdGetBool (PcdTpmPhysicalPresence)) {
    return TRUE;
  }

  //
  // Check the pending TPM requests. Lock TPM physical presence if there is no TPM
  // request.
  //
  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID **)&Variable
             );
  if (!EFI_ERROR (Status)) {
    DataSize = sizeof (EFI_PHYSICAL_PRESENCE);
    Status = Variable->GetVariable (
                         Variable,
                         PHYSICAL_PRESENCE_VARIABLE,
                         &gEfiPhysicalPresenceGuid,
                         NULL,
                         &DataSize,
                         &TcgPpData
                         );
    if (!EFI_ERROR (Status)) {
      if (TcgPpData.PPRequest != 0) {
        return FALSE;
      }
    }
  }

  //
  // Lock TPM physical presence by default.
  //
  return TRUE;
}

/**
  Entry point of this module.

  It installs lock physical presence PPI.

  @param[in] FileHandle   Handle of the file being invoked.
  @param[in] PeiServices  Describes the list of possible PEI Services.

  @return                 Status of install lock physical presence PPI.

**/
EFI_STATUS
EFIAPI
PeimEntry (
  IN       EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
{
  return PeiServicesInstallPpi (&mLockPhysicalPresencePpiList);
}
