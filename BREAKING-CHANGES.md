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

##### Breaking Change: New GptLib library class dependency

**Status**: Announced

**Tracking Issue**: tianocore/edk2#12808

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

None
