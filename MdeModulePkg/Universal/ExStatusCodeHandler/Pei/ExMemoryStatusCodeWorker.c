/** @file
  PEI memory status code worker.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "ExStatusCodeHandlerPei.h"

/**
  Create the first memory status code GUID'ed HOB as initialization for the
  memory status code worker.

  @retval EFI_SUCCESS  The GUID'ed HOB was created successfully.

**/
EFI_STATUS
MemoryStatusCodeInitializeWorker (
  VOID
  )
{
  MEMORY_STATUSCODE_PACKET_HEADER  *PacketHeader;

  //
  // Create a GUID'ed HOB using the PCD-defined size.
  //
  PacketHeader = BuildGuidHob (
                   &gMemoryStatusCodeRecordGuid,
                   PcdGet16 (PcdStatusCodeMemorySize) * 1024 + sizeof (MEMORY_STATUSCODE_PACKET_HEADER)
                   );
  ASSERT (PacketHeader != NULL);

  PacketHeader->MaxRecordsNumber = (PcdGet16 (PcdStatusCodeMemorySize) * 1024) / sizeof (MEMORY_STATUSCODE_RECORD);
  PacketHeader->PacketIndex      = 0;
  PacketHeader->RecordIndex      = 0;

  return EFI_SUCCESS;
}

/**
  Report a status code into the GUID'ed HOB.

  This function reports a status code into the GUID'ed HOB. If the current
  packet still has room, the status code is written into the next available
  entry. Otherwise, recording wraps to the beginning of the packet.

  @param  PeiServices      An indirect pointer to the EFI_PEI_SERVICES table
                           published by the PEI Foundation.
  @param  CodeType         Indicates the type of status code being reported.
  @param  Value            Describes the current status of a hardware or
                           software entity. This includes information about the
                           class and subclass that are used to classify the
                           entity as well as an operation. For progress codes,
                           the operation is the current activity. For error
                           codes, it is the exception. For debug codes, it is
                           not defined at this time.
  @param  Instance         The enumeration of a hardware or software entity
                           within the system. A system may contain multiple
                           entities that match a class/subclass pairing. The
                           instance differentiates between them. An instance of
                           0 indicates that instance information is unavailable,
                           not meaningful, or not relevant. Valid instance
                           numbers start with 1.
  @param  CallerId         This optional parameter may be used to identify the
                           caller. This parameter allows the status code driver
                           to apply different rules to different callers.
  @param  Data             This optional parameter may be used to pass
                           additional data.

  @retval EFI_SUCCESS      The function always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
MemoryStatusCodeReportWorker (
  IN CONST  EFI_PEI_SERVICES     **PeiServices,
  IN EFI_STATUS_CODE_TYPE        CodeType,
  IN EFI_STATUS_CODE_VALUE       Value,
  IN UINT32                      Instance,
  IN CONST EFI_GUID              *CallerId,
  IN CONST EFI_STATUS_CODE_DATA  *Data OPTIONAL
  )
{
  EFI_PEI_HOB_POINTERS             Hob;
  MEMORY_STATUSCODE_PACKET_HEADER  *PacketHeader;
  MEMORY_STATUSCODE_RECORD         *Record;

  //
  // Find the GUID'ed HOB that contains the current record buffer.
  //
  Hob.Raw = GetFirstGuidHob (&gMemoryStatusCodeRecordGuid);
  ASSERT (Hob.Raw != NULL);

  PacketHeader = (MEMORY_STATUSCODE_PACKET_HEADER *)GET_GUID_HOB_DATA (Hob.Guid);
  Record       = (MEMORY_STATUSCODE_RECORD *)(PacketHeader + 1);
  Record       = &Record[PacketHeader->RecordIndex++];

  //
  // Save the status code.
  //
  Record->CodeType = CodeType;
  Record->Instance = Instance;
  Record->Value    = Value;

  //
  // If the record index reaches the maximum record count, wrap it back to the
  // beginning. Consumers can detect wrap-around by comparing the total number
  // of recorded entries with MaxRecordsNumber.
  //
  if (PacketHeader->RecordIndex == PacketHeader->MaxRecordsNumber) {
    PacketHeader->RecordIndex = 0;
    PacketHeader->PacketIndex++;
  }

  return EFI_SUCCESS;
}
