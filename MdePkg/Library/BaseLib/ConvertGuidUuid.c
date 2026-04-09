/** @file
  Convert 128 bit unique identifier between GUID and UUID format.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "BaseLibInternals.h"

/*******************************************************************************

  UUID (Universally Unique IDentifier), as defined in RFC4122
  (https://datatracker.ietf.org/doc/html/rfc4122#section-4.1), is a 128-bit number
  used to uniquely identify information in computer systems.

  UUIDs contains 5 fields:
  - time_low: 32 bits
  - time_mid: 16 bits
  - time_hi_and_version: 16 bits
  - clock_seq_hi_and_reserved: 8 bits
  - clock_seq_low: 8 bits
  - node: 8 bits * 6

  Each field encoded with the Most Significant Byte first (known as network byte
  order, or big-endian).

  GUID (Globally Unique Identifier), on the other hand, is a 128-bit number used
  in UEFI environments, which is similar to UUID but has a different byte order
  in memory. See https://uefi.org/specs/UEFI/2.11/Apx_A_GUID_and_Time_Formats.html

  GUID also contains 5 fields:
  - TimeLow: 32 bits
  - TimeMid: 16 bits
  - TimeHiAndVersion: 16 bits
  - ClockSeqHighAndReserved: 16 bits
  - ClockSeqLow: 8 bits
  - Node: 8 bits * 6

  TimeLow, TimeMid, TimeHighAndVersion fields in the EFI are encoded with the Least
  Significant Byte first (also known as little-endian).

  Example:
  Consider the same string representation/registry format for MM communication v2:
  "378daedc-f06b-4446-8314-40ab933c87a3"

  In UUID format, it is represented as:
  - Data fields:
    - time_low: 0x37 0x8d 0xae 0xdc (0x378daedc in big-endian)
    - time_mid: 0xf0 0x6b (0xf06b in big-endian)
    - time_hi_and_version: 0x44 0x46 (0x4446 in big-endian)
    - clock_seq_hi_and_reserved: 0x83
    - clock_seq_low: 0x14
    - node: 0x00, 0xab, 0x93, 0x3c, 0x87, 0xa3
  - Byte representation in memory:
    - 37 8d ae dc f0 6b 44 46 83 14 40 ab 93 3c 87 a3

  However, in GUID format, it is represented as:
  - Data fields:
    - TimeLow: 0xdc 0xae 0x8d 0x37 (0x378daedc in little-endian)
    - TimeMid: 0x6b 0xf0 (0xf06b in little-endian)
    - TimeHiAndVersion: 0x46 0x44 (0x4446 in little-endian)
    - ClockSeqHighAndReserved: 0x83
    - ClockSeqLow: 0x14
    - Node: 0x00, 0xab, 0x93, 0x3c, 0x87, 0xa3
  - Byte representation in memory:
    - dc ae 8d 37 6b f0 46 44 83 14 40 ab 93 3c 87 a3

*******************************************************************************/

/**
  This function converts a GUID in UEFI format to a UUID in RFC4122 format.

  The conversion is done by swapping the byte order of the TimeLow, TimeMid, and
  TimeHiAndVersion fields, while keeping the ClockSeq and Node fields unchanged.

  @param [in] FromGuid  GUID in format to be converted to UUID RFC4122 format.
  @param [out] ToUuid   Pointer to a GUID structure that will hold the converted
                        UUID in RFC4122 format.
**/
VOID
EFIAPI
ConvertGuidToUuid (
  IN   GUID  *FromGuid,
  OUT  GUID  *ToUuid
  )
{
  ASSERT (ToUuid != NULL);
  ASSERT (FromGuid != NULL);

  CopyGuid (ToUuid, FromGuid);
  ToUuid->Data1 = SwapBytes32 (ToUuid->Data1);
  ToUuid->Data2 = SwapBytes16 (ToUuid->Data2);
  ToUuid->Data3 = SwapBytes16 (ToUuid->Data3);
}

/**
  This function converts a UUID in RFC4122 format to a GUID in UEFI format.

  The conversion is done by swapping the byte order of the time_low, time_mid, and
  time_hi_and_version fields, while keeping the ClockSeq and Node fields unchanged.
  This function is symmetric to ConvertGuidToUuid.

  @param [in] FromUuid  UUID in RFC4122 format to be converted to GUID in UEFI format.
  @param [out] ToGuid   Pointer to a GUID structure that will hold the converted
                        GUID in UEFI format.
**/
VOID
EFIAPI
ConvertUuidToGuid (
  IN   GUID  *FromUuid,
  OUT  GUID  *ToGuid
  )
{
  // The conversion is symmetric, so we can use the same function.
  // The only difference is the order of the parameters.
  ConvertGuidToUuid (
    FromUuid,
    ToGuid
    );
}
