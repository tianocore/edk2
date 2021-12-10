/** @file

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _BIOS_GRAPHICS_OUTPUT_H_
#define _BIOS_GRAPHICS_OUTPUT_H_

#include <FrameworkDxe.h>

#include <Protocol/PciIo.h>
#include <Protocol/EdidActive.h>
#include <Protocol/DevicePath.h>
#include <Protocol/EdidDiscovered.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/VgaMiniPort.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/EdidOverride.h>

#include <Guid/StatusCodeDataTypeId.h>
#include <Guid/LegacyBios.h>
#include <Guid/EventGroup.h>

#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>

#include <IndustryStandard/Pci.h>
#include "VesaBiosExtensions.h"

//
// Packed format support: The number of bits reserved for each of the colors and the actual
// position of RGB in the frame buffer is specified in the VBE Mode information
//
typedef struct {
  UINT8    Position; // Position of the color
  UINT8    Mask;     // The number of bits expressed as a mask
} BIOS_VIDEO_COLOR_PLACEMENT;

//
// BIOS Graphics Output Graphical Mode Data
//
typedef struct {
  UINT16                        VbeModeNumber;
  UINT16                        BytesPerScanLine;
  VOID                          *LinearFrameBuffer;
  UINTN                         FrameBufferSize;
  UINT32                        HorizontalResolution;
  UINT32                        VerticalResolution;
  UINT32                        ColorDepth;
  UINT32                        RefreshRate;
  UINT32                        BitsPerPixel;
  BIOS_VIDEO_COLOR_PLACEMENT    Red;
  BIOS_VIDEO_COLOR_PLACEMENT    Green;
  BIOS_VIDEO_COLOR_PLACEMENT    Blue;
  BIOS_VIDEO_COLOR_PLACEMENT    Reserved;
  EFI_GRAPHICS_PIXEL_FORMAT     PixelFormat;
  EFI_PIXEL_BITMASK             PixelBitMask;
} BIOS_VIDEO_MODE_DATA;

//
// BIOS video child handle private data Structure
//
#define BIOS_VIDEO_DEV_SIGNATURE  SIGNATURE_32 ('B', 'V', 'M', 'p')

typedef struct {
  UINTN                                          Signature;
  EFI_HANDLE                                     Handle;

  //
  // Consumed Protocols
  //
  EFI_PCI_IO_PROTOCOL                            *PciIo;
  EFI_LEGACY_BIOS_PROTOCOL                       *LegacyBios;

  //
  // Produced Protocols
  //
  EFI_GRAPHICS_OUTPUT_PROTOCOL                   GraphicsOutput;
  EFI_EDID_DISCOVERED_PROTOCOL                   EdidDiscovered;
  EFI_EDID_ACTIVE_PROTOCOL                       EdidActive;
  EFI_VGA_MINI_PORT_PROTOCOL                     VgaMiniPort;

  //
  // General fields
  //
  BOOLEAN                                        VgaCompatible;
  BOOLEAN                                        ProduceGraphicsOutput;

  //
  // Graphics Output Protocol related fields
  //
  BOOLEAN                                        HardwareNeedsStarting;
  UINTN                                          CurrentMode;
  UINTN                                          MaxMode;
  BIOS_VIDEO_MODE_DATA                           *ModeData;
  UINT8                                          *LineBuffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL                  *VbeFrameBuffer;
  UINT8                                          *VgaFrameBuffer;

  //
  // VESA Bios Extensions related fields
  //
  UINTN                                          NumberOfPagesBelow1MB;    // Number of 4KB pages in PagesBelow1MB
  EFI_PHYSICAL_ADDRESS                           PagesBelow1MB;            // Buffer for all VBE Information Blocks
  VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK         *VbeInformationBlock;     // 0x200 bytes.  Must be allocated below 1MB
  VESA_BIOS_EXTENSIONS_MODE_INFORMATION_BLOCK    *VbeModeInformationBlock; // 0x100 bytes.  Must be allocated below 1MB
  VESA_BIOS_EXTENSIONS_EDID_DATA_BLOCK           *VbeEdidDataBlock;        // 0x80  bytes.  Must be allocated below 1MB
  VESA_BIOS_EXTENSIONS_CRTC_INFORMATION_BLOCK    *VbeCrtcInformationBlock; // 59 bytes.  Must be allocated below 1MB
  UINTN                                          VbeSaveRestorePages;      // Number of 4KB pages in VbeSaveRestoreBuffer
  EFI_PHYSICAL_ADDRESS                           VbeSaveRestoreBuffer;     // Must be allocated below 1MB
  //
  // Status code
  //
  EFI_DEVICE_PATH_PROTOCOL                       *GopDevicePath;

  EFI_EVENT                                      ExitBootServicesEvent;
} BIOS_VIDEO_DEV;

#define BIOS_VIDEO_DEV_FROM_PCI_IO_THIS(a)           CR (a, BIOS_VIDEO_DEV, PciIo, BIOS_VIDEO_DEV_SIGNATURE)
#define BIOS_VIDEO_DEV_FROM_GRAPHICS_OUTPUT_THIS(a)  CR (a, BIOS_VIDEO_DEV, GraphicsOutput, BIOS_VIDEO_DEV_SIGNATURE)
#define BIOS_VIDEO_DEV_FROM_VGA_MINI_PORT_THIS(a)    CR (a, BIOS_VIDEO_DEV, VgaMiniPort, BIOS_VIDEO_DEV_SIGNATURE)

#define GRAPHICS_OUTPUT_INVALIDE_MODE_NUMBER  0xffff

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gBiosVideoDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gBiosVideoComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gBiosVideoComponentName2;

//
// Driver Binding Protocol functions
//

/**
  Supported.

  @param  This                   Pointer to driver binding protocol
  @param  Controller             Controller handle to connect
  @param  RemainingDevicePath    A pointer to the remaining portion of a device
                                 path

  @retval EFI_STATUS             EFI_SUCCESS:This controller can be managed by this
                                 driver, Otherwise, this controller cannot be
                                 managed by this driver

**/
EFI_STATUS
EFIAPI
BiosVideoDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Install Graphics Output Protocol onto VGA device handles.

  @param  This                   Pointer to driver binding protocol
  @param  Controller             Controller handle to connect
  @param  RemainingDevicePath    A pointer to the remaining portion of a device
                                 path

  @return EFI_STATUS

