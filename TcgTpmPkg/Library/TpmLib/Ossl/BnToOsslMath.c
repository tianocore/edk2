/** @file
  This file connects TCG TPM openssl usage to EDKII's crypto library.

  The original reference was taken from
    - https://github.com/TrustedComputingGroup/TPM/blob/main/TPMCmd/tpm/cryptolibs/Ossl/BnToOsslMath.c
  and has been modified to use the EDK2 crypto library interfaces.

**/
#include "BnOssl.h"

#ifdef MATH_LIB_OSSL
  #include <Ossl/BnToOsslMath_fp.h>
// ** Functions

// *** OsslToTpmBn()
// This function converts an OpenSSL BIGNUM to a TPM bigNum. In this implementation
// it is assumed that OpenSSL uses a different control structure but the same data
// layout -- an array of native-endian words in little-endian order.
//  Return Type: BOOL
//      TRUE(1)         success
//      FALSE(0)        failure because value will not fit or OpenSSL variable doesn't
//                      exist
BOOL
OsslToTpmBn (
  bigNum  bn,
  BIGNUM  *osslBn
  )
{
  GOTO_ERROR_UNLESS (osslBn != NULL);
  // If the bn is NULL, it means that an output value pointer was NULL meaning that
  // the results is simply to be discarded.
  if (bn != NULL) {
    int  i;
    //
    GOTO_ERROR_UNLESS ((unsigned)osslBn->top <= BnGetAllocated (bn));
    for (i = 0; i < osslBn->top; i++) {
      bn->d[i] = osslBn->d[i];
    }

    BnSetTop (bn, osslBn->top);
  }

  return TRUE;
Error:
  return FALSE;
}

// *** BigInitialized()
// This function initializes an OSSL BIGNUM from a TPM bigConst. Do not use this for
// values that are passed to OpenSLL when they are not declared as const in the
// function prototype. Instead, use BnNewVariable().
BIGNUM *
BigInitialized (
  BIGNUM    *toInit,
  bigConst  initializer
  )
{
  if (initializer == NULL) {
    FAIL (FATAL_ERROR_PARAMETER);
  }

  if ((toInit == NULL) || (initializer == NULL)) {
    return NULL;
  }

  toInit->d     = (BN_ULONG *)&initializer->d[0];
  toInit->dmax  = (int)initializer->allocated;
  toInit->top   = (int)initializer->size;
  toInit->neg   = 0;
  toInit->flags = 0;
  return toInit;
}

  #ifndef OSSL_DEBUG
#define BIGNUM_PRINT(label, bn, eol)
#define DEBUG_PRINT(x)
  #else
#define DEBUG_PRINT(x)                TPM_DEBUG_PRINTF("%s", x)
#define BIGNUM_PRINT(label, bn, eol)  BIGNUM_print((label), (bn), (eol))

// *** BIGNUM_print()
static void
BIGNUM_print (
  const char    *label,
  const BIGNUM  *a,
  BOOL          eol
  )
{
  BN_ULONG  *d;
  int       i;
  int       notZero = FALSE;

  if (label != NULL) {
    DEBUG_PRINT ("%s", label);
  }

  if (a == NULL) {
    DEBUG_PRINT ("NULL");
    goto done;
  }

  if (a->neg) {
    DEBUG_PRINT ("-");
  }

  for (i = a->top, d = &a->d[i - 1]; i > 0; i--) {
    int       j;
    BN_ULONG  l = *d--;
    for (j = BN_BITS2 - 8; j >= 0; j -= 8) {
      BYTE  b = (BYTE)((l >> j) & 0xFF);
      notZero = notZero || (b != 0);
      if (notZero) {
        DEBUG_PRINT ("%02x", b);
      }
    }

    if (!notZero) {
      DEBUG_PRINT ("0");
    }
  }

done:
  if (eol) {
    DEBUG_PRINT ("\n");
  }

  return;
}

  #endif

