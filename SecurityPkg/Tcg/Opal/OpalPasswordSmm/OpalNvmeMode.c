/** @file
  Provide functions to initialize NVME controller and perform NVME commands

Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "OpalPasswordSmm.h"


#define ALIGN(v, a)                         (UINTN)((((v) - 1) | ((a) - 1)) + 1)

///
/// NVME Host controller registers operation
///
#define NVME_GET_CAP(Nvme, Cap)             NvmeMmioRead  (Cap, Nvme->Nbar + NVME_CAP_OFFSET, sizeof (NVME_CAP))
#define NVME_GET_CC(Nvme, Cc)               NvmeMmioRead  (Cc, Nvme->Nbar + NVME_CC_OFFSET, sizeof (NVME_CC))
#define NVME_SET_CC(Nvme, Cc)               NvmeMmioWrite (Nvme->Nbar + NVME_CC_OFFSET, Cc, sizeof (NVME_CC))
#define NVME_GET_CSTS(Nvme, Csts)           NvmeMmioRead  (Csts, Nvme->Nbar + NVME_CSTS_OFFSET, sizeof (NVME_CSTS))
#define NVME_GET_AQA(Nvme, Aqa)             NvmeMmioRead  (Aqa, Nvme->Nbar + NVME_AQA_OFFSET, sizeof (NVME_AQA))
#define NVME_SET_AQA(Nvme, Aqa)             NvmeMmioWrite (Nvme->Nbar + NVME_AQA_OFFSET, Aqa, sizeof (NVME_AQA))
#define NVME_GET_ASQ(Nvme, Asq)             NvmeMmioRead  (Asq, Nvme->Nbar + NVME_ASQ_OFFSET, sizeof (NVME_ASQ))
#define NVME_SET_ASQ(Nvme, Asq)             NvmeMmioWrite (Nvme->Nbar + NVME_ASQ_OFFSET, Asq, sizeof (NVME_ASQ))
#define NVME_GET_ACQ(Nvme, Acq)             NvmeMmioRead  (Acq, Nvme->Nbar + NVME_ACQ_OFFSET, sizeof (NVME_ACQ))
#define NVME_SET_ACQ(Nvme, Acq)             NvmeMmioWrite (Nvme->Nbar + NVME_ACQ_OFFSET, Acq, sizeof (NVME_ACQ))
#define NVME_GET_VER(Nvme, Ver)             NvmeMmioRead  (Ver, Nvme->Nbar + NVME_VER_OFFSET, sizeof (NVME_VER))
#define NVME_SET_SQTDBL(Nvme, Qid, Sqtdbl)  NvmeMmioWrite (Nvme->Nbar + NVME_SQTDBL_OFFSET(Qid, Nvme->Cap.Dstrd), Sqtdbl, sizeof (NVME_SQTDBL))
#define NVME_SET_CQHDBL(Nvme, Qid, Cqhdbl)  NvmeMmioWrite (Nvme->Nbar + NVME_CQHDBL_OFFSET(Qid, Nvme->Cap.Dstrd), Cqhdbl, sizeof (NVME_CQHDBL))

///
/// Base memory address
///
enum {
  BASEMEM_CONTROLLER_DATA,
  BASEMEM_IDENTIFY_DATA,
  BASEMEM_ASQ,
  BASEMEM_ACQ,
  BASEMEM_SQ,
  BASEMEM_CQ,
  BASEMEM_PRP,
  BASEMEM_SECURITY,
  MAX_BASEMEM_COUNT
};

///
/// All of base memories are 4K(0x1000) alignment
///
#define NVME_MEM_BASE(Nvme)                 ((UINTN)(Nvme->BaseMem))
#define NVME_CONTROL_DATA_BASE(Nvme)        (ALIGN (NVME_MEM_BASE(Nvme) + ((NvmeGetBaseMemPages (BASEMEM_CONTROLLER_DATA))                        * EFI_PAGE_SIZE), EFI_PAGE_SIZE))
#define NVME_NAMESPACE_DATA_BASE(Nvme)      (ALIGN (NVME_MEM_BASE(Nvme) + ((NvmeGetBaseMemPages (BASEMEM_IDENTIFY_DATA))                          * EFI_PAGE_SIZE), EFI_PAGE_SIZE))
#define NVME_ASQ_BASE(Nvme)                 (ALIGN (NVME_MEM_BASE(Nvme) + ((NvmeGetBaseMemPages (BASEMEM_ASQ))                                    * EFI_PAGE_SIZE), EFI_PAGE_SIZE))
#define NVME_ACQ_BASE(Nvme)                 (ALIGN (NVME_MEM_BASE(Nvme) + ((NvmeGetBaseMemPages (BASEMEM_ACQ))                                    * EFI_PAGE_SIZE), EFI_PAGE_SIZE))
#define NVME_SQ_BASE(Nvme, index)           (ALIGN (NVME_MEM_BASE(Nvme) + ((NvmeGetBaseMemPages (BASEMEM_SQ) + ((index)*(NVME_MAX_IO_QUEUES-1)))  * EFI_PAGE_SIZE), EFI_PAGE_SIZE))
#define NVME_CQ_BASE(Nvme, index)           (ALIGN (NVME_MEM_BASE(Nvme) + ((NvmeGetBaseMemPages (BASEMEM_CQ) + ((index)*(NVME_MAX_IO_QUEUES-1)))  * EFI_PAGE_SIZE), EFI_PAGE_SIZE))
#define NVME_PRP_BASE(Nvme, index)          (ALIGN (NVME_MEM_BASE(Nvme) + ((NvmeGetBaseMemPages (BASEMEM_PRP) + ((index)*NVME_PRP_SIZE))          * EFI_PAGE_SIZE), EFI_PAGE_SIZE))
#define NVME_SEC_BASE(Nvme)                 (ALIGN (NVME_MEM_BASE(Nvme) + ((NvmeGetBaseMemPages (BASEMEM_SECURITY))                               * EFI_PAGE_SIZE), EFI_PAGE_SIZE))

/**
  Transfer MMIO Data to memory.

  @param[in,out] MemBuffer - Destination: Memory address
  @param[in] MmioAddr      - Source: MMIO address
  @param[in] Size          - Size for read

  @retval EFI_SUCCESS - MMIO read sucessfully
**/
EFI_STATUS
NvmeMmioRead (
  IN OUT VOID *MemBuffer,
  IN     UINTN MmioAddr,
  IN     UINTN Size
  )
{
  UINTN  Offset;
  UINT8  Data;
  UINT8  *Ptr;

  // priority has adjusted
  switch (Size) {
    case 4:
      *((UINT32 *)MemBuffer) = MmioRead32 (MmioAddr);
      break;

    case 8:
      *((UINT64 *)MemBuffer) = MmioRead64 (MmioAddr);
      break;

    case 2:
      *((UINT16 *)MemBuffer) = MmioRead16 (MmioAddr);
      break;

    case 1:
      *((UINT8 *)MemBuffer) = MmioRead8 (MmioAddr);
      break;

    default:
      Ptr = (UINT8 *)MemBuffer;
      for (Offset = 0; Offset < Size; Offset += 1) {
        Data = MmioRead8 (MmioAddr + Offset);
        Ptr[Offset] = Data;
      }
      break;
  }

  return EFI_SUCCESS;
}

/**
  Transfer memory data to MMIO.

  @param[in,out] MmioAddr - Destination: MMIO address
  @param[in] MemBuffer    - Source: Memory address
  @param[in] Size         - Size for write

  @retval EFI_SUCCESS - MMIO write sucessfully
**/
EFI_STATUS
NvmeMmioWrite (
  IN OUT UINTN MmioAddr,
  IN     VOID *MemBuffer,
  IN     UINTN Size
  )
{
  UINTN  Offset;
  UINT8  Data;
  UINT8  *Ptr;

  // priority has adjusted
  switch (Size) {
    case 4:
      MmioWrite32 (MmioAddr, *((UINT32 *)MemBuffer));
      break;

    case 8:
      MmioWrite64 (MmioAddr, *((UINT64 *)MemBuffer));
      break;

    case 2:
      MmioWrite16 (MmioAddr, *((UINT16 *)MemBuffer));
      break;

    case 1:
      MmioWrite8 (MmioAddr, *((UINT8 *)MemBuffer));
      break;

    default:
      Ptr = (UINT8 *)MemBuffer;
      for (Offset = 0; Offset < Size; Offset += 1) {
        Data = Ptr[Offset];
        MmioWrite8 (MmioAddr + Offset, Data);
      }
      break;
  }

  return EFI_SUCCESS;
}

/**
  Transfer MMIO data to memory.

  @param[in,out] MemBuffer - Destination: Memory address
  @param[in] MmioAddr      - Source: MMIO address
  @param[in] Size          - Size for read

  @retval EFI_SUCCESS - MMIO read sucessfully
**/
EFI_STATUS
OpalPciRead (
  IN OUT VOID *MemBuffer,
  IN     UINTN MmioAddr,
  IN     UINTN Size
  )
{
  UINTN  Offset;
  UINT8  Data;
  UINT8  *Ptr;

  // priority has adjusted
  switch (Size) {
    case 4:
      *((UINT32 *)MemBuffer) = PciRead32 (MmioAddr);
      break;

    case 2:
      *((UINT16 *)MemBuffer) = PciRead16 (MmioAddr);
      break;

    case 1:
      *((UINT8 *)MemBuffer) = PciRead8 (MmioAddr);
      break;

    default:
      Ptr = (UINT8 *)MemBuffer;
      for (Offset = 0; Offset < Size; Offset += 1) {
        Data = PciRead8 (MmioAddr + Offset);
        Ptr[Offset] = Data;
      }
      break;
  }

  return EFI_SUCCESS;
}

