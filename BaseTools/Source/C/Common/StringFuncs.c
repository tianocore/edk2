/** @file
Function prototypes and defines for string routines.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <string.h>
#include <ctype.h>
#include "StringFuncs.h"

//
// Functions implementations
//

/**
  Allocates a new string and copies 'String' to clone it

  @param String          The string to clone

  @return CHAR8* - NULL if there are not enough resources
**/
CHAR8*
CloneString (
  IN CHAR8       *String
  )
{
  CHAR8* NewString;

  NewString = malloc (strlen (String) + 1);
  if (NewString != NULL) {
    strcpy (NewString, String);
  }

  return NewString;
}

/**
  Remove all comments, leading and trailing whitespace from the string.

  @param String          The string to 'strip'

  @return EFI_STATUS
**/
EFI_STATUS
StripInfDscStringInPlace (
  IN CHAR8       *String
  )
{
  CHAR8 *Pos;

  if (String == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Remove leading whitespace
  //
  for (Pos = String; isspace ((int)*Pos); Pos++) {
  }
  if (Pos != String) {
    memmove (String, Pos, strlen (Pos) + 1);
  }

  //
  // Comment BUGBUGs!
  //
  // What about strings?  Comment characters are okay in strings.
  // What about multiline comments?
  //

  Pos = (CHAR8 *) strstr (String,  "//");
  if (Pos != NULL) {
    *Pos = '\0';
  }

  Pos = (CHAR8 *) strchr (String, '#');
  if (Pos != NULL) {
    *Pos = '\0';
  }

  //
  // Remove trailing whitespace
  //
  for (Pos = String + strlen (String);
       ((Pos - 1) >= String) && (isspace ((int)*(Pos - 1)));
       Pos--
      ) {
  }
  *Pos = '\0';

  return EFI_SUCCESS;
}

/**
  Creates and returns a 'split' STRING_LIST by splitting the string
  on whitespace boundaries.

  @param String          The string to 'split'

  @return EFI_STATUS
**/
STRING_LIST*
SplitStringByWhitespace (
  IN CHAR8       *String
  )
{
  CHAR8       *Pos;
  CHAR8       *EndOfSubString;
  CHAR8       *EndOfString;
  STRING_LIST *Output;
  UINTN       Item;

  String = CloneString (String);
  if (String == NULL) {
    return NULL;
  }
  EndOfString = String + strlen (String);

  Output = NewStringList ();

  for (Pos = String, Item = 0; Pos < EndOfString; Item++) {
    while (isspace ((int)*Pos)) {
      Pos++;
    }

    for (EndOfSubString=Pos;
         (*EndOfSubString != '\0') && !isspace ((int)*EndOfSubString);
         EndOfSubString++
         ) {
    }

    if (EndOfSubString == Pos) {
      break;
    }

    *EndOfSubString = '\0';

    AppendCopyOfStringToList (&Output, Pos);

    Pos = EndOfSubString + 1;
  }

  free (String);
  return Output;
}

/**
  Creates a new STRING_LIST with 0 strings.

  @return STRING_LIST* - Null if there is not enough resources to create the object.
**/
STRING_LIST*
NewStringList (
  )
{
  STRING_LIST *NewList;
  NewList = AllocateStringListStruct (0);
  if (NewList != NULL) {
    NewList->Count = 0;
  }
  return NewList;
}

/**
  Adds String to StringList.  A new copy of String is made before it is
  added to StringList.

  @return EFI_STATUS
**/
EFI_STATUS
AppendCopyOfStringToList (
  IN OUT STRING_LIST **StringList,
  IN CHAR8       *String
  )
{
  STRING_LIST *OldList;
  STRING_LIST *NewList;
  CHAR8       *NewString;

  OldList = *StringList;
  NewList = AllocateStringListStruct (OldList->Count + 1);
  if (NewList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewString = CloneString (String);
  if (NewString == NULL) {
    free (NewList);
    return EFI_OUT_OF_RESOURCES;
  }

  memcpy (
    NewList->Strings,
    OldList->Strings,
    sizeof (OldList->Strings[0]) * OldList->Count
    );
  NewList->Count = OldList->Count + 1;
  NewList->Strings[OldList->Count] = NewString;

  *StringList = NewList;
  free (OldList);

  return EFI_SUCCESS;
}

/**
  Removes the last string from StringList and frees the memory associated
  with it.

  @param StringList        The string list to remove the string from

  @return EFI_STATUS
**/
EFI_STATUS
RemoveLastStringFromList (
  IN STRING_LIST       *StringList
  )
{
  if (StringList->Count == 0) {
    return EFI_INVALID_PARAMETER;
  }

  free (StringList->Strings[StringList->Count - 1]);
  StringList->Count--;
  return EFI_SUCCESS;
}

/**
  Allocates a STRING_LIST structure that can store StringCount strings.

  @param StringCount        The number of strings that need to be stored

  @return EFI_STATUS
**/
STRING_LIST*
AllocateStringListStruct (
  IN UINTN StringCount
  )
{
  return malloc (OFFSET_OF(STRING_LIST, Strings[StringCount + 1]));
}

/**
  Frees all memory associated with StringList.

  @param StringList        The string list to free
**/
VOID
FreeStringList (
  IN STRING_LIST       *StringList
  )
{
  while (StringList->Count > 0) {
    RemoveLastStringFromList (StringList);
  }

  free (StringList);
}

/**
  Generates a string that represents the STRING_LIST

  @param StringList        The string list to convert to a string

  @return CHAR8* - The string list represented with a single string.  The returned
           string must be freed by the caller.
**/
CHAR8*
StringListToString (
  IN STRING_LIST       *StringList
  )
{
  UINTN Count;
  UINTN Length;
  CHAR8 *NewString;

  Length = 2;
  for (Count = 0; Count < StringList->Count; Count++) {
    if (Count > 0) {
      Length += 2;
    }
    Length += strlen (StringList->Strings[Count]) + 2;
  }

  NewString = malloc (Length + 1);
  if (NewString == NULL) {
    return NewString;
  }
  NewString[0] = '\0';

  strcat (NewString, "[");
  for (Count = 0; Count < StringList->Count; Count++) {
    if (Count > 0) {
      strcat (NewString, ", ");
    }
    strcat (NewString, "\"");
    strcat (NewString, StringList->Strings[Count]);
    strcat (NewString, "\"");
  }
  strcat (NewString, "]");

  return NewString;
}

/**
  Prints out the string list

  @param StringList        The string list to print

  @return EFI_STATUS
**/
VOID
PrintStringList (
  IN STRING_LIST       *StringList
  )
{
  CHAR8* String;
  String = StringListToString (StringList);
  if (String != NULL) {
    printf ("%s", String);
    free (String);
  }
}


