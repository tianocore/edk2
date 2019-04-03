/** @file
  Intel Firmware Version Info (FVI) related definitions.

  @todo update document/spec reference

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

@par Specification Reference:
  System Management BIOS (SMBIOS) Reference Specification v3.0.0 dated 2015-Feb-12
  http://www.dmtf.org/sites/default/files/standards/documents/DSP0134_3.0.0.pdf

**/

#ifndef __FIRMWARE_VERSION_INFO_H__
#define __FIRMWARE_VERSION_INFO_H__

#include <IndustryStandard/SmBios.h>

#define INTEL_FIRMWARE_VERSION_INFO_GROUP_NAME    "Firmware Version Info"

#pragma pack(1)

///
/// Firmware Version Structure
///
typedef struct {
  UINT8                       MajorVersion;
  UINT8                       MinorVersion;
  UINT8                       Revision;
  UINT16                      BuildNumber;
} INTEL_FIRMWARE_VERSION;

///
/// Firmware Version Info (FVI) Structure
///
typedef struct {
  SMBIOS_TABLE_STRING         ComponentName;  ///< String Index of Component Name
  SMBIOS_TABLE_STRING         VersionString;  ///< String Index of Version String
  INTEL_FIRMWARE_VERSION      Version;        ///< Firmware version
} INTEL_FIRMWARE_VERSION_INFO;

///
/// SMBIOS OEM Type Intel Firmware Version Info (FVI) Structure
///
typedef struct {
  SMBIOS_STRUCTURE            Header;         ///< SMBIOS structure header
  UINT8                       Count;          ///< Number of FVI entries in this structure
  INTEL_FIRMWARE_VERSION_INFO Fvi[1];         ///< FVI structure(s)
} SMBIOS_TABLE_TYPE_OEM_INTEL_FVI;

#pragma pack()

#endif
