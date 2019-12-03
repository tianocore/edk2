/** @file
  The Miscellaneous Routines for TlsDxe driver.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TlsImpl.h"

/**
  Encrypt the message listed in fragment.

  @param[in]       TlsInstance    The pointer to the TLS instance.
  @param[in, out]  FragmentTable  Pointer to a list of fragment.
                                  On input these fragments contain the TLS header and
                                  plain text TLS payload;
                                  On output these fragments contain the TLS header and
                                  cipher text TLS payload.
  @param[in]       FragmentCount  Number of fragment.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES    Can't allocate memory resources.
  @retval EFI_ABORTED             TLS session state is incorrect.
  @retval Others                  Other errors as indicated.
**/
EFI_STATUS
TlsEncryptPacket (
  IN     TLS_INSTANCE                  *TlsInstance,
  IN OUT EFI_TLS_FRAGMENT_DATA         **FragmentTable,
  IN     UINT32                        *FragmentCount
  )
{
  EFI_STATUS          Status;
  UINTN               Index;
  UINT32              BytesCopied;
  UINT32              BufferInSize;
  UINT8               *BufferIn;
  UINT8               *BufferInPtr;
  TLS_RECORD_HEADER   *RecordHeaderIn;
  UINT16              ThisPlainMessageSize;
  TLS_RECORD_HEADER   *TempRecordHeader;
  UINT16              ThisMessageSize;
  UINT32              BufferOutSize;
  UINT8               *BufferOut;
  UINT32              RecordCount;
  INTN                Ret;

  Status           = EFI_SUCCESS;
  BytesCopied      = 0;
  BufferInSize     = 0;
  BufferIn         = NULL;
  BufferInPtr      = NULL;
  RecordHeaderIn   = NULL;
  TempRecordHeader = NULL;
  BufferOutSize    = 0;
  BufferOut        = NULL;
  RecordCount      = 0;
  Ret              = 0;

  //
  // Calculate the size according to the fragment table.
  //
  for (Index = 0; Index < *FragmentCount; Index++) {
    BufferInSize += (*FragmentTable)[Index].FragmentLength;
  }

  //
  // Allocate buffer for processing data.
  //
  BufferIn = AllocateZeroPool (BufferInSize);
  if (BufferIn == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ERROR;
  }

  //
  // Copy all TLS plain record header and payload into BufferIn.
  //
  for (Index = 0; Index < *FragmentCount; Index++) {
    CopyMem (
      (BufferIn + BytesCopied),
      (*FragmentTable)[Index].FragmentBuffer,
      (*FragmentTable)[Index].FragmentLength
      );
    BytesCopied += (*FragmentTable)[Index].FragmentLength;
  }

  //
  // Count TLS record number.
  //
  BufferInPtr = BufferIn;
  while ((UINTN) BufferInPtr < (UINTN) BufferIn + BufferInSize) {
    RecordHeaderIn = (TLS_RECORD_HEADER *) BufferInPtr;
    if (RecordHeaderIn->ContentType != TlsContentTypeApplicationData || RecordHeaderIn->Length > TLS_PLAINTEXT_RECORD_MAX_PAYLOAD_LENGTH) {
      Status = EFI_INVALID_PARAMETER;
      goto ERROR;
    }
    BufferInPtr += TLS_RECORD_HEADER_LENGTH + RecordHeaderIn->Length;
    RecordCount ++;
  }

  //
  // Allocate enough buffer to hold TLS Ciphertext.
  //
  BufferOut = AllocateZeroPool (RecordCount * (TLS_RECORD_HEADER_LENGTH + TLS_CIPHERTEXT_RECORD_MAX_PAYLOAD_LENGTH));
  if (BufferOut == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ERROR;
  }

  //
  // Parsing buffer. Received packet may have multiple TLS record messages.
  //
  BufferInPtr = BufferIn;
  TempRecordHeader = (TLS_RECORD_HEADER *) BufferOut;
  while ((UINTN) BufferInPtr < (UINTN) BufferIn + BufferInSize) {
    RecordHeaderIn = (TLS_RECORD_HEADER *) BufferInPtr;

    ThisPlainMessageSize = RecordHeaderIn->Length;

    TlsWrite (TlsInstance->TlsConn, (UINT8 *) (RecordHeaderIn + 1), ThisPlainMessageSize);

    Ret = TlsCtrlTrafficOut (TlsInstance->TlsConn, (UINT8 *)(TempRecordHeader), TLS_RECORD_HEADER_LENGTH + TLS_CIPHERTEXT_RECORD_MAX_PAYLOAD_LENGTH);

    if (Ret > 0) {
      ThisMessageSize = (UINT16) Ret;
    } else {
      //
      // No data was successfully encrypted, continue to encrypt other messages.
      //
      DEBUG ((EFI_D_WARN, "TlsEncryptPacket: No data read from TLS object.\n"));

      ThisMessageSize = 0;
    }

    BufferOutSize += ThisMessageSize;

    BufferInPtr += TLS_RECORD_HEADER_LENGTH + ThisPlainMessageSize;
    TempRecordHeader = (TLS_RECORD_HEADER *)((UINT8 *)TempRecordHeader + ThisMessageSize);
  }

  FreePool (BufferIn);
  BufferIn = NULL;

  //
  // The caller will be responsible to handle the original fragment table.
  //
  *FragmentTable = AllocateZeroPool (sizeof (EFI_TLS_FRAGMENT_DATA));
  if (*FragmentTable == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ERROR;
  }

  (*FragmentTable)[0].FragmentBuffer  = BufferOut;
  (*FragmentTable)[0].FragmentLength  = BufferOutSize;
  *FragmentCount                      = 1;

  return Status;

ERROR:

  if (BufferIn != NULL) {
    FreePool (BufferIn);
    BufferIn = NULL;
  }

  if (BufferOut != NULL) {
    FreePool (BufferOut);
    BufferOut = NULL;
  }

  return Status;
}

