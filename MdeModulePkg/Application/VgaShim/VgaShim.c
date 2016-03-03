#include "VgaShim.h"
#include "Display.h"
#include "Util.h"
#include "Filesystem.h"
#include "External/LegacyVgaBios.h"
#include "Int10hHandler.h"
#include "BootflagSimple.h"
#include "Version.h"


/**
  -----------------------------------------------------------------------------
  Global variables.
  -----------------------------------------------------------------------------
**/

DISPLAY_INFO					DisplayInfo;
EFI_HANDLE						VgaShimImage;
EFI_LOADED_IMAGE_PROTOCOL		*VgaShimImageInfo;
BOOLEAN							DebugMode = TRUE;
EFI_CONSOLE_CONTROL_PROTOCOL	*ConsoleControl;


/**
  The entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       VGA ROM shim has been installed successfully
                            or it was found not to be required.
  @retval other             Some error occured during execution.

**/
EFI_STATUS
EFIAPI
UefiMain (
	IN EFI_HANDLE		ImageHandle,
	IN EFI_SYSTEM_TABLE	*SystemTable)
{
	EFI_PHYSICAL_ADDRESS	Int10hHandlerAddress;
	IVT_ENTRY				*Int10hHandlerEntry;
	EFI_PHYSICAL_ADDRESS	TempAddress;
	EFI_STATUS				Status;
	EFI_INPUT_KEY			Key;
	CHAR16					*LaunchPath = NULL;

	//
	// Give user time to press 'v' to enter debug mode.
	//
	gBS->Stall(1000 * 500);

	//
	// Initialization.
	//
	VgaShimImage = ImageHandle;
	Status = gBS->LocateProtocol(&gEfiConsoleControlProtocolGuid, NULL, (VOID**)&ConsoleControl);
	if (EFI_ERROR(Status)) {
		ConsoleControl = NULL;
	}
	Status = gBS->HandleProtocol(VgaShimImage, &gEfiLoadedImageProtocolGuid, (VOID **)&VgaShimImageInfo);
	if (EFI_ERROR(Status)) {
		PrintError(L"Unable to locate EFI_LOADED_IMAGE_PROTOCOL, aborting\n");
		goto Exit;
	}

	//
	// Check if we should run in debug mode ('v' pressed).
	//
	Status = gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
	if (!EFI_ERROR(Status) && Key.UnicodeChar == L'v') {
		DebugMode = TRUE;
	}
	if (DebugMode) {
		PrintDebug(L"VGA Shim v%s\n", VERSION);
		PrintDebug(L"You are running in debug mode, press Enter to continue\n");
		WaitForEnter();
	}

	// 
	// Show pretty graphics.
	//
	if (!DebugMode && ConsoleControl != NULL) {
		if (!ShowAnimatedLogo()) {
			ShowStaticLogo();
		}
	}

	//
	// If an Int10h handler exists there either is a real
	// VGA ROM in operation or we installed the shim before.
	//
	if (IsInt10HandlerDefined()) {
		PrintDebug(L"Int10h already has a handler, you should be all set\n");
		goto Exit;
	}

	//
	// Sanity checks.
	//
	if (sizeof INT10H_HANDLER > VGA_ROM_SIZE) {
		PrintError(L"Shim size (%u) bigger than allowed (%u), aborting\n", sizeof INT10H_HANDLER, VGA_ROM_SIZE);
		goto Exit;
	}

	//
	// Unlock VGA ROM memory for writing first.
	//
	Status = EnsureMemoryLock(VGA_ROM_ADDRESS, VGA_ROM_SIZE, UNLOCK);
	if (EFI_ERROR(Status)) {
		PrintError(L"Unable to unlock VGA ROM memory at %04x, aborting\n", VGA_ROM_ADDRESS);
		goto Exit;
	}

	//
	// Claim real mode IVT memory area. This can be done as the IDT
	// has already been initialized so we can overwrite the IVT.
	//
	Int10hHandlerEntry = (IVT_ENTRY *)IVT_ADDRESS + 0x10;
	TempAddress = IVT_ADDRESS;
	Status = gBS->AllocatePages(AllocateAddress, EfiBootServicesCode, 1, &TempAddress);
	if (EFI_ERROR(Status)) {
		PrintError(L"Unable to claim IVT area at %04x (error: %r), aborting\n", IVT_ADDRESS, Status);
		goto Exit;
	} else {
		PrintDebug(L"IVT area at %04x claimed\n", IVT_ADDRESS);
	}

	//
	// Copy ROM stub in place and fill in the missing information.
	//
	SetMem((VOID *)VGA_ROM_ADDRESS, VGA_ROM_SIZE, 0);
	CopyMem((VOID *)VGA_ROM_ADDRESS, INT10H_HANDLER, sizeof INT10H_HANDLER);
	Status = ShimVesaInformation(VGA_ROM_ADDRESS, &Int10hHandlerAddress);
	if (EFI_ERROR(Status)) {
		PrintError(L"VESA information could not be filled in, aborting\n");
		goto Exit;
	} else {
		PrintDebug(L"VESA information filled in, Int10h handler address = %x\n", 
			Int10hHandlerAddress);
	}
	
	//
	// Lock the VGA ROM memory to prevent further writes.
	//
	Status = EnsureMemoryLock(VGA_ROM_ADDRESS, VGA_ROM_SIZE, LOCK);
	if (EFI_ERROR(Status)) {
		PrintDebug(L"Unable to lock VGA ROM memory at %x but this is not essential\n", 
			VGA_ROM_ADDRESS);
	}

	//
	// Point the Int10h vector at the entry point in shim.
	//
	// Convert from real 32bit physical address to real mode segment address
	//Int10hHandlerEntry = (IVT_ENTRY *)IVT_ADDRESS + 0x10;
	Int10hHandlerEntry->Segment = (UINT16)((UINT32)VGA_ROM_ADDRESS >> 4);
	Int10hHandlerEntry->Offset = (UINT16)(Int10hHandlerAddress - VGA_ROM_ADDRESS);
	PrintDebug(L"Int10h handler installed at %04x:%04x\n",
		Int10hHandlerEntry->Segment, Int10hHandlerEntry->Offset);

Exit:
	//
	// Check if we can chainload the Windows Boot Manager.
	//
	if (FileExists(L"\\efi\\microsoft\\boot\\bootmgfw.efi")) {
		LaunchPath = L"\\efi\\microsoft\\boot\\bootmgfw.efi";
		PrintDebug(L"Found Windows Boot Manager at '%s'\n", LaunchPath);
		PrintDebug(L"Press Enter to continue loading Windows\n");
		if (DebugMode)
			WaitForEnter();
	} else {
		PrintError(L"Could not find Windows Boot Manager, press Enter to exit\n");
		WaitForEnter();
	}

	if (LaunchPath != NULL) {
		Launch(LaunchPath);
	}

	return EFI_SUCCESS;
}


