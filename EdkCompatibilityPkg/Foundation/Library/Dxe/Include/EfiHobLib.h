/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    EfiHobLib.h

Abstract:

 
--*/

#ifndef _EFI_HOB_LIB_H_
#define _EFI_HOB_LIB_H_

#include "PeiHob.h"

VOID  *
GetHob (
  IN UINT16  Type,
  IN VOID    *HobStart
  )
/*++

Routine Description:

  This function returns the first instance of a HOB type in a HOB list.
  
Arguments:

  Type          The HOB type to return.
  HobStart      The first HOB in the HOB list.
    
Returns:

  HobStart      There were no HOBs found with the requested type.
  else          Returns the first HOB with the matching type.

--*/
;

UINTN
GetHobListSize (
  IN VOID  *HobStart
  )
/*++

Routine Description:

  Get size of hob list.

Arguments:

  HobStart      - Start pointer of hob list

Returns:

  Size of hob list.

--*/
;

UINT32
GetHobVersion (
  IN VOID  *HobStart
  )
/*++

Routine Description:

  Get hob version.

Arguments:

  HobStart      - Start pointer of hob list

Returns:

  Hob version.

--*/
;

EFI_STATUS
GetHobBootMode (
  IN  VOID           *HobStart,
  OUT EFI_BOOT_MODE  *BootMode
  )
/*++

Routine Description:

  Get current boot mode.

Arguments:

  HobStart      - Start pointer of hob list
  
  BootMode      - Current boot mode recorded in PHIT hob

Returns:

  EFI_NOT_FOUND     - Invalid hob header
  
  EFI_SUCCESS       - Boot mode found

--*/
;

EFI_STATUS
GetCpuHobInfo (
  IN  VOID   *HobStart,
  OUT UINT8  *SizeOfMemorySpace,
  OUT UINT8  *SizeOfIoSpace
  )
/*++

Routine Description:

  Get information recorded in CPU hob (Memory space size, Io space size)

Arguments:

  HobStart            - Start pointer of hob list
  
  SizeOfMemorySpace   - Size of memory size
  
  SizeOfIoSpace       - Size of IO size

Returns:

  EFI_NOT_FOUND     - CPU hob not found
  
  EFI_SUCCESS       - CPU hob found and information got.

--*/
;

EFI_STATUS
GetDxeCoreHobInfo (
  IN  VOID                  *HobStart,
  OUT EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT UINT64                *Length,
  OUT VOID                  **EntryPoint,
  OUT EFI_GUID              **FileName
  )
/*++

Routine Description:

  Get memory allocation hob created for DXE core and extract its information

Arguments:

  HobStart        - Start pointer of the hob list
  
  BaseAddress     - Start address of memory allocated for DXE core
  
  Length          - Length of memory allocated for DXE core
  
  EntryPoint      - DXE core file name

  FileName        - FileName  

Returns:

  EFI_NOT_FOUND   - DxeCoreHob not found
  
  EFI_SUCCESS     - DxeCoreHob found and information got

--*/
;

EFI_STATUS
GetNextFirmwareVolumeHob (
  IN OUT VOID                  **HobStart,
  OUT    EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT    UINT64                *Length
  )
/*++

Routine Description:

  Get next firmware volume hob from HobStart

Arguments:

  HobStart        - Start pointer of hob list
  
  BaseAddress     - Start address of next firmware volume
  
  Length          - Length of next firmware volume

Returns:

  EFI_NOT_FOUND   - Next firmware volume not found
  
  EFI_SUCCESS     - Next firmware volume found with address information

--*/
;

#if (PI_SPECIFICATION_VERSION >= 0x00010000)
EFI_STATUS
GetNextFirmwareVolume2Hob (
  IN OUT VOID                  **HobStart,
  OUT    EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT    UINT64                *Length,
  OUT    EFI_GUID              *FileName
  );
#endif

EFI_STATUS
GetNextGuidHob (
  IN OUT VOID      **HobStart,
  IN     EFI_GUID  * Guid,
  OUT    VOID      **Buffer,
  OUT    UINTN     *BufferSize OPTIONAL
  )
/*++

Routine Description:
  Get the next guid hob.
  
Arguments:
  HobStart        A pointer to the start hob.
  Guid            A pointer to a guid.
  Buffer          A pointer to the buffer.
  BufferSize      Buffer size.
  
Returns:
  Status code.
  
  EFI_NOT_FOUND       - Next Guid hob not found
  
  EFI_SUCCESS         - Next Guid hob found and data for this Guid got

--*/
;

EFI_STATUS
GetPalEntryHobInfo (
  IN  VOID                  *HobStart,
  OUT EFI_PHYSICAL_ADDRESS  *PalEntry
  )
/*++

Routine Description:

  Get PAL entry from PalEntryHob

Arguments:

  HobStart      - Start pointer of hob list
  
  PalEntry      - Pointer to PAL entry

Returns:

  Status code.

--*/
;

EFI_STATUS
GetIoPortSpaceAddressHobInfo (
  IN  VOID                  *HobStart,
  OUT EFI_PHYSICAL_ADDRESS  *IoPortSpaceAddress
  )
/*++

Routine Description:

  Get IO port space address from IoBaseHob.

Arguments:

  HobStart              - Start pointer of hob list
  
  IoPortSpaceAddress    - IO port space address

Returns:

  Status code

--*/
;

#endif
