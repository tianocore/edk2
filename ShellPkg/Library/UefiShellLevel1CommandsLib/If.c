/** @file
  Main file for If and else shell level 1 function.

  (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
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
  EndTagOr,
  EndTagAnd,
  EndTagThen,
  EndTagMax
} END_TAG_TYPE;

typedef enum {
  OperatorGreaterThan,
  OperatorLessThan,
  OperatorEqual,
  OperatorNotEqual,
  OperatorGreatorOrEqual,
  OperatorLessOrEqual,
  OperatorUnisgnedGreaterThan,
  OperatorUnsignedLessThan,
  OperatorUnsignedGreaterOrEqual,
  OperatorUnsignedLessOrEqual,
  OperatorMax
} BIN_OPERATOR_TYPE;

/**
  Extract the next fragment, if there is one.

  @param[in, out] Statement    The current remaining statement.
  @param[in] Fragment          The current fragment.

  @retval FALSE   There is not another fragment.
  @retval TRUE    There is another fragment.
**/
BOOLEAN
EFIAPI
IsNextFragment (
  IN OUT CONST CHAR16     **Statement,
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

/**
  Determine if String represents a valid profile.

  @param[in] String     The pointer to the string to test.

  @retval TRUE    String is a valid profile.
  @retval FALSE   String is not a valid profile.
**/
BOOLEAN
EFIAPI
IsValidProfile (
  IN CONST CHAR16 *String
  )
{
  CONST CHAR16  *ProfilesString;
  CONST CHAR16  *TempLocation;

  ProfilesString = ShellGetEnvironmentVariable(L"profiles");
  ASSERT(ProfilesString != NULL);
  TempLocation = StrStr(ProfilesString, String);
  if ((TempLocation != NULL) && (*(TempLocation-1) == L';') && (*(TempLocation+StrLen(String)) == L';')) {
    return (TRUE);
  }
  return (FALSE);
}

/**
  Do a comparison between 2 things.

  @param[in] Compare1           The first item to compare.
  @param[in] Compare2           The second item to compare.
  @param[in] BinOp              The type of comparison to perform.
  @param[in] CaseInsensitive    TRUE to do non-case comparison, FALSE otherwise.
  @param[in] ForceStringCompare TRUE to force string comparison, FALSE otherwise.

  @return     The result of the comparison.
**/
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
  case OperatorUnisgnedGreaterThan:
  case OperatorGreaterThan:
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
        Cmp1 = 0 - (INTN)ShellStrToUintn(Compare1+1);
      } else {
        Cmp1 = (INTN)ShellStrToUintn(Compare1);
      }
      if (Compare2[0] == L'-') {
        Cmp2 = 0 - (INTN)ShellStrToUintn(Compare2+1);
      } else {
        Cmp2 = (INTN)ShellStrToUintn(Compare2);
      }
      if (BinOp == OperatorGreaterThan) {
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
  case OperatorUnsignedLessThan:
  case OperatorLessThan:
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
        Cmp1 = 0 - (INTN)ShellStrToUintn(Compare1+1);
      } else {
        Cmp1 = (INTN)ShellStrToUintn(Compare1);
      }
      if (Compare2[0] == L'-') {
        Cmp2 = 0 - (INTN)ShellStrToUintn(Compare2+1);
      } else {
        Cmp2 = (INTN)ShellStrToUintn(Compare2);
      }
      if (BinOp == OperatorLessThan) {
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
  case OperatorEqual:
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
  case OperatorNotEqual:
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
        Cmp1 = 0 - (INTN)ShellStrToUintn(Compare1+1);
      } else {
        Cmp1 = (INTN)ShellStrToUintn(Compare1);
      }
      if (Compare2[0] == L'-') {
        Cmp2 = 0 - (INTN)ShellStrToUintn(Compare2+1);
      } else {
        Cmp2 = (INTN)ShellStrToUintn(Compare2);
      }
      if (Cmp1 != Cmp2) {
        return (TRUE);
      }
    }
    return (FALSE);
  case OperatorUnsignedGreaterOrEqual:
  case OperatorGreatorOrEqual:
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
        Cmp1 = 0 - (INTN)ShellStrToUintn(Compare1+1);
      } else {
        Cmp1 = (INTN)ShellStrToUintn(Compare1);
      }
      if (Compare2[0] == L'-') {
        Cmp2 = 0 - (INTN)ShellStrToUintn(Compare2+1);
      } else {
        Cmp2 = (INTN)ShellStrToUintn(Compare2);
      }
      if (BinOp == OperatorGreatorOrEqual) {
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
  case OperatorLessOrEqual:
  case OperatorUnsignedLessOrEqual:
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
        Cmp1 = 0 - (INTN)ShellStrToUintn(Compare1+1);
      } else {
        Cmp1 = (INTN)ShellStrToUintn(Compare1);
      }
      if (Compare2[0] == L'-') {
        Cmp2 = 0 - (INTN)ShellStrToUintn(Compare2+1);
      } else {
        Cmp2 = (INTN)ShellStrToUintn(Compare2);
      }
      if (BinOp == OperatorLessOrEqual) {
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
  default:
    ASSERT(FALSE);
    return (FALSE);
  }
}

