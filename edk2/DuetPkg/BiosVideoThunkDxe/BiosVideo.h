/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BiosVideo.h
    
Abstract: 

Revision History
--*/

#ifndef _BIOS_UGA_H
#define _BIOS_UGA_H

#include <Uefi.h>

//
// Driver Consumed Protocol Prototypes
//
#include <Protocol/DevicePath.h>
#include <Protocol/PciIo.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/VgaMiniPort.h>
#include <Protocol/Legacy8259.h>
#include <Protocol/LegacyBios.h>

#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>

#include <IndustryStandard/Pci22.h>

#include "VesaBiosExtensions.h"
//
// Driver Produced Protocol Prototypes
//
//#include EFI_PROTOCOL_DEFINITION (DriverBinding)
//#include EFI_PROTOCOL_DEFINITION (ComponentName)
//#include EFI_PROTOCOL_DEFINITION (ComponentName2)
//#include EFI_PROTOCOL_DEFINITION (UgaDraw)
//#include EFI_PROTOCOL_DEFINITION (VgaMiniPort)

//
// Packed format support: The number of bits reserved for each of the colors and the actual
// position of RGB in the frame buffer is specified in the VBE Mode information
//
typedef struct {
  UINT8 Position; // Position of the color
  UINT8 Mask;     // The number of bits expressed as a mask
} BIOS_VIDEO_COLOR_PLACEMENT;

//
// BIOS UGA Draw Graphical Mode Data
//
typedef struct {
  UINT16                      VbeModeNumber;
  UINT16                      BytesPerScanLine;
  VOID                        *LinearFrameBuffer;
  UINT32                      HorizontalResolution;
  UINT32                      VerticalResolution;
  UINT32                      ColorDepth;
  UINT32                      RefreshRate;
  UINT32                      BitsPerPixel;
  BIOS_VIDEO_COLOR_PLACEMENT  Red;
  BIOS_VIDEO_COLOR_PLACEMENT  Green;
  BIOS_VIDEO_COLOR_PLACEMENT  Blue;
} BIOS_VIDEO_MODE_DATA;

//
// BIOS UGA Device Structure
//
#define BIOS_VIDEO_DEV_SIGNATURE  SIGNATURE_32 ('B', 'V', 'M', 'p')

typedef struct {
  UINTN                                       Signature;
  EFI_HANDLE                                  Handle;

  //
  // Consumed Protocols
  //
  EFI_PCI_IO_PROTOCOL                         *PciIo;
  //EFI_LEGACY_BIOS_THUNK_PROTOCOL              *LegacyBios;

  //
  // Produced Protocols
  //
  EFI_UGA_DRAW_PROTOCOL                       UgaDraw;
  EFI_VGA_MINI_PORT_PROTOCOL                  VgaMiniPort;

  //
  // General fields
  //
  EFI_EVENT                                   ExitBootServicesEvent;
  BOOLEAN                                     VgaCompatible;
  BOOLEAN                                     ProduceUgaDraw;

  //
  // UGA Draw related fields
  //
  BOOLEAN                                     HardwareNeedsStarting;
  UINTN                                       CurrentMode;
  UINTN                                       MaxMode;
  BIOS_VIDEO_MODE_DATA                        *ModeData;
  UINT8                                       *LineBuffer;
  EFI_UGA_PIXEL                               *VbeFrameBuffer;
  UINT8                                       *VgaFrameBuffer;

  //
  // VESA Bios Extensions related fields
  //
  UINTN                                       NumberOfPagesBelow1MB;    // Number of 4KB pages in PagesBelow1MB
  EFI_PHYSICAL_ADDRESS                        PagesBelow1MB;            // Buffer for all VBE Information Blocks
  VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK      *VbeInformationBlock;     // 0x200 bytes.  Must be allocated below 1MB
  VESA_BIOS_EXTENSIONS_MODE_INFORMATION_BLOCK *VbeModeInformationBlock; // 0x100 bytes.  Must be allocated below 1MB
  VESA_BIOS_EXTENSIONS_CRTC_INFORMATION_BLOCK *VbeCrtcInformationBlock; // 59 bytes.  Must be allocated below 1MB
  UINTN                                       VbeSaveRestorePages;      // Number of 4KB pages in VbeSaveRestoreBuffer
  EFI_PHYSICAL_ADDRESS                        VbeSaveRestoreBuffer;     // Must be allocated below 1MB
  //
  // Status code
  //
  EFI_DEVICE_PATH_PROTOCOL                    *DevicePath;
} BIOS_VIDEO_DEV;

