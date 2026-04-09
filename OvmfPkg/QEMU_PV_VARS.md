# QEMU uefi variable store

Starting with version 10.0 (released April 2025) qemu can provide an
uefi variable store to the guest.

## Motivation

Main difference to the traditional approach to provide pflash storage
for the variables is that qemu will also handle access control and
signature verification for authenticated variable updates.  Moving
that functionality from priviledged guest mode (SMM on x64) to the
host makes it much easier to support secure boot.  The requirement to
have some priviledged guest mode for VMs goes away.

## Build the firmware

Support for the qemu uefi variable store is a compile time option.  It
is disabled by default and can be enabled using the QEMU_PV_VARS
option.  It makes sense to also enable secure boot support, i.e. build
with '-D QEMU_PV_VARS=TRUE -D SECURE_BOOT_ENABLE=TRUE'.

Supported platforms:

 * OvmfPkg/OvmfPkgX64.dsc
 * ArmVirtPkg/ArmVirtQemu.dsc
 * ArmVirtPkg/ArmVirtQemuKernel.dsc
 * OvmfPkg/RiscVVirt/RiscVVirtQemu.dsc

## Using the qemu uefi variable store

The variable store is implemented as qemu device and it comes in two
variants.  The first is `uefi-vars-x64`, for the x64 platform, using
the 'etc/hardware-info' fw_cfg file for device discovery.  The second
is `uefi-vars-sysbus`, used by all other platforms, using device tree
for device discovery.

Simplest way to use it is this:

```
qemu-system-x86_64 \
    -bios OVMF.qemuvars.fd \
    -device uefi-vars-x64 \
    $otherargs
```

This runs the uefi variable store without persistence, i.e. the
variables will be lost on VM poweroff (i.e. qemu exit).  The variables
will survive reboots though.

QEMU can save the variable store in json format for persistence.  This
is enabled by passing a filename for the varstore to qemu:

```
touch /path/to/varstore.json
qemu-system-x86_64 \
    -bios OVMF.qemuvars.fd \
    -device uefi-vars-x64,jsonfile=/path/to/varstore.json \
    $otherargs
```

## Enroll secure boot variables

There are two projects which can handle the json format variable
store: https://gitlab.com/kraxel/virt-firmware and
https://github.com/awslabs/python-uefivars.

virt-firmware can be installed via `pip install virt-firmware`.  Your
linux distro might also have packages for you.  On Fedora, RHEL and
Centos Stream `dnf install virt-firmware` works.

Creating a variable store with secure boot variables enrolled works
this way:

```
virt-fw-vars \
    --secure-boot \
    --enroll-redhat \
    --set-dbx /usr/share/edk2/ovmf/DBXUpdate-${date}.x64.bin \
    --output-json /path/to/varstore.json
```

This will enroll the standard set of microsoft certificates (both 2011
and 2023).  There are a number of config options to change default
behavior, check out `virt-fw-vars --help`.

If your linux distro does not ship the dbx revocation database you can
get it from https://github.com/microsoft/secureboot_objects/
(subdirectory `PostSignedObjects/DBX`) instead.
