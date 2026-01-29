/** @file
  The Esrt Management Protocol used to register/set/update an updatable firmware resource .

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ESRT_MANAGEMENT_H_
#define _ESRT_MANAGEMENT_H_

#include <Guid/SystemResourceTable.h>

///
/// Global ID for the ESRT_MANAGEMENT_PROTOCOL.
///
#define ESRT_MANAGEMENT_PROTOCOL_GUID \
  { \
    0xa340c064, 0x723c, 0x4a9c, { 0xa4, 0xdd, 0xd5, 0xb4, 0x7a, 0x26, 0xfb, 0xb0 } \
  }

///
/// Forward declaration for the _ESRT_MANAGEMENT_PROTOCOL.
///
typedef struct _ESRT_MANAGEMENT_PROTOCOL ESRT_MANAGEMENT_PROTOCOL;

/**
  Get Variable name and data by Esrt Entry FwClass

  @param[in]       FwClass                FwClass of Esrt entry to get
  @param[in out]  Entry                   Esrt entry returned

  @retval EFI_SUCCESS                  The variable saving this Esrt Entry exists.
  @retval EF_NOT_FOUND                   No correct variable found.

**/
typedef
EFI_STATUS
(EFIAPI *GET_ESRT_ENTRY)(
  IN     EFI_GUID                  *FwClass,
  IN OUT EFI_SYSTEM_RESOURCE_ENTRY *Entry
  );

/**
  Update one ESRT entry in ESRT Cache.

  @param[in]  Entry                         Esrt entry to be updated

  @retval EFI_SUCCESS                   Successfully update an ESRT entry in cache.
  @retval EFI_INVALID_PARAMETER  Entry does't exist in ESRT Cache
  @retval EFI_WRITE_PROTECTED     ESRT Cache repositoy is locked

**/
typedef
EFI_STATUS
(EFIAPI *UPDATE_ESRT_ENTRY)(
  IN EFI_SYSTEM_RESOURCE_ENTRY *Entry
  );

/**
  Non-FMP instance to unregister Esrt Entry from ESRT Cache.

  @param[in]    FwClass                FwClass of Esrt entry to Unregister

  @retval EFI_SUCCESS         Insert all entries Successfully
  @retval EFI_NOT_FOUND     FwClass does not exsit

**/
typedef
EFI_STATUS
(EFIAPI *UNREGISTER_ESRT_ENTRY)(
  IN  EFI_GUID        *FwClass
  );

/**
  Non-FMP instance to register one ESRT entry into ESRT Cache.

  @param[in]  Entry                Esrt entry to be set

  @retval EFI_SUCCESS                   Successfully set a variable.
  @retval EFI_INVALID_PARAMETER  ESRT Entry is already exist
  @retval EFI_OUT_OF_RESOURCES  Non-FMP ESRT repository is full

**/
typedef
EFI_STATUS
(EFIAPI *REGISTER_ESRT_ENTRY)(
  IN EFI_SYSTEM_RESOURCE_ENTRY *Entry
  );

/**
  This function syn up Cached ESRT with data from FMP instances
  Function should be called after Connect All in order to locate all FMP protocols
  installed

  @retval EFI_SUCCESS                      Successfully sync cache repository from FMP instances
  @retval EFI_NOT_FOUND                   No FMP Instance are found
  @retval EFI_OUT_OF_RESOURCES     Resource allocaton fail

**/
typedef
EFI_STATUS
(EFIAPI *SYNC_ESRT_FMP)(
  VOID
  );

/**
  This function locks up Esrt repository to be readonly. It should be called
  before gEfiEndOfDxeEventGroupGuid event signaled

  @retval EFI_SUCCESS              Locks up FMP Non-FMP repository successfully

**/
typedef
EFI_STATUS
(EFIAPI *LOCK_ESRT_REPOSITORY)(
  VOID
  );

struct _ESRT_MANAGEMENT_PROTOCOL {
  GET_ESRT_ENTRY           GetEsrtEntry;
  UPDATE_ESRT_ENTRY        UpdateEsrtEntry;
  REGISTER_ESRT_ENTRY      RegisterEsrtEntry;
  UNREGISTER_ESRT_ENTRY    UnRegisterEsrtEntry;
  SYNC_ESRT_FMP            SyncEsrtFmp;
  LOCK_ESRT_REPOSITORY     LockEsrtRepository;
};

extern EFI_GUID  gEsrtManagementProtocolGuid;

#endif // #ifndef _ESRT_MANAGEMENT_H_