/**
  Fills in VESA-compatible information about supported video modes
  in the space left for this purpose at the beginning of the 
  generated VGA ROM assembly code.
  (See VESA BIOS EXTENSION Core Functions Standard v3.0, p26+.)

  @param[in] StartAddress Where to begin writing VESA information.
  @param[in] EndAddress   Pointer to the next byte after the end
                          of all video mode information data.

  @retval EFI_SUCCESS     The operation was successful
  @return other           The operation failed.

**/
EFI_STATUS
ShimVesaInformation(
	IN	EFI_PHYSICAL_ADDRESS	StartAddress,
	OUT	EFI_PHYSICAL_ADDRESS	*EndAddress)
{
	VBE_INFO				*VbeInfoFull;
	VBE_INFO_BASE			*VbeInfo;
	VBE_MODE_INFO			*VbeModeInfo;
	UINT8					*BufferPtr;
	UINT32					HorizontalOffsetPx;
	UINT32					VerticalOffsetPx;
	EFI_PHYSICAL_ADDRESS	FrameBufferBaseWithOffset;

	//
	// Get basic video hardware information first.
	//
	if (EFI_ERROR(EnsureDisplayAvailable())) {
		PrintError(L"No adapters were found, unable to fill in VESA information\n");
		return EFI_NOT_FOUND;
	}
	
	//
	// VESA general information.
	//
	VbeInfoFull = (VBE_INFO *)(UINTN)StartAddress;
	VbeInfo = &VbeInfoFull->Base;
	BufferPtr = VbeInfoFull->Buffer;
	CopyMem(VbeInfo->Signature, "VESA", 4);
	VbeInfo->VesaVersion = 0x0300;
	VbeInfo->OemNameAddress = (UINT32)StartAddress << 12 | (UINT16)(UINTN)BufferPtr;
	CopyMem(BufferPtr, VENDOR_NAME, sizeof VENDOR_NAME);
	BufferPtr += sizeof VENDOR_NAME;
	VbeInfo->Capabilities = BIT0;			// DAC width supports 8-bit color mode
	VbeInfo->ModeListAddress = (UINT32)StartAddress << 12 | (UINT16)(UINTN)BufferPtr;
	*(UINT16*)BufferPtr = 0x00f1;			// mode number
	BufferPtr += 2;
	*(UINT16*)BufferPtr = 0xFFFF;			// mode list terminator
	BufferPtr += 2;
	VbeInfo->VideoMem64K = (UINT16)((DisplayInfo.FrameBufferSize + 65535) / 65536);
	VbeInfo->OemSoftwareVersion = 0x0000;
	VbeInfo->VendorNameAddress = (UINT32)StartAddress << 12 | (UINT16)(UINTN)BufferPtr;
	CopyMem(BufferPtr, VENDOR_NAME, sizeof VENDOR_NAME);
	BufferPtr += sizeof VENDOR_NAME;
	VbeInfo->ProductNameAddress = (UINT32)StartAddress << 12 | (UINT16)(UINTN)BufferPtr;
	CopyMem(BufferPtr, PRODUCT_NAME, sizeof PRODUCT_NAME);
	BufferPtr += sizeof PRODUCT_NAME;
	VbeInfo->ProductRevAddress = (UINT32)StartAddress << 12 | (UINT16)(UINTN)BufferPtr;
	CopyMem(BufferPtr, PRODUCT_REVISION, sizeof PRODUCT_REVISION);
	BufferPtr += sizeof PRODUCT_REVISION;
	
	//
	// Basic VESA mode information.
	//
	VbeModeInfo = (VBE_MODE_INFO *)(VbeInfoFull + 1); // jump ahead by sizeof(VBE_INFO) ie. 256 bytes
	// bit0: mode supported by present hardware configuration
	// bit1: must be set for VBE v1.2+
	// bit3: color mode
	// bit4: graphics mode
	// bit5: mode not VGA-compatible (do not access VGA I/O ports and registers)
	// bit6: disable windowed memory mode = linear framebuffer only
	// bit7: linear framebuffer supported
	VbeModeInfo->ModeAttr = BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT1 | BIT0;
	
	//
	// Resolution.
	//
	VbeModeInfo->Width = 1024;						// as expected by Windows installer
	VbeModeInfo->Height = 768;						// as expected by Windows installer
	VbeModeInfo->CharCellWidth = 8;					// used to calculate resolution in text modes
	VbeModeInfo->CharCellHeight = 16;				// used to calculate resolution in text modes
	
	//
	// Center visible image on screen using framebuffer offset.
	//
	HorizontalOffsetPx = (DisplayInfo.HorizontalResolution - 1024) / 2;
	VerticalOffsetPx = (DisplayInfo.VerticalResolution - 768) / 2 * DisplayInfo.PixelsPerScanLine;
	FrameBufferBaseWithOffset = DisplayInfo.FrameBufferBase 
		+ VerticalOffsetPx * 4		// 4 bytes per pixel
		+ HorizontalOffsetPx * 4;	// 4 bytes per pixel

	//
	// Memory access (banking, windowing, paging).
	//
	VbeModeInfo->NumBanks = 1;						// disable memory banking
	VbeModeInfo->BankSizeKB = 0;					// disable memory banking
	VbeModeInfo->LfbAddress = 
		(UINT32)FrameBufferBaseWithOffset;			// 32-bit physical address
	VbeModeInfo->BytesPerScanLineLinear = 
		DisplayInfo.PixelsPerScanLine * 4;			// logical bytes in linear modes
	VbeModeInfo->NumImagePagesLessOne = 0;			// disable image paging
	VbeModeInfo->NumImagesLessOneLinear = 0;		// disable image paging
	VbeModeInfo->WindowPositioningAddress = 0x0;	// force windowing to Function 5h
	VbeModeInfo->WindowAAttr = 0x0;					// window disabled
	VbeModeInfo->WindowBAttr = 0x0;					// window disabled
	VbeModeInfo->WindowGranularityKB = 0x0;			// window disabled ie. not relocatable
	VbeModeInfo->WindowSizeKB = 0x0;				// window disabled
	VbeModeInfo->WindowAStartSegment = 0x0;			// linear framebuffer only
	VbeModeInfo->WindowBStartSegment = 0x0;			// linear framebuffer only

	//
	// Color mode.
	// 
	VbeModeInfo->NumPlanes = 1;						// packed pixel mode
	VbeModeInfo->MemoryModel = 6;					// Direct Color
	VbeModeInfo->DirectColorModeInfo = BIT1;		// alpha bytes may be used by application
	VbeModeInfo->BitsPerPixel = 32;					// 8+8+8+8 bits per channel
	VbeModeInfo->BlueMaskSizeLinear = 8;
	VbeModeInfo->GreenMaskSizeLinear = 8;
	VbeModeInfo->RedMaskSizeLinear = 8;
	VbeModeInfo->ReservedMaskSizeLinear = 8;

	if (DisplayInfo.PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
		VbeModeInfo->BlueMaskPosLinear = 0;			// blue offset
		VbeModeInfo->GreenMaskPosLinear = 8;		// green offset
		VbeModeInfo->RedMaskPosLinear = 16;			// red offset
		VbeModeInfo->ReservedMaskPosLinear = 24;	// reserved offset
	} else if (DisplayInfo.PixelFormat == PixelRedGreenBlueReserved8BitPerColor) {
		VbeModeInfo->RedMaskPosLinear = 0;			// red offset
		VbeModeInfo->GreenMaskPosLinear = 8;		// green offset
		VbeModeInfo->BlueMaskPosLinear = 16;		// blue offset
		VbeModeInfo->ReservedMaskPosLinear = 24;	// alpha offset
	} else {
		PrintError(L"Unsupported value of PixelFormat (%d), aborting\n", DisplayInfo.PixelFormat);
		return EFI_UNSUPPORTED;
	}
	
	//
	// Other.
	//
	VbeModeInfo->OffScreenAddress = 0;				// reserved, always set to 0
	VbeModeInfo->OffScreenSizeKB = 0;				// reserved, always set to 0
	VbeModeInfo->MaxPixelClockHz = 0;				// maximum available refresh rate
	VbeModeInfo->Vbe3 = 0x01;						// reserved, always set to 1

	*EndAddress = (UINTN)(VbeModeInfo + 1);			// jump ahead by sizeof(VBE_MODE_INFO) ie. 256 bytes
	return EFI_SUCCESS;
}


