/** @file

 Parser for IFR binary encoding.

 Copyright (c) 2011-2019, Intel Corporation. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "IfrParse.h"

#ifndef EDKII_IFR_BIT_VARSTORE_GUID
#define EDKII_IFR_BIT_VARSTORE_GUID \
  {0x82DDD68B, 0x9163, 0x4187, {0x9B, 0x27, 0x20, 0xA8, 0xFD, 0x60 ,0xA7, 0x1D}}
#endif

#ifndef EDKII_IFR_NUMERIC_SIZE_BIT
#define EDKII_IFR_NUMERIC_SIZE_BIT  0x3F
#endif

UINT16           mStatementIndex;
UINT16           mExpressionOpCodeIndex;

BOOLEAN          mInScopeSubtitle;
BOOLEAN          mInScopeSuppress;
BOOLEAN          mInScopeGrayOut;
BOOLEAN          mInScopeDisable;
FORM_EXPRESSION  *mSuppressExpression;
FORM_EXPRESSION  *mGrayOutExpression;
FORM_EXPRESSION  *mDisableExpression;

extern MULTI_PLATFORM_PARAMETERS   mMultiPlatformParam;
extern LIST_ENTRY                  mVarListEntry;
extern LIST_ENTRY                  mFormSetListEntry;
extern UINT32                      mFormSetOrderParse;

#define FORM_SET_GUID_PREFIX    "Form Set GUID: "
#define EFI_GUID_FORMAT         "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"

UINT32 mMaxCount = 0x100000;
UINT32 mCount = 0;
CHAR8  *mStringBuffer = NULL;
static EFI_GUID gEdkiiIfrBitVarGuid = EDKII_IFR_BIT_VARSTORE_GUID;

/**
  Produces a Null-terminated ASCII string in mStringBuffer based on a Null-terminated
  ASCII format string and variable argument list.

  @param  FormatString    A null-terminated ASCII format string.
  @param  ...             The variable argument list whose contents are accessed based on the
                          format string specified by FormatString.
**/
VOID
StringPrint (
  CHAR8 *FormatString,
  ...
)
{
  va_list Marker;
  INT32   Count;

  va_start (Marker, FormatString);
  Count = vsprintf (mStringBuffer + mCount, FormatString, Marker);
  mCount = mCount + Count;
  va_end (Marker);
  if (mCount + 0x400 > mMaxCount) {
    mStringBuffer[mCount] = '\0';
    fwrite (mStringBuffer, sizeof (CHAR8), mCount, stdout);
    mCount = 0;
  }
}

/**
  Print the information of questions.

  @param  FormSet     The pointer to the formset.
  @param  FormSet     The pointer to the form.
  @param  Question    The pointer to the question.
  @param  PrintOrNot  Decide whether print or not.

  @return NULL.

**/
static
VOID
PrintQuestion (
  IN  FORM_BROWSER_FORMSET    *FormSet,
  IN  FORM_BROWSER_FORM       *Form,
  IN  FORM_BROWSER_STATEMENT  *Question,
  IN  BOOLEAN                  PrintOrNot
  );

/**
  Writes a Unicode string specified by iStringToken and iLanguage to the script file (converted to ASCII).

  @param  Package        A pointer to the Unicode string.

  @return NULL

**/
static
VOID
LogUnicodeString (
  IN     CHAR16              *pcString
  )
{
  UINTN  Index;

  if (pcString == NULL) {
    return;
  }
  //
  // replace the 0x0D to 0x20, because if the pcString has 0D 0A, then after print it,
  // different editor may have different format
  //
  for (Index = 0; pcString[Index] != 0; Index++) {
    if (pcString[Index] == 0x0D) {
       pcString[Index] = 0x20;
    }

    StringPrint("%c", pcString[Index] & 0x00FF);
  }
}

/**
  Writes a UQIL Unicode string specified by iStringToken to the script file as an array of 16-bit integers in ASCII.

  @param  Package        A pointer to the Unicode string.

  @return NULL

**/
static
VOID
LogUqi (
  IN     CHAR16              *pcString
  )
{
  UINT16         Index;
  //
  //  Find the UNICODE string length (in CHAR16)
  //
  for (Index = 0; pcString[Index] != 0; Index++);
  //
  //  Write each word as a hex integer
  //
  for (Index = 0; pcString[Index] != 0; Index++) {
    StringPrint("%04X ", pcString[Index]);
  }
}

/**
  Get the question value with bit field from the buffer.

  @param  Question        The question refer to bit field.
  @param  Buffer          The buffer which the question value get from.
  @param  Value           Retun the value.

**/
VOID
GetBitsQuestionValue(
  IN  FORM_BROWSER_STATEMENT *Question,
  IN  UINT8                  *Buffer,
  OUT UINT32                 *Value
  )
{
  UINT32        PreBits;
  UINT32        Mask;

  PreBits = Question->BitVarOffset - Question->VarStoreInfo.VarOffset * 8;
  Mask = (1<< Question->BitStorageWidth) -1;

  *Value = *(UINT32*)Buffer;
  (*Value) >>= PreBits;
  (*Value) &= Mask;
}

/**
  Set the question value with bit field to the buffer.

  @param  Question        The question refer to bit field.
  @param  Buffer          The buffer which the question value set to.
  @param  Value           The value need to set.

**/
VOID
SetBitsQuestionValue (
  IN     FORM_BROWSER_STATEMENT *Question,
  IN OUT UINT8                  *Buffer,
  IN     UINT32                 Value
  )
{
  UINT32        PreBits;
  UINT32        Mask;
  UINT32        TmpValue;

  PreBits = Question->BitVarOffset - Question->VarStoreInfo.VarOffset * 8;
  Value <<= PreBits;
  Mask = (1<< Question->BitStorageWidth) -1;
  Mask <<= PreBits;

  TmpValue = *(UINT32*)(Buffer);
  TmpValue = (TmpValue & (~Mask)) | Value;
  CopyMem ((UINT32*)Buffer, &TmpValue, sizeof (UINT32));
}

/**
  Print the current value of the specified question.

  @param  Question       The pointer to question

  @return EFI_SUCCESS
**/
static
EFI_STATUS
LogIfrValue (
  IN  FORM_BROWSER_FORMSET     *FormSet,
  IN  FORM_BROWSER_STATEMENT   *Question
  )
{
  EFI_STATUS          Status;
  FORMSET_STORAGE     *VarList;
  UINT8               *VarBuffer;
  UINT32              Value;

  VarBuffer    = NULL;

  Status = SearchVarStorage (
             Question,
             NULL,
             Question->VarStoreInfo.VarOffset,
             FormSet->StorageListHead,
             (CHAR8 **)&VarBuffer,
             &VarList
             );

  if (EFI_ERROR (Status)) {
    StringPrint("\nCouldn't read current variable data.");
    return EFI_ABORTED;
  }
  //
  //  Log the Value
  //
  if (
    (Question->VarStoreInfo.VarOffset > VarList->Size)   \
    && (VarList->Type == EFI_IFR_VARSTORE_OP)
    ) {
    StringPrint("0000 // Offset larger than Variable Size -- FAILURE");
  } else if (
    (Question->VarStoreInfo.VarOffset > VarList->Size) \
    && (VarList->Type == EFI_IFR_VARSTORE_EFI_OP)       \
    && VarList->NewEfiVarstore
    ) {
    StringPrint("0000 // Offset larger than Variable Size -- FAILURE");
  }  else if (
    (Question->StorageWidth > VarList->Size)           \
    && (VarList->Type == EFI_IFR_VARSTORE_EFI_OP)      \
    && !VarList->NewEfiVarstore
    ) {
    StringPrint("0000 // Offset larger than Variable Size -- FAILURE");
  } else {
    if (Question->QuestionReferToBitField) {
      GetBitsQuestionValue (Question, VarBuffer, &Value);
      VarBuffer = (UINT8*)(&Value);
    }
    switch (Question->StorageWidth) {

    case sizeof (UINT8):
      StringPrint("%02X", (*(UINT8 *)VarBuffer) & 0xFF);
      break;

    case sizeof (UINT16):
      StringPrint("%04X", (*(UINT16 *)VarBuffer) & 0xFFFF);
      break;

    case sizeof (UINT32):
      StringPrint("%08X", (*(UINT32 *)VarBuffer) & 0xFFFFFFFF);
      break;

    case sizeof (UINT64):
      StringPrint("%016llX", *((UINT64 *)VarBuffer));
      break;

    default:
      StringPrint("0000 // Width > 16 is not supported -- FAILURE");
      break;
    }
  }
  return EFI_SUCCESS;
}

/**
  Print the current value of the STRING question.

  @param  Question       The pointer to question

  @return EFI_SUCCESS
**/
static
EFI_STATUS
LogIfrValueStr (
  IN  FORM_BROWSER_FORMSET     *FormSet,
  IN  FORM_BROWSER_STATEMENT   *Question
  )
{
  EFI_STATUS          Status;
  FORMSET_STORAGE     *VarList;
  UINT8               *VarBuffer;

  VarBuffer = NULL;
  Status = SearchVarStorage (
             Question,
             NULL,
             Question->VarStoreInfo.VarOffset,
             FormSet->StorageListHead,
             (CHAR8 **)&VarBuffer,
             &VarList
             );

  if (EFI_ERROR (Status)) {
    StringPrint("\nCouldn't read current variable data.");
    return EFI_ABORTED;
  }

  //
  //  Log the Value
  //
  if (
    (Question->VarStoreInfo.VarOffset > VarList->Size)   \
    && (VarList->Type == EFI_IFR_VARSTORE_OP)
    ) {
    StringPrint("0000 // Offset larger than Variable Size -- FAILURE");
  } else if (
    (Question->VarStoreInfo.VarOffset > VarList->Size) \
    && (VarList->Type == EFI_IFR_VARSTORE_EFI_OP)       \
    && VarList->NewEfiVarstore
    ) {
    StringPrint("0000 // Offset larger than Variable Size -- FAILURE");
  }  else if (
    (Question->StorageWidth > VarList->Size)           \
    && (VarList->Type == EFI_IFR_VARSTORE_EFI_OP)      \
    && !VarList->NewEfiVarstore
    ) {
    StringPrint("0000 // Offset larger than Variable Size -- FAILURE");
  } else {
      StringPrint("\"");
      LogUnicodeString((CHAR16 *)VarBuffer);
      StringPrint("\"");
  }
  return EFI_SUCCESS;
}

/**
  Print the current values of an Ordered List question.

  @param  Question       The pointer to question
  @param  MaxEntries       The max number of options
  @param  VarList          The dual pointer to the Node of VarList

  @return EFI_SUCCESS
**/
static
EFI_STATUS
LogIfrValueList (
  IN  FORM_BROWSER_FORMSET     *FormSet,
  IN  FORM_BROWSER_STATEMENT   *Question
  )
{
  EFI_STATUS          Status;
  FORMSET_STORAGE     *VarList;
  UINT8               CurrentEntry;
  UINT8               *VarBuffer;

  CurrentEntry = 0;
  VarBuffer    = NULL;

  Status = SearchVarStorage (
             Question,
             NULL,
             Question->VarStoreInfo.VarOffset,
             FormSet->StorageListHead,
             (CHAR8 **)&VarBuffer,
             &VarList
             );

  if (EFI_ERROR (Status)) {
    StringPrint("\nCouldn't read current variable data.");
    return EFI_ABORTED;
  }
  //
  // Log the value
  //
  if (
    ((Question->VarStoreInfo.VarOffset +  Question->MaxContainers) > VarList->Size)   \
    && (VarList->Type == EFI_IFR_VARSTORE_OP)
    ) {
    StringPrint("0000 // Offset larger than Variable Size -- FAILURE");
  } else if (
    (Question->MaxContainers > VarList->Size) \
    && (VarList->Type == EFI_IFR_VARSTORE_EFI_OP)
    ) {
    StringPrint("0000 // Offset larger than Variable Size -- FAILURE");
  } else {
    for (CurrentEntry = 0; CurrentEntry < Question->MaxContainers; CurrentEntry++) {

      switch (Question->StorageWidth/Question->MaxContainers){

      case 1:
        StringPrint("%02X ", VarBuffer[CurrentEntry]);
        break;

      case 2:
        StringPrint("%04X ", *((UINT16 *)VarBuffer + CurrentEntry));
        break;

      case 4:
        StringPrint("%08X ", *((UINT32 *)VarBuffer + CurrentEntry));
        break;

      case 8:
        StringPrint("%016llX ", *((UINT64 *)VarBuffer + CurrentEntry));
        break;

      default:
        StringPrint("%02X ", VarBuffer[CurrentEntry]);
      }
    }
  }
  return EFI_SUCCESS;
}

/**
  Compare two Uqi parameters

  @param UqiParm1       The pointer to the first Uqi parameter.
  @param UqiParm2       The pointer to the second Uqi parameter.

  @retval TRUE          If these two Uqi parameters are the same, return TRUE;
  @return FALSE         Otherwise, return FALSE;
**/
BOOLEAN
CompareUqiHeader (
  IN  CONST UQI_HEADER  *UqiParm1,
  IN  CONST UQI_HEADER  *UqiParm2
  )
{
  INT32    Index;

  if (UqiParm1->HexNum != UqiParm2->HexNum) {
    return FALSE;
  }

  for (Index = UqiParm1->HexNum - 1; Index >= 0; Index--) {
    if (UqiParm1->Data[Index] != UqiParm2->Data[Index]) {
      return FALSE;
    }
  }

  return TRUE;
}


/**
  Check whether existed a same variable in the LIST_ENTRY.

  @param  CurVarList        A pointer to a variable node.

  @return Pointer          If existed the same variable, return the pointer to the Node.
  @return NULL              Otherwise, return FALSE

**/
static
FORMSET_STORAGE *
NotSameVariableInVarList (
  IN  LIST_ENTRY         *VariableListEntry,
  IN  FORMSET_STORAGE    *StorageNode
  )
{
  FORMSET_STORAGE    *CurNode;
  LIST_ENTRY         *Link;
  LIST_ENTRY         *StorageListHead;

  StorageListHead       =  VariableListEntry;
  CurNode               = NULL;

  //
  // Find Storage for this Question
  //
  Link = GetFirstNode (StorageListHead);
  while (!IsNull (StorageListHead, Link)) {
    CurNode = FORMSET_STORAGE_FROM_LINK (Link);

    if (!CompareGuid (&StorageNode->Guid, &CurNode->Guid)   \
      && (CurNode->Name != NULL)                            \
      && (StorageNode->Name != NULL)                        \
      && !FceStrCmp (StorageNode->Name, CurNode->Name)         \
      && (StorageNode - CurNode != 0)
      ) {
      //
      // If not multi-plaform support mode, take VarStore as the EfiVarStore. So If there are
      // two variables with same guid same name, but different type, we will take as the same
      // in general mode
      //
      if (mMultiPlatformParam.MultiPlatformOrNot && (CurNode->Type == StorageNode->Type)) {
        //
        // If matched, get the address of EFI_IFR_VARSTORE data.
        //
        return CurNode;
        break;
      } else if (!mMultiPlatformParam.MultiPlatformOrNot) {
        return CurNode;
        break;
      }
    }
    Link = GetNextNode (StorageListHead, Link);
  }
  return NULL;
}

/**
  Get the UniString by the offset.

  @param  UniPackge         A pointer to the beginning of Null-terminated Unicode string Array.
  @param  CurUniPackge      A pointer to the current position of Null-terminated Unicode string Array.
  @param  VarDefaultNameId  The string ID.
  @param  CurOrDefaultLang  Use the current language or the default language.
  @param  VarDefaultName    return the name string.

  @return EFI_SUCCESS
  @return EFI_INVALID_PARAMETER
  @return EFI_NOT_FOUND
**/
static
EFI_STATUS
GetStringByOffset (
  IN     UINT8               *UniPackge,
  IN     UINT8               *CurUniPackge,
  IN     UINT16              VarDefaultNameId,
  IN     BOOLEAN             CurOrDefaultLang,
  IN OUT CHAR16              **VarDefaultName
  )
{
  UINT8                          *HiiStringHeader;
  UINT32                         Offset;
  UINT32                         Count;
  UINT32                         Index;
  EFI_HII_STRING_BLOCK           *Block;
  VOID                           *ThisBlock;

  assert ((UniPackge != NULL) && (CurUniPackge != NULL));

  HiiStringHeader  = NULL;
  Offset           = 1;
  Count            = 0;
  Block            = NULL;
  ThisBlock        = NULL;

  if (VarDefaultNameId == 0) {
    return EFI_INVALID_PARAMETER;
  }
  if (CurOrDefaultLang) {
    HiiStringHeader = CurUniPackge;
  } else {
    HiiStringHeader  = UniPackge + 4;
  }

  Block = (EFI_HII_STRING_BLOCK *)((UINT8*)HiiStringHeader + ((EFI_HII_STRING_PACKAGE_HDR *)HiiStringHeader)->HdrSize);
  //
  // Search the matched String in specificated language package by the Offset
  //
  while( Block->BlockType != EFI_HII_SIBT_END ) {
    switch (Block->BlockType) {
    case EFI_HII_SIBT_STRING_SCSU:
      ThisBlock = (VOID *)Block;
      for (Index = 0 ; ((EFI_HII_SIBT_STRING_SCSU_BLOCK *)ThisBlock)->StringText[Index] != 0 ; Index++) ;
      Block = (EFI_HII_STRING_BLOCK *)&((EFI_HII_SIBT_STRING_SCSU_BLOCK *)ThisBlock)->StringText[Index + 1];
      break;

    case EFI_HII_SIBT_STRING_SCSU_FONT:
      ThisBlock = (VOID *)Block;
      for (Index = 0 ; ((EFI_HII_SIBT_STRING_SCSU_FONT_BLOCK *)ThisBlock)->StringText[Index] != 0 ; Index++) ;
      Block = (EFI_HII_STRING_BLOCK *)(((EFI_HII_SIBT_STRING_SCSU_FONT_BLOCK *)ThisBlock) + 1);
      break;

    case EFI_HII_SIBT_STRINGS_SCSU:
      ThisBlock = (VOID *)Block;
      for( Count= ((EFI_HII_SIBT_STRINGS_SCSU_BLOCK *)ThisBlock)->StringCount, Index = 0 ; Count; Count--, Index++ ) {
        for ( ; ((EFI_HII_SIBT_STRINGS_SCSU_BLOCK *)ThisBlock)->StringText[Index] != 0 ; Index++) ;
      }
      Block = (EFI_HII_STRING_BLOCK *)&((EFI_HII_SIBT_STRINGS_SCSU_BLOCK *)ThisBlock)->StringText[Index];
      break;

    case EFI_HII_SIBT_STRINGS_SCSU_FONT:
      ThisBlock = (VOID *)Block;
      for( Count = ((EFI_HII_SIBT_STRINGS_SCSU_FONT_BLOCK *)ThisBlock)->StringCount, Index = 0 ; Count; Count--, Index++ ) ;
      Block = (EFI_HII_STRING_BLOCK *) & ((EFI_HII_SIBT_STRINGS_SCSU_FONT_BLOCK *)ThisBlock)->StringText[Index];
      break;

    case EFI_HII_SIBT_STRING_UCS2:
      ThisBlock = (VOID *)Block;
      if (Offset == VarDefaultNameId)  {
      *VarDefaultName = malloc ((FceStrLen ((CHAR16 *) ((EFI_HII_SIBT_STRING_UCS2_BLOCK *)ThisBlock)->StringText) + 1) * sizeof (CHAR16));
      if (*VarDefaultName == NULL) {
        printf ("Fail to allocate memory");
        return EFI_OUT_OF_RESOURCES;
      }
      memset (
        *VarDefaultName,
        0,
        (FceStrLen ((CHAR16 *) ((EFI_HII_SIBT_STRING_UCS2_BLOCK *)ThisBlock)->StringText) + 1) * sizeof (CHAR16)
      );
      StrCpy (
        *VarDefaultName,
        (CHAR16 *) ((EFI_HII_SIBT_STRING_UCS2_BLOCK *) ThisBlock)->StringText
      );
      return EFI_SUCCESS;
    }

      for (Index = 0 ; ((EFI_HII_SIBT_STRING_UCS2_BLOCK *)ThisBlock)->StringText[Index] != 0 ; Index++) ;
      Block = (EFI_HII_STRING_BLOCK *) & ((EFI_HII_SIBT_STRING_UCS2_BLOCK *) ThisBlock)->StringText[Index + 1];
      Offset += 1;
      break;

    case EFI_HII_SIBT_STRING_UCS2_FONT:
      ThisBlock = (VOID *)Block;
      for (Index = 0 ; ((EFI_HII_SIBT_STRING_UCS2_FONT_BLOCK *) ThisBlock)->StringText[Index] != 0 ; Index++) ;
      Block = (EFI_HII_STRING_BLOCK *)& ((EFI_HII_SIBT_STRING_UCS2_FONT_BLOCK *) ThisBlock)->StringText[Index + 1];
      break;

    case EFI_HII_SIBT_STRINGS_UCS2:
      ThisBlock = (VOID *)Block;
      for( Count = ((EFI_HII_SIBT_STRINGS_UCS2_BLOCK *)ThisBlock)->StringCount, Index = 0 ; Count; Count--, Index++ ) {
        for ( ; ((EFI_HII_SIBT_STRINGS_UCS2_BLOCK *) ThisBlock)->StringText[Index] != 0 ; Index++) ;
      }
      Block = (EFI_HII_STRING_BLOCK *) & ((EFI_HII_SIBT_STRINGS_UCS2_BLOCK *) ThisBlock)->StringText[Index];
      break;

    case EFI_HII_SIBT_STRINGS_UCS2_FONT:
      ThisBlock = (VOID *)Block;
      for( Count= ((EFI_HII_SIBT_STRINGS_UCS2_FONT_BLOCK *) ThisBlock)->StringCount, Index = 0 ; Count ; Count--, Index++ ) {
        for ( ; ((EFI_HII_SIBT_STRINGS_UCS2_FONT_BLOCK *) ThisBlock)->StringText[Index] != 0 ; Index++) ;
      }
      Block = (EFI_HII_STRING_BLOCK *) & ((EFI_HII_SIBT_STRINGS_UCS2_FONT_BLOCK *)ThisBlock)->StringText[Index];
      break;

     case EFI_HII_SIBT_DUPLICATE:
       ThisBlock = (VOID *)Block;
       Block = (EFI_HII_STRING_BLOCK *)((EFI_HII_SIBT_DUPLICATE_BLOCK *) ThisBlock + 1);
       break;

     case EFI_HII_SIBT_SKIP2:
       ThisBlock = (VOID *)Block;
       Offset += ((EFI_HII_SIBT_SKIP2_BLOCK *) ThisBlock)->SkipCount;
       Block = (EFI_HII_STRING_BLOCK *)( (EFI_HII_SIBT_SKIP2_BLOCK *) ThisBlock + 1);
       break;

     case EFI_HII_SIBT_SKIP1:
       ThisBlock = (VOID *)Block;
       Offset += ((EFI_HII_SIBT_SKIP1_BLOCK *) ThisBlock)->SkipCount;
       Block = (EFI_HII_STRING_BLOCK *)((EFI_HII_SIBT_SKIP1_BLOCK *)ThisBlock + 1);
       break;

     case EFI_HII_SIBT_EXT1:
       ThisBlock = (VOID *)Block;
       Block = (EFI_HII_STRING_BLOCK *)((UINT8*)ThisBlock + ((EFI_HII_SIBT_EXT1_BLOCK *) ThisBlock)->Length);
       break;

     case EFI_HII_SIBT_EXT2:
       ThisBlock = (VOID *)Block;
       Block = (EFI_HII_STRING_BLOCK *)((UINT8*)ThisBlock + ((EFI_HII_SIBT_EXT2_BLOCK *) ThisBlock)->Length);
       break;

     case EFI_HII_SIBT_EXT4:
       ThisBlock = (VOID *)Block;
       Block = (EFI_HII_STRING_BLOCK *)((UINT8*)ThisBlock + ((EFI_HII_SIBT_EXT4_BLOCK *)ThisBlock)->Length);
       break;

     case EFI_HII_SIBT_FONT:
       ThisBlock = (VOID *)Block;
       for (Index = 0 ; ((EFI_HII_SIBT_FONT_BLOCK *) ThisBlock)->FontName[Index] != 0 ; Index++) ;
       Block = (EFI_HII_STRING_BLOCK *)& ((EFI_HII_SIBT_FONT_BLOCK *) ThisBlock)->FontName[Index + 1];
       break;

     default:
       StringPrint("Unhandled type = 0x%x\n", Block->BlockType);
     }
   }

  return EFI_NOT_FOUND;
}