/**
  Transfer memory data to MMIO.

  @param[in,out] MmioAddr - Destination: MMIO address
  @param[in] MemBuffer    - Source: Memory address
  @param[in] Size         - Size for write

  @retval EFI_SUCCESS - MMIO write sucessfully
**/
EFI_STATUS
OpalPciWrite (
  IN OUT UINTN MmioAddr,
  IN     VOID *MemBuffer,
  IN     UINTN Size
  )
{
  UINTN  Offset;
  UINT8  Data;
  UINT8  *Ptr;

  // priority has adjusted
  switch (Size) {
    case 4:
      PciWrite32 (MmioAddr, *((UINT32 *)MemBuffer));
      break;

    case 2:
      PciWrite16 (MmioAddr, *((UINT16 *)MemBuffer));
      break;

    case 1:
      PciWrite8 (MmioAddr, *((UINT8 *)MemBuffer));
      break;

    default:
      Ptr = (UINT8 *)MemBuffer;
      for (Offset = 0; Offset < Size; Offset += 1) {
        Data = Ptr[Offset];
        PciWrite8 (MmioAddr + Offset, Data);
      }
      break;
  }

  return EFI_SUCCESS;
}

/**
  Get total pages for specific NVME based memory.

  @param[in] BaseMemIndex           - The Index of BaseMem (0-based).

  @retval - The page count for specific BaseMem Index

**/
UINT32
NvmeGetBaseMemPages (
  IN UINTN              BaseMemIndex
  )
{
  UINT32                Pages;
  UINTN                 Index;
  UINT32                PageSizeList[8];

  PageSizeList[0] = 1;  /* Controller Data */
  PageSizeList[1] = 1;  /* Identify Data */
  PageSizeList[2] = 1;  /* ASQ */
  PageSizeList[3] = 1;  /* ACQ */
  PageSizeList[4] = 1;  /* SQs */
  PageSizeList[5] = 1;  /* CQs */
  PageSizeList[6] = NVME_PRP_SIZE * NVME_CSQ_DEPTH;  /* PRPs */
  PageSizeList[7] = 1;  /* Security Commands */

  if (BaseMemIndex > MAX_BASEMEM_COUNT) {
    ASSERT (FALSE);
    return 0;
  }

  Pages = 0;
  for (Index = 0; Index < BaseMemIndex; Index++) {
    Pages += PageSizeList[Index];
  }

  return Pages;
}

/**
  Wait for NVME controller status to be ready or not.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in] WaitReady              - Flag for waitting status ready or not

  @return EFI_SUCCESS               - Successfully to wait specific status.
  @return others                    - Fail to wait for specific controller status.

**/
STATIC
EFI_STATUS
NvmeWaitController (
  IN NVME_CONTEXT       *Nvme,
  IN BOOLEAN            WaitReady
  )
{
  NVME_CSTS              Csts;
  EFI_STATUS             Status;
  UINT32                 Index;
  UINT8                  Timeout;

  //
  // Cap.To specifies max delay time in 500ms increments for Csts.Rdy to set after
  // Cc.Enable. Loop produces a 1 millisecond delay per itteration, up to 500 * Cap.To.
  //
  if (Nvme->Cap.To == 0) {
    Timeout = 1;
  } else {
    Timeout = Nvme->Cap.To;
  }

  Status = EFI_SUCCESS;
  for(Index = (Timeout * 500); Index != 0; --Index) {
    MicroSecondDelay (1000);

    //
    // Check if the controller is initialized
    //
    Status = NVME_GET_CSTS (Nvme, &Csts);
    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_ERROR, "NVME_GET_CSTS fail, Status = %r\n", Status));
      return Status;
    }

    if ((BOOLEAN) Csts.Rdy == WaitReady) {
      break;
    }
  }

  if (Index == 0) {
    Status = EFI_TIMEOUT;
  }

  return Status;
}

/**
  Disable the Nvm Express controller.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.

  @return EFI_SUCCESS               - Successfully disable the controller.
  @return others                    - Fail to disable the controller.

**/
STATIC
EFI_STATUS
NvmeDisableController (
  IN NVME_CONTEXT       *Nvme
  )
{
  NVME_CC                Cc;
  NVME_CSTS              Csts;
  EFI_STATUS             Status;

  Status = NVME_GET_CSTS (Nvme, &Csts);

  ///
  /// Read Controller Configuration Register.
  ///
  Status = NVME_GET_CC (Nvme, &Cc);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "NVME_GET_CC fail, Status = %r\n", Status));
    goto Done;
  }

  if (Cc.En == 1) {
    Cc.En = 0;
    ///
    /// Disable the controller.
    ///
    Status = NVME_SET_CC (Nvme, &Cc);
    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_ERROR, "NVME_SET_CC fail, Status = %r\n", Status));
      goto Done;
    }
  }

  Status = NvmeWaitController (Nvme, FALSE);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "NvmeWaitController fail, Status = %r\n", Status));
    goto Done;
  }

  return EFI_SUCCESS;

Done:
  DEBUG ((DEBUG_INFO, "NvmeDisableController fail, Status: %r\n", Status));
  return Status;
}

/**
  Enable the Nvm Express controller.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.

  @return EFI_SUCCESS               - Successfully enable the controller.
  @return EFI_DEVICE_ERROR          - Fail to enable the controller.
  @return EFI_TIMEOUT               - Fail to enable the controller in given time slot.

**/
STATIC
EFI_STATUS
NvmeEnableController (
  IN NVME_CONTEXT       *Nvme
  )
{
  NVME_CC                Cc;
  EFI_STATUS             Status;

  //
  // Enable the controller
  //
  ZeroMem (&Cc, sizeof (NVME_CC));
  Cc.En     = 1;
  Cc.Iosqes = 6;
  Cc.Iocqes = 4;
  Status    = NVME_SET_CC (Nvme, &Cc);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "NVME_SET_CC fail, Status = %r\n", Status));
    goto Done;
  }

  Status = NvmeWaitController (Nvme, TRUE);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "NvmeWaitController fail, Status = %r\n", Status));
    goto Done;
  }

  return EFI_SUCCESS;

Done:
  DEBUG ((DEBUG_INFO, "NvmeEnableController fail, Status: %r\n", Status));
  return Status;
}

/**
  Shutdown the Nvm Express controller.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.

  @return EFI_SUCCESS               - Successfully shutdown the controller.
  @return EFI_DEVICE_ERROR          - Fail to shutdown the controller.
  @return EFI_TIMEOUT               - Fail to shutdown the controller in given time slot.

**/
STATIC
EFI_STATUS
NvmeShutdownController (
  IN NVME_CONTEXT       *Nvme
  )
{
  NVME_CC                Cc;
  NVME_CSTS              Csts;
  EFI_STATUS             Status;
  UINT32                 Index;
  UINTN                  Timeout;

  Status    = NVME_GET_CC (Nvme, &Cc);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "NVME_GET_CC fail, Status = %r\n", Status));
    return Status;
  }

  Cc.Shn     = 1; // Normal shutdown

  Status    = NVME_SET_CC (Nvme, &Cc);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "NVME_SET_CC fail, Status = %r\n", Status));
    return Status;
  }

  Timeout = NVME_GENERIC_TIMEOUT/1000; // ms
  for(Index = (UINT32)(Timeout); Index != 0; --Index) {
    MicroSecondDelay (1000);

    Status = NVME_GET_CSTS (Nvme, &Csts);
    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_ERROR, "NVME_GET_CSTS fail, Status = %r\n", Status));
      return Status;
    }

    if (Csts.Shst == 2) { // Shutdown processing complete
      break;
    }
  }

  if (Index == 0) {
    Status = EFI_TIMEOUT;
  }

  return Status;
}

