/** @file
  Legal information

**/

#ifndef __CONFORMANCE_PROFILES_TABLE_GUID_H__
#define __CONFORMANCE_PROFILES_TABLE_GUID_H__


//
// This table allows the platform to advertise its UEFI specification conformance
// in the form of pre-defined profiles. Each profile is identified by a GUID, with
// known profiles listed in the section below.
// The absence of this table shall indicate that the platform implementation is
// conformant with the UEFI specification requirements, as defined in Section 2.6.
// This is equivalent to publishing this configuration table with the
// EFI_CONFORMANCE_PROFILES_UEFI_SPEC_GUID conformance profile.
//
#define EFI_CONFORMANCE_PROFILES_TABLE_GUID \
  { \
    0x36122546, 0xf7e7, 0x4c8f, { 0xbd, 0x9b, 0xeb, 0x85, 0x25, 0xb5, 0x0c, 0x0b } \
  }

#pragma pack(1)

typedef struct {
  ///
  /// Version of the table must be 0x1
  ///
  UINT16 Version;
  ///
  /// The number of profiles GUIDs present in ConformanceProfiles
  ///
  UINT16 NumberOfProfiles;
  ///
  /// An array of conformance profile GUIDs that are supported by this system.
  /// EFI_GUID        ConformanceProfiles[];
  ///
} EFI_CONFORMANCE_PROFILES_TABLE;

#define EFI_CONFORMANCE_PROFILES_TABLE_VERSION 0x1

//
// GUID defined in spec.
//
#define EFI_CONFORMANCE_PROFILES_UEFI_SPEC_GUID \
    { 0x523c91af, 0xa195, 0x4382, \
    { 0x81, 0x8d, 0x29, 0x5f, 0xe4, 0x00, 0x64, 0x65 }}

extern EFI_GUID  gEfiConfProfilesTableGuid;
extern EFI_GUID  gEfiConfProfilesUefiSpecGuid;

#endif
