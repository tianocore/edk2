/** @file
  Main file for If and else shell level 1 function.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellLevel1CommandsLib.h"
#include <Library/PrintLib.h>




typedef enum {
  END_TAG_OR,
  END_TAG_AND,
  END_TAG_THEN,
  END_TAG_MAX
} END_TAG_TYPE;

typedef enum {
  OPERATOR_GT,
  OPERATOR_LT,
  OPERATOR_EQ,
  OPERATOR_NE,
  OPERATOR_GE,
  OPERATOR_LE,
  OPERATOR_UGT,
  OPERATOR_ULT,
  OPERATOR_UGE,
  OPERATOR_ULE,
  OPERATOR_MAX
} BIN_OPERATOR_TYPE;

BOOLEAN
EFIAPI
IsNextFragment (
  IN CONST CHAR16         **Statement,
  IN CONST CHAR16         *Fragment
  )
{
  CHAR16                  *Tester;

  Tester = NULL;

  Tester = StrnCatGrow(&Tester, NULL, *Statement, StrLen(Fragment));
  ASSERT(Tester != NULL);
  Tester[StrLen(Fragment)] = CHAR_NULL;
  if (gUnicodeCollation->StriColl(
        gUnicodeCollation,
        (CHAR16*)Fragment,
        Tester) == 0) {
    //
    // increment the string pointer to the end of what we found and then chop off spaces...
    //
    *Statement+=StrLen(Fragment);
    while (*Statement[0] == L' ') {
      (*Statement)++;
    }
    FreePool(Tester);
    return (TRUE);
  }
  FreePool(Tester);
  return (FALSE);
}

BOOLEAN
EFIAPI
IsValidProfile (
  IN CONST CHAR16 *String
  )
{
  CONST CHAR16  *ProfilesString;
  CONST CHAR16  *TempLocation;

  ProfilesString = ShellGetEnvironmentVariable(L"profiles");
  TempLocation = StrStr(ProfilesString, String);
  if ((TempLocation != NULL) && (*(TempLocation-1) == L';') && (*(TempLocation+StrLen(String)) == L';')) {
    return (TRUE);
  }
  return (FALSE);
}

BOOLEAN
EFIAPI
TestOperation (
  IN CONST CHAR16             *Compare1,
  IN CONST CHAR16             *Compare2,
  IN CONST BIN_OPERATOR_TYPE  BinOp,
  IN CONST BOOLEAN            CaseInsensitive,
  IN CONST BOOLEAN            ForceStringCompare
  )
{
  INTN Cmp1;
  INTN Cmp2;

  //
  // "Compare1 BinOp Compare2"
  //
  switch (BinOp) {
  case OPERATOR_UGT:
  case OPERATOR_GT:
    if (ForceStringCompare || !ShellIsHexOrDecimalNumber(Compare1, FALSE, FALSE) || !ShellIsHexOrDecimalNumber(Compare2, FALSE, FALSE)) {
      //
      // string compare
      //
      if ((CaseInsensitive && StringNoCaseCompare(&Compare1, &Compare2) > 0) || (StringCompare(&Compare1, &Compare2) > 0)) {
        return (TRUE);
      }
    } else {
      //
      // numeric compare
      //
      if (Compare1[0] == L'-') {
        Cmp1 = 0 - (INTN)StrDecimalToUintn(Compare1+1);
      } else {
        Cmp1 = (INTN)StrDecimalToUintn(Compare1);
      }
      if (Compare2[0] == L'-') {
        Cmp2 = 0 - (INTN)StrDecimalToUintn(Compare2+1);
      } else {
        Cmp2 = (INTN)StrDecimalToUintn(Compare2);
      }
      if (BinOp == OPERATOR_GT) {
        if (Cmp1 > Cmp2) {
          return (TRUE);
        }
      } else {
        if ((UINTN)Cmp1 > (UINTN)Cmp2) {
          return (TRUE);
        }
      }
    }
    return (FALSE);
    break;
  case OPERATOR_ULT:
  case OPERATOR_LT:
    if (ForceStringCompare || !ShellIsHexOrDecimalNumber(Compare1, FALSE, FALSE) || !ShellIsHexOrDecimalNumber(Compare2, FALSE, FALSE)) {
      //
      // string compare
      //
      if ((CaseInsensitive && StringNoCaseCompare(&Compare1, &Compare2) < 0) || (StringCompare(&Compare1, &Compare2) < 0)) {
        return (TRUE);
      }
    } else {
      //
      // numeric compare
      //
      if (Compare1[0] == L'-') {
        Cmp1 = 0 - (INTN)StrDecimalToUintn(Compare1+1);
      } else {
        Cmp1 = (INTN)StrDecimalToUintn(Compare1);
      }
      if (Compare2[0] == L'-') {
        Cmp2 = 0 - (INTN)StrDecimalToUintn(Compare2+1);
      } else {
        Cmp2 = (INTN)StrDecimalToUintn(Compare2);
      }
      if (BinOp == OPERATOR_LT) {
        if (Cmp1 < Cmp2) {
          return (TRUE);
        }
      } else {
        if ((UINTN)Cmp1 < (UINTN)Cmp2) {
          return (TRUE);
        }
      }

    }
    return (FALSE);
    break;
  case OPERATOR_EQ:
    if (ForceStringCompare || !ShellIsHexOrDecimalNumber(Compare1, FALSE, FALSE) || !ShellIsHexOrDecimalNumber(Compare2, FALSE, FALSE)) {
      //
      // string compare
      //
      if ((CaseInsensitive && StringNoCaseCompare(&Compare1, &Compare2) == 0) || (StringCompare(&Compare1, &Compare2) == 0)) {
        return (TRUE);
      }
    } else {
      //
      // numeric compare
      //
      if (Compare1[0] == L'-') {
        Cmp1 = 0 - (INTN)ShellStrToUintn(Compare1+1);
      } else {
        Cmp1 = (INTN)ShellStrToUintn(Compare1);
      }
      if (Compare2[0] == L'-') {
        Cmp2 = 0 - (INTN)ShellStrToUintn(Compare2+1);
      } else {
        Cmp2 = (INTN)ShellStrToUintn(Compare2);
      }
      if (Cmp1 == Cmp2) {
        return (TRUE);
      }
    }
    return (FALSE);
    break;
  case OPERATOR_NE:
    if (ForceStringCompare || !ShellIsHexOrDecimalNumber(Compare1, FALSE, FALSE) || !ShellIsHexOrDecimalNumber(Compare2, FALSE, FALSE)) {
      //
      // string compare
      //
      if ((CaseInsensitive && StringNoCaseCompare(&Compare1, &Compare2) != 0) || (StringCompare(&Compare1, &Compare2) != 0)) {
        return (TRUE);
      }
    } else {
      //
      // numeric compare
      //
      if (Compare1[0] == L'-') {
        Cmp1 = 0 - (INTN)StrDecimalToUintn(Compare1+1);
      } else {
        Cmp1 = (INTN)StrDecimalToUintn(Compare1);
      }
      if (Compare2[0] == L'-') {
        Cmp2 = 0 - (INTN)StrDecimalToUintn(Compare2+1);
      } else {
        Cmp2 = (INTN)StrDecimalToUintn(Compare2);
      }
      if (Cmp1 != Cmp2) {
        return (TRUE);
      }
    }
    return (FALSE);
    break;
  case OPERATOR_UGE:
  case OPERATOR_GE:
    if (ForceStringCompare || !ShellIsHexOrDecimalNumber(Compare1, FALSE, FALSE) || !ShellIsHexOrDecimalNumber(Compare2, FALSE, FALSE)) {
      //
      // string compare
      //
      if ((CaseInsensitive && StringNoCaseCompare(&Compare1, &Compare2) >= 0) || (StringCompare(&Compare1, &Compare2) >= 0)) {
        return (TRUE);
      }
    } else {
      //
      // numeric compare
      //
      if (Compare1[0] == L'-') {
        Cmp1 = 0 - (INTN)StrDecimalToUintn(Compare1+1);
      } else {
        Cmp1 = (INTN)StrDecimalToUintn(Compare1);
      }
      if (Compare2[0] == L'-') {
        Cmp2 = 0 - (INTN)StrDecimalToUintn(Compare2+1);
      } else {
        Cmp2 = (INTN)StrDecimalToUintn(Compare2);
      }
      if (BinOp == OPERATOR_GE) {
        if (Cmp1 >= Cmp2) {
          return (TRUE);
        }
      } else {
        if ((UINTN)Cmp1 >= (UINTN)Cmp2) {
          return (TRUE);
        }
      }
    }
    return (FALSE);
    break;
  case OPERATOR_LE:
  case OPERATOR_ULE:
    if (ForceStringCompare || !ShellIsHexOrDecimalNumber(Compare1, FALSE, FALSE) || !ShellIsHexOrDecimalNumber(Compare2, FALSE, FALSE)) {
      //
      // string compare
      //
      if ((CaseInsensitive && StringNoCaseCompare(&Compare1, &Compare2) <= 0) || (StringCompare(&Compare1, &Compare2) <= 0)) {
        return (TRUE);
      }
    } else {
      //
      // numeric compare
      //
      if (Compare1[0] == L'-') {
        Cmp1 = 0 - (INTN)StrDecimalToUintn(Compare1+1);
      } else {
        Cmp1 = (INTN)StrDecimalToUintn(Compare1);
      }
      if (Compare2[0] == L'-') {
        Cmp2 = 0 - (INTN)StrDecimalToUintn(Compare2+1);
      } else {
        Cmp2 = (INTN)StrDecimalToUintn(Compare2);
      }
      if (BinOp == OPERATOR_LE) {
        if (Cmp1 <= Cmp2) {
          return (TRUE);
        }
      } else {
        if ((UINTN)Cmp1 <= (UINTN)Cmp2) {
          return (TRUE);
        }
      }
    }
    return (FALSE);
    break;
  default:
    ASSERT(FALSE);
    return (FALSE);
  }
  ASSERT(FALSE);
  return (FALSE);
}

EFI_STATUS
EFIAPI
ProcessStatement (
  IN OUT BOOLEAN          *PassingState,
  IN UINTN                StartParameterNumber,
  IN UINTN                EndParameterNumber,
  IN CONST END_TAG_TYPE   OperatorToUse,
  IN CONST BOOLEAN        CaseInsensitive,
  IN CONST BOOLEAN        ForceStringCompare
  )
{
  EFI_STATUS              Status;
  BOOLEAN                 OperationResult;
  BOOLEAN                 NotPresent;
  CHAR16                  *StatementWalker;
  BIN_OPERATOR_TYPE       BinOp;
  CHAR16                  *Compare1;
  CHAR16                  *Compare2;
  CHAR16                  HexString[20];
  CHAR16                  *TempSpot;

  ASSERT((END_TAG_TYPE)OperatorToUse != END_TAG_THEN);

  Status          = EFI_SUCCESS;
  BinOp           = OPERATOR_MAX;
  OperationResult = FALSE;
  StatementWalker = gEfiShellParametersProtocol->Argv[StartParameterNumber];
  if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"not")) {
    NotPresent      = TRUE;
    StatementWalker = gEfiShellParametersProtocol->Argv[++StartParameterNumber];
  } else {
    NotPresent = FALSE;
  }

  //
  // now check for 'boolfunc' operators
  //
  if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"isint")) {
    if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"(") && StatementWalker[StrLen(StatementWalker)-1] == L')') {
      StatementWalker[StrLen(StatementWalker)-1] = CHAR_NULL;
      OperationResult = ShellIsHexOrDecimalNumber(StatementWalker, FALSE, FALSE);
    } else {
      Status = EFI_INVALID_PARAMETER;
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_IN), gShellLevel1HiiHandle, L"isint");
    }
  } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"exists") || IsNextFragment((CONST CHAR16**)(&StatementWalker), L"exist")) {
    if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"(") && StatementWalker[StrLen(StatementWalker)-1] == L')') {
      StatementWalker[StrLen(StatementWalker)-1] = CHAR_NULL;
      //
      // is what remains a file in CWD???
      //
      OperationResult = (BOOLEAN)(ShellFileExists(StatementWalker)==EFI_SUCCESS);
    } else if (StatementWalker[0] == CHAR_NULL && StartParameterNumber+1 == EndParameterNumber) {
      OperationResult = (BOOLEAN)(ShellFileExists(gEfiShellParametersProtocol->Argv[++StartParameterNumber])==EFI_SUCCESS);
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_IN), gShellLevel1HiiHandle, L"exist(s)");
      Status = EFI_INVALID_PARAMETER;
    }
  } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"available")) {
    if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"(") && StatementWalker[StrLen(StatementWalker)-1] == L')') {
      StatementWalker[StrLen(StatementWalker)-1] = CHAR_NULL;
      //
      // is what remains a file in the CWD or path???
      //
      OperationResult = (BOOLEAN)(ShellIsFileInPath(StatementWalker)==EFI_SUCCESS);
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_IN), gShellLevel1HiiHandle, L"available");
      Status = EFI_INVALID_PARAMETER;
    }
  } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"profile")) {
    if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"(") && StatementWalker[StrLen(StatementWalker)-1] == L')') {
      //
      // Chop off that ')'
      //
      StatementWalker[StrLen(StatementWalker)-1] = CHAR_NULL;
      OperationResult = IsValidProfile(StatementWalker);
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_IN), gShellLevel1HiiHandle, L"profile");
      Status = EFI_INVALID_PARAMETER;
    }
  } else if (StartParameterNumber+1 >= EndParameterNumber) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_IN), gShellLevel1HiiHandle, gEfiShellParametersProtocol->Argv[StartParameterNumber]);
      Status = EFI_INVALID_PARAMETER;
  } else {
    //
    // must be 'item binop item' style
    //
    Compare1 = NULL;
    Compare2 = NULL;
    BinOp    = OPERATOR_MAX;

    //
    // get the first item
    //
    StatementWalker = gEfiShellParametersProtocol->Argv[StartParameterNumber];
    if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"efierror")) {
      TempSpot = StrStr(StatementWalker, L")");
      if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"(") && TempSpot != NULL) {
        *TempSpot = CHAR_NULL;
        if (ShellIsHexOrDecimalNumber(StatementWalker, FALSE, FALSE)) {
          UnicodeSPrint(HexString, sizeof(HexString), L"0x%x", ShellStrToUintn(StatementWalker)|MAX_BIT);
          ASSERT(Compare1 == NULL);
          Compare1 = StrnCatGrow(&Compare1, NULL, HexString, 0);
          StatementWalker += StrLen(StatementWalker) + 1;
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_IN), gShellLevel1HiiHandle, L"efierror");
          Status = EFI_INVALID_PARAMETER;
        }
      } else {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_IN), gShellLevel1HiiHandle, L"efierror");
        Status = EFI_INVALID_PARAMETER;
      }
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"pierror")) {
      TempSpot = StrStr(StatementWalker, L")");
      if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"(") && TempSpot != NULL) {
        *TempSpot = CHAR_NULL;
        if (ShellIsHexOrDecimalNumber(StatementWalker, FALSE, FALSE)) {
          UnicodeSPrint(HexString, sizeof(HexString), L"0x%x", ShellStrToUintn(StatementWalker)|MAX_BIT|(MAX_BIT>>2));
          ASSERT(Compare1 == NULL);
          Compare1 = StrnCatGrow(&Compare1, NULL, HexString, 0);
          StatementWalker += StrLen(StatementWalker) + 1;
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_IN), gShellLevel1HiiHandle, L"pierror");
          Status = EFI_INVALID_PARAMETER;
        }
      } else {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_IN), gShellLevel1HiiHandle, L"pierror");
        Status = EFI_INVALID_PARAMETER;
      }
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"oemerror")) {
      TempSpot = StrStr(StatementWalker, L")");
      if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"(") && TempSpot != NULL) {
        TempSpot = CHAR_NULL;
        if (ShellIsHexOrDecimalNumber(StatementWalker, FALSE, FALSE)) {
          UnicodeSPrint(HexString, sizeof(HexString), L"0x%x", ShellStrToUintn(StatementWalker)|MAX_BIT|(MAX_BIT>>1));
          ASSERT(Compare1 == NULL);
          Compare1 = StrnCatGrow(&Compare1, NULL, HexString, 0);
          StatementWalker += StrLen(StatementWalker) + 1;
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_IN), gShellLevel1HiiHandle, L"oemerror");
          Status = EFI_INVALID_PARAMETER;
        }
      } else {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_IN), gShellLevel1HiiHandle, L"oemerror");
        Status = EFI_INVALID_PARAMETER;
      }
    } else {
      ASSERT(Compare1 == NULL);
      if (EndParameterNumber - StartParameterNumber > 2) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_STARTING), gShellLevel1HiiHandle, gEfiShellParametersProtocol->Argv[StartParameterNumber+2]);
          Status = EFI_INVALID_PARAMETER;
      } else {
        //
        // must be a raw string
        //
        Compare1 = StrnCatGrow(&Compare1, NULL, StatementWalker, 0);
      }
    }

    //
    // get the operator
    //
    ASSERT(StartParameterNumber+1<EndParameterNumber);
    StatementWalker = gEfiShellParametersProtocol->Argv[StartParameterNumber+1];
    if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"gt")) {
      BinOp = OPERATOR_GT;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"lt")) {
      BinOp = OPERATOR_LT;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"eq")) {
      BinOp = OPERATOR_EQ;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"ne")) {
      BinOp = OPERATOR_NE;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"ge")) {
      BinOp = OPERATOR_GE;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"le")) {
      BinOp = OPERATOR_LE;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"==")) {
      BinOp = OPERATOR_EQ;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"ugt")) {
      BinOp = OPERATOR_UGT;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"ult")) {
      BinOp = OPERATOR_ULT;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"uge")) {
      BinOp = OPERATOR_UGE;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"ule")) {
      BinOp = OPERATOR_ULE;
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_INVALID_BINOP), gShellLevel1HiiHandle, StatementWalker);
      Status = EFI_INVALID_PARAMETER;
    }

    //
    // get the second item
    //
    ASSERT(StartParameterNumber+2<=EndParameterNumber);
    StatementWalker = gEfiShellParametersProtocol->Argv[StartParameterNumber+2];
    if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"efierror")) {
      TempSpot = StrStr(StatementWalker, L")");
      if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"(") && TempSpot != NULL) {
        TempSpot = CHAR_NULL;
        if (ShellIsHexOrDecimalNumber(StatementWalker, FALSE, FALSE)) {
          UnicodeSPrint(HexString, sizeof(HexString), L"0x%x", ShellStrToUintn(StatementWalker)|MAX_BIT);
          ASSERT(Compare2 == NULL);
          Compare2 = StrnCatGrow(&Compare2, NULL, HexString, 0);
          StatementWalker += StrLen(StatementWalker) + 1;
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_IN), gShellLevel1HiiHandle, L"efierror");
          Status = EFI_INVALID_PARAMETER;
        }
      } else {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_IN), gShellLevel1HiiHandle, L"efierror");
        Status = EFI_INVALID_PARAMETER;
      }
    //
    // can this be collapsed into the above?
    //
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"pierror")) {
      TempSpot = StrStr(StatementWalker, L")");
      if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"(") && TempSpot != NULL) {
        TempSpot = CHAR_NULL;
        if (ShellIsHexOrDecimalNumber(StatementWalker, FALSE, FALSE)) {
          UnicodeSPrint(HexString, sizeof(HexString), L"0x%x", ShellStrToUintn(StatementWalker)|MAX_BIT|(MAX_BIT>>2));
          ASSERT(Compare2 == NULL);
          Compare2 = StrnCatGrow(&Compare2, NULL, HexString, 0);
          StatementWalker += StrLen(StatementWalker) + 1;
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_IN), gShellLevel1HiiHandle, L"pierror");
          Status = EFI_INVALID_PARAMETER;
        }
      } else {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_IN), gShellLevel1HiiHandle, L"pierror");
        Status = EFI_INVALID_PARAMETER;
      }
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"oemerror")) {
      TempSpot = StrStr(StatementWalker, L")");
      if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"(") && TempSpot != NULL) {
        TempSpot = CHAR_NULL;
        if (ShellIsHexOrDecimalNumber(StatementWalker, FALSE, FALSE)) {
          UnicodeSPrint(HexString, sizeof(HexString), L"0x%x", ShellStrToUintn(StatementWalker)|MAX_BIT|(MAX_BIT>>1));
          ASSERT(Compare2 == NULL);
          Compare2 = StrnCatGrow(&Compare2, NULL, HexString, 0);
          StatementWalker += StrLen(StatementWalker) + 1;
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_IN), gShellLevel1HiiHandle, L"oemerror");
          Status = EFI_INVALID_PARAMETER;
        }
      } else {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_IN), gShellLevel1HiiHandle, L"oemerror");
        Status = EFI_INVALID_PARAMETER;
      }
    } else {
      //
      // must be a raw string
      //
      ASSERT(Compare2 == NULL);
      Compare2 = StrnCatGrow(&Compare2, NULL, StatementWalker, 0);
    }

    if (Compare1 != NULL && Compare2 != NULL && BinOp != OPERATOR_MAX) {
      OperationResult = TestOperation(Compare1, Compare2, BinOp, CaseInsensitive, ForceStringCompare);
    }

    SHELL_FREE_NON_NULL(Compare1);
    SHELL_FREE_NON_NULL(Compare2);
  }

  //
  // done processing do result...
  //

  if (!EFI_ERROR(Status)) {
    if (NotPresent) {
      OperationResult = (BOOLEAN)(!OperationResult);
    }
    switch(OperatorToUse) {
      case END_TAG_OR:
        *PassingState = (BOOLEAN)(*PassingState || OperationResult);
        break;
      case END_TAG_AND:
        *PassingState = (BOOLEAN)(*PassingState && OperationResult);
        break;
      case END_TAG_MAX:
        *PassingState = (BOOLEAN)(OperationResult);
        break;
      default:
        ASSERT(FALSE);
    }
  }
  return (Status);
}

BOOLEAN
EFIAPI
BuildNextStatement (
  IN UINTN          ParameterNumber,
  OUT UINTN         *EndParameter,
  OUT END_TAG_TYPE  *EndTag
  )
{
  CHAR16    *Buffer;
  UINTN     BufferSize;

  *EndTag = END_TAG_MAX;

  for(Buffer = NULL, BufferSize = 0
    ; ParameterNumber < gEfiShellParametersProtocol->Argc
    ; ParameterNumber++
   ) {
    if (gUnicodeCollation->StriColl(
          gUnicodeCollation,
          gEfiShellParametersProtocol->Argv[ParameterNumber],
          L"or") == 0) {
      *EndParameter = ParameterNumber - 1;
      *EndTag = END_TAG_OR;
      break;
    } else if (gUnicodeCollation->StriColl(
          gUnicodeCollation,
          gEfiShellParametersProtocol->Argv[ParameterNumber],
          L"and") == 0) {
      *EndParameter = ParameterNumber - 1;
      *EndTag = END_TAG_AND;
      break;
    } else if (gUnicodeCollation->StriColl(
          gUnicodeCollation,
          gEfiShellParametersProtocol->Argv[ParameterNumber],
          L"then") == 0) {
      *EndParameter = ParameterNumber - 1;
      *EndTag = END_TAG_THEN;
      break;
    }
  }
  if (*EndTag == END_TAG_MAX) {
    return (FALSE);
  }
  return (TRUE);
}

BOOLEAN
EFIAPI
MoveToTagSpecial (
  IN SCRIPT_FILE                *ScriptFile
  )
{
  SCRIPT_COMMAND_LIST *CommandNode;
  BOOLEAN             Found;
  UINTN               TargetCount;
  CHAR16              *CommandName;
  CHAR16              *CommandWalker;
  CHAR16              *TempLocation;

  TargetCount         = 1;
  Found               = FALSE;

  if (ScriptFile == NULL) {
    return FALSE;
  }

  for (CommandNode = (SCRIPT_COMMAND_LIST *)GetNextNode(&ScriptFile->CommandList, &ScriptFile->CurrentCommand->Link), Found = FALSE
    ;  !IsNull(&ScriptFile->CommandList, &CommandNode->Link) && !Found
    ;  CommandNode = (SCRIPT_COMMAND_LIST *)GetNextNode(&ScriptFile->CommandList, &CommandNode->Link)
   ){

    //
    // get just the first part of the command line...
    //
    CommandName   = NULL;
    CommandName   = StrnCatGrow(&CommandName, NULL, CommandNode->Cl, 0);
    CommandWalker = CommandName;
    while (CommandWalker[0] == L' ') {
      CommandWalker++;
    }
    TempLocation  = StrStr(CommandWalker, L" ");

    if (TempLocation != NULL) {
      *TempLocation = CHAR_NULL;
    }

    //
    // did we find a nested item ?
    //
    if (gUnicodeCollation->StriColl(
        gUnicodeCollation,
        (CHAR16*)CommandWalker,
        L"If") == 0) {
      TargetCount++;
    } else if (TargetCount == 1 && gUnicodeCollation->StriColl(
        gUnicodeCollation,
        (CHAR16*)CommandWalker,
        (CHAR16*)L"else") == 0) {
      //
      // else can only decrement the last part... not an nested if
      // hence the TargetCount compare added
      //
      TargetCount--;
    } else if (gUnicodeCollation->StriColl(
        gUnicodeCollation,
        (CHAR16*)CommandWalker,
        (CHAR16*)L"endif") == 0) {
      TargetCount--;
    }
    if (TargetCount == 0) {
      ScriptFile->CurrentCommand = (SCRIPT_COMMAND_LIST *)GetNextNode(&ScriptFile->CommandList, &CommandNode->Link);
      Found = TRUE;
    }

    //
    // Free the memory for this loop...
    //
    SHELL_FREE_NON_NULL(CommandName);
  }
  return (Found);
}

EFI_STATUS
EFIAPI
PerformResultOperation (
  IN CONST BOOLEAN Result
  )
{
  if (Result || MoveToTagSpecial(ShellCommandGetCurrentScriptFile())) {
    return (EFI_SUCCESS);
  }
  return (EFI_NOT_FOUND);
}

/**
  Function for 'if' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunIf (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  SHELL_STATUS        ShellStatus;
  BOOLEAN             CaseInsensitive;
  BOOLEAN             ForceString;
  UINTN               CurrentParameter;
  UINTN               EndParameter;
  BOOLEAN             CurrentValue;
  END_TAG_TYPE        Ending;
  END_TAG_TYPE        PreviousEnding;


  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  if (!gEfiShellProtocol->BatchIsActive()) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_NO_SCRIPT), gShellLevel1HiiHandle, L"If");
    return (SHELL_UNSUPPORTED);
  }

  if (gEfiShellParametersProtocol->Argc < 3) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel1HiiHandle);
    return (SHELL_INVALID_PARAMETER);
  }

  //
  // Make sure that an End exists.
  //
  if (!MoveToTag(GetNextNode, L"endif", L"if", NULL, ShellCommandGetCurrentScriptFile(), TRUE, TRUE, FALSE)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_NO_MATCHING), gShellLevel1HiiHandle, L"EnfIf", L"If", ShellCommandGetCurrentScriptFile()->CurrentCommand->Line);
    return (SHELL_DEVICE_ERROR);
  }

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  CurrentParameter    = 1;
  EndParameter        = 0;

  if (gUnicodeCollation->StriColl(
        gUnicodeCollation,
        gEfiShellParametersProtocol->Argv[1],
        L"/i") == 0 ||
      gUnicodeCollation->StriColl(
        gUnicodeCollation,
        gEfiShellParametersProtocol->Argv[2],
        L"/i") == 0 ||
      (gEfiShellParametersProtocol->Argc > 3 && gUnicodeCollation->StriColl(
        gUnicodeCollation,
        gEfiShellParametersProtocol->Argv[3],
        L"/i") == 0)) {
    CaseInsensitive = TRUE;
    CurrentParameter++;
  } else {
    CaseInsensitive = FALSE;
  }
  if (gUnicodeCollation->StriColl(
        gUnicodeCollation,
        gEfiShellParametersProtocol->Argv[1],
        L"/s") == 0 ||
      gUnicodeCollation->StriColl(
        gUnicodeCollation,
        gEfiShellParametersProtocol->Argv[2],
        L"/s") == 0 ||
      (gEfiShellParametersProtocol->Argc > 3 && gUnicodeCollation->StriColl(
        gUnicodeCollation,
        gEfiShellParametersProtocol->Argv[3],
        L"/s") == 0)) {
    ForceString     = TRUE;
    CurrentParameter++;
  } else {
    ForceString     = FALSE;
  }

  for ( ShellStatus = SHELL_SUCCESS, CurrentValue = FALSE, Ending = END_TAG_MAX
      ; CurrentParameter < gEfiShellParametersProtocol->Argc && ShellStatus == SHELL_SUCCESS
      ; CurrentParameter++) {
    if (gUnicodeCollation->StriColl(
          gUnicodeCollation,
          gEfiShellParametersProtocol->Argv[CurrentParameter],
          L"then") == 0) {
      //
      // we are at the then
      //
      if (CurrentParameter+1 != gEfiShellParametersProtocol->Argc) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_TEXT_AFTER_THEN), gShellLevel1HiiHandle);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        Status = PerformResultOperation(CurrentValue);
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_AFTER_BAD), gShellLevel1HiiHandle, gEfiShellParametersProtocol->Argv[CurrentParameter]);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
      }
    } else {
      PreviousEnding = Ending;
      //
      // build up the next statement for analysis
      //
      if (!BuildNextStatement(CurrentParameter, &EndParameter, &Ending)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_NO_MATCHING), gShellLevel1HiiHandle, L"Then", L"If", ShellCommandGetCurrentScriptFile()->CurrentCommand->Line);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        //
        // Analyze the statement
        //
        Status = ProcessStatement(&CurrentValue, CurrentParameter, EndParameter, PreviousEnding, CaseInsensitive, ForceString);
        if (EFI_ERROR(Status)) {
//          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_STARTING), gShellLevel1HiiHandle, gEfiShellParametersProtocol->Argv[CurrentParameter]);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          //
          // Optomize to get out of the loop early...
          //
          if ((Ending == END_TAG_OR && CurrentValue) || (Ending == END_TAG_AND && !CurrentValue)) {
            Status = PerformResultOperation(CurrentValue);
            if (EFI_ERROR(Status)) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_AFTER_BAD), gShellLevel1HiiHandle, gEfiShellParametersProtocol->Argv[CurrentParameter]);
              ShellStatus = SHELL_INVALID_PARAMETER;
            }
            break;
          }
        }
      }
      if (ShellStatus == SHELL_SUCCESS){
        CurrentParameter = EndParameter;
        //
        // Skip over the or or and parameter.
        //
        if (Ending == END_TAG_OR || Ending == END_TAG_AND) {
          CurrentParameter++;
        }
      }
    }
  }
  return (ShellStatus);
}

/**
  Function for 'else' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunElse (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  ASSERT_EFI_ERROR(CommandInit());

  if (gEfiShellParametersProtocol->Argc > 1) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel1HiiHandle);
    return (SHELL_INVALID_PARAMETER);
  }

  if (!gEfiShellProtocol->BatchIsActive()) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_NO_SCRIPT), gShellLevel1HiiHandle, L"Else");
    return (SHELL_UNSUPPORTED);
  }


  if (!MoveToTag(GetPreviousNode, L"if", L"endif", NULL, ShellCommandGetCurrentScriptFile(), FALSE, TRUE, FALSE)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_NO_MATCHING), gShellLevel1HiiHandle, L"If", L"Else", ShellCommandGetCurrentScriptFile()->CurrentCommand->Line);
    return (SHELL_DEVICE_ERROR);
  }
  if (!MoveToTag(GetPreviousNode, L"if", L"else", NULL, ShellCommandGetCurrentScriptFile(), FALSE, TRUE, FALSE)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_NO_MATCHING), gShellLevel1HiiHandle, L"If", L"Else", ShellCommandGetCurrentScriptFile()->CurrentCommand->Line);
    return (SHELL_DEVICE_ERROR);
  }

  if (!MoveToTag(GetNextNode, L"endif", L"if", NULL, ShellCommandGetCurrentScriptFile(), FALSE, FALSE, FALSE)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_NO_MATCHING), gShellLevel1HiiHandle, L"EndIf", "Else", ShellCommandGetCurrentScriptFile()->CurrentCommand->Line);
    return (SHELL_DEVICE_ERROR);
  }

  return (SHELL_SUCCESS);
}

/**
  Function for 'endif' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunEndIf (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  ASSERT_EFI_ERROR(CommandInit());

  if (gEfiShellParametersProtocol->Argc > 1) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel1HiiHandle);
    return (SHELL_INVALID_PARAMETER);
  }

  if (!gEfiShellProtocol->BatchIsActive()) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_NO_SCRIPT), gShellLevel1HiiHandle, L"Endif");
    return (SHELL_UNSUPPORTED);
  }

  if (!MoveToTag(GetPreviousNode, L"if", L"endif", NULL, ShellCommandGetCurrentScriptFile(), FALSE, TRUE, FALSE)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_NO_MATCHING), gShellLevel1HiiHandle, L"If", L"EndIf", ShellCommandGetCurrentScriptFile()->CurrentCommand->Line);
    return (SHELL_DEVICE_ERROR);
  }

  return (SHELL_SUCCESS);
}
