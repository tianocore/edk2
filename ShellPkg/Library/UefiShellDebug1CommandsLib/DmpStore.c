/** @file
  Main file for DmpStore shell Debug1 function.

  (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDebug1CommandsLib.h"

typedef enum {
  DmpStoreDisplay,
  DmpStoreDelete,
  DmpStoreSave,
  DmpStoreLoad
} DMP_STORE_TYPE;

typedef struct {
  UINT32        Signature;
  CHAR16        *Name;
  EFI_GUID      Guid;
  UINT32        Attributes;
  UINT32        DataSize;
  UINT8         *Data;
  LIST_ENTRY    Link;
} DMP_STORE_VARIABLE;

#define DMP_STORE_VARIABLE_SIGNATURE  SIGNATURE_32 ('_', 'd', 's', 's')

/**
  Base on the input attribute value to return the attribute string.

  @param[in]     Atts           The input attribute value

  @retval The attribute string info.
**/
CHAR16 *
GetAttrType (
  IN CONST UINT32  Atts
  )
{
  UINTN   BufLen;
  CHAR16  *RetString;

  BufLen    = 0;
  RetString = NULL;

  if ((Atts & EFI_VARIABLE_NON_VOLATILE) != 0) {
    StrnCatGrow (&RetString, &BufLen, L"+NV", 0);
  }

  if ((Atts & EFI_VARIABLE_RUNTIME_ACCESS) != 0) {
    StrnCatGrow (&RetString, &BufLen, L"+RT+BS", 0);
  } else if ((Atts & EFI_VARIABLE_BOOTSERVICE_ACCESS) != 0) {
    StrnCatGrow (&RetString, &BufLen, L"+BS", 0);
  }

  if ((Atts & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0) {
    StrnCatGrow (&RetString, &BufLen, L"+HR", 0);
  }

  if ((Atts & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
    StrnCatGrow (&RetString, &BufLen, L"+AW", 0);
  }

  if ((Atts & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) {
    StrnCatGrow (&RetString, &BufLen, L"+AT", 0);
  }

  if (RetString == NULL) {
    RetString = StrnCatGrow (&RetString, &BufLen, L"Invalid", 0);
  }

  if ((RetString != NULL) && (RetString[0] == L'+')) {
    CopyMem (RetString, RetString + 1, StrSize (RetString + 1));
  }

  return RetString;
}

/**
  Convert binary to hex format string.

  @param[in]  Buffer            The binary data.
  @param[in]  BufferSize        The size in bytes of the binary data.
  @param[in, out] HexString     Hex format string.
  @param[in]      HexStringSize The size in bytes of the string.

  @return The hex format string.
**/
CHAR16 *
BinaryToHexString (
  IN     VOID    *Buffer,
  IN     UINTN   BufferSize,
  IN OUT CHAR16  *HexString,
  IN     UINTN   HexStringSize
  )
{
  UINTN  Index;
  UINTN  StringIndex;

  ASSERT (Buffer != NULL);
  ASSERT ((BufferSize * 2 + 1) * sizeof (CHAR16) <= HexStringSize);

  for (Index = 0, StringIndex = 0; Index < BufferSize; Index += 1) {
    StringIndex +=
      UnicodeSPrint (
        &HexString[StringIndex],
        HexStringSize - StringIndex * sizeof (CHAR16),
        L"%02x",
        ((UINT8 *)Buffer)[Index]
        );
  }

  return HexString;
}

/**
  Load the variable data from file and set to variable data base.

  @param[in]  FileHandle     The file to be read.
  @param[in]  Name           The name of the variables to be loaded.
  @param[in]  Guid           The guid of the variables to be loaded.
  @param[out] Found          TRUE when at least one variable was loaded and set.

  @retval SHELL_DEVICE_ERROR      Cannot access the file.
  @retval SHELL_VOLUME_CORRUPTED  The file is in bad format.
  @retval SHELL_OUT_OF_RESOURCES  There is not enough memory to perform the operation.
  @retval SHELL_SUCCESS           Successfully load and set the variables.
**/
SHELL_STATUS
LoadVariablesFromFile (
  IN SHELL_FILE_HANDLE  FileHandle,
  IN CONST CHAR16       *Name,
  IN CONST EFI_GUID     *Guid,
  OUT BOOLEAN           *Found
  )
{
  EFI_STATUS          Status;
  SHELL_STATUS        ShellStatus;
  UINT32              NameSize;
  UINT32              DataSize;
  UINTN               BufferSize;
  UINTN               RemainingSize;
  UINT64              Position;
  UINT64              FileSize;
  LIST_ENTRY          List;
  DMP_STORE_VARIABLE  *Variable;
  LIST_ENTRY          *Link;
  CHAR16              *Attributes;
  UINT8               *Buffer;
  UINT32              Crc32;

  Status = ShellGetFileSize (FileHandle, &FileSize);
  if (EFI_ERROR (Status)) {
    return SHELL_DEVICE_ERROR;
  }

  ShellStatus = SHELL_SUCCESS;

  InitializeListHead (&List);

  Position = 0;
  while (Position < FileSize) {
    //
    // NameSize
    //
    BufferSize = sizeof (NameSize);
    Status     = ShellReadFile (FileHandle, &BufferSize, &NameSize);
    if (EFI_ERROR (Status) || (BufferSize != sizeof (NameSize))) {
      ShellStatus = SHELL_VOLUME_CORRUPTED;
      break;
    }

    //
    // DataSize
    //
    BufferSize = sizeof (DataSize);
    Status     = ShellReadFile (FileHandle, &BufferSize, &DataSize);
    if (EFI_ERROR (Status) || (BufferSize != sizeof (DataSize))) {
      ShellStatus = SHELL_VOLUME_CORRUPTED;
      break;
    }

    //
    // Name, Guid, Attributes, Data, Crc32
    //
    RemainingSize = NameSize + sizeof (EFI_GUID) + sizeof (UINT32) + DataSize + sizeof (Crc32);
    BufferSize    = sizeof (NameSize) + sizeof (DataSize) + RemainingSize;
    Buffer        = AllocatePool (BufferSize);
    if (Buffer == NULL) {
      ShellStatus = SHELL_OUT_OF_RESOURCES;
      break;
    }

    BufferSize = RemainingSize;
    Status     = ShellReadFile (FileHandle, &BufferSize, (UINT32 *)Buffer + 2);
    if (EFI_ERROR (Status) || (BufferSize != RemainingSize)) {
      ShellStatus = SHELL_VOLUME_CORRUPTED;
      FreePool (Buffer);
      break;
    }

    //
    // Check Crc32
    //
    *(UINT32 *)Buffer       = NameSize;
    *((UINT32 *)Buffer + 1) = DataSize;
    BufferSize              = RemainingSize + sizeof (NameSize) + sizeof (DataSize) - sizeof (Crc32);
    gBS->CalculateCrc32 (
           Buffer,
           BufferSize,
           &Crc32
           );
    if (Crc32 != *(UINT32 *)(Buffer + BufferSize)) {
      FreePool (Buffer);
      ShellStatus = SHELL_VOLUME_CORRUPTED;
      break;
    }

    Position += BufferSize + sizeof (Crc32);

    Variable = AllocateZeroPool (sizeof (*Variable) + NameSize + DataSize);
    if (Variable == NULL) {
      FreePool (Buffer);
      ShellStatus = SHELL_OUT_OF_RESOURCES;
      break;
    }

    Variable->Signature = DMP_STORE_VARIABLE_SIGNATURE;
    Variable->Name      = (CHAR16 *)(Variable + 1);
    Variable->DataSize  = DataSize;
    Variable->Data      = (UINT8 *)Variable->Name + NameSize;
    CopyMem (Variable->Name, Buffer + sizeof (NameSize) + sizeof (DataSize), NameSize);
    CopyMem (&Variable->Guid, Buffer + sizeof (NameSize) + sizeof (DataSize) + NameSize, sizeof (EFI_GUID));
    CopyMem (&Variable->Attributes, Buffer + sizeof (NameSize) + sizeof (DataSize) + NameSize + sizeof (EFI_GUID), sizeof (UINT32));
    CopyMem (Variable->Data, Buffer + sizeof (NameSize) + sizeof (DataSize) + NameSize + sizeof (EFI_GUID) + sizeof (UINT32), DataSize);

    InsertTailList (&List, &Variable->Link);
    FreePool (Buffer);
  }

  if ((Position != FileSize) || (ShellStatus != SHELL_SUCCESS)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_LOAD_BAD_FILE), gShellDebug1HiiHandle, L"dmpstore");
    if (Position != FileSize) {
      ShellStatus = SHELL_VOLUME_CORRUPTED;
    }
  }

  for ( Link = GetFirstNode (&List)
        ; !IsNull (&List, Link) && (ShellStatus == SHELL_SUCCESS)
        ; Link = GetNextNode (&List, Link)
        )
  {
    Variable = CR (Link, DMP_STORE_VARIABLE, Link, DMP_STORE_VARIABLE_SIGNATURE);

    if (((Name == NULL) || gUnicodeCollation->MetaiMatch (gUnicodeCollation, Variable->Name, (CHAR16 *)Name)) &&
        ((Guid == NULL) || CompareGuid (&Variable->Guid, Guid))
        )
    {
      Attributes = GetAttrType (Variable->Attributes);
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_DMPSTORE_HEADER_LINE),
        gShellDebug1HiiHandle,
        Attributes,
        &Variable->Guid,
        Variable->Name,
        Variable->DataSize
        );
      SHELL_FREE_NON_NULL (Attributes);

      *Found = TRUE;
      Status = gRT->SetVariable (
                      Variable->Name,
                      &Variable->Guid,
                      Variable->Attributes,
                      Variable->DataSize,
                      Variable->Data
                      );
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_LOAD_GEN_FAIL), gShellDebug1HiiHandle, L"dmpstore", Variable->Name, Status);
      }
    }
  }

  for (Link = GetFirstNode (&List); !IsNull (&List, Link); ) {
    Variable = CR (Link, DMP_STORE_VARIABLE, Link, DMP_STORE_VARIABLE_SIGNATURE);
    Link     = RemoveEntryList (&Variable->Link);
    FreePool (Variable);
  }

  return ShellStatus;
}

