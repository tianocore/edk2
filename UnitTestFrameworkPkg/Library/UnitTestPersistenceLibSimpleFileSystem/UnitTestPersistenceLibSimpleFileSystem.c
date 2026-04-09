/** @file
  This is an instance of the Unit Test Persistence Lib that will utilize
  the filesystem that a test application is running from to save a serialized
  version of the internal test state in case the test needs to quit and restore.

  Copyright (c) Microsoft Corporation.<BR>
  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Library/UnitTestPersistenceLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiLib.h>
#include <Protocol/LoadedImage.h>
#include <UnitTestFrameworkTypes.h>

#define CACHE_FILE_SUFFIX  L"_Cache.dat"

CHAR16  *mCachePath = NULL;

/**
  Generate the file name and path to the cache file.

  @param[in]  FrameworkHandle  A pointer to the framework that is being persisted.

  @retval  !NULL  A pointer to the EFI_FILE protocol instance for the filesystem.
  @retval  NULL   Filesystem could not be found or an error occurred.

**/
STATIC
CHAR16 *
GetCacheFileName (
  IN UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle
  )
{
  EFI_STATUS                 Status;
  UNIT_TEST_FRAMEWORK        *Framework;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  CHAR16                     *AppPath;
  CHAR16                     *CacheFilePath;
  CHAR16                     *TestName;
  UINTN                      DirectorySlashOffset;
  UINTN                      CacheFilePathLength;

  Framework     = (UNIT_TEST_FRAMEWORK *)FrameworkHandle;
  AppPath       = NULL;
  CacheFilePath = NULL;
  TestName      = NULL;

  //
  // First, we need to get some information from the loaded image.
  //
  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "%a - Failed to locate DevicePath for loaded image. %r\n", __func__, Status));
    return NULL;
  }

  //
  // Before we can start, change test name from ASCII to Unicode.
  //
  CacheFilePathLength = AsciiStrLen (Framework->ShortTitle) + 1;
  TestName            = AllocatePool (CacheFilePathLength * sizeof (CHAR16));
  if (!TestName) {
    goto Exit;
  }

  AsciiStrToUnicodeStrS (Framework->ShortTitle, TestName, CacheFilePathLength);

  //
  // Now we should have the device path of the root device and a file path for the rest.
  // In order to target the directory for the test application, we must process
  // the file path a little.
  //
  // NOTE: This may not be necessary... Path processing functions exist...
  // PathCleanUpDirectories (FileNameCopy);
  //     if (PathRemoveLastItem (FileNameCopy)) {
  //
  if (mCachePath == NULL) {
    AppPath = ConvertDevicePathToText (LoadedImage->FilePath, TRUE, TRUE); // NOTE: This must be freed.
    if (AppPath == NULL) {
      goto Exit;
    }

    DirectorySlashOffset = StrLen (AppPath);
    //
    // Make sure we didn't get any weird data.
    //
    if (DirectorySlashOffset == 0) {
      DEBUG ((DEBUG_ERROR, "%a - Weird 0-length string when processing app path.\n", __func__));
      goto Exit;
    }

    //
    // Now that we know we have a decent string, let's take a deeper look.
    //
    do {
      if (AppPath[DirectorySlashOffset] == L'\\') {
        break;
      }

      DirectorySlashOffset--;
    } while (DirectorySlashOffset > 0);

    //
    // After that little maneuver, DirectorySlashOffset should be pointing at the last '\' in AppString.
    // That would be the path to the parent directory that the test app is executing from.
    // Let's check and make sure that's right.
    //
    if (AppPath[DirectorySlashOffset] != L'\\') {
      DEBUG ((DEBUG_ERROR, "%a - Could not find a single directory separator in app path.\n", __func__));
      goto Exit;
    }
  } else {
    AppPath = FullyQualifyPath (mCachePath); // NOTE: This must be freed.
    if (AppPath == NULL) {
      goto Exit;
    }

    DirectorySlashOffset = StrLen (AppPath);

    if (AppPath[DirectorySlashOffset - 1] != L'\\') {
      // Set the slash if user did not specify it on the newly allocated pool
      AppPath = ReallocatePool (
                  (DirectorySlashOffset + 1) * sizeof (CHAR16),
                  (DirectorySlashOffset + 2) * sizeof (CHAR16),
                  AppPath
                  );
      AppPath[DirectorySlashOffset]     = L'\\';
      AppPath[DirectorySlashOffset + 1] = L'\0';
    } else {
      // Otherwise the user input is good enough to go, mostly
      DirectorySlashOffset--;
    }
  }

  //
  // Now we know some things, we're ready to produce our output string, I think.
  //
  CacheFilePathLength  = DirectorySlashOffset + 1;
  CacheFilePathLength += StrLen (TestName);
  CacheFilePathLength += StrLen (CACHE_FILE_SUFFIX);
  CacheFilePathLength += 1;   // Don't forget the NULL terminator.
  CacheFilePath        = AllocateZeroPool (CacheFilePathLength * sizeof (CHAR16));
  if (!CacheFilePath) {
    goto Exit;
  }

  //
  // Let's produce our final path string, shall we?
  //
  StrnCpyS (CacheFilePath, CacheFilePathLength, AppPath, DirectorySlashOffset + 1);  // Copy the path for the parent directory.
  StrCatS (CacheFilePath, CacheFilePathLength, TestName);                            // Copy the base name for the test cache.
  StrCatS (CacheFilePath, CacheFilePathLength, CACHE_FILE_SUFFIX);                   // Copy the file suffix.

