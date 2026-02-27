# TCG TPM Package

This package provides libraries to implement software-based fTPM
device using [TCG TPM v2.0 reference implementation](https://github.com/TrustedComputingGroup/TPM) project.
For this, this packages provides below libraries:

* **TpmLib**              - Provides TPM funtionalities
                            on TCG TPM v2.0 reference implementation
                            for software-based fTPM.
* **PlatformTpmLib**      - Provides platform interface which
                            used by TCG TPM v2.0 reference implementation.

## Feature Overview

TpmLib is wrapper library to call TCG TPM v2.0 reference implementation
and The structure of TCG TPM v2.0 reference implementation is as follows:

                       +----------------+
                       |   TCG TPM LIB  |
                       +----------------+
                                |
                                |
              -------------------------------------
              |                                   |
+----------------------------+          +-------------------------+
|    Crypto / BigInt Library |          |     Platform Layer      |
|    (openssl or wolfssl)    |          |  (TPMCmd/Platform/src)  |
+----------------------------+          +-------------------------+

For cryptograph and big integer functionalities,
the TCG TPM 2.0 reference library will be built using EDK II’s **OpensslLibFull**
and files in **Include/Ossl, Ossl** directories handle to call
openssl APIs  using **BaseCryptoLib** APIs.

For the platform specific layer, TCG TPM v2.0 reference implementation
will use **PlatformTpmLib**.
To make TCG TPM v2.0 reference implementation use **PlatformTpmLib**,
**TpmLib** provides bridge layer named **TpmPlatformFunctions.c**:

+---------------+                     +---------------+      crypto/BigInt        +-----------+
|    TpmLib    |  ------------------> |  TCG TPM Lib  | ----------------------->  |  Openssl  |
+---------------+                     +---------------+            |              +-----------+
                                                                   |
                                                                   | _plat__XXX()
                                                                   |
                                                                   |
+--------------------------------+                    +-----------------------------+
|      PlatformTpmLib            |  <---------------- |       Platform Layer        |
|   (via TpmPlatformFunctions.c) |                    | (via TpmPlatformFunctions.c)|
+--------------------------------+                    +-----------------------------+

When **TpmLib** requests to handle a TPM command (via TpmLibExecuteCommand())
to TCG TPM v2.0 reference implementation, some command might requires
special handling according to platform (e.x) store into non-volatile storage,
get platform-seed and etc.

For this, TCG TPM v2.0 implemntation calls "plat__XXX()" functions
(See https://github.com/TrustedComputingGroup/TPM/tree/main/TPMCmd/Platform/src)
and this functions are implmented in **TpmPlatformFunctions.c** to call
correspondant function in **PlatformTpmLib**.
Therefore, each platform can handle the request in platform-specific way
by implementing each **PlatfomrTpmLib**.

## How the TCG2 TPM reference implementation integration in EDKII

### Build options

To build TCG TPM v2.0 implementation with the proper option in EDKII,
TcgTpmPkg uses two build configuration files:

  - TpmLib/TpmLibCompileOptions.h for libraries configuration and
    extra compile options.

  - TpmLib/Include/TpmConfiguration/TpmBuildSwitches.h instead of
    TPMCmd/TpmConfiguration/TpmBuildSwitch.h in TCG TPM v2.0

### C standard headers

Since TCG TPM v2.0 implementation uses several c standard functions.
To build TCG TPM v2.0 implementation properly, copy c standard headers
from **CryptoPkg/Library/Include** to **Private/Include/Standard**.
Because All of related symbols in **BaseCryptoLib**, It's enough to
copy header files and link with **BaseCryptoLib** to use c standard functions.

### Openssl Layer Integration

As describe in **Feature Overview**, TCG TPM v2.0 implementation uses
openssl APIs for cryptograph operations and big number operation.
To use EDKII's BaseCryptoLib APIs correspondant to openssl APIs used
in TCG TPM v2.0 implementation below files are used:

  - TpmLib/Include/Ossl/BnToOsslMath.h
  - TpmLib/Include/Ossl/TpmToOsslHash.h
  - TpmLib/Include/Ossl/TpmToOsslSym.h
  - TpmLib/Ossl/BnToOsslMath.c
  - TpmLib/Ossl/TpmToOsslSupport.c

NOTE: the directory name **OSSL** couldn't be changed since
related header files are included  TCG TPM v2.0 implementation like below:

  #include <Ossl/BnToOsslMatch.h>

## Example usage of TpmLib

Using TpmLib, software-based fTPM StandaloneMm driver (FtpmSmm)
is implmented in FVP_RevC model and Juno platform:

1) with UEFI

         UEFI (Normal world)       |         Secure World
    -------------------------------|------------------------------
                                   |
       +--------------+            | +-----------+      +----------+
       |    Tcg2Dxe   |            | |  FtpmSmm  |<---->|  TpmLib  |
       +--------------+            | +-----------+      +----------+
               |                   |       |
               |                   |       ----------
               |                   |                |
               |                   |                |
               |                   |       +------------------+
               |                   |       | StandaloneMmCpu  |
               |                   |       +------------------+
               |                   |                |
               |                   |                |
               |                   |                |
       +----------------------+    |       +----------------------------+
       |  Tpm2InstanceFfaLib  |<---------->| StandaloneMmCoreEntryPoint |
       +----------------------+    .       |      (Misc Service)        |
                                   .       +----------------------------+
                                   .
                      Communicate via CRB over FF-A

2) with linux-kernel

         linux (Normal world)      |         Secure World
    -------------------------------|------------------------------
                                   |
       +----------------------+    | +-----------+      +----------+
       |  TPM infra-structure |    | |  FtpmSmm  |<---->|  TpmLib  |
       +----------------------+    | +-----------+      +----------+
               |                   |       |
               |                   |       ----------
               |                   |                |
               |                   |                |
               |                   |       +------------------+
               |                   |       | StandaloneMmCpu  |
               |                   |       +------------------+
               |                   |                |
               |                   |                |
               |                   |                |
       +----------------------+    |       +----------------------------+
       |  tpm_crb_ffa driver  |<---------->| StandaloneMmCoreEntryPoint |
       +----------------------+    .       |      (Misc Service)        |
                                   .       +----------------------------+
                                   .
                     Communicate via CRB over FF-A

