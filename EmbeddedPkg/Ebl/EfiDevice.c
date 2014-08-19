/** @file
  EBL commands for EFI and PI Devices

  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ebl.h"


EFI_DXE_SERVICES  *gDS = NULL;


/**
  Print information about the File System device.

  @param  File  Open File for the device

**/
VOID
EblPrintFsInfo (
  IN  EFI_OPEN_FILE   *File
  )
{
  CHAR16 *Str;

  if (File == NULL) {
    return;
  }

  AsciiPrint ("  %a: ", File->DeviceName);
  if (File->FsInfo != NULL) {
    for (Str = File->FsInfo->VolumeLabel; *Str != '\0'; Str++) {
      if (*Str == ' ') {
        // UI makes you enter _ for space, so make the printout match that
        *Str = '_';
      }
      AsciiPrint ("%c", *Str);
    }
    AsciiPrint (":");
    if (File->FsInfo->ReadOnly) {
      AsciiPrint ("ReadOnly");
    }
  }

  AsciiPrint ("\n");
  EfiClose (File);
}


/**
  Print information about the FV devices.

  @param  File  Open File for the device

**/
VOID
EblPrintFvbInfo (
  IN  EFI_OPEN_FILE   *File
  )
{
  if (File == NULL) {
    return;
  }

  AsciiPrint ("  %a: 0x%08lx - 0x%08lx : 0x%08x\n", File->DeviceName, File->FvStart, File->FvStart + File->FvSize - 1, File->FvSize);
  EfiClose (File);
}


/**
  Print information about the Blk IO devices.
  If the device supports PXE dump out extra information

  @param  File  Open File for the device

**/
VOID
EblPrintBlkIoInfo (
  IN  EFI_OPEN_FILE   *File
  )
{
  UINT64                    DeviceSize;
  UINTN                     Index;
  UINTN                     Max;
  EFI_OPEN_FILE             *FsFile;

  if (File == NULL) {
    return;
  }

  AsciiPrint ("  %a: ", File->DeviceName);

  // print out name of file system, if any, on this block device
  Max = EfiGetDeviceCounts (EfiOpenFileSystem);
  if (Max != 0) {
    for (Index = 0; Index < Max; Index++) {
      FsFile = EfiDeviceOpenByType (EfiOpenFileSystem, Index);
      if (FsFile != NULL) {
        if (FsFile->EfiHandle == File->EfiHandle) {
          AsciiPrint ("fs%d: ", Index);
          EfiClose (FsFile);
          break;
        }
        EfiClose (FsFile);
      }
    }
  }

  // Print out useful Block IO media properties
  if (File->FsBlockIoMedia->RemovableMedia) {
    AsciiPrint ("Removable ");
  }
  if (!File->FsBlockIoMedia->MediaPresent) {
    AsciiPrint ("No Media\n");
  } else {
    if (File->FsBlockIoMedia->LogicalPartition) {
      AsciiPrint ("Partition ");
    }
    DeviceSize = MultU64x32 (File->FsBlockIoMedia->LastBlock + 1, File->FsBlockIoMedia->BlockSize);
    AsciiPrint ("Size = 0x%lX\n", DeviceSize);
  }
  EfiClose (File);
}

 /**
  Print information about the Load File devices.
  If the device supports PXE dump out extra information

  @param  File  Open File for the device

**/
VOID
EblPrintLoadFileInfo (
  IN  EFI_OPEN_FILE   *File
  )
{
  EFI_DEVICE_PATH_PROTOCOL    *DevicePathNode;
  MAC_ADDR_DEVICE_PATH        *MacAddr;
  UINTN                       HwAddressSize;
  UINTN                       Index;

  if (File == NULL) {
    return;
  }

  AsciiPrint ("  %a: %a ", File->DeviceName, EblLoadFileBootTypeString (File->EfiHandle));

  if (File->DevicePath != NULL) {
    // Try to print out the MAC address
    for (DevicePathNode = File->DevicePath;
        !IsDevicePathEnd (DevicePathNode);
        DevicePathNode = NextDevicePathNode (DevicePathNode)) {

      if ((DevicePathType (DevicePathNode) == MESSAGING_DEVICE_PATH) && (DevicePathSubType (DevicePathNode) == MSG_MAC_ADDR_DP)) {
        MacAddr = (MAC_ADDR_DEVICE_PATH *)DevicePathNode;

        HwAddressSize = sizeof (EFI_MAC_ADDRESS);
        if (MacAddr->IfType == 0x01 || MacAddr->IfType == 0x00) {
          HwAddressSize = 6;
        }

        AsciiPrint ("MAC ");
        for (Index = 0; Index < HwAddressSize; Index++) {
          AsciiPrint ("%02x", MacAddr->MacAddress.Addr[Index] & 0xff);
        }
      }
    }
  }

  AsciiPrint ("\n");
  EfiClose (File);
  return;
}



