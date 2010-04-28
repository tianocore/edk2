/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


Module Name:
  SecMain.h

Abstract:
  Include file for Windows API based SEC

**/

#include <stdio.h>
#include <PiPei.h>
#include <WinNtPeim.h>
#include <Guid/StatusCodeDataTypeDebug.h>
#include <Library/BaseLib.h>
#include <Library/PeCoffLib.h>
#include <Ppi/NtPeiLoadFile.h>
#include <Ppi/NtAutoscan.h>
#include <Ppi/NtThunk.h>
#include <Ppi/StatusCode.h>
#include <Ppi/NtFwh.h>
#include <Ppi/TemporaryRamSupport.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ReportStatusCodeLib.h>

#include <IndustryStandard/PeImage.h>

#define STACK_SIZE                0x20000      

typedef struct {
  EFI_PHYSICAL_ADDRESS  Address;
  UINT64                Size;
} NT_FD_INFO;

typedef struct {
  EFI_PHYSICAL_ADDRESS  Memory;
  UINT64                Size;
} NT_SYSTEM_MEMORY;

#define MAX_PDB_NAME_TO_MOD_HANDLE_ARRAY_SIZE 0x100

typedef struct {
  CHAR8   *PdbPointer;
  VOID    *ModHandle;
} PDB_NAME_TO_MOD_HANDLE;




EFI_STATUS
EFIAPI
SecWinNtPeiLoadFile (
  VOID                  *Pe32Data,  // TODO: add IN/OUT modifier to Pe32Data
  EFI_PHYSICAL_ADDRESS  *ImageAddress,  // TODO: add IN/OUT modifier to ImageAddress
  UINT64                *ImageSize,  // TODO: add IN/OUT modifier to ImageSize
  EFI_PHYSICAL_ADDRESS  *EntryPoint  // TODO: add IN/OUT modifier to EntryPoint
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Pe32Data      - TODO: add argument description
  ImageAddress  - TODO: add argument description
  ImageSize     - TODO: add argument description
  EntryPoint    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
SecWinNtPeiAutoScan (
  IN  UINTN                 Index,
  OUT EFI_PHYSICAL_ADDRESS  *MemoryBase,
  OUT UINT64                *MemorySize
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Index       - TODO: add argument description
  MemoryBase  - TODO: add argument description
  MemorySize  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

VOID *
EFIAPI
SecWinNtWinNtThunkAddress (
  VOID
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  InterfaceSize - TODO: add argument description
  InterfaceBase - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
SecWinNtWinNtFwhAddress (
  IN OUT UINT64                *FwhSize,
  IN OUT EFI_PHYSICAL_ADDRESS  *FwhBase
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  FwhSize - TODO: add argument description
  FwhBase - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
SecPeiReportStatusCode (
  IN CONST EFI_PEI_SERVICES         **PeiServices,
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN CONST EFI_GUID                 * CallerId,
  IN CONST EFI_STATUS_CODE_DATA     * Data OPTIONAL
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PeiServices - TODO: add argument description
  CodeType    - TODO: add argument description
  Value       - TODO: add argument description
  Instance    - TODO: add argument description
  CallerId    - TODO: add argument description
  Data        - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

INTN
EFIAPI
main (
  IN  INTN  Argc,
  IN  CHAR8 **Argv,
  IN  CHAR8 **Envp
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Argc  - TODO: add argument description
  Argv  - TODO: add argument description
  Envp  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
WinNtOpenFile (
  CHAR16                *FileName,
  UINT32                MapSize,
  DWORD                 CreationDispostion,
  EFI_PHYSICAL_ADDRESS  *BaseAddress,
  UINT64                *Length
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  FileName            - TODO: add argument description
  MapSize             - TODO: add argument description
  CreationDispostion  - TODO: add argument description
  BaseAddress         - TODO: add argument description
  Length              - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

VOID
SecLoadFromCore (
  IN  UINTN   LargestRegion,
  IN  UINTN   LargestRegionSize,
  IN  UINTN   BootFirmwareVolumeBase,
  IN  VOID    *PeiCoreFile
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  LargestRegion           - TODO: add argument description
  LargestRegionSize       - TODO: add argument description
  BootFirmwareVolumeBase  - TODO: add argument description
  PeiCoreFile             - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
SecLoadFile (
  IN  VOID                    *Pe32Data,
  IN  EFI_PHYSICAL_ADDRESS    *ImageAddress,
  IN  UINT64                  *ImageSize,
  IN  EFI_PHYSICAL_ADDRESS    *EntryPoint
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Pe32Data      - TODO: add argument description
  ImageAddress  - TODO: add argument description
  ImageSize     - TODO: add argument description
  EntryPoint    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
SecFfsFindPeiCore (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader,
  OUT VOID                        **Pe32Data
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  FwVolHeader - TODO: add argument description
  Pe32Data    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
SecFfsFindNextFile (
  IN EFI_FV_FILETYPE             SearchType,
  IN EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader,
  IN OUT EFI_FFS_FILE_HEADER     **FileHeader
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  SearchType  - TODO: add argument description
  FwVolHeader - TODO: add argument description
  FileHeader  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
SecFfsFindSectionData (
  IN EFI_SECTION_TYPE      SectionType,
  IN EFI_FFS_FILE_HEADER   *FfsFileHeader,
  IN OUT VOID              **SectionData
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  SectionType   - TODO: add argument description
  FfsFileHeader - TODO: add argument description
  SectionData   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
SecWinNtPeCoffLoaderLoadAsDll (
  IN CHAR8    *PdbFileName,
  IN VOID     **ImageEntryPoint,
  OUT VOID    **ModHandle
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PdbFileName     - TODO: add argument description
  ImageEntryPoint - TODO: add argument description
  ModHandle       - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
SecWinNtPeCoffLoaderFreeLibrary (
  OUT VOID    *ModHandle
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ModHandle - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
SecWinNtFdAddress (
  IN     UINTN                 Index,
  IN OUT EFI_PHYSICAL_ADDRESS  *FdBase,
  IN OUT UINT64                *FdSize
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Index   - TODO: add argument description
  FdBase  - TODO: add argument description
  FdSize  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
GetImageReadFunction (
  IN PE_COFF_LOADER_IMAGE_CONTEXT          *ImageContext,
  IN EFI_PHYSICAL_ADDRESS                  *TopOfMemory
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ImageContext  - TODO: add argument description
  TopOfMemory   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
SecImageRead (
  IN     VOID    *FileHandle,
  IN     UINTN   FileOffset,
  IN OUT UINTN   *ReadSize,
  OUT    VOID    *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  FileHandle  - TODO: add argument description
  FileOffset  - TODO: add argument description
  ReadSize    - TODO: add argument description
  Buffer      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

CHAR16                            *
AsciiToUnicode (
  IN  CHAR8   *Ascii,
  IN  UINTN   *StrLen OPTIONAL
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Ascii   - TODO: add argument description
  StrLen  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

UINTN
CountSeperatorsInString (
  IN  CONST CHAR16   *String,
  IN  CHAR16   Seperator
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  String    - TODO: add argument description
  Seperator - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
SecTemporaryRamSupport (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  );


extern EFI_WIN_NT_THUNK_PROTOCOL  *gWinNt;
