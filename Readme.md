# EDK II Project

A modern, feature-rich, cross-platform firmware development environment
for the UEFI and PI specifications from www.uefi.org.

Contributions to the EDK II open source project are covered by the
[TianoCore Contribution Agreement 1.1](Contributions.txt)

The majority of the content in the EDK II open source project uses a
[BSD 2-Clause License](License.txt).  The EDK II open source project contains
the following components that are covered by additional licenses:
* [AppPkg/Applications/Python/Python-2.7.2/Tools/pybench](AppPkg/Applications/Python/Python-2.7.2/Tools/pybench/LICENSE)
* [AppPkg/Applications/Python/Python-2.7.2](AppPkg/Applications/Python/Python-2.7.2/LICENSE)
* [AppPkg/Applications/Python/Python-2.7.10](AppPkg/Applications/Python/Python-2.7.10/LICENSE)
* [BaseTools/Source/C/BrotliCompress](BaseTools/Source/C/BrotliCompress/LICENSE)
* [MdeModulePkg/Library/BrotliCustomDecompressLib](MdeModulePkg/Library/BrotliCustomDecompressLib/LICENSE)
* [OvmfPkg](OvmfPkg/License.txt)
* [CryptoPkg/Library/OpensslLib/openssl](CryptoPkg/Library/OpensslLib/openssl/LICENSE)

The EDK II Project is composed of packages.  The maintainers for each package
are listed in [Maintainers.txt](Maintainers.txt).

