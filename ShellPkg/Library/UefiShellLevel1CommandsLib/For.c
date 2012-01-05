/** @file
  Main file for endfor and for shell level 1 functions.

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellLevel1CommandsLib.h"
#include <Library/PrintLib.h>

/**
  Determine if a valid string is a valid number for the 'for' command.

  @param[in] Number The pointer to the string representation of the number to test.

  @retval TRUE    The number is valid.
  @retval FALSE   The number is not valid.
**/
BOOLEAN
EFIAPI
ShellIsValidForNumber (
  IN CONST CHAR16 *Number
  )
{
  if (Number == NULL || *Number == CHAR_NULL) {
    return (FALSE);
  }

  if (*Number == L'-') {
    Number++;
  }

  if (StrLen(Number) == 0) {
    return (FALSE);
  }

  if (StrLen(Number) >= 7) {
    if ((StrStr(Number, L" ") == NULL) || (((StrStr(Number, L" ") != NULL) && (StrStr(Number, L" ") - Number) >= 7))) {
      return (FALSE);
    }
  }

  if (!ShellIsDecimalDigitCharacter(*Number)) {
    return (FALSE);
  }

  return (TRUE);
}

/**
  Function for 'endfor' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunEndFor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  BOOLEAN             Found;
  SCRIPT_FILE         *CurrentScriptFile;

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  if (!gEfiShellProtocol->BatchIsActive()) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_NO_SCRIPT), gShellLevel1HiiHandle, L"EndFor");
    return (SHELL_UNSUPPORTED);
  }

  if (gEfiShellParametersProtocol->Argc > 1) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel1HiiHandle);
    return (SHELL_INVALID_PARAMETER);
  }

  Found = MoveToTag(GetPreviousNode, L"for", L"endfor", NULL, ShellCommandGetCurrentScriptFile(), FALSE, FALSE, FALSE);

  if (!Found) {
    CurrentScriptFile = ShellCommandGetCurrentScriptFile();
    ShellPrintHiiEx(
      -1, 
      -1, 
      NULL, 
      STRING_TOKEN (STR_SYNTAX_NO_MATCHING), 
      gShellLevel1HiiHandle, 
      L"For", 
      L"EndFor", 
      CurrentScriptFile!=NULL
        && CurrentScriptFile->CurrentCommand!=NULL
          ? CurrentScriptFile->CurrentCommand->Line:0);
    return (SHELL_NOT_FOUND);
  }
  return (SHELL_SUCCESS);
}

typedef struct {
  UINT32          Signature;
  INTN            Current;
  INTN            End;
  INTN            Step;
  CHAR16          *ReplacementName;
  CHAR16          *CurrentValue;
  BOOLEAN         RemoveSubstAlias;
  CHAR16          Set[1];
  } SHELL_FOR_INFO;
#define SIZE_OF_SHELL_FOR_INFO OFFSET_OF (SHELL_FOR_INFO, Set)
#define SHELL_FOR_INFO_SIGNATURE SIGNATURE_32 ('S', 'F', 'I', 's')

/**
  Update the value of a given alias on the list.  If the alias is not there then add it.

  @param[in] Alias               The alias to test for.
  @param[in] CommandString       The updated command string.
  @param[in, out] List           The list to search.

  @retval EFI_SUCCESS           The operation was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  There was not enough free memory.
**/
EFI_STATUS
EFIAPI
InternalUpdateAliasOnList(
  IN CONST CHAR16       *Alias,
  IN CONST CHAR16       *CommandString,
  IN OUT LIST_ENTRY     *List
  )
{
  ALIAS_LIST *Node;
  BOOLEAN    Found;

  //
  // assert for NULL parameter
  //
  ASSERT(Alias != NULL);

  //
  // check for the Alias
  //
  for ( Node = (ALIAS_LIST *)GetFirstNode(List), Found = FALSE
      ; !IsNull(List, &Node->Link)
      ; Node = (ALIAS_LIST *)GetNextNode(List, &Node->Link)
     ){
    ASSERT(Node->CommandString != NULL);
    ASSERT(Node->Alias != NULL);
    if (StrCmp(Node->Alias, Alias)==0) {
      FreePool(Node->CommandString);
      Node->CommandString = NULL;
      Node->CommandString = StrnCatGrow(&Node->CommandString, NULL, CommandString, 0);
      Found = TRUE;
      break;
    }
  }
  if (!Found) {
    Node = AllocateZeroPool(sizeof(ALIAS_LIST));
    if (Node == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }
    ASSERT(Node->Alias == NULL);
    Node->Alias         = StrnCatGrow(&Node->Alias, NULL, Alias, 0);
    ASSERT(Node->CommandString == NULL);
    Node->CommandString = StrnCatGrow(&Node->CommandString, NULL, CommandString, 0);
    InsertTailList(List, &Node->Link);
  }
  return (EFI_SUCCESS);
}

