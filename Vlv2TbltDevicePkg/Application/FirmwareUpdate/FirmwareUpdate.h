/** @file

Copyright (c) 1999  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

**/

#ifndef _FIRMWARE_UPDATE_H_
#define _FIRMWARE_UPDATE_H_

#include <Uefi.h>

#include <PiDxe.h>

#include <Guid/FileInfo.h>

#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/Spi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/FileHandleLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

//
// Function Prototypes.
//
STATIC
EFI_STATUS
ReadFileData (
  IN  CHAR16   *FileName,
  OUT UINT8    **Buffer,
  OUT UINT32   *BufferSize
  );

STATIC
EFI_STATUS
InternalEraseBlock (
  IN  EFI_PHYSICAL_ADDRESS    BaseAddress
  );

#if 0
STATIC
EFI_STATUS
InternalReadBlock (
  IN  EFI_PHYSICAL_ADDRESS    BaseAddress,
  OUT VOID                    *ReadBuffer
  );
#endif

STATIC
EFI_STATUS
InternalCompareBlock (
  IN  EFI_PHYSICAL_ADDRESS    BaseAddress,
  IN  UINT8                   *Buffer
  );

STATIC
EFI_STATUS
InternalWriteBlock (
  IN  EFI_PHYSICAL_ADDRESS    BaseAddress,
  IN  UINT8                   *Buffer,
  IN  UINT32                  BufferSize
  );

STATIC
VOID
PrintHelpInfo (
  VOID
  );

STATIC
EFI_STATUS
EFIAPI
SpiFlashRead (
  IN     UINTN     Address,
  IN OUT UINT32    *NumBytes,
     OUT UINT8     *Buffer
  );

STATIC
EFI_STATUS
EFIAPI
SpiFlashWrite (
  IN     UINTN     Address,
  IN OUT UINT32    *NumBytes,
  IN     UINT8     *Buffer
  );

STATIC
EFI_STATUS
EFIAPI
SpiFlashBlockErase (
  IN    UINTN    Address,
  IN    UINTN    *NumBytes
  );

STATIC
EFI_STATUS
EFIAPI
ConvertMac (
  CHAR16 *Str
  );

EFI_STATUS
InitializeFVUPDATE (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

//
// Flash specific definitions.
// - Should we use a PCD for this information?
//
#define BLOCK_SIZE          SIZE_4KB

//
// Flash region layout and update information.
//
typedef struct {
  EFI_PHYSICAL_ADDRESS  Base;
  UINTN                 Size;
  BOOLEAN               Update;
} FV_REGION_INFO;

//
// MAC Address information.
//
#define MAC_ADD_STR_LEN       12
#define MAC_ADD_STR_SIZE      (MAC_ADD_STR_LEN + 1)
#define MAC_ADD_BYTE_COUNT    6
#define MAC_ADD_TMP_STR_LEN   2
#define MAC_ADD_TMP_STR_SIZE  (MAC_ADD_TMP_STR_LEN + 1)

//
// Command Line Data.
//
#define INPUT_STRING_LEN    255
#define INPUT_STRING_SIZE   (INPUT_STRING_LEN + 1)
typedef struct {
  BOOLEAN   UpdateFromFile;
  CHAR16    FileName[INPUT_STRING_SIZE];
  BOOLEAN   UpdateMac;
  UINT8     MacValue[MAC_ADD_BYTE_COUNT];
  BOOLEAN   FullFlashUpdate;
} FV_INPUT_DATA;

//
// Prefix Opcode Index on the host SPI controller.
//
typedef enum {
  SPI_WREN,             // Prefix Opcode 0: Write Enable.
  SPI_EWSR,             // Prefix Opcode 1: Enable Write Status Register.
} PREFIX_OPCODE_INDEX;

//
// Opcode Menu Index on the host SPI controller.
//
typedef enum {
  SPI_READ_ID,        // Opcode 0: READ ID, Read cycle with address.
  SPI_READ,           // Opcode 1: READ, Read cycle with address.
  SPI_RDSR,           // Opcode 2: Read Status Register, No address.
  SPI_WRDI_SFDP,      // Opcode 3: Write Disable or Discovery Parameters, No address.
  SPI_SERASE,         // Opcode 4: Sector Erase (4KB), Write cycle with address.
  SPI_BERASE,         // Opcode 5: Block Erase (32KB), Write cycle with address.
  SPI_PROG,           // Opcode 6: Byte Program, Write cycle with address.
  SPI_WRSR,           // Opcode 7: Write Status Register, No address.
} SPI_OPCODE_INDEX;

#endif
