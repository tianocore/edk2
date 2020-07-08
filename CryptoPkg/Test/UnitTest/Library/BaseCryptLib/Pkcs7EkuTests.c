/** @file -- Pkcs7EkuVerify.c
 * Copyright (c) Microsoft Corporation.
 * SPDX-License-Identifier: BSD-2-Clause-Patent

  This is an test code which verifies specified
  Enhanced Key Usages (EKU)'s are present in the leaf signer
  of a PKCS7 formatted signature.


  A typical signing certificate chain looks like this: (Could be RSA or ECC).

      ------------------------------------------
     |                                          | // Root of trust. ECDSA P521 curve
     |          TestEKUParsingRoot              | // SHA 256 Key Usage: CERT_DIGITAL_SIGNATURE_KEY_USAGE
     |                                          | // CERT_KEY_CERT_SIGN_KEY_USAGE | CERT_CRL_SIGN_KEY_USAGE
      ------------------------------------------
                        ^
                        |
      ------------------------------------------
     |                                          | // Policy CA.  Issues subordinate CAs. ECC P384 curve.
     |       TestEKUParsingPolicyCA             | // SHA 256 Key Usage:
     |                                          | // CERT_KEY_CERT_SIGN_KEY_USAGE | CERT_CRL_SIGN_KEY_USAGE
      ------------------------------------------
                        ^
                        |
      ------------------------------------------
     |                                          | // Issues end-entity (leaf) signers. ECC P256 curve.
     |        TestEKUParsingIssuingCA           | // SHA 256 Key Usage: CERT_DIGITAL_SIGNATURE_KEY_USAGE
     |                                          | // Enhanced Key Usage:
      ------------------------------------------  // 1.3.6.1.4.1.311.76.9.21.1 (Surface firmware signing)
                        ^
                        |
         --------------------------------------
        /     TestEKUParsingLeafSigner &&     /   // Leaf signer,  ECC P256 curve.
       /    TestEKUParsingLeafSignerPid12345 /    // SHA 256 Key Usage: CERT_DIGITAL_SIGNATURE_KEY_USAGE
      /                                     /     // Enhanced Key usages:
      --------------------------------------      // 1.3.6.1.4.1.311.76.9.21.1 (Surface firmware signing)
                                                  // 1.3.6.1.4.1.311.76.9.21.1.N, N == Product ID.





**/

#include "TestBaseCryptLib.h"

#include "Pkcs7EkuTestSignatures.h"

EFI_STATUS
EFIAPI
VerifyEKUsInPkcs7Signature (
  IN CONST UINT8  *Pkcs7Signature,
  IN CONST UINT32  SignatureSize,
  IN CONST CHAR8  *RequiredEKUs[],
  IN CONST UINT32  RequiredEKUsSize,
  IN BOOLEAN       RequireAllPresent
  );

///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================

CONST CHAR8 FIRMWARE_SIGNER_EKU[] = "1.3.6.1.4.1.311.76.9.21.1";


