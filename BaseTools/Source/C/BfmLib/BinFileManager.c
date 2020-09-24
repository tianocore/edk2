/** @file

 The main entry of BFM tool.

 Copyright (c) 2011-2019, Intel Corporation. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BinFileManager.h"

BOOLEAN     mFvGuidIsSet = FALSE;
EFI_GUID    mFvNameGuid  = {0};
CHAR8*      mFvNameGuidString = NULL;
CHAR8*      mGuidToolDefinition     = "GuidToolDefinitionConf.ini";

//
// Store GUIDed Section guid->tool mapping
//
EFI_HANDLE mParsedGuidedSectionTools = NULL;


/**
  Search the config file from the path list.

  Split the path from env PATH, and then search the cofig
  file from these paths. The priority is from left to
  right of PATH string. When met the first Config file, it
  will break and return the pointer to the full file name.

  @param  PathList         the pointer to the path list.
  @param  FileName         the pointer to the file name.

  @retval The pointer to the file name.
  @return NULL       An error occurred.
**/
CHAR8 *
SearchConfigFromPathList (
  IN  CHAR8  *PathList,
  IN  CHAR8  *FileName
)
{
  CHAR8  *CurDir;
  CHAR8  *FileNamePath;

  CurDir       = NULL;
  FileNamePath = NULL;
#ifndef __GNUC__
  CurDir = strtok (PathList,";");
#else
  CurDir = strtok (PathList,":");
#endif
  while (CurDir != NULL) {
    FileNamePath  = (char *)calloc(
                     strlen (CurDir) + strlen (OS_SEP_STR) +strlen (FileName) + 1,
                     sizeof(char)
                     );
    if (FileNamePath == NULL) {
      return NULL;
    }
    sprintf(FileNamePath, "%s%c%s", CurDir, OS_SEP, FileName);
    if (access (FileNamePath, 0) != -1) {
      return FileNamePath;
    }
#ifndef __GNUC__
    CurDir = strtok(NULL, ";");
#else
    CurDir = strtok(NULL, ":");
#endif
    free (FileNamePath);
    FileNamePath = NULL;
  }
  return NULL;
}

