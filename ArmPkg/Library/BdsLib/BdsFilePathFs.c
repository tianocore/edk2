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

EFI_STATUS BdsLoadFileFromSimpleFileSystem(
    IN  EFI_HANDLE Handle,
    IN  CHAR16                              *FilePath,
    OUT BDS_FILE            *File
) {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL     *FsProtocol;
    EFI_FILE_PROTOCOL                   *Fs;
    EFI_STATUS Status;
    EFI_FILE_PROTOCOL   *FileHandle = NULL;

    if (File == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    Status = gBS->HandleProtocol(Handle,&gEfiSimpleFileSystemProtocolGuid, (VOID **)&FsProtocol);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    //Try to Open the volume and get root directory
	Status = FsProtocol->OpenVolume(FsProtocol, &Fs);
	if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = Fs->Open(Fs, &FileHandle, FilePath, EFI_FILE_MODE_READ, 0);

    File->Type = BDS_FILETYPE_FS;
    File->FilePath = FilePath;
    File->File.Fs.Handle = FileHandle;

    return Status;
}

EFI_STATUS BdsCopyRawFileToRuntimeMemoryFS(
    IN  EFI_FILE_PROTOCOL   *File,
    OUT VOID                **FileImage,
    OUT UINTN               *FileSize
) {
    EFI_FILE_INFO   *FileInfo;
    UINTN           Size;
    VOID*           Image;
    EFI_STATUS      Status;

    Size = 0;
    File->GetInfo(File, &gEfiFileInfoGuid, &Size, NULL);
    FileInfo = AllocatePool (Size);
    Status = File->GetInfo(File, &gEfiFileInfoGuid, &Size, FileInfo);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    // Get the file size
    Size = FileInfo->FileSize;
    if (FileSize) {
        *FileSize = Size;
    }
    FreePool(FileInfo);

    Image = AllocateRuntimePool(Size);
    if (Image == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }

    Status = File->Read(File, &Size, Image);
    if (!EFI_ERROR(Status)) {
        *FileImage = Image;
    }
    return Status;
}
