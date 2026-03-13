/** @file
  This file connects TCG TPM openssl usage to EDKII's crypto library.

  The original reference was taken from
    - https://github.com/TrustedComputingGroup/TPM/blob/main/TPMCmd/tpm/cryptolibs/Ossl/include/Ossl/TpmToOsslHash.h
  and has been modified to use the EDK2 crypto library interfaces.

**/

#pragma once

#define HASH_LIB_OSSL

#include <Library/BaseCryptLib.h>

// ***************************************************************
// ** OpenSSL structures for HASH
// ***************************************************************
#if ALG_SM3_256
#define SM3_DIGEST_LENGTH  32
#define SM3_WORD           UINT32

#define SM3_CBLOCK  64
#define SM3_LBLOCK  (SM3_CBLOCK / 4)

typedef struct SM3state_st {
  SM3_WORD        A, B, C, D, E, F, G, H;
  SM3_WORD        Nl, Nh;
  SM3_WORD        data[SM3_LBLOCK];
  unsigned int    num;
} SM3_CTX;
#endif // ALG_SM3_256

#define SHA_LONG       UINT32
#define SHA_LONG64     UINT64
#define SHA_LBLOCK     16
#define SHA512_CBLOCK  (SHA_LBLOCK * 8)

typedef struct SHAstate_st {
  SHA_LONG        h0, h1, h2, h3, h4;
  SHA_LONG        Nl, Nh;
  SHA_LONG        data[SHA_LBLOCK];
  unsigned int    num;
} SHA_CTX;

typedef struct SHA256state_st {
  SHA_LONG        h[8];
  SHA_LONG        Nl, Nh;
  SHA_LONG        data[SHA_LBLOCK];
  unsigned int    num, md_len;
} SHA256_CTX;

typedef struct SHA512state_st {
  SHA_LONG64      h[8];
  SHA_LONG64      Nl, Nh;
  union {
    SHA_LONG64       d[SHA_LBLOCK];
    unsigned char    p[SHA512_CBLOCK];
  } u;
  unsigned int    num, md_len;
} SHA512_CTX;

typedef struct SM3state_st     SM3_CTX;
typedef struct SHAstate_st     SHA_CTX;
typedef struct SHA256state_st  SHA256_CTX;
typedef struct SHA512state_st  SHA512_CTX;

// ***************************************************************
// ** Links to the OpenSSL HASH code
// ***************************************************************

// Redefine the internal name used for each of the hash state structures to the
// name used by the library.
// These defines need to be known in all parts of the TPM so that the structure
// sizes can be properly computed when needed.
#define tpmHashStateSHA1_t     SHA_CTX
#define tpmHashStateSHA256_t   SHA256_CTX
#define tpmHashStateSHA384_t   SHA512_CTX
#define tpmHashStateSHA512_t   SHA512_CTX
#define tpmHashStateSM3_256_t  SM3_CTX

// The defines below are only needed when compiling CryptHash.c or CryptSmac.c.
// This isolation is primarily to avoid name space collision. However, if there
// is a real collision, it will likely show up when the linker tries to put things
// together.

#ifdef _CRYPT_HASH_C_

typedef UINT8       *PBYTE;
typedef CONST VOID  *PCBYTE;

// Define the interface between CryptHash.c to the functions provided by the
// library. For each method, define the calling parameters of the method and then
// define how the method is invoked in CryptHash.c.
//
// All hashes are required to have the same calling sequence. If they don't, create
// a simple adaptation function that converts from the "standard" form of the call
// to the form used by the specific hash (and then send a nasty letter to the
// person who wrote the hash function for the library).
//
// The macro that calls the method also defines how the
// parameters get swizzled between the default form (in CryptHash.c)and the
// library form.
//
// Initialize the hash context
#define HASH_START_METHOD_DEF  BOOLEAN (HASH_START_METHOD)(PANY_HASH_STATE state)
#define HASH_START(hashState)  ((hashState)->def->method.start)(&(hashState)->state);

// Add data to the hash
#define HASH_DATA_METHOD_DEF   \
      BOOLEAN (HASH_DATA_METHOD)(PANY_HASH_STATE state, PCBYTE buffer, UINTN size)
