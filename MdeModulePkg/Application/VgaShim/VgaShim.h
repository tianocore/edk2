#ifndef __VGA_SHIM_H__
#define __VGA_SHIM_H__

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
#include "ConsoleControl.h"

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
	MEM_LOCK,
	MEM_UNLOCK
} MEMORY_LOCK_OPERATION;

typedef struct {
	UINT32						HorizontalResolution;
	UINT32						VerticalResolution;
	EFI_GRAPHICS_PIXEL_FORMAT	PixelFormat;
	UINT32						PixelsPerScanLine;
	EFI_PHYSICAL_ADDRESS		FrameBufferBase;
	UINTN						FrameBufferSize;
} VIDEO_INFO;

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
	UINT16		Planes;				// always 1
	UINT16		BitPerPixel;		// 1, 4, 8, 24
	UINT32		CompressionType;	// BMP_COMPRESSION_TYPE
	UINT32		ImageSize;			// size of the raw bitmap data
	UINT32		XPixelsPerMeter;
	UINT32		YPixelsPerMeter;
	UINT32		NumberOfColors;
	UINT32		ImportantColors;	// ignored
} BMP_HEADER;
#pragma pack()

EFI_STATUS InitializeGraphics(
	VOID);

BOOLEAN CanWriteAtAddress(
	IN		EFI_PHYSICAL_ADDRESS Address);

EFI_STATUS EnsureMemoryLock(
	IN		EFI_PHYSICAL_ADDRESS Address, 
	IN		UINT32 Length, 
	IN		MEMORY_LOCK_OPERATION Operation);

BOOLEAN IsInt10HandlerDefined();

EFI_STATUS FillVesaInformation(
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
	IN		UINTN	X,
	IN		UINTN	Y);

VOID
DrawImageCentered(
	IN		IMAGE	*Image);

STATIC CONST CHAR8 VENDOR_NAME[] = "Apple";
STATIC CONST CHAR8 PRODUCT_NAME[] = "Emulated VGA";
STATIC CONST CHAR8 PRODUCT_REVISION[] = "OVMF Int10h (fake)";
STATIC CONST EFI_PHYSICAL_ADDRESS VGA_ROM_ADDRESS = 0xc0000;
STATIC CONST EFI_PHYSICAL_ADDRESS IVT_ADDRESS = 0x00000;
STATIC CONST UINTN VGA_ROM_SIZE = 0x10000;
STATIC CONST UINTN FIXED_MTRR_SIZE = 0x20000;

#endif