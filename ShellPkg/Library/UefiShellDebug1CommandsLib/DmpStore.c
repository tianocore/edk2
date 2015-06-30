/** @file
  Main file for DmpStore shell Debug1 function.
   
  (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2005 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDebug1CommandsLib.h"

typedef enum {
  DmpStoreDisplay,
  DmpStoreDelete,
  DmpStoreSave,
  DmpStoreLoad
} DMP_STORE_TYPE;

typedef struct {
  UINT32     Signature;
  CHAR16     *Name;
  EFI_GUID   Guid;
  UINT32     Attributes;
  UINT32     DataSize;
  UINT8      *Data;
  LIST_ENTRY Link;
} DMP_STORE_VARIABLE;

#define DMP_STORE_VARIABLE_SIGNATURE  SIGNATURE_32 ('_', 'd', 's', 's')

/**
  Base on the input attribute value to return the attribute string.

  @param[in]     Atts           The input attribute value

  @retval The attribute string info.
**/
CHAR16 *
EFIAPI
GetAttrType (
  IN CONST UINT32 Atts
  )
{
  UINTN  BufLen;
  CHAR16 *RetString;

  BufLen      = 0;
  RetString   = NULL;
 
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
    RetString = StrnCatGrow(&RetString, &BufLen, L"Invalid", 0);
  }

  if ((RetString != NULL) && (RetString[0] == L'+')) {
    CopyMem(RetString, RetString + 1, StrSize(RetString + 1));
  }

  return RetString;
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
  IN SHELL_FILE_HANDLE FileHandle,
  IN CONST CHAR16      *Name,
  IN CONST EFI_GUID    *Guid,
  OUT BOOLEAN          *Found
  )
{
  EFI_STATUS           Status;
  SHELL_STATUS         ShellStatus;
  UINT32               NameSize;
  UINT32               DataSize;
  UINTN                BufferSize;
  UINTN                RemainingSize;
  UINT64               Position;
  UINT64               FileSize;
  LIST_ENTRY           List;
  DMP_STORE_VARIABLE   *Variable;
  LIST_ENTRY           *Link;
  CHAR16               *Attributes;
  UINT8                *Buffer;
  UINT32               Crc32;

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
    Status = ShellReadFile (FileHandle, &BufferSize, &NameSize);
    if (EFI_ERROR (Status) || (BufferSize != sizeof (NameSize))) {
      ShellStatus = SHELL_VOLUME_CORRUPTED;
      break;
    }

    //
    // DataSize
    //
    BufferSize = sizeof (DataSize);
    Status = ShellReadFile (FileHandle, &BufferSize, &DataSize);
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
    BufferSize    = RemainingSize;
    Status = ShellReadFile (FileHandle, &BufferSize, (UINT32 *) Buffer + 2);
    if (EFI_ERROR (Status) || (BufferSize != RemainingSize)) {
      ShellStatus = SHELL_VOLUME_CORRUPTED;
      FreePool (Buffer);
      break;
    }

    //
    // Check Crc32
    //
    * (UINT32 *) Buffer       = NameSize;
    * ((UINT32 *) Buffer + 1) = DataSize;
    BufferSize = RemainingSize + sizeof (NameSize) + sizeof (DataSize) - sizeof (Crc32);
    gBS->CalculateCrc32 (
           Buffer,
           BufferSize,
           &Crc32
           );
    if (Crc32 != * (UINT32 *) (Buffer + BufferSize)) {
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
    Variable->Name      = (CHAR16 *) (Variable + 1);
    Variable->DataSize  = DataSize;
    Variable->Data      = (UINT8 *) Variable->Name + NameSize;
    CopyMem (Variable->Name,        Buffer + sizeof (NameSize) + sizeof (DataSize),                                                  NameSize);
    CopyMem (&Variable->Guid,       Buffer + sizeof (NameSize) + sizeof (DataSize) + NameSize,                                       sizeof (EFI_GUID));
    CopyMem (&Variable->Attributes, Buffer + sizeof (NameSize) + sizeof (DataSize) + NameSize + sizeof (EFI_GUID),                   sizeof (UINT32));
    CopyMem (Variable->Data,        Buffer + sizeof (NameSize) + sizeof (DataSize) + NameSize + sizeof (EFI_GUID) + sizeof (UINT32), DataSize);

    InsertTailList (&List, &Variable->Link);
    FreePool (Buffer);
  }
    
  if ((Position != FileSize) || (ShellStatus != SHELL_SUCCESS)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_LOAD_BAD_FILE), gShellDebug1HiiHandle, L"dmpstore");  
    if (Position != FileSize) {
      ShellStatus = SHELL_VOLUME_CORRUPTED;
    }
  }
  
  for ( Link = GetFirstNode (&List)
      ; !IsNull (&List, Link) && (ShellStatus == SHELL_SUCCESS)
      ; Link = GetNextNode (&List, Link)
      ) {
    Variable = CR (Link, DMP_STORE_VARIABLE, Link, DMP_STORE_VARIABLE_SIGNATURE);
    
    if (((Name == NULL) || gUnicodeCollation->MetaiMatch (gUnicodeCollation, Variable->Name, (CHAR16 *) Name)) &&
        ((Guid == NULL) || CompareGuid (&Variable->Guid, Guid))
       ) {
      Attributes = GetAttrType (Variable->Attributes);
      ShellPrintHiiEx (
        -1, -1, NULL, STRING_TOKEN(STR_DMPSTORE_HEADER_LINE), gShellDebug1HiiHandle,
        Attributes, &Variable->Guid, Variable->Name, Variable->DataSize
        );
      SHELL_FREE_NON_NULL(Attributes);

      *Found = TRUE;
      Status = gRT->SetVariable (
                      Variable->Name,
                      &Variable->Guid,
                      Variable->Attributes,
                      Variable->DataSize,
                      Variable->Data
                      );
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_LOAD_GEN_FAIL), gShellDebug1HiiHandle, L"dmpstore", Variable->Name, Status);  
      }
    }
  }

  for (Link = GetFirstNode (&List); !IsNull (&List, Link); ) {
    Variable = CR (Link, DMP_STORE_VARIABLE, Link, DMP_STORE_VARIABLE_SIGNATURE);
    Link = RemoveEntryList (&Variable->Link);
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
  IN SHELL_FILE_HANDLE FileHandle,
  IN CONST CHAR16      *Name,
  IN CONST EFI_GUID    *Guid,
  IN UINT32            Attributes,
  IN UINT32            DataSize,
  IN CONST UINT8       *Data
  )
{
  UINT32              NameSize;
  UINT8               *Buffer;
  UINT8               *Ptr;
  UINTN               BufferSize;
  EFI_STATUS          Status;

  NameSize   = (UINT32) StrSize (Name);
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
  * (UINT32 *) Ptr = NameSize;
  Ptr += sizeof (NameSize);
  *(UINT32 *) Ptr = DataSize;
  Ptr += sizeof (DataSize);

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
  * (UINT32 *) Ptr = Attributes;
  Ptr += sizeof (Attributes);

  //
  // Data
  //
  CopyMem (Ptr, Data, DataSize);
  Ptr += DataSize;

  //
  // Crc32
  //
  gBS->CalculateCrc32 (Buffer, (UINTN) (Ptr - Buffer), (UINT32 *) Ptr);

  Status = ShellWriteFile (FileHandle, &BufferSize, Buffer);
  FreePool (Buffer);

  if (!EFI_ERROR (Status) && 
      (BufferSize != sizeof (NameSize) + sizeof (DataSize) + sizeof (*Guid) + sizeof (Attributes) + NameSize + DataSize + sizeof (UINT32))
    ) {
    Status = EFI_DEVICE_ERROR;
  }
  
  return Status;
}