/**
  Find out if an alias is on the given list.

  @param[in] Alias              The alias to test for.
  @param[in] List               The list to search.

  @retval TRUE                  The alias is on the list.
  @retval FALSE                 The alias is not on the list.
**/
BOOLEAN
EFIAPI
InternalIsAliasOnList(
  IN CONST CHAR16       *Alias,
  IN CONST LIST_ENTRY   *List
  )
{
  ALIAS_LIST *Node;

  //
  // assert for NULL parameter
  //
  ASSERT(Alias != NULL);

  //
  // check for the Alias
  //
  for ( Node = (ALIAS_LIST *)GetFirstNode(List)
      ; !IsNull(List, &Node->Link)
      ; Node = (ALIAS_LIST *)GetNextNode(List, &Node->Link)
     ){
    ASSERT(Node->CommandString != NULL);
    ASSERT(Node->Alias != NULL);
    if (StrCmp(Node->Alias, Alias)==0) {
      return (TRUE);
    }
  }
  return (FALSE);
}

/**
  Remove an alias from the given list.

  @param[in] Alias               The alias to remove.
  @param[in, out] List           The list to search.
**/
BOOLEAN
EFIAPI
InternalRemoveAliasFromList(
  IN CONST CHAR16       *Alias,
  IN OUT LIST_ENTRY     *List
  )
{
  ALIAS_LIST *Node;

  //
  // assert for NULL parameter
  //
  ASSERT(Alias != NULL);

  //
  // check for the Alias
  //
  for ( Node = (ALIAS_LIST *)GetFirstNode(List)
      ; !IsNull(List, &Node->Link)
      ; Node = (ALIAS_LIST *)GetNextNode(List, &Node->Link)
     ){
    ASSERT(Node->CommandString != NULL);
    ASSERT(Node->Alias != NULL);
    if (StrCmp(Node->Alias, Alias)==0) {
      RemoveEntryList(&Node->Link);
      FreePool(Node->Alias);
      FreePool(Node->CommandString);
      FreePool(Node);
      return (TRUE);
    }
  }
  return (FALSE);
}

