==============
EDK II Project
==============

A modern, feature-rich, cross-platform firmware development
environment for the UEFI and PI specifications from www.uefi.org.

Core CI Build Status
--------------------

============================= ================= =============== ===================
 Host Type & Toolchain        Build Status      Test Status     Code Coverage
============================= ================= =============== ===================
Windows_VS2019_               |WindowsCiBuild|  |WindowsCiTest| |WindowsCiCoverage|
Ubuntu_GCC5_                  |UbuntuCiBuild|   |UbuntuCiTest|  |UbuntuCiCoverage|
============================= ================= =============== ===================

`More CI Build information <.pytool/Readme.md>`__

Platform CI Build Status
------------------------

Microsoft Windows VS2019
````````````````````````

============================= ================= ============= ============= ==============
 Toolchain                    CONFIG            DEBUG         RELEASE       NOOPT
============================= ================= ============= ============= ==============
EmulatorPkg_Win_VS2019_       | IA32            |em32d|       |em32r|       |em32n|
|                             | X64             |em64d|       |em64r|       |em64n|
|                             | IA32 FULL       |em32fd|      |em32fr|      |em32fn|
|                             | X64 FULL        |em64fd|      |em64fr|      |em64fn|
OvmfPkg_Win_VS2019_           | IA32            |op32d|       |op32r|       |op32n|
|                             | X64             |op64d|       |op64r|       |op64n|
|                             | IA32 X64        |op3264d|     |op3264r|     |op3264n|
|                             | IA32 X64 FULL   |op3264fd|    |op3264fr|    |op3264fn|
============================= ================= ============= ============= ==============

Ubuntu 18.04 GCC5
`````````````````

============================= ================= ============= ============= ==============
 Toolchain                    CONFIG            DEBUG         RELEASE       NOOPT
============================= ================= ============= ============= ==============
ArmVirtPkg_Ubuntu_GCC5_       | AARCH64         |avAArch64du| |avAArch64ru| |avAArch64nu|
|                             | ARM             |avArmdu|     |avArmru|     |avArmnu|
EmulatorPkg_Ubuntu_GCC5_      | IA32            |em32du|      |em32ru|      |em32nu|
|                             | X64             |em64du|      |em64ru|      |em64nu|
|                             | IA32 FULL       |em32fdu|     |em32fru|     |em32fnu|
|                             | X64 FULL        |em64fdu|     |em64fru|     |em64fnu|
OvmfPkg_Ubuntu_GCC5_          | IA32            |op32du|      |op32ru|      |op32nu|
|                             | X64             |op64du|      |op64ru|      |op64nu|
|                             | IA32 X64        |op3264du|    |op3264ru|    |op3264nu|
|                             | IA32 X64 FULL   |op3264fdu|   |op3264fru|   |op3264fru|
============================= ================= ============= ============= ==============

|TCBZ_2668|_ - EmulatorPkg Ubuntu GCC5 Segfaults during execution.

|TCBZ_2639|_ - EmulatorPkg Ubuntu GCC5 Segfaults during execution.

`More ArmVirtPkg CI Build Information <ArmVirtPkg/PlatformCI/ReadMe.md>`__

`More EmulatorPkg CI Build Information <EmulatorPkg/PlatformCI/ReadMe.md>`__

`More OvmfPkg CI Build Information <OvmfPkg/PlatformCI/ReadMe.md>`__


License Details
---------------

The majority of the content in the EDK II open source project uses a
`BSD-2-Clause Plus Patent License <License.txt>`__. The EDK II open
source project contains the following components that are covered by additional
licenses:

-  `BaseTools/Source/C/LzmaCompress <BaseTools/Source/C/LzmaCompress/LZMA-SDK-README.txt>`__
-  `BaseTools/Source/C/VfrCompile/Pccts <BaseTools/Source/C/VfrCompile/Pccts/RIGHTS>`__
-  `CryptoPkg\Library\BaseCryptLib\SysCall\inet_pton.c <CryptoPkg\Library\BaseCryptLib\SysCall\inet_pton.c>`__
-  `CryptoPkg\Library\Include\crypto\dso_conf.h <https://github.com/openssl/openssl/blob/e2e09d9fba1187f8d6aafaa34d4172f56f1ffb72/LICENSE>`__
-  `CryptoPkg\Library\Include\openssl\opensslconf.h <https://github.com/openssl/openssl/blob/e2e09d9fba1187f8d6aafaa34d4172f56f1ffb72/LICENSE>`__
-  `EmbeddedPkg/Library/FdtLib <EmbeddedPkg/Library/FdtLib/fdt.c>`__.  (EDK II uses BSD License)
-  `EmbeddedPkg/Include/fdt.h <EmbeddedPkg/Include/fdt.h>`__.  (EDK II uses BSD Licence)
-  `EmbeddedPkg/Include/libfdt.h <EmbeddedPkg/Include/libfdt.h>`__.  (EDK II uses BSD License)
-  `MdeModulePkg/Library/LzmaCustomDecompressLib <MdeModulePkg/Library/LzmaCustomDecompressLib/LZMA-SDK-README.txt>`__
-  `OvmfPkg <OvmfPkg/License.txt>`__

