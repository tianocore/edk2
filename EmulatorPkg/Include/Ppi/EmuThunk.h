/** @file
  Emulator Thunk to abstract OS services from pure EFI code

  Copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EMU_THUNK_PPI_H__
#define __EMU_THUNK_PPI_H__

#define EMU_THUNK_PPI_GUID  \
 { 0xB958B78C, 0x1D3E, 0xEE40, { 0x8B, 0xF4, 0xF0, 0x63, 0x2D, 0x06, 0x39, 0x16 } }



/*++

Routine Description:
  This service is called from Index == 0 until it returns EFI_UNSUPPORTED.
  It allows discontiguous memory regions to be supported by the emulator.

Arguments:
  Index      - Which memory region to use
  MemoryBase - Return Base address of memory region
  MemorySize - Return size in bytes of the memory region

Returns:
  EFI_SUCCESS - If memory region was mapped
  EFI_UNSUPPORTED - If Index is not supported

**/
typedef
EFI_STATUS
(EFIAPI *EMU_PEI_AUTOSCAN) (
  IN  UINTN                 Index,
  OUT EFI_PHYSICAL_ADDRESS  *MemoryBase,
  OUT UINT64                *MemorySize
  );


/*++

Routine Description:
  Return the FD Size and base address. Since the FD is loaded from a
  file into host memory only the SEC will know it's address.

Arguments:
  Index  - Which FD, starts at zero.
  FdSize - Size of the FD in bytes
  FdBase - Start address of the FD. Assume it points to an FV Header
  FixUp  - Difference between actual FD address and build address

Returns:
  EFI_SUCCESS     - Return the Base address and size of the FV
  EFI_UNSUPPORTED - Index does nto map to an FD in the system

**/
typedef
EFI_STATUS
(EFIAPI *EMU_PEI_FD_INFORMATION) (
  IN     UINTN                  Index,
  IN OUT EFI_PHYSICAL_ADDRESS   *FdBase,
  IN OUT UINT64                 *FdSize,
  IN OUT EFI_PHYSICAL_ADDRESS   *FixUp
  );


/*++

Routine Description:
  Export of EMU_THUNK_PROTOCOL from the SEC.

Returns:
  EFI_SUCCESS - Data returned

**/
typedef
VOID *
(EFIAPI *EMU_PEI_THUNK_INTERFACE) (
  VOID
  );



/*++

Routine Description:
  Loads and relocates a PE/COFF image into memory.

Arguments:
  Pe32Data         - The base address of the PE/COFF file that is to be loaded and relocated
  ImageAddress     - The base address of the relocated PE/COFF image
  ImageSize        - The size of the relocated PE/COFF image
  EntryPoint       - The entry point of the relocated PE/COFF image

Returns:
  EFI_SUCCESS   - The file was loaded and relocated
  EFI_OUT_OF_RESOURCES - There was not enough memory to load and relocate the PE/COFF file

**/
typedef
EFI_STATUS
(EFIAPI *EMU_PEI_LOAD_FILE) (
  VOID                  *Pe32Data,
  EFI_PHYSICAL_ADDRESS  *ImageAddress,
  UINT64                *ImageSize,
  EFI_PHYSICAL_ADDRESS  *EntryPoint
  );


typedef struct {
  EMU_PEI_AUTOSCAN                  MemoryAutoScan;
  EMU_PEI_FD_INFORMATION            FirmwareDevices;
  EMU_PEI_THUNK_INTERFACE           Thunk;
} EMU_THUNK_PPI;

extern EFI_GUID gEmuThunkPpiGuid;

#endif
