/** @file
  Algorithm identifiers from the TCG Algorithm Registry

  (Trusted Computing Group Algorithm Registry, Version 2.0,
  @https://trustedcomputinggroup.org/resource/tcg-algorithm-registry/)

  Check https://trustedcomputinggroup.org for latest specification updates.

Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Base.h>

// Table 2 - Definition of (UINT16) TCG_ALG_ID Constants
typedef UINT16 TCG_ALG_ID;

#define TCG_ALG_RSA             (TCG_ALG_ID)(0x0001)
#define TCG_ALG_TDES            (TCG_ALG_ID)(0x0003)
#define TCG_ALG_SHA1            (TCG_ALG_ID)(0x0004)
#define TCG_ALG_HMAC            (TCG_ALG_ID)(0x0005)
#define TCG_ALG_AES             (TCG_ALG_ID)(0x0006)
#define TCG_ALG_MGF1            (TCG_ALG_ID)(0x0007)
#define TCG_ALG_KEYEDHASH       (TCG_ALG_ID)(0x0008)
#define TCG_ALG_XOR             (TCG_ALG_ID)(0x000A)
#define TCG_ALG_SHA256          (TCG_ALG_ID)(0x000B)
#define TCG_ALG_SHA384          (TCG_ALG_ID)(0x000C)
#define TCG_ALG_SHA512          (TCG_ALG_ID)(0x000D)
#define TCG_ALG_SHA256_192      (TCG_ALG_ID)(0x000E)
#define TCG_ALG_NULL            (TCG_ALG_ID)(0x0010)
#define TCG_ALG_SM3_256         (TCG_ALG_ID)(0x0012)
#define TCG_ALG_SM4             (TCG_ALG_ID)(0x0013)
#define TCG_ALG_RSASSA          (TCG_ALG_ID)(0x0014)
#define TCG_ALG_RSAES           (TCG_ALG_ID)(0x0015)
#define TCG_ALG_RSAPSS          (TCG_ALG_ID)(0x0016)
#define TCG_ALG_OAEP            (TCG_ALG_ID)(0x0017)
#define TCG_ALG_ECDSA           (TCG_ALG_ID)(0x0018)
#define TCG_ALG_ECDH            (TCG_ALG_ID)(0x0019)
#define TCG_ALG_ECDAA           (TCG_ALG_ID)(0x001A)
#define TCG_ALG_SM2             (TCG_ALG_ID)(0x001B)
#define TCG_ALG_ECSCHNORR       (TCG_ALG_ID)(0x001C)
#define TCG_ALG_ECMQV           (TCG_ALG_ID)(0x001D)
#define TCG_ALG_HKDF            (TCG_ALG_ID)(0x001F)
#define TCG_ALG_KDF1_SP800_56A  (TCG_ALG_ID)(0x0020)
#define TCG_ALG_KDF2            (TCG_ALG_ID)(0x0021)
#define TCG_ALG_KDF1_SP800_108  (TCG_ALG_ID)(0x0022)
#define TCG_ALG_ECC             (TCG_ALG_ID)(0x0023)
#define TCG_ALG_SYMCIPHER       (TCG_ALG_ID)(0x0025)
#define TCG_ALG_CAMELLIA        (TCG_ALG_ID)(0x0026)
#define TCG_ALG_SHA3_256        (TCG_ALG_ID)(0x0027)
#define TCG_ALG_SHA3_384        (TCG_ALG_ID)(0x0028)
#define TCG_ALG_SHA3_512        (TCG_ALG_ID)(0x0029)
#define TCG_ALG_SHAKE128        (TCG_ALG_ID)(0x002A)
#define TCG_ALG_SHAKE256        (TCG_ALG_ID)(0x002B)
#define TCG_ALG_SHAKE256_192    (TCG_ALG_ID)(0x002C)
#define TCG_ALG_SHAKE256_256    (TCG_ALG_ID)(0x002D)
#define TCG_ALG_SHAKE256_512    (TCG_ALG_ID)(0x002E)
#define TCG_ALG_CMAC            (TCG_ALG_ID)(0x003F)
#define TCG_ALG_CTR             (TCG_ALG_ID)(0x0040)
#define TCG_ALG_OFB             (TCG_ALG_ID)(0x0041)
#define TCG_ALG_CBC             (TCG_ALG_ID)(0x0042)
#define TCG_ALG_CFB             (TCG_ALG_ID)(0x0043)
#define TCG_ALG_ECB             (TCG_ALG_ID)(0x0044)
#define TCG_ALG_CCM             (TCG_ALG_ID)(0x0050)
#define TCG_ALG_GCM             (TCG_ALG_ID)(0x0051)
#define TCG_ALG_KW              (TCG_ALG_ID)(0x0052)
#define TCG_ALG_KWP             (TCG_ALG_ID)(0x0053)
#define TCG_ALG_EAX             (TCG_ALG_ID)(0x0054)
#define TCG_ALG_EDDSA           (TCG_ALG_ID)(0x0060)
#define TCG_ALG_HASH_EDDSA      (TCG_ALG_ID)(0x0061)
#define TCG_ALG_RSASVE_KEM      (TCG_ALG_ID)(0x0062)
#define TCG_ALG_ECDH_KEM        (TCG_ALG_ID)(0x0063)
#define TCG_ALG_LMS             (TCG_ALG_ID)(0x0070)
#define TCG_ALG_XMSS            (TCG_ALG_ID)(0x0071)
#define TCG_ALG_LMOTS           (TCG_ALG_ID)(0x0072)
#define TCG_ALG_WOTSP           (TCG_ALG_ID)(0x0073)
#define TCG_ALG_KEYEDXOF        (TCG_ALG_ID)(0x0080)
#define TCG_ALG_KMACXOF128      (TCG_ALG_ID)(0x0081)
#define TCG_ALG_KMACXOF256      (TCG_ALG_ID)(0x0082)
#define TCG_ALG_KMAC128         (TCG_ALG_ID)(0x0090)
#define TCG_ALG_KMAC256         (TCG_ALG_ID)(0x0091)
#define TCG_ALG_MLKEM           (TCG_ALG_ID)(0x00A0)
#define TCG_ALG_MLDSA           (TCG_ALG_ID)(0x00A1)
#define TCG_ALG_HASH_MLDSA      (TCG_ALG_ID)(0x00A2)
#define TCG_ALG_SLHDSA          (TCG_ALG_ID)(0x00A3)
#define TCG_ALG_HASH_SLHDSA     (TCG_ALG_ID)(0x00A4)
#define TCG_ALG_ASCON_AEAD      (TCG_ALG_ID)(0x00B0)
#define TCG_ALG_ASCON_HASH256   (TCG_ALG_ID)(0x00B1)
#define TCG_ALG_ASCON_XOF128    (TCG_ALG_ID)(0x00B2)
#define TCG_ALG_ASCON_CXOF128   (TCG_ALG_ID)(0x00B3)
