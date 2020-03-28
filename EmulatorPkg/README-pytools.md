# Building EmulatorPkg with EDK2 Pytools

## Summary

This README-pytools.md summarizes how to _build_ EmulatorPkg using the Pytools build system. For general documentation on EmulatorPkg, refer to the [local README](./README).

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
  - or use the flashonly feature like `stuart_build -c EmulatorPkg\PlatformBuild.py [TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG>] -a <TARGET_ARCH> --FlashOnly` to just run the emulator.

**NOTE:** configuring ACTIVE_PLATFORM and TARGET_ARCH in Conf/target.txt is __not__ required. This environment is set by PlatformBuild.py based upon the `[-a <TARGET_ARCH>]` parameter.

## Custom Build Options

**MAKE_STARTUP_NSH=TRUE** will output a _startup.nsh_ file to the location mapped as fs0. This is used in CI in combination with the --FlashOnly feature to run the emulator to the UEFI shell and then execute the contents of startup.nsh.

## Passing Build Defines

To pass build defines through stuart*build to the edk2 build, prepend `BLD_*_` to the define name and pass it on the commandline. stuart_build [currently requires values to be assigned](https://github.com/tianocore/edk2-pytool-extensions/issues/128), so add an `=1` suffix for bare defines.
For example, to enable the IP6 Network Stack, the stuart_build command-line would be:

`stuart_build -c EmulatorPkg/PlatformBuild.py BLD_*_NETWORK_IP6_ENABLE=1`

## References

- [Installing Pytools](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/using.md#installing)
- For each workspace, consider creating & using a [Python Virtual Environment](https://docs.python.org/3/library/venv.html). For example <https://microsoft.github.io/mu/CodeDevelopment/prerequisites/#workspace-virtual-environment-setup-process>
- [stuart_build commandline parser](https://github.com/tianocore/edk2-pytool-extensions/blob/56f6a7aee09995c2f22da4765e8b0a29c1cbf5de/edk2toolext/edk2_invocable.py#L109)
