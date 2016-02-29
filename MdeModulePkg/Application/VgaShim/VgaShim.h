#ifndef __VGA_SHIM_H__
#define __VGA_SHIM_H__


/**
  -----------------------------------------------------------------------------
  Defines.
  -----------------------------------------------------------------------------
**/

#define SCAN_ENTER	0x001C

/**
  -----------------------------------------------------------------------------
  Project-wide includes.
  -----------------------------------------------------------------------------
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/MtrrLib.h>
#include <Library/PciLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Protocol/LegacyRegion.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LegacyRegion2.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/GraphicsOutput.h>
// replace with #include <../../EdkCompatibilityPkg/Foundation/Protocol/ConsoleControl/ConsoleControl.h>
#include "ConsoleControl.h"


/**
  -----------------------------------------------------------------------------
  Type definitions and enums.
  -----------------------------------------------------------------------------
**/

#pragma pack(1)
typedef struct {
  UINT16 Offset;
  UINT16 Segment;
} IVT_ENTRY;
#pragma pack()

typedef struct {
	UINTN			Width;
	UINTN			Height;
	EFI_UGA_PIXEL	*PixelData;
} IMAGE;

typedef enum
{
	LOCK,
	UNLOCK
} MEMORY_LOCK_OPERATION;

typedef enum
{
	NONE,
	GOP,
	UGA
} GRAPHICS_PROTOCOL;

typedef struct {
	BOOLEAN							Initialized;
	BOOLEAN							AdapterFound;

	GRAPHICS_PROTOCOL				Protocol;
	EFI_UGA_DRAW_PROTOCOL			*UGA;
	EFI_GRAPHICS_OUTPUT_PROTOCOL	*GOP;

	UINT32							HorizontalResolution;
	UINT32							VerticalResolution;
	EFI_GRAPHICS_PIXEL_FORMAT		PixelFormat;
	UINT32							PixelsPerScanLine;
	EFI_PHYSICAL_ADDRESS			FrameBufferBase;
	UINTN							FrameBufferSize;
} DISPLAY_INFO;

#pragma pack(1)
typedef struct {
	// File header
	CHAR8		Signature[2];
	UINT32		FileSizeBytes;
	UINT16		Reserved[2];
	UINT32		PixelDataOffset;
	// DIB header
	UINT32		DibHeaderSize;
	UINT32		Width;
	UINT32		Height;
	UINT16		Planes;				// expect '1'
	UINT16		BitPerPixel;		// expect '24' for 24bpp
	UINT32		CompressionType;	// expect '0' for no compression
	UINT32		ImageSize;			// size of the raw bitmap data
	UINT32		XPixelsPerMeter;
	UINT32		YPixelsPerMeter;
	UINT32		NumberOfColors;
	UINT32		ImportantColors;	// ignored
} BMP_HEADER;
#pragma pack()


/**
  -----------------------------------------------------------------------------
  Method signatures.
  -----------------------------------------------------------------------------
**/

EFI_STATUS InitializeDisplay(
	VOID);

BOOLEAN CanWriteAtAddress(
	IN		EFI_PHYSICAL_ADDRESS Address);

EFI_STATUS EnsureMemoryLock(
	IN		EFI_PHYSICAL_ADDRESS Address, 
	IN		UINT32 Length, 
	IN		MEMORY_LOCK_OPERATION Operation);

BOOLEAN IsInt10HandlerDefined();

EFI_STATUS ShimVesaInformation(
	IN		EFI_PHYSICAL_ADDRESS StartAddress, 
	OUT		EFI_PHYSICAL_ADDRESS *EndAddress);

VOID PrintVideoInfo(
	VOID);

VOID ClearScreen(
	VOID);

VOID DestroyImage(
	IN		IMAGE	*Image);

IMAGE*
CreateImage(
	IN		UINTN	Width,
	IN		UINTN	Height);

IMAGE*
BmpFileToImage(
	IN		UINT8	*FileData,
	IN		UINTN	FileSizeBytes);

VOID
DrawImage(
	IN		IMAGE	*Image,
	IN		UINTN	Width,
	IN		UINTN	Height,
	IN		UINTN	ScreenX,
	IN		UINTN	ScreenY,
	IN		UINTN	ImageX,
	IN		UINTN	ImageY);

VOID
DrawImageCentered(
	IN		IMAGE	*Image,
	IN		UINTN	Width,
	IN		UINTN	Height,
	IN		UINTN	ImageWindowX,
	IN		UINTN	ImageWindowY);

VOID
AnimateImage(
	IN		IMAGE	*Image);

EFI_STATUS
EnsureDisplayAvailable(
	VOID);


/**
  -----------------------------------------------------------------------------
  Constants and project-wide variables.
  -----------------------------------------------------------------------------
**/

STATIC CONST	CHAR8					VENDOR_NAME[]		= "Apple";
STATIC CONST	CHAR8					PRODUCT_NAME[]		= "Emulated VGA";
STATIC CONST	CHAR8					PRODUCT_REVISION[]	= "OVMF Int10h (fake)";
STATIC CONST	EFI_PHYSICAL_ADDRESS	VGA_ROM_ADDRESS		= 0xc0000;
STATIC CONST	EFI_PHYSICAL_ADDRESS	IVT_ADDRESS			= 0x00000;
STATIC CONST	UINTN					VGA_ROM_SIZE		= 0x10000;
STATIC CONST	UINTN					FIXED_MTRR_SIZE		= 0x20000;

extern			DISPLAY_INFO			DisplayInfo;

#endif