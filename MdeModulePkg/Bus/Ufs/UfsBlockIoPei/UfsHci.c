/** @file

  Copyright (c) 2014 - 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UfsBlockIoPei.h"

/**
  Wait for the value of the specified system memory set to the test value.

  @param  Address           The system memory address to test.
  @param  MaskValue         The mask value of memory.
  @param  TestValue         The test value of memory.
  @param  Timeout           The time out value for wait memory set, uses 100ns as a unit.

  @retval EFI_TIMEOUT       The system memory setting is time out.
  @retval EFI_SUCCESS       The system memory is correct set.

**/
EFI_STATUS
EFIAPI
UfsWaitMemSet (
  IN  UINTN   Address,
  IN  UINT32  MaskValue,
  IN  UINT32  TestValue,
  IN  UINT64  Timeout
  )
{
  UINT32   Value;
  UINT64   Delay;
  BOOLEAN  InfiniteWait;

  if (Timeout == 0) {
    InfiniteWait = TRUE;
  } else {
    InfiniteWait = FALSE;
  }

  Delay = DivU64x32 (Timeout, 10) + 1;

  do {
    //
    // Access PCI MMIO space to see if the value is the tested one.
    //
    Value = MmioRead32 (Address) & MaskValue;

    if (Value == TestValue) {
      return EFI_SUCCESS;
    }

    //
    // Stall for 1 microseconds.
    //
    MicroSecondDelay (1);

    Delay--;
  } while (InfiniteWait || (Delay > 0));

  return EFI_TIMEOUT;
}

/**
  Dump UIC command execution result for debugging.

  @param[in]   UicOpcode  The executed UIC opcode.
  @param[in]   Result     The result to be parsed.

**/
VOID
DumpUicCmdExecResult (
  IN  UINT8  UicOpcode,
  IN  UINT8  Result
  )
{
  if (UicOpcode <= UfsUicDmePeerSet) {
    switch (Result) {
      case 0x00:
        break;
      case 0x01:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - INVALID_MIB_ATTRIBUTE\n"));
        break;
      case 0x02:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - INVALID_MIB_ATTRIBUTE_VALUE\n"));
        break;
      case 0x03:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - READ_ONLY_MIB_ATTRIBUTE\n"));
        break;
      case 0x04:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - WRITE_ONLY_MIB_ATTRIBUTE\n"));
        break;
      case 0x05:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - BAD_INDEX\n"));
        break;
      case 0x06:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - LOCKED_MIB_ATTRIBUTE\n"));
        break;
      case 0x07:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - BAD_TEST_FEATURE_INDEX\n"));
        break;
      case 0x08:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - PEER_COMMUNICATION_FAILURE\n"));
        break;
      case 0x09:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - BUSY\n"));
        break;
      case 0x0A:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - DME_FAILURE\n"));
        break;
      default:
        ASSERT (FALSE);
        break;
    }
  } else {
    switch (Result) {
      case 0x00:
        break;
      case 0x01:
        DEBUG ((DEBUG_VERBOSE, "UIC control command fails - FAILURE\n"));
        break;
      default:
        ASSERT (FALSE);
        break;
    }
  }
}

/**
  Dump QUERY RESPONSE UPIU result for debugging.

  @param[in]   Result  The result to be parsed.

**/
VOID
DumpQueryResponseResult (
  IN  UINT8  Result
  )
{
  switch (Result) {
    case 0xF6:
      DEBUG ((DEBUG_VERBOSE, "Query Response with Parameter Not Readable\n"));
      break;
    case 0xF7:
      DEBUG ((DEBUG_VERBOSE, "Query Response with Parameter Not Writeable\n"));
      break;
    case 0xF8:
      DEBUG ((DEBUG_VERBOSE, "Query Response with Parameter Already Written\n"));
      break;
    case 0xF9:
      DEBUG ((DEBUG_VERBOSE, "Query Response with Invalid Length\n"));
      break;
    case 0xFA:
      DEBUG ((DEBUG_VERBOSE, "Query Response with Invalid Value\n"));
      break;
    case 0xFB:
      DEBUG ((DEBUG_VERBOSE, "Query Response with Invalid Selector\n"));
      break;
    case 0xFC:
      DEBUG ((DEBUG_VERBOSE, "Query Response with Invalid Index\n"));
      break;
    case 0xFD:
      DEBUG ((DEBUG_VERBOSE, "Query Response with Invalid Idn\n"));
      break;
    case 0xFE:
      DEBUG ((DEBUG_VERBOSE, "Query Response with Invalid Opcode\n"));
      break;
    case 0xFF:
      DEBUG ((DEBUG_VERBOSE, "Query Response with General Failure\n"));
      break;
    default:
      ASSERT (FALSE);
      break;
  }
}

/**
  Swap little endian to big endian.

  @param[in, out] Buffer      The data buffer. In input, it contains little endian data.
                              In output, it will become big endian.
  @param[in]      BufferSize  The length of converted data.

**/
VOID
SwapLittleEndianToBigEndian (
  IN OUT UINT8   *Buffer,
  IN     UINT32  BufferSize
  )
{
  UINT32  Index;
  UINT8   Temp;
  UINT32  SwapCount;

  SwapCount = BufferSize / 2;
  for (Index = 0; Index < SwapCount; Index++) {
    Temp                           = Buffer[Index];
    Buffer[Index]                  = Buffer[BufferSize - 1 - Index];
    Buffer[BufferSize - 1 - Index] = Temp;
  }
}

/**
  Fill TSF field of QUERY REQUEST UPIU.

  @param[in, out] TsfBase      The base address of TSF field of QUERY REQUEST UPIU.
  @param[in]      Opcode       The opcode of request.
  @param[in]      DescId       The descriptor ID of request.
  @param[in]      Index        The index of request.
  @param[in]      Selector     The selector of request.
  @param[in]      Length       The length of transferred data. The maximum is 4.
  @param[in]      Value        The value of transferred data.

**/
VOID
UfsFillTsfOfQueryReqUpiu (
  IN OUT UTP_UPIU_TSF  *TsfBase,
  IN     UINT8         Opcode,
  IN     UINT8         DescId    OPTIONAL,
  IN     UINT8         Index     OPTIONAL,
  IN     UINT8         Selector  OPTIONAL,
  IN     UINT16        Length    OPTIONAL,
  IN     UINT32        Value     OPTIONAL
  )
{
  ASSERT (TsfBase != NULL);
  ASSERT (Opcode <= UtpQueryFuncOpcodeTogFlag);

  TsfBase->Opcode = Opcode;
  if (Opcode != UtpQueryFuncOpcodeNop) {
    TsfBase->DescId   = DescId;
    TsfBase->Index    = Index;
    TsfBase->Selector = Selector;

    if ((Opcode == UtpQueryFuncOpcodeRdDesc) || (Opcode == UtpQueryFuncOpcodeWrDesc)) {
      SwapLittleEndianToBigEndian ((UINT8 *)&Length, sizeof (Length));
      TsfBase->Length = Length;
    }

    if (Opcode == UtpQueryFuncOpcodeWrAttr) {
      SwapLittleEndianToBigEndian ((UINT8 *)&Value, sizeof (Value));
      TsfBase->Value = Value;
    }
  }
}

/**
  Initialize COMMAND UPIU.

  @param[in, out] Command         The base address of COMMAND UPIU.
  @param[in]      Lun             The Lun on which the SCSI command is executed.
  @param[in]      TaskTag         The task tag of request.
  @param[in]      Cdb             The cdb buffer containing SCSI command.
  @param[in]      CdbLength       The cdb length.
  @param[in]      DataDirection   The direction of data transfer.
  @param[in]      ExpDataTranLen  The expected transfer data length.

  @retval EFI_SUCCESS     The initialization succeed.

**/
EFI_STATUS
UfsInitCommandUpiu (
  IN OUT UTP_COMMAND_UPIU    *Command,
  IN     UINT8               Lun,
  IN     UINT8               TaskTag,
  IN     UINT8               *Cdb,
  IN     UINT8               CdbLength,
  IN     UFS_DATA_DIRECTION  DataDirection,
  IN     UINT32              ExpDataTranLen
  )
{
  UINT8  Flags;

  ASSERT ((Command != NULL) && (Cdb != NULL));

  //
  // Task attribute is hard-coded to Ordered.
  //
  if (DataDirection == UfsDataIn) {
    Flags = BIT0 | BIT6;
  } else if (DataDirection == UfsDataOut) {
    Flags = BIT0 | BIT5;
  } else {
    Flags = BIT0;
  }

  //
  // Fill UTP COMMAND UPIU associated fields.
  //
  Command->TransCode = 0x01;
  Command->Flags     = Flags;
  Command->Lun       = Lun;
  Command->TaskTag   = TaskTag;
  Command->CmdSet    = 0x00;
  SwapLittleEndianToBigEndian ((UINT8 *)&ExpDataTranLen, sizeof (ExpDataTranLen));
  Command->ExpDataTranLen = ExpDataTranLen;

  CopyMem (Command->Cdb, Cdb, CdbLength);

  return EFI_SUCCESS;
}

