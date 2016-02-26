/** @file
 VgaShim Code

**/

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/MtrrLib.h>
#include <Library/PciLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Protocol/LegacyRegion.h>
#include <Protocol/LegacyRegion2.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/GraphicsOutput.h>

#include "LegacyVgaBios.h"
#include "VgaShim.h"

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

BOOLEAN CanWriteAtAddress(EFI_PHYSICAL_ADDRESS address);
EFI_STATUS EnsureMemoryLock(EFI_PHYSICAL_ADDRESS Address, UINT32 Length, MEMORY_LOCK_OPERATION Operation);
BOOLEAN IsInt10HandlerDefined();
EFI_STATUS FillVesaInformation(IN EFI_PHYSICAL_ADDRESS StartAddress, OUT EFI_PHYSICAL_ADDRESS *EndAddress);
VOID ShowVideoInfo();
EFI_STATUS GetVideoInfo(VIDEO_INFO *Info);

// ensure correct alignment
#pragma pack (1)
typedef struct {
  UINT16 Offset;
  UINT16 Segment;
} IVT_ENTRY;
#pragma pack ()

STATIC CONST CHAR8 VENDOR_NAME[] = "OVMF";
STATIC CONST CHAR8 PRODUCT_NAME[] = "DummyCard";
STATIC CONST CHAR8 PRODUCT_REVISION[] = "OVMF Int10h (fake)";
STATIC CONST EFI_PHYSICAL_ADDRESS VROM_ADDRESS = 0xc0000;
STATIC CONST EFI_PHYSICAL_ADDRESS IVT_ADDRESS = 0x00000;
STATIC CONST UINTN VROM_SIZE = 0x10000;
STATIC CONST UINTN FIXED_MTRR_SIZE = 0x20000;


EFI_STATUS
EFIAPI
UefiMain (
	IN EFI_HANDLE		ImageHandle,
	IN EFI_SYSTEM_TABLE	*SystemTable)
{
	EFI_PHYSICAL_ADDRESS	Int10HandlerAddress;
	EFI_PHYSICAL_ADDRESS	TempAddress;
	IVT_ENTRY				*Int0x10;
	EFI_STATUS				Status;
	VBE_MODE_INFO			*VbeModeInfo;
	
	Print(L"VGA Shim v0.5\n");
	ShowVideoInfo();

	//
	// If an Int10h handler exists there either is a real
	// VGA ROM in operation or we installed the shim before.
	//
	if (IsInt10HandlerDefined()) {
		Print(L"%a: Int10h already has a handler, you should be all set\n", __FUNCTION__);
		goto Exit;
	}

	//
	// Sanity checks.
	//
	ASSERT(sizeof mVgaShim <= VROM_SIZE);

	//
	// Unlock VGA ROM memory for writing first.
	//
	Status = EnsureMemoryLock(VROM_ADDRESS, VROM_SIZE, MEM_UNLOCK);
	if (EFI_ERROR(Status)) {
		Print(L"%a: Unable to unlock VGA ROM memory at %x for shim insertion\n", 
			__FUNCTION__, VROM_ADDRESS);
		goto Exit;
	}

	//
	// Claim real mode IVT memory area. This can be done as the IDT
	// has already been initialized so we can overwrite the IVT.
	//
	Int0x10 = (IVT_ENTRY *)(UINTN)IVT_ADDRESS + 0x10;
	Print(L"%a: Claiming IVT area ... ", __FUNCTION__);
	TempAddress = IVT_ADDRESS;
	Status = gBS->AllocatePages(AllocateAddress, EfiBootServicesCode, 1, &TempAddress);
	if (EFI_ERROR(Status)) {
		Print(L"failure: %r\n", Status);
		return EFI_ABORTED;
	} else {
		Print(L"success\n");
		ASSERT(Int0x10->Segment == IVT_ADDRESS);
		ASSERT(Int0x10->Offset == 0x0);
	}

	//
	// Copy ROM stub in place and fill in the missing information.
	//
	SetMem((VOID *)(UINTN)VROM_ADDRESS, VROM_SIZE, 0);
	CopyMem((VOID *)(UINTN)VROM_ADDRESS, mVgaShim, sizeof mVgaShim);
	Status = FillVesaInformation(VROM_ADDRESS, &Int10HandlerAddress);
	if (EFI_ERROR(Status)) {
		Print(L"%a: Cannot complete shim installation, aborting\n", __FUNCTION__);
		return EFI_ABORTED;
	} else {
		Print(L"%a: VESA information filled in, Int10h handler address = %x\n", 
			__FUNCTION__, Int10HandlerAddress);
	}
	
	//
	// Lock the VGA ROM memory to prevent further writes.
	//
	Status = EnsureMemoryLock(VROM_ADDRESS, VROM_SIZE, MEM_LOCK);
	if (EFI_ERROR(Status)) {
		Print(L"%a: Unable to lock VGA ROM memory at %x but this is not essential\n", 
			__FUNCTION__, VROM_ADDRESS);
	}

	//
	// Point the Int10h vector at the entry point in shim.
	//
	// convert from real 32bit physical address to real mode segment address
	Int0x10->Segment = (UINT16)((UINT32)VROM_ADDRESS >> 4);
	Int0x10->Offset = (UINT16)(Int10HandlerAddress - VROM_ADDRESS);
	Print(L"%a: Int10h handler installed at %04x:%04x\n",
		__FUNCTION__, Int0x10->Segment, Int0x10->Offset);

Exit:
	Print(L"%a: Done!\n", __FUNCTION__);
	return EFI_SUCCESS;
}


