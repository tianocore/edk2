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

#include "BdsInternal.h"


EFI_STATUS BdsLoadFileFromMemMap (
    IN  EFI_HANDLE                  Handle,
    IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
    OUT BDS_FILE                    *File
) {
    EFI_DEVICE_PATH_PROTOCOL            *LastDevicePath;

    if ((File == NULL) || (DevicePath == NULL) || (IsDevicePathEnd (DevicePath))) {
        return EFI_INVALID_PARAMETER;
    }

    // Check if the last node of the device Path is a Memory Map Device Node
    LastDevicePath = DevicePath;
    DevicePath = NextDevicePathNode(DevicePath);
    while (!IsDevicePathEnd (DevicePath)) {
        LastDevicePath = DevicePath;
        DevicePath = NextDevicePathNode(DevicePath);
    }
    if ((LastDevicePath->Type != HARDWARE_DEVICE_PATH) || (LastDevicePath->SubType != HW_MEMMAP_DP)) {
        return EFI_UNSUPPORTED;
    }

    File->Type = BDS_FILETYPE_MEM;
    File->File.Mem.MemoryType = ((MEMMAP_DEVICE_PATH*)LastDevicePath)->MemoryType;
    File->File.Mem.StartingAddress = ((MEMMAP_DEVICE_PATH*)LastDevicePath)->StartingAddress;
    File->File.Mem.EndingAddress = ((MEMMAP_DEVICE_PATH*)LastDevicePath)->EndingAddress;

    return EFI_SUCCESS;
}

EFI_STATUS BdsCopyRawFileToRuntimeMemoryMemMap(
    IN  BDS_MEM_FILE        *MemFile,
    OUT VOID                **FileImage,
    OUT UINTN               *FileSize
) {
    UINTN           Size;
    VOID*           Image;

    Size = MemFile->EndingAddress - MemFile->StartingAddress;

    if ((Size == 0) || (FileImage == NULL)) {
        return EFI_INVALID_PARAMETER;
    }
    if (FileSize != NULL) {
        *FileSize = Size;
    }

    Image = AllocateRuntimePool(Size);
    if (Image == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }

    *FileImage = CopyMem(Image,(CONST VOID*)(UINTN)MemFile->StartingAddress,Size);

    return EFI_SUCCESS;
}