/**
  Initialize UTP PRDT for data transfer.

  @param[in] Prdt         The base address of PRDT.
  @param[in] Buffer       The buffer to be read or written.
  @param[in] BufferSize   The data size to be read or written.

  @retval EFI_SUCCESS     The initialization succeed.

**/
EFI_STATUS
UfsInitUtpPrdt (
  IN  UTP_TR_PRD  *Prdt,
  IN  VOID        *Buffer,
  IN  UINT32      BufferSize
  )
{
  UINTN   PrdtIndex;
  UINT32  RemainingLen;
  UINT8   *Remaining;
  UINTN   PrdtNumber;

  if ((BufferSize & (BIT0 | BIT1)) != 0) {
    BufferSize &= ~(BIT0 | BIT1);
    DEBUG ((DEBUG_WARN, "UfsInitUtpPrdt: The BufferSize [%d] is not dword-aligned!\n", BufferSize));
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  ASSERT (((UINTN)Buffer & (BIT0 | BIT1)) == 0);

  RemainingLen = BufferSize;
  Remaining    = Buffer;
  PrdtNumber   = (UINTN)DivU64x32 ((UINT64)BufferSize + UFS_MAX_DATA_LEN_PER_PRD - 1, UFS_MAX_DATA_LEN_PER_PRD);

  for (PrdtIndex = 0; PrdtIndex < PrdtNumber; PrdtIndex++) {
    if (RemainingLen < UFS_MAX_DATA_LEN_PER_PRD) {
      Prdt[PrdtIndex].DbCount = (UINT32)RemainingLen - 1;
    } else {
      Prdt[PrdtIndex].DbCount = UFS_MAX_DATA_LEN_PER_PRD - 1;
    }

    Prdt[PrdtIndex].DbAddr  = (UINT32)RShiftU64 ((UINT64)(UINTN)Remaining, 2);
    Prdt[PrdtIndex].DbAddrU = (UINT32)RShiftU64 ((UINT64)(UINTN)Remaining, 32);
    RemainingLen           -= UFS_MAX_DATA_LEN_PER_PRD;
    Remaining              += UFS_MAX_DATA_LEN_PER_PRD;
  }

  return EFI_SUCCESS;
}

/**
  Initialize QUERY REQUEST UPIU.

  @param[in, out] QueryReq      The base address of QUERY REQUEST UPIU.
  @param[in]      TaskTag       The task tag of request.
  @param[in]      Opcode        The opcode of request.
  @param[in]      DescId        The descriptor ID of request.
  @param[in]      Index         The index of request.
  @param[in]      Selector      The selector of request.
  @param[in]      DataSize      The data size to be read or written.
  @param[in]      Data          The buffer to be read or written.

  @retval EFI_SUCCESS           The initialization succeed.

**/
EFI_STATUS
UfsInitQueryRequestUpiu (
  IN OUT UTP_QUERY_REQ_UPIU  *QueryReq,
  IN     UINT8               TaskTag,
  IN     UINT8               Opcode,
  IN     UINT8               DescId,
  IN     UINT8               Index,
  IN     UINT8               Selector,
  IN     UINTN               DataSize   OPTIONAL,
  IN     UINT8               *Data      OPTIONAL
  )
{
  ASSERT (QueryReq != NULL);

  QueryReq->TransCode = 0x16;
  QueryReq->TaskTag   = TaskTag;
  if ((Opcode == UtpQueryFuncOpcodeRdDesc) || (Opcode == UtpQueryFuncOpcodeRdFlag) || (Opcode == UtpQueryFuncOpcodeRdAttr)) {
    QueryReq->QueryFunc = QUERY_FUNC_STD_READ_REQ;
  } else {
    QueryReq->QueryFunc = QUERY_FUNC_STD_WRITE_REQ;
  }

  if (Opcode == UtpQueryFuncOpcodeWrAttr) {
    UfsFillTsfOfQueryReqUpiu (&QueryReq->Tsf, Opcode, DescId, Index, Selector, 0, *(UINT32 *)Data);
  } else if ((Opcode == UtpQueryFuncOpcodeRdDesc) || (Opcode == UtpQueryFuncOpcodeWrDesc)) {
    UfsFillTsfOfQueryReqUpiu (&QueryReq->Tsf, Opcode, DescId, Index, Selector, (UINT16)DataSize, 0);
  } else {
    UfsFillTsfOfQueryReqUpiu (&QueryReq->Tsf, Opcode, DescId, Index, Selector, 0, 0);
  }

  if (Opcode == UtpQueryFuncOpcodeWrDesc) {
    CopyMem (QueryReq + 1, Data, DataSize);

    SwapLittleEndianToBigEndian ((UINT8 *)&DataSize, sizeof (UINT16));
    QueryReq->DataSegLen = (UINT16)DataSize;
  }

  return EFI_SUCCESS;
}

/**
  Allocate COMMAND/RESPONSE UPIU for filling UTP TRD's command descriptor field.

  @param[in]  Private           The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.
  @param[in]  Lun               The Lun on which the SCSI command is executed.
  @param[in]  Packet            The pointer to the UFS_SCSI_REQUEST_PACKET data structure.
  @param[in]  Trd               The pointer to the UTP Transfer Request Descriptor.
  @param[out] BufferMap         A resulting value, if not NULL, to pass to IoMmuUnmap().

  @retval EFI_SUCCESS           The creation succeed.
  @retval EFI_DEVICE_ERROR      The creation failed.
  @retval EFI_OUT_OF_RESOURCES  The memory resource is insufficient.

**/
EFI_STATUS
UfsCreateScsiCommandDesc (
  IN     UFS_PEIM_HC_PRIVATE_DATA  *Private,
  IN     UINT8                     Lun,
  IN     UFS_SCSI_REQUEST_PACKET   *Packet,
  IN     UTP_TRD                   *Trd,
  OUT VOID                         **BufferMap
  )
{
  UINT8                  *CommandDesc;
  UINTN                  TotalLen;
  UINTN                  PrdtNumber;
  VOID                   *Buffer;
  UINT32                 Length;
  UTP_COMMAND_UPIU       *CommandUpiu;
  UTP_TR_PRD             *PrdtBase;
  UFS_DATA_DIRECTION     DataDirection;
  EFI_STATUS             Status;
  EDKII_IOMMU_OPERATION  MapOp;
  UINTN                  MapLength;
  EFI_PHYSICAL_ADDRESS   BufferPhyAddr;

  ASSERT ((Private != NULL) && (Packet != NULL) && (Trd != NULL));

  BufferPhyAddr = 0;

  if (Packet->DataDirection == UfsDataIn) {
    Buffer        = Packet->InDataBuffer;
    Length        = Packet->InTransferLength;
    DataDirection = UfsDataIn;
    MapOp         = EdkiiIoMmuOperationBusMasterWrite;
  } else {
    Buffer        = Packet->OutDataBuffer;
    Length        = Packet->OutTransferLength;
    DataDirection = UfsDataOut;
    MapOp         = EdkiiIoMmuOperationBusMasterRead;
  }

  if (Length == 0) {
    DataDirection = UfsNoData;
  } else {
    MapLength = Length;
    Status    = IoMmuMap (MapOp, Buffer, &MapLength, &BufferPhyAddr, BufferMap);

    if (EFI_ERROR (Status) || (MapLength != Length)) {
      DEBUG ((DEBUG_ERROR, "UfsCreateScsiCommandDesc: Fail to map data buffer.\n"));
      return EFI_OUT_OF_RESOURCES;
    }
  }

  PrdtNumber = (UINTN)DivU64x32 ((UINT64)Length + UFS_MAX_DATA_LEN_PER_PRD - 1, UFS_MAX_DATA_LEN_PER_PRD);

  TotalLen    = ROUNDUP8 (sizeof (UTP_COMMAND_UPIU)) + ROUNDUP8 (sizeof (UTP_RESPONSE_UPIU)) + PrdtNumber * sizeof (UTP_TR_PRD);
  CommandDesc = UfsPeimAllocateMem (Private->Pool, TotalLen);
  if (CommandDesc == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CommandUpiu = (UTP_COMMAND_UPIU *)CommandDesc;
  PrdtBase    = (UTP_TR_PRD *)(CommandDesc + ROUNDUP8 (sizeof (UTP_COMMAND_UPIU)) + ROUNDUP8 (sizeof (UTP_RESPONSE_UPIU)));

  UfsInitCommandUpiu (CommandUpiu, Lun, Private->TaskTag++, Packet->Cdb, Packet->CdbLength, DataDirection, Length);
  UfsInitUtpPrdt (PrdtBase, (VOID *)(UINTN)BufferPhyAddr, Length);

  //
  // Fill UTP_TRD associated fields
  // NOTE: Some UFS host controllers request the Response UPIU and the Physical Region Description Table
  // *MUST* be located at a 64-bit aligned boundary.
  //
  Trd->Int    = UFS_INTERRUPT_COMMAND;
  Trd->Dd     = DataDirection;
  Trd->Ct     = UFS_STORAGE_COMMAND_TYPE;
  Trd->Ocs    = UFS_HC_TRD_OCS_INIT_VALUE;
  Trd->UcdBa  = (UINT32)RShiftU64 ((UINT64)(UINTN)CommandUpiu, 7);
  Trd->UcdBaU = (UINT32)RShiftU64 ((UINT64)(UINTN)CommandUpiu, 32);
  Trd->RuL    = (UINT16)DivU64x32 ((UINT64)ROUNDUP8 (sizeof (UTP_RESPONSE_UPIU)), sizeof (UINT32));
  Trd->RuO    = (UINT16)DivU64x32 ((UINT64)ROUNDUP8 (sizeof (UTP_COMMAND_UPIU)), sizeof (UINT32));
  Trd->PrdtL  = (UINT16)PrdtNumber;
  Trd->PrdtO  = (UINT16)DivU64x32 ((UINT64)(ROUNDUP8 (sizeof (UTP_COMMAND_UPIU)) + ROUNDUP8 (sizeof (UTP_RESPONSE_UPIU))), sizeof (UINT32));
  return EFI_SUCCESS;
}

/**
  Allocate QUERY REQUEST/QUERY RESPONSE UPIU for filling UTP TRD's command descriptor field.

  @param[in]  Private           The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.
  @param[in]  Packet            The pointer to the UFS_DEVICE_MANAGEMENT_REQUEST_PACKET data structure.
  @param[in]  Trd               The pointer to the UTP Transfer Request Descriptor.

  @retval EFI_SUCCESS           The creation succeed.
  @retval EFI_DEVICE_ERROR      The creation failed.
  @retval EFI_OUT_OF_RESOURCES  The memory resource is insufficient.
  @retval EFI_INVALID_PARAMETER The parameter passed in is invalid.

**/
EFI_STATUS
UfsCreateDMCommandDesc (
  IN  UFS_PEIM_HC_PRIVATE_DATA              *Private,
  IN  UFS_DEVICE_MANAGEMENT_REQUEST_PACKET  *Packet,
  IN  UTP_TRD                               *Trd
  )
{
  UINT8               *CommandDesc;
  UINTN               TotalLen;
  UTP_QUERY_REQ_UPIU  *QueryReqUpiu;
  UINT8               Opcode;
  UINT32              DataSize;
  UINT8               *Data;
  UINT8               DataDirection;

  ASSERT ((Private != NULL) && (Packet != NULL) && (Trd != NULL));

  Opcode = Packet->Opcode;
  if ((Opcode > UtpQueryFuncOpcodeTogFlag) || (Opcode == UtpQueryFuncOpcodeNop)) {
    return EFI_INVALID_PARAMETER;
  }

  DataDirection = Packet->DataDirection;
  if (DataDirection == UfsDataIn) {
    DataSize = Packet->InTransferLength;
    Data     = Packet->InDataBuffer;
  } else if (DataDirection == UfsDataOut) {
    DataSize = Packet->OutTransferLength;
    Data     = Packet->OutDataBuffer;
  } else {
    DataSize = 0;
    Data     = NULL;
  }

  if (((Opcode != UtpQueryFuncOpcodeRdFlag) && (Opcode != UtpQueryFuncOpcodeSetFlag) &&
       (Opcode != UtpQueryFuncOpcodeClrFlag) && (Opcode != UtpQueryFuncOpcodeTogFlag) &&
       (Opcode != UtpQueryFuncOpcodeRdAttr)) && ((DataSize != 0) && (Data == NULL)))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((Opcode == UtpQueryFuncOpcodeWrAttr) && (DataSize != sizeof (UINT32))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Opcode == UtpQueryFuncOpcodeWrDesc) || (Opcode == UtpQueryFuncOpcodeRdDesc)) {
    TotalLen = ROUNDUP8 (sizeof (UTP_QUERY_REQ_UPIU)) + ROUNDUP8 (sizeof (UTP_QUERY_RESP_UPIU)) + ROUNDUP8 (DataSize);
  } else {
    TotalLen = ROUNDUP8 (sizeof (UTP_QUERY_REQ_UPIU)) + ROUNDUP8 (sizeof (UTP_QUERY_RESP_UPIU));
  }

  CommandDesc = UfsPeimAllocateMem (Private->Pool, TotalLen);
  if (CommandDesc == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize UTP QUERY REQUEST UPIU
  //
  QueryReqUpiu = (UTP_QUERY_REQ_UPIU *)CommandDesc;
  UfsInitQueryRequestUpiu (
    QueryReqUpiu,
    Private->TaskTag++,
    Opcode,
    Packet->DescId,
    Packet->Index,
    Packet->Selector,
    DataSize,
    Data
    );

  //
  // Fill UTP_TRD associated fields
  // NOTE: Some UFS host controllers request the Query Response UPIU *MUST* be located at a 64-bit aligned boundary.
  //
  Trd->Int    = UFS_INTERRUPT_COMMAND;
  Trd->Dd     = DataDirection;
  Trd->Ct     = UFS_STORAGE_COMMAND_TYPE;
  Trd->Ocs    = UFS_HC_TRD_OCS_INIT_VALUE;
  Trd->UcdBa  = (UINT32)RShiftU64 ((UINT64)(UINTN)QueryReqUpiu, 7);
  Trd->UcdBaU = (UINT32)RShiftU64 ((UINT64)(UINTN)QueryReqUpiu, 32);
  if (Opcode == UtpQueryFuncOpcodeWrDesc) {
    Trd->RuL = (UINT16)DivU64x32 ((UINT64)ROUNDUP8 (sizeof (UTP_QUERY_RESP_UPIU)), sizeof (UINT32));
    Trd->RuO = (UINT16)DivU64x32 ((UINT64)ROUNDUP8 (sizeof (UTP_QUERY_REQ_UPIU)) + ROUNDUP8 (DataSize), sizeof (UINT32));
  } else {
    Trd->RuL = (UINT16)DivU64x32 ((UINT64)ROUNDUP8 (sizeof (UTP_QUERY_RESP_UPIU)) + ROUNDUP8 (DataSize), sizeof (UINT32));
    Trd->RuO = (UINT16)DivU64x32 ((UINT64)ROUNDUP8 (sizeof (UTP_QUERY_REQ_UPIU)), sizeof (UINT32));
  }

  return EFI_SUCCESS;
}

/**
  Allocate NOP IN and NOP OUT UPIU for filling UTP TRD's command descriptor field.

  @param[in]  Private           The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.
  @param[in]  Trd               The pointer to the UTP Transfer Request Descriptor.

  @retval EFI_SUCCESS           The creation succeed.
  @retval EFI_DEVICE_ERROR      The creation failed.
  @retval EFI_OUT_OF_RESOURCES  The memory resource is insufficient.

**/
EFI_STATUS
UfsCreateNopCommandDesc (
  IN  UFS_PEIM_HC_PRIVATE_DATA  *Private,
  IN  UTP_TRD                   *Trd
  )
{
  UINT8             *CommandDesc;
  UINTN             TotalLen;
  UTP_NOP_OUT_UPIU  *NopOutUpiu;

  ASSERT ((Private != NULL) && (Trd != NULL));

  TotalLen    = ROUNDUP8 (sizeof (UTP_NOP_OUT_UPIU)) + ROUNDUP8 (sizeof (UTP_NOP_IN_UPIU));
  CommandDesc = UfsPeimAllocateMem (Private->Pool, TotalLen);
  if (CommandDesc == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NopOutUpiu = (UTP_NOP_OUT_UPIU *)CommandDesc;

  NopOutUpiu->TaskTag = Private->TaskTag++;

  //
  // Fill UTP_TRD associated fields
  // NOTE: Some UFS host controllers request the Nop Out UPIU *MUST* be located at a 64-bit aligned boundary.
  //
  Trd->Int    = UFS_INTERRUPT_COMMAND;
  Trd->Dd     = 0x00;
  Trd->Ct     = UFS_STORAGE_COMMAND_TYPE;
  Trd->Ocs    = UFS_HC_TRD_OCS_INIT_VALUE;
  Trd->UcdBa  = (UINT32)RShiftU64 ((UINT64)(UINTN)NopOutUpiu, 7);
  Trd->UcdBaU = (UINT32)RShiftU64 ((UINT64)(UINTN)NopOutUpiu, 32);
  Trd->RuL    = (UINT16)DivU64x32 ((UINT64)ROUNDUP8 (sizeof (UTP_NOP_IN_UPIU)), sizeof (UINT32));
  Trd->RuO    = (UINT16)DivU64x32 ((UINT64)ROUNDUP8 (sizeof (UTP_NOP_OUT_UPIU)), sizeof (UINT32));

  return EFI_SUCCESS;
}

/**
  Find out available slot in transfer list of a UFS device.

  @param[in]  Private       The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.
  @param[out] Slot          The available slot.

  @retval EFI_SUCCESS       The available slot was found successfully.

**/
EFI_STATUS
UfsFindAvailableSlotInTrl (
  IN     UFS_PEIM_HC_PRIVATE_DATA  *Private,
  OUT UINT8                        *Slot
  )
{
  ASSERT ((Private != NULL) && (Slot != NULL));

  //
  // The simplest algo to always use slot 0.
  // TODO: enhance it to support async transfer with multiple slot.
  //
  *Slot = 0;

  return EFI_SUCCESS;
}

/**
  Start specified slot in transfer list of a UFS device.

  @param[in]  Private       The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.
  @param[in]  Slot          The slot to be started.

**/
VOID
UfsStartExecCmd (
  IN  UFS_PEIM_HC_PRIVATE_DATA  *Private,
  IN  UINT8                     Slot
  )
{
  UINTN   UfsHcBase;
  UINTN   Address;
  UINT32  Data;

  UfsHcBase = Private->UfsHcBase;

  Address = UfsHcBase + UFS_HC_UTRLRSR_OFFSET;
  Data    = MmioRead32 (Address);
  if ((Data & UFS_HC_UTRLRSR) != UFS_HC_UTRLRSR) {
    MmioWrite32 (Address, UFS_HC_UTRLRSR);
  }

  Address = UfsHcBase + UFS_HC_UTRLDBR_OFFSET;
  MmioWrite32 (Address, BIT0 << Slot);
}

/**
  Stop specified slot in transfer list of a UFS device.

  @param[in]  Private       The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.
  @param[in]  Slot          The slot to be stop.

**/
VOID
UfsStopExecCmd (
  IN  UFS_PEIM_HC_PRIVATE_DATA  *Private,
  IN  UINT8                     Slot
  )
{
  UINTN   UfsHcBase;
  UINTN   Address;
  UINT32  Data;

  UfsHcBase = Private->UfsHcBase;

  Address = UfsHcBase + UFS_HC_UTRLDBR_OFFSET;
  Data    = MmioRead32 (Address);
  if ((Data & (BIT0 << Slot)) != 0) {
    Address = UfsHcBase + UFS_HC_UTRLCLR_OFFSET;
    Data    = MmioRead32 (Address);
    MmioWrite32 (Address, (Data & ~(BIT0 << Slot)));
  }
}

/**
  Extracts return data from query response upiu.

  @param[in, out] Packet        Pointer to the UFS_DEVICE_MANAGEMENT_REQUEST_PACKET.
  @param[in]      QueryResp     Pointer to the query response.

  @retval EFI_INVALID_PARAMETER Packet or QueryResp are empty or opcode is invalid.
  @retval EFI_DEVICE_ERROR      Data returned from device is invalid.
  @retval EFI_SUCCESS           Data extracted.

**/
EFI_STATUS
UfsGetReturnDataFromQueryResponse (
  IN OUT UFS_DEVICE_MANAGEMENT_REQUEST_PACKET  *Packet,
  IN     UTP_QUERY_RESP_UPIU                   *QueryResp
  )
{
  UINT16  ReturnDataSize;
  UINT32  ReturnData;

  ReturnDataSize = 0;

  if ((Packet == NULL) || (QueryResp == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Packet->Opcode) {
    case UtpQueryFuncOpcodeRdDesc:
      ReturnDataSize = QueryResp->Tsf.Length;
      SwapLittleEndianToBigEndian ((UINT8 *)&ReturnDataSize, sizeof (UINT16));
      //
      // Make sure the hardware device does not return more data than expected.
      //
      if (ReturnDataSize > Packet->InTransferLength) {
        return EFI_DEVICE_ERROR;
      }

      CopyMem (Packet->InDataBuffer, (QueryResp + 1), ReturnDataSize);
      Packet->InTransferLength = ReturnDataSize;
      break;
    case UtpQueryFuncOpcodeWrDesc:
      ReturnDataSize = QueryResp->Tsf.Length;
      SwapLittleEndianToBigEndian ((UINT8 *)&ReturnDataSize, sizeof (UINT16));
      Packet->OutTransferLength = ReturnDataSize;
      break;
    case UtpQueryFuncOpcodeRdFlag:
      //
      // The 'FLAG VALUE' field is at byte offset 3 of QueryResp->Tsf.Value
      //
      *((UINT8 *)(Packet->InDataBuffer)) = *((UINT8 *)&(QueryResp->Tsf.Value) + 3);
      break;
    case UtpQueryFuncOpcodeSetFlag:
    case UtpQueryFuncOpcodeClrFlag:
    case UtpQueryFuncOpcodeTogFlag:
      //
      // The 'FLAG VALUE' field is at byte offset 3 of QueryResp->Tsf.Value
      //
      *((UINT8 *)(Packet->OutDataBuffer)) = *((UINT8 *)&(QueryResp->Tsf.Value) + 3);
      break;
    case UtpQueryFuncOpcodeRdAttr:
      ReturnData = QueryResp->Tsf.Value;
      SwapLittleEndianToBigEndian ((UINT8 *)&ReturnData, sizeof (UINT32));
      CopyMem (Packet->InDataBuffer, &ReturnData, sizeof (UINT32));
      break;
    case UtpQueryFuncOpcodeWrAttr:
      ReturnData = QueryResp->Tsf.Value;
      SwapLittleEndianToBigEndian ((UINT8 *)&ReturnData, sizeof (UINT32));
      CopyMem (Packet->OutDataBuffer, &ReturnData, sizeof (UINT32));
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Creates Transfer Request descriptor and sends Query Request to the device.

  @param[in]      Private       Pointer to the UFS_PEIM_HC_PRIVATE_DATA.
  @param[in, out] Packet        Pointer to the UFS_DEVICE_MANAGEMENT_REQUEST_PACKET.

  @retval EFI_SUCCESS           The device descriptor was read/written successfully.
  @retval EFI_INVALID_PARAMETER The DescId, Index and Selector fields in Packet are invalid
                                combination to point to a type of UFS device descriptor.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to r/w the device descriptor.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of r/w the device descriptor.

**/
EFI_STATUS
UfsSendDmRequestRetry (
  IN     UFS_PEIM_HC_PRIVATE_DATA              *Private,
  IN OUT UFS_DEVICE_MANAGEMENT_REQUEST_PACKET  *Packet
  )
{
  UINT8                Slot;
  EFI_STATUS           Status;
  UTP_TRD              *Trd;
  UINTN                Address;
  UTP_QUERY_RESP_UPIU  *QueryResp;
  UINT8                *CmdDescBase;
  UINT32               CmdDescSize;

  // Workaround: Adding this one second for reading descriptor
  MicroSecondDelay (1 * 1000 * 1000); // delay 1 seconds

  // Find out which slot of transfer request list is available.
  //
  Status = UfsFindAvailableSlotInTrl (Private, &Slot);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Trd = ((UTP_TRD *)Private->UtpTrlBase) + Slot;
  //
  // Fill transfer request descriptor to this slot.
  //
  Status = UfsCreateDMCommandDesc (Private, Packet, Trd);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to create DM command descriptor\n"));
    return Status;
  }

  //
  // Check the transfer request result.
  //
  CmdDescBase = (UINT8 *)(UINTN)(LShiftU64 ((UINT64)Trd->UcdBaU, 32) | LShiftU64 ((UINT64)Trd->UcdBa, 7));
  QueryResp   = (UTP_QUERY_RESP_UPIU *)(CmdDescBase + Trd->RuO * sizeof (UINT32));
  CmdDescSize = Trd->RuO * sizeof (UINT32) + Trd->RuL * sizeof (UINT32);

  //
  // Start to execute the transfer request.
  //
  UfsStartExecCmd (Private, Slot);

  //
  // Wait for the completion of the transfer request.
  //
  Address = Private->UfsHcBase + UFS_HC_UTRLDBR_OFFSET;
  Status  = UfsWaitMemSet (Address, (BIT0 << Slot), 0, Packet->Timeout);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  if ((Trd->Ocs != 0) || (QueryResp->QueryResp != UfsUtpQueryResponseSuccess)) {
    DEBUG ((DEBUG_ERROR, "Failed to send query request, OCS = %X, QueryResp = %X\n", Trd->Ocs, QueryResp->QueryResp));
    DumpQueryResponseResult (QueryResp->QueryResp);
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  Status = UfsGetReturnDataFromQueryResponse (Packet, QueryResp);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get return data from query response\n"));
    goto Exit;
  }

Exit:
  UfsStopExecCmd (Private, Slot);
  UfsPeimFreeMem (Private->Pool, CmdDescBase, CmdDescSize);

  return Status;
}

/**
  Sends Query Request to the device. Query is sent until device responds correctly or counter runs out.

  @param[in]      Private       Pointer to the UFS_PEIM_HC_PRIVATE_DATA.
  @param[in, out] Packet        Pointer to the UFS_DEVICE_MANAGEMENT_REQUEST_PACKET.

  @retval EFI_SUCCESS           The device responded correctly to the Query request.
  @retval EFI_INVALID_PARAMETER The DescId, Index and Selector fields in Packet are invalid
                                combination to point to a type of UFS device descriptor.
  @retval EFI_DEVICE_ERROR      A device error occurred while waiting for the response from the device.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of the operation.

**/
EFI_STATUS
UfsSendDmRequest (
  IN     UFS_PEIM_HC_PRIVATE_DATA              *Private,
  IN OUT UFS_DEVICE_MANAGEMENT_REQUEST_PACKET  *Packet
  )
{
  EFI_STATUS  Status;
  UINT8       Retry;

  Status = EFI_SUCCESS;

  for (Retry = 0; Retry < 5; Retry++) {
    Status = UfsSendDmRequestRetry (Private, Packet);
    if (!EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }
  }

  DEBUG ((DEBUG_ERROR, "Failed to get response from the device after %d retries\n", Retry));
  return Status;
}

/**
  Read or write specified device descriptor of a UFS device.

  @param[in]      Private       The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.
  @param[in]      Read          The boolean variable to show r/w direction.
  @param[in]      DescId        The ID of device descriptor.
  @param[in]      Index         The Index of device descriptor.
  @param[in]      Selector      The Selector of device descriptor.
  @param[in, out] Descriptor    The buffer of device descriptor to be read or written.
  @param[in]      DescSize      The size of device descriptor buffer.

  @retval EFI_SUCCESS           The device descriptor was read/written successfully.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to r/w the device descriptor.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of r/w the device descriptor.

**/
EFI_STATUS
UfsRwDeviceDesc (
  IN     UFS_PEIM_HC_PRIVATE_DATA  *Private,
  IN     BOOLEAN                   Read,
  IN     UINT8                     DescId,
  IN     UINT8                     Index,
  IN     UINT8                     Selector,
  IN OUT VOID                      *Descriptor,
  IN     UINT32                    DescSize
  )
{
  EFI_STATUS                            Status;
  UFS_DEVICE_MANAGEMENT_REQUEST_PACKET  Packet;

  ZeroMem (&Packet, sizeof (UFS_DEVICE_MANAGEMENT_REQUEST_PACKET));

  if (Read) {
    Packet.DataDirection    = UfsDataIn;
    Packet.InDataBuffer     = Descriptor;
    Packet.InTransferLength = DescSize;
    Packet.Opcode           = UtpQueryFuncOpcodeRdDesc;
  } else {
    Packet.DataDirection     = UfsDataOut;
    Packet.OutDataBuffer     = Descriptor;
    Packet.OutTransferLength = DescSize;
    Packet.Opcode            = UtpQueryFuncOpcodeWrDesc;
  }

  Packet.DescId   = DescId;
  Packet.Index    = Index;
  Packet.Selector = Selector;
  Packet.Timeout  = UFS_TIMEOUT;

  Status = UfsSendDmRequest (Private, &Packet);
  return Status;
}

/**
  Read or write specified attribute of a UFS device.

  @param[in]      Private       The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]      Read          The boolean variable to show r/w direction.
  @param[in]      AttrId        The ID of Attribute.
  @param[in]      Index         The Index of Attribute.
  @param[in]      Selector      The Selector of Attribute.
  @param[in, out] Attributes    The value of Attribute to be read or written.

  @retval EFI_SUCCESS           The Attribute was read/written successfully.
  @retval EFI_INVALID_PARAMETER AttrId, Index and Selector are invalid combination to point to a
                                type of UFS device descriptor.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to r/w the Attribute.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of r/w the Attribute.

**/
EFI_STATUS
UfsRwAttributes (
  IN     UFS_PEIM_HC_PRIVATE_DATA  *Private,
  IN     BOOLEAN                   Read,
  IN     UINT8                     AttrId,
  IN     UINT8                     Index,
  IN     UINT8                     Selector,
  IN OUT UINT32                    *Attributes
  )
{
  UFS_DEVICE_MANAGEMENT_REQUEST_PACKET  Packet;

  ZeroMem (&Packet, sizeof (UFS_DEVICE_MANAGEMENT_REQUEST_PACKET));

  if (Read) {
    Packet.DataDirection = UfsDataIn;
    Packet.Opcode        = UtpQueryFuncOpcodeRdAttr;
    Packet.InDataBuffer  = Attributes;
  } else {
    Packet.DataDirection     = UfsDataOut;
    Packet.Opcode            = UtpQueryFuncOpcodeWrAttr;
    Packet.OutDataBuffer     = Attributes;
    Packet.OutTransferLength = sizeof (UINT32);
  }

  Packet.DescId   = AttrId;
  Packet.Index    = Index;
  Packet.Selector = Selector;
  Packet.Timeout  = UFS_TIMEOUT;

  return UfsSendDmRequest (Private, &Packet);
}

/**
  Read or write specified flag of a UFS device.

  @param[in]      Private       The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.
  @param[in]      Read          The boolean variable to show r/w direction.
  @param[in]      FlagId        The ID of flag to be read or written.
  @param[in, out] Value         The value to set or clear flag.

  @retval EFI_SUCCESS           The flag was read/written successfully.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to r/w the flag.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of r/w the flag.

**/
EFI_STATUS
UfsRwFlags (
  IN     UFS_PEIM_HC_PRIVATE_DATA  *Private,
  IN     BOOLEAN                   Read,
  IN     UINT8                     FlagId,
  IN OUT UINT8                     *Value
  )
{
  EFI_STATUS                            Status;
  UFS_DEVICE_MANAGEMENT_REQUEST_PACKET  Packet;

  if (Value == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&Packet, sizeof (UFS_DEVICE_MANAGEMENT_REQUEST_PACKET));

  if (Read) {
    ASSERT (Value != NULL);
    Packet.DataDirection    = UfsDataIn;
    Packet.InDataBuffer     = (VOID *)Value;
    Packet.InTransferLength = 0;
    Packet.Opcode           = UtpQueryFuncOpcodeRdFlag;
  } else {
    Packet.DataDirection     = UfsDataOut;
    Packet.OutDataBuffer     = (VOID *)Value;
    Packet.OutTransferLength = 0;
    if (*Value == 1) {
      Packet.Opcode = UtpQueryFuncOpcodeSetFlag;
    } else if (*Value == 0) {
      Packet.Opcode = UtpQueryFuncOpcodeClrFlag;
    } else {
      return EFI_INVALID_PARAMETER;
    }
  }

  Packet.DescId   = FlagId;
  Packet.Index    = 0;
  Packet.Selector = 0;
  Packet.Timeout  = UFS_TIMEOUT;

  Status = UfsSendDmRequest (Private, &Packet);

  return Status;
}

/**
  Set specified flag to 1 on a UFS device.

  @param[in]  Private           The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.
  @param[in]  FlagId            The ID of flag to be set.

  @retval EFI_SUCCESS           The flag was set successfully.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to set the flag.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of setting the flag.

**/
EFI_STATUS
UfsSetFlag (
  IN  UFS_PEIM_HC_PRIVATE_DATA  *Private,
  IN  UINT8                     FlagId
  )
{
  EFI_STATUS  Status;
  UINT8       Value;

  Value  = 1;
  Status = UfsRwFlags (Private, FALSE, FlagId, &Value);

  return Status;
}

/**
  Read specified flag from a UFS device.

  @param[in]  Private           The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.
  @param[in]  FlagId            The ID of flag to be read.
  @param[out] Value             The flag's value.

  @retval EFI_SUCCESS           The flag was read successfully.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to read the flag.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of reading the flag.

**/
EFI_STATUS
UfsReadFlag (
  IN  UFS_PEIM_HC_PRIVATE_DATA  *Private,
  IN  UINT8                     FlagId,
  OUT UINT8                     *Value
  )
{
  EFI_STATUS  Status;

  Status = UfsRwFlags (Private, TRUE, FlagId, Value);

  return Status;
}

/**
  Sends NOP IN cmd to a UFS device for initialization process request.
  For more details, please refer to UFS 2.0 spec Figure 13.3.

  @param[in]  Private           The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS           The NOP IN command was sent by the host. The NOP OUT response was
                                received successfully.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to execute NOP IN command.
  @retval EFI_OUT_OF_RESOURCES  The resource for transfer is not available.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the NOP IN command to execute.

**/
EFI_STATUS
UfsExecNopCmds (
  IN  UFS_PEIM_HC_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS       Status;
  UINT8            Slot;
  UTP_TRD          *Trd;
  UTP_NOP_IN_UPIU  *NopInUpiu;
  UINT8            *CmdDescBase;
  UINT32           CmdDescSize;
  UINTN            Address;

  //
  // Find out which slot of transfer request list is available.
  //
  Status = UfsFindAvailableSlotInTrl (Private, &Slot);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Trd    = ((UTP_TRD *)Private->UtpTrlBase) + Slot;
  Status = UfsCreateNopCommandDesc (Private, Trd);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check the transfer request result.
  //
  CmdDescBase = (UINT8 *)(UINTN)(LShiftU64 ((UINT64)Trd->UcdBaU, 32) | LShiftU64 ((UINT64)Trd->UcdBa, 7));
  NopInUpiu   = (UTP_NOP_IN_UPIU *)(CmdDescBase + Trd->RuO * sizeof (UINT32));
  CmdDescSize = Trd->RuO * sizeof (UINT32) + Trd->RuL * sizeof (UINT32);

  //
  // Start to execute the transfer request.
  //
  UfsStartExecCmd (Private, Slot);

  //
  // Wait for the completion of the transfer request.
  //
  Address = Private->UfsHcBase + UFS_HC_UTRLDBR_OFFSET;
  Status  = UfsWaitMemSet (Address, BIT0 << Slot, 0, UFS_TIMEOUT);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  if (NopInUpiu->Resp != 0) {
    Status = EFI_DEVICE_ERROR;
  } else {
    Status = EFI_SUCCESS;
  }

Exit:
  UfsStopExecCmd (Private, Slot);
  UfsPeimFreeMem (Private->Pool, CmdDescBase, CmdDescSize);

  return Status;
}

/**
  Sends a UFS-supported SCSI Request Packet to a UFS device that is attached to the UFS host controller.

  @param[in]      Private       The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.
  @param[in]      Lun           The LUN of the UFS device to send the SCSI Request Packet.
  @param[in, out] Packet        A pointer to the SCSI Request Packet to send to a specified Lun of the
                                UFS device.

  @retval EFI_SUCCESS           The SCSI Request Packet was sent by the host. For bi-directional
                                commands, InTransferLength bytes were transferred from
                                InDataBuffer. For write and bi-directional commands,
                                OutTransferLength bytes were transferred by
                                OutDataBuffer.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to send the SCSI Request
                                Packet.
  @retval EFI_OUT_OF_RESOURCES  The resource for transfer is not available.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the SCSI Request Packet to execute.

**/
EFI_STATUS
UfsExecScsiCmds (
  IN     UFS_PEIM_HC_PRIVATE_DATA  *Private,
  IN     UINT8                     Lun,
  IN OUT UFS_SCSI_REQUEST_PACKET   *Packet
  )
{
  EFI_STATUS         Status;
  UINT8              Slot;
  UTP_TRD            *Trd;
  UINTN              Address;
  UINT8              *CmdDescBase;
  UINT32             CmdDescSize;
  UTP_RESPONSE_UPIU  *Response;
  UINT16             SenseDataLen;
  UINT32             ResTranCount;
  VOID               *PacketBufferMap;

  //
  // Find out which slot of transfer request list is available.
  //
  Status = UfsFindAvailableSlotInTrl (Private, &Slot);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Trd             = ((UTP_TRD *)Private->UtpTrlBase) + Slot;
  PacketBufferMap = NULL;

  //
  // Fill transfer request descriptor to this slot.
  //
  Status = UfsCreateScsiCommandDesc (Private, Lun, Packet, Trd, &PacketBufferMap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CmdDescBase = (UINT8 *)(UINTN)(LShiftU64 ((UINT64)Trd->UcdBaU, 32) | LShiftU64 ((UINT64)Trd->UcdBa, 7));
  CmdDescSize = Trd->PrdtO * sizeof (UINT32) + Trd->PrdtL * sizeof (UTP_TR_PRD);

  //
  // Start to execute the transfer request.
  //
  UfsStartExecCmd (Private, Slot);

  //
  // Wait for the completion of the transfer request.
  //
  Address = Private->UfsHcBase + UFS_HC_UTRLDBR_OFFSET;
  Status  = UfsWaitMemSet (Address, BIT0 << Slot, 0, Packet->Timeout);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Get sense data if exists
  //
  Response     = (UTP_RESPONSE_UPIU *)(CmdDescBase + Trd->RuO * sizeof (UINT32));
  SenseDataLen = Response->SenseDataLen;
  SwapLittleEndianToBigEndian ((UINT8 *)&SenseDataLen, sizeof (UINT16));

  if ((Packet->SenseDataLength != 0) && (Packet->SenseData != NULL)) {
    //
    // Make sure the hardware device does not return more data than expected.
    //
    if (SenseDataLen <= Packet->SenseDataLength) {
      CopyMem (Packet->SenseData, Response->SenseData, SenseDataLen);
      Packet->SenseDataLength = (UINT8)SenseDataLen;
    } else {
      Packet->SenseDataLength = 0;
    }
  }

  //
  // Check the transfer request result.
  //
  if (Response->Response != 0) {
    DEBUG ((DEBUG_ERROR, "UfsExecScsiCmds() fails with Target Failure\n"));
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  if (Trd->Ocs == 0) {
    if (Packet->DataDirection == UfsDataIn) {
      if ((Response->Flags & BIT5) == BIT5) {
        ResTranCount = Response->ResTranCount;
        SwapLittleEndianToBigEndian ((UINT8 *)&ResTranCount, sizeof (UINT32));
        Packet->InTransferLength -= ResTranCount;
      }
    } else if (Packet->DataDirection == UfsDataOut) {
      if ((Response->Flags & BIT5) == BIT5) {
        ResTranCount = Response->ResTranCount;
        SwapLittleEndianToBigEndian ((UINT8 *)&ResTranCount, sizeof (UINT32));
        Packet->OutTransferLength -= ResTranCount;
      }
    }
  } else {
    Status = EFI_DEVICE_ERROR;
  }

Exit:
  if (PacketBufferMap != NULL) {
    IoMmuUnmap (PacketBufferMap);
  }

  UfsStopExecCmd (Private, Slot);
  UfsPeimFreeMem (Private->Pool, CmdDescBase, CmdDescSize);

  return Status;
}

/**
  Sent UIC DME_LINKSTARTUP command to start the link startup procedure.

  @param[in] Private          The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.
  @param[in] UicOpcode        The opcode of the UIC command.
  @param[in] Arg1             The value for 1st argument of the UIC command.
  @param[in] Arg2             The value for 2nd argument of the UIC command.
  @param[in] Arg3             The value for 3rd argument of the UIC command.

  @return EFI_SUCCESS      Successfully execute this UIC command and detect attached UFS device.
  @return EFI_DEVICE_ERROR Fail to execute this UIC command and detect attached UFS device.
  @return EFI_NOT_FOUND    The presence of the UFS device isn't detected.

**/
EFI_STATUS
UfsExecUicCommands (
  IN  UFS_PEIM_HC_PRIVATE_DATA  *Private,
  IN  UINT8                     UicOpcode,
  IN  UINT32                    Arg1,
  IN  UINT32                    Arg2,
  IN  UINT32                    Arg3
  )
{
  EFI_STATUS  Status;
  UINTN       Address;
  UINT32      Data;
  UINTN       UfsHcBase;

  UfsHcBase = Private->UfsHcBase;
  Address   = UfsHcBase + UFS_HC_IS_OFFSET;
  Data      = MmioRead32 (Address);
  if ((Data & UFS_HC_IS_UCCS) == UFS_HC_IS_UCCS) {
    //
    // Clear IS.BIT10 UIC Command Completion Status (UCCS) at first.
    //
    MmioWrite32 (Address, Data);
  }

  //
  // When programming UIC command registers, host software shall set the register UICCMD
  // only after all the UIC command argument registers (UICCMDARG1, UICCMDARG2 and UICCMDARG3)
  // are set.
  //
  Address = UfsHcBase + UFS_HC_UCMD_ARG1_OFFSET;
  MmioWrite32 (Address, Arg1);

  Address = UfsHcBase + UFS_HC_UCMD_ARG2_OFFSET;
  MmioWrite32 (Address, Arg2);

  Address = UfsHcBase + UFS_HC_UCMD_ARG3_OFFSET;
  MmioWrite32 (Address, Arg3);

  //
  // Host software shall only set the UICCMD if HCS.UCRDY is set to 1.
  //
  Address = Private->UfsHcBase + UFS_HC_STATUS_OFFSET;
  Status  = UfsWaitMemSet (Address, UFS_HC_HCS_UCRDY, UFS_HC_HCS_UCRDY, UFS_TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Address = UfsHcBase + UFS_HC_UIC_CMD_OFFSET;
  MmioWrite32 (Address, (UINT32)UicOpcode);

  //
  // UFS 2.0 spec section 5.3.1 Offset:0x20 IS.Bit10 UIC Command Completion Status (UCCS)
  // This bit is set to '1' by the host controller upon completion of a UIC command.
  //
  Address = UfsHcBase + UFS_HC_IS_OFFSET;
  Data    = MmioRead32 (Address);
  Status  = UfsWaitMemSet (Address, UFS_HC_IS_UCCS, UFS_HC_IS_UCCS, UFS_TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (UicOpcode != UfsUicDmeReset) {
    Address = UfsHcBase + UFS_HC_UCMD_ARG2_OFFSET;
    Data    = MmioRead32 (Address);
    if ((Data & 0xFF) != 0) {
      DEBUG_CODE_BEGIN ();
      DumpUicCmdExecResult (UicOpcode, (UINT8)(Data & 0xFF));
      DEBUG_CODE_END ();
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}

/**
  Enable the UFS host controller for accessing.

  @param[in] UfsHcPlatformPpi        The pointer to the EDKII_UFS_HC_PLATFORM_PPI data structure
  @param[in] Private                 The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The UFS host controller enabling was executed successfully.
  @retval EFI_DEVICE_ERROR           A device error occurred while enabling the UFS host controller.

**/
EFI_STATUS
UfsEnableHostController (
  IN  EDKII_UFS_HC_PLATFORM_PPI  *UfsHcPlatformPpi,
  IN  UFS_PEIM_HC_PRIVATE_DATA   *Private
  )
{
  EFI_STATUS  Status;
  UINTN       Address;
  UINT32      Data;

  if ((UfsHcPlatformPpi != NULL) && (UfsHcPlatformPpi->Callback != NULL)) {
    Status = UfsHcPlatformPpi->Callback (&Private->UfsHcBase, EdkiiUfsHcPreHce);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failure from platform driver during EdkiiUfsHcPreHce, Status = %r\n", Status));
      return Status;
    }
  }

  //
  // UFS 2.0 spec section 7.1.1 - Host Controller Initialization
  //
  // Reinitialize the UFS host controller if HCE bit of HC register is set.
  //
  Address = Private->UfsHcBase + UFS_HC_ENABLE_OFFSET;
  Data    = MmioRead32 (Address);
  if ((Data & UFS_HC_HCE_EN) == UFS_HC_HCE_EN) {
    //
    // Write a 0 to the HCE register at first to disable the host controller.
    //
    MmioWrite32 (Address, 0);
    //
    // Wait until HCE is read as '0' before continuing.
    //
    Status = UfsWaitMemSet (Address, UFS_HC_HCE_EN, 0, UFS_TIMEOUT);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }
  }

  //
  // Write a 1 to the HCE register to enable the UFS host controller.
  //
  MmioWrite32 (Address, UFS_HC_HCE_EN);
  //
  // Wait until HCE is read as '1' before continuing.
  //
  Status = UfsWaitMemSet (Address, UFS_HC_HCE_EN, UFS_HC_HCE_EN, UFS_TIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if ((UfsHcPlatformPpi != NULL) && (UfsHcPlatformPpi->Callback != NULL)) {
    Status = UfsHcPlatformPpi->Callback (&Private->UfsHcBase, EdkiiUfsHcPostHce);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failure from platform driver during EdkiiUfsHcPostHce, Status = %r\n", Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Detect if a UFS device attached.

  @param[in] Private                 The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The UFS device detection was executed successfully.
  @retval EFI_NOT_FOUND              Not found a UFS device attached.
  @retval EFI_DEVICE_ERROR           A device error occurred while detecting the UFS device.

**/
EFI_STATUS
UfsDeviceDetection (
  IN  EDKII_UFS_HC_PLATFORM_PPI  *UfsHcPlatformPpi,
  IN  UFS_PEIM_HC_PRIVATE_DATA   *Private
  )
{
  UINTN       Retry;
  UINTN       Address;
  UINT32      Data;
  EFI_STATUS  Status;

  if ((UfsHcPlatformPpi != NULL) && (UfsHcPlatformPpi->Callback != NULL)) {
    Status = UfsHcPlatformPpi->Callback (&Private->UfsHcBase, EdkiiUfsHcPreLinkStartup);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failure from platform driver during EdkiiUfsHcPreLinkStartup, Status = %r\n", Status));
      return Status;
    }
  }

  //
  // Start UFS device detection.
  // Try up to 3 times for establishing data link with device.
  //
  for (Retry = 0; Retry < 3; Retry++) {
    Status = UfsExecUicCommands (Private, UfsUicDmeLinkStartup, 0, 0, 0);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    //
    // Check value of HCS.DP and make sure that there is a device attached to the Link
    //
    Address = Private->UfsHcBase + UFS_HC_STATUS_OFFSET;
    Data    = MmioRead32 (Address);
    if ((Data & UFS_HC_HCS_DP) == 0) {
      Address = Private->UfsHcBase + UFS_HC_IS_OFFSET;
      Status  = UfsWaitMemSet (Address, UFS_HC_IS_ULSS, UFS_HC_IS_ULSS, UFS_TIMEOUT);
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }
    } else {
      DEBUG ((DEBUG_INFO, "UfsblockioPei: found a attached UFS device\n"));
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Initialize UFS task management request list related h/w context.

  @param[in] Private                 The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The UFS task management list was initialzed successfully.
  @retval EFI_DEVICE_ERROR           The initialization fails.

**/
EFI_STATUS
UfsInitTaskManagementRequestList (
  IN  UFS_PEIM_HC_PRIVATE_DATA  *Private
  )
{
  UINTN                 Address;
  UINT32                Data;
  UINT8                 Nutmrs;
  VOID                  *CmdDescHost;
  EFI_PHYSICAL_ADDRESS  CmdDescPhyAddr;
  VOID                  *CmdDescMapping;
  EFI_STATUS            Status;

  //
  // Initial h/w and s/w context for future operations.
  //
  Address               = Private->UfsHcBase + UFS_HC_CAP_OFFSET;
  Data                  = MmioRead32 (Address);
  Private->Capabilities = Data;

  //
  // Allocate and initialize UTP Task Management Request List.
  //
  Nutmrs = (UINT8)(RShiftU64 ((Private->Capabilities & UFS_HC_CAP_NUTMRS), 16) + 1);
  Status = IoMmuAllocateBuffer (
             EFI_SIZE_TO_PAGES (Nutmrs * sizeof (UTP_TMRD)),
             &CmdDescHost,
             &CmdDescPhyAddr,
             &CmdDescMapping
             );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  ZeroMem (CmdDescHost, EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (Nutmrs * sizeof (UTP_TMRD))));

  //
  // Program the UTP Task Management Request List Base Address and UTP Task Management
  // Request List Base Address with a 64-bit address allocated at step 6.
  //
  Address = Private->UfsHcBase + UFS_HC_UTMRLBA_OFFSET;
  MmioWrite32 (Address, (UINT32)(UINTN)CmdDescPhyAddr);
  Address = Private->UfsHcBase + UFS_HC_UTMRLBAU_OFFSET;
  MmioWrite32 (Address, (UINT32)RShiftU64 ((UINT64)CmdDescPhyAddr, 32));
  Private->UtpTmrlBase = (VOID *)(UINTN)CmdDescHost;
  Private->Nutmrs      = Nutmrs;
  Private->TmrlMapping = CmdDescMapping;

  //
  // Enable the UTP Task Management Request List by setting the UTP Task Management
  // Request List RunStop Register (UTMRLRSR) to '1'.
  //
  Address = Private->UfsHcBase + UFS_HC_UTMRLRSR_OFFSET;
  MmioWrite32 (Address, UFS_HC_UTMRLRSR);

  return EFI_SUCCESS;
}

/**
  Initialize UFS transfer request list related h/w context.

  @param[in] Private                 The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The UFS transfer list was initialzed successfully.
  @retval EFI_DEVICE_ERROR           The initialization fails.

**/
EFI_STATUS
UfsInitTransferRequestList (
  IN  UFS_PEIM_HC_PRIVATE_DATA  *Private
  )
{
  UINTN                 Address;
  UINT32                Data;
  UINT8                 Nutrs;
  VOID                  *CmdDescHost;
  EFI_PHYSICAL_ADDRESS  CmdDescPhyAddr;
  VOID                  *CmdDescMapping;
  EFI_STATUS            Status;

  //
  // Initial h/w and s/w context for future operations.
  //
  Address               = Private->UfsHcBase + UFS_HC_CAP_OFFSET;
  Data                  = MmioRead32 (Address);
  Private->Capabilities = Data;

  //
  // Allocate and initialize UTP Transfer Request List.
  //
  Nutrs  = (UINT8)((Private->Capabilities & UFS_HC_CAP_NUTRS) + 1);
  Status = IoMmuAllocateBuffer (
             EFI_SIZE_TO_PAGES (Nutrs * sizeof (UTP_TRD)),
             &CmdDescHost,
             &CmdDescPhyAddr,
             &CmdDescMapping
             );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  ZeroMem (CmdDescHost, EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (Nutrs * sizeof (UTP_TRD))));

  //
  // Program the UTP Transfer Request List Base Address and UTP Transfer Request List
  // Base Address with a 64-bit address allocated at step 8.
  //
  Address = Private->UfsHcBase + UFS_HC_UTRLBA_OFFSET;
  MmioWrite32 (Address, (UINT32)(UINTN)CmdDescPhyAddr);
  Address = Private->UfsHcBase + UFS_HC_UTRLBAU_OFFSET;
  MmioWrite32 (Address, (UINT32)RShiftU64 ((UINT64)CmdDescPhyAddr, 32));
  Private->UtpTrlBase = (VOID *)(UINTN)CmdDescHost;
  Private->Nutrs      = Nutrs;
  Private->TrlMapping = CmdDescMapping;

  //
  // Enable the UTP Transfer Request List by setting the UTP Transfer Request List
  // RunStop Register (UTRLRSR) to '1'.
  //
  Address = Private->UfsHcBase + UFS_HC_UTRLRSR_OFFSET;
  MmioWrite32 (Address, UFS_HC_UTRLRSR);

  return EFI_SUCCESS;
}

/**
  Initialize the UFS host controller.

  @param[in] UfsHcPlatformPpi        The pointer to the EDKII_UFS_HC_PLATFORM_PPI data structure.    // APTIOV_OVERRIDE
  @param[in] Private                 The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The Ufs Host Controller is initialized successfully.
  @retval Others                     A device error occurred while initializing the controller.

**/
EFI_STATUS
UfsControllerInit (
  IN  EDKII_UFS_HC_PLATFORM_PPI  *UfsHcPlatformPpi,
  IN  UFS_PEIM_HC_PRIVATE_DATA   *Private
  )
{
  EFI_STATUS  Status;

  Status = UfsEnableHostController (UfsHcPlatformPpi, Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UfsDevicePei: Enable Host Controller Fails, Status = %r\n", Status));
    return Status;
  }

  Status = UfsDeviceDetection (UfsHcPlatformPpi, Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UfsDevicePei: Device Detection Fails, Status = %r\n", Status));
    return Status;
  }

  Status = UfsInitTaskManagementRequestList (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UfsDevicePei: Task management list initialization Fails, Status = %r\n", Status));
    return Status;
  }

  Status = UfsInitTransferRequestList (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UfsDevicePei: Transfer list initialization Fails, Status = %r\n", Status));

    if (Private->TmrlMapping != NULL) {
      IoMmuFreeBuffer (
        EFI_SIZE_TO_PAGES (Private->Nutmrs * sizeof (UTP_TMRD)),
        Private->UtpTmrlBase,
        Private->TmrlMapping
        );
      Private->TmrlMapping = NULL;
    }

    return Status;
  }

  DEBUG ((DEBUG_INFO, "UfsDevicePei Finished\n"));
  return EFI_SUCCESS;
}

/**
  Stop the UFS host controller.

  @param[in] Private                 The pointer to the UFS_PEIM_HC_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The Ufs Host Controller is stopped successfully.
  @retval Others                     A device error occurred while stopping the controller.

**/
EFI_STATUS
UfsControllerStop (
  IN  UFS_PEIM_HC_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS  Status;
  UINTN       Address;
  UINT32      Data;

  //
  // Enable the UTP Task Management Request List by setting the UTP Task Management
  // Request List RunStop Register (UTMRLRSR) to '1'.
  //
  Address = Private->UfsHcBase + UFS_HC_UTMRLRSR_OFFSET;
  MmioWrite32 (Address, 0);

  //
  // Enable the UTP Transfer Request List by setting the UTP Transfer Request List
  // RunStop Register (UTRLRSR) to '1'.
  //
  Address = Private->UfsHcBase + UFS_HC_UTRLRSR_OFFSET;
  MmioWrite32 (Address, 0);

  //
  // Write a 0 to the HCE register in order to disable the host controller.
  //
  Address = Private->UfsHcBase + UFS_HC_ENABLE_OFFSET;
  Data    = MmioRead32 (Address);
  ASSERT ((Data & UFS_HC_HCE_EN) == UFS_HC_HCE_EN);
  MmioWrite32 (Address, 0);

  //
  // Wait until HCE is read as '0' before continuing.
  //
  Status = UfsWaitMemSet (Address, UFS_HC_HCE_EN, 0, UFS_TIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  DEBUG ((DEBUG_INFO, "UfsDevicePei: Stop the UFS Host Controller\n"));

  return EFI_SUCCESS;
}
