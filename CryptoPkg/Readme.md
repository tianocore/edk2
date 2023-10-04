# Crypto Package

This package provides cryptographic services that are used to implement firmware
features such as UEFI Secure Boot, Measured Boot, firmware image authentication,
and network boot. The cryptographic service implementation in this package uses
services from the [OpenSSL](https://www.openssl.org/) project and
[MbedTLS](https://www.trustedfirmware.org/projects/mbed-tls/) project.

EDK II firmware modules/libraries that requires the use of cryptographic
services can either statically link all the required services, or the EDK II
firmware module/library can use a dynamic Protocol/PPI service to call
cryptographic services. The dynamic Protocol/PPI services are only available to
PEIMs, DXE Drivers, UEFI Drivers, and SMM Drivers, and only if the cryptographic
modules are included in the platform firmware image.

There may be firmware image size differences between the static and dynamic
options. Some experimentation may be required to find the solution that
provides the smallest overall firmware overhead.

# Public Library Classes

* **BaseCryptLib**        - Provides library functions based on OpenSSL for
                            cryptographic primitives.
* **BaseCryptLibMbedTls** - Provides library functions based on MbedTLS for
                            cryptographic primitives.
* **TlsLib**              - Provides TLS library functions for EFI TLS protocol.
* **HashApiLib**          - Provides Unified API for different hash implementations.

# Private Library Classes

* **OpensslLib**   - Provides library functions from the openssl project.
* **MbedTlsLib**   - Provides library functions from the mbedtls project.
* **IntrinsicLib** - Provides C runtime library (CRT) required by openssl
                     and mbedtls.

# Private Protocols and PPIs

* **EDK II Crypto PPI**          - PPI that provides all the services from
                                   the BaseCryptLib and TlsLib library classes.
* **EDK II Crypto Protocol**     - Protocol that provides all the services from
                                   the BaseCryptLib and TlsLib library classes.
* **EDK II SMM Crypto Protocol** - SMM Protocol that provides all the services
                                   from the BaseCryptLib and TlsLib library
                                   classes.

## Statically Linking Cryptographic Services

The figure below shows an example of a firmware module that requires the use of
cryptographic services. The cryptographic services are provided by three library
classes called BaseCryptLib, TlsLib, and HashApiLib. These library classes are
implemented using APIs from the OpenSSL project that are abstracted by the
private library class called OpensslLib. The OpenSSL project implementation
depends on C runtime library services. The EDK II project does not provide a
full C runtime library for firmware components. Instead, the CryptoPkg includes
the smallest subset of services required to build the OpenSSL project in the
private library class called IntrinsicLib.

The CryptoPkg provides several instances of the BaseCryptLib and OpensslLib with
different cryptographic service features and performance optimizations. The
platform developer must select the correct instances based on cryptographic
service requirements in each UEFI/PI firmware phase (SEC, PEI, DXE, UEFI,
UEFI RT, and SMM), firmware image size requirements, and firmware boot
performance requirements.

```
+================================+
| EDK II Firmware Module/Library |
+================================+
     ^          ^         ^
     |          |         |
     |          |         v
     |          |   +============+
     |          |   | HashApiLib |
     |          |   +============+
     |          |         ^
     |          |         |
     v          v         v
+========+  +====================+
| TlsLib |  |    BaseCryptLib    |
+========+  +====================+
     ^                ^
     |                |
     v                v
+================================+
|     OpensslLib (Private)       |
+================================+
               ^
               |
               v
+================================+
|     IntrinsicLib (Private)     |
+================================+
```

## Dynamically Linking Cryptographic Services

The figure below shows the entire stack when dynamic linking is used with
cryptographic services produced by the CryptoPei, CryptoDxe, or CryptoSmm module
through a PPI/Protocol. This solution requires the CryptoPei, CryptoDxe, and
CryptoSmm modules to be configured with the set of cryptographic services
required by all the PEIMs, DXE Drivers, UEFI Drivers, and SMM Drivers. Dynamic
linking is not available for SEC or UEFI RT modules.

The EDK II modules/libraries that require cryptographic services use the same
BaseCryptLib/TlsLib/HashApiLib APIs. This means no source changes are required
to use static linking or dynamic linking. It is a platform configuration option
to select static linking or dynamic linking. This choice can be made globally,
per firmware module type, or for individual modules.

```
+===================+    +===================+     +===================+
|    EDK II PEI     |    |  EDK II DXE/UEFI  |     |     EDK II SMM    |
|   Module/Library  |    |   Module/Library  |     |   Module/Library  |
+===================+    +===================+     +===================+
  ^   ^        ^           ^   ^        ^            ^   ^        ^
  |   |        |           |   |        |            |   |        |
  |   |        v           |   |        v            |   |        v
  |   |  +==========+      |   |  +==========+       |   |  +==========+
  |   |  |HashApiLib|      |   |  |HashApiLib|       |   |  |HashApiLib|
  |   |  +==========+      |   |  +==========+       |   |  +==========+
  |   |        ^           |   |        ^            |   |        ^
  |   |        |           |   |        |            |   |        |
  v   v        v           v   v        v            v   v        v
+===================+    +===================+     +===================+
|TlsLib|BaseCryptLib|    |TlsLib|BaseCryptLib|     |TlsLib|BaseCryptLib|
+-------------------+    +-------------------+     +-------------------+
|   BaseCryptLib    |    |   BaseCryptLib    |     |   BaseCryptLib    |
|   OnPpiProtocol/  |    |   OnPpiProtocol/  |     |   OnPpiProtocol/  |
|  PeiCryptLib.inf  |    |   DxeCryptLib.inf |     |  SmmCryptLib.inf  |
+===================+    +===================+     +===================+
           ^                      ^                         ^
          ||| (Dynamic)          ||| (Dynamic)             ||| (Dynamic)
           v                      v                         v
+===================+    +===================+    +=====================+
|     Crypto PPI    |    |  Crypto Protocol  |    | Crypto SMM Protocol |
+-------------------|    |-------------------|    |---------------------|
|     CryptoPei     |    |     CryptoDxe     |    |      CryptoSmm      |
+===================+    +===================+    +=====================+
     ^       ^                ^       ^                 ^       ^
     |       |                |       |                 |       |
     v       |                v       |                 v       |
+========+   |           +========+   |            +========+   |
| TlsLib |   |           | TlsLib |   |            | TlsLib |   |
+========+   v           +========+   v            +========+   v
  ^  +==============+      ^  +==============+       ^  +==============+
  |  | BaseCryptLib |      |  | BaseCryptLib |       |  | BaseCryptLib |
  |  +==============+      |  +==============+       |  +==============+
  |          ^             |          ^              |          ^
  |          |             |          |              |          |
  v          v             v          v              v          v
+===================+    +===================+     +===================+
|    OpensslLib     |    |    OpensslLib     |     |    OpensslLib     |
+===================+    +===================+     +===================+
          ^                        ^                         ^
          |                        |                         |
          v                        v                         v
+===================+    +===================+     +===================+
|    IntrinsicLib   |    |    IntrinsicLib   |     |    IntrinsicLib   |
+===================+    +===================+     +===================+
```

## Supported Cryptographic Families and Services

The table below provides a summary of the supported cryptographic services. It
indicates if the family or service is deprecated or recommended to not be used.
It also shows which *CryptLib library instances support the family or service.
If a cell is blank then the service or family is always disabled and the
`PcdCryptoServiceFamilyEnable` setting for that family or service is ignored.
If the cell is not blank, then the service or family is configurable using
`PcdCryptoServiceFamilyEnable` as long as the correct OpensslLib or TlsLib is
also configured.

|Key      | Description                                                                    |
|---------|--------------------------------------------------------------------------------|
| <blank> | Family or service is always disabled.                                          |
| C       | Configurable using PcdCryptoServiceFamilyEnable.                               |
| C-Tls   | Configurable using PcdCryptoServiceFamilyEnable. Requires TlsLib.inf.          |
| C-Full  | Configurable using PcdCryptoServiceFamilyEnable. Requires OpensslLibFull*.inf. |

|Family/Service                   | Deprecated | Don't Use | SecCryptLib | PeiCryptLib | BaseCryptLib | SmmCryptLib | RuntimeCryptLib |
|:--------------------------------|:----------:|:---------:|:-----------:|:-----------:|:------------:|:-----------:|:---------------:|
| HmacMd5                         |     Y      |     Y     |             |             |              |             |                 |
| HmacSha1                        |     Y      |     Y     |             |             |              |             |                 |
| HmacSha256                      |     N      |     N     |             |      C      |      C       |      C      |        C        |
| HmacSha384                      |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Md4                             |     Y      |     Y     |             |             |              |             |                 |
| Md5                             |     Y      |     Y     |             |      C      |      C       |      C      |        C        |
| Pkcs.Pkcs1v2Encrypt             |     N      |     N     |             |             |      C       |      C      |                 |
| Pkcs.Pkcs5HashPassword          |     N      |     N     |             |             |      C       |      C      |                 |
| Pkcs.Pkcs7Verify                |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Pkcs.VerifyEKUsInPkcs7Signature |     N      |     N     |             |      C      |      C       |      C      |                 |
| Pkcs.Pkcs7GetSigners            |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Pkcs.Pkcs7FreeSigners           |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Pkcs.Pkcs7Sign                  |     N      |     N     |             |             |      C       |             |                 |
| Pkcs.Pkcs7GetAttachedContent    |     N      |     N     |             |      C      |      C       |      C      |                 |
| Pkcs.Pkcs7GetCertificatesList   |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Pkcs.AuthenticodeVerify         |     N      |     N     |             |             |      C       |             |                 |
| Pkcs.ImageTimestampVerify       |     N      |     N     |             |             |      C       |             |                 |
| Dh                              |     N      |     N     |             |             |      C       |             |                 |
| Random                          |     N      |     N     |             |             |      C       |      C      |        C        |
| Rsa.VerifyPkcs1                 |     Y      |     Y     |             |             |              |             |                 |
| Rsa.New                         |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Rsa.Free                        |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Rsa.SetKey                      |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Rsa.GetKey                      |     N      |     N     |             |             |      C       |             |                 |
| Rsa.GenerateKey                 |     N      |     N     |             |             |      C       |             |                 |
| Rsa.CheckKey                    |     N      |     N     |             |             |      C       |             |                 |
| Rsa.Pkcs1Sign                   |     N      |     N     |             |             |      C       |             |                 |
| Rsa.Pkcs1Verify                 |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Sha1                            |     N      |     Y     |             |      C      |      C       |      C      |        C        |
| Sha256                          |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Sha384                          |     N      |     N     |      C      |      C      |      C       |      C      |        C        |
| Sha512                          |     N      |     N     |      C      |      C      |      C       |      C      |        C        |
| X509                            |     N      |     N     |             |             |      C       |      C      |        C        |
| Tdes                            |     Y      |     Y     |             |             |              |             |                 |
| Aes.GetContextSize              |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Aes.Init                        |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Aes.EcbEncrypt                  |     Y      |     Y     |             |             |              |             |                 |
| Aes.EcbDecrypt                  |     Y      |     Y     |             |             |              |             |                 |
| Aes.CbcEncrypt                  |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Aes.CbcDecrypt                  |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Arc4                            |     Y      |     Y     |             |             |              |             |                 |
| Sm3                             |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Hkdf                            |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Tls                             |     N      |     N     |             |             |    C-Tls     |             |                 |
| TlsSet                          |     N      |     N     |             |             |    C-Tls     |             |                 |
| TlsGet                          |     N      |     N     |             |             |    C-Tls     |             |                 |
| RsaPss.Sign                     |     N      |     N     |             |             |      C       |             |                 |
| RsaPss.Verify                   |     N      |     N     |             |      C      |      C       |      C      |                 |
| ParallelHash                    |     N      |     N     |             |             |              |      C      |                 |
| AeadAesGcm                      |     N      |     N     |             |             |      C       |             |                 |
| Bn                              |     N      |     N     |             |             |      C       |             |                 |
| Ec                              |     N      |     N     |             |             |    C-Full    |             |                 |

## Platform Configuration of Cryptographic Services

Configuring the cryptographic services requires library mappings and PCD
settings in a platform DSC file. This must be done for each of the firmware
phases (SEC, PEI, DXE, UEFI, SMM, UEFI RT).

The following table can be used to help select the best OpensslLib instance for
each phase. The Size column only shows the estimated size increase for a
compressed IA32/X64 module that uses the cryptographic services with
`OpensslLib.inf` as the baseline size. The actual size increase depends on the
specific set of enabled cryptographic services. If ECC services are not
required, then the size can be reduced by using OpensslLib.inf instead of
`OpensslLibFull.inf`. Performance optimization requires a size increase.

| OpensslLib Instance     | SSL | ECC | Perf Opt |      CPU Arch    | Size  |
|:------------------------|:---:|:---:|:--------:|:----------------:|:-----:|
| OpensslLibCrypto.inf    |  N  |  N  |    N     |        All       |   +0K |
| OpensslLib.inf          |  Y  |  N  |    N     |        All       |   +0K |
| OpensslLibAccel.inf     |  Y  |  N  |    Y     | IA32/X64/AARCH64 |  +20K |
| OpensslLibFull.inf      |  Y  |  Y  |    N     |        All       | +115K |
| OpensslLibFullAccel.inf |  Y  |  Y  |    Y     | IA32/X64/AARCH64 | +135K |

### SEC Phase Library Mappings

The SEC Phase only supports static linking of cryptographic services. The
following library mappings are recommended for the SEC Phase. It uses the SEC
specific version of the BaseCryptLib and the null version of the TlsLib because
TLS services are not typically used in SEC.

```
[LibraryClasses.common.SEC]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SecCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
```

### PEI Phase Library Mappings

The PEI Phase supports either static or dynamic linking of cryptographic
services. The following library mappings are recommended for the PEI Phase. It
uses the PEI specific version of the BaseCryptLib and the null version of the
TlsLib because TLS services are not typically used in PEI.

```
[LibraryClasses.common.PEIM]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
```

If dynamic linking is used, then all PEIMs except CryptoPei use the following
library mappings. The CryptoPei module uses the static linking settings.

```
[LibraryClasses.common.PEIM]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/PeiCryptLib.inf

[Components]
  CryptoPkg/Driver/CryptoPei.inf {
    <LibraryClasses>
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
      TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
      IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  }
```

### DXE Phase, UEFI Driver, UEFI Application Library Mappings

The DXE/UEFI Phase supports either static or dynamic linking of cryptographic
services. The following library mappings are recommended for the DXE/UEFI Phase.
It uses the DXE specific version of the BaseCryptLib and the full version of the
OpensslLib and TlsLib. If ECC services are not required then a smaller
OpensslLib instance can be used.

```
[LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
```

If dynamic linking is used, then all DXE Drivers except CryptoDxe use the
following library mappings. The CryptoDxe module uses the static linking
settings.

```
[LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/DxeCryptLib.inf

[Components]
  CryptoPkg/Driver/CryptoDxe.inf {
    <LibraryClasses>
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
      TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
      IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  }
```

### SMM Phase Library Mappings

The SMM Phase supports either static or dynamic linking of cryptographic
services. The following library mappings are recommended for the SMM Phase. It
uses the SMM specific version of the BaseCryptLib and the null version of the
TlsLib.

```
[LibraryClasses.common.DXE_SMM_DRIVER]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
```

If dynamic linking is used, then all SMM Drivers except CryptoSmm use the
following library mappings. The CryptoDxe module uses the static linking
settings.

```
[LibraryClasses.common.DXE_SMM_DRIVER]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/SmmCryptLib.inf

[Components]
  CryptoPkg/Driver/CryptoSmm.inf {
    <LibraryClasses>
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
      TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
      IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  }
```

### UEFI Runtime Driver Library Mappings

UEFI Runtime Drivers only support static linking of cryptographic services.
The following library mappings are recommended for UEFI Runtime Drivers. They
use the runtime specific version of the BaseCryptLib and the null version of the
TlsLib because TLS services are not typically used at runtime.

```
[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/RuntimeCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
```

### PCD Configuration Settings

There are 2 PCD settings that are used to configure cryptographic services.
`PcdHashApiLibPolicy` is used to configure the hash algorithm provided by the
BaseHashApiLib library instance. `PcdCryptoServiceFamilyEnable` is used to
configure the cryptographic services supported by the CryptoPei, CryptoDxe,
and CryptoSmm modules.

* `gEfiCryptoPkgTokenSpaceGuid.PcdHashApiLibPolicy` - This PCD indicates the
  HASH algorithm to use in the BaseHashApiLib to calculate hash of data. The
  default hashing algorithm for BaseHashApiLib is set to HASH_ALG_SHA256.
  |  Setting   |    Algorithm     |
  |------------|------------------|
  | 0x00000001 | HASH_ALG_SHA1    |
  | 0x00000002 | HASH_ALG_SHA256  |
  | 0x00000004 | HASH_ALG_SHA384  |
  | 0x00000008 | HASH_ALG_SHA512  |
  | 0x00000010 | HASH_ALG_SM3_256 |

* `gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable` - Enable/Disable
   the families and individual services produced by the EDK II Crypto
   Protocols/PPIs. The default is all services disabled. This Structured PCD is
   associated with the `PCD_CRYPTO_SERVICE_FAMILY_ENABLE` structure that is
   defined in `Include/Pcd/PcdCryptoServiceFamilyEnable.h`.

   There are three layers of priority that determine if a specific family or
   individual cryptographic service is actually enabled in the CryptoPei,
   CryptoDxe, and CryptoSmm modules.

   1) OpensslLib instance selection. When the CryptoPei, CryptoDxe, or CryptoSmm
      drivers are built, they are statically linked to an OpensslLib library
      instance. If the required cryptographic service is not enabled in the
      OpensslLib instance linked, then the service is always disabled.
   2) BaseCryptLib instance selection.
      * CryptoPei is always linked with the PeiCryptLib instance of the
        BaseCryptLib library class. The table above has a column for the
        PeiCryptLib. If the family or service is blank, then that family or
        service is always disabled.
      * CryptoDxe is always linked with the BaseCryptLib instance of the
        BaseCryptLib library class. The table above has a column for the
        BaseCryptLib. If the family or service is blank, then that family or
        service is always disabled.
      * CryptoSmm is always linked with the SmmCryptLib instance of the
        BaseCryptLib library class. The table above has a column for the
        SmmCryptLib. If the family or service is blank, then that family or
        service is always disabled.
   3) If a family or service is enabled in the OpensslLib instance and it is
      enabled in the BaseCryptLib instance, then it can be enabled/disabled
      using `PcdCryptoServiceFamilyEnable`. This structured PCD is associated
      with the `PCD_CRYPTO_SERVICE_FAMILY_ENABLE` data structure that contains
      bit fields for each family of services. All of the families are disabled
      by default. An entire family of services can be enabled by setting the
      family field to the value `PCD_CRYPTO_SERVICE_ENABLE_FAMILY`. Individual
      services can be enabled by setting a single service name (bit) to `TRUE`.
      Settings listed later in the DSC file have priority over settings listed
      earlier in the DSC file, so it is valid for an entire family to be enabled
      first and then for a few individual services to be disabled by setting
      those service names to `FALSE`.