The EDK II open source project uses content from upstream projects as git submodules
that are covered by additional licenses.

-  `ArmPkg/Library/ArmSoftFloatLib/berkeley-softfloat-3 <https://github.com/ucb-bar/berkeley-softfloat-3/blob/b64af41c3276f97f0e181920400ee056b9c88037/COPYING.txt>`__
-  `BaseTools/Source/C/BrotliCompress/brotli <https://github.com/google/brotli/blob/666c3280cc11dc433c303d79a83d4ffbdd12cc8d/LICENSE>`__
-  `CryptoPkg/Library/OpensslLib/openssl <https://github.com/openssl/openssl/blob/e2e09d9fba1187f8d6aafaa34d4172f56f1ffb72/LICENSE>`__
-  `MdeModulePkg/Library/BrotliCustomDecompressLib/brotli <https://github.com/google/brotli/blob/666c3280cc11dc433c303d79a83d4ffbdd12cc8d/LICENSE>`__
-  `MdeModulePkg/Universal/RegularExpressionDxe/oniguruma <https://github.com/kkos/oniguruma/blob/abfc8ff81df4067f309032467785e06975678f0d/COPYING>`__
-  `UnitTestFrameworkPkg/Library/CmockaLib/cmocka <https://github.com/tianocore/edk2-cmocka/blob/f5e2cd77c88d9f792562888d2b70c5a396bfbf7a/COPYING>`__
-  `RedfishPkg/Library/JsonLib/jansson <https://github.com/akheron/jansson/blob/2882ead5bb90cf12a01b07b2c2361e24960fae02/LICENSE>`__

The EDK II Project is composed of packages. The maintainers for each package
are listed in `Maintainers.txt <Maintainers.txt>`__.

Resources
---------

-  `TianoCore <http://www.tianocore.org>`__
-  `EDK
   II <https://github.com/tianocore/tianocore.github.io/wiki/EDK-II>`__
-  `Getting Started with EDK
   II <https://github.com/tianocore/tianocore.github.io/wiki/Getting-Started-with-EDK-II>`__
-  `Mailing
   Lists <https://github.com/tianocore/tianocore.github.io/wiki/Mailing-Lists>`__
-  `TianoCore Bugzilla <https://bugzilla.tianocore.org>`__
-  `How To
   Contribute <https://github.com/tianocore/tianocore.github.io/wiki/How-To-Contribute>`__
-  `Release
   Planning <https://github.com/tianocore/tianocore.github.io/wiki/EDK-II-Release-Planning>`__

Code Contributions
------------------

To make a contribution to a TianoCore project, follow these steps.

#. Create a change description in the format specified below to
    use in the source control commit log.
#. Your commit message must include your ``Signed-off-by`` signature
#. Submit your code to the TianoCore project using the process
    that the project documents on its web page. If the process is
    not documented, then submit the code on development email list
    for the project.
#. It is preferred that contributions are submitted using the same
    copyright license as the base project. When that is not possible,
    then contributions using the following licenses can be accepted:

-  BSD (2-clause): http://opensource.org/licenses/BSD-2-Clause
-  BSD (3-clause): http://opensource.org/licenses/BSD-3-Clause
-  MIT: http://opensource.org/licenses/MIT
-  Python-2.0: http://opensource.org/licenses/Python-2.0
-  Zlib: http://opensource.org/licenses/Zlib

For documentation:

-  FreeBSD Documentation License
    https://www.freebsd.org/copyright/freebsd-doc-license.html

Contributions of code put into the public domain can also be accepted.

Contributions using other licenses might be accepted, but further
review will be required.

Developer Certificate of Origin
-------------------------------

Your change description should use the standard format for a
commit message, and must include your ``Signed-off-by`` signature.

In order to keep track of who did what, all patches contributed must
include a statement that to the best of the contributor's knowledge
they have the right to contribute it under the specified license.

The test for this is as specified in the `Developer's Certificate of
Origin (DCO) 1.1 <https://developercertificate.org/>`__. The contributor
certifies compliance by adding a line saying

Signed-off-by: Developer Name developer@example.org

where ``Developer Name`` is the contributor's real name, and the email
address is one the developer is reachable through at the time of
contributing.

