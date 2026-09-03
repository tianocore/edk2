# EDK II Breaking Changes

This file is the in-tree record of breaking changes in EDK II. Each breaking change has a single entry that is added
in the pull request that introduces the change and updated in any later pull request that changes its state. The file
is organized by stable tag milestone with most recent first.

Within a given section, each breaking change is added after existing entries so they are in chronological merge order
within the section.

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

> Note: The entries below with "N/A" issues were merged during the edk2-stable202608 development period
> (after the `edk2-stable202605 tag`) and before the breaking change process took effect. They therefore have no
> associated GitHub Tracking, Deprecation, or Removal issues. Each entry links to the pull request that introduced
> the change in place of those issues. This is expected to be a one-time occurrence during the transition to the new
> breaking changes process during the `edk2-stable202608` development period.

### edk2-stable202608: Source-Level Breaking Changes

#### edk2-stable202608: Changes with Removal

##### Breaking Change: AArch64 exception handling relocated from ArmPkg to UefiCpuPkg

- **Status**: Removed
- **Tracking Issue**: N/A (merged before the breaking change process took effect)
- **Deprecation Issue**: N/A
- **Removal Issue**: N/A
- **Pull Request**: [tianocore/edk2#12340](https://github.com/tianocore/edk2/pull/12340)
- **Type**: Source-Level (Removal) - Library class restructure, API removal, and header file removal

**What changed**: AArch64 exception handling was moved from `ArmPkg` to `UefiCpuPkg`. `ArmExceptionLib` and
`DefaultExceptionHandlerLib` were merged into `CpuExceptionHandlerLib`, the `DefaultExceptionHandler()` API was renamed
to `DumpCpuContext()`, and the `ArmPkg/Include/Library/DefaultExceptionHandlerLib.h` public header was removed.

**What is removed**: The `ArmPkg` `ArmExceptionLib` and `DefaultExceptionHandlerLib` library instances, the
`DefaultExceptionHandlerLib.h` public header, and the `DefaultExceptionHandler()` API.

**Why it changed**: To provide consistent exception handling across all CPU architectures (IA32, X64, LoongArch64,
RISCV64, AARCH64) by unifying on the `UefiCpuPkg` `CpuExceptionHandlerLib`.

**What replaces it**: The `UefiCpuPkg` `CpuExceptionHandlerLib` (for example, `DxeCpuExceptionHandlerLib.inf`) and the
`DumpCpuContext()` API declared in the `UefiCpuPkg` `CpuExceptionLib.h` interface.

**How to migrate**: For out-of-tree modules that used `DefaultExceptionHandlerLib`, add `UefiCpuPkg.dec` as a package
dependency and update the platform DSC to map the exception handling library class to `CpuExceptionHandlerLib`. Replace
all `DefaultExceptionHandler()` calls with `DumpCpuContext()`. Remove all references of `ArmExceptionLib` from the DSC
file.

**Breaking conditions**: Affects AArch64 (ARM) platforms and out-of-tree modules that consumed `ArmExceptionLib`,
`DefaultExceptionHandlerLib`, or the `DefaultExceptionHandler()` API.

**Earliest removal**: Already removed in this change. The old code was removed in the same PR with no compatibility window.

##### Breaking Change: PrePiLib FfsFindSectionDataWithHook and FfsProcessFvFile gain new parameters

- **Status**: Removed
- **Tracking Issue**: N/A (merged before the breaking change process took effect)
- **Deprecation Issue**: N/A
- **Removal Issue**: N/A
- **Pull Request**: [tianocore/edk2#12672](https://github.com/tianocore/edk2/pull/12672)
- **Type**: Source-Level (Removal) - API signature change

**What changed**: `EmbeddedPkg` `PrePiLib` API signatures changed to improve FV2/FV3 HOB handling.
`FfsFindSectionDataWithHook()` gained an `AuthenticationStatus` output parameter and `FfsProcessFvFile()` gained a
`ParentVolumeHandle` parameter. In addition, `PrePi.h` dropped duplicate HOB definitions in favor of `HobLib.h`.

**What is removed**: The previous `FfsFindSectionDataWithHook()` and `FfsProcessFvFile()` function signatures.

**Why it changed**: To correctly produce FV3 HOBs (which supersede FV2 HOBs) for extracted FVs and to set the FV2 HOB
`FvName` to the parent FV name so DXE does not re-extract already-extracted FVs (a performance hit of approximately one
second on some platforms). Producing the FV3 HOB requires the authentication status from the GUIDed extraction, and
correct FV2 HOB production requires the parent FV handle.

**What replaces it**: The same functions with the new signatures.

**How to migrate**: Update callers of `FfsFindSectionDataWithHook()` to pass the new `AuthenticationStatus` argument
(pass `NULL` to retain existing behavior, or a `UINT32 *` to receive the status). Update callers of
`FfsProcessFvFile()` to pass the `ParentVolumeHandle`.

**Breaking conditions**: Affects modules that call `PrePiLib`'s `FfsFindSectionDataWithHook()` or `FfsProcessFvFile()`
(for example, `EmbeddedPkg` and `OvmfPkg` peiless startup consumers).

**Earliest removal**: Already removed in this change. The old signatures were replaced in the same PR with no
compatibility window.

##### Breaking Change: BaseRiscV64CpuTimerLib renamed and split into SEC and DXE TimerLib instances

- **Status**: Removed
- **Tracking Issue**: N/A (merged before the breaking change process took effect)
- **Deprecation Issue**: N/A
- **Removal Issue**: N/A
- **Pull Request**: [tianocore/edk2#12210](https://github.com/tianocore/edk2/pull/12210)
- **Type**: Source-Level (Removal) - Library class restructure

**What changed**: The `UefiCpuPkg` `BaseRiscV64CpuTimerLib` was renamed to `RiscV64CpuTimerLib` and split into two
instances: `RiscV64CpuTimerSecLib.inf` (`MODULE_TYPE` `SEC`, for SEC and PEI, no constructor) and
`RiscV64CpuTimerDxeLib.inf` (for `DXE_CORE`, `DXE_DRIVER`, and other DXE-phase modules, which retains the constructor).
The old `UefiCpuPkg/Library/BaseRiscV64CpuTimerLib/BaseRiscV64CpuTimerLib.inf` path was removed.

**What is removed**: The `BaseRiscV64CpuTimerLib` library instance and its INF path.

**Why it changed**: The library constructor calls `GetPerformanceCounterProperties()`, which requires the HOB list. In
the SEC and PEI phases the HOB list may not yet be available, causing a crash. Splitting the instances runs the
constructor only in the DXE instance.

**What replaces it**: The phase-specific instances `RiscV64CpuTimerSecLib.inf` (SEC and PEI) and
`RiscV64CpuTimerDxeLib.inf` (DXE).

**How to migrate**: Update platform DSC `TimerLib` mappings that referenced
`UefiCpuPkg/Library/BaseRiscV64CpuTimerLib/BaseRiscV64CpuTimerLib.inf`. Map `TimerLib` to `RiscV64CpuTimerSecLib.inf`
for SEC and PEI modules and to `RiscV64CpuTimerDxeLib.inf` for DXE modules.

**Breaking conditions**: Affects RISC-V (RISCV64) platforms that consume the RISC-V CPU timer library.

**Earliest removal**: Already removed in this change. The old INF path was removed in the same PR with no compatibility
window.

##### Breaking Change: TPM2 helper functions moved to new Tpm2HelpLib library class

- **Status**: Deprecation Active
- **Tracking Issue**: [tianocore/edk2#12797](https://github.com/tianocore/edk2/issues/12797)
- **Deprecation Issue**: [tianocore/edk2#12799](https://github.com/tianocore/edk2/issues/12799)
- **Pull Request**: [tianocore/edk2#11634](https://github.com/tianocore/edk2/pull/11634)
- **Type**: Source-Level (Non-removal) - Library class dependency addition (single expected instance)

**What changed**: The TPM2 helper functions in `Tpm2Help.c` were decoupled from `Tpm2CommandLib` into a new standalone
`SecurityPkg` library class, `Tpm2HelpLib`. This allows using the helpers without pulling in `Tpm2CommandLib` and
`Tpm2DeviceLib`, which are tied to TPM communication. In-tree modules (for example, `HashLibBaseCryptoRouter`,
`PeilessSecMeasureLib`, and `TdTcg2Dxe`) were updated to consume `Tpm2HelpLib`.

**Library class dependency case**: Single expected instance. `SecurityPkg` provides the single recommended instance at
`SecurityPkg/Library/Tpm2HelpLib/Tpm2HelpLib.inf`, so migration is a DSC library class mapping.

**Why it changed**: Some callers need the helper functions without TPM communication dependencies. Examples include
generating a HOB for pre-DXE measurements and the SEC phase, where there is no `Tpm2DeviceLib`.

**What replaces it**: The `Tpm2HelpLib` library class. The helper functions remaining in `Tpm2CommandLib`'s
`Tpm2Help.c` are now deprecated wrappers that delegate to `Tpm2HelpLib`.

**How to migrate**: Platforms building modules that now depend on `Tpm2HelpLib` must add a mapping in their DSC
(`Tpm2HelpLib|SecurityPkg/Library/Tpm2HelpLib/Tpm2HelpLib.inf`) or the build fails with an unresolved library class. For
code, add `Tpm2HelpLib` to the module INF and include the header, then update calls to the deprecated `Tpm2Help.c`
wrappers in `Tpm2CommandLib` to use the `Tpm2HelpLib` versions.

**Earliest removal**: The deprecated `Tpm2CommandLib` `Tpm2Help.c` wrappers may be removed in a future stable tag. A
removal stable tag has not been scheduled at this time.

> Note: GitHub issues were created to track edk2-platforms following migration instructions.
>
> - [Platform/ARM: Switch to Tpm2HelpLib](https://github.com/tianocore/edk2-platforms/issues/994)
> - [Platform/MinPlatformPkg: Switch to Tpm2HelpLib](https://github.com/tianocore/edk2-platforms/issues/995)
> - [Silicon/Ampere/AmpereAltraPkg: Switch to Tpm2HelpLib](https://github.com/tianocore/edk2-platforms/issues/996)

#### edk2-stable202608: Changes without Removal

##### Breaking Change: New GptLib library class dependency

- **Status**: Announced
- **Tracking Issue**: [tianocore/edk2#12808](https://github.com/tianocore/edk2/issues/12808)
- **Pull Request**: [tianocore/edk2#12745](https://github.com/tianocore/edk2/pull/12745)

**Type**: Source-Level (Non-removal) - Library class dependency addition
(single expected instance)

**What changed**: PartitionDxe, DxeTpm2MeasureBootLib and
DxeTpmMeasureBootLib gained a required dependency on the new GptLib library
class declared in MdeModulePkg. Platforms that build any of these modules
must resolve GptLib in their DSC or the build fails with an unresolved
library class.

**Why it changed**: As reported in CVE-2024-13745, DxeTpm2MeasureBootLib
could measure a GPT partition table that differs from the one parsed and
used by PartitionDxe, because the two components carried independent GPT
parsing logic. GptLib consolidates GPT parsing and validation so the
partition table measured into PCR[5] is validated by the same logic the
firmware uses.

**What replaces it**: Nothing is removed. A single canonical GptLib
instance is provided in-tree (MdeModulePkg/Library/GptLib/GptLib.inf).

**How to migrate**: Add the following mapping to the platform DSC
[LibraryClasses] section:

  GptLib|MdeModulePkg/Library/GptLib/GptLib.inf

### edk2-stable202608: Behavioral Breaking Changes

None

### edk2-stable202608: Build-System Breaking Changes

#### Breaking Change: GenFv ForceRebase now honors a per-module Xip flag

- **Status**: Removed
- **Tracking Issue**: N/A (merged before the breaking change process took effect)
- **Deprecation Issue**: N/A
- **Removal Issue**: N/A
- **Pull Request**: [tianocore/edk2#12551](https://github.com/tianocore/edk2/pull/12551)
- **Type**: Build-System - DSC/INF/DEC syntax change and BaseTools change

**What changed**: Previously the `GenFv` rebase feature (`ForceRebase=1`, that is `FvForceRebase=TRUE`) was
all-or-nothing: it rebased every eligible FFS file in the firmware volume to the FV base address, with no way to
selectively rebase only XIP modules. A new `Xip=TRUE/FALSE` keyword was added to the FDF `[Rule]` section PE32/TE
section syntax so specific module types can be tagged for XIP rebase.

```text
[Rule.Common.PEI_CORE]
  FILE PEI_CORE = $(NAMED_GUID) {
    PE32     PE32   Align=Auto   Xip=TRUE   $(INF_OUTPUT)/$(MODULE_NAME).efi
  }
```

When `ForceRebase=1`, `GenFv` now rebases only modules whose type is tagged `Xip=TRUE`; a module type with no `Xip`
keyword defaults to not being rebased. The eligible file types considered for rebase are `SECURITY_CORE`, `PEI_CORE`,
`PEIM`, `COMBINED_PEIM_DRIVER`, `DRIVER`, and `DXE_CORE`. Per-module (rather than per-type) control is available by
defining named `XIP`/`NOXIP` rules and selecting them with `RuleOverride` in the `[FV]` section:

```text
[Rule.Common.PEIM.XIP]
  FILE PEIM = $(NAMED_GUID) {
    PE32     PE32   Align=Auto   Xip=TRUE   $(INF_OUTPUT)/$(MODULE_NAME).efi
  }

[Rule.Common.PEIM.NOXIP]
  FILE PEIM = $(NAMED_GUID) {
    PE32     PE32   Align=Auto              $(INF_OUTPUT)/$(MODULE_NAME).efi
  }

[FV.PEIFV]
  INF RuleOverride=XIP    MdeModulePkg/Core/Pei/PeiMain.inf
  INF RuleOverride=NOXIP  SomePkg/SomePeim/SomePeim.inf
```

**`FvForceRebase` rebase decision table**

| FvForceRebase | FvBaseAddress | Xip in any Rule | Result |
| :---: | :---: | :---: | :--- |
| `TRUE` | any | No files have `Xip=TRUE` | Rebase ALL eligible files (legacy) |
| `TRUE` | any | At least one file has `Xip=TRUE` | Rebase ONLY files with `Xip=TRUE` |
| `FALSE` | any | any | No rebase |
| not specified | != 0 | any | Rebase ALL eligible files (legacy) |
| not specified | == 0 or not specified | any | No rebase |

**Detailed changes**: The pull request description has a detailed explanation of the change and its motivation that
readers may find useful: [tianocore/edk2#12551](https://github.com/tianocore/edk2/pull/12551).

**Why it changed**: Only XIP (execute-in-place) modules (for example, `PEI_CORE` and `PEIM`) need PE32 address fixups to
their flash location. Rebasing non-XIP modules (for example, `DXE_CORE` and `DRIVER`) is unnecessary and can cause
issues. This provides finer-grained control than the all-or-nothing `ForceRebase`.

**What replaces it**: The `Xip` keyword in FDF `[Rule]` sections used together with `ForceRebase`.

**How to migrate**: The selective rebase only activates for an FV once at least one of its `[Rule]` sections uses
`Xip=TRUE`. Platforms that relied on `ForceRebase=1` rebasing all eligible modules and do not use `Xip=TRUE` anywhere
in the FV are unaffected. Platforms adopting selective rebase must add `Xip=TRUE` to the relevant `[Rule]` sections
(for example, the `PEI_CORE` and `PEIM` rules) so those modules continue to be rebased. Module types without
`Xip=TRUE` will not be rebased once any rule in the FV uses `Xip=TRUE`. For per-module rather than per-type control,
define named `XIP`/`NOXIP` rules and select them with `RuleOverride`.

**Breaking conditions**: The new `Xip` filter only applies when `ForceRebase=1` (`FvForceRebase=TRUE`) **and** at
least one FFS file in the FV is produced by a `[Rule]` section with `Xip=TRUE`. If no rule in the FV uses
`Xip=TRUE`, `ForceRebase=1` continues to rebase all eligible files, matching pre-#12551 behavior. The default
behavior is unchanged: when `ForceRebase` is unspecified with a non-zero FV base address, all eligible files are still
rebased unconditionally and the `Xip` keyword is not consulted. When `ForceRebase=0`, nothing is rebased regardless of
`Xip`.

**Earliest removal**: Already in effect in this change. The previous "rebase all" behavior was replaced in the same PR
with no compatibility window.

> Note: [tianocore/edk2#12807](https://github.com/tianocore/edk2/pull/12807) fixed a backward-compatibility
> regression introduced by this change, in which `FvForceRebase=TRUE` stopped rebasing any files in an FV when no
> `[Rule]` section in that FV specified `Xip=TRUE`. The fix tracks the count of `Xip=TRUE` files per FV and only
> enables the selective rebase logic when that count is greater than zero. Otherwise the legacy "rebase all" behavior
> is preserved. The **Breaking conditions** and **How to migrate** guidance above reflect the behavior after this
> fix.

#### Breaking Change: Visual Studio 2015 and 2017 toolchain support removed

- **Status**: Removed
- **Tracking Issue**: N/A (merged before the breaking change process took effect)
- **Deprecation Issue**: N/A
- **Removal Issue**: N/A
- **Pull Request**: [tianocore/edk2#12683](https://github.com/tianocore/edk2/pull/12683)
- **Type**: Build-System - Tool version requirement

**What changed**: Support for the `VS2015` and `VS2017` toolchains was removed from the repository. This removed the
`VS2015` and `VS2017` toolchain definitions from `BaseTools/Conf/tools_def.template`, the associated environment setup
logic in `toolsetup.bat`, `set_vsprefix_envs.bat`, `get_vsvars.bat`, and `edksetup.bat`, the `VS2017` configuration in
the `WindowsVsToolChain` build plugin, the `VS2015`-specific `CryptoPkg` compiler flags, the `EmulatorPkg` `VS2017`
Visual Studio solution, and the unused `ShowEnvironment.bat` and `SetVisualStudio.bat` helper scripts. The oldest
supported Visual Studio toolchain is now `VS2019`.

**What is removed**: The `VS2015` and `VS2017` toolchain definitions and their supporting scripts and build options.

**Why it changed**: To reduce maintenance burden and align on newer, supported toolchains. Visual Studio 2015
mainstream support ended on October 13, 2020 and extended support ended on October 14, 2025. Visual Studio 2017 is past
its mainstream support end date.

**What replaces it**: The `VS2019`, `VS2022`, and `VS2026` toolchains.

**How to migrate**: Developers building locally with the `VS2015` or `VS2017` toolchain must move to a newer Visual
Studio version (`VS2019` is the next supported version).

**Breaking conditions**: Only affects Windows developers and CI environments that build with the `VS2015` or `VS2017`
toolchain.

**Earliest removal**: Already removed in this change. The toolchains were removed in the same PR with no compatibility
window.
