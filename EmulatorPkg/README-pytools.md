# EmulatorPkg

This README-pytools.md summarizes the current state of Platform CI for EmulatorPkg.
It also describes how to _build_ EmulatorPkg using the Pytools build system.
For general documentation on EmulatorPkg, refer to the [local README](./README).

## Platform CI Current Status

<table>
  <tr>
    <th>Config</th>
    <th colspan="3">Build & Run</th>
    <th>Notes</th>
  </tr>
  <tr>
    <th></th>
    <th>DEBUG</th>
    <th>RELEASE</th>
    <th>NOOPT</th>
    <th></th>
  </tr>
  <tr>
    <th colspan="5" align="left">
    Windows VS2019
    </th>
  </tr>
  <tr>
    <td>IA32</td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=40&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Windows%20VS2019?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_DEBUG"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=40&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Windows%20VS2019?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_RELEASE"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=40&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Windows%20VS2019?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_NOOPT"/></a>
    </td>
    <td></td>
  </tr>
  <tr>
    <td>X64</td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=39&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_DEBUG"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=39&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_RELEASE"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=39&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_NOOPT"/></a>
    </td>
    <td></td>
  </tr>
  <tr>
    <th colspan="5" align="left">
    Ubuntu 18.04 GCC5
    </th>
  </tr>
  <tr>
    <td>IA32</td>
    <td>
      NOT RUNNING
    </td>
    <td>
      NOT RUNNING
    </td>
    <td>
      NOT RUNNING
    </td>
    <td>IA32 fails to build.</td>
  </tr>
  <tr>
    <td>X64</td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=39&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_DEBUG"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=39&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_RELEASE"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=39&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_NOOPT"/></a>
    </td>
    <td>X64 fails to run to shell</td>
  </tr>
</table>

### Config Details

| Config       | Architectures      |Additional Flags |
| :----        | :-----             | :----           |
| IA32         | IA32               | None            |
| X64          | X64                | None            |

## Setup

### The Usual EDK2 Build Setup

- [Download & Install Python 3.x](https://www.python.org/downloads/)
- [Download & Install git](https://git-scm.com/download/)
- [Configure Git for EDK II](https://github.com/tianocore/tianocore.github.io/wiki/Windows-systems#github-help)
- [Download/Checkout the EDK II source tree from Github](https://github.com/tianocore/tianocore.github.io/wiki/Windows-systems#download)
  - **NOTE:** Do _not_ follow the EDK II Compile Tools and Build instructions, see below...

### Differences from EDK Classic Build Setup

- Build BaseTools using "`C:\git\edk2>python BaseTools\Edk2ToolsBuild.py [-t <ToolChainTag>]`"
  - This replaces "`edksetup Rebuild`" from the classic build system
  - For Windows `<ToolChainTag>` examples, refer to [Windows ToolChain Matrix](https://github.com/tianocore/tianocore.github.io/wiki/Windows-systems-ToolChain-Matrix), defaults to `VS2017` if not specified
- **No Action:** Submodule initialization and manual installation/setup of NASM and iASL is **not** required, it is handled by the Pytools build system

### Install & Configure Pytools for EmulatorPkg

- Install Pytools
  - `pip install --upgrade -r pip-requirements.txt`
- Initialize & Update Submodules
  - `stuart_setup -c EmulatorPkg\PlatformBuild.py`
- Initialize & Update Dependencies (e.g. iASL & NASM)
  - `stuart_update -c EmulatorPkg\PlatformBuild.py`
- Compile (IA32 or X64 supported)
  - `stuart_build -c EmulatorPkg\PlatformBuild.py [TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG>] -a <TARGET_ARCH>`
- Running Emulator
  - You can add `--FlashRom` to the end of your build command and the emulator will run after the build is complete.
  - or use the FlashOnly feature like `stuart_build -c EmulatorPkg\PlatformBuild.py [TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG>] -a <TARGET_ARCH> --FlashOnly` to just run the emulator.

**NOTE:** configuring ACTIVE_PLATFORM and TARGET_ARCH in Conf/target.txt is __not__ required. This environment is set by PlatformBuild.py based upon the `[-a <TARGET_ARCH>]` parameter.

## Custom Build Options

**MAKE_STARTUP_NSH=TRUE** will output a _startup.nsh_ file to the location mapped as fs0. This is used in CI in combination with the --FlashOnly feature to run the emulator to the UEFI shell and then execute the contents of startup.nsh.

## Passing Build Defines

To pass build defines through stuart*build to the edk2 build, prepend `BLD_*_` to the define name and pass it on the command-line. stuart_build currently requires values to be assigned, so add an `=1` suffix for bare defines.
For example, to enable the IP6 Network Stack, the stuart_build command-line would be:

`stuart_build -c EmulatorPkg/PlatformBuild.py BLD_*_NETWORK_IP6_ENABLE=1`

## References

- [Installing Pytools](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/using.md#installing)
- For each workspace, consider creating & using a [Python Virtual Environment](https://docs.python.org/3/library/venv.html). For example <https://microsoft.github.io/mu/CodeDevelopment/prerequisites/#workspace-virtual-environment-setup-process>
- [stuart_build command-line parser](https://github.com/tianocore/edk2-pytool-extensions/blob/56f6a7aee09995c2f22da4765e8b0a29c1cbf5de/edk2toolext/edk2_invocable.py#L109)
