/** @file
  function declarations for shell environment functions.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <ShellBase.h>

#include <Guid/ShellVariableGuid.h>

#include <Library/BaseLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

#include "ShellEnvVar.h"

/**
  Reports whether an environment variable is Volatile or Non-Volatile.

  @param EnvVarName             The name of the environment variable in question

  @retval TRUE                  This environment variable is Volatile
  @retval FALSE                 This environment variable is NON-Volatile
**/
BOOLEAN
EFIAPI
IsVolatileEnv (
  IN CONST CHAR16 *EnvVarName
  )
{
  EFI_STATUS  Status;
  UINTN       Size;
  VOID        *Buffer;
  UINT32      Attribs;

  Size = 0;
  Buffer = NULL;

  //
  // get the variable
  //
  Status = gRT->GetVariable((CHAR16*)EnvVarName,
                            &gShellVariableGuid,
                            &Attribs,
                            &Size,
                            Buffer);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Buffer = AllocateZeroPool(Size);
    ASSERT(Buffer != NULL);
    Status = gRT->GetVariable((CHAR16*)EnvVarName,
                              &gShellVariableGuid,
                              &Attribs,
                              &Size,
                              Buffer);
    FreePool(Buffer);
  }
  //
  // not found means volatile
  //
  if (Status == EFI_NOT_FOUND) {
    return (TRUE);
  }
  ASSERT_EFI_ERROR(Status);

  //
  // check for the Non Volatile bit
  //
  if ((Attribs & EFI_VARIABLE_NON_VOLATILE) == EFI_VARIABLE_NON_VOLATILE) {
    return (FALSE);
  }

  //
  // everything else is volatile
  //
  return (TRUE);
}

/**
  free function for ENV_VAR_LIST objects.

  @param[in] List               The pointer to pointer to list.
**/
VOID
EFIAPI
FreeEnvironmentVariableList(
  IN LIST_ENTRY *List
  )
{
  ENV_VAR_LIST *Node;

  ASSERT (List != NULL);
  if (List == NULL) {
    return;
  }

  for ( Node = (ENV_VAR_LIST*)GetFirstNode(List)
      ; IsListEmpty(List)
      ; Node = (ENV_VAR_LIST*)GetFirstNode(List)
     ){
    ASSERT(Node != NULL);
    RemoveEntryList(&Node->Link);
    if (Node->Key != NULL) {
      FreePool(Node->Key);
    }
    if (Node->Val != NULL) {
      FreePool(Node->Val);
    }
    FreePool(Node);
  }
}

