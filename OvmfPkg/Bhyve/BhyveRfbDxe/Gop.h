/*++ @file

Copyright (c) 2020, Rebecca Cran <rebecca@bsdio.com>
Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2010,Apple Inc. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _GOP_H_
#define _GOP_H_

#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/PciIo.h>
#include <Protocol/DriverSupportedEfiVersion.h>
#include <Protocol/DevicePath.h>

#include <Guid/EventGroup.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>

#include <IndustryStandard/Pci.h>

#define GRAPHICS_OUTPUT_INVALID_MODE_NUMBER  0xffff

typedef struct {
  UINT32    HorizontalResolution;
  UINT32    VerticalResolution;
  UINT32    ColorDepth;
  UINT32    RefreshRate;
} GOP_MODE_DATA;

#define PIXEL_RED_SHIFT    0
#define PIXEL_GREEN_SHIFT  3
#define PIXEL_BLUE_SHIFT   6

#define PIXEL_RED_MASK    (BIT7 | BIT6 | BIT5)
#define PIXEL_GREEN_MASK  (BIT4 | BIT3 | BIT2)
#define PIXEL_BLUE_MASK   (BIT1 | BIT0)

#define PIXEL_TO_COLOR_BYTE(pixel, mask, shift)  ((UINT8) ((pixel & mask) << shift))
#define PIXEL_TO_RED_BYTE(pixel)                 PIXEL_TO_COLOR_BYTE(pixel, PIXEL_RED_MASK, PIXEL_RED_SHIFT)
#define PIXEL_TO_GREEN_BYTE(pixel)               PIXEL_TO_COLOR_BYTE(pixel, PIXEL_GREEN_MASK, PIXEL_GREEN_SHIFT)
#define PIXEL_TO_BLUE_BYTE(pixel)                PIXEL_TO_COLOR_BYTE(pixel, PIXEL_BLUE_MASK, PIXEL_BLUE_SHIFT)

#define RGB_BYTES_TO_PIXEL(Red, Green, Blue) \
  (UINT8) ( (((Red) >> PIXEL_RED_SHIFT) & PIXEL_RED_MASK) | \
            (((Green) >> PIXEL_GREEN_SHIFT) & PIXEL_GREEN_MASK) | \
            (((Blue) >> PIXEL_BLUE_SHIFT) & PIXEL_BLUE_MASK) )

#define PIXEL24_RED_MASK    0x00ff0000
#define PIXEL24_GREEN_MASK  0x0000ff00
#define PIXEL24_BLUE_MASK   0x000000ff

extern EFI_DRIVER_BINDING_PROTOCOL  gEmuGopDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gEmuGopComponentName;

#define EMU_UGA_CLASS_NAME  L"EmuGopWindow"

#define GOP_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('G', 'o', 'p', 'N')
typedef struct {
  UINT64                          Signature;

  EFI_HANDLE                      Handle;
  EFI_PCI_IO_PROTOCOL             *PciIo;
  EFI_GRAPHICS_OUTPUT_PROTOCOL    GraphicsOutput;
  EFI_DEVICE_PATH_PROTOCOL        *GopDevicePath;

  EFI_UNICODE_STRING_TABLE        *ControllerNameTable;

  //
  // GOP Private Data for QueryMode ()
  //
  GOP_MODE_DATA                   *ModeData;

  UINT64                          FbAddr;
  UINT32                          FbSize;
} GOP_PRIVATE_DATA;

#define GOP_PRIVATE_DATA_FROM_THIS(a)  \
         CR(a, GOP_PRIVATE_DATA, GraphicsOutput, GOP_PRIVATE_DATA_SIGNATURE)

typedef struct {
  UINT32    FbSize;
  UINT16    Width;
  UINT16    Height;
  UINT16    Depth;
  UINT16    RefreshRate;
} BHYVE_FBUF_MEMREGS;

//
// Global Protocol Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gEmuGopDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gEmuGopComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gEmuGopComponentName2;

//
// Gop Hardware abstraction internal worker functions
//
EFI_STATUS
EmuGopConstructor (
  IN  GOP_PRIVATE_DATA  *Private
  );

EFI_STATUS
EmuGopDestructor (
  IN  GOP_PRIVATE_DATA  *Private
  );

VOID
EFIAPI
ShutdownGopEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

VOID
BhyveSetGraphicsMode (
  GOP_PRIVATE_DATA  *Private,
  UINT16            Width,
  UINT16            Height,
  UINT16            Depth
  );

VOID
BhyveGetMemregs (
  GOP_PRIVATE_DATA    *Private,
  BHYVE_FBUF_MEMREGS  *Memregs
  );

VOID
InstallVbeShim (
  IN CONST CHAR16          *CardName,
  IN EFI_PHYSICAL_ADDRESS  FrameBufferBase
  );

#endif /* _GOP_H_ */
