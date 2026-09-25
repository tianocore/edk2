# BaseCryptLib Host Unit Tests

## SLH-DSA Precomputed Signatures

### Background
SLH-DSA (FIPS 205, formerly SPHINCS+) is a stateless hash-based digital signature algorithm.
Generating an SLH-DSA-SHAKE-256s signature involves evaluating millions of Keccak hash rounds
across FORS and XMSS tree structures. On unoptimized or 32-bit builds (`NOOPT` / `IA32`), a single
live signing operation takes several minutes of CPU time.

To maintain complete test coverage across all 18 test cases without excessive execution times,
precomputed signature test vectors are stored in `SlhDsaTestVectors.h` and used for verification,
tampering, context validation, and boundary checking tests.

### Generated Test Vectors
`SlhDsaTestVectors.h` contains the following precomputed signatures matching `mSlhDsaShake256sTestPemKey` / `mSlhDsaShake256sTestCert`:
* `mSlhDsaShake256sTestSignature`: Default test message without context.
* `mSlhDsaShake256sTestContextSignature`: Default test message with context string `"SLH-DSA test context"`.
* `mSlhDsaShake256sTestEmptyMsgSignature`: Empty test message (`""`) without context.
* `mSlhDsaShake256sTestMaxContextSignature`: Default test message with 255-byte maximum context.
* `mSlhDsaShake256sTestMsg2Signature`: Secondary test message (`"Second message"`) for multi-signature tests.

### Regenerating Signatures
When test keys or test messages are updated, the signatures can be regenerated using `GenerateSlhDsaSignatures.py`:

#### Prerequisites
* Python 3.10+
* OpenSSL 3.5+ CLI with SLH-DSA support

#### Command
```bash
python GenerateSlhDsaSignatures.py --openssl /path/to/openssl --output new_signatures.h
```
Paste the generated arrays into `SlhDsaTestVectors.h`.