Exit:
  //
  // Free allocated buffers.
  //
  if (AppPath != NULL) {
    FreePool (AppPath);
  }

  if (TestName != NULL) {
    FreePool (TestName);
  }

  return CacheFilePath;
}

/**
  Determines whether a persistence cache already exists for
  the given framework.

  @param[in]  FrameworkHandle  A pointer to the framework that is being persisted.

  @retval  TRUE
  @retval  FALSE  Cache doesn't exist or an error occurred.

**/
BOOLEAN
EFIAPI
DoesCacheExist (
  IN UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle
  )
{
  CHAR16             *FileName;
  EFI_STATUS         Status;
  SHELL_FILE_HANDLE  FileHandle;

  //
  // NOTE: This devpath is allocated and must be freed.
  //
  FileName = GetCacheFileName (FrameworkHandle);
  if (FileName == NULL) {
    return FALSE;
  }

  //
  // Check to see whether the file exists.  If the file can be opened for
  // reading, it exists.  Otherwise, probably not.
  //
  Status = ShellOpenFileByName (
             FileName,
             &FileHandle,
             EFI_FILE_MODE_READ,
             0
             );
  if (!EFI_ERROR (Status)) {
    ShellCloseFile (&FileHandle);
  }

  if (FileName != NULL) {
    FreePool (FileName);
  }

  DEBUG ((DEBUG_VERBOSE, "%a - Returning %d\n", __func__, !EFI_ERROR (Status)));

  return (!EFI_ERROR (Status));
}

