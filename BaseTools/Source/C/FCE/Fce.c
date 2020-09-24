/** @file

 FCE is a tool which enables developers to retrieve and change HII configuration ("Setup")
 data in Firmware Device files (".fd" files).

 Copyright (c) 2011-2019, Intel Corporation. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Fce.h"

#ifndef __GNUC__
#define COPY_STR      "copy \"%s\" \"%s\" > NUL"
#define RMDIR_STR     "rmdir /S /Q \"%s\" > NUL"
#define DEL_STR       "del \"%s\" > NUL"
#else
#define COPY_STR      "cp \"%s\" \"%s\" > /dev/null"
#define RMDIR_STR     "rm -r \"%s\" > /dev/null"
#define DEL_STR       "rm \"%s\" > /dev/null"
#endif

//
// Utility global variables
//
OPERATION_TYPE  Operations;

CHAR8  mInputFdName[MAX_FILENAME_LEN];
CHAR8  mOutputFdName[MAX_FILENAME_LEN];
CHAR8  mOutTxtName[MAX_FILENAME_LEN];
CHAR8  mSetupTxtName[MAX_FILENAME_LEN];

CHAR8* mUtilityFilename        = NULL;
UINT32 mEfiVariableAddr        = 0;

UQI_PARAM_LIST           *mUqiList = NULL;
UQI_PARAM_LIST           *mLastUqiList = NULL;
LIST_ENTRY               mVarListEntry;
LIST_ENTRY               mBfvVarListEntry;
LIST_ENTRY               mAllVarListEntry;
LIST_ENTRY               mFormSetListEntry;

//
// Store GUIDed Section guid->tool mapping
//
EFI_HANDLE mParsedGuidedSectionTools = NULL;

CHAR8*     mGuidToolDefinition       = "GuidToolDefinitionConf.ini";

//
//gFfsArray is used to store all the FFS informations of Fd
//
G_EFI_FD_INFO               gEfiFdInfo;
//
//mMultiPlatformParam is used to store the structures about multi-platform support
//
MULTI_PLATFORM_PARAMETERS   mMultiPlatformParam;

UINT32                      mFormSetOrderRead;
UINT32                      mFormSetOrderParse;

CHAR8             mFullGuidToolDefinitionDir[_MAX_PATH];

CHAR8             *mFvNameGuidString = NULL;

/**
  Check the revision of BfmLib. If not matched, return an error.

  @retval  EFI_SUCCESS         If revision matched, return EFI_SUCCESS.
  @retval  EFI_UNSUPPORTED     Other cases.
**/
static
EFI_STATUS
CheckBfmLibRevision (
  VOID
  )
{
  CHAR8          *SystemCommandFormatString;
  CHAR8          *SystemCommand;
  CHAR8          *TempSystemCommand;
  CHAR8          *Revision;
  FILE           *FileHandle;
  CHAR8          RevisionStr[_MAX_BUILD_VERSION];
  UINT32         Len;

  SystemCommandFormatString = NULL;
  SystemCommand             = NULL;
  TempSystemCommand         = NULL;
  Revision                  = "Revision.txt";

  memset (RevisionStr, 0, _MAX_BUILD_VERSION);

  //
  // Construction 'system' command string
  //
  SystemCommandFormatString = "BfmLib -v > %s";
  SystemCommand = malloc (
                    strlen (SystemCommandFormatString) + strlen (Revision) + 1
                  );
  if (SystemCommand == NULL) {
    return EFI_UNSUPPORTED;
  }
  sprintf (
    SystemCommand,
    "BfmLib -v > %s",
    Revision
    );

  if (mFullGuidToolDefinitionDir[0] != 0) {
    TempSystemCommand = SystemCommand;
    SystemCommand = malloc (
                    strlen (mFullGuidToolDefinitionDir) + strlen (OS_SEP_STR) + strlen (SystemCommandFormatString) + strlen (Revision) + 1
                  );

    if (SystemCommand == NULL) {
      free (TempSystemCommand);
      return EFI_UNSUPPORTED;
    }
    strcpy (SystemCommand, mFullGuidToolDefinitionDir);
    strcat (SystemCommand, OS_SEP_STR);
    strcat (SystemCommand, TempSystemCommand);
    free (TempSystemCommand);
  }

  system (SystemCommand);
  free (SystemCommand);
  FileHandle = fopen (Revision, "r");
  if (FileHandle == NULL) {
    printf ("Error. Read the revision file of BfmLib failed.\n");
    return EFI_ABORTED;
  }
  fgets(RevisionStr, _MAX_BUILD_VERSION, FileHandle);
  Len = strlen(RevisionStr);
  if (RevisionStr[Len - 1] == '\n') {
    RevisionStr[Len - 1] = 0;
  }
  fclose (FileHandle);
  remove (Revision);

  if (strlen (RevisionStr) > _MAX_BUILD_VERSION - 1) {
    printf ("The revision string is too long");
    return EFI_UNSUPPORTED;
  }
  if (strcmp (RevisionStr, __BUILD_VERSION) == 0) {
    return EFI_SUCCESS;
  }
  return EFI_UNSUPPORTED;
}

/**
  Transfer the Ascii string to the Dec Number

  @param   InStr          The Ascii string.

  @retval  DecNum         Return the Dec number.
**/
static
UINT64
StrToDec (
  IN  CHAR8     *InStr
  )
{
  UINT8   Index;
  UINTN   DecNum;
  UINTN   Temp;

  Index   = 0;
  DecNum  = 0;
  Temp    = 0;

  if (InStr == NULL) {
    return (UINT64)-1;
  }
  while (Index < strlen (InStr)) {

    if ((*(InStr + Index) >= '0') && (*(InStr + Index) <= '9')) {
      Temp = *(InStr + Index) - '0';
    } else if ((*(InStr + Index) >= 'a') && (*(InStr + Index) <= 'f')) {
      Temp = *(InStr + Index) - 'a' + 10;
    } else if ((*(InStr + Index) >= 'A') && (*(InStr + Index) <= 'F')) {
      Temp = *(InStr + Index) - 'A' + 10;
    }
    DecNum = DecNum + Temp * (UINTN) pow (16, strlen (InStr) - Index - 1);
    Index++;
  }
  return DecNum;
}
/**
  Check whether there are some errors in uqi parameters of One_of

  @param Question      The pointer to the question Node.
  @param UqiValue      The value of One_of.

  @retval TRUE          If existed error, return TRUE;
  @return FALSE         Otherwise, return FALSE;
**/
static
BOOLEAN
CheckOneOfParamError (
  IN CONST FORM_BROWSER_STATEMENT   *Question,
  IN       UINT64                   UqiValue
  )
{
  LIST_ENTRY           *Link;
  QUESTION_OPTION      *Option;
  UINT64               Value;

  Link   = NULL;
  Option = NULL;
  Value  = 0;

  Link = GetFirstNode (&Question->OptionListHead);
  while (!IsNull (&Question->OptionListHead, Link)) {
    Option = QUESTION_OPTION_FROM_LINK (Link);
    Value  = 0;
    CopyMem (&Value, &Option->Value.Value.u64, Question->StorageWidth);
    if (Value == UqiValue) {
      return FALSE;
    }
    Link = GetNextNode (&Question->OptionListHead, Link);
  }

  return TRUE;
}

