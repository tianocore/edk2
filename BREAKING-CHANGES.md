# EDK II Breaking Changes

This file is the in-tree record of breaking changes in EDK II. Each breaking change has a single entry that is added
in the pull request that introduces the change and updated in any later pull request that changes its state. The file
is organized by stable tag milestone with most recent first.

For the full process, including the breaking change taxonomy, deprecation timeline, required entry content, and GitHub
issue requirements, see the [Breaking Change and Release Process for EDK II](https://raw.githubusercontent.com/tianocore/tianocore-wiki.github.io/refs/heads/main/rfc/text/0003-edk2-breaking-change-and-release-process.md)
RFC.

## edk2-stable202702

- Milestone: [edk2-stable202702](https://github.com/tianocore/edk2/milestone/7)

### edk2-stable202702: Source-Level Breaking Changes

#### edk2-stable202702: Changes with Removal

None

#### edk2-stable202702: Changes without Removal

None

### edk2-stable202702: Behavioral Breaking Changes

None

### edk2-stable202702: Build-System Breaking Changes

None

---

## edk2-stable202611

- Milestone: [edk2-stable202611](https://github.com/tianocore/edk2/milestone/6)

### edk2-stable202611: Source-Level Breaking Changes

#### edk2-stable202611: Changes with Removal

None

#### edk2-stable202611: Changes without Removal

None

### edk2-stable202611: Behavioral Breaking Changes

None

### edk2-stable202611: Build-System Breaking Changes

None

---

## edk2-stable202608

- Milestone: [edk2-stable202608](https://github.com/tianocore/edk2/milestone/5)

### edk2-stable202608: Source-Level Breaking Changes

#### edk2-stable202608: Changes with Removal

None

#### edk2-stable202608: Changes without Removal

None

### edk2-stable202608: Behavioral Breaking Changes

#### Breaking Change: Separate IPMI KCS command/status address

**Status**: Deprecation Active
**Tracking Issue**: [tianocore/edk2#12805](https://github.com/tianocore/edk2/issues/12805)
**Type**: Behavioral - Changed default behavior

**What changed**: The IPMI KCS command/status register address is no longer
derived from `PcdIpmiKcsIoBaseAddress + 1`. The new
`PcdIpmiKcsIoCommandAddress` specifies the address independently and defaults
to `0xCA3`. The data register PCD continues to default to `0xCA2`. The IPMI
ACPI device also moves from the platform-specific `\_SB.PC00.LPC0.IPMC`
namespace to `\_SB.IPMC`. `BmcAcpi` now consumes both KCS address PCDs as
FixedAtBuild, consistent with the other IPMI KCS modules in `ManageabilityPkg`.

**Why it changed**: Some KCS interfaces use non-contiguous data and
command/status registers, such as `0x62` and `0x66`. Describing both addresses
explicitly also keeps the ACPI resource declaration consistent with the
addresses used by the IPMI transport drivers.

**What replaces it**: Configure `PcdIpmiKcsIoBaseAddress` for the data register
and `PcdIpmiKcsIoCommandAddress` for the command/status register.

**How to migrate**: Platforms that override `PcdIpmiKcsIoBaseAddress` must also
set `PcdIpmiKcsIoCommandAddress` in their DSC. For example:

```text
gEfiMdePkgTokenSpaceGuid.PcdIpmiKcsIoBaseAddress|0x62
gEfiMdePkgTokenSpaceGuid.PcdIpmiKcsIoCommandAddress|0x66
```

References in platform ASL or tooling to `\_SB.PC00.LPC0.IPMC` must be updated
to `\_SB.IPMC`. Platforms that build `BmcAcpi` with these PCDs as
PatchableInModule must resolve them as FixedAtBuild instead.

**Breaking conditions**: The address change affects platforms that override
`PcdIpmiKcsIoBaseAddress` and do not configure the new command/status address
PCD. The namespace change affects platforms with absolute references to the
former `\_SB.PC00.LPC0.IPMC` path. The PCD access change affects platforms
that use `BmcAcpi` without the standard IPMI stack and resolve either KCS
address PCD as PatchableInModule.

**Companion PR**: N/A

### edk2-stable202608: Build-System Breaking Changes

None
