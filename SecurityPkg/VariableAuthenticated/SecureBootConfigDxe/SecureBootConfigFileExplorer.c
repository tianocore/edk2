/** @file
  Internal file explorer functions for SecureBoot configuration module.

Copyright (c) 2012 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SecureBootConfigImpl.h"

VOID                  *mStartOpCodeHandle = NULL;
VOID                  *mEndOpCodeHandle = NULL;
EFI_IFR_GUID_LABEL    *mStartLabel = NULL;
EFI_IFR_GUID_LABEL    *mEndLabel = NULL;

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
  mStartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
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
  IN UINT16                           LabelId,
  IN SECUREBOOT_CONFIG_PRIVATE_DATA   *PrivateData
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
  This function will open a file or directory referenced by DevicePath.

  This function opens a file with the open mode according to the file path. The
  Attributes is valid only for EFI_FILE_MODE_CREATE.

  @param[in, out]  FilePath        On input, the device path to the file.
                                   On output, the remaining device path.
  @param[out]      FileHandle      Pointer to the file handle.
  @param[in]       OpenMode        The mode to open the file with.
  @param[in]       Attributes      The file's file attributes.

  @retval EFI_SUCCESS              The information was set.
  @retval EFI_INVALID_PARAMETER    One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED          Could not open the file path.
  @retval EFI_NOT_FOUND            The specified file could not be found on the
                                   device or the file system could not be found on
                                   the device.
  @retval EFI_NO_MEDIA             The device has no medium.
  @retval EFI_MEDIA_CHANGED        The device has a different medium in it or the
                                   medium is no longer supported.
  @retval EFI_DEVICE_ERROR         The device reported an error.
  @retval EFI_VOLUME_CORRUPTED     The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED      The file or medium is write protected.
  @retval EFI_ACCESS_DENIED        The file was opened read only.
  @retval EFI_OUT_OF_RESOURCES     Not enough resources were available to open the
                                   file.
  @retval EFI_VOLUME_FULL          The volume is full.
**/
EFI_STATUS
EFIAPI
OpenFileByDevicePath(
  IN OUT EFI_DEVICE_PATH_PROTOCOL     **FilePath,
  OUT EFI_FILE_HANDLE                 *FileHandle,
  IN UINT64                           OpenMode,
  IN UINT64                           Attributes
  )
{
  EFI_STATUS                      Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *EfiSimpleFileSystemProtocol;
  EFI_FILE_PROTOCOL               *Handle1;
  EFI_FILE_PROTOCOL               *Handle2;
  EFI_HANDLE                      DeviceHandle;
  CHAR16                          *PathName;
  UINTN                           PathLength;

  if ((FilePath == NULL || FileHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->LocateDevicePath (
                  &gEfiSimpleFileSystemProtocolGuid,
                  FilePath,
                  &DeviceHandle
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol(
                  DeviceHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID**)&EfiSimpleFileSystemProtocol,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EfiSimpleFileSystemProtocol->OpenVolume(EfiSimpleFileSystemProtocol, &Handle1);
  if (EFI_ERROR (Status)) {
    FileHandle = NULL;
    return Status;
  }

  //
  // go down directories one node at a time.
  //
  while (!IsDevicePathEnd (*FilePath)) {
    //
    // For file system access each node should be a file path component
    //
    if (DevicePathType    (*FilePath) != MEDIA_DEVICE_PATH ||
        DevicePathSubType (*FilePath) != MEDIA_FILEPATH_DP
       ) {
      FileHandle = NULL;
      return (EFI_INVALID_PARAMETER);
    }
    //
    // Open this file path node
    //
    Handle2  = Handle1;
    Handle1 = NULL;
    PathLength = DevicePathNodeLength (*FilePath) - sizeof (EFI_DEVICE_PATH_PROTOCOL);
    PathName = AllocateCopyPool (PathLength, ((FILEPATH_DEVICE_PATH*)*FilePath)->PathName);
    if (PathName == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Try to test opening an existing file
    //
    Status = Handle2->Open (
                          Handle2,
                          &Handle1,
                          PathName,
                          OpenMode &~EFI_FILE_MODE_CREATE,
                          0
                         );

    //
    // see if the error was that it needs to be created
    //
    if ((EFI_ERROR (Status)) && (OpenMode != (OpenMode &~EFI_FILE_MODE_CREATE))) {
      Status = Handle2->Open (
                            Handle2,
                            &Handle1,
                            PathName,
                            OpenMode,
                            Attributes
                           );
    }
    //
    // Close the last node
    //
    Handle2->Close (Handle2);

    FreePool (PathName);

    if (EFI_ERROR(Status)) {
      return (Status);
    }

    //
    // Get the next node
    //
    *FilePath = NextDevicePathNode (*FilePath);
  }

  //
  // This is a weak spot since if the undefined SHELL_FILE_HANDLE format changes this must change also!
  //
  *FileHandle = (VOID*)Handle1;
  return EFI_SUCCESS;
}


/**
  Extract filename from device path. The returned buffer is allocated using AllocateCopyPool.
  The caller is responsible for freeing the allocated buffer using FreePool(). If return NULL
  means not enough memory resource.

  @param DevicePath       Device path.

  @retval NULL            Not enough memory resourece for AllocateCopyPool.
  @retval Other           A new allocated string that represents the file name.

**/
CHAR16 *
ExtractFileNameFromDevicePath (
  IN   EFI_DEVICE_PATH_PROTOCOL *DevicePath
  )
{
  CHAR16          *String;
  CHAR16          *MatchString;
  CHAR16          *LastMatch;
  CHAR16          *FileName;
  UINTN           Length;

  ASSERT(DevicePath != NULL);

  String = DevicePathToStr(DevicePath);
  MatchString = String;
  LastMatch   = String;
  FileName    = NULL;

  while(MatchString != NULL){
    LastMatch   = MatchString + 1;
    MatchString = StrStr(LastMatch,L"\\");
  }

  Length = StrLen(LastMatch);
  FileName = AllocateCopyPool ((Length + 1) * sizeof(CHAR16), LastMatch);
  if (FileName != NULL) {
    *(FileName + Length) = 0;
  }

  FreePool(String);

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
UpdatePage(
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN  EFI_FORM_ID               FormId
  )
{
  CHAR16                *FileName;
  EFI_STRING_ID         StringToken;

  FileName = NULL;

  if (FilePath != NULL) {
    FileName = ExtractFileNameFromDevicePath(FilePath);
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

  OpenFileByDevicePath(
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
  IN EFI_DEVICE_PATH_PROTOCOL    *FilePath
  )
{
  return UpdatePage(FilePath, FORMID_ENROLL_PK_FORM);

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
  IN EFI_DEVICE_PATH_PROTOCOL    *FilePath
  )
{
  return UpdatePage(FilePath, FORMID_ENROLL_KEK_FORM);
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
  IN EFI_DEVICE_PATH_PROTOCOL    *FilePath
  )
{
  return UpdatePage(FilePath, SECUREBOOT_ENROLL_SIGNATURE_TO_DB);
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
  IN EFI_DEVICE_PATH_PROTOCOL    *FilePath
  )
{
  return UpdatePage(FilePath, SECUREBOOT_ENROLL_SIGNATURE_TO_DBX);
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
  IN EFI_DEVICE_PATH_PROTOCOL    *FilePath
  )
{
  return UpdatePage(FilePath, SECUREBOOT_ENROLL_SIGNATURE_TO_DBT);
}

