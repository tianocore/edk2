/** @file
  function declarations for shell environment functions.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Shell.h"

#define INIT_NAME_BUFFER_SIZE  128
#define INIT_DATA_BUFFER_SIZE  1024

//
// The list is used to cache the environment variables.
//
ENV_VAR_LIST                   gShellEnvVarList;

/**
  Reports whether an environment variable is Volatile or Non-Volatile.

  @param EnvVarName             The name of the environment variable in question
  @param Volatile               Return TRUE if the environment variable is volatile

  @retval EFI_SUCCESS           The volatile attribute is returned successfully
  @retval others                Some errors happened.
**/
EFI_STATUS
IsVolatileEnv (
  IN CONST CHAR16 *EnvVarName,
  OUT BOOLEAN     *Volatile
  )
{
  EFI_STATUS  Status;
  UINTN       Size;
  VOID        *Buffer;
  UINT32      Attribs;

  ASSERT (Volatile != NULL);

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
    if (Buffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
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
    *Volatile = TRUE;
    return EFI_SUCCESS;
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // check for the Non Volatile bit
  //
  *Volatile = !(BOOLEAN) ((Attribs & EFI_VARIABLE_NON_VOLATILE) == EFI_VARIABLE_NON_VOLATILE);
  return EFI_SUCCESS;
}

/**
  free function for ENV_VAR_LIST objects.

  @param[in] List               The pointer to pointer to list.
**/
VOID
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
      ; !IsListEmpty(List)
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
GetEnvironmentVariableList(
  IN OUT LIST_ENTRY *ListHead
  )
{
  CHAR16            *VariableName;
  UINTN             NameSize;
  UINTN             NameBufferSize;
  EFI_STATUS        Status;
  EFI_GUID          Guid;
  UINTN             ValSize;
  UINTN             ValBufferSize;
  ENV_VAR_LIST      *VarList;

  if (ListHead == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  Status = EFI_SUCCESS;

  ValBufferSize = INIT_DATA_BUFFER_SIZE;
  NameBufferSize = INIT_NAME_BUFFER_SIZE;
  VariableName = AllocateZeroPool(NameBufferSize);
  if (VariableName == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }
  *VariableName = CHAR_NULL;

  while (!EFI_ERROR(Status)) {
    NameSize = NameBufferSize;
    Status = gRT->GetNextVariableName(&NameSize, VariableName, &Guid);
    if (Status == EFI_NOT_FOUND){
      Status = EFI_SUCCESS;
      break;
    } else if (Status == EFI_BUFFER_TOO_SMALL) {
      NameBufferSize = NameSize > NameBufferSize * 2 ? NameSize : NameBufferSize * 2;
      SHELL_FREE_NON_NULL(VariableName);
      VariableName = AllocateZeroPool(NameBufferSize);
      if (VariableName == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }
      NameSize = NameBufferSize;
      Status = gRT->GetNextVariableName(&NameSize, VariableName, &Guid);
    }

    if (!EFI_ERROR(Status) && CompareGuid(&Guid, &gShellVariableGuid)){
      VarList = AllocateZeroPool(sizeof(ENV_VAR_LIST));
      if (VarList == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
      } else {
        ValSize = ValBufferSize;
        //
        // We need another CHAR16 to save '\0' in VarList->Val.
        //
        VarList->Val = AllocateZeroPool (ValSize + sizeof (CHAR16));
        if (VarList->Val == NULL) {
            SHELL_FREE_NON_NULL(VarList);
            Status = EFI_OUT_OF_RESOURCES;
            break;
        }
        Status = SHELL_GET_ENVIRONMENT_VARIABLE_AND_ATTRIBUTES(VariableName, &VarList->Atts, &ValSize, VarList->Val);
        if (Status == EFI_BUFFER_TOO_SMALL){
          ValBufferSize = ValSize > ValBufferSize * 2 ? ValSize : ValBufferSize * 2;
          SHELL_FREE_NON_NULL (VarList->Val);
          //
          // We need another CHAR16 to save '\0' in VarList->Val.
          //
          VarList->Val = AllocateZeroPool (ValBufferSize + sizeof (CHAR16));
          if (VarList->Val == NULL) {
            SHELL_FREE_NON_NULL(VarList);
            Status = EFI_OUT_OF_RESOURCES;
            break;
          }

          ValSize = ValBufferSize;
          Status = SHELL_GET_ENVIRONMENT_VARIABLE_AND_ATTRIBUTES(VariableName, &VarList->Atts, &ValSize, VarList->Val);
        }
        if (!EFI_ERROR(Status)) {
          VarList->Key = AllocateCopyPool(StrSize(VariableName), VariableName);
          if (VarList->Key == NULL) {
            SHELL_FREE_NON_NULL(VarList->Val);
            SHELL_FREE_NON_NULL(VarList);
            Status = EFI_OUT_OF_RESOURCES;
          } else {
            InsertTailList(ListHead, &VarList->Link);
          }
        } else {
          SHELL_FREE_NON_NULL(VarList->Val);
          SHELL_FREE_NON_NULL(VarList);
        }
      } // if (VarList == NULL) ... else ...
    } // compare guid
  } // while
  SHELL_FREE_NON_NULL (VariableName);

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
    Size = StrSize (Node->Val) - sizeof (CHAR16);
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
SetEnvironmentVariables(
  IN CONST CHAR16 **Environment
  )
{
  CONST CHAR16  *CurrentString;
  UINTN         CurrentCount;
  ENV_VAR_LIST  *VarList;
  ENV_VAR_LIST  *Node;

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
    if (Node == NULL) {
      SetEnvironmentVariableList(&VarList->Link);
      return (EFI_OUT_OF_RESOURCES);
    }

    Node->Key = AllocateZeroPool((StrStr(CurrentString, L"=") - CurrentString + 1) * sizeof(CHAR16));
    if (Node->Key == NULL) {
      SHELL_FREE_NON_NULL(Node);
      SetEnvironmentVariableList(&VarList->Link);
      return (EFI_OUT_OF_RESOURCES);
    }

    //
    // Copy the string into the Key, leaving the last character allocated as NULL to terminate
    //
    StrnCpyS( Node->Key,
              StrStr(CurrentString, L"=") - CurrentString + 1,
              CurrentString,
              StrStr(CurrentString, L"=") - CurrentString
              );

    //
    // ValueSize = TotalSize - already removed size - size for '=' + size for terminator (the last 2 items cancel each other)
    //
    Node->Val = AllocateCopyPool(StrSize(CurrentString) - StrSize(Node->Key), CurrentString + StrLen(Node->Key) + 1);
    if (Node->Val == NULL) {
      SHELL_FREE_NON_NULL(Node->Key);
      SHELL_FREE_NON_NULL(Node);
      SetEnvironmentVariableList(&VarList->Link);
      return (EFI_OUT_OF_RESOURCES);
    }

    Node->Atts = EFI_VARIABLE_BOOTSERVICE_ACCESS;

    if (VarList == NULL) {
      VarList = AllocateZeroPool(sizeof(ENV_VAR_LIST));
      if (VarList == NULL) {
        SHELL_FREE_NON_NULL(Node->Key);
        SHELL_FREE_NON_NULL(Node->Val);
        SHELL_FREE_NON_NULL(Node);
        return (EFI_OUT_OF_RESOURCES);
      }
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

/**
  Find an environment variable in the gShellEnvVarList.

  @param Key        The name of the environment variable.
  @param Value      The value of the environment variable, the buffer
                    shoule be freed by the caller.
  @param ValueSize  The size in bytes of the environment variable
                    including the tailing CHAR_NELL.
  @param Atts       The attributes of the variable.

  @retval EFI_SUCCESS       The command executed successfully.
  @retval EFI_NOT_FOUND     The environment variable is not found in
                            gShellEnvVarList.

**/
EFI_STATUS
ShellFindEnvVarInList (
  IN  CONST CHAR16    *Key,
  OUT CHAR16          **Value,
  OUT UINTN           *ValueSize,
  OUT UINT32          *Atts OPTIONAL
  )
{
  ENV_VAR_LIST      *Node;

  if (Key == NULL || Value == NULL || ValueSize == NULL) {
    return SHELL_INVALID_PARAMETER;
  }

  for ( Node = (ENV_VAR_LIST*)GetFirstNode(&gShellEnvVarList.Link)
      ; !IsNull(&gShellEnvVarList.Link, &Node->Link)
      ; Node = (ENV_VAR_LIST*)GetNextNode(&gShellEnvVarList.Link, &Node->Link)
     ){
    if (Node->Key != NULL && StrCmp(Key, Node->Key) == 0) {
      *Value      = AllocateCopyPool(StrSize(Node->Val), Node->Val);
      *ValueSize  = StrSize(Node->Val);
      if (Atts != NULL) {
        *Atts = Node->Atts;
      }
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Add an environment variable into gShellEnvVarList.

  @param Key        The name of the environment variable.
  @param Value      The value of environment variable.
  @param ValueSize  The size in bytes of the environment variable
                    including the tailing CHAR_NULL
  @param Atts       The attributes of the variable.

  @retval EFI_SUCCESS  The environment variable was added to list successfully.
  @retval others       Some errors happened.

**/
EFI_STATUS
ShellAddEnvVarToList (
  IN CONST CHAR16     *Key,
  IN CONST CHAR16     *Value,
  IN UINTN            ValueSize,
  IN UINT32           Atts
  )
{
  ENV_VAR_LIST      *Node;
  CHAR16            *LocalKey;
  CHAR16            *LocalValue;

  if (Key == NULL || Value == NULL || ValueSize == 0) {
    return EFI_INVALID_PARAMETER;
  }

  LocalValue = AllocateCopyPool (ValueSize, Value);
  if (LocalValue == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Update the variable value if it exists in gShellEnvVarList.
  //
  for ( Node = (ENV_VAR_LIST*)GetFirstNode(&gShellEnvVarList.Link)
      ; !IsNull(&gShellEnvVarList.Link, &Node->Link)
      ; Node = (ENV_VAR_LIST*)GetNextNode(&gShellEnvVarList.Link, &Node->Link)
     ){
    if (Node->Key != NULL && StrCmp(Key, Node->Key) == 0) {
      Node->Atts = Atts;
      SHELL_FREE_NON_NULL(Node->Val);
      Node->Val  = LocalValue;
      return EFI_SUCCESS;
    }
  }

  //
  // If the environment varialbe key doesn't exist in list just insert
  // a new node.
  //
  LocalKey = AllocateCopyPool (StrSize(Key), Key);
  if (LocalKey == NULL) {
    FreePool (LocalValue);
    return EFI_OUT_OF_RESOURCES;
  }
  Node = (ENV_VAR_LIST*)AllocateZeroPool (sizeof(ENV_VAR_LIST));
  if (Node == NULL) {
    FreePool (LocalKey);
    FreePool (LocalValue);
    return EFI_OUT_OF_RESOURCES;
  }
  Node->Key = LocalKey;
  Node->Val = LocalValue;
  Node->Atts = Atts;
  InsertTailList(&gShellEnvVarList.Link, &Node->Link);

  return EFI_SUCCESS;
}

/**
  Remove a specified environment variable in gShellEnvVarList.

  @param Key        The name of the environment variable.

  @retval EFI_SUCCESS       The command executed successfully.
  @retval EFI_NOT_FOUND     The environment variable is not found in
                            gShellEnvVarList.
**/
EFI_STATUS
ShellRemvoeEnvVarFromList (
  IN CONST CHAR16           *Key
  )
{
  ENV_VAR_LIST      *Node;

  if (Key == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for ( Node = (ENV_VAR_LIST*)GetFirstNode(&gShellEnvVarList.Link)
      ; !IsNull(&gShellEnvVarList.Link, &Node->Link)
      ; Node = (ENV_VAR_LIST*)GetNextNode(&gShellEnvVarList.Link, &Node->Link)
     ){
    if (Node->Key != NULL && StrCmp(Key, Node->Key) == 0) {
      SHELL_FREE_NON_NULL(Node->Key);
      SHELL_FREE_NON_NULL(Node->Val);
      RemoveEntryList(&Node->Link);
      SHELL_FREE_NON_NULL(Node);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Initialize the gShellEnvVarList and cache all Shell-Guid-based environment
  variables.

**/
EFI_STATUS
ShellInitEnvVarList (
  VOID
  )
{
  EFI_STATUS    Status;

  InitializeListHead(&gShellEnvVarList.Link);
  Status = GetEnvironmentVariableList (&gShellEnvVarList.Link);

  return Status;
}

/**
  Destructe the gShellEnvVarList.

**/
VOID
ShellFreeEnvVarList (
  VOID
  )
{
  FreeEnvironmentVariableList (&gShellEnvVarList.Link);
  InitializeListHead(&gShellEnvVarList.Link);

  return;
}

