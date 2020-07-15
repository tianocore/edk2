# OvmfPkg - Platform CI

This ReadMe.md describes the Azure DevOps based Platform CI for OvmfPkg and how
to use the same Pytools based build infrastructure locally.

## Supported Configuration Details

This solution for building and running OvmfPkg has only been validated with Windows 10
with VS2019 and Ubuntu 18.04 with GCC5 toolchain. Four different firmware builds are
supported and are described below.

| Configuration name      | Architectures      | DSC File            |Additional Flags |
| :----                   | :-----             | :----               | :----           |
| IA32                    | IA32               | OvmfPkgIa32.dsc     | None            |
| X64                     | X64                | OvmfPkgIa64.dsc     | None            |
| IA32 X64                | PEI-IA32 DXE-X64   | OvmfPkgIa32X64.dsc  | None            |
| IA32 X64 Full           | PEI-IA32 DXE-X64   | OvmfPkgIa32X64.dsc  | SECURE_BOOT_ENABLE=1 SMM_REQUIRE=1 TPM_ENABLE=1 TPM_CONFIG_ENABLE=1 NETWORK_TLS_ENABLE=1 NETWORK_IP6_ENABLE=1 NETWORK_HTTP_BOOT_ENABLE=1 |

## EDK2 Developer environment

- [Python 3.8.x - Download & Install](https://www.python.org/downloads/)
- [GIT - Download & Install](https://git-scm.com/download/)
- [QEMU - Download, Install, and add to your path](https://www.qemu.org/download/)
- [Edk2 Source](https://github.com/tianocore/edk2)
- Additional packages found necessary for Ubuntu 18.04
  - apt-get install gcc g++ make uuid-dev

Note: edksetup, Submodule initialization and manual installation of NASM, iASL, or
the required cross-compiler toolchains are **not** required, this is handled by the
Pytools build system.

## Building with Pytools for OvmfPkg

1. [Optional] Create a Python Virtual Environment - generally once per workspace

    ``` bash
    python -m venv <name of virtual environment>
    ```

2. [Optional] Activate Virtual Environment - each time new shell opened
    - Linux

      ```bash
      source <name of virtual environment>/bin/activate
      ```

    - Windows

      ``` bash
      <name of virtual environment>/Scripts/activate.bat
      ```

3. Install Pytools - generally once per virtual env or whenever pip-requirements.txt changes

    ``` bash
    pip install --upgrade -r pip-requirements.txt
    ```

4. Initialize & Update Submodules - only when submodules updated

    ``` bash
    stuart_setup -c OvmfPkg/PlatformCI/PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>
    ```

5. Initialize & Update Dependencies - only as needed when ext_deps change

    ``` bash
    stuart_update -c OvmfPkg/PlatformCI/PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>
    ```

6. Compile the basetools if necessary - only when basetools C source files change

    ``` bash
    python BaseTools/Edk2ToolsBuild.py -t <ToolChainTag>
    ```

7. Compile Firmware
    - To build IA32

    ``` bash
    stuart_build -c OvmfPkg/PlatformCI/PlatformBuild.py -a IA32 TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG>
    ```

    - To build X64

    ``` bash
    stuart_build -c OvmfPkg/PlatformCI/PlatformBuild.py -a X64 TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG>
    ```

    - To build IA32 X64

    ``` bash
    stuart_build -c OvmfPkg/PlatformCI/PlatformBuild.py -a IA32,X64 TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG>
    ```

    - use `stuart_build -c OvmfPkg/PlatformCI/PlatformBuild.py -h` option to see additional
    options like `--clean`

8. Running Emulator
    - You can add `--FlashRom` to the end of your build command and the emulator will run after the
    build is complete.
    - or use the `--FlashOnly` feature to just run the emulator.

      ``` bash
      stuart_build -c OvmfPkg/PlatformCI/PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH> --FlashOnly
      ```

### Notes

1. Configuring *ACTIVE_PLATFORM* and *TARGET_ARCH* in Conf/target.txt is **not** required. This
   environment is set by PlatformBuild.py based upon the `[-a <TARGET_ARCH>]` parameter.
2. QEMU must be on your path.  On Windows this is a manual process and not part of the QEMU installer.

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
For example, to enable the TPM2 support, instead of the traditional "-D E1000_ENABLE", the stuart_build
command-line would be:

`stuart_build -c OvmfPkg/PlatformCI/PlatformBuild.py BLD_*_E1000_ENABLE=1`

## References

- [Installing and using Pytools](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/using.md#installing)
- More on [python virtual environments](https://docs.python.org/3/library/venv.html)