::

    Developer's Certificate of Origin 1.1

    By making a contribution to this project, I certify that:

    (a) The contribution was created in whole or in part by me and I
        have the right to submit it under the open source license
        indicated in the file; or

    (b) The contribution is based upon previous work that, to the best
        of my knowledge, is covered under an appropriate open source
        license and I have the right under that license to submit that
        work with modifications, whether created in whole or in part
        by me, under the same open source license (unless I am
        permitted to submit under a different license), as indicated
        in the file; or

    (c) The contribution was provided directly to me by some other
        person who certified (a), (b) or (c) and I have not modified
        it.

    (d) I understand and agree that this project and the contribution
        are public and that a record of the contribution (including all
        personal information I submit with it, including my sign-off) is
        maintained indefinitely and may be redistributed consistent with
        this project or the open source license(s) involved.

Sample Change Description / Commit Message
------------------------------------------

::

    From: Contributor Name <contributor@example.com>
    Subject: [Repository/Branch PATCH] Pkg-Module: Brief-single-line-summary

    Full-commit-message

    Signed-off-by: Contributor Name <contributor@example.com>

Notes for sample patch email
````````````````````````````

-  The first line of commit message is taken from the email's subject
   line following ``[Repository/Branch PATCH]``. The remaining portion
   of the commit message is the email's content.
-  ``git format-patch`` is one way to create this format

Definitions for sample patch email
``````````````````````````````````

-  ``Repository`` is the identifier of the repository the patch applies.
    This identifier should only be provided for repositories other than
    ``edk2``. For example ``edk2-BuildSpecification`` or ``staging``.
-  ``Branch`` is the identifier of the branch the patch applies. This
    identifier should only be provided for branches other than
   ``edk2/master``.
    For example ``edk2/UDK2015``,
   ``edk2-BuildSpecification/release/1.27``, or
    ``staging/edk2-test``.
-  ``Module`` is a short identifier for the affected code or
   documentation. For example ``MdePkg``, ``MdeModulePkg/UsbBusDxe``, ``Introduction``, or
    ``EDK II INF File Format``.
-  ``Brief-single-line-summary`` is a short summary of the change.
-  The entire first line should be less than ~70 characters.
-  ``Full-commit-message`` a verbose multiple line comment describing
    the change. Each line should be less than ~70 characters.
-  ``Signed-off-by`` is the contributor's signature identifying them
    by their real/legal name and their email address.

Submodules
----------

Submodule in EDK II is allowed but submodule chain should be avoided
as possible as we can. Currently EDK II contains the following submodules

-  CryptoPkg/Library/OpensslLib/openssl
-  ArmPkg/Library/ArmSoftFloatLib/berkeley-softfloat-3
-  MdeModulePkg/Universal/RegularExpressionDxe/oniguruma
-  MdeModulePkg/Library/BrotliCustomDecompressLib/brotli
-  BaseTools/Source/C/BrotliCompress/brotli

ArmSoftFloatLib is actually required by OpensslLib. It's inevitable
in openssl-1.1.1 (since stable201905) for floating point parameter
conversion, but should be dropped once there's no such need in future
release of openssl.

To get a full, buildable EDK II repository, use following steps of git
command

.. code-block:: bash

  git clone https://github.com/tianocore/edk2.git
  cd edk2
  git submodule update --init
  cd ..

If there's update for submodules, use following git commands to get
the latest submodules code.

.. code-block:: bash

  cd edk2
  git pull
  git submodule update

Note: When cloning submodule repos, '--recursive' option is not
recommended. EDK II itself will not use any code/feature from
submodules in above submodules. So using '--recursive' adds a
dependency on being able to reach servers we do not actually want
any code from, as well as needlessly downloading code we will not
use.

.. ===================================================================
.. This is a bunch of directives to make the README file more readable
.. ===================================================================

.. CoreCI

.. _Windows_VS2019: https://dev.azure.com/tianocore/edk2-ci/_build/latest?definitionId=32&branchName=master
.. |WindowsCiBuild| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/Windows%20VS2019%20CI?branchName=master
.. |WindowsCiTest| image:: https://img.shields.io/azure-devops/tests/tianocore/edk2-ci/32.svg
.. |WindowsCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue

.. _Ubuntu_GCC5: https://dev.azure.com/tianocore/edk2-ci/_build/latest?definitionId=31&branchName=master
.. |UbuntuCiBuild| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/Ubuntu%20GCC5%20CI?branchName=master
.. |UbuntuCiTest| image:: https://img.shields.io/azure-devops/tests/tianocore/edk2-ci/31.svg
.. |UbuntuCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue

.. ArmVirtPkg

