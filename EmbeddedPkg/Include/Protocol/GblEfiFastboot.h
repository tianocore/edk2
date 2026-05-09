/** @file

  Copyright (c) 2025, The Android Open Source Project.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/*
  GBL EFI Fastboot Protocol.
  Platform-specific helpers for Android Fastboot operations.

  Related docs:
  https://cs.android.com/android/kernel/superproject/+/common-android-mainline:bootable/libbootloader/gbl/docs/gbl_efi_fastboot_protocol.md
*/

#ifndef __GBL_EFI_FASTBOOT_PROTOCOL_H__
#define __GBL_EFI_FASTBOOT_PROTOCOL_H__

#include <Uefi/UefiBaseType.h>

//
// {c67e48a0-5eb8-4127-be89-df2ed93d8a9a}
//
#define GBL_EFI_FASTBOOT_PROTOCOL_GUID \
  { 0xc67e48a0, 0x5eb8, 0x4127, { 0xbe, 0x89, 0xdf, 0x2e, 0xd9, 0x3d, 0x8a, 0x9a } }

// WARNING: The current API is unstable. While backward compatibility is
// guaranteed for pre-release revisions after 0x00000100, the official
// `1.0` (0x00010000) release may include final breaking changes.
#define GBL_EFI_FASTBOOT_PROTOCOL_REVISION  0x00000100

#define GBL_EFI_FASTBOOT_SERIAL_NUMBER_MAX_LEN_UTF8  32
#define GBL_EFI_FASTBOOT_PARTITION_TYPE_BUF_LEN      56

typedef struct _GBL_EFI_FASTBOOT_PROTOCOL GBL_EFI_FASTBOOT_PROTOCOL;

typedef enum {
  GBL_EFI_FASTBOOT_MESSAGE_TYPE_OKAY,
  GBL_EFI_FASTBOOT_MESSAGE_TYPE_FAIL,
  GBL_EFI_FASTBOOT_MESSAGE_TYPE_INFO
} GBL_EFI_FASTBOOT_MESSAGE_TYPE;

typedef enum {
  GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT_PROHIBITED,
  GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT_DEFAULT_IMPL,
  GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT_CUSTOM_IMPL
} GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT;

typedef
VOID
(EFIAPI *GBL_EFI_FASTBOOT_GET_VAR_ALL_CALLBACK)(
  IN VOID               *Context,
  IN UINTN              NumArgs,
  IN CONST CHAR8 *CONST *Args,
  IN CONST CHAR8        *Value
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_MESSAGE_SENDER)(
  IN VOID                          *Context,
  IN UINT32                        MsgType, // GBL_EFI_FASTBOOT_MESSAGE_TYPE
  IN UINTN                         MsgLen,
  IN CONST CHAR8                   *Msg
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_GET_VAR)(
  IN GBL_EFI_FASTBOOT_PROTOCOL *This,
  IN UINTN                     NumArgs,
  IN CONST CHAR8 *CONST        *Args,
  IN OUT UINTN                 *BufferSize,
  OUT CHAR8                    *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_GET_VAR_ALL)(
  IN GBL_EFI_FASTBOOT_PROTOCOL             *This,
  IN VOID                                  *Context,
  IN GBL_EFI_FASTBOOT_GET_VAR_ALL_CALLBACK Callback
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_GET_STAGED)(
  IN GBL_EFI_FASTBOOT_PROTOCOL *This,
  IN OUT UINTN                 *BufferSize,
  OUT UINTN                    *BufferRemains,
  OUT UINT8                    *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_COMMAND_EXEC)(
  IN GBL_EFI_FASTBOOT_PROTOCOL *This,
  IN UINTN                     NumArgs,
  IN CONST CHAR8 *CONST        *Args,
  IN UINTN                     DownloadBufferSize,
  IN UINTN                     DownloadBufferUsedSize,
  IN UINT8                     *DownloadBuffer,
  OUT UINT32                   *Implementation, // GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT
  IN GBL_EFI_FASTBOOT_MESSAGE_SENDER Sender,
  IN VOID                      *Context
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_GET_PARTITION_TYPE)(
  IN GBL_EFI_FASTBOOT_PROTOCOL *This,
  IN CONST CHAR8               *PartName,
  IN OUT UINTN                 *PartTypeLen,
  OUT CHAR8                    *PartType
  );

struct _GBL_EFI_FASTBOOT_PROTOCOL {
  UINT64                                 Revision;
  CHAR8                                  SerialNumber[GBL_EFI_FASTBOOT_SERIAL_NUMBER_MAX_LEN_UTF8];
  GBL_EFI_FASTBOOT_GET_VAR               GetVar;
  GBL_EFI_FASTBOOT_GET_VAR_ALL           GetVarAll;
  GBL_EFI_FASTBOOT_GET_STAGED            GetStaged;
  GBL_EFI_FASTBOOT_COMMAND_EXEC          CommandExec;
  GBL_EFI_FASTBOOT_GET_PARTITION_TYPE    GetPartitionType;
};

extern EFI_GUID  gGblEfiFastbootProtocolGuid;

#endif // __GBL_EFI_FASTBOOT_PROTOCOL_H__
