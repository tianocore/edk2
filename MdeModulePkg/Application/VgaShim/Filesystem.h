#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__


/**
  -----------------------------------------------------------------------------
  Includes.
  -----------------------------------------------------------------------------
**/

#include <Uefi.h>
#include <Guid/FileInfo.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>


/**
  -----------------------------------------------------------------------------
  Exported method signatures.
  -----------------------------------------------------------------------------
**/

BOOLEAN
FileExists(
	IN	CHAR16	*FilePath);

EFI_STATUS
ChangeExtension(
	IN	CHAR16	*FilePath,
	IN	CHAR16	*NewExtension,
	OUT	VOID	**NewFilePath);

EFI_STATUS
FileRead(
	IN	CHAR16	*FilePath,
	OUT	VOID	**FileContents,
	OUT	UINTN	*FileBytes);

EFI_STATUS
Launch(
	IN	CHAR16	*FilePath);


/**
  -----------------------------------------------------------------------------
  Imported global variables.
  -----------------------------------------------------------------------------
**/

extern	EFI_HANDLE					VgaShimImage;
extern	EFI_LOADED_IMAGE_PROTOCOL	*VgaShimImageInfo;


#endif