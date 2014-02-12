AArch64 UEFI bootstraps
=======================

Copyright (c) 2011-2013 ARM Limited. All rights reserved.
See the `LICENSE.TXT` file for more information.

Contents:

* Introduction
* Build
* Use on ARMv8 RTSM and FVP models
* Use on ARMv8 Foundation model


Introduction
------------

A bootstrap can be used to change the model state, like the Exception
Level (EL), before executing the UEFI binary.

For the ARMv8 RTSM and FVP models this can be used to show/test the UEFI binary
starting at different exception levels. The ARMv8 models start at EL3 by
default.

In the case of the Foundation model a bootstrap is required to jump to the
UEFI binary as loaded in RAM. This is required as the Foundation model cannot
load and execute UEFI binaries directly. The Foundation model can only load and
execute ELF binaries.


Build
-----

Build the bootstraps using a AArch64 GCC cross-compiler. By default the
`Makefile` is configured to assume a GCC bare-metal toolchain:

    PATH=$PATH:<path/to/baremetal-tools/bin/> make clean
    PATH=$PATH:<path/to/baremetal-tools/bin/> make

To build the bootstraps with a Linux GCC toolchain use the following
commands:

    PATH=$PATH:<path/to/aarch64-linux-gnu-tools/bin/> make clean
    PATH=$PATH:<path/to/aarch64-linux-gnu-tools/bin/> CROSS_COMPILE=<gcc-prefix> make

The `gcc-prefix` depends on the specific toolchain distribution used. It can be
"aarch64-linux-gnu-" for example.

This will result in four `axf` files:

* uefi-bootstrap-el3 : The bootstrap jumps to the UEFI code in FLASH without
                     changing anything.

* uefi-bootstrap-el2 : Setup EL3 and switch the model to EL2 before jumping to the
                     UEFI code in FLASH.

* uefi-bootstrap-el1 : Setup EL3 and prepare to run at non-secure EL1. Switch to
                     non-secure EL1 and run the UEFI code in FLASH.

* uefi-bootstrap-el3-foundation : The bootstrap jumps to the UEFI code in RAM
                     without changing anything. Only to be used with the
                     Foundation model. The Foundation model does not have
                     non-secure memory at address `0x0` and thus the UEFI image
                     should be pre-loaded into non-secure RAM at address
                     `0xA0000000`.


Use on ARMv8 RTSM and FVP models
--------------------------------

Add the '-a' option to the model start script and point to the required
bootstrap:

    < ... model start script as described in top-level readme file ... >
     -a <path/to/bootstrap-binary-file>

NOTE: The Foundation model bootstrap should not be used with these models.


Use on ARMv8 Foundation model
-----------------------------

The Foundation model takes an option for an ELF file to be loaded as well as an
option to load a binary data blob into RAM. This can be used to run UEFI in the
following manner:

    <PATH_TO_INSTALLED_FOUNDATION_MODEL>/Foundation_v8 --cores=2 --visualization
      --image=uefi-bootstrap-el3-foundation.axf --nsdata=RTSM_VE_FOUNDATIONV8_EFI.fd@0xA0000000

NOTE: The RTSM version of the bootstraps and UEFI image will not work as
      expected on the Foundation model. Foundation model specific versions
      should be used.