// *** BnNewVariable()
// This function allocates a new variable in the provided context. If the context
// does not exist or the allocation fails, it is a catastrophic failure.
static BIGNUM *
BnNewVariable (
  BN_CTX  *CTX
  )
{
  BIGNUM  *new;

  //
  // This check is intended to protect against calling this function without
  // having initialized the CTX.
  if ((CTX == NULL) || ((new = BigNumContextGet (CTX)) == NULL)) {
    FAIL_NULL (FATAL_ERROR_ALLOCATION);
  }

  return new;
}

  #if LIBRARY_COMPATIBILITY_CHECK

// *** MathLibraryCompatibilityCheck()
BOOL
BnMathLibraryCompatibilityCheck (
  void
  )
{
  OSSL_ENTER ();
  BOOLEAN        OK = FALSE;
  BIGNUM         *osslTemp;
  crypt_uword_t  i;
  BYTE           test[] = {
    0x1F, 0x1E, 0x1D, 0x1C, 0x1B, 0x1A, 0x19, 0x18, 0x17, 0x16, 0x15,
    0x14, 0x13, 0x12, 0x11, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A,
    0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
  };

  BN_VAR (tpmTemp, sizeof (test) * 8);  // allocate some space for a test value

  // Convert the test data to a bigNum
  BnFromBytes (tpmTemp, test, sizeof (test));

  // Convert the test data to an OpenSSL BIGNUM
  osslTemp = BigNumFromBin (test, sizeof (test));

  // Make sure the values are consistent
  GOTO_ERROR_UNLESS (osslTemp->top == (int)tpmTemp->size);

  for (i = 0; i < tpmTemp->size; i++) {
    GOTO_ERROR_UNLESS (osslTemp->d[i] == tpmTemp->d[i]);
  }

  OK = TRUE;

Error:
  BigNumFree (osslTemp, FALSE);
  OSSL_LEAVE ();
  return OK;
}

  #endif

// *** BnModMult()
// This function does a modular multiply. It first does a multiply and then a divide
// and returns the remainder of the divide.
//  Return Type: BOOL
//      TRUE(1)         success
//      FALSE(0)        failure in operation
LIB_EXPORT BOOL
BnModMult (
  bigNum    result,
  bigConst  op1,
  bigConst  op2,
  bigConst  modulus
  )
{
  OSSL_ENTER ();
  BOOL    OK        = FALSE;
  BIGNUM  *bnResult = BN_NEW ();

  BIG_INITIALIZED (bnOp1, op1);
  BIG_INITIALIZED (bnOp2, op2);
  BIG_INITIALIZED (bnMod, modulus);

  GOTO_ERROR_UNLESS (BigNumMulMod (bnOp1, bnOp2, bnMod, bnResult));
  GOTO_ERROR_UNLESS (OsslToTpmBn (result, bnResult));
  OK = TRUE;

Error:
  OSSL_LEAVE ();
  return OK;
}

// *** BnMult()
// Multiplies two numbers
//  Return Type: BOOL
//      TRUE(1)         success
//      FALSE(0)        failure in operation
LIB_EXPORT BOOL
BnMult (
  bigNum    result,
  bigConst  multiplicand,
  bigConst  multiplier
  )
{
  OSSL_ENTER ();
  BIGNUM  *bnResult = BN_NEW ();
  BOOL    OK        = FALSE;

  BIG_INITIALIZED (bnA, multiplicand);
  BIG_INITIALIZED (bnB, multiplier);

  GOTO_ERROR_UNLESS (BigNumMul (bnA, bnB, bnResult));
  GOTO_ERROR_UNLESS (OsslToTpmBn (result, bnResult));
  OK = TRUE;

Error:
  OSSL_LEAVE ();
  return OK;
}

