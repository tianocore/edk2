#include "VgaShim.h"


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


IMAGE*
BmpFileToImage(
	IN	UINT8	*FileData,
	IN	UINTN	FileSizeBytes)
{
	IMAGE			*Image;
	BMP_HEADER		*BmpHeader;
	UINT8			*BmpCurrentPixel;
	UINT8			*BmpCurrentLine;
	UINTN			LineSizeBytes;
	EFI_UGA_PIXEL	*TargetPixel;
	UINTN			x, y;

	// Sanity checks.
	if (FileData == NULL || FileSizeBytes < sizeof(BMP_HEADER)) {
		Print(L"%a: File too small or does not exist, aborting\n", __FUNCTION__);
		return NULL;
	}

	BmpHeader = (BMP_HEADER *)FileData;
	if (BmpHeader->Signature[0] != 'B' 
		|| BmpHeader->Signature[1] != 'M'
		|| BmpHeader->CompressionType != 0	// only support uncompressed...
		|| BmpHeader->BitPerPixel != 24		// ...24 bits per pixel images
		|| BmpHeader->Width < 1
		|| BmpHeader->Height < 1) {
		return NULL;
	}
		
	Image = CreateImage(BmpHeader->Width, BmpHeader->Height);
	if (Image == NULL) {
		Print(L"%a: Unable to create image, aborting\n", __FUNCTION__);
		return NULL;
	}
	
	// Calculate line size and adjust with padding to multiple of 4 bytes.
	LineSizeBytes = BmpHeader->Width * 3; // 24 bits = 3 bytes
	LineSizeBytes += (LineSizeBytes % 4) != 0
		? (4 - (LineSizeBytes % 4))
		: 0;
	
	// Check if we have enough pixel data.
	if (BmpHeader->PixelDataOffset + BmpHeader->Height * LineSizeBytes > FileSizeBytes) {
		Print(L"%a: Not enough pixel data, aborting\n", __FUNCTION__);
		DestroyImage(Image);
		return NULL;
	}
		

	// Fill in pixel values.
	BmpCurrentLine = FileData + BmpHeader->PixelDataOffset;
	for (y = 0; y < BmpHeader->Height; y++) {
		BmpCurrentPixel = BmpCurrentLine;
		BmpCurrentLine += LineSizeBytes;
		// jump to the right pixel line; BMP PixelArray is bottom-to-top...
		TargetPixel = Image->PixelData + BmpHeader->Width * (BmpHeader->Height - y - 1);
		// ...but thankfully left-to-right
		for (x = 0; x < BmpHeader->Width; x++) {
			TargetPixel->Blue		= *BmpCurrentPixel++;
			TargetPixel->Green		= *BmpCurrentPixel++;
			TargetPixel->Red		= *BmpCurrentPixel++;
			TargetPixel->Reserved	= 0;
			TargetPixel++;
		}
	}

	Print(L"%a: Done creating image size %u x %u from bmp\n", __FUNCTION__, Image->Width, Image->Height);
	return Image;
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
	IN	UINTN	PosX,
	IN	UINTN	PosY)
{
	if (EFI_ERROR(EnsureDisplayAvailable())) {
		Print(L"%a: No graphics device found, unable to draw image\n", __FUNCTION__);
		return;
	}

	if (Image == NULL || Image->Width == 0 || Image->Height == 0) {
		Print(L"%a: No image specified\n", __FUNCTION__);
		return;
	}
	if ((PosX + Image->Width) > DisplayInfo.HorizontalResolution 
		|| (PosY + Image->Height) > DisplayInfo.VerticalResolution) {
		Print(L"%a: Image too big to draw on screen\n", __FUNCTION__);
		return;
	}

	if (DisplayInfo.Protocol == GOP) {
		DisplayInfo.GOP->Blt(DisplayInfo.GOP, 
			(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)Image->PixelData, 
			EfiBltBufferToVideo, 
			0, 0, PosX, PosY, 
			Image->Width, Image->Height, 0);
	} else if (DisplayInfo.Protocol == UGA) {
		DisplayInfo.UGA->Blt(DisplayInfo.UGA, 
			(EFI_UGA_PIXEL *)Image->PixelData, 
			EfiUgaBltBufferToVideo,
			0, 0, PosX, PosY, 
			Image->Width, Image->Height, 0);
	}
}


VOID
DrawImageCentered(
	IN	IMAGE	*Image)
{
	UINTN	PosX;
	UINTN	PosY;

	if (EFI_ERROR(EnsureDisplayAvailable())) {
		Print(L"%a: No graphics device found, unable to draw centered image\n", __FUNCTION__);
		return;
	}

	if (Image == NULL || Image->Width == 0 || Image->Height == 0) {
		Print(L"%a: No image specified\n", __FUNCTION__);
		return;
	}
	if ((Image->Width) > DisplayInfo.HorizontalResolution
		|| (Image->Height) > DisplayInfo.VerticalResolution) {
		Print(L"%a: Image too big to draw on screen\n", __FUNCTION__);
		return;
	}

	PosX = (DisplayInfo.HorizontalResolution / 2) - (Image->Width / 2);
	PosY = (DisplayInfo.VerticalResolution / 2) - (Image->Height / 2);

	if (PosX < 0)
		PosX = 0;
	if (PosY < 0)
		PosY = 0;
	if (PosX + Image->Width > DisplayInfo.HorizontalResolution)
		PosX = DisplayInfo.HorizontalResolution - Image->Width;
	if (PosY + Image->Height > DisplayInfo.VerticalResolution)
		PosY = DisplayInfo.VerticalResolution - Image->Height;

	DrawImage(Image, PosX, PosY);
}