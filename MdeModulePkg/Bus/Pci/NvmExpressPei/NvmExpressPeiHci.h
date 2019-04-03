/** @file
  The NvmExpressPei driver is used to manage non-volatile memory subsystem
  which follows NVM Express specification at PEI phase.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _NVM_EXPRESS_PEI_HCI_H_
#define _NVM_EXPRESS_PEI_HCI_H_

//
// NVME host controller registers operation definitions
//
#define NVME_GET_CAP(Private, Cap)             NvmeMmioRead  (Cap, Private->MmioBase + NVME_CAP_OFFSET, sizeof (NVME_CAP))
#define NVME_GET_CC(Private, Cc)               NvmeMmioRead  (Cc, Private->MmioBase + NVME_CC_OFFSET, sizeof (NVME_CC))
#define NVME_SET_CC(Private, Cc)               NvmeMmioWrite (Private->MmioBase + NVME_CC_OFFSET, Cc, sizeof (NVME_CC))
#define NVME_GET_CSTS(Private, Csts)           NvmeMmioRead  (Csts, Private->MmioBase + NVME_CSTS_OFFSET, sizeof (NVME_CSTS))
#define NVME_GET_AQA(Private, Aqa)             NvmeMmioRead  (Aqa, Private->MmioBase + NVME_AQA_OFFSET, sizeof (NVME_AQA))
#define NVME_SET_AQA(Private, Aqa)             NvmeMmioWrite (Private->MmioBase + NVME_AQA_OFFSET, Aqa, sizeof (NVME_AQA))
#define NVME_GET_ASQ(Private, Asq)             NvmeMmioRead  (Asq, Private->MmioBase + NVME_ASQ_OFFSET, sizeof (NVME_ASQ))
#define NVME_SET_ASQ(Private, Asq)             NvmeMmioWrite (Private->MmioBase + NVME_ASQ_OFFSET, Asq, sizeof (NVME_ASQ))
#define NVME_GET_ACQ(Private, Acq)             NvmeMmioRead  (Acq, Private->MmioBase + NVME_ACQ_OFFSET, sizeof (NVME_ACQ))
#define NVME_SET_ACQ(Private, Acq)             NvmeMmioWrite (Private->MmioBase + NVME_ACQ_OFFSET, Acq, sizeof (NVME_ACQ))
#define NVME_GET_VER(Private, Ver)             NvmeMmioRead  (Ver, Private->MmioBase + NVME_VER_OFFSET, sizeof (NVME_VER))
#define NVME_SET_SQTDBL(Private, Qid, Sqtdbl)  NvmeMmioWrite (Private->MmioBase + NVME_SQTDBL_OFFSET(Qid, Private->Cap.Dstrd), Sqtdbl, sizeof (NVME_SQTDBL))
#define NVME_SET_CQHDBL(Private, Qid, Cqhdbl)  NvmeMmioWrite (Private->MmioBase + NVME_CQHDBL_OFFSET(Qid, Private->Cap.Dstrd), Cqhdbl, sizeof (NVME_CQHDBL))

//
// Base memory address enum types
//
enum {
  BASEMEM_ASQ,
  BASEMEM_ACQ,
  BASEMEM_SQ,
  BASEMEM_CQ,
  BASEMEM_PRP,
  MAX_BASEMEM_COUNT
};

//
// All of base memories are 4K(0x1000) alignment
//
#define ALIGN(v, a)                          (UINTN)((((v) - 1) | ((a) - 1)) + 1)
#define NVME_MEM_BASE(Private)               ((UINTN)(Private->Buffer))
#define NVME_ASQ_BASE(Private)               (ALIGN (NVME_MEM_BASE(Private) + ((NvmeBaseMemPageOffset (BASEMEM_ASQ))                                * EFI_PAGE_SIZE), EFI_PAGE_SIZE))
#define NVME_ACQ_BASE(Private)               (ALIGN (NVME_MEM_BASE(Private) + ((NvmeBaseMemPageOffset (BASEMEM_ACQ))                                * EFI_PAGE_SIZE), EFI_PAGE_SIZE))
#define NVME_SQ_BASE(Private, Index)         (ALIGN (NVME_MEM_BASE(Private) + ((NvmeBaseMemPageOffset (BASEMEM_SQ) + ((Index)*(NVME_MAX_QUEUES-1))) * EFI_PAGE_SIZE), EFI_PAGE_SIZE))
#define NVME_CQ_BASE(Private, Index)         (ALIGN (NVME_MEM_BASE(Private) + ((NvmeBaseMemPageOffset (BASEMEM_CQ) + ((Index)*(NVME_MAX_QUEUES-1))) * EFI_PAGE_SIZE), EFI_PAGE_SIZE))
#define NVME_PRP_BASE(Private)               (ALIGN (NVME_MEM_BASE(Private) + ((NvmeBaseMemPageOffset (BASEMEM_PRP))                                * EFI_PAGE_SIZE), EFI_PAGE_SIZE))


/**
  Transfer MMIO Data to memory.

  @param[in,out] MemBuffer    Destination: Memory address.
  @param[in] MmioAddr         Source: MMIO address.
  @param[in] Size             Size for read.

  @retval EFI_SUCCESS         MMIO read sucessfully.

**/
EFI_STATUS
NvmeMmioRead (
  IN OUT VOID *MemBuffer,
  IN     UINTN MmioAddr,
  IN     UINTN Size
  );

/**
  Transfer memory data to MMIO.

  @param[in,out] MmioAddr    Destination: MMIO address.
  @param[in] MemBuffer       Source: Memory address.
  @param[in] Size            Size for write.

  @retval EFI_SUCCESS        MMIO write sucessfully.

**/
EFI_STATUS
NvmeMmioWrite (
  IN OUT UINTN MmioAddr,
  IN     VOID *MemBuffer,
  IN     UINTN Size
  );

/**
  Get the page offset for specific NVME based memory.

  @param[in] BaseMemIndex    The Index of BaseMem (0-based).

  @retval - The page count for specific BaseMem Index

**/
UINT32
NvmeBaseMemPageOffset (
  IN UINTN              BaseMemIndex
  );

/**
  Initialize the Nvm Express controller.

  @param[in] Private     The pointer to the PEI_NVME_CONTROLLER_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS    The NVM Express Controller is initialized successfully.
  @retval Others         A device error occurred while initializing the controller.

**/
EFI_STATUS
NvmeControllerInit (
  IN PEI_NVME_CONTROLLER_PRIVATE_DATA    *Private
  );

/**
  Get specified identify namespace data.

  @param[in] Private        The pointer to the PEI_NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] NamespaceId    The specified namespace identifier.
  @param[in] Buffer         The buffer used to store the identify namespace data.

  @return EFI_SUCCESS         Successfully get the identify namespace data.
  @return EFI_DEVICE_ERROR    Fail to get the identify namespace data.

**/
EFI_STATUS
NvmeIdentifyNamespace (
  IN PEI_NVME_CONTROLLER_PRIVATE_DATA    *Private,
  IN UINT32                              NamespaceId,
  IN VOID                                *Buffer
  );

/**
  Free the DMA resources allocated by an NVME controller.

  @param[in] Private     The pointer to the PEI_NVME_CONTROLLER_PRIVATE_DATA data structure.

**/
VOID
NvmeFreeDmaResource (
  IN PEI_NVME_CONTROLLER_PRIVATE_DATA    *Private
  );

#endif