#define HASH_DATA(hashState, dInSize, dIn) \
      ((hashState)->def->method.data)(&(hashState)->state, dIn, dInSize)

// Finalize the hash and get the digest
#define HASH_END_METHOD_DEF   \
      BOOLEAN (HASH_END_METHOD)(PANY_HASH_STATE state, PBYTE buffer)
#define HASH_END(hashState, buffer) \
      ((hashState)->def->method.end)(&(hashState)->state, buffer)

// Copy the hash context
// Note: For import, export, and copy, memcpy() is used since there is no
// reformatting necessary between the internal and external forms.
#define HASH_STATE_COPY_METHOD_DEF   \
      void(HASH_STATE_COPY_METHOD)(  \
          PANY_HASH_STATE to, PCANY_HASH_STATE from, size_t size)
#define HASH_STATE_COPY(hashStateOut, hashStateIn)            \
      ((hashStateIn)->def->method.copy)(&(hashStateOut)->state, \
                                        &(hashStateIn)->state,  \
                                        (hashStateIn)->def->contextSize)

// Copy (with reformatting when necessary) an internal hash structure to an
// external blob
#define HASH_STATE_EXPORT_METHOD_DEF   \
      void(HASH_STATE_EXPORT_METHOD)(BYTE * to, PCANY_HASH_STATE from, size_t size)
#define HASH_STATE_EXPORT(to, hashStateFrom)           \
      ((hashStateFrom)->def->method.copyOut)(            \
          &(((BYTE*)(to))[offsetof(HASH_STATE, state)]), \
          &(hashStateFrom)->state,                       \
          (hashStateFrom)->def->contextSize)

// Copy from an external blob to an internal formate (with reformatting when
// necessary
#define HASH_STATE_IMPORT_METHOD_DEF   \
      void(HASH_STATE_IMPORT_METHOD)(  \
          PANY_HASH_STATE to, const BYTE* from, size_t size)
#define HASH_STATE_IMPORT(hashStateTo, from)                   \
      ((hashStateTo)->def->method.copyIn)(                       \
          &(hashStateTo)->state,                                 \
          &(((const BYTE*)(from))[offsetof(HASH_STATE, state)]), \
          (hashStateTo)->def->contextSize)

// Function aliases. The code in CryptHash.c uses the internal designation for the
// functions. These need to be translated to the function names of the library.
#define tpmHashStart_SHA1           Sha1Init
#define tpmHashData_SHA1            Sha1Update
#define tpmHashEnd_SHA1             Sha1Final
#define tpmHashStateCopy_SHA1       memcpy
#define tpmHashStateExport_SHA1     memcpy
#define tpmHashStateImport_SHA1     memcpy
#define tpmHashStart_SHA256         Sha256Init
#define tpmHashData_SHA256          Sha256Update
#define tpmHashEnd_SHA256           Sha256Final
#define tpmHashStateCopy_SHA256     memcpy
#define tpmHashStateExport_SHA256   memcpy
#define tpmHashStateImport_SHA256   memcpy
#define tpmHashStart_SHA384         Sha384Init
#define tpmHashData_SHA384          Sha384Update
#define tpmHashEnd_SHA384           Sha384Final
#define tpmHashStateCopy_SHA384     memcpy
#define tpmHashStateExport_SHA384   memcpy
#define tpmHashStateImport_SHA384   memcpy
#define tpmHashStart_SHA512         Sha512Init
#define tpmHashData_SHA512          Sha512Update
#define tpmHashEnd_SHA512           Sha512Final
#define tpmHashStateCopy_SHA512     memcpy
#define tpmHashStateExport_SHA512   memcpy
#define tpmHashStateImport_SHA512   memcpy
#define tpmHashStart_SM3_256        Sm3Init
#define tpmHashData_SM3_256         Sm3Update
#define tpmHashEnd_SM3_256          Sm3Final
#define tpmHashStateCopy_SM3_256    memcpy
#define tpmHashStateExport_SM3_256  memcpy
#define tpmHashStateImport_SM3_256  memcpy

#endif // _CRYPT_HASH_C_

#define LibHashInit()
// This definition would change if there were something to report
#define HashLibSimulationEnd()
