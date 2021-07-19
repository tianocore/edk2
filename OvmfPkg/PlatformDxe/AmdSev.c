/**@file
  Detect KVM hypervisor support for SEV live migration and if
  detected, setup a new UEFI enviroment variable indicating
  OVMF support for SEV live migration.

  Copyright (c) 2021, Advanced Micro Devices. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
//
// The package level header files this module uses
//

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Guid/MemEncryptLib.h>

/**

  Function checks if SEV Live Migration support is available, if present then it sets
  a UEFI enviroment variable to be queried later using Runtime services.

  **/
VOID
AmdSevSetRuntimeConfig(
  VOID
  )
{
  EFI_STATUS Status;
  BOOLEAN SevLiveMigrationEnabled;

  SevLiveMigrationEnabled = MemEncryptSevLiveMigrationIsEnabled();

  if (SevLiveMigrationEnabled) {
    Status = gRT->SetVariable (
               L"SevLiveMigrationEnabled",
               &gMemEncryptGuid,
               EFI_VARIABLE_NON_VOLATILE |
               EFI_VARIABLE_BOOTSERVICE_ACCESS |
               EFI_VARIABLE_RUNTIME_ACCESS,
               sizeof (BOOLEAN),
               &SevLiveMigrationEnabled
               );

    DEBUG ((
      DEBUG_INFO,
      "%a: Setting SevLiveMigrationEnabled variable, status = %lx\n",
      __FUNCTION__,
      Status
      ));
  }
}
