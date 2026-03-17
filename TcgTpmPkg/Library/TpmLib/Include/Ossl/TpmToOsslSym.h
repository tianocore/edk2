/** @file
  This file connects TCG TPM openssl usage to EDKII's crypto library.

  The original reference was taken from
    - https://github.com/TrustedComputingGroup/TPM/blob/main/TPMCmd/tpm/cryptolibs/Ossl/include/Ossl/TpmToOsslSym.h
  and has been modified to use the EDK2 crypto library interfaces.

**/

#pragma once

#define SYM_LIB_OSSL

#include <Library/BaseCryptLib.h>

// ***************************************************************
// ** OpenSSL structures for Cipher
// ***************************************************************
#define AES_MAXNR  14

struct aes_key_st {
 #ifdef AES_LONG
  unsigned long    rd_key[4 * (AES_MAXNR + 1)];
 #else
  unsigned int     rd_key[4 * (AES_MAXNR + 1)];
 #endif
  int              rounds;
};

struct edkii_aes_key_st {
  struct aes_key_st    aes_key[2];
};

#define CAMELLIA_TABLE_BYTE_LEN  272
#define CAMELLIA_TABLE_WORD_LEN  (CAMELLIA_TABLE_BYTE_LEN / 4)

typedef unsigned int KEY_TABLE_TYPE[CAMELLIA_TABLE_WORD_LEN]; /* to match
                                                               * with WORD */
struct camellia_key_st {
  union {
    double            d;        /* ensures 64-bit align */
    KEY_TABLE_TYPE    rd_key;
  } u;
  int    grand_rounds;
};

typedef struct edkii_aes_key_st AES_KEY;
typedef struct camellia_key_st  CAMELLIA_KEY;

// ***************************************************************
// ** Links to the OpenSSL symmetric algorithms.
// ***************************************************************

// The Crypt functions that call the block encryption function use the parameters
// in the order:
//  1) keySchedule
//  2) in buffer
//  3) out buffer
// Since open SSL uses the order in encryptoCall_t above, need to swizzle the
// values to the order required by the library.
#define SWIZZLE(keySchedule, in, out) \
    (VOID *)(keySchedule), (CONST UINT8 *)(in), (UINT8 *)(out)

// Define the order of parameters to the library functions that do block encryption
// and decryption.
typedef BOOLEAN (*TpmCryptSetSymKeyCall_t)(
  VOID   *keySchedule,
  CONST UINT8   *in,
  UINT8  *out
  );

// ***************************************************************
// ** Links to the OpenSSL AES code
// ***************************************************************
// Macros to set up the encryption/decryption key schedules
//
// AES:

/*
 * TCG TPM v2.0 implementation wants to receive 0 when it success.
 * But AesInit() returns TRUE on success.
 */
#define TpmCryptSetEncryptKeyAES(key, keySizeInBits, schedule) \
    !AesInit((tpmKeyScheduleAES *)(schedule), (key), (keySizeInBits))
#define TpmCryptSetDecryptKeyAES(key, keySizeInBits, schedule) \
    !AesInit((tpmKeyScheduleAES *)(schedule), (key), (keySizeInBits))

// Macros to alias encryption calls to specific algorithms. This should be used
// sparingly. Currently, only used by CryptSym.c and CryptRand.c
//
// When using these calls, to call the AES block encryption code, the caller
// should use:
//      TpmCryptEncryptAES(SWIZZLE(keySchedule, in, out));
#define TpmCryptEncryptAES  AesEncrypt
#define TpmCryptDecryptAES  AesDecrypt
#define tpmKeyScheduleAES   AES_KEY

// ***************************************************************
// ** Links to the OpenSSL SM4 code
// ***************************************************************
// Macros to set up the encryption/decryption key schedules

/* SM4 unsupported by EDKII */

// ***************************************************************
// ** Links to the OpenSSL CAMELLIA code
// ***************************************************************
// Macros to set up the encryption/decryption key schedules

/*
 * TCG TPM v2.0 implementation wants to receive 0 when it success.
 * But CamelliaInit() returns TRUE on success.
 */
#define TpmCryptSetEncryptKeyCAMELLIA(key, keySizeInBits, schedule) \
    !CamelliaInit((tpmKeyScheduleCAMELLIA*)(schedule), (key), (keySizeInBits))
#define TpmCryptSetDecryptKeyCAMELLIA(key, keySizeInBits, schedule) \
    !CamelliaInit((tpmKeyScheduleCAMELLIA*)(schedule), (key), (keySizeInBits))

// Macros to alias encryption calls to specific algorithms. This should be used
// sparingly.
#define TpmCryptEncryptCAMELLIA  CamelliaEncrypt
#define TpmCryptDecryptCAMELLIA  CamelliaDecrypt
#define tpmKeyScheduleCAMELLIA   CAMELLIA_KEY

// Forward reference

typedef union tpmCryptKeySchedule_t tpmCryptKeySchedule_t;

// This definition would change if there were something to report
#define SymLibSimulationEnd()
