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

#define GBL_EFI_FASTBOOT_PROTOCOL_REVISION  0x00000001

#define GBL_EFI_FASTBOOT_SERIAL_NUMBER_MAX_LEN_UTF8  32

typedef struct _GBL_EFI_FASTBOOT_PROTOCOL GBL_EFI_FASTBOOT_PROTOCOL;

typedef struct {
  BOOLEAN    CanUnlock;
  BOOLEAN    HasCriticalLock;
  BOOLEAN    CanRamBoot;
} GBL_EFI_FASTBOOT_POLICY;

typedef enum {
  GBL_EFI_FASTBOOT_PARTITION_READ  = 1u << 0,
  GBL_EFI_FASTBOOT_PARTITION_WRITE = 1u << 1,
  GBL_EFI_FASTBOOT_PARTITION_ERASE = 1u << 2,
} GBL_EFI_FASTBOOT_PARTITION_PERMISSION_FLAGS;

typedef enum {
  GBL_EFI_FASTBOOT_LOCK_FLAGS_LOCKED          = 1u << 0,
  GBL_EFI_FASTBOOT_LOCK_FLAGS_CRITICAL_LOCKED = 1u << 1,
} GBL_EFI_FASTBOOT_LOCK_FLAGS;

typedef enum {
  GBL_EFI_FASTBOOT_MESSAGE_TYPE_OKAY = 0,
  GBL_EFI_FASTBOOT_MESSAGE_TYPE_FAIL,
  GBL_EFI_FASTBOOT_MESSAGE_TYPE_INFO,
} GBL_EFI_FASTBOOT_MESSAGE_TYPE;

typedef enum {
  GBL_EFI_FASTBOOT_ERASE_ACTION_ERASE_AS_PHYSICAL_PARTITION = 0,
  GBL_EFI_FASTBOOT_ERASE_ACTION_NOOP,
} GBL_EFI_FASTBOOT_ERASE_ACTION;

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
  OUT UINT8                    *Buf,
  IN OUT UINTN                 *BufSize
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
(EFIAPI *GBL_EFI_FASTBOOT_RUN_OEM_FUNCTION)(
  IN GBL_EFI_FASTBOOT_PROTOCOL       *This,
  IN CONST CHAR8                     *Command,
  IN UINTN                           CommandLen,
  OUT UINT8                          *DownloadBuffer,
  IN UINTN                           DownloadDataSize,
  IN GBL_EFI_FASTBOOT_MESSAGE_SENDER Sender,
  IN VOID                            *Context,
  OUT CHAR8                          *Buf,
  IN OUT UINTN                       *BufSize
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_GET_POLICY)(
  IN GBL_EFI_FASTBOOT_PROTOCOL *This,
  OUT GBL_EFI_FASTBOOT_POLICY  *Policy
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_SET_LOCK)(
  IN GBL_EFI_FASTBOOT_PROTOCOL  *This,
  IN UINT64                     LockState
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_CLEAR_LOCK)(
  IN GBL_EFI_FASTBOOT_PROTOCOL *This,
  IN UINT64                    LockState
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_GET_LOCK)(
  IN GBL_EFI_FASTBOOT_PROTOCOL *This,
  IN BOOLEAN                   Critical,
  OUT BOOLEAN                  *IsLocked
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_START_LOCAL_SESSION)(
  IN GBL_EFI_FASTBOOT_PROTOCOL  *This,
  OUT VOID                      **SessionCtx
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_UPDATE_LOCAL_SESSION)(
  IN GBL_EFI_FASTBOOT_PROTOCOL *This,
  IN VOID                      *SessionCtx,
  IN OUT UINT8                 *Buf,
  IN OUT UINTN                 *BufSize
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_CLOSE_LOCAL_SESSION)(
  IN GBL_EFI_FASTBOOT_PROTOCOL *This,
  IN VOID                      *SessionCtx
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_GET_PARTITION_PERMISSIONS)(
  IN GBL_EFI_FASTBOOT_PROTOCOL *This,
  IN CONST CHAR8               *PartName,
  IN UINTN                     PartNameLen,
  OUT UINT64                   *Permissions // GBL_EFI_FASTBOOT_PARTITION_PERMISSION_FLAGS
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
(EFIAPI *GBL_EFI_FASTBOOT_IS_COMMAND_ALLOWED)(
  IN GBL_EFI_FASTBOOT_PROTOCOL *This,
  IN UINTN                     NumArgs,
  IN CONST CHAR8 *CONST        *Args,
  IN UINTN                     DownloadDataLen,
  IN UINT8                     *DownloadData,
  OUT BOOLEAN                  *Allowed,
  IN UINTN                     MsgBufSize,
  OUT UINT8                    *MsgBuf
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_WIPE_USER_DATA)(
  IN GBL_EFI_FASTBOOT_PROTOCOL *This
  );

typedef
BOOLEAN
(EFIAPI *GBL_EFI_FASTBOOT_SHOULD_STOP_IN_FASTBOOT)(
  IN GBL_EFI_FASTBOOT_PROTOCOL *This
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_GET_STAGED)(
  IN GBL_EFI_FASTBOOT_PROTOCOL *This,
  OUT UINT8                    *Buf,
  IN OUT UINTN                 *BufSize,
  OUT UINTN                    *Remaining
  );

struct _GBL_EFI_FASTBOOT_PROTOCOL {
  UINT32                                        Revision;
  CHAR8                                         SerialNumber[GBL_EFI_FASTBOOT_SERIAL_NUMBER_MAX_LEN_UTF8];

  GBL_EFI_FASTBOOT_GET_VAR                      GetVar;
  GBL_EFI_FASTBOOT_GET_VAR_ALL                  GetVarAll;
  GBL_EFI_FASTBOOT_RUN_OEM_FUNCTION             RunOemFunction;

  GBL_EFI_FASTBOOT_GET_POLICY                   GetPolicy;
  GBL_EFI_FASTBOOT_SET_LOCK                     SetLock;
  GBL_EFI_FASTBOOT_CLEAR_LOCK                   ClearLock;
  GBL_EFI_FASTBOOT_GET_LOCK                     GetLock;

  GBL_EFI_FASTBOOT_START_LOCAL_SESSION          StartLocalSession;
  GBL_EFI_FASTBOOT_UPDATE_LOCAL_SESSION         UpdateLocalSession;
  GBL_EFI_FASTBOOT_CLOSE_LOCAL_SESSION          CloseLocalSession;

  GBL_EFI_FASTBOOT_GET_PARTITION_PERMISSIONS    GetPartitionPermissions;
  GBL_EFI_FASTBOOT_VENDOR_ERASE                 VendorErase;
  GBL_EFI_FASTBOOT_IS_COMMAND_ALLOWED           IsCommandAllowed;

  GBL_EFI_FASTBOOT_WIPE_USER_DATA               WipeUserData;
  GBL_EFI_FASTBOOT_SHOULD_STOP_IN_FASTBOOT      ShouldStopInFastboot;
  GBL_EFI_FASTBOOT_GET_STAGED                   GetStaged;
};

#endif // GBL_EFI_FASTBOOT_PROTOCOL_H_