/**
  Check the execution status from a given completion queue entry.

  @param[in]     Cq                 - A pointer to the NVME_CQ item.

**/
EFI_STATUS
NvmeCheckCqStatus (
  IN NVME_CQ             *Cq
  )
{
  if (Cq->Sct == 0x0 && Cq->Sc == 0x0) {
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "Dump NVMe Completion Entry Status from [0x%x]:\n", (UINTN)Cq));
  DEBUG ((DEBUG_INFO, "  SQ Identifier : [0x%x], Phase Tag : [%d], Cmd Identifier : [0x%x]\n", Cq->Sqid, Cq->Pt, Cq->Cid));
  DEBUG ((DEBUG_INFO, "  NVMe Cmd Execution Result - "));

  switch (Cq->Sct) {
    case 0x0:
      switch (Cq->Sc) {
        case 0x0:
          DEBUG ((DEBUG_INFO, "Successful Completion\n"));
          return EFI_SUCCESS;
        case 0x1:
          DEBUG ((DEBUG_INFO, "Invalid Command Opcode\n"));
          break;
        case 0x2:
          DEBUG ((DEBUG_INFO, "Invalid Field in Command\n"));
          break;
        case 0x3:
          DEBUG ((DEBUG_INFO, "Command ID Conflict\n"));
          break;
        case 0x4:
          DEBUG ((DEBUG_INFO, "Data Transfer Error\n"));
          break;
        case 0x5:
          DEBUG ((DEBUG_INFO, "Commands Aborted due to Power Loss Notification\n"));
          break;
        case 0x6:
          DEBUG ((DEBUG_INFO, "Internal Device Error\n"));
          break;
        case 0x7:
          DEBUG ((DEBUG_INFO, "Command Abort Requested\n"));
          break;
        case 0x8:
          DEBUG ((DEBUG_INFO, "Command Aborted due to SQ Deletion\n"));
          break;
        case 0x9:
          DEBUG ((DEBUG_INFO, "Command Aborted due to Failed Fused Command\n"));
          break;
        case 0xA:
          DEBUG ((DEBUG_INFO, "Command Aborted due to Missing Fused Command\n"));
          break;
        case 0xB:
          DEBUG ((DEBUG_INFO, "Invalid Namespace or Format\n"));
          break;
        case 0xC:
          DEBUG ((DEBUG_INFO, "Command Sequence Error\n"));
          break;
        case 0xD:
          DEBUG ((DEBUG_INFO, "Invalid SGL Last Segment Descriptor\n"));
          break;
        case 0xE:
          DEBUG ((DEBUG_INFO, "Invalid Number of SGL Descriptors\n"));
          break;
        case 0xF:
          DEBUG ((DEBUG_INFO, "Data SGL Length Invalid\n"));
          break;
        case 0x10:
          DEBUG ((DEBUG_INFO, "Metadata SGL Length Invalid\n"));
          break;
        case 0x11:
          DEBUG ((DEBUG_INFO, "SGL Descriptor Type Invalid\n"));
          break;
        case 0x80:
          DEBUG ((DEBUG_INFO, "LBA Out of Range\n"));
          break;
        case 0x81:
          DEBUG ((DEBUG_INFO, "Capacity Exceeded\n"));
          break;
        case 0x82:
          DEBUG ((DEBUG_INFO, "Namespace Not Ready\n"));
          break;
        case 0x83:
          DEBUG ((DEBUG_INFO, "Reservation Conflict\n"));
          break;
      }
      break;

    case 0x1:
      switch (Cq->Sc) {
        case 0x0:
          DEBUG ((DEBUG_INFO, "Completion Queue Invalid\n"));
          break;
        case 0x1:
          DEBUG ((DEBUG_INFO, "Invalid Queue Identifier\n"));
          break;
        case 0x2:
          DEBUG ((DEBUG_INFO, "Maximum Queue Size Exceeded\n"));
          break;
        case 0x3:
          DEBUG ((DEBUG_INFO, "Abort Command Limit Exceeded\n"));
          break;
        case 0x5:
          DEBUG ((DEBUG_INFO, "Asynchronous Event Request Limit Exceeded\n"));
          break;
        case 0x6:
          DEBUG ((DEBUG_INFO, "Invalid Firmware Slot\n"));
          break;
        case 0x7:
          DEBUG ((DEBUG_INFO, "Invalid Firmware Image\n"));
          break;
        case 0x8:
          DEBUG ((DEBUG_INFO, "Invalid Interrupt Vector\n"));
          break;
        case 0x9:
          DEBUG ((DEBUG_INFO, "Invalid Log Page\n"));
          break;
        case 0xA:
          DEBUG ((DEBUG_INFO, "Invalid Format\n"));
          break;
        case 0xB:
          DEBUG ((DEBUG_INFO, "Firmware Application Requires Conventional Reset\n"));
          break;
        case 0xC:
          DEBUG ((DEBUG_INFO, "Invalid Queue Deletion\n"));
          break;
        case 0xD:
          DEBUG ((DEBUG_INFO, "Feature Identifier Not Saveable\n"));
          break;
        case 0xE:
          DEBUG ((DEBUG_INFO, "Feature Not Changeable\n"));
          break;
        case 0xF:
          DEBUG ((DEBUG_INFO, "Feature Not Namespace Specific\n"));
          break;
        case 0x10:
          DEBUG ((DEBUG_INFO, "Firmware Application Requires NVM Subsystem Reset\n"));
          break;
        case 0x80:
          DEBUG ((DEBUG_INFO, "Conflicting Attributes\n"));
          break;
        case 0x81:
          DEBUG ((DEBUG_INFO, "Invalid Protection Information\n"));
          break;
        case 0x82:
          DEBUG ((DEBUG_INFO, "Attempted Write to Read Only Range\n"));
          break;
      }
      break;

    case 0x2:
      switch (Cq->Sc) {
        case 0x80:
          DEBUG ((DEBUG_INFO, "Write Fault\n"));
          break;
        case 0x81:
          DEBUG ((DEBUG_INFO, "Unrecovered Read Error\n"));
          break;
        case 0x82:
          DEBUG ((DEBUG_INFO, "End-to-end Guard Check Error\n"));
          break;
        case 0x83:
          DEBUG ((DEBUG_INFO, "End-to-end Application Tag Check Error\n"));
          break;
        case 0x84:
          DEBUG ((DEBUG_INFO, "End-to-end Reference Tag Check Error\n"));
          break;
        case 0x85:
          DEBUG ((DEBUG_INFO, "Compare Failure\n"));
          break;
        case 0x86:
          DEBUG ((DEBUG_INFO, "Access Denied\n"));
          break;
      }
      break;

    default:
      DEBUG ((DEBUG_INFO, "Unknown error\n"));
      break;
  }

  return EFI_DEVICE_ERROR;
}

/**
  Create PRP lists for Data transfer which is larger than 2 memory pages.
  Note here we calcuate the number of required PRP lists and allocate them at one time.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in] SqId                   - The SQ index for this PRP
  @param[in] PhysicalAddr           - The physical base address of Data Buffer.
  @param[in] Pages                  - The number of pages to be transfered.
  @param[out] PrpListHost           - The host base address of PRP lists.
  @param[in,out] PrpListNo          - The number of PRP List.

  @retval The pointer Value to the first PRP List of the PRP lists.

**/
STATIC
UINT64
NvmeCreatePrpList (
  IN     NVME_CONTEXT                 *Nvme,
  IN     UINT16                       SqId,
  IN     EFI_PHYSICAL_ADDRESS         PhysicalAddr,
  IN     UINTN                        Pages,
     OUT VOID                         **PrpListHost,
  IN OUT UINTN                        *PrpListNo
  )
{
  UINTN                       PrpEntryNo;
  UINT64                      PrpListBase;
  UINTN                       PrpListIndex;
  UINTN                       PrpEntryIndex;
  UINT64                      Remainder;
  EFI_PHYSICAL_ADDRESS        PrpListPhyAddr;
  UINTN                       Bytes;
  UINT8                       *PrpEntry;
  EFI_PHYSICAL_ADDRESS        NewPhyAddr;

  ///
  /// The number of Prp Entry in a memory page.
  ///
  PrpEntryNo = EFI_PAGE_SIZE / sizeof (UINT64);

  ///
  /// Calculate total PrpList number.
  ///
  *PrpListNo = (UINTN) DivU64x64Remainder ((UINT64)Pages, (UINT64)PrpEntryNo, &Remainder);
  if (Remainder != 0) {
    *PrpListNo += 1;
  }

  if (*PrpListNo > NVME_PRP_SIZE) {
    DEBUG ((DEBUG_INFO, "NvmeCreatePrpList (PhysicalAddr: %lx, Pages: %x) PrpEntryNo: %x\n",
      PhysicalAddr, Pages, PrpEntryNo));
    DEBUG ((DEBUG_INFO, "*PrpListNo: %x, Remainder: %lx", *PrpListNo, Remainder));
    ASSERT (FALSE);
  }
  *PrpListHost = (VOID *)(UINTN) NVME_PRP_BASE (Nvme, SqId);

  Bytes = EFI_PAGES_TO_SIZE (*PrpListNo);
  PrpListPhyAddr = (UINT64)(UINTN)(*PrpListHost);

  ///
  /// Fill all PRP lists except of last one.
  ///
  ZeroMem (*PrpListHost, Bytes);
  for (PrpListIndex = 0; PrpListIndex < *PrpListNo - 1; ++PrpListIndex) {
    PrpListBase = *(UINT64*)PrpListHost + PrpListIndex * EFI_PAGE_SIZE;

    for (PrpEntryIndex = 0; PrpEntryIndex < PrpEntryNo; ++PrpEntryIndex) {
      PrpEntry = (UINT8 *)(UINTN) (PrpListBase + PrpEntryIndex * sizeof(UINT64));
      if (PrpEntryIndex != PrpEntryNo - 1) {
        ///
        /// Fill all PRP entries except of last one.
        ///
        CopyMem (PrpEntry, (VOID *)(UINTN) (&PhysicalAddr), sizeof (UINT64));
        PhysicalAddr += EFI_PAGE_SIZE;
      } else {
        ///
        /// Fill last PRP entries with next PRP List pointer.
        ///
        NewPhyAddr = (PrpListPhyAddr + (PrpListIndex + 1) * EFI_PAGE_SIZE);
        CopyMem (PrpEntry, (VOID *)(UINTN) (&NewPhyAddr), sizeof (UINT64));
      }
    }
  }

  ///
  /// Fill last PRP list.
  ///
  PrpListBase = *(UINT64*)PrpListHost + PrpListIndex * EFI_PAGE_SIZE;
  for (PrpEntryIndex = 0; PrpEntryIndex < ((Remainder != 0) ? Remainder : PrpEntryNo); ++PrpEntryIndex) {
    PrpEntry = (UINT8 *)(UINTN) (PrpListBase + PrpEntryIndex * sizeof(UINT64));
    CopyMem (PrpEntry, (VOID *)(UINTN) (&PhysicalAddr), sizeof (UINT64));

    PhysicalAddr += EFI_PAGE_SIZE;
  }

  return PrpListPhyAddr;
}

/**
  Check whether there are available command slots.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in] Qid                    - Queue index

  @retval EFI_SUCCESS               - Available command slot is found
  @retval EFI_NOT_READY             - No available command slot is found
  @retval EFI_DEVICE_ERROR          - Error occurred on device side.

**/
EFI_STATUS
NvmeHasFreeCmdSlot (
  IN NVME_CONTEXT       *Nvme,
  IN UINT8              Qid
  )
{
  return TRUE;
}

/**
  Check whether all command slots are clean.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in] Qid                    - Queue index

  @retval EFI_SUCCESS               - All command slots are clean
  @retval EFI_NOT_READY             - Not all command slots are clean
  @retval EFI_DEVICE_ERROR          - Error occurred on device side.

**/
EFI_STATUS
NvmeIsAllCmdSlotClean (
  IN NVME_CONTEXT       *Nvme,
  IN UINT8              Qid
  )
{
  return EFI_SUCCESS;
}