**/
EFI_STATUS
EFIAPI
BiosVideoDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Stop.

  @param  This                   Pointer to driver binding protocol
  @param  Controller             Controller handle to connect
  @param  NumberOfChildren       Number of children handle created by this driver
  @param  ChildHandleBuffer      Buffer containing child handle created

  @retval EFI_SUCCESS            Driver disconnected successfully from controller
  @retval EFI_UNSUPPORTED        Cannot find BIOS_VIDEO_DEV structure

**/
EFI_STATUS
EFIAPI
BiosVideoDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

//
// Private worker functions
//

/**
  Check for VBE device.

  @param  BiosVideoPrivate       Pointer to BIOS_VIDEO_DEV structure

  @retval EFI_SUCCESS            VBE device found

**/
EFI_STATUS
BiosVideoCheckForVbe (
  IN OUT BIOS_VIDEO_DEV  *BiosVideoPrivate
  );

/**
  Check for VGA device.

  @param  BiosVideoPrivate       Pointer to BIOS_VIDEO_DEV structure

  @retval EFI_SUCCESS            Standard VGA device found

**/
EFI_STATUS
BiosVideoCheckForVga (
  IN OUT BIOS_VIDEO_DEV  *BiosVideoPrivate
  );

/**
  Release resource for BIOS video instance.

  @param  BiosVideoPrivate       Video child device private data structure

**/
VOID
BiosVideoDeviceReleaseResource (
  BIOS_VIDEO_DEV  *BiosVideoPrivate
  );

//
// BIOS Graphics Output Protocol functions
//

/**
  Graphics Output protocol interface to get video mode.

  @param  This                   Protocol instance pointer.
  @param  ModeNumber             The mode number to return information on.
  @param  SizeOfInfo             A pointer to the size, in bytes, of the Info
                                 buffer.
  @param  Info                   Caller allocated buffer that returns information
                                 about ModeNumber.

  @retval EFI_SUCCESS            Mode information returned.
  @retval EFI_BUFFER_TOO_SMALL   The Info buffer was too small.
  @retval EFI_DEVICE_ERROR       A hardware error occurred trying to retrieve the
                                 video mode.
  @retval EFI_NOT_STARTED        Video display is not initialized. Call SetMode ()
  @retval EFI_INVALID_PARAMETER  One of the input args was NULL.

**/
EFI_STATUS
EFIAPI
BiosVideoGraphicsOutputQueryMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  );