.. _ArmVirtPkg_Ubuntu_GCC5: https://dev.azure.com/tianocore/edk2-ci/_build/latest?definitionId=46&branchName=master
.. |avAArch64du| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_ArmVirtPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20QEMU_AARCH64_DEBUG
.. |avAArch64ru| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_ArmVirtPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20QEMU_AARCH64_RELEASE
.. |avAArch64nu| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_ArmVirtPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20QEMU_AARCH64_NOOPT

.. |avArmdu| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_ArmVirtPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20QEMU_ARM_DEBUG
.. |avArmru| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_ArmVirtPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20QEMU_ARM_RELEASE
.. |avArmnu| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_ArmVirtPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20QEMU_ARM_NOOPT

.. EmulatorPkg

.. |TCBZ_2668| image:: https://img.shields.io/bugzilla/2668?baseUrl=https%3A%2F%2Fbugzilla.tianocore.org
.. _TCBZ_2668: https://bugzilla.tianocore.org/show_bug.cgi?id=2668

.. |TCBZ_2639| image:: https://img.shields.io/bugzilla/2639?baseUrl=https%3A%2F%2Fbugzilla.tianocore.org
.. _TCBZ_2639: https://bugzilla.tianocore.org/show_bug.cgi?id=2639

.. _EmulatorPkg_Win_VS2019:  https://dev.azure.com/tianocore/edk2-ci/_build/latest?definitionId=44&branchName=master
.. _EmulatorPkg_Ubuntu_GCC5: https://dev.azure.com/tianocore/edk2-ci/_build/latest?definitionId=43&branchName=master

.. |em32d| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_DEBUG
.. |em32du| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_DEBUG
.. |em32r| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_RELEASE
.. |em32ru| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_RELEASE
.. |em32n| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_NOOPT
.. |em32nu| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_NOOPT

.. |em32fd| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_FULL_DEBUG
.. |em32fdu| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_FULL_DEBUG
.. |em32fr| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_FULL_RELEASE
.. |em32fru| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_FULL_RELEASE
.. |em32fn| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_FULL_NOOPT
.. |em32fnu| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_IA32_FULL_NOOPT

.. |em64d| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_DEBUG
.. |em64du| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_DEBUG
.. |em64r| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_RELEASE
.. |em64ru| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_RELEASE
.. |em64n| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_NOOPT
.. |em64nu| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_NOOPT

.. |em64fd| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_FULL_DEBUG
.. |em64fdu| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_FULL_DEBUG
.. |em64fr| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_FULL_RELEASE
.. |em64fru| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_FULL_RELEASE
.. |em64fn| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_FULL_NOOPT
.. |em64fnu| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_EmulatorPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20EmulatorPkg_X64_FULL_NOOPT

.. OvmfPkg

.. |TCBZ_2661| image:: https://img.shields.io/bugzilla/2661?baseUrl=https%3A%2F%2Fbugzilla.tianocore.org
.. _TCBZ_2661: https://bugzilla.tianocore.org/show_bug.cgi?id=2661

.. _OvmfPkg_Win_VS2019:  https://dev.azure.com/tianocore/edk2-ci/_build/latest?definitionId=50&branchName=master
.. _OvmfPkg_Ubuntu_GCC5: https://dev.azure.com/tianocore/edk2-ci/_build/latest?definitionId=48&branchName=master

.. |op32d| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32_DEBUG
.. |op32du| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32_DEBUG
.. |op32r| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32_RELEASE
.. |op32ru| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32_RELEASE
.. |op32n| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32_NOOPT
.. |op32nu| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32_NOOPT

.. |op64d| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_X64_DEBUG
.. |op64du| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_X64_DEBUG
.. |op64r| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_X64_RELEASE
.. |op64ru| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_X64_RELEASE
.. |op64n| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_X64_NOOPT
.. |op64nu| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_X64_NOOPT


.. |op3264d| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_DEBUG
.. |op3264du| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_DEBUG
.. |op3264r| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_RELEASE
.. |op3264ru| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_RELEASE
.. |op3264n| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_NOOPT
.. |op3264nu| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_NOOPT

.. |op3264fd| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_FULL_DEBUG
.. |op3264fdu| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_FULL_DEBUG
.. |op3264fr| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Windows_VS2019_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_FULL_RELEASE
.. |op3264fru| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_FULL_RELEASE
.. |op3264fn| replace:: |TCBZ_2661|_
.. |op3264fnu| image:: https://dev.azure.com/tianocore/edk2-ci/_apis/build/status/PlatformCI_OvmfPkg_Ubuntu_GCC5_CI?branchName=master&jobName=Platform_CI&configuration=Platform_CI%20OVMF_IA32X64_FULL_NOOPT
