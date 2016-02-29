#include "VgaShim.h"


EFI_STATUS
EnsureDisplayAvailable()
{
	if (!DisplayInfo.Initialized) {
		InitializeDisplay();
	}
	return DisplayInfo.AdapterFound && DisplayInfo.Protocol != NONE ? EFI_SUCCESS : EFI_NOT_FOUND;
}


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

	SetMem(&DisplayInfo, sizeof(DISPLAY_INFO), 0);

	//
	// Try a GOP adapter first.
	//
	Status = gBS->HandleProtocol(gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID **)&DisplayInfo.GOP);
	if (!EFI_ERROR(Status)) {
		Print(L"%a: Found a GOP protocol provider\n", __FUNCTION__);
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
	}

	//
	// Try a UGA adapter.
	//
	DisplayInfo.GOP = NULL;
	Status = gBS->HandleProtocol(gST->ConsoleOutHandle, &gEfiUgaDrawProtocolGuid, (VOID **)&DisplayInfo.UGA);
	if (!EFI_ERROR(Status)) {
		Print(L"%a: Found a UGA protocol provider\n", __FUNCTION__);
		Status = DisplayInfo.UGA->GetMode(DisplayInfo.UGA, &DisplayInfo.HorizontalResolution, &DisplayInfo.VerticalResolution, &Temp1, &Temp2);
		if (EFI_ERROR(Status)) {
			Print(L"%a: Unable to get current UGA mode\n", __FUNCTION__);
			DisplayInfo.UGA = NULL;
			goto Exit;
		}
		DisplayInfo.PixelFormat = PixelBlueGreenRedReserved8BitPerColor; // default for UGA
		// TODO: find framebuffer base
		// TODO: find scanline length
		// https://github.com/coreos/grub/blob/master/grub-core%2Fvideo%2Fefi_uga.c
		DisplayInfo.Protocol = UGA;
		DisplayInfo.AdapterFound = TRUE;
	}

Exit:
	DisplayInfo.Initialized = TRUE;
	return Status;
}


/**
  Prints important information about the currently running video
  mode. Initializes adapters if they have not yet been detected.

**/
VOID
PrintVideoInfo()
{
	if (EFI_ERROR(EnsureDisplayAvailable())) {
		return;
	}

	Print(L"%a: HorizontalResolution = %u\n", __FUNCTION__, DisplayInfo.HorizontalResolution);
	Print(L"%a: VerticalResolution = %u\n", __FUNCTION__, DisplayInfo.VerticalResolution);
	Print(L"%a: PixelFormat = %u\n", __FUNCTION__, DisplayInfo.PixelFormat);
	Print(L"%a: PixelsPerScanLine = %u\n", __FUNCTION__, DisplayInfo.PixelsPerScanLine);
	Print(L"%a: FrameBufferBase = %x\n", __FUNCTION__, DisplayInfo.FrameBufferBase);
	Print(L"%a: FrameBufferSize = %u\n", __FUNCTION__, DisplayInfo.FrameBufferSize);
}


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
		Print(L"%a: File too small or does not exist, aborting\n", __FUNCTION__);
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
		Print(L"%a: Unable to create image, aborting\n", __FUNCTION__);
		return EFI_OUT_OF_RESOURCES;
	}
	
	// Calculate line size and adjust with padding to multiple of 4 bytes.
	LineSizeBytes = BmpHeader->Width * 3; // 24 bits = 3 bytes
	LineSizeBytes += (LineSizeBytes % 4) != 0
		? (4 - (LineSizeBytes % 4))
		: 0;
	
	// Check if we have enough pixel data.
	if (BmpHeader->PixelDataOffset + BmpHeader->Height * LineSizeBytes > FileSizeBytes) {
		Print(L"%a: Not enough pixel data, aborting\n", __FUNCTION__);
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

	Print(L"%a: Done creating image size %u x %u from bmp\n", 
		__FUNCTION__, ((IMAGE *)*Result)->Width, ((IMAGE *)*Result)->Height);
	return EFI_SUCCESS;
}


VOID
ClearScreen()
{
    EFI_UGA_PIXEL	FillColor;
	
	FillColor.Red		= 0;
	FillColor.Green		= 0;
	FillColor.Blue		= 0;
	FillColor.Reserved	= 0;

	if (EFI_ERROR(EnsureDisplayAvailable())) {
		Print(L"%a: No graphics device found, unable to clear screen\n", __FUNCTION__);
		return;
	}

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
		Print(L"%a: No graphics device found, unable to draw image\n", __FUNCTION__);
		return;
	}

	if (Image == NULL || DrawWidth == 0 || DrawHeight == 0) {
		return;
	}
	if ((ScreenX + DrawWidth) > DisplayInfo.HorizontalResolution 
		|| (ScreenY + DrawHeight) > DisplayInfo.VerticalResolution) {
		Print(L"%a: Image too big to draw on screen\n", __FUNCTION__);
		return;
	}

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


EFI_STATUS
CalculatePositionForCenter(
	IN	UINTN	ImageWidth,
	IN	UINTN	ImageHeight,
	OUT	UINTN	*PositionX,
	OUT	UINTN	*PositionY)
{
	if (EFI_ERROR(EnsureDisplayAvailable())) {
		Print(L"%a: No graphics device found, unable to calculate position\n", __FUNCTION__);
		return EFI_DEVICE_ERROR;
	}

	if (ImageWidth == 0 || ImageHeight == 0 
		|| ImageWidth > DisplayInfo.HorizontalResolution
		|| ImageHeight > DisplayInfo.VerticalResolution) {
		Print(L"%a: Wrong image size (%ux%u) for this screen resolution (%ux%u)\n", 
			__FUNCTION__, ImageWidth, ImageHeight, 
			DisplayInfo.HorizontalResolution, DisplayInfo.VerticalResolution);
		return EFI_INVALID_PARAMETER;
	}

	*PositionX = (DisplayInfo.HorizontalResolution / 2) - (ImageWidth / 2);
	*PositionY = (DisplayInfo.VerticalResolution / 2) - (ImageHeight / 2);

	if (*PositionX + ImageWidth > DisplayInfo.HorizontalResolution)
		*PositionX = DisplayInfo.HorizontalResolution - ImageWidth;
	if (*PositionY + ImageHeight > DisplayInfo.VerticalResolution)
		*PositionY = DisplayInfo.VerticalResolution - ImageHeight;

	return EFI_SUCCESS;
}


VOID
DrawImageCentered(
	IN	IMAGE	*Image)
{
	EFI_STATUS	Status;
	UINTN		PositionX;
	UINTN		PositionY;

	if (EFI_ERROR(EnsureDisplayAvailable())) {
		Print(L"%a: No graphics device found, unable to draw centered image\n", __FUNCTION__);
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
	UINTN		MsPerFrame = 10;
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