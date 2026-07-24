# Firmware Management Protocol (FMP) Device Package

FmpDevicePkg provides the common resources necessary to manage the firmware on a given device. The
[UEFI Specification](https://uefi.org/specifications) defines several elements used in the firmware management process
that are implemented or depended upon in FmpDevicePkg such as:

1. `EFI_FIRMWARE_MANAGEMENT_PROTOCOL`
2. Firmware Management Protocol dependency expression support
3. FMP capsule format
4. EFI System Resource Table (ESRT)

## Package Organization

This section briefly describes the package modules and libraries.

### Modules

1. **CapsuleUpdatePolicyDxe** [readme](../CapsuleUpdatePolicyDxe/ReadMe.md)
   * **Purpose:** \
   Produces the Capsule Update Policy Protocol using the services of the Capsule Update Policy Library.
2. **FmpDxe** [readme](../FmpDxe/ReadMe.md)
   * **Purpose:** \
   Produces an instance of the Firmware Management Protocol (`EFI_FIRMWARE_MANAGEMENT_PROTOCOL`) that is used
   to support updates to a firmware image stored on a firmware device

### Libraries

1. **CapsuleUpdatePolicyLib**
   * **Purpose:** \
   Provides platform policy services used during a capsule update.

    1. **CapsuleUpdatePolicyLibNull** [readme](../Library/CapsuleUpdatePolicyLibNull/ReadMe.md)
    2. **CapsuleUpdatePolicyLibOnProtocol** [readme](../Library/CapsuleUpdatePolicyLibOnProtocol/ReadMe.md)
2. **FmpDependencyCheckLib**
   * **Purpose:** \
   Provides services to check that capsule dependencies are met during firmware update.

    1. **FmpDependencyCheckLib** [readme](../Library/FmpDependencyCheckLib/ReadMe.md)
    2. **FmpDependencyCheckLibNull** [readme](../Library/FmpDependencyCheckLibNull/ReadMe.md)
3. **FmpDependencyDeviceLib**
   * **Purpose:** \
   Provides firmware device specific services to support saving dependency expressions to a firmware device and
   getting dependency expressions from a firmware device.

    1. **FmpDependencyDeviceLibNull** [readme](../Library/FmpDependencyDeviceLibNull/ReadMe.md)
4. **FmpDependencyLib**
   * **Purpose:** \
   Provides functions used to manage dependencies between firmware components during the update of device firmware
   images.

    1. **FmpDependencyLib** [readme](../Library/FmpDependencyLib/ReadMe.md)
5. **FmpDeviceLib**
   * **Purpose:** \
   Provides firmware device specific services to support firmware updates on a given device.

    1. **FmpDeviceLibNull** [readme](../Library/FmpDeviceLibNull/ReadMe.md)
6. **FmpPayloadHeaderLib**
   * **Purpose:** \
   Provides services to retrieve values from a capsule FMP Payload Header.

    1. **FmpPayloadHeaderLibV1** [readme](../Library/FmpPayloadHeaderLibV1/ReadMe.md)

## Further Reading

Several documents describe important elements involved in understanding `FmpDevicePkg`. Consult the following
resource for more information on a particular topic.

1. `FmpDevicePkg` Overview
    1. The ReadMe documents referenced above that reside in the package.
    2. [Tianocore wiki: FmpDevicePkg](https://github.com/tianocore/tianocore.github.io/wiki/FmpDevicePkg)

2. UEFI Specification Definitions for Firmware Updating and Reporting
    1. [Section 23 of the UEFI Specification 2.8B](https://uefi.org/specifications)

3. Technical Overview of the EDK II Capsule Update and Recovery Flow
    1. [A Tour Beyond BIOS - Capsule Update and Recovery in EDK II](https://github.com/tianocore-docs/Docs/blob/master/White_Papers/A_Tour_Beyond_BIOS_Capsule_Update_and_Recovery_in_EDK_II.pdf)

4. Windows UEFI Firmware Update Resources
    1. [Windows UEFI Firmware Update Platform](https://docs.microsoft.com/en-us/windows-hardware/drivers/bringup/windows-uefi-firmware-update-platform)
    2. [Validating Windows UEFI Firmware Update Platform Functionality](https://docs.microsoft.com/en-us/windows-hardware/manufacture/desktop/validating-windows-uefi-firmware-update-platform-functionality)

5. NIST Guidelines for Authenticated Firmware Update
    1. [SP800-147](https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-147.pdf)
    2. [SP800-147B](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-147B.pdf)
    3. [SP800-193](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-193.pdf)