/**
  TestVerifyEKUsInSignature()

  Verify that "1.3.6.1.4.1.311.76.9.21.1" (Firmware signature) is in the
  leaf signer certificate.


  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required EKUs were found in the signature.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestVerifyEKUsInSignature (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS       Status     = EFI_SUCCESS;

  CONST CHAR8* RequiredEKUs[] = { FIRMWARE_SIGNER_EKU };

  Status = VerifyEKUsInPkcs7Signature(ProductionECCSignature,
                                      ARRAY_SIZE(ProductionECCSignature),
                                      (CONST CHAR8**)RequiredEKUs,
                                      ARRAY_SIZE(RequiredEKUs),
                                      TRUE);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  return UNIT_TEST_PASSED;
}// TestVerifyEKUsInSignature()


/**
  TestVerifyEKUsWith3CertsInSignature()

  This PKCS7 signature has 3 certificates in it. (Policy CA, Issuing CA
  and leaf signer). It has one firmware signing EKU in it.
  "1.3.6.1.4.1.311.76.9.21.1"

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required EKUs were found in the signature.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestVerifyEKUsWith3CertsInSignature (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS       Status     = EFI_SUCCESS;

  CONST CHAR8* RequiredEKUs[] = { FIRMWARE_SIGNER_EKU };

  Status = VerifyEKUsInPkcs7Signature(TestSignEKUsWith3CertsInSignature,
                                      ARRAY_SIZE(TestSignEKUsWith3CertsInSignature),
                                      (CONST CHAR8**)RequiredEKUs,
                                      ARRAY_SIZE(RequiredEKUs),
                                      TRUE);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  return UNIT_TEST_PASSED;
}// TestVerifyEKUsWith3CertsInSignature()

/**
  TestVerifyEKUsWith2CertsInSignature()

  This PKCS7 signature has 2 certificates in it. (Issuing CA and leaf signer).
  It has one firmware signing EKU in it. "1.3.6.1.4.1.311.76.9.21.1"

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required EKUs were found in the signature.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestVerifyEKUsWith2CertsInSignature (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS       Status     = EFI_SUCCESS;

  CONST CHAR8* RequiredEKUs[] = { FIRMWARE_SIGNER_EKU };

  Status = VerifyEKUsInPkcs7Signature(TestSignEKUsWith2CertsInSignature,
                                      ARRAY_SIZE(TestSignEKUsWith2CertsInSignature),
                                      (CONST CHAR8**)RequiredEKUs,
                                      ARRAY_SIZE(RequiredEKUs),
                                      TRUE);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  return UNIT_TEST_PASSED;
}// TestVerifyEKUsWith2CertsInSignature()


/**
  TestVerifyEKUsWith1CertInSignature()

  This PKCS7 signature only has the leaf signer in it.
  It has one firmware signing EKU in it. "1.3.6.1.4.1.311.76.9.21.1"

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required EKUs were found in the signature.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestVerifyEKUsWith1CertInSignature (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS       Status     = EFI_SUCCESS;

  CONST CHAR8* RequiredEKUs[] = { FIRMWARE_SIGNER_EKU };

  Status = VerifyEKUsInPkcs7Signature(TestSignEKUsWith1CertInSignature,
                                      ARRAY_SIZE(TestSignEKUsWith1CertInSignature),
                                      (CONST CHAR8**)RequiredEKUs,
                                      ARRAY_SIZE(RequiredEKUs),
                                      TRUE);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  return UNIT_TEST_PASSED;
}// TestVerifyEKUsWith1CertInSignature()


/**
  TestVerifyEKUsWithMultipleEKUsInCert()


  This signature has two EKU's in it:
  "1.3.6.1.4.1.311.76.9.21.1"
  "1.3.6.1.4.1.311.76.9.21.2"
  We verify that both EKU's were present in the leaf signer.

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required EKUs were found in the signature.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestVerifyEKUsWithMultipleEKUsInCert (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS       Status     = EFI_SUCCESS;

  CONST CHAR8* RequiredEKUs[] = { "1.3.6.1.4.1.311.76.9.21.1",
                                  "1.3.6.1.4.1.311.76.9.21.1.2" };

  Status = VerifyEKUsInPkcs7Signature(TestSignedWithMultipleEKUsInCert,
                                      ARRAY_SIZE(TestSignedWithMultipleEKUsInCert),
                                      (CONST CHAR8**)RequiredEKUs,
                                      ARRAY_SIZE(RequiredEKUs),
                                      TRUE);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  return UNIT_TEST_PASSED;
}// TestVerifyEKUsWithMultipleEKUsInCert()


/**
  TestEkusNotPresentInSignature()

  This test verifies that if we send an EKU that is not in the signature,
  that we get back an error.

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required EKUs were found in the signature.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestEkusNotPresentInSignature (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS       Status = EFI_SUCCESS;

  //
  // This EKU is not in the signature.
  //
  CONST CHAR8* RequiredEKUs[] = { "1.3.6.1.4.1.311.76.9.21.3" };

  Status = VerifyEKUsInPkcs7Signature(TestSignedWithMultipleEKUsInCert,
                                      ARRAY_SIZE(TestSignedWithMultipleEKUsInCert),
                                      (CONST CHAR8**)RequiredEKUs,
                                      ARRAY_SIZE(RequiredEKUs),
                                      TRUE);
  UT_ASSERT_NOT_EQUAL (Status, EFI_SUCCESS);

  return UNIT_TEST_PASSED;
}// TestEkusNotPresentInSignature()

/**
  TestEkusNotPresentInSignature()

  This test signature has two EKU's in it:  (Product ID is 10001)
  "1.3.6.1.4.1.311.76.9.21.1"
  "1.3.6.1.4.1.311.76.9.21.1.10001"

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required EKUs were found in the signature.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/

static
UNIT_TEST_STATUS
EFIAPI
TestProductId10001PresentInSignature(
  IN UNIT_TEST_CONTEXT           Context
)
{
  EFI_STATUS       Status = EFI_SUCCESS;

  //
  // These EKU's are present in the leaf signer certificate.
  //
  CONST CHAR8* RequiredEKUs[] = { "1.3.6.1.4.1.311.76.9.21.1",
                                  "1.3.6.1.4.1.311.76.9.21.1.10001" };

  Status = VerifyEKUsInPkcs7Signature(TestSignedWithProductId10001,
                                      ARRAY_SIZE(TestSignedWithProductId10001),
                                      (CONST CHAR8**)RequiredEKUs,
                                      ARRAY_SIZE(RequiredEKUs),
                                      TRUE);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  return UNIT_TEST_PASSED;
}// TestProductId10001PresentInSignature()


/**
  TestOnlyOneEkuInListRequired()

  This test will check the BOOLEAN RequireAllPresent parameter in the
  call to VerifyEKUsInPkcs7Signature() behaves properly.  The signature
  has two EKU's in it:

  "1.3.6.1.4.1.311.76.9.21.1"
  "1.3.6.1.4.1.311.76.9.21.1.10001"

  but we only pass in one of them, and set RequireAllPresent to FALSE.

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required EKUs were found in the signature.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/

static
UNIT_TEST_STATUS
EFIAPI
TestOnlyOneEkuInListRequired(
  IN UNIT_TEST_CONTEXT           Context
)
{
  EFI_STATUS       Status = EFI_SUCCESS;

  //
  // This will test the flag that specifies it is OK to succeed if
  // any one of the EKU's passed in is found.
  //
  CONST CHAR8* RequiredEKUs[] = { "1.3.6.1.4.1.311.76.9.21.1.10001" };

  Status = VerifyEKUsInPkcs7Signature(TestSignedWithProductId10001,
                                      ARRAY_SIZE(TestSignedWithProductId10001),
                                      (CONST CHAR8**)RequiredEKUs,
                                      ARRAY_SIZE(RequiredEKUs),
                                      FALSE);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  return UNIT_TEST_PASSED;
}// TestOnlyOneEkuInListRequired()

/**
  TestNoEKUsInSignature()

  This test uses a signature that was signed with a certificate that does
  not contain any EKUs.


  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required EKUs were found in the signature.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/

static
UNIT_TEST_STATUS
EFIAPI
TestNoEKUsInSignature(
  IN UNIT_TEST_CONTEXT           Context
)
{
  EFI_STATUS       Status     = EFI_SUCCESS;

  //
  // This EKU is not in the certificate, so it should fail.
  //
  CONST CHAR8* RequiredEKUs[] = { "1.3.6.1.4.1.311.76.9.21.1" };

  Status = VerifyEKUsInPkcs7Signature(TestSignatureWithNoEKUsPresent,
                                      ARRAY_SIZE(TestSignatureWithNoEKUsPresent),
                                      (CONST CHAR8**)RequiredEKUs,
                                      ARRAY_SIZE(RequiredEKUs),
                                      TRUE);
  UT_ASSERT_NOT_EQUAL (Status, EFI_SUCCESS);

  return UNIT_TEST_PASSED;
}// TestNoEKUsInSignature()


/**
  TestInvalidParameters()

  Passes the API invalid parameters, and ensures that it does not succeed.

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required EKUs were found in the signature.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestInvalidParameters(
  IN UNIT_TEST_CONTEXT           Context
)
{
  EFI_STATUS       Status     = EFI_SUCCESS;

  CONST CHAR8* RequiredEKUs[] = { "1.3.6.1.4.1.311.76.9.21.1" };

  //
  // Check bad signature.
  //
  Status = VerifyEKUsInPkcs7Signature(NULL,
                                      0,
                                      (CONST CHAR8**)RequiredEKUs,
                                      ARRAY_SIZE(RequiredEKUs),
                                      TRUE);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  //
  // Check invalid EKU's
  //
  Status = VerifyEKUsInPkcs7Signature(TestSignatureWithNoEKUsPresent,
                                      ARRAY_SIZE(TestSignatureWithNoEKUsPresent),
                                      (CONST CHAR8**)NULL,
                                      0,
                                      TRUE);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}// TestInvalidParameters()


/**
  TestEKUSubStringFails()

  Pass the API a sub set and super set of an EKU and ensure that they
  don't pass.

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required EKUs were found in the signature.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestEKUSubsetSupersetFails(
  IN UNIT_TEST_CONTEXT           Context
)
{
  EFI_STATUS       Status     = EFI_SUCCESS;

  //
  // This signature has an EKU of:
  // "1.3.6.1.4.1.311.76.9.21.1.10001"
  // so ensure that
  // "1.3.6.1.4.1.311.76.9.21"
  // does not pass.
  //
  CONST CHAR8* RequiredEKUs1[] = { "1.3.6.1.4.1.311.76.9.21" };

  Status = VerifyEKUsInPkcs7Signature(TestSignedWithProductId10001,
                                      ARRAY_SIZE(TestSignedWithProductId10001),
                                      (CONST CHAR8**)RequiredEKUs1,
                                      ARRAY_SIZE(RequiredEKUs1),
                                      TRUE);
  UT_ASSERT_NOT_EQUAL (Status, EFI_SUCCESS);

  //
  // This signature has an EKU of:
  // "1.3.6.1.4.1.311.76.9.21.1.10001"
  // so ensure that a super set
  // "1.3.6.1.4.1.311.76.9.21.1.10001.1"
  // does not pass.
  //
  CONST CHAR8* RequiredEKUs2[] = { "1.3.6.1.4.1.311.76.9.21.1.10001.1" };

  Status = VerifyEKUsInPkcs7Signature(TestSignedWithProductId10001,
                                      ARRAY_SIZE(TestSignedWithProductId10001),
                                      (CONST CHAR8**)RequiredEKUs2,
                                      ARRAY_SIZE(RequiredEKUs2),
                                      TRUE);
  UT_ASSERT_NOT_EQUAL (Status, EFI_SUCCESS);

  return UNIT_TEST_PASSED;
}// TestEKUSubsetSupersetFails()

TEST_DESC mPkcs7EkuTest[] = {
    //
    // -----Description--------------------------------Class----------------------------Function------------------------------Pre---Post--Context
    //
    {"TestVerifyEKUsInSignature()",            "CryptoPkg.BaseCryptLib.Eku",   TestVerifyEKUsInSignature,            NULL, NULL, NULL},
    {"TestVerifyEKUsWith3CertsInSignature()",  "CryptoPkg.BaseCryptLib.Eku",   TestVerifyEKUsWith3CertsInSignature,  NULL, NULL, NULL},
    {"TestVerifyEKUsWith2CertsInSignature()",  "CryptoPkg.BaseCryptLib.Eku",   TestVerifyEKUsWith2CertsInSignature,  NULL, NULL, NULL},
    {"TestVerifyEKUsWith1CertInSignature()",   "CryptoPkg.BaseCryptLib.Eku",   TestVerifyEKUsWith1CertInSignature,   NULL, NULL, NULL},
    {"TestVerifyEKUsWithMultipleEKUsInCert()", "CryptoPkg.BaseCryptLib.Eku",   TestVerifyEKUsWithMultipleEKUsInCert, NULL, NULL, NULL},
    {"TestEkusNotPresentInSignature()",        "CryptoPkg.BaseCryptLib.Eku",   TestEkusNotPresentInSignature,        NULL, NULL, NULL},
    {"TestProductId10001PresentInSignature()", "CryptoPkg.BaseCryptLib.Eku",   TestProductId10001PresentInSignature, NULL, NULL, NULL},
    {"TestOnlyOneEkuInListRequired()",         "CryptoPkg.BaseCryptLib.Eku",   TestOnlyOneEkuInListRequired,         NULL, NULL, NULL},
    {"TestNoEKUsInSignature()",                "CryptoPkg.BaseCryptLib.Eku",   TestNoEKUsInSignature,                NULL, NULL, NULL},
    {"TestInvalidParameters()",                "CryptoPkg.BaseCryptLib.Eku",   TestInvalidParameters,                NULL, NULL, NULL},
    {"TestEKUSubsetSupersetFails()",           "CryptoPkg.BaseCryptLib.Eku",   TestEKUSubsetSupersetFails,           NULL, NULL, NULL},
};

UINTN mPkcs7EkuTestNum = ARRAY_SIZE(mPkcs7EkuTest);
