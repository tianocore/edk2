===========
EmulatorPkg
===========

This README.rst summarizes the current state of Azure DevOps Platform CI
for EmulatorPkg. It also describes how to *build* EmulatorPkg locally using the
Pytools build system. For general documentation on EmulatorPkg, refer
to the `ReadMe <./Readme.md>`_.

Platform CI Current Status
---------------------------

IA32 Configuration
``````````````````
=============== ============= ============= =============
 Toolchain      DEBUG         RELEASE       NOOPT
=============== ============= ============= =============
`Win VS2019`_   |ap32d|       |ap32r|       |ap32n|
`Ubuntu GCC5`_  |ap32du|      |ap32ru|      |ap32nu|
=============== ============= ============= =============

|TCBZ_2668|_ - Ubuntu GCC5 Segfaults during execution.  The builds
only compile for Ubuntu GCC5 (not run to shell).

X64 Configuration
`````````````````
=============== ============= ============= =============
 Toolchain      DEBUG         RELEASE       NOOPT
=============== ============= ============= =============
`Win VS2019`_   |ap64d|       |ap64r|       |ap64n|
`Ubuntu GCC5`_  |ap64du|      |ap64ru|      |ap64nu|
=============== ============= ============= =============

|TCBZ_2639|_ - Ubuntu GCC5 Segfaults during execution.  The builds
only compile for Ubuntu GCC5 (not run to shell).

Setup
-----

The Usual EDK2 Build Setup
``````````````````````````

- `Python 3.8.x - Download & Install <https://www.python.org/downloads/>`_
- `GIT - Download & Install <https://git-scm.com/download/>`_
- `GIT - Configure for EDK II <https://github.com/tianocore/tianocore.github.io/wiki/Windows-systems#github-help>`_
- `EDKII Source - Download/Checkout from Github <https://github.com/tianocore/tianocore.github.io/wiki/Windows-systems#download>`_

**NOTE:** Do *not* follow the EDK II Compile Tools and Build instructions, see below...

Install the necessary development packages for your distribution
````````````````````````````````````````````````````````````````

This varies by distribution, toolchain, and your configuration but here are a few hints.

* For building ARCH IA32 on X64 Ubuntu 18.04 LTS these steps where needed.

  .. code-block:: bash

    sudo dpkg --add-architecture i386
    sudo apt-get update
    sudo apt-get install libc6-dev:i386 libx11-dev:i386 libxext-dev:i386 lib32gcc-7-dev

* For building Basetools and other host applications

  .. code-block:: bash

    sudo apt-get update
    sudo apt-get install gcc g++ make uuid-dev

Differences from EDK Classic Build Setup
````````````````````````````````````````

- Build BaseTools using `python BaseTools/Edk2ToolsBuild.py [-t <ToolChainTag>]`

  - This replaces `edksetup Rebuild`" from the classic build system
  - For Windows `<ToolChainTag>` examples, refer to `Windows ToolChain Matrix <https://github.com/tianocore/tianocore.github.io/wiki/Windows-systems-ToolChain-Matrix>`_,
    defaults to `VS2017` if not specified

- **No Action:** edksetup, Submodule initialization and manual setup of NASM and iASL are **not** required, it is
  handled by the Pytools build system

Install & Configure Pytools for EmulatorPkg
```````````````````````````````````````````

* Install Pytools

  .. code-block:: bash

    pip install --upgrade -r pip-requirements.txt

* Initialize & Update Submodules

  .. code-block:: bash

    stuart_setup -c EmulatorPkg/PlatformCI/PlatformBuild.py

* Initialize & Update Dependencies (e.g. iASL & NASM)

  .. code-block:: bash

    stuart_update -c EmulatorPkg/PlatformCI/PlatformBuild.py

* Compile (IA32 or X64 supported)

  .. code-block:: bash

    stuart_build -c EmulatorPkg/PlatformCI/PlatformBuild.py [TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG>] -a <TARGET_ARCH>

* Running Emulator

  - You can add `--FlashRom` to the end of your build command and the emulator will run after the build is complete.
  - or use the `--FlashOnly` feature to just run the emulator.

  .. code-block:: bash

    stuart_build -c EmulatorPkg/PlatformCI/PlatformBuild.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH> --FlashOnly

**NOTE:** configuring ACTIVE_PLATFORM and TARGET_ARCH in Conf/target.txt is *not* required.
This environment is set by PlatformBuild.py based upon the `[-a <TARGET_ARCH>]` parameter.

Custom Build Options
````````````````````

**MAKE_STARTUP_NSH=TRUE** will output a *startup.nsh* file to the location mapped as fs0. This is used in CI in
combination with the `--FlashOnly` feature to run the emulator to the UEFI shell and then execute the
contents of startup.nsh.

Passing Build Defines
`````````````````````

To pass build defines through stuart_build, prepend `BLD_*_` to the define name and pass it on the command-line.
stuart_build currently requires values to be assigned, so add a `=1` suffix for bare defines.
For example, to enable the IP6 Network Stack, the stuart_build command-line would be:

.. code-block:: bash

  stuart_build -c EmulatorPkg/PlatformCI/PlatformBuild.py BLD_*_NETWORK_IP6_ENABLE=1

References
----------

- `Installing Pytools <https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/using.md#installing>`_
- For each workspace, consider creating & using a `Python Virtual Environment <https://docs.python.org/3/library/venv.html>`_

  * `Sample Layout <https://microsoft.github.io/mu/CodeDevelopment/prerequisites/#workspace-virtual-environment-setup-process>`_

- `stuart_build commandline parser <https://github.com/tianocore/edk2-pytool-extensions/blob/56f6a7aee09995c2f22da4765e8b0a29c1cbf5de/edk2toolext/edk2_invocable.py#L109>`_


.. ===================================================================
.. This is a bunch of directives to make the README file more readable
.. ===================================================================

.. |TCBZ_2668| image:: https://img.shields.io/bugzilla/2668?baseUrl=https%3A%2F%2Fbugzilla.tianocore.org
.. _TCBZ_2668: https://bugzilla.tianocore.org/show_bug.cgi?id=2668

.. |TCBZ_2639| image:: https://img.shields.io/bugzilla/2639?baseUrl=https%3A%2F%2Fbugzilla.tianocore.org
.. _TCBZ_2639: https://bugzilla.tianocore.org/show_bug.cgi?id=2639

.. _Win VS2019:  https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=40&branchName=master
.. _Ubuntu GCC5: https://dev.azure.com/tianocore/edk2-ci-play/_build/latest?definitionId=39&branchName=master

.. |ap32d| image:: https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Windows%20VS2019?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_DEBUG
.. |ap32du| image:: https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_DEBUG
.. |ap32r| image:: https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Windows%20VS2019?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_RELEASE
.. |ap32ru| image:: https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_RELEASE
.. |ap32n| image:: https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Windows%20VS2019?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_NOOPT
.. |ap32nu| image:: https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_NOOPT

.. |ap64d| image:: https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_DEBUG
.. |ap64du| image:: https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_DEBUG
.. |ap64r| image:: https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_RELEASE
.. |ap64ru| image:: https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_RELEASE
.. |ap64n| image:: https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_NOOPT
.. |ap64nu| image:: https://dev.azure.com/tianocore/edk2-ci-play/_apis/build/status/EmulatorPkg/EmulatorPkg%20Ubuntu%20GCC5?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_NOOPT
