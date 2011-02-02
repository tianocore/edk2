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

EFI_STATUS BdsLoadFileFromFirmwareVolume(
    IN  EFI_HANDLE      FvHandle,
    IN  CHAR16    *FilePath,
    IN  EFI_FV_FILETYPE FileTypeFilter,
    OUT BDS_FILE        *File
) {
    EFI_FIRMWARE_VOLUME2_PROTOCOL *FvProtocol;
    VOID                          *Key;    
    EFI_STATUS                    Status, FileStatus;
    EFI_GUID                      NameGuid;
    EFI_FV_FILETYPE               FileType;
    EFI_FV_FILE_ATTRIBUTES        Attributes;
    UINTN                         Size;
    UINTN                         UiStringLen;
    CHAR16                        *UiSection;
    UINT32                        Authentication;

    if (File == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    Status = gBS->HandleProtocol(FvHandle,&gEfiFirmwareVolume2ProtocolGuid, (VOID **)&FvProtocol);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    // Length of FilePath
    UiStringLen = StrLen (FilePath);

    // Allocate Key
    Key = AllocatePool (FvProtocol->KeySize);
    ASSERT (Key != NULL);
    ZeroMem (Key, FvProtocol->KeySize);

    do {
        // Search in all files
        FileType = FileTypeFilter;

        Status = FvProtocol->GetNextFile (FvProtocol, Key, &FileType, &NameGuid, &Attributes, &Size);
        if (!EFI_ERROR (Status)) {
            UiSection = NULL;
            FileStatus = FvProtocol->ReadSection (
                          FvProtocol, 
                          &NameGuid, 
                          EFI_SECTION_USER_INTERFACE, 
                          0,
                          (VOID **)&UiSection,
                          &Size,
                          &Authentication
                          );
            if (!EFI_ERROR (FileStatus)) {
              if (StrnCmp (FilePath, UiSection, UiStringLen) == 0) {
                  
                //
                // We found a UiString match. 
                //
                //*FileGuid = NameGuid;
                File->Type = BDS_FILETYPE_FV;
                File->FilePath = FilePath;
                CopyGuid (&(File->File.Fv.Guid),&NameGuid);
                File->File.Fv.FvProtocol = FvProtocol;
                File->File.Fv.FileType = FileType;

                Status = gBS->HandleProtocol(FvHandle,&gEfiDevicePathProtocolGuid, (VOID **)&(File->DevicePath));

                FreePool (Key);
                FreePool (UiSection);
                return FileStatus;
              }
              FreePool (UiSection);
            }
        }
    } while (!EFI_ERROR (Status));

    FreePool(Key);
    return Status;
}

EFI_STATUS BdsCopyRawFileToRuntimeMemoryFV(
    IN  BDS_FV_FILE         *FvFile,
    OUT VOID                **FileImage,
    OUT UINTN               *FileSize
) {
    EFI_STATUS Status = EFI_INVALID_PARAMETER;
    EFI_FIRMWARE_VOLUME2_PROTOCOL *FvProtocol;
    EFI_FV_FILETYPE               FileType;
    EFI_GUID*    NameGuid;
    EFI_FV_FILE_ATTRIBUTES        Attributes;
    UINT32                        Authentication;

    FvProtocol = FvFile->FvProtocol;
    FileType = FvFile->FileType;
    NameGuid = &(FvFile->Guid);

    if (FileType == EFI_FV_FILETYPE_RAW) {
        *FileImage = NULL;
        *FileSize = 0;
        Status = FvProtocol->ReadFile(
            FvProtocol,NameGuid,                            // IN
            FileImage,FileSize,                             // IN OUT
            &FileType,&Attributes,&Authentication           // OUT
        );
        ASSERT_EFI_ERROR(Status);

        // This raw file also contains a header
        *FileSize = *FileSize - 4;
        *FileImage = (UINT8*)FileImage + 4;
    } else if (FileType == EFI_FV_FILETYPE_FREEFORM) {
        Status = FvProtocol->ReadSection (
            FvProtocol,NameGuid,EFI_SECTION_RAW,0,          // IN
            FileImage,FileSize,                             // IN OUT
            &Authentication                                 // OUT
        );
        ASSERT_EFI_ERROR(Status);
    } else {
        ASSERT(0); //Maybe support application as well ???
    }

    return Status;
}