/**
  Process an if statement and determine if its is valid or not.

  @param[in, out] PassingState     Opon entry, the current state.  Upon exit, 
                                   the new state.
  @param[in] StartParameterNumber  The number of the first parameter of
                                   this statement.
  @param[in] EndParameterNumber    The number of the final parameter of
                                   this statement.
  @param[in] OperatorToUse         The type of termination operator.
  @param[in] CaseInsensitive       TRUE for case insensitive, FALSE otherwise.
  @param[in] ForceStringCompare    TRUE for all string based, FALSE otherwise.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_SUCCESS             The operation was successful.                                  
**/
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

  ASSERT((END_TAG_TYPE)OperatorToUse != EndTagThen);

  Status          = EFI_SUCCESS;
  BinOp           = OperatorMax;
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
    BinOp    = OperatorMax;

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
      BinOp = OperatorGreaterThan;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"lt")) {
      BinOp = OperatorLessThan;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"eq")) {
      BinOp = OperatorEqual;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"ne")) {
      BinOp = OperatorNotEqual;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"ge")) {
      BinOp = OperatorGreatorOrEqual;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"le")) {
      BinOp = OperatorLessOrEqual;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"==")) {
      BinOp = OperatorEqual;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"ugt")) {
      BinOp = OperatorUnisgnedGreaterThan;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"ult")) {
      BinOp = OperatorUnsignedLessThan;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"uge")) {
      BinOp = OperatorUnsignedGreaterOrEqual;
    } else if (IsNextFragment((CONST CHAR16**)(&StatementWalker), L"ule")) {
      BinOp = OperatorUnsignedLessOrEqual;
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

    if (Compare1 != NULL && Compare2 != NULL && BinOp != OperatorMax) {
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
      case EndTagOr:
        *PassingState = (BOOLEAN)(*PassingState || OperationResult);
        break;
      case EndTagAnd:
        *PassingState = (BOOLEAN)(*PassingState && OperationResult);
        break;
      case EndTagMax:
        *PassingState = (BOOLEAN)(OperationResult);
        break;
      default:
        ASSERT(FALSE);
    }
  }
  return (Status);
}