/**

  Show the FD image layout information. Only display the modules with UI name.

  @param[in]   FdInName    Input FD binary/image file name;
  @param[in]   FvName      The FV ID in the FD file;
  @param[in]   ViewFlag    Is this call for view or other operate(add/del/replace)
  @param[in]   FdData      The Fd data structure store the FD information.

  @retval      EFI_SUCCESS
  @retval      EFI_INVALID_PARAMETER
  @retval      EFI_ABORTED

**/
EFI_STATUS
BfmImageView (
  IN     CHAR8*           FdInName,
  IN     CHAR8*           FvName,
  IN     BOOLEAN          ViewFlag,
  IN     FIRMWARE_DEVICE  **FdData
)
{
  EFI_STATUS                  Status;
  FIRMWARE_DEVICE             *LocalFdData;
  FV_INFORMATION              *CurrentFv;
  FILE                        *InputFile;
  UINT32                      FvSize;
  UINTN                       BytesRead;
  EFI_FIRMWARE_VOLUME_HEADER  *FvImage;
  UINT32                      FfsCount;
  UINT8                       FvCount;
  UINT8                       LastFvNumber;

  LocalFdData    = NULL;
  CurrentFv      = NULL;
  FvImage        = NULL;
  FvSize         = 0;
  BytesRead      = 0;
  FfsCount       = 0;
  FvCount        = 0;
  LastFvNumber   = 0;

  //
  // Check the FD file name/path.
  //
  if (FdInName == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Open the file containing the FV
  //
  InputFile = fopen (FdInName, "rb");
  if (InputFile == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = LibFindFvInFd (InputFile, &LocalFdData);

  if (EFI_ERROR(Status)) {
    fclose (InputFile);
    return EFI_ABORTED;
  }

  CurrentFv = LocalFdData->Fv;

  do {

    memset (CurrentFv->FvName, '\0', _MAX_PATH);

    if (LastFvNumber == 0) {
      sprintf (CurrentFv->FvName, "FV%d", LastFvNumber);
    } else {
      sprintf (CurrentFv->FvName, "FV%d", LastFvNumber);
    }

    //
    // Determine size of FV
    //
    if (fseek (InputFile, CurrentFv->ImageAddress, SEEK_SET) != 0) {
      fclose (InputFile);
      LibBfmFreeFd( LocalFdData);
      return EFI_ABORTED;
    }

    Status = LibGetFvSize(InputFile, &FvSize);
    if (EFI_ERROR (Status)) {
      fclose (InputFile);
      return EFI_ABORTED;
    }

    //
    // Seek to the start of the image, then read the entire FV to the buffer
    //
    fseek (InputFile, CurrentFv->ImageAddress, SEEK_SET);

    FvImage = (EFI_FIRMWARE_VOLUME_HEADER *) malloc (FvSize);

    if (FvImage == NULL) {
      fclose (InputFile);
      LibBfmFreeFd( LocalFdData);
      return EFI_ABORTED;
    }

    BytesRead = fread (FvImage, 1, FvSize, InputFile);
    if ((unsigned int) BytesRead != FvSize) {
      free (FvImage);
      fclose (InputFile);
      LibBfmFreeFd( LocalFdData);
      return EFI_ABORTED;
    }

    //
    // Collect FV information each by each.
    //
    Status = LibGetFvInfo (FvImage, CurrentFv, FvName, 0, &FfsCount, ViewFlag, FALSE);
    free (FvImage);
    FvImage = NULL;
    if (EFI_ERROR (Status)) {
      fclose (InputFile);
      LibBfmFreeFd( LocalFdData);
      return Status;
    }
    FvCount = CurrentFv->FvLevel;
    LastFvNumber = LastFvNumber+FvCount;

    FfsCount = 0;

    CurrentFv = CurrentFv->FvNext;

  } while (CurrentFv != NULL);

  if (!ViewFlag) {
    *FdData = LocalFdData;
  } else {
    LibBfmFreeFd( LocalFdData);
  }

  fclose (InputFile);

  return EFI_SUCCESS;
}

/**
  Add an FFS file into a specify FV.

  @param[in]   FdInName     Input FD binary/image file name;
  @param[in]   NewFile      The name of the file add in;
  @param[in]   FdOutName    Name of output fd file.

  @retval      EFI_SUCCESS
  @retval      EFI_INVALID_PARAMETER
  @retval      EFI_ABORTED

**/
EFI_STATUS
BfmImageAdd (
  IN     CHAR8*  FdInName,
  IN     CHAR8*  NewFile,
  IN     CHAR8*  FdOutName
)
{
  EFI_STATUS                  Status;
  FIRMWARE_DEVICE             *FdData;
  FV_INFORMATION              *FvInFd;
  ENCAP_INFO_DATA             *LocalEncapData;
  ENCAP_INFO_DATA             *LocalEncapDataTemp;
  FILE*                       NewFdFile;
  FILE*                       NewFvFile;
  UINT64                      NewFvLength;
  UINT64                      NewFfsLength;
  VOID*                       Buffer;
  CHAR8                       *TemDir;
  UINT8                       FvEncapLevel;
  UINT8                       NewAddedFfsLevel;
  BOOLEAN                     FfsLevelFoundFlag;
  CHAR8                       *OutputFileName;
  CHAR8                       *FvId;
  BOOLEAN                     FirstInFlag;
  BOOLEAN                     FvGuidExisted;

  NewFvLength                 = 0;
  FvEncapLevel                = 0;
  NewAddedFfsLevel            = 0;

  FfsLevelFoundFlag           = FALSE;
  FirstInFlag                 = TRUE;
  FdData                      = NULL;
  FvInFd                      = NULL;
  LocalEncapData              = NULL;
  NewFdFile                   = NULL;
  NewFvFile                   = NULL;
  Buffer                      = NULL;
  TemDir                      = NULL;
  LocalEncapDataTemp          = NULL;
  OutputFileName              = NULL;
  FvId                        = NULL;
  FvGuidExisted               = FALSE;

  //
  // Get the size of ffs file to be inserted.
  //
  NewFfsLength = GetFileSize(NewFile);

  Status = BfmImageView (FdInName, NULL, FALSE, &FdData);

  if (EFI_ERROR (Status)) {
    printf ("Error while parse %s FD image.\n", FdInName);
    return Status;
  }
  //
  // Check the FvGuid whether exists or not when the BIOS hasn't default setting.
  // If the FV image with -g GUID can't be found, the storage is still saved into the BFV and report warning message.
  //
  FvInFd = FdData->Fv;
  do {
    if (mFvGuidIsSet && FvInFd->IsInputFvFlag) {
      FvGuidExisted = TRUE;
    break;
    }
    FvInFd = FvInFd->FvNext;
  } while (FvInFd != NULL);

  if (mFvGuidIsSet && !FvGuidExisted) {
    printf ("Fv image with the specified FV Name Guid %s can't be found in current FD.\n", mFvNameGuidString);
    LibBfmFreeFd(FdData);
    return EFI_ABORTED;
  }
  //
  // Iterate to write FFS to each BFV.
  //
  FvInFd = FdData->Fv;
  do {
    if ((FvGuidExisted && mFvGuidIsSet && FvInFd->IsInputFvFlag) || ((!FvGuidExisted || (!mFvGuidIsSet)) && FvInFd->IsBfvFlag)) {

      Status = LibLocateBfv (FdData, &FvId, &FvInFd);

      if (EFI_ERROR (Status)) {
        printf("Error while locate BFV from FD.\n");
        LibBfmFreeFd(FdData);
        return Status;
      }

      //
      // Determine the new added ffs file level in the FV.
      //
      LocalEncapData = FvInFd->EncapData;

      while (LocalEncapData != NULL && !FfsLevelFoundFlag ) {
        if (LocalEncapData->Type == BFM_ENCAP_TREE_FV) {
          if (FvEncapLevel == ((UINT8) atoi (FvId + 2) - (UINT8) atoi (FvInFd->FvName + 2))) {
            //
            // Found the FFS level in this FV.
            //
            LocalEncapDataTemp = LocalEncapData;
            while (LocalEncapDataTemp != NULL) {
              if (LocalEncapDataTemp->Type == BFM_ENCAP_TREE_FFS) {
                NewAddedFfsLevel  = LocalEncapDataTemp->Level;
                FfsLevelFoundFlag = TRUE;
                break;
              }
              if (LocalEncapDataTemp->NextNode != NULL) {
                LocalEncapDataTemp = LocalEncapDataTemp->NextNode;
              } else {
                break;
              }
            }
          }
          FvEncapLevel ++;
        }

        if (LocalEncapData->NextNode == NULL) {
          break;
        } else {
          LocalEncapData = LocalEncapData->NextNode;
        }
      }

      //
      // Add the new file into FV.
      //
      FvInFd->FfsNumbers += 1;
      if (strlen (NewFile) > _MAX_PATH - 1) {
        printf ("The NewFile name is too long \n");
        LibBfmFreeFd(FdData);
        return EFI_ABORTED;
      }
      strncpy (FvInFd->FfsAttuibutes[FvInFd->FfsNumbers].FfsName, NewFile, _MAX_PATH - 1);
      FvInFd->FfsAttuibutes[FvInFd->FfsNumbers].FfsName[_MAX_PATH - 1] = 0;
      FvInFd->FfsAttuibutes[FvInFd->FfsNumbers].Level   = NewAddedFfsLevel;

      TemDir = getcwd (NULL, _MAX_PATH);
      TemDir = realloc (TemDir, _MAX_PATH);
      if (strlen (TemDir) + strlen (OS_SEP_STR) + strlen (TEMP_DIR_NAME) > _MAX_PATH - 1) {
        printf ("The directory is too long \n");
        LibBfmFreeFd(FdData);
        return EFI_ABORTED;
      }
      strncat (TemDir, OS_SEP_STR, _MAX_PATH - strlen (TemDir) - 1);
      strncat (TemDir, TEMP_DIR_NAME, _MAX_PATH - strlen (TemDir) - 1);
      mkdir(TemDir, S_IRWXU | S_IRWXG | S_IRWXO);
      Status = LibEncapNewFvFile (FvInFd, TemDir, &OutputFileName);

      if (EFI_ERROR (Status)) {
        printf("Error. The boot firmware volume (BFV) has only the spare space 0x%lx bytes. But the default setting data takes 0x%llx bytes, which can't be inserted into BFV. \n",(unsigned long) GetBfvPadSize (), (unsigned long long)NewFfsLength);
        LibBfmFreeFd(FdData);
        return Status;
      }

      if (FirstInFlag) {
        //
        // Write New Fv file into the NewFd file.
        //
        Status = LibCreateNewFdCopy (FdInName, FdOutName);
        if (EFI_ERROR (Status)) {
          printf("Error while copy from %s to %s file. \n", FdInName, FdOutName);
          LibBfmFreeFd(FdData);
          return Status;
        }
        FirstInFlag = FALSE;
      }

      NewFdFile = fopen (FdOutName, "rb+");
      if (NewFdFile == NULL) {
        printf("Error while create FD file %s. \n", FdOutName);
        LibBfmFreeFd(FdData);
        return EFI_ABORTED;
      }

      NewFvFile = fopen (OutputFileName, "rb+");

      if (NewFvFile == NULL) {
        printf("Error while create Fv file %s. \n", OutputFileName);
        fclose(NewFdFile);
        LibBfmFreeFd(FdData);
        return EFI_ABORTED;
      }

      fseek(NewFvFile,0,SEEK_SET);
      fseek(NewFvFile,0,SEEK_END);

      NewFvLength = ftell(NewFvFile);

      fseek(NewFvFile,0,SEEK_SET);

      Buffer = malloc ((size_t)NewFvLength);

      if (Buffer == NULL)  {
        printf ("Error while allocate resource! \n");
        fclose(NewFdFile);
        fclose(NewFvFile);
        LibBfmFreeFd(FdData);
        return EFI_ABORTED;
      }

      if (fread (Buffer, 1, (size_t) NewFvLength, NewFvFile) != (size_t) NewFvLength) {
        printf("Error while reading Fv file %s. \n", OutputFileName);
        free (Buffer);
        fclose(NewFdFile);
        fclose(NewFvFile);
        LibBfmFreeFd(FdData);
        return EFI_ABORTED;
      }

      fseek(NewFdFile, FvInFd->ImageAddress, SEEK_SET);
      fseek(NewFdFile, FvInFd->ImageAddress, SEEK_SET);

      if (NewFvLength <= FvInFd->FvHeader->FvLength) {
        if (fwrite (Buffer, 1, (size_t) NewFvLength, NewFdFile) != (size_t) NewFvLength) {
          printf("Error while writing FD file %s. \n", FdOutName);
          fclose(NewFdFile);
          fclose (NewFvFile);
          free (Buffer);
          LibBfmFreeFd(FdData);
          return EFI_ABORTED;
        }
      } else {
        printf("Error. The new size of BFV is 0x%llx bytes, which is larger than the previous size of BFV 0x%llx bytes. \n", (unsigned long long) NewFvLength, (unsigned long long) FvInFd->FvHeader->FvLength);
        free (Buffer);
        fclose(NewFdFile);
        fclose(NewFvFile);
        LibBfmFreeFd(FdData);
        return EFI_ABORTED;
      }

      fclose(NewFdFile);
      fclose(NewFvFile);
      free (Buffer);
      Buffer = NULL;

    }
    FvInFd = FvInFd->FvNext;
  } while (FvInFd != NULL);


  LibBfmFreeFd(FdData);

  if (TemDir == NULL) {
    if (mFvGuidIsSet) {
      printf ("Fv image with the specified FV Name Guid %s can't be found.\n", mFvNameGuidString);
    } else {
      printf ("BFV image can't be found.\n");
    }
    return EFI_NOT_FOUND;
  }

  Status = LibRmDir (TemDir);

  if (EFI_ERROR (Status)) {
    printf("Error while remove temporary directory. \n");
    return Status;
  }

  return EFI_SUCCESS;
}


/**
  Replace an FFS file into a specify FV.

  @param[in]   FdInName     Input FD binary/image file name;
  @param[in]   NewFile      The name of the file add in;
  @param[in]   FdOutName    Name of output fd file.

  @retval      EFI_SUCCESS
  @retval      EFI_INVALID_PARAMETER
  @retval      EFI_ABORTED

**/
EFI_STATUS
BfmImageReplace (
  IN     CHAR8*  FdInName,
  IN     CHAR8*  NewFile,
  IN     CHAR8*  FdOutName
)
{
  EFI_STATUS                  Status;
  FIRMWARE_DEVICE             *FdData;
  FV_INFORMATION              *FvInFd;
  FILE*                       NewFdFile;
  FILE*                       NewFvFile;
  UINT64                      NewFvLength;
  UINT64                      NewFfsLength;
  VOID*                       Buffer;
  CHAR8                       *TemDir;
  CHAR8                       *OutputFileName;
  CHAR8                       *FvId;
  BOOLEAN                     FirstInFlag;
  UINT32                      Index;
  BOOLEAN                     FvToBeUpdate;
  BOOLEAN                     FdIsUpdate;
  ENCAP_INFO_DATA             *LocalEncapData;
  ENCAP_INFO_DATA             *LocalEncapDataTemp;
  UINT8                       FvEncapLevel;
  UINT8                       NewAddedFfsLevel;
  BOOLEAN                     FfsLevelFoundFlag;
  EFI_GUID                    EfiNewAddToBfvGuid;
  FILE*                       FfsFile;
  UINTN                       BytesRead;
  BOOLEAN                     ReplaceSameFv;
  BOOLEAN                     FvGuidExisted;

  NewFvLength                 = 0;
  FdIsUpdate                  = FALSE;
  FirstInFlag                 = TRUE;
  FdData                      = NULL;
  FvInFd                      = NULL;
  NewFdFile                   = NULL;
  NewFvFile                   = NULL;
  Buffer                      = NULL;
  TemDir                      = NULL;
  OutputFileName              = NULL;
  FvId                        = NULL;
  FfsFile                     = NULL;
  BytesRead                   = 0;
  ReplaceSameFv               = FALSE;
  FvGuidExisted               = FALSE;

  //
  // Get the size of ffs file to be inserted.
  //
  NewFfsLength = GetFileSize(NewFile);
  //
  // Get FFS GUID
  //
  FfsFile = fopen (NewFile, "rb");
  if (FfsFile == NULL) {
    printf ("Error while read %s.\n", NewFile);
  return EFI_ABORTED;
  }
  fseek (FfsFile, 0, SEEK_SET);
  BytesRead = fread (&EfiNewAddToBfvGuid, 1, sizeof(EFI_GUID), FfsFile);
  fclose (FfsFile);
  if (BytesRead != sizeof(EFI_GUID)) {
    printf ("Error while read the GUID from %s.\n", NewFile);
  return EFI_ABORTED;
  }
  Status = BfmImageView (FdInName, NULL, FALSE, &FdData);

  if (EFI_ERROR (Status)) {
    printf ("Error while parse %s FD image.\n", FdInName);
    return Status;
  }

  //
  // Check the FvGuid whether exists or not when the BIOS has default setting.
  // 1.  No option means the storage is saved into the same FV image.
  // 2.  The FV image with -g GUID can't be found. The storage is still saved into the same FV image and report warning message.
  //
  if (!mFvGuidIsSet) {
    ReplaceSameFv = TRUE;
  }
  FvInFd = FdData->Fv;
  do {
    if (mFvGuidIsSet && FvInFd->IsInputFvFlag) {
      FvGuidExisted = TRUE;
    break;
    }
    FvInFd = FvInFd->FvNext;
  } while (FvInFd != NULL);

  if (mFvGuidIsSet && !FvGuidExisted) {
    printf ("Fv image with the specified FV Name Guid %s can't be found in current FD.\n", mFvNameGuidString);
    ReplaceSameFv = TRUE;
    LibBfmFreeFd(FdData);
    return EFI_ABORTED;
  }
  //
  // Interate to insert or replace default setting to Fv
  //
  FvInFd = FdData->Fv;
  do {
    FvToBeUpdate = FALSE;
    if (mFvGuidIsSet && FvInFd->IsInputFvFlag) {
      FvToBeUpdate = TRUE;
    }

    Status = LibLocateBfv (FdData, &FvId, &FvInFd);

    if (EFI_ERROR (Status)) {
      printf("Error while locate BFV from FD.\n");
      LibBfmFreeFd(FdData);
      return Status;
    }

    Index = 0;
    while (Index <= FvInFd->FfsNumbers) {
      //
      // Locate the multi-platform ffs in Fv and then replace or delete it.
      //
      if (!CompareGuid(&FvInFd->FfsHeader[Index].Name, &EfiNewAddToBfvGuid)) {
      if (ReplaceSameFv) {
      FvToBeUpdate = TRUE;
      }
        break;
      }
      Index ++;
    }

    if (FvToBeUpdate || (Index <= FvInFd->FfsNumbers)) {
      if (FvToBeUpdate) {
        FdIsUpdate = TRUE;
        if (Index <= FvInFd->FfsNumbers) {
          //
          // Override original default data by New File
          //
          if (strlen (NewFile) > _MAX_PATH - 1) {
            printf ("The NewFile name is too long \n");
            LibBfmFreeFd(FdData);
            return EFI_ABORTED;
          }
          strncpy (FvInFd->FfsAttuibutes[Index].FfsName, NewFile, _MAX_PATH - 1);
          FvInFd->FfsAttuibutes[Index].FfsName[_MAX_PATH - 1] = 0;
        } else {
          FfsLevelFoundFlag           = FALSE;
          FvEncapLevel                = 0;
          NewAddedFfsLevel            = 0;
          //
          // Determine the new added ffs file level in the FV.
          //
          LocalEncapData = FvInFd->EncapData;

          while (LocalEncapData != NULL && !FfsLevelFoundFlag ) {
            if (LocalEncapData->Type == BFM_ENCAP_TREE_FV) {
              if (FvEncapLevel == ((UINT8) atoi (FvId + 2) - (UINT8) atoi (FvInFd->FvName + 2))) {
                //
                // Found the FFS level in this FV.
                //
                LocalEncapDataTemp = LocalEncapData;
                while (LocalEncapDataTemp != NULL) {
                  if (LocalEncapDataTemp->Type == BFM_ENCAP_TREE_FFS) {
                    NewAddedFfsLevel  = LocalEncapDataTemp->Level;
                    FfsLevelFoundFlag = TRUE;
                    break;
                  }
                  if (LocalEncapDataTemp->NextNode != NULL) {
                    LocalEncapDataTemp = LocalEncapDataTemp->NextNode;
                  } else {
                    break;
                  }
                }
              }
              FvEncapLevel ++;
            }

            if (LocalEncapData->NextNode == NULL) {
              break;
            } else {
              LocalEncapData = LocalEncapData->NextNode;
            }
          }

          //
          // Add the new file into FV.
          //
          FvInFd->FfsNumbers += 1;
          memcpy (FvInFd->FfsAttuibutes[FvInFd->FfsNumbers].FfsName, NewFile, _MAX_PATH);
          FvInFd->FfsAttuibutes[FvInFd->FfsNumbers].Level   = NewAddedFfsLevel;
        }
      } else {
        //
        // Remove original default data from FV.
        //
        FvInFd->FfsAttuibutes[Index].FfsName[0] = '\0';
      }

      if (TemDir == NULL) {
        TemDir = getcwd (NULL, _MAX_PATH);
        TemDir = realloc (TemDir, _MAX_PATH);
        if (strlen (TemDir) + strlen (OS_SEP_STR)+ strlen (TEMP_DIR_NAME) > _MAX_PATH - 1) {
          printf ("The directory is too long \n");
          LibBfmFreeFd(FdData);
          return EFI_ABORTED;
        }
        strncat (TemDir, OS_SEP_STR, _MAX_PATH - strlen (TemDir) - 1);
        strncat (TemDir, TEMP_DIR_NAME, _MAX_PATH - strlen (TemDir) - 1);
        mkdir(TemDir, S_IRWXU | S_IRWXG | S_IRWXO);
      }

      Status = LibEncapNewFvFile (FvInFd, TemDir, &OutputFileName);

      if (EFI_ERROR (Status)) {
        printf("Error. The boot firmware volume (BFV) has only the spare space 0x%lx bytes. But the default setting data takes 0x%llx bytes, which can't be inserted into BFV. \n", (unsigned long) GetBfvPadSize (), (unsigned long long) NewFfsLength);
        LibBfmFreeFd(FdData);
        return Status;
      }

      if (FirstInFlag) {
        //
        // Write New Fv file into the NewFd file.
        //
        Status = LibCreateNewFdCopy (FdInName, FdOutName);
        if (EFI_ERROR (Status)) {
          printf("Error while copy from %s to %s file. \n", FdInName, FdOutName);
          LibBfmFreeFd(FdData);
          return Status;
        }
        FirstInFlag = FALSE;
      }

      NewFdFile = fopen (FdOutName, "rb+");
      if (NewFdFile == NULL) {
        printf("Error while create FD file %s. \n", FdOutName);
        LibBfmFreeFd(FdData);
        return EFI_ABORTED;
      }

      NewFvFile = fopen (OutputFileName, "rb+");

      if (NewFvFile == NULL) {
        printf("Error while create Fv file %s. \n", OutputFileName);
        LibBfmFreeFd(FdData);
        fclose (NewFdFile);
        return EFI_ABORTED;
      }

      fseek(NewFvFile,0,SEEK_SET);
      fseek(NewFvFile,0,SEEK_END);

      NewFvLength = ftell(NewFvFile);

      fseek(NewFvFile,0,SEEK_SET);

      Buffer = malloc ((size_t)NewFvLength);

      if (Buffer == NULL)  {
        LibBfmFreeFd(FdData);
        fclose (NewFdFile);
        fclose (NewFvFile);
        return EFI_ABORTED;
      }

      if (fread (Buffer, 1, (size_t) NewFvLength, NewFvFile) != (size_t) NewFvLength) {
        printf("Error while read Fv file %s. \n", OutputFileName);
        LibBfmFreeFd(FdData);
        free (Buffer);
        fclose (NewFdFile);
        fclose (NewFvFile);
        return EFI_ABORTED;
      }

      fseek(NewFdFile, FvInFd->ImageAddress, SEEK_SET);
      fseek(NewFdFile, FvInFd->ImageAddress, SEEK_SET);

      if (NewFvLength <= FvInFd->FvHeader->FvLength) {
        if (fwrite (Buffer, 1, (size_t) NewFvLength, NewFdFile) != (size_t) NewFvLength) {
          printf("Error while write FD file %s. \n", FdOutName);
          fclose(NewFdFile);
          fclose (NewFvFile);
          LibBfmFreeFd(FdData);
          free (Buffer);
          return EFI_ABORTED;
        }
      } else {
        printf("Error. The new size of BFV is 0x%llx bytes, which is larger than the previous size of BFV 0x%llx bytes. \n", (unsigned long long) NewFvLength, (unsigned long long) FvInFd->FvHeader->FvLength);
        free (Buffer);
        LibBfmFreeFd(FdData);
        fclose (NewFdFile);
        fclose (NewFvFile);
        return EFI_ABORTED;
      }

      fclose(NewFdFile);
      fclose(NewFvFile);
      free (Buffer);
      Buffer = NULL;

    }
    FvInFd = FvInFd->FvNext;
  } while (FvInFd != NULL);

  LibBfmFreeFd(FdData);

  if (TemDir == NULL || !FdIsUpdate) {
    if (mFvGuidIsSet) {
      printf ("Fv image with the specified FV Name Guid %s can't be found.\n", mFvNameGuidString);
    } else {
      printf ("BFV image can't be found.\n");
    }
    return EFI_NOT_FOUND;
  }

  Status = LibRmDir (TemDir);

  if (EFI_ERROR (Status)) {
    printf("Error while remove temporary directory. \n");
    return Status;
  }

  return EFI_SUCCESS;
}

/**

  The main entry of BFM tool.

**/
int main(
  int      Argc,
  char     *Argv[]
)
{
  EFI_STATUS                    Status;
  FIRMWARE_DEVICE               *FdData;
  CHAR8                         *InFilePath;
  CHAR8                         FullGuidToolDefinition[_MAX_PATH];
  CHAR8                         *FileName;
  UINTN                         FileNameIndex;
  CHAR8                         *PathList;
  UINTN                         EnvLen;
  CHAR8                         *NewPathList;

  FdData                      = NULL;
  PathList                    = NULL;
  NewPathList                 = NULL;
  EnvLen                      = 0;

  if (Argc <= 1) {
    return -1;
  }
  FileName = Argv[0];
  //
  // Save, skip filename arg
  //
  Argc--;
  Argv++;

  if ((stricmp(Argv[0], "-v") == 0)) {
    //
    // Check the revison of BfmLib
    // BfmLib -v
    //
    printf("%s\n", __BUILD_VERSION);
    return 1;

  }
  //
  // Workaroud: the first call to this function
  //            returns a file name ends with dot
  //
#ifndef __GNUC__
  tmpnam (NULL);
#else
  CHAR8 tmp[] = "/tmp/fileXXXXXX";
  UINTN Fdtmp;
  Fdtmp = mkstemp(tmp);
  close(Fdtmp);
#endif
  //
  // Get the same path with the application itself
  //
  if (strlen (FileName) > _MAX_PATH - 1) {
    Error (NULL, 0, 2000, "Parameter: Input file name is too long", NULL);
    return -1;
  }
  strncpy (FullGuidToolDefinition, FileName, _MAX_PATH - 1);
  FullGuidToolDefinition[_MAX_PATH - 1] = 0;
  FileNameIndex = strlen (FullGuidToolDefinition);
  while (FileNameIndex != 0) {
    FileNameIndex --;
    if (FullGuidToolDefinition[FileNameIndex] == OS_SEP) {
    FullGuidToolDefinition[FileNameIndex] = 0;
      break;
    }
  }
  //
  // Build the path list for Config file scan. The priority is below.
  // 1. Scan the current path
  // 2. Scan the same path with the application itself
  // 3. Scan the current %PATH% of OS environment
  // 4. Use the build-in default configuration
  //
  PathList = getenv("PATH");
  if (PathList == NULL) {
    Error (NULL, 0, 1001, "Option: Environment variable 'PATH' does not exist", NULL);
    return -1;
  }
  EnvLen = strlen(PathList);
  NewPathList  = (char *)calloc(
                     strlen (".")
                     + strlen (";")
                     + strlen (FullGuidToolDefinition)
                     + strlen (";")
                     + EnvLen
                     + 1,
                     sizeof(char)
                  );
  if (NewPathList == NULL) {
    Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
    PathList = NULL;
    free (PathList);
    return -1;
  }
#ifndef __GNUC__
  sprintf (NewPathList, "%s;%s;%s", ".", FullGuidToolDefinition, PathList);
#else
  sprintf (NewPathList, "%s:%s:%s", ".", FullGuidToolDefinition, PathList);
#endif

  PathList = NULL;
  free (PathList);
  //
  // Load Guid Tools definition
  //
  InFilePath = SearchConfigFromPathList(NewPathList, mGuidToolDefinition);
  free (NewPathList);
  if (InFilePath != NULL) {
    printf ("\nThe Guid Tool Definition of BfmLib comes from the '%s'. \n", InFilePath);
    mParsedGuidedSectionTools = ParseGuidedSectionToolsFile (InFilePath);
    free (InFilePath);
  } else {
    //
    // Use the pre-defined standard guided tools.
    //
  printf ("\nThe Guid Tool Definition of BfmLib comes from the build-in default configuration. \n");
    mParsedGuidedSectionTools = LibPreDefinedGuidedTools ();
  }

  //
  // BfmLib -e FdName.Fd
  //
  if ((stricmp(Argv[0], "-e") == 0)) {

    if (Argc != 2) {
      return -1;
    }
    //
    // Extract FFS files.
    //
    Status = BfmImageView (Argv[1], NULL, FALSE, &FdData);

    if (EFI_ERROR (Status)) {
      return -1;
    }

    if (FdData == NULL) {
      return -1;
    }

    LibBfmFreeFd(FdData);

  } else if ((stricmp(Argv[0], "-i") == 0)) {
    //
    // Insert FFS files to BFV
    // BfmLib -i InFdName.Fd FfsName.ffs OutFdName.Fd -g FvNameGuid
    //
    if (Argc == 6) {
      mFvGuidIsSet      = TRUE;
      mFvNameGuidString = Argv[5];
      StringToGuid (Argv[5], &mFvNameGuid);
      Argc -= 2;
    }
    if (Argc != 4) {
      return -1;
    }
    Status = BfmImageAdd(Argv[1], Argv[2], Argv[3]);

    if (EFI_ERROR (Status)) {
      return -1;
    }

  } else if ((stricmp(Argv[0], "-r") == 0)) {
    //
    // Replace FFS files in BFV
    // BfmLib -r InFdName.Fd FfsName.ffs OutFdName.Fd -g FvNameGuid
    //
    if (Argc == 6) {
      mFvGuidIsSet      = TRUE;
      mFvNameGuidString = Argv[5];
      StringToGuid (Argv[5], &mFvNameGuid);
      Argc -= 2;
    }
    if (Argc != 4) {
      return -1;
    }
    Status = BfmImageReplace (Argv[1], Argv[2], Argv[3]);

    if (EFI_ERROR (Status)) {
      return -1;
    }

  } else {
    //
    // Invalid parameter.
    //
    return -1;
  }

  return 1;
}

