/** @file
  Internal file explorer functions for SecureBoot configuration module.

Copyright (c) 2012 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SecureBootConfigImpl.h"

VOID                *mStartOpCodeHandle = NULL;
VOID                *mEndOpCodeHandle   = NULL;
EFI_IFR_GUID_LABEL  *mStartLabel        = NULL;
EFI_IFR_GUID_LABEL  *mEndLabel          = NULL;

/**
  Refresh the global UpdateData structure.

**/
VOID
RefreshUpdateData (
  VOID
  )
{
  //
  // Free current updated date
  //
  if (mStartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mStartOpCodeHandle);
  }

  //
  // Create new OpCode Handle
  //
  mStartOpCodeHandle = HiiAllocateOpCodeHandle ();

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  mStartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                        mStartOpCodeHandle,
                                        &gEfiIfrTianoGuid,
                                        NULL,
                                        sizeof (EFI_IFR_GUID_LABEL)
                                        );
  mStartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
}

/**
  Clean up the dynamic opcode at label and form specified by both LabelId.

  @param[in] LabelId         It is both the Form ID and Label ID for opcode deletion.
  @param[in] PrivateData     Module private data.

**/
VOID
CleanUpPage (
  IN UINT16                          LabelId,
  IN SECUREBOOT_CONFIG_PRIVATE_DATA  *PrivateData
  )
{
  RefreshUpdateData ();

  //
  // Remove all op-codes from dynamic page
  //
  mStartLabel->Number = LabelId;
  HiiUpdateForm (
    PrivateData->HiiHandle,
    &gSecureBootConfigFormSetGuid,
    LabelId,
    mStartOpCodeHandle, // Label LabelId
    mEndOpCodeHandle    // LABEL_END
    );
}

/**
  Extract filename from device path. The returned buffer is allocated using AllocateCopyPool.
  The caller is responsible for freeing the allocated buffer using FreePool(). If return NULL
  means not enough memory resource.

  @param DevicePath       Device path.

  @retval NULL            Not enough memory resource for AllocateCopyPool.
  @retval Other           A new allocated string that represents the file name.

**/
CHAR16 *
ExtractFileNameFromDevicePath (
  IN   EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  CHAR16  *String;
  CHAR16  *MatchString;
  CHAR16  *LastMatch;
  CHAR16  *FileName;
  UINTN   Length;

  ASSERT (DevicePath != NULL);

  String      = DevicePathToStr (DevicePath);
  MatchString = String;
  LastMatch   = String;
  FileName    = NULL;

  while (MatchString != NULL) {
    LastMatch   = MatchString + 1;
    MatchString = StrStr (LastMatch, L"\\");
  }

  Length   = StrLen (LastMatch);
  FileName = AllocateCopyPool ((Length + 1) * sizeof (CHAR16), LastMatch);
  if (FileName != NULL) {
    *(FileName + Length) = 0;
  }

  FreePool (String);

  return FileName;
}

/**
  Update  the form base on the selected file.

  @param FilePath   Point to the file path.
  @param FormId     The form need to display.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.

**/
BOOLEAN
UpdatePage (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN  EFI_FORM_ID               FormId
  )
{
  CHAR16         *FileName;
  EFI_STRING_ID  StringToken;

  FileName = NULL;

  if (FilePath != NULL) {
    FileName = ExtractFileNameFromDevicePath (FilePath);
  }

  if (FileName == NULL) {
    //
    // FileName = NULL has two case:
    // 1. FilePath == NULL, not select file.
    // 2. FilePath != NULL, but ExtractFileNameFromDevicePath return NULL not enough memory resource.
    // In these two case, no need to update the form, and exit the caller function.
    //
    return TRUE;
  }

  StringToken =  HiiSetString (gSecureBootPrivateData->HiiHandle, 0, FileName, NULL);

  gSecureBootPrivateData->FileContext->FileName = FileName;

  EfiOpenFileByDevicePath (
    &FilePath,
    &gSecureBootPrivateData->FileContext->FHandle,
    EFI_FILE_MODE_READ,
    0
    );
  //
  // Create Subtitle op-code for the display string of the option.
  //
  RefreshUpdateData ();
  mStartLabel->Number = FormId;

  HiiCreateSubTitleOpCode (
    mStartOpCodeHandle,
    StringToken,
    0,
    0,
    0
    );

  HiiUpdateForm (
    gSecureBootPrivateData->HiiHandle,
    &gSecureBootConfigFormSetGuid,
    FormId,
    mStartOpCodeHandle, // Label FormId
    mEndOpCodeHandle    // LABEL_END
    );

  return TRUE;
}

/**
  Update the PK form base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.
**/
BOOLEAN
EFIAPI
UpdatePKFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  return UpdatePage (FilePath, FORMID_ENROLL_PK_FORM);
}

/**
  Update the KEK form base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.
**/
BOOLEAN
EFIAPI
UpdateKEKFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  return UpdatePage (FilePath, FORMID_ENROLL_KEK_FORM);
}

/**
  Update the DB form base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.
**/
BOOLEAN
EFIAPI
UpdateDBFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  return UpdatePage (FilePath, SECUREBOOT_ENROLL_SIGNATURE_TO_DB);
}

/**
  Update the DBX form base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.
**/
BOOLEAN
EFIAPI
UpdateDBXFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  return UpdatePage (FilePath, SECUREBOOT_ENROLL_SIGNATURE_TO_DBX);
}

/**
  Update the DBT form base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.
**/
BOOLEAN
EFIAPI
UpdateDBTFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  return UpdatePage (FilePath, SECUREBOOT_ENROLL_SIGNATURE_TO_DBT);
}
