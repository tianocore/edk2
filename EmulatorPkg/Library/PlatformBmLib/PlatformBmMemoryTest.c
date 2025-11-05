/** @file
  Perform the platform memory test

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PlatformBm.h"

//
// BDS Platform Functions
//

/**
  Perform the memory test base on the memory test intensive level,
  and update the memory resource.

  @param  Level         The memory test intensive level.

  @retval EFI_STATUS    Success test all the system memory and update
                        the memory resource

**/
EFI_STATUS
PlatformBootManagerMemoryTest (
  IN EXTENDMEM_COVERAGE_LEVEL  Level
  )
{
  EFI_STATUS                        Status;
  EFI_STATUS                        KeyStatus;
  EFI_STATUS                        InitStatus;
  EFI_STATUS                        ReturnStatus;
  BOOLEAN                           RequireSoftECCInit;
  EFI_GENERIC_MEMORY_TEST_PROTOCOL  *GenMemoryTest;
  UINT64                            TestedMemorySize;
  UINT64                            TotalMemorySize;
  BOOLEAN                           ErrorOut;
  BOOLEAN                           TestAbort;
  EFI_INPUT_KEY                     Key;

  ReturnStatus = EFI_SUCCESS;
  ZeroMem (&Key, sizeof (EFI_INPUT_KEY));

  TestedMemorySize = 0;
  TotalMemorySize  = 0;
  ErrorOut         = FALSE;
  TestAbort        = FALSE;

  RequireSoftECCInit = FALSE;

  Status = gBS->LocateProtocol (
                  &gEfiGenericMemTestProtocolGuid,
                  NULL,
                  (VOID **)&GenMemoryTest
                  );
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  InitStatus = GenMemoryTest->MemoryTestInit (
                                GenMemoryTest,
                                Level,
                                &RequireSoftECCInit
                                );
  if (InitStatus == EFI_NO_MEDIA) {
    //
    // The PEI codes also have the relevant memory test code to check the memory,
    // it can select to test some range of the memory or all of them. If PEI code
    // checks all the memory, this BDS memory test will has no not-test memory to
    // do the test, and then the status of EFI_NO_MEDIA will be returned by
    // "MemoryTestInit". So it does not need to test memory again, just return.
    //
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "Enter memory test.\n"));
  do {
    Status = GenMemoryTest->PerformMemoryTest (
                              GenMemoryTest,
                              &TestedMemorySize,
                              &TotalMemorySize,
                              &ErrorOut,
                              TestAbort
                              );
    if (ErrorOut && (Status == EFI_DEVICE_ERROR)) {
      PrintXY (10, 10, NULL, NULL, L"Memory Testing failed!");
      ASSERT (0);
    }

    DEBUG ((DEBUG_INFO, "Perform memory test (ESC to skip).\n"));

    if (!PcdGetBool (PcdConInConnectOnDemand)) {
      KeyStatus = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
      if (!EFI_ERROR (KeyStatus) && (Key.ScanCode == SCAN_ESC)) {
        if (!RequireSoftECCInit) {
          Status = GenMemoryTest->Finished (GenMemoryTest);
          goto Done;
        }

        TestAbort = TRUE;
      }
    }
  } while (Status != EFI_NOT_FOUND);

  Status = GenMemoryTest->Finished (GenMemoryTest);

Done:
  DEBUG ((DEBUG_INFO, "%d bytes of system memory tested OK\r\n", TotalMemorySize));

  return ReturnStatus;
}
