/** @file
  Example program using BltLib

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/BltLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>


UINT64
ReadTimestamp (
  VOID
  )
{
#if defined (MDE_CPU_IA32) || defined (MDE_CPU_X64)
  return AsmReadTsc ();
#elif defined (MDE_CPU_IPF)
  return AsmReadItc ();
#else
#error ReadTimestamp not supported for this architecture!
#endif
}

UINT32
Rand32 (
  VOID
  )
{
  UINTN    Found;
  INTN     Bits;
  UINT64   Tsc1;
  UINT64   Tsc2;
  UINT64   TscBits;
  UINT32   R32;

  R32 = 0;
  Found = 0;
  Tsc1 = ReadTimestamp ();
  Tsc2 = ReadTimestamp ();
  do {
    Tsc2 = ReadTimestamp ();
    TscBits = Tsc2 ^ Tsc1;
    Bits = HighBitSet64 (TscBits);
    if (Bits > 0) {
      Bits = Bits - 1;
    }
    R32 = (UINT32)((R32 << Bits) |
                   RShiftU64 (LShiftU64 (TscBits, (UINTN) (64 - Bits)), (UINTN) (64 - Bits)));
    Found = Found + Bits;
  } while (Found < 32);

  return R32;
}


VOID
TestFills (
  VOID
  )
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Color;
  UINTN                          Loop;
  UINTN                          X;
  UINTN                          Y;
  UINTN                          W;
  UINTN                          H;
  UINTN                          Width;
  UINTN                          Height;

  BltLibGetSizes (&Width, &Height);
  for (Loop = 0; Loop < 10000; Loop++) {
    W = Width - (Rand32 () % Width);
    H = Height - (Rand32 () % Height);
    if (W != Width) {
      X = Rand32 () % (Width - W);
    } else {
      X = 0;
    }
    if (H != Height) {
      Y = Rand32 () % (Height - H);
    } else {
      Y = 0;
    }
    *(UINT32*) (&Color) = Rand32 () & 0xffffff;
    BltLibVideoFill (&Color, X, Y, W, H);
  }
}


VOID
TestColor1 (
  VOID
  )
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Color;
  UINTN                          X;
  UINTN                          Y;
  UINTN                          Width;
  UINTN                          Height;

  BltLibGetSizes (&Width, &Height);
  *(UINT32*) (&Color) = 0;

  for (Y = 0; Y < Height; Y++) {
    for (X = 0; X < Width; X++) {
      Color.Red =   (UINT8) ((X * 0x100) / Width);
      Color.Green = (UINT8) ((Y * 0x100) / Height);
      Color.Blue =  (UINT8) ((Y * 0x100) / Height);
      BltLibVideoFill (&Color, X, Y, 1, 1);
    }
  }
}


UINT32
Uint32SqRt (
  IN  UINT32  Uint32
  )
{
  UINT32 Mask;
  UINT32 SqRt;
  UINT32 SqRtMask;
  UINT32 Squared;

  if (Uint32 == 0) {
    return 0;
  }

  for (SqRt = 0, Mask = (UINT32) (1 << (HighBitSet32 (Uint32) / 2));
       Mask != 0;
       Mask = Mask >> 1
      ) {
    SqRtMask = SqRt | Mask;
    //DEBUG ((EFI_D_INFO, "Uint32=0x%x SqRtMask=0x%x\n", Uint32, SqRtMask));
    Squared = (UINT32) (SqRtMask * SqRtMask);
    if (Squared > Uint32) {
      continue;
    } else if (Squared < Uint32) {
      SqRt = SqRtMask;
    } else {
      return SqRtMask;
    }
  }

  return SqRt;
}


UINT32
Uint32Dist (
  IN UINTN X,
  IN UINTN Y
  )
{
  return Uint32SqRt ((UINT32) ((X * X) + (Y * Y)));
}

UINT8
GetTriColor (
  IN UINTN ColorDist,
  IN UINTN TriWidth
  )
{
  return (UINT8) (((TriWidth - ColorDist) * 0x100) / TriWidth);
  //return (((TriWidth * TriWidth - ColorDist * ColorDist) * 0x100) / (TriWidth * TriWidth));
}

VOID
TestColor (
  VOID
  )
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Color;
  UINTN                          X, Y;
  UINTN                          X1, X2, X3;
  UINTN                          Y1, Y2;
  UINTN                          LineWidth, TriWidth, ScreenWidth;
  UINTN                          TriHeight, ScreenHeight;
  UINT32                         ColorDist;

  BltLibGetSizes (&ScreenWidth, &ScreenHeight);
  *(UINT32*) (&Color) = 0;
  BltLibVideoFill (&Color, 0, 0, ScreenWidth, ScreenHeight);

  TriWidth = (UINTN) DivU64x32 (
                       MultU64x32 (11547005, (UINT32) ScreenHeight),
                       10000000
                       );
  TriHeight = (UINTN) DivU64x32 (
                        MultU64x32 (8660254, (UINT32) ScreenWidth),
                        10000000
                        );
  if (TriWidth > ScreenWidth) {
    DEBUG ((EFI_D_INFO, "TriWidth at %d was too big\n", TriWidth));
    TriWidth = ScreenWidth;
  } else if (TriHeight > ScreenHeight) {
    DEBUG ((EFI_D_INFO, "TriHeight at %d was too big\n", TriHeight));
    TriHeight = ScreenHeight;
  }

  DEBUG ((EFI_D_INFO, "Triangle Width: %d; Height: %d\n", TriWidth, TriHeight));

  X1 = (ScreenWidth - TriWidth) / 2;
  X3 = X1 + TriWidth - 1;
  X2 = (X1 + X3) / 2;
  Y2 = (ScreenHeight - TriHeight) / 2;
  Y1 = Y2 + TriHeight - 1;

  for (Y = Y2; Y <= Y1; Y++) {
    LineWidth =
      (UINTN) DivU64x32 (
                MultU64x32 (11547005, (UINT32) (Y - Y2)),
                20000000
                );
    for (X = X2 - LineWidth; X < (X2 + LineWidth); X++) {
      ColorDist = Uint32Dist(X - X1, Y1 - Y);
      Color.Red = GetTriColor (ColorDist, TriWidth);

      ColorDist = Uint32Dist((X < X2) ? X2 - X : X - X2, Y - Y2);
      Color.Green = GetTriColor (ColorDist, TriWidth);

      ColorDist = Uint32Dist(X3 - X, Y1 - Y);
      Color.Blue = GetTriColor (ColorDist, TriWidth);

      BltLibVideoFill (&Color, X, Y, 1, 1);
    }
  }
}


/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                     Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL   *Gop;

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **) &Gop
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = BltLibConfigure (
             (VOID*)(UINTN) Gop->Mode->FrameBufferBase,
             Gop->Mode->Info
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TestFills ();

  TestColor ();

  return EFI_SUCCESS;
}