/**
  Waits until all NVME commands completed.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in] Qid                    - Queue index

  @retval EFI_SUCCESS               - All NVME commands have completed
  @retval EFI_TIMEOUT               - Timeout occured
  @retval EFI_NOT_READY             - Not all NVME commands have completed
  @retval others                    - Error occurred on device side.
**/
EFI_STATUS
NvmeWaitAllComplete (
  IN NVME_CONTEXT       *Nvme,
  IN UINT8              Qid
  )
{
  return EFI_SUCCESS;
}

/**
  Sends an NVM Express Command Packet to an NVM Express controller or namespace. This function supports
  both blocking I/O and nonblocking I/O. The blocking I/O functionality is required, and the nonblocking
  I/O functionality is optional.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in] NamespaceId            - Is a 32 bit Namespace ID to which the Express HCI command packet will be sent.
                                      A Value of 0 denotes the NVM Express controller, a Value of all 0FFh in the namespace
                                      ID specifies that the command packet should be sent to all valid namespaces.
  @param[in] NamespaceUuid          - Is a 64 bit Namespace UUID to which the Express HCI command packet will be sent.
                                      A Value of 0 denotes the NVM Express controller, a Value of all 0FFh in the namespace
                                      UUID specifies that the command packet should be sent to all valid namespaces.
  @param[in,out] Packet             - A pointer to the NVM Express HCI Command Packet to send to the NVMe namespace specified
                                      by NamespaceId.

  @retval EFI_SUCCESS               - The NVM Express Command Packet was sent by the host. TransferLength bytes were transferred
                                      to, or from DataBuffer.
  @retval EFI_NOT_READY             - The NVM Express Command Packet could not be sent because the controller is not ready. The caller
                                      may retry again later.
  @retval EFI_DEVICE_ERROR          - A device error occurred while attempting to send the NVM Express Command Packet.
  @retval EFI_INVALID_PARAMETER     - Namespace, or the contents of NVM_EXPRESS_PASS_THRU_COMMAND_PACKET are invalid. The NVM
                                      Express Command Packet was not sent, so no additional status information is available.
  @retval EFI_UNSUPPORTED           - The command described by the NVM Express Command Packet is not supported by the host adapter.
                                      The NVM Express Command Packet was not sent, so no additional status information is available.
  @retval EFI_TIMEOUT               - A timeout occurred while waiting for the NVM Express Command Packet to execute.

**/
EFI_STATUS
NvmePassThru (
  IN     NVME_CONTEXT                         *Nvme,
  IN     UINT32                               NamespaceId,
  IN     UINT64                               NamespaceUuid,
  IN OUT NVM_EXPRESS_PASS_THRU_COMMAND_PACKET *Packet
  )
{
  EFI_STATUS                    Status;
  NVME_SQ                       *Sq;
  NVME_CQ                       *Cq;
  UINT8                         Qid;
  UINT32                        Bytes;
  UINT32                        Offset;
  EFI_PHYSICAL_ADDRESS          PhyAddr;
  VOID                          *PrpListHost;
  UINTN                         PrpListNo;
  UINT32                        Timer;
  UINTN SqSize;
  UINTN CqSize;

  ///
  /// check the Data fields in Packet parameter.
  ///
  if ((Nvme == NULL) || (Packet == NULL)) {
    DEBUG ((DEBUG_ERROR, "NvmePassThru, invalid parameter: Nvme(%x)/Packet(%x)\n",
      (UINTN)Nvme, (UINTN)Packet));
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet->NvmeCmd == NULL) || (Packet->NvmeResponse == NULL)) {
    DEBUG ((DEBUG_ERROR, "NvmePassThru, invalid parameter: NvmeCmd(%x)/NvmeResponse(%x)\n",
      (UINTN)Packet->NvmeCmd, (UINTN)Packet->NvmeResponse));
    return EFI_INVALID_PARAMETER;
  }

  if (Packet->QueueId != NVME_ADMIN_QUEUE && Packet->QueueId != NVME_IO_QUEUE) {
    DEBUG ((DEBUG_ERROR, "NvmePassThru, invalid parameter: QueueId(%x)\n",
      Packet->QueueId));
    return EFI_INVALID_PARAMETER;
  }

  PrpListHost = NULL;
  PrpListNo   = 0;
  Status      = EFI_SUCCESS;

  Qid = Packet->QueueId;
  Sq  = Nvme->SqBuffer[Qid] + Nvme->SqTdbl[Qid].Sqt;
  Cq  = Nvme->CqBuffer[Qid] + Nvme->CqHdbl[Qid].Cqh;
  if (Qid == NVME_ADMIN_QUEUE) {
    SqSize = NVME_ASQ_SIZE + 1;
    CqSize = NVME_ACQ_SIZE + 1;
  } else {
    SqSize = NVME_CSQ_DEPTH;
    CqSize = NVME_CCQ_DEPTH;
  }

  if (Packet->NvmeCmd->Nsid != NamespaceId) {
    DEBUG ((DEBUG_ERROR, "NvmePassThru: Nsid mismatch (%x, %x)\n",
      Packet->NvmeCmd->Nsid, NamespaceId));
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Sq, sizeof (NVME_SQ));
  Sq->Opc  = Packet->NvmeCmd->Cdw0.Opcode;
  Sq->Fuse = Packet->NvmeCmd->Cdw0.FusedOperation;
  Sq->Cid  = Packet->NvmeCmd->Cdw0.Cid;
  Sq->Nsid = Packet->NvmeCmd->Nsid;

  ///
  /// Currently we only support PRP for Data transfer, SGL is NOT supported.
  ///
  ASSERT (Sq->Psdt == 0);
  if (Sq->Psdt != 0) {
    DEBUG ((DEBUG_ERROR, "NvmePassThru: doesn't support SGL mechanism\n"));
    return EFI_UNSUPPORTED;
  }

  Sq->Prp[0] = Packet->TransferBuffer;
  Sq->Prp[1] = 0;

  if(Packet->MetadataBuffer != (UINT64)(UINTN)NULL) {
    Sq->Mptr = Packet->MetadataBuffer;
  }

  ///
  /// If the Buffer Size spans more than two memory pages (page Size as defined in CC.Mps),
  /// then build a PRP list in the second PRP submission queue entry.
  ///
  Offset = ((UINT32)Sq->Prp[0]) & (EFI_PAGE_SIZE - 1);
  Bytes  = Packet->TransferLength;

  if ((Offset + Bytes) > (EFI_PAGE_SIZE * 2)) {
    ///
    /// Create PrpList for remaining Data Buffer.
    ///
    PhyAddr = (Sq->Prp[0] + EFI_PAGE_SIZE) & ~(EFI_PAGE_SIZE - 1);
    Sq->Prp[1] = NvmeCreatePrpList (Nvme, Nvme->SqTdbl[Qid].Sqt, PhyAddr, EFI_SIZE_TO_PAGES(Offset + Bytes) - 1, &PrpListHost, &PrpListNo);
    if (Sq->Prp[1] == 0) {
      Status = EFI_OUT_OF_RESOURCES;
      DEBUG ((DEBUG_ERROR, "NvmeCreatePrpList fail, Status: %r\n", Status));
      goto EXIT;
    }

  } else if ((Offset + Bytes) > EFI_PAGE_SIZE) {
    Sq->Prp[1] = (Sq->Prp[0] + EFI_PAGE_SIZE) & ~(EFI_PAGE_SIZE - 1);
  }

  if(Packet->NvmeCmd->Flags & CDW10_VALID) {
    Sq->Payload.Raw.Cdw10 = Packet->NvmeCmd->Cdw10;
  }
  if(Packet->NvmeCmd->Flags & CDW11_VALID) {
    Sq->Payload.Raw.Cdw11 = Packet->NvmeCmd->Cdw11;
  }
  if(Packet->NvmeCmd->Flags & CDW12_VALID) {
    Sq->Payload.Raw.Cdw12 = Packet->NvmeCmd->Cdw12;
  }
  if(Packet->NvmeCmd->Flags & CDW13_VALID) {
    Sq->Payload.Raw.Cdw13 = Packet->NvmeCmd->Cdw13;
  }
  if(Packet->NvmeCmd->Flags & CDW14_VALID) {
    Sq->Payload.Raw.Cdw14 = Packet->NvmeCmd->Cdw14;
  }
  if(Packet->NvmeCmd->Flags & CDW15_VALID) {
    Sq->Payload.Raw.Cdw15 = Packet->NvmeCmd->Cdw15;
  }

  ///
  /// Ring the submission queue doorbell.
  ///
  Nvme->SqTdbl[Qid].Sqt++;
  if(Nvme->SqTdbl[Qid].Sqt == SqSize) {
    Nvme->SqTdbl[Qid].Sqt = 0;
  }
  Status = NVME_SET_SQTDBL (Nvme, Qid, &Nvme->SqTdbl[Qid]);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "NVME_SET_SQTDBL fail, Status: %r\n", Status));
    goto EXIT;
  }

  ///
  /// Wait for completion queue to get filled in.
  ///
  Status = EFI_TIMEOUT;
  Timer   = 0;
  while (Timer < NVME_CMD_TIMEOUT) {
    //DEBUG ((DEBUG_VERBOSE, "Timer: %x, Cq:\n", Timer));
    //DumpMem (Cq, sizeof (NVME_CQ));
    if (Cq->Pt != Nvme->Pt[Qid]) {
      Status = EFI_SUCCESS;
      break;
    }

    MicroSecondDelay (NVME_CMD_WAIT);
    Timer += NVME_CMD_WAIT;
  }

  Nvme->CqHdbl[Qid].Cqh++;
  if (Nvme->CqHdbl[Qid].Cqh == CqSize) {
    Nvme->CqHdbl[Qid].Cqh = 0;
    Nvme->Pt[Qid] ^= 1;
  }

  ///
  /// Copy the Respose Queue entry for this command to the callers response Buffer
  ///
  CopyMem (Packet->NvmeResponse, Cq, sizeof(NVM_EXPRESS_RESPONSE));

  if (!EFI_ERROR(Status)) { // We still need to check CQ status if no timeout error occured
    Status = NvmeCheckCqStatus (Cq);
  }
  NVME_SET_CQHDBL (Nvme, Qid, &Nvme->CqHdbl[Qid]);

