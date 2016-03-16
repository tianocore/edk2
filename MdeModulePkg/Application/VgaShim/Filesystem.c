#include "Filesystem.h"
#include "Util.h"


/**
  -----------------------------------------------------------------------------
  Exported method implementations.
  -----------------------------------------------------------------------------
**/

/**
  Creates a new string representing path to a file identical
  to the one specified as input but with a different extension.
  Extension lengths do not have to match, but have to be greater
  than zero.

  @param[in] FilePath     Pointer to a string representing a file
                          path that will be the base for the
						  new path.

  @param[in] NewExtension Pointer to a string representing new
                          file extension that will be swapped
						  for the original one.

  @param[out] NewFilePath Pointer to a memory location storing
                          a the location of the first character
						  in the resultant string.

  @retval EFI_SUCCESS     No problems were encountered during
                          execution.
  @retval other           The operation failed.
  
**/
EFI_STATUS
ChangeExtension(
	IN	CHAR16	*FilePath,
	IN	CHAR16	*NewExtension,
	OUT	VOID	**NewFilePath)
{
	UINTN	DotIndex;
	UINTN	NewExtensionLen = StrLen(NewExtension);

	*NewFilePath = 0;
	DotIndex = StrLen(FilePath) - 1;
	while ((FilePath[DotIndex] != L'.') && (DotIndex != 0)) {
		DotIndex--;
	}

	if (DotIndex == 0)
		return EFI_INVALID_PARAMETER;

	// Move index as we want to copy the '.' too.
	DotIndex++;

	// Allocate enough to hold the new extension and null terminator.
	*NewFilePath = (CHAR16*)AllocateZeroPool((DotIndex + NewExtensionLen + 1) * sizeof(CHAR16));
	if (*NewFilePath == NULL)
		return EFI_OUT_OF_RESOURCES;
	
	// Copy the relevant strings and add the null terminator.
	CopyMem((CHAR16 *)*NewFilePath, FilePath, (DotIndex) * sizeof(CHAR16));
	CopyMem(((CHAR16 *)*NewFilePath) + DotIndex, NewExtension, NewExtensionLen * sizeof(CHAR16));
	
	return EFI_SUCCESS;
}


/**
  Checks whether a file located at a specified path exists on the 
  filesystem where the VgaShim executable is located.

  Any error messages will only be printed on the debug console
  and only the error code returned to caller.

  @param[in] FilePath     Pointer to a string representing a file
                          path whose existence will be checked.

  @retval TRUE            File exists at the specified location.
  @retval FALSE           File does not exist or other problems
                          were encountered during execution.
  
**/
BOOLEAN
FileExists(
	IN	CHAR16*	FilePath)
{
	EFI_STATUS				Status;
	EFI_FILE_IO_INTERFACE	*Volume;
	EFI_FILE_HANDLE			VolumeRoot;
	EFI_FILE_HANDLE			RequestedFile;

	// Open volume where VgaShim lives.
	Status = gBS->HandleProtocol(VgaShimImageInfo->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void **)&Volume);
	if (EFI_ERROR(Status)) {
		PrintDebug(L"Unable to find simple file system protocol (error: %r)\n", Status);
		return Status;
	} else {
		PrintDebug(L"Found simple file system protocol\n");
	}
	Status = Volume->OpenVolume(Volume, &VolumeRoot);
	if (EFI_ERROR(Status)) {
		PrintDebug(L"Unable to open volume (error: %r)\n", Status);
		return Status;
	} else {
		PrintDebug(L"Opened volume\n");
	}

	// Try to open file for reading.
	Status = VolumeRoot->Open(VolumeRoot, &RequestedFile, FilePath, EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(Status)) {
		PrintDebug(L"Unable to open file '%s' for reading (error: %r)\n", FilePath, Status);
		VolumeRoot->Close(VolumeRoot);
		return FALSE;
	} else {
		PrintDebug(L"Opened file '%s' for reading\n", FilePath);
		RequestedFile->Close(RequestedFile);
		VolumeRoot->Close(VolumeRoot);
		return TRUE;
	}
}


