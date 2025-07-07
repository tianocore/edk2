/** @file
  This module provides communication with RAS Agent over RPMI/MPXY

  Copyright (c) 2024, Ventana Micro Systems, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _RAS_AGENT_CLIENT_H
#define _RAS_AGENT_CLIENT_H

#define MAX_SOURCES    512
#define MAX_DESC_SIZE  1024

/* RAS Agent Services on MPXY/RPMI */
#define RAS_GET_NUM_ERR_SRCS      0x1
#define RAS_GET_ERR_SRCS_ID_LIST  0x2
#define RAS_GET_ERR_SRC_DESC      0x3

#define ERROR_DESCRIPTOR_TYPE_SHIFT  4
#define MAX_ERROR_DESCRIPTOR_TYPES   (0x1UL << ERROR_DESCRIPTOR_TYPE_SHIFT)
#define ERROR_DESCRIPTOR_TYPE_MASK   (MAX_ERROR_DESCRIPTOR_TYPES - 1)

#define MPXY_SHMEM_SIZE    4096
#define MAX_MPXY_CHANNELS  64

#define BASE_ATTR_ID       (UINT32)0x80000000
#define RAS_SERVICE_GROUP  (UINT32)0xB
#define __packed32         __attribute__((packed,aligned(__alignof__(UINT32))))

typedef struct __packed32 {
  UINT32    status;
  UINT32    flags;
  UINT32    remaining;
  UINT32    returned;
  UINT32    func_id;
} RasRpmiRespHeader;

typedef struct __packed32 {
  RasRpmiRespHeader    RespHdr;
  UINT32               ErrSourceList[MAX_SOURCES];
} ErrorSourceListResp;

typedef struct __packed32 {
  RasRpmiRespHeader    RspHdr;
  UINT8                desc[MAX_DESC_SIZE];
} ErrDescResp;

typedef enum {
  DT_GHESV2,
  NUM_ERR_DESC_TYPES
} ErrorDescriptorType;

#define ERROR_DESCRIPTOR_TYPE_SHIFT  4
#define MAX_ERROR_DESCRIPTOR_TYPES   (0x1UL << ERROR_DESCRIPTOR_TYPE_SHIFT)
#define ERROR_DESCRIPTOR_TYPE_MASK   (MAX_ERROR_DESCRIPTOR_TYPES - 1)

/**
  Initialize the RAS agent client

  @retval EFI_SUCCESS  If initialization is successful
**/
EFI_STATUS
EFIAPI
RacInit (
  VOID
  );

/**
  Get the number of hardware error sources from the RAS Agent

  @param NumErrorSources  Pointer to an array of 32-bit integers which will
                          contain number of hardware error sources available.

  @retval EFI_SUCCESS  If fetching the number of error sources succeeded.
**/
EFI_STATUS
EFIAPI
RacGetNumberErrorSources (
  OUT UINT32  *NumErrorSources
  );

/**
  Get the list of hardware error source IDs from the RAS Agent

  @param ErrorSourceList  Will contain pointer to error of 32-bit integers
                          containing the error source IDs.
  @param NumSources       Will contain the number of IDs in *ErrorSourceList

  @retval EFI_SUCCESS  If fetching the error source IDs succeeded.
**/
EFI_STATUS
EFIAPI
RacGetErrorSourceIDList (
  OUT UINT32  **ErrorSourceList,
  OUT UINT32  *NumSources
  );

/**
  Get the hardware error source descriptor for a given error source ID.

  @param SourceID  Error source ID for which descriptor is to be fetched
  @param DescriptorType  Type of error descritor (GHESv2 or platform specific)
  @param ErrorDesciptor  Pointer to buffer containing the descriptor. The caller
                         must free the buffer when done.
  @param ErrorDescriptorSize  Size of the error descriptor buffer in ErrorDescriptor

  @retval EFI_SUCCESS  On success.
**/
EFI_STATUS
EFIAPI
RacGetErrorSourceDescriptor (
  IN UINT32   SourceID,
  OUT UINTN   *DescriptorType,
  OUT VOID    **ErrorDescriptor,
  OUT UINT32  *ErrorDescriptorSize
  );

#endif
