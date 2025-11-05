
# virtual machine platform hsti driver

This driver supports three tests.

## VIRT_HSTI_BYTE0_SMM_SMRAM_LOCK

Verify the SMM memory is properly locked down.

Supported platforms:
 * Qemu Q35 (SMM_REQUIRE=TRUE builds).

## VIRT_HSTI_BYTE0_SMM_SECURE_VARS_FLASH

Verify the variable store is not writable for normal (not SMM) code.

Supported platforms:
 * Qemu Q35 (SMM_REQUIRE=TRUE builds).

## VIRT_HSTI_BYTE0_READONLY_CODE_FLASH

Verify the firmware code is not writable for the guest.

Supported platforms:
 * Qemu Q35
 * Qemu PC

# qemu flash configuration

With qemu being configured properly flash behavior should be this:

configuration                  |  OVMF_CODE.fd  |  OVMF_VARS.fd
-------------------------------|----------------|---------------
SMM_REQUIRE=TRUE, SMM mode     |  read-only     |  writable
SMM_REQUIRE=TRUE, normal mode  |  read-only (1) |  read-only (2)
SMM_REQUIRE=FALSE              |  read-only (3) |  writable

VIRT_HSTI_BYTE0_READONLY_CODE_FLASH will verify (1) + (3).
VIRT_HSTI_BYTE0_SMM_SECURE_VARS_FLASH will verify (2).

## qemu command line for SMM_REQUIRE=TRUE builds
```
qemu-system-x86-64 -M q35,smm=on,pflash0=code,pflash1=vars \
  -blockdev node-name=code,driver=file,filename=OVMF_CODE.fd,read-only=on \
  -blockdev node-name=vars,driver=file,filename=OVMF_VARS.fd \
  -global driver=cfi.pflash01,property=secure,value=on \
  [ ... more options here ... ]
```