EXIT:
  return Status;
}

/**
  Get identify controller Data.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in] Buffer                 - The Buffer used to store the identify controller Data.

  @return EFI_SUCCESS               - Successfully get the identify controller Data.
  @return others                    - Fail to get the identify controller Data.

**/
STATIC
EFI_STATUS
NvmeIdentifyController (
  IN NVME_CONTEXT                          *Nvme,
  IN VOID                                  *Buffer
  )
{
  NVM_EXPRESS_PASS_THRU_COMMAND_PACKET     CommandPacket;
  NVM_EXPRESS_COMMAND                      Command;
  NVM_EXPRESS_RESPONSE                     Response;
  EFI_STATUS                               Status;

  ZeroMem (&CommandPacket, sizeof(NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof(NVM_EXPRESS_COMMAND));
  ZeroMem (&Response, sizeof(NVM_EXPRESS_RESPONSE));

  Command.Cdw0.Opcode = NVME_ADMIN_IDENTIFY_OPC;
  Command.Cdw0.Cid    = Nvme->Cid[NVME_ADMIN_QUEUE]++;
  //
  // According to Nvm Express 1.1 spec Figure 38, When not used, the field shall be cleared to 0h.
  // For the Identify command, the Namespace Identifier is only used for the Namespace Data structure.
  //
  Command.Nsid        = 0;

  CommandPacket.NvmeCmd        = &Command;
  CommandPacket.NvmeResponse   = &Response;
  CommandPacket.TransferBuffer = (UINT64)(UINTN)Buffer;
  CommandPacket.TransferLength = sizeof (NVME_ADMIN_CONTROLLER_DATA);
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueId        = NVME_ADMIN_QUEUE;
  //
  // Set bit 0 (Cns bit) to 1 to identify a controller
  //
  Command.Cdw10                = 1;
  Command.Flags                = CDW10_VALID;

  Status = NvmePassThru (
              Nvme,
              NVME_CONTROLLER_ID,
              0,
              &CommandPacket
              );
  if (!EFI_ERROR (Status)) {
    Status = NvmeWaitAllComplete (Nvme, CommandPacket.QueueId);
  }

  return Status;
}

/**
  Get specified identify namespace Data.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in] NamespaceId            - The specified namespace identifier.
  @param[in] Buffer                 - The Buffer used to store the identify namespace Data.

  @return EFI_SUCCESS               - Successfully get the identify namespace Data.
  @return others                    - Fail to get the identify namespace Data.

**/
STATIC
EFI_STATUS
NvmeIdentifyNamespace (
  IN NVME_CONTEXT                          *Nvme,
  IN UINT32                                NamespaceId,
  IN VOID                                  *Buffer
  )
{
  NVM_EXPRESS_PASS_THRU_COMMAND_PACKET     CommandPacket;
  NVM_EXPRESS_COMMAND                      Command;
  NVM_EXPRESS_RESPONSE                     Response;
  EFI_STATUS                               Status;

  ZeroMem (&CommandPacket, sizeof(NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof(NVM_EXPRESS_COMMAND));
  ZeroMem (&Response, sizeof(NVM_EXPRESS_RESPONSE));

  CommandPacket.NvmeCmd      = &Command;
  CommandPacket.NvmeResponse = &Response;

  Command.Cdw0.Opcode = NVME_ADMIN_IDENTIFY_OPC;
  Command.Cdw0.Cid    = Nvme->Cid[NVME_ADMIN_QUEUE]++;
  Command.Nsid        = NamespaceId;
  CommandPacket.TransferBuffer = (UINT64)(UINTN)Buffer;
  CommandPacket.TransferLength = sizeof (NVME_ADMIN_NAMESPACE_DATA);
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueId        = NVME_ADMIN_QUEUE;
  //
  // Set bit 0 (Cns bit) to 1 to identify a namespace
  //
  CommandPacket.NvmeCmd->Cdw10 = 0;
  CommandPacket.NvmeCmd->Flags = CDW10_VALID;

  Status = NvmePassThru (
              Nvme,
              NamespaceId,
              0,
              &CommandPacket
              );
  if (!EFI_ERROR (Status)) {
    Status = NvmeWaitAllComplete (Nvme, CommandPacket.QueueId);
  }

  return Status;
}

/**
  Get Block Size for specific namespace of NVME.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.

  @return                           - Block Size in bytes

**/
STATIC
UINT32
NvmeGetBlockSize (
  IN NVME_CONTEXT       *Nvme
  )
{
  UINT32                BlockSize;
  UINT32                Lbads;
  UINT32                Flbas;
  UINT32                LbaFmtIdx;

  Flbas     = Nvme->NamespaceData->Flbas;
  LbaFmtIdx = Flbas & 3;
  Lbads     = Nvme->NamespaceData->LbaFormat[LbaFmtIdx].Lbads;

  BlockSize = (UINT32)1 << Lbads;
  return BlockSize;
}

/**
  Get last LBA for specific namespace of NVME.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.

  @return                           - Last LBA address

**/
STATIC
EFI_LBA
NvmeGetLastLba (
  IN NVME_CONTEXT       *Nvme
  )
{
  EFI_LBA               LastBlock;
  LastBlock = Nvme->NamespaceData->Nsze - 1;
  return LastBlock;
}

/**
  Create io completion queue.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.

  @return EFI_SUCCESS               - Successfully create io completion queue.
  @return others                    - Fail to create io completion queue.

**/
STATIC
EFI_STATUS
NvmeCreateIoCompletionQueue (
  IN     NVME_CONTEXT                      *Nvme
  )
{
  NVM_EXPRESS_PASS_THRU_COMMAND_PACKET     CommandPacket;
  NVM_EXPRESS_COMMAND                      Command;
  NVM_EXPRESS_RESPONSE                     Response;
  EFI_STATUS                               Status;
  NVME_ADMIN_CRIOCQ                        CrIoCq;

  ZeroMem (&CommandPacket, sizeof(NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof(NVM_EXPRESS_COMMAND));
  ZeroMem (&Response, sizeof(NVM_EXPRESS_RESPONSE));
  ZeroMem (&CrIoCq, sizeof(NVME_ADMIN_CRIOCQ));

  CommandPacket.NvmeCmd      = &Command;
  CommandPacket.NvmeResponse = &Response;

  Command.Cdw0.Opcode = NVME_ADMIN_CRIOCQ_OPC;
  Command.Cdw0.Cid    = Nvme->Cid[NVME_ADMIN_QUEUE]++;
  CommandPacket.TransferBuffer = (UINT64)(UINTN)Nvme->CqBuffer[NVME_IO_QUEUE];
  CommandPacket.TransferLength = EFI_PAGE_SIZE;
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueId        = NVME_ADMIN_QUEUE;

  CrIoCq.Qid   = NVME_IO_QUEUE;
  CrIoCq.Qsize = NVME_CCQ_SIZE;
  CrIoCq.Pc    = 1;
  CopyMem (&CommandPacket.NvmeCmd->Cdw10, &CrIoCq, sizeof (NVME_ADMIN_CRIOCQ));
  CommandPacket.NvmeCmd->Flags = CDW10_VALID | CDW11_VALID;

  Status = NvmePassThru (
              Nvme,
              NVME_CONTROLLER_ID,
              0,
              &CommandPacket
              );
  if (!EFI_ERROR (Status)) {
    Status = NvmeWaitAllComplete (Nvme, CommandPacket.QueueId);
  }

  return Status;
}

/**
  Create io submission queue.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.

  @return EFI_SUCCESS               - Successfully create io submission queue.
  @return others                    - Fail to create io submission queue.

**/
STATIC
EFI_STATUS
NvmeCreateIoSubmissionQueue (
  IN NVME_CONTEXT                          *Nvme
  )
{
  NVM_EXPRESS_PASS_THRU_COMMAND_PACKET     CommandPacket;
  NVM_EXPRESS_COMMAND                      Command;
  NVM_EXPRESS_RESPONSE                     Response;
  EFI_STATUS                               Status;
  NVME_ADMIN_CRIOSQ                        CrIoSq;

  ZeroMem (&CommandPacket, sizeof(NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof(NVM_EXPRESS_COMMAND));
  ZeroMem (&Response, sizeof(NVM_EXPRESS_RESPONSE));
  ZeroMem (&CrIoSq, sizeof(NVME_ADMIN_CRIOSQ));

  CommandPacket.NvmeCmd      = &Command;
  CommandPacket.NvmeResponse = &Response;

  Command.Cdw0.Opcode = NVME_ADMIN_CRIOSQ_OPC;
  Command.Cdw0.Cid    = Nvme->Cid[NVME_ADMIN_QUEUE]++;
  CommandPacket.TransferBuffer = (UINT64)(UINTN)Nvme->SqBuffer[NVME_IO_QUEUE];
  CommandPacket.TransferLength = EFI_PAGE_SIZE;
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueId        = NVME_ADMIN_QUEUE;

  CrIoSq.Qid   = NVME_IO_QUEUE;
  CrIoSq.Qsize = NVME_CSQ_SIZE;
  CrIoSq.Pc    = 1;
  CrIoSq.Cqid  = NVME_IO_QUEUE;
  CrIoSq.Qprio = 0;
  CopyMem (&CommandPacket.NvmeCmd->Cdw10, &CrIoSq, sizeof (NVME_ADMIN_CRIOSQ));
  CommandPacket.NvmeCmd->Flags = CDW10_VALID | CDW11_VALID;

  Status = NvmePassThru (
              Nvme,
              NVME_CONTROLLER_ID,
              0,
              &CommandPacket
              );
  if (!EFI_ERROR (Status)) {
    Status = NvmeWaitAllComplete (Nvme, CommandPacket.QueueId);
  }

  return Status;
}

/**
  Security send and receive commands.

  @param[in]     Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in]     SendCommand            - The flag to indicate the command type, TRUE for Send command and FALSE for receive command
  @param[in]     SecurityProtocol       - Security Protocol
  @param[in]     SpSpecific             - Security Protocol Specific
  @param[in]     TransferLength         - Transfer Length of Buffer (in bytes) - always a multiple of 512
  @param[in,out] TransferBuffer         - Address of Data to transfer

  @return EFI_SUCCESS               - Successfully create io submission queue.
  @return others                    - Fail to send/receive commands.

**/
EFI_STATUS
NvmeSecuritySendReceive (
  IN NVME_CONTEXT                          *Nvme,
  IN BOOLEAN                               SendCommand,
  IN UINT8                                 SecurityProtocol,
  IN UINT16                                SpSpecific,
  IN UINTN                                 TransferLength,
  IN OUT VOID                              *TransferBuffer
  )
{
  NVM_EXPRESS_PASS_THRU_COMMAND_PACKET     CommandPacket;
  NVM_EXPRESS_COMMAND                      Command;
  NVM_EXPRESS_RESPONSE                     Response;
  EFI_STATUS                               Status;
  NVME_ADMIN_SECSEND                       SecSend;
  OACS                                     *Oacs;
  UINT8                                    Opcode;
  VOID*                                    *SecBuff;

  Oacs = (OACS *)&Nvme->ControllerData->Oacs;

  //
  // Verify security bit for Security Send/Receive commands
  //
  if (Oacs->Security == 0) {
    DEBUG ((DEBUG_ERROR, "Security command doesn't support.\n"));
    return EFI_NOT_READY;
  }

  SecBuff = (VOID *)(UINTN) NVME_SEC_BASE (Nvme);

  //
  // Actions for sending security command
  //
  if (SendCommand) {
    CopyMem (SecBuff, TransferBuffer, TransferLength);
  }

  ZeroMem (&CommandPacket, sizeof(NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof(NVM_EXPRESS_COMMAND));
  ZeroMem (&Response, sizeof(NVM_EXPRESS_RESPONSE));
  ZeroMem (&SecSend, sizeof(NVME_ADMIN_SECSEND));

  CommandPacket.NvmeCmd      = &Command;
  CommandPacket.NvmeResponse = &Response;

  Opcode = (UINT8)(SendCommand ? NVME_ADMIN_SECURITY_SEND_OPC : NVME_ADMIN_SECURITY_RECV_OPC);
  Command.Cdw0.Opcode = Opcode;
  Command.Cdw0.Cid    = Nvme->Cid[NVME_ADMIN_QUEUE]++;
  CommandPacket.TransferBuffer = (UINT64)(UINTN)SecBuff;
  CommandPacket.TransferLength = (UINT32)TransferLength;
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueId        = NVME_ADMIN_QUEUE;

  SecSend.Spsp = SpSpecific;
  SecSend.Secp = SecurityProtocol;
  SecSend.Tl   = (UINT32)TransferLength;

  CopyMem (&CommandPacket.NvmeCmd->Cdw10, &SecSend, sizeof (NVME_ADMIN_SECSEND));
  CommandPacket.NvmeCmd->Flags = CDW10_VALID | CDW11_VALID;

  Status = NvmePassThru (
              Nvme,
              NVME_CONTROLLER_ID,
              0,
              &CommandPacket
              );
  if (!EFI_ERROR (Status)) {
    Status = NvmeWaitAllComplete (Nvme, CommandPacket.QueueId);
  }

  //
  // Actions for receiving security command
  //
  if (!SendCommand) {
    CopyMem (TransferBuffer, SecBuff, TransferLength);
  }

  return Status;
}

/**
  Destroy io completion queue.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.

  @return EFI_SUCCESS               - Successfully destroy io completion queue.
  @return others                    - Fail to destroy io completion queue.

**/
STATIC
EFI_STATUS
NvmeDestroyIoCompletionQueue (
  IN     NVME_CONTEXT                      *Nvme
  )
{
  NVM_EXPRESS_PASS_THRU_COMMAND_PACKET     CommandPacket;
  NVM_EXPRESS_COMMAND                      Command;
  NVM_EXPRESS_RESPONSE                     Response;
  EFI_STATUS                               Status;
  NVME_ADMIN_DEIOCQ                        DelIoCq;

  ZeroMem (&CommandPacket, sizeof(NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof(NVM_EXPRESS_COMMAND));
  ZeroMem (&Response, sizeof(NVM_EXPRESS_RESPONSE));
  ZeroMem (&DelIoCq, sizeof(NVME_ADMIN_DEIOCQ));

  CommandPacket.NvmeCmd      = &Command;
  CommandPacket.NvmeResponse = &Response;

  Command.Cdw0.Opcode = NVME_ADMIN_DELIOCQ_OPC;
  Command.Cdw0.Cid    = Nvme->Cid[NVME_ADMIN_QUEUE]++;
  CommandPacket.TransferBuffer = (UINT64)(UINTN)Nvme->CqBuffer[NVME_IO_QUEUE];
  CommandPacket.TransferLength = EFI_PAGE_SIZE;
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueId        = NVME_ADMIN_QUEUE;

  DelIoCq.Qid   = NVME_IO_QUEUE;
  CopyMem (&CommandPacket.NvmeCmd->Cdw10, &DelIoCq, sizeof (NVME_ADMIN_DEIOCQ));
  CommandPacket.NvmeCmd->Flags = CDW10_VALID | CDW11_VALID;

  Status = NvmePassThru (
              Nvme,
              NVME_CONTROLLER_ID,
              0,
              &CommandPacket
              );
  if (!EFI_ERROR (Status)) {
    Status = NvmeWaitAllComplete (Nvme, CommandPacket.QueueId);
  }

  return Status;
}

/**
  Destroy io submission queue.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.

  @return EFI_SUCCESS               - Successfully destroy io submission queue.
  @return others                    - Fail to destroy io submission queue.

**/
STATIC
EFI_STATUS
NvmeDestroyIoSubmissionQueue (
  IN NVME_CONTEXT                          *Nvme
  )
{
  NVM_EXPRESS_PASS_THRU_COMMAND_PACKET     CommandPacket;
  NVM_EXPRESS_COMMAND                      Command;
  NVM_EXPRESS_RESPONSE                     Response;
  EFI_STATUS                               Status;
  NVME_ADMIN_DEIOSQ                        DelIoSq;

  ZeroMem (&CommandPacket, sizeof(NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof(NVM_EXPRESS_COMMAND));
  ZeroMem (&Response, sizeof(NVM_EXPRESS_RESPONSE));
  ZeroMem (&DelIoSq, sizeof(NVME_ADMIN_DEIOSQ));

  CommandPacket.NvmeCmd      = &Command;
  CommandPacket.NvmeResponse = &Response;

  Command.Cdw0.Opcode = NVME_ADMIN_DELIOSQ_OPC;
  Command.Cdw0.Cid    = Nvme->Cid[NVME_ADMIN_QUEUE]++;
  CommandPacket.TransferBuffer = (UINT64)(UINTN)Nvme->SqBuffer[NVME_IO_QUEUE];
  CommandPacket.TransferLength = EFI_PAGE_SIZE;
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueId        = NVME_ADMIN_QUEUE;

  DelIoSq.Qid   = NVME_IO_QUEUE;
  CopyMem (&CommandPacket.NvmeCmd->Cdw10, &DelIoSq, sizeof (NVME_ADMIN_DEIOSQ));
  CommandPacket.NvmeCmd->Flags = CDW10_VALID | CDW11_VALID;

  Status = NvmePassThru (
              Nvme,
              NVME_CONTROLLER_ID,
              0,
              &CommandPacket
              );
  if (!EFI_ERROR (Status)) {
    Status = NvmeWaitAllComplete (Nvme, CommandPacket.QueueId);
  }

  return Status;
}

/**
  Allocate transfer-related Data struct which is used at Nvme.

  @param[in] ImageHandle         Image handle for this driver image
  @param[in] Nvme                The pointer to the NVME_CONTEXT Data structure.

  @retval  EFI_OUT_OF_RESOURCE   The allocation is failure.
  @retval  EFI_SUCCESS           Successful to allocate memory.

**/
EFI_STATUS
EFIAPI
NvmeAllocateResource (
  IN EFI_HANDLE                         ImageHandle,
  IN NVME_CONTEXT                       *Nvme
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Addr;
  UINT32                Size;

  //
  // Allocate resources required by NVMe host controller.
  //
  // NBAR
  Size = 0x10000;
  Addr = 0xFFFFFFFF;
  Status = gDS->AllocateMemorySpace (
                  EfiGcdAllocateMaxAddressSearchBottomUp,
                  EfiGcdMemoryTypeMemoryMappedIo,
                  15,                             // 2^15: 32K Alignment
                  Size,
                  &Addr,
                  ImageHandle,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }
  Nvme->Nbar = (UINT32) Addr;

  // DMA Buffer
  Size = NVME_MEM_MAX_SIZE;
  Addr = 0xFFFFFFFF;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES (Size),
                  (EFI_PHYSICAL_ADDRESS *)&Addr
                  );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }
  Nvme->BaseMem = (UINT32) Addr;

  // Clean up DMA Buffer before using
  ZeroMem ((VOID *)(UINTN)Addr, NVME_MEM_MAX_SIZE);

  return EFI_SUCCESS;
}

/**
  Free allocated transfer-related Data struct which is used at NVMe.

  @param[in] Nvme                The pointer to the NVME_CONTEXT Data structure.

**/
VOID
EFIAPI
NvmeFreeResource (
  IN NVME_CONTEXT                       *Nvme
  )
{
  UINT32                Size;

  // NBAR
  if (Nvme->BaseMem != 0) {
    Size = 0x10000;
    gDS->FreeMemorySpace (Nvme->Nbar, Size);
  }

  // DMA Buffer
  if (Nvme->Nbar != 0) {
    Size = NVME_MEM_MAX_SIZE;
    gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN) Nvme->Nbar, EFI_SIZE_TO_PAGES (Size));
  }
}


/**
  Initialize the Nvm Express controller.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.

  @retval EFI_SUCCESS               - The NVM Express Controller is initialized successfully.
  @retval Others                    - A device error occurred while initializing the controller.

**/
EFI_STATUS
NvmeControllerInit (
  IN NVME_CONTEXT       *Nvme
  )
{
  EFI_STATUS            Status;
  NVME_AQA              Aqa;
  NVME_ASQ              Asq;
  NVME_ACQ              Acq;
  NVME_VER              Ver;

  UINT32                MlBAR;
  UINT32                MuBAR;

  ///
  /// Update PCIE BAR0/1 for NVME device
  ///
  MlBAR = Nvme->Nbar;
  MuBAR = 0;
  PciWrite32 (Nvme->PciBase + 0x10, MlBAR); // MLBAR (BAR0)
  PciWrite32 (Nvme->PciBase + 0x14, MuBAR); // MUBAR (BAR1)

  ///
  /// Enable PCIE decode
  ///
  PciWrite8 (Nvme->PciBase + NVME_PCIE_PCICMD, 0x6);

  // Version
  NVME_GET_VER (Nvme, &Ver);
  if (!(Ver.Mjr == 0x0001) && (Ver.Mnr == 0x0000)) {
    DEBUG ((DEBUG_INFO, "\n!!!\n!!! NVME Version mismatch for the implementation !!!\n!!!\n"));
  }

  ///
  /// Read the Controller Capabilities register and verify that the NVM command set is supported
  ///
  Status = NVME_GET_CAP (Nvme, &Nvme->Cap);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "NVME_GET_CAP fail, Status: %r\n", Status));
    goto Done;
  }

  if (Nvme->Cap.Css != 0x01) {
    DEBUG ((DEBUG_ERROR, "NvmeControllerInit fail: the controller doesn't support NVMe command set\n"));
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  ///
  /// Currently the driver only supports 4k page Size.
  ///
  if ((Nvme->Cap.Mpsmin + 12) > EFI_PAGE_SHIFT) {
    DEBUG ((DEBUG_ERROR, "NvmeControllerInit fail: only supports 4k page Size\n"));
    ASSERT (FALSE);
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Nvme->Cid[0] = 0;
  Nvme->Cid[1] = 0;

  Nvme->Pt[0]  = 0;
  Nvme->Pt[1]  = 0;

  ZeroMem ((VOID *)(UINTN)(&(Nvme->SqTdbl[0])), sizeof (NVME_SQTDBL) * NVME_MAX_IO_QUEUES);
  ZeroMem ((VOID *)(UINTN)(&(Nvme->CqHdbl[0])), sizeof (NVME_CQHDBL) * NVME_MAX_IO_QUEUES);

  ZeroMem ((VOID *)(UINTN)Nvme->BaseMem, NVME_MEM_MAX_SIZE);

  Status = NvmeDisableController (Nvme);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "NvmeDisableController fail, Status: %r\n", Status));
    goto Done;
  }

  ///
  /// set number of entries admin submission & completion queues.
  ///
  Aqa.Asqs  = NVME_ASQ_SIZE;
  Aqa.Rsvd1 = 0;
  Aqa.Acqs  = NVME_ACQ_SIZE;
  Aqa.Rsvd2 = 0;

  ///
  /// Address of admin submission queue.
  ///
  Asq = (UINT64)(UINTN)(NVME_ASQ_BASE (Nvme) & ~0xFFF);

  ///
  /// Address of admin completion queue.
  ///
  Acq = (UINT64)(UINTN)(NVME_ACQ_BASE (Nvme) & ~0xFFF);

  ///
  /// Address of I/O submission & completion queue.
  ///
  Nvme->SqBuffer[0] = (NVME_SQ *)(UINTN)NVME_ASQ_BASE (Nvme);   // NVME_ADMIN_QUEUE
  Nvme->CqBuffer[0] = (NVME_CQ *)(UINTN)NVME_ACQ_BASE (Nvme);   // NVME_ADMIN_QUEUE
  Nvme->SqBuffer[1] = (NVME_SQ *)(UINTN)NVME_SQ_BASE (Nvme, 0); // NVME_IO_QUEUE
  Nvme->CqBuffer[1] = (NVME_CQ *)(UINTN)NVME_CQ_BASE (Nvme, 0); // NVME_IO_QUEUE

  DEBUG ((DEBUG_INFO, "BaseMem = [%08X]\n", Nvme->BaseMem));
  DEBUG ((DEBUG_INFO, "Admin Submission Queue Size (Aqa.Asqs) = [%08X]\n", Aqa.Asqs));
  DEBUG ((DEBUG_INFO, "Admin Completion Queue Size (Aqa.Acqs) = [%08X]\n", Aqa.Acqs));
  DEBUG ((DEBUG_INFO, "Admin Submission Queue (SqBuffer[0]) =   [%08X]\n", Nvme->SqBuffer[0]));
  DEBUG ((DEBUG_INFO, "Admin Completion Queue (CqBuffer[0]) =   [%08X]\n", Nvme->CqBuffer[0]));
  DEBUG ((DEBUG_INFO, "I/O   Submission Queue (SqBuffer[1]) =   [%08X]\n", Nvme->SqBuffer[1]));
  DEBUG ((DEBUG_INFO, "I/O   Completion Queue (CqBuffer[1]) =   [%08X]\n", Nvme->CqBuffer[1]));

  ///
  /// Program admin queue attributes.
  ///
  Status = NVME_SET_AQA (Nvme, &Aqa);
  if (EFI_ERROR(Status)) {
    goto Done;
  }

  ///
  /// Program admin submission queue address.
  ///
  Status = NVME_SET_ASQ (Nvme, &Asq);
  if (EFI_ERROR(Status)) {
    goto Done;
  }

  ///
  /// Program admin completion queue address.
  ///
  Status = NVME_SET_ACQ (Nvme, &Acq);
  if (EFI_ERROR(Status)) {
    goto Done;
  }

  Status = NvmeEnableController (Nvme);
  if (EFI_ERROR(Status)) {
    goto Done;
  }

  ///
  /// Create one I/O completion queue.
  ///
  Status = NvmeCreateIoCompletionQueue (Nvme);
  if (EFI_ERROR(Status)) {
    goto Done;
  }

  ///
  /// Create one I/O Submission queue.
  ///
  Status = NvmeCreateIoSubmissionQueue (Nvme);
  if (EFI_ERROR(Status)) {
    goto Done;
  }

  ///
  /// Get current Identify Controller Data
  ///
  Nvme->ControllerData = (NVME_ADMIN_CONTROLLER_DATA *)(UINTN) NVME_CONTROL_DATA_BASE (Nvme);
  Status = NvmeIdentifyController (Nvme, Nvme->ControllerData);
  if (EFI_ERROR(Status)) {
    goto Done;
  }

  ///
  /// Dump NvmExpress Identify Controller Data
  ///
  Nvme->ControllerData->Sn[19] = 0;
  Nvme->ControllerData->Mn[39] = 0;
  //NvmeDumpIdentifyController (Nvme->ControllerData);

  ///
  /// Get current Identify Namespace Data
  ///
  Nvme->NamespaceData = (NVME_ADMIN_NAMESPACE_DATA *)NVME_NAMESPACE_DATA_BASE (Nvme);
  Status = NvmeIdentifyNamespace (Nvme, Nvme->Nsid, Nvme->NamespaceData);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "NvmeIdentifyNamespace fail, Status = %r\n", Status));
    goto Done;
  }

  ///
  /// Dump NvmExpress Identify Namespace Data
  ///
  if (Nvme->NamespaceData->Ncap == 0) {
    DEBUG ((DEBUG_ERROR, "Invalid Namespace, Ncap: %lx\n", Nvme->NamespaceData->Ncap));
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  Nvme->BlockSize = NvmeGetBlockSize (Nvme);
  Nvme->LastBlock = NvmeGetLastLba (Nvme);

  Nvme->State    = NvmeStatusInit;

  return EFI_SUCCESS;

Done:
  return Status;
}