/**
  Break up the next part of the if statement (until the next 'and', 'or', or 'then').

  @param[in] ParameterNumber      The current parameter number.
  @param[out] EndParameter        Upon successful return, will point to the 
                                  parameter to start the next iteration with.
  @param[out] EndTag              Upon successful return, will point to the 
                                  type that was found at the end of this statement.

  @retval TRUE    A valid statement was found.
  @retval FALSE   A valid statement was not found.
**/
BOOLEAN
EFIAPI
BuildNextStatement (
  IN UINTN          ParameterNumber,
  OUT UINTN         *EndParameter,
  OUT END_TAG_TYPE  *EndTag
  )
{
  *EndTag = EndTagMax;

  for(
    ; ParameterNumber < gEfiShellParametersProtocol->Argc
    ; ParameterNumber++
   ) {
    if (gUnicodeCollation->StriColl(
          gUnicodeCollation,
          gEfiShellParametersProtocol->Argv[ParameterNumber],
          L"or") == 0) {
      *EndParameter = ParameterNumber - 1;
      *EndTag = EndTagOr;
      break;
    } else if (gUnicodeCollation->StriColl(
          gUnicodeCollation,
          gEfiShellParametersProtocol->Argv[ParameterNumber],
          L"and") == 0) {
      *EndParameter = ParameterNumber - 1;
      *EndTag = EndTagAnd;
      break;
    } else if (gUnicodeCollation->StriColl(
          gUnicodeCollation,
          gEfiShellParametersProtocol->Argv[ParameterNumber],
          L"then") == 0) {
      *EndParameter = ParameterNumber - 1;
      *EndTag = EndTagThen;
      break;
    }
  }
  if (*EndTag == EndTagMax) {
    return (FALSE);
  }
  return (TRUE);
}

