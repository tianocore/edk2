# Edk2 Continuous Integration

This file focuses on information for those working with the `.pytools` directory
directly or interested in lower-level details about how CI works.

If you just want to get started building code, visit
[Build Instructions](https://github.com/tianocore/tianocore.github.io/wiki/Build-Instruction)
on the TianoCore wiki.

## Basic Status

| Package              | Windows VS2019 (IA32/X64)| Ubuntu GCC (IA32/X64/ARM/AARCH64) | Known Issues |
| :----                | :-----                   | :----                             | :---         |
| ArmPkg               |                    | :heavy_check_mark: |
| ArmPlatformPkg       |                    | :heavy_check_mark: |
| ArmVirtPkg           | SEE PACKAGE README | SEE PACKAGE README |
| CryptoPkg            | :heavy_check_mark: | :heavy_check_mark: | Spell checking in audit mode
| DynamicTablesPkg     | :heavy_check_mark: | :heavy_check_mark: |
| EmbeddedPkg          |
| EmulatorPkg          | SEE PACKAGE README | SEE PACKAGE README | Spell checking in audit mode
| FatPkg               | :heavy_check_mark: | :heavy_check_mark: |
| FmpDevicePkg         | :heavy_check_mark: | :heavy_check_mark: |
| IntelFsp2Pkg         |
| IntelFsp2WrapperPkg  |
| MdeModulePkg         | :heavy_check_mark: | :heavy_check_mark: | DxeIpl dependency on ArmPkg, Depends on StandaloneMmPkg, Spell checking in audit mode
| MdePkg               | :heavy_check_mark: | :heavy_check_mark: | Spell checking in audit mode
| NetworkPkg           | :heavy_check_mark: | :heavy_check_mark: | Spell checking in audit mode
| OvmfPkg              | SEE PACKAGE README | SEE PACKAGE README | Spell checking in audit mode
| PcAtChipsetPkg       | :heavy_check_mark: | :heavy_check_mark: |
| SecurityPkg          | :heavy_check_mark: | :heavy_check_mark: | Spell checking in audit mode
| ShellPkg             | :heavy_check_mark: | :heavy_check_mark: | Spell checking in audit mode, 3 modules are not being built by DSC
| SignedCapsulePkg     |
| SourceLevelDebugPkg  |
| StandaloneMmPkg      | :heavy_check_mark: | :heavy_check_mark: |
| UefiCpuPkg           | :heavy_check_mark: | :heavy_check_mark: | Spell checking in audit mode, 2 binary modules not being built by DSC
| UefiPayloadPkg       |
| UnitTestFrameworkPkg | :heavy_check_mark: | :heavy_check_mark: |

For more detailed status look at the test results of the latest CI run on the
repo readme.

## Background

This Continuous integration and testing infrastructure leverages the TianoCore EDKII Tools PIP modules:
[library](https://pypi.org/project/edk2-pytool-library/) and
[extensions](https://pypi.org/project/edk2-pytool-extensions/) (with repos
located [here](https://github.com/tianocore/edk2-pytool-library) and
[here](https://github.com/tianocore/edk2-pytool-extensions)).

The primary execution flows can be found in the
`.azurepipelines/Windows-VS2019.yml` and `.azurepipelines/Ubuntu-GCC5.yml`
files. These YAML files are consumed by the Azure Dev Ops Build Pipeline and
dictate what server resources should be used, how they should be configured, and
what processes should be run on them. An overview of this schema can be found
[here](https://docs.microsoft.com/en-us/azure/devops/pipelines/yaml-schema?view=azure-devops&tabs=schema).

Inspection of these files reveals the EDKII Tools commands that make up the
primary processes for the CI build: 'stuart_setup', 'stuart_update', and
'stuart_ci_build'. These commands come from the EDKII Tools PIP modules and are
configured as described below. More documentation on the tools can be
found [here](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/using.md)
and [here](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/features/feature_invocables.md).

## Configuration

Configuration of the CI process consists of (in order of precedence):

* command-line arguments passed in via the Pipeline YAML
* a per-package configuration file (e.g. `<package-name>.ci.yaml`) that is
  detected by the CI system in EDKII Tools.
* a global configuration Python module (e.g. `CISetting.py`) passed in via the
  command-line

The global configuration file is described in
[this readme](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/usability/using_settings_manager.md)
from the EDKII Tools documentation. This configuration is written as a Python
module so that decisions can be made dynamically based on command line
parameters and codebase state.

The per-package configuration file can override most settings in the global
configuration file, but is not dynamic. This file can be used to skip or
customize tests that may be incompatible with a specific package.  Each test generally requires
per package configuration which comes from this file.

## Running CI locally

The EDKII Tools environment (and by extension the ci) is designed to support
easily and consistently running locally and in a cloud ci environment.  To do
that a few steps should be followed.  Details of EDKII Tools can be found in the
[docs folder here](https://github.com/tianocore/edk2-pytool-extensions/tree/master/docs)

### Running CI

Quick notes:

* By default all CI plugins are opted in.
  * Setting the plugin to `skip` as an argument will skip running the plugin.
    Examples:
    * `CompilerPlugin=skip` skip the build test
    * `GuidCheck=skip` skip the Guid check
    * `SpellCheck=skip` skip the spell checker
    * etc.
* Detailed reports and logs per package are captured in the `Build` directory.

## Current PyTool Test Capabilities

All CI tests are instances of EDKII Tools plugins. Documentation on the plugin
system can be found [here](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/usability/using_plugin_manager.md)
and [here](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/features/feature_plugin_manager.md).
Upon invocation, each plugin will be passed the path to the current package
under test and a dictionary containing its targeted configuration, as assembled
from the command line, per-package configuration, and global configuration.

Note: CI plugins are considered unique from build plugins and helper plugins,
even though some CI plugins may execute steps of a build.

In the example, these plugins live alongside the code under test (in the
`.pytool/Plugin` directory), but may be moved to the 'edk2-test' repo if that
location makes more sense for the community.

### Module Inclusion Test - DscCompleteCheck

This scans all INF files from a package and confirms they are
listed in the package level DSC file. The test considers it an error if any INF
does not appear in the `Components` section of the package-level DSC (indicating
that it would not be built if the package were built). This is critical because
much of the CI infrastructure assumes that all modules will be listed in the DSC
and compiled.

This test will ignore INFs in the following cases:

1. When `MODULE_TYPE` = `HOST_APPLICATION`
2. When a Library instance **only** supports the `HOST_APPLICATION` environment

### Host Module Inclusion Test - HostUnitTestDscCompleteCheck

This test scans all INF files from a package for those related to host
based unit tests and confirms they are listed in the unit test DSC file for the package.
The test considers it an error if any INF meeting the requirements does not appear
in the `Components` section of the unit test DSC. This is critical because
much of the CI infrastructure assumes that  modules will be listed in the DSC
and compiled.

This test will only require INFs in the following cases:

1. When `MODULE_TYPE` = `HOST_APPLICATION`
2. When a Library instance explicitly supports the `HOST_APPLICATION` environment

### Code Compilation Test - CompilerPlugin

Once the Module Inclusion Test has verified that all modules would be built if
all package-level DSCs were built, the Code Compilation Test simply runs through
and builds every package-level DSC on every toolchain and for every architecture
that is supported. Any module that fails to build is considered an error.

### Host Unit Test Compilation and Run Test - HostUnitTestCompilerPlugin

A test that compiles the dsc for host based unit test apps.
On Windows this will also enable a build plugin to execute that will run the unit tests and verify the results.

These tools will be invoked on any CI
pass that includes the NOOPT target. In order for these tools to do their job,
the package and tests must be configured in a particular way...

#### Including Host-Based Tests in the Package YAML

For example, looking at the `MdeModulePkg.ci.yaml` config file, there are two
config options that control HostBased test behavior:

```json
    ## options defined .pytool/Plugin/HostUnitTestCompilerPlugin
    "HostUnitTestCompilerPlugin": {
        "DscPath": "Test/MdeModulePkgHostTest.dsc"
    },
```

This option tell the test builder to run. The test builder needs to know which
modules in this package are host-based tests, so that DSC path is provided.

#### Configuring the HostBased DSC

The HostBased DSC for `MdeModulePkg` is located at
`MdeModulePkg/Test/MdeModulePkgHostTest.dsc`.

To add automated host-based unit test building to a new package, create a
similar DSC. The new DSC should make sure to have the `NOOPT` BUILD_TARGET
and should include the line:

```
!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc
```

All of the modules that are included in the `Components` section of this
DSC should be of type HOST_APPLICATION.

### GUID Uniqueness Test - GuidCheck

This test works on the collection of all packages rather than an individual
package. It looks at all FILE_GUIDs and GUIDs declared in DEC files and ensures
that they are unique for the codebase. This prevents, for example, accidental
duplication of GUIDs when using an existing INF as a template for a new module.

### Cross-Package Dependency Test - DependencyCheck

This test compares the list of all packages used in INFs files for a given
package against a list of "allowed dependencies" in plugin configuration for
that package. Any module that depends on a disallowed package will cause a test
failure.

### Library Declaration Test - LibraryClassCheck

This test scans at all library header files found in the `Library` folders in
all of the package's declared include directories and ensures that all files
have a matching LibraryClass declaration in the DEC file for the package. Any
missing declarations will cause a failure.

### Invalid Character Test - CharEncodingCheck

This test scans all files in a package to make sure that there are no invalid
Unicode characters that may cause build errors in some character
sets/localizations.

### Spell Checking - cspell

This test runs a spell checker on all files within the package.  This is done
using the NodeJs cspell tool.  For details check `.pytool/Plugin/SpellCheck`.
For this plugin to run during ci you must install nodejs and cspell and have
both available to the command line when running your CI.

Install

* Install nodejs from https://nodejs.org/en/
* Install cspell
  1. Open cmd prompt with access to node and npm
  2. Run `npm install -g cspell`

  More cspell info: https://github.com/streetsidesoftware/cspell

### License Checking - LicenseCheck

Scans all new added files in a package to make sure code is contributed under
BSD-2-Clause-Patent.

### Ecc tool - EccCheck

Run the Ecc tool on the package. The Ecc tool is available in the BaseTools
package. It checks that the code complies to the EDKII coding standard.

### Coding Standard Compliance - UncrustifyCheck

Runs the Uncrustify application to check for coding standard compliance issues.

## PyTool Scopes

Scopes are how the PyTool ext_dep, path_env, and plugins are activated.  Meaning
that if an invocable process has a scope active then those ext_dep and path_env
will be active. To allow easy integration of PyTools capabilities there are a
few standard scopes.

| Scope      | Invocable                                            | Description    |
| :----      | :-----                                               | :----          |
| global     | edk2_invocable++ - should be base_abstract_invocable | Running an invocables |
| global-win | edk2_invocable++                                     | Running on Microsoft Windows |
| global-nix | edk2_invocable++                                     | Running on Linux based OS    |
| edk2-build |                                                      | This indicates that an invocable is building EDK2 based UEFI code |
| cibuild    | set in .pytool/CISettings.py                         | Suggested target for edk2 continuous integration builds.  Tools used for CiBuilds can use this scope.  Example: asl compiler |
| host-based-test | set in .pytool/CISettings.py                    | Turns on the host based tests and plugin |
| host-test-win | set in .pytool/CISettings.py                      | Enables the host based test runner for Windows |

## Future investments

* PatchCheck tests as plugins
* MacOS/xcode support
* Clang/LLVM support
* Visual Studio AARCH64 and ARM support
* BaseTools C tools CI/PR and binary release process
* BaseTools Python tools CI/PR process
* Extensible private/closed source platform reporting
* UEFI SCTs
* Other automation