#define BIOS_VIDEO_DEV_FROM_UGA_DRAW_THIS(a)      CR (a, BIOS_VIDEO_DEV, UgaDraw, BIOS_VIDEO_DEV_SIGNATURE)

#define BIOS_VIDEO_DEV_FROM_VGA_MINI_PORT_THIS(a) CR (a, BIOS_VIDEO_DEV, VgaMiniPort, BIOS_VIDEO_DEV_SIGNATURE)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gBiosVideoDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gBiosVideoComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gBiosVideoComponentName2;

//
// Driver Binding Protocol functions
//
EFI_STATUS
EFIAPI
BiosVideoDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++
  
  Routine Description:

    Supported.
    
  Arguments:

  This - Pointer to driver binding protocol
  Controller - Controller handle to connect
  RemainingDevicePath - A pointer to the remaining portion of a device path
    
    
  Returns:

  EFI_STATUS - EFI_SUCCESS:This controller can be managed by this driver,
               Otherwise, this controller cannot be managed by this driver
  
--*/
;

EFI_STATUS
EFIAPI
BiosVideoDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++
  
  Routine Description:

    Install UGA Draw Protocol onto VGA device handles
  
  Arguments:

  This - Pointer to driver binding protocol
  Controller - Controller handle to connect
  RemainingDevicePath - A pointer to the remaining portion of a device path
    
  Returns:

    EFI_STATUS
    
--*/
;

EFI_STATUS
EFIAPI
BiosVideoDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
/*++
  
  Routine Description:

    Stop.
  
  Arguments:

  This - Pointer to driver binding protocol
  Controller - Controller handle to connect
  NumberOfChilren - Number of children handle created by this driver
  ChildHandleBuffer - Buffer containing child handle created
  
  Returns:

  EFI_SUCCESS - Driver disconnected successfully from controller
  EFI_UNSUPPORTED - Cannot find BIOS_VIDEO_DEV structure
  
--*/
;

//
// Private worker functions
//
EFI_STATUS
BiosVideoCheckForVbe (
  BIOS_VIDEO_DEV  *BiosVideoPrivate
  )
/*++
  
  Routine Description:

  Check for VBE device   
  
  Arguments:
  
  BiosVideoPrivate - Pointer to BIOS_VIDEO_DEV structure
  
  Returns:
  
  EFI_SUCCESS - VBE device found
  
--*/
;

EFI_STATUS
BiosVideoCheckForVga (
  BIOS_VIDEO_DEV  *BiosVideoPrivate
  )
/*++
  
  Routine Description:

    Check for VGA device
  
  Arguments:
  
    BiosVideoPrivate - Pointer to BIOS_VIDEO_DEV structure
  
  Returns:
  
    EFI_SUCCESS - Standard VGA device found
  
--*/
;

//
// BIOS UGA Draw Protocol functions
//
EFI_STATUS
EFIAPI
BiosVideoUgaDrawGetMode (
  IN  EFI_UGA_DRAW_PROTOCOL  *This,
  OUT UINT32                 *HorizontalResolution,
  OUT UINT32                 *VerticalResolution,
  OUT UINT32                 *ColorDepth,
  OUT UINT32                 *RefreshRate
  )
