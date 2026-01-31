/** @file
  This file connects TCG TPM openssl usage to EDKII's crypto library.

  The original reference was taken from
    - https://github.com/TrustedComputingGroup/TPM/blob/main/TPMCmd/tpm/cryptolibs/Ossl/TpmToOsslSupport.c
  and has been modified to use the EDK2 crypto library interfaces.

**/
#include "BnOssl.h"
#include <CryptoInterface.h>
#include <Ossl/TpmToOsslSym.h>
#include <Ossl/TpmToOsslHash.h>
#include <stdio.h>

#if defined (HASH_LIB_OSSL) || defined (MATH_LIB_OSSL) || defined (SYM_LIB_OSSL)
// Used to pass the pointers to the correct sub-keys
typedef const BYTE *desKeyPointers[3];

// *** BnSupportLibInit()
// This does any initialization required by the support library.
LIB_EXPORT int
BnSupportLibInit (
  void
  )
{
  return TRUE;
}

// *** OsslContextEnter()
// This function is used to initialize an OpenSSL context at the start of a function
// that will call to an OpenSSL math function.
BN_CTX *
OsslContextEnter (
  void
  )
{
  BN_CTX  *CTX = BigNumNewContext ();

  //
  return OsslPushContext (CTX);
}

// *** OsslContextLeave()
// This is the companion function to OsslContextEnter().
void
OsslContextLeave (
  BN_CTX  *CTX
  )
{
  OsslPopContext (CTX);
  BigNumContextFree (CTX);
}

// *** OsslPushContext()
// This function is used to create a frame in a context. All values allocated within
// this context after the frame is started will be automatically freed when the
// context (OsslPopContext()
BN_CTX *
OsslPushContext (
  BN_CTX  *CTX
  )
{
  if (CTX == NULL) {
    FAIL (FATAL_ERROR_ALLOCATION);
  }

  BigNumContextStart (CTX);
  return CTX;
}

// *** OsslPopContext()
// This is the companion function to OsslPushContext().
void
OsslPopContext (
  BN_CTX  *CTX
  )
{
  // BN_CTX_end can't be called with NULL. It will blow up.
  if (CTX != NULL) {
    BigNumContextEnd (CTX);
  }
}

#endif // HASH_LIB_OSSL || MATH_LIB_OSSL || SYM_LIB_OSSL