/**
  Parse the UniString to get the string information.

  @param  CachedStringList  A pointer to cached string list
  @param  UniPackge         A pointer to a Null-terminated Unicode string Array.
  @param  VarDefaultNameId  The string ID.
  @param  Language          The language, en-US UQI or eng.
  @param  VarDefaultName    return the name string.

  @return EFI_SUCCESS       If get the name string successfully
  @return EFI_NOT_FOUND       An error occurred.
  @return EFI_INVALID_PARAMETER

**/
EFI_STATUS
FindDefaultName (
  IN     FORMSET_STRING_LIST *CachedStringList,
  IN     UINT8               *UniPackge,
  IN     UINT16              VarDefaultNameId,
  IN     LANGUAGE            Language,
  IN OUT CHAR16              **VarDefaultName
  )
{
  CHAR8          *UniPackgeEnd;
  CHAR8          *UniBin;
  CHAR8          LangStr[10];
  BOOLEAN        IsFound;
  EFI_STATUS     Status;
  EFI_STRING_ID  Index;
  STRING_INFO    *TempBuffer;

  UniBin     = NULL;
  IsFound    = FALSE;
  Status     = EFI_NOT_FOUND;

  UniBin       = (CHAR8 *) UniPackge + 4;
  UniPackgeEnd = (CHAR8 *) UniPackge + *(UINT32 *)UniPackge;

  //
  //Handle with the invalid usage "STRING_TOKEN(0)"
  //
  if (VarDefaultNameId == 0) {
    *VarDefaultName = L"";
    return EFI_SUCCESS;
  }

  if (CachedStringList != NULL) {
    for (Index = 0; Index < CachedStringList->CachedIdNum; Index ++) {
      if (VarDefaultNameId == CachedStringList->StringInfoList[Index].StringId) {
        *VarDefaultName = CachedStringList->StringInfoList[Index].String;
        return EFI_SUCCESS;
      }
    }
  }

  switch (Language) {

  case UQI:
    strcpy (LangStr, "uqi");
    break;

  case EN_US:
    strcpy (LangStr, "en-US");
    break;

  case ENG:
    strcpy (LangStr, "eng");
    break;

  default:
    strcpy (LangStr, "en-US");
    break;
  }
  IsFound    = FALSE;

  if (((EFI_HII_PACKAGE_HEADER *)UniBin)->Type == EFI_HII_PACKAGE_STRINGS) {
    //
    // Search the specified language package
    //
    while ((UniBin < UniPackgeEnd) && ((EFI_HII_PACKAGE_HEADER *)UniBin)->Type == EFI_HII_PACKAGE_STRINGS) {
      if (strcmp (((EFI_HII_STRING_PACKAGE_HDR *)UniBin)->Language, LangStr) == 0) {
        IsFound = TRUE;
        break;
      }
      UniBin += ((EFI_HII_STRING_PACKAGE_HDR *)UniBin)->Header.Length;
    }
    //
    //If not find the string ID, use the en eng or en-US instead.
    //
    if (!IsFound) {
      UniBin     = (CHAR8 *) UniPackge + 4;

      while ((UniBin < UniPackgeEnd) && ((EFI_HII_PACKAGE_HEADER *)UniBin)->Type == EFI_HII_PACKAGE_STRINGS) {
        if ((strcmp (((EFI_HII_STRING_PACKAGE_HDR *)UniBin)->Language, "en") == 0)    \
          || (strcmp (((EFI_HII_STRING_PACKAGE_HDR *)UniBin)->Language, "en-US") == 0) \
          || (strcmp (((EFI_HII_STRING_PACKAGE_HDR *)UniBin)->Language, "eng") == 0)   \
          ) {
          IsFound = TRUE;
          break;
        }
        UniBin += ((EFI_HII_STRING_PACKAGE_HDR *)UniBin)->Header.Length;
      }
    }
     //
     //If not still find the string ID, use the first one instead.
     //
     Status = GetStringByOffset (
                UniPackge,
                (UINT8 *)UniBin,
                VarDefaultNameId,
                IsFound,
                VarDefaultName
                );
     if (!EFI_ERROR (Status)) {
       goto Done;
     }
    //
    //If not find the specified string in UQI package, we use the en en-us eng or uqi insteadly
    //
    IsFound    = FALSE;
    UniBin     = (CHAR8 *) UniPackge + 4;

    while ((UniBin < UniPackgeEnd) && ((EFI_HII_PACKAGE_HEADER *)UniBin)->Type == EFI_HII_PACKAGE_STRINGS) {
      if (strcmp (((EFI_HII_STRING_PACKAGE_HDR *)UniBin)->Language, "en") == 0) {
        IsFound = TRUE;
        break;
      }
      UniBin += ((EFI_HII_STRING_PACKAGE_HDR *)UniBin)->Header.Length;
    }
    Status = GetStringByOffset (
              UniPackge,
              (UINT8 *)UniBin,
              VarDefaultNameId,
              IsFound,
              VarDefaultName
              );
    if (!EFI_ERROR (Status)) {
      goto Done;
    }

    IsFound    = FALSE;
    UniBin     = (CHAR8 *) UniPackge + 4;

    while ((UniBin < UniPackgeEnd) && ((EFI_HII_PACKAGE_HEADER *)UniBin)->Type == EFI_HII_PACKAGE_STRINGS) {
      if (strcmp (((EFI_HII_STRING_PACKAGE_HDR *)UniBin)->Language, "en-US") == 0) {
        IsFound = TRUE;
        break;
      }
      UniBin += ((EFI_HII_STRING_PACKAGE_HDR *)UniBin)->Header.Length;
    }
    Status = GetStringByOffset (
              UniPackge,
              (UINT8 *)UniBin,
              VarDefaultNameId,
              IsFound,
              VarDefaultName
              );
    if (!EFI_ERROR (Status)) {
      goto Done;
    }

    IsFound    = FALSE;
    UniBin     = (CHAR8 *) UniPackge + 4;

    while ((UniBin < UniPackgeEnd) && ((EFI_HII_PACKAGE_HEADER *)UniBin)->Type == EFI_HII_PACKAGE_STRINGS) {
      if (strcmp (((EFI_HII_STRING_PACKAGE_HDR *)UniBin)->Language, "eng") == 0) {
        IsFound = TRUE;
        break;
      }
      UniBin += ((EFI_HII_STRING_PACKAGE_HDR *)UniBin)->Header.Length;
    }
    Status = GetStringByOffset (
              UniPackge,
              (UINT8 *)UniBin,
              VarDefaultNameId,
              IsFound,
              VarDefaultName
              );
    if (!EFI_ERROR (Status)) {
      goto Done;
    }

    IsFound    = FALSE;
    UniBin     = (CHAR8 *) UniPackge + 4;

    while ((UniBin < UniPackgeEnd) && ((EFI_HII_PACKAGE_HEADER *)UniBin)->Type == EFI_HII_PACKAGE_STRINGS) {
      if (strcmp (((EFI_HII_STRING_PACKAGE_HDR *)UniBin)->Language, "uqi") == 0) {
        IsFound = TRUE;
        break;
      }
      UniBin += ((EFI_HII_STRING_PACKAGE_HDR *)UniBin)->Header.Length;
    }
    Status = GetStringByOffset (
              UniPackge,
              (UINT8 *)UniBin,
              VarDefaultNameId,
              IsFound,
              VarDefaultName
              );
    if (!EFI_ERROR (Status)) {
      goto Done;
    }
  }

Done:
  if (EFI_ERROR (Status)) {
    *VarDefaultName = NULL;
    return EFI_NOT_FOUND;
  }

  if (CachedStringList != NULL) {
    if (CachedStringList->CachedIdNum >= CachedStringList->MaxIdNum) {
      TempBuffer = calloc (sizeof (STRING_INFO), CachedStringList->MaxIdNum + STRING_NUMBER);
      if (TempBuffer == NULL) {
        printf ("Fail to allocate memory! \n");
        free (*VarDefaultName);
        *VarDefaultName = NULL;
        return EFI_OUT_OF_RESOURCES;
      }
      CopyMem (TempBuffer, CachedStringList->StringInfoList, sizeof (STRING_INFO) * CachedStringList->MaxIdNum);
      FreePool (CachedStringList->StringInfoList);
      CachedStringList->StringInfoList = TempBuffer;
      CachedStringList->MaxIdNum = CachedStringList->MaxIdNum + STRING_NUMBER;
    }
    CachedStringList->StringInfoList[CachedStringList->CachedIdNum].StringId = VarDefaultNameId;
    CachedStringList->StringInfoList[CachedStringList->CachedIdNum].String   = *VarDefaultName;
    CachedStringList->CachedIdNum ++;
  }
  return EFI_SUCCESS;
}

/**
  Get the variable Guid and Name by the variableId and FormSetOrder.

  @param  FormSet        The pointer to the formset.
  @param  FormSet        The pointer to the form.
  @param  ListEntry      The pointer to the LIST_ENTRY.

  @return EFI_SUCCESS
  @return EFI_NOT_FOUND  If not find the the variable, or the variable doesn't belong to EfiVarStore or VarStore.
**/
static
EFI_STATUS
GetGuidNameByVariableId (
  IN       FORM_BROWSER_FORMSET    *FormSet,
  IN  OUT  FORM_BROWSER_STATEMENT  *Question,
  IN       LIST_ENTRY              *ListEntry
  )
{
  FORMSET_STORAGE    *CurNode;
  LIST_ENTRY         *Link;
  LIST_ENTRY         *StorageListHead;
  EFI_STATUS         Status;
  CHAR16             *EfiVariableName;

  StorageListHead       = ListEntry;
  CurNode               = NULL;
  Status                = EFI_SUCCESS;
  EfiVariableName       = NULL;

  Link = GetFirstNode (StorageListHead);
  while (!IsNull (StorageListHead, Link)) {
    CurNode = FORMSET_STORAGE_FROM_LINK (Link);

    if ((FormSet->FormSetOrder == CurNode->FormSetOrder) \
      && (Question->VarStoreId == CurNode->VarStoreId)
      ) {
      //
      // Copy type to question to avoid the case that EfiVarStore and VarStore have the same Guid and name.
      //
      Question->Type           = CurNode->Type;
      Question->NewEfiVarstore = CurNode->NewEfiVarstore;
      Question->Attributes     = CurNode->Attributes;

      if (CurNode->Type == EFI_IFR_VARSTORE_EFI_OP) {
        CopyMem (&Question->Guid, &CurNode->Guid, sizeof (EFI_GUID));
        //
        // If the first time to access the old EfiVarStore, need to sync the variable name
        //
        if (!CurNode->NewEfiVarstore) {
          if (CurNode->Buffer == NULL) {
            CurNode->Buffer = malloc (Question->StorageWidth);
          }
          if (CurNode->Name == NULL) {
            Status  = FindDefaultName (
                      &(FormSet->EnUsStringList),
                      FormSet->UnicodeBinary,
                      Question->VarStoreInfo.VarName,
                      EN_US,
                      &EfiVariableName
                     );
            if (EFI_ERROR(Status)) {
              return Status;
            }
            CurNode->Name = EfiVariableName;
          }
          if (CurNode->Size == 0) {
            CurNode->Size = Question->StorageWidth;
          }
        }
        //
        // Check whether the Efivariable variable name is valid.
        //
         if ((CurNode->Name == NULL) || (FceStrLen (CurNode->Name) == 0)) {
          StringPrint ("Error. The variable name of question is NULL. Its UQI is: ");
          StringPrint("Q %04X ", Question->Uqi.HexNum);
          LogUqi (Question->Uqi.Data);
          StringPrint ("\n");
          return EFI_ABORTED;
        }
        if (Question->VariableName == NULL) {
          Question->VariableName = (CHAR16 *) malloc (2 * (FceStrLen ((CONST CHAR16 *)CurNode->Name) + 1));
          if (Question->VariableName == NULL) {
           return EFI_ABORTED;
          }
        }
        StrCpy (Question->VariableName, CurNode->Name);

        return EFI_SUCCESS;

      } else if (CurNode->Type == EFI_IFR_VARSTORE_OP) {
        CopyMem (&Question->Guid, &CurNode->Guid, sizeof (EFI_GUID));
        if (Question->VariableName == NULL) {
          Question->VariableName = (CHAR16 *) malloc (2 * (FceStrLen ((CONST CHAR16 *)CurNode->Name) + 1));
          if (Question->VariableName == NULL) {
           return EFI_ABORTED;
          }
        }
        //
        // Check whether the variable variable name is valid.
        //
         if ((CurNode->Name == NULL) || (FceStrLen (CurNode->Name) == 0)) {
          StringPrint ("Error. The variable name of question is NULL. UQI:");
          StringPrint("Q %04X ", Question->Uqi.HexNum);
          LogUqi (Question->Uqi.Data);
          StringPrint ("\n");
          return EFI_ABORTED;
        }
        StrCpy (Question->VariableName, CurNode->Name);
        return EFI_SUCCESS;
      }
    }
    Link = GetNextNode (StorageListHead, Link);
  }
  return EFI_NOT_FOUND;
}

/**
  Search the variable list according to the variable Guid and name, and return the pointer
  of that Node.

  @param  HiiObjList       The pointer to the Question
  @param  VarName          The EFI variable name need to be updated to VarList
  @param  Offset           The offset of the variable
  @param  StorageListHead  The pointer to the LIST_ENTRY of Storage
  @param  Vaue             The value in that value offset of the variable
  @param  VarList          The dual pointer of Varlist

  @return EFI_SUCCESS
  @return EFI_NOT_FOUND
**/
EFI_STATUS
SearchVarStorage (
  IN     FORM_BROWSER_STATEMENT   *Question,
  IN     CHAR16*                  VarName,
  IN     UINT32                   Offset,
  IN     LIST_ENTRY               *StorageListHead,
  IN OUT CHAR8                    **Value,
  IN OUT FORMSET_STORAGE          **VarList
  )
{
  FORMSET_STORAGE   *CurNode;
  LIST_ENTRY        *Link;
  BOOLEAN           FindOrNot;

  CurNode         = NULL;
  FindOrNot       = FALSE;
  *VarList        = NULL;
  //
  // Find Storage for this Question
  //
  Link = GetFirstNode (StorageListHead);
  while (!IsNull (StorageListHead, Link)) {
    CurNode = FORMSET_STORAGE_FROM_LINK (Link);
    //
    // Deal with the old EfiVarstore before UEFI2.31
    //
    if (!CompareGuid (&Question->Guid, &CurNode->Guid)          \
      && (CurNode->Type == EFI_IFR_VARSTORE_EFI_OP)             \
      && !CurNode->NewEfiVarstore                               \
      && (Question->VariableName != NULL)                       \
      && (CurNode->Name != NULL)                                \
      && !FceStrCmp(Question->VariableName, CurNode->Name)
      ) {
      //
      // If not multi-plaform support mode, take VarStore as the EfiVarStore. So If there are
      // two variables with same guid same name, but different type, we will take as the same
      // in general mode
      //
      if (mMultiPlatformParam.MultiPlatformOrNot && (CurNode->Type == Question->Type)) {
        //
        // check whether exist a old EFI_IFR_VARSTORE_EFI or not.
        //
        *Value = (CHAR8 *)CurNode->Buffer;
        *VarList = CurNode;
        FindOrNot  = TRUE;
        break;
      } else if (!mMultiPlatformParam.MultiPlatformOrNot) {
        //
        // check whether exist a old EFI_IFR_VARSTORE_EFI or not.
        //
        *Value = (CHAR8 *)CurNode->Buffer;
        *VarList = CurNode;
        FindOrNot  = TRUE;
        break;
      }
    }

    if (!CompareGuid (&Question->Guid, &CurNode->Guid)           \
      && (CurNode->Name != NULL)                                   \
      && (Question->VariableName != NULL)                          \
      && !FceStrCmp(Question->VariableName, CurNode->Name)
      ) {
      //
      // If not multi-plaform support mode, take VarStore as the EfiVarStore. So If there are
      // two variables with same guid same name, but different type, we will take as the same
      // in general mode
      //
      if (mMultiPlatformParam.MultiPlatformOrNot && (CurNode->Type == Question->Type)) {
        //
        // If matched, get the address of EFI_IFR_VARSTORE data.
        //
        *Value     = (CHAR8 *)(CurNode->Buffer + Offset);
        *VarList   = CurNode;
        FindOrNot  = TRUE;
        break;
      } else if (!mMultiPlatformParam.MultiPlatformOrNot) {
        //
        // If matched, get the address of EFI_IFR_VARSTORE data.
        //
        *Value     = (CHAR8 *)(CurNode->Buffer + Offset);
        *VarList   = CurNode;
        FindOrNot  = TRUE;
        break;
      }
    }
    Link = GetNextNode (StorageListHead, Link);
  }
  if (!FindOrNot) {
    return EFI_NOT_FOUND;
  }
  return EFI_SUCCESS;
}

/**
  Get the string based on the StringId and HII Package List Handle.

  @param  Token                  The String's ID.
  @param  HiiHandle              The package list in the HII database to search for
                                 the specified string.

  @return The output string.

**/
CHAR16 *
GetToken (
  IN  EFI_STRING_ID                Token,
  IN  UINT8                        *UniPackge
  )
{
  CHAR16      *VarDefaultName;
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (UniPackge == NULL) {
    return NULL;
  }

  Status  = FindDefaultName (
              NULL,
              UniPackge,
              Token,
              EN_US,
              &VarDefaultName
              );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return VarDefaultName;
}

