# UefiPayloadPkg - Platform CI

This ReadMe.md describes the Azure DevOps based Platform CI for UefiPayloadPkg and how
to use the same Pytools based build infrastructure locally.

## Supported Configuration Details

This solution for building UefiPayloadPkg has only been validated with Windows 11
with VS2019 + CLANG/LLVM. Different firmware builds are
supported and are described below.

| Configuration name      | Architectures      | DSC File            |Additional Flags |
| :----                   | :-----             | :----               | :----           |
| IA32                    | IA32               | UefiPayloadPkg.dsc  | None            |
| X64                     | X64                | UefiPayloadPkg.dsc  | None            |

More build configuration detail are in [UPL ReadMe](https://github.com/tianocore/edk2/blob/master/UefiPayloadPkg/Readme.md)

## EDK2 Developer environment

![Minimum Python Version](https://img.shields.io/badge/dynamic/toml?url=https%3A%2F%2Fraw.githubusercontent.com%2Ftianocore%2Fedk2-pytool-extensions%2Frefs%2Fheads%2Fmaster%2Fpyproject.toml&query=%24.%5B'requires-python'%5D&style=for-the-badge&logo=python&logoColor=ffd343&label=Minimum%20Python%20Version%20for%20CI&color=3776ab&link=https%3A%2F%2Fwww.python.org%2Fdownloads%2F)

- [GIT - Download & Install](https://git-scm.com/download/)
- [Edk2 Source](https://github.com/tianocore/edk2)

Note: edksetup, Submodule initialization and manual installation of NASM, iASL, or
the required cross-compiler toolchains are **not** required, this is handled by the
Pytools build system.

## Building with Pytools for UefiPayloadPkg

If you are unfamiliar with Pytools, it is recommended to first read through
the generic set of edk2 [Build Instructions](https://github.com/tianocore/tianocore.github.io/wiki/Build-Instructions).

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
    stuart_setup -c UefiPayloadPkg/PlatformCI/PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>
    ```

5. Initialize & Update Dependencies - only as needed when ext_deps change

    ``` bash
    stuart_update -c UefiPayloadPkg/PlatformCI/PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>
    ```

6. Compile the basetools if necessary - only when basetools C source files change

    ``` bash
    python BaseTools/Edk2ToolsBuild.py -t <ToolChainTag>
    ```

7. Compile Firmware

    ``` bash
    stuart_build -c UefiPayloadPkg/PlatformCI/PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>
    ```

    - use `stuart_build -c UefiPayloadPkg/PlatformCI/PlatformBuild.py -h` option to see additional
    options like `--clean`

### Notes

1. Configuring *ACTIVE_PLATFORM* and *TARGET_ARCH* in Conf/target.txt is **not** required. This
   environment is set by PlatformBuild.py based upon the `[-a <TARGET_ARCH>]` parameter.

**NOTE:** Logging the execution output will be in the normal stuart log as well as to your console.

### Custom Build Options

### Passing Build Defines

To pass build defines through _stuart_build_, prepend `BLD_*_`to the define name and pass it on the
command-line. _stuart_build_ currently requires values to be assigned, so add an`=1` suffix for bare defines.
For example, to enable the IP6 Network Stack, the stuart_build command-line would be:

`stuart_build -c UefiPayloadPkg/PlatformCI/PlatformBuild.py BLD_*_NETWORK_DRIVER_ENABLE=1`

## References

- [Installing and using Pytools](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/using.md#installing)
- More on [python virtual environments](https://docs.python.org/3/library/venv.html)
