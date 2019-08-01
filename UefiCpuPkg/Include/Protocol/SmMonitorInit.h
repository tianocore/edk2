/** @file
  STM service protocol definition

  Copyright (c) 2015 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SM_MONITOR_INIT_PROTOCOL_H_
#define _SM_MONITOR_INIT_PROTOCOL_H_

#include <PiSmm.h>
#include <Register/Intel/StmApi.h>

#define EFI_SM_MONITOR_INIT_PROTOCOL_GUID \
    { 0x228f344d, 0xb3de, 0x43bb, 0xa4, 0xd7, 0xea, 0x20, 0xb, 0x1b, 0x14, 0x82}

//
// STM service
//

/**

  Load STM image to MSEG.

  @param StmImage      STM image
  @param StmImageSize  STM image size

  @retval EFI_SUCCESS            Load STM to MSEG successfully
  @retval EFI_ALREADY_STARTED    STM image is already loaded to MSEG
  @retval EFI_BUFFER_TOO_SMALL   MSEG is smaller than minimal requirement of STM image
  @retval EFI_UNSUPPORTED        MSEG is not enabled

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SM_MONITOR_LOAD_MONITOR) (
  IN EFI_PHYSICAL_ADDRESS StmImage,
  IN UINTN                StmImageSize
  );

/**

  Add resources in list to database.

  @param ResourceList  A pointer to resource list to be added
  @param NumEntries    Optional number of entries.
                       If 0, list must be terminated by END_OF_RESOURCES.

  @retval EFI_SUCCESS            If resources are added
  @retval EFI_INVALID_PARAMETER  If nested procedure detected resource failer
  @retval EFI_OUT_OF_RESOURCES   If nested procedure returned it and we cannot allocate more areas.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SM_MONITOR_ADD_PI_RESOURCE) (
  IN STM_RSC *ResourceList,
  IN UINT32   NumEntries OPTIONAL
  );

/**

  Delete resources in list to database.

  @param ResourceList  A pointer to resource list to be deleted
                       NULL means delete all resources.
  @param NumEntries    Optional number of entries.
                       If 0, list must be terminated by END_OF_RESOURCES.

  @retval EFI_SUCCESS            If resources are deleted
  @retval EFI_INVALID_PARAMETER  If nested procedure detected resource failer

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SM_MONITOR_DELETE_PI_RESOURCE) (
  IN STM_RSC *ResourceList OPTIONAL,
  IN UINT32   NumEntries OPTIONAL
  );

/**

  Get BIOS resources.

  @param ResourceList  A pointer to resource list to be filled
  @param ResourceSize  On input it means size of resource list input.
                       On output it means size of resource list filled,
                       or the size of resource list to be filled if size of too small.

  @retval EFI_SUCCESS            If resources are returned.
  @retval EFI_BUFFER_TOO_SMALL   If resource list buffer is too small to hold the whole resources.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SM_MONITOR_GET_PI_RESOURCE) (
  OUT    STM_RSC *ResourceList,
  IN OUT UINT32  *ResourceSize
  );

typedef UINT32 EFI_SM_MONITOR_STATE;
#define EFI_SM_MONITOR_STATE_ENABLED     0x1
#define EFI_SM_MONITOR_STATE_ACTIVATED   0x2

/**

  Get STM state

  @return STM state

**/
typedef
EFI_SM_MONITOR_STATE
(EFIAPI *EFI_SM_MONITOR_GET_MONITOR_STATE) (
  VOID
  );

typedef struct _EFI_SM_MONITOR_INIT_PROTOCOL {
  //
  // Valid at boot-time only
  //
  EFI_SM_MONITOR_LOAD_MONITOR                      LoadMonitor;
  EFI_SM_MONITOR_ADD_PI_RESOURCE                   AddPiResource;
  EFI_SM_MONITOR_DELETE_PI_RESOURCE                DeletePiResource;
  EFI_SM_MONITOR_GET_PI_RESOURCE                   GetPiResource;
  //
  // Valid at runtime
  //
  EFI_SM_MONITOR_GET_MONITOR_STATE                 GetMonitorState;
} EFI_SM_MONITOR_INIT_PROTOCOL;

extern EFI_GUID gEfiSmMonitorInitProtocolGuid;

#endif