/**
  Initialize Statement header members.

  @param  OpCodeData             Pointer of the raw OpCode data.
  @param  FormSet                Pointer of the current FormSe.
  @param  Form                   Pointer of the current Form.

  @return The Statement.

**/
FORM_BROWSER_STATEMENT *
CreateStatement (
  IN UINT8                        *OpCodeData,
  IN OUT FORM_BROWSER_FORMSET     *FormSet,
  IN OUT FORM_BROWSER_FORM        *Form
  )
{
  FORM_BROWSER_STATEMENT    *Statement;
  EFI_IFR_STATEMENT_HEADER  *StatementHdr;

  if (Form == NULL) {
    //
    // We are currently not in a Form Scope, so just skip this Statement
    //
    return NULL;
  }

  Statement = &FormSet->StatementBuffer[mStatementIndex];
  mStatementIndex++;

  InitializeListHead (&Statement->DefaultListHead);
  InitializeListHead (&Statement->OptionListHead);
  InitializeListHead (&Statement->InconsistentListHead);
  InitializeListHead (&Statement->NoSubmitListHead);
  InitializeListHead (&Statement->WarningListHead);

  Statement->Signature = FORM_BROWSER_STATEMENT_SIGNATURE;

  Statement->Operand = ((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode;
  Statement->QuestionReferToBitField = FALSE;

  StatementHdr = (EFI_IFR_STATEMENT_HEADER *) (OpCodeData + sizeof (EFI_IFR_OP_HEADER));
  CopyMem (&Statement->Prompt, &StatementHdr->Prompt, sizeof (EFI_STRING_ID));
  CopyMem (&Statement->Help, &StatementHdr->Help, sizeof (EFI_STRING_ID));

  if (mInScopeSuppress) {
    Statement->SuppressExpression = mSuppressExpression;
  }

  if (mInScopeGrayOut) {
    Statement->GrayOutExpression = mGrayOutExpression;
  }


  if (mInScopeDisable) {
    Statement->DisableExpression = mDisableExpression;
  }

  Statement->InSubtitle = mInScopeSubtitle;

  //
  // Insert this Statement into current Form
  //
  InsertTailList (&Form->StatementListHead, &Statement->Link);

  return Statement;
}

/**
  Initialize Question's members.

  @param  OpCodeData             Pointer of the raw OpCode data.
  @param  FormSet                Pointer of the current FormSet.
  @param  Form                   Pointer of the current Form.

  @return The Question.

**/
FORM_BROWSER_STATEMENT *
CreateQuestion (
  IN UINT8                        *OpCodeData,
  IN OUT FORM_BROWSER_FORMSET     *FormSet,
  IN OUT FORM_BROWSER_FORM        *Form
  )
{
  FORM_BROWSER_STATEMENT   *Statement;
  EFI_IFR_QUESTION_HEADER  *QuestionHdr;
  LIST_ENTRY               *Link;
  FORMSET_STORAGE          *Storage;
  NAME_VALUE_NODE          *NameValueNode;

  Statement = CreateStatement (OpCodeData, FormSet, Form);
  if (Statement == NULL) {
    return NULL;
  }

  QuestionHdr = (EFI_IFR_QUESTION_HEADER *) (OpCodeData + sizeof (EFI_IFR_OP_HEADER));
  CopyMem (&Statement->QuestionId, &QuestionHdr->QuestionId, sizeof (EFI_QUESTION_ID));
  CopyMem (&Statement->VarStoreId, &QuestionHdr->VarStoreId, sizeof (EFI_VARSTORE_ID));
  CopyMem (&Statement->VarStoreInfo.VarOffset, &QuestionHdr->VarStoreInfo.VarOffset, sizeof (UINT16));

  Statement->QuestionFlags = QuestionHdr->Flags;

  Statement->FormSetOrder = mFormSetOrderParse;

  if (Statement->VarStoreId == 0) {
    //
    // VarStoreId of zero indicates no variable storage
    //
    return Statement;
  }

  //
  // Find Storage for this Question
  //
  Link = GetFirstNode (FormSet->StorageListHead);
  while (!IsNull (FormSet->StorageListHead, Link)) {
    Storage = FORMSET_STORAGE_FROM_LINK (Link);

    if ((Storage->VarStoreId == Statement->VarStoreId)
      && (Storage->FormSetOrder == Statement->FormSetOrder)) {
      Statement->Storage = Storage;
      break;
    }

    Link = GetNextNode (FormSet->StorageListHead, Link);
  }
  ASSERT (Statement->Storage != NULL);

  if (Statement->Storage->Type == EFI_HII_VARSTORE_NAME_VALUE) {
    Statement->VariableName = GetToken (Statement->VarStoreInfo.VarName, FormSet->UnicodeBinary);
    ASSERT (Statement->VariableName != NULL);
    //
    // Insert to Name/Value varstore list
    //
    NameValueNode = AllocateZeroPool (sizeof (NAME_VALUE_NODE));
    ASSERT (NameValueNode != NULL);
    NameValueNode->Signature = NAME_VALUE_NODE_SIGNATURE;
    NameValueNode->Name = FceAllocateCopyPool (FceStrSize (Statement->VariableName), Statement->VariableName);
    ASSERT (NameValueNode->Name != NULL);
    NameValueNode->Value = AllocateZeroPool (0x10);
    ASSERT (NameValueNode->Value != NULL);
    NameValueNode->EditValue = AllocateZeroPool (0x10);
    ASSERT (NameValueNode->EditValue != NULL);

    InsertTailList (&Statement->Storage->NameValueListHead, &NameValueNode->Link);
  }

  return Statement;
}


/**
  Allocate a FORM_EXPRESSION node.

  @param  Form                   The Form associated with this Expression

  @return Pointer to a FORM_EXPRESSION data structure.

**/
FORM_EXPRESSION *
CreateExpression (
  IN OUT FORM_BROWSER_FORM        *Form
  )
{
  FORM_EXPRESSION  *Expression;

  Expression = AllocateZeroPool (sizeof (FORM_EXPRESSION));
  ASSERT (Expression != NULL);
  Expression->Signature = FORM_EXPRESSION_SIGNATURE;
  InitializeListHead (&Expression->OpCodeListHead);

  return Expression;
}


/**
  Allocate a FORMSET_STORAGE data structure and insert to FormSet Storage List.

  @param  FormSet                Pointer of the current FormSet

  @return Pointer to a FORMSET_STORAGE data structure.

**/
FORMSET_STORAGE *
CreateStorage (
  IN FORM_BROWSER_FORMSET  *FormSet
  )
{
  FORMSET_STORAGE  *Storage;

  Storage = AllocateZeroPool (sizeof (FORMSET_STORAGE));
  ASSERT (Storage != NULL);
  Storage->Signature = FORMSET_STORAGE_SIGNATURE;
  InitializeListHead (&Storage->NameValueListHead);
  InsertTailList (FormSet->StorageListHead, &Storage->Link);

  return Storage;
}

/**
  Free resources of a Expression.

  @param  FormSet                Pointer of the Expression

**/
VOID
DestroyExpression (
  IN FORM_EXPRESSION   *Expression
  )
{
  LIST_ENTRY         *Link;
  EXPRESSION_OPCODE  *OpCode;
  LIST_ENTRY         *SubExpressionLink;
  FORM_EXPRESSION    *SubExpression;

  while (!IsListEmpty (&Expression->OpCodeListHead)) {
    Link = GetFirstNode (&Expression->OpCodeListHead);
    OpCode = EXPRESSION_OPCODE_FROM_LINK (Link);
    RemoveEntryList (&OpCode->Link);

    if (OpCode->ValueList != NULL) {
      FreePool (OpCode->ValueList);
    }

    if (OpCode->ValueName != NULL) {
      FreePool (OpCode->ValueName);
    }

    if (OpCode->MapExpressionList.ForwardLink != NULL) {
      while (!IsListEmpty (&OpCode->MapExpressionList)) {
        SubExpressionLink = GetFirstNode(&OpCode->MapExpressionList);
        SubExpression     = FORM_EXPRESSION_FROM_LINK (SubExpressionLink);
        RemoveEntryList(&SubExpression->Link);
        DestroyExpression (SubExpression);
      }
    }
  }

  //
  // Free this Expression
  //
  FreePool (Expression);
}


/**
  Free resources of a storage.

  @param  Storage                Pointer of the storage

**/
VOID
DestroyStorage (
  IN FORMSET_STORAGE   *Storage
  )
{
  LIST_ENTRY         *Link;
  NAME_VALUE_NODE    *NameValueNode;

  if (Storage == NULL) {
    return;
  }

  if (Storage->Name != NULL) {
    FreePool (Storage->Name);
  }
  if (Storage->Buffer != NULL) {
    FreePool (Storage->Buffer);
  }

  while (!IsListEmpty (&Storage->NameValueListHead)) {
    Link = GetFirstNode (&Storage->NameValueListHead);
    NameValueNode = NAME_VALUE_NODE_FROM_LINK (Link);
    RemoveEntryList (&NameValueNode->Link);

    if (NameValueNode->Name != NULL) {
      FreePool (NameValueNode->Name);
    }
    if (NameValueNode->Value != NULL) {
      FreePool (NameValueNode->Value);
    }
    if (NameValueNode->EditValue != NULL) {
      FreePool (NameValueNode->EditValue);
    }
    FreePool (NameValueNode);
  }

  FreePool (Storage);
  Storage = NULL;
}

/**
  Free resources allocated for all Storage in an LIST_ENTRY.

  @param  FormSet                Pointer of the FormSet

**/
VOID
DestroyAllStorage (
  IN LIST_ENTRY    *StorageEntryListHead
  )
{
  LIST_ENTRY              *Link;
  FORMSET_STORAGE         *Storage;
  //
  // Parse Fromset one by one
  //
  if (StorageEntryListHead->ForwardLink != NULL) {
    while (!IsListEmpty (StorageEntryListHead)) {
      Link = GetFirstNode (StorageEntryListHead);
      Storage = FORMSET_STORAGE_FROM_LINK (Link);
      RemoveEntryList (&Storage->Link);

      DestroyStorage (Storage);
    }
  }
  StorageEntryListHead = NULL;
}

/**
  Free resources of a Statement.

  @param  FormSet                Pointer of the FormSet
  @param  Statement              Pointer of the Statement

**/
VOID
DestroyStatement (
  IN     FORM_BROWSER_FORMSET    *FormSet,
  IN OUT FORM_BROWSER_STATEMENT  *Statement
  )
{
  LIST_ENTRY        *Link;
  QUESTION_DEFAULT  *Default;
  QUESTION_OPTION   *Option;
  FORM_EXPRESSION   *Expression;

  //
  // Free Default value List
  //
  while (!IsListEmpty (&Statement->DefaultListHead)) {
    Link = GetFirstNode (&Statement->DefaultListHead);
    Default = QUESTION_DEFAULT_FROM_LINK (Link);
    RemoveEntryList (&Default->Link);

    if (Default->Value.Buffer != NULL) {
      FreePool(Default->Value.Buffer);
    }
    FreePool (Default);
  }

  //
  // Free Options List
  //
  while (!IsListEmpty (&Statement->OptionListHead)) {
    Link = GetFirstNode (&Statement->OptionListHead);
    Option = QUESTION_OPTION_FROM_LINK (Link);
    RemoveEntryList (&Option->Link);

    FreePool (Option);
  }

  //
  // Free Inconsistent List
  //
  while (!IsListEmpty (&Statement->InconsistentListHead)) {
    Link = GetFirstNode (&Statement->InconsistentListHead);
    Expression = FORM_EXPRESSION_FROM_LINK (Link);
    RemoveEntryList (&Expression->Link);

    DestroyExpression (Expression);
  }

  //
  // Free NoSubmit List
  //
  while (!IsListEmpty (&Statement->NoSubmitListHead)) {
    Link = GetFirstNode (&Statement->NoSubmitListHead);
    Expression = FORM_EXPRESSION_FROM_LINK (Link);
    RemoveEntryList (&Expression->Link);

    DestroyExpression (Expression);
  }

  //
  // Free WarningIf List
  //
  while (!IsListEmpty (&Statement->WarningListHead)) {
    Link = GetFirstNode (&Statement->WarningListHead);
    Expression = FORM_EXPRESSION_FROM_LINK (Link);
    RemoveEntryList (&Expression->Link);

    DestroyExpression (Expression);
  }
  if (Statement->VariableName != NULL) {
    FreePool (Statement->VariableName);
  }
  if (Statement->BufferValue != NULL) {
    FreePool (Statement->BufferValue);
  }
}

/**
  Free resources of a Form.

  @param  FormSet                Pointer of the FormSet
  @param  Form                   Pointer of the Form.

**/
VOID
DestroyForm (
  IN     FORM_BROWSER_FORMSET  *FormSet,
  IN OUT FORM_BROWSER_FORM     *Form
  )
{
  LIST_ENTRY              *Link;
  FORM_EXPRESSION         *Expression;
  FORM_BROWSER_STATEMENT  *Statement;

  //
  // Free Form Expressions
  //
  while (!IsListEmpty (&Form->ExpressionListHead)) {
    Link = GetFirstNode (&Form->ExpressionListHead);
    Expression = FORM_EXPRESSION_FROM_LINK (Link);
    RemoveEntryList (&Expression->Link);

    DestroyExpression (Expression);
  }

  //
  // Free Statements/Questions
  //
  while (!IsListEmpty (&Form->StatementListHead)) {
    Link = GetFirstNode (&Form->StatementListHead);
    Statement = FORM_BROWSER_STATEMENT_FROM_LINK (Link);
    RemoveEntryList (&Statement->Link);

    DestroyStatement (FormSet, Statement);
  }

  //
  // Free this Form
  //
  FreePool (Form);
}


/**
  Free resources allocated for a FormSet.

  @param  FormSet                Pointer of the FormSet

**/
VOID
DestroyFormSet (
  IN FORM_BROWSER_FORMSET  *FormSet
  )
{
  LIST_ENTRY            *Link;
  FORMSET_DEFAULTSTORE  *DefaultStore;
  FORM_EXPRESSION       *Expression;
  FORM_BROWSER_FORM     *Form;
  UINT16                Index;

  //
  // Free FormSet Default Store
  //
  if (FormSet->DefaultStoreListHead.ForwardLink != NULL) {
    while (!IsListEmpty (&FormSet->DefaultStoreListHead)) {
      Link = GetFirstNode (&FormSet->DefaultStoreListHead);
      DefaultStore = FORMSET_DEFAULTSTORE_FROM_LINK (Link);
      RemoveEntryList (&DefaultStore->Link);

      FreePool (DefaultStore);
    }
  }

  //
  // Free Formset Expressions
  //
  while (!IsListEmpty (&FormSet->ExpressionListHead)) {
    Link = GetFirstNode (&FormSet->ExpressionListHead);
    Expression = FORM_EXPRESSION_FROM_LINK (Link);
    RemoveEntryList (&Expression->Link);

    DestroyExpression (Expression);
  }

  //
  // Free Forms
  //
  if (FormSet->FormListHead.ForwardLink != NULL) {
    while (!IsListEmpty (&FormSet->FormListHead)) {
      Link = GetFirstNode (&FormSet->FormListHead);
      Form = FORM_BROWSER_FORM_FROM_LINK (Link);
      RemoveEntryList (&Form->Link);

      DestroyForm (FormSet, Form);
    }
  }

  if (FormSet->StatementBuffer != NULL) {
    FreePool (FormSet->StatementBuffer);
  }
  if (FormSet->ExpressionBuffer != NULL) {
    FreePool (FormSet->ExpressionBuffer);
  }
  if (FormSet->EnUsStringList.StringInfoList != NULL) {
    for (Index = 0; Index < FormSet->EnUsStringList.CachedIdNum; Index ++) {
      FreePool (FormSet->EnUsStringList.StringInfoList[Index].String);
    }
    FreePool (FormSet->EnUsStringList.StringInfoList);
  }
  if (FormSet->UqiStringList.StringInfoList != NULL) {
    for (Index = 0; Index < FormSet->UqiStringList.CachedIdNum; Index ++) {
      FreePool (FormSet->UqiStringList.StringInfoList[Index].String);
    }
    FreePool (FormSet->UqiStringList.StringInfoList);
  }

  FreePool (FormSet);
}

/**
  Free resources allocated for all FormSet in an LIST_ENTRY.

  @param  FormSet                Pointer of the FormSet

**/
VOID
DestroyAllFormSet (
  IN LIST_ENTRY    *FormSetEntryListHead
  )
{
  LIST_ENTRY              *Link;
  FORM_BROWSER_FORMSET    *FormSet;
  //
  // Parse Fromset one by one
  //
  if (FormSetEntryListHead->ForwardLink != NULL) {
    while (!IsListEmpty (FormSetEntryListHead)) {
      Link = GetFirstNode (FormSetEntryListHead);
      FormSet = FORM_BROWSER_FORMSET_FROM_LINK (Link);
      RemoveEntryList (&FormSet->Link);
      DestroyFormSet (FormSet);
    }
  }
}

/**
  Tell whether this Operand is an Expression OpCode or not

  @param  Operand                Operand of an IFR OpCode.

  @retval TRUE                   This is an Expression OpCode.
  @retval FALSE                  Not an Expression OpCode.

**/
BOOLEAN
IsExpressionOpCode (
  IN UINT8              Operand
  )
{
  if (((Operand >= EFI_IFR_EQ_ID_VAL_OP) && (Operand <= EFI_IFR_NOT_OP)) ||
      ((Operand >= EFI_IFR_MATCH_OP) && (Operand <= EFI_IFR_SET_OP))  ||
      ((Operand >= EFI_IFR_EQUAL_OP) && (Operand <= EFI_IFR_SPAN_OP)) ||
      (Operand == EFI_IFR_CATENATE_OP) ||
      (Operand == EFI_IFR_TO_LOWER_OP) ||
      (Operand == EFI_IFR_TO_UPPER_OP) ||
      (Operand == EFI_IFR_MAP_OP)      ||
      (Operand == EFI_IFR_VERSION_OP)  ||
      (Operand == EFI_IFR_SECURITY_OP)) {
    return TRUE;
  } else {
    return FALSE;
  }
}


/**
  Calculate number of Statemens(Questions) and Expression OpCodes.

  @param  FormSet                The FormSet to be counted.
  @param  NumberOfStatement      Number of Statemens(Questions)
  @param  NumberOfExpression     Number of Expression OpCodes

**/
VOID
CountOpCodes (
  IN  FORM_BROWSER_FORMSET  *FormSet,
  IN OUT  UINT16            *NumberOfStatement,
  IN OUT  UINT16            *NumberOfExpression
  )
{
  UINT16  StatementCount;
  UINT16  ExpressionCount;
  UINT8   *OpCodeData;
  UINTN   Offset;
  UINTN   OpCodeLen;

  Offset = 0;
  StatementCount = 0;
  ExpressionCount = 0;

  while (Offset < FormSet->IfrBinaryLength) {
    OpCodeData = FormSet->IfrBinaryData + Offset;
    OpCodeLen = ((EFI_IFR_OP_HEADER *) OpCodeData)->Length;
    Offset += OpCodeLen;

    if (IsExpressionOpCode (((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode)) {
      ExpressionCount++;
    } else {
      StatementCount++;
    }
  }

  *NumberOfStatement = StatementCount;
  *NumberOfExpression = ExpressionCount;
}



/**
  Parse opcodes in the formset IFR binary.

  @param  FormSet                Pointer of the FormSet data structure.

  @retval EFI_SUCCESS            Opcode parse success.
  @retval Other                  Opcode parse fail.

**/
EFI_STATUS
ParseOpCodes (
  IN FORM_BROWSER_FORMSET              *FormSet
  )
{
  EFI_STATUS              Status;
  UINT16                  Index;
  FORM_BROWSER_FORM       *CurrentForm;
  FORM_BROWSER_STATEMENT  *CurrentStatement;
  EXPRESSION_OPCODE       *ExpressionOpCode;
  FORM_EXPRESSION         *CurrentExpression;
  UINT8                   Operand;
  UINT8                   Scope;
  UINTN                   OpCodeOffset;
  UINTN                   OpCodeLength;
  UINT8                   *OpCodeData;
  UINT8                   ScopeOpCode;
  FORMSET_STORAGE         *Storage;
  FORMSET_STORAGE         *TempStorage;
  FORMSET_DEFAULTSTORE    *DefaultStore;
  QUESTION_DEFAULT        *CurrentDefault;
  QUESTION_OPTION         *CurrentOption;
  UINT8                   Width;
  CHAR8                   *AsciiString;
  UINT16                  NumberOfStatement;
  UINT16                  NumberOfExpression;
  BOOLEAN                 SuppressForQuestion;
  BOOLEAN                 SuppressForOption;
  BOOLEAN                 InScopeOptionSuppress;
  FORM_EXPRESSION         *OptionSuppressExpression;
  BOOLEAN                 InScopeFormSuppress;
  FORM_EXPRESSION         *FormSuppressExpression;
  UINT16                  DepthOfDisable;
  BOOLEAN                 OpCodeDisabled;
  BOOLEAN                 SingleOpCodeExpression;
  BOOLEAN                 InScopeDefault;
  EFI_HII_VALUE           *Value;
  UINT8                   MapScopeDepth;
  LIST_ENTRY              *Link;
  FORMSET_STORAGE         *VarStorage;
  LIST_ENTRY              *MapExpressionList;
  EFI_VARSTORE_ID         TempVarstoreId;
  BOOLEAN                 ConstantFlag;
  FORMSET_DEFAULTSTORE    *PreDefaultStore;
  LIST_ENTRY              *DefaultLink;
  BOOLEAN                 HaveInserted;
  BOOLEAN                 BitFieldStorage;
  UINT16                  TotalBits;

  mInScopeSubtitle         = FALSE;
  SuppressForQuestion      = FALSE;
  SuppressForOption        = FALSE;
  InScopeFormSuppress      = FALSE;
  mInScopeSuppress         = FALSE;
  InScopeOptionSuppress    = FALSE;
  mInScopeGrayOut          = FALSE;
  mInScopeDisable          = FALSE;
  DepthOfDisable           = 0;
  OpCodeDisabled           = FALSE;
  SingleOpCodeExpression   = FALSE;
  InScopeDefault           = FALSE;
  CurrentExpression        = NULL;
  CurrentDefault           = NULL;
  CurrentOption            = NULL;
  OptionSuppressExpression = NULL;
  FormSuppressExpression   = NULL;
  MapScopeDepth            = 0;
  Link                     = NULL;
  VarStorage               = NULL;
  MapExpressionList        = NULL;
  TempVarstoreId           = 0;
  ConstantFlag             = TRUE;
  BitFieldStorage          = FALSE;

  //
  // Get the number of Statements and Expressions
  //
  CountOpCodes (FormSet, &NumberOfStatement, &NumberOfExpression);

  mStatementIndex = 0;
  FormSet->StatementBuffer = AllocateZeroPool (NumberOfStatement * sizeof (FORM_BROWSER_STATEMENT));
  if (FormSet->StatementBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mExpressionOpCodeIndex = 0;
  FormSet->ExpressionBuffer = AllocateZeroPool (NumberOfExpression * sizeof (EXPRESSION_OPCODE));
  if (FormSet->ExpressionBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  FormSet->StorageListHead = &mVarListEntry;
  InitializeListHead (&FormSet->DefaultStoreListHead);
  InitializeListHead (&FormSet->FormListHead);
  InitializeListHead (&FormSet->ExpressionListHead);
  ResetCurrentExpressionStack ();
  ResetMapExpressionListStack ();

  CurrentForm = NULL;
  CurrentStatement = NULL;

  ResetScopeStack ();

  OpCodeOffset = 0;
  while (OpCodeOffset < FormSet->IfrBinaryLength) {
    OpCodeData = FormSet->IfrBinaryData + OpCodeOffset;

    OpCodeLength = ((EFI_IFR_OP_HEADER *) OpCodeData)->Length;
    OpCodeOffset += OpCodeLength;
    Operand = ((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode;
    Scope = ((EFI_IFR_OP_HEADER *) OpCodeData)->Scope;

    //
    // If scope bit set, push onto scope stack
    //
    if (Scope != 0) {
      PushScope (Operand);
    }

    if (OpCodeDisabled) {
      //
      // DisableIf Expression is evaluated to be TRUE, try to find its end.
      // Here only cares the EFI_IFR_DISABLE_IF and EFI_IFR_END
      //
      if (Operand == EFI_IFR_DISABLE_IF_OP) {
        DepthOfDisable++;
      } else if (Operand == EFI_IFR_END_OP) {
        Status = PopScope (&ScopeOpCode);
        if (EFI_ERROR (Status)) {
          return Status;
        }

        if (ScopeOpCode == EFI_IFR_DISABLE_IF_OP) {
          if (DepthOfDisable == 0) {
            mInScopeDisable = FALSE;
            OpCodeDisabled = FALSE;
          } else {
            DepthOfDisable--;
          }
        }
      }
      continue;
    }

    if (IsExpressionOpCode (Operand)) {
      ExpressionOpCode = &FormSet->ExpressionBuffer[mExpressionOpCodeIndex];
      mExpressionOpCodeIndex++;

      ExpressionOpCode->Signature = EXPRESSION_OPCODE_SIGNATURE;
      ExpressionOpCode->Operand = Operand;
      Value = &ExpressionOpCode->Value;

      switch (Operand) {
      case EFI_IFR_EQ_ID_VAL_OP:
        CopyMem (&ExpressionOpCode->QuestionId, &((EFI_IFR_EQ_ID_VAL *) OpCodeData)->QuestionId, sizeof (EFI_QUESTION_ID));

        Value->Type = EFI_IFR_TYPE_NUM_SIZE_16;
        CopyMem (&Value->Value.u16, &((EFI_IFR_EQ_ID_VAL *) OpCodeData)->Value, sizeof (UINT16));
        break;

      case EFI_IFR_EQ_ID_ID_OP:
        CopyMem (&ExpressionOpCode->QuestionId, &((EFI_IFR_EQ_ID_ID *) OpCodeData)->QuestionId1, sizeof (EFI_QUESTION_ID));
        CopyMem (&ExpressionOpCode->QuestionId2, &((EFI_IFR_EQ_ID_ID *) OpCodeData)->QuestionId2, sizeof (EFI_QUESTION_ID));
        break;

      case EFI_IFR_EQ_ID_VAL_LIST_OP:
        CopyMem (&ExpressionOpCode->QuestionId, &((EFI_IFR_EQ_ID_VAL_LIST *) OpCodeData)->QuestionId, sizeof (EFI_QUESTION_ID));
        CopyMem (&ExpressionOpCode->ListLength, &((EFI_IFR_EQ_ID_VAL_LIST *) OpCodeData)->ListLength, sizeof (UINT16));
        ExpressionOpCode->ValueList = FceAllocateCopyPool (ExpressionOpCode->ListLength * sizeof (UINT16), &((EFI_IFR_EQ_ID_VAL_LIST *) OpCodeData)->ValueList);
        break;

      case EFI_IFR_TO_STRING_OP:
      case EFI_IFR_FIND_OP:
        ExpressionOpCode->Format = (( EFI_IFR_TO_STRING *) OpCodeData)->Format;
        break;

      case EFI_IFR_STRING_REF1_OP:
        Value->Type = EFI_IFR_TYPE_STRING;
        CopyMem (&Value->Value.string, &(( EFI_IFR_STRING_REF1 *) OpCodeData)->StringId, sizeof (EFI_STRING_ID));
        break;

      case EFI_IFR_RULE_REF_OP:
        ExpressionOpCode->RuleId = (( EFI_IFR_RULE_REF *) OpCodeData)->RuleId;
        break;

      case EFI_IFR_SPAN_OP:
        ExpressionOpCode->Flags = (( EFI_IFR_SPAN *) OpCodeData)->Flags;
        break;

      case EFI_IFR_THIS_OP:
        ASSERT (CurrentStatement != NULL);
        ExpressionOpCode->QuestionId = CurrentStatement->QuestionId;
        break;

      case EFI_IFR_SECURITY_OP:
        CopyMem (&ExpressionOpCode->Guid, &((EFI_IFR_SECURITY *) OpCodeData)->Permissions, sizeof (EFI_GUID));
        break;

      case EFI_IFR_GET_OP:
      case EFI_IFR_SET_OP:
        CopyMem (&TempVarstoreId, &((EFI_IFR_GET *) OpCodeData)->VarStoreId, sizeof (TempVarstoreId));
        if (TempVarstoreId != 0) {
          if (FormSet->StorageListHead->ForwardLink != NULL) {
            Link = GetFirstNode (FormSet->StorageListHead);
            while (!IsNull (FormSet->StorageListHead, Link)) {
              VarStorage = FORMSET_STORAGE_FROM_LINK (Link);
              if (VarStorage->VarStoreId == ((EFI_IFR_GET *) OpCodeData)->VarStoreId) {
                ExpressionOpCode->VarStorage = VarStorage;
                break;
              }
              Link = GetNextNode (FormSet->StorageListHead, Link);
            }
          }
          if (ExpressionOpCode->VarStorage == NULL) {
            //
            // VarStorage is not found.
            //
            return EFI_INVALID_PARAMETER;
          }
        }
        ExpressionOpCode->ValueType = ((EFI_IFR_GET *) OpCodeData)->VarStoreType;
        switch (ExpressionOpCode->ValueType) {
        case EFI_IFR_TYPE_BOOLEAN:
        case EFI_IFR_TYPE_NUM_SIZE_8:
          ExpressionOpCode->ValueWidth = 1;
          break;

        case EFI_IFR_TYPE_NUM_SIZE_16:
        case EFI_IFR_TYPE_STRING:
          ExpressionOpCode->ValueWidth = 2;
          break;

        case EFI_IFR_TYPE_NUM_SIZE_32:
          ExpressionOpCode->ValueWidth = 4;
          break;

        case EFI_IFR_TYPE_NUM_SIZE_64:
          ExpressionOpCode->ValueWidth = 8;
          break;

        case EFI_IFR_TYPE_DATE:
          ExpressionOpCode->ValueWidth = (UINT8) sizeof (EFI_IFR_DATE);
          break;

        case EFI_IFR_TYPE_TIME:
          ExpressionOpCode->ValueWidth = (UINT8) sizeof (EFI_IFR_TIME);
          break;

        case EFI_IFR_TYPE_REF:
          ExpressionOpCode->ValueWidth = (UINT8) sizeof (EFI_IFR_REF);
          break;

        case EFI_IFR_TYPE_OTHER:
        case EFI_IFR_TYPE_UNDEFINED:
        case EFI_IFR_TYPE_ACTION:
        case EFI_IFR_TYPE_BUFFER:
        default:
          //
          // Invalid value type for Get/Set opcode.
          //
          return EFI_INVALID_PARAMETER;
        }
        CopyMem (&ExpressionOpCode->VarStoreInfo.VarName,   &((EFI_IFR_GET *) OpCodeData)->VarStoreInfo.VarName,   sizeof (EFI_STRING_ID));
        CopyMem (&ExpressionOpCode->VarStoreInfo.VarOffset, &((EFI_IFR_GET *) OpCodeData)->VarStoreInfo.VarOffset, sizeof (UINT16));
        if ((ExpressionOpCode->VarStorage != NULL) &&
            ((ExpressionOpCode->VarStorage->Type == EFI_HII_VARSTORE_NAME_VALUE)
            || ((ExpressionOpCode->VarStorage->Type == EFI_IFR_VARSTORE_EFI_OP) && !ExpressionOpCode->VarStorage->NewEfiVarstore))
             ) {
          ExpressionOpCode->ValueName = GetToken (ExpressionOpCode->VarStoreInfo.VarName, FormSet->UnicodeBinary);
          if (ExpressionOpCode->ValueName == NULL) {
            //
            // String ID is invalid.
            //
            return EFI_INVALID_PARAMETER;
          }
        }
        break;

      case EFI_IFR_QUESTION_REF1_OP:
        CopyMem (&ExpressionOpCode->QuestionId, &((EFI_IFR_EQ_ID_VAL_LIST *) OpCodeData)->QuestionId, sizeof (EFI_QUESTION_ID));
        break;

      case EFI_IFR_QUESTION_REF3_OP:
        if (OpCodeLength >= sizeof (EFI_IFR_QUESTION_REF3_2)) {
          CopyMem (&ExpressionOpCode->DevicePath, &(( EFI_IFR_QUESTION_REF3_2 *) OpCodeData)->DevicePath, sizeof (EFI_STRING_ID));

          if (OpCodeLength >= sizeof (EFI_IFR_QUESTION_REF3_3)) {
            CopyMem (&ExpressionOpCode->Guid, &(( EFI_IFR_QUESTION_REF3_3 *) OpCodeData)->Guid, sizeof (EFI_GUID));
          }
        }
        break;

      //
      // constant
      //
      case EFI_IFR_TRUE_OP:
        Value->Type = EFI_IFR_TYPE_BOOLEAN;
        Value->Value.b = TRUE;
        break;

      case EFI_IFR_FALSE_OP:
        Value->Type = EFI_IFR_TYPE_BOOLEAN;
        Value->Value.b = FALSE;
        break;

      case EFI_IFR_ONE_OP:
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_8;
        Value->Value.u8 = 1;
        break;

      case EFI_IFR_ZERO_OP:
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_8;
        Value->Value.u8 = 0;
        break;

      case EFI_IFR_ONES_OP:
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_64;
        Value->Value.u64 = 0xffffffffffffffffULL;
        break;

      case EFI_IFR_UINT8_OP:
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_8;
        Value->Value.u8 = (( EFI_IFR_UINT8 *) OpCodeData)->Value;
        break;

      case EFI_IFR_UINT16_OP:
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_16;
        CopyMem (&Value->Value.u16, &(( EFI_IFR_UINT16 *) OpCodeData)->Value, sizeof (UINT16));
        break;

      case EFI_IFR_UINT32_OP:
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_32;
        CopyMem (&Value->Value.u32, &(( EFI_IFR_UINT32 *) OpCodeData)->Value, sizeof (UINT32));
        break;

      case EFI_IFR_UINT64_OP:
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_64;
        CopyMem (&Value->Value.u64, &(( EFI_IFR_UINT64 *) OpCodeData)->Value, sizeof (UINT64));
        break;

      case EFI_IFR_UNDEFINED_OP:
        Value->Type = EFI_IFR_TYPE_UNDEFINED;
        break;

      case EFI_IFR_VERSION_OP:
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_16;
        break;

      default:
        break;
      }
      //
      // Create sub expression nested in MAP opcode
      //
      if ((CurrentExpression == NULL) && (MapScopeDepth > 0)) {
        CurrentExpression = CreateExpression (CurrentForm);
        ASSERT (MapExpressionList != NULL);
        InsertTailList (MapExpressionList, &CurrentExpression->Link);
        if (Scope == 0) {
          SingleOpCodeExpression = TRUE;
        }
      }
      ASSERT (CurrentExpression != NULL);
      InsertTailList (&CurrentExpression->OpCodeListHead, &ExpressionOpCode->Link);
      if (Operand == EFI_IFR_MAP_OP) {
        //
        // Store current Map Expression List.
        //
        if (MapExpressionList != NULL) {
          PushMapExpressionList (MapExpressionList);
        }
        //
        // Initialize new Map Expression List.
        //
        MapExpressionList = &ExpressionOpCode->MapExpressionList;
        InitializeListHead (MapExpressionList);
        //
        // Store current expression.
        //
        PushCurrentExpression (CurrentExpression);
        CurrentExpression = NULL;
        MapScopeDepth ++;
      } else if (SingleOpCodeExpression) {
        //
        // There are two cases to indicate the end of an Expression:
        // for single OpCode expression: one Expression OpCode
        // for expression consists of more than one OpCode: EFI_IFR_END
        //
        SingleOpCodeExpression = FALSE;

        if (mInScopeDisable && (CurrentForm == NULL)) {
          //
          // This is DisableIf expression for Form, it should be a constant expression
          //
          ConstantFlag = TRUE;
          Status = EvaluateExpression (FormSet, CurrentForm, CurrentExpression, &ConstantFlag);
          if (EFI_ERROR (Status)) {
            return Status;
          }

          if (CurrentExpression->Result.Type != EFI_IFR_TYPE_BOOLEAN) {
            return EFI_INVALID_PARAMETER;
          }
          if (!ConstantFlag) {
            StringPrint ("WARNING. The DisableIf expression for Form should be a constant expression.\n");
          }
          OpCodeDisabled = CurrentExpression->Result.Value.b;
        }

        CurrentExpression = NULL;
      }

      continue;
    }

    //
    // Parse the Opcode
    //
    switch (Operand) {

    case EFI_IFR_FORM_SET_OP:

      CopyMem (&FormSet->FormSetTitle, &((EFI_IFR_FORM_SET *) OpCodeData)->FormSetTitle, sizeof (EFI_STRING_ID));
      CopyMem (&FormSet->Help,         &((EFI_IFR_FORM_SET *) OpCodeData)->Help,         sizeof (EFI_STRING_ID));
      CopyMem (&FormSet->Guid,         &((EFI_IFR_FORM_SET *) OpCodeData)->Guid,         sizeof (EFI_GUID));

      if (OpCodeLength > OFFSET_OF (EFI_IFR_FORM_SET, Flags)) {
        //
        // The formset OpCode contains ClassGuid
        //
        FormSet->NumberOfClassGuid = (UINT8) (((EFI_IFR_FORM_SET *) OpCodeData)->Flags & 0x3);
        CopyMem (FormSet->ClassGuid, OpCodeData + sizeof (EFI_IFR_FORM_SET), FormSet->NumberOfClassGuid * sizeof (EFI_GUID));
      }
      FormSet->FormSetOrder = ++mFormSetOrderParse;
      break;

    case EFI_IFR_FORM_OP:
    case EFI_IFR_FORM_MAP_OP:
      //
      // Create a new Form for this FormSet
      //
      CurrentForm = AllocateZeroPool (sizeof (FORM_BROWSER_FORM));
      ASSERT (CurrentForm != NULL);
      CurrentForm->Signature = FORM_BROWSER_FORM_SIGNATURE;
      InitializeListHead (&CurrentForm->ExpressionListHead);
      InitializeListHead (&CurrentForm->StatementListHead);

      CurrentForm->FormType = STANDARD_MAP_FORM_TYPE;
      CopyMem (&CurrentForm->FormId,    &((EFI_IFR_FORM *) OpCodeData)->FormId,    sizeof (UINT16));
      CopyMem (&CurrentForm->FormTitle, &((EFI_IFR_FORM *) OpCodeData)->FormTitle, sizeof (EFI_STRING_ID));

      if (InScopeFormSuppress) {
        //
        // Form is inside of suppressif
        //
        CurrentForm->SuppressExpression = FormSuppressExpression;
      }

      if (Scope != 0) {
        //
        // Enter scope of a Form, suppressif will be used for Question or Option
        //
        SuppressForQuestion = TRUE;
      }

      //
      // Insert into Form list of this FormSet
      //
      InsertTailList (&FormSet->FormListHead, &CurrentForm->Link);
      break;
    //
    // Storage
    //
    case EFI_IFR_VARSTORE_OP:
      //
      // Create a buffer Storage for this FormSet
      //

      Storage = CreateStorage (FormSet);
      Storage->Type = EFI_IFR_VARSTORE_OP;

      CopyMem (&Storage->VarStoreId, &((EFI_IFR_VARSTORE *) OpCodeData)->VarStoreId, sizeof (EFI_VARSTORE_ID));
      CopyMem (&Storage->Guid,       &((EFI_IFR_VARSTORE *) OpCodeData)->Guid,       sizeof (EFI_GUID));
      CopyMem (&Storage->Size,       &((EFI_IFR_VARSTORE *) OpCodeData)->Size,       sizeof (UINT16));

      Storage->Buffer = AllocateZeroPool (Storage->Size);

      AsciiString = (CHAR8 *) ((EFI_IFR_VARSTORE *) OpCodeData)->Name;
      Storage->Name = AllocateZeroPool ((strlen (AsciiString) + 1) * 2);
      ASSERT (Storage->Name != NULL);
      for (Index = 0; AsciiString[Index] != 0; Index++) {
        Storage->Name[Index] = (CHAR16) AsciiString[Index];
      }
      Storage->FormSetOrder = mFormSetOrderParse;

      //
      // If not existed the same variable in StorageList, insert the new one. Or else, use the old one.
      // If these two variales have the same Guid name but different size, report an error.
      //
      if ((TempStorage = NotSameVariableInVarList (FormSet->StorageListHead, Storage)) != NULL) {
        if (Storage->Size != TempStorage->Size) {
           StringPrint ("Error. Two modules found with VarStore variables with same name %S and GUID %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x, but with different sizes %d and %d.\n",
           Storage->Name,
           Storage->Guid.Data1,
           Storage->Guid.Data2,
           Storage->Guid.Data3,
           Storage->Guid.Data4[0],
           Storage->Guid.Data4[1],
           Storage->Guid.Data4[2],
           Storage->Guid.Data4[3],
           Storage->Guid.Data4[4],
           Storage->Guid.Data4[5],
           Storage->Guid.Data4[6],
           Storage->Guid.Data4[7],
           Storage->Size,
           TempStorage->Size
           );
          return EFI_ABORTED;
        }
        //
        // Update the VarStoreId for current question to get the variable guid and name information
        //
        TempStorage->VarStoreId   = Storage->VarStoreId;
        TempStorage->FormSetOrder = Storage->FormSetOrder;
        RemoveEntryList (&Storage->Link);
        DestroyStorage(Storage);
      }
      break;

    case EFI_IFR_VARSTORE_NAME_VALUE_OP:
      //
      // Create a name/value Storage for this FormSet
      //
      Storage = CreateStorage (FormSet);
      Storage->Type = EFI_HII_VARSTORE_NAME_VALUE;

      CopyMem (&Storage->VarStoreId, &((EFI_IFR_VARSTORE_NAME_VALUE *) OpCodeData)->VarStoreId, sizeof (EFI_VARSTORE_ID));
      CopyMem (&Storage->Guid,       &((EFI_IFR_VARSTORE_NAME_VALUE *) OpCodeData)->Guid,       sizeof (EFI_GUID));

      Storage->FormSetOrder = mFormSetOrderParse;
      break;

    case EFI_IFR_VARSTORE_EFI_OP:
      //
      // Create a EFI variable Storage for this FormSet
      //
      Storage = CreateStorage (FormSet);
      Storage->Type = EFI_IFR_VARSTORE_EFI_OP;
      CopyMem (&Storage->VarStoreId, &((EFI_IFR_VARSTORE_EFI *) OpCodeData)->VarStoreId, sizeof (EFI_VARSTORE_ID));
      CopyMem (&Storage->Guid,       &((EFI_IFR_VARSTORE_EFI *) OpCodeData)->Guid,       sizeof (EFI_GUID));
      CopyMem (&Storage->Attributes, &((EFI_IFR_VARSTORE_EFI *) OpCodeData)->Attributes, sizeof (UINT32));
      //
      // Check whether the EfiVarStore before UEFI2.31 or not
      //
      Storage->Size             = sizeof (EFI_IFR_VARSTORE_EFI_OLD);
      if (((EFI_IFR_VARSTORE_EFI *) OpCodeData)->Header.Length == sizeof (EFI_IFR_VARSTORE_EFI_OLD)) {
        Storage->NewEfiVarstore   = FALSE;
        Storage->Size             = 0;
        Storage->Buffer           = NULL;
        Storage->Name             = NULL;
        Storage->Size             = 0;
      } else {
        //
        // EfiVarStore structure for UEFI2.31
        //
        Storage->NewEfiVarstore   = TRUE;
        CopyMem (&Storage->Size, &((EFI_IFR_VARSTORE_EFI *) OpCodeData)->Size, sizeof (UINT16));

        Storage->Buffer = AllocateZeroPool (Storage->Size);
        AsciiString = (CHAR8 *) ((EFI_IFR_VARSTORE_EFI *) OpCodeData)->Name;
        Storage->Name = AllocateZeroPool ((strlen (AsciiString) + 1) * 2);
        ASSERT (Storage->Name != NULL);
        for (Index = 0; AsciiString[Index] != 0; Index++) {
          Storage->Name[Index] = (CHAR16) AsciiString[Index];
        }
      }
      Storage->FormSetOrder = mFormSetOrderParse;
      //
      // If not existed the same variable in StorageList, insert the new one. Or else, use the old one.
      // If these two variales have the same Guid name but different size, report an error.
      //
      if ((TempStorage = NotSameVariableInVarList (FormSet->StorageListHead, Storage)) != NULL) {
        if (Storage->Size != TempStorage->Size) {
          StringPrint ("Error. Two modules found with EfiVarStore variables with same name %S and GUID %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x, but different sizes %d and %d.\n",
          Storage->Name,
          Storage->Guid.Data1,
          Storage->Guid.Data2,
          Storage->Guid.Data3,
          Storage->Guid.Data4[0],
          Storage->Guid.Data4[1],
          Storage->Guid.Data4[2],
          Storage->Guid.Data4[3],
          Storage->Guid.Data4[4],
          Storage->Guid.Data4[5],
          Storage->Guid.Data4[6],
          Storage->Guid.Data4[7],
          Storage->Size,
          TempStorage->Size
          );
          return EFI_ABORTED;
        }
        //
        // Update the VarStoreId for current question to get the variable guid and name information
        //
        TempStorage->VarStoreId   = Storage->VarStoreId;
        TempStorage->FormSetOrder = Storage->FormSetOrder;
        RemoveEntryList (&Storage->Link);
        DestroyStorage( Storage);
      }
      break;

    //
    // DefaultStore
    //
    case EFI_IFR_DEFAULTSTORE_OP:
      HaveInserted = FALSE;
      DefaultStore = AllocateZeroPool (sizeof (FORMSET_DEFAULTSTORE));
      ASSERT (DefaultStore != NULL);
      DefaultStore->Signature = FORMSET_DEFAULTSTORE_SIGNATURE;

      CopyMem (&DefaultStore->DefaultId,   &((EFI_IFR_DEFAULTSTORE *) OpCodeData)->DefaultId,   sizeof (UINT16));
      CopyMem (&DefaultStore->DefaultName, &((EFI_IFR_DEFAULTSTORE *) OpCodeData)->DefaultName, sizeof (EFI_STRING_ID));

      //
      // Insert it to the DefaultStore list of this Formset with ascending order.
      //
      if (!IsListEmpty (&FormSet->DefaultStoreListHead)) {
        DefaultLink = GetFirstNode (&FormSet->DefaultStoreListHead);
        while (!IsNull (&FormSet->DefaultStoreListHead, DefaultLink)) {
          PreDefaultStore = FORMSET_DEFAULTSTORE_FROM_LINK(DefaultLink);
          DefaultLink = GetNextNode (&FormSet->DefaultStoreListHead, DefaultLink);
          if (DefaultStore->DefaultId < PreDefaultStore->DefaultId) {
            InsertTailList (&PreDefaultStore->Link, &DefaultStore->Link);
            HaveInserted = TRUE;
            break;
          }
        }
      }
      if (!HaveInserted) {
        InsertTailList (&FormSet->DefaultStoreListHead, &DefaultStore->Link);
      }
      break;

    //
    // Statements
    //
    case EFI_IFR_SUBTITLE_OP:
      CurrentStatement = CreateStatement (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);

      CurrentStatement->Flags = ((EFI_IFR_SUBTITLE *) OpCodeData)->Flags;

      if (Scope != 0) {
        mInScopeSubtitle = TRUE;
      }
      break;

    case EFI_IFR_TEXT_OP:
      CurrentStatement = CreateStatement (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);

      CopyMem (&CurrentStatement->TextTwo, &((EFI_IFR_TEXT *) OpCodeData)->TextTwo, sizeof (EFI_STRING_ID));
      break;

    case EFI_IFR_RESET_BUTTON_OP:
      CurrentStatement = CreateStatement (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);
      CopyMem (&CurrentStatement->DefaultId, &((EFI_IFR_RESET_BUTTON *) OpCodeData)->DefaultId, sizeof (EFI_DEFAULT_ID));
      break;

    //
    // Questions
    //
    case EFI_IFR_ACTION_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);
      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_ACTION;
      //
      // No need to deal with the EFI_IFR_ACTION
      //
      break;

    case EFI_IFR_REF_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);
      Value = &CurrentStatement->HiiValue;
      Value->Type = EFI_IFR_TYPE_REF;
      if (OpCodeLength >= sizeof (EFI_IFR_REF)) {
        CopyMem (&Value->Value.ref.FormId, &((EFI_IFR_REF *) OpCodeData)->FormId, sizeof (EFI_FORM_ID));

        if (OpCodeLength >= sizeof (EFI_IFR_REF2)) {
          CopyMem (&Value->Value.ref.QuestionId, &((EFI_IFR_REF2 *) OpCodeData)->QuestionId, sizeof (EFI_QUESTION_ID));

          if (OpCodeLength >= sizeof (EFI_IFR_REF3)) {
            CopyMem (&Value->Value.ref.FormSetGuid, &((EFI_IFR_REF3 *) OpCodeData)->FormSetId, sizeof (EFI_GUID));

            if (OpCodeLength >= sizeof (EFI_IFR_REF4)) {
              CopyMem (&Value->Value.ref.DevicePath, &((EFI_IFR_REF4 *) OpCodeData)->DevicePath, sizeof (EFI_STRING_ID));
            }
          }
        }
      }
      CurrentStatement->StorageWidth = (UINT16) sizeof (EFI_HII_REF);
      break;

    case EFI_IFR_ONE_OF_OP:
    case EFI_IFR_NUMERIC_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT(CurrentStatement != NULL);

      CurrentStatement->Flags = ((EFI_IFR_ONE_OF *) OpCodeData)->Flags;
      Value = &CurrentStatement->HiiValue;

      if (BitFieldStorage) {
        //
        // Get the bit var store info (bit/byte offset, bit/byte offset)
        //
        CurrentStatement->QuestionReferToBitField = TRUE;
        CurrentStatement->BitStorageWidth = CurrentStatement->Flags & EDKII_IFR_NUMERIC_SIZE_BIT;
        CurrentStatement->BitVarOffset = CurrentStatement->VarStoreInfo.VarOffset;
        CurrentStatement->VarStoreInfo.VarOffset = CurrentStatement->BitVarOffset / 8;
        TotalBits = CurrentStatement->BitVarOffset % 8 + CurrentStatement->BitStorageWidth;
        CurrentStatement->StorageWidth = (TotalBits % 8 == 0? TotalBits / 8: TotalBits / 8 + 1);
        //
        // Get the Minimum/Maximum/Step value(Note: bit field type has been stored as UINT32 type)
        //
        CurrentStatement->Minimum = ((EFI_IFR_NUMERIC *) OpCodeData)->data.u32.MinValue;
        CurrentStatement->Maximum = ((EFI_IFR_NUMERIC *) OpCodeData)->data.u32.MaxValue;
        CurrentStatement->Step    = ((EFI_IFR_NUMERIC *) OpCodeData)->data.u32.Step;
      } else {
        switch (CurrentStatement->Flags & EFI_IFR_NUMERIC_SIZE) {
        case EFI_IFR_NUMERIC_SIZE_1:
          CurrentStatement->Minimum = ((EFI_IFR_NUMERIC *) OpCodeData)->data.u8.MinValue;
          CurrentStatement->Maximum = ((EFI_IFR_NUMERIC *) OpCodeData)->data.u8.MaxValue;
          CurrentStatement->Step    = ((EFI_IFR_NUMERIC *) OpCodeData)->data.u8.Step;
          CurrentStatement->StorageWidth = (UINT16) sizeof (UINT8);
          Value->Type = EFI_IFR_TYPE_NUM_SIZE_8;
          break;

        case EFI_IFR_NUMERIC_SIZE_2:
          CopyMem (&CurrentStatement->Minimum, &((EFI_IFR_NUMERIC *) OpCodeData)->data.u16.MinValue, sizeof (UINT16));
          CopyMem (&CurrentStatement->Maximum, &((EFI_IFR_NUMERIC *) OpCodeData)->data.u16.MaxValue, sizeof (UINT16));
          CopyMem (&CurrentStatement->Step,    &((EFI_IFR_NUMERIC *) OpCodeData)->data.u16.Step,     sizeof (UINT16));
          CurrentStatement->StorageWidth = (UINT16) sizeof (UINT16);
          Value->Type = EFI_IFR_TYPE_NUM_SIZE_16;
          break;

        case EFI_IFR_NUMERIC_SIZE_4:
          CopyMem (&CurrentStatement->Minimum, &((EFI_IFR_NUMERIC *) OpCodeData)->data.u32.MinValue, sizeof (UINT32));
          CopyMem (&CurrentStatement->Maximum, &((EFI_IFR_NUMERIC *) OpCodeData)->data.u32.MaxValue, sizeof (UINT32));
          CopyMem (&CurrentStatement->Step,    &((EFI_IFR_NUMERIC *) OpCodeData)->data.u32.Step,     sizeof (UINT32));
          CurrentStatement->StorageWidth = (UINT16) sizeof (UINT32);
          Value->Type = EFI_IFR_TYPE_NUM_SIZE_32;
          break;

        case EFI_IFR_NUMERIC_SIZE_8:
          CopyMem (&CurrentStatement->Minimum, &((EFI_IFR_NUMERIC *) OpCodeData)->data.u64.MinValue, sizeof (UINT64));
          CopyMem (&CurrentStatement->Maximum, &((EFI_IFR_NUMERIC *) OpCodeData)->data.u64.MaxValue, sizeof (UINT64));
          CopyMem (&CurrentStatement->Step,    &((EFI_IFR_NUMERIC *) OpCodeData)->data.u64.Step,     sizeof (UINT64));
          CurrentStatement->StorageWidth = (UINT16) sizeof (UINT64);
          Value->Type = EFI_IFR_TYPE_NUM_SIZE_64;
          break;

        default:
          break;
        }
      }
      if ((Operand == EFI_IFR_ONE_OF_OP) && (Scope != 0)) {
        SuppressForOption = TRUE;
      }
      //
      // Get the UQI information
      //
      PrintQuestion(FormSet, CurrentForm, CurrentStatement, FALSE);
      //
      // Exchange the Guid and name information between VarList and Question List
      //
      Status = GetGuidNameByVariableId (FormSet, CurrentStatement, FormSet->StorageListHead);
      //
      // Remove the question which isn't stored by EfiVarStore or VarStore
      //
      if (EFI_ERROR (Status)) {
        RemoveEntryList (&CurrentStatement->Link);
        DestroyStatement (FormSet, CurrentStatement);
      }
      break;

    case EFI_IFR_ORDERED_LIST_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT(CurrentStatement != NULL);

      CurrentStatement->Flags = ((EFI_IFR_ORDERED_LIST *) OpCodeData)->Flags;
      CurrentStatement->MaxContainers = ((EFI_IFR_ORDERED_LIST *) OpCodeData)->MaxContainers;

      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_BUFFER;
      CurrentStatement->BufferValue   = NULL;
      if (Scope != 0) {
        SuppressForOption = TRUE;
      }
      //
      // Get the UQI information
      //
      PrintQuestion(FormSet, CurrentForm, CurrentStatement, FALSE);
      //
      // Exchange the Guid and name information between VarList and Question List
      //
      Status = GetGuidNameByVariableId (FormSet, CurrentStatement, FormSet->StorageListHead);
      //
      // Remove the question which isn't stored by EfiVarStore or VarStore
      //
      if (EFI_ERROR (Status)) {
        RemoveEntryList (&CurrentStatement->Link);
        DestroyStatement (FormSet, CurrentStatement);
      }

      break;

    case EFI_IFR_CHECKBOX_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT(CurrentStatement != NULL);

      CurrentStatement->Flags = ((EFI_IFR_CHECKBOX *) OpCodeData)->Flags;
      CurrentStatement->StorageWidth  = (UINT16) sizeof (BOOLEAN);
      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_BOOLEAN;

      if (BitFieldStorage) {
        //
        // Get the bit var store info (bit/byte offset, bit/byte width)
        //
        CurrentStatement->QuestionReferToBitField = TRUE;
        CurrentStatement->BitStorageWidth = 1;
        CurrentStatement->BitVarOffset = CurrentStatement->VarStoreInfo.VarOffset;
        CurrentStatement->VarStoreInfo.VarOffset = CurrentStatement->BitVarOffset / 8;
        TotalBits = CurrentStatement->BitVarOffset % 8 + CurrentStatement->BitStorageWidth;
        CurrentStatement->StorageWidth = (TotalBits % 8 == 0? TotalBits / 8: TotalBits / 8 + 1);
      }
      //
      // Get the UQI information
      //
      PrintQuestion(FormSet, CurrentForm, CurrentStatement, FALSE);
      //
      // Exchange the Guid and name information between VarList and Question List
      //
      Status = GetGuidNameByVariableId (FormSet, CurrentStatement, FormSet->StorageListHead);
      //
      // Remove the question which isn't stored by EfiVarStore or VarStore
      //
      if (EFI_ERROR (Status)) {
        RemoveEntryList (&CurrentStatement->Link);
        DestroyStatement (FormSet, CurrentStatement);
      }

      break;

    case EFI_IFR_STRING_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);
      //
      // MinSize is the minimum number of characters that can be accepted for this opcode,
      // MaxSize is the maximum number of characters that can be accepted for this opcode.
      // The characters are stored as Unicode, so the storage width should multiply 2.
      //
      CurrentStatement->Minimum = ((EFI_IFR_STRING *) OpCodeData)->MinSize;
      CurrentStatement->Maximum = ((EFI_IFR_STRING *) OpCodeData)->MaxSize;
      CurrentStatement->StorageWidth = (UINT16)((UINTN) CurrentStatement->Maximum * sizeof (CHAR16));
      CurrentStatement->Flags = ((EFI_IFR_STRING *) OpCodeData)->Flags;

      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_STRING;
      CurrentStatement->BufferValue = AllocateZeroPool (CurrentStatement->StorageWidth + sizeof (CHAR16));
      //
      // Get the UQI information
      //
      PrintQuestion(FormSet, CurrentForm, CurrentStatement, FALSE);
      //
      // Exchange the Guid and name information between VarList and Question List
      //
      Status = GetGuidNameByVariableId (FormSet, CurrentStatement, FormSet->StorageListHead);
      //
      // Remove the question which isn't stored by EfiVarStore or VarStore
      //
      if (EFI_ERROR (Status)) {
        RemoveEntryList (&CurrentStatement->Link);
        DestroyStatement (FormSet, CurrentStatement);
      }

      break;

    case EFI_IFR_PASSWORD_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);
      //
      // MinSize is the minimum number of characters that can be accepted for this opcode,
      // MaxSize is the maximum number of characters that can be accepted for this opcode.
      // The characters are stored as Unicode, so the storage width should multiply 2.
      //
      CopyMem (&CurrentStatement->Minimum, &((EFI_IFR_PASSWORD *) OpCodeData)->MinSize, sizeof (UINT16));
      CopyMem (&CurrentStatement->Maximum, &((EFI_IFR_PASSWORD *) OpCodeData)->MaxSize, sizeof (UINT16));
      CurrentStatement->StorageWidth = (UINT16)((UINTN) CurrentStatement->Maximum * sizeof (CHAR16));

      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_STRING;
      CurrentStatement->BufferValue = AllocateZeroPool ((CurrentStatement->StorageWidth + sizeof (CHAR16)));
      break;

    case EFI_IFR_DATE_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT(CurrentStatement != NULL);

      CurrentStatement->Flags = ((EFI_IFR_DATE *) OpCodeData)->Flags;
      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_DATE;

      if ((CurrentStatement->Flags & EFI_QF_DATE_STORAGE) == QF_DATE_STORAGE_NORMAL) {
        CurrentStatement->StorageWidth = (UINT16) sizeof (EFI_HII_DATE);
      } else {
        //
        // Don't assign storage for RTC type of date/time
        //
        CurrentStatement->Storage = NULL;
        CurrentStatement->StorageWidth = 0;
      }
      break;

    case EFI_IFR_TIME_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT(CurrentStatement != NULL);

      CurrentStatement->Flags = ((EFI_IFR_TIME *) OpCodeData)->Flags;
      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_TIME;

      if ((CurrentStatement->Flags & QF_TIME_STORAGE) == QF_TIME_STORAGE_NORMAL) {
        CurrentStatement->StorageWidth = (UINT16) sizeof (EFI_HII_TIME);
      } else {
        //
        // Don't assign storage for RTC type of date/time
        //
        CurrentStatement->Storage = NULL;
        CurrentStatement->StorageWidth = 0;
      }
      break;

    //
    // Default
    //
    case EFI_IFR_DEFAULT_OP:
      //
      // EFI_IFR_DEFAULT appear in scope of a Question,
      // It creates a default value for the current question.
      // A Question may have more than one Default value which have different default types.
      //
      CurrentDefault = AllocateZeroPool (sizeof (QUESTION_DEFAULT));
      ASSERT (CurrentDefault != NULL);
      CurrentDefault->Signature = QUESTION_DEFAULT_SIGNATURE;

      CurrentDefault->Value.Type = ((EFI_IFR_DEFAULT *) OpCodeData)->Type;
      CopyMem (&CurrentDefault->DefaultId, &((EFI_IFR_DEFAULT *) OpCodeData)->DefaultId, sizeof (UINT16));
      if (CurrentDefault->Value.Type == EFI_IFR_TYPE_BUFFER) {
        CurrentDefault->Value.BufferLen = (UINT16)(OpCodeLength - OFFSET_OF(EFI_IFR_DEFAULT, Value));
        CurrentDefault->Value.Buffer = FceAllocateCopyPool(CurrentDefault->Value.BufferLen, &((EFI_IFR_DEFAULT *)OpCodeData)->Value);
        ASSERT(CurrentDefault->Value.Buffer != NULL);
      } else {
        CopyMem(&CurrentDefault->Value.Value, &((EFI_IFR_DEFAULT *)OpCodeData)->Value, OpCodeLength - OFFSET_OF(EFI_IFR_DEFAULT, Value));
        ExtendValueToU64(&CurrentDefault->Value);
      }

      //
      // Insert to Default Value list of current Question
      //
      InsertTailList (&CurrentStatement->DefaultListHead, &CurrentDefault->Link);

      if (Scope != 0) {
        InScopeDefault = TRUE;
      }
      break;

    //
    // Option
    //
    case EFI_IFR_ONE_OF_OPTION_OP:
      ASSERT (CurrentStatement != NULL);
      if (CurrentStatement->Operand == EFI_IFR_ORDERED_LIST_OP &&
        ((((EFI_IFR_ONE_OF_OPTION *)OpCodeData)->Flags & (EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG)) != 0)) {
        //
        // It's keep the default value for ordered list opcode.
        //
        CurrentDefault = AllocateZeroPool(sizeof (QUESTION_DEFAULT));
        ASSERT(CurrentDefault != NULL);
        CurrentDefault->Signature = QUESTION_DEFAULT_SIGNATURE;

        CurrentDefault->Value.Type = EFI_IFR_TYPE_BUFFER;
        if ((((EFI_IFR_ONE_OF_OPTION *)OpCodeData)->Flags & EFI_IFR_OPTION_DEFAULT) != 0) {
          CurrentDefault->DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD;
        } else {
          CurrentDefault->DefaultId = EFI_HII_DEFAULT_CLASS_MANUFACTURING;
        }

        CurrentDefault->Value.BufferLen = (UINT16)(OpCodeLength - OFFSET_OF(EFI_IFR_ONE_OF_OPTION, Value));
        CurrentDefault->Value.Buffer = FceAllocateCopyPool(CurrentDefault->Value.BufferLen, &((EFI_IFR_ONE_OF_OPTION *)OpCodeData)->Value);
        ASSERT(CurrentDefault->Value.Buffer != NULL);

        //
        // Insert to Default Value list of current Question
        //
        InsertTailList(&CurrentStatement->DefaultListHead, &CurrentDefault->Link);
        break;
      }
      //
      // EFI_IFR_ONE_OF_OPTION appear in scope of a Question.
      // It create a selection for use in current Question.
      //
      CurrentOption = AllocateZeroPool (sizeof (QUESTION_OPTION));
      ASSERT (CurrentOption != NULL);
      CurrentOption->Signature = QUESTION_OPTION_SIGNATURE;

      CurrentOption->Flags = ((EFI_IFR_ONE_OF_OPTION *) OpCodeData)->Flags;
      CurrentOption->Value.Type = ((EFI_IFR_ONE_OF_OPTION *) OpCodeData)->Type;
      CopyMem (&CurrentOption->Text, &((EFI_IFR_ONE_OF_OPTION *) OpCodeData)->Option, sizeof (EFI_STRING_ID));
      CopyMem (&CurrentOption->Value.Value, &((EFI_IFR_ONE_OF_OPTION *) OpCodeData)->Value, OpCodeLength - OFFSET_OF (EFI_IFR_ONE_OF_OPTION, Value));
      ExtendValueToU64 (&CurrentOption->Value);

      if (InScopeOptionSuppress) {
        CurrentOption->SuppressExpression = OptionSuppressExpression;
      }

      //
      // Insert to Option list of current Question
      //
      InsertTailList (&CurrentStatement->OptionListHead, &CurrentOption->Link);

      //
      // Now we know the Storage width of nested Ordered List
      //
      if ((CurrentStatement->Operand == EFI_IFR_ORDERED_LIST_OP) && (CurrentStatement->BufferValue == NULL)) {
        Width = 1;
        switch (CurrentOption->Value.Type) {
        case EFI_IFR_TYPE_NUM_SIZE_8:
          Width = 1;
          break;

        case EFI_IFR_TYPE_NUM_SIZE_16:
          Width = 2;
          break;

        case EFI_IFR_TYPE_NUM_SIZE_32:
          Width = 4;
          break;

        case EFI_IFR_TYPE_NUM_SIZE_64:
          Width = 8;
          break;

        default:
          //
          // Invalid type for Ordered List
          //
          break;
        }

        CurrentStatement->StorageWidth = (UINT16) (CurrentStatement->MaxContainers * Width);
        CurrentStatement->BufferValue = AllocateZeroPool (CurrentStatement->StorageWidth);
        CurrentStatement->ValueType = CurrentOption->Value.Type;
        if (CurrentStatement->HiiValue.Type == EFI_IFR_TYPE_BUFFER) {
          CurrentStatement->HiiValue.Buffer = CurrentStatement->BufferValue;
          CurrentStatement->HiiValue.BufferLen = CurrentStatement->StorageWidth;
        }
      }
      break;

    //
    // Conditional
    //
    case EFI_IFR_NO_SUBMIT_IF_OP:
    case EFI_IFR_INCONSISTENT_IF_OP:
      //
      // Create an Expression node
      //
      CurrentExpression = CreateExpression (CurrentForm);
      CopyMem (&CurrentExpression->Error, &((EFI_IFR_INCONSISTENT_IF *) OpCodeData)->Error, sizeof (EFI_STRING_ID));

      if (Operand == EFI_IFR_NO_SUBMIT_IF_OP) {
        CurrentExpression->Type = EFI_HII_EXPRESSION_NO_SUBMIT_IF;
        InsertTailList (&CurrentStatement->NoSubmitListHead, &CurrentExpression->Link);
      } else {
        CurrentExpression->Type = EFI_HII_EXPRESSION_INCONSISTENT_IF;
        InsertTailList (&CurrentStatement->InconsistentListHead, &CurrentExpression->Link);
      }
      //
      // Take a look at next OpCode to see whether current expression consists
      // of single OpCode
      //
      if (((EFI_IFR_OP_HEADER *) (OpCodeData + OpCodeLength))->Scope == 0) {
        SingleOpCodeExpression = TRUE;
      }
      break;

    case EFI_IFR_WARNING_IF_OP:
      //
      // Create an Expression node
      //
      CurrentExpression = CreateExpression (CurrentForm);
      CopyMem (&CurrentExpression->Error, &((EFI_IFR_WARNING_IF *) OpCodeData)->Warning, sizeof (EFI_STRING_ID));
      CurrentExpression->TimeOut = ((EFI_IFR_WARNING_IF *) OpCodeData)->TimeOut;
      CurrentExpression->Type    = EFI_HII_EXPRESSION_WARNING_IF;
      InsertTailList (&CurrentStatement->WarningListHead, &CurrentExpression->Link);

      //
      // Take a look at next OpCode to see whether current expression consists
      // of single OpCode
      //
      if (((EFI_IFR_OP_HEADER *) (OpCodeData + OpCodeLength))->Scope == 0) {
        SingleOpCodeExpression = TRUE;
      }
      break;

    case EFI_IFR_SUPPRESS_IF_OP:
      //
      // Question and Option will appear in scope of this OpCode
      //
      CurrentExpression = CreateExpression (CurrentForm);
      CurrentExpression->Type = EFI_HII_EXPRESSION_SUPPRESS_IF;

      if (CurrentForm == NULL) {
        InsertTailList (&FormSet->ExpressionListHead, &CurrentExpression->Link);
      } else {
        InsertTailList (&CurrentForm->ExpressionListHead, &CurrentExpression->Link);
      }

      if (SuppressForOption) {
        InScopeOptionSuppress = TRUE;
        OptionSuppressExpression = CurrentExpression;
      } else if (SuppressForQuestion) {
        mInScopeSuppress = TRUE;
        mSuppressExpression = CurrentExpression;
      } else {
        InScopeFormSuppress = TRUE;
        FormSuppressExpression = CurrentExpression;
      }
      //
      // Take a look at next OpCode to see whether current expression consists
      // of single OpCode
      //
      if (((EFI_IFR_OP_HEADER *) (OpCodeData + OpCodeLength))->Scope == 0) {
        SingleOpCodeExpression = TRUE;
      }
      break;

    case EFI_IFR_GRAY_OUT_IF_OP:
      //
      // Questions will appear in scope of this OpCode
      //
      CurrentExpression = CreateExpression (CurrentForm);
      CurrentExpression->Type = EFI_HII_EXPRESSION_GRAY_OUT_IF;
      InsertTailList (&CurrentForm->ExpressionListHead, &CurrentExpression->Link);

      mInScopeGrayOut = TRUE;
      mGrayOutExpression = CurrentExpression;

      //
      // Take a look at next OpCode to see whether current expression consists
      // of single OpCode
      //
      if (((EFI_IFR_OP_HEADER *) (OpCodeData + OpCodeLength))->Scope == 0) {
        SingleOpCodeExpression = TRUE;
      }
      break;

    case EFI_IFR_DISABLE_IF_OP:
      //
      // The DisableIf expression should only rely on constant, so it could be
      // evaluated at initialization and it will not be queued
      //
      CurrentExpression = AllocateZeroPool (sizeof (FORM_EXPRESSION));
      ASSERT (CurrentExpression != NULL);
      CurrentExpression->Signature = FORM_EXPRESSION_SIGNATURE;
      CurrentExpression->Type = EFI_HII_EXPRESSION_DISABLE_IF;
      InitializeListHead (&CurrentExpression->OpCodeListHead);

      if (CurrentForm != NULL) {
        //
        // This is DisableIf for Question, enqueue it to Form expression list
        //
        InsertTailList (&CurrentForm->ExpressionListHead, &CurrentExpression->Link);
      }

      mDisableExpression = CurrentExpression;
      mInScopeDisable    = TRUE;
      OpCodeDisabled     = FALSE;

      //
      // Take a look at next OpCode to see whether current expression consists
      // of single OpCode
      //
      if (((EFI_IFR_OP_HEADER *) (OpCodeData + OpCodeLength))->Scope == 0) {
        SingleOpCodeExpression = TRUE;
      }
      break;

    //
    // Expression
    //
    case EFI_IFR_VALUE_OP:
      CurrentExpression = CreateExpression (CurrentForm);
      CurrentExpression->Type = EFI_HII_EXPRESSION_VALUE;
      InsertTailList (&CurrentForm->ExpressionListHead, &CurrentExpression->Link);

      if (InScopeDefault) {
        //
        // Used for default (EFI_IFR_DEFAULT)
        //
        CurrentDefault->ValueExpression = CurrentExpression;
      } else {
        //
        // If used for a question, then the question will be read-only
        //
        //
        // Make sure CurrentStatement is not NULL.
        // If it is NULL, 1) ParseOpCodes functions may parse the IFR wrongly. Or 2) the IFR
        // file is wrongly generated by tools such as VFR Compiler. There may be a bug in VFR Compiler.
        //
        ASSERT (CurrentStatement != NULL);
        CurrentStatement->ValueExpression = CurrentExpression;
      }

      //
      // Take a look at next OpCode to see whether current expression consists
      // of single OpCode
      //
      if (((EFI_IFR_OP_HEADER *) (OpCodeData + OpCodeLength))->Scope == 0) {
        SingleOpCodeExpression = TRUE;
      }
      break;

    case EFI_IFR_RULE_OP:
      CurrentExpression = CreateExpression (CurrentForm);
      CurrentExpression->Type = EFI_HII_EXPRESSION_RULE;

      CurrentExpression->RuleId = ((EFI_IFR_RULE *) OpCodeData)->RuleId;
      InsertTailList (&CurrentForm->ExpressionListHead, &CurrentExpression->Link);

      //
      // Take a look at next OpCode to see whether current expression consists
      // of single OpCode
      //
      if (((EFI_IFR_OP_HEADER *) (OpCodeData + OpCodeLength))->Scope == 0) {
        SingleOpCodeExpression = TRUE;
      }
      break;

    case EFI_IFR_READ_OP:
      CurrentExpression = CreateExpression (CurrentForm);
      CurrentExpression->Type = EFI_HII_EXPRESSION_READ;
      InsertTailList (&CurrentForm->ExpressionListHead, &CurrentExpression->Link);

      //
      // Make sure CurrentStatement is not NULL.
      // If it is NULL, 1) ParseOpCodes functions may parse the IFR wrongly. Or 2) the IFR
      // file is wrongly generated by tools such as VFR Compiler. There may be a bug in VFR Compiler.
      //
      ASSERT (CurrentStatement != NULL);
      CurrentStatement->ReadExpression = CurrentExpression;

      //
      // Take a look at next OpCode to see whether current expression consists
      // of single OpCode
      //
      if (((EFI_IFR_OP_HEADER *) (OpCodeData + OpCodeLength))->Scope == 0) {
        SingleOpCodeExpression = TRUE;
      }
      break;

    case EFI_IFR_WRITE_OP:
      CurrentExpression = CreateExpression (CurrentForm);
      CurrentExpression->Type = EFI_HII_EXPRESSION_WRITE;
      InsertTailList (&CurrentForm->ExpressionListHead, &CurrentExpression->Link);

      //
      // Make sure CurrentStatement is not NULL.
      // If it is NULL, 1) ParseOpCodes functions may parse the IFR wrongly. Or 2) the IFR
      // file is wrongly generated by tools such as VFR Compiler. There may be a bug in VFR Compiler.
      //
      ASSERT (CurrentStatement != NULL);
      CurrentStatement->WriteExpression = CurrentExpression;

      //
      // Take a look at next OpCode to see whether current expression consists
      // of single OpCode
      //
      if (((EFI_IFR_OP_HEADER *) (OpCodeData + OpCodeLength))->Scope == 0) {
        SingleOpCodeExpression = TRUE;
      }
      break;

    //
    // Image
    //
    case EFI_IFR_IMAGE_OP:
      //
      // Get ScopeOpcode from top of stack
      //
      PopScope (&ScopeOpCode);
      PushScope (ScopeOpCode);

      switch (ScopeOpCode) {
      case EFI_IFR_FORM_SET_OP:
        break;

      case EFI_IFR_FORM_OP:
      case EFI_IFR_FORM_MAP_OP:
        ASSERT (CurrentForm != NULL);
        break;

      case EFI_IFR_ONE_OF_OPTION_OP:
        break;

      default:
        //
        // Make sure CurrentStatement is not NULL.
        // If it is NULL, 1) ParseOpCodes functions may parse the IFR wrongly. Or 2) the IFR
        // file is wrongly generated by tools such as VFR Compiler.
        //
        ASSERT (CurrentStatement != NULL);
        break;
      }
      break;

    //
    // Refresh
    //
    case EFI_IFR_REFRESH_OP:
      break;

    //
    // Refresh guid.
    //
    case EFI_IFR_REFRESH_ID_OP:
      break;

    //
    // Modal tag
    //
    case EFI_IFR_MODAL_TAG_OP:
      break;

    //
    // Vendor specific
    //
    case EFI_IFR_GUID_OP:
      if (CompareGuid ((EFI_GUID *)(OpCodeData + sizeof (EFI_IFR_OP_HEADER)), &gEdkiiIfrBitVarGuid)==0) {
        Scope = 0;
        BitFieldStorage = TRUE;
      }
      break;

    //
    // Scope End
    //
    case EFI_IFR_END_OP:
      BitFieldStorage = FALSE;
      Status = PopScope (&ScopeOpCode);
      if (EFI_ERROR (Status)) {
        ResetScopeStack ();
        return Status;
      }

      switch (ScopeOpCode) {
      case EFI_IFR_FORM_SET_OP:
        //
        // End of FormSet, update FormSet IFR binary length
        // to stop parsing substantial OpCodes
        //
        FormSet->IfrBinaryLength = OpCodeOffset;
        break;

      case EFI_IFR_FORM_OP:
      case EFI_IFR_FORM_MAP_OP:
        //
        // End of Form
        //
        CurrentForm = NULL;
        SuppressForQuestion = FALSE;
        break;

      case EFI_IFR_ONE_OF_OPTION_OP:
        //
        // End of Option
        //
        CurrentOption = NULL;
        break;

      case EFI_IFR_SUBTITLE_OP:
        mInScopeSubtitle = FALSE;
        break;

      case EFI_IFR_NO_SUBMIT_IF_OP:
      case EFI_IFR_INCONSISTENT_IF_OP:
      case EFI_IFR_WARNING_IF_OP:
        //
        // Ignore end of EFI_IFR_NO_SUBMIT_IF and EFI_IFR_INCONSISTENT_IF
        //
        break;

      case EFI_IFR_SUPPRESS_IF_OP:
        if (SuppressForOption) {
          InScopeOptionSuppress = FALSE;
        } else if (SuppressForQuestion) {
          mInScopeSuppress = FALSE;
        } else {
          InScopeFormSuppress = FALSE;
        }
        break;

      case EFI_IFR_GRAY_OUT_IF_OP:
        mInScopeGrayOut = FALSE;
        break;

      case EFI_IFR_DISABLE_IF_OP:
        mInScopeDisable = FALSE;
        OpCodeDisabled  = FALSE;
        break;

      case EFI_IFR_ONE_OF_OP:
      case EFI_IFR_ORDERED_LIST_OP:
        SuppressForOption = FALSE;
        break;

      case EFI_IFR_DEFAULT_OP:
        InScopeDefault = FALSE;
        break;

      case EFI_IFR_MAP_OP:
        //
        // Get current Map Expression List.
        //
        Status = PopMapExpressionList ((VOID **) &MapExpressionList);
        if (Status == EFI_ACCESS_DENIED) {
          MapExpressionList = NULL;
        }
        //
        // Get current expression.
        //
        Status = PopCurrentExpression ((VOID **) &CurrentExpression);
        ASSERT (!EFI_ERROR (Status));
        ASSERT (MapScopeDepth > 0);
        MapScopeDepth --;
        break;

      default:
        if (IsExpressionOpCode (ScopeOpCode)) {
          if (mInScopeDisable && (CurrentForm == NULL)) {
            //
            // This is DisableIf expression for Form, it should be a constant expression
            //
            ASSERT (CurrentExpression != NULL);
            ConstantFlag = TRUE;
            Status = EvaluateExpression (FormSet, CurrentForm, CurrentExpression, &ConstantFlag);
            if (EFI_ERROR (Status)) {
              return Status;
            }
            if (CurrentExpression->Result.Type != EFI_IFR_TYPE_BOOLEAN) {
              return EFI_INVALID_PARAMETER;
            }
            if (!ConstantFlag) {
              StringPrint ("WARNING. The DisableIf expression for Form should be a constant expression.\n");
            }
            OpCodeDisabled = CurrentExpression->Result.Value.b;
            //
            // DisableIf Expression is only used once and not queued, free it
            //
            DestroyExpression (CurrentExpression);
          }

          //
          // End of current Expression
          //
          CurrentExpression = NULL;
        }
        break;
      }
      break;

    default:
      break;
    }
  }

  return EFI_SUCCESS;
}

/**
  Search an Option of a Question by its value.

  @param  Question               The Question
  @param  OptionValue            Value for Option to be searched.

  @retval Pointer                Pointer to the found Option.
  @retval NULL                   Option not found.

**/
QUESTION_OPTION *
ValueToOption (
  IN FORM_BROWSER_FORMSET     *FormSet,
  IN FORM_BROWSER_STATEMENT   *Question,
  IN EFI_HII_VALUE            *OptionValue
  )
{
  LIST_ENTRY       *Link;
  QUESTION_OPTION  *Option;

  Link = GetFirstNode (&Question->OptionListHead);
  while (!IsNull (&Question->OptionListHead, Link)) {
    Option = QUESTION_OPTION_FROM_LINK (Link);

    if (CompareHiiValue (&Option->Value, OptionValue, FormSet) == 0) {
      return Option;
    }

    Link = GetNextNode (&Question->OptionListHead, Link);
  }

  return NULL;
}

/**
  Set value of a data element in an Array by its Index.

  @param  Array                  The data array.
  @param  Type                   Type of the data in this array.
  @param  Index                  Zero based index for data in this array.
  @param  Value                  The value to be set.

**/
VOID
SetArrayData (
  IN VOID                     *Array,
  IN UINT8                    Type,
  IN UINTN                    Index,
  IN UINT64                   Value
  )
{

  ASSERT (Array != NULL);

  switch (Type) {
  case EFI_IFR_TYPE_NUM_SIZE_8:
    *(((UINT8 *) Array) + Index) = (UINT8) Value;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_16:
    *(((UINT16 *) Array) + Index) = (UINT16) Value;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_32:
    *(((UINT32 *) Array) + Index) = (UINT32) Value;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_64:
    *(((UINT64 *) Array) + Index) = (UINT64) Value;
    break;

  default:
    break;
  }
}

/**
  Reset Question of five kinds to its default value.

  @param  FormSet                The form set.
  @param  Form                   The form.
  @param  Question               The question.
  @param  DefaultId              The default Id.
  @param  DefaultId              The platform Id.

  @retval EFI_SUCCESS            Question is reset to default value.

**/
EFI_STATUS
GetQuestionDefault (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form,
  IN FORM_BROWSER_STATEMENT           *Question,
  IN UINT16                           DefaultId,
  IN UINT64                           PlatformId
  )
{
  EFI_STATUS              Status;
  LIST_ENTRY              *Link;
  QUESTION_DEFAULT        *Default;
  QUESTION_OPTION         *Option;
  EFI_HII_VALUE           *HiiValue;
  UINT8                   Index;
  FORMSET_STORAGE         *VarList;
  UINT8                   *VarBuffer;
  BOOLEAN                 ConstantFlag;
  UINT16                  OriginalDefaultId;
  FORMSET_DEFAULTSTORE    *DefaultStore;
  LIST_ENTRY              *DefaultLink;
  CHAR16                  *VarDefaultName;

  VarDefaultName  = NULL;
  Status          = EFI_SUCCESS;
  ConstantFlag    = TRUE;
  OriginalDefaultId  = DefaultId;
  DefaultLink        = GetFirstNode (&FormSet->DefaultStoreListHead);

  //
  // Statement don't have storage, skip them
  //
  if (Question->QuestionId == 0) {
    return EFI_NOT_FOUND;
  }
  //
  // Return if no any kinds of
  //
  if ((Question->Operand != EFI_IFR_CHECKBOX_OP)
    && (Question->Operand != EFI_IFR_ONE_OF_OP)
    && (Question->Operand != EFI_IFR_ORDERED_LIST_OP)
    && (Question->Operand != EFI_IFR_NUMERIC_OP)
    && (Question->Operand != EFI_IFR_STRING_OP)
    ) {
    return EFI_ABORTED;
  }
  //
  // Search the variable for this question (Compatible with the old EfiVarStore before UEFI2.31)
  //

  //
  //VarStoreInfoDepending on the type of variable store selected,
  //this contains either a 16-bit Buffer Storage offset (VarOffset)
  //or a Name/Value or EFI Variable name (VarName).
  //
  Status = SearchVarStorage (
             Question,
             NULL,
             Question->VarStoreInfo.VarOffset,
             FormSet->StorageListHead,
             (CHAR8 **)&VarBuffer,
             &VarList
             );
  if (EFI_ERROR(Status)) {
    return Status;
  }
  //
  // There are three ways to specify default value for a Question:
  //  1, use nested EFI_IFR_DEFAULT
  //  2, set flags of EFI_ONE_OF_OPTION (provide Standard and Manufacturing default)
  //  3, set flags of EFI_IFR_CHECKBOX (provide Standard and Manufacturing default) (lowest priority)
  //
ReGetDefault:
  HiiValue = &Question->HiiValue;
  //
  // EFI_IFR_DEFAULT has highest priority
  //
  if (!IsListEmpty (&Question->DefaultListHead)) {
    Link = GetFirstNode (&Question->DefaultListHead);
    while (!IsNull (&Question->DefaultListHead, Link)) {
      Default = QUESTION_DEFAULT_FROM_LINK (Link);

      if (Default->DefaultId == DefaultId) {
        if (Default->ValueExpression != NULL) {
          //
          // Default is provided by an Expression, evaluate it
          //
          Status = EvaluateExpression (FormSet, Form, Default->ValueExpression, &ConstantFlag);
          if (EFI_ERROR (Status)) {
            return Status;
          }

          if (Default->ValueExpression->Result.Type == EFI_IFR_TYPE_BUFFER) {
            if (Question->StorageWidth > Default->ValueExpression->Result.BufferLen) {
              CopyMem(Question->HiiValue.Buffer, Default->ValueExpression->Result.Buffer, Default->ValueExpression->Result.BufferLen);
              Question->HiiValue.BufferLen = Default->ValueExpression->Result.BufferLen;
            } else {
              CopyMem(Question->HiiValue.Buffer, Default->ValueExpression->Result.Buffer, Question->StorageWidth);
              Question->HiiValue.BufferLen = Question->StorageWidth;
            }
            FreePool(Default->ValueExpression->Result.Buffer);
          }
          HiiValue->Type = Default->ValueExpression->Result.Type;
          CopyMem(&HiiValue->Value, &Default->ValueExpression->Result.Value, sizeof (EFI_IFR_TYPE_VALUE));
        } else {
          //
          // Default value is embedded in EFI_IFR_DEFAULT
          //
          if (Default->Value.Type == EFI_IFR_TYPE_BUFFER) {
            CopyMem(HiiValue->Buffer, Default->Value.Buffer, Default->Value.BufferLen);
          } else {
            CopyMem(HiiValue, &Default->Value, sizeof (EFI_HII_VALUE));
          }
        }
        if (Default->Value.Type == EFI_IFR_TYPE_BUFFER) {
          CopyMem(VarBuffer, HiiValue->Buffer, HiiValue->BufferLen);
        } else if (HiiValue->Type == EFI_IFR_TYPE_STRING){
          Status = FindDefaultName (
                 &(FormSet->EnUsStringList),
                 FormSet->UnicodeBinary,
                 HiiValue->Value.string,
                 EN_US,
                 &VarDefaultName
                 );
          if (VarDefaultName == NULL) {
            return EFI_NOT_FOUND;
          }
          if (Question->StorageWidth > FceStrSize(VarDefaultName)) {
            ZeroMem (VarBuffer, Question->StorageWidth);
            CopyMem (VarBuffer, VarDefaultName, FceStrSize(VarDefaultName));
          } else {
            CopyMem (VarBuffer, VarDefaultName, Question->StorageWidth);
          }
        } else {
          if (Question->QuestionReferToBitField) {
            SetBitsQuestionValue(Question, VarBuffer, HiiValue->Value.u32);
          } else {
            CopyMem(VarBuffer, &HiiValue->Value.u64, Question->StorageWidth);
          }
        }
        return EFI_SUCCESS;
      }
      if (Default->DefaultId == DefaultId) {
          return EFI_SUCCESS;
       }
      Link = GetNextNode (&Question->DefaultListHead, Link);
    }
  }

  if (HiiValue->Buffer == NULL) {
    ZeroMem (HiiValue, sizeof (EFI_HII_VALUE));
  }

  //
  // EFI_ONE_OF_OPTION
  //
  if ((Question->Operand == EFI_IFR_ONE_OF_OP) && !IsListEmpty (&Question->OptionListHead)) {
    if (DefaultId <= EFI_HII_DEFAULT_CLASS_MANUFACTURING)  {
      //
      // OneOfOption could only provide Standard and Manufacturing default
      //
      Link = GetFirstNode (&Question->OptionListHead);
      while (!IsNull (&Question->OptionListHead, Link)) {
        Option = QUESTION_OPTION_FROM_LINK (Link);

        if (((DefaultId == EFI_HII_DEFAULT_CLASS_STANDARD) && ((Option->Flags & EFI_IFR_OPTION_DEFAULT) != 0)) ||
            ((DefaultId == EFI_HII_DEFAULT_CLASS_MANUFACTURING) && ((Option->Flags & EFI_IFR_OPTION_DEFAULT_MFG) != 0))
           ) {
          CopyMem (HiiValue, &Option->Value, sizeof (EFI_HII_VALUE));
          if (Question->QuestionReferToBitField) {
            SetBitsQuestionValue(Question, VarBuffer, HiiValue->Value.u32);
          } else {
            CopyMem (VarBuffer, &HiiValue->Value.u64, Question->StorageWidth);
          }
          return EFI_SUCCESS;
        }

        Link = GetNextNode (&Question->OptionListHead, Link);
      }
    }
  }

  //
  // EFI_IFR_CHECKBOX - lowest priority
  //
  if (Question->Operand == EFI_IFR_CHECKBOX_OP) {
    if (DefaultId <= EFI_HII_DEFAULT_CLASS_MANUFACTURING)  {
      //
      // Checkbox could only provide Standard and Manufacturing default
      //
      if (((DefaultId == EFI_HII_DEFAULT_CLASS_STANDARD) && ((Question->Flags & EFI_IFR_CHECKBOX_DEFAULT) != 0)) ||
          ((DefaultId == EFI_HII_DEFAULT_CLASS_MANUFACTURING) && ((Question->Flags & EFI_IFR_CHECKBOX_DEFAULT_MFG) != 0))
         ) {
        HiiValue->Value.b = TRUE;
        if (Question->QuestionReferToBitField) {
          SetBitsQuestionValue(Question, VarBuffer, HiiValue->Value.u32);
        } else {
          CopyMem (VarBuffer, &HiiValue->Value.u64, Question->StorageWidth);
        }
        return EFI_SUCCESS;
      }
    }
  }

  //
  // For question without default value for current default Id, we try to re-get the default value form other default id in the DefaultStoreList.
  // If get, will exit the function, if not, will choose next default id in the DefaultStoreList.
  // The default id in DefaultStoreList are in ascending order to make sure choose the smallest default id every time.
  //
  while (!IsNull(&FormSet->DefaultStoreListHead, DefaultLink)) {
    DefaultStore = FORMSET_DEFAULTSTORE_FROM_LINK(DefaultLink);
    DefaultLink = GetNextNode (&FormSet->DefaultStoreListHead,DefaultLink);
    DefaultId = DefaultStore->DefaultId;
    if (DefaultId == OriginalDefaultId) {
      continue;
    }
    goto ReGetDefault;
  }

  //
  // For Questions without default
  //
  Status = EFI_NOT_FOUND;
  switch (Question->Operand) {
  case EFI_IFR_CHECKBOX_OP:
    HiiValue->Value.b = FALSE;
    if (Question->QuestionReferToBitField) {
      SetBitsQuestionValue(Question, VarBuffer, HiiValue->Value.u32);
    } else {
      CopyMem (VarBuffer, &HiiValue->Value.u64, Question->StorageWidth);
    }
    break;

  case EFI_IFR_NUMERIC_OP:
    //
    // Take minimum value as numeric default value
    //
    if ((HiiValue->Value.u64 < Question->Minimum) || (HiiValue->Value.u64 > Question->Maximum)) {
      HiiValue->Value.u64 = Question->Minimum;
      if (Question->QuestionReferToBitField) {
        SetBitsQuestionValue(Question, VarBuffer, HiiValue->Value.u32);
      } else {
        CopyMem (VarBuffer, &HiiValue->Value.u64, Question->StorageWidth);
      }
      return EFI_SUCCESS;
    }
    break;

  case EFI_IFR_ONE_OF_OP:
    //
    // Take first oneof option as oneof's default value
    //
    if (ValueToOption (FormSet, Question, HiiValue) == NULL) {
      Link = GetFirstNode (&Question->OptionListHead);
      if (!IsNull (&Question->OptionListHead, Link)) {
        Option = QUESTION_OPTION_FROM_LINK (Link);
        CopyMem (HiiValue, &Option->Value, sizeof (EFI_HII_VALUE));
        if (Question->QuestionReferToBitField) {
          SetBitsQuestionValue(Question, VarBuffer, HiiValue->Value.u32);
        } else {
          CopyMem (VarBuffer, &HiiValue->Value.u64, Question->StorageWidth);
        }
        return EFI_SUCCESS;
      }
    }
    break;

  case EFI_IFR_ORDERED_LIST_OP:
    //
    // Take option sequence in IFR as ordered list's default value
    //
    Index = 0;
    Link = GetFirstNode (&Question->OptionListHead);
    while (!IsNull (&Question->OptionListHead, Link)) {
      Option = QUESTION_OPTION_FROM_LINK (Link);

      SetArrayData (Question->BufferValue, Question->ValueType, Index, Option->Value.Value.u64);
      SetArrayData (VarBuffer, Question->ValueType, Index, Option->Value.Value.u64);

      Index++;
      if (Index >= Question->MaxContainers) {
        break;
      }

      Link = GetNextNode (&Question->OptionListHead, Link);
    }
    break;

  default:
    break;
  }

  return EFI_SUCCESS;
}

/**
  Set the value to the variable of platformId question.

  @param  PlatformId             The form set.

  @retval EFI_SUCCESS            Set successfully.

**/
EFI_STATUS
AssignThePlatformId (
  IN  UINT64   PlatformId
  )
{
  EFI_STATUS          Status;
  FORMSET_STORAGE     *VarList;
  UINT8               *VarBuffer;

  Status       = EFI_SUCCESS;
  VarBuffer    = NULL;
  //
  // Set the Storage
  //
  Status = SearchVarStorage (
             &mMultiPlatformParam.PlatformIdQuestion,
             NULL,
             mMultiPlatformParam.PlatformIdQuestion.VarStoreInfo.VarOffset,
             &mVarListEntry,
             (CHAR8 **)&VarBuffer,
             &VarList
             );
  if (EFI_ERROR(Status)) {
    return Status;
  }
  CopyMem (VarBuffer, &PlatformId, mMultiPlatformParam.PlatformIdWidth);
  //
  // Set the  HIIvalue of this questions
  //
  CopyMem (&mMultiPlatformParam.Question->HiiValue.Value.u64, &PlatformId, mMultiPlatformParam.PlatformIdWidth);

  switch (mMultiPlatformParam.PlatformIdWidth) {
    case sizeof (UINT8):
      mMultiPlatformParam.Question->HiiValue.Type = EFI_IFR_TYPE_NUM_SIZE_8;
      break;

    case sizeof (UINT16):
      mMultiPlatformParam.Question->HiiValue.Type = EFI_IFR_TYPE_NUM_SIZE_16;
      break;

    case sizeof (UINT32):
      mMultiPlatformParam.Question->HiiValue.Type = EFI_IFR_TYPE_NUM_SIZE_32;
      break;

    case sizeof (UINT64):
      mMultiPlatformParam.Question->HiiValue.Type = EFI_IFR_TYPE_NUM_SIZE_64;
      break;

    default:
      mMultiPlatformParam.Question->HiiValue.Type = EFI_IFR_TYPE_NUM_SIZE_64;
   }
  return EFI_SUCCESS;
}
/**
  Reset Questions to their default value in a Form, Formset or System.

  @param  FormSet                FormSet data structure.
  @param  Form                   Form data structure.
  @param  DefaultId              The default Id
  @param  PlatformId             The platform Id
  @param  SettingScope           Setting Scope for Default action.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_UNSUPPORTED        Unsupport SettingScope.

**/
EFI_STATUS
ExtractDefault (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form,
  IN UINT16                           DefaultId,
  IN UINT64                           PlatformId,
  IN BROWSER_SETTING_SCOPE            SettingScope
  )
{
  EFI_STATUS              Status;
  LIST_ENTRY              *FormLink;
  LIST_ENTRY              *Link;
  LIST_ENTRY              *FormSetEntryListHead;
  FORM_BROWSER_STATEMENT  *Question;
  //
  // Check the supported setting level.
  //
  if (SettingScope >= MaxLevel) {
    return EFI_UNSUPPORTED;
  }

  if (SettingScope == FormLevel) {
    //
    // Extract Form default
    //
    Link = GetFirstNode (&Form->StatementListHead);
    while (!IsNull (&Form->StatementListHead, Link)) {
      Question = FORM_BROWSER_STATEMENT_FROM_LINK (Link);
      Link = GetNextNode (&Form->StatementListHead, Link);
      //
      // Re-set the platformId before calcuate the platformId of every question to avoid over-written.
      //
      if (mMultiPlatformParam.MultiPlatformOrNot) {
        Status = AssignThePlatformId (PlatformId);
        if (EFI_ERROR (Status)) {
          StringPrint ("Error. Failed to assign the platformId.\n");
          return Status;
        }
      }
      //
      // Reset Question to its default value, and store the default to variable
      //
      Status = GetQuestionDefault (FormSet, Form, Question, DefaultId, PlatformId);
      if (EFI_ERROR (Status)) {
        continue;
      }
    }
  } else if (SettingScope == FormSetLevel) {
    FormLink = GetFirstNode (&FormSet->FormListHead);
    while (!IsNull (&FormSet->FormListHead, FormLink)) {
      Form = FORM_BROWSER_FORM_FROM_LINK (FormLink);
      ExtractDefault (FormSet, Form, DefaultId, PlatformId, FormLevel);
      FormLink = GetNextNode (&FormSet->FormListHead, FormLink);
    }
  } else if (SettingScope == SystemLevel) {
    //
    // Parse Fromset one by one
    //
    FormSetEntryListHead = &mFormSetListEntry;

    FormLink = GetFirstNode (FormSetEntryListHead);
    while (!IsNull (FormSetEntryListHead, FormLink)) {
      FormSet = FORM_BROWSER_FORMSET_FROM_LINK (FormLink);
      ExtractDefault (FormSet, NULL, DefaultId, PlatformId, FormSetLevel);
      FormLink = GetNextNode (FormSetEntryListHead, FormLink);
    }
  }

  return EFI_SUCCESS;
}

/**
  Check whether existed the UQI in Current Unicode String.

  @param  UniPackge         A pointer to a Null-terminated Unicode string Array.

  @return TRUE              If find the uqi, return TRUE
  @return FALSE             Otherwise, return FALSE

**/
static
BOOLEAN
IsUqiOrNot (
  IN  UINT8  *UniPackge
  )
{
  CHAR8          *UniBin;
  UINTN          UniLength;
  UINTN          Index;
  BOOLEAN        FindIt;

  UniBin     = (CHAR8 *) UniPackge + 4;
  Index      = 4;
  FindIt     = FALSE;
  UniLength  = *(UINT32 *) UniPackge;

  if (((EFI_HII_PACKAGE_HEADER *)UniBin)->Type == EFI_HII_PACKAGE_STRINGS) {
    //
    // Search the uqi language
    //
    while ((Index < UniLength) && ((EFI_HII_PACKAGE_HEADER *)UniBin)->Type == EFI_HII_PACKAGE_STRINGS){
      if (!strcmp (((EFI_HII_STRING_PACKAGE_HDR *)UniBin)->Language, "uqi")) {
        FindIt = TRUE;
        break;
      }
      Index = Index + ((EFI_HII_STRING_PACKAGE_HDR *)UniBin)->Header.Length;
      UniBin += ((EFI_HII_STRING_PACKAGE_HDR *)UniBin)->Header.Length;
    }
  }
  return FindIt;
}

 /**
  Returns Length of UQI string (in CHAR16) (not including null termination).

  @param  UniPackge         A pointer to a UQI string.

  @return Number            Length of UQIL string (in words) or 0

**/
static
UINT16
GetUqiNum (
  IN     CHAR16        *UniString
  )
{
  UINT16              Number;

  if (UniString == NULL) {
    return 0;
  }
  for (Number = 0; UniString[Number] != 0; Number++) {
    ;
  }
  return Number;
}

/**
  Print the formset title information.

  @param  FormSet    The pointer to the formset.

  @return NULL.

**/
static
VOID
StringPrintormSetTitle (
  IN  FORM_BROWSER_FORMSET  *FormSet
  )
{
  CHAR16      *VarDefaultName;
  EFI_STATUS  Status;

  VarDefaultName = NULL;

  StringPrint("\n\n// Form Set: ");

  Status  = FindDefaultName (
              &(FormSet->EnUsStringList),
              FormSet->UnicodeBinary,
              FormSet->FormSetTitle,
              EN_US,
              &VarDefaultName
             );
  assert (!EFI_ERROR (Status));
  LogUnicodeString (VarDefaultName);

  StringPrint("\n// %s",FORM_SET_GUID_PREFIX);
  StringPrint(
    EFI_GUID_FORMAT,
    FormSet->Guid.Data1,   FormSet->Guid.Data2,
    FormSet->Guid.Data3,   FormSet->Guid.Data4[0],
    FormSet->Guid.Data4[1],FormSet->Guid.Data4[2],
    FormSet->Guid.Data4[3],FormSet->Guid.Data4[4],
    FormSet->Guid.Data4[5],FormSet->Guid.Data4[6],
    FormSet->Guid.Data4[7]);
  StringPrint("\n");

  if (&(FormSet->EnUsStringList) == NULL && VarDefaultName != NULL && FormSet->FormSetTitle != 0) {
    free (VarDefaultName);
    VarDefaultName = NULL;
  }
}

/**
  Print the formset title information.

  @param  FormSet    The pointer to the formset.
  @param  Question   The pointer to the question of ONE_OF.

  @return NULL.

**/
static
EFI_STATUS
PrintOneOfOptions (
  IN  FORM_BROWSER_FORMSET    *FormSet,
  IN  FORM_BROWSER_STATEMENT  *Question
  )
{
  LIST_ENTRY       *Link;
  QUESTION_OPTION  *Option;
  CHAR16           *VarDefaultName;
  EFI_STATUS       Status;

  Status = EFI_SUCCESS;
  VarDefaultName = NULL;

  if ((Question->Operand != EFI_IFR_ONE_OF_OP)
    && (Question->Operand != EFI_IFR_ORDERED_LIST_OP)
    ) {
    return EFI_ABORTED;
  }

  Link = GetFirstNode (&Question->OptionListHead);
  while (!IsNull (&Question->OptionListHead, Link)) {
    Option = QUESTION_OPTION_FROM_LINK (Link);
    if (Question->QuestionReferToBitField) {
      StringPrint("// %08X = ", Option->Value.Value.u32);
    } else {
      switch(Option->Value.Type) {

        case EFI_IFR_TYPE_NUM_SIZE_8:
          StringPrint("// %02X = ", Option->Value.Value.u8);
          break;

        case EFI_IFR_TYPE_NUM_SIZE_16:
          StringPrint("// %04X = ", Option->Value.Value.u16);
          break;

        case EFI_IFR_TYPE_NUM_SIZE_32:
          StringPrint("// %08X = ", Option->Value.Value.u32);
          break;

        case EFI_IFR_TYPE_NUM_SIZE_64:
          StringPrint("// %016llX = ", Option->Value.Value.u64);
          break;

        case EFI_IFR_TYPE_BOOLEAN:
          StringPrint("// %X = ", Option->Value.Value.b);
          break;

        case EFI_IFR_TYPE_STRING:
          StringPrint("// %X = ", Option->Value.Value.string);
          break;

        default:
          break;
        }
      }
    Status = FindDefaultName (
               &(FormSet->EnUsStringList),
               FormSet->UnicodeBinary,
               Option->Text,
               EN_US,
               &VarDefaultName
               );

    LogUnicodeString (VarDefaultName);
    StringPrint("\n");
    if (&(FormSet->EnUsStringList) == NULL && VarDefaultName != NULL && Option->Text != 0) {
      free (VarDefaultName);
      VarDefaultName = NULL;
    }
    Link = GetNextNode (&Question->OptionListHead, Link);
  }
  return Status;
}

/**
  Print the form title information.

  @param  FormSet    The pointer to the formset.
  @param  FormSet    The pointer to the form.

  @return NULL.

**/
static
VOID
StringPrintormTitle (
  IN  FORM_BROWSER_FORMSET  *FormSet,
  IN  FORM_BROWSER_FORM     *Form
  )
{
  CHAR16      *VarDefaultName;
  EFI_STATUS  Status;

  VarDefaultName = NULL;

  StringPrint("\n// Form: ");
  Status  = FindDefaultName (
              &(FormSet->EnUsStringList),
              FormSet->UnicodeBinary,
              Form->FormTitle,
              EN_US,
              &VarDefaultName
             );
  assert (!EFI_ERROR (Status));

  LogUnicodeString (VarDefaultName);
  StringPrint("\n");

  if (&(FormSet->EnUsStringList) == NULL && VarDefaultName != NULL && Form->FormTitle != 0) {
    free (VarDefaultName);
    VarDefaultName  = NULL;
  }

}

/**
  Print the information of questions.

  @param  FormSet     The pointer to the formset.
  @param  FormSet     The pointer to the form.
  @param  Question    The pointer to the question.
  @param  PrintOrNot  Decide whether print or not.

  @return NULL.

**/
static
VOID
PrintQuestion (
  IN  FORM_BROWSER_FORMSET    *FormSet,
  IN  FORM_BROWSER_FORM       *Form,
  IN  FORM_BROWSER_STATEMENT  *Question,
  IN  BOOLEAN                  PrintOrNot
  )
{
  EFI_STATUS       Status;
  CHAR16           *VarDefaultName;
  UINT16           UqiStringLength;
  BOOLEAN          HaveUQIlanguage;

  Status           = EFI_SUCCESS;
  VarDefaultName   = NULL;
  UqiStringLength  = 0;

  HaveUQIlanguage = IsUqiOrNot (FormSet->UnicodeBinary);

  switch (Question->Operand) {

  case EFI_IFR_SUBTITLE_OP:
  if (PrintOrNot) {
    Status  = FindDefaultName (
                &(FormSet->EnUsStringList),
                FormSet->UnicodeBinary,
                Question->Prompt,
                EN_US,
                &VarDefaultName
                );
    assert (!EFI_ERROR (Status));
    if ((VarDefaultName != NULL) && (FceStrCmp (VarDefaultName, L"") != 0)) {
      StringPrint("// Subtitle: ");
      StringPrint("// ");
      LogUnicodeString (VarDefaultName);
      StringPrint("\n");
    }
  }
    break;

  case EFI_IFR_ONE_OF_OP:

    if( HaveUQIlanguage ) {
      Status = FindDefaultName (
                 &(FormSet->UqiStringList),
                 FormSet->UnicodeBinary,
                 Question->Prompt,
                 UQI,
                 &VarDefaultName
                 );
      assert (!EFI_ERROR (Status));

      UqiStringLength = GetUqiNum (VarDefaultName);
      if (PrintOrNot) {
        if (UqiStringLength > 0) {
          StringPrint("\nQ %04X ", UqiStringLength);
          LogUqi(VarDefaultName);
        } else {
          StringPrint("\n// [No UQI] ");
        }
      }
    } else {
      Status       = FindDefaultName (
                       &(FormSet->EnUsStringList),
                       FormSet->UnicodeBinary,
                       Question->Prompt,
                       EN_US,
                       &VarDefaultName
                     );
      assert (!EFI_ERROR (Status));
      UqiStringLength = GetUqiNum (VarDefaultName);

      if (PrintOrNot) {
        if (UqiStringLength > 0) {
          StringPrint("\nQ %04X ", UqiStringLength);
          LogUqi(VarDefaultName);
        } else {
          StringPrint("\n// [No UQI] ");
        }
      }
    }
    //
    //Record the UQi to the Question
    //
    Question->Uqi.HexNum = UqiStringLength;
    Question->Uqi.Data   = VarDefaultName;
    Question->Uqi.Type   = ONE_OF;

    if (PrintOrNot) {
      StringPrint("ONE_OF ");

      LogIfrValue (
        FormSet,
        Question
        );
      StringPrint(" // ");
      Status       = FindDefaultName (
                       &(FormSet->EnUsStringList),
                       FormSet->UnicodeBinary,
                       Question->Prompt,
                       EN_US,
                       &VarDefaultName
                       );
      assert (!EFI_ERROR (Status));
      LogUnicodeString (VarDefaultName);
      StringPrint("\n");
      //
      // Print ONE_OF_OPTION
      //
      PrintOneOfOptions (FormSet, Question);
    }
    break;

  case EFI_IFR_CHECKBOX_OP:

    if( HaveUQIlanguage ) {
      Status = FindDefaultName (
                 &(FormSet->UqiStringList),
                 FormSet->UnicodeBinary,
                 Question->Prompt,
                 UQI,
                 &VarDefaultName
                 );
      assert (!EFI_ERROR (Status));

      UqiStringLength = GetUqiNum (VarDefaultName);
      if (PrintOrNot) {
        if (UqiStringLength > 0) {
          StringPrint("\nQ %04X ", UqiStringLength);
          LogUqi(VarDefaultName);
        } else {
          StringPrint("\n// [No UQI] ");
        }
      }
    } else {
      Status = FindDefaultName (
                 &(FormSet->EnUsStringList),
                 FormSet->UnicodeBinary,
                 Question->Prompt,
                 EN_US,
                 &VarDefaultName
                 );
      assert (!EFI_ERROR (Status));

      UqiStringLength = GetUqiNum (VarDefaultName);

      if (PrintOrNot) {
        if (UqiStringLength > 0) {
          StringPrint("\nQ %04X ", UqiStringLength);
          LogUqi(VarDefaultName);
        } else {
          StringPrint("\n// [No UQI] ");
        }
      }
    }
    //
    //Record the UQi to the HiiObjList
    //
    Question->Uqi.HexNum = UqiStringLength;
    Question->Uqi.Data   = VarDefaultName;
    Question->Uqi.Type   = CHECKBOX;
    if (PrintOrNot) {
      StringPrint("CHECKBOX ");
      LogIfrValue (
        FormSet,
        Question
        );
      StringPrint(" // ");
      Status = FindDefaultName (
                 &(FormSet->EnUsStringList),
                 FormSet->UnicodeBinary,
                 Question->Prompt,
                 EN_US,
                 &VarDefaultName
                 );
      assert (!EFI_ERROR (Status));
      LogUnicodeString (VarDefaultName);
      StringPrint("\n");
      StringPrint("// 0 = Unchecked\n");
      StringPrint("// 1 = Checked\n");
    }
    break;

  case EFI_IFR_STRING_OP:
    if( HaveUQIlanguage ) {
      Status = FindDefaultName (
                 &(FormSet->UqiStringList),
                 FormSet->UnicodeBinary,
                 Question->Prompt,
                 UQI,
                 &VarDefaultName
                 );
      assert (!EFI_ERROR (Status));

      UqiStringLength = GetUqiNum (VarDefaultName);
      if (PrintOrNot) {
        if (UqiStringLength > 0) {
          StringPrint("\nQ %04X ", UqiStringLength);
          LogUqi(VarDefaultName);
        } else {
          StringPrint("\n// [No UQI] ");
        }
      }
    } else {
      Status = FindDefaultName (
                 &(FormSet->EnUsStringList),
                 FormSet->UnicodeBinary,
                 Question->Prompt,
                 EN_US,
                 &VarDefaultName
                 );
      assert (!EFI_ERROR (Status));

      UqiStringLength = GetUqiNum (VarDefaultName);

      if (PrintOrNot) {
        if (UqiStringLength > 0) {
          StringPrint("\nQ %04X ", UqiStringLength);
          LogUqi(VarDefaultName);
        } else {
          StringPrint("\n// [No UQI] ");
        }
      }
    }
    //
    //Record the UQi to the HiiObjList
    //
    Question->Uqi.HexNum = UqiStringLength;
    Question->Uqi.Data   = VarDefaultName;
    Question->Uqi.Type   = STRING;
    if (PrintOrNot) {
      StringPrint("STRING ");
      LogIfrValueStr (
        FormSet,
        Question
        );
      StringPrint(" // ");
      Status = FindDefaultName (
                 &(FormSet->EnUsStringList),
                 FormSet->UnicodeBinary,
                 Question->Prompt,
                 EN_US,
                 &VarDefaultName
                 );
      assert (!EFI_ERROR (Status));
      LogUnicodeString (VarDefaultName);
      StringPrint("\n");
    }
    break;

  case EFI_IFR_NUMERIC_OP:

    if( HaveUQIlanguage ) {
      Status = FindDefaultName (
                 &(FormSet->UqiStringList),
                 FormSet->UnicodeBinary,
                 Question->Prompt,
                 UQI,
                 &VarDefaultName
                 );
      assert (!EFI_ERROR (Status));

      UqiStringLength = GetUqiNum (VarDefaultName);
      if (PrintOrNot) {
        if (UqiStringLength > 0) {
          StringPrint("\nQ %04X ", UqiStringLength);
          LogUqi(VarDefaultName);
        } else {
          StringPrint("\n// [No UQI] ");
        }
      }
    } else {
      Status = FindDefaultName (
                 &(FormSet->EnUsStringList),
                 FormSet->UnicodeBinary,
                 Question->Prompt,
                 EN_US,
                 &VarDefaultName
                 );
      assert (!EFI_ERROR (Status));

      UqiStringLength = GetUqiNum (VarDefaultName);
      if (PrintOrNot) {
        if (UqiStringLength > 0) {
          StringPrint("\nQ %04X ", UqiStringLength);
          LogUqi(VarDefaultName);
        } else {
          StringPrint("\n// [No UQI] ");
        }
      }
    }
    //
    //Record the UQi to the HiiObjList
    //
    Question->Uqi.HexNum = UqiStringLength;
    Question->Uqi.Data   = VarDefaultName;
    Question->Uqi.Type   = NUMERIC;
    if (PrintOrNot) {
      StringPrint("NUMERIC ");
      LogIfrValue (
        FormSet,
        Question
      );
      StringPrint(" // ");
      Status = FindDefaultName (
                 &(FormSet->EnUsStringList),
                 FormSet->UnicodeBinary,
                 Question->Prompt,
                 EN_US,
                 &VarDefaultName
                 );
      assert (!EFI_ERROR (Status));
      LogUnicodeString (VarDefaultName);
      StringPrint("\n");

      if (Question->QuestionReferToBitField) {
        StringPrint("// Minimum = %08llX \n", Question->Minimum);
        StringPrint("// Maximum = %08llX \n", Question->Maximum);
        StringPrint("// Step    = %08llX \n", Question->Step);
      } else {
        switch (Question->StorageWidth) {

        case sizeof (UINT8):
          StringPrint("// Minimum = %02llX \n", Question->Minimum);
          StringPrint("// Maximum = %02llX \n", Question->Maximum);
          StringPrint("// Step    = %02llX \n", Question->Step);
          break;

        case sizeof (UINT16):
          StringPrint("// Minimum = %04llX \n", Question->Minimum);
          StringPrint("// Maximum = %04llX \n", Question->Maximum);
          StringPrint("// Step    = %04llX \n", Question->Step);
          break;

        case sizeof (UINT32):
          StringPrint("// Minimum = %08llX \n", Question->Minimum);
          StringPrint("// Maximum = %08llX \n", Question->Maximum);
          StringPrint("// Step    = %08llX \n", Question->Step);
          break;

        case sizeof (UINT64):
          StringPrint("// Minimum = %016llX \n", Question->Minimum);
          StringPrint("// Maximum = %016llX \n", Question->Maximum);
          StringPrint("// Step    = %016llX \n", Question->Step);
          break;

        default:
          StringPrint("0000 // Width > 16 is not supported -- FAILURE");
          break;
        }
      }
    }
    break;

  case EFI_IFR_ORDERED_LIST_OP:

    if( HaveUQIlanguage ) {
      Status = FindDefaultName (
                 &(FormSet->UqiStringList),
                 FormSet->UnicodeBinary,
                 Question->Prompt,
                 UQI,
                 &VarDefaultName
                 );
      assert (!EFI_ERROR (Status));
      UqiStringLength = GetUqiNum (VarDefaultName);

      if (PrintOrNot) {
        if (UqiStringLength > 0) {
          StringPrint("\nQ %04X ", UqiStringLength);
          LogUqi(VarDefaultName);
        } else {
          StringPrint("\n// [No UQI] ");
        }
      }
    } else {
      Status = FindDefaultName (
                 &(FormSet->EnUsStringList),
                 FormSet->UnicodeBinary,
                 Question->Prompt,
                 EN_US,
                 &VarDefaultName
                 );

      assert (!EFI_ERROR (Status));

      UqiStringLength = GetUqiNum (VarDefaultName);
      if (PrintOrNot) {
        if (UqiStringLength > 0) {
          StringPrint("\nQ %04X ", UqiStringLength);
          LogUqi(VarDefaultName);
        } else {
          StringPrint("\n// [No UQI] ");
        }
      }
    }
    //
    //Record the UQi to the HiiObjList
    //
    Question->Uqi.HexNum = UqiStringLength;
    Question->Uqi.Data   = VarDefaultName;
    Question->Uqi.Type   = ORDERED_LIST;

    if (PrintOrNot) {
      StringPrint("ORDERED_LIST %04X ", Question->MaxContainers);

      LogIfrValueList (
        FormSet,
        Question
        );
      StringPrint(" // ");
      Status = FindDefaultName (
                 &(FormSet->EnUsStringList),
                 FormSet->UnicodeBinary,
                 Question->Prompt,
                 EN_US,
                 &VarDefaultName
                 );
      assert (!EFI_ERROR (Status));
      LogUnicodeString (VarDefaultName);
      StringPrint("\n");
    }
    //
    // Print ONE_OF_OPTION
    //
    PrintOneOfOptions (FormSet, Question);
    break;

  default:
    break;
  }

  if (&(FormSet->EnUsStringList) == NULL &&VarDefaultName != NULL && Question->Prompt != 0) {
    free (VarDefaultName);
    VarDefaultName = NULL;
  }

  if (PrintOrNot && Question->Storage) {
    StringPrint("// size = 0x%x", Question->StorageWidth);
    StringPrint("\n// offset = 0x%x", Question->VarStoreInfo.VarOffset);
    StringPrint("\n// name = ");
    LogUnicodeString(Question->VariableName);
    StringPrint("\n// guid = ");
    StringPrint(
      EFI_GUID_FORMAT,
      Question->Guid.Data1,   Question->Guid.Data2,
      Question->Guid.Data3,   Question->Guid.Data4[0],
      Question->Guid.Data4[1],Question->Guid.Data4[2],
      Question->Guid.Data4[3],Question->Guid.Data4[4],
      Question->Guid.Data4[5],Question->Guid.Data4[6],
      Question->Guid.Data4[7]
    );
    StringPrint("\n// attribute = 0x%x", Question->Attributes);
    StringPrint("\n// help = ");
    Status = FindDefaultName (
               &(FormSet->EnUsStringList),
               FormSet->UnicodeBinary,
               Question->Help,
               EN_US,
               &VarDefaultName
               );
    assert (!EFI_ERROR (Status));
    LogUnicodeString (VarDefaultName);
    StringPrint("\n");
    if (&(FormSet->EnUsStringList) == NULL &&VarDefaultName != NULL && Question->Help != 0) {
      free (VarDefaultName);
      VarDefaultName = NULL;
    }
  }

}

/**
  Check whether current Formset or Form is NULL. If no valid questions, return FASLE.

  @param  FormSet     The pointer to the formset.
  @param  FormSet     The pointer to the form.
  @param  IsFormSet   FormSet or Form.

  @retval TRUE
  @return FALSE
**/
BOOLEAN
CheckFormSetOrFormNull (
  IN  FORM_BROWSER_FORMSET    *FormSet,
  IN  FORM_BROWSER_FORM       *Form,
  IN  BOOLEAN                  IsFormSet
  )
{
  LIST_ENTRY              *FormLink;
  FORM_BROWSER_STATEMENT  *Question;
  LIST_ENTRY              *QuestionLink;

  FormLink     = NULL;
  Question     = NULL;
  QuestionLink = NULL;

  //
  // Parse all forms in formset
  //
  if (IsFormSet) {
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
          if (mMultiPlatformParam.MultiPlatformOrNot) {
            //
            // Only compare the valid EFI_IFR_VARSTORE_EFI_OP in multi-platform mode
            //
            if (Question->Type != EFI_IFR_VARSTORE_EFI_OP) {
              QuestionLink = GetNextNode (&Form->StatementListHead, QuestionLink);
              continue;
            }
            if (Question->Type == EFI_IFR_VARSTORE_EFI_OP
              && Question->NewEfiVarstore
              && ((Question->Attributes & EFI_VARIABLE_NON_VOLATILE) == 0)) {
              QuestionLink = GetNextNode (&Form->StatementListHead, QuestionLink);
              continue;
            }
          }
          //
          //If invalid variable type, skip it.
          //
           if ((Question->Type != EFI_IFR_VARSTORE_EFI_OP)
             && (Question->Type != EFI_IFR_VARSTORE_OP)) {
             QuestionLink = GetNextNode (&Form->StatementListHead, QuestionLink);
             continue;
          }
          return TRUE;
        }
        QuestionLink = GetNextNode (&Form->StatementListHead, QuestionLink);
      }

      FormLink = GetNextNode (&FormSet->FormListHead, FormLink);
    }
 } else {
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
       if (mMultiPlatformParam.MultiPlatformOrNot) {
         //
         // Only compare the valid EFI_IFR_VARSTORE_EFI_OP in multi-platform mode
         //
         if (Question->Type != EFI_IFR_VARSTORE_EFI_OP) {
           QuestionLink = GetNextNode (&Form->StatementListHead, QuestionLink);
           continue;
         }
         if ((Question->Type == EFI_IFR_VARSTORE_EFI_OP)
           && Question->NewEfiVarstore
           && ((Question->Attributes & EFI_VARIABLE_NON_VOLATILE) == 0)) {
           QuestionLink = GetNextNode (&Form->StatementListHead, QuestionLink);
           continue;
         }
       }
       //
       //If invalid variable type, skip it.
       //
       if ((Question->Type != EFI_IFR_VARSTORE_EFI_OP)
         && (Question->Type != EFI_IFR_VARSTORE_OP)) {
         QuestionLink = GetNextNode (&Form->StatementListHead, QuestionLink);
         continue;
       }
       return TRUE;
     }
     QuestionLink = GetNextNode (&Form->StatementListHead, QuestionLink);
   }
 }
 return FALSE;
}

