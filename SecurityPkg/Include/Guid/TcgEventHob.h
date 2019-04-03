/** @file
  Defines the HOB GUID used to pass a TCG_PCR_EVENT or TCG_PCR_EVENT2 from a TPM PEIM to
  a TPM DXE Driver. A GUIDed HOB is generated for each measurement
  made in the PEI Phase.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TCG_EVENT_HOB_H_
#define _TCG_EVENT_HOB_H_

///
/// The Global ID of a GUIDed HOB used to pass a TCG_PCR_EVENT from a TPM PEIM to a TPM DXE Driver.
///
#define EFI_TCG_EVENT_HOB_GUID \
  { \
    0x2b9ffb52, 0x1b13, 0x416f, { 0xa8, 0x7b, 0xbc, 0x93, 0xd, 0xef, 0x92, 0xa8 } \
  }

extern EFI_GUID gTcgEventEntryHobGuid;

#define EFI_TCG_EVENT2_HOB_GUID \
  { \
    0xd26c221e, 0x2430, 0x4c8a, { 0x91, 0x70, 0x3f, 0xcb, 0x45, 0x0, 0x41, 0x3f } \
  }

extern EFI_GUID gTcgEvent2EntryHobGuid;

///
/// The Global ID of a GUIDed HOB used to record TPM device error.
///
#define EFI_TPM_ERROR_GUID \
  { \
    0xef598499, 0xb25e, 0x473a, { 0xbf, 0xaf, 0xe7, 0xe5, 0x7d, 0xce, 0x82, 0xc4 } \
  }

extern EFI_GUID gTpmErrorHobGuid;

///
/// The Global ID of a GUIDed HOB used to record TPM2 Startup Locality.
/// HOB payload is UINT8 according to Startup Locality Event.
///
#define EFI_TPM2_STARTUP_LOCALITY_HOB_GUID \
  { \
    0xef598499, 0xb25e, 0x473a, { 0xbf, 0xaf, 0xe7, 0xe5, 0x7d, 0xce, 0x82, 0xc4 } \
  }

extern EFI_GUID gTpm2StartupLocalityHobGuid;

#endif
