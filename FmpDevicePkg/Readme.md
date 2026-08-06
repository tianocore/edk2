# Firmware Management Protocol (FMP) Device Package

FmpDevicePkg provides the common resources necessary to manage the firmware on a given device. The
[UEFI Specification](https://uefi.org/specifications) defines several elements used in the firmware management process
that are implemented or depended upon in FmpDevicePkg such as:

1. [`EFI_FIRMWARE_MANAGEMENT_PROTOCOL`](https://github.com/tianocore/edk2/blob/HEAD/MdePkg/Include/Protocol/FirmwareManagement.h)
2. Firmware Management Protocol dependency expression support
3. FMP capsule format
4. EFI System Resource Table (ESRT)

## Architecture

`FmpDxe` is the central driver in `FmpDevicePkg`. It produces the Firmware Management Protocol
(`EFI_FIRMWARE_MANAGEMENT_PROTOCOL`) and coordinates with the supporting modules and libraries listed later in this
document to carry out a firmware update. This section describes the high level design of that update flow.

### Capsule Authentication

The firmware update capsule must be signed. FmpDxe verifies the integrity of the capsule contents. The
actual capsule data is preceded by an `EFI_FIRMWARE_IMAGE_AUTHENTICATION` structure. This structure contains a
monotonic count and a `WIN_CERTIFICATE_UEFI_GUID` member that contains a signature that covers both the monotonic
count and the capsule payload data. These two elements ensure replay protection across update operations and
authentication. The certificate type used must be `EFI_CERT_TYPE_PKCS7_GUID`.

An EDK II implementation of signature verification is available in the following `FmpAuthenticationLib` instance:
[SecurityPkg/Library/FmpAuthenticationLibPkcs7](https://github.com/tianocore/edk2/tree/master/SecurityPkg/Library/FmpAuthenticationLibPkcs7).

### Capsule Versioning

The capsule version should only be allowed to increment in value across updates to prevent rollback attacks. The
`EFI_FIRMWARE_IMAGE_DESCRIPTOR` structure contains `Version` and `LowestSupportedImageVersion` fields that are used
to check for compliance during firmware update. `Version` must be greater than or equal to `LowestSupportedImageVersion`
in the current firmware and greater than `Version` of the current firmware.

`FmpDxe` performs this check directly. The value used for `LowestSupportedImageVersion` is the greatest of the
build-time `PcdFmpDeviceBuildTimeLowestSupportedVersion` PCD value, the value returned by the `FmpDeviceLib`
instance's `FmpDeviceGetLowestSupportedVersion()` function, and the lowest supported version most recently saved
from an applied capsule's FMP Payload Header.

### Device-Specific Functionality During Update

A capsule can target firmware update to a diverse set of devices on a system. Each device might bring unique logic
and requirements to the firmware update process. Therefore, a library class called `FmpDeviceLib` exists that allows
for instances written specific to a particular device.

### Dependency Considerations

The UEFI Specification 2.8 version introduced support for expressing dependencies between components involved in a
capsule update. For instance, FW`x` can require FW`y` to be at least version 2.0 to install. This information is
primarily conveyed to `FmpDxe` through the `FmpDependencyCheckLib` and `FmpDependencyLib` library classes.

More information about the overall infrastructure is available in:

- The [UEFI Specification](https://uefi.org/specifications) - "Firmware Update and Reporting" section.
- [TianoCore Wiki: FMP Capsule Dependency Introduction](https://www.tianocore.org/tianocore-wiki.github.io/development/tutorials-howto/fmp_capsule_dependency_introduction.html)

### Update Policy

A library class (`CapsuleUpdatePolicyLib`) is used to make platform-specific policy decisions available to the
firmware update process. This includes information such as whether the system power/thermal state permits firmware
to be updated. A few functions also exist to modify expected behavior such as ignoring the
`LowestSupportedImageVersion` check or not locking the firmware device for update when the FMP lock event is signaled.
It is important to note that the latter functions should only be used in very rare special cases such as during
manufacturing flows.

## Package Organization

This section briefly describes the package modules and libraries.

### Modules

1. **CapsuleUpdatePolicyDxe**
   - **Purpose:** Produces the Capsule Update Policy Protocol using the services of the Capsule Update Policy Library.
2. **FmpDxe** [readme](FmpDxe/Readme.md)
   - **Purpose:** Produces an instance of the Firmware Management Protocol (`EFI_FIRMWARE_MANAGEMENT_PROTOCOL`) that is
     used to support updates to a firmware image stored on a firmware device

### Libraries

1. **CapsuleUpdatePolicyLib**
   - **Purpose:** Provides platform policy services used during a capsule update.
   - **Instances:**
     1. CapsuleUpdatePolicyLibNull
     2. CapsuleUpdatePolicyLibOnProtocol
2. **FmpDependencyCheckLib**
   - **Purpose:** Provides services to check that capsule dependencies are met during firmware update.
   - **Instances:**
     1. FmpDependencyCheckLib
     2. FmpDependencyCheckLibNull
3. **FmpDependencyDeviceLib**
   - **Purpose:** Provides firmware device specific services to support saving dependency expressions to a firmware
     device and getting dependency expressions from a firmware device.
   - **Instances:**
     1. FmpDependencyDeviceLibNull
4. **FmpDependencyLib**
   - **Purpose:** Provides functions used to manage dependencies between firmware components during the update of device firmware
     images.
   - **Instances:**
     1. FmpDependencyLib
5. **FmpDeviceLib**
   - **Purpose:** Provides firmware device specific services to support firmware updates on a given device.
   - **Instances:**
     1. FmpDeviceLibNull
6. **FmpPayloadHeaderLib**
   - **Purpose:** Provides services to retrieve values from a capsule FMP Payload Header.
   - **Instances:**
     1. FmpPayloadHeaderLibV1

## Further Reading

Several documents describe important elements involved in understanding `FmpDevicePkg`. Consult the following
resource for more information on a particular topic.

1. `FmpDevicePkg` Overview
    1. The Readme documents in this package.
    2. [TianoCore Wiki: FmpDevicePkg](https://www.tianocore.org/tianocore-wiki.github.io/platforms-packages/core-packages/fmp_device_pkg.html)

2. UEFI Specification Definitions for Firmware Updating and Reporting
    1. [UEFI Specification - Firmware Update and Reporting](https://uefi.org/specifications)

3. Technical Overview of the EDK II Capsule Update and Recovery Flow
    1. [A Tour Beyond BIOS - Capsule Update and Recovery in EDK II](https://github.com/tianocore-docs/Docs/blob/master/White_Papers/A_Tour_Beyond_BIOS_Capsule_Update_and_Recovery_in_EDK_II.pdf)

4. Windows UEFI Firmware Update Resources
    1. [Windows UEFI Firmware Update Platform](https://docs.microsoft.com/windows-hardware/drivers/bringup/windows-uefi-firmware-update-platform)
    2. [Validating Windows UEFI Firmware Update Platform Functionality](https://docs.microsoft.com/windows-hardware/manufacture/desktop/validating-windows-uefi-firmware-update-platform-functionality)

5. NIST Guidelines for Authenticated Firmware Update
    1. [SP800-147](https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-147.pdf)
    2. [SP800-147B](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-147B.pdf)
    3. [SP800-193](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-193.pdf)
