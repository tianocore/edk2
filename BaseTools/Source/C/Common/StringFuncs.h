/** @file
String routines implementation

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_STRING_FUNCS_H
#define _EFI_STRING_FUNCS_H

#include <stdio.h>
#include <stdlib.h>
#include <Common/UefiBaseTypes.h>

//
// Common data structures
//
typedef struct {
  UINTN      Count;
  //
  // Actually this array can be 0 or more items (based on Count)
  //
  CHAR8*     Strings[1];
} STRING_LIST;


//
// Functions declarations
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
;

/**
  Remove all comments, leading and trailing whitespace from the string.

  @param String          The string to 'strip'

  @return EFI_STATUS
**/
EFI_STATUS
StripInfDscStringInPlace (
  IN CHAR8       *String
  )
;

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
;

/**
  Creates a new STRING_LIST with 0 strings.

  @return STRING_LIST* - Null if there is not enough resources to create the object.
**/
STRING_LIST*
NewStringList (
  )
;


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
;

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
;


/**
  Allocates a STRING_LIST structure that can store StringCount strings.

  @param StringCount        The number of strings that need to be stored

  @return EFI_STATUS
**/
STRING_LIST*
AllocateStringListStruct (
  IN UINTN StringCount
  )
;


/**
  Frees all memory associated with StringList.

  @param StringList        The string list to free

  @return EFI_STATUS
**/
VOID
FreeStringList (
  IN STRING_LIST       *StringList
  )
;


/**
  Generates a string that represents the STRING_LIST

  @param StringList        The string list to convert to a string

  @return CHAR8* The string list represented with a single string.  The returned
           string must be freed by the caller.
**/
CHAR8*
StringListToString (
  IN STRING_LIST       *StringList
  )
;


/**
  Prints out the string list

  @param StringList        The string list to print
**/
VOID
PrintStringList (
  IN STRING_LIST       *StringList
  )
;



#endif
