TDVF Overview
-------------

**Intel Trust Domain Extension (TDX)** is Intel Architecture extension
to provide trusted, isolated VM execution by removing CSP software
(hypervisor etc) from the TCB. **TDX Virtual Firmware (TDVF)** is an
EDK II based project to enable UEFI support for TDX based Virtual
Machines. It provides the capability to launch a TD.

The **Intel TDX Virtual Firmware Design Guide** is at
https://www.intel.com/content/dam/develop/external/us/en/documents/tdx-virtual-firmware-design-guide-rev-1.01.pdf.

More information can be found at:
https://www.intel.com/content/www/us/en/developer/articles/technical/intel-trust-domain-extensions.html


Configurations and Features
----------------------------

There are 2 configurations for TDVF.

**Config-A:**
 - Merge the *basic* TDVF feature to existing **OvmfPkgX64.dsc**. (Align
   with existing SEV)
 - Threat model: VMM is **NOT** out of TCB. (We don't make things worse)
 - The OvmfPkgX64.dsc includes SEV/TDX/normal OVMF basic boot capability.
   The final binary can run on SEV/TDX/normal OVMF.
 - No changes to existing OvmfPkgX64 image layout.
 - No need to remove features if they exist today.
 - PEI phase is **NOT** skipped in either TD or Non-TD.
 - RTMR based measurement (CC_MEASUREMENT) is supported as an optional requirement.
 - External inputs from Host VMM are measured, such as TdHob, CFV.
 - Other external inputs are measured, such as FW_CFG data, os loader,
   initrd, etc.

**Config-B:**
 - Add a standalone **IntelTdxX64.dsc** to a TDX specific directory
   (**OvmfPkg/IntelTdx**) for a *full* feature TDVF.(Align with existing SEV)
 - Threat model: VMM is out of TCB. (We need necessary change to prevent
   attack from VMM)
 - IntelTdxX64.dsc includes TDX/normal OVMF basic boot capability. The final
   binary can run on TDX/normal OVMF.
 - It might eventually merge with AmdSev.dsc, but NOT at this point of
   time. And we don't know when it will happen. We need sync with AMD in
   the community after both of us think the solutions are mature to merge.
 - RTMR based measurement (CC_MEASUREMENT) is supported as a mandatory requirement.
 - External inputs from Host VMM are measured, such as TdHob, CFV.
 - Other external inputs are measured, such as FW_CFG data, os loader,
   initrd, etc.
 - PEI phase is skipped to remove unnecessary attack surface.
 - DXE FV is split into 2 FVs (DXEFV & NCCFV) to remove the unnecessary attack
   surface in a TD guest..
   - When launching a TD guest, only drivers in DXEFV are loaded.
   - When launching a Non-TD guest, dirvers in both DXEFV and NCCFV are
     loaded.

Build
------
- Build the TDVF (Config-A) target:
```
cd /path/to/edk2
source edksetup.sh

## CC_MEASUREMENT disabled
build -p OvmfPkg/OvmfPkgX64.dsc -a X64 -t GCC5 -D CC_MEASUREMENT_ENABLE=FALSE -b RELEASE

## CC_MEASUREMENT enabled
build -p OvmfPkg/OvmfPkgX64.dsc -a X64 -t GCC5 -D CC_MEASUREMENT_ENABLE=TRUE -b RELEASE
```

- Build the TDVF (Config-B) target:
```
cd /path/to/edk2
source edksetup.sh
build -p OvmfPkg/IntelTdx/IntelTdxX64.dsc -a X64 -t GCC5 -b RELEASE
```

Usage
-----

Assuming TDX-QEMU/TDX-KVM are already built, one can start a TD virtual
machine as [launching-a-tdx-vm](https://gitlab.com/qemu/qemu/-/blob/master/docs/system/i386/tdx.rst):

```
qemu_system_x86 \
   -accel kvm \
   -cpu host \
   -object tdx-guest,id=tdx0 \
   -machine ...,confidential-guest-support=tdx0 \
   -bios /path/to/OVMF.fd
```

Note:
Avoid using the '-pflash' QEMU parameter with TDX configurations, as TDX lacks support for read-only memory slots.

Linux Software Stack
--------------------
KVM with TDX support:
  - Linux kernel version >= 6.16
  - Source code: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git

QEMU with TDX support:
  - QEMU version >= 10.1
  - Source code: https://gitlab.com/qemu-project/qemu.git
