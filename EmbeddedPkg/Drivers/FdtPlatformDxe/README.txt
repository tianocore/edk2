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
Table by the "gFdtTableGuid" GUID defined in EmbeddedPkg.dec.

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
