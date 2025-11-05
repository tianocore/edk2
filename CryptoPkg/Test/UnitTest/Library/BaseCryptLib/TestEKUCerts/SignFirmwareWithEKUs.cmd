@ECHO OFF
REM   This script will use various certificates to sign blobs for testing purposes.
REM
REM
REM   Our EKU test certificate chain:
REM   ------------------------------------------
REM   |                                          | // Root of trust. ECDSA P521 curve
REM   |          TestEKUParsingRoot              | // SHA 256 Key Usage: CERT_DIGITAL_SIGNATURE_KEY_USAGE
REM   |                                          | // CERT_KEY_CERT_SIGN_KEY_USAGE | CERT_CRL_SIGN_KEY_USAGE
REM    ------------------------------------------
REM                      ^
REM                      |
REM    ------------------------------------------
REM   |                                          | // Issues subordinate CAs. ECC P384 curve.
REM   |       TestEKUParsingPolicyCA             | // SHA 256 Key Usage:
REM   |                                          | // CERT_KEY_CERT_SIGN_KEY_USAGE | CERT_CRL_SIGN_KEY_USAGE
REM    ------------------------------------------
REM                      ^
REM                      |
REM    ------------------------------------------
REM   |                                          | // Issues end-entity (leaf) signers. ECC P256 curve.
REM   |        TestEKUParsingIssuingCA           | // SHA 256 Key Usage: CERT_DIGITAL_SIGNATURE_KEY_USAGE
REM   |                                          | // Enhanced Key Usage:
REM    ------------------------------------------  // 1.3.6.1.4.1.311.76.9.21.1 (Surface firmware signing)
REM                      ^
REM                      |
REM       --------------------------------------
REM      /     TestEKUParsingLeafSigner &&     /   // Leaf signer,  ECC P256 curve.
REM     /    TestEKUParsingLeafSignerPid12345 /    // SHA 256 Key Usage: CERT_DIGITAL_SIGNATURE_KEY_USAGE
REM    /                                     /     // Enhanced Key usages:
REM    --------------------------------------      // 1.3.6.1.4.1.311.76.9.21.1 (Surface firmware signing)
REM                                                // 1.3.6.1.4.1.311.76.9.21.1.N, N == Product ID.
REM
REM
REM
REM  Dev Note:  SignTool.exe must be in your path when running this script.

del *.p7b
ECHO -------------------------------------------------------------------
ECHO Press any key 4 times to append time to the test blobs to sign.
time >> TestSignWithOneEKUInLeafSigner.bin
time >> TestSignWithTwoEKUsInLeafSignerPid1.bin
time >> TestSignWithTwoEKUsInLeafSignerPid12345.bin
time >> TestSignWithNoEKUsInLeafSigner.bin


REM
REM Create a signature with TestEKUParsingLeafSigner.cer which has one EKU in it,
REM and add the Policy CA in the signature.
REM
call signtool.exe sign /fd sha256 /f TestEKUParsingLeafSigner.cer                           /p7 .  /u 1.3.6.1.4.1.311.76.9.21.1    /ac TestEKUParsingPolicyCA.cer /p7co 1.2.840.113549.1.7.1 /p7ce DetachedSignedData /v /debug TestSignWithOneEKUInLeafSigner.bin

REM
REM Create a signature with two EKU's in the leaf signer.  (1.3.6.1.4.1.311.76.9.21.1, and 1.3.6.1.4.1.311.76.9.21.1.1)
REM
call signtool.exe sign /fd sha256 /f TestEKUParsingLeafSignerPid1.cer                           /p7 .  /u 1.3.6.1.4.1.311.76.9.21.1.1  /p7co 1.2.840.113549.1.7.1 /p7ce DetachedSignedData /v /debug TestSignWithTwoEKUsInLeafSignerPid1.bin

REM
REM Create a signature with two EKUs in the leaf (1.3.6.1.4.1.311.76.9.21.1, and 1.3.6.1.4.1.311.76.9.21.1.12345)
REM
call signtool.exe sign /fd sha256 /f TestEKUParsingLeafSignerPid12345.cer                           /p7 .  /u 1.3.6.1.4.1.311.76.9.21.1.12345   /p7co 1.2.840.113549.1.7.1 /p7ce DetachedSignedData /v /debug TestSignWithTwoEKUsInLeafSignerPid12345.bin


REM
REM Create a signature with a leaf that does not have any EKUs in the signture.
REM
call signtool.exe sign /fd sha256 /f TestEKUParsingNoEKUsInSigner.cer /p7 .  /p7co 1.2.840.113549.1.7.1 /p7ce DetachedSignedData /v /debug TestSignWithNoEKUsInLeafSigner.bin

REM
REM Rename *.p7 to *.p7b
REM
rename *.p7 *.p7b
ECHO ---------------------------------------------------------------------------
ECHO Now you can use your favorite "Binary To Hex" converter to convert the
ECHO signatures (P7B files) to byte arrays and add them to AllTestSignatures.h
ECHO ---------------------------------------------------------------------------
