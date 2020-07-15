/** @file
  Esrt management implementation head file.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DXE_ESRT_IMPL_H_
#define _DXE_ESRT_IMPL_H_

#include <Guid/EventGroup.h>
#include <Guid/SystemResourceTable.h>

#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/CapsuleLib.h>

#include <Protocol/FirmwareManagement.h>
#include <Protocol/EsrtManagement.h>
#include <Protocol/VariableLock.h>

//
// Name of  Variable for Non-FMP ESRT Repository
//
#define EFI_ESRT_NONFMP_VARIABLE_NAME    L"EsrtNonFmp"

//
// Name of Variable for FMP
//
#define EFI_ESRT_FMP_VARIABLE_NAME       L"EsrtFmp"

//
// Attribute of Cached ESRT entry
//
#define ESRT_FROM_FMP                    0x00000001
#define ESRT_FROM_NONFMP                 0x00000002

typedef struct {
  EFI_HANDLE            Handle;
  //
  // Ready to boot event
  //
  EFI_EVENT             Event;

  //
  // Updates to Fmp storage must be locked.
  //
  EFI_LOCK              FmpLock;

  //
  // Update to Non-Fmp storage must be locked
  //
  EFI_LOCK              NonFmpLock;
} ESRT_PRIVATE_DATA;


/**
  Find Esrt Entry stored in ESRT repository.

  @param[in]     FwClass           Firmware class guid in Esrt entry
  @param[in]     Attribute         Esrt from Non FMP or FMP instance
  @param[out]    Entry             Esrt entry returned

  @retval EFI_SUCCESS            Successfully find an Esrt entry
  @retval EF_NOT_FOUND           No Esrt entry found

**/
EFI_STATUS
GetEsrtEntry (
  IN  EFI_GUID              *FwClass,
  IN  UINTN                 Attribute,
  OUT EFI_SYSTEM_RESOURCE_ENTRY *Entry
  );

/**
  Insert a new ESRT entry into ESRT Cache repository.

  @param[in]  Entry                Esrt entry to be set
  @param[in]  Attribute            Esrt from Esrt private protocol or FMP instance

  @retval EFI_SUCCESS          Successfully set a variable.

**/
EFI_STATUS
InsertEsrtEntry(
  IN EFI_SYSTEM_RESOURCE_ENTRY *Entry,
  UINTN                        Attribute
  );

/**
  Delete ESRT Entry from ESRT repository.

  @param[in]    FwClass              FwClass of Esrt entry to delete
  @param[in]    Attribute            Esrt from Esrt private protocol or FMP instance

  @retval EFI_SUCCESS         Insert all entries Successfully
  @retval EFI_NOT_FOUND       ESRT entry with FwClass doesn't exsit

**/
EFI_STATUS
DeleteEsrtEntry(
  IN  EFI_GUID        *FwClass,
  IN  UINTN           Attribute
  );

/**
  Update one ESRT entry in ESRT repository

  @param[in]    Entry                Esrt entry to be set
  @param[in]    Attribute            Esrt from Non Esrt or FMP instance

  @retval EFI_SUCCESS          Successfully Update a variable.
  @retval EFI_NOT_FOUND        The Esrt enry doesn't exist

**/
EFI_STATUS
UpdateEsrtEntry(
  IN EFI_SYSTEM_RESOURCE_ENTRY *Entry,
  UINTN                        Attribute
  );

/**
  Init one ESRT entry according to input FmpImageInfo (V1, V2, V3) .

  @param[in, out]    EsrtEntry             Esrt entry to be Init
  @param[in]         FmpImageInfo          FMP image info descriptor
  @param[in]         DescriptorVersion     FMP Image info descriptor version

**/
VOID
SetEsrtEntryFromFmpInfo (
  IN OUT EFI_SYSTEM_RESOURCE_ENTRY   *EsrtEntry,
  IN EFI_FIRMWARE_IMAGE_DESCRIPTOR   *FmpImageInfo,
  IN UINT32                          DescriptorVersion
  );

/**
  Get ESRT entry from ESRT Cache by FwClass Guid

  @param[in]       FwClass                FwClass of Esrt entry to get
  @param[in, out]  Entry                  Esrt entry returned

  @retval EFI_SUCCESS                   The variable saving this Esrt Entry exists.
  @retval EF_NOT_FOUND                  No correct variable found.
  @retval EFI_WRITE_PROTECTED           ESRT Cache repository is locked

**/
EFI_STATUS
EFIAPI
EsrtDxeGetEsrtEntry(
  IN     EFI_GUID                  *FwClass,
  IN OUT EFI_SYSTEM_RESOURCE_ENTRY *Entry
  );

/**
  Update one ESRT entry in ESRT Cache.

  @param[in]  Entry                         Esrt entry to be updated

  @retval EFI_SUCCESS                   Successfully update an ESRT entry in cache.
  @retval EFI_INVALID_PARAMETER  Entry does't exist in ESRT Cache
  @retval EFI_WRITE_PROTECTED     ESRT Cache is locked

**/
EFI_STATUS
EFIAPI
EsrtDxeUpdateEsrtEntry(
  IN EFI_SYSTEM_RESOURCE_ENTRY *Entry
  );

/**
  Non-FMP instance to unregister Esrt Entry from ESRT Cache.

  @param[in]    FwClass                FwClass of Esrt entry to Unregister

  @retval EFI_SUCCESS             Insert all entries Successfully
  @retval EFI_NOT_FOUND           Entry of FwClass does not exsit

**/
EFI_STATUS
EFIAPI
EsrtDxeUnRegisterEsrtEntry(
  IN  EFI_GUID        *FwClass
  );

/**
  Non-FMP instance to register one ESRT entry into ESRT Cache.

  @param[in]  Entry                Esrt entry to be set

  @retval EFI_SUCCESS          Successfully set a variable.
  @retval EFI_INVALID_PARAMETER  ESRT Entry is already exist
**/
EFI_STATUS
EFIAPI
EsrtDxeRegisterEsrtEntry(
  IN EFI_SYSTEM_RESOURCE_ENTRY *Entry
  );

/**
  This function syn up Cached ESRT with data from FMP instances
  Function should be called after Connect All in order to locate all FMP protocols
  installed.

  @retval EFI_SUCCESS                      Successfully sync cache repository from FMP instances
  @retval EFI_NOT_FOUND                   No FMP Instance are found
  @retval EFI_OUT_OF_RESOURCES     Resource allocaton fail

**/
EFI_STATUS
EFIAPI
EsrtDxeSyncFmp(
  VOID
  );

/**
  This function locks up Esrt repository to be readonly. It should be called
  before gEfiEndOfDxeEventGroupGuid event signaled

  @retval EFI_SUCCESS              Locks up FMP Non-FMP repository successfully

**/
EFI_STATUS
EFIAPI
EsrtDxeLockEsrtRepository(
  VOID
  );

#endif // #ifndef _EFI_ESRT_IMPL_H_

