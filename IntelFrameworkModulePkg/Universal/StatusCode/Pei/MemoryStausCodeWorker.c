/** @file
  Memory status code worker in PEI.

  Copyright (c) 2006, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  MemoryStatusCodeWorker.c

**/

#include "PeiStatusCode.h"

/**
  Create one memory status code GUID'ed HOB, use PacketIndex
  to identify the packet.

  @param   PacketIndex    Index of records packet.

  @return                 Always return pointer of memory status code packet.

**/
MEMORY_STATUSCODE_PACKET_HEADER *
CreateMemoryStatusCodePacket (
  UINT16 PacketIndex
  )
{
  MEMORY_STATUSCODE_PACKET_HEADER *PacketHeader;

  //
  // Build GUID'ed HOB with PCD defined size.
  //
  PacketHeader =
    (MEMORY_STATUSCODE_PACKET_HEADER *) BuildGuidHob (
                                          &gMemoryStatusCodeRecordGuid,
                                          PcdGet16 (PcdStatusCodeMemorySize) *
                                          1024 +
                                          sizeof (MEMORY_STATUSCODE_PACKET_HEADER)
                                          );
  ASSERT (PacketHeader != NULL);

  PacketHeader->MaxRecordsNumber = (PcdGet16 (PcdStatusCodeMemorySize) * 1024)/ sizeof (MEMORY_STATUSCODE_RECORD);
  PacketHeader->PacketIndex      = PacketIndex;
  PacketHeader->RecordIndex      = 0;

  return PacketHeader;
}



/**
  Initialize memory status code.
  Create one GUID'ed HOB with PCD defined size. If create required size
  GUID'ed HOB failed, then ASSERT().

  @return           The function always return EFI_SUCCESS

**/
EFI_STATUS
MemoryStatusCodeInitializeWorker (
  VOID
  )
{
  //
  // Create first memory status code GUID'ed HOB.
  //
  CreateMemoryStatusCodePacket (0);

  return EFI_SUCCESS;
}


/**
  Report status code into GUID'ed HOB..

  @param  CodeType      Indicates the type of status code being reported.  Type EFI_STATUS_CODE_TYPE is defined in "Related Definitions" below.

  @param  Value         Describes the current status of a hardware or software entity.
                        This included information about the class and subclass that is used to classify the entity
                        as well as an operation.  For progress codes, the operation is the current activity.
                        For error codes, it is the exception.  For debug codes, it is not defined at this time.
                        Type EFI_STATUS_CODE_VALUE is defined in "Related Definitions" below.
                        Specific values are discussed in the Intel? Platform Innovation Framework for EFI Status Code Specification.

  @param  Instance      The enumeration of a hardware or software entity within the system.
                        A system may contain multiple entities that match a class/subclass pairing.
                        The instance differentiates between them.  An instance of 0 indicates that instance information is unavailable,
                        not meaningful, or not relevant.  Valid instance numbers start with 1.

  @return               The function always return EFI_SUCCESS.

**/
EFI_STATUS
MemoryStatusCodeReportWorker (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance
  )
{

  EFI_PEI_HOB_POINTERS              Hob;
  MEMORY_STATUSCODE_PACKET_HEADER   *PacketHeader;
  MEMORY_STATUSCODE_RECORD          *Record     = NULL;
  UINT16                            PacketIndex = 0;;

  //
  // Journal GUID'ed HOBs to find empty record entry, if found, then save status code in it.
  // otherwise, create a new GUID'ed HOB.
  //
  Hob.Raw = GetFirstGuidHob (&gMemoryStatusCodeRecordGuid);
  while (Hob.Raw != NULL) {
    PacketHeader = (MEMORY_STATUSCODE_PACKET_HEADER *) GET_GUID_HOB_DATA (Hob.Guid);

    //
    // Check whether pccket is full or not.
    //
    if (PacketHeader->RecordIndex < PacketHeader->MaxRecordsNumber) {
      Record = (MEMORY_STATUSCODE_RECORD *) (PacketHeader + 1);
      Record = &Record[PacketHeader->RecordIndex++];
      break;
    }
    //
    // Cache number of found packet in PacketIndex.
    //
    PacketIndex++;

    Hob.Raw = GetNextGuidHob (&gMemoryStatusCodeRecordGuid, Hob.Raw);
  }

  if (NULL == Record) {
    //
    // In order to save status code , create new packet.
    //
    PacketHeader = CreateMemoryStatusCodePacket (PacketIndex);

    Record = (MEMORY_STATUSCODE_RECORD *) (PacketHeader + 1);
    Record = &Record[PacketHeader->RecordIndex++];
  }

  Record->CodeType = CodeType;
  Record->Instance = Instance;
  Record->Value    = Value;

  return EFI_SUCCESS;
}

