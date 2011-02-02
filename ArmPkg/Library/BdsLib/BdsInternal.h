/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*  
*  This program and the accompanying materials                          
*  are licensed and made available under the terms and conditions of the BSD License         
*  which accompanies this distribution.  The full text of the license may be found at        
*  http://opensource.org/licenses/bsd-license.php                                            
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
*
**/

#ifndef __BDS_INTERNAL_H__
#define __BDS_INTERNAL_H__

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BdsUnixLib.h>

#include <Guid/FileInfo.h>

#include <Protocol/DevicePath.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/FirmwareVolume2.h>


typedef enum { BDS_FILETYPE_MEM, BDS_FILETYPE_FS, BDS_FILETYPE_FV } BDS_FILE_TYPE;

typedef struct {
    UINT32                  MemoryType;
    EFI_PHYSICAL_ADDRESS    StartingAddress;
    EFI_PHYSICAL_ADDRESS    EndingAddress;
} BDS_MEM_FILE;

typedef struct {
    EFI_FILE_PROTOCOL   *Handle;
} BDS_FS_FILE;

typedef struct {
    EFI_FIRMWARE_VOLUME2_PROTOCOL *FvProtocol;
    EFI_FV_FILETYPE               FileType;
    EFI_GUID            Guid;
} BDS_FV_FILE;

typedef struct _BDS_FILE {
    CHAR16* FilePath;
    EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
    BDS_FILE_TYPE Type;
    union {
        BDS_MEM_FILE    Mem;
        BDS_FS_FILE Fs;
        BDS_FV_FILE Fv;
    } File;
} BDS_FILE;

typedef struct _BDS_SYSTEM_MEMORY_RESOURCE {
    LIST_ENTRY                  Link; // This attribute must be the first entry of this structure (to avoid pointer computation)
    EFI_PHYSICAL_ADDRESS        PhysicalStart;
    UINT64                      ResourceLength;
} BDS_SYSTEM_MEMORY_RESOURCE;


// BdsHelper.c
EFI_STATUS
ShutdownUefiBootServices( VOID );

EFI_STATUS
GetSystemMemoryResources (LIST_ENTRY *ResourceList);

// BdsFilePath.c
EFI_STATUS BdsLoadDevicePath(
    IN  EFI_DEVICE_PATH_PROTOCOL* DevicePath,
    OUT EFI_HANDLE                *Handle
);

EFI_STATUS BdsLoadFilePath(
    IN  CONST CHAR16        *DeviceFilePath,
    OUT BDS_FILE            *File
);

EFI_STATUS BdsCopyRawFileToRuntimeMemory(
    IN  BDS_FILE            *File,
    OUT VOID                **FileImage,
    OUT UINTN               *FileSize
);

// BdsFilePathFs.c
EFI_STATUS BdsLoadFileFromSimpleFileSystem(
    IN  EFI_HANDLE Handle,
    IN  CHAR16                              *FilePath,
    OUT BDS_FILE            *File
);

EFI_STATUS BdsCopyRawFileToRuntimeMemoryFS(
    IN  EFI_FILE_PROTOCOL   *File,
    OUT VOID                **FileImage,
    OUT UINTN               *FileSize
);

// BdsFilePathFv.c
EFI_STATUS BdsLoadFileFromFirmwareVolume(
    IN  EFI_HANDLE          FvHandle,
    IN  CHAR16                              *FilePath,
    IN  EFI_FV_FILETYPE     FileTypeFilter,
    OUT BDS_FILE            *File
);

EFI_STATUS BdsCopyRawFileToRuntimeMemoryFV(
    IN  BDS_FV_FILE         *FvFile,
    OUT VOID                **FileImage,
    OUT UINTN               *FileSize
);

// BdsFilePathMem.c
EFI_STATUS BdsLoadFileFromMemMap (
    IN  EFI_HANDLE                  Handle,
    IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
    OUT BDS_FILE                    *File
);

EFI_STATUS BdsCopyRawFileToRuntimeMemoryMemMap(
    IN  BDS_MEM_FILE        *MemFile,
    OUT VOID                **FileImage,
    OUT UINTN               *FileSize
);

#endif