/**
  Function for 'for' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunFor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  SHELL_STATUS        ShellStatus;
  SCRIPT_FILE         *CurrentScriptFile;
  CHAR16              *ArgSet;
  CHAR16              *ArgSetWalker;
  UINTN               ArgSize;
  UINTN               LoopVar;
  SHELL_FOR_INFO      *Info;
  CHAR16              *TempString;
  CHAR16              *TempSpot;
  BOOLEAN             FirstPass;
  EFI_SHELL_FILE_INFO *Node;
  EFI_SHELL_FILE_INFO *FileList;
  UINTN               NewSize;

  ArgSet              = NULL;
  ArgSize             = 0;
  ShellStatus         = SHELL_SUCCESS;
  ArgSetWalker        = NULL;
  TempString          = NULL;
  FirstPass           = FALSE;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  if (!gEfiShellProtocol->BatchIsActive()) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_NO_SCRIPT), gShellLevel1HiiHandle, L"For");
    return (SHELL_UNSUPPORTED);
  }

  if (gEfiShellParametersProtocol->Argc < 4) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel1HiiHandle);
    return (SHELL_INVALID_PARAMETER);
  }

  CurrentScriptFile = ShellCommandGetCurrentScriptFile();
  ASSERT(CurrentScriptFile != NULL);

  if (CurrentScriptFile->CurrentCommand->Data == NULL) {
    FirstPass = TRUE;

    //
    // Make sure that an End exists.
    //
    if (!MoveToTag(GetNextNode, L"endfor", L"for", NULL, CurrentScriptFile, TRUE, TRUE, FALSE)) {
      ShellPrintHiiEx(
        -1, 
        -1, 
        NULL, 
        STRING_TOKEN (STR_SYNTAX_NO_MATCHING), 
        gShellLevel1HiiHandle, 
        L"EndFor", 
        L"For", 
        CurrentScriptFile->CurrentCommand!=NULL
          ?CurrentScriptFile->CurrentCommand->Line:0);
      return (SHELL_DEVICE_ERROR);
    }

    //
    // Process the line.
    //
    if (gEfiShellParametersProtocol->Argv[1][0] != L'%' || gEfiShellParametersProtocol->Argv[1][2] != CHAR_NULL
      ||!((gEfiShellParametersProtocol->Argv[1][1] >= L'a' && gEfiShellParametersProtocol->Argv[1][1] <= L'z')
       ||(gEfiShellParametersProtocol->Argv[1][1] >= L'A' && gEfiShellParametersProtocol->Argv[1][1] <= L'Z'))
     ) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_VAR), gShellLevel1HiiHandle, gEfiShellParametersProtocol->Argv[1]);
      return (SHELL_INVALID_PARAMETER);
    }

    if (gUnicodeCollation->StriColl(
        gUnicodeCollation,
        L"in",
        gEfiShellParametersProtocol->Argv[2]) == 0) {
      for (LoopVar = 0x3 ; LoopVar < gEfiShellParametersProtocol->Argc ; LoopVar++) {
        ASSERT((ArgSet == NULL && ArgSize == 0) || (ArgSet != NULL));
        if (ArgSet == NULL) {
  //        ArgSet = StrnCatGrow(&ArgSet, &ArgSize, L"\"", 0);
        } else {
          ArgSet = StrnCatGrow(&ArgSet, &ArgSize, L" \"", 0);
        }
        if (StrStr(gEfiShellParametersProtocol->Argv[LoopVar], L"*") != NULL
          ||StrStr(gEfiShellParametersProtocol->Argv[LoopVar], L"?") != NULL
          ||StrStr(gEfiShellParametersProtocol->Argv[LoopVar], L"[") != NULL
          ||StrStr(gEfiShellParametersProtocol->Argv[LoopVar], L"]") != NULL) {
          FileList = NULL;
          Status = ShellOpenFileMetaArg ((CHAR16*)gEfiShellParametersProtocol->Argv[LoopVar], EFI_FILE_MODE_READ, &FileList);
          if (EFI_ERROR(Status) || FileList == NULL || IsListEmpty(&FileList->Link)) {
            ArgSet = StrnCatGrow(&ArgSet, &ArgSize, gEfiShellParametersProtocol->Argv[LoopVar], 0);
          } else {
            for (Node = (EFI_SHELL_FILE_INFO *)GetFirstNode(&FileList->Link)
              ;  !IsNull(&FileList->Link, &Node->Link)
              ;  Node = (EFI_SHELL_FILE_INFO *)GetNextNode(&FileList->Link, &Node->Link)
             ){
              ArgSet = StrnCatGrow(&ArgSet, &ArgSize, L" \"", 0);
              ArgSet = StrnCatGrow(&ArgSet, &ArgSize, Node->FullName, 0);
              ArgSet = StrnCatGrow(&ArgSet, &ArgSize, L"\"", 0);
            }
            ShellCloseFileMetaArg(&FileList);
          }
        } else {
          ArgSet = StrnCatGrow(&ArgSet, &ArgSize, gEfiShellParametersProtocol->Argv[LoopVar], 0);
        }
        ArgSet = StrnCatGrow(&ArgSet, &ArgSize, L"\"", 0);
      }
      if (ArgSet == NULL) {
        ShellStatus = SHELL_OUT_OF_RESOURCES;
      } else {
        //
        // set up for an 'in' for loop
        //
        NewSize = StrSize(ArgSet);
        NewSize += sizeof(SHELL_FOR_INFO)+StrSize(gEfiShellParametersProtocol->Argv[1]);
        Info = AllocateZeroPool(NewSize);
        ASSERT(Info != NULL);
        Info->Signature = SHELL_FOR_INFO_SIGNATURE;
        CopyMem(Info->Set, ArgSet, StrSize(ArgSet));
        NewSize = StrSize(gEfiShellParametersProtocol->Argv[1]);
        CopyMem(Info->Set+(StrSize(ArgSet)/sizeof(Info->Set[0])), gEfiShellParametersProtocol->Argv[1], NewSize);
        Info->ReplacementName = Info->Set+StrSize(ArgSet)/sizeof(Info->Set[0]);
        Info->CurrentValue  = (CHAR16*)Info->Set;
        Info->Step          = 0;
        Info->Current       = 0;
        Info->End           = 0;

        if (InternalIsAliasOnList(Info->ReplacementName, &CurrentScriptFile->SubstList)) {
          Info->RemoveSubstAlias  = FALSE;
        } else {
          Info->RemoveSubstAlias  = TRUE;
        }
        CurrentScriptFile->CurrentCommand->Data = Info;
      }
    } else if (gUnicodeCollation->StriColl(
        gUnicodeCollation,
        L"run",
        gEfiShellParametersProtocol->Argv[2]) == 0) {
      for (LoopVar = 0x3 ; LoopVar < gEfiShellParametersProtocol->Argc ; LoopVar++) {
        ASSERT((ArgSet == NULL && ArgSize == 0) || (ArgSet != NULL));
        if (ArgSet == NULL) {
//        ArgSet = StrnCatGrow(&ArgSet, &ArgSize, L"\"", 0);
        } else {
          ArgSet = StrnCatGrow(&ArgSet, &ArgSize, L" ", 0);
        }
        ArgSet = StrnCatGrow(&ArgSet, &ArgSize, gEfiShellParametersProtocol->Argv[LoopVar], 0);
//        ArgSet = StrnCatGrow(&ArgSet, &ArgSize, L" ", 0);
      }
      if (ArgSet == NULL) {
        ShellStatus = SHELL_OUT_OF_RESOURCES;
      } else {
        //
        // set up for a 'run' for loop
        //
        Info = AllocateZeroPool(sizeof(SHELL_FOR_INFO)+StrSize(gEfiShellParametersProtocol->Argv[1]));
        ASSERT(Info != NULL);
        Info->Signature = SHELL_FOR_INFO_SIGNATURE;
        CopyMem(Info->Set, gEfiShellParametersProtocol->Argv[1], StrSize(gEfiShellParametersProtocol->Argv[1]));
        Info->ReplacementName = Info->Set;
        Info->CurrentValue    = NULL;
        ArgSetWalker            = ArgSet;
        if (ArgSetWalker[0] != L'(') {
          ShellPrintHiiEx(
            -1, 
            -1, 
            NULL, 
            STRING_TOKEN (STR_GEN_PROBLEM_SCRIPT), 
            gShellLevel1HiiHandle, 
            ArgSet, 
            CurrentScriptFile!=NULL 
              && CurrentScriptFile->CurrentCommand!=NULL
              ? CurrentScriptFile->CurrentCommand->Line:0);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          TempSpot = StrStr(ArgSetWalker, L")");
          if (TempSpot != NULL) {
            TempString = TempSpot+1;
            if (*(TempString) != CHAR_NULL) {
              while(TempString != NULL && *TempString == L' ') {
                TempString++;
              }
              if (StrLen(TempString) > 0) {
                TempSpot = NULL;
              }
            }
          }
          if (TempSpot == NULL) {
            ShellPrintHiiEx(
              -1, 
              -1, 
              NULL, 
              STRING_TOKEN (STR_GEN_PROBLEM_SCRIPT), 
              gShellLevel1HiiHandle, 
              CurrentScriptFile!=NULL 
                && CurrentScriptFile->CurrentCommand!=NULL
                ? CurrentScriptFile->CurrentCommand->Line:0);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            *TempSpot = CHAR_NULL;
            ArgSetWalker++;
            while (ArgSetWalker != NULL && ArgSetWalker[0] == L' ') {
              ArgSetWalker++;
            }
            if (!ShellIsValidForNumber(ArgSetWalker)) {
              ShellPrintHiiEx(
                -1, 
                -1, 
                NULL, 
                STRING_TOKEN (STR_GEN_PROBLEM_SCRIPT), 
                gShellLevel1HiiHandle, 
                ArgSet, 
                CurrentScriptFile!=NULL 
                  && CurrentScriptFile->CurrentCommand!=NULL
                  ? CurrentScriptFile->CurrentCommand->Line:0);
              ShellStatus = SHELL_INVALID_PARAMETER;
            } else {
              if (ArgSetWalker[0] == L'-') {
                Info->Current = 0 - (INTN)ShellStrToUintn(ArgSetWalker+1);
              } else {
                Info->Current = (INTN)ShellStrToUintn(ArgSetWalker);
              }
              ArgSetWalker  = StrStr(ArgSetWalker, L" ");
              while (ArgSetWalker != NULL && ArgSetWalker[0] == L' ') {
                ArgSetWalker++;
              }
              if (ArgSetWalker == NULL || *ArgSetWalker == CHAR_NULL || !ShellIsValidForNumber(ArgSetWalker)){
                ShellPrintHiiEx(
                  -1, 
                  -1, 
                  NULL, 
                  STRING_TOKEN (STR_GEN_PROBLEM_SCRIPT), 
                  gShellLevel1HiiHandle, 
                  ArgSet, 
                  CurrentScriptFile!=NULL 
                    && CurrentScriptFile->CurrentCommand!=NULL
                    ? CurrentScriptFile->CurrentCommand->Line:0);
                ShellStatus = SHELL_INVALID_PARAMETER;
              } else {
                if (ArgSetWalker[0] == L'-') {
                  Info->End = 0 - (INTN)ShellStrToUintn(ArgSetWalker+1);
                } else {
                  Info->End = (INTN)ShellStrToUintn(ArgSetWalker);
                }
                if (Info->Current < Info->End) {
                  Info->Step            = 1;
                } else {
                  Info->Step            = -1;
                }

                ArgSetWalker  = StrStr(ArgSetWalker, L" ");
                while (ArgSetWalker != NULL && ArgSetWalker[0] == L' ') {
                  ArgSetWalker++;
                }
                if (ArgSetWalker != NULL && *ArgSetWalker != CHAR_NULL) {
                  if (ArgSetWalker == NULL || *ArgSetWalker == CHAR_NULL || !ShellIsValidForNumber(ArgSetWalker)){
                    ShellPrintHiiEx(
                      -1, 
                      -1, 
                      NULL, 
                      STRING_TOKEN (STR_GEN_PROBLEM_SCRIPT), 
                      gShellLevel1HiiHandle, 
                      ArgSet, 
                      CurrentScriptFile!=NULL 
                        && CurrentScriptFile->CurrentCommand!=NULL
                        ? CurrentScriptFile->CurrentCommand->Line:0);
                    ShellStatus = SHELL_INVALID_PARAMETER;
                  } else {
                    if (*ArgSetWalker == L')') {
                      ASSERT(Info->Step == 1 || Info->Step == -1);
                    } else {
                      if (ArgSetWalker[0] == L'-') {
                        Info->Step = 0 - (INTN)ShellStrToUintn(ArgSetWalker+1);
                      } else {
                        Info->Step = (INTN)ShellStrToUintn(ArgSetWalker);
                      }

                      if (StrStr(ArgSetWalker, L" ") != NULL) {
                        ShellPrintHiiEx(
                          -1, 
                          -1, 
                          NULL, 
                          STRING_TOKEN (STR_GEN_PROBLEM_SCRIPT), 
                          gShellLevel1HiiHandle, 
                          ArgSet, 
                          CurrentScriptFile!=NULL 
                            && CurrentScriptFile->CurrentCommand!=NULL
                            ? CurrentScriptFile->CurrentCommand->Line:0);
                        ShellStatus = SHELL_INVALID_PARAMETER;
                      }
                    }
                  }
                  
                }
              }
            }
          }
        }
        if (ShellStatus == SHELL_SUCCESS) {
          if (InternalIsAliasOnList(Info->ReplacementName, &CurrentScriptFile->SubstList)) {
            Info->RemoveSubstAlias  = FALSE;
          } else {
            Info->RemoveSubstAlias  = TRUE;
          }
        }
        if (CurrentScriptFile->CurrentCommand != NULL) {
          CurrentScriptFile->CurrentCommand->Data = Info;
        }
      }
    } else {
      ShellPrintHiiEx(
        -1, 
        -1, 
        NULL, 
        STRING_TOKEN (STR_GEN_PROBLEM_SCRIPT), 
        gShellLevel1HiiHandle, 
        ArgSet, 
        CurrentScriptFile!=NULL 
          && CurrentScriptFile->CurrentCommand!=NULL
          ? CurrentScriptFile->CurrentCommand->Line:0);
      ShellStatus = SHELL_INVALID_PARAMETER;
    }
  } else {
    //
    // These need to be NULL since they are used to determine if this is the first pass later on...
    //
    ASSERT(ArgSetWalker == NULL);
    ASSERT(ArgSet       == NULL);
  }

  if (CurrentScriptFile != NULL && CurrentScriptFile->CurrentCommand != NULL) {
    Info = (SHELL_FOR_INFO*)CurrentScriptFile->CurrentCommand->Data;
    if (CurrentScriptFile->CurrentCommand->Reset) {
      Info->CurrentValue  = (CHAR16*)Info->Set;
      FirstPass = TRUE;
      CurrentScriptFile->CurrentCommand->Reset = FALSE;
    }
  } else {
    ShellStatus = SHELL_UNSUPPORTED;
    Info = NULL;
  }
  if (ShellStatus == SHELL_SUCCESS) {
    ASSERT(Info != NULL);
    if (Info->Step != 0) {
      //
      // only advance if not the first pass
      //
      if (!FirstPass) {
        //
        // sequence version of for loop...
        //
        Info->Current += Info->Step;
      }

      TempString = AllocateZeroPool(50*sizeof(CHAR16));
      UnicodeSPrint(TempString, 50*sizeof(CHAR16), L"%d", Info->Current);
      InternalUpdateAliasOnList(Info->ReplacementName, TempString, &CurrentScriptFile->SubstList);
      FreePool(TempString);

      if ((Info->Step > 0 && Info->Current > Info->End) || (Info->Step < 0 && Info->Current < Info->End)) {
        CurrentScriptFile->CurrentCommand->Data = NULL;
        //
        // find the matching endfor (we're done with the loop)
        //
        if (!MoveToTag(GetNextNode, L"endfor", L"for", NULL, CurrentScriptFile, TRUE, FALSE, FALSE)) {
          ShellPrintHiiEx(
            -1, 
            -1, 
            NULL, 
            STRING_TOKEN (STR_SYNTAX_NO_MATCHING), 
            gShellLevel1HiiHandle, 
            L"EndFor", 
            L"For", 
            CurrentScriptFile!=NULL 
              && CurrentScriptFile->CurrentCommand!=NULL
              ? CurrentScriptFile->CurrentCommand->Line:0);
          ShellStatus = SHELL_DEVICE_ERROR;
        }
        if (Info->RemoveSubstAlias) {
          //
          // remove item from list
          //
          InternalRemoveAliasFromList(Info->ReplacementName, &CurrentScriptFile->SubstList);
        }
        FreePool(Info);
      }
    } else {
      //
      // Must be in 'in' version of for loop...
      //
      ASSERT(Info->Set != NULL);
      if (Info->CurrentValue != NULL && *Info->CurrentValue != CHAR_NULL) {
        if (Info->CurrentValue[0] == L' ') {
          Info->CurrentValue++;
        }
        if (Info->CurrentValue[0] == L'\"') {
          Info->CurrentValue++;
        }
        //
        // do the next one of the set
        //
        ASSERT(TempString == NULL);
        TempString = StrnCatGrow(&TempString, NULL, Info->CurrentValue, 0);
        if (TempString == NULL) {
          ShellStatus = SHELL_OUT_OF_RESOURCES;
        } else {
          TempSpot   = StrStr(TempString, L"\" \"");
          if (TempSpot != NULL) {
            *TempSpot = CHAR_NULL;
          }
          while (TempString[StrLen(TempString)-1] == L'\"') {
            TempString[StrLen(TempString)-1] = CHAR_NULL;
          }
          InternalUpdateAliasOnList(Info->ReplacementName, TempString, &CurrentScriptFile->SubstList);
          Info->CurrentValue += StrLen(TempString);

          if (Info->CurrentValue[0] == L'\"') {
            Info->CurrentValue++;
          }
          while (Info->CurrentValue[0] == L' ') {
            Info->CurrentValue++;
          }
          if (Info->CurrentValue[0] == L'\"') {
            Info->CurrentValue++;
          }
          FreePool(TempString);
        }
      } else {
        CurrentScriptFile->CurrentCommand->Data = NULL;
        //
        // find the matching endfor (we're done with the loop)
        //
        if (!MoveToTag(GetNextNode, L"endfor", L"for", NULL, CurrentScriptFile, TRUE, FALSE, FALSE)) {
          ShellPrintHiiEx(
            -1, 
            -1, 
            NULL, 
            STRING_TOKEN (STR_SYNTAX_NO_MATCHING), 
            gShellLevel1HiiHandle, 
            L"EndFor", 
            L"For", 
            CurrentScriptFile!=NULL 
              && CurrentScriptFile->CurrentCommand!=NULL
              ? CurrentScriptFile->CurrentCommand->Line:0);
          ShellStatus = SHELL_DEVICE_ERROR;
        }
        if (Info->RemoveSubstAlias) {
          //
          // remove item from list
          //
          InternalRemoveAliasFromList(Info->ReplacementName, &CurrentScriptFile->SubstList);
        }
        FreePool(Info);
      }
    }
  }
  if (ArgSet != NULL) {
    FreePool(ArgSet);
  }
  return (ShellStatus);
}

