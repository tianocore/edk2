/** @file
*
*  Copyright (c) 2014, ARM Limited. All rights reserved.
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

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/FdtLoadLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/SimpleFileSystem.h>

#include <Guid/Fdt.h>
#include <Guid/FileInfo.h>

#include <libfdt.h>

//
// Device path for SemiHosting
//
STATIC CONST struct {
  VENDOR_DEVICE_PATH        Guid;
  EFI_DEVICE_PATH_PROTOCOL  End;
} mSemihostingDevicePath = {
  {
    { HARDWARE_DEVICE_PATH, HW_VENDOR_DP, { sizeof (VENDOR_DEVICE_PATH), 0 } },
    { 0xC5B9C74A, 0x6D72, 0x4719, { 0x99, 0xAB, 0xC5, 0x9F, 0x19, 0x90, 0x91, 0xEB } }
  },
  { END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 } }
};


/**
  This function declares the passed FDT into the UEFI Configuration Table

  @param FdtBlob    Base address of the Fdt Blob in System Memory
  @param FdtSize    Size of the Fdt Blob in System Memory

  @return EFI_SUCCESS           Fdt Blob was successfully installed into the configuration table
  @return !EFI_SUCCESS          Error returned by BS.InstallConfigurationTable()

**/
STATIC
EFI_STATUS
InstallFdtIntoConfigurationTable (
  IN VOID* FdtBlob,
  IN UINTN FdtSize
  )
{
  EFI_STATUS Status;

  // Check the FDT header is valid. We only make this check in DEBUG mode in case the FDT header change on
  // production device and this ASSERT() becomes not valid.
  ASSERT (fdt_check_header (FdtBlob) == 0);

  // Ensure the Size of the Device Tree is smaller than the size of the read file
  ASSERT ((UINTN)fdt_totalsize (FdtBlob) <= FdtSize);

  // Install the FDT into the Configuration Table
  Status = gBS->InstallConfigurationTable (&gFdtTableGuid, FdtBlob);

  return Status;
}


/**
  Load and Install FDT from Semihosting

  @param Filename   Name of the file to load from semihosting

  @return EFI_SUCCESS           Fdt Blob was successfully installed into the configuration table
                                from semihosting
  @return EFI_NOT_FOUND         Fail to locate the file in semihosting
  @return EFI_OUT_OF_RESOURCES  Fail to allocate memory to contain the blob
**/
EFI_STATUS
InstallFdtFromSemihosting (
  IN  CONST CHAR16*   FileName
  )
{
  EFI_STATUS                       Status;
  EFI_DEVICE_PATH*                 Remaining;
  EFI_HANDLE                       Handle;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SemihostingFs;
  EFI_FILE_PROTOCOL               *Fs;
  EFI_FILE_PROTOCOL               *File;
  EFI_PHYSICAL_ADDRESS             FdtBase;
  EFI_FILE_INFO                   *FileInfo;
  UINTN                            FdtSize;
  UINTN                            FileInfoSize;

  // Ensure the Semihosting driver is initialized
  Remaining = (EFI_DEVICE_PATH*)&mSemihostingDevicePath;
  // The LocateDevicePath() function locates all devices on DevicePath that support Protocol and returns
  // the handle to the device that is closest to DevicePath. On output, the device path pointer is modified
  // to point to the remaining part of the device path
  Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &Remaining, &Handle);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Recursive = FALSE: We do not want to start the whole device tree
  Status = gBS->ConnectController (Handle, NULL, Remaining, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Locate the FileSystem
  Status = gBS->HandleProtocol (Handle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&SemihostingFs);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Try to Open the volume and get root directory
  Status = SemihostingFs->OpenVolume (SemihostingFs, &Fs);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_WARN, "Warning: Fail to open semihosting filesystem that should contain FDT file.\n"));
    return Status;
  }

  File = NULL;
  Status = Fs->Open (Fs, &File, (CHAR16*)FileName, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_WARN, "Warning: Fail to load FDT file '%s'.\n", FileName));
    Fs->Close (Fs);
    return Status;
  }

  FileInfoSize = 0;
  File->GetInfo (File, &gEfiFileInfoGuid, &FileInfoSize, NULL);
  FileInfo = AllocatePool (FileInfoSize);
  if (FileInfo == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto CLOSE_FILES;
  }
  Status = File->GetInfo (File, &gEfiFileInfoGuid, &FileInfoSize, FileInfo);
  if (EFI_ERROR (Status)) {
    FreePool (FileInfo);
    goto CLOSE_FILES;
  }

  // Get the file size
  FdtSize = FileInfo->FileSize;
  FreePool (FileInfo);

  // The FDT blob is attached to the Configuration Table. It is recommended to load it as Runtime Service Data
  // to prevent the kernel to overwrite its data
  Status = gBS->AllocatePages (AllocateAnyPages, EfiRuntimeServicesData, EFI_SIZE_TO_PAGES (FdtSize), &FdtBase);
  if (!EFI_ERROR (Status)) {
    Status = File->Read (File, &FdtSize, (VOID*)(UINTN)(FdtBase));
    if (EFI_ERROR (Status)) {
      gBS->FreePages (FdtBase, EFI_SIZE_TO_PAGES (FdtSize));
    } else {
      // Install the FDT as part of the UEFI Configuration Table
      Status = InstallFdtIntoConfigurationTable ((VOID*)(UINTN)FdtBase, FdtSize);
      if (EFI_ERROR (Status)) {
        gBS->FreePages (FdtBase, EFI_SIZE_TO_PAGES (FdtSize));
      }
    }
  }