VOID
ShowVideoInfo()
{
	VIDEO_INFO	Info;

	if (EFI_ERROR(GetVideoInfo(&Info))) {
		return;
	}

	Print(L"%a: HorizontalResolution = %u\n", __FUNCTION__, Info.HorizontalResolution);
	Print(L"%a: VerticalResolution = %u\n", __FUNCTION__, Info.VerticalResolution);
	Print(L"%a: PixelFormat = %u\n", __FUNCTION__, Info.PixelFormat);
	Print(L"%a: PixelsPerScanLine = %u\n", __FUNCTION__, Info.PixelsPerScanLine);
	Print(L"%a: FrameBufferBase = %x\n", __FUNCTION__, Info.FrameBufferBase);
	Print(L"%a: FrameBufferSize = %u\n", __FUNCTION__, Info.FrameBufferSize);
}


EFI_STATUS
GetVideoInfo(
	VIDEO_INFO	*Info)
{
	EFI_STATUS						Status;
	EFI_UGA_DRAW_PROTOCOL			*UgaDraw;
	EFI_GRAPHICS_OUTPUT_PROTOCOL	*GraphicsOutput;

	SetMem(Info, sizeof Info, 0);

	//
	// Try a GOP adapter first.
	//
	Status = gBS->HandleProtocol(gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID **)&GraphicsOutput);
	if (!EFI_ERROR(Status)) {
		Print(L"%a: Found a GOP protocol provider\n", __FUNCTION__);
		Info->HorizontalResolution = GraphicsOutput->Mode->Info->HorizontalResolution;
		Info->VerticalResolution = GraphicsOutput->Mode->Info->VerticalResolution;
		Info->PixelFormat = GraphicsOutput->Mode->Info->PixelFormat;
		Info->PixelsPerScanLine = GraphicsOutput->Mode->Info->PixelsPerScanLine;
		Info->FrameBufferBase = GraphicsOutput->Mode->FrameBufferBase;
		// usually = PixelsPerScanLine * VerticalResolution * BytesPerPixel
		// for MacBookAir7,2: 1536 * 900 * 4 = 5,529,600 bytes
		Info->FrameBufferSize = GraphicsOutput->Mode->FrameBufferSize;

		return EFI_SUCCESS;
	}

	//
	// Try a UGA adapter.
	//
	Status = gBS->HandleProtocol(gST->ConsoleOutHandle, &gEfiUgaDrawProtocolGuid, (VOID **)&UgaDraw);
	if (!EFI_ERROR(Status)) {
		UINT32 Temp1;
		UINT32 Temp2;
		Print(L"%a: Found a UGA protocol provider\n", __FUNCTION__);
		
		Status = UgaDraw->GetMode(UgaDraw, 
			&Info->HorizontalResolution, &Info->VerticalResolution, &Temp1, &Temp2);
		if (EFI_ERROR(Status)) {
			Print(L"%a: Unable to get current UGA mode\n", __FUNCTION__);
			return EFI_DEVICE_ERROR;
		}

		Info->PixelFormat = PixelBlueGreenRedReserved8BitPerColor; // default for UGA
		
		// TODO: find framebuffer base
		// TODO: find scanline length
		// https://github.com/coreos/grub/blob/master/grub-core%2Fvideo%2Fefi_uga.c

		return EFI_SUCCESS;
	}

	Print(L"%a: No GOP or UGA adapters found\n", __FUNCTION__);
	return EFI_NOT_FOUND;
}


