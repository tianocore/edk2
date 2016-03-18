//// @file
//
// Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
//
// This program and the accompanying materials
// are licensed and made available under the terms and conditions
// of the BSD License which accompanies this distribution.  The
// full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
////

#define NUM_REAL_GDT_ENTRIES						3
#define LOW_STACK_SIZE								(8*1024)            // 8k?

//
// Low memory Thunk Structure
//
#define		Code									0
#define		LowReverseThunkStart					Code + 4096
#define		GdtDesc									LowReverseThunkStart + 4
#define		IdtDesc					 				GdtDesc + 6
#define		FlatSs					 				IdtDesc + 6
#define		FlatEsp					 				FlatSs + 4
#define		LowCodeSelector			 				FlatEsp + 4
#define		LowDataSelector			 				LowCodeSelector + 4
#define		LowStack			 					LowDataSelector + 4
#define		RealModeIdtDesc			 				LowStack + 4
#define		RealModeGdt								RealModeIdtDesc + 6
#define		RealModeGdtDesc							RealModeGdt + (8 * NUM_REAL_GDT_ENTRIES)
#define		RevRealDs								RealModeGdtDesc + 6
#define		RevRealSs								RevRealDs + 2
#define		RevRealEsp								RevRealSs + 2
#define		RevRealIdtDesc							RevRealEsp + 4
#define		RevFlatDataSelector						RevRealIdtDesc + 6
#define		RevFlatStack							RevFlatDataSelector + 2
#define		Stack									RevFlatStack + 4
#define		RevThunkStack							Stack + LOW_STACK_SIZE

#define		EfiToLegacy16InitTable					RevThunkStack + LOW_STACK_SIZE
#define		InitTableBiosLessThan1MB				EfiToLegacy16InitTable
#define		InitTableHiPmmMemory					InitTableBiosLessThan1MB + 4
#define		InitTablePmmMemorySizeInBytes			InitTableHiPmmMemory + 4
#define		InitTableReverseThunkCallSegment		InitTablePmmMemorySizeInBytes + 4
#define		InitTableReverseThunkCallOffset			InitTableReverseThunkCallSegment + 2
#define		InitTableNumberE820Entries				InitTableReverseThunkCallOffset + 2
#define		InitTableOsMemoryAbove1Mb				InitTableNumberE820Entries + 4
#define		InitTableThunkStart						InitTableOsMemoryAbove1Mb + 4
#define		InitTableThunkSizeInBytes				InitTableThunkStart + 4
#define		InitTable16InitTableEnd					InitTableThunkSizeInBytes + 4

#define		EfiToLegacy16BootTable					InitTable16InitTableEnd
#define		BootTableBiosLessThan1MB				EfiToLegacy16BootTable
#define		BootTableHiPmmMemory					BootTableBiosLessThan1MB + 4
#define		BootTablePmmMemorySizeInBytes			BootTableHiPmmMemory + 4
#define		BootTableReverseThunkCallSegment		BootTablePmmMemorySizeInBytes + 4
#define		BootTableReverseThunkCallOffset			BootTableReverseThunkCallSegment + 2
#define		BootTableNumberE820Entries				BootTableReverseThunkCallOffset + 2
#define		BootTableOsMemoryAbove1Mb				BootTableNumberE820Entries + 4
#define		BootTableThunkStart						BootTableOsMemoryAbove1Mb + 4
#define		BootTableThunkSizeInBytes				BootTableThunkStart + 4
#define		EfiToLegacy16BootTableEnd				BootTableThunkSizeInBytes + 4

#define		InterruptRedirectionCode				EfiToLegacy16BootTableEnd
#define		PciHandler								InterruptRedirectionCode + 32


//
// Register Sets (16 Bit)
//

#define		AX		0
#define		BX		2
#define		CX		4
#define		DX		6
#define		SI		8
#define		DI		10
#define		Flags	12
#define		ES		14
#define		CS		16
#define		SS		18
#define		DS		20
#define		BP		22