This kind of software stack can be used:
  - End to End measured boot with software-based TPM
  - Encrypted block device with software-based TPM
  - Arm CCA software stack.

For more detail of FtpmSmm, See **edk2-platform/Platform/ARM/Drivers/FtpmSmm**.

## Precautions

### PlatformTpmLib

  - Currently, Physical Presence is not implemented; instead,
    the default behavior of the TCG TPM 2.0 reference implementation is used,
    which always assumes physical presence (See _plat__PhysicalPresenceAsserted()).

  - Non-volatile storage should not be accessiable arbitrarily by
    normal world components (UEFI, OS and etc).

  - Non-volatile storage must be at least 16 KB in size
    (See the NV_MEMORY_SIZE in TCG TPM 2.0 reference implementation).

  - PlatformTpmLibGetEPS() is only called via TPM_Manufacture().
    EPS should be unique per device for example:

      EPS = SHA-512( TRNG_output || nonce || optional_mixing || DeviceUnique)

  - When platform generates platofrm secret values, it recommands
    to use DRBG_Generate() with TRNG.

### Error handling when FAIL() or FAIL_NORET() are called from TCG TPM implementation

When the TCG TPM 2.0 implementation encounters a FAIL() condition,
the current implementation does not allow any further execution of the TPM code.
The platform is notified of the FAIL() condition via PlatformTpmLibFail(),
after which a dead loop is entered to prevent any further execution.

### Limitations

  - Currently, fTPM implementation with TcgTpmPkg/TpmLib is supported only
    several AARCH64 platform -- VExpressPkg and JunoPkg.