/**
  Un-initialize the Nvm Express controller.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.

  @retval EFI_SUCCESS               - The NVM Express Controller is un-initialized successfully.
  @retval Others                    - A device error occurred while un-initializing the controller.

**/
EFI_STATUS
NvmeControllerExit (
  IN NVME_CONTEXT       *Nvme
  )
{
  EFI_STATUS            Status;

  Status = EFI_SUCCESS;
  if (Nvme->State == NvmeStatusInit || Nvme->State == NvmeStatusMax) {
    ///
    /// Destroy I/O Submission queue.
    ///
    Status = NvmeDestroyIoSubmissionQueue (Nvme);
    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_ERROR, "NvmeDestroyIoSubmissionQueue fail, Status = %r\n", Status));
      return Status;
    }

    ///
    /// Destroy I/O completion queue.
    ///
    Status = NvmeDestroyIoCompletionQueue (Nvme);
    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_ERROR, "NvmeDestroyIoCompletionQueue fail, Status = %r\n", Status));
      return Status;
    }

    Status = NvmeShutdownController (Nvme);
    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_ERROR, "NvmeShutdownController fail, Status: %r\n", Status));
    }
  }

  ///
  /// Disable PCIE decode
  ///
  PciWrite8  (Nvme->PciBase + NVME_PCIE_PCICMD, 0x0);
  PciWrite32 (Nvme->PciBase + 0x10, 0); // MLBAR (BAR0)
  PciWrite32 (Nvme->PciBase + 0x14, 0); // MUBAR (BAR1)

  Nvme->State = NvmeStatusUnknown;
  return Status;
}