/**
  Checkes if an Int10h handler is already defined in the
  Interrupt Vector Table (IVT).

  @retval TRUE            An Int10h handler was found in IVT.
  @retval FALSE           An Int10h handler was not found in IVT.

**/
BOOLEAN
IsInt10HandlerDefined()
{
	IN	IVT_ENTRY				*Int10Entry;
	IN	EFI_PHYSICAL_ADDRESS	Int10Handler;
	
	// (convert from real mode segment address to 32bit physical address)
	Int10Entry = (IVT_ENTRY *)(UINTN)IVT_ADDRESS + 0x10;
	Int10Handler = (Int10Entry->Segment << 4) + Int10Entry->Offset;

	if (Int10Handler >= VGA_ROM_ADDRESS && Int10Handler < (VGA_ROM_ADDRESS+VGA_ROM_SIZE)) {
		PrintDebug(L"Int10h handler found at %04x:%04x\n", Int10Entry->Segment, Int10Entry->Offset);
		return TRUE;
	} else {
		PrintDebug(L"No existing Int10h handler found\n");
		return FALSE;
	}
}


/**
  Attempts to either unlock a memory area for writing or
  lock it to prevent writes. Makes use of a number of approaches
  to achieve the desired result.

  @param[in] StartAddress Where the desired memory area begins.
  @param[in] Length       Number of bytes from StartAddress that
                          need to be locked or unlocked.
  @param[in] Operation    Whether the area is to be locked or unlocked. 

  @retval TRUE            An Int10h handler was found in IVT.
  @retval FALSE           An Int10h handler was not found in IVT.

**/
EFI_STATUS
EnsureMemoryLock(
	IN	EFI_PHYSICAL_ADDRESS	StartAddress,
	IN	UINT32					Length,
	IN	MEMORY_LOCK_OPERATION	Operation)
{
	EFI_STATUS					Status = EFI_NOT_READY;
	UINT32						Granularity;
	EFI_LEGACY_REGION_PROTOCOL	*mLegacyRegion = NULL;
	EFI_LEGACY_REGION2_PROTOCOL	*mLegacyRegion2 = NULL;

	//
	// Check if we need to perform any operation.
	// 
	if (Operation == UNLOCK && CanWriteAtAddress(StartAddress)) {
		PrintDebug(L"Memory at %x already unlocked\n", StartAddress);
		Status = EFI_SUCCESS;
	} else if (Operation == LOCK && !CanWriteAtAddress(StartAddress)) {
		PrintDebug(L"Memory at %x already locked\n", StartAddress);
		Status = EFI_SUCCESS;
	}

	//
	// Try to lock/unlock with EfiLegacyRegionProtocol.
	// 
	if (EFI_ERROR(Status)) {
		Status = gBS->LocateProtocol(&gEfiLegacyRegionProtocolGuid, NULL, (VOID **)&mLegacyRegion);
		if (!EFI_ERROR(Status)) {
			if (Operation == UNLOCK) {
				Status = mLegacyRegion->UnLock(mLegacyRegion, (UINT32)StartAddress, Length, &Granularity);
				Status = CanWriteAtAddress(StartAddress) ? EFI_SUCCESS : EFI_DEVICE_ERROR;
			} else {
				Status = mLegacyRegion->Lock(mLegacyRegion, (UINT32)StartAddress, Length, &Granularity);
				Status = CanWriteAtAddress(StartAddress) ? EFI_DEVICE_ERROR : EFI_SUCCESS;
			}

			PrintDebug(L"%s %s memory at %x with EfiLegacyRegionProtocol\n", 
				EFI_ERROR(Status) ? L"Failure" : L"Success",
				Operation == UNLOCK ? L"unlocking" : L"locking", 
				StartAddress);
		}
	}
	
	//
	// Try to lock/unlock with EfiLegacyRegion2Protocol.
	//
	if (EFI_ERROR(Status)) {
		Status = gBS->LocateProtocol(&gEfiLegacyRegion2ProtocolGuid, NULL, (VOID **)&mLegacyRegion2);
		if (!EFI_ERROR(Status)) {
			if (Operation == UNLOCK) {
				Status = mLegacyRegion2->UnLock(mLegacyRegion2, (UINT32)StartAddress, Length, &Granularity);
				Status = CanWriteAtAddress(StartAddress) ? EFI_SUCCESS : EFI_DEVICE_ERROR;;
			} else {
				Status = mLegacyRegion2->Lock(mLegacyRegion2, (UINT32)StartAddress, Length, &Granularity);
				Status = CanWriteAtAddress(StartAddress) ? EFI_DEVICE_ERROR : EFI_SUCCESS;
			}

			PrintDebug(L"%s %s memory at %x with EfiLegacyRegion2Protocol\n", 
				EFI_ERROR(Status) ? L"Failure" : L"Success",
				Operation == UNLOCK ? L"unlocking" : L"locking", 
				StartAddress);
		}
	}
	
	//
	// Try to lock/unlock via an MTRR.
	//
	if (EFI_ERROR(Status) && IsMtrrSupported() && FIXED_MTRR_SIZE >= Length) {
		if (Operation == UNLOCK) {
			MtrrSetMemoryAttribute(StartAddress, FIXED_MTRR_SIZE, CacheUncacheable);
			Status = CanWriteAtAddress(StartAddress) ? EFI_SUCCESS : EFI_DEVICE_ERROR;
		} else {
			MtrrSetMemoryAttribute(StartAddress, FIXED_MTRR_SIZE, CacheWriteProtected);
			Status = CanWriteAtAddress(StartAddress) ? EFI_DEVICE_ERROR : EFI_SUCCESS;
		}

		PrintDebug(L"%s %s memory at %x with MTRRs\n", 
			EFI_ERROR(Status) ? "Failure" : "Success",
			Operation == UNLOCK ? "unlocking" : "locking", 
			StartAddress);
	}
	
	//
	// None of the methods worked?
	// 
	if (EFI_ERROR(Status)) {
		PrintError(L"Unable to find a way to %s memory at %x\n", 
			Operation == UNLOCK ? "unlock" : "lock", StartAddress);
	}
	
	return Status;
}


