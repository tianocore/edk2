/** @file

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FB_GOP_H_
#define _FB_GOP_H_

#include <Protocol/PciIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/EdidActive.h>
#include <Protocol/EdidDiscovered.h>

#include <Guid/StatusCodeDataTypeId.h>
#include <Guid/EventGroup.h>
#include <Guid/FrameBufferInfoGuid.h>

#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>

#include <IndustryStandard/Pci.h>

//
// Packed format support: The number of bits reserved for each of the colors and the actual
// position of RGB in the frame buffer is specified in the VBE Mode information
//
typedef struct {
  UINT8 Position; // Position of the color
  UINT8 Mask;     // The number of bits expressed as a mask
} FB_VIDEO_COLOR_PLACEMENT;

//
// BIOS Graphics Output Graphical Mode Data
//
typedef struct {
  UINT16                      VbeModeNumber;
  UINT16                      BytesPerScanLine;
  VOID                        *LinearFrameBuffer;
  UINTN                       FrameBufferSize;
  UINT32                      HorizontalResolution;
  UINT32                      VerticalResolution;
  UINT32                      ColorDepth;
  UINT32                      RefreshRate;
  UINT32                      BitsPerPixel;
  FB_VIDEO_COLOR_PLACEMENT    Red;
  FB_VIDEO_COLOR_PLACEMENT    Green;
  FB_VIDEO_COLOR_PLACEMENT    Blue;
  FB_VIDEO_COLOR_PLACEMENT    Reserved;
  EFI_GRAPHICS_PIXEL_FORMAT   PixelFormat;
  EFI_PIXEL_BITMASK           PixelBitMask;
} FB_VIDEO_MODE_DATA;

//
// BIOS video child handle private data Structure
//
#define FB_VIDEO_DEV_SIGNATURE    SIGNATURE_32 ('B', 'V', 'M', 'p')

typedef struct {
  UINTN                                       Signature;
  EFI_HANDLE                                  Handle;

  //
  // Consumed Protocols
  //
  EFI_PCI_IO_PROTOCOL                         *PciIo;  

  //
  // Produced Protocols
  //
  EFI_GRAPHICS_OUTPUT_PROTOCOL                GraphicsOutput;
  EFI_EDID_DISCOVERED_PROTOCOL                EdidDiscovered;
  EFI_EDID_ACTIVE_PROTOCOL                    EdidActive;

  //
  // Graphics Output Protocol related fields
  //
  UINTN                                       CurrentMode;
  UINTN                                       MaxMode;
  FB_VIDEO_MODE_DATA                          *ModeData;
  
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL               *VbeFrameBuffer;
  
  //
  // Status code
  //
  EFI_DEVICE_PATH_PROTOCOL                    *GopDevicePath;
  
} FB_VIDEO_DEV;

#define FB_VIDEO_DEV_FROM_PCI_IO_THIS(a)           CR (a, FB_VIDEO_DEV, PciIo, FB_VIDEO_DEV_SIGNATURE)
#define FB_VIDEO_DEV_FROM_GRAPHICS_OUTPUT_THIS(a)  CR (a, FB_VIDEO_DEV, GraphicsOutput, FB_VIDEO_DEV_SIGNATURE)

#define GRAPHICS_OUTPUT_INVALIDE_MODE_NUMBER  0xffff

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gFbGopDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gFbGopComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gFbGopComponentName2;

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
FbGopDriverBindingSupported (
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
FbGopDriverBindingStart (
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
  @retval EFI_UNSUPPORTED        Cannot find FB_VIDEO_DEV structure

**/
EFI_STATUS
EFIAPI
FbGopDriverBindingStop (
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

  @param  FbGopPrivate       Pointer to FB_VIDEO_DEV structure

  @retval EFI_SUCCESS            VBE device found

**/
EFI_STATUS
FbGopCheckForVbe (
  IN OUT FB_VIDEO_DEV  *FbGopPrivate
  );



/**
  Release resource for biso video instance.

  @param  FbGopPrivate       Video child device private data structure

**/
VOID
FbGopDeviceReleaseResource (
  FB_VIDEO_DEV  *FbGopPrivate
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
FbGopGraphicsOutputQueryMode (
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
FbGopGraphicsOutputSetMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL * This,
  IN  UINT32                       ModeNumber
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
FbGopGraphicsOutputVbeBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer, OPTIONAL
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
  Grahpics Output protocol instance to block transfer for VGA device.

  @param  This                   Pointer to Grahpics Output protocol instance
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
FbGopGraphicsOutputVgaBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer, OPTIONAL
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
FbGopChildHandleInstall (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ParentHandle,
  IN  EFI_PCI_IO_PROTOCOL          *ParentPciIo,
  IN  VOID                         *ParentLegacyBios,
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
FbGopChildHandleUninstall (
  EFI_DRIVER_BINDING_PROTOCOL    *This,
  EFI_HANDLE                     Controller,
  EFI_HANDLE                     Handle
  );

/**
  Release resource for biso video instance.

  @param  FbGopPrivate       Video child device private data structure

**/
VOID
FbGopDeviceReleaseResource (
  FB_VIDEO_DEV  *FbGopPrivate
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
