/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:


  IdccInfo.c

Abstract:

  Platform information used by IDCC.

Revision History

--*/

#include "PlatformDxe.h"

#include <Guid/IdccData.h>

extern EFI_GUID mPlatformDriverGuid;


EFI_STATUS
WriteIdccInfo (
  )
{
  EFI_STATUS                Status;
  EFI_DATA_HUB_PROTOCOL     *DataHub;
  UINT8                     Ratio;
  EFI_IDCC_PROCESSOR_RATIO  ProcRatio;

  //
  // Locate the data hub protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiDataHubProtocolGuid,
                  NULL,
                  (VOID **) &DataHub
                  );

  //
  // Find processor actual ratio
  //
  Ratio = 15; //Temporary - some dummy value.

  //
  // Fill in IDCC Type 5 structure
  //
  ProcRatio.IdccHeader.Type = EFI_IDCC_PROC_RATIO_TYPE;
  ProcRatio.IdccHeader.RecordLength = sizeof(EFI_IDCC_PROCESSOR_RATIO);
  ProcRatio.ProcessorRatio = Ratio;

  //
  // Write data to the data hub
  //
  Status = DataHub->LogData (
                      DataHub,
                      &gIdccDataHubGuid,
                      &mPlatformDriverGuid,
                      EFI_DATA_RECORD_CLASS_DATA,
                      &ProcRatio,
                      sizeof(EFI_IDCC_PROCESSOR_RATIO)
                      );

  return Status;
}