CLOSE_FILES:
  File->Close (File);
  Fs->Close (Fs);
  return Status;
}

/**
  Load and Install FDT from Firmware Volume

  @param Filename   Guid of the FDT blob to load from firmware volume

  @return EFI_SUCCESS           Fdt Blob was successfully installed into the configuration table
                                from firmware volume
  @return EFI_NOT_FOUND         Fail to locate the file in firmware volume
  @return EFI_OUT_OF_RESOURCES  Fail to allocate memory to contain the blob
**/
EFI_STATUS
InstallFdtFromFv (
  IN  CONST EFI_GUID *FileName
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         NumberOfHandles;
  UINT32                        FvStatus;
  UINTN                         Index;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *FvInstance;
  INTN                          SectionInstance;
  UINTN                         FdtSize;
  VOID*                         FdtBlob;
  EFI_PHYSICAL_ADDRESS          FdtBase;

  FvStatus        = 0;
  SectionInstance = 0;

  // Locate all the Firmware Volume protocols.
  Status = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiFirmwareVolume2ProtocolGuid,
                   NULL,
                   &NumberOfHandles,
                   &HandleBuffer
                   );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Looking for FV that contains the FDT blob
  for (Index = 0; Index < NumberOfHandles; Index++) {
    //
    // Get the protocol on this handle
    // This should not fail because of LocateHandleBuffer
    //
    Status = gBS->HandleProtocol (
                     HandleBuffer[Index],
                     &gEfiFirmwareVolume2ProtocolGuid,
                     (VOID**) &FvInstance
                     );
    if (EFI_ERROR (Status)) {
      goto FREE_HANDLE_BUFFER;
    }

    while (Status == EFI_SUCCESS) {
      // FdtBlob must be allocated by ReadSection
      FdtBlob = NULL;

      // See if it contains the FDT file
      Status = FvInstance->ReadSection (
                        FvInstance,
                        FileName,
                        EFI_SECTION_RAW,
                        SectionInstance,
                        &FdtBlob,
                        &FdtSize,
                        &FvStatus
                        );
      if (!EFI_ERROR (Status)) {
        // When the FDT blob is attached to the Configuration Table it is recommended to load it as Runtime Service Data
        // to prevent the kernel to overwrite its data
        Status = gBS->AllocatePages (AllocateAnyPages, EfiRuntimeServicesData, EFI_SIZE_TO_PAGES (FdtSize), &FdtBase);
        if (EFI_ERROR (Status)) {
          goto FREE_HANDLE_BUFFER;
        }

        // Copy the FDT to the Runtime memory
        gBS->CopyMem ((VOID*)(UINTN)FdtBase, FdtBlob, FdtSize);
        // Free the buffer allocated by FvInstance->ReadSection()
        gBS->FreePool (FdtBlob);

        // Install the FDT as part of the UEFI Configuration Table
        Status = InstallFdtIntoConfigurationTable ((VOID*)(UINTN)FdtBase, FdtSize);
        if (EFI_ERROR (Status)) {
          gBS->FreePages (FdtBase, EFI_SIZE_TO_PAGES (FdtSize));
        }
        break;
      }
    }
  }

FREE_HANDLE_BUFFER:
  // Free any allocated buffers
  gBS->FreePool (HandleBuffer);

  return Status;
}
