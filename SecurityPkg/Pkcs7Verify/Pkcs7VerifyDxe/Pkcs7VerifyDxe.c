/** @file
  Pkcs7Verify Driver to produce the UEFI PKCS7 Verification Protocol.

  The driver will produce the UEFI PKCS7 Verification Protocol which is used to
  verify data signed using PKCS7 structure. The PKCS7 data to be verified must
  be ASN.1 (DER) encoded.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseCryptLib.h>
#include <Protocol/Pkcs7Verify.h>

#define MAX_DIGEST_SIZE  SHA512_DIGEST_SIZE

/**
  Calculates the hash of the given data based on the specified hash GUID.

  @param[in]  Data      Pointer to the data buffer to be hashed.
  @param[in]  DataSize  The size of data buffer in bytes.
  @param[in]  CertGuid  The GUID to identify the hash algorithm to be used.
  @param[out] HashValue Pointer to a buffer that receives the hash result.

  @retval TRUE          Data hash calculation succeeded.
  @retval FALSE         Data hash calculation failed.

**/
BOOLEAN
CalculateDataHash (
  IN  VOID               *Data,
  IN  UINTN              DataSize,
  IN  EFI_GUID           *CertGuid,
  OUT UINT8              *HashValue
  )
{
  BOOLEAN  Status;
  VOID     *HashCtx;
  UINTN    CtxSize;

  Status  = FALSE;
  HashCtx = NULL;

  if (CompareGuid (CertGuid, &gEfiCertSha1Guid)) {
    //
    // SHA-1 Hash
    //
    CtxSize = Sha1GetContextSize ();
    HashCtx = AllocatePool (CtxSize);
    if (HashCtx == NULL) {
      goto _Exit;
    }
    Status = Sha1Init   (HashCtx);
    Status = Sha1Update (HashCtx, Data, DataSize);
    Status = Sha1Final  (HashCtx, HashValue);

  } else if (CompareGuid (CertGuid, &gEfiCertSha256Guid)) {
    //
    // SHA256 Hash
    //
    CtxSize = Sha256GetContextSize ();
    HashCtx = AllocatePool (CtxSize);
    if (HashCtx == NULL) {
      goto _Exit;
    }
    Status = Sha256Init   (HashCtx);
    Status = Sha256Update (HashCtx, Data, DataSize);
    Status = Sha256Final  (HashCtx, HashValue);

  } else if (CompareGuid (CertGuid, &gEfiCertSha384Guid)) {
    //
    // SHA384 Hash
    //
    CtxSize = Sha384GetContextSize ();
    HashCtx = AllocatePool (CtxSize);
    if (HashCtx == NULL) {
      goto _Exit;
    }
    Status = Sha384Init   (HashCtx);
    Status = Sha384Update (HashCtx, Data, DataSize);
    Status = Sha384Final  (HashCtx, HashValue);

  } else if (CompareGuid (CertGuid, &gEfiCertSha512Guid)) {
    //
    // SHA512 Hash
    //
    CtxSize = Sha512GetContextSize ();
    HashCtx = AllocatePool (CtxSize);
    if (HashCtx == NULL) {
      goto _Exit;
    }
    Status = Sha512Init   (HashCtx);
    Status = Sha512Update (HashCtx, Data, DataSize);
    Status = Sha512Final  (HashCtx, HashValue);
  }

_Exit:
  if (HashCtx != NULL) {
    FreePool (HashCtx);
  }

  return Status;
}

