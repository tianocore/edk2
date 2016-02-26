/** @file
 VgaShim Code

**/

#define VBIOS_START         0xc0000
#define VBIOS_SIZE          0x10000

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

#include "LegacyVgaBios.h"
#include "VgaShim.h"

// ensure correct alignment
#pragma pack (1)
typedef struct {
  UINT16 Offset;
  UINT16 Segment;
} IVT_ENTRY;
#pragma pack ()

//
// This string is displayed by Windows 2008 R2 SP1 in the Screen Resolution,
// Advanced Settings dialog. It should be short.
//
STATIC CONST CHAR8 mProductRevision[] = "OVMF Int10h (fake)";

EFI_STATUS
EFIAPI
UefiMain (
	IN EFI_HANDLE				ImageHandle,
	IN EFI_SYSTEM_TABLE	*SystemTable)
{
	EFI_PHYSICAL_ADDRESS Segment0, SegmentC, SegmentF, FrameBufferBase;
	UINTN Segment0Pages;
	IVT_ENTRY *Int0x10;
	EFI_STATUS Status;
	UINTN Pam1Address;
	UINT8 Int8temp;
	UINTN SegmentCPages;
	VBE_INFO *VbeInfoFull;
	VBE_INFO_BASE *VbeInfo;
	UINT8 *Ptr;
	UINTN Printed;
	VBE_MODE_INFO *VbeModeInfo;
	UINT32 MyTempU;
	INT32 MyTemp;

	UINT32            Granularity;
	UINT8             *TstPtr;
	UINT8             TstVar;

	EFI_LEGACY_REGION_PROTOCOL    *mLegacyRegion = NULL;
	EFI_LEGACY_REGION2_PROTOCOL   *mLegacyRegion2 = NULL;

	Print(L"VGA Shim v0.1\n");

	Segment0 = 0x00000;
	SegmentC = 0xC0000;
	SegmentF = 0xF0000;
	FrameBufferBase = 0xA0000;

	Status = gBS->LocateProtocol(&gEfiLegacyRegionProtocolGuid, NULL, (VOID **)&mLegacyRegion);
	Print(L"LegacyRegion = %r\n", Status);
	if (EFI_ERROR(Status)) {
		mLegacyRegion = NULL;
		Status = gBS->LocateProtocol(&gEfiLegacyRegion2ProtocolGuid, NULL, (VOID **)&mLegacyRegion2);
		Print(L"LegacyRegion2 = %r\n", Status);
		if (EFI_ERROR(Status)) {
			mLegacyRegion2 = NULL;
		}
	}

	Status = mLegacyRegion->UnLock(mLegacyRegion, VBIOS_START, VBIOS_SIZE, &Granularity);
	Print(L"Unlock = %r\n", Status);

	//
	// Test some vbios address for writing
	//
	TstPtr = (UINT8*)(UINTN)(VBIOS_START + 0xA110);
	TstVar = *TstPtr;
	*TstPtr = *TstPtr + 1;
	if (TstVar == *TstPtr) {
		Print(L"Test unlock: Error, not unlocked!\n");
	} else {
		Print(L"Test unlock: Yeah!\n");
	}
	*TstPtr = TstVar;

	//
	// Attempt to cover the real mode IVT with an allocation. This is a UEFI
	// driver, hence the arch protocols have been installed previously. Among
	// those, the CPU arch protocol has configured the IDT, so we can overwrite
	// the IVT used in real mode.
	//
	// The allocation request may fail, eg. if LegacyBiosDxe has already run.
	//
	Segment0Pages = 1;
	Int0x10 = (IVT_ENTRY *)(UINTN)Segment0 + 0x10;
	Status = gBS->AllocatePages(AllocateAddress, EfiBootServicesCode, Segment0Pages, &Segment0);

	if (EFI_ERROR(Status)) {
		Print(L"Whoops, unable to allocate pages! Status=%r\n", Status);
		EFI_PHYSICAL_ADDRESS Handler;

		//
		// Check if a video BIOS handler has been installed previously -- we
		// shouldn't override a real video BIOS with our shim, nor our own shim if
		// it's already present.
		//
		Handler = (Int0x10->Segment << 4) + Int0x10->Offset;
		if (Handler >= SegmentC && Handler < SegmentF) {
			Print(L"%a: Video BIOS handler found at %04x:%04x\n",
				__FUNCTION__, Int0x10->Segment, Int0x10->Offset);
			return EFI_SUCCESS;
		}

		//
		// Otherwise we'll overwrite the Int10h vector, even though we may not own
		// the page at zero.
		//
		Print(L"%a: failed to allocate page at zero: %r\n",
			__FUNCTION__, Status);
	} else {
		Print(L"Managed to allocate the page at zero\n");
		//
		// We managed to allocate the page at zero. SVN r14218 guarantees that it
		// is NUL-filled.
		//
		ASSERT(Int0x10->Segment == 0x0000);
		ASSERT(Int0x10->Offset == 0x0000);
	}

	// check if area is protected
	//MyTemp = MtrrGetMemoryAttribute(SegmentC);
	//Print(L"MtrrGetMemoryAttribute = %d\n", MyTemp);
	//if (MyTemp == CacheWriteProtected) {}
	MtrrDebugPrintAllMtrrs();
	//Print(L"IsMttrSupported = %d\n", IsMtrrSupported());

	// 
	// Enable writes to C0000-DFFFF memory area
	// We only need C0000-C7FFF but a lot more is protected
	// 
	//MtrrSetMemoryAttribute(SegmentC, 131072 /*DFFFF-C0000+1*/, CacheUncacheable);
	MtrrSetMemoryAttribute(SegmentC, 131072 /*DFFFF-C0000+1*/, CacheUncacheable);
	
	SegmentCPages = 4;
	ASSERT(sizeof mVgaShim <= VBIOS_SIZE);
	
	//
	// Put the shim in place!
	//
	SetMem((VOID *)(UINTN)VBIOS_START, VBIOS_SIZE, 0);
	CopyMem((VOID *)(UINTN)SegmentC, mVgaShim, sizeof mVgaShim);



	//
	// Fill in the VESA BIOS Extensions INFO structure.
	//
	// point VbeInfoFull at 0xC0000
	VbeInfoFull = (VBE_INFO *)(UINTN)SegmentC;
	// point VbeInfo at 0xC0000
	VbeInfo = &VbeInfoFull->Base;
	// point Ptr at 0xC0022, ie. start of an array of size 256-sizeof(VBE_INFO_BASE)
	Ptr = VbeInfoFull->Buffer;
	// set offset 00h: signature
	CopyMem(VbeInfo->Signature, "VESA", 4);
	// set offset 04h: VESA version number
	VbeInfo->VesaVersion = 0x0300;
	// set offset 06h: pointer to OEM name
	VbeInfo->OemNameAddress = (UINT32)SegmentC << 12 | (UINT16)(UINTN)Ptr;
	CopyMem(Ptr, "QEMU", 5);		// copy name to buffer
	Ptr += 5;						// advance buffer by 5 (name+zero-terminator)
	// set offset 0Ah: capabilities
	VbeInfo->Capabilities = BIT0;	// DAC can be switched into 8-bit mode
	// set offset 0Eh: pointer to available video modes terminated with 0FFFFh
	VbeInfo->ModeListAddress = (UINT32)SegmentC << 12 | (UINT16)(UINTN)Ptr;
	*(UINT16*)Ptr = 0x00f1;			// mode number
	Ptr += 2;						// advance buffer by 2 bytes
	*(UINT16*)Ptr = 0xFFFF;			// mode list terminator
	Ptr += 2;						// advance buffer by 2 bytes
	// set offset 12h: amount of video memory in 64k blocks
	VbeInfo->VideoMem64K = (UINT16)((1024 * 768 * 4 + 65535) / 65536);
	// set offset 14h: OEM software version
	VbeInfo->OemSoftwareVersion = 0x0000;
	// set offset 16h: Pointer to vendor name
	VbeInfo->VendorNameAddress = (UINT32)SegmentC << 12 | (UINT16)(UINTN)Ptr;
	CopyMem(Ptr, "OVMF", 5);		// copy name to buffer
	Ptr += 5;						// advance buffer by 5 (name+zero-terminator)
	// set offset 1Ah: Pointer to product name
	VbeInfo->ProductNameAddress = (UINT32)SegmentC << 12 | (UINT16)(UINTN)Ptr;
	CopyMem(Ptr, "DummyCard", 10);	// copy name to buffer
	Ptr += 10;						// advance buffer by 10 (name+zero-terminator)
	// set offset 1Eh: Pointer to product revision string
	VbeInfo->ProductRevAddress = (UINT32)SegmentC << 12 | (UINT16)(UINTN)Ptr;
	CopyMem(Ptr, mProductRevision, sizeof mProductRevision);
	Ptr += sizeof mProductRevision;
	// make sure we did not fill more data than we had space for
	ASSERT(sizeof VbeInfoFull->Buffer >= Ptr - VbeInfoFull->Buffer);
	// zero remaining memory... but this should not be necessary
	ZeroMem(Ptr, sizeof VbeInfoFull->Buffer - (Ptr - VbeInfoFull->Buffer));
	
	//
	// Fil in the VBE MODE INFO structure.
	//
	VbeModeInfo = (VBE_MODE_INFO *)(VbeInfoFull + 1);
	// bit0: mode supported by present hardware configuration
	// bit1: optional information available (must be =1 for VBE v1.2+)
	// bit3: set if color, clear if monochrome
	// bit4: set if graphics mode, clear if text mode
	// bit5: mode is not VGA-compatible
	// bit7: linear framebuffer mode supported
	VbeModeInfo->ModeAttr = BIT7 | BIT5 | BIT4 | BIT3 | BIT1 | BIT0;
	// bit0: exists
	// bit1: bit1: readable
	// bit2: writeable
	VbeModeInfo->WindowAAttr = BIT2 | BIT1 | BIT0;

	VbeModeInfo->WindowBAttr = 0x00;
	VbeModeInfo->WindowGranularityKB = 0x0040;
	VbeModeInfo->WindowSizeKB = 0x0040;
	VbeModeInfo->WindowAStartSegment = 0xA000;
	VbeModeInfo->WindowBStartSegment = 0x0000;
	VbeModeInfo->WindowPositioningAddress = 0x0000;
	VbeModeInfo->BytesPerScanLine = 1024 * 4;

	// MBA 1440x900
	VbeModeInfo->Width = 1024;
	VbeModeInfo->Height = 768;
	VbeModeInfo->CharCellWidth = 8;
	VbeModeInfo->CharCellHeight = 16;
	VbeModeInfo->NumPlanes = 1;
	VbeModeInfo->BitsPerPixel = 32;
	VbeModeInfo->NumBanks = 1;
	VbeModeInfo->MemoryModel = 6; // direct color
	VbeModeInfo->BankSizeKB = 0;
	VbeModeInfo->NumImagePagesLessOne = 0;
	VbeModeInfo->Vbe3 = 0x01;

	VbeModeInfo->RedMaskSize = 8;
	VbeModeInfo->RedMaskPos = 16;
	VbeModeInfo->GreenMaskSize = 8;
	VbeModeInfo->GreenMaskPos = 8;
	VbeModeInfo->BlueMaskSize = 8;
	VbeModeInfo->BlueMaskPos = 0;
	VbeModeInfo->ReservedMaskSize = 8;
	VbeModeInfo->ReservedMaskPos = 24;
	
	VbeModeInfo->DirectColorModeInfo = BIT1; // bit1: bytes in reserved field may be used by application
	VbeModeInfo->LfbAddress = (UINT32)FrameBufferBase;
	VbeModeInfo->OffScreenAddress = 0;
	VbeModeInfo->OffScreenSizeKB = 0;

	VbeModeInfo->BytesPerScanLineLinear = 1024 * 4;
	VbeModeInfo->NumImagesLessOneBanked = 0;
	VbeModeInfo->NumImagesLessOneLinear = 0;
	VbeModeInfo->RedMaskSizeLinear = 8;
	VbeModeInfo->RedMaskPosLinear = 16;
	VbeModeInfo->GreenMaskSizeLinear = 8;
	VbeModeInfo->GreenMaskPosLinear = 8;
	VbeModeInfo->BlueMaskSizeLinear = 8;
	VbeModeInfo->BlueMaskPosLinear = 0;
	VbeModeInfo->ReservedMaskSizeLinear = 8;
	VbeModeInfo->ReservedMaskPosLinear = 24;
	VbeModeInfo->MaxPixelClockHz = 0;
	
	// zero remaining memory
	ZeroMem(VbeModeInfo->Reserved, sizeof VbeModeInfo->Reserved);

	//
	// Point the Int10h vector at the shim.
	//
	Int0x10->Segment = (UINT16)((UINT32)SegmentC >> 4);
	Int0x10->Offset = (UINT16)((UINTN)(VbeModeInfo + 1) - SegmentC);
	Print(L"%a: Video BIOS handler installed at %04x:%04x\n",
		__FUNCTION__, Int0x10->Segment, Int0x10->Offset);

	Print(L"Done!\n");
	
	return EFI_SUCCESS;
}
