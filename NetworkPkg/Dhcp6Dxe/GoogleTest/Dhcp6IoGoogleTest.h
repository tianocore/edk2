/** @file
  Acts as header for private functions under test in Dhcp6Io.c

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef DHCP6_IO_GOOGLE_TEST_H_
#define DHCP6_IO_GOOGLE_TEST_H_

////////////////////////////////////////////////////////////////////////////////
// These are the functions that are being unit tested
////////////////////////////////////////////////////////////////////////////////

#include <Uefi.h>

/**
  Seeks the Inner Options from a DHCP6 Option

  @param[in]  IaType          The type of the IA option.
  @param[in]  Option          The pointer to the DHCP6 Option.
  @param[in]  OptionLen       The length of the DHCP6 Option.
  @param[out] IaInnerOpt      The pointer to the IA inner option.
  @param[out] IaInnerLen      The length of the IA inner option.

  @retval EFI_SUCCESS         Seek the inner option successfully.
  @retval EFI_DEVICE_ERROR    The OptionLen is invalid.
*/
EFI_STATUS
Dhcp6SeekInnerOptionSafe (
  UINT16  IaType,
  UINT8   *Option,
  UINT32  OptionLen,
  UINT8   **IaInnerOpt,
  UINT16  *IaInnerLen
  );

/**
  Seek StatusCode Option in package. A Status Code option may appear in the
  options field of a DHCP message and/or in the options field of another option.
  See details in section 22.13, RFC3315.

  @param[in]       Instance        The pointer to the Dhcp6 instance.
  @param[in]       Packet          The pointer to reply messages.
  @param[out]      Option          The pointer to status code option.

  @retval EFI_SUCCESS              Seek status code option successfully.
  @retval EFI_DEVICE_ERROR         An unexpected error.

**/
EFI_STATUS
Dhcp6SeekStsOption (
  IN     DHCP6_INSTANCE    *Instance,
  IN     EFI_DHCP6_PACKET  *Packet,
  OUT    UINT8             **Option
  );

#endif // DHCP6_IO_GOOGLE_TEST_H