EFI_STATUS
FillVesaInformation(
	IN	EFI_PHYSICAL_ADDRESS	StartAddress,
	OUT	EFI_PHYSICAL_ADDRESS	*EndAddress)
{
	// (Page 26 in VESA BIOS EXTENSION Core Functions Standard v3.0.)
	// (Page 30 in VESA BIOS EXTENSION Core Functions Standard v3.0.)

	EFI_STATUS				Status;
	VBE_INFO				*VbeInfoFull;
	VBE_INFO_BASE			*VbeInfo;
	VBE_MODE_INFO			*VbeModeInfo;
	UINT8					*BufferPtr;
	VIDEO_INFO				VideoInfo;
	UINT32					HorizontalOffsetPx;
	UINT32					VerticalOffsetPx;
	EFI_PHYSICAL_ADDRESS	FrameBufferBaseWithOffset;

	//
	// Get basic video hardware information first.
	//
	Status = GetVideoInfo(&VideoInfo);
	if (EFI_ERROR(Status)) {
		Print(L"%a: Unable to fill in VESA information as no adapters were found\n", __FUNCTION__);
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
	VbeInfo->VideoMem64K = (UINT16)((VideoInfo.FrameBufferSize + 65535) / 65536);
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
	
	// make sure we did not use more buffer than we had space for
	ASSERT(sizeof VbeInfoFull->Buffer >= BufferPtr - VbeInfoFull->Buffer);
	
	//
	// Basic VESA mode information
	//
	VbeModeInfo = (VBE_MODE_INFO *)(VbeInfoFull + 1); // jump ahead by sizeof VBE_INFO ie. 256
	// bit0: mode supported by present hardware configuration
	// bit1: must be set for VBE v1.2+
	// bit3: color mode
	// bit4: graphics mode
	// bit5: mode not VGA-compatible (do not access VGA I/O ports and registers)
	// bit6: disable windowed memory mode = linear framebuffer only
	// bit7: linear framebuffer supported
	VbeModeInfo->ModeAttr = BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT1 | BIT0;
	
	//
	// Resolution
	//
	VbeModeInfo->Width = 1024;						// as expected by Windows installer
	VbeModeInfo->Height = 768;						// as expected by Windows installer
	VbeModeInfo->CharCellWidth = 8;					// used to calculate resolution in text modes
	VbeModeInfo->CharCellHeight = 16;				// used to calculate resolution in text modes
	
	// Calculate offsets so that the smaller image appears centered on the screen
	HorizontalOffsetPx = (VideoInfo.HorizontalResolution - 1024) / 2;
	VerticalOffsetPx = (VideoInfo.VerticalResolution - 768) / 2 * VideoInfo.PixelsPerScanLine;
	FrameBufferBaseWithOffset = VideoInfo.FrameBufferBase 
		+ VerticalOffsetPx * 4
		+ HorizontalOffsetPx * 4;

	//
	// Memory access (banking, windowing, paging)
	//
	VbeModeInfo->NumBanks = 1;						// disable memory banking
	VbeModeInfo->BankSizeKB = 0;					// disable memory banking
	VbeModeInfo->LfbAddress = 
		(UINT32)FrameBufferBaseWithOffset;			// 32-bit physical address
	VbeModeInfo->BytesPerScanLineLinear = 
		VideoInfo.PixelsPerScanLine * 4;			// logical bytes in linear modes
	VbeModeInfo->NumImagePagesLessOne = 0;			// disable image paging
	VbeModeInfo->NumImagesLessOneLinear = 0;		// disable image paging
	VbeModeInfo->WindowPositioningAddress = 0x0;	// force windowing to Function 5h
	VbeModeInfo->WindowAAttr = 0x0;					// window disabled
	VbeModeInfo->WindowBAttr = 0x0;					// window disabled
	VbeModeInfo->WindowGranularityKB = 0x0;			// window disabled and not relocatable
	VbeModeInfo->WindowSizeKB = 0x0;				// set to 64KB even thou window is disabled
	VbeModeInfo->WindowAStartSegment = 0x0;			// linear framebuffer only
	VbeModeInfo->WindowBStartSegment = 0x0;			// linear framebuffer only

	//
	// Color mode
	// 
	VbeModeInfo->NumPlanes = 1;						// packed pixel mode
	VbeModeInfo->MemoryModel = 6;					// Direct Color
	VbeModeInfo->DirectColorModeInfo = BIT1;		// alpha bytes may be used by application
	VbeModeInfo->BitsPerPixel = 32;					// 8+8+8+8 bits per channel
	VbeModeInfo->BlueMaskSizeLinear = 8;
	VbeModeInfo->GreenMaskSizeLinear = 8;
	VbeModeInfo->RedMaskSizeLinear = 8;
	VbeModeInfo->ReservedMaskSizeLinear = 8;

	if (VideoInfo.PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
		VbeModeInfo->BlueMaskPosLinear = 0;			// blue offset
		VbeModeInfo->GreenMaskPosLinear = 8;		// green offset
		VbeModeInfo->RedMaskPosLinear = 16;			// green offset
		VbeModeInfo->ReservedMaskPosLinear = 24;	// reserved offset
	} else if (VideoInfo.PixelFormat == PixelRedGreenBlueReserved8BitPerColor) {
		VbeModeInfo->RedMaskPosLinear = 0;			// red offset
		VbeModeInfo->GreenMaskPosLinear = 8;		// green offset
		VbeModeInfo->BlueMaskPosLinear = 16;		// blue offset
		VbeModeInfo->ReservedMaskPosLinear = 24;	// alpha offset
	} else {
		Print(L"%a: Unsupported value of PixelFormat (%d), aborting\n", 
			__FUNCTION__, VideoInfo.PixelFormat);
		return EFI_UNSUPPORTED;
	}
	
	//
	// Other
	//
	VbeModeInfo->OffScreenAddress = 0;				// reserved, always set to 0
	VbeModeInfo->OffScreenSizeKB = 0;				// reserved, always set to 0
	VbeModeInfo->MaxPixelClockHz = 0;				// maximum available refresh rate
	VbeModeInfo->Vbe3 = 0x01;						// reserved, always set to 1

	*EndAddress = (UINTN)(VbeModeInfo + 1);			// jump ahead by sizeof VBE_MODE_INFO ie. 256
	return EFI_SUCCESS;
}


BOOLEAN
IsInt10HandlerDefined()
{
	IVT_ENTRY				*Int10Entry;
	EFI_PHYSICAL_ADDRESS	Int10Handler;
	
	// convert from real mode segment address to 32bit physical address
	Int10Entry = (IVT_ENTRY *)(UINTN)IVT_ADDRESS + 0x10;
	Int10Handler = (Int10Entry->Segment << 4) + Int10Entry->Offset;

	Print(L"%a: Checking for an existing Int10h handler ... ", __FUNCTION__);

	if (Int10Handler >= VROM_ADDRESS && Int10Handler < (VROM_ADDRESS+VROM_SIZE)) {
		Print(L"found at %04x:%04x\n", Int10Entry->Segment, Int10Entry->Offset);
		return TRUE;
	} else {
		Print(L"not found\n");
		return FALSE;
	}
}


EFI_STATUS
EnsureMemoryLock(
	EFI_PHYSICAL_ADDRESS	Address,
	UINT32					Length,
	MEMORY_LOCK_OPERATION	Operation)
{
	EFI_STATUS					Status = EFI_NOT_READY;
	UINT32						Granularity;
	EFI_LEGACY_REGION_PROTOCOL	*mLegacyRegion = NULL;
	EFI_LEGACY_REGION2_PROTOCOL	*mLegacyRegion2 = NULL;

	//
	// Check if we need to perform any operation
	// 
	if (Operation == MEM_UNLOCK && CanWriteAtAddress(Address)) {
		Print(L"%a: Memory at %x already unlocked\n", __FUNCTION__, Address);
		Status = EFI_SUCCESS;
	} else if (Operation == MEM_LOCK && !CanWriteAtAddress(Address)) {
		Print(L"%a: Memory at %x already locked\n", __FUNCTION__, Address);
		Status = EFI_SUCCESS;
	}

	//
	// Try to lock/unlock with EfiLegacyRegionProtocol
	// 
	if (EFI_ERROR(Status)) {
		Status = gBS->LocateProtocol(&gEfiLegacyRegionProtocolGuid, NULL, (VOID **)&mLegacyRegion);
		if (!EFI_ERROR(Status)) {
			if (Operation == MEM_UNLOCK) {
				Status = mLegacyRegion->UnLock(mLegacyRegion, (UINT32)Address, Length, &Granularity);
				Status = CanWriteAtAddress(Address) ? EFI_SUCCESS : EFI_DEVICE_ERROR;
			} else {
				Status = mLegacyRegion->Lock(mLegacyRegion, (UINT32)Address, Length, &Granularity);
				Status = CanWriteAtAddress(Address) ? EFI_DEVICE_ERROR : EFI_SUCCESS;
			}

			Print(L"%a: %s %s memory at %x using EfiLegacyRegionProtocol\n", 
				__FUNCTION__, 
				EFI_ERROR(Status) ? L"Failure" : L"Success",
				Operation == MEM_UNLOCK ? L"unlocking" : L"locking", 
				Address);
		}
	}
	
	//
	// Try to lock/unlock with EfiLegacyRegion2Protocol
	//
	if (EFI_ERROR(Status)) {
		Status = gBS->LocateProtocol(&gEfiLegacyRegion2ProtocolGuid, NULL, (VOID **)&mLegacyRegion2);
		if (!EFI_ERROR(Status)) {
			if (Operation == MEM_UNLOCK) {
				Status = mLegacyRegion2->UnLock(mLegacyRegion2, (UINT32)Address, Length, &Granularity);
				Status = CanWriteAtAddress(Address);
			} else {
				Status = mLegacyRegion2->Lock(mLegacyRegion2, (UINT32)Address, Length, &Granularity);
				Status = CanWriteAtAddress(Address) == EFI_WRITE_PROTECTED ? EFI_SUCCESS : EFI_DEVICE_ERROR;
			}

			Print(L"%a: %s %s memory at %x using EfiLegacyRegion2Protocol\n", 
				__FUNCTION__, 
				EFI_ERROR(Status) ? L"Failure" : L"Success",
				Operation == MEM_UNLOCK ? L"unlocking" : L"locking", 
				Address);
		}
	}
	
	//
	// Try to lock/unlock via an MTRR
	//
	if (EFI_ERROR(Status) && IsMtrrSupported()) {
		if (Operation == MEM_UNLOCK) {
			MtrrSetMemoryAttribute(Address, FIXED_MTRR_SIZE, CacheUncacheable);
			Status = CanWriteAtAddress(Address);
		} else {
			MtrrSetMemoryAttribute(Address, FIXED_MTRR_SIZE, CacheWriteProtected);
			Status = CanWriteAtAddress(Address) == EFI_WRITE_PROTECTED ? EFI_SUCCESS : EFI_DEVICE_ERROR;
		}

		Print(L"%a: %s %s memory at %x using MTRR\n", 
			__FUNCTION__, 
			EFI_ERROR(Status) ? "Failure" : "Success",
			Operation == MEM_UNLOCK ? "unlocking" : "locking", 
			Address);
	}
	
	//
	// None of the methods worked?
	// 
	if (EFI_ERROR(Status)) {
		Print(L"%a: Unable to find a way to %s memory at %x\n", 
			__FUNCTION__, Operation == MEM_UNLOCK ? "unlock" : "lock", Address);
	}
		
	return Status;
}


BOOLEAN
CanWriteAtAddress(
	EFI_PHYSICAL_ADDRESS	Address)
{
	BOOLEAN	CanWrite;
	UINT8	*TestPtr;
	UINT8	OldValue;

	TestPtr = (UINT8*)(Address);
	OldValue = *TestPtr;

	//Print(L"%a: Attempting memory write at %x ... ", __FUNCTION__, TestPtr);
	
	*TestPtr = *TestPtr + 1;
	CanWrite = OldValue != *TestPtr;
	
	//Print(L"%s\n", CanWrite ? L"successful" : L"unsuccessful");
	
	*TestPtr = OldValue;
	return CanWrite;
}