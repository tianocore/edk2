/** @file

  Copyright (c) 2016, Dawid Ciecierski

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Display.h"
#include "Util.h"


/**
  -----------------------------------------------------------------------------
  Local method signatures.
  -----------------------------------------------------------------------------
**/

EFI_STATUS
CalculatePositionForCenter(
	IN		UINTN		ImageWidth,
	IN		UINTN		ImageHeight,
	OUT		UINTN		*PositionX,
	OUT		UINTN		*PositionY);

EFI_STATUS
InitializeDisplay(
	VOID);


/**
  -----------------------------------------------------------------------------
  Local method implementations.
  -----------------------------------------------------------------------------
**/

/**
  Scans the system for Graphics Output Protocol (GOP) and
  Universal Graphic Adapter (UGA) compatible adapters/GPUs.
  If one is found, vital information about its video mode is
  retrieved and stored for later use.

  @retval EFI_SUCCESS     An adapter was found and its current
                          mode parameters stored in DisplayInfo
						  global variable.
  @retval other           No compatible adapters were found or
                          their mode parameters could not be
						  retrieved.
  
**/
EFI_STATUS
InitializeDisplay()
{
	EFI_STATUS	Status;
	UINT32		Temp1;
	UINT32		Temp2;
	
	// Sets AdapterFound = FALSE and Protocol = NONE
	SetMem(&DisplayInfo, sizeof(DISPLAY_INFO), 0);

	//
	// Try a GOP adapter first.
	//
	Status = gBS->HandleProtocol(gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID **)&DisplayInfo.GOP);
	if (!EFI_ERROR(Status)) {
		PrintDebug(L"Found a GOP display adapter\n");
		DisplayInfo.HorizontalResolution = DisplayInfo.GOP->Mode->Info->HorizontalResolution;
		DisplayInfo.VerticalResolution = DisplayInfo.GOP->Mode->Info->VerticalResolution;
		DisplayInfo.PixelFormat = DisplayInfo.GOP->Mode->Info->PixelFormat;
		DisplayInfo.PixelsPerScanLine = DisplayInfo.GOP->Mode->Info->PixelsPerScanLine;
		DisplayInfo.FrameBufferBase = DisplayInfo.GOP->Mode->FrameBufferBase;
		// usually = PixelsPerScanLine * VerticalResolution * BytesPerPixel
		// for MacBookAir7,2: 1536 * 900 * 4 = 5,529,600 bytes
		DisplayInfo.FrameBufferSize = DisplayInfo.GOP->Mode->FrameBufferSize;

		DisplayInfo.Protocol = GOP;
		DisplayInfo.AdapterFound = TRUE;
		goto Exit;
	} else {
		PrintDebug(L"GOP display adapter not found\n");
	}

	//
	// Try a UGA adapter.
	//
	DisplayInfo.GOP = NULL;
	Status = gBS->HandleProtocol(gST->ConsoleOutHandle, &gEfiUgaDrawProtocolGuid, (VOID **)&DisplayInfo.UGA);
	if (!EFI_ERROR(Status)) {
		PrintDebug(L"Found a UGA display adapter\n");
		Status = DisplayInfo.UGA->GetMode(DisplayInfo.UGA, &DisplayInfo.HorizontalResolution, &DisplayInfo.VerticalResolution, &Temp1, &Temp2);
		if (EFI_ERROR(Status)) {
			PrintError(L"Unable to get current UGA mode (error: %r)\n", Status);
			DisplayInfo.UGA = NULL;
			goto Exit;
		} else {
			PrintDebug(L"Received current UGA mode information\n");
		}
		DisplayInfo.PixelFormat = PixelBlueGreenRedReserved8BitPerColor; // default for UGA
		// TODO: find framebuffer base
		// TODO: find scanline length
		// https://github.com/coreos/grub/blob/master/grub-core%2Fvideo%2Fefi_uga.c
		DisplayInfo.Protocol = UGA;
		DisplayInfo.AdapterFound = TRUE;
	} else {
		PrintDebug(L"UGA display adapter not found\n");
	}

Exit:
	if (!DisplayInfo.AdapterFound) {
		PrintError(L"No display adapters found\n", Status);
	}
	DisplayInfo.Initialized = TRUE;
	return Status;
}


