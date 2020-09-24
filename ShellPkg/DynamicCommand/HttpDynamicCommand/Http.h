/** @file
  Header file for 'http' command functions.

  Copyright (c) 2010 - 2017, Intel Corporation. All rights reserved. <BR>
  Copyright (c) 2015, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2020, Broadcom. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _HTTP_H_
#define _HTTP_H_

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/HttpLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NetLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/HiiPackageList.h>
#include <Protocol/HttpUtilities.h>
#include <Protocol/ServiceBinding.h>

#define HTTP_APP_NAME L"http"

#define REQ_OK           0
#define REQ_NEED_REPEAT  1

//
// Download Flags.
//
#define DL_FLAG_TIME     BIT0 // Show elapsed time.
#define DL_FLAG_KEEP_BAD BIT1 // Keep files even if download failed.

extern EFI_HII_HANDLE mHttpHiiHandle;

typedef struct {
  UINTN                 ContentDownloaded;
  UINTN                 ContentLength;
  UINTN                 LastReportedNbOfBytes;
  UINTN                 BufferSize;
  UINTN                 Status;
  UINTN                 Flags;
  UINT8                 *Buffer;
  CHAR16                *ServerAddrAndProto;
  CHAR16                *Uri;
  EFI_HTTP_TOKEN        ResponseToken;
  EFI_HTTP_TOKEN        RequestToken;
  EFI_HTTP_PROTOCOL     *Http;
  EFI_HTTP_CONFIG_DATA  HttpConfigData;
} HTTP_DOWNLOAD_CONTEXT;

/**
  Function for 'http' command.

  @param[in]  ImageHandle     The image handle.
  @param[in]  SystemTable     The system table.

  @retval SHELL_SUCCESS            Command completed successfully.
  @retval SHELL_INVALID_PARAMETER  Command usage error.
  @retval SHELL_ABORTED            The user aborts the operation.
  @retval value                    Unknown error.
**/
SHELL_STATUS
RunHttp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Retrieve HII package list from ImageHandle and publish to HII database.

  @param[in] ImageHandle            The image handle of the process.

  @retval HII handle.
**/
EFI_HII_HANDLE
InitializeHiiPackage (
  IN EFI_HANDLE                  ImageHandle
  );
#endif // _HTTP_H_
