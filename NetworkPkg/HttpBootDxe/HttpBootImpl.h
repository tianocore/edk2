/** @file
  The declaration of UEFI HTTP boot function.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_HTTP_BOOT_IMPL_H__
#define __EFI_HTTP_BOOT_IMPL_H__

#define HTTP_BOOT_CHECK_MEDIA_WAITING_TIME  EFI_TIMER_PERIOD_SECONDS(20)

typedef enum {
  GetBootFileHead,
  GetBootFileGet,
  LoadBootFile,
  GetBootFileError
} HTTP_GET_BOOT_FILE_STATE;

/**
  Attempt to complete a DHCPv4 D.O.R.A or DHCPv6 S.R.A.A sequence to retrieve the boot resource information.

  @param[in]    Private            The pointer to the driver's private data.

  @retval EFI_SUCCESS              Boot info was successfully retrieved.
  @retval EFI_INVALID_PARAMETER    Private is NULL.
  @retval EFI_NOT_STARTED          The driver is in stopped state.
  @retval EFI_DEVICE_ERROR         An unexpected network error occurred.
  @retval Others                   Other errors as indicated.

**/
EFI_STATUS
HttpBootDhcp (
  IN HTTP_BOOT_PRIVATE_DATA  *Private
  );

/**
  Disable the use of UEFI HTTP boot function.

  @param[in]    Private            The pointer to the driver's private data.

  @retval EFI_SUCCESS              HTTP boot was successfully disabled.
  @retval EFI_NOT_STARTED          The driver is already in stopped state.
  @retval EFI_INVALID_PARAMETER    Private is NULL.
  @retval Others                   Unexpected error when stop the function.

**/
EFI_STATUS
HttpBootStop (
  IN HTTP_BOOT_PRIVATE_DATA  *Private
  );

extern EFI_HTTP_BOOT_CALLBACK_PROTOCOL  gHttpBootDxeHttpBootCallback;

#endif