/**
  Calculates the x and y coordinates so that the given image
  would be displayed in screen center at the current resolution.
  Image width and height are given explicitly to allow for arbitrary
  calculations useful for sprites.

  @param[in] ImageWidth   Image width.
  @param[in] ImageHeight  Image height.
  @param[out] PositionX   Screen X coordinate of the top left corner
                          of the centered image.
  @param[out] PositionX   Screen Y coordinate of the top left corner
                          of the centered image.

  @retval EFI_SUCCESS     Screen center values were successfully
                          calculated for the current resolution
						  and specified image.
  @retval other           Either no graphics adapter was found,
                          the image was too big to fit on the
						  screen at current resolution or some
						  other problem was encountered.
  
**/
EFI_STATUS
CalculatePositionForCenter(
	IN	UINTN	ImageWidth,
	IN	UINTN	ImageHeight,
	OUT	UINTN	*PositionX,
	OUT	UINTN	*PositionY)
{
	if (EFI_ERROR(EnsureDisplayAvailable())) {
		PrintDebug(L"No display adapters found, unable to calculate centered position\n");
		return EFI_DEVICE_ERROR;
	}

	if (ImageWidth == 0 || ImageHeight == 0 
		|| ImageWidth > DisplayInfo.HorizontalResolution
		|| ImageHeight > DisplayInfo.VerticalResolution) {
		PrintDebug(L"Wrong image size (%ux%u) for this screen resolution (%ux%u)\n", 
			ImageWidth, ImageHeight, DisplayInfo.HorizontalResolution, DisplayInfo.VerticalResolution);
		return EFI_INVALID_PARAMETER;
	}

	*PositionX = (DisplayInfo.HorizontalResolution / 2) - (ImageWidth / 2);
	*PositionY = (DisplayInfo.VerticalResolution / 2) - (ImageHeight / 2);

	if (*PositionX + ImageWidth > DisplayInfo.HorizontalResolution)
		*PositionX = DisplayInfo.HorizontalResolution - ImageWidth;
	if (*PositionY + ImageHeight > DisplayInfo.VerticalResolution)
		*PositionY = DisplayInfo.VerticalResolution - ImageHeight;

	//PrintDebug(L"Top left corner position for centered image: %u,%u\n", *PositionX, *PositionY);

	return EFI_SUCCESS;
}


/**
  -----------------------------------------------------------------------------
  Exported method implementations.
  -----------------------------------------------------------------------------
**/

/**
  Performs the initial scan for graphics adapters if one
  has not been performed yet and returns a simple TRUE/FALSE
  information if one has been found and information about
  it is ready for use in the global DisplayInfo variable.

  @retval TRUE            An adapter has been found and its current
                          mode parameters stored in DisplayInfo
						  global variable.
  @retval other           No compatible adapters were found or
                          their mode parameters could not be
						  retrieved.
  
**/
EFI_STATUS
EnsureDisplayAvailable()
{
	if (!DisplayInfo.Initialized) {
		InitializeDisplay();
	}
	return DisplayInfo.AdapterFound && DisplayInfo.Protocol != NONE ? EFI_SUCCESS : EFI_NOT_FOUND;
}


/**
  Prints important information about the currently running video
  mode. Initializes adapters if they have not yet been detected.

**/
VOID
PrintVideoInfo()
{
	UINT32									MaxMode;
	UINT32									i;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION	*ModeInfo;
	UINTN									SizeOfInfo;
	EFI_STATUS								Status;

	if (EFI_ERROR(EnsureDisplayAvailable())) {
		PrintDebug(L"No display adapters found, unable to print display information\n");
		return;
	}

	PrintDebug(L"Current mode:\n");
	PrintDebug(L"  HorizontalResolution = %u\n", DisplayInfo.HorizontalResolution);
	PrintDebug(L"  VerticalResolution = %u\n", DisplayInfo.VerticalResolution);
	PrintDebug(L"  PixelFormat = %u\n", DisplayInfo.PixelFormat);
	PrintDebug(L"  PixelsPerScanLine = %u\n", DisplayInfo.PixelsPerScanLine);
	PrintDebug(L"  FrameBufferBase = %x\n", DisplayInfo.FrameBufferBase);
	PrintDebug(L"  FrameBufferSize = %u\n", DisplayInfo.FrameBufferSize);

	// Query available modes.
	MaxMode = DisplayInfo.GOP->Mode->MaxMode;
	PrintDebug(L"Available modes (MaxMode = %u):\n", MaxMode);
	for (i = 0; i < MaxMode; i++) {
			
		Status = DisplayInfo.GOP->QueryMode(DisplayInfo.GOP, i, &SizeOfInfo, &ModeInfo);
		if (!EFI_ERROR(Status)) {
			PrintDebug(L"  Mode%u: %ux%u\n", i, ModeInfo->HorizontalResolution, ModeInfo->VerticalResolution);
		}
	}
}