/**
  Reads a file located at a specified path on the filesystem 
  where the VgaShim executable is located into a buffer.

  Any error messages will only be printed on the debug console
  and only the error code returned to caller.

  @param[in] FilePath      Pointer to a string representing a file
                           path that will be the base for the
						   new path.

  @param[out] FileContents Pointer to a memory location string
                           the contents of the file if no problems
						   were encountered over the course of
						   execution; NULL otherwise.

  @param[out] FileBytes    Number of bytes read off the disk if
                           no problems were encountered over
						   the course of execution; NULL otherwise.

  @retval EFI_SUCCESS      No problems were encountered over the
                           course of execution.
  @retval other            The operation failed.
  
**/
EFI_STATUS
FileRead(
	IN	CHAR16	*FilePath,
	OUT	VOID	**FileContents,
	OUT	UINTN	*FileBytes)
{
	EFI_STATUS				Status;
	EFI_FILE_IO_INTERFACE	*Volume;
	EFI_FILE_HANDLE			VolumeRoot;
	EFI_FILE_HANDLE			File;
	EFI_FILE_INFO			*FileInfo;
	UINTN					Size;

	// Open volume where VgaShim lives.
	Status = gBS->HandleProtocol(VgaShimImageInfo->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void **)&Volume);
	if (EFI_ERROR(Status)) {
		PrintDebug(L"Unable to find simple file system protocol (error: %r)\n", Status);
		goto Exit;
	} else {
		PrintDebug(L"Found simple file system protocol\n");
	}
		
	Status = Volume->OpenVolume(Volume, &VolumeRoot);
	if (EFI_ERROR(Status)) {
		PrintDebug(L"Unable to open volume (error: %r)\n", Status);
		goto Exit;
	} else {
		PrintDebug(L"Opened volume\n");
	}
	
	// Try to open file for reading.
	Status = VolumeRoot->Open(VolumeRoot, &File, FilePath, EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(Status)) {
		PrintDebug(L"Unable to open file '%s' for reading (error: %r)\n", FilePath, Status);
		goto Exit;
	} else {
		PrintDebug(L"Opened file '%s' for reading\n", FilePath);
	}

	// First gather information on total file size.
	File->GetInfo(File, &gEfiFileInfoGuid, &Size, NULL);
	FileInfo = AllocatePool(Size);
	if (FileInfo == NULL) {
		PrintDebug(L"Unable to allocate %u bytes for file info\n", Size);
		Status = EFI_OUT_OF_RESOURCES;
		goto Exit;
	} else {
		PrintDebug(L"Allocated %u bytes for file info\n", Size);
	}
	File->GetInfo(File, &gEfiFileInfoGuid, &Size, FileInfo);
	Size = FileInfo->FileSize;
	FreePool(FileInfo);

	// Allocate a buffer...
	*FileContents = AllocatePool(Size);
	if (*FileContents == NULL) {
		PrintDebug(L"Unable to allocate %u bytes for file contents\n", Size);
		Status = EFI_OUT_OF_RESOURCES;
		goto Exit;
	} else {
		PrintDebug(L"Allocated %u bytes for file contents\n", Size);
	}

	// ... and read the entire file into it.
	Status = File->Read(File, &Size, *FileContents);
	if (EFI_ERROR(Status)) {
		PrintDebug(L"Unable to read file contents (error: %r)\n", Status);
		goto Exit;
	} else {
		PrintDebug(L"Read file contents\n", Status);
		*FileBytes = Size;
	}

Exit:
	// Cleanup.
	if (EFI_ERROR(Status) && *FileContents != NULL) {
		FreePool(*FileContents);
		*FileContents = NULL;
	}
	if (File != NULL)
		File->Close(File);
	if (VolumeRoot != NULL)
		VolumeRoot->Close(VolumeRoot);
	return Status;
}


EFI_STATUS
Launch(
	IN	CHAR16	*FilePath,
	IN	VOID	(*WaitForEnterCallback)(BOOLEAN))
{
	EFI_STATUS					Status;
	EFI_DEVICE_PATH_PROTOCOL	*FilePathOnDevice;
	EFI_HANDLE					FileImageHandle;
	EFI_LOADED_IMAGE_PROTOCOL	*FileImageInfo;
	CHAR16						*FilePathOnDeviceText;

	//
	// Try to load the image first.
	//
	FilePathOnDevice = FileDevicePath(VgaShimImageInfo->DeviceHandle, FilePath);
	FilePathOnDeviceText = ConvertDevicePathToText(FilePathOnDevice, TRUE, FALSE);
	Status = gBS->LoadImage(TRUE, VgaShimImage, FilePathOnDevice, NULL, 0, &FileImageHandle);
	if (EFI_ERROR(Status)) {
		PrintError(L"Unable to load '%s' (error: %r)\n", FilePathOnDeviceText, Status);
	} else {
		PrintDebug(L"Loaded '%s'\n", FilePathOnDeviceText);
		PrintDebug(L"Addresss behind FileImageHandle=%x\n", FileImageHandle);
	}
	FreePool(FilePathOnDeviceText);
	
	// 
	// Make sure this is a valid EFI loader and fill in the options.
	//
	Status = gBS->HandleProtocol(FileImageHandle, &gEfiLoadedImageProtocolGuid, (VOID *)&FileImageInfo);
	if (EFI_ERROR(Status) || FileImageInfo->ImageCodeType != EfiLoaderCode) {
		PrintError(L"File does not match an EFI loader signature\n");
		gBS->UnloadImage(FileImageHandle);
		return EFI_UNSUPPORTED;
	} else {
		PrintDebug(L"File matches an EFI loader signature\n");
	}

	if (WaitForEnterCallback != NULL) {
		WaitForEnterCallback(TRUE);
	}
	
	//
	// Launch!
	//
	Status = gBS->StartImage(FileImageHandle, NULL, NULL);
	if (EFI_ERROR(Status)) {
		PrintError(L"Unable to start image (error: %r)\n", Status);
	}

	return Status;
}