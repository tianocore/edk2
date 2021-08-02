/** @file
  This library provides helper functions to set/clear Secure Boot
  keys and databases.

  Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2018 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2021, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2021, Semihalf All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Guid/GlobalVariable.h>
#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/ImageAuthentication.h>
#include <Library/BaseCryptLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SecureBootVariableLib.h>
#include "Library/DxeServicesLib.h"

/** Creates EFI Signature List structure.

  @param[in]      Data     A pointer to signature data.
  @param[in]      Size     Size of signature data.
  @param[out]     SigList  Created Signature List.

  @retval  EFI_SUCCESS           Signature List was created successfully.
  @retval  EFI_OUT_OF_RESOURCES  Failed to allocate memory.
**/
STATIC
EFI_STATUS
CreateSigList (
  IN VOID                *Data,
  IN UINTN               Size,
  OUT EFI_SIGNATURE_LIST **SigList
  )
{
  UINTN                  SigListSize;
  EFI_SIGNATURE_LIST     *TmpSigList;
  EFI_SIGNATURE_DATA     *SigData;

  //
  // Allocate data for Signature Database
  //
  SigListSize = sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_SIGNATURE_DATA) - 1 + Size;
  TmpSigList = (EFI_SIGNATURE_LIST *) AllocateZeroPool (SigListSize);
  if (TmpSigList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Only gEfiCertX509Guid type is supported
  //
  TmpSigList->SignatureListSize = (UINT32)SigListSize;
  TmpSigList->SignatureSize = (UINT32) (sizeof (EFI_SIGNATURE_DATA) - 1 + Size);
  TmpSigList->SignatureHeaderSize = 0;
  CopyGuid (&TmpSigList->SignatureType, &gEfiCertX509Guid);

  //
  // Copy key data
  //
  SigData = (EFI_SIGNATURE_DATA *) (TmpSigList + 1);
  CopyGuid (&SigData->SignatureOwner, &gEfiGlobalVariableGuid);
  CopyMem (&SigData->SignatureData[0], Data, Size);

  *SigList = TmpSigList;

  return EFI_SUCCESS;
}

