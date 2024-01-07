/** @file
  EmulaotPkg RedfishPlatformCredentialLib instance

  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>

#include <Protocol/EdkIIRedfishCredential.h>

#include <Guid/GlobalVariable.h>
#include <Guid/ImageAuthentication.h>

BOOLEAN  mSecureBootDisabled = FALSE;
BOOLEAN  mStopRedfishService = FALSE;

EFI_STATUS
EFIAPI
LibStopRedfishService (
  IN EDKII_REDFISH_CREDENTIAL_PROTOCOL           *This,
  IN EDKII_REDFISH_CREDENTIAL_STOP_SERVICE_TYPE  ServiceStopType
  );

/**
  Return the credential for accessing to Redfish servcice.

  @param[out]  AuthMethod     The authentication method.
  @param[out]  UserId         User ID.
  @param[out]  Password       USer password.

  @retval EFI_SUCCESS              Get the authentication information successfully.
  @retval EFI_OUT_OF_RESOURCES     There are not enough memory resources.

**/
EFI_STATUS
GetRedfishCredential (
  OUT EDKII_REDFISH_AUTH_METHOD  *AuthMethod,
  OUT CHAR8                      **UserId,
  OUT CHAR8                      **Password
  )
{
  UINTN  UserIdSize;
  UINTN  PasswordSize;

  //
  // AuthMethod set to HTTP Basic authentication.
  //
  *AuthMethod = AuthMethodHttpBasic;

  //
  // User ID and Password.
  //
  UserIdSize   = AsciiStrSize ((CHAR8 *)PcdGetPtr (PcdRedfishServiceUserId));
  PasswordSize = AsciiStrSize ((CHAR8 *)PcdGetPtr (PcdRedfishServicePassword));
  if ((UserIdSize == 0) || (PasswordSize == 0)) {
    DEBUG ((DEBUG_ERROR, "Incorrect string of UserID or Password for REdfish service.\n"));
    return EFI_INVALID_PARAMETER;
  }

  *UserId = AllocateZeroPool (UserIdSize);
  if (*UserId == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (*UserId, (CHAR8 *)PcdGetPtr (PcdRedfishServiceUserId), UserIdSize);

  *Password = AllocateZeroPool (PasswordSize);
  if (*Password == NULL) {
    FreePool (*UserId);
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (*Password, (CHAR8 *)PcdGetPtr (PcdRedfishServicePassword), PasswordSize);
  return EFI_SUCCESS;
}

/**
  Retrieve platform's Redfish authentication information.

  This functions returns the Redfish authentication method together with the user Id and
  password.
  - For AuthMethodNone, the UserId and Password could be used for HTTP header authentication
    as defined by RFC7235.
  - For AuthMethodRedfishSession, the UserId and Password could be used for Redfish
    session login as defined by  Redfish API specification (DSP0266).

  Callers are responsible for and freeing the returned string storage.

  @param[in]   This                Pointer to EDKII_REDFISH_CREDENTIAL_PROTOCOL instance.
  @param[out]  AuthMethod          Type of Redfish authentication method.
  @param[out]  UserId              The pointer to store the returned UserId string.
  @param[out]  Password            The pointer to store the returned Password string.

  @retval EFI_SUCCESS              Get the authentication information successfully.
  @retval EFI_ACCESS_DENIED        SecureBoot is disabled after EndOfDxe.
  @retval EFI_INVALID_PARAMETER    This or AuthMethod or UserId or Password is NULL.
  @retval EFI_OUT_OF_RESOURCES     There are not enough memory resources.
  @retval EFI_UNSUPPORTED          Unsupported authentication method is found.

**/
EFI_STATUS
EFIAPI
LibCredentialGetAuthInfo (
  IN  EDKII_REDFISH_CREDENTIAL_PROTOCOL  *This,
  OUT EDKII_REDFISH_AUTH_METHOD          *AuthMethod,
  OUT CHAR8                              **UserId,
  OUT CHAR8                              **Password
  )
{
  EFI_STATUS  Status;

  if ((This == NULL) || (AuthMethod == NULL) || (UserId == NULL) || (Password == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (mStopRedfishService) {
    return EFI_ACCESS_DENIED;
  }

  if (mSecureBootDisabled) {
    Status = LibStopRedfishService (This, ServiceStopTypeSecureBootDisabled);
    if (EFI_ERROR (Status) && (Status != EFI_UNSUPPORTED)) {
      DEBUG ((DEBUG_ERROR, "SecureBoot has been disabled, but failed to stop RedfishService - %r\n", Status));
      return Status;
    }
  }

  Status = GetRedfishCredential (
             AuthMethod,
             UserId,
             Password
             );

  return Status;
}

/**
  Notify the Redfish service to stop provide configuration service to this platform.

  This function should be called when the platfrom is about to leave the safe environment.
  It will notify the Redfish service provider to abort all logined session, and prohibit
  further login with original auth info. GetAuthInfo() will return EFI_UNSUPPORTED once this
  function is returned.

  @param[in]   This                Pointer to EDKII_REDFISH_CREDENTIAL_PROTOCOL instance.
  @param[in]   ServiceStopType     Reason of stopping Redfish service.

  @retval EFI_SUCCESS              Service has been stoped successfully.
  @retval EFI_INVALID_PARAMETER    This is NULL or given the worng ServiceStopType.
  @retval EFI_UNSUPPORTED          Not support to stop Redfish service.
  @retval Others                   Some error happened.

**/
EFI_STATUS
EFIAPI
LibStopRedfishService (
  IN EDKII_REDFISH_CREDENTIAL_PROTOCOL           *This,
  IN EDKII_REDFISH_CREDENTIAL_STOP_SERVICE_TYPE  ServiceStopType
  )
{
  EFI_STATUS  Status;
  UINT8       *SecureBootVar;

  if (ServiceStopType >= ServiceStopTypeMax) {
    return EFI_INVALID_PARAMETER;
  }

  if (ServiceStopType == ServiceStopTypeSecureBootDisabled) {
    //
    // Check platform PCD to determine the action for stopping
    // Redfish service due to secure boot is disabled.
    //
    if (!PcdGetBool (PcdRedfishServiceStopIfSecureBootDisabled)) {
      return EFI_UNSUPPORTED;
    } else {
      //
      // Check Secure Boot status and lock Redfish service if Secure Boot is disabled.
      //
      Status = GetVariable2 (EFI_SECURE_BOOT_MODE_NAME, &gEfiGlobalVariableGuid, (VOID **)&SecureBootVar, NULL);
      if (EFI_ERROR (Status) || (*SecureBootVar != SECURE_BOOT_MODE_ENABLE)) {
        //
        // Secure Boot is disabled
        //
        mSecureBootDisabled = TRUE;
        mStopRedfishService = TRUE;
        DEBUG ((DEBUG_INFO, "EFI Redfish service is stopped due to SecureBoot is disabled!!\n"));
      }
    }
  } else if (ServiceStopType == ServiceStopTypeExitBootService) {
    //
    // Check platform PCD to determine the action for stopping
    // Redfish service due to exit boot service.
    //
    if (PcdGetBool (PcdRedfishServiceStopIfExitbootService)) {
      return EFI_UNSUPPORTED;
    } else {
      mStopRedfishService = TRUE;
      DEBUG ((DEBUG_INFO, "EFI Redfish service is stopped due to Exit Boot Service!!\n"));
    }
  } else {
    mStopRedfishService = TRUE;
    DEBUG ((DEBUG_INFO, "EFI Redfish service is stopped without Redfish service stop type!!\n"));
  }

  return EFI_SUCCESS;
}

/**
  Notification of Exit Boot Service.

  @param[in]  This    Pointer to EDKII_REDFISH_CREDENTIAL_PROTOCOL.
**/
VOID
EFIAPI
LibCredentialExitBootServicesNotify (
  IN  EDKII_REDFISH_CREDENTIAL_PROTOCOL  *This
  )
{
}

/**
  Notification of End of DXE.

  @param[in]  This    Pointer to EDKII_REDFISH_CREDENTIAL_PROTOCOL.
**/
VOID
EFIAPI
LibCredentialEndOfDxeNotify (
  IN  EDKII_REDFISH_CREDENTIAL_PROTOCOL  *This
  )
{
}
