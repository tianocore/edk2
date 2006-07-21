/** @file
  Status Code Architectural Protocol implementation as defined in Tiano
  Architecture Specification.

  This driver has limited functionality at runtime and will not log to Data Hub
  at runtime.

  Notes:
  This driver assumes the following ReportStatusCode strategy:
  PEI       -> uses PeiReportStatusCode
  DXE IPL   -> uses PeiReportStatusCode
  early DXE -> uses PeiReportStatusCode via HOB
  DXE       -> This driver
  RT        -> This driver

// Copyright (c) 2006, Intel Corporation. All rights reserved. 
// This software and associated documentation (if any) is furnished
// under a license and may only be used or copied in accordance
// with the terms of the license. Except as permitted by such
// license, no part of this software or documentation may be
// reproduced, stored in a retrieval system, or transmitted in any
// form or by any means without the express written consent of
// Intel Corporation.

  Module Name:  StatusCode.c

**/

#include "DxeStatusCode.h"

/**
  
  Dispatch initialization request to sub status code devices based on 
  customized feature flags.
 
**/
VOID
InitializationDispatcherWorker (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS              Hob;
  MEMORY_STATUSCODE_PACKET_HEADER   *PacketHeader;
  MEMORY_STATUSCODE_RECORD          *Record;
  UINTN                             ExpectedPacketIndex = 0;
  UINTN                             Index;
  VOID                              *HobStart;

  //
  // If enable UseSerial, then initialize serial port.
  // if enable UseRuntimeMemory, then initialize runtime memory status code worker.
  // if enable UseDataHub, then initialize data hub status code worker.
  //
  if (FeaturePcdGet (PcdStatusCodeUseEfiSerial)) {
    EfiSerialStatusCodeInitializeWorker ();
  }
  if (FeaturePcdGet (PcdStatusCodeUseHardSerial)) {
    SerialPortInitialize ();
  }
  if (FeaturePcdGet (PcdStatusCodeUseRuntimeMemory)) {
    RtMemoryStatusCodeInitializeWorker ();
  }
  if (FeaturePcdGet (PcdStatusCodeUseDataHub)) {
    DataHubStatusCodeInitializeWorker ();
  }
  if (FeaturePcdGet (PcdStatusCodeUseOEM)) {
    OemHookStatusCodeInitialize ();
  }

  //
  // Replay Status code which saved in GUID'ed HOB to all supported device. 
  //

  // 
  // Journal GUID'ed HOBs to find all record entry, if found, 
  // then output record to support replay device.
  //
  Hob.Raw   = GetFirstGuidHob (&gMemoryStatusCodeRecordGuid);
  HobStart  = Hob.Raw;
  while (Hob.Raw != NULL) {
    PacketHeader = (MEMORY_STATUSCODE_PACKET_HEADER *) GET_GUID_HOB_DATA (Hob.Guid);
    if (PacketHeader->PacketIndex == ExpectedPacketIndex) {
      Record = (MEMORY_STATUSCODE_RECORD *) (PacketHeader + 1);
      for (Index = 0; Index < PacketHeader->RecordIndex; Index++) {
        //
        // Dispatch records to devices based on feature flag.
        //
        if (FeaturePcdGet (PcdStatusCodeReplayInSerial) && 
            (FeaturePcdGet (PcdStatusCodeUseHardSerial) ||
             FeaturePcdGet (PcdStatusCodeUseEfiSerial))) {
          SerialStatusCodeReportWorker (
            Record[Index].CodeType,
            Record[Index].Value,
            Record[Index].Instance,
            NULL,
            NULL
            );
        }
        if (FeaturePcdGet (PcdStatusCodeReplayInRuntimeMemory) &&
            FeaturePcdGet (PcdStatusCodeUseRuntimeMemory)) {
          RtMemoryStatusCodeReportWorker (
            gDxeStatusCode.RtMemoryStatusCodeTable[PHYSICAL_MODE],
            Record[Index].CodeType,
            Record[Index].Value,
            Record[Index].Instance
            );
        }
        if (FeaturePcdGet (PcdStatusCodeReplayInDataHub) &&
            FeaturePcdGet (PcdStatusCodeUseDataHub)) {
          DataHubStatusCodeReportWorker (
            Record[Index].CodeType,
            Record[Index].Value,
            Record[Index].Instance,
            NULL,
            NULL
            );
        }
        if (FeaturePcdGet (PcdStatusCodeReplayInOEM) &&
            FeaturePcdGet (PcdStatusCodeUseOEM)) {
          OemHookStatusCodeReport (
            Record[Index].CodeType,
            Record[Index].Value,
            Record[Index].Instance,
            NULL,
            NULL
            );
        }
      }
      ExpectedPacketIndex++;

      //
      // See whether there is gap of packet or not
      //
      if (HobStart) {
        HobStart  = NULL;
        Hob.Raw   = HobStart;
        continue;
      }
    } else if (HobStart != NULL) {
      //
      // Cache the found packet for improve the performance
      //
      HobStart = Hob.Raw;
    }

    Hob.Raw = GetNextGuidHob (&gMemoryStatusCodeRecordGuid, Hob.Raw);
  }
}

