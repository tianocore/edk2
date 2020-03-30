# Building ArmVirtQemu with EDK2 Pytools on Ubuntu with GCC5

## Summary

This README-pytools.md summarizes how to _build_ ArmVirtPkg using the Pytools build system.

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

### Building with Pytools for ArmVirtPkg

- Install Pytools
  - `pip install --upgrade -r pip-requirements.txt`
- Initialize & Update Submodules
  - `stuart_setup -c ArmVirt\PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>`
- Initialize & Update Dependencies (e.g. iASL, NASM & GCC Arm/Aarch64 Compilers)
  - `stuart_update -c ArmVirtPkg\PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>`
- Compile (AARCH64 supported / ARM support coming soon)
  - `stuart_build -c ArmVirtPkg\PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>`
- Running Emulator
  - You can add `--FlashRom` to the end of your build command and the emulator will run after the build is complete.
  - or use the flashonly feature like `stuart_build -c ArmVirtPkg\PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH> --FlashOnly` to just run the emulator.

### Notes

1. Including the expected build architecture and toolchain to the _stuart_update_ command is critical. This is because there are extra scopes and tools that will be resolved during the update step that need to match your build step.
2. Configuring _ACTIVE_PLATFORM_ and _TARGET_ARCH_ in Conf/target.txt is _not_ required. This environment is set by PlatformBuild.py based upon the "`[-a <TARGET_ARCH>]`" parameter.
3. QEMU must be on your path.  On Windows this is a manual process and not part of the QEMU installer.

**NOTE:** Logging the execution output will be in the normal stuart log as well as to your console.

## Custom Build Options

**MAKE_STARTUP_NSH=TRUE** will output a _startup.nsh_ file to the location mapped as fs0. This is used in CI in combination with the --FlashOnly feature to run QEMU to the UEFI shell and then execute the contents of startup.nsh.

**QEMU_HEADLESS=TRUE** Since CI servers run headless QEMU must be told to run with no display otherwise an error occurs. Locally you don't need to set this.

## Passing Build Defines

To pass build defines through _stuart_build_, prepend `BLD_*_`to the define name and pass it on the commandline. _stuart_build_ [currently requires values to be assigned](https://github.com/tianocore/edk2-pytool-extensions/issues/128), so add an`=1` suffix for bare defines.
For example, to enable the Intel E1000 NIC, instead of the traditional "-D E1000_ENABLE", the stuart_build command-line would be:

`stuart_build -c ArmVirtPkg/PlatformBuild.py BLD_*_E1000_ENABLE=1`

## References

- [Installing Pytools](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/using.md#installing)
- For each workspace, consider creating & using a [Python Virtual Environment](https://docs.python.org/3/library/venv.html). For example <https://microsoft.github.io/mu/CodeDevelopment/prerequisites/#workspace-virtual-environment-setup-process>
- [stuart_build commandline parser](https://github.com/tianocore/edk2-pytool-extensions/blob/56f6a7aee09995c2f22da4765e8b0a29c1cbf5de/edk2toolext/edk2_invocable.py#L109)