/*++

Routine Description:

  UGA protocol interface to get video mode

Arguments:

  This                  - Pointer to UGA draw protocol instance
  HorizontalResolution  - Horizontal Resolution, in pixels
  VerticalResolution    - Vertical Resolution, in pixels
  ColorDepth            - Bit number used to represent color value of a pixel 
  RefreshRate           - Refresh rate, in Hertz

Returns:

  EFI_DEVICE_ERROR - Hardware need starting
  EFI_INVALID_PARAMETER - Invalid parameter passed in
  EFI_SUCCESS - Video mode query successfully

--*/
;

EFI_STATUS
EFIAPI
BiosVideoUgaDrawSetMode (
  IN  EFI_UGA_DRAW_PROTOCOL  *This,
  IN  UINT32                 HorizontalResolution,
  IN  UINT32                 VerticalResolution,
  IN  UINT32                 ColorDepth,
  IN  UINT32                 RefreshRate
  )
/*++

Routine Description:

  UGA draw protocol interface to set video mode

Arguments:

  This                  - Pointer to UGA draw protocol instance
  HorizontalResolution  - Horizontal Resolution, in pixels
  VerticalResolution    - Vertical Resolution, in pixels
  ColorDepth            - Bit number used to represent color value of a pixel 
  RefreshRate           - Refresh rate, in Hertz

Returns:

  EFI_DEVICE_ERROR - Device error
  EFI_SUCCESS - Video mode set successfully
  EFI_UNSUPPORTED - Cannot support this video mode

--*/
;

EFI_STATUS
EFIAPI
BiosVideoUgaDrawVbeBlt (
  IN  EFI_UGA_DRAW_PROTOCOL  *This,
  IN  EFI_UGA_PIXEL          *BltBuffer, OPTIONAL
  IN  EFI_UGA_BLT_OPERATION  BltOperation,
  IN  UINTN                  SourceX,
  IN  UINTN                  SourceY,
  IN  UINTN                  DestinationX,
  IN  UINTN                  DestinationY,
  IN  UINTN                  Width,
  IN  UINTN                  Height,
  IN  UINTN                  Delta
  )
/*++

Routine Description:

  UGA draw protocol instance to block transfer for VBE device

Arguments:

  This          - Pointer to UGA draw protocol instance
  BltBuffer     - The data to transfer to screen
  BltOperation  - The operation to perform
  SourceX       - The X coordinate of the source for BltOperation
  SourceY       - The Y coordinate of the source for BltOperation
  DestinationX  - The X coordinate of the destination for BltOperation
  DestinationY  - The Y coordinate of the destination for BltOperation
  Width         - The width of a rectangle in the blt rectangle in pixels
  Height        - The height of a rectangle in the blt rectangle in pixels
  Delta         - Not used for EfiUgaVideoFill and EfiUgaVideoToVideo operation.
                  If a Delta of 0 is used, the entire BltBuffer will be operated on.
                  If a subrectangle of the BltBuffer is used, then Delta represents 
                  the number of bytes in a row of the BltBuffer.

Returns:

  EFI_INVALID_PARAMETER - Invalid parameter passed in
  EFI_SUCCESS - Blt operation success

--*/
;

EFI_STATUS
EFIAPI
BiosVideoUgaDrawVgaBlt (
  IN  EFI_UGA_DRAW_PROTOCOL  *This,
  IN  EFI_UGA_PIXEL          *BltBuffer, OPTIONAL
  IN  EFI_UGA_BLT_OPERATION  BltOperation,
  IN  UINTN                  SourceX,
  IN  UINTN                  SourceY,
  IN  UINTN                  DestinationX,
  IN  UINTN                  DestinationY,
  IN  UINTN                  Width,
  IN  UINTN                  Height,
  IN  UINTN                  Delta
  )