#### Common PEI PcdCryptoServiceFamilyEnable Settings

```
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha256.Family                    | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha384.Family                    | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha1.Family                          | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha256.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha384.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha512.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sm3.Family                           | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Family                           | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.Pkcs1Verify             | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.New                     | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.Free                    | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.SetKey                  | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs5HashPassword      | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Hkdf.Family                          | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
```

#### Common DXE and SMM PcdCryptoServiceFamilyEnable Settings

```
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha256.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha384.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Hkdf.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs1v2Encrypt             | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs5HashPassword          | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs7Verify                | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.VerifyEKUsInPkcs7Signature | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs7GetSigners            | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs7FreeSigners           | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.AuthenticodeVerify         | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Random.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.Pkcs1Verify                 | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.New                         | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.Free                        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.SetKey                      | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.GetPublicKeyFromX509        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha1.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha256.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha256.Services.HashAll                  | FALSE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Services.GetSubjectName             | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Services.GetCommonName              | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Services.GetOrganizationName        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Services.GetTBSCert                 | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Tls.Family                               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.TlsSet.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.TlsGet.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.GetContextSize              | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.Init                        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.CbcEncrypt                  | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.CbcDecrypt                  | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.AeadAesGcm.Services.Encrypt              | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.AeadAesGcm.Services.Decrypt              | TRUE
```
