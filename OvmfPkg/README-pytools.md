# OvmfPkg

This README-pytools.md summarizes the current state of Platform CI for OvmfPkg.
It also describes how to _build_ OvmfPkg using the Pytools build system.
For general documentation on OvmfPkg, refer to the [local README](./README).

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
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=38&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Windows%20VS2019?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32_DEBUG"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=38&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Windows%20VS2019?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32_RELEASE"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=38&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Windows%20VS2019?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32_NOOPT"/></a>
    </td>
    <td></td>
  </tr>
  <tr>
    <td>X64</td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=38&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Windows%20VS2019?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_X64_DEBUG"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=38&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Windows%20VS2019?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_X64_RELEASE"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=38&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Windows%20VS2019?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_X64_NOOPT"/></a>
    </td>
    <td></td>
  </tr>
  <tr>
    <td>IA32X64</td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=38&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Windows%20VS2019?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_DEBUG"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=38&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Windows%20VS2019?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_RELEASE"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=38&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Windows%20VS2019?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_NOOPT"/></a>
    </td>
    <td></td>
  </tr>
  <tr>
    <td>IA32X64 Full</td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=38&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Windows%20VS2019?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_FULL_DEBUG"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=38&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Windows%20VS2019?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_FULL_RELEASE"/></a>
    </td>
    <td>
      Image sizes too large.  Skipping
    </td>
    <td>NOOPT build out of space in FD</td>
  </tr>
  <tr>
    <th colspan="5" align="left">
    Ubuntu 18.04 GCC5
    </th>
  </tr>
  <tr>
    <td>IA32</td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=37&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32_DEBUG"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=37&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32_RELEASE"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=37&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32_NOOPT"/></a>
    </td>
    <td></td>
  </tr>
  <tr>
    <td>X64</td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=37&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_X64_DEBUG"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=37&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_X64_RELEASE"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=37&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_X64_NOOPT"/></a>
    </td>
    <td></td>
  </tr>
  <tr>
    <td>IA32X64</td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=37&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_DEBUG"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=37&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_RELEASE"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=37&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_NOOPT"/></a>
    </td>
    <td></td>
  </tr>
  <tr>
    <td>IA32X64 Full</td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=37&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_FULL_DEBUG"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=37&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_FULL_RELEASE"/></a>
    </td>
    <td>
      <a  href="https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=37&branchName=master">
      <img src="https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/OVMF/OVMF%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_FULL_NOOPT"/></a>
    </td>
    <td></td>
  </tr>
</table>

### Config Details

| Config       | Architectures      |Additional Flags |
| :----        | :-----             | :----           |
| IA32         | IA32               | None            |
| X64          | X64                | None            |
| IA32X64      | PEI: IA32 DXE: X64 | None            |
| IA32X64 FULL | PEI: IA32 DXE: X64 | SECURE_BOOT_ENABLE=1 SMM_REQUIRE=1 TPM_ENABLE=1 TPM_CONFIG_ENABLE=1 NETWORK_TLS_ENABLE=1 NETWORK_IP6_ENABLE=1 NETWORK_HTTP_BOOT_ENABLE=1 |

## Setup

### The Usual EDK2 Build Setup