/**
  Recursive function to display or delete variables.

  This function will call itself to create a stack-based list of allt he variables to process, 
  then fromt he last to the first, they will do either printing or deleting.

  This is necessary since once a delete happens GetNextVariableName() will work.

  @param[in] Name           The variable name of the EFI variable (or NULL).
  @param[in] Guid           The GUID of the variable set (or NULL).
  @param[in] Type           The operation type.
  @param[in] FileHandle     The file to operate on (or NULL).
  @param[in] PrevName       The previous variable name from GetNextVariableName. L"" to start.
  @param[in] FoundVarGuid   The previous GUID from GetNextVariableName. ignored at start.
  @param[in] FoundOne       If a VariableName or Guid was specified and one was printed or
                            deleted, then set this to TRUE, otherwise ignored.

  @retval SHELL_SUCCESS           The operation was successful.
  @retval SHELL_OUT_OF_RESOURCES  A memorty allocation failed.
  @retval SHELL_ABORTED           The abort message was received.
  @retval SHELL_DEVICE_ERROR      UEFI Variable Services returned an error.
  @retval SHELL_NOT_FOUND         the Name/Guid pair could not be found.
**/
SHELL_STATUS
EFIAPI
CascadeProcessVariables (
  IN CONST CHAR16      *Name        OPTIONAL,
  IN CONST EFI_GUID    *Guid        OPTIONAL,
  IN DMP_STORE_TYPE    Type,
  IN EFI_FILE_PROTOCOL *FileHandle  OPTIONAL,
  IN CONST CHAR16      * CONST PrevName,
  IN EFI_GUID          FoundVarGuid,
  IN BOOLEAN           *FoundOne
  )
{
  EFI_STATUS                Status;
  CHAR16                    *FoundVarName;
  UINT8                     *DataBuffer;
  UINTN                     DataSize;
  UINT32                    Atts;
  SHELL_STATUS              ShellStatus;
  UINTN                     NameSize;
  CHAR16                    *RetString;

  if (ShellGetExecutionBreakFlag()) {
    return (SHELL_ABORTED);
  }

  NameSize      = 0;
  FoundVarName  = NULL;

  if (PrevName!=NULL) {
    StrnCatGrow(&FoundVarName, &NameSize, PrevName, 0);
  } else {
    FoundVarName = AllocateZeroPool(sizeof(CHAR16));
  }

  Status = gRT->GetNextVariableName (&NameSize, FoundVarName, &FoundVarGuid);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    SHELL_FREE_NON_NULL(FoundVarName);
    FoundVarName = AllocateZeroPool (NameSize);
    if (FoundVarName != NULL) {
      if (PrevName != NULL) {
        StrCpyS(FoundVarName, NameSize/sizeof(CHAR16), PrevName);
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
    SHELL_FREE_NON_NULL(FoundVarName);
    return (SHELL_SUCCESS);
  } else if (EFI_ERROR(Status)) {
    SHELL_FREE_NON_NULL(FoundVarName);
    return (SHELL_DEVICE_ERROR);
  }

  //
  // Recurse to the next iteration.  We know "our" variable's name.
  //
  ShellStatus = CascadeProcessVariables(Name, Guid, Type, FileHandle, FoundVarName, FoundVarGuid, FoundOne);

  if (ShellGetExecutionBreakFlag() || (ShellStatus == SHELL_ABORTED)) {
    SHELL_FREE_NON_NULL(FoundVarName);
    return (SHELL_ABORTED);
  }

  //
  // No matter what happened we process our own variable
  // Only continue if Guid and VariableName are each either NULL or a match
  //
  if ( ( Name == NULL 
      || gUnicodeCollation->MetaiMatch(gUnicodeCollation, FoundVarName, (CHAR16*) Name) )
     && ( Guid == NULL 
      || CompareGuid(&FoundVarGuid, Guid) )
      ) {
    DataSize      = 0;
    DataBuffer    = NULL;
    //
    // do the print or delete
    //
    *FoundOne = TRUE;
    Status = gRT->GetVariable (FoundVarName, &FoundVarGuid, &Atts, &DataSize, DataBuffer);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      SHELL_FREE_NON_NULL (DataBuffer);
      DataBuffer = AllocatePool (DataSize);
      if (DataBuffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
      } else {
        Status = gRT->GetVariable (FoundVarName, &FoundVarGuid, &Atts, &DataSize, DataBuffer);
      }
    }
    if ((Type == DmpStoreDisplay) || (Type == DmpStoreSave)) {
      //
      // Last error check then print this variable out.
      //
      if (!EFI_ERROR(Status) && (DataBuffer != NULL) && (FoundVarName != NULL)) {
        RetString = GetAttrType(Atts);
        ShellPrintHiiEx(
          -1,
          -1,
          NULL,
          STRING_TOKEN(STR_DMPSTORE_HEADER_LINE),
          gShellDebug1HiiHandle,
          RetString,
          &FoundVarGuid,
          FoundVarName,
          DataSize);
        if (Type == DmpStoreDisplay) {
          DumpHex(2, 0, DataSize, DataBuffer);
        } else {
          Status = AppendSingleVariableToFile (
                     FileHandle,
                     FoundVarName,
                     &FoundVarGuid,
                     Atts,
                     (UINT32) DataSize,
                     DataBuffer
                     );
        }
        SHELL_FREE_NON_NULL(RetString);
      }
    } else if (Type == DmpStoreDelete) {
      //
      // We only need name to delete it...
      //
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN(STR_DMPSTORE_DELETE_LINE),
        gShellDebug1HiiHandle,
        &FoundVarGuid,
        FoundVarName,
        gRT->SetVariable (FoundVarName, &FoundVarGuid, Atts, 0, NULL)
        );
    }
    SHELL_FREE_NON_NULL(DataBuffer);
  }

  SHELL_FREE_NON_NULL(FoundVarName);

  if (Status == EFI_DEVICE_ERROR) {
    ShellStatus = SHELL_DEVICE_ERROR;
  } else if (Status == EFI_SECURITY_VIOLATION) {
    ShellStatus = SHELL_SECURITY_VIOLATION;
  } else if (EFI_ERROR(Status)) {
    ShellStatus = SHELL_NOT_READY;
  }

  return (ShellStatus);
}

