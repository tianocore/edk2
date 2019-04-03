/** @file
  Perform the platform memory test

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Bds.h"
#include "String.h"

//
// BDS Platform Functions
//
/**

  Show progress bar with title above it. It only works in Graphics mode.


  @param TitleForeground Foreground color for Title.
  @param TitleBackground Background color for Title.
  @param Title           Title above progress bar.
  @param ProgressColor   Progress bar color.
  @param Progress        Progress (0-100)
  @param PreviousValue   The previous value of the progress.

  @retval  EFI_STATUS       Success update the progress bar

**/
EFI_STATUS
PlatformBdsShowProgress (
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleForeground,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleBackground,
  IN CHAR16                        *Title,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL ProgressColor,
  IN UINTN                         Progress,
  IN UINTN                         PreviousValue
  )
{
  EFI_STATUS                     Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL   *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL          *UgaDraw;
  UINT32                         SizeOfX;
  UINT32                         SizeOfY;
  UINT32                         ColorDepth;
  UINT32                         RefreshRate;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Color;
  UINTN                          BlockHeight;
  UINTN                          BlockWidth;
  UINTN                          BlockNum;
  UINTN                          PosX;
  UINTN                          PosY;
  UINTN                          Index;

  if (Progress > 100) {
    return EFI_INVALID_PARAMETER;
  }

  UgaDraw = NULL;
  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **) &GraphicsOutput
                  );
  if (EFI_ERROR (Status) && FeaturePcdGet (PcdUgaConsumeSupport)) {
    GraphicsOutput = NULL;

    Status = gBS->HandleProtocol (
                    gST->ConsoleOutHandle,
                    &gEfiUgaDrawProtocolGuid,
                    (VOID **) &UgaDraw
                    );
  }
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  SizeOfX = 0;
  SizeOfY = 0;
  if (GraphicsOutput != NULL) {
    SizeOfX = GraphicsOutput->Mode->Info->HorizontalResolution;
    SizeOfY = GraphicsOutput->Mode->Info->VerticalResolution;
  } else if (UgaDraw != NULL) {
    Status = UgaDraw->GetMode (
                        UgaDraw,
                        &SizeOfX,
                        &SizeOfY,
                        &ColorDepth,
                        &RefreshRate
                        );
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
  } else {
    return EFI_UNSUPPORTED;
  }

  BlockWidth  = SizeOfX / 100;
  BlockHeight = SizeOfY / 50;

  BlockNum    = Progress;

  PosX        = 0;
  PosY        = SizeOfY * 48 / 50;

  if (BlockNum == 0) {
    //
    // Clear progress area
    //
    SetMem (&Color, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0x0);

    if (GraphicsOutput != NULL) {
      Status = GraphicsOutput->Blt (
                          GraphicsOutput,
                          &Color,
                          EfiBltVideoFill,
                          0,
                          0,
                          0,
                          PosY - EFI_GLYPH_HEIGHT - 1,
                          SizeOfX,
                          SizeOfY - (PosY - EFI_GLYPH_HEIGHT - 1),
                          SizeOfX * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                          );
    } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
      Status = UgaDraw->Blt (
                          UgaDraw,
                          (EFI_UGA_PIXEL *) &Color,
                          EfiUgaVideoFill,
                          0,
                          0,
                          0,
                          PosY - EFI_GLYPH_HEIGHT - 1,
                          SizeOfX,
                          SizeOfY - (PosY - EFI_GLYPH_HEIGHT - 1),
                          SizeOfX * sizeof (EFI_UGA_PIXEL)
                          );
    } else {
      return EFI_UNSUPPORTED;
    }
  }
  //
  // Show progress by drawing blocks
  //
  for (Index = PreviousValue; Index < BlockNum; Index++) {
    PosX = Index * BlockWidth;
    if (GraphicsOutput != NULL) {
      Status = GraphicsOutput->Blt (
                          GraphicsOutput,
                          &ProgressColor,
                          EfiBltVideoFill,
                          0,
                          0,
                          PosX,
                          PosY,
                          BlockWidth - 1,
                          BlockHeight,
                          (BlockWidth) * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                          );
    } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
      Status = UgaDraw->Blt (
                          UgaDraw,
                          (EFI_UGA_PIXEL *) &ProgressColor,
                          EfiUgaVideoFill,
                          0,
                          0,
                          PosX,
                          PosY,
                          BlockWidth - 1,
                          BlockHeight,
                          (BlockWidth) * sizeof (EFI_UGA_PIXEL)
                          );
    } else {
      return EFI_UNSUPPORTED;
    }
  }

  PrintXY (
    (SizeOfX - StrLen (Title) * EFI_GLYPH_WIDTH) / 2,
    PosY - EFI_GLYPH_HEIGHT - 1,
    &TitleForeground,
    &TitleBackground,
    Title
    );

  return EFI_SUCCESS;
}

/**
  Perform the memory test base on the memory test intensive level,
  and update the memory resource.

  @param  Level         The memory test intensive level.

  @retval EFI_STATUS    Success test all the system memory and update
                        the memory resource

**/
EFI_STATUS
EFIAPI
BdsMemoryTest (
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
  BOOLEAN                           IsFirstBoot;
  UINT32                            TempData;
  UINTN                             StrTotalMemorySize;

  ReturnStatus = EFI_SUCCESS;
  ZeroMem (&Key, sizeof (EFI_INPUT_KEY));

  StrTotalMemorySize = 128;
  Pos = AllocateZeroPool (StrTotalMemorySize);

  if (Pos == NULL) {
    return ReturnStatus;
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
    TmpStr = GetStringById (STRING_TOKEN (STR_ESC_TO_SKIP_MEM_TEST));

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
      TmpStr = GetStringById (STRING_TOKEN (STR_SYSTEM_MEM_ERROR));
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
        UnicodeValueToStringS (StrPercent, sizeof (StrPercent), 0, TestPercent, 0);
        TmpStr = GetStringById (STRING_TOKEN (STR_MEMORY_TEST_PERCENT));
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

        TmpStr = GetStringById (STRING_TOKEN (STR_PERFORM_MEM_TEST));
        if (TmpStr != NULL) {
          PlatformBdsShowProgress (
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
            TmpStr = GetStringById (STRING_TOKEN (STR_PERFORM_MEM_TEST));
            if (TmpStr != NULL) {
              PlatformBdsShowProgress (
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
    UnicodeValueToStringS (StrTotalMemory, StrTotalMemorySize, COMMA_TYPE, TotalMemorySize, 0);
    if (StrTotalMemory[0] == L',') {
      StrTotalMemory++;
      StrTotalMemorySize -= sizeof (CHAR16);
    }

    TmpStr = GetStringById (STRING_TOKEN (STR_MEM_TEST_COMPLETED));
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
    PlatformBdsShowProgress (
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


  //
  // Use a DynamicHii type pcd to save the boot status, which is used to
  // control configuration mode, such as FULL/MINIMAL/NO_CHANGES configuration.
  //
  IsFirstBoot = PcdGetBool(PcdBootState);
  if (IsFirstBoot) {
    Status = PcdSetBoolS(PcdBootState, FALSE);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Set PcdBootState to FALSE failed.\n"));
    }
  }

  return ReturnStatus;
}
