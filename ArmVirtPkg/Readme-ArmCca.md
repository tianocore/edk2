# Arm Confidential Compute Architecture (CCA)

Arm CCA is a reference software architecture and implementation that
builds on the Realm Management Extension (RME), enabling the execution
of Virtual machines (VMs), while preventing access by more privileged
software, such as hypervisor. Arm CCA allows the hypervisor to control
the VM, but removes the right for access to the code, register state or
data used by VM.

More information on the architecture is available here [1].
```

        Realm World     ||    Normal World   ||  Secure World  ||
                        ||        |          ||                ||
 EL0 x---------x        || x----x | x------x ||                ||
     | Realm   |        || |    | | |      | ||                ||
     |  VM*    |        || | VM | | |      | ||                ||
     |x-------x|        || |    | | |      | ||                ||
     ||       ||        || |    | | |  H   | ||                ||
     || Guest ||        || |    | | |      | ||                ||
 ----||  OS   ||--------||-|    |---|  o   |-||----------------||
     ||       ||        || |    | | |      | ||                ||
     |x-------x|        || |    | | |  s   | ||                ||
     |    ^    |        || |    | | |      | ||                ||
     |    |    |        || |    | | |  t   | ||                ||
     |+-------+|        || |    | | |      | ||                ||
     || REALM ||        || |    | | |      | ||                ||
     || GUEST ||        || |    | | |  O   | ||                ||
     || UEFI  ||        || |    | | |      | ||                ||
     |+-------+|        || |    | | |  S   | ||                ||
 EL1 x---------x        || x----x | |      | ||                ||
          ^             ||        | |      | ||                ||
          |             ||        | |      | ||                ||
 -------- R*------------||----------|      |-||----------------||
          S             ||          |      | ||                ||
          I             ||      x-->|      | ||                ||
          |             ||      |   |      | ||                ||
          |             ||      |   x------x ||                ||
          |             ||      |       ^    ||                ||
          v             ||     SMC      |    ||                ||
      x-------x         ||      |   x------x ||                ||
      |  RMM* |         ||      |   | HOST | ||                ||
      x-------x         ||      |   | UEFI | ||                ||
          ^             ||      |   x------x ||                ||
 EL2      |             ||      |            ||                ||
          |             ||      |            ||                ||
 =========|=====================|================================
          |                     |
          x------- *RMI* -------x

 EL3                   Root World
                       EL3 Firmware
 ===============================================================
```

Where:
 RMM - Realm Management Monitor
 RMI - Realm Management Interface
 RSI - Realm Service Interface
 SMC - Secure Monitor Call

RME introduces two added additional worlds, "Realm world" and "Root
World" in addition to the traditional Secure world and Normal world.
The Arm CCA defines a new component, Realm Management Monitor (RMM)
that runs at R-EL2. This is a standard piece of firmware, verified,
installed and loaded by the EL3 firmware (e.g., TF-A), at system boot.

The RMM provides a standard interface Realm Management Interface (RMI)
to the Normal world hypervisor to manage the VMs running in the Realm
world (also called Realms). These are exposed via SMC and are routed
through the EL3 firmware.

The RMM also provides certain services to the Realms via SMC, called
the Realm Service Interface (RSI). These include:
 - Realm Guest Configuration
 - Attestation & Measurement services
 - Managing the state of an Intermediate Physical Address (IPA aka GPA)
   page
 - Host Call service (Communication with the Normal world Hypervisor).

The Arm CCA reference software currently aligns with the RMM *v1.0-rel0*
specification, and the latest version is available here [2].

The Trusted Firmware foundation has an implementation of the RMM -
TF-RMM - available here [4].

# Related Modules
  1. Trusted Firmware RMM - TF-RMM, see [4]
  2. Trusted Firmware for A class, see [6]
  3. Linux kernel support for Arm CCA, see [7]
  4. kvmtool support for Arm CCA, see [8]


# Implementation

This version of the Realm Guest UEFI firmware is intended to be
used with the Linux Kernel stack[7] which is also based on the
RMM specification v1.0-rel0[3].

This release includes the following features:
  1. Boot a Linux Kernel in a Realm VM using the Realm Guest UEFI
    firmware
  2. Hardware description is provided using ACPI tables
  3. Support for Virtio v1.0
  4. All I/O are treated as non-secure/shared
  5. Load the Linux Kernel and RootFS from a Virtio attached disk
using the Virtio-1.0 PCIe transport.


# Overview of updates for enabling Arm CCA

The Arm CCA implementation is spread across a number of libraries
that provide required functionality during various phases of the
firmware boot.

The following libraries have been provided:

1. ArmCcaInitPeiLib - A library that implements the hook functions
in the PEI phase

2. ArmCcaLib - A library that implements common functions like
checking if RME extension is implemented and to configure the
Protection attribute for the memory regions

3. ArmCcaRsiLib - A library that implements the Realm Service
Interface functions.

A NULL implementation of the ArmCcaInitPeiLib and ArmCcaLib is also
provided for platforms that do not implement the RME extensions.

Additionally, the following DXE modules have been provided to implement
the required functionality in the DXE phase.

1. RealmApertureManagementProtocolDxe - A DXE that implements the
Realm Aperture Management Protocol, used to manage the sharing
of buffers in a Realm with the Host

2. ArmCcaIoMmuDxe - A driver which implements the EDKII_IOMMU_PROTOCOL
that provides the necessary hooks so that DMA operations can be
performed by bouncing buffers using pages shared with the Host.

# Arm CCA updates in PEI phase

For supporting Arm CCA An early hook to configure the System Memory
as Protected RAM has been added in the PrePi module.
This hook function is implemented in ArmCcaInitPeiLib. A NULL
version of the library has also been provided for implementations
that do not have the RME extensions.