/**
  Dump information about devices in the system.

  fv:       PI Firmware Volume
  fs:       EFI Simple File System
  blk:      EFI Block IO
  LoadFile: EFI Load File Protocol (commonly PXE network boot)

  Argv[0] - "device"

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblDeviceCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  UINTN         Index;
  UINTN         CurrentRow;
  UINTN         Max;

  CurrentRow = 0;

  // Need to call here to make sure Device Counts are valid
  EblUpdateDeviceLists ();

  // Now we can print out the info...
  Max = EfiGetDeviceCounts (EfiOpenFirmwareVolume);
  if (Max != 0) {
    AsciiPrint ("Firmware Volume Devices:\n");
    for (Index = 0; Index < Max; Index++) {
      EblPrintFvbInfo (EfiDeviceOpenByType (EfiOpenFirmwareVolume, Index));
      if (EblAnyKeyToContinueQtoQuit (&CurrentRow, TRUE)) {
        break;
      }
    }
  }

  Max = EfiGetDeviceCounts (EfiOpenFileSystem);
  if (Max != 0) {
    AsciiPrint ("File System Devices:\n");
    for (Index = 0; Index < Max; Index++) {
      EblPrintFsInfo (EfiDeviceOpenByType (EfiOpenFileSystem, Index));
      if (EblAnyKeyToContinueQtoQuit (&CurrentRow, TRUE)) {
        break;
      }
    }
  }

  Max = EfiGetDeviceCounts (EfiOpenBlockIo);
  if (Max != 0) {
    AsciiPrint ("Block IO Devices:\n");
    for (Index = 0; Index < Max; Index++) {
      EblPrintBlkIoInfo (EfiDeviceOpenByType (EfiOpenBlockIo, Index));
      if (EblAnyKeyToContinueQtoQuit (&CurrentRow, TRUE)) {
        break;
      }
    }
  }

  Max = EfiGetDeviceCounts (EfiOpenLoadFile);
  if (Max != 0) {
    AsciiPrint ("LoadFile Devices: (usually network)\n");
    for (Index = 0; Index < Max; Index++) {
      EblPrintLoadFileInfo (EfiDeviceOpenByType (EfiOpenLoadFile, Index));
      if (EblAnyKeyToContinueQtoQuit (&CurrentRow, TRUE)) {
        break;
      }
    }
  }

  return EFI_SUCCESS;
}


/**
  Start an EFI image (PE32+ with EFI defined entry point).

  Argv[0] - "start"
  Argv[1] - device name and path
  Argv[2] - "" string to pass into image being started

  start fs1:\Temp\Fv.Fv "arg to pass" ; load an FV from the disk and pass the
                                      ; ascii string arg to pass to the image
  start fv0:\FV                       ; load an FV from an FV (not common)
  start LoadFile0:                    ; load an FV via a PXE boot

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblStartCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  EFI_STATUS                  Status;
  EFI_OPEN_FILE               *File;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  EFI_HANDLE                  ImageHandle;
  UINTN                       ExitDataSize;
  CHAR16                      *ExitData;
  VOID                        *Buffer;
  UINTN                       BufferSize;
  EFI_LOADED_IMAGE_PROTOCOL   *ImageInfo;

  ImageHandle = NULL;

  if (Argc < 2) {
    return EFI_INVALID_PARAMETER;
  }

  File = EfiOpen (Argv[1], EFI_FILE_MODE_READ, 0);
  if (File == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DevicePath = File->DevicePath;
  if (DevicePath != NULL) {
    // check for device path form: blk, fv, fs, and loadfile
    Status = gBS->LoadImage (FALSE, gImageHandle, DevicePath, NULL, 0, &ImageHandle);
  } else {
    // Check for buffer form: A0x12345678:0x1234 syntax.
    // Means load using buffer starting at 0x12345678 of size 0x1234.

    Status = EfiReadAllocatePool (File, &Buffer, &BufferSize);
    if (EFI_ERROR (Status)) {
      EfiClose (File);
      return Status;
    }
    Status = gBS->LoadImage (FALSE, gImageHandle, DevicePath, Buffer, BufferSize, &ImageHandle);

    FreePool (Buffer);
  }

  EfiClose (File);

  if (!EFI_ERROR (Status)) {
    if (Argc >= 3) {
      // Argv[2] is a "" string that we pass directly to the EFI application without the ""
      // We don't pass Argv[0] to the EFI Application (it's name) just the args
      Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&ImageInfo);
      ASSERT_EFI_ERROR (Status);

      ImageInfo->LoadOptionsSize = (UINT32)AsciiStrSize (Argv[2]);
      ImageInfo->LoadOptions     = AllocatePool (ImageInfo->LoadOptionsSize);
      AsciiStrCpy (ImageInfo->LoadOptions, Argv[2]);
    }

    // Transfer control to the EFI image we loaded with LoadImage()
    Status = gBS->StartImage (ImageHandle, &ExitDataSize, &ExitData);
  }

  return Status;
}


/**
  Load a Firmware Volume (FV) into memory from a device. This causes drivers in
  the FV to be dispatched if the dependencies of the drivers are met.

  Argv[0] - "loadfv"
  Argv[1] - device name and path

  loadfv fs1:\Temp\Fv.Fv ; load an FV from the disk
  loadfv fv0:\FV         ; load an FV from an FV (not common)
  loadfv LoadFile0:      ; load an FV via a PXE boot

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblLoadFvCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  EFI_STATUS                        Status;
  EFI_OPEN_FILE                     *File;
  VOID                              *FvStart;
  UINTN                             FvSize;
  EFI_HANDLE                        FvHandle;


  if (Argc < 2) {
    return EFI_INVALID_PARAMETER;
  }

  File = EfiOpen (Argv[1], EFI_FILE_MODE_READ, 0);
  if (File == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (File->Type == EfiOpenMemoryBuffer) {
    // If it is a address just use it.
    Status = gDS->ProcessFirmwareVolume (File->Buffer, File->Size, &FvHandle);
  } else {
    // If it is a file read it into memory and use it
    Status = EfiReadAllocatePool (File, &FvStart, &FvSize);
    EfiClose (File);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = gDS->ProcessFirmwareVolume (FvStart, FvSize, &FvHandle);
    if (EFI_ERROR (Status)) {
      FreePool (FvStart);
    }
  }
  return Status;
}


/**
  Perform an EFI connect to connect devices that follow the EFI driver model.
  If it is a PI system also call the dispatcher in case a new FV was made
  available by one of the connect EFI drivers (this is not a common case).

  Argv[0] - "connect"

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblConnectCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  EFI_STATUS    Status;
  UINTN         HandleCount;
  EFI_HANDLE    *HandleBuffer;
  UINTN         Index;
  BOOLEAN       Dispatch;
  EFI_OPEN_FILE *File;


  if (Argc > 1) {
    if ((*Argv[1] == 'd') || (*Argv[1] == 'D')) {
      Status = gBS->LocateHandleBuffer (
                      AllHandles,
                      NULL,
                      NULL,
                      &HandleCount,
                      &HandleBuffer
                      );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      for (Index = 0; Index < HandleCount; Index++) {
        gBS->DisconnectController (HandleBuffer[Index], NULL, NULL);
      }

      //
      // Given we disconnect our console we should go and do a connect now
      //
    } else {
      File = EfiOpen (Argv[1], EFI_FILE_MODE_READ, 0);
      if (File != NULL) {
        AsciiPrint ("Connecting %a\n", Argv[1]);
        gBS->ConnectController (File->EfiHandle, NULL, NULL, TRUE);
        EfiClose (File);
        return EFI_SUCCESS;
      }
    }
  }

  Dispatch = FALSE;
  do {
    Status = gBS->LocateHandleBuffer (
                    AllHandles,
                    NULL,
                    NULL,
                    &HandleCount,
                    &HandleBuffer
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    for (Index = 0; Index < HandleCount; Index++) {
      gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
    }

    FreePool (HandleBuffer);

    //
    // Check to see if it's possible to dispatch an more DXE drivers.
    // The BdsLibConnectAllEfi () may have made new DXE drivers show up.
    // If anything is Dispatched Status == EFI_SUCCESS and we will try
    // the connect again.
    //
    if (gDS == NULL) {
      Status = EFI_NOT_FOUND;
    } else {
      Status = gDS->Dispatch ();
      if (!EFI_ERROR (Status)) {
        Dispatch = TRUE;
      }
    }

  } while (!EFI_ERROR (Status));

  if (Dispatch) {
    AsciiPrint ("Connected and dispatched\n");
  } else {
    AsciiPrint ("Connect\n");
  }

  return EFI_SUCCESS;
}



CHAR8 *gMemMapType[] = {
  "reserved  ",
  "LoaderCode",
  "LoaderData",
  "BS_code   ",
  "BS_data   ",
  "RT_code   ",
  "RT_data   ",
  "available ",
  "Unusable  ",
  "ACPI_recl ",
  "ACPI_NVS  ",
  "MemMapIO  ",
  "MemPortIO ",
  "PAL_code  "
};


/**
  Dump out the EFI memory map

  Argv[0] - "memmap"

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblMemMapCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  EFI_STATUS            Status;
  EFI_MEMORY_DESCRIPTOR *MemMap;
  EFI_MEMORY_DESCRIPTOR *OrigMemMap;
  UINTN                 MemMapSize;
  UINTN                 MapKey;
  UINTN                 DescriptorSize;
  UINT32                DescriptorVersion;
  UINT64                PageCount[EfiMaxMemoryType];
  UINTN                 Index;
  UINT64                EntrySize;
  UINTN                 CurrentRow;
  UINT64                TotalMemory;

  ZeroMem (PageCount, sizeof (PageCount));

  AsciiPrint ("EFI Memory Map\n");

  // First call is to figure out how big the buffer needs to be
  MemMapSize = 0;
  MemMap     = NULL;
  Status = gBS->GetMemoryMap (&MemMapSize, MemMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    // In case the AllocatPool changes the memory map we added in some extra descriptors
    MemMapSize += (DescriptorSize * 0x100);
    OrigMemMap = MemMap = AllocatePool (MemMapSize);
    if (OrigMemMap != NULL) {
      // 2nd time we get the data
      Status = gBS->GetMemoryMap (&MemMapSize, MemMap, &MapKey, &DescriptorSize, &DescriptorVersion);
      if (!EFI_ERROR (Status)) {
        for (Index = 0, CurrentRow = 0; Index < MemMapSize/DescriptorSize; Index++) {
          EntrySize = LShiftU64 (MemMap->NumberOfPages, 12);
          AsciiPrint ("\n%a %016lx - %016lx: # %08lx %016lx", gMemMapType[MemMap->Type % EfiMaxMemoryType], MemMap->PhysicalStart, MemMap->PhysicalStart + EntrySize -1, MemMap->NumberOfPages, MemMap->Attribute);
          if (EblAnyKeyToContinueQtoQuit (&CurrentRow, TRUE)) {
            break;
          }

          PageCount[MemMap->Type % EfiMaxMemoryType] += MemMap->NumberOfPages;
          MemMap = NEXT_MEMORY_DESCRIPTOR (MemMap, DescriptorSize);
        }
      }

      for (Index = 0, TotalMemory = 0; Index < EfiMaxMemoryType; Index++) {
        if (PageCount[Index] != 0) {
          AsciiPrint ("\n  %a %,7ld Pages (%,14ld)", gMemMapType[Index], PageCount[Index], LShiftU64 (PageCount[Index], 12));
          if (Index == EfiLoaderCode ||
              Index == EfiLoaderData ||
              Index == EfiBootServicesCode ||
              Index == EfiBootServicesData ||
              Index == EfiRuntimeServicesCode ||
              Index == EfiRuntimeServicesData ||
              Index == EfiConventionalMemory ||
              Index == EfiACPIReclaimMemory ||
              Index == EfiACPIMemoryNVS ||
              Index == EfiPalCode
          ) {
            // Count total memory
            TotalMemory += PageCount[Index];
          }
        }
      }

      AsciiPrint ("\nTotal Memory: %,ld MB (%,ld bytes)\n", RShiftU64 (TotalMemory, 8), LShiftU64 (TotalMemory, 12));

      FreePool (OrigMemMap);

    }
  }

  return EFI_SUCCESS;
}




/**
  Load a file into memory and optionally jump to it. A load address can be
  specified or automatically allocated. A quoted command line can optionally
  be passed into the image.

  Argv[0] - "go"
  Argv[1] - Device Name:path for the file to load
  Argv[2] - Address to load to or '*' if the load address will be allocated
  Argv[3] - Optional Entry point to the image. Image will be called if present
  Argv[4] - "" string that will be passed as Argc & Argv to EntryPoint. Needs
            to include the command name

  go fv1:\EblCmdX  0x10000  0x10010 "EblCmdX Arg2 Arg3 Arg4"; - load EblCmdX
    from FV1 to location 0x10000 and call the entry point at 0x10010 passing
    in "EblCmdX Arg2 Arg3 Arg4" as the arguments.

  go fv0:\EblCmdX  *  0x10 "EblCmdX Arg2 Arg3 Arg4"; - load EblCmdX from FS0
    to location allocated by this command and call the entry point at offset 0x10
    passing in "EblCmdX Arg2 Arg3 Arg4" as the arguments.

  go fv1:\EblCmdX  0x10000; Load EblCmdX to address 0x10000 and return

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblGoCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  EFI_STATUS                    Status;
  EFI_OPEN_FILE                 *File;
  VOID                          *Address;
  UINTN                         Size;
  EBL_COMMMAND                  EntryPoint;
  UINTN                         EntryPointArgc;
  CHAR8                         *EntryPointArgv[MAX_ARGS];


  if (Argc <= 2) {
    // device name and laod address are required
    return EFI_SUCCESS;
  }

  File = EfiOpen (Argv[1], EFI_FILE_MODE_READ, 0);
  if (File == NULL) {
    AsciiPrint ("  %a is not a valid path\n", Argv[1]);
    return EFI_SUCCESS;
  }

  EntryPoint  = (EBL_COMMMAND)((Argc > 3) ? (UINTN)AsciiStrHexToUintn (Argv[3]) : (UINTN)NULL);
  if (Argv[2][0] == '*') {
    // * Means allocate the buffer
    Status = EfiReadAllocatePool (File, &Address, &Size);

    // EntryPoint is relative to the start of the image
    EntryPoint = (EBL_COMMMAND)((UINTN)EntryPoint + (UINTN)Address);

  } else {
    Address = (VOID *)AsciiStrHexToUintn (Argv[2]);
    Size = File->Size;

    // File->Size for LoadFile is lazy so we need to use the tell to figure it out
    EfiTell (File, NULL);
    Status = EfiRead (File, Address, &Size);
  }

  if (!EFI_ERROR (Status)) {
    AsciiPrint ("Loaded %,d bytes to 0x%08x\n", Size, Address);

    if (Argc > 3) {
      if (Argc > 4) {
        ParseArguments (Argv[4], &EntryPointArgc, EntryPointArgv);
      } else {
        EntryPointArgc = 1;
        EntryPointArgv[0] = File->FileName;
      }

      Status = EntryPoint (EntryPointArgc, EntryPointArgv);
    }
  }

  EfiClose (File);
  return Status;
}

#define FILE_COPY_CHUNK 0x20000

EFI_STATUS
EblFileCopyCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  EFI_OPEN_FILE *Source      = NULL;
  EFI_OPEN_FILE *Destination = NULL;
  EFI_STATUS    Status       = EFI_SUCCESS;
  VOID          *Buffer      = NULL;
  UINTN         Size;
  UINTN         Offset;
  UINTN         Chunk        = FILE_COPY_CHUNK;
  UINTN         FileNameLen;
  CHAR8*        DestFileName;
  CHAR8*        SrcFileName;
  CHAR8*        SrcPtr;

  if (Argc < 3) {
    return EFI_INVALID_PARAMETER;
  }

  DestFileName = Argv[2];
  FileNameLen = AsciiStrLen (DestFileName);

  // Check if the destination file name looks like a directory
  if ((DestFileName[FileNameLen-1] == '\\') || (DestFileName[FileNameLen-1] == ':')) {
    // Set the pointer after the source drive (eg: after fs1:)
    SrcPtr = AsciiStrStr (Argv[1], ":");
    if (SrcPtr == NULL) {
      SrcPtr = Argv[1];
    } else {
      SrcPtr++;
      if (*SrcPtr == '\\') {
        SrcPtr++;
      }
    }

    if (*SrcPtr == '\0') {
      AsciiPrint("Source file incorrect.\n");
    }

    // Skip the Source Directories
    while (1) {
      SrcFileName = SrcPtr;
      SrcPtr = AsciiStrStr (SrcPtr,"\\");
      if (SrcPtr != NULL) {
        SrcPtr++;
      } else {
        break;
      }
    }

    if (*SrcFileName == '\0') {
      AsciiPrint("Source file incorrect (Error 2).\n");
    }

    // Construct the destination filepath
    DestFileName = (CHAR8*)AllocatePool (FileNameLen + AsciiStrLen (SrcFileName) + 1);
    AsciiStrCpy (DestFileName, Argv[2]);
    AsciiStrCat (DestFileName, SrcFileName);
  }

  Source = EfiOpen(Argv[1], EFI_FILE_MODE_READ, 0);
  if (Source == NULL) {
    AsciiPrint("Source file open error.\n");
    return EFI_NOT_FOUND;
  }

  Destination = EfiOpen(DestFileName, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
  if (Destination == NULL) {
    AsciiPrint("Destination file open error.\n");
    return EFI_NOT_FOUND;
  }

  Buffer = AllocatePool(FILE_COPY_CHUNK);
  if (Buffer == NULL) {
    goto Exit;
  }

  Size = EfiTell(Source, NULL);

  for (Offset = 0; Offset + FILE_COPY_CHUNK <= Size; Offset += Chunk) {
    Chunk = FILE_COPY_CHUNK;

    Status = EfiRead(Source, Buffer, &Chunk);
    if (EFI_ERROR(Status)) {
      AsciiPrint("Read file error %r\n", Status);
      goto Exit;
    }

    Status = EfiWrite(Destination, Buffer, &Chunk);
    if (EFI_ERROR(Status)) {
      AsciiPrint("Write file error %r\n", Status);
      goto Exit;
    }
  }

  // Any left over?
  if (Offset < Size) {
    Chunk = Size - Offset;

    Status = EfiRead(Source, Buffer, &Chunk);
    if (EFI_ERROR(Status)) {
      AsciiPrint("Read file error %r\n", Status);
      goto Exit;
    }

    Status = EfiWrite(Destination, Buffer, &Chunk);
    if (EFI_ERROR(Status)) {
      AsciiPrint("Write file error %r\n", Status);
      goto Exit;
    }
  }


Exit:
  if (Source != NULL) {
    Status = EfiClose(Source);
    if (EFI_ERROR(Status)) {
      AsciiPrint("Source close error %r\n", Status);
    }
  }
  if (Destination != NULL) {
    Status = EfiClose(Destination);
    if (EFI_ERROR(Status)) {
      AsciiPrint("Destination close error %r\n", Status);
    }

    // Case when we have concated the filename to the destination directory
    if (DestFileName != Argv[2]) {
      FreePool (DestFileName);
    }
  }

  if (Buffer != NULL) {
    FreePool(Buffer);
  }

  return Status;
}

EFI_STATUS
EblFileDiffCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  EFI_OPEN_FILE *File1   = NULL;
  EFI_OPEN_FILE *File2   = NULL;
  EFI_STATUS    Status   = EFI_SUCCESS;
  VOID          *Buffer1 = NULL;
  VOID          *Buffer2 = NULL;
  UINTN         Size1;
  UINTN         Size2;
  UINTN         Offset;
  UINTN         Chunk   = FILE_COPY_CHUNK;

  if (Argc != 3) {
    return EFI_INVALID_PARAMETER;
  }

  File1 = EfiOpen(Argv[1], EFI_FILE_MODE_READ, 0);
  if (File1 == NULL) {
    AsciiPrint("File 1 open error.\n");
    return EFI_NOT_FOUND;
  }

  File2 = EfiOpen(Argv[2], EFI_FILE_MODE_READ, 0);
  if (File2 == NULL) {
    AsciiPrint("File 2 open error.\n");
    return EFI_NOT_FOUND;
  }

  Size1 = EfiTell(File1, NULL);
  Size2 = EfiTell(File2, NULL);

  if (Size1 != Size2) {
    AsciiPrint("Files differ.\n");
    goto Exit;
  }

  Buffer1 = AllocatePool(FILE_COPY_CHUNK);
  if (Buffer1 == NULL) {
    goto Exit;
  }

  Buffer2 = AllocatePool(FILE_COPY_CHUNK);
  if (Buffer2 == NULL) {
    goto Exit;
  }

  for (Offset = 0; Offset + FILE_COPY_CHUNK <= Size1; Offset += Chunk) {
    Chunk = FILE_COPY_CHUNK;

    Status = EfiRead(File1, Buffer1, &Chunk);
    if (EFI_ERROR(Status)) {
      AsciiPrint("File 1 read error\n");
      goto Exit;
    }

    Status = EfiRead(File2, Buffer2, &Chunk);
    if (EFI_ERROR(Status)) {
      AsciiPrint("File 2 read error\n");
      goto Exit;
    }

    if (CompareMem(Buffer1, Buffer2, Chunk) != 0) {
      AsciiPrint("Files differ.\n");
      goto Exit;
    };
  }

  // Any left over?
  if (Offset < Size1) {
    Chunk = Size1 - Offset;

    Status = EfiRead(File1, Buffer1, &Chunk);
    if (EFI_ERROR(Status)) {
      AsciiPrint("File 1 read error\n");
      goto Exit;
    }

    Status = EfiRead(File2, Buffer2, &Chunk);
    if (EFI_ERROR(Status)) {
      AsciiPrint("File 2 read error\n");
      goto Exit;
    }
  }

  if (CompareMem(Buffer1, Buffer2, Chunk) != 0) {
    AsciiPrint("Files differ.\n");
  } else {
    AsciiPrint("Files are identical.\n");
  }

Exit:
  if (File1 != NULL) {
    Status = EfiClose(File1);
    if (EFI_ERROR(Status)) {
      AsciiPrint("File 1 close error %r\n", Status);
    }
  }

  if (File2 != NULL) {
    Status = EfiClose(File2);
    if (EFI_ERROR(Status)) {
      AsciiPrint("File 2 close error %r\n", Status);
    }
  }

  if (Buffer1 != NULL) {
    FreePool(Buffer1);
  }

  if (Buffer2 != NULL) {
    FreePool(Buffer2);
  }

  return Status;
}

GLOBAL_REMOVE_IF_UNREFERENCED const EBL_COMMAND_TABLE mCmdDeviceTemplate[] =
{
  {
    "connect",
    "[d]; Connect all EFI devices. d means disconnect",
    NULL,
    EblConnectCmd
  },
  {
    "device",
    "; Show information about boot devices",
    NULL,
    EblDeviceCmd
  },
  {
    "go",
    " dev:path loadaddress entrypoint args; load to given address and jump in",
    NULL,
    EblGoCmd
  },
  {
    "loadfv",
    " devname; Load PI FV from device",
    NULL,
    EblLoadFvCmd
  },
  {
    "start",
    " path; EFI Boot Device:filepath. fs1:\\EFI\\BOOT.EFI",
    NULL,
    EblStartCmd
  },
  {
    "memmap",
    "; dump EFI memory map",
    NULL,
    EblMemMapCmd
  },
  {
    "cp",
    " file1 file2; copy file only.",
    NULL,
    EblFileCopyCmd
  },
  {
    "diff",
    " file1 file2; compare files",
    NULL,
    EblFileDiffCmd
  }
};


/**
  Initialize the commands in this in this file
**/

VOID
EblInitializeDeviceCmd (
  VOID
  )
{
  EfiGetSystemConfigurationTable (&gEfiDxeServicesTableGuid, (VOID **) &gDS);
  EblAddCommands (mCmdDeviceTemplate, sizeof (mCmdDeviceTemplate)/sizeof (EBL_COMMAND_TABLE));
}

