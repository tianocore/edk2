/** @file
  Main file for Comp shell Debug1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDebug1CommandsLib.h"

STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-n", TypeValue },
  { L"-s", TypeValue },
  { NULL,  TypeMax   }
};

typedef enum {
  OutOfDiffPoint,
  InDiffPoint,
  InPrevDiffPoint
} READ_STATUS;

//
// Buffer type, for reading both file operands in chunks.
//
typedef struct {
  UINT8    *Data;     // dynamically allocated buffer
  UINTN    Allocated; // the allocated size of Data
  UINTN    Next;      // next position in Data to fetch a byte at
  UINTN    Left;      // number of bytes left in Data for fetching at Next
} FILE_BUFFER;

/**
  Function to print differnt point data.

  @param[in]  FileName        File name.
  @param[in]  FileTag         File tag name.
  @param[in]  Buffer          Data buffer to be printed.
  @param[in]  BufferSize      Size of the data to be printed.
  @param[in]  Address         Address of the differnt point.
  @param[in]  DifferentBytes  Total size of the buffer.

**/
VOID
PrintDifferentPoint (
  CONST CHAR16  *FileName,
  CHAR16        *FileTag,
  UINT8         *Buffer,
  UINT64        BufferSize,
  UINTN         Address,
  UINT64        DifferentBytes
  )
{
  UINTN  Index;

  ShellPrintEx (-1, -1, L"%s: %s\r\n  %08x:", FileTag, FileName, Address);

  //
  // Print data in hex-format.
  //
  for (Index = 0; Index < BufferSize; Index++) {
    ShellPrintEx (-1, -1, L" %02x", Buffer[Index]);
  }

  if (BufferSize < DifferentBytes) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_COMP_END_OF_FILE), gShellDebug1HiiHandle);
  }

  ShellPrintEx (-1, -1, L"    *");

  //
  // Print data in char-format.
  //
  for (Index = 0; Index < BufferSize; Index++) {
    if ((Buffer[Index] >= 0x20) && (Buffer[Index] <= 0x7E)) {
      ShellPrintEx (-1, -1, L"%c", Buffer[Index]);
    } else {
      //
      // Print dots for control characters
      //
      ShellPrintEx (-1, -1, L".");
    }
  }

  ShellPrintEx (-1, -1, L"*\r\n");
}

/**
  Initialize a FILE_BUFFER.

  @param[out] FileBuffer  The FILE_BUFFER to initialize. On return, the caller
                          is responsible for checking FileBuffer->Data: if
                          FileBuffer->Data is NULL on output, then memory
                          allocation failed.
**/
STATIC
VOID
FileBufferInit (
  OUT FILE_BUFFER  *FileBuffer
  )
{
  FileBuffer->Allocated = PcdGet32 (PcdShellFileOperationSize);
  FileBuffer->Data      = AllocatePool (FileBuffer->Allocated);
  FileBuffer->Left      = 0;
}

/**
  Uninitialize a FILE_BUFFER.

  @param[in,out] FileBuffer  The FILE_BUFFER to uninitialize. The caller is
                             responsible for making sure FileBuffer was first
                             initialized with FileBufferInit(), successfully or
                             unsuccessfully.
**/
STATIC
VOID
FileBufferUninit (
  IN OUT FILE_BUFFER  *FileBuffer
  )
{
  SHELL_FREE_NON_NULL (FileBuffer->Data);
}

