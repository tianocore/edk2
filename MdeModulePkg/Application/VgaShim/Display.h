#ifndef __DISPLAY_H__
#define __DISPLAY_H__


/**
  -----------------------------------------------------------------------------
  Includes.
  -----------------------------------------------------------------------------
**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/UgaDraw.h>
#include "ConsoleControl.h"


/**
  -----------------------------------------------------------------------------
  Type definitions and enums.
  -----------------------------------------------------------------------------
**/

typedef struct {
	UINTN			Width;
	UINTN			Height;
	EFI_UGA_PIXEL	*PixelData;
} IMAGE;

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
	CHAR8	Signature[2];
	UINT32	FileSizeBytes;
	UINT16	Reserved[2];
	UINT32	PixelDataOffset;
	// DIB header
	UINT32	DibHeaderSize;
	UINT32	Width;
	UINT32	Height;
	UINT16	Planes;				// expect '1'
	UINT16	BitPerPixel;		// expect '24' for 24bpp
	UINT32	CompressionType;	// expect '0' for no compression
	UINT32	ImageSize;			// size of the raw bitmap data
	UINT32	XPixelsPerMeter;
	UINT32	YPixelsPerMeter;
	UINT32	NumberOfColors;
	UINT32	ImportantColors;	// ignored
} BMP_HEADER;
#pragma pack()


/**
  -----------------------------------------------------------------------------
  Exported method signatures.
  -----------------------------------------------------------------------------
**/

VOID PrintVideoInfo(
	VOID);

VOID ClearScreen(
	VOID);

VOID DestroyImage(
	IN	IMAGE	*Image);

IMAGE*
CreateImage(
	IN	UINTN	Width,
	IN	UINTN	Height);

EFI_STATUS
BmpFileToImage(
	IN	UINT8	*FileData,
	IN	UINTN	FileSizeBytes,
	OUT	VOID	**Result);

VOID
DrawImage(
	IN	IMAGE	*Image,
	IN	UINTN	Width,
	IN	UINTN	Height,
	IN	UINTN	ScreenX,
	IN	UINTN	ScreenY,
	IN	UINTN	ImageX,
	IN	UINTN	ImageY);

VOID
DrawImageCentered(
	IN	IMAGE	*Image);

VOID
AnimateImage(
	IN	IMAGE	*Image);

EFI_STATUS
EnsureDisplayAvailable(
	VOID);


/**
  -----------------------------------------------------------------------------
  Imported global variables.
  -----------------------------------------------------------------------------
**/

extern	DISPLAY_INFO				DisplayInfo;
extern	EFI_LOADED_IMAGE_PROTOCOL	*VgaShimImage;

#endif