/**
  Graphics Output protocol interface to set video mode.

  @param  This                   Protocol instance pointer.
  @param  ModeNumber             The mode number to be set.

  @retval EFI_SUCCESS            Graphics mode was changed.
  @retval EFI_DEVICE_ERROR       The device had an error and could not complete the
                                 request.
  @retval EFI_UNSUPPORTED        ModeNumber is not supported by this device.

**/
EFI_STATUS
EFIAPI
BiosVideoGraphicsOutputSetMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
  IN  UINT32                        ModeNumber
  );

/**
  Graphics Output protocol instance to block transfer for VBE device.

  @param  This                   Pointer to Graphics Output protocol instance
  @param  BltBuffer              The data to transfer to screen
  @param  BltOperation           The operation to perform
  @param  SourceX                The X coordinate of the source for BltOperation
  @param  SourceY                The Y coordinate of the source for BltOperation
  @param  DestinationX           The X coordinate of the destination for
                                 BltOperation
  @param  DestinationY           The Y coordinate of the destination for
                                 BltOperation
  @param  Width                  The width of a rectangle in the blt rectangle in
                                 pixels
  @param  Height                 The height of a rectangle in the blt rectangle in
                                 pixels
  @param  Delta                  Not used for EfiBltVideoFill and
                                 EfiBltVideoToVideo operation. If a Delta of 0 is
                                 used, the entire BltBuffer will be operated on. If
                                 a subrectangle of the BltBuffer is used, then
                                 Delta represents the number of bytes in a row of
                                 the BltBuffer.

  @retval EFI_INVALID_PARAMETER  Invalid parameter passed in
  @retval EFI_SUCCESS            Blt operation success

**/
EFI_STATUS
EFIAPI
BiosVideoGraphicsOutputVbeBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer  OPTIONAL,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta
  );

/**
  Graphics Output protocol instance to block transfer for VGA device.

  @param  This                   Pointer to Graphics Output protocol instance
  @param  BltBuffer              The data to transfer to screen
  @param  BltOperation           The operation to perform
  @param  SourceX                The X coordinate of the source for BltOperation
  @param  SourceY                The Y coordinate of the source for BltOperation
  @param  DestinationX           The X coordinate of the destination for
                                 BltOperation
  @param  DestinationY           The Y coordinate of the destination for
                                 BltOperation
  @param  Width                  The width of a rectangle in the blt rectangle in
                                 pixels
  @param  Height                 The height of a rectangle in the blt rectangle in
                                 pixels
  @param  Delta                  Not used for EfiBltVideoFill and
                                 EfiBltVideoToVideo operation. If a Delta of 0 is
                                 used, the entire BltBuffer will be operated on. If
                                 a subrectangle of the BltBuffer is used, then
                                 Delta represents the number of bytes in a row of
                                 the BltBuffer.

  @retval EFI_INVALID_PARAMETER  Invalid parameter passed in
  @retval EFI_SUCCESS            Blt operation success

**/
EFI_STATUS
EFIAPI
BiosVideoGraphicsOutputVgaBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer  OPTIONAL,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta
  );

//
// BIOS VGA Mini Port Protocol functions
//

/**
  VgaMiniPort protocol interface to set mode.

  @param  This                   Pointer to VgaMiniPort protocol instance
  @param  ModeNumber             The index of the mode

  @retval EFI_UNSUPPORTED        The requested mode is not supported
  @retval EFI_SUCCESS            The requested mode is set successfully

**/
EFI_STATUS
EFIAPI
BiosVideoVgaMiniPortSetMode (
  IN  EFI_VGA_MINI_PORT_PROTOCOL  *This,
  IN  UINTN                       ModeNumber
  );