/**
  Allocates resources for a new in-memory image of the specified
  width and height.

  @param[in] Width        Desired image width.
  @param[in] Height       Desired image height.

  @retval IMAGE*          Pointer to a zero-initialized memory 
                          location ready to receive pixel data
						  up to the width and height specified.

**/
IMAGE*
CreateImage(
	IN	UINTN	Width,
	IN	UINTN	Height)
{
	IMAGE	*Image;

	Image = (IMAGE *)AllocatePool(sizeof(IMAGE));
	if (Image == NULL) {
		return NULL;
	}
		
	Image->Width = Width;
	Image->Height = Height;
	Image->PixelData = (EFI_UGA_PIXEL *)AllocatePool(Width * Height * sizeof(EFI_UGA_PIXEL));
	if (Image->PixelData == NULL) {
		DestroyImage(Image);
		return NULL;
	}
	
	SetMem(Image->PixelData, sizeof Image->PixelData, 0);
	return Image;
}


/**
  Releases all resouces held by the specified image.

  @param[in] Image        Image whose memory resources are to
                          be released.

**/
VOID
DestroyImage(
	IN	IMAGE	*Image)
{
	if (Image != NULL) {
        if (Image->PixelData != NULL)
            FreePool(Image->PixelData);
        FreePool(Image);
    }
}


/**
  Converts bytes of a bitmap file into a memory representation
  useful for other graphics in-memory operations.

  Any error messages will only be printed on the debug console
  and only the error code returned to caller.

  @param[in] FileData      Pointer to the first byte of file contents.
  @param[in] FileSizeBytes Total number of bytes available at the
                           specified location.
  @param[out] Result       Pointer to a mermory location holding
                           the address of the image structure
						   and data representing the specified bmp file.

  @retval EFI_SUCCESS      File data was interpreted successfully.
  @retval other            Either the file contained no valid or 
                           supported image, no memory could be
						   allocated to hold pixel data or some other
						   problem was encountered.

**/
EFI_STATUS
BmpFileToImage(
	IN	UINT8	*FileData,
	IN	UINTN	FileSizeBytes,
	OUT	VOID	**Result)
{
	//IMAGE			*Image;
	BMP_HEADER		*BmpHeader;
	UINT8			*BmpCurrentPixel;
	UINT8			*BmpCurrentLine;
	UINTN			LineSizeBytes;
	EFI_UGA_PIXEL	*TargetPixel;
	UINTN			x, y;

	// Sanity checks.
	if (FileData == NULL || FileSizeBytes < sizeof(BMP_HEADER)) {
		PrintDebug(L"File too small or does not exist\n");
		return EFI_INVALID_PARAMETER;
	}

	BmpHeader = (BMP_HEADER *)FileData;
	if (BmpHeader->Signature[0] != 'B' 
		|| BmpHeader->Signature[1] != 'M'
		|| BmpHeader->CompressionType != 0	// only support uncompressed...
		|| BmpHeader->BitPerPixel != 24		// ...24 bits per pixel images
		|| BmpHeader->Width < 1
		|| BmpHeader->Height < 1) {
		return EFI_INVALID_PARAMETER;
	}
		
	*Result = CreateImage(BmpHeader->Width, BmpHeader->Height);
	if (*Result == NULL) {
		PrintDebug(L"Unable to allocate enough memory for image size %ux%u\n", 
			BmpHeader->Width, BmpHeader->Height);
		return EFI_OUT_OF_RESOURCES;
	}
	
	// Calculate line size and adjust with padding to multiple of 4 bytes.
	LineSizeBytes = BmpHeader->Width * 3; // 24 bits = 3 bytes
	LineSizeBytes += (LineSizeBytes % 4) != 0
		? (4 - (LineSizeBytes % 4))
		: 0;
	
	// Check if we have enough pixel data.
	if (BmpHeader->PixelDataOffset + BmpHeader->Height * LineSizeBytes > FileSizeBytes) {
		PrintDebug(L"Not enough pixel data (%u bytes, expected %u)\n", 
			FileSizeBytes, BmpHeader->PixelDataOffset + BmpHeader->Height * LineSizeBytes);
		DestroyImage((IMAGE *)*Result);
		return EFI_INVALID_PARAMETER;
	}
		
	// Fill in pixel values.
	BmpCurrentLine = FileData + BmpHeader->PixelDataOffset;
	for (y = 0; y < BmpHeader->Height; y++) {
		BmpCurrentPixel = BmpCurrentLine;
		BmpCurrentLine += LineSizeBytes;
		// jump to the right pixel line; BMP PixelArray is bottom-to-top...
		TargetPixel = ((IMAGE *)*Result)->PixelData + BmpHeader->Width * (BmpHeader->Height - y - 1);
		// ...but thankfully left-to-right
		for (x = 0; x < BmpHeader->Width; x++) {
			TargetPixel->Blue		= *BmpCurrentPixel++;
			TargetPixel->Green		= *BmpCurrentPixel++;
			TargetPixel->Red		= *BmpCurrentPixel++;
			TargetPixel->Reserved	= 0;
			TargetPixel++;
		}
	}

	PrintDebug(L"Successfully imported image size %ux%u from bmp file\n", 
		((IMAGE *)*Result)->Width, ((IMAGE *)*Result)->Height);
	return EFI_SUCCESS;
}