/**
  Checks if writes are possible in a particular memory area.

  @param[in] Address      The memory location to be checked.

  @retval TRUE            Writes to the specified location are
                          allowed changes are persisted.
  @retval FALSE           Writes to the specified location are
                          not allowed or have no effect.
  
**/
BOOLEAN
CanWriteAtAddress(
	IN	EFI_PHYSICAL_ADDRESS	Address)
{
	BOOLEAN	CanWrite;
	UINT8	*TestPtr;
	UINT8	OldValue;

	TestPtr = (UINT8*)(Address);
	OldValue = *TestPtr;
	
	*TestPtr = *TestPtr + 1;
	CanWrite = OldValue != *TestPtr;
	
	*TestPtr = OldValue;
	return CanWrite;
}


/**
  Displays a static built-in Windows flag (last frame in
  animation shown when starting Windows 7).

  @param[in] ConsoleControl Protocol for sharing the display
                            between console and graphical output.
							Considered legacy but still used
							by Apple devices.

  @retval TRUE              Static logo was successfully retrieved
                            and displayed on screen.
  @retval FALSE             Either the required resource was not found
                            or was unable to switch to graphical output.
  
**/
BOOLEAN
ShowStaticLogo()
{
	EFI_STATUS	Status;
	IMAGE		*WindowsFlag;

	// Sanity checks.
	Status = BmpFileToImage(BootflagSimple, sizeof BootflagSimple, (VOID **)&WindowsFlag);
	if (EFI_ERROR(Status)) {
		return FALSE;
	}
	
	// All fine, let's do some drawing.
	ConsoleControl->SetMode(ConsoleControl, EfiConsoleControlScreenGraphics);
	ClearScreen();
	DrawImageCentered(WindowsFlag);

	// Cleanup & return.
	DestroyImage(WindowsFlag);
	return TRUE;
}