/**
  Read sector Data from the NVMe device.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in,out] Buffer             - The Buffer used to store the Data read from the device.
  @param[in] Lba                    - The start block number.
  @param[in] Blocks                 - Total block number to be read.

  @retval EFI_SUCCESS               - Datum are read from the device.
  @retval Others                    - Fail to read all the datum.

**/
EFI_STATUS
NvmeReadSectors (
  IN NVME_CONTEXT                          *Nvme,
  IN OUT UINT64                            Buffer,
  IN UINT64                                Lba,
  IN UINT32                                Blocks
  )
{
  UINT32                                   Bytes;
  NVM_EXPRESS_PASS_THRU_COMMAND_PACKET     CommandPacket;
  NVM_EXPRESS_COMMAND                      Command;
  NVM_EXPRESS_RESPONSE                     Response;
  EFI_STATUS                               Status;
  UINT32                                   BlockSize;

  BlockSize  = Nvme->BlockSize;
  Bytes      = Blocks * BlockSize;

  ZeroMem (&CommandPacket, sizeof(NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof(NVM_EXPRESS_COMMAND));
  ZeroMem (&Response, sizeof(NVM_EXPRESS_RESPONSE));

  CommandPacket.NvmeCmd      = &Command;
  CommandPacket.NvmeResponse = &Response;

  CommandPacket.NvmeCmd->Cdw0.Opcode = NVME_IO_READ_OPC;
  CommandPacket.NvmeCmd->Cdw0.Cid    = Nvme->Cid[NVME_IO_QUEUE]++;
  CommandPacket.NvmeCmd->Nsid        = Nvme->Nsid;
  CommandPacket.TransferBuffer       = Buffer;

  CommandPacket.TransferLength = Bytes;
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueId        = NVME_IO_QUEUE;

  CommandPacket.NvmeCmd->Cdw10 = (UINT32)Lba;
  CommandPacket.NvmeCmd->Cdw11 = (UINT32)(RShiftU64 (Lba, 32));
  CommandPacket.NvmeCmd->Cdw12 = (Blocks - 1) & 0xFFFF;

  CommandPacket.NvmeCmd->Flags = CDW10_VALID | CDW11_VALID | CDW12_VALID;

  Status = NvmePassThru (
              Nvme,
              Nvme->Nsid,
              0,
              &CommandPacket
              );

  return Status;
}

/**
  Write sector Data to the NVMe device.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in] Buffer                 - The Buffer to be written into the device.
  @param[in] Lba                    - The start block number.
  @param[in] Blocks                 - Total block number to be written.

  @retval EFI_SUCCESS               - Datum are written into the Buffer.
  @retval Others                    - Fail to write all the datum.

**/
EFI_STATUS
NvmeWriteSectors (
  IN NVME_CONTEXT                          *Nvme,
  IN UINT64                                Buffer,
  IN UINT64                                Lba,
  IN UINT32                                Blocks
  )
{
  NVM_EXPRESS_PASS_THRU_COMMAND_PACKET     CommandPacket;
  NVM_EXPRESS_COMMAND                      Command;
  NVM_EXPRESS_RESPONSE                     Response;
  EFI_STATUS                               Status;
  UINT32                                   Bytes;
  UINT32                                   BlockSize;

  BlockSize  = Nvme->BlockSize;
  Bytes      = Blocks * BlockSize;

  ZeroMem (&CommandPacket, sizeof(NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof(NVM_EXPRESS_COMMAND));
  ZeroMem (&Response, sizeof(NVM_EXPRESS_RESPONSE));

  CommandPacket.NvmeCmd      = &Command;
  CommandPacket.NvmeResponse = &Response;

  CommandPacket.NvmeCmd->Cdw0.Opcode = NVME_IO_WRITE_OPC;
  CommandPacket.NvmeCmd->Cdw0.Cid    = Nvme->Cid[NVME_IO_QUEUE]++;
  CommandPacket.NvmeCmd->Nsid  = Nvme->Nsid;
  CommandPacket.TransferBuffer = Buffer;

  CommandPacket.TransferLength = Bytes;
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueId        = NVME_IO_QUEUE;

  CommandPacket.NvmeCmd->Cdw10 = (UINT32)Lba;
  CommandPacket.NvmeCmd->Cdw11 = (UINT32)(RShiftU64 (Lba, 32));
  CommandPacket.NvmeCmd->Cdw12 = (Blocks - 1) & 0xFFFF;

  CommandPacket.MetadataBuffer = (UINT64)(UINTN)NULL;
  CommandPacket.MetadataLength = 0;

  CommandPacket.NvmeCmd->Flags = CDW10_VALID | CDW11_VALID | CDW12_VALID;

  Status = NvmePassThru (
              Nvme,
              Nvme->Nsid,
              0,
              &CommandPacket
              );

  return Status;
}

/**
  Flushes all modified Data to the device.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.

  @retval EFI_SUCCESS               - Datum are written into the Buffer.
  @retval Others                    - Fail to write all the datum.

**/
EFI_STATUS
NvmeFlush (
  IN NVME_CONTEXT                          *Nvme
  )
{
  NVM_EXPRESS_PASS_THRU_COMMAND_PACKET     CommandPacket;
  NVM_EXPRESS_COMMAND                      Command;
  NVM_EXPRESS_RESPONSE                     Response;
  EFI_STATUS                               Status;

  ZeroMem (&CommandPacket, sizeof(NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof(NVM_EXPRESS_COMMAND));
  ZeroMem (&Response, sizeof(NVM_EXPRESS_RESPONSE));

  CommandPacket.NvmeCmd      = &Command;
  CommandPacket.NvmeResponse = &Response;

  CommandPacket.NvmeCmd->Cdw0.Opcode = NVME_IO_FLUSH_OPC;
  CommandPacket.NvmeCmd->Cdw0.Cid    = Nvme->Cid[NVME_IO_QUEUE]++;
  CommandPacket.NvmeCmd->Nsid  = Nvme->Nsid;
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueId        = NVME_IO_QUEUE;

  Status = NvmePassThru (
              Nvme,
              Nvme->Nsid,
              0,
              &CommandPacket
              );
  if (!EFI_ERROR (Status)) {
    Status = NvmeWaitAllComplete (Nvme, CommandPacket.QueueId);
  }

  return Status;
}

/**
  Read some blocks from the device.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[out] Buffer                - The Buffer used to store the Data read from the device.
  @param[in] Lba                    - The start block number.
  @param[in] Blocks                 - Total block number to be read.

  @retval EFI_SUCCESS               - Datum are read from the device.
  @retval Others                    - Fail to read all the datum.

**/
EFI_STATUS
NvmeRead (
  IN NVME_CONTEXT                  *Nvme,
  OUT UINT64                       Buffer,
  IN UINT64                        Lba,
  IN UINTN                         Blocks
  )
{
  EFI_STATUS                       Status;
  UINT32                           BlockSize;
  UINT32                           MaxTransferBlocks;

  ASSERT (Blocks <= NVME_MAX_SECTORS);
  Status        = EFI_SUCCESS;
  BlockSize     = Nvme->BlockSize;
  if (Nvme->ControllerData->Mdts != 0) {
    MaxTransferBlocks = (1 << (Nvme->ControllerData->Mdts)) * (1 << (Nvme->Cap.Mpsmin + 12)) / BlockSize;
  } else {
    MaxTransferBlocks = 1024;
  }

  while (Blocks > 0) {
    if (Blocks > MaxTransferBlocks) {
      Status = NvmeReadSectors (Nvme, Buffer, Lba, MaxTransferBlocks);

      Blocks -= MaxTransferBlocks;
      Buffer += (MaxTransferBlocks * BlockSize);
      Lba    += MaxTransferBlocks;
    } else {
      Status = NvmeReadSectors (Nvme, Buffer, Lba, (UINT32) Blocks);
      Blocks = 0;
    }

    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_ERROR, "NvmeRead fail, Status = %r\n", Status));
      break;
    }
  }

  return Status;
}

