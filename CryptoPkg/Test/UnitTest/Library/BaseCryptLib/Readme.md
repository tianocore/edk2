# BaseCryptLib Signature Vectors

These verifier tests use checked-in signature vectors. To regenerate a vector, run the listed profile
from the `edk2` repository root. The generator updates that profile's arrays in the corresponding
header; generation is a maintainer task and is not performed by unit tests.

The helper's requirements, options, design, all generated cases, and Python test instructions are in
[`CryptoPkg/Test/Tools/README.md`](../../../Tools/README.md).

## ECDSA: `EcTests.c`

Updates `VerifyTestSignatures.h` with the P-256 SHA-256 verifier signature.

```powershell
python CryptoPkg/Test/Tools/GenerateBaseCryptLibTestSignatures.py --algorithm ecdsa
```

## EdDSA: `EdDsaTests.c`

Updates `VerifyTestSignatures.h` with the Ed448 message and context signatures.

```powershell
python CryptoPkg/Test/Tools/GenerateBaseCryptLibTestSignatures.py --algorithm eddsa
```

## ML-DSA: `MlDsaTests.c`

Updates `VerifyTestSignatures.h` with the ML-DSA-87 message, context, empty-message, maximum-context,
and multiple-message signatures.

```powershell
python CryptoPkg/Test/Tools/GenerateBaseCryptLibTestSignatures.py --algorithm mldsa
```

## PKCS#7: `RsaPkcs7Tests.c`

Updates `VerifyTestSignatures.h` with attached PKCS#7 message and partial-chain signatures.

```powershell
python CryptoPkg/Test/Tools/GenerateBaseCryptLibTestSignatures.py --algorithm pkcs7
```

## SLH-DSA: `SlhDsaTests.c`

Updates `SlhDsaTestVectors.h` with the SLH-DSA-SHAKE-256s message, context, empty-message,
255-byte binary-context, and second-message signatures.

```powershell
python CryptoPkg/Test/Tools/GenerateBaseCryptLibTestSignatures.py --algorithm slh-dsa
```

To provide a separate SLH-DSA PEM key, add `--key C:\path\to\slh-dsa-key.pem` to that command.