/**
  Event handler for Exit Boot Service.

  @param  Event       The event that be signalled when exiting boot service.
  @param  Context     Pointer to instance of BIOS_VIDEO_DEV.

**/
VOID
EFIAPI
BiosVideoNotifyExitBootServices (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

//
// Standard VGA Definitions
//
#define VGA_HORIZONTAL_RESOLUTION  640
#define VGA_VERTICAL_RESOLUTION    480
#define VGA_NUMBER_OF_BIT_PLANES   4
#define VGA_PIXELS_PER_BYTE        8
#define VGA_BYTES_PER_SCAN_LINE    (VGA_HORIZONTAL_RESOLUTION / VGA_PIXELS_PER_BYTE)
#define VGA_BYTES_PER_BIT_PLANE    (VGA_VERTICAL_RESOLUTION * VGA_BYTES_PER_SCAN_LINE)

#define VGA_GRAPHICS_CONTROLLER_ADDRESS_REGISTER  0x3ce
#define VGA_GRAPHICS_CONTROLLER_DATA_REGISTER     0x3cf

#define VGA_GRAPHICS_CONTROLLER_SET_RESET_REGISTER  0x00

#define VGA_GRAPHICS_CONTROLLER_ENABLE_SET_RESET_REGISTER  0x01

#define VGA_GRAPHICS_CONTROLLER_COLOR_COMPARE_REGISTER  0x02

#define VGA_GRAPHICS_CONTROLLER_DATA_ROTATE_REGISTER  0x03
#define VGA_GRAPHICS_CONTROLLER_FUNCTION_REPLACE      0x00
#define VGA_GRAPHICS_CONTROLLER_FUNCTION_AND          0x08
#define VGA_GRAPHICS_CONTROLLER_FUNCTION_OR           0x10
#define VGA_GRAPHICS_CONTROLLER_FUNCTION_XOR          0x18

#define VGA_GRAPHICS_CONTROLLER_READ_MAP_SELECT_REGISTER  0x04

#define VGA_GRAPHICS_CONTROLLER_MODE_REGISTER  0x05
#define VGA_GRAPHICS_CONTROLLER_READ_MODE_0    0x00
#define VGA_GRAPHICS_CONTROLLER_READ_MODE_1    0x08
#define VGA_GRAPHICS_CONTROLLER_WRITE_MODE_0   0x00
#define VGA_GRAPHICS_CONTROLLER_WRITE_MODE_1   0x01
#define VGA_GRAPHICS_CONTROLLER_WRITE_MODE_2   0x02
#define VGA_GRAPHICS_CONTROLLER_WRITE_MODE_3   0x03

#define VGA_GRAPHICS_CONTROLLER_MISCELLANEOUS_REGISTER  0x06

#define VGA_GRAPHICS_CONTROLLER_COLOR_DONT_CARE_REGISTER  0x07

#define VGA_GRAPHICS_CONTROLLER_BIT_MASK_REGISTER  0x08

/**
  Install child handles if the Handle supports MBR format.

  @param  This                   Calling context.
  @param  ParentHandle           Parent Handle
  @param  ParentPciIo            Parent PciIo interface
  @param  ParentLegacyBios       Parent LegacyBios interface
  @param  ParentDevicePath       Parent Device Path
  @param  RemainingDevicePath    Remaining Device Path

  @retval EFI_SUCCESS            If a child handle was added
  @retval other                  A child handle was not added

**/
EFI_STATUS
BiosVideoChildHandleInstall (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ParentHandle,
  IN  EFI_PCI_IO_PROTOCOL          *ParentPciIo,
  IN  EFI_LEGACY_BIOS_PROTOCOL     *ParentLegacyBios,
  IN  EFI_DEVICE_PATH_PROTOCOL     *ParentDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Deregister an video child handle and free resources.

  @param  This                   Protocol instance pointer.
  @param  Controller             Video controller handle
  @param  Handle                 Video child handle

  @return EFI_STATUS

**/
EFI_STATUS
BiosVideoChildHandleUninstall (
  EFI_DRIVER_BINDING_PROTOCOL  *This,
  EFI_HANDLE                   Controller,
  EFI_HANDLE                   Handle
  );

/**
  Release resource for BIOS video instance.

  @param  BiosVideoPrivate       Video child device private data structure

**/
VOID
BiosVideoDeviceReleaseResource (
  BIOS_VIDEO_DEV  *BiosVideoPrivate
  );

/**
  Check if all video child handles have been uninstalled.

  @param  Controller             Video controller handle

  @return TRUE                   Child handles exist.
  @return FALSE                  All video child handles have been uninstalled.

**/
BOOLEAN
HasChildHandle (
  IN EFI_HANDLE  Controller
  );

#endif