/** Adds new signature list to signature database.

  @param[in]      SigLists        A pointer to signature database.
  @param[in]      SigListAppend  A signature list to be added.
  @param[out]     *SigListOut     Created signature database.
  @param[in, out] SigListsSize    A size of created signature database.

  @retval  EFI_SUCCESS           Signature List was added successfully.
  @retval  EFI_OUT_OF_RESOURCES  Failed to allocate memory.
**/
STATIC
EFI_STATUS
ConcatenateSigList (
  IN  EFI_SIGNATURE_LIST *SigLists,
  IN  EFI_SIGNATURE_LIST *SigListAppend,
  OUT EFI_SIGNATURE_LIST **SigListOut,
  IN OUT UINTN           *SigListsSize
)
{
  EFI_SIGNATURE_LIST *TmpSigList;
  UINT8              *Offset;
  UINTN              NewSigListsSize;

  NewSigListsSize = *SigListsSize + SigListAppend->SignatureListSize;

  TmpSigList = (EFI_SIGNATURE_LIST *) AllocateZeroPool (NewSigListsSize);
  if (TmpSigList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (TmpSigList, SigLists, *SigListsSize);

  Offset = (UINT8 *)TmpSigList;
  Offset += *SigListsSize;
  CopyMem ((VOID *)Offset, SigListAppend, SigListAppend->SignatureListSize);

  *SigListsSize = NewSigListsSize;
  *SigListOut = TmpSigList;
  return EFI_SUCCESS;
}

/**
  Create a EFI Signature List with data fetched from section specified as a argument.
  Found keys are verified using RsaGetPublicKeyFromX509().

  @param[in]        KeyFileGuid    A pointer to to the FFS filename GUID
  @param[out]       SigListsSize   A pointer to size of signature list
  @param[out]       SigListOut    a pointer to a callee-allocated buffer with signature lists

  @retval EFI_SUCCESS              Create time based payload successfully.
  @retval EFI_NOT_FOUND            Section with key has not been found.
  @retval EFI_INVALID_PARAMETER    Embedded key has a wrong format.
  @retval Others                   Unexpected error happens.

**/
EFI_STATUS
SecureBootFetchData (
    IN  EFI_GUID           *KeyFileGuid,
    OUT UINTN              *SigListsSize,
    OUT EFI_SIGNATURE_LIST **SigListOut
)
{
  EFI_SIGNATURE_LIST *EfiSig;
  EFI_SIGNATURE_LIST *TmpEfiSig;
  EFI_SIGNATURE_LIST *TmpEfiSig2;
  EFI_STATUS         Status;
  VOID               *Buffer;
  VOID               *RsaPubKey;
  UINTN               Size;
  UINTN               KeyIndex;


  KeyIndex = 0;
  EfiSig = NULL;
  *SigListsSize = 0;
  while (1) {
    Status = GetSectionFromAnyFv (
               KeyFileGuid,
               EFI_SECTION_RAW,
               KeyIndex,
               &Buffer,
               &Size
               );

    if (Status == EFI_SUCCESS) {
      RsaPubKey = NULL;
      if (RsaGetPublicKeyFromX509 (Buffer, Size, &RsaPubKey) == FALSE) {
        DEBUG ((DEBUG_ERROR, "%a: Invalid key format: %d\n", __FUNCTION__, KeyIndex));
        if (EfiSig != NULL) {
          FreePool(EfiSig);
        }
        FreePool(Buffer);
        return EFI_INVALID_PARAMETER;
      }

      Status = CreateSigList (Buffer, Size, &TmpEfiSig);

      //
      // Concatenate lists if more than one section found
      //
      if (KeyIndex == 0) {
        EfiSig = TmpEfiSig;
        *SigListsSize = TmpEfiSig->SignatureListSize;
      } else {
        ConcatenateSigList (EfiSig, TmpEfiSig, &TmpEfiSig2, SigListsSize);
        FreePool (EfiSig);
        FreePool (TmpEfiSig);
        EfiSig = TmpEfiSig2;
      }

      KeyIndex++;
      FreePool (Buffer);
    } if (Status == EFI_NOT_FOUND) {
      break;
    }
  };

  if (KeyIndex == 0) {
    return EFI_NOT_FOUND;
  }

  *SigListOut = EfiSig;

  return EFI_SUCCESS;
}

/**
  Create a time based data payload by concatenating the EFI_VARIABLE_AUTHENTICATION_2
  descriptor with the input data. NO authentication is required in this function.

  @param[in, out]   DataSize       On input, the size of Data buffer in bytes.
                                   On output, the size of data returned in Data
                                   buffer in bytes.
  @param[in, out]   Data           On input, Pointer to data buffer to be wrapped or
                                   pointer to NULL to wrap an empty payload.
                                   On output, Pointer to the new payload date buffer allocated from pool,
                                   it's caller's responsibility to free the memory when finish using it.

  @retval EFI_SUCCESS              Create time based payload successfully.
  @retval EFI_OUT_OF_RESOURCES     There are not enough memory resources to create time based payload.
  @retval EFI_INVALID_PARAMETER    The parameter is invalid.
  @retval Others                   Unexpected error happens.

**/
EFI_STATUS
CreateTimeBasedPayload (
  IN OUT UINTN            *DataSize,
  IN OUT UINT8            **Data
  )
{
  EFI_STATUS                       Status;
  UINT8                            *NewData;
  UINT8                            *Payload;
  UINTN                            PayloadSize;
  EFI_VARIABLE_AUTHENTICATION_2    *DescriptorData;
  UINTN                            DescriptorSize;
  EFI_TIME                         Time;

  if (Data == NULL || DataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // In Setup mode or Custom mode, the variable does not need to be signed but the
  // parameters to the SetVariable() call still need to be prepared as authenticated
  // variable. So we create EFI_VARIABLE_AUTHENTICATED_2 descriptor without certificate
  // data in it.
  //
  Payload     = *Data;
  PayloadSize = *DataSize;

  DescriptorSize    = OFFSET_OF (EFI_VARIABLE_AUTHENTICATION_2, AuthInfo) + OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  NewData = (UINT8*) AllocateZeroPool (DescriptorSize + PayloadSize);
  if (NewData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if ((Payload != NULL) && (PayloadSize != 0)) {
    CopyMem (NewData + DescriptorSize, Payload, PayloadSize);
  }

  DescriptorData = (EFI_VARIABLE_AUTHENTICATION_2 *) (NewData);

  ZeroMem (&Time, sizeof (EFI_TIME));
  Status = gRT->GetTime (&Time, NULL);
  if (EFI_ERROR (Status)) {
    FreePool(NewData);
    return Status;
  }
  Time.Pad1       = 0;
  Time.Nanosecond = 0;
  Time.TimeZone   = 0;
  Time.Daylight   = 0;
  Time.Pad2       = 0;
  CopyMem (&DescriptorData->TimeStamp, &Time, sizeof (EFI_TIME));

  DescriptorData->AuthInfo.Hdr.dwLength         = OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  DescriptorData->AuthInfo.Hdr.wRevision        = 0x0200;
  DescriptorData->AuthInfo.Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  CopyGuid (&DescriptorData->AuthInfo.CertType, &gEfiCertPkcs7Guid);

  if (Payload != NULL) {
    FreePool(Payload);
  }

  *DataSize = DescriptorSize + PayloadSize;
  *Data     = NewData;
  return EFI_SUCCESS;
}

/**
  Internal helper function to delete a Variable given its name and GUID, NO authentication
  required.

  @param[in]      VariableName            Name of the Variable.
  @param[in]      VendorGuid              GUID of the Variable.

  @retval EFI_SUCCESS              Variable deleted successfully.
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
DeleteVariable (
  IN  CHAR16                    *VariableName,
  IN  EFI_GUID                  *VendorGuid
  )
{
  EFI_STATUS              Status;
  VOID*                   Variable;
  UINT8                   *Data;
  UINTN                   DataSize;
  UINT32                  Attr;

  GetVariable2 (VariableName, VendorGuid, &Variable, NULL);
  if (Variable == NULL) {
    return EFI_SUCCESS;
  }
  FreePool (Variable);

  Data     = NULL;
  DataSize = 0;
  Attr     = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS
             | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;

  Status = CreateTimeBasedPayload (&DataSize, &Data);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fail to create time-based data payload: %r", Status));
    return Status;
  }

  Status = gRT->SetVariable (
                  VariableName,
                  VendorGuid,
                  Attr,
                  DataSize,
                  Data
                  );
  if (Data != NULL) {
    FreePool (Data);
  }
  return Status;
}

/**

  Set the platform secure boot mode into "Custom" or "Standard" mode.

  @param[in]   SecureBootMode        New secure boot mode: STANDARD_SECURE_BOOT_MODE or
                                     CUSTOM_SECURE_BOOT_MODE.

  @return EFI_SUCCESS                The platform has switched to the special mode successfully.
  @return other                      Fail to operate the secure boot mode.

**/
EFI_STATUS
SetSecureBootMode (
  IN  UINT8  SecureBootMode
  )
{
  return gRT->SetVariable (
                EFI_CUSTOM_MODE_NAME,
                &gEfiCustomModeEnableGuid,
                EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                sizeof (UINT8),
                &SecureBootMode
                );
}

/**
  Fetches the value of SetupMode variable.

  @param[out] SetupMode             Pointer to UINT8 for SetupMode output

  @retval other                     Retval from GetVariable.
**/
EFI_STATUS
EFIAPI
GetSetupMode (
    OUT UINT8 *SetupMode
)
{
  UINTN      Size;
  EFI_STATUS Status;

  Size = sizeof (*SetupMode);
  Status = gRT->GetVariable (
                  EFI_SETUP_MODE_NAME,
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &Size,
                  SetupMode
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Clears the content of the 'db' variable.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2 (), GetTime () and SetVariable ()
**/
EFI_STATUS
EFIAPI
DeleteDb (
  VOID
)
{
  EFI_STATUS Status;

  Status = DeleteVariable (
             EFI_IMAGE_SECURITY_DATABASE,
             &gEfiImageSecurityDatabaseGuid
             );

  return Status;
}

/**
  Clears the content of the 'dbx' variable.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2 (), GetTime () and SetVariable ()
**/
EFI_STATUS
EFIAPI
DeleteDbx (
  VOID
)
{
  EFI_STATUS Status;

  Status = DeleteVariable (
             EFI_IMAGE_SECURITY_DATABASE1,
             &gEfiImageSecurityDatabaseGuid
             );

  return Status;
}

/**
  Clears the content of the 'dbt' variable.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2 (), GetTime () and SetVariable ()
**/
EFI_STATUS
EFIAPI
DeleteDbt (
  VOID
)
{
  EFI_STATUS Status;

  Status = DeleteVariable (
             EFI_IMAGE_SECURITY_DATABASE2,
             &gEfiImageSecurityDatabaseGuid
             );

  return Status;
}

/**
  Clears the content of the 'KEK' variable.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2 (), GetTime () and SetVariable ()
**/
EFI_STATUS
EFIAPI
DeleteKEK (
  VOID
)
{
  EFI_STATUS Status;

  Status = DeleteVariable (
             EFI_KEY_EXCHANGE_KEY_NAME,
             &gEfiGlobalVariableGuid
             );

  return Status;
}

/**
  Remove the PK variable.

  @retval EFI_SUCCESS    Delete PK successfully.
  @retval Others         Could not allow to delete PK.

**/
EFI_STATUS
EFIAPI
DeletePlatformKey (
  VOID
)
{
  EFI_STATUS Status;

  Status = SetSecureBootMode(CUSTOM_SECURE_BOOT_MODE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DeleteVariable (
             EFI_PLATFORM_KEY_NAME,
             &gEfiGlobalVariableGuid
             );
  return Status;
}
