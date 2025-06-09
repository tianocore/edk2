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

#define GBL_EFI_FASTBOOT_PROTOCOL_REVISION           0x00000000
#define GBL_EFI_FASTBOOT_SERIAL_NUMBER_MAX_LEN_UTF8  32

typedef struct _GBL_EFI_FASTBOOT_PROTOCOL  GBL_EFI_FASTBOOT_PROTOCOL;
typedef struct _GBL_EFI_FASTBOOT_POLICY    GBL_EFI_FASTBOOT_POLICY;

/*
  Device-policy capabilities and states.
*/
struct _GBL_EFI_FASTBOOT_POLICY {
  BOOLEAN    CanUnlock;
  BOOLEAN    HasCriticalLock;
  BOOLEAN    CanRamBoot;
};

/*
  Partition-permission flags.
*/
typedef enum {
  GBL_EFI_FASTBOOT_PARTITION_READ  = 1u << 0,
  GBL_EFI_FASTBOOT_PARTITION_WRITE = 1u << 1,
  GBL_EFI_FASTBOOT_PARTITION_ERASE = 1u << 2,
} GBL_EFI_FASTBOOT_PARTITION_PERMISSION_FLAGS;

/*
  Device-lock flags.
*/
typedef enum {
  GBL_EFI_FASTBOOT_LOCKED          = 1u << 0,
  GBL_EFI_FASTBOOT_CRITICAL_LOCKED = 1u << 1,
} GBL_EFI_FASTBOOT_LOCK_FLAGS;

/* Callback for GetVarAll */
typedef
VOID
(EFIAPI *GBL_EFI_FASTBOOT_GET_VAR_ALL_CALLBACK)(
  IN VOID                    *Context,
  IN CONST CHAR8 *CONST      *Args,
  IN UINTN                    NumArgs,
  IN CONST CHAR8             *Value
  );

/* Method typedefs */
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_GET_VAR)(
  IN  GBL_EFI_FASTBOOT_PROTOCOL  *This,
  IN  CONST CHAR8 *CONST         *Args,
  IN  UINTN                       NumArgs,
  OUT UINT8                      *Buf,
  IN  OUT UINTN                  *BufSize
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_GET_VAR_ALL)(
  IN GBL_EFI_FASTBOOT_PROTOCOL            *This,
  IN VOID                                 *Context,
  IN GBL_EFI_FASTBOOT_GET_VAR_ALL_CALLBACK Callback
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_RUN_OEM_FUNCTION)(
  IN  GBL_EFI_FASTBOOT_PROTOCOL  *This,
  IN  CONST CHAR8                *Command,
  IN  UINTN                       CommandLen,
  OUT CHAR8                      *Buf,
  IN  OUT UINTN                  *BufSize
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_GET_POLICY)(
  IN  GBL_EFI_FASTBOOT_PROTOCOL  *This,
  OUT GBL_EFI_FASTBOOT_POLICY    *Policy
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_SET_LOCK)(
  IN GBL_EFI_FASTBOOT_PROTOCOL  *This,
  IN UINT64                      LockState
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_CLEAR_LOCK)(
  IN GBL_EFI_FASTBOOT_PROTOCOL  *This,
  IN UINT64                      LockState
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_START_LOCAL_SESSION)(
  IN  GBL_EFI_FASTBOOT_PROTOCOL  *This,
  OUT VOID                      **SessionCtx
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_UPDATE_LOCAL_SESSION)(
  IN  GBL_EFI_FASTBOOT_PROTOCOL  *This,
  IN  VOID                       *SessionCtx,
  IN  OUT UINT8                  *Buf,
  IN  OUT UINTN                  *BufSize
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_CLOSE_LOCAL_SESSION)(
  IN GBL_EFI_FASTBOOT_PROTOCOL  *This,
  IN VOID                       *SessionCtx
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_GET_PARTITION_PERMISSIONS)(
  IN  GBL_EFI_FASTBOOT_PROTOCOL  *This,
  IN  CONST CHAR8                *PartName,
  IN  UINTN                       PartNameLen,
  OUT UINT64                     *Permissions   /* GBL_EFI_FASTBOOT_PARTITION_PERMISSION_FLAGS */
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_WIPE_USER_DATA)(
  IN GBL_EFI_FASTBOOT_PROTOCOL  *This
  );

typedef
BOOLEAN
(EFIAPI *GBL_EFI_FASTBOOT_SHOULD_STOP_IN_FASTBOOT)(
  IN GBL_EFI_FASTBOOT_PROTOCOL  *This
  );

/*
  Firmware-published protocol instance.
*/
struct _GBL_EFI_FASTBOOT_PROTOCOL {
  UINT32                                        Revision;
  CHAR8                                         SerialNumber[GBL_EFI_FASTBOOT_SERIAL_NUMBER_MAX_LEN_UTF8];

  GBL_EFI_FASTBOOT_GET_VAR                      GetVar;
  GBL_EFI_FASTBOOT_GET_VAR_ALL                  GetVarAll;
  GBL_EFI_FASTBOOT_RUN_OEM_FUNCTION             RunOemFunction;

  GBL_EFI_FASTBOOT_GET_POLICY                   GetPolicy;
  GBL_EFI_FASTBOOT_SET_LOCK                     SetLock;
  GBL_EFI_FASTBOOT_CLEAR_LOCK                   ClearLock;

  GBL_EFI_FASTBOOT_START_LOCAL_SESSION          StartLocalSession;
  GBL_EFI_FASTBOOT_UPDATE_LOCAL_SESSION         UpdateLocalSession;
  GBL_EFI_FASTBOOT_CLOSE_LOCAL_SESSION          CloseLocalSession;

  GBL_EFI_FASTBOOT_GET_PARTITION_PERMISSIONS    GetPartitionPermissions;
  GBL_EFI_FASTBOOT_WIPE_USER_DATA               WipeUserData;
  GBL_EFI_FASTBOOT_SHOULD_STOP_IN_FASTBOOT      ShouldStopInFastboot;
};

#endif // GBL_EFI_FASTBOOT_PROTOCOL_H_