// *** BnDiv()
// This function divides two bigNum values. The function returns FALSE if
// there is an error in the operation.
//  Return Type: BOOL
//      TRUE(1)         success
//      FALSE(0)        failure in operation
LIB_EXPORT BOOL
BnDiv (
  bigNum    quotient,
  bigNum    remainder,
  bigConst  dividend,
  bigConst  divisor
  )
{
  OSSL_ENTER ();
  BIGNUM  *bnQ = BN_NEW ();
  BIGNUM  *bnR = BN_NEW ();
  BOOL    OK   = FALSE;

  BIG_INITIALIZED (bnDend, dividend);
  BIG_INITIALIZED (bnSor, divisor);

  if (BnEqualZero (divisor)) {
    FAIL (FATAL_ERROR_DIVIDE_ZERO);
  }

  GOTO_ERROR_UNLESS (BigNumDiv2 (bnDend, bnSor, bnQ, bnR));
  GOTO_ERROR_UNLESS (OsslToTpmBn (quotient, bnQ));
  GOTO_ERROR_UNLESS (OsslToTpmBn (remainder, bnR));
  OK = TRUE;

  DEBUG_PRINT ("In BnDiv:\n");
  BIGNUM_PRINT ("   bnDividend: ", bnDend, TRUE);
  BIGNUM_PRINT ("    bnDivisor: ", bnSor, TRUE);
  BIGNUM_PRINT ("   bnQuotient: ", bnQ, TRUE);
  BIGNUM_PRINT ("  bnRemainder: ", bnR, TRUE);

Error:
  OSSL_LEAVE ();
  return OK;
}

  #if ALG_RSA
// *** BnGcd()
// Get the greatest common divisor of two numbers
//  Return Type: BOOL
//      TRUE(1)         success
//      FALSE(0)        failure in operation
LIB_EXPORT BOOL
BnGcd (
  bigNum    gcd,                         // OUT: the common divisor
  bigConst  number1,                     // IN:
  bigConst  number2                      // IN:
  )
{
  OSSL_ENTER ();
  BIGNUM  *bnGcd = BN_NEW ();
  BOOL    OK     = FALSE;

  BIG_INITIALIZED (bn1, number1);
  BIG_INITIALIZED (bn2, number2);

  GOTO_ERROR_UNLESS (BigNumGcd (bn1, bn2, bnGcd));
  GOTO_ERROR_UNLESS (OsslToTpmBn (gcd, bnGcd));
  OK = TRUE;

Error:
  OSSL_LEAVE ();
  return OK;
}

// ***BnModExp()
// Do modular exponentiation using bigNum values. The conversion from a bignum_t to
// a bigNum is trivial as they are based on the same structure
//  Return Type: BOOL
//      TRUE(1)         success
//      FALSE(0)        failure in operation
LIB_EXPORT BOOL
BnModExp (
  bigNum    result,                          // OUT: the result
  bigConst  number,                          // IN: number to exponentiate
  bigConst  exponent,                        // IN:
  bigConst  modulus                          // IN:
  )
{
  OSSL_ENTER ();
  BIGNUM  *bnResult = BN_NEW ();
  BOOL    OK        = FALSE;

  BIG_INITIALIZED (bnN, number);
  BIG_INITIALIZED (bnE, exponent);
  BIG_INITIALIZED (bnM, modulus);

  GOTO_ERROR_UNLESS (BigNumExpMod (bnN, bnE, bnM, bnResult));
  GOTO_ERROR_UNLESS (OsslToTpmBn (result, bnResult));
  OK = TRUE;

Error:
  OSSL_LEAVE ();
  return OK;
}

// *** BnModInverse()
// Modular multiplicative inverse
//  Return Type: BOOL
//      TRUE(1)         success
//      FALSE(0)        failure in operation
LIB_EXPORT BOOL
BnModInverse (
  bigNum    result,
  bigConst  number,
  bigConst  modulus
  )
{
  OSSL_ENTER ();
  BIGNUM  *bnResult = BN_NEW ();
  BOOL    OK        = FALSE;

  BIG_INITIALIZED (bnN, number);
  BIG_INITIALIZED (bnM, modulus);

  GOTO_ERROR_UNLESS (BigNumInverseMod (bnN, bnM, bnResult));
  GOTO_ERROR_UNLESS (OsslToTpmBn (result, bnResult));
  OK = TRUE;

Error:
  OSSL_LEAVE ();
  return OK;
}

  #endif // ALG_RSA

  #if ALG_ECC