# Resources
* [TianoCore](http://www.tianocore.org)
* [EDK II](https://github.com/tianocore/tianocore.github.io/wiki/EDK-II)
* [Getting Started with EDK II](https://github.com/tianocore/tianocore.github.io/wiki/Getting-Started-with-EDK-II)
* [Mailing Lists](https://github.com/tianocore/tianocore.github.io/wiki/Mailing-Lists)
* [TianoCore Bugzilla](https://bugzilla.tianocore.org)
* [How To Contribute](https://github.com/tianocore/tianocore.github.io/wiki/How-To-Contribute)

# FmpDevicePkg-master edk2-staging branch

The FmpDevicePkg provides a simple method to support UEFI Capsules for firmware
update of system firmware or devices using the Firmware Management Protocol.

The content in this edk2-staging branch is based on content from the following:

  https://github.com/Microsoft/MS_UEFI/tree/share/MsCapsuleSupport/MsCapsuleUpdatePkg

The Firmware Management Protocol advertises that a component supports a firmware
update using a UEFI capsule.  The FmpDevicePkg provides library classes and PCDs
used to customize the behavior of a Firmware Management Protocol instance.

## Goals
The goal of this branch in edk2-staging is to provide a version of the source
code that can be used to validate the functionality before it is added to
edk2/master.  The target date for integration into edk2/master is early August
2018.

## Developers
* Michael D Kinney <michael.d.kinney@intel.com>
* Sean Brogan <sean.brogan@microsoft.com>
* Jiewen Yao <jiewen.yao@intel.com>
* Yonghong Zhu <yonghong.zhu@intel.com>
* Liming Gao <liming.gao@intel.com>
* Ruiyu Ni <ruiyu.ni@intel.com>
* Star Zeng <star.zeng@intel.com>
* Eric Dong <eric.dong@intel.com>
* David Wei <david.wei@intel.com>
* Mang Guo <mang.guo@intel.com>

## Library Classes
* FmpDeviceLib - Provides firmware device specific services
  to support updates of a firmware image stored in a firmware
  device.
* CapsuleUpdatePolicyLib - Provides platform policy services
  used during a capsule update.
* FmpPayloadHeaderLib - Provides services to retrieve values
  from a capsule's FMP Payload Header.  The structure is not
  included in the library class.  Instead, services are
  provided to retrieve information from the FMP Payload Header.
  If information is added to the FMP Payload Header, then new
  services may be added to this library class to retrieve the
  new information.

## PCDs set per module
* PcdFmpDeviceSystemResetRequired - Indicates if a full
  system reset is required before a firmware update to a
  firmware devices takes effect
* PcdFmpDeviceTestKeySha256Digest - The SHA-256 hash of a
  PKCS7 test key that is used to detect if a test key is
  being used to authenticate capsules.  Test key detection
  is disabled by setting the value to {0}.
* PcdFmpDeviceProgressColor - The color of the progress bar
  during a firmware update.
* PcdFmpDeviceImageIdName - The Null-terminated Unicode
  string used to fill in the ImageIdName field of the
  EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned
  by the GetImageInfo() service of the Firmware Management
  Protocol for the firmware device.
* PcdFmpDeviceBuildTimeLowestSupportedVersion - The build
  time value used to fill in the LowestSupportedVersion field
  of the EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is
  returned by the GetImageInfo() service of the Firmware
  Management Protocol.
* PcdFmpDeviceProgressWatchdogTimeInSeconds - The time in
  seconds to arm a watchdog timer during the update of a
  firmware device.

## PCDs set per module or for entire platform
* PcdFmpDevicePkcs7CertBufferXdr - One or more PKCS7
  certificates used to verify a firmware device capsule
  update image.
* PcdFmpDeviceLockEventGuid - An event GUID that locks
  the firmware device when the event is signaled.

## GenerateCapsule tool to create UEFI Capsules
* GenerateCapsule is a new standalone tool that is used to create a UEFI
  capsule that can be processed by an FmpDxe module from the FmpDevicePkg.

```
    usage: GenerateCapsule [-h] [-o OUTPUTFILE] (-e | -d | --dump-info)
                           [--capflag {PersistAcrossReset,PopulateSystemTable,InitiateReset}]
                           [--capoemflag CAPSULEOEMFLAG] [--guid GUID]
                           [--hardware-instance HARDWAREINSTANCE]
                           [--monotonic-count MONOTONICCOUNT]
                           [--fw-version FWVERSION] [--lsv LOWESTSUPPORTEDVERSION]
                           [--pfx-file SIGNTOOLPFXFILE]
                           [--signer-private-cert OPENSSLSIGNERPRIVATECERTFILE]
                           [--other-public-cert OPENSSLOTHERPUBLICCERTFILE]
                           [--trusted-public-cert OPENSSLTRUSTEDPUBLICCERTFILE]
                           [--signing-tool-path SIGNINGTOOLPATH] [--version] [-v]
                           [-q] [--debug [0-9]]
                           InputFile
```

```
    Generate a capsule. Copyright (c) 2018, Intel Corporation. All rights
    reserved.

    positional arguments:
      InputFile             Input binary payload filename.

    optional arguments:
      -h, --help            show this help message and exit
      -o OUTPUTFILE, --output OUTPUTFILE
                            Output filename.
      -e, --encode          Encode file
      -d, --decode          Decode file
      --dump-info           Display FMP Payload Header information
      --capflag {PersistAcrossReset,PopulateSystemTable,InitiateReset}
                            Capsule flag can be PersistAcrossReset, or
                            PopulateSystemTable or InitiateReset or not set
      --capoemflag CAPSULEOEMFLAG
                            Capsule OEM Flag is an integer between 0x0000 and
                            0xffff.
      --guid GUID           The FMP/ESRT GUID in registry format. Required for
                            encode operations.
      --hardware-instance HARDWAREINSTANCE
                            The 64-bit hardware instance. The default is
                            0x0000000000000000
      --monotonic-count MONOTONICCOUNT
                            64-bit monotonic count value in header. Default is
                            0x0000000000000000.
      --fw-version FWVERSION
                            The 32-bit version of the binary payload (e.g.
                            0x11223344 or 5678).
      --lsv LOWESTSUPPORTEDVERSION
                            The 32-bit lowest supported version of the binary
                            payload (e.g. 0x11223344 or 5678).
      --pfx-file SIGNTOOLPFXFILE
                            signtool PFX certificate filename.
      --signer-private-cert OPENSSLSIGNERPRIVATECERTFILE
                            OpenSSL signer private certificate filename.
      --other-public-cert OPENSSLOTHERPUBLICCERTFILE
                            OpenSSL other public certificate filename.
      --trusted-public-cert OPENSSLTRUSTEDPUBLICCERTFILE
                            OpenSSL trusted public certificate filename.
      --signing-tool-path SIGNINGTOOLPATH
                            Path to signtool or OpenSSL tool. Optional if path to
                            tools are already in PATH.
      --version             show program's version number and exit
      -v, --verbose         Turn on verbose output with informational messages
                            printed, including capsule headers and warning
                            messages.
      -q, --quiet           Disable all messages except fatal errors.
      --debug [0-9]         Set debug level
```

## Vlv2TbltDevicePkg Platform
* Support system firmware update
* Support device firmware device update for three sample devices (Red, Green, Blue)

## Validation Plans

## Documentation Links
* [FmpDevicewPkg High Level Design Overview]()
* [FmpDevicePkg Platform Integration Guide]()
