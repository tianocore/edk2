/** @file
  Detects if PcdFmpDevicePkcs7CertBufferXdr contains a test key.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

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
  );
