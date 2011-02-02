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

// Count the number of DevicePath Node
static UINTN NumberNodeFromDevicePath(
    IN  EFI_DEVICE_PATH_PROTOCOL* DevicePath
) {
    UINTN NumberDevicePathNode = 0;

    while (!IsDevicePathEnd (DevicePath)) {
        NumberDevicePathNode++;
        DevicePath = NextDevicePathNode(DevicePath);
    }
    return NumberDevicePathNode;
}

// Extract the FilePath from the Device Path
CHAR16* BdsExtractFilePathFromDevicePath(
    IN  CONST CHAR16    *StrDevicePath,
    IN  UINTN           NumberDevicePathNode
) {
    UINTN       Node;
    CHAR16      *Str;

    Str = (CHAR16*)StrDevicePath;
    Node = 0;
    while ((Str != NULL) && (*Str != L'\0') && (Node < NumberDevicePathNode)) {
        if ((*Str == L'/') || (*Str == L'\\')) {
            Node++;
        }
        Str++;
    }

    if (*Str == L'\0') {
        return NULL;
    } else {
        return Str;
    }
}

EFI_STATUS
BdsLoadDevicePath(
    IN  EFI_DEVICE_PATH_PROTOCOL* DevicePath,
    OUT EFI_HANDLE                *Handle
) {
    EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath;
    EFI_STATUS                  Status;

    if ((DevicePath == NULL) || (Handle == NULL)) {
        return EFI_INVALID_PARAMETER;
    }

    do {
        RemainingDevicePath = DevicePath;
        // The LocateDevicePath() function locates all devices on DevicePath that support Protocol and returns
        // the handle to the device that is closest to DevicePath. On output, the device path pointer is modified
        // to point to the remaining part of the device path
        Status = gBS->LocateDevicePath(&gEfiDevicePathProtocolGuid,&RemainingDevicePath,Handle);
        if (!EFI_ERROR (Status)) {
            // Recursive = FALSE: We do not want to start all the device tree
            Status = gBS->ConnectController (*Handle, NULL, RemainingDevicePath, FALSE);
        }

        // We need to check if RemainingDevicePath does not point on the last node. Otherwise, calling
        // NextDevicePathNode() will return an undetermined Device Path Node
        if (!IsDevicePathEnd (RemainingDevicePath)) {
            RemainingDevicePath = NextDevicePathNode (RemainingDevicePath);
        }
    } while (!EFI_ERROR (Status) && !IsDevicePathEnd (RemainingDevicePath));

    if (!EFI_ERROR (Status)) {
        // Now, we have got the whole Device Path connected, call again ConnectController to ensure all the supported Driver
        // Binding Protocol are connected (such as DiskIo and SimpleFileSystem)
        RemainingDevicePath = DevicePath;
        Status = gBS->LocateDevicePath(&gEfiDevicePathProtocolGuid,&RemainingDevicePath,Handle);
        if (!EFI_ERROR (Status)) {
            Status = gBS->ConnectController (*Handle, NULL, RemainingDevicePath, FALSE);
            if (EFI_ERROR (Status)) {
                // If the last node is a Memory Map Device Path just return EFI_SUCCESS.
                if ((RemainingDevicePath->Type == HARDWARE_DEVICE_PATH) && (RemainingDevicePath->SubType == HW_MEMMAP_DP)) {
                    Status = EFI_SUCCESS;
                }
            }
        }
    } else if (IsDevicePathEnd (RemainingDevicePath)) {
        // Case when the DevicePath contains a MemoryMap Device Path Node and all drivers are connected.
        // Ensure the Device Path exists
        RemainingDevicePath = DevicePath;
        Status = gBS->LocateDevicePath(&gEfiDevicePathProtocolGuid,&RemainingDevicePath,Handle);
    }

    return Status;
}


EFI_STATUS
BdsLoadFilePath (
    IN  CONST CHAR16        *DeviceFilePath,
    OUT BDS_FILE            *File
) {
    EFI_DEVICE_PATH_PROTOCOL            *DevicePath;
    EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL  *EfiDevicePathFromTextProtocol;
    EFI_STATUS Status;
    EFI_HANDLE Handle;
    UINTN                               NumberDevicePathNode;
    CHAR16                              *FilePath;

    //Do a sanity check on the Device file path
    if (DeviceFilePath == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    //  Convert the Device Path String into Device Path Protocol
    Status = gBS->LocateProtocol(&gEfiDevicePathFromTextProtocolGuid, NULL, (VOID **)&EfiDevicePathFromTextProtocol);
    ASSERT_EFI_ERROR(Status);
    DevicePath = EfiDevicePathFromTextProtocol->ConvertTextToDevicePath(DeviceFilePath);

    //Do a sanity check on the Device Path
    if (DevicePath == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    // Count the number of DevicePath Node
    NumberDevicePathNode = NumberNodeFromDevicePath(DevicePath);
    // Extract the FilePath from the Device Path
    FilePath = BdsExtractFilePathFromDevicePath(DeviceFilePath,NumberDevicePathNode);

    Status = BdsLoadDevicePath(DevicePath,&Handle);
    if (EFI_ERROR (Status)) {
        return Status;
    }

    //If FilePath == NULL then let consider if a MemoryMap Device Path
    if (FilePath == NULL) {
        // Check if the Node is a MemoryMap Device Path
        Status = BdsLoadFileFromMemMap(Handle,DevicePath,File);
    } else {
        Status = BdsLoadFileFromSimpleFileSystem(Handle,FilePath,File);
        if (EFI_ERROR (Status)) {
            Status = BdsLoadFileFromFirmwareVolume(Handle,FilePath,EFI_FV_FILETYPE_ALL,File);
        }
    }

    if (!EFI_ERROR (Status)) {
        File->DevicePath = DevicePath;
    }

    return Status;
}

EFI_STATUS BdsCopyRawFileToRuntimeMemory(
    IN  BDS_FILE            *File,
    OUT VOID                **FileImage,
    OUT UINTN               *FileSize
) {
    if (File == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    if (File->Type == BDS_FILETYPE_FS) {
        return BdsCopyRawFileToRuntimeMemoryFS(File->File.Fs.Handle,FileImage,FileSize);
    } else if (File->Type == BDS_FILETYPE_FV) {
        return BdsCopyRawFileToRuntimeMemoryFV(&(File->File.Fv),FileImage,FileSize);
    } else if (File->Type == BDS_FILETYPE_MEM) {
        return BdsCopyRawFileToRuntimeMemoryMemMap(&(File->File.Mem),FileImage,FileSize);
    } else {
        return EFI_INVALID_PARAMETER;
    }
}