/**
  Append one variable to file.

  @param[in] FileHandle        The file to be appended.
  @param[in] Name              The variable name.
  @param[in] Guid              The variable GUID.
  @param[in] Attributes        The variable attributes.
  @param[in] DataSize          The variable data size.
  @param[in] Data              The variable data.

  @retval EFI_OUT_OF_RESOURCES  There is not enough memory to perform the operation.
  @retval EFI_SUCCESS           The variable is appended to file successfully.
  @retval others                Failed to append the variable to file.
**/
EFI_STATUS
AppendSingleVariableToFile (
  IN SHELL_FILE_HANDLE  FileHandle,
  IN CONST CHAR16       *Name,
  IN CONST EFI_GUID     *Guid,
  IN UINT32             Attributes,
  IN UINT32             DataSize,
  IN CONST UINT8        *Data
  )
{
  UINT32      NameSize;
  UINT8       *Buffer;
  UINT8       *Ptr;
  UINTN       BufferSize;
  EFI_STATUS  Status;

  NameSize   = (UINT32)StrSize (Name);
  BufferSize = sizeof (NameSize) + sizeof (DataSize)
               + sizeof (*Guid)
               + sizeof (Attributes)
               + NameSize + DataSize
               + sizeof (UINT32);

  Buffer = AllocatePool (BufferSize);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr = Buffer;
  //
  // NameSize and DataSize
  //
  *(UINT32 *)Ptr = NameSize;
  Ptr           += sizeof (NameSize);
  *(UINT32 *)Ptr = DataSize;
  Ptr           += sizeof (DataSize);

  //
  // Name
  //
  CopyMem (Ptr, Name, NameSize);
  Ptr += NameSize;

  //
  // Guid
  //
  CopyMem (Ptr, Guid, sizeof (*Guid));
  Ptr += sizeof (*Guid);

  //
  // Attributes
  //
  *(UINT32 *)Ptr = Attributes;
  Ptr           += sizeof (Attributes);

  //
  // Data
  //
  CopyMem (Ptr, Data, DataSize);
  Ptr += DataSize;

  //
  // Crc32
  //
  gBS->CalculateCrc32 (Buffer, (UINTN)Ptr - (UINTN)Buffer, (UINT32 *)Ptr);

  Status = ShellWriteFile (FileHandle, &BufferSize, Buffer);
  FreePool (Buffer);

  if (!EFI_ERROR (Status) &&
      (BufferSize != sizeof (NameSize) + sizeof (DataSize) + sizeof (*Guid) + sizeof (Attributes) + NameSize + DataSize + sizeof (UINT32))
      )
  {
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

/**
  Recursive function to display or delete variables.

  This function will call itself to create a stack-based list of allt he variables to process,
  then fromt he last to the first, they will do either printing or deleting.

  This is necessary since once a delete happens GetNextVariableName() will work.

  @param[in] Name                 The variable name of the EFI variable (or NULL).
  @param[in] Guid                 The GUID of the variable set (or NULL).
  @param[in] Type                 The operation type.
  @param[in] FileHandle           The file to operate on (or NULL).
  @param[in] PrevName             The previous variable name from GetNextVariableName. L"" to start.
  @param[in] FoundVarGuid         The previous GUID from GetNextVariableName. ignored at start.
  @param[in] FoundOne             If a VariableName or Guid was specified and one was printed or
                                  deleted, then set this to TRUE, otherwise ignored.
  @param[in] StandardFormatOutput TRUE indicates Standard-Format Output.

  @retval SHELL_SUCCESS           The operation was successful.
  @retval SHELL_OUT_OF_RESOURCES  A memorty allocation failed.
  @retval SHELL_ABORTED           The abort message was received.
  @retval SHELL_DEVICE_ERROR      UEFI Variable Services returned an error.
  @retval SHELL_NOT_FOUND         the Name/Guid pair could not be found.
**/
SHELL_STATUS
CascadeProcessVariables (
  IN CONST CHAR16              *Name        OPTIONAL,
  IN CONST EFI_GUID            *Guid        OPTIONAL,
  IN DMP_STORE_TYPE            Type,
  IN EFI_FILE_PROTOCOL         *FileHandle  OPTIONAL,
  IN CONST CHAR16      *CONST  PrevName,
  IN EFI_GUID                  FoundVarGuid,
  IN BOOLEAN                   *FoundOne,
  IN BOOLEAN                   StandardFormatOutput
  )
{
  EFI_STATUS    Status;
  CHAR16        *FoundVarName;
  UINT8         *DataBuffer;
  UINTN         DataSize;
  UINT32        Atts;
  SHELL_STATUS  ShellStatus;
  UINTN         NameSize;
  CHAR16        *AttrString;
  CHAR16        *HexString;
  EFI_STATUS    SetStatus;
  CONST CHAR16  *GuidName;

  if (ShellGetExecutionBreakFlag ()) {
    return (SHELL_ABORTED);
  }

  NameSize     = 0;
  FoundVarName = NULL;

  if (PrevName != NULL) {
    StrnCatGrow (&FoundVarName, &NameSize, PrevName, 0);
  } else {
    FoundVarName = AllocateZeroPool (sizeof (CHAR16));
    NameSize     = sizeof (CHAR16);
  }

  Status = gRT->GetNextVariableName (&NameSize, FoundVarName, &FoundVarGuid);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    SHELL_FREE_NON_NULL (FoundVarName);
    FoundVarName = AllocateZeroPool (NameSize);
    if (FoundVarName != NULL) {
      if (PrevName != NULL) {
        StrnCpyS (FoundVarName, NameSize/sizeof (CHAR16), PrevName, NameSize/sizeof (CHAR16) - 1);
      }

      Status = gRT->GetNextVariableName (&NameSize, FoundVarName, &FoundVarGuid);
    } else {
      Status = EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // No more is fine.
  //
  if (Status == EFI_NOT_FOUND) {
    SHELL_FREE_NON_NULL (FoundVarName);
    return (SHELL_SUCCESS);
  } else if (EFI_ERROR (Status)) {
    SHELL_FREE_NON_NULL (FoundVarName);
    return (SHELL_DEVICE_ERROR);
  }

  //
  // Recurse to the next iteration.  We know "our" variable's name.
  //
  ShellStatus = CascadeProcessVariables (Name, Guid, Type, FileHandle, FoundVarName, FoundVarGuid, FoundOne, StandardFormatOutput);

  if (ShellGetExecutionBreakFlag () || (ShellStatus == SHELL_ABORTED)) {
    SHELL_FREE_NON_NULL (FoundVarName);
    return (SHELL_ABORTED);
  }

  //
  // No matter what happened we process our own variable
  // Only continue if Guid and VariableName are each either NULL or a match
  //
  if (  (  (Name == NULL)
        || gUnicodeCollation->MetaiMatch (gUnicodeCollation, FoundVarName, (CHAR16 *)Name))
     && (  (Guid == NULL)
        || CompareGuid (&FoundVarGuid, Guid))
        )
  {
    DataSize   = 0;
    DataBuffer = NULL;
    //
    // do the print or delete
    //
    *FoundOne = TRUE;
    Status    = gRT->GetVariable (FoundVarName, &FoundVarGuid, &Atts, &DataSize, DataBuffer);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      SHELL_FREE_NON_NULL (DataBuffer);
      DataBuffer = AllocatePool (DataSize);
      if (DataBuffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
      } else {
        Status = gRT->GetVariable (FoundVarName, &FoundVarGuid, &Atts, &DataSize, DataBuffer);
      }
    }

    //
    // Last error check then print this variable out.
    //
    if (Type == DmpStoreDisplay) {
      if (!EFI_ERROR (Status) && (DataBuffer != NULL) && (FoundVarName != NULL)) {
        AttrString = GetAttrType (Atts);
        if (StandardFormatOutput) {
          HexString = AllocatePool ((DataSize * 2 + 1) * sizeof (CHAR16));
          if (HexString != NULL) {
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_DMPSTORE_VAR_SFO),
              gShellDebug1HiiHandle,
              FoundVarName,
              &FoundVarGuid,
              Atts,
              DataSize,
              BinaryToHexString (
                DataBuffer,
                DataSize,
                HexString,
                (DataSize * 2 + 1) * sizeof (CHAR16)
                )
              );
            FreePool (HexString);
          } else {
            Status = EFI_OUT_OF_RESOURCES;
          }
        } else {
          Status = gEfiShellProtocol->GetGuidName (&FoundVarGuid, &GuidName);
          if (EFI_ERROR (Status)) {
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_DMPSTORE_HEADER_LINE),
              gShellDebug1HiiHandle,
              AttrString,
              &FoundVarGuid,
              FoundVarName,
              DataSize
              );
          } else {
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_DMPSTORE_HEADER_LINE2),
              gShellDebug1HiiHandle,
              AttrString,
              GuidName,
              FoundVarName,
              DataSize
              );
          }

          DumpHex (2, 0, DataSize, DataBuffer);
        }

        SHELL_FREE_NON_NULL (AttrString);
      }
    } else if (Type == DmpStoreSave) {
      if (!EFI_ERROR (Status) && (DataBuffer != NULL) && (FoundVarName != NULL)) {
        AttrString = GetAttrType (Atts);
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_DMPSTORE_HEADER_LINE),
          gShellDebug1HiiHandle,
          AttrString,
          &FoundVarGuid,
          FoundVarName,
          DataSize
          );
        Status = AppendSingleVariableToFile (
                   FileHandle,
                   FoundVarName,
                   &FoundVarGuid,
                   Atts,
                   (UINT32)DataSize,
                   DataBuffer
                   );
        SHELL_FREE_NON_NULL (AttrString);
      }
    } else if (Type == DmpStoreDelete) {
      //
      // We only need name to delete it...
      //
      SetStatus = gRT->SetVariable (FoundVarName, &FoundVarGuid, Atts, 0, NULL);
      if (StandardFormatOutput) {
        if (SetStatus == EFI_SUCCESS) {
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_DMPSTORE_NO_VAR_FOUND_NG_SFO),
            gShellDebug1HiiHandle,
            FoundVarName,
            &FoundVarGuid
            );
        }
      } else {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_DMPSTORE_DELETE_LINE),
          gShellDebug1HiiHandle,
          &FoundVarGuid,
          FoundVarName,
          SetStatus
          );
      }
    }

    SHELL_FREE_NON_NULL (DataBuffer);
  }

  SHELL_FREE_NON_NULL (FoundVarName);

  if (Status == EFI_DEVICE_ERROR) {
    ShellStatus = SHELL_DEVICE_ERROR;
  } else if (Status == EFI_SECURITY_VIOLATION) {
    ShellStatus = SHELL_SECURITY_VIOLATION;
  } else if (EFI_ERROR (Status)) {
    ShellStatus = SHELL_NOT_READY;
  }

  return (ShellStatus);
}