The ArmVirtGetMemoryMap() implementation for the Guest firmware
has been updated to check if the device MMIO region is protected
MMIO, i.e. if it is a Realm emulated device or a Host emulated
device.
If the device is Host emulated, the Arm CCA Protection attribute bit
is set to shared, i.e. PhysicalBase = BaseAddress | (1 << (IpaWidth - 1))

```
   +=====+
   |PrePi|
   +=====+
      |
      _ModuleEntryPoint()
      ===================
              |
              DiscoverDramFromDt()
              |
              +--> ArmCcaInitPeiLib|ArmCcaConfigureSystemMemory()
                   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                             |      // configure System Memory
              ----------------      // as Protected RAM.
              |
             ...
              |
      --------
      |
      CEntryPoint()
      |
      PrePiMain()
      ===========
          |
         ...
          |
          ProcessLibraryConstructorList()
          |
          MemoryPeim()
               |
               ArmVirtMemInfoLib|ArmVirtGetMemoryMap()
               ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                   |
                   ParsePlatformDeviceFdt()
                   |    // Populate the memory map
                   ArmCcaMemRangeIsProtectedMmio()
                   |    // Check if device MMIO is protected MMIO i.e. Realm emulated
                   |    // device. If not then the device is Host emulated, so set the
                   |    // Arm CCA Protection attribute bit to shared,
                   |    // i.e. PhysicalBase = BaseAddress | (1 << (IpaWidth - 1))
                   |
               -----
               |
               ArmConfigureMmu()   // MMU is configured.
               |
         -------
         |
        ...
         |
       +===+
       |DXE|
       +===+
```

# Building the UEFI firmware

1. Set up the development environment
Follow the steps as described in
https://github.com/tianocore/edk2-platforms/blob/master/Platform/ARM/Readme.md

2. The source code for the Host and Realm Guest firmware can
be found at [10].

3. Building the Host UEFI firmware for FVP Base RevC AEM Model
Follow the instructions in
https://github.com/tianocore/edk2-platforms/blob/master/Platform/ARM/Readme.md
to "Build the firmware for Arm FVP Base AEMv8A-AEMv8A model
platform" based on your development environment configuration.

> Note: The same firmware binary can be used for both the Arm FVP
> Base AEMv8A-AEMv8A and the FVP Base RevC AEM Model.

4. Building the Realm Guest UEFI firmware for kvmtool:
To build the kvmtool guest firmware, run the following commands:

```
   $build -a AARCH64 -t GCC5 -p ArmVirtPkg/ArmVirtKvmTool.dsc -b DEBUG
   $build -a AARCH64 -t GCC5 -p ArmVirtPkg/ArmVirtKvmTool.dsc -b RELEASE
```

The Kvmtool guest firmware binaries are at the following location:
```
   $WORKSPACE/Build/ArmVirtKvmTool-AARCH64/<DEBUG|RELEASE>_GCC5/FV/KVMTOOL_EFI.fd
```

# Running the stack

To run/test the stack, you would need the following components:

1. FVP Base AEM RevC model with FEAT_RME support [5]
2. TF-A firmware for EL3 [6]
3. TF-A RMM for R-EL2 [4]
4. Linux Kernel [7]
5. kvmtool [8]
6. UEFI Firmware for Arm CCA [10].

Instructions for building the remaining firmware components and
running the model are available here [9]. Once, the host kernel
has finished booting, a Realm can be launched by invoking the
`lkvm` command as follows:

```
 $ lkvm run --realm \
   --restricted_mem \
   --measurement-algo=["sha256", "sha512"] \
   --firmware KVMTOOL_EFI.fd \
   -m 512 \
   --irqchip=gicv3-its \
   --force-pci \
   --disk <Disk image containing the Guest Kernel & RootFS>
   <normal-vm-options>

```
Where:
  - --measurement-algo (Optional) specifies the algorithm selected for
    creating the initial measurements by the RMM for this Realm (defaults to sha256)
 - GICv3 is mandatory for the Realms
 - --force-pci is required as only Virtio-v1.0 PCIe transport is
    supported.

# Links

  [1] [Arm CCA Landing page](https://www.arm.com/armcca) (See Key Resources section for various documentations)

  [2] [RMM Specification Latest](https://developer.arm.com/documentation/den0137/latest)

  [3] [RMM v1.0-rel0 specification](https://developer.arm.com/documentation/den0137/1-0rel0)

  [4] [Trusted Firmware RMM - TF-RMM](https://www.trustedfirmware.org/projects/tf-rmm/)

    GIT: https://git.trustedfirmware.org/TF-RMM/tf-rmm.git

  [5] [FVP Base RevC AEM Model](https://developer.arm.com/Tools%20and%20Software/Fixed%20Virtual%20Platforms)
      (available on x86_64 / Arm64 Linux)

  [6] [Trusted Firmware for A class](https://www.trustedfirmware.org/projects/tf-a/)

  [7] Linux kernel support for Arm CCA

    https://gitlab.arm.com/linux-arm/linux-cca
    Linux Host branch: cca-host/v10
    Linux Guest branch: Linux kernel mainline

  [8] kvmtool support for Arm CCA

    https://gitlab.arm.com/linux-arm/kvmtool-cca
    Branch: cca/v8

  [9] Instructions for Building Firmware components and running the model, see [section 4.19.2 "Building and running TF-A with RME"](https://trustedfirmware-a.readthedocs.io/en/latest/components/realm-management-extension.html#building-and-running-tf-a-with-rme)

  [10] UEFI Firmware support for Arm CCA
   ```
   Host & Guest Support:
   - Repo:
         edk2: git@github.com:tianocore/edk2.git
         edk2-platforms: git@github.com:tianocore/edk2-platforms.git
   ```