/**
  Print all ONE_OF ORDER_LIST NUMERIC STRING and CHECKBOX in all fromsets.

  @param Formset        The pointer to the entry of the fromset list
  @param Formset        The pointer to the entry of the storage list

  @retval EFI_SUCCESS   It was complete successfully
  @return EFI_ABORTED   An error occurred
**/
EFI_STATUS
PrintInfoInAllFormset (
  IN LIST_ENTRY      *FormSetEntryListHead,
  IN LIST_ENTRY      *StorageEntryListHead
  )
{
  EFI_STATUS              Status;
  FORM_BROWSER_FORMSET    *FormSet;
  LIST_ENTRY              *FormSetLink;
  LIST_ENTRY              *FormLink;
  FORM_BROWSER_FORM       *Form;
  FORM_BROWSER_STATEMENT  *Question;
  LIST_ENTRY              *QuestionLink;
  FORMSET_STORAGE         *Storage;
  CHAR8                   *VarBuffer;
  LIST_ENTRY              *TempStorageLink;
  UINT32                  Index;
  BOOLEAN                 Skip;
  BOOLEAN                 ConstantFlag;

  Status          = EFI_SUCCESS;
  FormSet         = NULL;
  FormSetLink     = NULL;
  FormLink        = NULL;
  Form            = NULL;
  Question        = NULL;
  QuestionLink    = NULL;
  Storage         = NULL;
  VarBuffer       = NULL;
  TempStorageLink = NULL;
  Index           = 0;
  Skip            = FALSE;
  ConstantFlag    = TRUE;
  //
  // Print platformId, defaultId and platformIdUqi
  //
  if (mMultiPlatformParam.MultiPlatformOrNot) {
    StringPrint("\n\n// FCEKEY DEFAULT_ID:");
    TempStorageLink = GetFirstNode (StorageEntryListHead);
    Storage = FORMSET_STORAGE_FROM_LINK (TempStorageLink);
    for (Index = 0; Index <= Storage->DefaultPlatformIdNum; Index++) {
      StringPrint (" %4d", Storage->DefaultId[Index]);
    }
    StringPrint("\n\n//FCEKEY PLATFORM_ID:");
    for (Index = 0; Index <= Storage->DefaultPlatformIdNum; Index++) {
      StringPrint (" %4lld", Storage->PlatformId[Index]);
    }
    if (mMultiPlatformParam.Uqi.Data != NULL) {
      StringPrint("\n\n//FCEKEY PLATFORM_UQI:");
      StringPrint(" %04X ", mMultiPlatformParam.Uqi.HexNum);
      LogUqi(mMultiPlatformParam.Uqi.Data);
    }
  }
  FormSetLink = GetFirstNode (FormSetEntryListHead);
  while (!IsNull (FormSetEntryListHead, FormSetLink)) {
    FormSet = FORM_BROWSER_FORMSET_FROM_LINK (FormSetLink);
    //
    //Assign the new storage list
    //
    FormSet->StorageListHead = StorageEntryListHead;

    if (CheckFormSetOrFormNull (FormSet, NULL, TRUE)) {
      StringPrintormSetTitle (FormSet);
    } else {
      FormSetLink = GetNextNode (FormSetEntryListHead, FormSetLink);
      continue;
    }
    //
    // Parse all forms in formset
    //
    FormLink = GetFirstNode (&FormSet->FormListHead);

    while (!IsNull (&FormSet->FormListHead, FormLink)) {
      Form = FORM_BROWSER_FORM_FROM_LINK (FormLink);

      if (CheckFormSetOrFormNull (NULL, Form, FALSE)) {
        StringPrintormTitle(FormSet,Form);
      } else {
        FormLink = GetNextNode (&FormSet->FormListHead, FormLink);
        continue;
      }
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
          || (Question->Operand == EFI_IFR_SUBTITLE_OP)
          || (Question->Operand == EFI_IFR_STRING_OP)
          ) {
          Skip = FALSE;

          //
          //Only output the questions stored by EFI_IFR_VARSTORE_EFI_OP.
          //
          if (mMultiPlatformParam.MultiPlatformOrNot
            && (Question->Operand != EFI_IFR_SUBTITLE_OP)
            ) {
            Status = SearchVarStorage (
                       Question,
                       NULL,
                       Question->VarStoreInfo.VarOffset,
                       StorageEntryListHead,
                       (CHAR8 **)&VarBuffer,
                       &Storage
                     );

            if (EFI_ERROR (Status)) {
              Skip = TRUE;
            }
          }
          //
          // If Question is constant expression and "disabledIf True", don't output it.
          //
          ConstantFlag    = TRUE;
          if (!Skip && (Question->DisableExpression != NULL)) {
            Status = EvaluateExpression (FormSet, Form, Question->DisableExpression, &ConstantFlag);
            if (!EFI_ERROR (Status) && Question->DisableExpression->Result.Value.b && ConstantFlag) {
              Skip = TRUE;
            }
          }

          if (!Skip) {
            PrintQuestion(FormSet, Form, Question, TRUE);
          }
        }
        QuestionLink = GetNextNode (&Form->StatementListHead, QuestionLink);
      }

      FormLink = GetNextNode (&FormSet->FormListHead, FormLink);
    }
    FormSetLink = GetNextNode (FormSetEntryListHead, FormSetLink);
  }

  return EFI_SUCCESS;
}

