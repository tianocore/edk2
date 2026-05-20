/** @file
  This file connects TCG TPM openssl usage to EDKII's crypto library.

  The original reference was taken from
    - https://github.com/TrustedComputingGroup/TPM/blob/main/TPMCmd/tpm/cryptolibs/Ossl/include/Ossl/BnToOsslMath.h
  and has been modified to use the EDK2 crypto library interfaces.

**/

#pragma once

#define MATH_LIB_OSSL

// Require TPM Big Num types
#if !defined (MATH_LIB_TPMBIGNUM) && !defined (_BNOSSL_H_)
  #error this OpenSSL Interface expects to be used from TpmBigNum
#endif

#include <BnValues.h>

#include <CrtLibSupport.h>
#include <Library/BaseCryptLib.h>

// ***************************************************************
// ** OpenSSL structures for Big Number
// ***************************************************************

/*
 * 64-bit processor with LP64 ABI
 */
#ifdef SIXTY_FOUR_BIT_LONG
#define BN_ULONG  unsigned long
#define BN_BYTES  8
#endif

/*
 * 64-bit processor other than LP64 ABI
 */
#ifdef SIXTY_FOUR_BIT
#define BN_ULONG  unsigned long long
#define BN_BYTES  8
#endif

#ifdef THIRTY_TWO_BIT
#define BN_ULONG  unsigned int
#define BN_BYTES  4
#endif

struct bignum_st {
  // d is a pointer to an array of |width| |BN_BITS2|-bit chunks in
  // little-endian order. This stores the absolute value of the number.
  BN_ULONG    *d;
  // width is the number of elements of |d| which are valid. This value is not
  // necessarily minimal; the most-significant words of |d| may be zero.
  // |width| determines a potentially loose upper-bound on the absolute value
  // of the |BIGNUM|.
  //
  // Functions taking |BIGNUM| inputs must compute the same answer for all
  // possible widths. |bn_minimal_width|, |bn_set_minimal_width|, and other
  // helpers may be used to recover the minimal width, provided it is not
  // secret. If it is secret, use a different algorithm. Functions may output
  // minimal or non-minimal |BIGNUM|s depending on secrecy requirements, but
  // those which cause widths to unboundedly grow beyond the minimal value
  // should be documented such.
  //
  // Note this is different from historical |BIGNUM| semantics.
  int    top; /**< width */
  // dmax is number of elements of |d| which are allocated.
  int    dmax;
  // neg is one if the number if negative and zero otherwise.
  int    neg;
  // flags is a bitmask of |BN_FLG_*| values
  int    flags;
};

typedef struct bignum_st  BIGNUM;
typedef void              BN_CTX;
typedef void              EC_GROUP;
typedef void              EC_POINT;

// ** Macros and Defines

// Make sure that the library is using the correct size for a crypt word
#if defined THIRTY_TWO_BIT && (RADIX_BITS != 32)                \
  || ((defined SIXTY_FOUR_BIT_LONG || defined SIXTY_FOUR_BIT) \
  && (RADIX_BITS != 64))
  #error Ossl library is using different radix
#endif

// Allocate a local BIGNUM value. For the allocation, a bigNum structure is created
// as is a local BIGNUM. The bigNum is initialized and then the BIGNUM is
// set to reference the local value.
#define BIG_VAR(name, bits)        \
    BN_VAR(name##Bn, (bits));      \
    BIGNUM  _##name;               \
    BIGNUM* name = BigInitialized( \
        &_##name, BnInit(name##Bn, BYTES_TO_CRYPT_WORDS(sizeof(_##name##Bn.d))))

// Allocate a BIGNUM and initialize with the values in a bigNum initializer
#define BIG_INITIALIZED(name, initializer) \
    BIGNUM  _##name;                       \
    BIGNUM* name = BigInitialized(&_##name, initializer)

typedef struct {
  const TPMBN_ECC_CURVE_CONSTANTS    *C;   // the TPM curve values
  EC_GROUP                           *G;   // group parameters
  BN_CTX                             *CTX; // the context for the math (this might not be
                                           // the context in which the curve was created>;
} OSSL_CURVE_DATA;

// Define the curve data type expected by the TpmBigNum library:
typedef OSSL_CURVE_DATA bigCurveData;

TPM_INLINE const TPMBN_ECC_CURVE_CONSTANTS *
AccessCurveConstants (
  const bigCurveData  *E
  )
{
  return E->C;
}

#include <Ossl/TpmToOsslSupport_fp.h>

// Start and end a context within which the OpenSSL memory management works
#define OSSL_ENTER()  BN_CTX* CTX = OsslContextEnter()
#define OSSL_LEAVE()  OsslContextLeave(CTX)

// Start and end a local stack frame within the context of the curve frame
#define ECC_ENTER()  BN_CTX* CTX = OsslPushContext(E->CTX)
#define ECC_LEAVE()  OsslPopContext(CTX)

#define BN_NEW()  BnNewVariable(CTX)

// This definition would change if there were something to report
#define MathLibSimulationEnd()