/**
  Clears screen in both text and graphics modes.

**/
VOID
ClearScreen()
{
    EFI_UGA_PIXEL	FillColor;
	
	FillColor.Red		= 0;
	FillColor.Green		= 0;
	FillColor.Blue		= 0;
	FillColor.Reserved	= 0;

	if (EFI_ERROR(EnsureDisplayAvailable())) {
		PrintDebug(L"No display adapters found, unable to clear screen\n");
		return;
	}

	SwtichToGraphics(FALSE);

	if (DisplayInfo.Protocol == GOP) {
		DisplayInfo.GOP->Blt(
			DisplayInfo.GOP,
			(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)&FillColor,
			EfiBltVideoFill,
			0, 0, 0, 0,
			DisplayInfo.HorizontalResolution, DisplayInfo.VerticalResolution, 0);
	} else if (DisplayInfo.Protocol == UGA) {
		DisplayInfo.UGA->Blt(
			DisplayInfo.UGA,
			&FillColor,
			EfiUgaVideoFill,
			0, 0, 0, 0,
			DisplayInfo.HorizontalResolution, DisplayInfo.VerticalResolution, 0);
	}
}


VOID 
DrawImage(
	IN	IMAGE	*Image,
	IN	UINTN	DrawWidth,
	IN	UINTN	DrawHeight,
	IN	UINTN	ScreenX,
	IN	UINTN	ScreenY,
	IN	UINTN	SpriteX,
	IN	UINTN	SpriteY)
{
	if (EFI_ERROR(EnsureDisplayAvailable())) {
		PrintDebug(L"No display adapters found, unable to draw image\n");
		return;
	}

	if (Image == NULL || DrawWidth == 0 || DrawHeight == 0) {
		PrintDebug(L"No image to draw\n");
		return;
	}
	if ((ScreenX + DrawWidth) > DisplayInfo.HorizontalResolution 
		|| (ScreenY + DrawHeight) > DisplayInfo.VerticalResolution) {
		PrintDebug(L"Image too big to draw on screen\n");
		return;
	}

	SwtichToGraphics(FALSE);

	if (DisplayInfo.Protocol == GOP) {
		DisplayInfo.GOP->Blt(DisplayInfo.GOP, 
			(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)Image->PixelData, 
			EfiBltBufferToVideo, 
			SpriteX, SpriteY, ScreenX, ScreenY, 
			DrawWidth, DrawHeight, 0);
	} else if (DisplayInfo.Protocol == UGA) {
		DisplayInfo.UGA->Blt(DisplayInfo.UGA, 
			(EFI_UGA_PIXEL *)Image->PixelData, 
			EfiUgaBltBufferToVideo,
			SpriteX, SpriteY, ScreenX, ScreenY, 
			DrawWidth, DrawHeight, 0);
	}
}