/**
  Displays an animated logo. It has to be stored in a .bmp file
  whose filename (sans extension) has to match the runtime filename
  of VgaShim. It must also reside in the same folder as VgaShim.
  The image will be split into rectangular frames whose side
  is assumed to be equal to the shorter side of the image.

  Eg. if you run VgaShim.efi and have VgaShim.bmp in the same
  folder, and VgaShim.bmp is a valid, 24bpp bmp image file of
  of size 200x10000, 50 frames will be shown (top to bottom).

  @retval TRUE              Static logo was successfully retrieved
                            and displayed on screen.
  @retval FALSE             Either the required resource was not found
                            or was unable to switch to graphical output.
  
**/
BOOLEAN
ShowAnimatedLogo()
{
	EFI_STATUS	Status;
	CHAR16		*BmpFilePath;
	UINT8		*BmpFileContents;
	UINTN		BmpFileBytes;
	IMAGE		*WindowsFlag;
	
	// Check if <MyName>.bmp exists
	Status = ChangeExtension(
		PathCleanUpDirectories(ConvertDevicePathToText(VgaShimImageInfo->FilePath, FALSE, FALSE)), 
		L"bmp", 
		(VOID **)&BmpFilePath);
	if (EFI_ERROR(Status) || !FileExists(BmpFilePath)) {
		return FALSE;
	}

	// Read file contents.
	Status = FileRead(BmpFilePath, (VOID **)&BmpFileContents, &BmpFileBytes);
	if (EFI_ERROR(Status)) {
		FreePool(BmpFilePath);
		return FALSE;
	}
	Status = BmpFileToImage(BmpFileContents, BmpFileBytes, (VOID **)&WindowsFlag);
	if (EFI_ERROR(Status)) {
		FreePool(BmpFilePath);
		FreePool(BmpFileContents);
		return FALSE;
	}

	FreePool(BmpFilePath);
	FreePool(BmpFileContents);
	BmpFileContents = NULL;

	// All fine, let's do some drawing.
	ConsoleControl->SetMode(ConsoleControl, EfiConsoleControlScreenGraphics);
	ClearScreen();
	AnimateImage(WindowsFlag);

	// Cleanup & return.
	DestroyImage(WindowsFlag);
	return TRUE;
}