/**
  Move the script file pointer to a different place in the script file.
  This one is special since it handles the if/else/endif syntax.

  @param[in] ScriptFile     The script file from GetCurrnetScriptFile().

  @retval TRUE     The move target was found and the move was successful.
  @retval FALSE    Something went wrong.
**/
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
    if (CommandName == NULL) {
      continue;
    }
    CommandWalker = CommandName;

    //
    // Skip leading spaces and tabs.
    //
    while ((CommandWalker[0] == L' ') || (CommandWalker[0] == L'\t')) {
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

/**
  Deal with the result of the if operation.

  @param[in] Result     The result of the if.

  @retval EFI_SUCCESS       The operation was successful.
  @retval EFI_NOT_FOUND     The ending tag could not be found.
**/
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
  SCRIPT_FILE         *CurrentScriptFile;

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  if (!gEfiShellProtocol->BatchIsActive()) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_NO_SCRIPT), gShellLevel1HiiHandle, L"if");  
    return (SHELL_UNSUPPORTED);
  }

  if (gEfiShellParametersProtocol->Argc < 3) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel1HiiHandle, L"if");  
    return (SHELL_INVALID_PARAMETER);
  }

  //
  // Make sure that an End exists.
  //
  CurrentScriptFile = ShellCommandGetCurrentScriptFile();
  if (!MoveToTag(GetNextNode, L"endif", L"if", NULL, CurrentScriptFile, TRUE, TRUE, FALSE)) {
    ShellPrintHiiEx(
      -1, 
      -1, 
      NULL, 
      STRING_TOKEN (STR_SYNTAX_NO_MATCHING), 
      gShellLevel1HiiHandle, 
      L"EndIf", 
      L"If", 
      CurrentScriptFile!=NULL 
        && CurrentScriptFile->CurrentCommand!=NULL
        ? CurrentScriptFile->CurrentCommand->Line:0);
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

  for ( ShellStatus = SHELL_SUCCESS, CurrentValue = FALSE, Ending = EndTagMax
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
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_TEXT_AFTER_THEN), gShellLevel1HiiHandle, L"if");  
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        Status = PerformResultOperation(CurrentValue);
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_AFTER_BAD), gShellLevel1HiiHandle, L"if", gEfiShellParametersProtocol->Argv[CurrentParameter]);  
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
      }
    } else {
      PreviousEnding = Ending;
      //
      // build up the next statement for analysis
      //
      if (!BuildNextStatement(CurrentParameter, &EndParameter, &Ending)) {
        CurrentScriptFile = ShellCommandGetCurrentScriptFile();
        ShellPrintHiiEx(
          -1, 
          -1, 
          NULL, 
          STRING_TOKEN (STR_SYNTAX_NO_MATCHING), 
          gShellLevel1HiiHandle, 
          L"Then", 
          L"If",
          CurrentScriptFile!=NULL 
            && CurrentScriptFile->CurrentCommand!=NULL
            ? CurrentScriptFile->CurrentCommand->Line:0);
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
          if ((Ending == EndTagOr && CurrentValue) || (Ending == EndTagAnd && !CurrentValue)) {
            Status = PerformResultOperation(CurrentValue);
            if (EFI_ERROR(Status)) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SYNTAX_AFTER_BAD), gShellLevel1HiiHandle, L"if", gEfiShellParametersProtocol->Argv[CurrentParameter]);  
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
        if (Ending == EndTagOr || Ending == EndTagAnd) {
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
  SCRIPT_FILE *CurrentScriptFile;
  ASSERT_EFI_ERROR(CommandInit());

  if (gEfiShellParametersProtocol->Argc > 1) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel1HiiHandle, L"if");  
    return (SHELL_INVALID_PARAMETER);
  }

  if (!gEfiShellProtocol->BatchIsActive()) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_NO_SCRIPT), gShellLevel1HiiHandle, L"Else");
    return (SHELL_UNSUPPORTED);
  }

  CurrentScriptFile = ShellCommandGetCurrentScriptFile();

  if (!MoveToTag(GetPreviousNode, L"if", L"endif", NULL, CurrentScriptFile, FALSE, TRUE, FALSE)) {
    ShellPrintHiiEx(
      -1, 
      -1, 
      NULL, 
      STRING_TOKEN (STR_SYNTAX_NO_MATCHING), 
      gShellLevel1HiiHandle, 
      L"If", 
      L"Else", 
      CurrentScriptFile!=NULL 
        && CurrentScriptFile->CurrentCommand!=NULL
        ? CurrentScriptFile->CurrentCommand->Line:0);
    return (SHELL_DEVICE_ERROR);
  }
  if (!MoveToTag(GetPreviousNode, L"if", L"else", NULL, CurrentScriptFile, FALSE, TRUE, FALSE)) {
    ShellPrintHiiEx(
      -1, 
      -1, 
      NULL, 
      STRING_TOKEN (STR_SYNTAX_NO_MATCHING), 
      gShellLevel1HiiHandle, 
      L"If", 
      L"Else", 
      CurrentScriptFile!=NULL 
        && CurrentScriptFile->CurrentCommand!=NULL
        ? CurrentScriptFile->CurrentCommand->Line:0);
    return (SHELL_DEVICE_ERROR);
  }

  if (!MoveToTag(GetNextNode, L"endif", L"if", NULL, CurrentScriptFile, FALSE, FALSE, FALSE)) {
    ShellPrintHiiEx(
      -1, 
      -1, 
      NULL, 
      STRING_TOKEN (STR_SYNTAX_NO_MATCHING), 
      gShellLevel1HiiHandle, 
      L"EndIf", 
      "Else", 
      CurrentScriptFile!=NULL 
        && CurrentScriptFile->CurrentCommand!=NULL
        ? CurrentScriptFile->CurrentCommand->Line:0);
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
  SCRIPT_FILE *CurrentScriptFile;
  ASSERT_EFI_ERROR(CommandInit());

  if (gEfiShellParametersProtocol->Argc > 1) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel1HiiHandle, L"if");  
    return (SHELL_INVALID_PARAMETER);
  }

  if (!gEfiShellProtocol->BatchIsActive()) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_NO_SCRIPT), gShellLevel1HiiHandle, L"Endif");
    return (SHELL_UNSUPPORTED);
  }

  CurrentScriptFile = ShellCommandGetCurrentScriptFile();
  if (!MoveToTag(GetPreviousNode, L"if", L"endif", NULL, CurrentScriptFile, FALSE, TRUE, FALSE)) {
    ShellPrintHiiEx(
      -1, 
      -1, 
      NULL, 
      STRING_TOKEN (STR_SYNTAX_NO_MATCHING), 
      gShellLevel1HiiHandle, 
      L"If", 
      L"EndIf", 
      CurrentScriptFile!=NULL 
        && CurrentScriptFile->CurrentCommand!=NULL
        ? CurrentScriptFile->CurrentCommand->Line:0);
    return (SHELL_DEVICE_ERROR);
  }

  return (SHELL_SUCCESS);
}
