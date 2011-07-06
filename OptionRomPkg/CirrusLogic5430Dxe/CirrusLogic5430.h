/** @file
  Cirrus Logic 5430 Controller Driver

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//
// Cirrus Logic 5430 Controller Driver
//

#ifndef _CIRRUS_LOGIC_5430_H_
#define _CIRRUS_LOGIC_5430_H_


#include <Uefi.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/PciIo.h>
#include <Protocol/DriverSupportedEfiVersion.h>
#include <Protocol/EdidOverride.h>
#include <Protocol/EdidDiscovered.h>
#include <Protocol/EdidActive.h>
#include <Protocol/DevicePath.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/TimerLib.h>

#include <IndustryStandard/Pci.h>
//
// Cirrus Logic 5430 PCI Configuration Header values
//
#define CIRRUS_LOGIC_VENDOR_ID                0x1013
#define CIRRUS_LOGIC_5430_DEVICE_ID           0x00a8
#define CIRRUS_LOGIC_5430_ALTERNATE_DEVICE_ID 0x00a0
#define CIRRUS_LOGIC_5446_DEVICE_ID           0x00b8

//
// Cirrus Logic Graphical Mode Data
//
#define CIRRUS_LOGIC_5430_MODE_COUNT         3

typedef struct {
  UINT32  ModeNumber;
  UINT32  HorizontalResolution;
  UINT32  VerticalResolution;
  UINT32  ColorDepth;
  UINT32  RefreshRate;
} CIRRUS_LOGIC_5430_MODE_DATA;

#define PIXEL_RED_SHIFT   0
#define PIXEL_GREEN_SHIFT 3
#define PIXEL_BLUE_SHIFT  6

#define PIXEL_RED_MASK    (BIT7 | BIT6 | BIT5)
#define PIXEL_GREEN_MASK  (BIT4 | BIT3 | BIT2)
#define PIXEL_BLUE_MASK   (BIT1 | BIT0)

#define PIXEL_TO_COLOR_BYTE(pixel, mask, shift) ((UINT8) ((pixel & mask) << shift))
#define PIXEL_TO_RED_BYTE(pixel) PIXEL_TO_COLOR_BYTE(pixel, PIXEL_RED_MASK, PIXEL_RED_SHIFT)
#define PIXEL_TO_GREEN_BYTE(pixel) PIXEL_TO_COLOR_BYTE(pixel, PIXEL_GREEN_MASK, PIXEL_GREEN_SHIFT)
#define PIXEL_TO_BLUE_BYTE(pixel) PIXEL_TO_COLOR_BYTE(pixel, PIXEL_BLUE_MASK, PIXEL_BLUE_SHIFT)

#define RGB_BYTES_TO_PIXEL(Red, Green, Blue) \
  (UINT8) ( (((Red) >> PIXEL_RED_SHIFT) & PIXEL_RED_MASK) | \
            (((Green) >> PIXEL_GREEN_SHIFT) & PIXEL_GREEN_MASK) | \
            (((Blue) >> PIXEL_BLUE_SHIFT) & PIXEL_BLUE_MASK) )

#define GRAPHICS_OUTPUT_INVALIDE_MODE_NUMBER  0xffff

//
// Cirrus Logic 5440 Private Data Structure
//
#define CIRRUS_LOGIC_5430_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('C', 'L', '5', '4')

typedef struct {
  UINT64                                Signature;
  EFI_HANDLE                            Handle;
  EFI_PCI_IO_PROTOCOL                   *PciIo;
  UINT64                                OriginalPciAttributes;
  EFI_UGA_DRAW_PROTOCOL                 UgaDraw;
  EFI_GRAPHICS_OUTPUT_PROTOCOL          GraphicsOutput;
  EFI_EDID_DISCOVERED_PROTOCOL          EdidDiscovered;
  EFI_EDID_ACTIVE_PROTOCOL              EdidActive;
  EFI_DEVICE_PATH_PROTOCOL              *GopDevicePath;
  EFI_DEVICE_PATH_PROTOCOL              *UgaDevicePath;
  UINTN                                 CurrentMode;
  UINTN                                 MaxMode;
  CIRRUS_LOGIC_5430_MODE_DATA           ModeData[CIRRUS_LOGIC_5430_MODE_COUNT];
  UINT8                                 *LineBuffer;
  BOOLEAN                               HardwareNeedsStarting;
} CIRRUS_LOGIC_5430_PRIVATE_DATA;

///
/// Video Mode structure
///
typedef struct {
  UINT32  Width;
  UINT32  Height;
  UINT32  ColorDepth;
  UINT32  RefreshRate;
  UINT8   *CrtcSettings;
  UINT16  *SeqSettings;
  UINT8   MiscSetting;
} CIRRUS_LOGIC_5430_VIDEO_MODES;

#define CIRRUS_LOGIC_5430_PRIVATE_DATA_FROM_UGA_DRAW_THIS(a) \
  CR(a, CIRRUS_LOGIC_5430_PRIVATE_DATA, UgaDraw, CIRRUS_LOGIC_5430_PRIVATE_DATA_SIGNATURE)

#define CIRRUS_LOGIC_5430_PRIVATE_DATA_FROM_GRAPHICS_OUTPUT_THIS(a) \
  CR(a, CIRRUS_LOGIC_5430_PRIVATE_DATA, GraphicsOutput, CIRRUS_LOGIC_5430_PRIVATE_DATA_SIGNATURE)


//
// Global Variables
//
extern UINT8                                      AttributeController[];
extern UINT8                                      GraphicsController[];
extern UINT8                                      Crtc_640_480_256_60[];
extern UINT16                                     Seq_640_480_256_60[];
extern UINT8                                      Crtc_800_600_256_60[];
extern UINT16                                     Seq_800_600_256_60[];
extern UINT8                                      Crtc_1024_768_256_60[];
extern UINT16                                     Seq_1024_768_256_60[];
extern CIRRUS_LOGIC_5430_VIDEO_MODES              CirrusLogic5430VideoModes[];
extern EFI_DRIVER_BINDING_PROTOCOL                gCirrusLogic5430DriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL                gCirrusLogic5430ComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL               gCirrusLogic5430ComponentName2;
extern EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL  gCirrusLogic5430DriverSupportedEfiVersion;

//
// Io Registers defined by VGA
//
#define CRTC_ADDRESS_REGISTER   0x3d4
#define CRTC_DATA_REGISTER      0x3d5
#define SEQ_ADDRESS_REGISTER    0x3c4
#define SEQ_DATA_REGISTER       0x3c5
#define GRAPH_ADDRESS_REGISTER  0x3ce
#define GRAPH_DATA_REGISTER     0x3cf
#define ATT_ADDRESS_REGISTER    0x3c0
#define MISC_OUTPUT_REGISTER    0x3c2
#define INPUT_STATUS_1_REGISTER 0x3da
#define DAC_PIXEL_MASK_REGISTER 0x3c6
#define PALETTE_INDEX_REGISTER  0x3c8
#define PALETTE_DATA_REGISTER   0x3c9

//
// UGA Draw Hardware abstraction internal worker functions
//
EFI_STATUS
CirrusLogic5430UgaDrawConstructor (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  );

EFI_STATUS
CirrusLogic5430UgaDrawDestructor (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  );

//
// Graphics Output Hardware abstraction internal worker functions
//
EFI_STATUS
CirrusLogic5430GraphicsOutputConstructor (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  );

EFI_STATUS
CirrusLogic5430GraphicsOutputDestructor (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  );


//
// EFI_DRIVER_BINDING_PROTOCOL Protocol Interface
//
/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  Controller TODO: add argument description
  @param  RemainingDevicePath TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EFIAPI
CirrusLogic5430ControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  Controller TODO: add argument description
  @param  RemainingDevicePath TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EFIAPI
CirrusLogic5430ControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  Controller TODO: add argument description
  @param  NumberOfChildren TODO: add argument description
  @param  ChildHandleBuffer TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EFIAPI
CirrusLogic5430ControllerDriverStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  );

//
// EFI Component Name Functions
//
/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.

  @param  DriverName[out]       A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER DriverName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
CirrusLogic5430ComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );


/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle[in]  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle[in]       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param  ControllerName[out]   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
CirrusLogic5430ComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );


//
// Local Function Prototypes
//
VOID
InitializeGraphicsMode (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  CIRRUS_LOGIC_5430_VIDEO_MODES   *ModeData
  );

VOID
SetPaletteColor (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  UINTN                           Index,
  UINT8                           Red,
  UINT8                           Green,
  UINT8                           Blue
  );

VOID
SetDefaultPalette (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  );

VOID
DrawLogo (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  UINTN                           ScreenWidth,
  UINTN                           ScreenHeight
  );

VOID
outb (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  UINTN                           Address,
  UINT8                           Data
  );

VOID
outw (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  UINTN                           Address,
  UINT16                          Data
  );

UINT8
inb (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  UINTN                           Address
  );

UINT16
inw (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  UINTN                           Address
  );

EFI_STATUS
CirrusLogic5430VideoModeSetup (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  );

#endif
