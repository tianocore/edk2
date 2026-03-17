/** @file
  Provide vendor-specific version and identifiers to core TPM library
  for return in capabilities. These may not be compile time constants and
  therefore are provided by platform callbacks.
  These platform functions are expected to always be available,
  even in failure mode.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/BaseLib.h>
#include <Library/PlatformTpmLib.h>

/**
  _plat__GetManufacturerCapabilityCode()

  return the 4 character Manufacturer Capability code.
  This should come from the platform library since
  that is provided by the manufacturer.


  @return               Manufacturer capability Code.

**/
UINT32
EFIAPI
PlatformTpmLibGetManufacturerCapabilityCode (
  VOID
  )
{
  return 0x58595A20; // "XYZ "
}

/**
  _plat__GetVendorCapabilityCode()

  return the 4 character VendorStrings for Capabilities.
  Index is ONE-BASED, and may be in the range [1,4] inclusive.
  Any other index returns all zeros. The return value will be interpreted
  as an array of 4 ASCII characters (with no null terminator).

  @param[in] Index      index

  @return               Vendor specific capability code.

**/
UINT32
EFIAPI
PlatformTpmLibGetVendorCapabilityCode (
  IN INT32  Index
  )
{
  return 0;
}

/**
  _plat__GetTpmFirmwareVersionHigh()

  return the most-significant 32-bits of the TPM Firmware Version

  @return               High 32-bits of TPM Firmware Version.

**/
UINT32
EFIAPI
PlatformTpmLibGetTpmFirmwareVersionHigh (
  VOID
  )
{
  return 0;
}

/**
  _plat__GetTpmFirmwareVersionLow()

  return the least-significant 32-bits of the TPM Firmware Version

  @return               Low 32-bits of TPM Firmware Version.

**/
UINT32
EFIAPI
PlatformTpmLibGetTpmFirmwareVersionLow (
  VOID
  )
{
  return 0;
}

/**
  _plat__GetTpmFirmwareSvn()

  return the TPM Firmware current SVN.

  @return               current SVN.

**/
UINT16
EFIAPI
PlatformTpmLibGetTpmFirmwareSvn (
  VOID
  )
{
  return 0;
}

/**
  _plat__GetTpmFirmwareMaxSvn()

  return the TPM Firmware maximum SVN.

  @return               Maximum SVN.

**/
UINT16
EFIAPI
PlatformTpmLibGetTpmFirmwareMaxSvn (
  VOID
  )
{
  return 0;
}

/**
  _plat__GetTpmFirmwareSvnSecret().

  return the TPM Firmware SVN Secret value associated with SVN.

  @param[in]            Svn                     Svn.
  @param[in]            SecretBufferSize        Secret Buffer Size.
  @param[out]           SecretBuffer            Secret Buffer.
  @param[out]           SecretSize              Secret Svn Size.

  @return               0                       Success.
  @return               < 0                     Error.

**/
INT32
EFIAPI
PlatformTpmLibGetTpmFirmwareSvnSecret (
  IN  UINT16  Svn,
  IN  UINT16  SecretBufferSize,
  OUT UINT8   *SecretBuffer,
  OUT UINT16  *SecretSize
  )
{
  return -1;
}

/**
  _plat__GetTpmFirmwareSecret().

  return the TPM Firmware Secret value associated with SVN.

  @param[in]            SecretBufferSize        Secret Buffer Size.
  @param[out]           SecretBuffer            Secret Buffer.
  @param[out]           SecretSize              Secret Svn Size.

  @return               0                       Success.
  @return               < 0                     Error.

**/
INT32
EFIAPI
PlatformTpmLibGetTpmFirmwareSecret (
  IN  UINT16  SecretBufferSize,
  OUT UINT8   *SecretBuffer,
  OUT UINT16  *SecretSize
  )
{
  return -1;
}

/**
  _plat__GetVendorTpmType().

  return the Platform TPM type.

  @return               TPM type.

**/
UINT32
EFIAPI
PlatformTpmLibGetVendorTpmType (
  VOID
  )
{
  return PLATFORM_TPM_TYPE_UNKNOWN;
}
