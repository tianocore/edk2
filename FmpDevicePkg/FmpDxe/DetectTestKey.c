/** @file
  Detects if PcdFmpDevicePkcs7CertBufferXdr contains a test key.

  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseCryptLib.h>

/**
  Check to see if any of the keys in PcdFmpDevicePkcs7CertBufferXdr matches
  the test key.  PcdFmpDeviceTestKeySha256Digest contains the SHA256 hash of
  the test key.  For each key in PcdFmpDevicePkcs7CertBufferXdr, compute the
  SHA256 hash and compare it to PcdFmpDeviceTestKeySha256Digest.  If the
  SHA256 hash matches or there is then error computing the SHA256 hash, then
  set PcdTestKeyUsed to TRUE.  Skip this check if PcdTestKeyUsed is already
  TRUE or PcdFmpDeviceTestKeySha256Digest is not exactly SHA256_DIGEST_SIZE
  bytes.
**/
VOID
DetectTestKey (
  VOID
  )
{
  BOOLEAN  TestKeyUsed;
  UINTN    PublicKeyDataLength;
  UINT8    *PublicKeyDataXdr;
  UINT8    *PublicKeyDataXdrEnd;
  VOID     *HashContext;
  UINT8    Digest[SHA256_DIGEST_SIZE];
  UINTN    TestKeyDigestSize;

  //
  // If PcdFmpDeviceTestKeySha256Digest is not exacty SHA256_DIGEST_SIZE bytes,
  // then skip the test key detection.
  //
  TestKeyDigestSize = PcdGetSize (PcdFmpDeviceTestKeySha256Digest);
  if (TestKeyDigestSize != SHA256_DIGEST_SIZE) {
    return;
  }

  //
  // If PcdTestKeyUsed is already TRUE, then skip test key detection
  //
  TestKeyUsed = PcdGetBool (PcdTestKeyUsed);
  if (TestKeyUsed) {
    return;
  }

  //
  // If PcdFmpDevicePkcs7CertBufferXdr is invalid, then skip test key detection
  //
  PublicKeyDataXdr    = PcdGetPtr (PcdFmpDevicePkcs7CertBufferXdr);
  PublicKeyDataXdrEnd = PublicKeyDataXdr + PcdGetSize (PcdFmpDevicePkcs7CertBufferXdr);
  if (PublicKeyDataXdr == NULL || PublicKeyDataXdr == PublicKeyDataXdrEnd) {
    return;
  }

  //
  // Allocate hash context buffer required for SHA 256
  //
  HashContext = AllocatePool (Sha256GetContextSize ());
  if (HashContext == NULL) {
    TestKeyUsed = TRUE;
  }

  //
  // Loop through all keys in PcdFmpDevicePkcs7CertBufferXdr
  //
  while (!TestKeyUsed && PublicKeyDataXdr < PublicKeyDataXdrEnd) {
    if (PublicKeyDataXdr + sizeof (UINT32) > PublicKeyDataXdrEnd) {
      //
      // Key data extends beyond end of PCD
      //
      break;
    }
    //
    // Read key length stored in big endian format
    //
    PublicKeyDataLength = SwapBytes32 (*(UINT32 *)(PublicKeyDataXdr));
    //
    // Point to the start of the key data
    //
    PublicKeyDataXdr += sizeof (UINT32);
    if (PublicKeyDataXdr + PublicKeyDataLength > PublicKeyDataXdrEnd) {
      //
      // Key data extends beyond end of PCD
      //
      break;
    }

    //
    // Hash public key from PcdFmpDevicePkcs7CertBufferXdr using SHA256.
    // If error occurs computing SHA256, then assume test key is in use.
    //
    ZeroMem (Digest, SHA256_DIGEST_SIZE);
    if (!Sha256Init (HashContext)) {
      TestKeyUsed = TRUE;
      break;
    }
    if (!Sha256Update (HashContext, PublicKeyDataXdr, PublicKeyDataLength)) {
      TestKeyUsed = TRUE;
      break;
    }
    if (!Sha256Final (HashContext, Digest)) {
      TestKeyUsed = TRUE;
      break;
    }

    //
    // Check if SHA256 hash of public key matches SHA256 hash of test key
    //
    if (CompareMem (Digest, PcdGetPtr (PcdFmpDeviceTestKeySha256Digest), SHA256_DIGEST_SIZE) == 0) {
      TestKeyUsed = TRUE;
      break;
    }

    //
    // Point to start of next key
    //
    PublicKeyDataXdr += PublicKeyDataLength;
    PublicKeyDataXdr = (UINT8 *)ALIGN_POINTER (PublicKeyDataXdr, sizeof (UINT32));
  }

  //
  // Free hash context buffer required for SHA 256
  //
  if (HashContext != NULL) {
    FreePool (HashContext);
    HashContext = NULL;
  }

  //
  // If test key detected or an error occured checking for the test key, then
  // set PcdTestKeyUsed to TRUE.
  //
  if (TestKeyUsed) {
    DEBUG ((DEBUG_INFO, "FmpDxe: Test key detected in PcdFmpDevicePkcs7CertBufferXdr.\n"));
    PcdSetBoolS (PcdTestKeyUsed, TRUE);
  } else {
    DEBUG ((DEBUG_INFO, "FmpDxe: No test key detected in PcdFmpDevicePkcs7CertBufferXdr.\n"));
  }
}