/**
  Decrypt the message listed in fragment.

  @param[in]       TlsInstance    The pointer to the TLS instance.
  @param[in, out]  FragmentTable  Pointer to a list of fragment.
                                  On input these fragments contain the TLS header and
                                  cipher text TLS payload;
                                  On output these fragments contain the TLS header and
                                  plain text TLS payload.
  @param[in]       FragmentCount  Number of fragment.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES    Can't allocate memory resources.
  @retval EFI_ABORTED             TLS session state is incorrect.
  @retval Others                  Other errors as indicated.
**/
EFI_STATUS
TlsDecryptPacket (
  IN     TLS_INSTANCE                  *TlsInstance,
  IN OUT EFI_TLS_FRAGMENT_DATA         **FragmentTable,
  IN     UINT32                        *FragmentCount
  )
{
  EFI_STATUS          Status;
  UINTN               Index;
  UINT32              BytesCopied;
  UINT8               *BufferIn;
  UINT32              BufferInSize;
  UINT8               *BufferInPtr;
  TLS_RECORD_HEADER   *RecordHeaderIn;
  UINT16              ThisCipherMessageSize;
  TLS_RECORD_HEADER   *TempRecordHeader;
  UINT16              ThisPlainMessageSize;
  UINT8               *BufferOut;
  UINT32              BufferOutSize;
  UINT32              RecordCount;
  INTN                Ret;

  Status           = EFI_SUCCESS;
  BytesCopied      = 0;
  BufferIn         = NULL;
  BufferInSize     = 0;
  BufferInPtr      = NULL;
  RecordHeaderIn   = NULL;
  TempRecordHeader = NULL;
  BufferOut        = NULL;
  BufferOutSize    = 0;
  RecordCount      = 0;
  Ret              = 0;

  //
  // Calculate the size according to the fragment table.
  //
  for (Index = 0; Index < *FragmentCount; Index++) {
    BufferInSize += (*FragmentTable)[Index].FragmentLength;
  }

  //
  // Allocate buffer for processing data
  //
  BufferIn = AllocateZeroPool (BufferInSize);
  if (BufferIn == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ERROR;
  }

  //
  // Copy all TLS plain record header and payload to BufferIn
  //
  for (Index = 0; Index < *FragmentCount; Index++) {
    CopyMem (
      (BufferIn + BytesCopied),
      (*FragmentTable)[Index].FragmentBuffer,
      (*FragmentTable)[Index].FragmentLength
      );
    BytesCopied += (*FragmentTable)[Index].FragmentLength;
  }

  //
  // Count TLS record number.
  //
  BufferInPtr = BufferIn;
  while ((UINTN) BufferInPtr < (UINTN) BufferIn + BufferInSize) {
    RecordHeaderIn = (TLS_RECORD_HEADER *) BufferInPtr;
    if (RecordHeaderIn->ContentType != TlsContentTypeApplicationData || NTOHS (RecordHeaderIn->Length) > TLS_CIPHERTEXT_RECORD_MAX_PAYLOAD_LENGTH) {
      Status = EFI_INVALID_PARAMETER;
      goto ERROR;
    }
    BufferInPtr += TLS_RECORD_HEADER_LENGTH + NTOHS (RecordHeaderIn->Length);
    RecordCount ++;
  }

  //
  // Allocate enough buffer to hold TLS Plaintext.
  //
  BufferOut = AllocateZeroPool (RecordCount * (TLS_RECORD_HEADER_LENGTH + TLS_PLAINTEXT_RECORD_MAX_PAYLOAD_LENGTH));
  if (BufferOut == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ERROR;
  }

  //
  // Parsing buffer. Received packet may have multiple TLS record messages.
  //
  BufferInPtr = BufferIn;
  TempRecordHeader = (TLS_RECORD_HEADER *) BufferOut;
  while ((UINTN) BufferInPtr < (UINTN) BufferIn + BufferInSize) {
    RecordHeaderIn = (TLS_RECORD_HEADER *) BufferInPtr;

    ThisCipherMessageSize = NTOHS (RecordHeaderIn->Length);

    Ret = TlsCtrlTrafficIn (TlsInstance->TlsConn, (UINT8 *) (RecordHeaderIn), TLS_RECORD_HEADER_LENGTH + ThisCipherMessageSize);
    if (Ret != TLS_RECORD_HEADER_LENGTH + ThisCipherMessageSize) {
      TlsInstance->TlsSessionState = EfiTlsSessionError;
      Status = EFI_ABORTED;
      goto ERROR;
    }

    Ret = 0;
    Ret = TlsRead (TlsInstance->TlsConn, (UINT8 *) (TempRecordHeader + 1), TLS_PLAINTEXT_RECORD_MAX_PAYLOAD_LENGTH);

    if (Ret > 0) {
      ThisPlainMessageSize = (UINT16) Ret;
    } else {
      //
      // No data was successfully decrypted, continue to decrypt other messages.
      //
      DEBUG ((EFI_D_WARN, "TlsDecryptPacket: No data read from TLS object.\n"));

      ThisPlainMessageSize = 0;
    }

    CopyMem (TempRecordHeader, RecordHeaderIn, TLS_RECORD_HEADER_LENGTH);
    TempRecordHeader->Length = ThisPlainMessageSize;
    BufferOutSize += TLS_RECORD_HEADER_LENGTH + ThisPlainMessageSize;

    BufferInPtr += TLS_RECORD_HEADER_LENGTH + ThisCipherMessageSize;
    TempRecordHeader = (TLS_RECORD_HEADER *)((UINT8 *)TempRecordHeader + TLS_RECORD_HEADER_LENGTH + ThisPlainMessageSize);
  }

  FreePool (BufferIn);
  BufferIn = NULL;

  //
  // The caller will be responsible to handle the original fragment table
  //
  *FragmentTable = AllocateZeroPool (sizeof (EFI_TLS_FRAGMENT_DATA));
  if (*FragmentTable == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ERROR;
  }

  (*FragmentTable)[0].FragmentBuffer  = BufferOut;
  (*FragmentTable)[0].FragmentLength  = BufferOutSize;
  *FragmentCount                      = 1;

  return Status;

ERROR:

  if (BufferIn != NULL) {
    FreePool (BufferIn);
    BufferIn = NULL;
  }

  if (BufferOut != NULL) {
    FreePool (BufferOut);
    BufferOut = NULL;
  }

  return Status;
}