// *** PointFromOssl()
// Function to copy the point result from an OSSL function to a bigNum
//  Return Type: BOOL
//      TRUE(1)         success
//      FALSE(0)        failure in operation
static BOOL
PointFromOssl (
  bigPoint            pOut,                          // OUT: resulting point
  EC_POINT            *pIn,                          // IN: the point to return
  const bigCurveData  *E                             // IN: the curve
  )
{
  BIGNUM  *x = NULL;
  BIGNUM  *y = NULL;
  BOOL    OK;

  BigNumContextStart (E->CTX);

  x = BigNumContextGet (E->CTX);
  y = BigNumContextGet (E->CTX);

  if (y == NULL) {
    FAIL (FATAL_ERROR_ALLOCATION);
  }

  // If this returns false, then the point is at infinity
  OK = EcPointGetAffineCoordinates (E->G, pIn, x, y, E->CTX);

  if (OK) {
    OsslToTpmBn (pOut->x, x);
    OsslToTpmBn (pOut->y, y);
    BnSetWord (pOut->z, 1);
  } else {
    BnSetWord (pOut->z, 0);
  }

  BigNumContextEnd (E->CTX);
  return OK;
}

// *** EcPointInitialized()
// Allocate and initialize a point.
static EC_POINT *
EcPointInitialized (
  pointConst          initializer,
  const bigCurveData  *E
  )
{
  EC_POINT  *P = NULL;

  if (initializer != NULL) {
    BIG_INITIALIZED (bnX, initializer->x);
    BIG_INITIALIZED (bnY, initializer->y);
    if (E == NULL) {
      FAIL (FATAL_ERROR_ALLOCATION);
    }

    P = EcPointInit (E->G);
    if (!EcPointSetAffineCoordinates (E->G, P, bnX, bnY, E->CTX)) {
      P = NULL;
    }
  }

  return P;
}

// *** BnCurveInitialize()
// This function initializes the OpenSSL curve information structure. This
// structure points to the TPM-defined values for the curve, to the context for the
// number values in the frame, and to the OpenSSL-defined group values.
//  Return Type: bigCurveData*
//      NULL        the TPM_ECC_CURVE is not valid or there was a problem in
//                  in initializing the curve data
//      non-NULL    points to 'E'
LIB_EXPORT bigCurveData *
BnCurveInitialize (
  bigCurveData   *E,       // IN: curve structure to initialize
  TPM_ECC_CURVE  curveId   // IN: curve identifier
  )
{
  const TPMBN_ECC_CURVE_CONSTANTS  *C = BnGetCurveData (curveId);

  if (C == NULL) {
    E = NULL;
  }

  if (E != NULL) {
    // This creates the OpenSSL memory context that stays in effect as long as the
    // curve (E) is defined.
    OSSL_ENTER ();     // if the allocation fails, the TPM fails
    EC_POINT  *P = NULL;
    BIG_INITIALIZED (bnP, C->prime);
    BIG_INITIALIZED (bnA, C->a);
    BIG_INITIALIZED (bnB, C->b);
    BIG_INITIALIZED (bnX, C->base.x);
    BIG_INITIALIZED (bnY, C->base.y);
    BIG_INITIALIZED (bnN, C->order);
    BIG_INITIALIZED (bnH, C->h);
    //
    E->C   = C;
    E->CTX = CTX;

    // initialize EC group, associate a generator point and initialize the point
    // from the parameter data
    // Create a group structure
    E->G = EcGroupInitGFp (bnP, bnA, bnB, CTX);
    GOTO_ERROR_UNLESS (E->G != NULL);

    // Allocate a point in the group that will be used in setting the
    // generator. This is not needed after the generator is set.
    P = EcPointInit (E->G);
    GOTO_ERROR_UNLESS (P != NULL);

    // Need to use this in case Montgomery method is being used
    GOTO_ERROR_UNLESS (EcPointSetAffineCoordinates (E->G, P, bnX, bnY, CTX));

    // Now set the generator
    GOTO_ERROR_UNLESS (EcGroupSetGenerator (E->G, P, bnN, bnH));

    EcPointDeInit (P, FALSE);
    goto Exit;
Error:
    EcPointDeInit (P, FALSE);
    BnCurveFree (E);
    E = NULL;
  }

Exit:
  return E;
}

