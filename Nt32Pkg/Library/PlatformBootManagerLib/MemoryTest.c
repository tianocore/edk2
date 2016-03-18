/** @file
  Perform the platform memory test

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PlatformBootManager.h"

EFI_HII_HANDLE gStringPackHandle                  = NULL;
EFI_GUID       mPlatformBootManagerStringPackGuid = {
  0x154dd51, 0x9079, 0x4a10, { 0x89, 0x5c, 0x9c, 0x7, 0x72, 0x81, 0x57, 0x88 }
  };
// extern UINT8  BdsDxeStrings[];

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
  IN EXTENDMEM_COVERAGE_LEVEL Level
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
  UINTN                             TestPercent;
  UINT64                            PreviousValue;
  BOOLEAN                           ErrorOut;
  BOOLEAN                           TestAbort;
  EFI_INPUT_KEY                     Key;
  CHAR16                            StrPercent[80];
  CHAR16                            *StrTotalMemory;
  CHAR16                            *Pos;
  CHAR16                            *TmpStr;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL     Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL     Background;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL     Color;
  UINT32                            TempData;
  UINTN                             StrTotalMemorySize;

  ReturnStatus = EFI_SUCCESS;
  ZeroMem (&Key, sizeof (EFI_INPUT_KEY));

  StrTotalMemorySize = 128;
  Pos = AllocateZeroPool (StrTotalMemorySize);
  ASSERT (Pos != NULL);

  if (gStringPackHandle == NULL) {  
    gStringPackHandle = HiiAddPackages (
                           &mPlatformBootManagerStringPackGuid,
                           gImageHandle,
                           PlatformBootManagerLibStrings,
                           NULL
                           );
    ASSERT (gStringPackHandle != NULL);
  }

  StrTotalMemory    = Pos;

  TestedMemorySize  = 0;
  TotalMemorySize   = 0;
  PreviousValue     = 0;
  ErrorOut          = FALSE;
  TestAbort         = FALSE;

  SetMem (&Foreground, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);
  SetMem (&Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0x0);
  SetMem (&Color, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);

  RequireSoftECCInit = FALSE;

  Status = gBS->LocateProtocol (
                  &gEfiGenericMemTestProtocolGuid,
                  NULL,
                  (VOID **) &GenMemoryTest
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Pos);
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
    FreePool (Pos);
    return EFI_SUCCESS;
  }
  
  if (!FeaturePcdGet(PcdBootlogoOnlyEnable)) {
    TmpStr = HiiGetString (gStringPackHandle, STRING_TOKEN (STR_ESC_TO_SKIP_MEM_TEST), NULL);

    if (TmpStr != NULL) {
      PrintXY (10, 10, NULL, NULL, TmpStr);
      FreePool (TmpStr);
    }
  } else {
    DEBUG ((EFI_D_INFO, "Enter memory test.\n"));
  }
  do {
    Status = GenMemoryTest->PerformMemoryTest (
                              GenMemoryTest,
                              &TestedMemorySize,
                              &TotalMemorySize,
                              &ErrorOut,
                              TestAbort
                              );
    if (ErrorOut && (Status == EFI_DEVICE_ERROR)) {
      TmpStr = HiiGetString (gStringPackHandle, STRING_TOKEN (STR_SYSTEM_MEM_ERROR), NULL);
      if (TmpStr != NULL) {
        PrintXY (10, 10, NULL, NULL, TmpStr);
        FreePool (TmpStr);
      }

      ASSERT (0);
    }
    
    if (!FeaturePcdGet(PcdBootlogoOnlyEnable)) {
      TempData = (UINT32) DivU64x32 (TotalMemorySize, 16);
      TestPercent = (UINTN) DivU64x32 (
                              DivU64x32 (MultU64x32 (TestedMemorySize, 100), 16),
                              TempData
                              );
      if (TestPercent != PreviousValue) {
        UnicodeValueToString (StrPercent, 0, TestPercent, 0);
        TmpStr = HiiGetString (gStringPackHandle, STRING_TOKEN (STR_MEMORY_TEST_PERCENT), NULL);
        if (TmpStr != NULL) {
          //
          // TmpStr size is 64, StrPercent is reserved to 16.
          //
          StrnCatS (
            StrPercent,
            sizeof (StrPercent) / sizeof (CHAR16), 
            TmpStr,
            sizeof (StrPercent) / sizeof (CHAR16) - StrLen (StrPercent) - 1
            );
          PrintXY (10, 10, NULL, NULL, StrPercent);
          FreePool (TmpStr);
        }

        TmpStr = HiiGetString (gStringPackHandle, STRING_TOKEN (STR_PERFORM_MEM_TEST), NULL);
        if (TmpStr != NULL) {
          BootLogoUpdateProgress (
            Foreground,
            Background,
            TmpStr,
            Color,
            TestPercent,
            (UINTN) PreviousValue
            );
          FreePool (TmpStr);
        }
      }

      PreviousValue = TestPercent;
    } else {
      DEBUG ((EFI_D_INFO, "Perform memory test (ESC to skip).\n"));
    }

    if (!PcdGetBool (PcdConInConnectOnDemand)) {
      KeyStatus     = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
      if (!EFI_ERROR (KeyStatus) && (Key.ScanCode == SCAN_ESC)) {
        if (!RequireSoftECCInit) {
          if (!FeaturePcdGet(PcdBootlogoOnlyEnable)) {
            TmpStr = HiiGetString (gStringPackHandle, STRING_TOKEN (STR_PERFORM_MEM_TEST), NULL);
            if (TmpStr != NULL) {
              BootLogoUpdateProgress (
                Foreground,
                Background,
                TmpStr,
                Color,
                100,
                (UINTN) PreviousValue
                );
              FreePool (TmpStr);
            }

            PrintXY (10, 10, NULL, NULL, L"100");
          }
          Status = GenMemoryTest->Finished (GenMemoryTest);
          goto Done;
        }

        TestAbort = TRUE;
      }
    }
  } while (Status != EFI_NOT_FOUND);

  Status = GenMemoryTest->Finished (GenMemoryTest);

Done:
  if (!FeaturePcdGet(PcdBootlogoOnlyEnable)) {
    UnicodeValueToString (StrTotalMemory, COMMA_TYPE, TotalMemorySize, 0);
    if (StrTotalMemory[0] == L',') {
      StrTotalMemory++;
      StrTotalMemorySize -= sizeof (CHAR16);
    }

    TmpStr = HiiGetString (gStringPackHandle, STRING_TOKEN (STR_MEM_TEST_COMPLETED), NULL);
    if (TmpStr != NULL) {
      StrnCatS (
        StrTotalMemory,
        StrTotalMemorySize / sizeof (CHAR16),
        TmpStr,
        StrTotalMemorySize / sizeof (CHAR16) - StrLen (StrTotalMemory) - 1
        );
      FreePool (TmpStr);
    }

    PrintXY (10, 10, NULL, NULL, StrTotalMemory);
    BootLogoUpdateProgress (
      Foreground,
      Background,
      StrTotalMemory,
      Color,
      100,
      (UINTN) PreviousValue
      );
    
  } else {
    DEBUG ((EFI_D_INFO, "%d bytes of system memory tested OK\r\n", TotalMemorySize));
  }
  
  FreePool (Pos);
  return ReturnStatus;
}