- [Python 3.8.x - Download & Install](https://www.python.org/downloads/)
- [GIT - Download & Install](https://git-scm.com/download/)
- [GIT - Configure for EDK II](https://github.com/tianocore/tianocore.github.io/wiki/Windows-systems#github-help)
- [QEMU - Download, Install, and add to your path](https://www.qemu.org/download/)
- [EDKII Source - Download/Checkout from Github](https://github.com/tianocore/tianocore.github.io/wiki/Windows-systems#download)
  - **NOTE:** Do _not_ follow the EDK II Compile Tools and Build instructions, see below...

### Differences from EDK Classic Build Setup

- Build BaseTools using "`C:\git\edk2>python BaseTools\Edk2ToolsBuild.py [-t <ToolChainTag>]`"
  - This replaces "`edksetup Rebuild`" from the classic build system
  - For Windows `<ToolChainTag>` examples, refer to [Windows ToolChain Matrix](https://github.com/tianocore/tianocore.github.io/wiki/Windows-systems-ToolChain-Matrix), defaults to `VS2017` if not specified
- **No Action:** Submodule initialization and manual installation/setup of NASM and iASL is **not** required, it is handled by the Pytools build system

### Install & Configure Pytools for OvmfPkg

- Install Pytools
  - `pip install --upgrade -r pip-requirements.txt`
- Initialize & Update Submodules
  - `stuart_setup -c OvmfPkg\PlatformBuild.py`
- Initialize & Update Dependencies (e.g. iASL & NASM)
  - `stuart_update -c OvmfPkg\PlatformBuild.py`

## Building

OVMF has [3 versions](https://github.com/tianocore/tianocore.github.io/wiki/How-to-build-OVMF#choosing-which-version-of-ovmf-to-build). To build them using Pytools:

First set the `TOOL_CHAIN_TAG` via environment variable, Conf/target.txt, or pass it on the command-lines below using "`TOOL_CHAIN_TAG=<value>`" syntax.

| Platform           | Commandline                                                                                                                                                                        |
| ------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| OvmfPkgIa32X64.dsc | `stuart_build -c OvmfPkg\PlatformBuild.py [TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG>]`<BR>**OR**<BR>`stuart_build -c OvmfPkg\PlatformBuild.py -a IA32,X64 [TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG>]` |
| OvmfPkgIa32.dsc    | `stuart_build -c OvmfPkg\PlatformBuild.py -a IA32 [TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG>]`                                                                                               |
| OvmfPkgX64.dsc     | `stuart_build -c OvmfPkg\PlatformBuild.py -a X64 [TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG>]`                                                                                                |

**NOTE:** configuring ACTIVE*PLATFORM and TARGET_ARCH in Conf/target.txt is *not* required. This environment is set by PlatformBuild.py based upon the "`[-a <TARGET_ARCH>]`" parameter.

### Custom Build Options

**MAKE_STARTUP_NSH=TRUE** will output a _startup.nsh_ file to the location mapped as fs0. This is used in CI in combination with the --FlashOnly feature to run QEMU to the UEFI shell and then execute the contents of startup.nsh.

**QEMU_HEADLESS=TRUE** Since CI servers run headless QEMU must be told to run with no display otherwise an error occurs. Locally you don't need to set this.

### Passing Build Defines

To pass build defines through stuart*build, prepend `BLD_*_`to the define name and pass it on the commandline. stuart_build currently requires values to be assigned, so add an `=1` suffix for bare defines.
For example, to enable the Intel E1000 NIC, instead of the traditional "-D E1000_ENABLE", the stuart_build command-line would be:

`stuart_build -c OvmfPkg/PlatformBuild.py BLD_*_E1000_ENABLE=1`

## Running QEMU Emulator

QEMU can be automatically launched using stuart_build.  This makes path management and quick verification easy.
QEMU must be added to your path.  On Windows this is a manual process and not part of the QEMU installer.

1. To run as part of the build but after building add the `--FlashRom` parameter.
2. To run after the build process standalone use your build command mentioned above plus `--FlashOnly`.

**NOTE:** Logging the execution output will be in the normal stuart log as well as to your console.

## References

- [Installing Pytools](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/using.md#installing)
- For each workspace, consider creating & using a [Python Virtual Environment](https://docs.python.org/3/library/venv.html). For example <https://microsoft.github.io/mu/CodeDevelopment/prerequisites/#workspace-virtual-environment-setup-process>
- [stuart_build commandline parser](https://github.com/tianocore/edk2-pytool-extensions/blob/56f6a7aee09995c2f22da4765e8b0a29c1cbf5de/edk2toolext/edk2_invocable.py#L109)
