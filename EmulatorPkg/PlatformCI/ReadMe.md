# EmulatorPkg - Platform CI

This ReadMe.md describes the Azure DevOps based Platform CI for EmulatorPkg and how
to use the same Pytools based build infrastructure locally.

## Supported Configuration Details

This solution for building and running EmulatorPkg has only been validated with Windows 10
with VS2019 and Ubuntu 18.04 with GCC5 toolchain. Four different firmware builds are
supported and are described below.

| Configuration name      | Architectures      | DSC File         |Additional Flags |
| :----                   | :-----             | :----            | :----           |
| IA32                    | IA32               | EmulatorPkg.dsc  | None            |
| X64                     | X64                | EmulatorPkg.dsc  | None            |
| IA32 Full               | IA32               | EmulatorPkg.dsc  | SECURE_BOOT_ENABLE=TRUE |
| X64 Full                | X64                | EmulatorPkg.dsc  | SECURE_BOOT_ENABLE=TRUE |

## EDK2 Developer environment

- [Python 3.8.x - Download & Install](https://www.python.org/downloads/)
- [GIT - Download & Install](https://git-scm.com/download/)
- [Edk2 Source](https://github.com/tianocore/edk2)
- For building Basetools and other host applications

  ``` bash
  sudo apt-get update
  sudo apt-get install gcc g++ make uuid-dev
  ```

- For building ARCH IA32 on X64 Ubuntu 18.04 LTS these steps where needed.

  ``` bash
  sudo dpkg --add-architecture i386
  sudo apt-get update
  sudo apt-get install libc6-dev:i386 libx11-dev:i386 libxext-dev:i386 lib32gcc-7-dev
  ```

Note: edksetup, Submodule initialization and manual installation of NASM, iASL, or
the required cross-compiler toolchains are **not** required, this is handled by the
Pytools build system.

## Building with Pytools for EmulatorPkg

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
    stuart_setup -c EmulatorPkg/PlatformCI/PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>
    ```

5. Initialize & Update Dependencies - only as needed when ext_deps change

    ``` bash
    stuart_update -c EmulatorPkg/PlatformCI/PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>
    ```

6. Compile the basetools if necessary - only when basetools C source files change

    ``` bash
    python BaseTools/Edk2ToolsBuild.py -t <ToolChainTag>
    ```

7. Compile Firmware

    ``` bash
    stuart_build -c EmulatorPkg/PlatformCI/PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>
    ```

    - use `stuart_build -c EmulatorPkg/PlatformCI/PlatformBuild.py -h` option to see additional
    options like `--clean`

8. Running Emulator
    - You can add `--FlashRom` to the end of your build command and the emulator will run after the
    build is complete.
    - or use the `--FlashOnly` feature to just run the emulator.

      ``` bash
      stuart_build -c EmulatorPkg/PlatformCI/PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH> --FlashOnly
      ```

### Notes

1. Configuring *ACTIVE_PLATFORM* and *TARGET_ARCH* in Conf/target.txt is **not** required. This
   environment is set by PlatformBuild.py based upon the `[-a <TARGET_ARCH>]` parameter.

**NOTE:** Logging the execution output will be in the normal stuart log as well as to your console.

### Custom Build Options

**MAKE_STARTUP_NSH=TRUE** will output a *startup.nsh* file to the location mapped as fs0. This is
used in CI in combination with the `--FlashOnly` feature to run the Emulator to the UEFI shell and then execute
the contents of *startup.nsh*.

### Passing Build Defines

To pass build defines through _stuart_build_, prepend `BLD_*_`to the define name and pass it on the
command-line. _stuart_build_ currently requires values to be assigned, so add an`=1` suffix for bare defines.
For example, to enable the IP6 Network Stack, the stuart_build command-line would be:

`stuart_build -c EmulatorPkg/PlatformCI/PlatformBuild.py BLD_*_NETWORK_IP6_ENABLE=1`

## References

- [Installing and using Pytools](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/using.md#installing)
- More on [python virtual environments](https://docs.python.org/3/library/venv.html)
