/** @file
Function prototypes and defines for string routines.

Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <string.h>
#include <ctype.h>
#include "StringFuncs.h"

//
// Functions implementations
//

CHAR8*
CloneString (
  IN CHAR8       *String
  )
/*++

Routine Description:

  Allocates a new string and copies 'String' to clone it

Arguments:

  String          The string to clone

Returns:

  CHAR8* - NULL if there are not enough resources

--*/
{
  CHAR8* NewString;

  NewString = malloc (strlen (String) + 1);
  if (NewString != NULL) {
    strcpy (NewString, String);
  }

  return NewString;
}


EFI_STATUS
StripInfDscStringInPlace (
  IN CHAR8       *String
  )
/*++

Routine Description:

  Remove all comments, leading and trailing whitespace from the string.

Arguments:

  String          The string to 'strip'

Returns:

  EFI_STATUS

--*/
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


STRING_LIST*
SplitStringByWhitespace (
  IN CHAR8       *String
  )
/*++

Routine Description:

  Creates and returns a 'split' STRING_LIST by splitting the string
  on whitespace boundaries.

Arguments:

  String          The string to 'split'

Returns:

  EFI_STATUS

--*/
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


STRING_LIST*
NewStringList (
  )
/*++

Routine Description:

  Creates a new STRING_LIST with 0 strings.

Returns:

  STRING_LIST* - Null if there is not enough resources to create the object.

--*/
{
  STRING_LIST *NewList;
  NewList = AllocateStringListStruct (0);
  if (NewList != NULL) {
    NewList->Count = 0;
  }
  return NewList;
}


EFI_STATUS
AppendCopyOfStringToList (
  IN OUT STRING_LIST **StringList,
  IN CHAR8       *String
  )
/*++

Routine Description:

  Adds String to StringList.  A new copy of String is made before it is
  added to StringList.

Returns:

  EFI_STATUS

--*/
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


EFI_STATUS
RemoveLastStringFromList (
  IN STRING_LIST       *StringList
  )
/*++

Routine Description:

  Removes the last string from StringList and frees the memory associated
  with it.

Arguments:

  StringList        The string list to remove the string from

Returns:

  EFI_STATUS

--*/
{
  if (StringList->Count == 0) {
    return EFI_INVALID_PARAMETER;
  }

  free (StringList->Strings[StringList->Count - 1]);
  StringList->Count--;
  return EFI_SUCCESS;
}


STRING_LIST*
AllocateStringListStruct (
  IN UINTN StringCount
  )
/*++

Routine Description:

  Allocates a STRING_LIST structure that can store StringCount strings.

Arguments:

  StringCount        The number of strings that need to be stored

Returns:

  EFI_STATUS

--*/
{
  return malloc (OFFSET_OF(STRING_LIST, Strings[StringCount + 1]));
}


VOID
FreeStringList (
  IN STRING_LIST       *StringList
  )
/*++

Routine Description:

  Frees all memory associated with StringList.

Arguments:

  StringList        The string list to free

Returns:

  VOID
--*/
{
  while (StringList->Count > 0) {
    RemoveLastStringFromList (StringList);
  }

  free (StringList);
}


CHAR8*
StringListToString (
  IN STRING_LIST       *StringList
  )
/*++

Routine Description:

  Generates a string that represents the STRING_LIST

Arguments:

  StringList        The string list to convert to a string

Returns:

  CHAR8* - The string list represented with a single string.  The returned
           string must be freed by the caller.

--*/
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


VOID
PrintStringList (
  IN STRING_LIST       *StringList
  )
/*++

Routine Description:

  Prints out the string list

Arguments:

  StringList        The string list to print

Returns:

  EFI_STATUS

--*/
{
  CHAR8* String;
  String = StringListToString (StringList);
  if (String != NULL) {
    printf ("%s", String);
    free (String);
  }
}


