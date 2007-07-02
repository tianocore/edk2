/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  MemoryTest.c

Abstract:

  Perform the platform memory test

--*/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "bds.h"
#include "BdsPlatform.h"
#include "String.h"

//
// BDS Platform Functions
//
EFI_STATUS
PlatformBdsShowProgress (
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleForeground,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleBackground,
  IN CHAR16        *Title,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL ProgressColor,
  IN UINTN         Progress,
  IN UINTN         PreviousValue
  )
/*++

Routine Description:

  Show progress bar with title above it. It only works in UGA mode.

Arguments:

  TitleForeground - Foreground color for Title.
  TitleBackground - Background color for Title.
  Title           - Title above progress bar.
  ProgressColor   - Progress bar color.
  Progress        - Progress (0-100)

Returns:

  EFI_STATUS      - Success update the progress bar

--*/
{
  EFI_STATUS            Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL   *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL *UgaDraw;
  UINT32                SizeOfX;
  UINT32                SizeOfY;
  UINT32                ColorDepth;
  UINT32                RefreshRate;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Color;
  UINTN                 BlockHeight;
  UINTN                 BlockWidth;
  UINTN                 BlockNum;
  UINTN                 PosX;
  UINTN                 PosY;
  UINTN                 Index;

  if (Progress > 100) {
    return EFI_INVALID_PARAMETER;
  }

  UgaDraw = NULL;
  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  &GraphicsOutput
                  );
  if (EFI_ERROR (Status)) {
    GraphicsOutput = NULL;

    Status = gBS->HandleProtocol (
                    gST->ConsoleOutHandle,
                    &gEfiUgaDrawProtocolGuid,
                    &UgaDraw
                    );
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
  }

  if (GraphicsOutput != NULL) {
    SizeOfX = GraphicsOutput->Mode->Info->HorizontalResolution;
    SizeOfY = GraphicsOutput->Mode->Info->VerticalResolution;
  } else {
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
                          PosY - GLYPH_HEIGHT - 1,
                          SizeOfX,
                          SizeOfY - (PosY - GLYPH_HEIGHT - 1),
                          SizeOfX * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                          );
    } else {
      Status = UgaDraw->Blt (
                          UgaDraw,
                          (EFI_UGA_PIXEL *) &Color,
                          EfiUgaVideoFill,
                          0,
                          0,
                          0,
                          PosY - GLYPH_HEIGHT - 1,
                          SizeOfX,
                          SizeOfY - (PosY - GLYPH_HEIGHT - 1),
                          SizeOfX * sizeof (EFI_UGA_PIXEL)
                          );
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
    } else {
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
    }
  }

  PrintXY (
    (SizeOfX - StrLen (Title) * GLYPH_WIDTH) / 2,
    PosY - GLYPH_HEIGHT - 1,
    &TitleForeground,
    &TitleBackground,
    Title
    );

  return EFI_SUCCESS;
}

EFI_STATUS
BdsMemoryTest (
  IN EXTENDMEM_COVERAGE_LEVEL Level
  )
/*++

Routine Description:

  Perform the memory test base on the memory test intensive level,
  and update the memory resource.

Arguments:

  Level  - The memory test intensive level.

Returns:

  EFI_STATUS      - Success test all the system memory and update
                    the memory resource

--*/
{
  EFI_STATUS                        Status;
  EFI_STATUS                        InitStatus;
  EFI_STATUS                        KeyStatus;
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
  CHAR16                            StrPercent[16];
  CHAR16                            *StrTotalMemory;
  CHAR16                            *Pos;
  CHAR16                            *TmpStr;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL     Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL     Background;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL     Color;
  UINT8                             Value;
  UINTN                             DataSize;

  ReturnStatus = EFI_SUCCESS;
  ZeroMem (&Key, sizeof (EFI_INPUT_KEY));

  Pos = AllocatePool (128);

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

  gST->ConOut->ClearScreen (gST->ConOut);
  gST->ConOut->SetAttribute (gST->ConOut, EFI_YELLOW | EFI_BRIGHT);
  gST->ConOut->EnableCursor (gST->ConOut, FALSE);

  Status = gBS->LocateProtocol (
                  &gEfiGenericMemTestProtocolGuid,
                  NULL,
                  &GenMemoryTest
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

  gST->ConOut->SetCursorPosition (gST->ConOut, 0, 2);
  TmpStr = GetStringById (STRING_TOKEN (STR_ESC_TO_SKIP_MEM_TEST));

  if (TmpStr != NULL) {
    gST->ConOut->OutputString (gST->ConOut, TmpStr);
    FreePool (TmpStr);
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
        gST->ConOut->SetCursorPosition (gST->ConOut, 0, 4);
        gST->ConOut->OutputString (gST->ConOut, TmpStr);
        FreePool (TmpStr);
      }

      ASSERT (0);
    }

    TestPercent = (UINTN) DivU64x32 (
                            DivU64x32 (MultU64x32 (TestedMemorySize, 100), 16),
                            (UINTN)DivU64x32 (TotalMemorySize, 16)
                            );
    if (TestPercent != PreviousValue) {
      UnicodeValueToString (StrPercent, 0, TestPercent, 0);
      gST->ConOut->SetCursorPosition (gST->ConOut, 0, 0);
      TmpStr = GetStringById (STRING_TOKEN (STR_MEMORY_TEST_PERCENT));
      if (TmpStr != NULL) {
        BdsLibOutputStrings (gST->ConOut, StrPercent, TmpStr, NULL);
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

    KeyStatus     = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    if (Key.ScanCode == SCAN_ESC) {
      if (!RequireSoftECCInit) {
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

        gST->ConOut->SetCursorPosition (gST->ConOut, 0, 0);
        gST->ConOut->OutputString (gST->ConOut, L"100");
        Status = GenMemoryTest->Finished (GenMemoryTest);
        goto Done;
      }

      TestAbort = TRUE;
    }
  } while (Status != EFI_NOT_FOUND);

  Status = GenMemoryTest->Finished (GenMemoryTest);

Done:
  UnicodeValueToString (StrTotalMemory, COMMA_TYPE, (UINTN) TotalMemorySize, 0);
  if (StrTotalMemory[0] == L',') {
    StrTotalMemory++;
  }

  TmpStr = GetStringById (STRING_TOKEN (STR_MEM_TEST_COMPLETED));
  if (TmpStr != NULL) {
    StrCat (StrTotalMemory, TmpStr);
    FreePool (TmpStr);
  }

  gST->ConOut->ClearScreen (gST->ConOut);
  gST->ConOut->SetAttribute (gST->ConOut, EFI_YELLOW | EFI_BRIGHT);
  gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  gST->ConOut->OutputString (gST->ConOut, StrTotalMemory);
  PlatformBdsShowProgress (
    Foreground,
    Background,
    StrTotalMemory,
    Color,
    100,
    (UINTN) PreviousValue
    );

  FreePool (Pos);

  DataSize = sizeof (Value);
  Status = gRT->GetVariable (
                  L"BootState",
                  &gEfiBootStateGuid,
                  NULL,
                  &DataSize,
                  &Value
                  );

  if (EFI_ERROR (Status)) {
    Value = 1;
    gRT->SetVariable (
          L"BootState",
          &gEfiBootStateGuid,
          EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
          sizeof (Value),
          &Value
          );
  }

  return ReturnStatus;
}