/**
  Read a byte from a SHELL_FILE_HANDLE, buffered with a FILE_BUFFER.

  @param[in] FileHandle      The SHELL_FILE_HANDLE to replenish FileBuffer
                             from, if needed.

  @param[in,out] FileBuffer  The FILE_BUFFER to read a byte from. If FileBuffer
                             is empty on entry, then FileBuffer is refilled
                             from FileHandle, before outputting a byte from
                             FileBuffer to Byte. The caller is responsible for
                             ensuring that FileBuffer was successfully
                             initialized with FileBufferInit().

  @param[out] BytesRead      On successful return, BytesRead is set to 1 if the
                             next byte from FileBuffer has been stored to Byte.
                             On successful return, BytesRead is set to 0 if
                             FileBuffer is empty, and FileHandle is at EOF.
                             When an error is returned, BytesRead is not set.

  @param[out] Byte           On output, the next byte from FileBuffer. Only set
                             if (a) EFI_SUCCESS is returned and (b) BytesRead
                             is set to 1 on output.

  @retval EFI_SUCCESS  BytesRead has been set to 0 or 1. In the latter case,
                       Byte has been set as well.

  @return              Error codes propagated from
                       gEfiShellProtocol->ReadFile().
**/
STATIC
EFI_STATUS
FileBufferReadByte (
  IN     SHELL_FILE_HANDLE  FileHandle,
  IN OUT FILE_BUFFER        *FileBuffer,
  OUT UINTN                 *BytesRead,
  OUT UINT8                 *Byte
  )
{
  UINTN       ReadSize;
  EFI_STATUS  Status;

  if (FileBuffer->Left == 0) {
    ReadSize = FileBuffer->Allocated;
    Status   = gEfiShellProtocol->ReadFile (
                                    FileHandle,
                                    &ReadSize,
                                    FileBuffer->Data
                                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (ReadSize == 0) {
      *BytesRead = 0;
      return EFI_SUCCESS;
    }

    FileBuffer->Next = 0;
    FileBuffer->Left = ReadSize;
  }

  *BytesRead = 1;
  *Byte      = FileBuffer->Data[FileBuffer->Next];

  FileBuffer->Next++;
  FileBuffer->Left--;
  return EFI_SUCCESS;
}

/**
  Function for 'comp' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunComp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS         Status;
  LIST_ENTRY         *Package;
  CHAR16             *ProblemParam;
  CHAR16             *FileName1;
  CHAR16             *FileName2;
  CONST CHAR16       *TempParam;
  SHELL_STATUS       ShellStatus;
  SHELL_FILE_HANDLE  FileHandle1;
  SHELL_FILE_HANDLE  FileHandle2;
  UINT64             Size1;
  UINT64             Size2;
  UINT64             DifferentBytes;
  UINT64             DifferentCount;
  UINT8              DiffPointNumber;
  UINT8              OneByteFromFile1;
  UINT8              OneByteFromFile2;
  UINT8              *DataFromFile1;
  UINT8              *DataFromFile2;
  FILE_BUFFER        FileBuffer1;
  FILE_BUFFER        FileBuffer2;
  UINTN              InsertPosition1;
  UINTN              InsertPosition2;
  UINTN              DataSizeFromFile1;
  UINTN              DataSizeFromFile2;
  UINTN              TempAddress;
  UINTN              Index;
  UINTN              DiffPointAddress;
  READ_STATUS        ReadStatus;

  ShellStatus      = SHELL_SUCCESS;
  Status           = EFI_SUCCESS;
  FileName1        = NULL;
  FileName2        = NULL;
  FileHandle1      = NULL;
  FileHandle2      = NULL;
  DataFromFile1    = NULL;
  DataFromFile2    = NULL;
  ReadStatus       = OutOfDiffPoint;
  DifferentCount   = 10;
  DifferentBytes   = 4;
  DiffPointNumber  = 0;
  InsertPosition1  = 0;
  InsertPosition2  = 0;
  TempAddress      = 0;
  DiffPointAddress = 0;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  Status = CommandInit ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"comp", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetCount (Package) > 3) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"comp");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetCount (Package) < 3) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle, L"comp");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      TempParam = ShellCommandLineGetRawValue (Package, 1);
      ASSERT (TempParam != NULL);
      FileName1 = ShellFindFilePath (TempParam);
      if (FileName1 == NULL) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_FILE_FIND_FAIL), gShellDebug1HiiHandle, L"comp", TempParam);
        ShellStatus = SHELL_NOT_FOUND;
      } else {
        Status = ShellOpenFileByName (FileName1, &FileHandle1, EFI_FILE_MODE_READ, 0);
        if (EFI_ERROR (Status)) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellDebug1HiiHandle, L"comp", TempParam);
          ShellStatus = SHELL_NOT_FOUND;
        }
      }

      TempParam = ShellCommandLineGetRawValue (Package, 2);
      ASSERT (TempParam != NULL);
      FileName2 = ShellFindFilePath (TempParam);
      if (FileName2 == NULL) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_FILE_FIND_FAIL), gShellDebug1HiiHandle, L"comp", TempParam);
        ShellStatus = SHELL_NOT_FOUND;
      } else {
        Status = ShellOpenFileByName (FileName2, &FileHandle2, EFI_FILE_MODE_READ, 0);
        if (EFI_ERROR (Status)) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellDebug1HiiHandle, L"comp", TempParam);
          ShellStatus = SHELL_NOT_FOUND;
        }
      }

      if (ShellStatus == SHELL_SUCCESS) {
        Status = gEfiShellProtocol->GetFileSize (FileHandle1, &Size1);
        ASSERT_EFI_ERROR (Status);
        Status = gEfiShellProtocol->GetFileSize (FileHandle2, &Size2);
        ASSERT_EFI_ERROR (Status);

        if (ShellCommandLineGetFlag (Package, L"-n")) {
          TempParam = ShellCommandLineGetValue (Package, L"-n");
          if (TempParam == NULL) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDebug1HiiHandle, L"comp", L"-n");
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            if (gUnicodeCollation->StriColl (gUnicodeCollation, (CHAR16 *)TempParam, L"all") == 0) {
              DifferentCount = MAX_UINTN;
            } else {
              Status = ShellConvertStringToUint64 (TempParam, &DifferentCount, FALSE, TRUE);
              if (EFI_ERROR (Status) || (DifferentCount == 0)) {
                ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM_VAL), gShellDebug1HiiHandle, L"comp", TempParam, L"-n");
                ShellStatus = SHELL_INVALID_PARAMETER;
              }
            }
          }
        }

        if (ShellCommandLineGetFlag (Package, L"-s")) {
          TempParam = ShellCommandLineGetValue (Package, L"-s");
          if (TempParam == NULL) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDebug1HiiHandle, L"comp", L"-s");
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            Status = ShellConvertStringToUint64 (TempParam, &DifferentBytes, FALSE, TRUE);
            if (EFI_ERROR (Status) || (DifferentBytes == 0)) {
              ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM_VAL), gShellDebug1HiiHandle, L"comp", TempParam, L"-s");
              ShellStatus = SHELL_INVALID_PARAMETER;
            } else {
              if (DifferentBytes > MAX (Size1, Size2)) {
                DifferentBytes = MAX (Size1, Size2);
              }
            }
          }
        }
      }

      if (ShellStatus == SHELL_SUCCESS) {
        DataFromFile1 = AllocateZeroPool ((UINTN)DifferentBytes);
        DataFromFile2 = AllocateZeroPool ((UINTN)DifferentBytes);
        FileBufferInit (&FileBuffer1);
        FileBufferInit (&FileBuffer2);
        if ((DataFromFile1 == NULL) || (DataFromFile2 == NULL) ||
            (FileBuffer1.Data == NULL) || (FileBuffer2.Data == NULL))
        {
          ShellStatus = SHELL_OUT_OF_RESOURCES;
          SHELL_FREE_NON_NULL (DataFromFile1);
          SHELL_FREE_NON_NULL (DataFromFile2);
          FileBufferUninit (&FileBuffer1);
          FileBufferUninit (&FileBuffer2);
        }
      }

      if (ShellStatus == SHELL_SUCCESS) {
        while (DiffPointNumber < DifferentCount) {
          DataSizeFromFile1 = 1;
          DataSizeFromFile2 = 1;
          OneByteFromFile1  = 0;
          OneByteFromFile2  = 0;
          Status            = FileBufferReadByte (
                                FileHandle1,
                                &FileBuffer1,
                                &DataSizeFromFile1,
                                &OneByteFromFile1
                                );
          ASSERT_EFI_ERROR (Status);
          Status = FileBufferReadByte (
                     FileHandle2,
                     &FileBuffer2,
                     &DataSizeFromFile2,
                     &OneByteFromFile2
                     );
          ASSERT_EFI_ERROR (Status);

          TempAddress++;

          //
          // 1.When end of file and no chars in DataFromFile buffer, then break while.
          // 2.If no more char in File1 or File2, The ReadStatus is InPrevDiffPoint forever.
          //   So the previous different point is the last one, then break the while block.
          //
          if (((DataSizeFromFile1 == 0) && (InsertPosition1 == 0) && (DataSizeFromFile2 == 0) && (InsertPosition2 == 0)) ||
              ((ReadStatus == InPrevDiffPoint) && ((DataSizeFromFile1 == 0) || (DataSizeFromFile2 == 0)))
              )
          {
            break;
          }

          if (ReadStatus == OutOfDiffPoint) {
            if (OneByteFromFile1 != OneByteFromFile2) {
              ReadStatus       = InDiffPoint;
              DiffPointAddress = TempAddress;
              if (DataSizeFromFile1 == 1) {
                DataFromFile1[InsertPosition1++] = OneByteFromFile1;
              }

              if (DataSizeFromFile2 == 1) {
                DataFromFile2[InsertPosition2++] = OneByteFromFile2;
              }
            }
          } else if (ReadStatus == InDiffPoint) {
            if (DataSizeFromFile1 == 1) {
              DataFromFile1[InsertPosition1++] = OneByteFromFile1;
            }

            if (DataSizeFromFile2 == 1) {
              DataFromFile2[InsertPosition2++] = OneByteFromFile2;
            }
          } else if (ReadStatus == InPrevDiffPoint) {
            if (OneByteFromFile1 == OneByteFromFile2) {
              ReadStatus = OutOfDiffPoint;
            }
          }

          //
          // ReadStatus should be always equal InDiffPoint.
          //
          if ((InsertPosition1 == DifferentBytes) ||
              (InsertPosition2 == DifferentBytes) ||
              ((DataSizeFromFile1 == 0) && (DataSizeFromFile2 == 0))
              )
          {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_COMP_DIFFERENCE_POINT), gShellDebug1HiiHandle, ++DiffPointNumber);
            PrintDifferentPoint (FileName1, L"File1", DataFromFile1, InsertPosition1, DiffPointAddress, DifferentBytes);
            PrintDifferentPoint (FileName2, L"File2", DataFromFile2, InsertPosition2, DiffPointAddress, DifferentBytes);

            //
            // One of two buffuers is empty, it means this is the last different point.
            //
            if ((InsertPosition1 == 0) || (InsertPosition2 == 0)) {
              break;
            }

            for (Index = 1; Index < InsertPosition1 && Index < InsertPosition2; Index++) {
              if (DataFromFile1[Index] == DataFromFile2[Index]) {
                ReadStatus = OutOfDiffPoint;
                break;
              }
            }

            if (ReadStatus == OutOfDiffPoint) {
              //
              // Try to find a new different point in the rest of DataFromFile.
              //
              for ( ; Index < MAX (InsertPosition1, InsertPosition2); Index++) {
                if (DataFromFile1[Index] != DataFromFile2[Index]) {
                  ReadStatus        = InDiffPoint;
                  DiffPointAddress += Index;
                  break;
                }
              }
            } else {
              //
              // Doesn't find a new different point, still in the same different point.
              //
              ReadStatus = InPrevDiffPoint;
            }

            CopyMem (DataFromFile1, DataFromFile1 + Index, InsertPosition1 - Index);
            CopyMem (DataFromFile2, DataFromFile2 + Index, InsertPosition2 - Index);

            SetMem (DataFromFile1 + InsertPosition1 - Index, (UINTN)DifferentBytes - InsertPosition1 + Index, 0);
            SetMem (DataFromFile2 + InsertPosition2 - Index, (UINTN)DifferentBytes - InsertPosition2 + Index, 0);

            InsertPosition1 -= Index;
            InsertPosition2 -= Index;
          }
        }

        SHELL_FREE_NON_NULL (DataFromFile1);
        SHELL_FREE_NON_NULL (DataFromFile2);
        FileBufferUninit (&FileBuffer1);
        FileBufferUninit (&FileBuffer2);

        if (DiffPointNumber == 0) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_COMP_FOOTER_PASS), gShellDebug1HiiHandle);
        } else {
          ShellStatus = SHELL_NOT_EQUAL;
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_COMP_FOOTER_FAIL), gShellDebug1HiiHandle);
        }
      }
    }

    ShellCommandLineFreeVarList (Package);
  }

  SHELL_FREE_NON_NULL (FileName1);
  SHELL_FREE_NON_NULL (FileName2);

  if (FileHandle1 != NULL) {
    gEfiShellProtocol->CloseFile (FileHandle1);
  }

  if (FileHandle2 != NULL) {
    gEfiShellProtocol->CloseFile (FileHandle2);
  }

  return (ShellStatus);
}