VOID
EFIAPI
PrintFuncNameMessage(
	IN CONST	BOOLEAN	IsError,
	IN CONST	CHAR8	*FuncName,
	IN CONST	CHAR16	*FormatString,
	...)
{
	VA_LIST							Marker;
	CHAR16							*Buffer;
	UINTN							BufferSize;
	EFI_STATUS						Status;
	EFI_CONSOLE_CONTROL_SCREEN_MODE	CurrentMode;

	if (!(IsError || DebugMode)) {
		return;
	}

	//
	// Switch to text mode if needed and possible.
	//
	if (IsError && ConsoleControl != NULL) {
		Status = ConsoleControl->GetMode(ConsoleControl, &CurrentMode, NULL, NULL);
		if (!EFI_ERROR(Status) && CurrentMode != EfiConsoleControlScreenText) {
			ConsoleControl->SetMode(ConsoleControl, EfiConsoleControlScreenText);
		}
	}
	
	//
	// Generate the main message.
	//
	BufferSize = DEBUG_MESSAGE_LENGTH * sizeof(CHAR16);
	Buffer = (CHAR16 *)AllocatePool(BufferSize);
	VA_START(Marker, FormatString);
	UnicodeVSPrint(Buffer, BufferSize, FormatString, Marker);
	VA_END(Marker);

	//
	// Output using apropriate colors.
	//
	gST->ConOut->SetAttribute(gST->ConOut, EFI_DARKGRAY);
	AsciiPrint("%.10a ", FuncName);
	gST->ConOut->SetAttribute(gST->ConOut, IsError ? EFI_YELLOW : EFI_LIGHTGRAY);
	if ((gST != NULL) && (gST->ConOut != NULL)) {
		gST->ConOut->OutputString(gST->ConOut, Buffer);
	}

	//
	// Cleanup.
	//
	gST->ConOut->SetAttribute(gST->ConOut, EFI_LIGHTGRAY);
	FreePool(Buffer);
}


VOID
WaitForEnter()
{
	EFI_INPUT_KEY	Key;
	UINTN			EventIndex;

	gST->ConIn->Reset(gST->ConIn, FALSE);
	do {
		gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);
		gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
	} while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);	
}