/**
  Will save the data associated with an internal Unit Test Framework
  state in a manner that can persist a Unit Test Application quit or
  even a system reboot.

  @param[in]  FrameworkHandle  A pointer to the framework that is being persisted.
  @param[in]  SaveData         A pointer to the buffer containing the serialized
                               framework internal state.
  @param[in]  SaveStateSize    The size of SaveData in bytes.

  @retval  EFI_SUCCESS  Data is persisted and the test can be safely quit.
  @retval  Others       Data is not persisted and test cannot be resumed upon exit.

**/
EFI_STATUS
EFIAPI
SaveUnitTestCache (
  IN UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle,
  IN VOID                        *SaveData,
  IN UINTN                       SaveStateSize
  )
{
  CHAR16             *FileName;
  EFI_STATUS         Status;
  SHELL_FILE_HANDLE  FileHandle;
  UINTN              WriteCount;

  //
  // Check the inputs for sanity.
  //
  if ((FrameworkHandle == NULL) || (SaveData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Determine the path for the cache file.
  // NOTE: This devpath is allocated and must be freed.
  //
  FileName = GetCacheFileName (FrameworkHandle);
  if (FileName == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // First lets open the file if it exists so we can delete it...This is the work around for truncation
  //
  Status = ShellOpenFileByName (
             FileName,
             &FileHandle,
             (EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE),
             0
             );

  if (!EFI_ERROR (Status)) {
    //
    // If file handle above was opened it will be closed by the delete.
    //
    Status = ShellDeleteFile (&FileHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a failed to delete file %r\n", __func__, Status));
    }
  }

  //
  // Now that we know the path to the file... let's open it for writing.
  //
  Status = ShellOpenFileByName (
             FileName,
             &FileHandle,
             (EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE),
             0
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Opening file for writing failed! %r\n", __func__, Status));
    goto Exit;
  }

  //
  // Write the data to the file.
  //
  WriteCount = SaveStateSize;
  DEBUG ((DEBUG_INFO, "%a - Writing %d bytes to file...\n", __func__, WriteCount));
  Status = ShellWriteFile (
             FileHandle,
             &WriteCount,
             SaveData
             );

  if (EFI_ERROR (Status) || (WriteCount != SaveStateSize)) {
    DEBUG ((DEBUG_ERROR, "%a - Writing to file failed! %r\n", __func__, Status));
  } else {
    DEBUG ((DEBUG_INFO, "%a - SUCCESS!\n", __func__));
  }

  //
  // No matter what, we should probably close the file.
  //
  ShellCloseFile (&FileHandle);

Exit:
  if (FileName != NULL) {
    FreePool (FileName);
  }

  return Status;
}

/**
  Will retrieve any cached state associated with the given framework.
  Will allocate a buffer to hold the loaded data.

  @param[in]  FrameworkHandle  A pointer to the framework that is being persisted.
  @param[out] SaveData         A pointer pointer that will be updated with the address
                               of the loaded data buffer.
  @param[out] SaveStateSize    Return the size of SaveData in bytes.

  @retval  EFI_SUCCESS  Data has been loaded successfully and SaveData is updated
                        with a pointer to the buffer.
  @retval  Others       An error has occurred and no data has been loaded. SaveData
                        is set to NULL.

**/
EFI_STATUS
EFIAPI
LoadUnitTestCache (
  IN  UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle,
  OUT VOID                        **SaveData,
  OUT UINTN                       *SaveStateSize
  )
{
  EFI_STATUS         Status;
  CHAR16             *FileName;
  SHELL_FILE_HANDLE  FileHandle;
  BOOLEAN            IsFileOpened;
  UINT64             LargeFileSize;
  UINTN              FileSize;
  VOID               *Buffer;

  IsFileOpened = FALSE;
  Buffer       = NULL;

  //
  // Check the inputs for sanity.
  //
  if ((FrameworkHandle == NULL) || (SaveData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Determine the path for the cache file.
  // NOTE: This devpath is allocated and must be freed.
  //
  FileName = GetCacheFileName (FrameworkHandle);
  if (FileName == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Now that we know the path to the file... let's open it for writing.
  //
  Status = ShellOpenFileByName (
             FileName,
             &FileHandle,
             EFI_FILE_MODE_READ,
             0
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Opening file for writing failed! %r\n", __func__, Status));
    goto Exit;
  } else {
    IsFileOpened = TRUE;
  }

  //
  // Now that the file is opened, we need to determine how large a buffer we need.
  //
  Status = ShellGetFileSize (FileHandle, &LargeFileSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to determine file size! %r\n", __func__, Status));
    goto Exit;
  }

  //
  // Now that we know the size, let's allocated a buffer to hold the contents.
  //
  FileSize       = (UINTN)LargeFileSize; // You know what... if it's too large, this lib don't care.
  *SaveStateSize = FileSize;
  Buffer         = AllocatePool (FileSize);
  if (Buffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to allocate a pool to hold the file contents! %r\n", __func__, Status));
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  //
  // Finally, let's read the data.
  //
  Status = ShellReadFile (FileHandle, &FileSize, Buffer);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to read the file contents! %r\n", __func__, Status));
  }

Exit:
  //
  // Free allocated buffers
  //
  if (FileName != NULL) {
    FreePool (FileName);
  }

  if (IsFileOpened) {
    ShellCloseFile (&FileHandle);
  }

  //
  // If we're returning an error, make sure
  // the state is sane.
  if (EFI_ERROR (Status) && (Buffer != NULL)) {
    FreePool (Buffer);
    Buffer = NULL;
  }

  *SaveData = Buffer;
  return Status;
}

/**
  Shell based UnitTestPersistenceLib library constructor.

  @param[in]  ImageHandle  The firmware allocated handle for the EFI image.
  @param[in]  SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS      The constructor finished successfully.
  @retval Others           Error codes returned from gBS->HandleProtocol.
 **/
EFI_STATUS
EFIAPI
UnitTestPersistenceLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN                          Index;
  UINTN                          Argc;
  CHAR16                         **Argv;
  EFI_STATUS                     Status;
  EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters;

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiShellParametersProtocolGuid,
                  (VOID **)&ShellParameters
                  );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto Done;
  }

  Argc = ShellParameters->Argc;
  Argv = ShellParameters->Argv;

  Status = EFI_SUCCESS;
  if ((Argc > 1) && (Argv != NULL)) {
    // This might be our cue, check for whether we need to do anything
    for (Index = 1; Index < Argc; Index++) {
      if (StrCmp (Argv[Index], L"--CachePath") == 0) {
        // Need to update the potential cache path to designated path
        if (Index < Argc - 1) {
          mCachePath = Argv[Index + 1];
        } else {
          Print (L"  --CachePath <Path of where to save unit test cache files, i.e. FS0:TestFolder>\n");
        }
      }
    }
  }

Done:
  return Status;
}
