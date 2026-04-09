/** @file
  The file operation functions for WiFi Connection Manager.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "WifiConnectionMgrFileUtil.h"

CHAR16  *mDerPemEncodedSuffix[] = {
  L".cer",
  L".der",
  L".crt",
  L".pem",
  NULL
};

/**
  This code checks if the FileSuffix is one of the possible DER/PEM-encoded certificate suffix.

  @param[in] FileSuffix     The suffix of the input certificate file

  @retval    TRUE           It's a DER/PEM-encoded certificate.
  @retval    FALSE          It's NOT a DER/PEM-encoded certificate.

**/
BOOLEAN
IsDerPemEncodeCertificate (
  IN CONST CHAR16  *FileSuffix
  )
{
  UINTN  Index;

  for (Index = 0; mDerPemEncodedSuffix[Index] != NULL; Index++) {
    if (StrCmp (FileSuffix, mDerPemEncodedSuffix[Index]) == 0) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Read file content into BufferPtr, the size of the allocate buffer
  is *FileSize plus AddtionAllocateSize.

  @param[in]       FileHandle            The file to be read.
  @param[in, out]  BufferPtr             Pointers to the pointer of allocated buffer.
  @param[out]      FileSize              Size of input file
  @param[in]       AddtionAllocateSize   Addtion size the buffer need to be allocated.
                                         In case the buffer need to contain others besides the file content.

  @retval   EFI_SUCCESS                  The file was read into the buffer.
  @retval   EFI_INVALID_PARAMETER        A parameter was invalid.
  @retval   EFI_OUT_OF_RESOURCES         A memory allocation failed.
  @retval   others                       Unexpected error.

**/
EFI_STATUS
ReadFileContent (
  IN      EFI_FILE_HANDLE  FileHandle,
  IN OUT  VOID             **BufferPtr,
  OUT  UINTN               *FileSize,
  IN      UINTN            AddtionAllocateSize
  )
{
  UINTN       BufferSize;
  UINT64      SourceFileSize;
  VOID        *Buffer;
  EFI_STATUS  Status;

  if ((FileHandle == NULL) || (FileSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Buffer = NULL;

  //
  // Get the file size
  //
  Status = FileHandle->SetPosition (FileHandle, (UINT64)-1);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = FileHandle->GetPosition (FileHandle, &SourceFileSize);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = FileHandle->SetPosition (FileHandle, 0);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  BufferSize = (UINTN)SourceFileSize + AddtionAllocateSize;
  Buffer     =  AllocateZeroPool (BufferSize);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  BufferSize = (UINTN)SourceFileSize;
  *FileSize  = BufferSize;

  Status = FileHandle->Read (FileHandle, &BufferSize, Buffer);
  if (EFI_ERROR (Status) || (BufferSize != *FileSize)) {
    FreePool (Buffer);
    Buffer = NULL;
    Status = EFI_BAD_BUFFER_SIZE;
    goto ON_EXIT;
  }

ON_EXIT:

  *BufferPtr = Buffer;
  return Status;
}

/**
  This function converts an input device structure to a Unicode string.

  @param[in] DevPath                  A pointer to the device path structure.

  @return A new allocated Unicode string that represents the device path.

**/
CHAR16 *
EFIAPI
DevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevPath
  )
{
  return ConvertDevicePathToText (
           DevPath,
           FALSE,
           TRUE
           );
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
  IN   EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  CHAR16  *String;
  CHAR16  *MatchString;
  CHAR16  *LastMatch;
  CHAR16  *FileName;
  UINTN   Length;

  ASSERT (DevicePath != NULL);

  String = DevicePathToStr (DevicePath);
  if (String == NULL) {
    return NULL;
  }

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
  Update the form base on the selected file.

  @param[in]  Private             The pointer to the global private data structure.
  @param[in]  FilePath            Point to the file path.
  @param[in]  FormId              The form needs to display.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.

**/
BOOLEAN
UpdatePage (
  IN  WIFI_MGR_PRIVATE_DATA     *Private,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN  EFI_FORM_ID               FormId
  )
{
  CHAR16      *FileName;
  EFI_STATUS  Status;

  FileName = NULL;

  if (FilePath != NULL) {
    FileName = ExtractFileNameFromDevicePath (FilePath);
  }

  if (FileName == NULL) {
    //
    // FileName = NULL has two cases:
    // 1. FilePath == NULL, not select file.
    // 2. FilePath != NULL, but ExtractFileNameFromDevicePath return NULL not enough memory resource.
    // In these two case, no need to update the form, and exit the caller function.
    //
    return TRUE;
  }

  //
  // Close the previous file handle before open a new one.
  //
  if (Private->FileContext->FHandle != NULL) {
    Private->FileContext->FHandle->Close (Private->FileContext->FHandle);
  }

  Private->FileContext->FHandle = NULL;

  Status = EfiOpenFileByDevicePath (
             &FilePath,
             &Private->FileContext->FHandle,
             EFI_FILE_MODE_READ,
             0
             );
  if (EFI_ERROR (Status)) {
    if (FormId == FORMID_ENROLL_CERT) {
      HiiSetString (
        Private->RegisteredHandle,
        STRING_TOKEN (STR_EAP_ENROLLED_CERT_NAME),
        L"",
        NULL
        );
    } else if (FormId == FORMID_ENROLL_PRIVATE_KEY) {
      HiiSetString (
        Private->RegisteredHandle,
        STRING_TOKEN (STR_EAP_ENROLLED_PRIVATE_KEY_NAME),
        L"",
        NULL
        );
    }
  } else {
    if (Private->FileContext->FileName != NULL) {
      FreePool (Private->FileContext->FileName);
      Private->FileContext->FileName = NULL;
    }

    Private->FileContext->FileName = FileName;

    if (FormId == FORMID_ENROLL_CERT) {
      HiiSetString (
        Private->RegisteredHandle,
        STRING_TOKEN (STR_EAP_ENROLLED_CERT_NAME),
        FileName,
        NULL
        );
    } else if (FormId == FORMID_ENROLL_PRIVATE_KEY) {
      HiiSetString (
        Private->RegisteredHandle,
        STRING_TOKEN (STR_EAP_ENROLLED_PRIVATE_KEY_NAME),
        FileName,
        NULL
        );
    }
  }

  return TRUE;
}

/**
  Update the CA form base on the input file path info.

  @param[in]  Private             The pointer to the global private data structure.
  @param[in]  FilePath            Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.

**/
BOOLEAN
UpdateCAFromFile (
  IN  WIFI_MGR_PRIVATE_DATA     *Private,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  return UpdatePage (Private, FilePath, FORMID_ENROLL_CERT);
}

/**
  Update the Private Key form base on the input file path info.

  @param[in]  Private             The pointer to the global private data structure.
  @param[in]  FilePath            Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.

**/
BOOLEAN
UpdatePrivateKeyFromFile (
  IN  WIFI_MGR_PRIVATE_DATA     *Private,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  return UpdatePage (Private, FilePath, FORMID_ENROLL_PRIVATE_KEY);
}
