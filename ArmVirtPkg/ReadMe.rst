==========
ArmVirtPkg
==========

This ReadMe.rst summarizes the current state of Azure DevOps based Platform CI
for ArmVirtPkg. It also describes how to *build* ArmVirtPkg locally using the Pytools build system.

Platform CI Current Status
--------------------------

AARCH64 Configuration
`````````````````````
=============== ============= ============= =============
 Toolchain      DEBUG         RELEASE       NOOPT
=============== ============= ============= =============
`Ubuntu GCC5`_  |apAArch64du| |apAArch64ru| |apAArch64nu|
=============== ============= ============= =============

ARM Configuration
`````````````````
=============== ============= ============= =============
 Toolchain      DEBUG         RELEASE       NOOPT
=============== ============= ============= =============
`Ubuntu GCC5`_  |apArmdu|     |apArmru|     |apArmnu|
=============== ============= ============= =============

Setup
-----

The Usual EDK2 Build Setup
``````````````````````````

- `Python 3.8.x - Download & Install <https://www.python.org/downloads/>`_
- `GIT - Download & Install <https://git-scm.com/download/>`_
- `GIT - Configure for EDK II <https://github.com/tianocore/tianocore.github.io/wiki/Windows-systems#github-help>`_
- `QEMU - Download, Install, and add to your path <https://www.qemu.org/download/>`_
- `EDKII Source - Download/Checkout from Github <https://github.com/tianocore/tianocore.github.io/wiki/Windows-systems#download>`_

**NOTE:** Do *not* follow the EDK II Compile Tools and Build instructions, see below...

Differences from EDK Classic Build Setup
````````````````````````````````````````

- Build BaseTools using `python BaseTools/Edk2ToolsBuild.py [-t <ToolChainTag>]`

  - This replaces `edksetup Rebuild`" from the classic build system

- **No Action:** edksetup, Submodule initialization and manual installation of NASM and iASL are **not**
  required, it is handled by the Pytools build system.

Building with Pytools for ArmVirtPkg
````````````````````````````````````

* Install Pytools

  .. code-block:: bash

    pip install --upgrade -r pip-requirements.txt

* Initialize & Update Submodules

  .. code-block:: bash

    stuart_setup -c ArmVirtPkg/PlatformCI/PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>

* Initialize & Update Dependencies (e.g. iASL & NASM)

  .. code-block:: bash

    stuart_update -c ArmVirtPkg/PlatformCI/PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>

* Compile

  .. code-block:: bash

    stuart_build -c ArmVirtPkg/PlatformCI/PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>

* Running Emulator

  - You can add `--FlashRom` to the end of your build command and the emulator will run after the build is complete.
  - or use the `--FlashOnly` feature to just run the emulator.

  .. code-block:: bash

    stuart_build -c ArmVirtPkg/PlatformCI/PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH> --FlashOnly

Notes
`````

1. Including the expected build architecture and toolchain to the *stuart_update* command is critical. This is because there
   are extra scopes and tools that will be resolved during the update step that need to match your build step.
2. Configuring *ACTIVE_PLATFORM* and *TARGET_ARCH* in Conf/target.txt is *not* required. This environment is set by
   PlatformBuild.py based upon the `[-a <TARGET_ARCH>]` parameter.
3. QEMU must be on your path.  On Windows this is a manual process and not part of the QEMU installer.

**NOTE:** Logging the execution output will be in the normal stuart log as well as to your console.

Custom Build Options
````````````````````

**MAKE_STARTUP_NSH=TRUE** will output a *startup.nsh* file to the location mapped as fs0. This is used in CI in combination
with the `--FlashOnly` feature to run QEMU to the UEFI shell and then execute the contents of startup.nsh.

**QEMU_HEADLESS=TRUE** Since CI servers run headless QEMU must be told to run with no display otherwise an error occurs.
Locally you don't need to set this.

Passing Build Defines
`````````````````````
To pass build defines through stuart_build, prepend `BLD_*_` to the define name and pass it on the commandline. stuart_build currently
requires values to be assigned, so add a `=1` suffix for bare defines.
For example, to enable TPM2 support, instead of the traditional "-D TPM2_ENABLE=TRUE", the stuart_build command-line would be:

.. code-block:: bash

  stuart_build -c ArmVirtPkg/PlatformCI/PlatformBuild.py BLD_*_TPM2_ENABLE=TRUE

References
----------
- `Installing Pytools <https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/using.md#installing>`_
- For each workspace, consider creating & using a `Python Virtual Environment <https://docs.python.org/3/library/venv.html>`_

  * `Sample Layout <https://microsoft.github.io/mu/CodeDevelopment/prerequisites/#workspace-virtual-environment-setup-process>`_

- `stuart_build commandline parser <https://github.com/tianocore/edk2-pytool-extensions/blob/56f6a7aee09995c2f22da4765e8b0a29c1cbf5de/edk2toolext/edk2_invocable.py#L109>`_


.. ===================================================================
.. This is a bunch of directives to make the README file more readable
.. ===================================================================

.. _Ubuntu GCC5: https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=41&branchName=master

.. |apAArch64du| image:: https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/ArmVirtPkg/ArmVirtQemu%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20QEMU_AARCH64_DEBUG
.. |apAArch64ru| image:: https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/ArmVirtPkg/ArmVirtQemu%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20QEMU_AARCH64_RELEASE
.. |apAArch64nu| image:: https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/ArmVirtPkg/ArmVirtQemu%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20QEMU_AARCH64_NOOPT

.. |apArmdu| image:: https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/ArmVirtPkg/ArmVirtQemu%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20QEMU_ARM_DEBUG
.. |apArmru| image:: https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/ArmVirtPkg/ArmVirtQemu%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20QEMU_ARM_RELEASE
.. |apArmnu| image:: https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/ArmVirtPkg/ArmVirtQemu%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20QEMU_ARM_NOOPT
