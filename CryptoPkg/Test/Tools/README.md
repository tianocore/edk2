# BaseCryptLib Offline Signature Vectors

## Purpose

Verifier unit tests must not use the firmware signing APIs being removed. Maintainers generate
signatures offline from test-only private-key fixtures, check the resulting vectors into the test
headers, and run the unit tests against verification APIs only. Normal builds and test runs do not
invoke this generator or need its signing dependencies.

The tool is `GenerateBaseCryptLibTestSignatures.py`. It generates one selected profile and updates
only that profile's arrays in its existing header. It never writes private keys into generated
headers.

## Requirements

- The helper is required to support Windows and Linux host platforms.
- Python 3.10 or newer.
- OpenSSL command-line executable. OpenSSL 3.5 or newer is required for ML-DSA and SLH-DSA. The
    EdDSA context vector requires support for the `hexcontext-string` option.
- OpenSSL `libcrypto` shared library for empty-message ML-DSA and SLH-DSA signatures. On Windows, one
    `libcrypto-*-x64.dll` must be beside the selected OpenSSL executable. On Linux, `libcrypto` must be
    the same major version as OpenSSL; the helper checks the executable prefix's `lib` and `lib64`
    directories before using the system library lookup.
- The checked-in test fixtures, unless supplying an SLH-DSA key with `--key`.

## Command-Line Options

Run `python GenerateBaseCryptLibTestSignatures.py --help` for the live help text.

| Option | Description |
|---|---|
| `--algorithm` | Required profile: `ecdsa`, `eddsa`, `mldsa`, `pkcs7`, or `slh-dsa`. |
| `--openssl` | OpenSSL executable path; defaults to `openssl` from `PATH`. |
| `--test-dir` | Directory containing profile fixtures; defaults to the BaseCryptLib test directory. |
| `--output-dir` | Directory for the generated header; defaults to the BaseCryptLib test directory. |
| `--output-header` | Output header name or path; defaults to the selected profile's header name. |
| `--output` | Compatibility option for overriding the complete output-header path. |
| `--key` | SLH-DSA PEM private-key override. Accepted only with `--algorithm slh-dsa`; otherwise the test key is read from `SlhDsaTestVectors.h`. |

The defaults update `CryptoPkg/Test/UnitTest/Library/BaseCryptLib/VerifyTestSignatures.h` for ECDSA,
EdDSA, ML-DSA, and PKCS#7, and `SlhDsaTestVectors.h` for SLH-DSA. Use `--output-dir` to select another
directory and `--output-header` to select another header name. `--output` remains available for a
complete path override. The selected output header must already exist and contain each array produced
by the chosen profile.

## Profiles And Unit Tests

The profile-specific C tests and vector headers are in
`CryptoPkg/Test/UnitTest/Library/BaseCryptLib`. The per-test README there also lists these commands
beside the verifier tests.

### ECDSA: `EcTests.c`

Generates the P-256 SHA-256 verifier signature `mEcDsaTestSignature` in `VerifyTestSignatures.h`.

```powershell
python GenerateBaseCryptLibTestSignatures.py --algorithm ecdsa --openssl C:\path\to\openssl.exe
```

### EdDSA: `EdDsaTests.c`

Generates the Ed448 message and context signatures in `VerifyTestSignatures.h`.

```powershell
python GenerateBaseCryptLibTestSignatures.py --algorithm eddsa --openssl C:\path\to\openssl.exe
```

### ML-DSA: `MlDsaTests.c`

Generates the ML-DSA-87 message, context, empty-message, maximum-context, and multiple-message
signatures in `VerifyTestSignatures.h`.

```powershell
python GenerateBaseCryptLibTestSignatures.py --algorithm mldsa --openssl C:\path\to\openssl.exe
```

### PKCS#7: `RsaPkcs7Tests.c`

Generates a certificate-key RSA PKCS#1 signature, attached PKCS#7 test content signed by the
issued test certificate, and the non-self-issued partial-chain vector in `VerifyTestSignatures.h`.

```powershell
python GenerateBaseCryptLibTestSignatures.py --algorithm pkcs7 --openssl C:\path\to\openssl.exe
```

### SLH-DSA: `SlhDsaTests.c`

Generates the default, context-bound, empty-message, 255-byte binary-context, and second-message
SLH-DSA-SHAKE-256s signatures in `SlhDsaTestVectors.h`.

```powershell
python GenerateBaseCryptLibTestSignatures.py --algorithm slh-dsa --openssl C:\path\to\openssl.exe
```

To use a separate SLH-DSA PEM fixture:

```powershell
python GenerateBaseCryptLibTestSignatures.py --algorithm slh-dsa --openssl C:\path\to\openssl.exe --key C:\path\to\slh-dsa-key.pem
```

## Generator Design

The shared `SignatureGenerator` base class provides fixture extraction and common options. Each
algorithm has its own generator class, registered by name in `GENERATOR_TYPES`; adding a profile
means implementing its vector generation and declaring its output header. `SignatureVector` records
the array name, bytes, description, and declaration style. Shared functions handle OpenSSL execution,
temporary files, ECDSA DER conversion, C formatting, and in-place replacement of named arrays while
preserving unrelated header content.

OpenSSL `pkeyutl` cannot sign a zero-byte input. Empty-message ML-DSA and SLH-DSA vectors therefore use
OpenSSL EVP through `libcrypto`; all other raw signature cases use the OpenSSL command line. Signing
remains an explicit, offline maintainer action with test-only keys.

## Python Unit Tests

The tests use only Python's standard library and do not sign with OpenSSL. From the `edk2` repository
root, run:

```powershell
python -m unittest discover -s CryptoPkg/Test/Tools -p "test_*.py" -v
```

The suite checks the real `--help` subprocess, argument validation and profile registration, Windows
and POSIX `libcrypto` loading, fixture parsing, ECDSA DER conversion, vector formatting/header updates,
and binary context encoding using a mocked OpenSSL runner. The help invocation is therefore verified as
part of the ordinary unit-test run.