/**
  Creates a list of all Shell-Guid-based environment variables.

  @param[in, out] ListHead       The pointer to pointer to LIST ENTRY object for
                                 storing this list.

  @retval EFI_SUCCESS           the list was created sucessfully.
**/
EFI_STATUS
EFIAPI
GetEnvironmentVariableList(
  IN OUT LIST_ENTRY *ListHead
  )
{
  CHAR16            *VariableName;
  UINTN             NameSize;
  UINT64            MaxStorSize;
  UINT64            RemStorSize;
  UINT64            MaxVarSize;
  EFI_STATUS        Status;
  EFI_GUID          Guid;
  UINTN             ValSize;
  ENV_VAR_LIST      *VarList;

  if (ListHead == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  if (gRT->Hdr.Revision >= EFI_2_00_SYSTEM_TABLE_REVISION) {
    Status = gRT->QueryVariableInfo(EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS, &MaxStorSize, &RemStorSize, &MaxVarSize);
    if (EFI_ERROR(Status)) {
      return (Status);
    }
  } else {
    Status = EFI_SUCCESS;
    MaxVarSize = 16384;
  }

  NameSize = (UINTN)MaxVarSize;
  VariableName = AllocateZeroPool(NameSize);
  if (VariableName == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }
  StrCpy(VariableName, L"");

  while (!EFI_ERROR(Status)) {
    NameSize = (UINTN)MaxVarSize;
    Status = gRT->GetNextVariableName(&NameSize, VariableName, &Guid);
    if (Status == EFI_NOT_FOUND){
      Status = EFI_SUCCESS;
      break;
    }
    if (!EFI_ERROR(Status) && CompareGuid(&Guid, &gShellVariableGuid)){
      VarList = AllocateZeroPool(sizeof(ENV_VAR_LIST));
      if (VarList == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
      } else {
        ValSize = 0;
        Status = SHELL_GET_ENVIRONMENT_VARIABLE_AND_ATTRIBUTES(VariableName, &VarList->Atts, &ValSize, VarList->Val);
        if (Status == EFI_BUFFER_TOO_SMALL){
          VarList->Val = AllocateZeroPool(ValSize);
          if (VarList->Val == NULL) {
            SHELL_FREE_NON_NULL(VarList);
            Status = EFI_OUT_OF_RESOURCES;
          } else {
            Status = SHELL_GET_ENVIRONMENT_VARIABLE_AND_ATTRIBUTES(VariableName, &VarList->Atts, &ValSize, VarList->Val);
          }
        }
        if (!EFI_ERROR(Status) && VarList != NULL) {
          VarList->Key = AllocateZeroPool(StrSize(VariableName));
          if (VarList->Key == NULL) {
            SHELL_FREE_NON_NULL(VarList->Val);
            SHELL_FREE_NON_NULL(VarList);
            Status = EFI_OUT_OF_RESOURCES;
          } else {
            StrCpy(VarList->Key, VariableName);
            InsertTailList(ListHead, &VarList->Link);
          }
        }
      }
    } // compare guid
  } // while
  FreePool(VariableName);

  if (EFI_ERROR(Status)) {
    FreeEnvironmentVariableList(ListHead);
  }

  return (Status);
}

/**
  Sets a list of all Shell-Guid-based environment variables.  this will
  also eliminate all existing shell environment variables (even if they
  are not on the list).

  This function will also deallocate the memory from List.

  @param[in] ListHead           The pointer to LIST_ENTRY from
                                GetShellEnvVarList().

  @retval EFI_SUCCESS           the list was Set sucessfully.
**/
EFI_STATUS
EFIAPI
SetEnvironmentVariableList(
  IN LIST_ENTRY *ListHead
  )
{
  ENV_VAR_LIST      VarList;
  ENV_VAR_LIST      *Node;
  EFI_STATUS        Status;
  UINTN             Size;

  InitializeListHead(&VarList.Link);

  //
  // Delete all the current environment variables
  //
  Status = GetEnvironmentVariableList(&VarList.Link);
  ASSERT_EFI_ERROR(Status);

  for ( Node = (ENV_VAR_LIST*)GetFirstNode(&VarList.Link)
      ; !IsNull(&VarList.Link, &Node->Link)
      ; Node = (ENV_VAR_LIST*)GetNextNode(&VarList.Link, &Node->Link)
     ){
    if (Node->Key != NULL) {
      Status = SHELL_DELETE_ENVIRONMENT_VARIABLE(Node->Key);
    }
    ASSERT_EFI_ERROR(Status);
  }

  FreeEnvironmentVariableList(&VarList.Link);

  //
  // set all the variables fron the list
  //
  for ( Node = (ENV_VAR_LIST*)GetFirstNode(ListHead)
      ; !IsNull(ListHead, &Node->Link)
      ; Node = (ENV_VAR_LIST*)GetNextNode(ListHead, &Node->Link)
     ){
    Size = StrSize(Node->Val);
    if (Node->Atts & EFI_VARIABLE_NON_VOLATILE) {
      Status = SHELL_SET_ENVIRONMENT_VARIABLE_NV(Node->Key, Size, Node->Val);
    } else {
      Status = SHELL_SET_ENVIRONMENT_VARIABLE_V (Node->Key, Size, Node->Val);
    }
    ASSERT_EFI_ERROR(Status);
  }
  FreeEnvironmentVariableList(ListHead);

  return (Status);
}

/**
  sets a list of all Shell-Guid-based environment variables.

  @param Environment        Points to a NULL-terminated array of environment
                            variables with the format 'x=y', where x is the
                            environment variable name and y is the value.

  @retval EFI_SUCCESS       The command executed successfully.
  @retval EFI_INVALID_PARAMETER The parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES Out of resources.

  @sa SetEnvironmentVariableList
**/
EFI_STATUS
EFIAPI
SetEnvironmentVariables(
  IN CONST CHAR16 **Environment
  )
{
  CONST CHAR16  *CurrentString;
  UINTN         CurrentCount;
  ENV_VAR_LIST  *VarList;
  ENV_VAR_LIST  *Node;
  UINTN         NewSize;

  VarList = NULL;

  if (Environment == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  //
  // Build a list identical to the ones used for get/set list functions above
  //
  for ( CurrentCount = 0
      ;
      ; CurrentCount++
     ){
    CurrentString = Environment[CurrentCount];
    if (CurrentString == NULL) {
      break;
    }
    ASSERT(StrStr(CurrentString, L"=") != NULL);
    Node = AllocateZeroPool(sizeof(ENV_VAR_LIST));
    ASSERT(Node != NULL);
    Node->Key = AllocateZeroPool((StrStr(CurrentString, L"=") - CurrentString + 1) * sizeof(CHAR16));
    ASSERT(Node->Key != NULL);
    StrnCpy(Node->Key, CurrentString, StrStr(CurrentString, L"=") - CurrentString);
    NewSize = StrSize(CurrentString);
    NewSize -= StrLen(Node->Key) - 1;
    Node->Val = AllocateZeroPool(NewSize);
    ASSERT(Node->Val != NULL);
    StrCpy(Node->Val, CurrentString + StrLen(Node->Key) + 1);
    Node->Atts = EFI_VARIABLE_BOOTSERVICE_ACCESS;

    if (VarList == NULL) {
      VarList = AllocateZeroPool(sizeof(ENV_VAR_LIST));
      ASSERT(VarList != NULL);
      InitializeListHead(&VarList->Link);
    }
    InsertTailList(&VarList->Link, &Node->Link);

  } // for loop

  //
  // set this new list as the set of all environment variables.
  // this function also frees the memory and deletes all pre-existing
  // shell-guid based environment variables.
  //
  return (SetEnvironmentVariableList(&VarList->Link));
}
