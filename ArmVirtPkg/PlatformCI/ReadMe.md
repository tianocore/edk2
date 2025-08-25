# ArmVirtPkg - Platform CI

This Readme.md describes the Azure DevOps based Platform CI for ArmVirtPkg and how
to use the same Pytools based build infrastructure locally.

## Supported Configuration Details

This solution for building and running ArmVirtPkg has been validated with Fedora
37 Linux and the GCC5 toolchain. The following different firmware builds are
supported.

| Configuration name      | Architecture       | DSC File               | Additional Flags |
| :----------             | :-----             | :-----                 | :----            |
| AARCH64 - KVM Cloud HV  | AARCH64            | ArmVirtCloudHv.dsc     | None             |
| AARCH64 - kvmtool       | AARCH64            | ArmVirtKvmTool.dsc     | None             |
| AARCH64 - QEMU          | AARCH64            | ArmVirtQemu.dsc        | None             |
| AARCH64 - QEMU Kernel   | AARCH64            | ArmVirtQemuKernel.dsc  | None             |
| AARCH64 - Xen HV        | AARCH64            | ArmVirtXen.dsc         | None             |

## EDK2 Developer environment

- [Python 3.12.x - Download & Install](https://www.python.org/downloads/)
- [GIT - Download & Install](https://git-scm.com/download/)
- [QEMU - Download, Install, and add to your path](https://www.qemu.org/download/)
- [Edk2 Source](https://github.com/tianocore/edk2)
- Additional packages found necessary for Fedora Linux 37
  - dnf install gcc g++ make libuuid-devel

Note: edksetup, Submodule initialization and manual installation of NASM, iASL, or
the required cross-compiler toolchains are **not** required, this is handled by the
Pytools build system.

The code is built in CI using a container. The latest Fedora Linux 37 container is
available in this GitHub container registry feed
[fedora-37-test](https://github.com/tianocore/containers/pkgs/container/containers%2Ffedora-37-test).

The exact container version tested in CI is maintained in this file
[edk2/.azurepipelines/templates/default.yml](https://github.com/tianocore/edk2/blob/HEAD/.azurepipelines/templates/defaults.yml).

## Building with Pytools for ArmVirtPkg

If you are unfamiliar with Pytools, it is recommended to first read through
the generic set of edk2 [Build Instructions](https://github.com/tianocore/tianocore.github.io/wiki/Build-Instructions).

1. [Optional] Create a Python Virtual Environment - generally once per workspace

    ``` bash
    python -m venv <name of virtual environment>
    ```

2. [Optional] Activate Virtual Environment - each time new shell opened
    - Windows

      ``` bash
      <name of virtual environment>/Scripts/activate.bat
      ```

    - Linux

      ```bash
      source <name of virtual environment>/bin/activate
      ```

3. Install Pytools - generally once per virtual env or whenever pip-requirements.txt changes

    ``` bash
    pip install --upgrade -r pip-requirements.txt
    ```

4. Initialize & Update Submodules - only when submodules updated (QEMU build example)

    ``` bash
    stuart_setup -c ArmVirtPkg/PlatformCI/QemuBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>
    ```

5. Initialize & Update Dependencies - only as needed when ext_deps change (QEMU build example)

    ``` bash
    stuart_update -c ArmVirtPkg/PlatformCI/QemuBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>
    ```

6. Compile the basetools if necessary - only when basetools C source files change

    ``` bash
    python BaseTools/Edk2ToolsBuild.py -t <ToolChainTag>
    ```

7. Compile Firmware (QEMU build example)

    ``` bash
    stuart_build -c ArmVirtPkg/PlatformCI/QemuBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>
    ```

    - use `stuart_build -c ArmVirtPkg/PlatformCI/QemuBuild.py -h` option to see additional
    options like `--clean`

8. Running Emulator
    - You can add `--FlashRom` to the end of your build command and the emulator will run after the
    build is complete.
    - or use the `--FlashOnly` feature to just run the emulator.

      ``` bash
      stuart_build -c ArmVirtPkg/PlatformCI/QemuBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH> --FlashOnly
      ```

### Notes

1. Including the expected build architecture and toolchain to the _stuart_update_ command is critical.
   This is because there are extra scopes and tools that will be resolved during the update step that
   need to match your build step.
2. Configuring *ACTIVE_PLATFORM* and *TARGET_ARCH* in Conf/target.txt is **not** required. This
   environment is set by PlatformBuild.py based upon the `[-a <TARGET_ARCH>]` parameter.
3. QEMU must be on your path.  On Windows this is a manual process and not part of the QEMU installer.

**NOTE:** Logging the execution output will be in the normal stuart log as well as to your console.

### Custom Build Options

**MAKE_STARTUP_NSH=TRUE** will output a *startup.nsh* file to the location mapped as fs0. This is
used in CI in combination with the `--FlashOnly` feature to run QEMU to the UEFI shell and then execute
the contents of *startup.nsh*.

**QEMU_HEADLESS=TRUE** Since CI servers run headless QEMU must be told to run with no display otherwise
an error occurs. Locally you don't need to set this.

### Passing Build Defines

To pass build defines through _stuart_build_, prepend `BLD_*_`to the define name and pass it on the
command-line. _stuart_build_ currently requires values to be assigned, so add an`=1` suffix for bare defines.
For example, to enable the TPM2 support, instead of the traditional "-D TPM2_ENABLE=TRUE", the stuart_build
command-line would be:

`stuart_build -c ArmVirtPkg/PlatformCI/QemuBuild.py BLD_*_TPM2_ENABLE=TRUE`

## References

- [Installing and using Pytools](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/using.md#installing)
- More on [python virtual environments](https://docs.python.org/3/library/venv.html)
