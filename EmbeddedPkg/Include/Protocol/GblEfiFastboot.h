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

#ifndef GBL_EFI_FASTBOOT_PROTOCOL_H_
#define GBL_EFI_FASTBOOT_PROTOCOL_H_

#include <Uefi/UefiBaseType.h>

//
// {c67e48a0-5eb8-4127-be89-df2ed93d8a9a}
//
#define GBL_EFI_FASTBOOT_PROTOCOL_GUID \
  { 0xc67e48a0, 0x5eb8, 0x4127, { 0xbe, 0x89, 0xdf, 0x2e, 0xd9, 0x3d, 0x8a, 0x9a } }

#define GBL_EFI_FASTBOOT_PROTOCOL_REVISION  0x00000004

#define GBL_EFI_FASTBOOT_SERIAL_NUMBER_MAX_LEN_UTF8  32

typedef struct _GBL_EFI_FASTBOOT_PROTOCOL GBL_EFI_FASTBOOT_PROTOCOL;

typedef enum {
  GBL_EFI_FASTBOOT_MESSAGE_TYPE_OKAY,
  GBL_EFI_FASTBOOT_MESSAGE_TYPE_FAIL,
  GBL_EFI_FASTBOOT_MESSAGE_TYPE_INFO
} GBL_EFI_FASTBOOT_MESSAGE_TYPE;

typedef enum {
  GBL_EFI_FASTBOOT_ERASE_ACTION_ERASE_AS_PHYSICAL_PARTITION,
  GBL_EFI_FASTBOOT_ERASE_ACTION_NOOP
} GBL_EFI_FASTBOOT_ERASE_ACTION;

typedef enum {
  GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT_PROHIBITED,
  GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT_DEFAULT_IMPL,
  GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT_CUSTOM_IMPL
} GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT;

typedef
VOID
(EFIAPI *GBL_EFI_FASTBOOT_GET_VAR_ALL_CALLBACK)(
  IN VOID               *Context,
  IN CONST CHAR8 *CONST *Args,
  IN UINTN              NumArgs,
  IN CONST CHAR8        *Value
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_MESSAGE_SENDER)(
  IN VOID                          *Context,
  IN GBL_EFI_FASTBOOT_MESSAGE_TYPE MsgType,
  IN CONST CHAR8                   *Msg,
  IN UINTN                         MsgLen
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_GET_VAR)(
  IN GBL_EFI_FASTBOOT_PROTOCOL *This,
  IN CONST CHAR8 *CONST        *Args,
  IN UINTN                     NumArgs,
  OUT UINT8                    *Out,
  IN OUT UINTN                 *OutSize
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
  OUT UINT8                    *Out,
  IN OUT UINTN                 *OutSize,
  OUT UINTN                    *OutRemain
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_SET_LOCK)(
  IN GBL_EFI_FASTBOOT_PROTOCOL *This,
  IN BOOLEAN                   Critical,
  IN BOOLEAN                   Lock
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_GET_LOCK)(
  IN GBL_EFI_FASTBOOT_PROTOCOL *This,
  IN BOOLEAN                   Critical,
  OUT BOOLEAN                  *OutLock
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_VENDOR_ERASE)(
  IN GBL_EFI_FASTBOOT_PROTOCOL      *This,
  IN CONST CHAR8                    *PartName,
  IN UINTN                          PartNameLen,
  OUT GBL_EFI_FASTBOOT_ERASE_ACTION *Action
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_COMMAND_EXEC)(
  IN GBL_EFI_FASTBOOT_PROTOCOL             *This,
  IN UINTN                                 NumArgs,
  IN CONST CHAR8 *CONST                    *Args,
  IN UINTN                                 DownloadDataUsedLen,
  IN UINT8                                 *DownloadData,
  IN UINTN                                 DownloadDataFullSize,
  OUT GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT *Implementation,
  IN GBL_EFI_FASTBOOT_MESSAGE_SENDER       Sender,
  IN VOID                                  *Context
  );

struct _GBL_EFI_FASTBOOT_PROTOCOL {
  UINT64                           Revision;
  CHAR8                            SerialNumber[GBL_EFI_FASTBOOT_SERIAL_NUMBER_MAX_LEN_UTF8];
  GBL_EFI_FASTBOOT_GET_VAR         GetVar;
  GBL_EFI_FASTBOOT_GET_VAR_ALL     GetVarAll;
  GBL_EFI_FASTBOOT_GET_STAGED      GetStaged;
  GBL_EFI_FASTBOOT_SET_LOCK        SetLock;
  GBL_EFI_FASTBOOT_GET_LOCK        GetLock;
  GBL_EFI_FASTBOOT_VENDOR_ERASE    VendorErase;
  GBL_EFI_FASTBOOT_COMMAND_EXEC    CommandExec;
};

#endif // GBL_EFI_FASTBOOT_PROTOCOL_H_