/**
  Check whether the hash of data content is revoked by the revocation database.

  @param[in]  Content       Pointer to the content buffer that is searched for.
  @param[in]  ContentSize   The size of data content in bytes.
  @param[in]  RevokedDb     Pointer to a list of pointers to EFI_SIGNATURE_LIST
                            structure which contains list of X.509 certificates
                            of revoked signers and revoked content hashes.

  @return TRUE   The matched content hash is found in the revocation database.
  @return FALSE  The matched content hash is not found in the revocation database.

**/
BOOLEAN
IsContentHashRevoked (
  IN  UINT8              *Content,
  IN  UINTN              ContentSize,
  IN  EFI_SIGNATURE_LIST **RevokedDb
  )
{
  EFI_SIGNATURE_LIST  *SigList;
  EFI_SIGNATURE_DATA  *SigData;
  UINTN               Index;
  UINT8               HashVal[MAX_DIGEST_SIZE];
  UINTN               EntryIndex;
  UINTN               EntryCount;
  BOOLEAN             Status;

  if (RevokedDb == NULL) {
    return FALSE;
  }

  Status = FALSE;
  //
  // Check if any hash matching content hash can be found in RevokedDB
  //
  for (Index = 0; ; Index++) {
    SigList = (EFI_SIGNATURE_LIST *)(RevokedDb[Index]);

    //
    // The list is terminated by a NULL pointer.
    //
    if (SigList == NULL) {
      break;
    }

    //
    // Calculate the digest of supplied data based on the signature hash type.
    //
    if (!CalculateDataHash (Content, ContentSize, &SigList->SignatureType, HashVal)) {
      //
      // Un-matched Hash GUID or other failure.
      //
      continue;
    }

    //
    // Search the signature database to search the revoked content hash
    //
    SigData    = (EFI_SIGNATURE_DATA *) ((UINT8 *) SigList + sizeof (EFI_SIGNATURE_LIST) +
                                        SigList->SignatureHeaderSize);
    EntryCount = (SigList->SignatureListSize - SigList->SignatureHeaderSize -
                 sizeof (EFI_SIGNATURE_LIST)) / SigList->SignatureSize;
    for (EntryIndex = 0; EntryIndex < EntryCount; EntryIndex++) {
      //
      // Compare Data Hash with Signature Data
      //
      if (CompareMem (SigData->SignatureData, HashVal, (SigList->SignatureSize - sizeof (EFI_GUID))) == 0) {
        Status = TRUE;
        goto _Exit;
      }

      SigData = (EFI_SIGNATURE_DATA *) ((UINT8 *) SigData + SigList->SignatureSize);
    }
  }

_Exit:
  return Status;
}

/**
  Check whether the hash of an given certificate is revoked by the revocation database.

  @param[in]  Certificate     Pointer to the certificate that is searched for.
  @param[in]  CertSize        Size of certificate in bytes.
  @param[in]  RevokedDb       Pointer to a list of pointers to EFI_SIGNATURE_LIST
                              structures which contains list of X.509 certificate
                              of revoked signers and revoked content hashes.
  @param[out] RevocationTime  Return the time that the certificate was revoked.

  @return TRUE   The certificate hash is found in the revocation database.
  @return FALSE  The certificate hash is not found in the revocation database.

**/
BOOLEAN
IsCertHashRevoked (
  IN  UINT8              *Certificate,
  IN  UINTN              CertSize,
  IN  EFI_SIGNATURE_LIST **RevokedDb,
  OUT EFI_TIME           *RevocationTime
  )
{
  BOOLEAN             Status;
  EFI_SIGNATURE_LIST  *SigList;
  EFI_SIGNATURE_DATA  *SigData;
  UINT8               *TBSCert;
  UINTN               TBSCertSize;
  UINTN               Index;
  UINTN               EntryIndex;
  UINTN               EntryCount;
  UINT8               CertHashVal[MAX_DIGEST_SIZE];

  if ((RevocationTime == NULL) || (RevokedDb == NULL)) {
    return FALSE;
  }

  //
  // Retrieve the TBSCertificate from the X.509 Certificate for hash calculation
  //
  if (!X509GetTBSCert (Certificate, CertSize, &TBSCert, &TBSCertSize)) {
    return FALSE;
  }

  Status = FALSE;
  for (Index = 0; ; Index++) {

    SigList = (EFI_SIGNATURE_LIST *)(RevokedDb[Index]);
    //
    // The list is terminated by a NULL pointer.
    //
    if (SigList == NULL) {
      break;
    }

    //
    // Determine Hash Algorithm based on the entry type in revocation database, and
    // calculate the certificate hash.
    //
    if (CompareGuid (&SigList->SignatureType, &gEfiCertX509Sha256Guid)) {
      Status = CalculateDataHash (TBSCert, TBSCertSize, &gEfiCertSha256Guid, CertHashVal);

    } else if (CompareGuid (&SigList->SignatureType, &gEfiCertX509Sha384Guid)) {
      Status = CalculateDataHash (TBSCert, TBSCertSize, &gEfiCertSha384Guid, CertHashVal);

    } else if (CompareGuid (&SigList->SignatureType, &gEfiCertX509Sha512Guid)) {
      Status = CalculateDataHash (TBSCert, TBSCertSize, &gEfiCertSha512Guid, CertHashVal);

    } else {
      //
      // Un-matched Cert Hash GUID
      //
      continue;
    }

    if (!Status) {
      continue;
    }

    SigData    = (EFI_SIGNATURE_DATA *) ((UINT8 *) SigList + sizeof (EFI_SIGNATURE_LIST) +
                                      SigList->SignatureHeaderSize);
    EntryCount = (SigList->SignatureListSize - SigList->SignatureHeaderSize -
                  sizeof (EFI_SIGNATURE_LIST)) / SigList->SignatureSize;
    for (EntryIndex = 0; EntryIndex < EntryCount; Index++) {
      //
      // Check if the Certificate Hash is revoked.
      //
      if (CompareMem (SigData->SignatureData, CertHashVal,
                      SigList->SignatureSize - sizeof (EFI_GUID) - sizeof (EFI_TIME)) == 0) {
        Status = TRUE;
        //
        // Return the revocation time of this revoked certificate.
        //
        CopyMem (
          RevocationTime,
          (EFI_TIME *)((UINT8 *)SigData + SigList->SignatureSize - sizeof (EFI_TIME)),
          sizeof (EFI_TIME)
          );
        goto _Exit;
      }

      SigData = (EFI_SIGNATURE_DATA *) ((UINT8 *) SigData + SigList->SignatureSize);
    }
  }

_Exit:
  return Status;
}