/**
  Check whether there are some errors in uqi parameters of Orderlist

  @param HiiObjList     The pointer to the Hii Object Node.
  @param UqiValue       The pointer to the Uqi parameter.

  @retval TRUE          If existed error, return TRUE;
  @return FALSE         Otherwise, return FALSE;
**/
static
BOOLEAN
CheckOrderParamError (
  IN CONST FORM_BROWSER_STATEMENT   *Question,
  IN       CHAR8                    *UqiValue
  )
{
  UINT8    MaxNum;
  MaxNum     = UqiValue[0];

  if (MaxNum != (UINT8)(Question->MaxContainers)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Writes a Unicode string

  @param  Package        A pointer to the Unicode string.

  @return NULL
**/
static
VOID
WriteUnicodeStr (
  IN     CHAR16              *pcString
  )
{
  UINTN  Index;

  if (pcString == NULL) {
    return;
  }

  for (Index = 0; pcString[Index] != 0; Index++) {
    printf("%c", pcString[Index] & 0x00FF);
  }
}

/**
  Parse and set the quick configure information by the command line.

  Read the UQI Config information from command line directly, and then compare it with the value in VarList.
  Update the Update flag in Varlist.

  @param  CurUqiList           The pointer to the current uqi
  @param  DefaultId            The default Id.
  @param  PlatformId           The platform Id.
  @param  CurQuestion          The pointer to the matched question

  @retval EFI_SUCCESS       It was complete successfully
  @retval EFI_UNSUPPORTED   Update a read-only value
  @return EFI_ABORTED       An error occurred
**/
static
EFI_STATUS
SetUqiParametersWithQuestion (
  IN  UQI_PARAM_LIST   *CurUqiList,
  IN  UINT16           DefaultId,
  IN  UINT64           PlatformId,
  IN  FORM_BROWSER_STATEMENT *CurQuestion
  )
{
  FORMSET_STORAGE         *VarList;
  BOOLEAN                 IsFound;
  UINT8                   *ValueAddrOfVar;
  UINT16                  Index;
  UINT64                  QuestionValue;
  UINT64                  UqiValue;
  EFI_STATUS              Status;
  CHAR8                   *ErrorStr;
  FORM_BROWSER_FORMSET    *FormSet;
  LIST_ENTRY              *FormSetLink;
  UINT64                  CurValue;

  VarList        = NULL;
  Index          = 0;
  ValueAddrOfVar = NULL;
  QuestionValue  = 0;
  UqiValue       = 0;
  ErrorStr       = NULL;
  Status         = EFI_SUCCESS;
  FormSet        = NULL;
  FormSetLink    = NULL;
  CurValue       = 0;

  //
  // Search the Variable List by VarStoreId and Offset
  //
  IsFound  = FALSE;
  FormSetLink = GetFirstNode (&mFormSetListEntry);
  while (!IsNull (&mFormSetListEntry, FormSetLink)) {
    FormSet = FORM_BROWSER_FORMSET_FROM_LINK (FormSetLink);
    Status = SearchVarStorage (
               CurQuestion,
               NULL,
               CurQuestion->VarStoreInfo.VarOffset,
               FormSet->StorageListHead,
              (CHAR8 **) &ValueAddrOfVar,
               &VarList
             );

    if (!EFI_ERROR (Status)) {
      IsFound = TRUE;
      break;
    }
    FormSetLink = GetNextNode (&mFormSetListEntry, FormSetLink);
  }

  if (!IsFound) {
    if (CurUqiList->Header.ScriptsLine == 0) {
      printf ("Error. The question in the command line doesn't store value by EFI_VARSTORE_IFR or EFI_VARSTORE_IFR_EFI.\n");
    } else {
      printf ("Error. The question in the line %d of script doesn't store value by EFI_VARSTORE_IFR or EFI_VARSTORE_IFR_EFI.\n", CurUqiList->Header.ScriptsLine);
    }
    return EFI_ABORTED;
  }

  //
  // Check the length of variable value
  //
  if (CurQuestion->QuestionReferToBitField) {
    GetBitsQuestionValue (CurQuestion, ValueAddrOfVar, (UINT32*)&QuestionValue);
  } else {
    switch (CurQuestion->StorageWidth) {

      case sizeof (UINT8):
        QuestionValue  = *(UINT8 *)ValueAddrOfVar;
        break;

      case sizeof (UINT16):
        QuestionValue  = *(UINT16 *)ValueAddrOfVar;
        break;

      case sizeof (UINT32):
        QuestionValue  = *(UINT32 *)ValueAddrOfVar;
        break;

      case sizeof (UINT64):
        QuestionValue  = *(UINT64 *)ValueAddrOfVar;
        break;

      default:
      //
      // The storage width of ORDERED_LIST may not be any type above.
      //
        ;
        break;
    }
  }
  UqiValue = *(UINT64 *)CurUqiList->Header.Value;
  CurUqiList->SameOrNot = TRUE;

  //
  // Check and set the checkbox value
  //
  if (CurQuestion->Operand == EFI_IFR_CHECKBOX_OP) {
    if ((UqiValue != 0) && (UqiValue != 1)) {
      CurUqiList->ErrorOrNot = TRUE;
      CurUqiList->SameOrNot  = FALSE;
      CurUqiList->Error      = malloc (ERROR_INFO_LENGTH * sizeof (CHAR8));
      if (CurUqiList->Error == NULL) {
        printf ("Fail to allocate memory!\n");
        return EFI_ABORTED;
      }
      memset (CurUqiList->Error, 0, ERROR_INFO_LENGTH);
      sprintf (CurUqiList->Error, "Error. The updated value of CHECKBOX 0x%llx is invalid.\n", (unsigned long long) UqiValue);
      if (CurQuestion->QuestionReferToBitField) {
        GetBitsQuestionValue (CurQuestion, ValueAddrOfVar, (UINT32*)CurUqiList->Header.DiffValue);
      } else {
        memcpy (
          CurUqiList->Header.DiffValue,
          ValueAddrOfVar,
          CurQuestion->StorageWidth
         );
      }
    } else {
      if (QuestionValue != UqiValue) {
        if (CurQuestion->QuestionReferToBitField) {
          GetBitsQuestionValue (CurQuestion, ValueAddrOfVar, (UINT32*)CurUqiList->Header.DiffValue);
          SetBitsQuestionValue (CurQuestion, ValueAddrOfVar, *(UINT32*)CurUqiList->Header.Value);
        } else {
          memcpy (
            CurUqiList->Header.DiffValue,
            ValueAddrOfVar,
            CurQuestion->StorageWidth
            );
          memcpy (
            ValueAddrOfVar,
            CurUqiList->Header.Value,
            CurQuestion->StorageWidth
            );
        }
        CurUqiList->SameOrNot   = FALSE;
      }
    }
  }
  if (CurQuestion->Operand == EFI_IFR_STRING_OP) {
   if ((FceStrLen((wchar_t *)CurUqiList->Header.Value) + 1) * 2 > CurQuestion->StorageWidth) {
     CurUqiList->ErrorOrNot = TRUE;
     CurUqiList->Error = malloc (ERROR_INFO_LENGTH * sizeof (CHAR8));
      if (CurUqiList->Error == NULL) {
        printf ("Fail to allocate memory!\n");
        return EFI_ABORTED;
      }
     memset (CurUqiList->Error, 0, ERROR_INFO_LENGTH);
     sprintf (CurUqiList->Error, "Error. The updated value of STRING exceed its Size.\n");
     memcpy (
        CurUqiList->Header.DiffValue,
        ValueAddrOfVar,
        CurQuestion->StorageWidth
      );
   } else {
     if (memcmp((CHAR8 *)CurUqiList->Header.Value, ValueAddrOfVar, CurQuestion->StorageWidth) != 0) {
       memcpy(
          CurUqiList->Header.DiffValue,
          ValueAddrOfVar,
          CurQuestion->StorageWidth
          );
       memcpy(
          ValueAddrOfVar,
          CurUqiList->Header.Value,
          CurQuestion->StorageWidth
          );
       CurUqiList->SameOrNot = FALSE;
     }
    }
  }
  //
  // Check and set the NUMERIC value
  //
  if (CurQuestion->Operand == EFI_IFR_NUMERIC_OP) {
    if ((UqiValue < CurQuestion->Minimum) || (UqiValue > CurQuestion->Maximum)) {
      CurUqiList->ErrorOrNot = TRUE;
      CurUqiList->SameOrNot  = FALSE;
      CurUqiList->Error = malloc (ERROR_INFO_LENGTH * sizeof (CHAR8));
      if (CurUqiList->Error == NULL) {
        printf ("Fail to allocate memory!\n");
        return EFI_ABORTED;
      }
      memset (CurUqiList->Error, 0, ERROR_INFO_LENGTH);
      sprintf (CurUqiList->Error, "Error. The updated value of NUMERIC 0x%llx is invalid.\n", (unsigned long long) UqiValue);
      if (CurQuestion->QuestionReferToBitField) {
        GetBitsQuestionValue (CurQuestion, ValueAddrOfVar, (UINT32*)CurUqiList->Header.DiffValue);
      } else {
        memcpy (
          CurUqiList->Header.DiffValue,
          ValueAddrOfVar,
          CurQuestion->StorageWidth
        );
      }
    } else {
      if (QuestionValue != UqiValue) {
        if (CurQuestion->QuestionReferToBitField) {
          GetBitsQuestionValue (CurQuestion, ValueAddrOfVar, (UINT32*)CurUqiList->Header.DiffValue);
          SetBitsQuestionValue (CurQuestion, ValueAddrOfVar, *(UINT32*)CurUqiList->Header.Value);
        } else {
          memcpy (
          CurUqiList->Header.DiffValue,
            ValueAddrOfVar,
            CurQuestion->StorageWidth
            );
          memcpy (
            ValueAddrOfVar,
            CurUqiList->Header.Value,
            CurQuestion->StorageWidth
            );
        }
        CurUqiList->SameOrNot = FALSE;
      }
    }
  }
  //
  // Check and set the ONE_OF value
  //
  if (CurQuestion->Operand == EFI_IFR_ONE_OF_OP) {
    if (CheckOneOfParamError (CurQuestion, UqiValue)) {
      CurUqiList->ErrorOrNot = TRUE;
      CurUqiList->SameOrNot  = FALSE;
      CurUqiList->Error = malloc (ERROR_INFO_LENGTH * sizeof (CHAR8));
      if (CurUqiList->Error == NULL) {
        printf ("Fail to allocate memory!\n");
        return EFI_ABORTED;
      }
      memset (CurUqiList->Error, 0, ERROR_INFO_LENGTH);
      sprintf (CurUqiList->Error, "Error. The updated ONE_OF value 0x%llx is invalid.\n", (unsigned long long) UqiValue);
      if (CurQuestion->QuestionReferToBitField) {
        GetBitsQuestionValue (CurQuestion, ValueAddrOfVar, (UINT32*)CurUqiList->Header.DiffValue);
      } else {
        memcpy (
          CurUqiList->Header.DiffValue,
          ValueAddrOfVar,
          CurQuestion->StorageWidth
        );
      }
    } else {
      if (QuestionValue != UqiValue) {
        if (CurQuestion->QuestionReferToBitField) {
          GetBitsQuestionValue (CurQuestion, ValueAddrOfVar, (UINT32*)CurUqiList->Header.DiffValue);
          SetBitsQuestionValue (CurQuestion, ValueAddrOfVar, *(UINT32*)CurUqiList->Header.Value);
        } else {
          memcpy (
            CurUqiList->Header.DiffValue,
            ValueAddrOfVar,
            CurQuestion->StorageWidth
            );
          memcpy (
            ValueAddrOfVar,
            CurUqiList->Header.Value,
            CurQuestion->StorageWidth
            );
        }
        CurUqiList->SameOrNot   = FALSE;
      }
    }
  }
  //
  // Check and set the ORDER_LIST value
  //
  if (CurQuestion->Operand == EFI_IFR_ORDERED_LIST_OP) {
    //
    // Synchronize the MaxContainers value
    //
    *CurUqiList->Header.DiffValue = (UINT8)CurQuestion->MaxContainers;

    if (CheckOrderParamError (CurQuestion, (CHAR8 *)CurUqiList->Header.Value)) {

      CurUqiList->ErrorOrNot = TRUE;
      CurUqiList->SameOrNot  = FALSE;
      CurUqiList->Error = malloc (ERROR_INFO_LENGTH * sizeof (CHAR8));
      if (CurUqiList->Error == NULL) {
        printf ("Fail to allocate memory!\n");
        return EFI_ABORTED;
      }
      memset (CurUqiList->Error, 0, ERROR_INFO_LENGTH);

      ErrorStr = CurUqiList->Error;
      sprintf (ErrorStr, "Error. The updated NORDERED_LIST value ");
      ErrorStr = ErrorStr + strlen ("Error. The updated NORDERED_LIST value ");

      for (Index = 0; Index < CurQuestion->StorageWidth; Index++) {
        sprintf (ErrorStr, "%02x ", *(CurUqiList->Header.Value + Index + 1));
        ErrorStr +=3;
      }
      sprintf (ErrorStr, "is invalid.\n");
      memcpy (
        CurUqiList->Header.DiffValue + 1,
        ValueAddrOfVar,
        CurQuestion->StorageWidth
      );
    } else {
      for (Index = 0; Index < CurQuestion->StorageWidth/CurQuestion->MaxContainers; Index++) {
        CurValue = 0;
        memcpy (
           &CurValue,
          (ValueAddrOfVar + Index * CurQuestion->StorageWidth/CurQuestion->MaxContainers),
           CurQuestion->StorageWidth/CurQuestion->MaxContainers
        );
        if (CurValue != *((UINT64 *)CurUqiList->Header.Value + Index + 1)) {
          CurUqiList->SameOrNot   = FALSE;
          break;
        }
      }
      if (!CurUqiList->SameOrNot) {
        memcpy (
          CurUqiList->Header.DiffValue + 1,
          ValueAddrOfVar,
          CurQuestion->StorageWidth
        );
        for (Index = 0; Index < CurQuestion->MaxContainers; Index++) {
          switch (CurQuestion->StorageWidth/CurQuestion->MaxContainers) {

          case sizeof (UINT8):
            *((UINT8 *)ValueAddrOfVar + Index) = *(UINT8 *)((UINT64 *)CurUqiList->Header.Value + 1 + Index);
            break;

          case sizeof (UINT16):
            *((UINT16 *)ValueAddrOfVar + Index) = *(UINT16 *)((UINT64 *)CurUqiList->Header.Value + 1 + Index);
            break;

          case sizeof (UINT32):
            *((UINT32 *)ValueAddrOfVar + Index) = *(UINT32 *)((UINT64 *)CurUqiList->Header.Value + 1 + Index);
            break;

          case sizeof (UINT64):
            *((UINT64 *)ValueAddrOfVar + Index) = *((UINT64 *)CurUqiList->Header.Value + 1 + Index);
            break;

          default:
            *((UINT8 *)ValueAddrOfVar + Index) = *(UINT8 *)((UINT64 *)CurUqiList->Header.Value + 1 + Index);
            break;
          }
        }
        //
        // Update the vaule of ORDERED_LIST according to its size
        //
        CurUqiList->Header.Value = (UINT8 *)((UINT64 *)CurUqiList->Header.Value);
        memcpy (
          CurUqiList->Header.Value + 1,
          ValueAddrOfVar,
          CurQuestion->StorageWidth
          );
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Parse and set the quick configure information by the command line.

  Read the UQI Config information from command line directly, and then compare it with the value in VarList.
  Update the Update flag in Varlist.

  @param  UqiList              The pointer to the uqi list
  @param  DefaultId            The default Id.
  @param  PlatformId           The platform Id.

  @retval EFI_SUCCESS       It was complete successfully
  @retval EFI_UNSUPPORTED   Update a read-only value
  @return EFI_ABORTED       An error occurred
**/
static
EFI_STATUS
SetUqiParameters (
  IN  UQI_PARAM_LIST   *UqiList,
  IN  UINT16           DefaultId,
  IN  UINT64           PlatformId
  )
{
  FORM_BROWSER_FORMSET    *FormSet;
  LIST_ENTRY              *FormSetLink;
  FORM_BROWSER_FORM       *Form;
  LIST_ENTRY              *FormLink;
  FORM_BROWSER_STATEMENT  *Question;
  LIST_ENTRY              *QuestionLink;
  LIST_ENTRY              *FormSetEntryListHead;
  UQI_PARAM_LIST          *CurUqiList;

  FormSet      = NULL;
  FormSetLink  = NULL;
  Form         = NULL;
  FormLink     = NULL;
  Question     = NULL;
  QuestionLink = NULL;
  FormSetEntryListHead = &mFormSetListEntry;

  FormSetLink = GetFirstNode (FormSetEntryListHead);
  while (!IsNull (FormSetEntryListHead, FormSetLink)) {
    FormSet = FORM_BROWSER_FORMSET_FROM_LINK (FormSetLink);
    //
    // Parse all forms in formset
    //
    FormLink = GetFirstNode (&FormSet->FormListHead);

    while (!IsNull (&FormSet->FormListHead, FormLink)) {
      Form = FORM_BROWSER_FORM_FROM_LINK (FormLink);
      //
      // Parse five kinds of Questions in Form
      //
      QuestionLink = GetFirstNode (&Form->StatementListHead);

      while (!IsNull (&Form->StatementListHead, QuestionLink)) {
        Question = FORM_BROWSER_STATEMENT_FROM_LINK (QuestionLink);
        QuestionLink = GetNextNode (&Form->StatementListHead, QuestionLink);
        //
        // Parse five kinds of Questions in Form
        //
        if ((Question->Operand == EFI_IFR_ONE_OF_OP)
          || (Question->Operand == EFI_IFR_NUMERIC_OP)
          || (Question->Operand == EFI_IFR_CHECKBOX_OP)
          || (Question->Operand == EFI_IFR_ORDERED_LIST_OP)
          || (Question->Operand == EFI_IFR_STRING_OP)
          ) {
          if (mMultiPlatformParam.MultiPlatformOrNot) {
            //
            // Only compare the valid EFI_IFR_VARSTORE_EFI_OP in multi-platform mode
            //
            if (Question->Type != EFI_IFR_VARSTORE_EFI_OP) {
              continue;
            }
            if ((Question->Type == EFI_IFR_VARSTORE_EFI_OP)
              && Question->NewEfiVarstore
              && ((Question->Attributes & EFI_VARIABLE_NON_VOLATILE) == 0)) {
              continue;
            }
          }

          CurUqiList = UqiList;
          while (CurUqiList != NULL) {
            if ((PlatformId == CurUqiList->Header.PlatformId[0])
              && (DefaultId == CurUqiList->Header.DefaultId[0])
              && CompareUqiHeader (&(Question->Uqi), &(CurUqiList->Header))
              ) {
              //
              // Add further check to avoid a case that there are many options with a
              // same UQI (en-Us), but always returns the first one.
              //
              if (!CurUqiList->ParseOrNot) {
                CurUqiList->ParseOrNot = TRUE;
                break;
              }
            }
            CurUqiList = CurUqiList->Next;
          }
          if (CurUqiList != NULL) {
            SetUqiParametersWithQuestion (CurUqiList, DefaultId, PlatformId, Question);
          }
        }
      }
      FormLink = GetNextNode (&FormSet->FormListHead, FormLink);
    }
    FormSetLink = GetNextNode (FormSetEntryListHead, FormSetLink);
  }

  return EFI_SUCCESS;
}

/**
  Set question value per UqiList.

  @param  UqiList              The pointer to the uqi list
  @param  DefaultId            The default Id.
  @param  PlatformId           The platform Id.
**/
VOID
SetUqiParametersMultiMode (
  IN  UQI_PARAM_LIST   *UqiList,
  IN  UINT16           DefaultId,
  IN  UINT64           PlatformId
  )
{
  UQI_PARAM_LIST   *CurUqiList;

  CurUqiList = UqiList;
  while (CurUqiList != NULL) {
    if ((CurUqiList->ParseOrNot == FALSE)
      && (PlatformId == CurUqiList->Header.PlatformId[0])
      && (DefaultId == CurUqiList->Header.DefaultId[0])
      ) {
      CurUqiList->ParseOrNot = TRUE;
      if (CurUqiList->Header.Question != NULL) {
        SetUqiParametersWithQuestion (CurUqiList, DefaultId, PlatformId, CurUqiList->Header.Question);
      }
    }
    CurUqiList = CurUqiList->Next;
  }
}

/**
  Find the matched question for each UQI string in UqiList

**/
EFI_STATUS
ScanUqiFullList (
  IN  UQI_PARAM_LIST   *UqiList
  )
{
  FORM_BROWSER_FORMSET    *FormSet;
  LIST_ENTRY              *FormSetLink;
  FORM_BROWSER_FORM       *Form;
  LIST_ENTRY              *FormLink;
  FORM_BROWSER_STATEMENT  *Question;
  LIST_ENTRY              *QuestionLink;
  LIST_ENTRY              *FormSetEntryListHead;
  UQI_PARAM_LIST          *CurUqiList;
  UINT64           PlatformId;
  UINT16           DefaultId;

  if (UqiList == NULL) {
    return EFI_SUCCESS;
  }
  FormSet      = NULL;
  FormSetLink  = NULL;
  Form         = NULL;
  FormLink     = NULL;
  Question     = NULL;
  QuestionLink = NULL;
  FormSetEntryListHead = &mFormSetListEntry;

  FormSetLink = FormSetEntryListHead->ForwardLink;
  while (FormSetEntryListHead != FormSetLink) {
    FormSet = FORM_BROWSER_FORMSET_FROM_LINK (FormSetLink);
    //
    // Parse all forms in formset
    //
    FormLink = FormSet->FormListHead.ForwardLink;

    while (&FormSet->FormListHead != FormLink) {
      Form = FORM_BROWSER_FORM_FROM_LINK (FormLink);
      //
      // Parse five kinds of Questions in Form
      //
      QuestionLink = Form->StatementListHead.ForwardLink;

      while (&Form->StatementListHead != QuestionLink) {
        Question = FORM_BROWSER_STATEMENT_FROM_LINK (QuestionLink);
        QuestionLink = QuestionLink->ForwardLink;
        //
        // Parse five kinds of Questions in Form
        //
        if ((Question->Operand == EFI_IFR_ONE_OF_OP)
          || (Question->Operand == EFI_IFR_NUMERIC_OP)
          || (Question->Operand == EFI_IFR_CHECKBOX_OP)
          || (Question->Operand == EFI_IFR_ORDERED_LIST_OP)
          || (Question->Operand == EFI_IFR_STRING_OP)
          ) {
          //
          // Only compare the valid EFI_IFR_VARSTORE_EFI_OP in multi-platform mode
          //
          if (Question->Type != EFI_IFR_VARSTORE_EFI_OP) {
            continue;
          } else if (Question->NewEfiVarstore
            && ((Question->Attributes & EFI_VARIABLE_NON_VOLATILE) == 0)) {
            continue;
          }

          CurUqiList = UqiList;
          PlatformId = 0xFFFFFFFF;
          DefaultId  = 0xFFFF;
          while (CurUqiList != NULL) {
            if ((CurUqiList->Header.Question == NULL)
              && CompareUqiHeader (&(Question->Uqi), &(CurUqiList->Header))
              && ((PlatformId != CurUqiList->Header.PlatformId[0]) || (DefaultId != CurUqiList->Header.DefaultId[0]))
              ) {
              CurUqiList->Header.Question = Question;
              PlatformId = CurUqiList->Header.PlatformId[0];
              DefaultId  = CurUqiList->Header.DefaultId[0];
            }
            CurUqiList = CurUqiList->Next;
          }
        }
      }
      FormLink = FormLink->ForwardLink;
    }
    FormSetLink = FormSetLink->ForwardLink;
  }

  return EFI_SUCCESS;
}

/**
  Create and insert the UQI Node to the UQI parameter list.

  @retval  Node address              If successed, return the node address.
  @retval  NULL                      An error occurred.

**/
static
UQI_PARAM_LIST *
CreateInsertUqiNode (
  )
{
  UQI_PARAM_LIST    *Node;

  Node = (UQI_PARAM_LIST *) malloc (sizeof (UQI_PARAM_LIST));
  if (Node == NULL) {
    return NULL;
  }
  memset (Node, 0, sizeof (UQI_PARAM_LIST));

  if (mUqiList == NULL) {
    mUqiList = Node;
  } else {
    mLastUqiList->Next = Node;
  }

  mLastUqiList = Node;
  return Node;
}
/**
  Parse the first part of QUI string

  @param  **argv[]                   The dual array pointer to the parameters.
  @param  Number                     The pointer to the number of the first character of UQI.
  @param  Buffer                     The buffer to store the first part of UQI.

  @retval  EFI_SUCCESS               Parse the QUI parameters successfully.
  @retval  EFI_INVALID_PARAMETER     An error occurred.

**/
static
EFI_STATUS
ParseFirstpartOfUqi (
  IN     CHAR8          **argv[],
  OUT    UINT32         *Number,
  OUT    UINT16          **Buffer
  )
{
  UINT32   Index;

  Index = 0;

  *Number = (UINT32)StrToDec ((*argv)[0]);

  if ((*Number <= 0) || (*Number > MAX_QUI_PARAM_LEN)) {
    printf ("Error. Invalid UQI.\n");
    return EFI_INVALID_PARAMETER;
  }

  *Buffer = malloc ((*Number + 1) * sizeof (CHAR16));
  assert (*Buffer != NULL);
  memset (*Buffer, 0, (*Number + 1) * sizeof (CHAR16));

  for (Index = 0; Index < *Number; Index++) {
    if (StrToDec ((*argv)[Index]) > 0xff) {
      printf ("Error. Invalid UQI.\n");
      return EFI_INVALID_PARAMETER;
    }
  *(*Buffer + Index) = (UINT16)StrToDec ((*argv)[Index + 1]);
 }
  return EFI_SUCCESS;
}

/**
  Parse the QUI input parameters from the command line

  @param  argc                       The pointer to the number of input parameters.
  @param  **argv[]                   The dual array pointer to the parameters.

  @retval  EFI_SUCCESS               Parse the QUI parameters successfully.
  @retval  EFI_INVALID_PARAMETER     An error occurred.

**/
static
EFI_STATUS
ParseUqiParam (
  IN  UINT32         *argc,
  IN  CHAR8          **argv[]
  )
{
  UINT32          Index;
  UQI_PARAM_LIST  *UqiNode;
  EFI_STATUS      Status;

  Index   = 0;
  UqiNode = NULL;

  if (*argc < 4) {
    printf ("Error. The correct command is 'FCE updateq -i <infd> -o <outfd> <UQI> <Question Type> <Value>'.\n");
    return EFI_INVALID_PARAMETER;
  }

  UqiNode = CreateInsertUqiNode ();
  assert (UqiNode != NULL);

  UqiNode->Header.DefaultId  = (UINT16 *) calloc (1, sizeof (UINT16));
  UqiNode->Header.PlatformId = (UINT64 *) calloc (1, sizeof (UINT64));

  Status = ParseFirstpartOfUqi (argv, &(UqiNode->Header.HexNum), &(UqiNode->Header.Data));
  if (EFI_ERROR (Status)) {
    return Status;
  }
  *argc -= (UqiNode->Header.HexNum + 1);
  *argv += UqiNode->Header.HexNum + 1;
  //
  // Parse the TYPE and value
  //
  if (strcmp ("ONE_OF", (*argv)[0]) == 0) {
    UqiNode->Header.Type = ONE_OF;
  } else if (strcmp ("NUMERIC", (*argv)[0]) == 0) {
    UqiNode->Header.Type = NUMERIC;
  } else if (strcmp ("CHECKBOX", (*argv)[0]) == 0) {
    UqiNode->Header.Type = CHECKBOX;
  } else if (strcmp ("STRING", (*argv)[0]) == 0) {
    UqiNode->Header.Type = STRING;
  } else if (strcmp ("ORDERED_LIST", (*argv)[0]) == 0) {
    UqiNode->Header.Type = ORDERED_LIST;
  } else {
    printf ("Error. The correct command is 'FCE updateq -i <infd> -o <outfd> <UQI> <Question Type> <Value>'.\n");
    return EFI_INVALID_PARAMETER;
  }
  *argc -= 1;
  *argv += 1;
  //
  // Get the value according to the type of questions.
  //
  switch (UqiNode->Header.Type) {

  case ONE_OF:
  case NUMERIC:
  case CHECKBOX:
    UqiNode->Header.Value    = malloc (sizeof (UINT64));
    if (UqiNode->Header.Value == NULL) {
      printf ("Fali to allocate memory!\n");
      return EFI_OUT_OF_RESOURCES;
    }
    memset (UqiNode->Header.Value, 0, sizeof (UINT64));

    UqiNode->Header.DiffValue    = malloc (sizeof (UINT64));
    if (UqiNode->Header.DiffValue == NULL) {
      printf ("Fali to allocate memory!\n");
      return EFI_OUT_OF_RESOURCES;
    }
    memset (
      UqiNode->Header.DiffValue,
      0,
      sizeof (UINT64)
      );
    *(UINT64 *)(UqiNode->Header.Value) = (UINT64)StrToDec ((*argv)[0]);
    break;

  case ORDERED_LIST:
    UqiNode->Header.Value    = malloc (((UINTN)StrToDec ((*argv)[0]) + 1) * sizeof (UINT64));
    if (UqiNode->Header.Value == NULL) {
      printf ("Fali to allocate memory!\n");
      return EFI_OUT_OF_RESOURCES;
    }
    memset (
      UqiNode->Header.Value,
      0,
      (UINTN)StrToDec(((*argv)[0]) + 1) * sizeof (UINT64)
      );

    UqiNode->Header.DiffValue    = malloc (((UINTN)StrToDec ((*argv)[0]) + 1) * sizeof (UINT64));
    if (UqiNode->Header.DiffValue == NULL) {
      printf ("Fali to allocate memory!\n");
      return EFI_OUT_OF_RESOURCES;
    }
    memset (
      UqiNode->Header.DiffValue,
      0,
      (UINTN)(StrToDec ((*argv)[0]) + 1) * sizeof (UINT64)
      );

    *UqiNode->Header.Value = (UINT8)StrToDec ((*argv)[0]);
    for (Index = 1; Index <= StrToDec ((*argv)[0]); Index++) {
      *((UINT64 *)UqiNode->Header.Value + Index) = StrToDec ((*argv)[Index]);
    }
    *argc -= (UINTN)StrToDec ((*argv)[0]);
    *argv += (UINTN)StrToDec ((*argv)[0]);
    break;

  default:
    break;
  }

  *argc -= 1;
  *argv += 1;

  if (*argc > 0) {
    printf ("Error. The correct command is 'FCE updateq -i <infd> -o <outfd> <UQI> <Question Type> <Value>'.\n");
    return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}
/**
  Parse the input Fd file, and get the file name according to the FILETYPE.

  @param  FdName             The Name of Fd file
  @param  FILETYPE           The type of Fd file

  @return EFI_SUCCESS        Get the file name successfully
  @return EFI_ABORTED        An error occurred.
**/
static
EFI_STATUS
ParseInFile (
  IN CHAR8    *FdName,
  IN FILETYPE Type
)
{
  FILE      *Fptr;

  Fptr  = NULL;

  if ((Type == INFD) && ((FdName == NULL) || (Fptr = fopen (FdName, "r")) == NULL)) {
    if (FdName != NULL) {
      printf ("Error: The <infd> file doesn't exist '%s'\n", FdName);
    }
    return EFI_ABORTED;
  }
  if ((Type == OUTFD) && (FdName == NULL) ) {
    printf ("Error: The <Outfd> name is NULL.\n");
    return EFI_ABORTED;
  }
  if ((Type == SETUPTXT) && (FdName == NULL) ) {
    printf ("Error: The <script> name is NULL.\n");
    return EFI_ABORTED;
  }
  if (strlen (FdName) > MAX_FILENAME_LEN - 1) {
    printf ("Error: The <fd> name is too long.\n");
    if (Fptr != NULL) {
      fclose (Fptr);
    }
    return EFI_ABORTED;
  }
  //
  // Get and copy the file name
  //
  if (Type == INFD) {
    strncpy (mInputFdName, FdName, MAX_FILENAME_LEN - 1);
    mInputFdName[MAX_FILENAME_LEN - 1] = 0;
  }
  if (Type == OUTFD) {
    strncpy (mOutputFdName, FdName, MAX_FILENAME_LEN - 1);
    mOutputFdName[MAX_FILENAME_LEN - 1] = 0;
  }
  if (Type == OUTTXT) {
    strncpy (mOutTxtName, FdName, MAX_FILENAME_LEN - 1);
    mOutTxtName[MAX_FILENAME_LEN - 1] = 0;
  }
  if (Type == SETUPTXT) {
    strncpy (mSetupTxtName, FdName, MAX_FILENAME_LEN - 1);
    mSetupTxtName[MAX_FILENAME_LEN - 1] = 0;
  }
  if (Fptr != NULL) {
    fclose (Fptr);
  }
  return EFI_SUCCESS;
}
/**
  Print the usage of this tools.

  @return NULL
**/
static
VOID
Usage (
  VOID
  )
{
  //
  // Print utility header
  //
  printf ("\nIntel(R) Firmware Configuration Editor. (Intel(R) %s) Version %d.%d. %s.\n\n",
    UTILITY_NAME,
    UTILITY_MAJOR_VERSION,
    UTILITY_MINOR_VERSION,
  __BUILD_VERSION
    );
  //
  // Copyright declaration
  //
  fprintf (stdout, "Copyright (c) 2010-2019, Intel Corporation. All rights reserved.\n\n");
  //
  // Summary usage
  //
  fprintf (stdout, "The tool enables you to retrieve and change HII configuration (Setup) data in \n");
  fprintf (stdout, "Firmware Device files.\n");
  fprintf (stdout, "\nUsage: \n");
  fprintf (stdout, "    FCE  [read    -i <infd>  [<PlatformId UQI>] ['>' <script>]] \n");
  fprintf (stdout, "    FCE  [update  -i <infd>  [<PlatformId UQI>|-s <script>] -o <outfd>\n");
  fprintf (stdout, "               [--remove|--ignore] [-g <FvNameGuid>]] [-a]\n");
  fprintf (stdout, "    FCE  [verify  -i <infd>  -s <script>] \n");
  fprintf (stdout, "    FCE  [updateq -i <infd>  -o <outfd> <UQI> <Question Type> <Value>] \n");
  fprintf (stdout, "    FCE  [[help] | [-?]] \n");
  fprintf (stdout, "\n");

  fprintf (stdout, "Options:\n");
  fprintf (stdout, "    read             Extract the HII data from the <infd> file. \n");
  fprintf (stdout, "    update           Update the HII data to the <outfd> file. \n");
  fprintf (stdout, "    verify           Verify the current platform configuration. \n");
  fprintf (stdout, "    updateq          Update the current platform configuration by command line.\n");
  fprintf (stdout, "                     updateq only supports common mode.\n");
  fprintf (stdout, "    help             Display the help information.\n");

  fprintf (stdout, "    <infd>           The name of a existing Firmware Device binary file input. \n");
  fprintf (stdout, "    <PlatformId UQI> The UQI is required in multi-platform mode to represent a \n");
  fprintf (stdout, "                     PlatformId question from the VFR files used during binary \n");
  fprintf (stdout, "                     image creation. It must not be used for common mode. \n");
  fprintf (stdout, "    <outfd>          The name of a Firmware Device binary file output. \n");
  fprintf (stdout, "    <script>         The name of a configure scripts.\n");
  fprintf (stdout, "    <UQI>            A hex number followed by an array of hex numbers. \n");
  fprintf (stdout, "    <Question Type>  One of ORDERED_LIST, ONE_OF, NUMERIC, STRING or CHECKBOX. \n");
  fprintf (stdout, "    <Value>          A single hex number, if the <Question Type> is ONE_OF,\n");
  fprintf (stdout, "                     NUMERIC, or CHECKBOX. Or a single hex number containing \n");
  fprintf (stdout, "                     the array size followed by an array of that length of hex\n");
  fprintf (stdout, "                     numbers, if the <Question Type> is ORDERED_LIST. \n");
  fprintf (stdout, "    <FvNameGuid>     GuidValue is one specific FvName guid value.\n");
  fprintf (stdout, "                     Its format is xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx.\n");
  fprintf (stdout, "    -i               Import an existing FD file <infd>. \n");
  fprintf (stdout, "    -o               Create the new FD with the changes made. \n");
  fprintf (stdout, "    -s               Import config scripts. \n");
  fprintf (stdout, "     >               Redirect the output to a scripts. \n");
  fprintf (stdout, "    -?               Display the help information. \n");
  fprintf (stdout, "    --remove         If one or more of the settings that are updated also \n");
  fprintf (stdout, "                     exists in NV RAM, remove them only in multi-platform mode.\n");
  fprintf (stdout, "    --ignore         If one or more of the settings that are updated also \n");
  fprintf (stdout, "                     existsin NV RAM, ignore them only in multi-platform mode.\n");
  fprintf (stdout, "    -g               Specify the FV image to store the multi-platform default \n");
  fprintf (stdout, "                     setting. If it is missing, the multi-platform default \n");
  fprintf (stdout, "                     will be inserted into BFV image.\n");
  fprintf (stdout, "    -a               Specify this tool to choose the smaller size between two \n");
  fprintf (stdout, "                     different storage formats in NV RAM. It's only vaild in \n");
  fprintf (stdout, "                     multi-platform mode. \n");
  fprintf (stdout, "\n");

  fprintf (stdout, "Mode:\n");
  fprintf (stdout, "    Common           Extract the HII data from IFR binary and update it to the \n");
  fprintf (stdout, "                     EFI variable. \n");
  fprintf (stdout, "    Multi-platform   The default value depends on the PlatformId and DefaultId \n");
  fprintf (stdout, "                     described in the VFR files. This tool will create the \n");
  fprintf (stdout, "                     binary file to store default settings at build time for \n");
  fprintf (stdout, "                     different platforms and modes adding all default settings \n");
  fprintf (stdout, "                     into BFV as one FFS.\n");

  fprintf (stdout, "\n");
}

/**
  Parse the command line parameters

  @param   argc                      The pointer to number of input parameters.
  @param   *argv[]                   The pointer to the parameters.

  @retval  EFI_SUCCESS               Parse the parameters successfully.
  @retval  EFI_INVALID_PARAMETER     An error occurred.

**/
static
EFI_STATUS
ParseCommmadLine (
  IN     UINTN          argc,
  IN     CHAR8          *argv[]
  )
{
  EFI_STATUS     Status;
  UINT8          Index;
  UINT8          IndexParamI;
  UINT8          IndexParamS;
  UINT8          IndexParamO;
  UINT8          IndexParamRemove;
  UINT8          IndexParamIgnore;
  UINT8          IndexParamOptimize;
  EFI_GUID       FvNameGuid;

  Status      = EFI_SUCCESS;
  Index       = 0;
  IndexParamI = 0;
  IndexParamO = 0;
  IndexParamS = 0;
  IndexParamRemove = 0;
  IndexParamIgnore = 0;
  IndexParamOptimize                     = 0;
  mMultiPlatformParam.SizeOptimizedParam = FALSE;
  //
  // Check for only own one operations
  //
  if (argc == 0) {
    Usage ();
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }
  if (
    argc == 1                           \
    && ((stricmp(argv[0], "read") == 0)   \
    || (stricmp(argv[0], "update") == 0)  \
    || (stricmp(argv[0], "updateq") == 0) \
    || (stricmp(argv[0], "verify") == 0) )
    ) {
      printf ("Error:  Some parameters have been lost. Please correct. \n");
      Usage ();
      Status = EFI_INVALID_PARAMETER;
      goto Done;
   }

  while (argc > 0) {
    if (stricmp(argv[0], "read") == 0) {
      Operations = READ;
      argc--;
      argv++;

      if (argc < 2) {
        printf ("Error. The correct command is 'FCE read -i <infd>'. \n");
        Usage ();
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }
      //
      // Get input FD file name.
      //
      if (stricmp (argv[0], "-i") == 0) {
        Status = ParseInFile (argv[1], INFD);
        if (EFI_ERROR (Status)) {
          goto Done;
        }
        argc -= 2;
        argv += 2;
      }
     //
     // Parse the QUI parameters
     //
     if (argc > 2) {
       Status = ParseFirstpartOfUqi (&argv, &(mMultiPlatformParam.Uqi.HexNum), &(mMultiPlatformParam.Uqi.Data));
       mMultiPlatformParam.MultiPlatformOrNot = TRUE;
       if (EFI_ERROR (Status)) {
         goto Done;
       }
     }
     break;

    } else if (stricmp(argv[0], "update") == 0) {
      //
      // Update the config file to Fd
      //
      Operations = UPDATE;
      argc--;
      argv++;

      if (argc < 4) {
        printf ("Error. The correct command is 'FCE update -i <infd> [<PlatformId UQI>|-s <script>] -o <outfd>' \n");
        Usage ();
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }

      continue;
    } else if (stricmp(argv[0], "verify") == 0) {
      //
      // 3. Parse the command line "FCE verify -i <infd> -s <script>"
      //
      Operations = VERIFY;
      argc--;
      argv++;

      if (argc < 4) {
        printf ("Error. The correct command is 'FCE verify -i <infd> -s <script>'\n");
        Usage ();
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }

      continue;
    } else if (stricmp(argv[0], "updateq") == 0) {
      //
      // Parse the command line "FCE updateq -i <infd> -o<outfd> <UQI> <Question Type> <Value>"
      //
      argc--;
      argv++;

      //
      // Get input/output FD file name.
      //
      Index = 2;
      while ((Index > 0) && (argc > 1)) {
        if (stricmp (argv[0], "-i") == 0) {
          Status = ParseInFile (argv[1], INFD);
          if (EFI_ERROR (Status)) {
            goto Done;
          }
          argc -= 2;
          argv += 2;
          Index--;
          IndexParamI++;
          continue;
        }

        if (stricmp (argv[0], "-o") == 0) {
          Status = ParseInFile (argv[1], OUTFD);
          if (EFI_ERROR (Status)) {
            goto Done;
          }
          argc -= 2;
          argv += 2;
          Index--;
          IndexParamO++;
          continue;
        }
        if (
         (argc >= 1) \
         && (stricmp(argv[0], "-o") != 0) \
         && (stricmp(argv[0], "-i") != 0)
          ) {
          printf ("Error. The correct command is 'FCE updateq -i <infd> -o <outfd> <UQI> <Question Type> <Value>' \n");
          Usage();
          Status = EFI_INVALID_PARAMETER;
          goto Done;
        }
      }

      if (Index != 0) {
        printf ("Error. The correct command is 'FCE updateq -i <infd> -o <outfd> <UQI> <Question Type> <Value>' \n");
        Usage ();
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }

      //
      // Parse the QUI parameters
      //
      Status = ParseUqiParam ((UINT32 *)&argc,&argv);
      if (EFI_ERROR (Status)) {
        goto Done;
      }
      Operations = UPDATEQ;
      break;
    }

    if (stricmp (argv[0], "-i") == 0) {
      Status = ParseInFile (argv[1], INFD);
      if (EFI_ERROR (Status)) {
        goto Done;
      }
      argc -= 2;
      argv += 2;
      IndexParamI++;
      continue;
    }

    if (stricmp (argv[0], "-o") == 0) {
      Status = ParseInFile (argv[1], OUTFD);
      if (EFI_ERROR (Status)) {
        goto Done;
      }
      argc -= 2;
      argv += 2;
      IndexParamO++;
      continue;
    }

    if (stricmp (argv[0], "-s") == 0) {
      Status = ParseInFile (argv[1], SETUPTXT);
      if (EFI_ERROR (Status)) {
        goto Done;
      }
      argc -= 2;
      argv += 2;
      IndexParamS++;
      continue;
    }

    if (stricmp (argv[0], "-g") == 0) {
      Status = StringToGuid (argv[1], &FvNameGuid);
      if (EFI_ERROR (Status)) {
        printf ("Error: Invalid parameters for -g option in command line. \n");
        Usage();
        goto Done;
      }
      mFvNameGuidString = argv[1];
      argc -= 2;
      argv += 2;
      continue;
    }

    if (stricmp (argv[0], "-a") == 0) {
      argc -= 1;
      argv += 1;
      IndexParamOptimize++;
      mMultiPlatformParam.SizeOptimizedParam = TRUE;
      continue;
    }

    if (stricmp (argv[0], "--remove") == 0) {
      argc -= 1;
      argv += 1;
      IndexParamRemove++;
      Operations = UPDATE_REMOVE;
      continue;
    }

    if (stricmp (argv[0], "--ignore") == 0) {
      argc -= 1;
      argv += 1;
      IndexParamIgnore++;
      Operations = UPDATE_IGNORE;
      continue;
    }

    if ((stricmp(argv[0], "help") == 0) || (stricmp(argv[0], "-?") == 0)) {

      Usage();
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }
    //
    // Operations should not be none
    //
    if ( Operations == NONE ) {
      printf ("Error. Only support read/update/verify/updateq mode. \n");
      Usage();
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    if (Operations == UPDATE) {
      Status = ParseFirstpartOfUqi (&argv, &(mMultiPlatformParam.Uqi.HexNum), &(mMultiPlatformParam.Uqi.Data));
      if (!EFI_ERROR (Status)) {
        mMultiPlatformParam.MultiPlatformOrNot = TRUE;
        argc = argc - mMultiPlatformParam.Uqi.HexNum - 1;
        argv = argv + mMultiPlatformParam.Uqi.HexNum + 1;
        continue;
      }
    }

    if (
      (argc >= 1) \
      && (stricmp(argv[0], "-?") != 0)
      && (stricmp(argv[0], "help") != 0)
      && (stricmp(argv[0], "verify") != 0)
      && (stricmp(argv[0], "read") != 0)
      && (stricmp(argv[0], "update") != 0)
      && (stricmp(argv[0], "updateq") != 0)
      && (stricmp(argv[0], "-o") != 0)
      && (stricmp(argv[0], "-i") != 0)
      && (stricmp(argv[0], "-s") != 0)
      && (stricmp(argv[0], "-a") != 0)
      && (stricmp(argv[0], "-g") != 0)
      && (stricmp(argv[0], "--remove") != 0)
      && (stricmp(argv[0], "--ignore") != 0)
      ) {
      printf ("Error: Invalid parameters exist in command line. \n");
      Usage();
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }
  }
  //
  // Check the repeated parameters in command line, such as "-i -i"
  //
  if (
    (IndexParamI > 1)
    || (IndexParamO > 1)
    || (IndexParamS > 1)
    || (IndexParamOptimize > 1)
    || (IndexParamRemove > 1)
    || (IndexParamIgnore > 1)
    ) {
    printf ("Error:  Redundant parameters exist in command line.\n");
    Usage();
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }
  //
  // Check improper parameters in command line, such as "FCE read -i -s"
  //
  if (
    ((Operations == READ) && ((IndexParamO >= 1) || (IndexParamS >= 1) || (IndexParamRemove >= 1) || (IndexParamIgnore >= 1))) \
    || ((Operations == VERIFY) && ((IndexParamO >= 1) || (IndexParamRemove >= 1) || (IndexParamIgnore >= 1))) \
    || ((Operations == UPDATEQ) && ((IndexParamS >= 1) || (IndexParamRemove >= 1) || (IndexParamIgnore >= 1))) \
    || ((Operations == UPDATE) && ((IndexParamRemove >= 1) && (IndexParamIgnore >= 1)))
    || ((Operations != UPDATE && Operations != UPDATE_REMOVE && Operations != UPDATE_IGNORE) && ((IndexParamOptimize >= 1) || (mFvNameGuidString != NULL)))
   ) {
    printf ("Error:  Improper parameters exist in command line. \n");
    Usage();
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }
Done:
  return Status;
}

/**
  Check whether exists the valid variables in NvStorage or not.

  @retval TRUE      If existed, return TRUE.
  @retval FALSE     Others
**/
static
BOOLEAN
ExistEfiVarOrNot (
  IN  LIST_ENTRY  *StorageListHead
  )
{
   EFI_FIRMWARE_VOLUME_HEADER   *VarAddr;
   BOOLEAN                      Existed;
   VOID                         *VariableStoreHeader;
   BOOLEAN                      AuthencitatedMonotonicOrNot;
   BOOLEAN                      AuthencitatedBasedTimeOrNot;

   VarAddr                     = (EFI_FIRMWARE_VOLUME_HEADER   *) gEfiFdInfo.EfiVariableAddr;
   VariableStoreHeader         = (VOID *)((CHAR8 *)VarAddr + VarAddr->HeaderLength);
   AuthencitatedMonotonicOrNot = FALSE;
   AuthencitatedBasedTimeOrNot = FALSE;
   Existed                     = TRUE;
   //
   // Judge the layout of NV by gEfiVariableGuid
   //
   AuthencitatedMonotonicOrNot  = CheckMonotonicBasedVarStore (VariableStoreHeader);
   AuthencitatedBasedTimeOrNot  = CheckTimeBasedVarStoreOrNot (VariableStoreHeader);

   if (AuthencitatedMonotonicOrNot) {
     //
     // Check the Monotonic based authenticate variable
     //
     Existed = ExistMonotonicBasedEfiVarOrNot (StorageListHead);
   } else if (AuthencitatedBasedTimeOrNot){
     //
     // Check the Time Sample authenticate variable
     //
     Existed = ExistTimeBasedEfiVarOrNot (StorageListHead);
   } else {
     //
     // Check the normal variable
     //
     Existed = ExistNormalEfiVarOrNot (StorageListHead);
   }

   return Existed;
}

/**
  Exchange the data between Efi variable and the data of VarList

  If VarToList is TRUE, copy the efi variable data to the VarList; Otherwise,
  update the data from varlist to efi variable.

  @param VarToList         The flag to control the direction of exchange.
  @param StorageListHead   Decide which variale list be updated

  @retval EFI_SUCCESS      Get the address successfully.
**/
static
EFI_STATUS
EfiVarAndListExchange (
  IN BOOLEAN      VarToList,
  IN  LIST_ENTRY  *StorageListHead
  )
{
   EFI_FIRMWARE_VOLUME_HEADER   *VarAddr;
   EFI_STATUS                   Status;
   VOID                         *VariableStoreHeader;
   BOOLEAN                      AuthencitatedMonotonicOrNot;
   BOOLEAN                      AuthencitatedBasedTimeOrNot;

   Status                      = EFI_ABORTED;
   VarAddr                     = (EFI_FIRMWARE_VOLUME_HEADER   *) gEfiFdInfo.EfiVariableAddr;
   VariableStoreHeader         = (VOID *)((CHAR8 *)VarAddr + VarAddr->HeaderLength);
   AuthencitatedMonotonicOrNot = FALSE;
   AuthencitatedBasedTimeOrNot = FALSE;
   //
   // Judge the layout of NV by gEfiVariableGuid
   //
   AuthencitatedMonotonicOrNot  = CheckMonotonicBasedVarStore (VariableStoreHeader);
   AuthencitatedBasedTimeOrNot  = CheckTimeBasedVarStoreOrNot (VariableStoreHeader);

   if (AuthencitatedMonotonicOrNot) {
     //
     // Update the Monotonic based authenticate variable
     //
     Status = SynAuthEfiVariable (VarToList, StorageListHead);
   } else if (AuthencitatedBasedTimeOrNot){
     //
     // Update the Time Sample authenticate variable
     //
     Status = SynAuthEfiVariableBasedTime (VarToList, StorageListHead);
   } else {
     //
     // Update the normal variable
     //
     Status = SynEfiVariable (VarToList, StorageListHead);
   }

   return Status;
}

/**
  Check the layout of NvStorage and remove the variable from Efi variable

  Found the variable with the same name in StorageListHead and remove it.

  @param StorageListHead   Decide which variale list be removed.

  @retval EFI_SUCCESS      Remove the variables successfully.
**/
static
EFI_STATUS
RemoveEfiVar (
  IN  LIST_ENTRY  *StorageListHead
  )
{
   EFI_FIRMWARE_VOLUME_HEADER   *VarAddr;
   EFI_STATUS                   Status;
   VOID                         *VariableStoreHeader;
   BOOLEAN                      AuthencitatedMonotonicOrNot;
   BOOLEAN                      AuthencitatedBasedTimeOrNot;

   Status                      = EFI_ABORTED;
   VarAddr                     = (EFI_FIRMWARE_VOLUME_HEADER   *) gEfiFdInfo.EfiVariableAddr;
   VariableStoreHeader         = (VOID *)((CHAR8 *)VarAddr + VarAddr->HeaderLength);
   AuthencitatedMonotonicOrNot = FALSE;
   AuthencitatedBasedTimeOrNot = FALSE;
   //
   // Judge the layout of NV by gEfiVariableGuid
   //
   AuthencitatedMonotonicOrNot  = CheckMonotonicBasedVarStore (VariableStoreHeader);
   AuthencitatedBasedTimeOrNot  = CheckTimeBasedVarStoreOrNot (VariableStoreHeader);

   if (AuthencitatedMonotonicOrNot) {
     //
     // Update the Monotonic based authenticate variable
     //
     Status = RemoveAuthEfiVariable (StorageListHead);
   } else if (AuthencitatedBasedTimeOrNot){
     //
     // Update the Time Sample authenticate variable
     //
     Status = RemoveAuthEfiVariableBasedTime (StorageListHead);
   } else {
     //
     // Update the normal variable
     //
     Status = RemoveNormalEfiVariable (StorageListHead);
   }

   return Status;
}

/**
  Parse the all formset in one VfrBin.

  Parse all questions, variables and expression in VfrBin, and store
  it in Formset and Form.

  @param BaseAddr           The pointer to the array of VfrBin base address.
  @param UniBinBaseAddress  The pointer to one Uni string base address.

  @retval EFI_SUCCESS       The Search was complete successfully
  @return EFI_ABORTED       An error occurred
**/
static
EFI_STATUS
ParseFormSetInVfrBin (
  IN  UINTN     **BaseAddr,
  IN  UINTN     *UniBinBaseAddress,
  IN  UINT8     Index
  )
{
  UINT32                PackageLength;
  EFI_STATUS            Status;
  FORM_BROWSER_FORMSET  *FormSet;
  UINT8                 *IfrBinaryData;
  EFI_IFR_OP_HEADER     *IfrOpHdr;
  UINT32                IfrOffset;

  PackageLength = 0;
  Status        = EFI_SUCCESS;
  FormSet       = NULL;
  IfrBinaryData = NULL;
  IfrOpHdr      = NULL;

  //
  // The first 4 Bytes of VfrBin is used to record the Array Length
  // The header of VfrBin is, ARRAY LENGTH:4 Bytes, PACKAGE HEADER:4 Bytes (2 Bytes for Len, and the other for Type)
  //
  PackageLength = *(UINT32 *)*(BaseAddr + Index) - 4;
  IfrBinaryData   = (UINT8 *)*(BaseAddr + Index) + 4;

  //
  // Go through the form package to parse OpCode one by one.
  //
  IfrOffset       = sizeof (EFI_HII_PACKAGE_HEADER);

  while (IfrOffset < PackageLength) {
    //
    // Search the Formset in VfrBin
    //
    IfrOpHdr  = (EFI_IFR_OP_HEADER *) (IfrBinaryData + IfrOffset);

    if (IfrOpHdr->OpCode == EFI_IFR_FORM_SET_OP) {
      FormSet = calloc (sizeof (FORM_BROWSER_FORMSET), sizeof (CHAR8));
      if (FormSet == NULL) {
        return EFI_ABORTED;
      }
      FormSet->IfrBinaryData   = IfrBinaryData + IfrOffset;
      FormSet->UnicodeBinary   = (UINT8 *) UniBinBaseAddress;
      //
      //This length will be corrected in ParseOpCodes function.
      //
      FormSet->IfrBinaryLength = PackageLength - IfrOffset;
      //
      // Parse opcodes in the formset IFR binary.
      //
      Status = ParseOpCodes (FormSet);
      if (EFI_ERROR (Status)) {
        DestroyAllStorage (FormSet->StorageListHead);
        DestroyFormSet (FormSet);
        return EFI_ABORTED;
      }
      IfrOffset += FormSet->IfrBinaryLength;
      FormSet->EnUsStringList.StringInfoList = calloc (sizeof (STRING_INFO), STRING_NUMBER);
      FormSet->EnUsStringList.CachedIdNum = 0;
      FormSet->EnUsStringList.MaxIdNum = STRING_NUMBER;
      FormSet->UqiStringList.StringInfoList = calloc (sizeof (STRING_INFO), STRING_NUMBER);
      FormSet->UqiStringList.CachedIdNum = 0;
      FormSet->UqiStringList.MaxIdNum = STRING_NUMBER;
      //
      // Insert the FormSet to mFormSet
      //
      InsertTailList (&mFormSetListEntry, &FormSet->Link);
    } else {
      IfrOffset += IfrOpHdr->Length;
    }
  }

  return EFI_SUCCESS;
}

/**
  Store all defaultId to mMultiPlatformParam.

  The mMultiPlatformParam.DefaultId[0] is used to store standard default.

  @param DefaultId      The current defaultID.

  @retval EFI_SUCCESS   It was complete successfully
  @return EFI_ABORTED   An error occurred
**/
VOID
StoreAllDefaultId (
  IN  UINT16   DefaultId
)
{
  UINT32            Index;

  if ((mMultiPlatformParam.DefaultIdNum == 0) && (DefaultId != 0)) {
      mMultiPlatformParam.DefaultId[0] = DefaultId;
      mMultiPlatformParam.DefaultIdNum++;
      return;
  }
  //
  // Only store the different value to mMultiPlatformParam.DefaultId[1] - mMultiPlatformParam.DefaultId[n]
  //
  for (Index = 0; Index < mMultiPlatformParam.DefaultIdNum; Index++) {
    if (mMultiPlatformParam.DefaultId[Index] == DefaultId) {
      return;
    }
  }
  mMultiPlatformParam.DefaultId[Index] = DefaultId;
  mMultiPlatformParam.DefaultIdNum++;
}

/**
  Read all defaultId and platformId from binary.

  @param  Binary        The pointer to the bianry
  @param  Storage       The pointer to the Storage
**/
VOID
ReadDefaultAndPlatformIdFromBfv (
  IN  UINT8             *Binary,
  IN  FORMSET_STORAGE   *Storage
)
{
  UINT16   Length;
  UINT16   Size;
  UINT32   Index;

  Length = *(UINT16 *)Binary - sizeof (UINT16);
  Index  = 0;
  Size   = 0;

  Binary = Binary + sizeof (UINT16);

  for (Size = 0; Size < Length; Size += sizeof (UINT16) + mMultiPlatformParam.PlatformIdWidth, Index++) {
    Storage->DefaultId[Index] = *(UINT16 *)(Binary + Size);
    memcpy (&Storage->PlatformId[Index], (Binary + Size + sizeof (UINT16)), mMultiPlatformParam.PlatformIdWidth);
  }
  Storage->DefaultPlatformIdNum = Index - 1;
}

/**
  Store all defaultId and platformId to binary.

  @param  Binary        The pointer to the bianry
  @param  Storage       The pointer to the Storage

  @retval Length        Return the length of the header
**/
UINT32
WriteDefaultAndPlatformId (
  IN  UINT8             *Binary,
  IN  FORMSET_STORAGE   *Storage
)
{
  UINT16   Length;
  UINT32   Index;
  UINT8    *Buffer;

  Length = 0;
  Buffer = Binary;

  Buffer = Buffer + sizeof (CHAR16);

  for (Index = 0; Index <= Storage->DefaultPlatformIdNum; Index++) {
    *(UINT16 *)Buffer = Storage->DefaultId[Index];
    Buffer = Buffer + sizeof (CHAR16);
    memcpy (Buffer, &Storage->PlatformId[Index], mMultiPlatformParam.PlatformIdWidth);
    Buffer = Buffer + mMultiPlatformParam.PlatformIdWidth;
  }
  Length = (UINT16) (Buffer - Binary);
  //
  // Record the offset to the first two bytes
  //
  *(UINT16 *)Binary = Length;

  return Length;
}

/**
  Store all defaultId and platformId to binary.

  @param  Binary        The pointer to the bianry
  @param  Storage       The pointer to the Storage

  @retval Length        Return the length of the header
**/
UINT32
WriteNvStoreDefaultAndPlatformId (
  IN  UINT8             *Binary,
  IN  FORMSET_STORAGE   *Storage
)
{
  UINT16   Length;
  UINT32   Index;
  UINT8    *Buffer;

  Length = 0;
  Buffer = Binary + 8;

  for (Index = 0; Index <= Storage->DefaultPlatformIdNum; Index++) {
    *(UINT64 *)Buffer = Storage->PlatformId[Index];
    Buffer = Buffer + sizeof(UINT64);
    *(UINT16 *)Buffer = Storage->DefaultId[Index];
    Buffer = Buffer + sizeof(UINT16);
    // for Resered
    Buffer = Buffer + 6;
  }
  Length = (UINT16) (Buffer - Binary - 8);
  // for Header size
  Length += 4;
  return Length;
}

/**
  Read the platformId from questions.

  @retval EFI_SUCCESS   It was complete successfully.
  @return EFI_ABORTED   An error occurred.
**/
EFI_STATUS
ReadPlaformId (
  IN  FORM_BROWSER_STATEMENT  *CurQuestion
)
{
  UINT16           Index;
  UINT64           IdValue;
  LIST_ENTRY       *Link;
  QUESTION_OPTION  *Option;
  UINT64           Step;

  Index            = 0;
  IdValue          = 0;
  Step             = 0;
  //
  // Check whether it is the question of paltform Id
  //
  if (!CompareUqiHeader (&(CurQuestion->Uqi), &(mMultiPlatformParam.Uqi))) {
    return EFI_ABORTED;
  }
  //
  // Copy the Question with platform to mMultiPlatformParam
  //
  memcpy (&mMultiPlatformParam.PlatformIdQuestion, CurQuestion, sizeof (FORM_BROWSER_STATEMENT));
  mMultiPlatformParam.Question = CurQuestion;
  //
  // Pick up the value of NUMERIC and ONE_OF from current question and fill it in mMultiPlatformParam
  //
  mMultiPlatformParam.PlatformIdWidth = CurQuestion->StorageWidth;

  if (CurQuestion->Operand == EFI_IFR_NUMERIC_OP) {
    Index = 0;
    if (CurQuestion->Step == 0) {
      Step  = 1;
    } else {
      Step  = CurQuestion->Step;
    }
    for (IdValue = CurQuestion->Minimum; IdValue < CurQuestion->Maximum; IdValue += Step) {
      mMultiPlatformParam.PlatformId[Index++] = (UINT64)IdValue;
    }
  }

  if (CurQuestion->Operand == EFI_IFR_ONE_OF_OP) {
    Index = 0;

    Link = GetFirstNode (&CurQuestion->OptionListHead);
    while (!IsNull (&CurQuestion->OptionListHead, Link)) {
      Option = QUESTION_OPTION_FROM_LINK (Link);
      mMultiPlatformParam.PlatformId[Index++] = Option->Value.Value.u64;
      Link = GetNextNode (&CurQuestion->OptionListHead, Link);
    }
  }

  if (Index >= MAX_PLATFORM_DEFAULT_ID_NUM) {
    return EFI_ABORTED;
  }
  mMultiPlatformParam.PlatformIdNum = Index;

  return EFI_SUCCESS;
}

/**
  Clear the buffer of Storage list.

  @param StorageListHead The pointer to the entry of the storage list
  @param DefaultId       The default Id for multi-platform support
  @param PlatformId      The platform Id for multi-platform support

  @retval NULL
**/
VOID
ClearStorageEntryList (
  IN  LIST_ENTRY   *StorageListHead,
  IN  UINT16       DefaultId,
  IN  UINT64       PlatformId
  )
{

  LIST_ENTRY        *StorageLink;
  FORMSET_STORAGE   *Storage;

  Storage = NULL;

  StorageLink = GetFirstNode (StorageListHead);

  while (!IsNull (StorageListHead, StorageLink)) {
    Storage = FORMSET_STORAGE_FROM_LINK (StorageLink);
    if (Storage != NULL) {
      memset (Storage->Buffer, 0x0, Storage->Size);
      Storage->DefaultId[0]   = DefaultId;
      Storage->PlatformId[0] = PlatformId;
      Storage->DefaultPlatformIdNum = 0;
    }
    StorageLink = GetNextNode (StorageListHead, StorageLink);
  }
}

/**
  Append the platformid and default to the variables of CurDefaultId and CurPlatformId

  @param  StorageListHead     The pointer to the storage list
  @param  CurDefaultId        The default Id for multi-platform mode
  @param  CurPlatformId       The platform Id for multi-platform mode
  @param  DefaultId           The default Id for multi-platform mode
  @param  PlatformId          The platform Id for multi-platform mode

  @retval NULL
**/
VOID
AppendIdToVariables (
  IN  LIST_ENTRY   *StorageListHead,
  IN  UINT16       CurDefaultId,
  IN  UINT64       CurPlatformId,
  IN  UINT16       DefaultId,
  IN  UINT64       PlatformId
)
{
  LIST_ENTRY        *StorageLink;
  FORMSET_STORAGE   *Storage;

  Storage          = NULL;

  StorageLink = GetFirstNode (StorageListHead);

  while (!IsNull (StorageListHead, StorageLink)) {
    Storage = FORMSET_STORAGE_FROM_LINK (StorageLink);

    if ((Storage->DefaultId[0] == CurDefaultId)
      && (Storage->PlatformId[0] == CurPlatformId)
      ) {
      ++Storage->DefaultPlatformIdNum;
      Storage->DefaultId[Storage->DefaultPlatformIdNum]  = DefaultId;
      Storage->PlatformId[Storage->DefaultPlatformIdNum] = PlatformId;
    }

    StorageLink = GetNextNode (StorageListHead, StorageLink);
  }
}


/**
  Check whether StorageListHead2 is included in StorageListHead1

  @param  StorageListHead1    The pointer to the entry of storage list
  @param  StorageListHead2    The pointer to the entry of storage list
  @param  DefaultId           The default Id for multi-platform mode
  @param  PlatformId          The platform Id for multi-platform mode

  @retval TRUE        Totally included.
  @return FALSE       Other cases.
**/
BOOLEAN
ComparePartSameVariableList (
  IN   LIST_ENTRY   *StorageListHead1,
  IN   LIST_ENTRY   *StorageListHead2,
  OUT  UINT16       *DefaultId,
  OUT  UINT64       *PlatformId
)
{
  LIST_ENTRY        *StorageLink;
  LIST_ENTRY        *TempStorageHead;
  LIST_ENTRY        *TempStorageLink;
  LIST_ENTRY        *TempStorageHead2;
  LIST_ENTRY        *TempStorageLink2;
  FORMSET_STORAGE   *Storage;
  FORMSET_STORAGE   *TempStorage;
  FORMSET_STORAGE   *TempStorage2;
  UINT16            CurDefaultId;
  UINT64            CurPlatformId;
  UINT32            VariableNum;
  UINT32            Index;

  StorageLink      = NULL;
  TempStorageHead  = NULL;
  TempStorageLink  = NULL;
  TempStorageHead2 = NULL;
  TempStorageLink2 = NULL;
  Storage          = NULL;
  TempStorage      = NULL;
  TempStorage2     = NULL;
  CurDefaultId     = 0;
  CurPlatformId    = 0;
  VariableNum      = 0;
  Index            = 0;
  TempStorageHead  = StorageListHead1;


  //
  // Get the number of variables in StorageListHead2;
  //
  StorageLink = GetFirstNode (StorageListHead2);

  while (!IsNull (StorageListHead2, StorageLink)) {
    Storage = FORMSET_STORAGE_FROM_LINK (StorageLink);
    //
    // For multi-platform support, only need to calcuate the type of EFI_IFR_VARSTORE_EFI_OP.
    //
    if (mMultiPlatformParam.MultiPlatformOrNot &&
        (Storage->Type == EFI_IFR_VARSTORE_EFI_OP) && (Storage->Name != NULL) && (Storage->NewEfiVarstore) &&
        ((Storage->Attributes & EFI_VARIABLE_NON_VOLATILE) == EFI_VARIABLE_NON_VOLATILE)) {
      VariableNum++;
    }
    StorageLink = GetNextNode (StorageListHead2, StorageLink);
  }
  //
  // Parse every variables in StorageListHead1 and compare with ones in StorageListHead2
  // Only all variables in StorageListHead2 are included in StorageListHead1, return TRUE.
  //
  StorageLink = GetFirstNode (StorageListHead1);

  while (!IsNull (StorageListHead1, StorageLink)) {
    Storage = FORMSET_STORAGE_FROM_LINK (StorageLink);
    //
    // Get specified DefaultId and PlatformId firstly
    //
    CurDefaultId  = Storage->DefaultId[0];
    CurPlatformId = Storage->PlatformId[0];
    Index         = 0;
    //
    // Compare all variables under same defaultId and platformId
    //
    TempStorageLink = GetFirstNode (TempStorageHead);
    while (!IsNull (TempStorageHead, TempStorageLink)) {
      TempStorage = FORMSET_STORAGE_FROM_LINK (TempStorageLink);
      if ((TempStorage->DefaultId[0] == CurDefaultId)
        && (TempStorage->PlatformId[0] == CurPlatformId)
        && (TempStorage->Name != NULL)
        && (TempStorage->Type == EFI_IFR_VARSTORE_EFI_OP)
        && (TempStorage->NewEfiVarstore)
        && ((TempStorage->Attributes & EFI_VARIABLE_NON_VOLATILE) == EFI_VARIABLE_NON_VOLATILE)
        ) {
        //
        //Search the matched variable by Guid and name in StorageListHead2
        //
        TempStorageHead2 = StorageListHead2;
        TempStorageLink2 = GetFirstNode (TempStorageHead2);

        while (!IsNull (TempStorageHead2, TempStorageLink2)) {
          TempStorage2     = FORMSET_STORAGE_FROM_LINK (TempStorageLink2);
          if ((TempStorage2->Name != NULL)
            && (TempStorage2->Type == EFI_IFR_VARSTORE_EFI_OP)
            && (TempStorage2->NewEfiVarstore)
            && ((TempStorage2->Attributes & EFI_VARIABLE_NON_VOLATILE) == EFI_VARIABLE_NON_VOLATILE)
            && !FceStrCmp (TempStorage->Name, TempStorage2->Name)
            && !CompareGuid(&TempStorage->Guid, &TempStorage2->Guid)
            ) {
            if (CompareMem (TempStorage->Buffer, TempStorage2->Buffer, TempStorage->Size) == 0) {
              Index++;
            }
          }
          TempStorageLink2 = GetNextNode (TempStorageHead2, TempStorageLink2);
        }
      }
      TempStorageLink = GetNextNode (TempStorageHead, TempStorageLink);
    }
    //
    // Check the matched variable number
    //
    if (Index == VariableNum) {
      *DefaultId  = CurDefaultId;
      *PlatformId = CurPlatformId;
      return TRUE;
    }
    StorageLink = GetNextNode (StorageListHead1, StorageLink);
  }
  return FALSE;
}

/**
  Check whether the defaultId and platformId mathched the variable or not

  @param  Storage             The pointer to the storage
  @param  DefaultId           The default Id for multi-platform mode
  @param  PlatformId          The platform Id for multi-platform mode

  @retval TRUE    Not Matched.
  @return FALSE   Matched any one
**/
BOOLEAN
NotEqualAnyIdOfStorage (
IN  FORMSET_STORAGE   *Storage,
IN  UINT16            DefaultId,
IN  UINT64            PlatformId
)
{
  UINT32  Index;

  for (Index = 0; Index <= Storage->DefaultPlatformIdNum; Index++) {
    if ((Storage->DefaultId[Index] == DefaultId)
      &&(Storage->PlatformId[Index] == PlatformId)
      ) {
      return FALSE;
    }
  }
  return TRUE;
}

/**
  Copy Stroage from StorageListHead2 to StorageListHead1

  @param  NewStorageListHead  The pointer to the entry of storage list
  @param  OldStorageListHead  The pointer to the entry of storage list
  @param  DefaultId           The default Id for multi-platform mode
  @param  PlatformId          The platform Id for multi-platform mode
  @param  AssignIdOrNot       If True, assign the platform Id and default Id to storage;
                              Or else, only copy the variables under the specified platform
                              Id and default to the other list.
  @param  Mode                The operation of mode

  @retval EFI_SUCCESS   It was complete successfully
  @return EFI_ABORTED   An error occurred
**/
EFI_STATUS
BuildVariableList (
  IN  LIST_ENTRY     *NewStorageListHead,
  IN  LIST_ENTRY     *OldStorageListHead,
  IN  UINT16         DefaultId,
  IN  UINT64         PlatformId,
  IN  BOOLEAN        AssignIdOrNot,
  IN  OPERATION_TYPE Mode
)
{
  LIST_ENTRY        *StorageLink;
  LIST_ENTRY        *NameValueLink;
  FORMSET_STORAGE   *Storage;
  FORMSET_STORAGE   *StorageCopy;
  IN  LIST_ENTRY    *StorageListHead1;
  IN  LIST_ENTRY    *StorageListHead2;
  NAME_VALUE_NODE   *NameValueNode;
  NAME_VALUE_NODE   *CopyNameValueNode;

  Storage          = NULL;
  NameValueNode    = NULL;
  StorageListHead1 = NewStorageListHead;
  StorageListHead2 = OldStorageListHead;

  StorageLink = GetFirstNode (StorageListHead2);

  while (!IsNull (StorageListHead2, StorageLink)) {
    Storage = FORMSET_STORAGE_FROM_LINK (StorageLink);

    if ((Storage->Type == EFI_IFR_VARSTORE_EFI_OP)
      ||(Storage->Type == EFI_IFR_VARSTORE_OP)
      ) {
      //
      // Only support EfiVarStore in Multi-Platform mode, and the attribute must be EFI_VARIABLE_NON_VOLATILE
      //
      if (mMultiPlatformParam.MultiPlatformOrNot) {
        if (Storage->Type == EFI_IFR_VARSTORE_OP) {
          StorageLink = GetNextNode (StorageListHead2, StorageLink);
          continue;
        }
        if ((Storage->Type == EFI_IFR_VARSTORE_EFI_OP)
          && Storage->NewEfiVarstore
          && ((Storage->Attributes & EFI_VARIABLE_NON_VOLATILE) == 0)
          ) {
          StorageLink = GetNextNode (StorageListHead2, StorageLink);
          continue;
        }
        if (AssignIdOrNot) {
          Storage->DefaultId[0]  = DefaultId;
          Storage->PlatformId[0] = PlatformId;
        } else {
          //
          //only copy the variables under the specified platform Id and default to the other list.
          //
          if ((Mode == VERIFY) && (NotEqualAnyIdOfStorage (Storage, DefaultId, PlatformId))) {
            StorageLink = GetNextNode (StorageListHead2, StorageLink);
            continue;
          } else if ((Mode != VERIFY) && (Storage->DefaultId[0] != DefaultId || Storage->PlatformId[0] != PlatformId) ) {
            StorageLink = GetNextNode (StorageListHead2, StorageLink);
            continue;
          }
        }
      }
      //
      // Copy Storage Node
      //
      if (Storage->Name == NULL) {
        return EFI_ABORTED;
      }
      StorageCopy     = NULL;
      StorageCopy     = calloc (sizeof (FORMSET_STORAGE), sizeof (CHAR8));
      if (StorageCopy == NULL) {
        printf ("Memory allocation failed.\n");
        return EFI_ABORTED;
      }
      memcpy (StorageCopy, Storage, sizeof (FORMSET_STORAGE));

      if (Mode == VERIFY) {
        StorageCopy->DefaultId[0]  = DefaultId;
        StorageCopy->PlatformId[0] = PlatformId;
      }
      //
      //Set the flags for sorting out the variables
      //
      StorageCopy->Skip = FALSE;

      StorageCopy->Name = NULL;
      StorageCopy->Name = calloc (FceStrLen (Storage->Name) + 1, sizeof (CHAR16));
      if (StorageCopy->Name == NULL) {
        printf ("Memory allocation failed.\n");
        free (StorageCopy);
        return EFI_ABORTED;
      }
      StrCpy (StorageCopy->Name, Storage->Name);

      StorageCopy->Buffer = NULL;
      StorageCopy->Buffer = calloc (Storage->Size, sizeof (CHAR8));
      if (StorageCopy->Buffer == NULL) {
        free (StorageCopy->Name);
        free (StorageCopy);
        printf ("Memory allocation failed.\n");
        return EFI_ABORTED;
      }
      CopyMem (StorageCopy->Buffer, Storage->Buffer, Storage->Size);
      //
      // Copy NameValue list of storage node
      //
      InitializeListHead (&StorageCopy->NameValueListHead);

      NameValueLink = GetFirstNode (&Storage->NameValueListHead);

      while (!IsNull (&Storage->NameValueListHead, NameValueLink)) {

        NameValueNode     = NAME_VALUE_NODE_FROM_LINK (NameValueLink);
        CopyNameValueNode = NULL;
        CopyNameValueNode = calloc (sizeof (NAME_VALUE_NODE), sizeof (CHAR8));
        if (CopyNameValueNode == NULL) {
          free (StorageCopy->Name);
          free (StorageCopy->Buffer);
          free (StorageCopy);
          printf ("Memory allocation failed.\n");
          return EFI_ABORTED;
        }
        memcpy (CopyNameValueNode, NameValueNode, sizeof (NAME_VALUE_NODE));

        CopyNameValueNode->Name      = NULL;
        CopyNameValueNode->Value     = NULL;
        CopyNameValueNode->EditValue = NULL;

        CopyNameValueNode->Name      = calloc (FceStrLen (NameValueNode->Name) + 1, sizeof (CHAR16));
        CopyNameValueNode->Value     = calloc (FceStrLen (NameValueNode->Value) + 1, sizeof (CHAR16));
        CopyNameValueNode->EditValue = calloc (FceStrLen (NameValueNode->EditValue) + 1, sizeof (CHAR16));
        if ((CopyNameValueNode->Name == NULL)
          || (CopyNameValueNode->Value == NULL)
          || (CopyNameValueNode->EditValue == NULL)
          ) {
          free (StorageCopy->Name);
          free (StorageCopy->Buffer);
          free (StorageCopy);
          if (CopyNameValueNode->Name != NULL) {
            free (CopyNameValueNode->Name );
          }
          if (CopyNameValueNode->Value != NULL) {
            free (CopyNameValueNode->Value);
          }
          if (CopyNameValueNode->EditValue != NULL) {
            free (CopyNameValueNode->EditValue);
          }
          free (CopyNameValueNode);
          printf ("Memory allocation failed.\n");
          return EFI_ABORTED;
        }
        StrCpy (CopyNameValueNode->Name, NameValueNode->Name);
        StrCpy (CopyNameValueNode->Value, NameValueNode->Value);
        StrCpy (CopyNameValueNode->EditValue, NameValueNode->EditValue);
        //
        // Insert it to StorageCopy->NameValueListHead
        //
        InsertTailList(&StorageCopy->NameValueListHead,&CopyNameValueNode->Link);
        NameValueLink = GetNextNode (&Storage->NameValueListHead, NameValueLink);
      }

      //
      // Insert it to StorageListHead1
      //
      InsertTailList(StorageListHead1,&StorageCopy->Link);
    }
    StorageLink = GetNextNode (StorageListHead2, StorageLink);
  }
  return EFI_SUCCESS;
}

/**
  Check whether the current defaultId and platfrom is equal to the first one of one
  group of defaultId and platformId which have the same variable data.

  @param DefaultId      The default Id
  @param PlatformId     The platform Id

  @retval TRUE          If not equal to the first defaultId and platformId, return TRUE
  @return EFI_ABORTED   Others
**/
BOOLEAN
NoTheKeyIdOfGroup (
  IN  UINT16   DefaultId,
  IN  UINT64   PlatformId
  )
{
  UINT32   Index;

  for (Index = 0; Index < mMultiPlatformParam.KeyIdNum; Index++) {
    if (
      (DefaultId == mMultiPlatformParam.KeyDefaultId[Index])
      && (PlatformId == mMultiPlatformParam.KeyPlatformId[Index])
      ) {
      return FALSE;
    }
  }
  return TRUE;
}

/**
  Evaluate the value in all formset according to the defaultId and platformId.

  If not the multi-platform mode, the defaultId is 0. In this case, the
  platform Id will be the default value of that question.

  @param UpdateMode     It will be TRUE in update Mode

  @retval EFI_SUCCESS   It was complete successfully
  @return EFI_ABORTED   An error occurred
**/
EFI_STATUS
EvaluateTheValueInFormset (
  IN  BOOLEAN  UpdateMode
  )
{
  EFI_STATUS              Status;
  UINT16                  DefaultId;
  UINT64                  PlatformId;
  UINT16                  CurDefaultId;
  UINT64                  CurPlatformId;
  UINT16                  DefaultIndex;
  UINT16                  PlatformIndex;
  UINT32                  Index;
  UQI_PARAM_LIST          *CurUqiList;

  Status        = EFI_SUCCESS;

  if (mMultiPlatformParam.MultiPlatformOrNot) {
    ScanUqiFullList (mUqiList);

    //
    // Multi-platform mode support
    //
    for (DefaultIndex = 0; DefaultIndex < mMultiPlatformParam.DefaultIdNum; DefaultIndex++) {
      for (PlatformIndex = 0; PlatformIndex < mMultiPlatformParam.PlatformIdNum; PlatformIndex++) {
        DefaultId  = mMultiPlatformParam.DefaultId[DefaultIndex];
        PlatformId = mMultiPlatformParam.PlatformId[PlatformIndex];
        //
        //Only parse one time, if a group of defaultId and platformId which have the same variable
        // Take the first one as a key Id of a group
        //
        if (UpdateMode && NoTheKeyIdOfGroup (DefaultId, PlatformId)) {
          continue;
        }
        //
        // Initialize the Storage of mVarListEntry
        //
        ClearStorageEntryList (&mVarListEntry, DefaultId, PlatformId);

        Status = ExtractDefault (
                   NULL,
                   NULL,
                   DefaultId,
                   PlatformId,
                   SystemLevel
                 );
        if (EFI_ERROR (Status)) {
          return EFI_ABORTED;
        }
        //
        // Clear the platformId as 0 after calculation.
        //
        Status = AssignThePlatformId (0);
        if (EFI_ERROR (Status)) {
          printf ("Failed to clear the platformid.\n");
          return Status;
        }
        //
        // Update the value from script file
        //
        if (UpdateMode) {
          SetUqiParametersMultiMode (mUqiList,DefaultId, PlatformId);
        }
        //
        // If not existed the same variables in mAllVarListEntry, insert the new ones.
        // Or else, only add the defaultId and platformId to the former one.
        //
        if (ComparePartSameVariableList (&mAllVarListEntry, &mVarListEntry, &CurDefaultId, &CurPlatformId)) {
          AppendIdToVariables (&mAllVarListEntry, CurDefaultId, CurPlatformId, DefaultId, PlatformId);
        } else {
          //
          // Copy Stroage from mVarListEntry to mAllVarListEntry and assign the defaultId and platformId as well
          //
          Status = BuildVariableList (&mAllVarListEntry, &mVarListEntry, DefaultId, PlatformId, TRUE, UPDATE);
          if (EFI_ERROR (Status)) {
            return EFI_ABORTED;
          }
          //
          // In update mode, add the other defaultId and platform of a group to the variale list
          //
          if (UpdateMode) {
            CurUqiList = mUqiList;

            while (CurUqiList != NULL) {
              if ((DefaultId == CurUqiList->Header.DefaultId[0])
                && (PlatformId == CurUqiList->Header.PlatformId[0])
                ) {
                break;
              }
              CurUqiList = CurUqiList->Next;
            }

            if (CurUqiList == NULL) {
              return EFI_ABORTED;
            }

            for (Index = 1; Index < CurUqiList->Header.IdNum; Index++) {
              CurDefaultId  =  CurUqiList->Header.DefaultId[Index];
              CurPlatformId =  CurUqiList->Header.PlatformId[Index];
              AppendIdToVariables (&mAllVarListEntry, DefaultId, PlatformId, CurDefaultId, CurPlatformId);
            }
          }
        }
      }
    }
  } else {
      //
      // General mode
      //
      Status = ExtractDefault (
                 NULL,
                 NULL,
                 0,
                 0,
                 SystemLevel
               );
      if (EFI_ERROR (Status)) {
        return EFI_ABORTED;
      }
      //
      // If existed the variable in NvStorage, copy them to mVarListEntry.
      // Synchronize the default value from the EFI variable zone to variable list
      //
      Status = EfiVarAndListExchange (TRUE, &mVarListEntry);
      if (Status == EFI_INVALID_PARAMETER) {
        Status = EFI_ABORTED;
        return Status;
      }
      //
      // Update the value from script file
      //
      if (UpdateMode) {
        Status = SetUqiParameters (mUqiList,0, 0);
        if (EFI_ERROR (Status)) {
          return EFI_ABORTED;
        }
      }
      //
      // Copy Stroage from mVarListEntry to mAllVarListEntry
      //
      Status = BuildVariableList (&mAllVarListEntry, &mVarListEntry, 0, 0, TRUE, UPDATE);
      if (EFI_ERROR (Status)) {
        return EFI_ABORTED;
      }
    }
  return EFI_SUCCESS;
}

/**
  Check and compare the value between the script file and variable from BFV.

  It's used in the update operation of multi-plaform mode.

  @retval EFI_SUCCESS   It was complete successfully
  @return EFI_ABORTED   An error occurred
**/
static
EFI_STATUS
CheckValueUpdateList (
  VOID
  )
{
  UINT16              DefaultId;
  UINT64              PlatformId;
  UINT16              DefaultIndex;
  UINT16              PlatformIndex;
  EFI_STATUS          Status;
  FORMSET_STORAGE     *Storage;
  LIST_ENTRY          *StorageLink;
  UINT16              PreDefaultId;
  UINT64              PrePlatformId;

  Storage             = NULL;
  PreDefaultId        = 0xFFFF;
  PrePlatformId       = 0xFFFFFFFFFFFFFFFF;

  ScanUqiFullList (mUqiList);
  if (gEfiFdInfo.ExistNvStoreDatabase) {
    StorageLink = GetFirstNode (&mBfvVarListEntry);
    while (!IsNull (&mBfvVarListEntry, StorageLink)) {
      Storage = FORMSET_STORAGE_FROM_LINK (StorageLink);
      if (PreDefaultId == Storage->DefaultId[0] && PrePlatformId == Storage->PlatformId[0]) {
        StorageLink = GetNextNode (&mBfvVarListEntry, StorageLink);
        continue;
      } else {
        PreDefaultId = Storage->DefaultId[0];
        PrePlatformId = Storage->PlatformId[0];
      }
      DefaultId = PreDefaultId;
      PlatformId = PrePlatformId;
        //
        //Only parse one time, if a group of defaultId and platformId which have the same variable
        // Take the first one as a key Id of a group
        //
        if (NoTheKeyIdOfGroup (DefaultId, PlatformId)) {
          continue;
        }
        //
        // Copy Stroage from mBfvVarListEntry to mVarListEntry. The mVarListEntry was attached to
        // the FormSet->StorageListHead.
        //
        DestroyAllStorage (&mVarListEntry);
        Status = BuildVariableList (&mVarListEntry, &mBfvVarListEntry, DefaultId, PlatformId, FALSE, UPDATE);
        if (EFI_ERROR (Status)) {
          return EFI_ABORTED;
        }
        SetUqiParametersMultiMode (mUqiList,DefaultId, PlatformId);
        //
        // Copy Stroage from mVarListEntry to mAllVarListEntry and assign the defaultId and platformId as well
        //
        Status = BuildVariableList (&mAllVarListEntry, &mVarListEntry, DefaultId, PlatformId, TRUE, UPDATE);
        if (EFI_ERROR (Status)) {
          return EFI_ABORTED;
        }
        StorageLink = GetNextNode (&mBfvVarListEntry, StorageLink);
     }
  } else {
    for (DefaultIndex = 0; DefaultIndex < mMultiPlatformParam.DefaultIdNum; DefaultIndex++) {
      for (PlatformIndex = 0; PlatformIndex < mMultiPlatformParam.PlatformIdNum; PlatformIndex++) {
        DefaultId  = mMultiPlatformParam.DefaultId[DefaultIndex];
        PlatformId = mMultiPlatformParam.PlatformId[PlatformIndex];
        //
        //Only parse one time, if a group of defaultId and platformId which have the same variable
        // Take the first one as a key Id of a group
        //
        if (NoTheKeyIdOfGroup (DefaultId, PlatformId)) {
          continue;
        }
        //
        // Copy Stroage from mBfvVarListEntry to mVarListEntry. The mVarListEntry was attached to
        // the FormSet->StorageListHead.
        //
        DestroyAllStorage (&mVarListEntry);
        Status = BuildVariableList (&mVarListEntry, &mBfvVarListEntry, DefaultId, PlatformId, FALSE, UPDATE);
        if (EFI_ERROR (Status)) {
          return EFI_ABORTED;
        }
        SetUqiParametersMultiMode (mUqiList,DefaultId, PlatformId);
        //
        // Copy Stroage from mVarListEntry to mAllVarListEntry and assign the defaultId and platformId as well
        //
        Status = BuildVariableList (&mAllVarListEntry, &mVarListEntry, DefaultId, PlatformId, TRUE, UPDATE);
        if (EFI_ERROR (Status)) {
          return EFI_ABORTED;
        }
      }
    }
  }
   return EFI_SUCCESS;
}
/**
  Read defaultId and platformId from the whole FD, and store these two values to mMultiPlatformParam.

  If not multi-platform mode, only return EFI_ABORTED.

  @param Fv             the Pointer to the FFS
  @param Length         the length of FFS

  @retval EFI_SUCCESS   It was complete successfully
  @return EFI_ABORTED   An error occurred
**/
static
EFI_STATUS
ReadAllIfrToFromset (
  IN VOID      *Fv,
  IN UINTN     Length
  )
{
  UINT8               NumberofMachingVfrBin;
  UINTN               *VfrBinBaseAddress;
  UINTN               *UniBinBaseAddress;
  EFI_STATUS          Status;
  UINT8               Index;
  EFI_SECTION_STRUCT  *EfiBufferHeader;
  VOID                *EfiAddr;

  NumberofMachingVfrBin = 0;
  VfrBinBaseAddress     = NULL;
  UniBinBaseAddress     = NULL;
  Status                = EFI_SUCCESS;
  EfiBufferHeader       = NULL;
  EfiAddr               = NULL;

  //
  // Locate the efi base address
  //
  EfiBufferHeader = malloc (sizeof (EFI_SECTION_STRUCT));
  if (EfiBufferHeader == NULL) {
    return EFI_ABORTED;
  }
  memset (
    EfiBufferHeader,
    0,
    sizeof (EFI_SECTION_STRUCT)
    );

  Status = ParseSection (
             TRUE,
             (UINT8 *)Fv,
             Length,
             &EfiBufferHeader
             );
  if (Status != EFI_SUCCESS) {
    Status = EFI_ABORTED;
    goto Done;
  }

  EfiAddr = (VOID *)EfiBufferHeader->BufferBase;
  //
  //Search the Offset at the end of FFS, whatever it is compressed or not
  //
  Status = SearchVfrBinInFFS (Fv, EfiAddr, Length, &VfrBinBaseAddress, &NumberofMachingVfrBin);
  if (Status != EFI_SUCCESS) {
    Status = EFI_ABORTED;
    goto Done;
  }
  Status = SearchUniBinInFFS (
             Fv,
             EfiAddr,
             Length,
             &UniBinBaseAddress
             );
  if (Status != EFI_SUCCESS) {
    Status = EFI_ABORTED;
    goto Done;
  }
  //
  // Read all Ifr information into Formset and Form structure
  //
  for (Index = 0; Index < NumberofMachingVfrBin; Index++) {
    if ((EfiBufferHeader->BufferBase + EfiBufferHeader->Length) < *(VfrBinBaseAddress + Index)
      || (EfiBufferHeader->BufferBase + EfiBufferHeader->Length) < *UniBinBaseAddress
    ) {
      printf ("Failed to locate Ifr data from efi by the offset.\n");
      Status = EFI_ABORTED;
      goto Done;
    }
    Status = ParseFormSetInVfrBin (
               (UINTN **)VfrBinBaseAddress,
               (UINTN *)*UniBinBaseAddress,
               Index
             );
    if (EFI_ERROR (Status)) {
      Status = EFI_ABORTED;
      goto Done;
    }
  }

Done:
  //
  // Free the memory which stores the offset
  //
  if (VfrBinBaseAddress != NULL) {
    free (VfrBinBaseAddress);
  }
  if (UniBinBaseAddress != NULL) {
    free (UniBinBaseAddress);
  }
  //
  // Free the memory for uncompressed space in section
  //
  for (Index = 0; Index < EfiBufferHeader->UnCompressIndex; Index++) {
    if ((VOID *)EfiBufferHeader->UncompressedBuffer[Index] != NULL) {
      free ((VOID *)EfiBufferHeader->UncompressedBuffer[Index]);
    }
  }
  return Status;
}

/**
  Get next questions of four kinds in FormSet list.

  If not one kinds of ONE_OF CHECK_BOX ORDER_LIST and NUMERIC, continue to parse next question.
  If parse to the end of questions, return NULL.

  @param FormSetEntryListHead  the pointer to the LIST_ENTRY
  @param OrderOfQuestion       the order of question

  @retval If succeed, return the pointer to the question
  @return NULL
**/
FORM_BROWSER_STATEMENT *
GetNextQuestion (
  IN     LIST_ENTRY     *FormSetEntryListHead,
  IN OUT UINT32         *OrderOfQuestion
  )
{
  FORM_BROWSER_FORMSET    *FormSet;
  LIST_ENTRY              *FormSetLink;
  FORM_BROWSER_FORM       *Form;
  LIST_ENTRY              *FormLink;
  FORM_BROWSER_STATEMENT  *Question;
  LIST_ENTRY              *QuestionLink;
  UINT32                  Index;

  FormSet      = NULL;
  FormSetLink  = NULL;
  Form         = NULL;
  FormLink     = NULL;
  Question     = NULL;
  QuestionLink = NULL;
  Index        = 0;

  FormSetLink = GetFirstNode (FormSetEntryListHead);
  while (!IsNull (FormSetEntryListHead, FormSetLink)) {
    FormSet = FORM_BROWSER_FORMSET_FROM_LINK (FormSetLink);
    //
    // Parse all forms in formset
    //
    FormLink = GetFirstNode (&FormSet->FormListHead);

    while (!IsNull (&FormSet->FormListHead, FormLink)) {
      Form = FORM_BROWSER_FORM_FROM_LINK (FormLink);
      //
      // Parse five kinds of Questions in Form
      //
      QuestionLink = GetFirstNode (&Form->StatementListHead);

      while (!IsNull (&Form->StatementListHead, QuestionLink)) {
        Question = FORM_BROWSER_STATEMENT_FROM_LINK (QuestionLink);
        //
        // Parse five kinds of Questions in Form
        //
        if ((Question->Operand == EFI_IFR_ONE_OF_OP)
          || (Question->Operand == EFI_IFR_NUMERIC_OP)
          || (Question->Operand == EFI_IFR_CHECKBOX_OP)
          || (Question->Operand == EFI_IFR_ORDERED_LIST_OP)
          || (Question->Operand == EFI_IFR_STRING_OP)
          ) {
          if (*OrderOfQuestion == Index++) {
            (*OrderOfQuestion)++;
            return Question;
          }
        }
        QuestionLink = GetNextNode (&Form->StatementListHead, QuestionLink);
      }

      FormLink = GetNextNode (&FormSet->FormListHead, FormLink);
    }
    FormSetLink = GetNextNode (FormSetEntryListHead, FormSetLink);
  }
  return NULL;
}

/**
  Read defaultId and platformId from the whole FD, and store these two values to mMultiPlatformParam.

  If not multi-platform mode, only return EFI_ABORTED.

  @param Fv             the Pointer to the FFS
  @param Length         the length of FFS

  @retval EFI_SUCCESS   It was complete successfully
  @return EFI_ABORTED   An error occurred
**/
static
EFI_STATUS
ReadDefaultAndPlatformId (
  IN  LIST_ENTRY     *FormSetEntryListHead
  )
{
  EFI_STATUS              Status;
  FORM_BROWSER_FORMSET    *FormSet;
  LIST_ENTRY              *FormLink;
  FORMSET_DEFAULTSTORE    *DefaultStore;
  LIST_ENTRY              *DefaultStoreLink;
  FORM_BROWSER_STATEMENT  *CurQuestion;
  UINT32                  MatchedNum;
  UINT32                  QuestionOrder;

  Status                = EFI_SUCCESS;
  CurQuestion           = NULL;
  MatchedNum            = 0;
  QuestionOrder         = 0;
  //
  //Check whether it is the Multi-Platform solution or not
  //
  if (!mMultiPlatformParam.MultiPlatformOrNot) {
    Status = EFI_SUCCESS;
    goto Done;
  }
  //
  // Read defaultId from FormSet List
  //
  FormLink = GetFirstNode (FormSetEntryListHead);
  while (!IsNull (FormSetEntryListHead, FormLink)) {
    FormSet = FORM_BROWSER_FORMSET_FROM_LINK (FormLink);
    //
    // Parse Default Store in formset
    //
    DefaultStoreLink = GetFirstNode (&FormSet->DefaultStoreListHead);

    while (!IsNull (&FormSet->DefaultStoreListHead, DefaultStoreLink)) {
      DefaultStore = FORMSET_DEFAULTSTORE_FROM_LINK (DefaultStoreLink);
      StoreAllDefaultId (DefaultStore->DefaultId);
      DefaultStoreLink = GetNextNode (&FormSet->DefaultStoreListHead, DefaultStoreLink);
    }
    FormLink = GetNextNode (FormSetEntryListHead, FormLink);
  }

  //
  //Read the platformId from FormSetList
  //
  while ((CurQuestion = GetNextQuestion (FormSetEntryListHead, &QuestionOrder)) != NULL) {
    Status = ReadPlaformId (CurQuestion);
    if (!EFI_ERROR (Status)) {
      MatchedNum++;
    }
  }

  if (MatchedNum == 0) {
    printf ("There are no questions included in the platformid in the FD.");
    Status = EFI_ABORTED;
  } else if (MatchedNum > 1) {
    printf ("There are %d questions included in the platformid in the FD.", MatchedNum);
    Status = EFI_ABORTED;
  } else {
    Status = EFI_SUCCESS;
  }

Done:

   return Status;
}

/**
  Read the first two bytes to check whether it is Ascii or UCS2.

  @param  ScriptFile   Pointer to the script file.

  @reture TRUE         If ascii, return TRUE
  @reture FALSE        Others, return FALSE.

**/
FILE_TYPE
IsAsciiOrUcs2 (
  IN   FILE    *ScriptFile
  )
{
  CHAR16    FirstTwoBytes;

  fread(&FirstTwoBytes, 1, 2, ScriptFile);

  if (FirstTwoBytes == BigUnicodeFileTag) {
    return BIG_UCS2;
  } else if (FirstTwoBytes == LittleUnicodeFileTag) {
    return LITTLE_UCS2;
  } else {
    return ASCII;
  }
}

/**
  Read Unicode characters and transfer it to ASCII.

  @param  ScriptFile   The pointer to the script file.
  @param  Type         The type of BigUCS2 or LittleUCS2

  @return              The ASCII characters

**/
CHAR8 *
ReadUcs2ToStr (
  IN OUT  FILE       *ScriptFile,
  IN      FILE_TYPE  Type
)
{
  CHAR16    TempChar16;
  CHAR16    TempChar8;
  CHAR8     *StrChar8;
  UINTN     Index;
  BOOLEAN   FirstParse;

  TempChar16 = L'\0';
  TempChar8  = '\0';
  StrChar8   = NULL;
  Index      = 0;
  FirstParse = TRUE;

  if (ScriptFile == NULL) {
    return NULL;
  }
  StrChar8 = calloc (MAX_STR_LEN_FOR_PICK_UQI, sizeof (CHAR8));
  assert (StrChar8);

  for (Index = 0; Index < MAX_STR_LEN_FOR_PICK_UQI; Index++) {
    //
    // The first parse should skip the 0x0020 (BigUCS2) or 0x2000 (LittleUCS2)
    //
    if (FirstParse) {
      do {
        fread(&TempChar16, sizeof (CHAR16), 1, ScriptFile);
      } while ((TempChar16 == 0x0020) || (TempChar16 == 0x2000));
      FirstParse = FALSE;
    } else {
      fread(&TempChar16, sizeof (CHAR16), 1, ScriptFile);
    }
    //
    // Read until break the "Space"
    //
    if ((TempChar16 == 0x0020) || (TempChar16 == 0x2000)) {
      break;
    }
    if (Type == BIG_UCS2) {
      TempChar8 = (CHAR8)TempChar16;
    } else {
      TempChar8 = (CHAR8)(TempChar16 >> 8);
    }
    memcpy (StrChar8 + Index, &TempChar8, 0x1);
  }
  if (Index == MAX_STR_LEN_FOR_PICK_UQI) {
    free (StrChar8);
    return  NULL;
  }
  *(StrChar8 + Index) = '\0';

  return StrChar8;
}

/**
  Read Unicode characters and transfer it to ASCII.

  @param  AsciiStr     The pointer to the Ascii string. It may inlcudes blank space.
  @param  IdArray      The pointer to the array of Id.

  @return              The number of default or platform Id

**/
static
UINT16
GetNumFromAsciiString (
  IN       CHAR8      *AsciiStr,
  IN OUT   UINT64     *IdArray
)
{
  UINT16   Index1;
  UINT16   Index2;
  UINT16   NumofId;
  CHAR8    CharArray[16];
  BOOLEAN  ExistedValue;

  Index1       = 0;
  Index2       = 0;
  NumofId      = 0;
  ExistedValue = FALSE;

  while (*(AsciiStr + Index1) != '\n') {

    Index2       = 0;
    ExistedValue = FALSE;
    memset (CharArray, 0, 16);
    for (; *(AsciiStr + Index1) == ' '; Index1++);
    for (; (*(AsciiStr + Index1) != ' ') && (*(AsciiStr + Index1) != '\n'); Index1++) {
      assert (Index2 <= 15);
      *(CharArray + Index2++) = *(AsciiStr + Index1);
      ExistedValue = TRUE;
    }
    if (ExistedValue && (*(AsciiStr + Index1 - 1) != '\n')) {
      sscanf(CharArray, "%lld", (long long *)&IdArray[NumofId++]);
    }
  }
  return NumofId;
}

/**
  Parse the script file to build the linked list of question structures.

  Pick up the UQI information and save it to the UqiList. The current scripts
  could be Ascii or UCS2 (Little or Big Mode).

  @param  ScriptFile   The pointer to the script file.

  @return EFI_SUCCESS
  @return EFI_INVALID_PARAMETER  ScriptFile is NULL

**/
static
EFI_STATUS
PickUpUqiFromScript (
  IN     FILE            *ScriptFile
  )
{
  CHAR8           TempChar;
  BOOLEAN         InQuote;
  CHAR16          TempChar16;
  UINT32          UqiSize;
  CHAR16          *UqiBuffer;
  CHAR8           Type[32];
  UINT16          Index;
  UQI_PARAM_LIST  *UqiNode;
  UINTN           MaxContainers;
  UINT32          ScriptsLine;
  FILE_TYPE       AsciiOrUcs2;
  CHAR8           *Char8Str;
  CHAR8           DefaultIdStr[30];
  CHAR8           PlatformIdStr[30];
  CHAR8           PlatformUqi[30];
  UINT16          Index1;
  UINT16          Index2;
  UINT16          Index3;
  UINT16          Index4;
  UINT16          Index5;
  BOOLEAN         ReadDefaultId;
  BOOLEAN         ReadPlatformId;
  BOOLEAN         ReadPlatformIdUqi;
  UINT64          DefaultId[MAX_PLATFORM_DEFAULT_ID_NUM];
  UINT64          PlatformId[MAX_PLATFORM_DEFAULT_ID_NUM];
  UINT16          DefaultIdNum;
  UINT16          PlatformIdNum;
  UINT8           Array[MAX_PLATFORM_DEFAULT_ID_NUM * MAX_PLATFORM_DEFAULT_ID_NUM];
  UINT16          IdStart;
  UINT16          IdEnd;

  Char8Str          = NULL;
  ScriptsLine       = 0;
  AsciiOrUcs2       = ASCII;
  Index1            = 0;
  Index2            = 0;
  Index3            = 0;
  Index4            = 0;
  Index5            = 0;
  ReadDefaultId     = FALSE;
  ReadPlatformId    = FALSE;
  ReadPlatformIdUqi = FALSE;
  DefaultIdNum      = 1;
  PlatformIdNum     = 1;
  InQuote           = FALSE;

  memset (DefaultId,  0, MAX_PLATFORM_DEFAULT_ID_NUM * sizeof (UINT64));
  memset (PlatformId, 0, MAX_PLATFORM_DEFAULT_ID_NUM * sizeof (UINT64));
  memcpy (DefaultIdStr, "/ FCEKEY DEFAULT_ID:",  strlen ("/ FCEKEY DEFAULT_ID:") + 1);
  memcpy (PlatformIdStr,"/FCEKEY PLATFORM_ID:",  strlen ("/FCEKEY PLATFORM_ID:") + 1);
  memcpy (PlatformUqi,  "/FCEKEY PLATFORM_UQI:", strlen ("/FCEKEY PLATFORM_UQI:") + 1);

  if (ScriptFile == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Check the file type
  //
  AsciiOrUcs2 = IsAsciiOrUcs2 (ScriptFile);
  if (AsciiOrUcs2 == ASCII) {
    fseek (ScriptFile, 0, SEEK_SET);
  }

  while(!feof(ScriptFile)) {
    //
    //  Reset local data
    //
    TempChar      = '\0';
    TempChar16    = L'\0';
    UqiSize       = 0;
    UqiBuffer     = NULL;
    MaxContainers = 0;
    UqiNode       = NULL;
    memset(Type, 0, 32);
    //
    //  Read first character of the line to find the line type
    //
    if (AsciiOrUcs2 == ASCII) {
      fread(&TempChar, sizeof (CHAR8), 1, ScriptFile);
    } else {
      fread(&TempChar16, sizeof (CHAR16), 1, ScriptFile);
      if (AsciiOrUcs2 == BIG_UCS2) {
        TempChar = (CHAR8)TempChar16;
      } else {
        TempChar = (CHAR8)(TempChar16 >> 8);
      }
    }
    //
    // Take "\n\r" one line
    //
     if (TempChar != 0x0d) {
       ScriptsLine++;
     }
    switch (TempChar) {

    case 'Q':
      //
      //  Process question line
      //
      //  Read size of UQI string
      //
      if (AsciiOrUcs2 == ASCII) {
        fscanf(ScriptFile, " %x", (INT32 *)&UqiSize);
      } else {
        Char8Str = ReadUcs2ToStr (ScriptFile, AsciiOrUcs2);
        if (Char8Str == NULL) {
          return EFI_ABORTED;
        }
        sscanf(Char8Str, " %x", (INT32 *)&UqiSize);
        if (Char8Str != NULL) {
          free (Char8Str);
          Char8Str = NULL;
        }
      }
      if (UqiSize > MAX_INPUT_ALLOCATE_SIZE) {
        return EFI_ABORTED;
      }
      if (UqiSize == 0) {
        do {
          if (AsciiOrUcs2 == ASCII) {
            fread(&TempChar, sizeof (CHAR8), 1, ScriptFile);
          } else {
            fread(&TempChar16, sizeof (CHAR16), 1, ScriptFile);
            if (AsciiOrUcs2 == BIG_UCS2) {
              TempChar = (CHAR8)TempChar16;
            } else {
              TempChar = (CHAR8)(TempChar16 >> 8);
            }
          }
        } while((TempChar != '\n') && !feof(ScriptFile));
        break;
      }
      //
      //  Malloc buffer for string size + null termination
      //
      UqiBuffer = (CHAR16 *)calloc(UqiSize + 1, sizeof(CHAR16));

      if (UqiBuffer == NULL) {
        printf("Error. Unable to allocate 0x%04lx bytes for UQI string -- FAILURE\n", (unsigned long)((UqiSize + 1) * sizeof(CHAR16)));
        break;
      }
      //
      //  Read UQI string
      //
      for (Index = 0; Index < UqiSize; Index++) {
        if (AsciiOrUcs2 == ASCII) {
          fscanf(ScriptFile, " %hx", (short *)&(UqiBuffer[Index]));
        } else {
          Char8Str = ReadUcs2ToStr (ScriptFile, AsciiOrUcs2);
          if (Char8Str == NULL) {
            free (UqiBuffer );
            return EFI_ABORTED;
          }
          sscanf(Char8Str, " %hx", (short *)&(UqiBuffer[Index]));
          if (Char8Str != NULL) {
            free (Char8Str);
            Char8Str = NULL;
          }
        }
      }
      //
      //  Set null termination
      //
      UqiBuffer[Index] = 0;
      //
      //  Read question type
      //
      if (AsciiOrUcs2 == ASCII) {
        fscanf(ScriptFile, "%31s", Type);
      } else {
        Char8Str = ReadUcs2ToStr (ScriptFile, AsciiOrUcs2);
        if (Char8Str == NULL) {
          free (UqiBuffer);
          return EFI_ABORTED;
        }
        sscanf(Char8Str, " %31s", Type);
        if (Char8Str != NULL) {
          free (Char8Str);
          Char8Str = NULL;
        }
      }
      if (stricmp(Type, "ONE_OF") == 0) {
        UqiNode = CreateInsertUqiNode ();
        if (UqiNode == NULL) {
          free (UqiBuffer);
          return EFI_ABORTED;
        }
        UqiNode->Header.Type       = ONE_OF;
        UqiNode->Header.HexNum     = UqiSize;
        UqiNode->Header.Data       = UqiBuffer;
        UqiNode->Header.Value      = (UINT8 *)calloc(1, sizeof(UINT64));
        UqiNode->Header.DiffValue  = (UINT8 *)calloc(1, sizeof(UINT64));
        UqiNode->Header.IdNum      = PlatformIdNum;
        UqiNode->Header.DefaultId  = (UINT16 *)calloc (PlatformIdNum, sizeof (UINT16));
        if (UqiNode->Header.DefaultId == NULL) {
          printf("Fail to allocate memory");
          return EFI_OUT_OF_RESOURCES;
        }
        UqiNode->Header.PlatformId = (UINT64 *)calloc (PlatformIdNum, sizeof (UINT64));
        if (UqiNode->Header.PlatformId == NULL) {
          printf("Fail to allocate memory");
          return EFI_OUT_OF_RESOURCES;
        }
        for (Index5 = 0; Index5 < PlatformIdNum; Index5++) {
          UqiNode->Header.DefaultId[Index5] = (UINT16)DefaultId[Index5];
        }
        memcpy (UqiNode->Header.PlatformId, PlatformId, PlatformIdNum * sizeof (UINT64));

        if (AsciiOrUcs2 == ASCII) {
          fscanf(ScriptFile, " %llx", (long long *)UqiNode->Header.Value);
        } else {
          Char8Str = ReadUcs2ToStr (ScriptFile, AsciiOrUcs2);
          if (Char8Str == NULL) {
            return EFI_ABORTED;
          }
          sscanf(Char8Str, " %llx", (long long *)UqiNode->Header.Value);
          if (Char8Str != NULL) {
            free (Char8Str);
            Char8Str = NULL;
          }
        }

      } else if (stricmp(Type, "CHECKBOX") == 0) {
        UqiNode = CreateInsertUqiNode ();
        if (UqiNode == NULL) {
          free (UqiBuffer);
          return EFI_ABORTED;
        }
        UqiNode->Header.Type       = CHECKBOX;
        UqiNode->Header.HexNum     = UqiSize;
        UqiNode->Header.Data       = UqiBuffer;
        UqiNode->Header.Value      = (UINT8 *)calloc(1, sizeof(UINT64));
        UqiNode->Header.DiffValue  = (UINT8 *)calloc(1, sizeof(UINT64));
        UqiNode->Header.IdNum      = PlatformIdNum;
        UqiNode->Header.DefaultId  = (UINT16 *)calloc (PlatformIdNum, sizeof (UINT16));
        if (UqiNode->Header.DefaultId == NULL) {
          printf ("Fali to allocate memory!\n");
          return EFI_OUT_OF_RESOURCES;
        }
        UqiNode->Header.PlatformId = (UINT64 *)calloc (PlatformIdNum, sizeof (UINT64));
        if (UqiNode->Header.PlatformId == NULL) {
          printf ("Fali to allocate memory!\n");
          return EFI_OUT_OF_RESOURCES;
        }
        for (Index5 = 0; Index5 < PlatformIdNum; Index5++) {
          UqiNode->Header.DefaultId[Index5] = (UINT16)DefaultId[Index5];
        }
        memcpy (UqiNode->Header.PlatformId, PlatformId, PlatformIdNum * sizeof (UINT64));

        if (AsciiOrUcs2 == ASCII) {
          fscanf(ScriptFile, " %llx", (long long *)UqiNode->Header.Value);
        } else {
          Char8Str = ReadUcs2ToStr (ScriptFile, AsciiOrUcs2);
          if (Char8Str == NULL) {
            return EFI_ABORTED;
          }
          sscanf(Char8Str, " %llx", (long long *)UqiNode->Header.Value);
          if (Char8Str != NULL) {
            free (Char8Str);
            Char8Str = NULL;
          }
        }

      } else if (stricmp(Type, "STRING") == 0) {
        UqiNode = CreateInsertUqiNode ();
        if (UqiNode == NULL) {
          free (UqiBuffer);
          return EFI_ABORTED;
        }

        UqiNode->Header.Type       = STRING;
        UqiNode->Header.HexNum     = UqiSize;
        UqiNode->Header.Data       = UqiBuffer;
        UqiNode->Header.Value      = (UINT8 *)calloc(MAX_INPUT_ALLOCATE_SIZE, sizeof(CHAR16));
        UqiNode->Header.DiffValue  = (UINT8 *)calloc(MAX_INPUT_ALLOCATE_SIZE, sizeof(CHAR16));
        if (UqiNode->Header.Value == NULL) {
          printf ("Fali to allocate memory!\n");
          return EFI_OUT_OF_RESOURCES;
        }
        if (UqiNode->Header.DiffValue == NULL) {
          printf ("Fali to allocate memory!\n");
          return EFI_OUT_OF_RESOURCES;
        }
        UqiNode->Header.IdNum      = PlatformIdNum;
        UqiNode->Header.DefaultId  = (UINT16 *)calloc (PlatformIdNum, sizeof (UINT16));
        if (UqiNode->Header.DefaultId == NULL) {
          printf ("Fali to allocate memory!\n");
          return EFI_OUT_OF_RESOURCES;
        }
        UqiNode->Header.PlatformId = (UINT64 *)calloc (PlatformIdNum, sizeof (UINT64));
        if (UqiNode->Header.PlatformId == NULL) {
          printf ("Fali to allocate memory!\n");
          return EFI_OUT_OF_RESOURCES;
        }
        for (Index5 = 0; Index5 < PlatformIdNum; Index5++) {
          UqiNode->Header.DefaultId[Index5] = (UINT16)DefaultId[Index5];
        }
        memcpy (UqiNode->Header.PlatformId, PlatformId, PlatformIdNum * sizeof (UINT64));
       InQuote = FALSE;
       IdStart = 0;
       IdEnd = 0;
        for (Index = 0; Index < MAX_INPUT_ALLOCATE_SIZE; Index ++) {
          if (AsciiOrUcs2 == ASCII) {
            fread(&TempChar, sizeof (CHAR8), 1, ScriptFile);
          } else {
            fread(&TempChar16, sizeof (CHAR16), 1, ScriptFile);
            if (AsciiOrUcs2 == BIG_UCS2) {
              TempChar = (CHAR8)TempChar16;
            } else {
              TempChar = (CHAR8)(TempChar16 >> 8);
            }
          }
         if (TempChar == '\"') {
             if (InQuote == TRUE) {
                 InQuote = FALSE;
                 IdEnd = Index;
             } else {
                 InQuote = TRUE;
                 IdStart = Index;
             }
         }
          if (Index > IdStart) {
             if (InQuote == FALSE) {
                 break;
             }
             *(UqiNode->Header.Value + Index - IdStart -1) = TempChar;
             Index ++;
          }

        }
       if (IdEnd < IdStart) {
           printf("The STRING is not end with \" character!\n");
           return EFI_ABORTED;
       }
       if (IdStart == 0) {
           printf("The STRING is not start with \" character!\n");
           return EFI_ABORTED;
       }

      } else if (stricmp(Type, "NUMERIC") == 0) {
        UqiNode = CreateInsertUqiNode ();
        if (UqiNode == NULL) {
          free (UqiBuffer);
          return EFI_ABORTED;
        }
        UqiNode->Header.Type       = NUMERIC;
        UqiNode->Header.HexNum     = UqiSize;
        UqiNode->Header.Data       = UqiBuffer;
        UqiNode->Header.Value      = (UINT8 *)calloc(1, sizeof(UINT64));
        UqiNode->Header.DiffValue  = (UINT8 *)calloc(1, sizeof(UINT64));
        UqiNode->Header.IdNum      = PlatformIdNum;
        UqiNode->Header.DefaultId  = (UINT16 *)calloc (PlatformIdNum, sizeof (UINT16));
        if (UqiNode->Header.DefaultId == NULL) {
          printf ("Fali to allocate memory!\n");
          return EFI_OUT_OF_RESOURCES;
        }
        UqiNode->Header.PlatformId = (UINT64 *)calloc (PlatformIdNum, sizeof (UINT64));
        if (UqiNode->Header.PlatformId == NULL) {
          printf ("Fali to allocate memory!\n");
          return EFI_OUT_OF_RESOURCES;
        }
        for (Index5 = 0; Index5 < PlatformIdNum; Index5++) {
          UqiNode->Header.DefaultId[Index5] = (UINT16)DefaultId[Index5];
        }
        memcpy (UqiNode->Header.PlatformId, PlatformId, PlatformIdNum * sizeof (UINT64));

        if (AsciiOrUcs2 == ASCII) {
          fscanf(ScriptFile, " %llx", (long long *)UqiNode->Header.Value);
        } else {
          Char8Str = ReadUcs2ToStr (ScriptFile, AsciiOrUcs2);
          if (Char8Str == NULL) {
            return EFI_ABORTED;
          }
          sscanf(Char8Str, " %llx", (long long *)UqiNode->Header.Value);
          if (Char8Str != NULL) {
            free (Char8Str);
            Char8Str = NULL;
          }
        }

      } else if (stricmp(Type, "ORDERED_LIST") == 0) {
        UqiNode = CreateInsertUqiNode ();
        if (UqiNode == NULL) {
          free (UqiBuffer);
          return EFI_ABORTED;
        }
        UqiNode->Header.Type       = ORDERED_LIST;
        UqiNode->Header.HexNum     = UqiSize;
        UqiNode->Header.Data       = UqiBuffer;
        UqiNode->Header.IdNum      = PlatformIdNum;
        UqiNode->Header.DefaultId  = (UINT16 *)calloc (PlatformIdNum, sizeof (UINT16));
        if (UqiNode->Header.DefaultId == NULL) {
          printf ("Fali to allocate memory!\n");
          return EFI_OUT_OF_RESOURCES;
        }
        UqiNode->Header.PlatformId = (UINT64 *)calloc (PlatformIdNum, sizeof (UINT64));
        if (UqiNode->Header.PlatformId == NULL) {
          printf ("Fali to allocate memory!\n");
          return EFI_OUT_OF_RESOURCES;
        }
        for (Index5 = 0; Index5 < PlatformIdNum; Index5++) {
          UqiNode->Header.DefaultId[Index5] = (UINT16)DefaultId[Index5];
        }
        memcpy (UqiNode->Header.PlatformId, PlatformId, PlatformIdNum * sizeof (UINT64));

        if (AsciiOrUcs2 == ASCII) {
          fscanf(ScriptFile, " %x", (INT32 *)&MaxContainers);
        } else {
          Char8Str = ReadUcs2ToStr (ScriptFile, AsciiOrUcs2);
          if (Char8Str == NULL) {
            return EFI_ABORTED;
          }
          sscanf(Char8Str, " %x", (INT32 *)&MaxContainers);
          if (Char8Str != NULL) {
            free (Char8Str);
            Char8Str = NULL;
          }
        }
        if (MaxContainers > MAX_INPUT_ALLOCATE_SIZE) {
          return EFI_ABORTED;
        }
        UqiNode->Header.Value      = (UINT8 *)calloc(MaxContainers + 1, sizeof(UINT64));
        if (UqiNode->Header.Value == NULL) {
          printf ("Fali to allocate memory!\n");
          return EFI_OUT_OF_RESOURCES;
        }
        UqiNode->Header.DiffValue  = (UINT8 *)calloc(MaxContainers + 1, sizeof(UINT64));
        if (UqiNode->Header.DiffValue == NULL) {
          printf ("Fali to allocate memory!\n");
          return EFI_OUT_OF_RESOURCES;
        }
        *UqiNode->Header.Value     = (UINT8) MaxContainers;
        *UqiNode->Header.DiffValue = (UINT8) MaxContainers;

        for (Index = 1; Index <= MaxContainers; Index++) {
          if (*(UqiNode->Header.Value + Index) == '/') {
            printf ("Error.  Failed to parse the value of ORDERED_LIST.\n");
            return EFI_INVALID_PARAMETER;
          }
          if (AsciiOrUcs2 == ASCII) {
            fscanf(ScriptFile, " %llx", ((long long *)UqiNode->Header.Value + Index));
          } else {
            Char8Str = ReadUcs2ToStr (ScriptFile, AsciiOrUcs2);
            if (Char8Str == NULL) {
              return EFI_ABORTED;
            }
            sscanf(Char8Str, " %llx", ((long long *)UqiNode->Header.Value + Index));
            if (Char8Str != NULL) {
              free (Char8Str);
              Char8Str = NULL;
            }
          }
        }

      } else {
        //
        //  Unknown type
        //
        //  Free UQI buffer before skipping to next line
        //
        free(UqiBuffer);
        UqiBuffer = NULL;
        printf ("Error.  Invalid parameters exist in scripts line %d", ScriptsLine);
        return EFI_ABORTED;
      }
      UqiNode->Header.ScriptsLine = ScriptsLine;
      //
      //  Skip to next line
      //
      do {
        if (AsciiOrUcs2 == ASCII) {
          fread(&TempChar, sizeof (CHAR8), 1, ScriptFile);
        } else {
          fread(&TempChar16, sizeof (CHAR16), 1, ScriptFile);
          if (AsciiOrUcs2 == BIG_UCS2) {
            TempChar = (CHAR8)TempChar16;
          } else {
            TempChar = (CHAR8)(TempChar16 >> 8);
          }
        }
      } while((TempChar != '\n') && !feof(ScriptFile));
      break;

    case '\n':
    case '\r':
      //
      //  Newline, skip to next character
      //
      break;

     case '/':
      //
      // Get the value of PlatformId and DefaultId from comments
      //
      do {
        if (AsciiOrUcs2 == ASCII) {
          fread(&TempChar, sizeof (CHAR8), 1, ScriptFile);
        } else {
          fread(&TempChar16, sizeof (CHAR16), 1, ScriptFile);
          if (AsciiOrUcs2 == BIG_UCS2) {
             TempChar = (CHAR8)TempChar16;
           } else {
             TempChar = (CHAR8)(TempChar16 >> 8);
           }
        }
        //
        //"/ DefaultId :"
        //
        if (!ReadDefaultId) {
          if (TempChar == DefaultIdStr[Index1]) {
            Index1++;
          } else {
            Index1 = 0;
          }
          if (Index1 == strlen (DefaultIdStr)) {
            Index1        = 0;
            Index3        = 0;
            memset (Array, 0, MAX_PLATFORM_DEFAULT_ID_NUM * MAX_PLATFORM_DEFAULT_ID_NUM);
            ReadDefaultId = TRUE;
            mMultiPlatformParam.MultiPlatformOrNot = TRUE;
          }
        } else if (ReadDefaultId) {
          if (TempChar == '\n') {
            ReadDefaultId = FALSE;
            Array[Index3] = TempChar;
            DefaultIdNum  = GetNumFromAsciiString ((CHAR8 *)Array, DefaultId);
            mMultiPlatformParam.KeyDefaultId[mMultiPlatformParam.KeyIdNum] = (UINT16)DefaultId[0];
          } else {
            Array[Index3++] = TempChar;
          }
        }
        //
        //"/ PlatformId:"
        //
        if (!ReadPlatformId) {
          if (TempChar == PlatformIdStr[Index2]) {
            Index2++;
          } else {
            Index2 = 0;
          }
          if (Index2 == strlen (PlatformIdStr)) {
            Index2         = 0;
            Index3         = 0;
            memset (Array, 0, MAX_PLATFORM_DEFAULT_ID_NUM * MAX_PLATFORM_DEFAULT_ID_NUM);
            ReadPlatformId = TRUE;
          }
        } else if (ReadPlatformId) {
          if (TempChar == '\n') {
            ReadPlatformId = FALSE;
            Array[Index3]  = TempChar;
            PlatformIdNum  = GetNumFromAsciiString ((CHAR8 *)Array, PlatformId);
            //
            // Take the first defaultid an platformid as the key of this group
            //
            mMultiPlatformParam.KeyPlatformId[mMultiPlatformParam.KeyIdNum++] = PlatformId[0];
            assert (DefaultIdNum == PlatformIdNum);
          } else {
            Array[Index3++] = TempChar;
          }
        }
        //
        //"/ PlatformIdUqi:"
        //
        if (!ReadPlatformIdUqi) {
          if (TempChar == PlatformUqi[Index4]) {
            Index4++;
          } else {
            Index4 = 0;
          }
          if (Index4 == strlen (PlatformUqi)) {
            Index4         = 0;
            Index3         = 0;
            memset (Array, 0, MAX_PLATFORM_DEFAULT_ID_NUM * MAX_PLATFORM_DEFAULT_ID_NUM);
            ReadPlatformIdUqi = TRUE;
          }
        } else if (ReadPlatformIdUqi) {
          if (mMultiPlatformParam.Uqi.HexNum > 0) {
            continue;
          }
          //
          //  Read size of UQI string
          //
          if (AsciiOrUcs2 == ASCII) {
            fscanf(ScriptFile, " %x", (INT32 *)&mMultiPlatformParam.Uqi.HexNum);
          } else {
            Char8Str = ReadUcs2ToStr (ScriptFile, AsciiOrUcs2);
            if (Char8Str == NULL) {
              return EFI_ABORTED;
            }
            sscanf(Char8Str, " %x", (INT32 *)&mMultiPlatformParam.Uqi.HexNum);
            if (Char8Str != NULL) {
              free (Char8Str);
              Char8Str = NULL;
            }
          }
          if (mMultiPlatformParam.Uqi.HexNum > MAX_INPUT_ALLOCATE_SIZE) {
            return EFI_ABORTED;
          }
          //
          //  Malloc buffer for string size + null termination
          //
          if (mMultiPlatformParam.Uqi.Data != NULL) {
            free (mMultiPlatformParam.Uqi.Data);
            mMultiPlatformParam.Uqi.Data = NULL;
          }
          mMultiPlatformParam.Uqi.Data = (CHAR16 *)calloc(mMultiPlatformParam.Uqi.HexNum + 1, sizeof(CHAR16));

          if (mMultiPlatformParam.Uqi.Data == NULL) {
            printf("Error. Unable to allocate 0x%04zx bytes for UQI string -- FAILURE\n", (mMultiPlatformParam.Uqi.HexNum + 1) * sizeof(CHAR16));
            break;
          }
          //
          //  Read UQI string
          //
          for (Index = 0; Index < mMultiPlatformParam.Uqi.HexNum; Index++) {
            if (AsciiOrUcs2 == ASCII) {
              fscanf(ScriptFile, " %hx", (short *)&(mMultiPlatformParam.Uqi.Data[Index]));
            } else {
              Char8Str = ReadUcs2ToStr (ScriptFile, AsciiOrUcs2);
              if (Char8Str == NULL) {
                return EFI_ABORTED;
              }
              sscanf(Char8Str, " %hx", (short *)&(mMultiPlatformParam.Uqi.Data[Index]));
              if (Char8Str != NULL) {
                free (Char8Str);
                Char8Str = NULL;
              }
            }
          }
          //
          //  Set null termination
          //
          mMultiPlatformParam.Uqi.Data[Index] = 0;
          ReadPlatformIdUqi = FALSE;
        }

      } while((TempChar != '\n') && !feof(ScriptFile));
      break;
      //
      // To do: Get and set DefaultId and PlatformId here!
      //
     default:
      //
      //  Comment or garbage, skip to next line
      //
       do {
         if (AsciiOrUcs2 == ASCII) {
           fread(&TempChar, sizeof (CHAR8), 1, ScriptFile);
         } else {
           fread(&TempChar16, sizeof (CHAR16), 1, ScriptFile);
           if (AsciiOrUcs2 == BIG_UCS2) {
              TempChar = (CHAR8)TempChar16;
            } else {
              TempChar = (CHAR8)(TempChar16 >> 8);
            }
         }
       } while((TempChar != '\n') && !feof(ScriptFile));
       break;
    }
  }
  return EFI_SUCCESS;
}

/**
  Get the offset of file name from the whole path.

  @param  NameStr    The whole file path.

  @retval Offset     Return the offset of file name in path
**/
static
UINTN
GetOffsetOfFileName (
  IN CHAR8   *NameStr
  )
{
  CHAR8       *Str;
  UINTN       Index;
  UINTN       CurIndex;

  Index    = 0;
  CurIndex = 0;
  Str      = NameStr;

  if (NameStr == NULL) {
    return 0;
  }
  while (*Str != '\0') {
    if (*Str == OS_SEP) {
      CurIndex = Index;
    }
    Str++;
    Index++;
  }
  if (*(NameStr + CurIndex) == OS_SEP) {
    return CurIndex + 1;
  } else {
    return 0;
  }
}
/**
  Print the questions which is updated to the current platform successfully.

  Parse the Uqi List, and print it till break NULL.

  @param  List          Pointer to a List.

  @retval EFI_SUCCESS   The Print was complete successfully
  @return EFI_ABORTED   An error occurred
**/
static
VOID
PrintUpdateListInfo (
  IN UQI_PARAM_LIST   *UqiList
  )
{
  UINT32           Index;
  UINT32           Index1;
  UINT32           Index2;
  UQI_PARAM_LIST   *CurList;

  Index1   = 0;
  Index2   = 0;
  Index    = 0;
  CurList  = UqiList;

  printf ("\n\n                            -- Update List --                         ");

  while (CurList != NULL) {
    if (!CurList->ErrorOrNot && CurList->ParseOrNot && !CurList->SameOrNot) {
      ++Index;
      printf ("\n\n[Script line %d] Update No.%d:\n", CurList->Header.ScriptsLine, ++Index1);
      printf ("Q  %04x ", CurList->Header.HexNum);
      for (Index2 = 0; Index2 < CurList->Header.HexNum; Index2++) {
        printf ("%04x ", CurList->Header.Data[Index2]);
      }
      if (CurList->Header.Type == ORDERED_LIST) {
        printf ("ORDERED_LIST ");
      } else if (CurList->Header.Type == CHECKBOX) {
        printf ("CHECKBOX ");
      } else if (CurList->Header.Type == ONE_OF) {
        printf ("ONE_OF ");
      } else if (CurList->Header.Type == NUMERIC) {
        printf ("NUMERIC ");
      } else if (CurList->Header.Type == STRING) {
        printf ("STRING ");
      } else {
        printf ("UNKNOWN ");
      }
      //
      //Print the value of scripts
      //
      printf ("\n[ Update Value From: ");
      if (CurList->Header.Type == ORDERED_LIST) {
        for (Index2 = 0; Index2 <= *(CurList->Header.DiffValue); Index2++) {
          printf ("%02x ", *(CurList->Header.DiffValue + Index2));
        }
      } else if (CurList->Header.Type == STRING) {
        printf("\"");
        WriteUnicodeStr((CHAR16 *)CurList->Header.DiffValue);
        printf("\"");
      } else {
        printf ("%llx ", *(unsigned long long*)(UINT64 *)CurList->Header.DiffValue);
      }
      //
      //Print the value of current platform
      //
      printf (" To: ");
      if (CurList->Header.Type == ORDERED_LIST) {
        for (Index2 = 0; Index2 <= *(CurList->Header.Value); Index2++) {
          printf ("%02x ", *(CurList->Header.Value + Index2));
        }
      } else if (CurList->Header.Type == STRING) {
        printf("\"");
        WriteUnicodeStr((CHAR16 *)CurList->Header.Value);
        printf("\"");
      } else {
        printf ("%llx ", *(unsigned long long*)(UINT64 *)CurList->Header.Value);
      }
      printf ("]");
    }
    CurList = CurList->Next;
  }
  if (Index > 1) {
    printf ("\n\n\n[Results]: %d questions have been updated successfully in total. \n", Index);
  } else {
    printf ("\n\n\n[Results]: %d question has been updated successfully in total. \n", Index);
  }
}


/**
  Print the error, when update questions.

  Parse the Uqi List, and print it till break NULL.

  @param  List          The Pointer to a List.

**/
static
BOOLEAN
PrintErrorInfo (
  IN UQI_PARAM_LIST   *UqiList
  )
{
  UINT32           Index1;
  UINT32           Index2;
  UINT32           Index;
  UQI_PARAM_LIST   *CurList;
  BOOLEAN          IsError;

  Index1   = 0;
  Index2   = 0;
  Index    = 0;
  CurList  = UqiList;
  IsError  = FALSE;

  while (CurList != NULL) {
    if (CurList->ErrorOrNot && CurList->ParseOrNot) {
      IsError  = TRUE;
      ++Index;
      printf ("\n\n[Script line %d] Error Information No.%d:\n", CurList->Header.ScriptsLine, ++Index1);
      printf ("Q  %04x ", CurList->Header.HexNum);
      for (Index2 = 0; Index2 < CurList->Header.HexNum; Index2++) {
        printf ("%04x ", CurList->Header.Data[Index2]);
      }
      if (CurList->Header.Type == ORDERED_LIST) {
        printf ("ORDERED_LIST ");
      } else if (CurList->Header.Type == CHECKBOX) {
        printf ("CHECKBOX ");
      } else if (CurList->Header.Type == ONE_OF) {
        printf ("ONE_OF ");
      } else if (CurList->Header.Type == NUMERIC) {
        printf ("NUMERIC ");
      } else if (CurList->Header.Type == STRING) {
        printf ("STRING ");
      } else {
        printf ("UNKNOWN ");
      }
      //
      //Print the Input value of scripts
      //
      if (CurList->Header.Type == ORDERED_LIST) {
        for (Index2 = 0; Index2 <= *CurList->Header.Value; Index2++) {
          printf ("%02x ", *(CurList->Header.Value + Index2));
        }
      } else if (CurList->Header.Type == STRING) {
        printf("\"");
        WriteUnicodeStr((CHAR16 *)CurList->Header.Value);
        printf("\"");
      } else {
        printf ("%llx ", *(unsigned long long*)(UINT64 *)CurList->Header.Value);
      }
      //
      //Print the Error information
      //
      if (CurList->Error != NULL) {
        printf ("\n%s ", CurList->Error);
      }
    }
    CurList = CurList->Next;
  }
  if (IsError) {
    if (Index > 1) {
      printf ("\n\n[Results]: Occurred %d errors during the update process. \n", Index);
    } else {
      printf ("\n\n[Results]: Occurred %d error during the update process. \n", Index);
    }
  }
  return IsError;
}

/**
  Any questions that exist in both the script and the current platform and have
  different values will be logged to the screen.

  Parse the Uqi List, and print it till break NULL.

  @param  List          Pointer to a List.

  @retval EFI_SUCCESS   The Print was complete successfully
  @return EFI_ABORTED   An error occurreds
**/
static
VOID
PrintVerifiedListInfo (
  IN UQI_PARAM_LIST   *UqiList
  )
{
  UINT32           Index1;
  UINT32           Index2;
  UINT32           Index3;
  UINT32           Index;
  UQI_PARAM_LIST   *CurList;
  UINT32           StrLen;
  UINT32           StrLen1;
  UINT32           StrLen2;

  Index1   = 0;
  Index2   = 0;
  Index    = 0;
  StrLen   = 0;
  CurList  = UqiList;

  StrLen1  = strlen (mSetupTxtName + GetOffsetOfFileName (mSetupTxtName));
  StrLen2  = strlen (mInputFdName + GetOffsetOfFileName (mInputFdName));

  StrLen   = (StrLen1 > StrLen2) ? StrLen1:StrLen2;

  printf ("\n\n                            -- Different List --                         ");

  while (CurList != NULL) {
    if (!CurList->SameOrNot && CurList->ParseOrNot) {
      ++Index;
      printf ("\n\n[Script line %d] Difference No.%d:", CurList->Header.ScriptsLine, ++Index1);
      printf ("\n[%s", mSetupTxtName + GetOffsetOfFileName (mSetupTxtName));
      for (Index3 = 0; Index3 < StrLen - StrLen1; Index3++) {
        printf (" ");
      }
      printf ("]:");
      printf (" Q  %04x ", CurList->Header.HexNum);
      for (Index2 = 0; Index2 < CurList->Header.HexNum; Index2++) {
        printf ("%04x ", CurList->Header.Data[Index2]);
      }
      if (CurList->Header.Type == ORDERED_LIST) {
        printf ("ORDERED_LIST ");
      } else if (CurList->Header.Type == CHECKBOX) {
        printf ("CHECKBOX ");
      } else if (CurList->Header.Type == ONE_OF) {
        printf ("ONE_OF ");
      } else if (CurList->Header.Type == NUMERIC) {
        printf ("NUMERIC ");
      } else if (CurList->Header.Type == STRING) {
        printf ("STRING ");
      } else {
        printf ("UNKNOWN ");
      }
      //
      //Print the Input value of scripts
      //
      if (CurList->Header.Type == ORDERED_LIST) {
        for (Index2 = 0; Index2 <= *(CurList->Header.Value); Index2++) {
          printf ("%02x ", *(CurList->Header.Value + Index2));
        }
      } else if (CurList->Header.Type == STRING) {
        printf("\"");
        WriteUnicodeStr((CHAR16 *)CurList->Header.Value);
        printf("\"");
      } else {
        printf ("%llx ", *(unsigned long long*)(UINT64 *)CurList->Header.Value);
      }
      //
      //Print the value of current platform
      //
      printf ("\n[%s", mInputFdName + GetOffsetOfFileName (mInputFdName));
      for (Index3 = 0; Index3 < StrLen - StrLen2; Index3++) {
        printf (" ");
      }
      printf ("]:");
      printf (" Q  %04x ", CurList->Header.HexNum);
      for (Index2 = 0; Index2 < CurList->Header.HexNum; Index2++) {
        printf ("%04x ", CurList->Header.Data[Index2]);
      }
      if (CurList->Header.Type == ORDERED_LIST) {
        printf ("ORDERED_LIST ");
      } else if (CurList->Header.Type == CHECKBOX) {
        printf ("CHECKBOX ");
      } else if (CurList->Header.Type == ONE_OF) {
        printf ("ONE_OF ");
      } else if (CurList->Header.Type == NUMERIC) {
        printf ("NUMERIC ");
      } else if (CurList->Header.Type == STRING) {
        printf ("STRING ");
      } else {
        printf ("UNKNOWN ");
      }
      if (CurList->Header.Type == ORDERED_LIST) {
        for (Index2 = 0; Index2 <= *(CurList->Header.DiffValue); Index2++) {
          printf ("%02x ", *(CurList->Header.DiffValue + Index2));
        }
      } else if (CurList->Header.Type == STRING) {
        printf("\"");
        WriteUnicodeStr((CHAR16 *)CurList->Header.DiffValue);
        printf("\"");
      } else {
        printf ("%llx ", *(unsigned long long*)(UINT64 *)CurList->Header.DiffValue);
      }
    }
    CurList = CurList->Next;
  }
  if (Index > 1) {
    printf (
      "\n\n\n[Results]: There are %d differences between '%s' and '%s' in total.\n\n\n",
      Index,
      mSetupTxtName + GetOffsetOfFileName (mSetupTxtName),
      mInputFdName + GetOffsetOfFileName (mInputFdName)
      );
  } else {
    printf (
      "\n\n\n[Results]: There is %d difference between '%s' and '%s' in total.\n\n\n",
      Index,
      mSetupTxtName + GetOffsetOfFileName (mSetupTxtName),
      mInputFdName + GetOffsetOfFileName (mInputFdName)
     );
  }
}

/**
  Insert Uqi object to the end of unidirection List.

  @param  InList           The Pointer to the current object
  @param  UqiListEntry     The pointer to the entry of UqiList

  @return EFI_SUCCESS
**/
EFI_STATUS
InsertUnidirectionList (
  IN     UQI_PARAM_LIST  *InList,
  IN     UQI_PARAM_LIST  **UqiListEntry
  )
{
  UQI_PARAM_LIST **UqiCurList;
  UQI_PARAM_LIST *UqiNext;

  UqiCurList = NULL;
  UqiNext    = NULL;

  if (UqiListEntry == NULL) {
    return EFI_ABORTED;
  }
  //
  // Insert to Uqi Node to UqiList
  //
  UqiCurList = UqiListEntry;
  UqiNext    = *UqiCurList;
  if (UqiNext == NULL) {
    //
    //Insert is the first node as node header
    //
    *UqiCurList = InList;
  } else {
    while ((UqiNext != NULL) && (UqiNext->Next != NULL)) {
      UqiNext = UqiNext->Next;
    }
    UqiNext->Next = InList;
  }
  return EFI_SUCCESS;
}

/**
  Free variable unidirection List.

  @param  UqiListEntry     The pointer to the entry of UqiList

  @return EFI_SUCCESS
**/
EFI_STATUS
FreeUnidirectionList (
  IN     UQI_PARAM_LIST  *UqiListEntry
  )
{
  UQI_PARAM_LIST  *Next;

  Next = NULL;
  //
  // Free Uqi List
  //
  while (UqiListEntry != NULL) {
    Next = UqiListEntry->Next;
    if (UqiListEntry->Header.Value != NULL) {
      free (UqiListEntry->Header.Value);
    }
    if (UqiListEntry->Header.DiffValue != NULL) {
      free (UqiListEntry->Header.DiffValue);
    }
    if (UqiListEntry->Header.Data != NULL) {
      free (UqiListEntry->Header.Data);
    }
    if (UqiListEntry->Header.DefaultId != NULL) {
      free (UqiListEntry->Header.DefaultId);
    }
    if (UqiListEntry->Header.PlatformId != NULL) {
      free (UqiListEntry->Header.PlatformId);
    }
    if (UqiListEntry->Error != NULL) {
      free (UqiListEntry->Error);
    }
    free (UqiListEntry);
    UqiListEntry = Next;
  }

  return EFI_SUCCESS;
}

/**
  Delete a directory and files in it.

  @param   DirName   Name of the directory need to be deleted.

  @return EFI_INVALID_PARAMETER
  @return EFI_SUCCESS
**/
static
EFI_STATUS
LibRmDir (
  IN  CHAR8*  DirName
)
{
  CHAR8*          SystemCommand;

  SystemCommand             = NULL;

  if (DirName == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Delete a directory and files in it.
  //

  SystemCommand = malloc (
    strlen (RMDIR_STR) +
    strlen (DirName)     +
    1
    );
  if (SystemCommand == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  sprintf (
    SystemCommand,
    RMDIR_STR,
    DirName
    );

  system (SystemCommand);
  free(SystemCommand);

  return EFI_SUCCESS;
}

/**
  Pick up the FFS from the FD image.

  Call BfmLib to get all FFS in one FD image, and save all which includes IFR
  Binary to gEfiFdInfo structure.

  @retval EFI_SUCCESS          Get the address successfully.
**/
static
EFI_STATUS
PickUpFfsFromFd (
  VOID
  )
{
  CHAR8          *SystemCommandFormatString;
  CHAR8          *SystemCommand;
  CHAR8          *TempSystemCommand;
  CHAR8          *TemDir;
  EFI_STATUS     Status;
  INT32          ReturnValue;

  Status                    = EFI_SUCCESS;
  SystemCommandFormatString = NULL;
  SystemCommand             = NULL;
  TempSystemCommand         = NULL;
  TemDir                    = NULL;
  ReturnValue               = 0;

  memset (&gEfiFdInfo, 0, sizeof (G_EFI_FD_INFO));
  //
  // Construction 'system' command string
  //
  SystemCommandFormatString = "BfmLib -e \"%s\" ";

  SystemCommand = malloc (
                    strlen (SystemCommandFormatString) + strlen (mInputFdName) + 1
                  );

  if (SystemCommand == NULL) {
    printf ("Fail to allocate memory.\n");
    return EFI_ABORTED;
  }

  sprintf (
    SystemCommand,
    "BfmLib -e \"%s\" ",
    mInputFdName
    );

  if (mFullGuidToolDefinitionDir[0] != 0) {
    TempSystemCommand = SystemCommand;
    SystemCommand = malloc (
                    strlen (mFullGuidToolDefinitionDir) + strlen (OS_SEP_STR) + strlen (TempSystemCommand) + 1
                  );

    if (SystemCommand == NULL) {
      free (TempSystemCommand);
      return EFI_UNSUPPORTED;
    }
    strcpy (SystemCommand, mFullGuidToolDefinitionDir);
    strcat (SystemCommand, OS_SEP_STR);
    strcat (SystemCommand, TempSystemCommand);
    free (TempSystemCommand);

  }

  //
  // Call BfmLib to get all FFS in Temp folder of current path
  //
  ReturnValue = system (SystemCommand);
  free (SystemCommand);
  if (ReturnValue == -1) {
    printf ("Error. Call BfmLib failed.\n");
    return EFI_ABORTED;
  }
  //
  //Pick up the FFS which is interrelated with the IFR binary.
  //
  TemDir = getcwd (NULL, _MAX_PATH);
  TemDir = realloc (TemDir, _MAX_PATH);
  if (strlen (TemDir) + strlen (OS_SEP_STR) + strlen (TEMP_DIR_NAME)> _MAX_PATH - 1) {
    printf ("The directory is too long \n");
    return EFI_ABORTED;
  }
  strncat (TemDir, OS_SEP_STR, _MAX_PATH - strlen (TemDir) - 1);
  strncat (TemDir, TEMP_DIR_NAME, _MAX_PATH - strlen (TemDir) - 1);
  mMultiPlatformParam.ExistStorageFfsInBfv = FALSE;
  Status = FindFileInFolder (TemDir, &mMultiPlatformParam.ExistStorageFfsInBfv, &mMultiPlatformParam.SizeOptimized);

  return Status;
}

#define BUILD_IN_TOOL_COUNT 4
/**
  Generate pre-defined guided tools data.

  @return  An EFI_HANDLE contain guided tools data.

**/
static
EFI_HANDLE
PreDefinedGuidedTools (
  VOID
)
{
  EFI_GUID            Guid;
  STRING_LIST         *Tool;
  GUID_SEC_TOOL_ENTRY *FirstGuidTool;
  GUID_SEC_TOOL_ENTRY *LastGuidTool;
  GUID_SEC_TOOL_ENTRY *NewGuidTool;
  UINT8               Index;
  EFI_STATUS          Status;

  CHAR8 PreDefinedGuidedTool[BUILD_IN_TOOL_COUNT][255] = {
    "a31280ad-481e-41b6-95e8-127f4c984779 TIANO TianoCompress",
    "ee4e5898-3914-4259-9d6e-dc7bd79403cf LZMA LzmaCompress",
    "fc1bcdb0-7d31-49aa-936a-a4600d9dd083 CRC32 GenCrc32",
    "3d532050-5cda-4fd0-879e-0f7f630d5afb BROTLI BrotliCompress"
  };

  Tool            = NULL;
  FirstGuidTool   = NULL;
  LastGuidTool    = NULL;
  NewGuidTool     = NULL;

  for (Index = 0; Index < BUILD_IN_TOOL_COUNT; Index++) {
    Tool = SplitStringByWhitespace (PreDefinedGuidedTool[Index]);
    if ((Tool != NULL) &&
        (Tool->Count == 3)
       ) {
      Status = StringToGuid (Tool->Strings[0], &Guid);
      if (!EFI_ERROR (Status)) {
        NewGuidTool = malloc (sizeof (GUID_SEC_TOOL_ENTRY));
        if (NewGuidTool != NULL) {
          memcpy (&(NewGuidTool->Guid), &Guid, sizeof (Guid));
          NewGuidTool->Name = CloneString(Tool->Strings[1]);
          NewGuidTool->Path = CloneString(Tool->Strings[2]);
          NewGuidTool->Next = NULL;
        } else {
          printf ( "Fail to allocate memory. \n");
          if (Tool != NULL) {
            FreeStringList (Tool);
          }
          return NULL;
        }
        if (FirstGuidTool == NULL) {
          FirstGuidTool = NewGuidTool;
        } else {
          LastGuidTool->Next = NewGuidTool;
        }
        LastGuidTool = NewGuidTool;
      }

    } else {
      fprintf (stdout, "Error");
    }
    if (Tool != NULL) {
      FreeStringList (Tool);
      Tool = NULL;
    }
  }
  return FirstGuidTool;
}

/**
  Read all storages under a specified platformId and defaultId from BFV.

  @param  Binary            The pointer to the buffer of binary.
  @param  StorageListEntry  The pointer to the storage list.

  @return length          The length of storage
**/
UINT32
ReadStorageFromBinary (
  IN  UINT8             *Binary,
  IN  LIST_ENTRY        *StorageListEntry
  )
{
  UINT32                       Length;
  UINT8                        *DataBase;
  BOOLEAN                      AuthencitatedMonotonicOrNot;
  BOOLEAN                      AuthencitatedBasedTimeOrNot;

  Length                      = 0;
  AuthencitatedMonotonicOrNot = FALSE;
  AuthencitatedBasedTimeOrNot = FALSE;
  DataBase                    = Binary + sizeof (EFI_COMMON_SECTION_HEADER);
  //
  // Judge the layout of NV by Variable Guid
  //
  AuthencitatedMonotonicOrNot  = CheckMonotonicBasedVarStore ((VOID *)(DataBase + *(UINT16 *)DataBase));
  AuthencitatedBasedTimeOrNot  = CheckTimeBasedVarStoreOrNot ((VOID *)(DataBase + *(UINT16 *)DataBase));

  if (AuthencitatedMonotonicOrNot) {
    //
    // Read variable with Monotonic based layout from binary
    //
    Length = ReadMonotonicBasedVariableToList (Binary, StorageListEntry);
  } else if (AuthencitatedBasedTimeOrNot){
    //
    // Read variable with time-based layout from binary
    //
    Length = ReadTimeBasedVariableToList (Binary, StorageListEntry);
  } else {
    //
    // Read variable with normal layout from binary
    //
    Length = ReadVariableToList (Binary, StorageListEntry);
  }

  return Length;
}

/**
  Insert one storage to the raw bianry, and return its length.

  @param  Storage         The pointer to a storage in storage list.
  @param  Binary          The pointer to the buffer of binary.

  @return length          The length of storage
**/
UINT32
PutStorageToBinary (
  IN  FORMSET_STORAGE   *Storage,
  IN  UINT8             *Binary,
  IN  LIST_ENTRY        *StorageListEntry
  )
{
  EFI_FIRMWARE_VOLUME_HEADER   *VarAddr;
  UINT32                       Length;
  UINT32                       Index;
  UINT8                        *BinaryBeginning;
  FORMSET_STORAGE              *CurStorage;
  LIST_ENTRY                   *StorageLink;
  VOID                         *VariableStoreHeader;
  BOOLEAN                      AuthencitatedMonotonicOrNot;
  BOOLEAN                      AuthencitatedBasedTimeOrNot;

  VarAddr                     = (EFI_FIRMWARE_VOLUME_HEADER   *) gEfiFdInfo.EfiVariableAddr;
  Length                      = 0;
  Index                       = 0;
  BinaryBeginning             = Binary;
  VariableStoreHeader         = (VOID *)((CHAR8 *)VarAddr + VarAddr->HeaderLength);
  AuthencitatedMonotonicOrNot = FALSE;
  AuthencitatedBasedTimeOrNot = FALSE;
  //
  // Judge the layout of NV by gEfiVariableGuid
  //
  AuthencitatedMonotonicOrNot  = CheckMonotonicBasedVarStore (VariableStoreHeader);
  AuthencitatedBasedTimeOrNot  = CheckTimeBasedVarStoreOrNot (VariableStoreHeader);
  //
  // Build the binary for BFV
  //
  StorageLink = GetFirstNode (StorageListEntry);

  while (!IsNull (StorageListEntry, StorageLink)) {
    CurStorage = FORMSET_STORAGE_FROM_LINK (StorageLink);
    if ((CurStorage->DefaultId[0] == Storage->DefaultId[0])
      && (CurStorage->PlatformId[0] == Storage->PlatformId[0])
      && !CurStorage->Skip
      ) {
      CurStorage->Skip = TRUE;

      if (AuthencitatedMonotonicOrNot) {
        //
        // Copy variable with Monotonic based layout to binary
        //
        Length = CopyMonotonicBasedVariableToBinary (CurStorage, BinaryBeginning, Index);
      } else if (AuthencitatedBasedTimeOrNot){
        //
        // Copy variable with time-based layout to binary
        //
        Length = CopyTimeBasedVariableToBinary (CurStorage, BinaryBeginning, Index);
      } else {
        //
        // Copy variable with normall layout to binary
        //
        Length = CopyVariableToBinary (CurStorage, BinaryBeginning, Index);
      }
      Index++;
    }
    StorageLink = GetNextNode (StorageListEntry, StorageLink);
  }
  //
  // Fix the length of storage header under a specified DefaultId and PlatformId
  //
  if (AuthencitatedMonotonicOrNot) {
    FixMontonicVariableHeaderSize (BinaryBeginning, Length);
  } else if (AuthencitatedBasedTimeOrNot){
    FixBasedTimeVariableHeaderSize (BinaryBeginning, Length);
  } else {
    FixVariableHeaderSize (BinaryBeginning, Length);
  }
  return Length;
}

/**
  Insert one storage to Fd's NvStoreDatabase, and return its length.

  @param  Storage         The pointer to a storage in storage list.
  @param  Binary          The pointer to the buffer of binary.

  @return length          The length of storage
**/
UINT32
PutStorageToNvStoreBinary (
  IN  FORMSET_STORAGE   *Storage,
  IN  UINT8             *Binary,
  IN  LIST_ENTRY        *StorageListEntry
  )
{
  UINT32                       Length;
  UINT32                       Index;
  UINT8                        *BinaryBeginning;
  FORMSET_STORAGE              *CurStorage;
  LIST_ENTRY                   *StorageLink;

  Length                      = 0;
  Index                       = 0;
  BinaryBeginning             = Binary;
  //
  // Build the binary for NvStorDatabase
  //
  StorageLink = GetFirstNode (StorageListEntry);
  while (!IsNull (StorageListEntry, StorageLink)) {
    CurStorage = FORMSET_STORAGE_FROM_LINK (StorageLink);
    if ((CurStorage->PlatformId[0] == Storage->PlatformId[0])
      && (CurStorage->DefaultId[0] == Storage->DefaultId[0])
      && !CurStorage->Skip
      ) {
      CurStorage->Skip = TRUE;
      Length = CopyVariableToNvStoreBinary (CurStorage, BinaryBeginning, Index);
      Index++;
    }
    StorageLink = GetNextNode (StorageListEntry, StorageLink);
  }
  // Alignment
  Length = (Length + 3) & ~3;
  FixNvStoreVariableHeaderSize (BinaryBeginning, Length);
  return Length;
}

/**
  Optimize the Delta binary size based on the default setting binary, and
  create a new binary with new size on the Storage.ffs.

  @param  DefaultBinary         The pointer to a default setting binary
  @param  DefaultHeaderLen      The header lenght of default setting binary
  @param  DeltaBinary           The pointer to a delta setting binary
  @param  CurrentSize           The size of current delta data.

  @return length          The length of new storage
**/
UINT32
OptimizeStorageDeltaData (
  IN      UINT8             *DefaultBinary,
  IN      UINT8             *CurrentBinary,
  IN  OUT UINT8             *DeltaBinary,
  IN      UINT32            CurrentSize
  )
{
  UINT32         Size;
  UINT16         Index;
  UINT32         DefaultHeaderSize;
  UINT32         DeltaHeaderSize;
  UINT32         AlignSize;
  PCD_DATA_DELTA   DeltaData;
  DefaultHeaderSize = ((PCD_DEFAULT_DATA *)DefaultBinary)->HeaderSize + 4;
  DeltaHeaderSize   = ((PCD_DEFAULT_DATA *)CurrentBinary)->HeaderSize + 4;
  //
  // Copy the Delta Header directly
  //
  Size = DeltaHeaderSize;
  memcpy (DeltaBinary, CurrentBinary, Size);
  //
  // Compare the delta data and optimize the size
  //
  for (Index = 0; Index < CurrentSize - DeltaHeaderSize; Index++) {
    if (*(DefaultBinary + DefaultHeaderSize + Index) != *(CurrentBinary + DeltaHeaderSize + Index)) {
      DeltaData.Offset = Index;
      DeltaData.Value  = *(CurrentBinary + DeltaHeaderSize + Index);
      memcpy (DeltaBinary + Size, &DeltaData, sizeof (DeltaData));
      Size = Size + sizeof(DeltaData);
    }
  }
  *(UINT32 *)DeltaBinary = Size;
  AlignSize = (Size + 7) & ~7;
  //set Alignment data 0x00
  for (Index = 0; Index < AlignSize - Size; Index++){
    *(DeltaBinary + Size + Index) = 0x0;
  }
  return Size;
}

/**
  Optimize the Delta binary size based on the default setting binary, and
  create a new binary with new size on the Storage.ffs.

  @param  DefaultBinary         The pointer to a default setting binary
  @param  DefaultHeaderLen      The header lenght of default setting binary
  @param  DeltaBinary           The pointer to a delta setting binary
  @param  CurrentSize           The size of current delta data.

  @return length          The length of new storage
**/
UINT32
OptimizeStorageSection (
  IN      UINT8             *DefaultBinary,
  IN      UINT8             *CurrentBinary,
  IN  OUT UINT8             *DeltaBinary,
  IN      UINT32            CurrentSize
  )
{
  UINT32         Size;
  UINT16         Index;
  UINT32         DefaultHeaderSize;
  UINT32         DeltaHeaderSize;
  DATA_DELTA     DeltaData;

  DefaultHeaderSize = *(UINT16 *)DefaultBinary;
  DeltaHeaderSize   = *(UINT16 *)CurrentBinary;

  //
  // Copy the Delta Header directly
  //
  Size = DeltaHeaderSize;
  memcpy (DeltaBinary, CurrentBinary, Size);
  //
  // Compare the delta data and optimize the size
  //
  for (Index = 0; Index < CurrentSize - DeltaHeaderSize; Index++) {
    if (*(DefaultBinary + DefaultHeaderSize + Index) != *(CurrentBinary + DeltaHeaderSize + Index)) {
    DeltaData.Offset = Index;
    DeltaData.Value  = *(CurrentBinary + DeltaHeaderSize + Index);
    memcpy (DeltaBinary + Size, &DeltaData, sizeof (DeltaData));
    Size = Size + sizeof(DeltaData);
    }
  }
  return Size;
}

/**
  Create the storage section and copy it to memory.

  @param  Buffer       The pointer to the buffer
  @param  Size         The size of input buffer.

  @return the new size
**/
UINT32
CreateStorageSection (
  IN OUT  UINT8   *Buffer,
  IN      UINT32  Size,
  IN      CHAR8   *FileName
)
{
  FILE            *BinaryFd;
  UINTN           BytesWrite;
  UINT32          SectionSize;

  BinaryFd   = NULL;
  //
  // Create the raw section files in FFS
  //
  BinaryFd = fopen (FileName, "wb+");
  if (BinaryFd == NULL) {
    printf ("Error. Failed to create the raw data section.\n");
    return 0;
  }
  fseek (BinaryFd, 0, SEEK_SET);
  BytesWrite = fwrite (Buffer, sizeof (CHAR8), Size, BinaryFd);
  fclose (BinaryFd);
  if (BytesWrite != Size) {
    printf ("Error. Failed to write the raw data section.\n");
    return 0;
  }
  CreateRawSection (FileName, FileName);

  BinaryFd = fopen (FileName, "rb");
  if (BinaryFd == NULL) {
    printf ("Error. Failed to open the raw data section.\n");
    return 0;
  }
  fseek (BinaryFd, 0, SEEK_SET);
  BytesWrite = fread (Buffer, sizeof (CHAR8), (Size + sizeof (EFI_COMMON_SECTION_HEADER)), BinaryFd);
  fclose (BinaryFd);
  if (BytesWrite != (Size + sizeof (EFI_COMMON_SECTION_HEADER))) {
    printf ("Error. Failed to read the raw data section.\n");
    return 0;
  }

  SectionSize = FvBufExpand3ByteSize (((EFI_COMMON_SECTION_HEADER *)Buffer)->Size);
  return SectionSize;
}

/**
  Read NvStoreDataBase and insert it to the Storage list.

  @param  InputFdName     The pointer to the input fd name.
  @param  VarListEntry    The pointer to the variable list.

  @return EFI_INVALID_PARAMETER
  @return EFI_SUCCESS
**/
EFI_STATUS
ReadStorageFromNvStoreDatabase (
  IN  LIST_ENTRY  *VarListEntry
)
{

  UINT8               *Binary;
  UINT8               *FullBinary;
  UINT8               *VarDataBinary;
  UINT8               *PreVarDataBinary;
  UINT8               *DataBase;
  PCD_DEFAULT_DATA      *DeltaVarStoreHeader;
  PCD_DEFAULT_DATA    *PrePcdDefaultData;
  UINT8               *DeltaData;
  UINT32              DeltaSize;
  UINT32              DataSize;
  UINT32              HeaderSize;
  UINT32              BinaryLength;
  UINT32              Size;
  UINT32              PreVarDataSize;
  PCD_NV_STORE_DEFAULT_BUFFER_HEADER *NvStoreHeader;
  UINT32              Offset;
  UINT32              Value;
  UINT32              Index;

  BinaryLength              = 0;
  Binary                    = NULL;
  FullBinary                = NULL;
  DataBase                  = NULL;
  DeltaVarStoreHeader       = NULL;
  PreVarDataBinary          = NULL;
  PreVarDataSize            = 0;
  DeltaSize                 = 0;
  Size                      = sizeof (PCD_NV_STORE_DEFAULT_BUFFER_HEADER);
  VarDataBinary             = NULL;

  //
  // Check whether the FD has included the storage FFS
  //
  //if (!mMultiPlatformParam.ExistStorageFfsInBfv) {
  //  return EFI_ABORTED;
  //}
  NvStoreHeader    = (PCD_NV_STORE_DEFAULT_BUFFER_HEADER *)gEfiFdInfo.NvStoreDatabase;
  BinaryLength     = NvStoreHeader->Length;
  Binary       = (UINT8 *)gEfiFdInfo.NvStoreDatabase;
  //
  // If detect size optimized format, transfer it to normal format
  // before parse it
  //
  if (mMultiPlatformParam.SizeOptimized) {
    FullBinary = calloc(gEfiFdInfo.FdSize, sizeof(UINT8));
    if (FullBinary == NULL) {
      printf ("Error. Memory allocation failed.\n");
      return EFI_ABORTED;
    }
  }
  while (Size < BinaryLength) {
    DataBase = Binary + Size;
    DataSize = *(UINT32 *)DataBase;
    if (Size == sizeof (PCD_NV_STORE_DEFAULT_BUFFER_HEADER)) {
      PrePcdDefaultData = (PCD_DEFAULT_DATA *) DataBase;
      HeaderSize = PrePcdDefaultData->HeaderSize;
      PreVarDataSize   = DataSize - 4 - HeaderSize;
      VarDataBinary    = malloc(DataSize);
      if (VarDataBinary == NULL) {
        printf ("Error. Memory allocation failed.\n");
        return EFI_ABORTED;
      }
      memcpy (VarDataBinary, DataBase, DataSize);
      PreVarDataBinary = malloc(DataSize - 4 - HeaderSize);
      if (PreVarDataBinary == NULL) {
        printf ("Error. Memory allocation failed.\n");
        return EFI_ABORTED;
      }
      memcpy (PreVarDataBinary, DataBase + 4 + HeaderSize , DataSize - 4 - HeaderSize);
    } else {
      DeltaVarStoreHeader = (PCD_DEFAULT_DATA *)DataBase;
      DeltaSize           = DeltaVarStoreHeader->DataSize;
      HeaderSize     = DeltaVarStoreHeader->HeaderSize;
      DeltaData           = (UINT8*) DeltaVarStoreHeader;

    VarDataBinary = malloc(PreVarDataSize + HeaderSize + 4);
    if (VarDataBinary == NULL) {
        printf ("Error. Memory allocation failed.\n");
        return EFI_ABORTED;
      }
      //
      // Copy the default setting data
      //
      memcpy (VarDataBinary, DataBase, HeaderSize + 4);
      memcpy (VarDataBinary + HeaderSize + 4, PreVarDataBinary, PreVarDataSize);
      //
      // Merge the delta data with default setting to get the full delta data
      //
      for (Index = 0; Index < (DeltaSize - HeaderSize - 4)/sizeof(PCD_DATA_DELTA); Index++) {
        Offset = ((PCD_DATA_DELTA *)(DeltaData + HeaderSize + 4 + Index * sizeof(PCD_DATA_DELTA)))->Offset;
        Value  = ((PCD_DATA_DELTA *)(DeltaData + HeaderSize + 4 + Index * sizeof(PCD_DATA_DELTA)))->Value;
        if (*(VarDataBinary + HeaderSize + 4 + Offset) != Value) {
          *(VarDataBinary + HeaderSize + 4 + Offset) = (UINT8)Value;
        }
      }
    }
    //
    // Store the Variable Data to VarListEntry
    //

    ReadNvStoreVariableToList(VarDataBinary, VarListEntry);
    Size += (DataSize + 7) & ~7;
  }

  if (VarDataBinary != NULL) {
    free (VarDataBinary);
  }
  return EFI_SUCCESS;
}

/**
  Read FFS from BFV and insert it to the Storage list.

  @param  InputFdName     The pointer to the input fd name.
  @param  VarListEntry    The pointer to the variable list.

  @return EFI_INVALID_PARAMETER
  @return EFI_SUCCESS
**/
EFI_STATUS
ReadStorageFromBfv (
  IN  LIST_ENTRY  *VarListEntry
)
{

  UINT8               *Binary;
  UINT8               *FullBinary;
  UINT8               *DataBase;
  UINT8               *PreVarStoreHeader;
  DATA_DELTA          *DeltaVarStoreHeader;
  UINT8               *DeltaData;
  UINT32              PreDataSize;
  UINT32              DeltaSize;
  UINT32              BinaryLength;
  UINT32              Size;
  UINT32              SectionSize;
  UINT32              FullSectionLen;
  UINT32              FullSectionSize;
  EFI_FFS_FILE_HEADER *FfsHeader;
  UINT16              Offset;
  UINT8               Value;
  UINT32              Index;
  CHAR8               *SectionName;

  BinaryLength              = 0;
  Binary                    = NULL;
  FullBinary                = NULL;
  DataBase                  = NULL;
  PreVarStoreHeader         = NULL;
  DeltaVarStoreHeader       = NULL;
  PreDataSize               = 0;
  DeltaSize                 = 0;
  FullSectionSize           = 0;
  Size                      = sizeof (EFI_FFS_FILE_HEADER);
  FfsHeader                 = NULL;
  FullSectionLen            = 0;
  SectionName               = NULL;

  SectionName = getcwd(NULL, _MAX_PATH);
  SectionName = realloc (SectionName, _MAX_PATH);
  if (strlen (SectionName) + 2 * strlen (OS_SEP_STR) + strlen ("Temp") + strlen ("TempSection.sec") >
      _MAX_PATH - 1) {
    printf ("Error. The current path is too long.\n");
    return EFI_INVALID_PARAMETER;
  }

  sprintf (SectionName + strlen (SectionName), "%cTemp%cTempSection.sec", OS_SEP, OS_SEP);
  //
  // Check whether the FD has included the storage FFS
  //
  if (!mMultiPlatformParam.ExistStorageFfsInBfv) {
    return EFI_ABORTED;
  }
  FfsHeader    = (EFI_FFS_FILE_HEADER *)gEfiFdInfo.StorageFfsInBfv;
  BinaryLength = FvBufExpand3ByteSize (FfsHeader->Size);
  Binary       = (UINT8 *)FfsHeader;
  //
  // If detect size optimized format, transfer it to normal format
  // before parse it
  //
  if (mMultiPlatformParam.SizeOptimized) {
    FullBinary = calloc(gEfiFdInfo.FdSize, sizeof(UINT8));
  if (FullBinary == NULL) {
      printf ("Error. Memory allocation failed.\n");
      return EFI_ABORTED;
  }
  while (Size < BinaryLength) {
    SectionSize = FvBufExpand3ByteSize (((EFI_COMMON_SECTION_HEADER *)(Binary + Size))->Size);
    DataBase    = Binary + Size + sizeof (EFI_COMMON_SECTION_HEADER);
    if (Size == sizeof (EFI_FFS_FILE_HEADER)) {
      PreVarStoreHeader     = DataBase + *(UINT16 *)DataBase;
    PreDataSize           = SectionSize - (*(UINT16 *)DataBase + sizeof(EFI_COMMON_SECTION_HEADER));
      memcpy (FullBinary, DataBase, SectionSize - sizeof(EFI_COMMON_SECTION_HEADER));
    FullSectionLen        = CreateStorageSection (FullBinary, *(UINT16 *)DataBase + PreDataSize, SectionName);
    } else {
    DeltaVarStoreHeader = (DATA_DELTA *)(DataBase + *(UINT16 *)DataBase);
    DeltaSize           = *(UINT16 *)DataBase + PreDataSize;
    DeltaData           = FullBinary + FullSectionSize + *(UINT16 *)DataBase;
    //
    // Copy the DefaultId and PlatformId directly
    //
      memcpy (FullBinary + FullSectionSize, DataBase, *(UINT16 *)DataBase);
    //
    // Copy the default setting data
    //
    memcpy (DeltaData, PreVarStoreHeader, PreDataSize);
    //
    // Merge the delta data with default setting to get the full delta data
    //
    for (Index = 0; Index < (SectionSize - *(UINT16 *)DataBase - sizeof(EFI_COMMON_SECTION_HEADER))/sizeof(DATA_DELTA); Index++) {
      Offset = (DeltaVarStoreHeader + Index)->Offset;
      Value  = (DeltaVarStoreHeader + Index)->Value;
      if (*(DeltaData + Offset) != Value) {
        *(DeltaData + Offset) = Value;
      }
    }
    FullSectionLen = CreateStorageSection (FullBinary + FullSectionSize, DeltaSize, SectionName);
    }
    //
    // Store the previous binary information
    //
    DataBase              = FullBinary + FullSectionSize + sizeof (EFI_COMMON_SECTION_HEADER);
    PreVarStoreHeader     = DataBase + *(UINT16 *)DataBase;

    Size                 += (SectionSize + 3) & ~3;
    FullSectionSize      += (FullSectionLen + 3) & ~3;;
  }
  //
  // Update to the new size
  //
    BinaryLength = FullSectionSize;
  Binary       = FullBinary;
  Size         = 0;
  }

  //
  // Read the storage from BFV and insert to storage list
  //
  while (Size < BinaryLength) {
    SectionSize = ReadStorageFromBinary ((Binary + Size), VarListEntry);
  Size += (SectionSize + 3) & ~3;
  }
  if (FullBinary != NULL) {
    free (FullBinary);
  }

  return EFI_SUCCESS;
}

#define SIZE_64K 0x10000

/**
  Create the storage and insert it to BFV by calling BfmLib.

  @param  InputFdName     The pointer to the input fd name.
  @param  OutputFdName    The pointer to the input fd name.
  @param  VarListEntry    The pointer to the variable list.

  @return EFI_INVALID_PARAMETER
  @return EFI_SUCCESS
**/
EFI_STATUS
InsertBinaryToBfv (
  IN  CHAR8       *InputFdName,
  IN  CHAR8       *OutputFdName,
  IN  LIST_ENTRY  *VarListEntry
)
{
  UINT8             *Binary;
  UINT8             *PreBinary;
  UINT32            BinaryLength;
  UINT32            PreBinaryLength;
  UINT32            OptimizedBinaryLength;
  UINT32            Size;
  UINT32            OptimizedSize;
  EFI_STATUS        Status;
  LIST_ENTRY        *StorageLink;
  FORMSET_STORAGE   *Storage;
  CHAR8             *SystemCommandFormatString;
  CHAR8             *SectionNameFormatString;
  CHAR8             *SystemCommand;
  CHAR8             *TempSystemCommand;
  INT32             ReturnValue;
  CHAR8             *FileName;
  BOOLEAN           SizeOptimizedFlag;
  CHAR8             *SectionName[_MAXIMUM_SECTION_FILE_NUM];
  UINT32            Index;
  CHAR8             *TemDir;
  //
  // Workaround for static code checkers.
  // Ensures the size of 'IndexStr' can hold all the digits of an unsigned
  // 32-bit integer.
  //
  CHAR8             IndexStr[16];

  BinaryLength              = 0;
  PreBinaryLength           = 0;
  Storage                   = NULL;
  StorageLink               = NULL;
  Binary                    = NULL;
  PreBinary                 = NULL;
  Size                      = 0;
  OptimizedSize             = 0;
  Status                    = EFI_SUCCESS;
  SystemCommandFormatString = NULL;
  SectionNameFormatString   = NULL;
  SystemCommand             = NULL;
  TempSystemCommand         = NULL;
  SizeOptimizedFlag         = FALSE;
  Index                     = 0;
  FileName                  = NULL;

  TemDir = getcwd (NULL, _MAX_PATH);
  TemDir = realloc (TemDir, _MAX_PATH);
  SectionNameFormatString = "%s%cTemp%c%s.sec";

  memset (SectionName, 0, _MAXIMUM_SECTION_FILE_NUM * sizeof(CHAR8 *));
  FileName = malloc (strlen (TemDir) + 1 + strlen ("Storage.ffs") + 1);
  if (FileName == NULL) {
    printf ("Error. Memory allocation failed.\n");
    Status = EFI_ABORTED;
    goto Done;
  }
  sprintf (FileName, "%s%cStorage.ffs", TemDir, OS_SEP);
  //
  // Allocate the buffer which is the same with the input FD
  //
  Binary = malloc (SIZE_64K);
  if (Binary == NULL) {
    printf ("Error. Memory allocation failed.\n");
    Status = EFI_ABORTED;
    goto Done;
  }
  PreBinary = malloc (SIZE_64K);
  if (PreBinary == NULL) {
    printf ("Error. Memory allocation failed.\n");
    Status = EFI_ABORTED;
    goto Done;
  }
  //
  // If already existed a Storage.ffs in FD, keep the same format when execute update operation whatever input -a or not -a options.
  //
  if (mMultiPlatformParam.SizeOptimized
    || (!mMultiPlatformParam.ExistStorageFfsInBfv && mMultiPlatformParam.SizeOptimizedParam)
    ) {
    SizeOptimizedFlag = TRUE;
  } else if (mMultiPlatformParam.ExistStorageFfsInBfv && mMultiPlatformParam.SizeOptimizedParam && !mMultiPlatformParam.SizeOptimized) {
    printf ("\nWarning. The \"-a\" parameter is ignored.\n");
  }
  //
  // Build the binary for BFV
  //
  StorageLink           = GetFirstNode (VarListEntry);

  while (!IsNull (VarListEntry, StorageLink)) {
    Storage = FORMSET_STORAGE_FROM_LINK (StorageLink);
    if (!Storage->Skip) {
    //
    // Assign the section name under the Temp directory
    //
      sprintf (IndexStr, "%d", Index);
    SectionName[Index] = calloc (
                      strlen (SectionNameFormatString) + strlen (TemDir) + strlen(IndexStr) + 1,
                      sizeof(CHAR8)
                    );
    if (SectionName[Index] == NULL) {
        printf ("Error. Memory allocation failed.\n");
        Status = EFI_ABORTED;
        goto Done;
      }
    sprintf (
        SectionName[Index],
        "%s%cTemp%c%s.sec",
        TemDir,
        OS_SEP,
        OS_SEP,
        IndexStr
      );
    memset(Binary, 0, SIZE_64K);
      Size = PutStorageToBinary (Storage, Binary, VarListEntry);
      assert (Size < SIZE_64K);
    //
    // Re-calculate the storage section by size optimization
    //
      if (PreBinaryLength != 0 && SizeOptimizedFlag) {
    OptimizedSize = OptimizeStorageSection (
                    PreBinary + sizeof (EFI_COMMON_SECTION_HEADER),
                          Binary,
                    PreBinary + PreBinaryLength,
                    Size
                  );
    if (OptimizedSize == 0) {
        printf ("Error. Failed to optimize the storage section.\n");
          Status = EFI_ABORTED;
          goto Done;
    }
      }
    //
    // Create the raw section with normal format
    //
      assert (Size < SIZE_64K - sizeof (EFI_COMMON_SECTION_HEADER));
    BinaryLength = CreateStorageSection (Binary, Size, SectionName[Index]);
      if (BinaryLength == 0) {
      printf ("Error. Failed to create the storage section.\n");
        Status = EFI_ABORTED;
        goto Done;
      }
      assert (BinaryLength < SIZE_64K);

    //
    // Create the raw section with optimized format
    //
      if (PreBinaryLength != 0 && SizeOptimizedFlag) {
      OptimizedBinaryLength = CreateStorageSection (PreBinary + PreBinaryLength, OptimizedSize, SectionName[Index]);
        if (OptimizedBinaryLength == 0) {
        printf ("Error. Failed to create the storage section.\n");
          Status = EFI_ABORTED;
          goto Done;
        }
      }
      PreBinaryLength = BinaryLength;
    memcpy (PreBinary, Binary, PreBinaryLength);
    Index++;
    }
    StorageLink = GetNextNode (VarListEntry, StorageLink);
  }
  //
  // Create the raw ffs by GenFfs
  //
  CreateRawFfs (&SectionName[0], FileName, SizeOptimizedFlag);

  //
  // Call BfmLib to insert this binary into the BFV of FD.
  //
  //
  // Construction 'system' command string
  //
  if (mMultiPlatformParam.ExistStorageFfsInBfv) {
    if (mFvNameGuidString != NULL) {
      SystemCommandFormatString = "BfmLib -r \"%s\" \"%s\" \"%s\" -g %s";
      SystemCommand = malloc (
                      strlen (SystemCommandFormatString) + strlen (mInputFdName) + strlen (mOutputFdName) + strlen (FileName) + strlen (mFvNameGuidString) + 1
                    );
      if (SystemCommand == NULL) {
        Status = EFI_ABORTED;
        goto Done;
      }
      sprintf (
        SystemCommand,
        "BfmLib -r \"%s\" \"%s\" \"%s\" -g %s",
        mInputFdName,
        FileName,
        mOutputFdName,
        mFvNameGuidString
        );
    } else {
      SystemCommandFormatString = "BfmLib -r \"%s\" \"%s\" \"%s\"";
      SystemCommand = malloc (
                      strlen (SystemCommandFormatString) + strlen (mInputFdName) + strlen (mOutputFdName) + strlen (FileName) + 1
                    );
      if (SystemCommand == NULL) {
        Status = EFI_ABORTED;
        goto Done;
      }
      sprintf (
        SystemCommand,
        "BfmLib -r \"%s\" \"%s\" \"%s\"",
        mInputFdName,
        FileName,
        mOutputFdName
        );
    }
  } else {
    if (mFvNameGuidString != NULL) {
      SystemCommandFormatString = "BfmLib -i \"%s\" \"%s\" \"%s\" -g %s";
      SystemCommand = malloc (
                      strlen (SystemCommandFormatString) + strlen (mInputFdName) + strlen (mOutputFdName) + strlen (FileName) + strlen (mFvNameGuidString) + 1
                    );
      if (SystemCommand == NULL) {
        Status = EFI_ABORTED;
        goto Done;
      }
      sprintf (
        SystemCommand,
        "BfmLib -i \"%s\" \"%s\" \"%s\" -g %s",
        mInputFdName,
        FileName,
        mOutputFdName,
        mFvNameGuidString
        );
    } else {
      SystemCommandFormatString = "BfmLib -i \"%s\" \"%s\" \"%s\"";
      SystemCommand = malloc (
                      strlen (SystemCommandFormatString) + strlen (mInputFdName) + strlen (mOutputFdName) + strlen (FileName) + 1
                    );
      if (SystemCommand == NULL) {
        Status = EFI_ABORTED;
        goto Done;
      }
      sprintf (
        SystemCommand,
        "BfmLib -i \"%s\" \"%s\" \"%s\"",
        mInputFdName,
        FileName,
        mOutputFdName
        );
    }
  }

  if (mFullGuidToolDefinitionDir[0] != 0) {
    TempSystemCommand = SystemCommand;
    SystemCommand = malloc (
                    strlen (mFullGuidToolDefinitionDir) + strlen ("\\") + strlen (TempSystemCommand ) + 1
                  );

    if (SystemCommand == NULL) {
      free (TempSystemCommand);
      goto Done;
    }
    strcpy (SystemCommand, mFullGuidToolDefinitionDir);
    strcat (SystemCommand, OS_SEP_STR);
    strcat (SystemCommand, TempSystemCommand);
    free (TempSystemCommand);
  }

  ReturnValue = system (SystemCommand);
  free (SystemCommand);
  remove (FileName);
  if (ReturnValue == -1) {
    Status = EFI_ABORTED;
  }
Done:
  for (Index = 0; SectionName[Index] != NULL; Index++) {
    free (SectionName[Index]);
  }
  if (PreBinary != NULL) {
    free (PreBinary);
  }
  if (Binary) {
    free (Binary);
  }
  return Status;
}

/**
  Create the storage and insert it to NvStoreDatabase.

  @param  InputFdName     The pointer to the input fd name.
  @param  OutputFdName    The pointer to the input fd name.
  @param  VarListEntry    The pointer to the variable list.

  @return EFI_INVALID_PARAMETER
  @return EFI_SUCCESS
**/
EFI_STATUS
InsertBinaryToNvStoreDatabase (
  IN  CHAR8       *InputFdName,
  IN  CHAR8       *OutputFdName,
  IN  LIST_ENTRY  *VarListEntry
)
{
  UINT8             *Binary;
  UINT8             *PreBinary;
  UINT8             *NvStoreDatabaseBuffer;
  UINT32            PreBinaryLength;
  UINT32            Size;
  UINT32            NvStoreDatabaseSize;
  UINT32            OptimizedSize;
  EFI_STATUS        Status;
  LIST_ENTRY        *StorageLink;
  FORMSET_STORAGE   *Storage;
  BOOLEAN           SizeOptimizedFlag;
  PCD_NV_STORE_DEFAULT_BUFFER_HEADER  *NvStoreBufferHeader;
  PCD_DEFAULT_DATA  *PcdDefaultData;

  //
  // Workaround for static code checkers.
  // Ensures the size of 'IndexStr' can hold all the digits of an unsigned
  // 32-bit integer.
  //

  PreBinaryLength           = 0;
  Storage                   = NULL;
  StorageLink               = NULL;
  Binary                    = NULL;
  PreBinary                 = NULL;
  NvStoreDatabaseBuffer     = NULL;
  PcdDefaultData            = NULL;
  Size                      = 0;
  NvStoreDatabaseSize       = 0;
  OptimizedSize             = 0;
  Status                    = EFI_SUCCESS;
  SizeOptimizedFlag         = FALSE;

  //
  // Allocate the buffer which is the same with the input FD
  //

  Binary = malloc (SIZE_64K);
  if (Binary == NULL) {
    printf ("Error. Memory allocation failed.\n");
    Status = EFI_ABORTED;
    goto Done;
  }
  NvStoreBufferHeader = (PCD_NV_STORE_DEFAULT_BUFFER_HEADER *) gEfiFdInfo.NvStoreDatabase;
  NvStoreDatabaseBuffer = malloc (NvStoreBufferHeader->MaxLength);
  if (NvStoreDatabaseBuffer == NULL) {
    printf ("Error. Memory allocation failed.\n");
    Status = EFI_ABORTED;
    goto Done;
  }
  memcpy(NvStoreDatabaseBuffer, gEfiFdInfo.NvStoreDatabase, NvStoreBufferHeader->MaxLength);
  PreBinary = malloc (SIZE_64K);
  if (PreBinary == NULL) {
    printf ("Error. Memory allocation failed.\n");
    Status = EFI_ABORTED;
    goto Done;
  }

  if (gEfiFdInfo.ExistNvStoreDatabase) {
    SizeOptimizedFlag = TRUE;
  } else {
    Status = EFI_ABORTED;
    goto Done;
  }
  //
  // Build the binary for BFV
  //
  StorageLink           = GetFirstNode (VarListEntry);
  while (!IsNull (VarListEntry, StorageLink)) {
    Storage = FORMSET_STORAGE_FROM_LINK (StorageLink);
    if (!Storage->Skip) {
      memset(Binary, 0, SIZE_64K);
      Size = PutStorageToNvStoreBinary (Storage, Binary, VarListEntry);
      assert (Size < SIZE_64K);
      //
      // Re-calculate the storage section by size optimization
      //
      if (PreBinaryLength != 0 && SizeOptimizedFlag) {
         OptimizedSize = OptimizeStorageDeltaData (
                  PreBinary,
                  Binary,
                  NvStoreDatabaseBuffer + NvStoreDatabaseSize,
                  Size
                  );
         if (OptimizedSize == 0) {
           printf ("Error. Failed to optimize the storage section.\n");
           Status = EFI_ABORTED;
           goto Done;
         }
         //Alignment
         OptimizedSize = (OptimizedSize + 7) & ~7;
         NvStoreDatabaseSize += OptimizedSize;
      } else {
        //Alignment
        Size = (Size + 7) & ~7;
        PcdDefaultData = (PCD_DEFAULT_DATA *)Binary;
        memcpy(NvStoreDatabaseBuffer + sizeof(PCD_NV_STORE_DEFAULT_BUFFER_HEADER), Binary, Size + PcdDefaultData->HeaderSize + 4 );
        PreBinaryLength = Size  + PcdDefaultData->HeaderSize + 4;
        NvStoreDatabaseSize = sizeof(PCD_NV_STORE_DEFAULT_BUFFER_HEADER) + PreBinaryLength;
        memcpy(PreBinary, Binary, PreBinaryLength);
      }
    }
    StorageLink = GetNextNode (VarListEntry, StorageLink);
  }
  if (NvStoreBufferHeader->Length != NvStoreDatabaseSize) {
    ((PCD_NV_STORE_DEFAULT_BUFFER_HEADER *)NvStoreDatabaseBuffer)->Length = NvStoreDatabaseSize;
    }
  memcpy(gEfiFdInfo.NvStoreDatabase, NvStoreDatabaseBuffer, NvStoreDatabaseSize);

Done:
  DestroyAllStorage (&mAllVarListEntry);
  if (PreBinary != NULL) {
    free (PreBinary);
  }
  if (Binary) {
    free (Binary);
  }
  return Status;
}

extern UINT32 mMaxCount;
extern UINT32 mCount;
extern CHAR8  *mStringBuffer;

/**
  Read the HII configure file from all FFS

  @retval EFI_SUCCESS       It was complete successfully
  @return EFI_ABORTED       An error occurred
**/
static
EFI_STATUS
ReadCongFile (
  VOID
  )
{
  EFI_STATUS     Status;
  UINT32         Index;
  UINT16         DefaultIndex;
  UINT16         PlatformIndex;
  UINT16         PreDefaultId;
  UINT64         PrePlatformId;
  LIST_ENTRY     NewStorageListHead;
  BOOLEAN        BfvOverried;
  FORMSET_STORAGE *Storage;
  LIST_ENTRY      *StorageLink;

  Storage        = NULL;
  Status         = EFI_SUCCESS;
  BfvOverried    = FALSE;
  Index          = 0;
  PreDefaultId   = 0xFFFF;
  PrePlatformId  = 0xFFFFFFFFFFFFFFFF;
  //
  // Read all Ifr information to Formset list
  //
  for (Index = 0; (gEfiFdInfo.FfsArray[Index] != NULL) && (gEfiFdInfo.Length[Index] != 0); Index++) {
    Status = ReadAllIfrToFromset (
               gEfiFdInfo.FfsArray[Index],
               gEfiFdInfo.Length[Index]
               );
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  }
  //
  // Read defaultId and platformId
  //
  if (!gEfiFdInfo.ExistNvStoreDatabase) {
    Status = ReadDefaultAndPlatformId (&mFormSetListEntry);
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  }
  //
  // If existed the variable data in BFV, abstract them to a variable list.
  // If not exsited, just skip it.
  //
  if (mMultiPlatformParam.MultiPlatformOrNot) {
    if (gEfiFdInfo.ExistNvStoreDatabase) {
      Status = ReadStorageFromNvStoreDatabase(&mBfvVarListEntry);
    } else {
      Status = ReadStorageFromBfv (&mBfvVarListEntry);
    }
    if (!EFI_ERROR (Status)) {
      BfvOverried = TRUE;
    }
  }
    //
    // If not existed the storage data in BFV, evaluate the
    // default value according to the defaultId and platformId
    // Or else, skip it.
    //
  if (!BfvOverried) {
    Status = EvaluateTheValueInFormset (FALSE);
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  }

  //
  // Output the question and value information on screen
  //
  if (mMultiPlatformParam.MultiPlatformOrNot) {
    //
    // Multi-platform mode support
    //
    if (gEfiFdInfo.ExistNvStoreDatabase) {
      StorageLink = GetFirstNode (&mBfvVarListEntry);
      while (!IsNull (&mBfvVarListEntry, StorageLink)) {
        Storage = FORMSET_STORAGE_FROM_LINK (StorageLink);
        if (PreDefaultId == Storage->DefaultId[0] && PrePlatformId == Storage->PlatformId[0]) {
          StorageLink = GetNextNode (&mBfvVarListEntry, StorageLink);
          continue;
        } else {
          PreDefaultId = Storage->DefaultId[0];
          PrePlatformId = Storage->PlatformId[0];
        }
        InitializeListHead(&NewStorageListHead);
        //
        // Use the varaible stroage list from BFV
        //
        Status = BuildVariableList(
                       &NewStorageListHead,
                       &mBfvVarListEntry,
                       Storage->DefaultId[0],
                       Storage->PlatformId[0],
                       FALSE,
                       READ
                     );

        if (EFI_ERROR (Status)) {
          DestroyAllStorage (&NewStorageListHead);
          return EFI_ABORTED;
        }
        if (IsListEmpty (&NewStorageListHead)) {
          continue;
        }
        Status = PrintInfoInAllFormset (&mFormSetListEntry, &NewStorageListHead);
        if (EFI_ERROR (Status)) {
          DestroyAllStorage (&NewStorageListHead);
          return EFI_ABORTED;
        }
        DestroyAllStorage (&NewStorageListHead);
        StorageLink = GetNextNode (&mBfvVarListEntry, StorageLink);
      }
    } else {
      for (DefaultIndex = 0; DefaultIndex < mMultiPlatformParam.DefaultIdNum; DefaultIndex++) {
        for (PlatformIndex = 0; PlatformIndex < mMultiPlatformParam.PlatformIdNum; PlatformIndex++) {
          InitializeListHead(&NewStorageListHead);
          if (BfvOverried) {
            //
            // Use the varaible stroage list from BFV
            //
            Status = BuildVariableList(
                       &NewStorageListHead,
                       &mBfvVarListEntry,
                       mMultiPlatformParam.DefaultId[DefaultIndex],
                       mMultiPlatformParam.PlatformId[PlatformIndex],
                       FALSE,
                       READ
                     );
           } else {
             //
             // Use the varaible storage list from IFR
             //
             Status = BuildVariableList(
                       &NewStorageListHead,
                       &mAllVarListEntry,
                       mMultiPlatformParam.DefaultId[DefaultIndex],
                       mMultiPlatformParam.PlatformId[PlatformIndex],
                       FALSE,
                       READ
                     );
           }
          if (EFI_ERROR (Status)) {
            DestroyAllStorage (&NewStorageListHead);
            return EFI_ABORTED;
          }
          if (IsListEmpty (&NewStorageListHead)) {
            continue;
          }
          Status = PrintInfoInAllFormset (&mFormSetListEntry, &NewStorageListHead);
          if (EFI_ERROR (Status)) {
            DestroyAllStorage (&NewStorageListHead);
            return EFI_ABORTED;
          }
          DestroyAllStorage (&NewStorageListHead);
        }
      }
    }
  } else {
    Status = PrintInfoInAllFormset (&mFormSetListEntry, &mAllVarListEntry);
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  }

  return Status;
}

/**
  Update the HII setup value.

  Read the Config information from config file, and then compare it with the current FFS.
  Record the different value to EFI variable.

  @param Fv             the Pointer to the FFS
  @param Length         the length of FFS

  @retval EFI_SUCCESS       It was complete successfully
  @return EFI_ABORTED       An error occurred
**/
static
EFI_STATUS
UpdateCongFile (
  VOID
  )
{
  EFI_STATUS     Status;
  UINT32         Index;
  BOOLEAN        BfvOverried;

  Status         = EFI_SUCCESS;
  BfvOverried    = FALSE;
  Index          = 0;
  //
  // Read all Ifr information to Formset list
  //
  for (Index = 0; (gEfiFdInfo.FfsArray[Index] != NULL) && (gEfiFdInfo.Length[Index] != 0); Index++) {
    Status = ReadAllIfrToFromset (
               gEfiFdInfo.FfsArray[Index],
               gEfiFdInfo.Length[Index]
               );
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  }
  //
  // Read defaultId and platformId
  //
  if (!gEfiFdInfo.ExistNvStoreDatabase) {
    Status = ReadDefaultAndPlatformId (&mFormSetListEntry);
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  }
  //
  // If existed the variable data in BFV, abstract them to a variable list.
  // If not exsited, just skip it.
  //
  if (mMultiPlatformParam.MultiPlatformOrNot) {
    if (gEfiFdInfo.ExistNvStoreDatabase) {
      Status = ReadStorageFromNvStoreDatabase (&mBfvVarListEntry);
    } else {
      Status = ReadStorageFromBfv (&mBfvVarListEntry);
    }
    if (!EFI_ERROR (Status)) {
      BfvOverried = TRUE;
    }
  }
  if (mMultiPlatformParam.MultiPlatformOrNot && BfvOverried) {
    if (mUqiList == NULL) {
      return EFI_SUCCESS;
    }
    Status = CheckValueUpdateList ();
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  } else {
    //
    // Evaluate the default value according to the defaultId and platformId
    //
    if (mUqiList == NULL) {
      Status = EvaluateTheValueInFormset (FALSE);
    } else {
      Status = EvaluateTheValueInFormset (TRUE);
    }
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  }
  //
  // print error information in UQI list
  //
  if (PrintErrorInfo (mUqiList)) {
    return EFI_ABORTED;
  }
  //
  // Output the variable information to BFV in multi-platform mode
  // Or write it to the Nvstrage in general mode
  //
  if (mMultiPlatformParam.MultiPlatformOrNot) {
    if (ExistEfiVarOrNot (&mAllVarListEntry) && Operations == UPDATE) {
      printf ("Error. Please use --remove or --ignore to update the variable storage for an FD with variables in its NvStorage.\n");
      return EFI_ABORTED;
    }
  } else {
    //
    // Sync the data from List data to efi variable.
    //
    Status = EfiVarAndListExchange (FALSE, &mAllVarListEntry);
    if (Status == EFI_OUT_OF_RESOURCES) {
      printf ("Error. There is no available space in efi variable. \n");
      return EFI_ABORTED;
    }
    if (Status == EFI_INVALID_PARAMETER) {
      return EFI_ABORTED;
    }
  }

  PrintUpdateListInfo (mUqiList);

  return Status;
}

/**
  Quick Update the HII setup value.

  Read the Config information from command line directly, and then compare it with the current FFS.
  Record the different value to EFI variable.

  @retval EFI_SUCCESS   It was complete successfully
  @return EFI_ABORTED   An error occurred
**/

EFI_STATUS
QuickUpdateCongFile (
  VOID
  )
{
  EFI_STATUS     Status;
  UINT32         Index;

  Status         = EFI_SUCCESS;
  Index          = 0;

  if (mMultiPlatformParam.MultiPlatformOrNot) {
    printf ("Error. The quick update operation is not supported in multi-platform mode.\n");
    return EFI_ABORTED;
  }
  //
  // Check whether the FD has included the storage FFS
  //
  if (mMultiPlatformParam.ExistStorageFfsInBfv) {
    printf ("Error. Variable storage exists in BFV of Fd. This is generated in multi-platform mode.\n");
    printf ("Error. The quick update operation is not supported in multi-platform mode.\n");
    return EFI_ABORTED;
  }
  //
  // Read all Ifr information to Formset list
  //
  for (Index = 0; (gEfiFdInfo.FfsArray[Index] != NULL) && (gEfiFdInfo.Length[Index] != 0); Index++) {
    Status = ReadAllIfrToFromset (
               gEfiFdInfo.FfsArray[Index],
               gEfiFdInfo.Length[Index]
               );
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  }
  //
  // Evaluate the default value according to the defaultId and platformId
  //
  Status = EvaluateTheValueInFormset (TRUE);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  //
  // print error information in UQI list
  //
  if (PrintErrorInfo (mUqiList)) {
    return EFI_ABORTED;
  }
  //
  // Sync the data from mAllVarListEntry data to efi variable.
  //
  Status = EfiVarAndListExchange (FALSE, &mAllVarListEntry);
  if (Status == EFI_OUT_OF_RESOURCES) {
    printf ("Error. There is no available space in Nvstorage. \n");
    return EFI_ABORTED;
  }
  if (Status == EFI_INVALID_PARAMETER) {
    return EFI_ABORTED;
  }

  PrintUpdateListInfo (mUqiList);

  return Status;
}

/**
  Check the HII setup value.

  Read the Config information from config file, and then compare it with the current FFS.
  Print the different values on screen.

  @retval EFI_SUCCESS       It was complete successfully
  @return EFI_ABORTED       An error occurred
**/
static
EFI_STATUS
CheckCongFile (
  VOID
  )
{
  EFI_STATUS     Status;
  UINT32         Index;
  UINT16         DefaultIndex;
  UINT16         PlatformIndex;
  UINT16         DefaultId;
  UINT64         PlatformId;

  Status         = EFI_SUCCESS;
  Index          = 0;
  DefaultIndex   = 0;
  PlatformIndex  = 0;
  DefaultId      = 0;
  PlatformId     = 0;
  //
  // Read all Ifr information to Formset list
  //
  for (Index = 0; (gEfiFdInfo.FfsArray[Index] != NULL) && (gEfiFdInfo.Length[Index] != 0); Index++) {
    Status = ReadAllIfrToFromset (
               gEfiFdInfo.FfsArray[Index],
               gEfiFdInfo.Length[Index]
               );
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  }
  //
  // Read defaultId and platformId
  //
  Status = ReadDefaultAndPlatformId (&mFormSetListEntry);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  //
  // Read the config data from BFV in multi-platform mode
  //
  if (mMultiPlatformParam.MultiPlatformOrNot) {
    Status = ReadStorageFromBfv (&mAllVarListEntry);
    if (EFI_ERROR (Status)) {
      printf ("Error. No storage variable data exists in BFV.\n");
      return EFI_ABORTED;
    }
  }

  if (mMultiPlatformParam.MultiPlatformOrNot) {
    ScanUqiFullList (mUqiList);

    //
    // Multi-platform mode support
    //
    for (DefaultIndex = 0; DefaultIndex < mMultiPlatformParam.DefaultIdNum; DefaultIndex++) {
      for (PlatformIndex = 0; PlatformIndex < mMultiPlatformParam.PlatformIdNum; PlatformIndex++) {
        DefaultId  = mMultiPlatformParam.DefaultId[DefaultIndex];
        PlatformId = mMultiPlatformParam.PlatformId[PlatformIndex];
        //
        //Only parse one time, if a group of defaultId and platformId which have the same variable
        // Take the first one as a key Id of a group
        //
        if (NoTheKeyIdOfGroup (DefaultId, PlatformId)) {
          continue;
        }

        InitializeListHead(&mVarListEntry);
        Status = BuildVariableList(
                   &mVarListEntry,
                   &mAllVarListEntry,
                   DefaultId,
                   PlatformId,
                   FALSE,
                   VERIFY
                   );
        if (EFI_ERROR (Status)) {
          return EFI_ABORTED;
        }
        if (IsListEmpty (&mVarListEntry)) {
          continue;
        }
        SetUqiParametersMultiMode (mUqiList, DefaultId, PlatformId);
        DestroyAllStorage (&mVarListEntry);
      }
    }
  } else {
    //
    // General mode
    //
    Status = ExtractDefault (
               NULL,
               NULL,
               0,
               0,
               SystemLevel
             );
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
    //
    // If existed the variable in NvStorage, copy them to mVarListEntry.
    // Synchronize the default value from the EFI variable zone to variable list
    //
    Status = EfiVarAndListExchange (TRUE, &mVarListEntry);
    if (Status == EFI_INVALID_PARAMETER) {
      Status = EFI_ABORTED;
      return Status;
    }
    //
    // Update the value from script file
    //
    Status = SetUqiParameters (mUqiList,0, 0);
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
    //
    // Copy Stroage from mVarListEntry to mAllVarListEntry
    //
    Status = BuildVariableList (&mAllVarListEntry, &mVarListEntry, 0, 0, TRUE, VERIFY);
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  }
  PrintVerifiedListInfo (mUqiList);
  return Status;
}

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
  FCE application entry point

  @param  argc     The number of input parameters.
  @param  *argv[]  The array pointer to the parameters.

  @retval  0       The application exited normally.
  @retval  1       An error occurred.
  @retval  2       An error about check occurred.

**/
int
main (
  int       argc,
  char      *argv[]
  )
{
  EFI_STATUS        Status;
  FILE              *OutputFd;
  FILE              *ScriptFile;
  UINTN             BytesWrite;
  UINTN             Index;
  CHAR8             *TemDir;
  BOOLEAN           IsFileExist;
  CHAR8             FullGuidToolDefinition[_MAX_PATH];
  CHAR8             *PathList;
  UINTN             EnvLen;
  CHAR8             *NewPathList;
  UINTN             FileNameIndex;
  CHAR8             *InFilePath;
  BOOLEAN           UqiIsSet;

  Status             = EFI_SUCCESS;
  OutputFd           = NULL;
  ScriptFile         = NULL;
  Operations         = NONE;
  BytesWrite         = 0;
  Index              = 0;
  TemDir             = NULL;
  mFormSetOrderRead  = 0;
  mFormSetOrderParse = 0;
  IsFileExist        = TRUE;
  PathList           = NULL;
  NewPathList        = NULL;
  EnvLen             = 0;
  UqiIsSet           = FALSE;

  TemDir = getcwd (NULL, _MAX_PATH);
  TemDir = realloc (TemDir, _MAX_PATH);
  if (strlen (TemDir) + strlen (OS_SEP_STR) + strlen (TEMP_DIR_NAME) > _MAX_PATH - 1) {
    printf ("The directory is too long \n");
    return FAIL;
  }
  strncat (TemDir, OS_SEP_STR, _MAX_PATH - strlen (TemDir) - 1);
  strncat (TemDir, TEMP_DIR_NAME, _MAX_PATH - strlen (TemDir) - 1);
  memset (&mMultiPlatformParam, 0, sizeof (MULTI_PLATFORM_PARAMETERS));

  SetUtilityName (UTILITY_NAME);
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
  // Save, and then skip filename arg
  //
  mUtilityFilename = argv[0];
  argc--;
  argv++;
  //
  // Get the same path with the application itself
  //
  if (strlen (mUtilityFilename) > _MAX_PATH - 1) {
    Error (NULL, 0, 2000, "Parameter: The input file name is too long", NULL);
    return FAIL;
  }
  strncpy (FullGuidToolDefinition, mUtilityFilename, _MAX_PATH - 1);
  FullGuidToolDefinition[_MAX_PATH - 1] = 0;
  FileNameIndex = strlen (FullGuidToolDefinition);
  while (FileNameIndex != 0) {
    FileNameIndex --;
    if (FullGuidToolDefinition[FileNameIndex] == OS_SEP) {
    FullGuidToolDefinition[FileNameIndex] = 0;
      strcpy (mFullGuidToolDefinitionDir, FullGuidToolDefinition);
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
    return FAIL;
  }
  EnvLen = strlen(PathList);
  NewPathList  = (char *)calloc(
                     strlen (".")
                     + strlen (";")
                     + strlen (mFullGuidToolDefinitionDir)
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
  sprintf (NewPathList, "%s;%s;%s", ".", mFullGuidToolDefinitionDir, PathList);
#else
  sprintf (NewPathList, "%s:%s:%s", ".", mFullGuidToolDefinitionDir, PathList);
#endif

  PathList = NULL;
  free (PathList);

  //
  // Load Guid Tools definition
  //
  InFilePath = SearchConfigFromPathList(NewPathList, mGuidToolDefinition);
  free (NewPathList);
  if (InFilePath != NULL) {
    printf ("\nThe Guid Tool Definition comes from the '%s'. \n", InFilePath);
    mParsedGuidedSectionTools = ParseGuidedSectionToolsFile (InFilePath);
    free (InFilePath);
  } else {
    //
    // Use the pre-defined standard guided tools.
    //
  printf ("\nThe Guid Tool Definition comes from the build-in default configuration. \n");
    mParsedGuidedSectionTools = PreDefinedGuidedTools ();
  }
  //
  // Parse the command line
  //
  strcpy (mSetupTxtName, "NoSetupFile");
  Status = ParseCommmadLine (argc,argv);
  if (EFI_ERROR (Status)) {
    return FAIL;
  }
  //
  // Print utility header
  //
  printf ("\nIntel(R) Firmware Configuration Editor. (Intel(R) %s) Version %d.%d. %s.\n\n",
    UTILITY_NAME,
    UTILITY_MAJOR_VERSION,
    UTILITY_MINOR_VERSION,
    __BUILD_VERSION
    );
  //
  // Check the revision of BfmLib
  //
  Status = CheckBfmLibRevision ();
  if (EFI_ERROR (Status)) {
    printf ("Please use the correct revision of BfmLib %s. \n", __BUILD_VERSION);
    return FAIL;
  }
  if (strcmp (mSetupTxtName, "NoSetupFile")) {
    ScriptFile = fopen (mSetupTxtName, "r");
    if (ScriptFile == NULL) {
      printf ("Error. Cannot open the script file.\n");
      return FAIL;
    }
    Status = PickUpUqiFromScript (ScriptFile);
    if (EFI_ERROR (Status)) {
      fclose (ScriptFile);
      IsFileExist = FALSE;
      goto Done;
    }
    fclose (ScriptFile);
  }
  if (!mMultiPlatformParam.MultiPlatformOrNot
    && (Operations == UPDATE_REMOVE || Operations == UPDATE_IGNORE)
    ) {
    printf ("Error. --remove and --ignore cannot be used in normal mode.\n");
    Status      = FAIL;
    goto Done;
  }

   if (access (TemDir, 0) != -1) {
    LibRmDir (TemDir);
   }

  //
  // Initialize the variables
  //
  Status = PickUpFfsFromFd ();
  if (EFI_ERROR (Status)) {
    printf ("Error. Invalid FD file.\n");
    IsFileExist = FALSE;
    Status      = FAIL;
    goto Done;
  }
  if (gEfiFdInfo.FfsArray[0] == NULL) {
    printf ("Error. Cannot find any HII offset in current FD files, please check the BaseTools.\n");
    Status  = FAIL;
    goto Done;
  }
  //
  //Config the global variables
  //
  if (mMultiPlatformParam.Uqi.Data != NULL) {
    UqiIsSet = TRUE;
  }
  Status = GetEfiVariablesAddr (UqiIsSet);
  if (EFI_ERROR (Status)) {
    printf ("Error. Cannot locate the EFI variable zone in FD.\n");
    Status = FAIL;
    goto Done;
  }
  if (gEfiFdInfo.ExistNvStoreDatabase && !mMultiPlatformParam.MultiPlatformOrNot) {
    mMultiPlatformParam.MultiPlatformOrNot = TRUE;
  }
  //
  // Initialize the FormSet and VarList List
  //
  InitializeListHead (&mFormSetListEntry);
  InitializeListHead (&mVarListEntry);
  InitializeListHead (&mBfvVarListEntry);
  InitializeListHead (&mAllVarListEntry);

  mStringBuffer = malloc (mMaxCount);
  if (mStringBuffer == NULL) {
    printf ("Fali to allocate memory!\n");
    Status = FAIL;
    goto Done;
  }

  //
  // Decide how to deal with the Fd
  //
  switch (Operations) {

  case READ:
    printf ("\nStart the Read Mode:\n");
    Status = ReadCongFile ();
    if (EFI_ERROR (Status)) {
      Status = FAIL;
    }
    break;

  case UPDATE:
  case UPDATE_REMOVE:
  case UPDATE_IGNORE:
    printf ("\nStart the Update Mode:\n");
    Status = UpdateCongFile ();
    if (EFI_ERROR (Status)) {
      Status = FAIL;
    }
    break;

  case VERIFY:
    printf ("\nStart the Verify Mode:\n");
    Status = CheckCongFile ();
    if (EFI_ERROR (Status)) {
      Status = VR_FAIL;
    }
    break;

  case UPDATEQ:
    printf ("\nStart the Update Quick Mode:\n");
    Status = QuickUpdateCongFile ();
    if (EFI_ERROR (Status)) {
      Status = FAIL;
    }
    break;

  default:
    break;
  }

  if (mCount > 0) {
    mStringBuffer[mCount] = '\0';
    fwrite (mStringBuffer, sizeof (CHAR8), mCount, stdout);
  }
  free (mStringBuffer);

  if (Status != SUCCESS) {
    goto Done;
  }
  //
  // If multi-platform mode, insert the variables to BFV
  //
  if (mMultiPlatformParam.MultiPlatformOrNot
    && (IsListEmpty (&mAllVarListEntry) == FALSE)
    &&((Operations == UPDATE) || (Operations == UPDATE_REMOVE) || (Operations == UPDATE_IGNORE) || (Operations == UPDATEQ))
    ) {
    IsFileExist = FALSE;
    if (gEfiFdInfo.ExistNvStoreDatabase) {
      Status = InsertBinaryToNvStoreDatabase (mInputFdName, mOutputFdName, &mAllVarListEntry);
    } else {
      Status = InsertBinaryToBfv (mInputFdName, mOutputFdName, &mAllVarListEntry);
    }
    if (EFI_ERROR (Status)) {
      goto Done;
    }
    //
    // Remove the variables in NvStorage in multi-platform mode by user specified requirement
    //
    if (Operations == UPDATE_REMOVE) {
      if (gEfiFdInfo.Fd != NULL) {
        free (gEfiFdInfo.Fd);
      }
      gEfiFdInfo.Fd = ReadFileToMemory (mOutputFdName, &gEfiFdInfo.FdSize);
      if (gEfiFdInfo.Fd == NULL) {
        Status = EFI_ABORTED;
      } else {
        Status = RemoveEfiVar (&mAllVarListEntry);
      }
      if (EFI_ERROR (Status)) {
        printf ("Error. Failed to remove the variable from NVRAM.\n");
        Status = FAIL;
        goto Done;
      }
    }
  }

  if (
  (!mMultiPlatformParam.MultiPlatformOrNot &&((Operations == UPDATE) || (Operations == UPDATEQ)))
  || (mMultiPlatformParam.MultiPlatformOrNot && (Operations == UPDATE_REMOVE || ((Operations == UPDATE) && IsListEmpty (&mAllVarListEntry))))
    ) {
    OutputFd = fopen (mOutputFdName, "wb+");
    if (OutputFd == NULL) {
      printf ("Error. Failed to create the output FD file.\n");
      Status = FAIL;
      goto Done;
    }
    fseek (OutputFd, 0, SEEK_SET);
    BytesWrite = fwrite (gEfiFdInfo.Fd, sizeof (CHAR8), gEfiFdInfo.FdSize, OutputFd);
    fclose (OutputFd);
    if (BytesWrite != gEfiFdInfo.FdSize) {
      printf ("Error. Failed to create the FD image. \n");
      Status = FAIL;
      goto Done;
    }
  }
  if ((Operations == UPDATE) || (Operations == UPDATE_REMOVE) || (Operations == UPDATE_IGNORE) || (Operations == UPDATEQ)) {
    printf ("\nCongratulations. The output Fd file '%s' has been completed successfully.\n", mOutputFdName);
  }
Done:
  //
  // Delete the temporary directory and files
  //
  if (IsFileExist) {
    LibRmDir (TemDir);
  }
  //
  // Clean up
  //
  if (gEfiFdInfo.Fd != NULL) {
    free (gEfiFdInfo.Fd);
  }

  if (mMultiPlatformParam.Uqi.Value != NULL) {
    free (mMultiPlatformParam.Uqi.Value);
  }
  if (mMultiPlatformParam.Uqi.Data != NULL) {
    free (mMultiPlatformParam.Uqi.Data);
  }
  while (gEfiFdInfo.FfsArray[Index] != NULL) {
    free (gEfiFdInfo.FfsArray[Index++]);
  }

  DestroyAllFormSet (&mFormSetListEntry);
  DestroyAllStorage (&mVarListEntry);
  DestroyAllStorage (&mBfvVarListEntry);
  DestroyAllStorage (&mAllVarListEntry);
  FreeUnidirectionList (mUqiList);

  return Status;
}

