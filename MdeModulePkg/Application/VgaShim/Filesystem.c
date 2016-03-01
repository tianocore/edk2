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


CHAR16*
GetFileName(
	IN	CHAR16	*FilePath)
{
	UINTN	SlashIndex;
	UINTN	FileNameLength;
	CHAR16	*FileName;

	// Find position of the last '\'.
	SlashIndex = StrLen(FilePath) - 1;
	while ((FilePath[SlashIndex] != L'\\') && (SlashIndex != 0)) {
		SlashIndex--;
	}
	
	if (SlashIndex == 0)
		return NULL;
	
	// Move index as we do not want the '\'.
	SlashIndex++;
	FileNameLength = StrLen(FilePath) - SlashIndex;

	// Allocate memory for file name and null terminator.
	FileName = (CHAR16*)AllocateZeroPool((FileNameLength + 1) * sizeof(CHAR16));
	if (FileName == NULL)
		return NULL;
	
	// Copy the relevant strings and add the null terminator.
	StrCpy(FileName, FilePath + SlashIndex);
	
	return FileName;
}


/**
  Checks whether a file located at a specified path exists on the 
  filesystem where the VgaShim executable is located.

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
	BOOLEAN					Exists;

	// Open volume where VgaShim lives.
	Status = gBS->HandleProtocol(VgaShimLoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void **)&Volume);
	if (EFI_ERROR(Status))
		return FALSE;
	Status = Volume->OpenVolume(Volume, &VolumeRoot);
	if (EFI_ERROR(Status))
		return FALSE;

	// Try to open file for reading.
	Status = VolumeRoot->Open(VolumeRoot, &RequestedFile, FilePath, EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(Status)) {
		VolumeRoot->Close(VolumeRoot);
		return FALSE;
	} else {
		RequestedFile->Close(RequestedFile);
		VolumeRoot->Close(VolumeRoot);
		return TRUE;
	}
}


/**
  Reads a file located at a specified path on the filesystem 
  where the VgaShim executable is located into a buffer.

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
	Status = gBS->HandleProtocol(VgaShimLoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void **)&Volume);
	if (EFI_ERROR(Status))
		return Status;
	Status = Volume->OpenVolume(Volume, &VolumeRoot);
	if (EFI_ERROR(Status))
		return Status;

	// Try to open file for reading.
	Status = VolumeRoot->Open(VolumeRoot, &File, FilePath, EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(Status)) {
		VolumeRoot->Close(VolumeRoot);
		return Status;
	}

	// First gather information on total file size.
	File->GetInfo(File, &gEfiFileInfoGuid, &Size, NULL);
	FileInfo = AllocatePool(Size);
	if (FileInfo == NULL)
		return EFI_OUT_OF_RESOURCES;
	File->GetInfo(File, &gEfiFileInfoGuid, &Size, FileInfo);
	Size = FileInfo->FileSize;
	FreePool(FileInfo);

	// Allocate a buffer and read the entire file into it.
	*FileContents = AllocatePool(Size);
	Print(L"%a: Reading %u bytes of %s ... ", __FUNCTION__, Size, FilePath);
	Status = File->Read(File, &Size, *FileContents);
	if (EFI_ERROR(Status)) {
		Print(L"unsuccessful (error: %r)\n", Status);
		FreePool(*FileContents);
	} else {
		Print(L"successful\n");
		*FileBytes = Size;
	}
	
	// Cleanup.
	File->Close(File);
	VolumeRoot->Close(VolumeRoot);
	return Status;
}


CHAR16*
GetMyLoadOptions()
{
	UINTN	SkipCharacters;
	UINTN	LoadOptionsLength;
	UINTN	LoadOptionsFullLength;
	CHAR16	*LoadOptions;
	CHAR16	*ExtensionLocation;

	//
	// Allocate memory for load options buffer.
	//
	LoadOptionsFullLength = StrLen(VgaShimLoadedImage->LoadOptions);
	LoadOptions = (CHAR16*)AllocateZeroPool((LoadOptionsFullLength + 1) * sizeof(CHAR16));
	if (LoadOptions == NULL)
		return NULL;
	
	//
	// Find out how many characters of path + file name to skip.
	//
	StrCpy(LoadOptions, VgaShimLoadedImage->LoadOptions);
	StrToLowercase(LoadOptions);
	ExtensionLocation = StrStr(LoadOptions, L".efi");
	if (ExtensionLocation == NULL) {
		FreePool(LoadOptions);
		return NULL;
	}
	*(ExtensionLocation + StrLen(L".efi")) = CHAR_NULL;
	SkipCharacters = StrLen(LoadOptions);

	//
	// Copy load options in the buffer skipping the file name and null-terminate.
	//
	LoadOptionsLength = StrLen(VgaShimLoadedImage->LoadOptions) - SkipCharacters;
	StrCpy(LoadOptions, ((CHAR16 *)VgaShimLoadedImage->LoadOptions) + SkipCharacters);

	return LoadOptions;
}


EFI_STATUS
Launch(
	IN	CHAR16	*FilePath)
{
	EFI_STATUS					Status;
	EFI_DEVICE_PATH_PROTOCOL	*FilePathOnDevice;
	EFI_HANDLE					FileImageHandle;
	EFI_LOADED_IMAGE_PROTOCOL	*FileImageInfo;
	CHAR16						*LoadOptions;
	CHAR16						*LoadOptionsWithPath;

	//
	// Construct load options based on mine.
	//
	LoadOptions = GetMyLoadOptions();
	if (LoadOptions == NULL) {
		return EFI_DEVICE_ERROR;
	}
	LoadOptionsWithPath = (CHAR16 *)AllocateZeroPool((StrLen(FilePath) + 1 + StrLen(LoadOptions)) * sizeof(CHAR16));
	LoadOptionsWithPath = StrCpy(LoadOptionsWithPath, FilePath);
	LoadOptionsWithPath = StrCat(LoadOptionsWithPath, L" ");
	LoadOptionsWithPath = StrCat(LoadOptionsWithPath, LoadOptions);
	Print(L"LoadOptions='%s'\n", LoadOptions);
	Print(L"LoadOptionsWithPath='%s'\n", LoadOptionsWithPath);
	FreePool(LoadOptions);

	//
	// Try to load the image first.
	//
	FilePathOnDevice = FileDevicePath(VgaShimLoadedImage->DeviceHandle, FilePath);
	Print(L"%a: Loading '%s' ... ", __FUNCTION__, ConvertDevicePathToText(FilePathOnDevice, TRUE, FALSE));
	Status = gBS->LoadImage(FALSE, VgaShimImage, FilePathOnDevice, NULL, 0, &FileImageHandle);
	if (EFI_ERROR(Status)) {
		Print(L"unsuccessful (error: %r)\n", Status);
	} else {
		Print(L"successful\n");
	}
	
	// 
	// Make sure this is a valid EFI loader.
	//
	gBS->HandleProtocol(FileImageHandle, &gEfiLoadedImageProtocolGuid, (VOID *)&FileImageInfo);
	if (EFI_ERROR(Status) || FileImageInfo->ImageCodeType != EfiLoaderCode) {
		gBS->UnloadImage(FileImageHandle);
		return EFI_UNSUPPORTED;
	}

	//Print(L"FileName='%s' %u\n", FileName, StrLen(FileName));
	//Print(L"LoadOptionsWithPath='%s'\n", LoadOptionsWithPath);
	//gBS->Exit(FileImageHandle, EFI_INVALID_PARAMETER, 0, NULL);
	//StrCpy (Destination + StrLen (Destination), Source);
	return EFI_SUCCESS;
}