VOID
DrawImageCentered(
	IN	IMAGE	*Image)
{
	EFI_STATUS	Status;
	UINTN		PositionX;
	UINTN		PositionY;

	if (EFI_ERROR(EnsureDisplayAvailable())) {
		PrintDebug(L"No display adapters found, unable to draw centered image\n");
		return;
	}

	Status = CalculatePositionForCenter(Image->Width, Image->Height, &PositionX, &PositionY);
	if (EFI_ERROR(Status)) {
		return;
	}
	
	DrawImage(Image, Image->Width, Image->Height, PositionX, PositionY, 0, 0);
}


VOID
AnimateImage(
	IN	IMAGE	*Image)
{
	EFI_STATUS	Status;
	UINTN		NumFrames;
	UINTN		Frame;
	UINTN		MsPerFrame = 20;
	UINTN		PositionX;
	UINTN		PositionY;

	if (Image->Width == Image->Height) {
		// animation called by mistake, just show on screen
		DrawImageCentered(Image);
	} else if (Image->Width > Image->Height) {
		// frames are stacked left-to-right
		Status = CalculatePositionForCenter(Image->Height, Image->Height, &PositionX, &PositionY);
		if (EFI_ERROR(Status)) {
			return;
		}
		NumFrames = Image->Width / Image->Height;
		for (Frame = 0; Frame < NumFrames; Frame++) {
			DrawImage(Image, Image->Height, Image->Height, PositionX, PositionY, Frame * Image->Height, 0);
			gBS->Stall(MsPerFrame * 1000);
		}
	} else {
		// frames are stacked top-to-bottom
		Status = CalculatePositionForCenter(Image->Width, Image->Width, &PositionX, &PositionY);
		if (EFI_ERROR(Status)) {
			return;
		}
		NumFrames = Image->Height / Image->Width;
		for (Frame = 0; Frame < NumFrames; Frame++) {
			DrawImage(Image, Image->Width, Image->Width, PositionX, PositionY, 0, Frame * Image->Width);
			gBS->Stall(MsPerFrame * 1000);
		}
	}
}


VOID
SwitchToText(
	IN	BOOLEAN	Force)
{
	EFI_CONSOLE_CONTROL_SCREEN_MODE	CurrentMode;
	EFI_STATUS						Status;
	EFI_CONSOLE_CONTROL_PROTOCOL	*ConsoleControl;

	Status = gBS->LocateProtocol(&gEfiConsoleControlProtocolGuid, NULL, (VOID**)&ConsoleControl);
	if (EFI_ERROR(Status)) {
		ConsoleControl = NULL;
	}

	if (ConsoleControl != NULL) {
		Status = ConsoleControl->GetMode(ConsoleControl, &CurrentMode, NULL, NULL);
		if (Force || (!EFI_ERROR(Status) && CurrentMode != EfiConsoleControlScreenText)) {
			ConsoleControl->SetMode(ConsoleControl, EfiConsoleControlScreenText);
		}
	}
}


VOID
SwtichToGraphics(
	IN	BOOLEAN	Force)
{
	
	EFI_CONSOLE_CONTROL_SCREEN_MODE	CurrentMode;
	EFI_STATUS						Status;
	EFI_CONSOLE_CONTROL_PROTOCOL	*ConsoleControl;

	Status = gBS->LocateProtocol(&gEfiConsoleControlProtocolGuid, NULL, (VOID**)&ConsoleControl);
	if (EFI_ERROR(Status)) {
		ConsoleControl = NULL;
	}

	if (ConsoleControl != NULL) {
		Status = ConsoleControl->GetMode(ConsoleControl, &CurrentMode, NULL, NULL);
		if (Force || (!EFI_ERROR(Status) && CurrentMode != EfiConsoleControlScreenGraphics)) {
			ConsoleControl->SetMode(ConsoleControl, EfiConsoleControlScreenGraphics);
		}
	}
}