/*++

Routine Description:

  UGA draw protocol instance to block transfer for VGA device

Arguments:

  This          - Pointer to UGA draw protocol instance
  BltBuffer     - The data to transfer to screen
  BltOperation  - The operation to perform
  SourceX       - The X coordinate of the source for BltOperation
  SourceY       - The Y coordinate of the source for BltOperation
  DestinationX  - The X coordinate of the destination for BltOperation
  DestinationY  - The Y coordinate of the destination for BltOperation
  Width         - The width of a rectangle in the blt rectangle in pixels
  Height        - The height of a rectangle in the blt rectangle in pixels
  Delta         - Not used for EfiUgaVideoFill and EfiUgaVideoToVideo operation.
                  If a Delta of 0 is used, the entire BltBuffer will be operated on.
                  If a subrectangle of the BltBuffer is used, then Delta represents 
                  the number of bytes in a row of the BltBuffer.

Returns:

  EFI_INVALID_PARAMETER - Invalid parameter passed in
  EFI_SUCCESS - Blt operation success

--*/
;

//
// BIOS VGA Mini Port Protocol functions
//
EFI_STATUS
EFIAPI
BiosVideoVgaMiniPortSetMode (
  IN  EFI_VGA_MINI_PORT_PROTOCOL  *This,
  IN  UINTN                       ModeNumber
  )
/*++

Routine Description:

  VgaMiniPort protocol interface to set mode

Arguments:

  This        - Pointer to VgaMiniPort protocol instance
  ModeNumber  - The index of the mode

Returns:

  EFI_UNSUPPORTED - The requested mode is not supported
  EFI_SUCCESS - The requested mode is set successfully

--*/
;


BOOLEAN
BiosVideoIsVga (
  IN  EFI_PCI_IO_PROTOCOL       *PciIo
  )
;


//
// Standard VGA Definitions
//
#define VGA_HORIZONTAL_RESOLUTION                         640
#define VGA_VERTICAL_RESOLUTION                           480
#define VGA_NUMBER_OF_BIT_PLANES                          4
#define VGA_PIXELS_PER_BYTE                               8
#define VGA_BYTES_PER_SCAN_LINE                           (VGA_HORIZONTAL_RESOLUTION / VGA_PIXELS_PER_BYTE)
#define VGA_BYTES_PER_BIT_PLANE                           (VGA_VERTICAL_RESOLUTION * VGA_BYTES_PER_SCAN_LINE)

#define VGA_GRAPHICS_CONTROLLER_ADDRESS_REGISTER          0x3ce
#define VGA_GRAPHICS_CONTROLLER_DATA_REGISTER             0x3cf

#define VGA_GRAPHICS_CONTROLLER_SET_RESET_REGISTER        0x00

#define VGA_GRAPHICS_CONTROLLER_ENABLE_SET_RESET_REGISTER 0x01

#define VGA_GRAPHICS_CONTROLLER_COLOR_COMPARE_REGISTER    0x02

#define VGA_GRAPHICS_CONTROLLER_DATA_ROTATE_REGISTER      0x03
#define VGA_GRAPHICS_CONTROLLER_FUNCTION_REPLACE          0x00
#define VGA_GRAPHICS_CONTROLLER_FUNCTION_AND              0x08
#define VGA_GRAPHICS_CONTROLLER_FUNCTION_OR               0x10
#define VGA_GRAPHICS_CONTROLLER_FUNCTION_XOR              0x18

#define VGA_GRAPHICS_CONTROLLER_READ_MAP_SELECT_REGISTER  0x04

#define VGA_GRAPHICS_CONTROLLER_MODE_REGISTER             0x05
#define VGA_GRAPHICS_CONTROLLER_READ_MODE_0               0x00
#define VGA_GRAPHICS_CONTROLLER_READ_MODE_1               0x08
#define VGA_GRAPHICS_CONTROLLER_WRITE_MODE_0              0x00
#define VGA_GRAPHICS_CONTROLLER_WRITE_MODE_1              0x01
#define VGA_GRAPHICS_CONTROLLER_WRITE_MODE_2              0x02
#define VGA_GRAPHICS_CONTROLLER_WRITE_MODE_3              0x03

#define VGA_GRAPHICS_CONTROLLER_MISCELLANEOUS_REGISTER    0x06

#define VGA_GRAPHICS_CONTROLLER_COLOR_DONT_CARE_REGISTER  0x07

#define VGA_GRAPHICS_CONTROLLER_BIT_MASK_REGISTER         0x08

#endif
