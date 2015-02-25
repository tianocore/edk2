/** @file

  Copyright (c) 2015, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

The purpose of the FdtPlatformDxe UEFI driver is to install the Flat Device
Tree (FDT) of the platform the UEFI frimware is running on into the UEFI
Configuration Table. The FDT is identified within the UEFI Configuration
Table by the "gFdtTableGuid" GUID defined in "EmbeddedPkg.dec".

Once installed, an UEFI application or OS boot loader can get from the UEFI
Configuration Table the FDT of the platform from the "gFdtTableGuid" GUID.

The installation is done after each boot at the end of the DXE phase,
just before the BDS phase. It is done at the end of the DXE phase to be sure
that all drivers have been dispatched. That way, all UEFI protocols that may
be needed to retrieve the FDT can be made available. It is done before the BDS
phase to be able to provide the FDT during that phase.

The present driver tries to retrieve the FDT from the device paths defined in the
"gEmbeddedTokenSpaceGuid.PcdFdtDevicePaths" PCD. The "PcdFdtDevicePaths" PCD
contains a list a device paths. The device paths are in the text form and
separated by semi-colons. The present driver tries the device paths in the order
it finds them in the "PcdFdtDevicePaths" PCD as long as he did not install
succesfully a FDT.

The "PcdFdtDevicePaths" PCD is a dynamic PCD that can be modified during the
DXE phase. This allows for exemple to select the right FDT when a binary is
intended to run on several platforms and/or variants of a platform.

If the driver manages to download a FDT from one of the device paths mentioned
above then it installs it in the UEFI Configuration table and the run over the
device paths is stopped.

For development purposes only, if the feature PCD "gEmbeddedTokenSpaceGuid.
PcdOverridePlatformFdt" is equal to TRUE, then before to try to install the
FDT from the device paths listed in the "PcdFdtDevicePaths" PCD, the present
driver tries to install it using the device path defined by the UEFI variable
"Fdt". If the variable does not exist or the installation using the device path
defined by the UEFI variable fails then the installation proceeds as described
above.

Furthermore and again for development purposes only, if the feature PCD
"PcdOverridePlatformFdt" is equal to TRUE, the current driver provides the EFI
Shell command "setfdt" to define the location of the FDT by the mean of an EFI
Shell file path (like "fs2:\boot\fdt.dtb") or a device path.

If the path passed in to the command is a valid EFI Shell file path, the
command translates it into the corresponding device path and stores that
device path in the "Fdt" UEFI variable asking for the variable to be non
volatile.

If the path passed in to the command is not recognised as a valid EFI
Shell device path, the command handles it as device path and stored
in the "Fdt" UEFI variable as it is.

Finally, the "-i" option of the "setfdt" command allows to trigger the FDT
installation process. The installation process is completed when the command
returns. The command can be invoked with the "-i" option only and in that
case the "Fdt" UEFI variable is not updated and the command just runs the
FDT installation process. If the command is invoked with the "-i" option and
an EFI Shell file path then first the "Fdt" UEFI variable is updated accordingly
and then the FDT installation process is run.