/**
  Function to display or delete variables.  This will set up and call into the recursive function.

  @param[in] Name                 The variable name of the EFI variable (or NULL).
  @param[in] Guid                 The GUID of the variable set (or NULL).
  @param[in] Type                 The operation type.
  @param[in] FileHandle           The file to save or load variables.
  @param[in] StandardFormatOutput TRUE indicates Standard-Format Output.

  @retval SHELL_SUCCESS           The operation was successful.
  @retval SHELL_OUT_OF_RESOURCES  A memorty allocation failed.
  @retval SHELL_ABORTED           The abort message was received.
  @retval SHELL_DEVICE_ERROR      UEFI Variable Services returned an error.
  @retval SHELL_NOT_FOUND         the Name/Guid pair could not be found.
**/
SHELL_STATUS
ProcessVariables (
  IN CONST CHAR16       *Name      OPTIONAL,
  IN CONST EFI_GUID     *Guid      OPTIONAL,
  IN DMP_STORE_TYPE     Type,
  IN SHELL_FILE_HANDLE  FileHandle OPTIONAL,
  IN BOOLEAN            StandardFormatOutput
  )
{
  SHELL_STATUS  ShellStatus;
  BOOLEAN       Found;
  EFI_GUID      FoundVarGuid;

  Found       = FALSE;
  ShellStatus = SHELL_SUCCESS;
  ZeroMem (&FoundVarGuid, sizeof (EFI_GUID));

  if (StandardFormatOutput) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_SFO_HEADER), gShellDebug1HiiHandle, L"dmpstore");
  }

  if (Type == DmpStoreLoad) {
    ShellStatus = LoadVariablesFromFile (FileHandle, Name, Guid, &Found);
  } else {
    ShellStatus = CascadeProcessVariables (Name, Guid, Type, FileHandle, NULL, FoundVarGuid, &Found, StandardFormatOutput);
  }

  if (!Found) {
    if (ShellStatus == SHELL_OUT_OF_RESOURCES) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellDebug1HiiHandle, L"dmpstore");
      return (ShellStatus);
    } else if ((Name != NULL) && (Guid == NULL)) {
      if (StandardFormatOutput) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_NO_VAR_FOUND_N_SFO), gShellDebug1HiiHandle, Name);
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_NO_VAR_FOUND_N), gShellDebug1HiiHandle, L"dmpstore", Name);
      }
    } else if ((Name != NULL) && (Guid != NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_NO_VAR_FOUND_GN), gShellDebug1HiiHandle, L"dmpstore", Guid, Name);
    } else if ((Name == NULL) && (Guid == NULL)) {
      if (StandardFormatOutput) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_NO_VAR_FOUND_SFO), gShellDebug1HiiHandle);
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_NO_VAR_FOUND), gShellDebug1HiiHandle, L"dmpstore");
      }
    } else if ((Name == NULL) && (Guid != NULL)) {
      if (StandardFormatOutput) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_NO_VAR_FOUND_G_SFO), gShellDebug1HiiHandle, Guid);
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_NO_VAR_FOUND_G), gShellDebug1HiiHandle, L"dmpstore", Guid);
      }
    }

    return (SHELL_NOT_FOUND);
  }

  return (ShellStatus);
}

STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-d",    TypeFlag  },
  { L"-l",    TypeValue },
  { L"-s",    TypeValue },
  { L"-all",  TypeFlag  },
  { L"-guid", TypeValue },
  { L"-sfo",  TypeFlag  },
  { NULL,     TypeMax   }
};

/**
  Function for 'dmpstore' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunDmpStore (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS         Status;
  RETURN_STATUS      RStatus;
  LIST_ENTRY         *Package;
  CHAR16             *ProblemParam;
  SHELL_STATUS       ShellStatus;
  CONST CHAR16       *GuidStr;
  CONST CHAR16       *File;
  EFI_GUID           *Guid;
  EFI_GUID           GuidData;
  CONST CHAR16       *Name;
  DMP_STORE_TYPE     Type;
  SHELL_FILE_HANDLE  FileHandle;
  EFI_FILE_INFO      *FileInfo;
  BOOLEAN            StandardFormatOutput;

  ShellStatus          = SHELL_SUCCESS;
  Package              = NULL;
  FileHandle           = NULL;
  File                 = NULL;
  Type                 = DmpStoreDisplay;
  StandardFormatOutput = FALSE;

  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"dmpstore", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetCount (Package) > 2) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"dmpstore");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetFlag (Package, L"-all") && ShellCommandLineGetFlag (Package, L"-guid")) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CONFLICT), gShellDebug1HiiHandle, L"dmpstore", L"-all", L"-guid");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetFlag (Package, L"-s") && ShellCommandLineGetFlag (Package, L"-l")) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CONFLICT), gShellDebug1HiiHandle, L"dmpstore", L"-l", L"-s");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if ((ShellCommandLineGetFlag (Package, L"-s") || ShellCommandLineGetFlag (Package, L"-l")) && ShellCommandLineGetFlag (Package, L"-d")) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CONFLICT), gShellDebug1HiiHandle, L"dmpstore", L"-l or -s", L"-d");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if ((ShellCommandLineGetFlag (Package, L"-s") || ShellCommandLineGetFlag (Package, L"-l")) && ShellCommandLineGetFlag (Package, L"-sfo")) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CONFLICT), gShellDebug1HiiHandle, L"dmpstore", L"-l or -s", L"-sfo");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      //
      // Determine the GUID to search for based on -all and -guid parameters
      //
      if (!ShellCommandLineGetFlag (Package, L"-all")) {
        GuidStr = ShellCommandLineGetValue (Package, L"-guid");
        if (GuidStr != NULL) {
          RStatus = StrToGuid (GuidStr, &GuidData);
          if (RETURN_ERROR (RStatus) || (GuidStr[GUID_STRING_LENGTH] != L'\0')) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"dmpstore", GuidStr);
            ShellStatus = SHELL_INVALID_PARAMETER;
          }

          Guid = &GuidData;
        } else {
          Guid = &gEfiGlobalVariableGuid;
        }
      } else {
        Guid = NULL;
      }

      //
      // Get the Name of the variable to find
      //
      Name = ShellCommandLineGetRawValue (Package, 1);

      if (ShellStatus == SHELL_SUCCESS) {
        if (ShellCommandLineGetFlag (Package, L"-s")) {
          Type = DmpStoreSave;
          File = ShellCommandLineGetValue (Package, L"-s");
          if (File == NULL) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDebug1HiiHandle, L"dmpstore", L"-s");
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            Status = ShellOpenFileByName (File, &FileHandle, EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, 0);
            if (!EFI_ERROR (Status)) {
              //
              // Delete existing file, but do not delete existing directory
              //
              FileInfo = ShellGetFileInfo (FileHandle);
              if (FileInfo == NULL) {
                ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellDebug1HiiHandle, L"dmpstore", File);
                Status = EFI_DEVICE_ERROR;
              } else {
                if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) == EFI_FILE_DIRECTORY) {
                  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_IS_DIRECTORY), gShellDebug1HiiHandle, L"dmpstore", File);
                  Status = EFI_INVALID_PARAMETER;
                } else {
                  Status = ShellDeleteFile (&FileHandle);
                  if (EFI_ERROR (Status)) {
                    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_DELETE_FAIL), gShellDebug1HiiHandle, L"dmpstore", File);
                  }
                }

                FreePool (FileInfo);
              }
            } else if (Status == EFI_NOT_FOUND) {
              //
              // Good when file doesn't exist
              //
              Status = EFI_SUCCESS;
            } else {
              //
              // Otherwise it's bad.
              //
              ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellDebug1HiiHandle, L"dmpstore", File);
            }

            if (!EFI_ERROR (Status)) {
              Status = ShellOpenFileByName (File, &FileHandle, EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, 0);
              if (EFI_ERROR (Status)) {
                ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellDebug1HiiHandle, L"dmpstore", File);
              }
            }

            if (EFI_ERROR (Status)) {
              ShellStatus = SHELL_INVALID_PARAMETER;
            }
          }
        } else if (ShellCommandLineGetFlag (Package, L"-l")) {
          Type = DmpStoreLoad;
          File = ShellCommandLineGetValue (Package, L"-l");
          if (File == NULL) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDebug1HiiHandle, L"dmpstore", L"-l");
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            Status = ShellOpenFileByName (File, &FileHandle, EFI_FILE_MODE_READ, 0);
            if (EFI_ERROR (Status)) {
              ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellDebug1HiiHandle, L"dmpstore", File);
              ShellStatus = SHELL_INVALID_PARAMETER;
            } else {
              FileInfo = ShellGetFileInfo (FileHandle);
              if (FileInfo == NULL) {
                ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellDebug1HiiHandle, L"dmpstore", File);
                ShellStatus = SHELL_DEVICE_ERROR;
              } else {
                if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) == EFI_FILE_DIRECTORY) {
                  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_IS_DIRECTORY), gShellDebug1HiiHandle, L"dmpstore", File);
                  ShellStatus = SHELL_INVALID_PARAMETER;
                }

                FreePool (FileInfo);
              }
            }
          }
        } else if (ShellCommandLineGetFlag (Package, L"-d")) {
          Type = DmpStoreDelete;
        }

        if (ShellCommandLineGetFlag (Package, L"-sfo")) {
          StandardFormatOutput = TRUE;
        }
      }

      if (ShellStatus == SHELL_SUCCESS) {
        if (Type == DmpStoreSave) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_SAVE), gShellDebug1HiiHandle, File);
        } else if (Type == DmpStoreLoad) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_LOAD), gShellDebug1HiiHandle, File);
        }

        ShellStatus = ProcessVariables (Name, Guid, Type, FileHandle, StandardFormatOutput);
        if ((Type == DmpStoreLoad) || (Type == DmpStoreSave)) {
          ShellCloseFile (&FileHandle);
        }
      }
    }
  }

  if (Package != NULL) {
    ShellCommandLineFreeVarList (Package);
  }

  return ShellStatus;
}