/**
  Function to display or delete variables.  This will set up and call into the recursive function.

  @param[in] Name        The variable name of the EFI variable (or NULL).
  @param[in] Guid        The GUID of the variable set (or NULL).
  @param[in] Type        The operation type.
  @param[in] FileHandle  The file to save or load variables.

  @retval SHELL_SUCCESS           The operation was successful.
  @retval SHELL_OUT_OF_RESOURCES  A memorty allocation failed.
  @retval SHELL_ABORTED           The abort message was received.
  @retval SHELL_DEVICE_ERROR      UEFI Variable Services returned an error.
  @retval SHELL_NOT_FOUND         the Name/Guid pair could not be found.
**/
SHELL_STATUS
EFIAPI
ProcessVariables (
  IN CONST CHAR16      *Name      OPTIONAL,
  IN CONST EFI_GUID    *Guid      OPTIONAL,
  IN DMP_STORE_TYPE    Type,
  IN SHELL_FILE_HANDLE FileHandle OPTIONAL
  )
{
  SHELL_STATUS              ShellStatus;
  BOOLEAN                   Found;
  EFI_GUID                  FoundVarGuid;

  Found         = FALSE;
  ShellStatus   = SHELL_SUCCESS;
  ZeroMem (&FoundVarGuid, sizeof(EFI_GUID));

  if (Type == DmpStoreLoad) {
    ShellStatus = LoadVariablesFromFile (FileHandle, Name, Guid, &Found);
  } else {
    ShellStatus = CascadeProcessVariables(Name, Guid, Type, FileHandle, NULL, FoundVarGuid, &Found);
  }

  if (!Found) {
    if (ShellStatus == SHELL_OUT_OF_RESOURCES) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellDebug1HiiHandle, L"dmpstore");  
      return (ShellStatus);
    } else if (Name != NULL && Guid == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_NO_VAR_FOUND_N), gShellDebug1HiiHandle, L"dmpstore", Name);  
    } else if (Name != NULL && Guid != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_NO_VAR_FOUND_GN), gShellDebug1HiiHandle, L"dmpstore", Guid, Name);  
    } else if (Name == NULL && Guid == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_NO_VAR_FOUND), gShellDebug1HiiHandle, L"dmpstore");  
    } else if (Name == NULL && Guid != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_NO_VAR_FOUND_G), gShellDebug1HiiHandle, L"dmpstore", Guid);  
    } 
    return (SHELL_NOT_FOUND);
  }
  return (ShellStatus);
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-d", TypeFlag},
  {L"-l", TypeValue},
  {L"-s", TypeValue},
  {L"-all", TypeFlag},
  {L"-guid", TypeValue},
  {NULL, TypeMax}
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
  EFI_STATUS        Status;
  LIST_ENTRY        *Package;
  CHAR16            *ProblemParam;
  SHELL_STATUS      ShellStatus;
  CONST CHAR16      *GuidStr;
  CONST CHAR16      *File;
  EFI_GUID          *Guid;
  EFI_GUID          GuidData;
  CONST CHAR16      *Name;
  DMP_STORE_TYPE    Type;
  SHELL_FILE_HANDLE FileHandle;
  EFI_FILE_INFO     *FileInfo;

  ShellStatus   = SHELL_SUCCESS;
  Package       = NULL;
  FileHandle    = NULL;
  File          = NULL;
  Type          = DmpStoreDisplay;

  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"dmpstore", ProblemParam);  
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) > 2) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"dmpstore");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetFlag(Package, L"-all") && ShellCommandLineGetFlag(Package, L"-guid")) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CONFLICT), gShellDebug1HiiHandle, L"dmpstore", L"-all", L"-guid");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetFlag(Package, L"-s") && ShellCommandLineGetFlag(Package, L"-l")) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CONFLICT), gShellDebug1HiiHandle,  L"dmpstore", L"-l", L"-s");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if ((ShellCommandLineGetFlag(Package, L"-s") || ShellCommandLineGetFlag(Package, L"-l")) && ShellCommandLineGetFlag(Package, L"-d")) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CONFLICT), gShellDebug1HiiHandle, L"dmpstore", L"-l or -s", L"-d");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      //
      // Determine the GUID to search for based on -all and -guid parameters
      //
      if (!ShellCommandLineGetFlag(Package, L"-all")) {
        GuidStr = ShellCommandLineGetValue(Package, L"-guid");
        if (GuidStr != NULL) {
          Status = ConvertStringToGuid(GuidStr, &GuidData);
          if (EFI_ERROR(Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"dmpstore", GuidStr);  
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          Guid = &GuidData;
        } else  {
          Guid = &gEfiGlobalVariableGuid;
        }
      } else {
        Guid  = NULL;
      }

      //
      // Get the Name of the variable to find
      //
      Name = ShellCommandLineGetRawValue(Package, 1);

      if (ShellStatus == SHELL_SUCCESS) {
        if (ShellCommandLineGetFlag(Package, L"-s")) {
          Type = DmpStoreSave;
          File = ShellCommandLineGetValue(Package, L"-s");
          if (File == NULL) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDebug1HiiHandle, L"dmpstore", L"-s");  
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
        } else if (ShellCommandLineGetFlag(Package, L"-l")) {
          Type = DmpStoreLoad;
          File = ShellCommandLineGetValue(Package, L"-l");
          if (File == NULL) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDebug1HiiHandle, L"dmpstore", L"-l");  
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            Status = ShellOpenFileByName (File, &FileHandle, EFI_FILE_MODE_READ, 0);
            if (EFI_ERROR (Status)) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellDebug1HiiHandle, L"dmpstore", File);  
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
        } else if (ShellCommandLineGetFlag(Package, L"-d")) {
          Type = DmpStoreDelete;
        }
      }

      if (ShellStatus == SHELL_SUCCESS) {
        if (Type == DmpStoreSave) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_SAVE), gShellDebug1HiiHandle, File);
        } else if (Type == DmpStoreLoad) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_LOAD), gShellDebug1HiiHandle, File);
        }
        ShellStatus = ProcessVariables (Name, Guid, Type, FileHandle);
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

