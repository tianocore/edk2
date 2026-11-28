/** @file
  Register and enumerate UFDT drivers.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/UfdtDispatcherLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

STATIC LIST_ENTRY  mUfdtDrivers = INITIALIZE_LIST_HEAD_VARIABLE (mUfdtDrivers);

/** Register a static UFDT driver, without discovering hardware.

  @param[in,out] Driver  Static driver to register once; its device list is initialized.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
**/
EFI_STATUS
EFIAPI
UfdtRegisterDriver (
  IN OUT UFDT_BINDING_DRIVER  *Driver
  )
{
  if ((Driver == NULL) || (Driver->PresentProtocol == NULL) ||
      (Driver->AbsentProtocol == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (CompareGuid (Driver->AbsentProtocol, Driver->PresentProtocol)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Driver->GenericClass != NULL) &&
      CompareGuid (Driver->AbsentProtocol, Driver->GenericClass))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((Driver->Phase <= UfdtPhaseInvalid) || (Driver->Phase >= UfdtPhaseMax)) {
    DEBUG ((DEBUG_ERROR, "UfdtRegisterDriver: invalid phase for %a\n", Driver->Name));
    return EFI_INVALID_PARAMETER;
  }

  InitializeListHead (&Driver->Devices);
  InsertHeadList (&mUfdtDrivers, &Driver->Link);
  return EFI_SUCCESS;
}

/** Iterate over registered drivers.

  @param[in] Driver  Registered driver, or NULL to retrieve the first one.

  @retval NULL   Reached the end of the registered drivers.
  @retval Other  Next registered driver.
**/
UFDT_BINDING_DRIVER *
EFIAPI
UfdtGetNextDriver (
  IN UFDT_BINDING_DRIVER  *Driver OPTIONAL
  )
{
  LIST_ENTRY  *Link;

  if (IsListEmpty (&mUfdtDrivers)) {
    return NULL;
  }

  if (Driver == NULL) {
    Link = GetFirstNode (&mUfdtDrivers);
  } else {
    Link = GetNextNode (&mUfdtDrivers, &Driver->Link);
  }

  ASSERT (Link != NULL);
  if (IsNull (&mUfdtDrivers, Link)) {
    return NULL;
  }

  return BASE_CR (Link, UFDT_BINDING_DRIVER, Link);
}