/**
  Write some blocks to the device.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in] Buffer                 - The Buffer to be written into the device.
  @param[in] Lba                    - The start block number.
  @param[in] Blocks                 - Total block number to be written.

  @retval EFI_SUCCESS               - Datum are written into the Buffer.
  @retval Others                    - Fail to write all the datum.

**/
EFI_STATUS
NvmeWrite (
  IN NVME_CONTEXT                  *Nvme,
  IN UINT64                        Buffer,
  IN UINT64                        Lba,
  IN UINTN                         Blocks
  )
{
  EFI_STATUS                       Status;
  UINT32                           BlockSize;
  UINT32                           MaxTransferBlocks;

  ASSERT (Blocks <= NVME_MAX_SECTORS);
  Status        = EFI_SUCCESS;
  BlockSize     = Nvme->BlockSize;

  if (Nvme->ControllerData->Mdts != 0) {
    MaxTransferBlocks = (1 << (Nvme->ControllerData->Mdts)) * (1 << (Nvme->Cap.Mpsmin + 12)) / BlockSize;
  } else {
    MaxTransferBlocks = 1024;
  }

  while (Blocks > 0) {
    if (Blocks > MaxTransferBlocks) {
      Status = NvmeWriteSectors (Nvme, Buffer, Lba, MaxTransferBlocks);

      Blocks -= MaxTransferBlocks;
      Buffer += (MaxTransferBlocks * BlockSize);
      Lba    += MaxTransferBlocks;
    } else {
      Status = NvmeWriteSectors (Nvme, Buffer, Lba, (UINT32) Blocks);
      Blocks = 0;
    }

    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_ERROR, "NvmeWrite fail, Status = %r\n", Status));
      break;
    }
  }

  return Status;
}