// *** BnCurveFree()
// This function will free the allocated components of the curve and end the
// frame in which the curve data exists
LIB_EXPORT void
BnCurveFree (
  bigCurveData  *E
  )
{
  if (E) {
    EcGroupFree (E->G);
    OsslContextLeave (E->CTX);
  }
}

// *** BnEccModMult()
// This function does a point multiply of the form R = [d]S
//  Return Type: BOOL
//      TRUE(1)         success
//      FALSE(0)        failure in operation; treat as result being point at infinity
LIB_EXPORT BOOL
BnEccModMult (
  bigPoint            R,                    // OUT: computed point
  pointConst          S,                    // IN: point to multiply by 'd' (optional)
  bigConst            d,                    // IN: scalar for [d]S
  const bigCurveData  *E
  )
{
  EC_POINT  *pR = EcPointInit (E->G);
  EC_POINT  *pS = EcPointInitialized (S, E);

  BIG_INITIALIZED (bnD, d);

  if (S == NULL) {
    EcPointMul2 (E->G, pR, bnD, NULL, NULL, E->CTX);
  } else {
    EcPointMul (E->G, pR, pS, bnD, E->CTX);
  }

  PointFromOssl (R, pR, E);
  EcPointDeInit (pR, FALSE);
  EcPointDeInit (pS, FALSE);
  return !BnEqualZero (R->z);
}

// *** BnEccModMult2()
// This function does a point multiply of the form R = [d]G + [u]Q
//  Return Type: BOOL
//      TRUE(1)         success
//      FALSE(0)        failure in operation; treat as result being point at infinity
LIB_EXPORT BOOL
BnEccModMult2 (
  bigPoint            R,                              // OUT: computed point
  pointConst          S,                              // IN: optional point
  bigConst            d,                              // IN: scalar for [d]S or [d]G
  pointConst          Q,                              // IN: second point
  bigConst            u,                              // IN: second scalar
  const bigCurveData  *E                              // IN: curve
  )
{
  EC_POINT  *pR = EcPointInit (E->G);
  EC_POINT  *pS = EcPointInitialized (S, E);

  BIG_INITIALIZED (bnD, d);
  EC_POINT  *pQ = EcPointInitialized (Q, E);

  BIG_INITIALIZED (bnU, u);

  if ((S == NULL) || (S == (pointConst) & (AccessCurveConstants (E)->base))) {
    EcPointMul2 (E->G, pR, bnD, pQ, bnU, E->CTX);
  } else {
    const EC_POINT  *points[2];
    const BIGNUM    *scalars[2];
    points[0]  = pS;
    points[1]  = pQ;
    scalars[0] = bnD;
    scalars[1] = bnU;
    EcPointsMul (E->G, pR, NULL, 2, (CONST VOID **)points, (CONST VOID **)scalars, E->CTX);
  }

  PointFromOssl (R, pR, E);
  EcPointDeInit (pR, FALSE);
  EcPointDeInit (pS, FALSE);
  EcPointDeInit (pQ, FALSE);
  return !BnEqualZero (R->z);
}

// ** BnEccAdd()
// This function does addition of two points.
//  Return Type: BOOL
//      TRUE(1)         success
//      FALSE(0)        failure in operation; treat as result being point at infinity
LIB_EXPORT BOOL
BnEccAdd (
  bigPoint            R,                         // OUT: computed point
  pointConst          S,                         // IN: first point to add
  pointConst          Q,                         // IN: second point
  const bigCurveData  *E                         // IN: curve
  )
{
  EC_POINT  *pR = EcPointInit (E->G);
  EC_POINT  *pS = EcPointInitialized (S, E);
  EC_POINT  *pQ = EcPointInitialized (Q, E);

  EcPointAdd (E->G, pR, pS, pQ, E->CTX);

  PointFromOssl (R, pR, E);
  EcPointDeInit (pR, FALSE);
  EcPointDeInit (pS, FALSE);
  EcPointDeInit (pQ, FALSE);
  return !BnEqualZero (R->z);
}

  #endif // ALG_ECC

#endif // MATHLIB OSSL