/**
  Check if the given time value is zero.

  @param[in]  Time      Pointer of a time value.

  @retval     TRUE      The Time is Zero.
  @retval     FALSE     The Time is not Zero.

**/
BOOLEAN
IsTimeZero (
  IN EFI_TIME            *Time
  )
{
  if ((Time->Year == 0) && (Time->Month == 0) &&  (Time->Day == 0) &&
      (Time->Hour == 0) && (Time->Minute == 0) && (Time->Second == 0)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Check whether the timestamp is valid by comparing the signing time and the revocation time.

  @param SigningTime     Pointer to the signing time.
  @param RevocationTime  Pointer to the revocation time.

  @retval  TRUE          The SigningTime is not later than the RevocationTime.
  @retval  FALSE         The SigningTime is later than the RevocationTime.

**/
BOOLEAN
CompareTimestamp (
  IN EFI_TIME            *SigningTime,
  IN EFI_TIME            *RevocationTime
  )
{
  if (SigningTime->Year != RevocationTime->Year) {
    return (BOOLEAN) (SigningTime->Year < RevocationTime->Year);
  } else if (SigningTime->Month != RevocationTime->Month) {
    return (BOOLEAN) (SigningTime->Month < RevocationTime->Month);
  } else if (SigningTime->Day != RevocationTime->Day) {
    return (BOOLEAN) (SigningTime->Day < RevocationTime->Day);
  } else if (SigningTime->Hour != RevocationTime->Hour) {
    return (BOOLEAN) (SigningTime->Hour < RevocationTime->Hour);
  } else if (SigningTime->Minute != RevocationTime->Minute) {
    return (BOOLEAN) (SigningTime->Minute < RevocationTime->Minute);
  }

  return (BOOLEAN) (SigningTime->Second <= RevocationTime->Second);
}

/**
  Check whether the timestamp signature embedded in PKCS7 signedData is valid and
  the signing time is also earlier than the revocation time.

  @param[in]  SignedData        Pointer to the PKCS#7 signedData.
  @param[in]  SignedDataSize    Size of SignedData in bytes.
  @param[in]  TimeStampDb       Pointer to a list of pointers to EFI_SIGNATURE_LIST
                                structures which is used to pass a list of X.509
                                certificates of trusted timestamp signers.
  @param[in]  RevocationTime    The time that the certificate was revoked.

  @retval TRUE      Timestamp signature is valid and the signing time is no later
                    than the revocation time.
  @retval FALSE     Timestamp signature is not valid or the signing time is later
                    than the revocation time.

**/
BOOLEAN
IsValidTimestamp (
  IN UINT8               *SignedData,
  IN UINTN               SignedDataSize,
  IN EFI_SIGNATURE_LIST  **TimeStampDb,
  IN EFI_TIME            *RevocationTime
  )
{
  BOOLEAN             Status;
  EFI_SIGNATURE_LIST  *SigList;
  EFI_SIGNATURE_DATA  *SigData;
  UINT8               *TsaCert;
  UINTN               TsaCertSize;
  UINTN               Index;
  EFI_TIME            SigningTime;

  //
  // If no supplied database for verification or RevocationTime is zero,
  // the certificate shall be considered to always be revoked.
  //
  if ((TimeStampDb == NULL) || (IsTimeZero (RevocationTime))) {
    return FALSE;
  }

  Status = FALSE;
  //
  // RevocationTime is non-zero, the certificate should be considered to be revoked
  // from that time and onwards.
  //
  for (Index = 0; ; Index++) {
    SigList = (EFI_SIGNATURE_LIST *) (TimeStampDb[Index]);

    //
    // The list is terminated by a NULL pointer.
    //
    if (SigList == NULL) {
      break;
    }

    //
    // Ignore any non-X509-format entry in the list
    //
    if (!CompareGuid (&SigList->SignatureType, &gEfiCertX509Guid)) {
      continue;
    }


    SigData = (EFI_SIGNATURE_DATA *) ((UINT8 *) SigList + sizeof (EFI_SIGNATURE_LIST) +
                                      SigList->SignatureHeaderSize);
    TsaCert     = SigData->SignatureData;
    TsaCertSize = SigList->SignatureSize - sizeof (EFI_GUID);

    //
    // Each TSA Certificate will normally be in a seperate EFI_SIGNATURE_LIST
    // Leverage ImageTimestampVerify interface for Timestamp counterSignature Verification
    //
    if (ImageTimestampVerify (SignedData, SignedDataSize, TsaCert, TsaCertSize, &SigningTime)) {
      //
      // The signer signature is valid only when the signing time is earlier than revocation time.
      //
      if (CompareTimestamp (&SigningTime, RevocationTime)) {
        Status = TRUE;
        break;
      }
    }
  }

  return Status;
}

/**
  Check whether the PKCS7 signedData is revoked by verifying with the revoked
  certificates database, and if the signedData is timestamped, the embedded timestamp
  couterSignature will be checked with the supplied timestamp database.

  @param[in]  SignedData      Pointer to buffer containing ASN.1 DER-encoded PKCS7
                              signature.
  @param[in]  SignedDataSize  The size of SignedData buffer in bytes.
  @param[in]  InData          Pointer to the buffer containing the raw message data
                              previously signed and to be verified.
  @param[in]  InDataSize      The size of InData buffer in bytes.
  @param[in]  RevokedDb       Pointer to a list of pointers to EFI_SIGNATURE_LIST
                              structure which contains list of X.509 certificates
                              of revoked signers and revoked content hashes.
  @param[in]  TimeStampDb     Pointer to a list of pointers to EFI_SIGNATURE_LIST
                              structures which is used to pass a list of X.509
                              certificates of trusted timestamp signers.

  @retval  EFI_SUCCESS             The PKCS7 signedData is revoked.
  @retval  EFI_SECURITY_VIOLATION  Fail to verify the signature in PKCS7 signedData.
  @retval  EFI_INVALID_PARAMETER   SignedData is NULL or SignedDataSize is zero.
                                   AllowedDb is NULL.
                                   Content is not NULL and ContentSize is NULL.
  @retval  EFI_NOT_FOUND           Content not found because InData is NULL and no
                                   content embedded in PKCS7 signedData.
  @retval  EFI_UNSUPPORTED         The PKCS7 signedData was not correctly formatted.

**/
EFI_STATUS
P7CheckRevocation (
  IN UINT8                *SignedData,
  IN UINTN                SignedDataSize,
  IN UINT8                *InData,
  IN UINTN                InDataSize,
  IN EFI_SIGNATURE_LIST   **RevokedDb,
  IN EFI_SIGNATURE_LIST   **TimeStampDb
  )
{
  EFI_STATUS          Status;
  EFI_SIGNATURE_LIST  *SigList;
  EFI_SIGNATURE_DATA  *SigData;
  UINT8               *RevokedCert;
  UINTN               RevokedCertSize;
  UINTN               Index;
  UINT8               *CertBuffer;
  UINTN               BufferLength;
  UINT8               *TrustedCert;
  UINTN               TrustedCertLength;
  UINT8               CertNumber;
  UINT8               *CertPtr;
  UINT8               *Cert;
  UINTN               CertSize;
  EFI_TIME            RevocationTime;

  Status          = EFI_UNSUPPORTED;
  SigData         = NULL;
  RevokedCert     = NULL;
  RevokedCertSize = 0;
  CertBuffer      = NULL;
  TrustedCert     = NULL;

  //
  // The signedData is revoked if the hash of content existed in RevokedDb
  //
  if (IsContentHashRevoked (InData, InDataSize, RevokedDb)) {
    Status = EFI_SUCCESS;
    goto _Exit;
  }

  //
  // Check if the signer's certificate can be found in Revoked database
  //
  for (Index = 0; ; Index++) {
    SigList = (EFI_SIGNATURE_LIST *)(RevokedDb[Index]);

    //
    // The list is terminated by a NULL pointer.
    //
    if (SigList == NULL) {
      break;
    }

    //
    // Ignore any non-X509-format entry in the list.
    //
    if (!CompareGuid (&SigList->SignatureType, &gEfiCertX509Guid)) {
      continue;
    }

    SigData = (EFI_SIGNATURE_DATA *) ((UINT8 *) SigList + sizeof (EFI_SIGNATURE_LIST) +
                                      SigList->SignatureHeaderSize);

    RevokedCert     = SigData->SignatureData;
    RevokedCertSize = SigList->SignatureSize - sizeof (EFI_GUID);

    //
    // Verifying the PKCS#7 SignedData with the revoked certificate in RevokedDb
    //
    if (Pkcs7Verify (SignedData, SignedDataSize, RevokedCert, RevokedCertSize, InData, InDataSize)) {
      //
      // The signedData was verified by one entry in Revoked Database
      //
      Status = EFI_SUCCESS;
      break;
    }
  }

  if (!EFI_ERROR (Status)) {
    //
    // The signedData was revoked, since it was hit by RevokedDb
    //
    goto _Exit;
  }

  //
  // Now we will continue to check the X.509 Certificate Hash & Possible Timestamp
  //
  if ((TimeStampDb == NULL) || (*TimeStampDb == NULL)) {
    goto _Exit;
  }

  Pkcs7GetSigners (SignedData, SignedDataSize, &CertBuffer, &BufferLength, &TrustedCert, &TrustedCertLength);
  if ((BufferLength == 0) || (CertBuffer == NULL)) {
    Status = EFI_SUCCESS;
    goto _Exit;
  }

  //
  // Check if any hash of certificates embedded in P7 data is in the revoked database.
  //
  CertNumber = (UINT8) (*CertBuffer);
  CertPtr    = CertBuffer + 1;
  for (Index = 0; Index < CertNumber; Index++) {
    //
    // Retrieve the Certificate data
    //
    CertSize = (UINTN) ReadUnaligned32 ((UINT32 *) CertPtr);
    Cert     = (UINT8 *)CertPtr + sizeof (UINT32);

    if (IsCertHashRevoked (Cert, CertSize, RevokedDb, &RevocationTime)) {
      //
      // Check the timestamp signature and signing time to determine if p7 data can be trusted.
      //
      Status = EFI_SUCCESS;
      if (IsValidTimestamp (SignedData, SignedDataSize, TimeStampDb, &RevocationTime)) {
        //
        // Use EFI_NOT_READY to identify the P7Data is not reovked, because the timestamping
        // occured prior to the time of certificate revocation.
        //
        Status = EFI_NOT_READY;
      }

      goto _Exit;
    }

    CertPtr = CertPtr + sizeof (UINT32) + CertSize;
  }

_Exit:
  Pkcs7FreeSigners (CertBuffer);
  Pkcs7FreeSigners (TrustedCert);

  return Status;
}

/**
  Check whether the PKCS7 signedData can be verified by the trusted certificates
  database, and return the content of the signedData if requested.

  @param[in]  SignedData      Pointer to buffer containing ASN.1 DER-encoded PKCS7
                              signature.
  @param[in]  SignedDataSize  The size of SignedData buffer in bytes.
  @param[in]  InData          Pointer to the buffer containing the raw message data
                              previously signed and to be verified.
  @param[in]  InDataSize      The size of InData buffer in bytes.
  @param[in]  AllowedDb       Pointer to a list of pointers to EFI_SIGNATURE_LIST
                              structures which contains lists of X.509 certificates
                              of approved signers.
  @param[out] Content         An optional caller-allocated buffer into which the
                              function will copy the content of PKCS7 signedData.
  @param[in,out] ContentSize  On input, points of the size in bytes of the optional
                              buffer Content previously allocated by caller. On output,
                              the value will contain the actual size of the content
                              extracted from the signedData.

  @retval  EFI_SUCCESS             The PKCS7 signedData is trusted.
  @retval  EFI_SECURITY_VIOLATION  Fail to verify the signature in PKCS7 signedData.
  @retval  EFI_INVALID_PARAMETER   SignedData is NULL or SignedDataSize is zero.
                                   AllowedDb is NULL.
                                   Content is not NULL and ContentSize is NULL.
  @retval  EFI_NOT_FOUND           Content not found because InData is NULL and no
                                   content embedded in PKCS7 signedData.
  @retval  EFI_UNSUPPORTED         The PKCS7 signedData was not correctly formatted.
  @retval  EFI_BUFFER_TOO_SMALL    The size of buffer indicated by ContentSize is too
                                   small to hold the content. ContentSize updated to
                                   the required size.

**/
EFI_STATUS
P7CheckTrust (
  IN UINT8               *SignedData,
  IN UINTN               SignedDataSize,
  IN UINT8               *InData,
  IN UINTN               InDataSize,
  IN EFI_SIGNATURE_LIST  **AllowedDb
  )
{
  EFI_STATUS          Status;
  EFI_SIGNATURE_LIST  *SigList;
  EFI_SIGNATURE_DATA  *SigData;
  UINT8               *TrustCert;
  UINTN               TrustCertSize;
  UINTN               Index;

  Status        = EFI_SECURITY_VIOLATION;
  SigData       = NULL;
  TrustCert     = NULL;
  TrustCertSize = 0;

  if (AllowedDb == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Build Certificate Stack with all valid X509 certificates in the supplied
  // Signature List for PKCS7 Verification.
  //
  for (Index = 0; ; Index++) {
    SigList = (EFI_SIGNATURE_LIST *)(AllowedDb[Index]);

    //
    // The list is terminated by a NULL pointer.
    //
    if (SigList == NULL) {
      break;
    }

    //
    // Ignore any non-X509-format entry in the list.
    //
    if (!CompareGuid (&SigList->SignatureType, &gEfiCertX509Guid)) {
      continue;
    }

    SigData = (EFI_SIGNATURE_DATA *) ((UINT8 *) SigList + sizeof (EFI_SIGNATURE_LIST) +
                                      SigList->SignatureHeaderSize);

    TrustCert     = SigData->SignatureData;
    TrustCertSize = SigList->SignatureSize - sizeof (EFI_GUID);

    //
    // Verifying the PKCS#7 SignedData with the trusted certificate from AllowedDb
    //
    if (Pkcs7Verify (SignedData, SignedDataSize, TrustCert, TrustCertSize, InData, InDataSize)) {
      //
      // The SignedData was verified successfully by one entry in Trusted Database
      //
      Status = EFI_SUCCESS;
      break;
    }
  }

  return Status;
}

/**
  Processes a buffer containing binary DER-encoded PKCS7 signature.
  The signed data content may be embedded within the buffer or separated. Function
  verifies the signature of the content is valid and signing certificate was not
  revoked and is contained within a list of trusted signers.

  @param[in]     This                 Pointer to EFI_PKCS7_VERIFY_PROTOCOL instance.
  @param[in]     SignedData           Points to buffer containing ASN.1 DER-encoded PKCS7
                                      signature.
  @param[in]     SignedDataSize       The size of SignedData buffer in bytes.
  @param[in]     InData               In case of detached signature, InData points to
                                      buffer containing the raw message data previously
                                      signed and to be verified by function. In case of
                                      SignedData containing embedded data, InData must be
                                      NULL.
  @param[in]     InDataSize           When InData is used, the size of InData buffer in
                                      bytes. When InData is NULL. This parameter must be
                                      0.
  @param[in]     AllowedDb            Pointer to a list of pointers to EFI_SIGNATURE_LIST
                                      structures. The list is terminated by a null
                                      pointer. The EFI_SIGNATURE_LIST structures contain
                                      lists of X.509 certificates of approved signers.
                                      Function recognizes signer certificates of type
                                      EFI_CERT_X509_GUID. Any hash certificate in AllowedDb
                                      list is ignored by this function. Function returns
                                      success if signer of the buffer is within this list
                                      (and not within RevokedDb). This parameter is
                                      required.
  @param[in]     RevokedDb            Optional pointer to a list of pointers to
                                      EFI_SIGNATURE_LIST structures. The list is terminated
                                      by a null pointer. List of X.509 certificates of
                                      revoked signers and revoked file hashes. Except as
                                      noted in description of TimeStampDb signature
                                      verification will always fail if the signer of the
                                      file or the hash of the data component of the buffer
                                      is in RevokedDb list. This list is optional and
                                      caller may pass Null or pointer to NULL if not
                                      required.
  @param[in]     TimeStampDb          Optional pointer to a list of pointers to
                                      EFI_SIGNATURE_LIST structures. The list is terminated
                                      by a null pointer. This parameter can be used to pass
                                      a list of X.509 certificates of trusted time stamp
                                      signers. This list is optional and caller must pass
                                      Null or pointer to NULL if not required.
  @param[out]    Content              On input, points to an optional caller-allocated
                                      buffer into which the function will copy the content
                                      portion of the file after verification succeeds.
                                      This parameter is optional and if NULL, no copy of
                                      content from file is performed.
  @param[in,out] ContentSize          On input, points to the size in bytes of the optional
                                      buffer Content previously allocated by caller. On
                                      output, if the verification succeeds, the value
                                      referenced by ContentSize will contain the actual
                                      size of the content from signed file. If ContentSize
                                      indicates the caller-allocated buffer is too small
                                      to contain content, an error is returned, and
                                      ContentSize will be updated with the required size.
                                      This parameter must be 0 if Content is Null.

  @retval EFI_SUCCESS                 Content signature was verified against hash of
                                      content, the signer's certificate was not found in
                                      RevokedDb, and was found in AllowedDb or if in signer
                                      is found in both AllowedDb and RevokedDb, the
                                      signing was allowed by reference to TimeStampDb as
                                      described above, and no hash matching content hash
                                      was found in RevokedDb.
  @retval EFI_SECURITY_VIOLATION      The SignedData buffer was correctly formatted but
                                      signer was in RevokedDb or not in AllowedDb. Also
                                      returned if matching content hash found in RevokedDb.
  @retval EFI_COMPROMISED_DATA        Calculated hash differs from signed hash.
  @retval EFI_INVALID_PARAMETER       SignedData is NULL or SignedDataSize is zero.
                                      AllowedDb is NULL.
  @retval EFI_INVALID_PARAMETER       Content is not NULL and ContentSize is NULL.
  @retval EFI_ABORTED                 Unsupported or invalid format in TimeStampDb,
                                      RevokedDb or AllowedDb list contents was detected.
  @retval EFI_NOT_FOUND               Content not found because InData is NULL and no
                                      content embedded in SignedData.
  @retval EFI_UNSUPPORTED             The SignedData buffer was not correctly formatted
                                      for processing by the function.
  @retval EFI_UNSUPPORTED             Signed data embedded in SignedData but InData is not
                                      NULL.
  @retval EFI_BUFFER_TOO_SMALL        The size of buffer indicated by ContentSize is too
                                      small to hold the content. ContentSize updated to
                                      required size.

**/
EFI_STATUS
EFIAPI
VerifyBuffer (
  IN EFI_PKCS7_VERIFY_PROTOCOL    *This,
  IN VOID                         *SignedData,
  IN UINTN                        SignedDataSize,
  IN VOID                         *InData          OPTIONAL,
  IN UINTN                        InDataSize,
  IN EFI_SIGNATURE_LIST           **AllowedDb,
  IN EFI_SIGNATURE_LIST           **RevokedDb      OPTIONAL,
  IN EFI_SIGNATURE_LIST           **TimeStampDb    OPTIONAL,
  OUT VOID                        *Content         OPTIONAL,
  IN OUT UINTN                    *ContentSize
  )
{
  EFI_STATUS  Status;
  UINT8       *AttachedData;
  UINTN       AttachedDataSize;
  UINT8       *DataPtr;
  UINTN       DataSize;

  //
  // Parameters Checking
  //
  if ((SignedData == NULL) || (SignedDataSize == 0) || (AllowedDb == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  if ((Content != NULL) && (ContentSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Try to retrieve the attached content from PKCS7 signedData
  //
  AttachedData     = NULL;
  AttachedDataSize = 0;
  if (!Pkcs7GetAttachedContent (
         SignedData,
         SignedDataSize,
         (VOID **)&AttachedData,
         &AttachedDataSize)) {
    //
    // The SignedData buffer was not correctly formatted for processing
    //
    return EFI_UNSUPPORTED;
  }
  if (AttachedData != NULL) {
    //
    // PKCS7-formatted signedData with attached content; Use the embedded
    // content for verification
    //
    DataPtr  = AttachedData;
    DataSize = AttachedDataSize;

  } else if (InData != NULL) {
    //
    // PKCS7-formatted signedData with detached content; Use the user-supplied
    // input data for verification
    //
    DataPtr  = (UINT8 *)InData;
    DataSize = InDataSize;
  } else {
    //
    // Content not found because InData is NULL and no content attached in SignedData
    //
    Status = EFI_NOT_FOUND;
    goto _Exit;
  }

  Status = EFI_UNSUPPORTED;

  //
  // Verify PKCS7 SignedData with Revoked database
  //
  if (RevokedDb != NULL) {
    Status = P7CheckRevocation (
               SignedData,
               SignedDataSize,
               DataPtr,
               DataSize,
               RevokedDb,
               TimeStampDb
               );
    if (!EFI_ERROR (Status)) {
      //
      // The PKCS7 SignedData is reovked
      //
      Status = EFI_SECURITY_VIOLATION;
      goto _Exit;
    }
  }

  //
  // Verify PKCS7 SignedData with AllowedDB
  //
  Status = P7CheckTrust (
             SignedData,
             SignedDataSize,
             DataPtr,
             DataSize,
             AllowedDb
             );
  if (EFI_ERROR (Status)) {
      //
      // Verification failed with AllowedDb
      //
      goto _Exit;
  }

  //
  // Copy the content portion after verification succeeds
  //
  if (Content != NULL) {
    if (*ContentSize < DataSize) {
      //
      // Caller-allocated buffer is too small to contain content
      //
      *ContentSize = DataSize;
      Status = EFI_BUFFER_TOO_SMALL;
    } else {
      *ContentSize = DataSize;
      CopyMem (Content, DataPtr, DataSize);
    }
  }

_Exit:
  if (AttachedData != NULL) {
    FreePool (AttachedData);
  }

  return Status;
}

/**
  Processes a buffer containing binary DER-encoded detached PKCS7 signature.
  The hash of the signed data content is calculated and passed by the caller. Function
  verifies the signature of the content is valid and signing certificate was not revoked
  and is contained within a list of trusted signers.

  @param[in]     This                 Pointer to EFI_PKCS7_VERIFY_PROTOCOL instance.
  @param[in]     Signature            Points to buffer containing ASN.1 DER-encoded PKCS
                                      detached signature.
  @param[in]     SignatureSize        The size of Signature buffer in bytes.
  @param[in]     InHash               InHash points to buffer containing the caller
                                      calculated hash of the data. The parameter may not
                                      be NULL.
  @param[in]     InHashSize           The size in bytes of InHash buffer.
  @param[in]     AllowedDb            Pointer to a list of pointers to EFI_SIGNATURE_LIST
                                      structures. The list is terminated by a null
                                      pointer. The EFI_SIGNATURE_LIST structures contain
                                      lists of X.509 certificates of approved signers.
                                      Function recognizes signer certificates of type
                                      EFI_CERT_X509_GUID. Any hash certificate in AllowedDb
                                      list is ignored by this function. Function returns
                                      success if signer of the buffer is within this list
                                      (and not within RevokedDb). This parameter is
                                      required.
  @param[in]     RevokedDb            Optional pointer to a list of pointers to
                                      EFI_SIGNATURE_LIST structures. The list is terminated
                                      by a null pointer. List of X.509 certificates of
                                      revoked signers and revoked file hashes. Signature
                                      verification will always fail if the signer of the
                                      file or the hash of the data component of the buffer
                                      is in RevokedDb list. This parameter is optional
                                      and caller may pass Null if not required.
  @param[in]     TimeStampDb          Optional pointer to a list of pointers to
                                      EFI_SIGNATURE_LIST structures. The list is terminated
                                      by a null pointer. This parameter can be used to pass
                                      a list of X.509 certificates of trusted time stamp
                                      counter-signers.

  @retval EFI_SUCCESS                 Signed hash was verified against caller-provided
                                      hash of content, the signer's certificate was not
                                      found in RevokedDb, and was found in AllowedDb or
                                      if in signer is found in both AllowedDb and
                                      RevokedDb, the signing was allowed by reference to
                                      TimeStampDb as described above, and no hash matching
                                      content hash was found in RevokedDb.
  @retval EFI_SECURITY_VIOLATION      The SignedData buffer was correctly formatted but
                                      signer was in RevokedDb or not in AllowedDb. Also
                                      returned if matching content hash found in RevokedDb.
  @retval EFI_COMPROMISED_DATA        Caller provided hash differs from signed hash. Or,
                                      caller and encrypted hash are different sizes.
  @retval EFI_INVALID_PARAMETER       Signature is NULL or SignatureSize is zero. InHash
                                      is NULL or InHashSize is zero. AllowedDb is NULL.
  @retval EFI_ABORTED                 Unsupported or invalid format in TimeStampDb,
                                      RevokedDb or AllowedDb list contents was detected.
  @retval EFI_UNSUPPORTED             The Signature buffer was not correctly formatted
                                      for processing by the function.

**/
EFI_STATUS
EFIAPI
VerifySignature (
  IN EFI_PKCS7_VERIFY_PROTOCOL    *This,
  IN VOID                         *Signature,
  IN UINTN                        SignatureSize,
  IN VOID                         *InHash,
  IN UINTN                        InHashSize,
  IN EFI_SIGNATURE_LIST           **AllowedDb,
  IN EFI_SIGNATURE_LIST           **RevokedDb       OPTIONAL,
  IN EFI_SIGNATURE_LIST           **TimeStampDb     OPTIONAL
  )
{
  //
  // NOTE: Current EDKII-OpenSSL interface cannot support VerifySignature
  //       directly. EFI_UNSUPPORTED is returned in this version.
  //
  return EFI_UNSUPPORTED;
}

//
// The PKCS7 Verification Protocol
//
EFI_PKCS7_VERIFY_PROTOCOL mPkcs7Verify = {
  VerifyBuffer,
  VerifySignature
};

/**
  The user Entry Point for the PKCS7 Verification driver.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval EFI_NOT_SUPPORTED Platform does not support PKCS7 Verification.
  @retval Other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
Pkcs7VerifyDriverEntry (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS    Status;
  EFI_HANDLE    Handle;

  //
  // Install UEFI Pkcs7 Verification Protocol
  //
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiPkcs7VerifyProtocolGuid,
                  &mPkcs7Verify,
                  NULL
                  );

  return Status;
}
