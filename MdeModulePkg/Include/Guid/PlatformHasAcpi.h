/** @file
  EDKII Platform Has ACPI GUID

  A NULL protocol instance with this GUID in the DXE protocol database, and/or
  a NULL PPI with this GUID in the PPI database, implies that the platform
  provides the operating system with an ACPI-based hardware description. Note
  that this is not necessarily exclusive with different kinds of hardware
  description (for example, a Device Tree-based one). A platform driver and/or
  PEIM is supposed to produce a single instance of the protocol and/or PPI
  (with NULL contents), if appropriate.

  Copyright (C) 2017, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License that accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/


#ifndef __EDKII_PLATFORM_HAS_ACPI_H__
#define __EDKII_PLATFORM_HAS_ACPI_H__

#define EDKII_PLATFORM_HAS_ACPI_GUID \
  { \
    0xf0966b41, 0xc23f, 0x41b9, \
    { 0x96, 0x04, 0x0f, 0xf7, 0xe1, 0x11, 0x96, 0x5a } \
  }

extern EFI_GUID gEdkiiPlatformHasAcpiGuid;

#endif
