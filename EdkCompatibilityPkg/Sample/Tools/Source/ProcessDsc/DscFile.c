/*++

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:

    DscFile.c

  Abstract:

    This module is used to process description files at a high level. For the
    most part, it pre-parses the file to find and save off positions of all
    the sections ([section.subsection.subsection]) in a linked list, then
    provides services to find the sections by name, and read the lines from
    the section until you run into the next section.

  NOTE: DSC file is synonomous with section file. A DSC file is simply a file
    containing bracketed section names [section.subsection.subsection...]

--*/

#include <stdio.h>  // for file ops
#include <string.h>
#include <ctype.h>
#include <stdlib.h> // for malloc
#include "Common.h"
#include "DSCFile.h"

#define MAX_INCLUDE_NEST_LEVEL  20

static
void
DSCFileFree (
  DSC_FILE *DSC
  );

static
STATUS
DSCParseInclude (
  DSC_FILE  *DSC,
  char      *FileName,
  int       NestLevel
  );

//
// Constructor for a DSC file
//
int
DSCFileInit (
  DSC_FILE *DSC
  )
{
  memset ((char *) DSC, 0, sizeof (DSC_FILE));
  DSC->SavedPositionIndex = -1;
  return STATUS_SUCCESS;
}
//
// Destructor for a DSC file
//
int
DSCFileDestroy (
  DSC_FILE *DSC
  )
{
  DSC->SavedPositionIndex = -1;
  DSCFileFree (DSC);
  return STATUS_SUCCESS;
}
//
// Get the next line from a DSC file.
//
char *
DSCFileGetLine (
  DSC_FILE  *DSC,
  char      *Line,
  int       LineLen
  )
{
  char  *Cptr;

  if (DSC->CurrentLine == NULL) {
    return NULL;
  }
  //
  // Check for running into next section
  //
  if (DSC->CurrentLine->Line[0] == '[') {
    return NULL;
  }
  //
  // Allow special case where the line starts with backslash-bracket. If we
  // see this, then shift everything left one character.
  //
  if ((DSC->CurrentLine->Line[0] == '\\') && (DSC->CurrentLine->Line[1] == '[')) {
    Cptr = DSC->CurrentLine->Line + 1;
  } else {
    Cptr = DSC->CurrentLine->Line;
  }

  strncpy (Line, Cptr, LineLen);
  ParserSetPosition (DSC->CurrentLine->FileName, DSC->CurrentLine->LineNum);
  DSC->CurrentLine = DSC->CurrentLine->Next;
  return Line;
}

int
DSCFileSetFile (
  DSC_FILE  *DSC,
  char      *FileName
  )
/*++

Routine Description:
  
  Pre-scan a section file to find all the sections. Then we can speed up
  searching for the different sections.

Arguments:

  DSC       - pointer to a DSC structure (this pointer)
  FileName  - name of the file to process

Returns:

  STATUS_SUCCESS if everything went well.

--*/
{
  STATUS  Status;

  //
  // Called to open a new sectioned file.
  //
  Status = DSCParseInclude (DSC, FileName, 1);
  return Status;
}

static
STATUS
DSCParseInclude (
  DSC_FILE    *DSC,
  char        *FileName,
  int         NestLevel
  )
{
  SECTION       *NewSect;
  SECTION_LINE  *NewLine;
  DSC_FILE_NAME *NewDscFileName;
  char          Line[MAX_LINE_LEN];
  char          *Start;
  char          *End;
  char          SaveChar;
  char          *TempCptr;
  char          ShortHandSectionName[MAX_LINE_LEN];
  char          ThisSectionName[MAX_LINE_LEN];
  SECTION       *CurrSect;
  SECTION       *TempSect;
  FILE          *FilePtr;
  STATUS        Status;
  UINT32        LineNum;

  //
  // Make sure we haven't exceeded our maximum nesting level
  //
  if (NestLevel > MAX_INCLUDE_NEST_LEVEL) {
    Error (NULL, 0, 0, "application error", "maximum !include nesting level exceeded");
    return STATUS_ERROR;
  }
  //
  // Try to open the file
  //
  if ((FilePtr = fopen (FileName, "r")) == NULL) {
    //
    // This function is called to handle the DSC file from the command line too,
    // so differentiate whether this file is an include file or the main file
    // by examining the nest level.
    //
    if (NestLevel == 1) {
      Error (NULL, 0, 0, FileName, "could not open DSC file for reading");
    } else {
      Error (NULL, 0, 0, FileName, "could not open !include DSC file for reading");
    }

    return STATUS_ERROR;
  }
  //
  // We keep a linked list of files we parse for error reporting purposes.
  //
  NewDscFileName = malloc (sizeof (DSC_FILE_NAME));
  if (NewDscFileName == NULL) {
    Error (__FILE__, __LINE__, 0, "memory allocation failed", NULL);
    return STATUS_ERROR;
  }

  memset (NewDscFileName, 0, sizeof (DSC_FILE_NAME));
  NewDscFileName->FileName = (INT8 *) malloc (strlen (FileName) + 1);
  if (NewDscFileName->FileName == NULL) {
    Error (__FILE__, __LINE__, 0, "memory allocation failed", NULL);
    return STATUS_ERROR;
  }

  strcpy (NewDscFileName->FileName, FileName);
  if (DSC->FileName == NULL) {
    DSC->FileName = NewDscFileName;
  } else {
    DSC->LastFileName->Next = NewDscFileName;
  }

  DSC->LastFileName = NewDscFileName;
  //
  // Read lines and process until done
  //
  Status  = STATUS_SUCCESS;
  LineNum = 0;
  for (;;) {
    if (fgets (Line, sizeof (Line), FilePtr) == NULL) {
      break;
    }

    LineNum++;
    ParserSetPosition (FileName, LineNum);
    //
    // Add the line to our list if it's not a !include line
    //
    if ((strncmp (Line, "!include", 8) == 0) && (isspace (Line[8]))) {
      Start = Line + 9;
      while (*Start && (*Start != '"')) {
        Start++;
      }

      if (*Start != '"') {
        Error (FileName, LineNum, 0, NULL, "invalid format for !include");
        Status = STATUS_ERROR;
        goto Done;
      }

      Start++;
      for (End = Start; *End && (*End != '"'); End++)
        ;
      if (*End != '"') {
        Error (FileName, LineNum, 0, NULL, "invalid format for !include");
        Status = STATUS_ERROR;
        goto Done;
      }

      *End = 0;
      //
      // Expand symbols. Use 'ThisSectionName' as scratchpad
      //
      ExpandSymbols (Start, ThisSectionName, sizeof (ThisSectionName), EXPANDMODE_NO_UNDEFS);
      Status = DSCParseInclude (DSC, ThisSectionName, NestLevel + 1);
      if (Status != STATUS_SUCCESS) {
        Error (FileName, LineNum, 0, NULL, "failed to parse !include file");
        goto Done;
      }
    } else {
      NewLine = (SECTION_LINE *) malloc (sizeof (SECTION_LINE));
      if (NewLine == NULL) {
        Error (NULL, 0, 0, NULL, "failed to allocate memory");
        Status = STATUS_ERROR;
        goto Done;
      }

      memset ((char *) NewLine, 0, sizeof (SECTION_LINE));
      NewLine->LineNum  = LineNum;
      NewLine->FileName = NewDscFileName->FileName;
      NewLine->Line     = (char *) malloc (strlen (Line) + 1);
      if (NewLine->Line == NULL) {
        Error (NULL, 0, 0, NULL, "failed to allocate memory");
        Status = STATUS_ERROR;
        goto Done;
      }

      strcpy (NewLine->Line, Line);
      if (DSC->Lines == NULL) {
        DSC->Lines = NewLine;
      } else {
        DSC->LastLine->Next = NewLine;
      }

      DSC->LastLine = NewLine;
      //
      // Parse the line for []. Ignore [] and [----] delimiters. The
      // line may have multiple definitions separated by commas, so
      // take each separately
      //
      Start = Line;
      if ((Line[0] == '[') && ((Line[1] != ']') && (Line[1] != '-'))) {
        //
        // Skip over open bracket and preceeding spaces
        //
        Start++;
        ShortHandSectionName[0] = 0;

        while (*Start && (*Start != ']')) {
          while (isspace (*Start)) {
            Start++;
          }
          //
          // Hack off closing bracket or trailing spaces or comma separator.
          // Also allow things like [section.subsection1|subsection2], which
          // is shorthand for [section.subsection1,section.subsection2]
          //
          End = Start;
          while (*End && (*End != ']') && !isspace (*End) && (*End != ',') && (*End != '|')) {
            End++;
          }
          //
          // Save the character and null-terminate the string
          //
          SaveChar  = *End;
          *End      = 0;
          //
          // Now allocate space for a new section and add it to the linked list.
          // If the previous section ended with the shorthand indicator, then
          // the section name was saved off. Append this section name to it.
          //
          strcpy (ThisSectionName, ShortHandSectionName);
          if (*Start == '.') {
            strcat (ThisSectionName, Start + 1);
          } else {
            strcat (ThisSectionName, Start);
          }
          //
          // Allocate memory for the section. Then clear it out.
          //
          NewSect = (SECTION *) malloc (sizeof (SECTION));
          if (NewSect == NULL) {
            Error (NULL, 0, 0, NULL, "failed to allocation memory for sections");
            Status = STATUS_ERROR;
            goto Done;
          }

          memset ((char *) NewSect, 0, sizeof (SECTION));
          NewSect->FirstLine  = NewLine;
          NewSect->Name       = (char *) malloc (strlen (ThisSectionName) + 1);
          if (NewSect->Name == NULL) {
            Error (NULL, 0, 0, NULL, "failed to allocation memory for sections");
            Status = STATUS_ERROR;
            goto Done;
          }

          strcpy (NewSect->Name, ThisSectionName);
          if (DSC->Sections == NULL) {
            DSC->Sections = NewSect;
          } else {
            DSC->LastSection->Next = NewSect;
          }

          DSC->LastSection  = NewSect;
          *End              = SaveChar;
          //
          // If the name ended in a shorthand indicator, then save the
          // section name and truncate it at the last dot.
          //
          if (SaveChar == '|') {
            strcpy (ShortHandSectionName, ThisSectionName);
            for (TempCptr = ShortHandSectionName + strlen (ShortHandSectionName) - 1;
                 (TempCptr != ShortHandSectionName) && (*TempCptr != '.');
                 TempCptr--
                )
              ;
            //
            // If we didn't find a dot, then hopefully they have [name1|name2]
            // instead of [name1,name2].
            //
            if (TempCptr == ShortHandSectionName) {
              ShortHandSectionName[0] = 0;
            } else {
              //
              // Truncate after the dot
              //
              *(TempCptr + 1) = 0;
            }
          } else {
            //
            // Kill the shorthand string
            //
            ShortHandSectionName[0] = 0;
          }
          //
          // Skip to next section name or closing bracket
          //
          while (*End && ((*End == ',') || isspace (*End) || (*End == '|'))) {
            End++;
          }

          Start = End;
        }
      }
    }
  }
  //
  // Look through all the sections to make sure we don't have any duplicates.
  // Allow [----] and [====] section separators
  //
  CurrSect = DSC->Sections;
  while (CurrSect != NULL) {
    TempSect = CurrSect->Next;
    while (TempSect != NULL) {
      if (isalpha (CurrSect->Name[0]) && (_stricmp (CurrSect->Name, TempSect->Name) == 0)) {
        Error (
          TempSect->FirstLine->FileName,
          TempSect->FirstLine->LineNum,
          0,
          TempSect->Name,
          "duplicate section found"
          );
        Error (
          CurrSect->FirstLine->FileName,
          CurrSect->FirstLine->LineNum,
          0,
          TempSect->Name,
          "first definition of duplicate section"
          );
        Status = STATUS_ERROR;
        goto Done;
      }

      TempSect = TempSect->Next;
    }

    CurrSect = CurrSect->Next;
  }

Done:
  fclose (FilePtr);
  return Status;
}
//
// Free up memory allocated for DSC file handling.
//
static
void
DSCFileFree (
  DSC_FILE *DSC
  )
{
  SECTION       *NextSection;
  SECTION_LINE  *NextLine;
  DSC_FILE_NAME *NextName;

  while (DSC->Sections != NULL) {
    NextSection = DSC->Sections->Next;
    if (DSC->Sections->Name != NULL) {
      free (DSC->Sections->Name);
    }

    free (DSC->Sections);
    DSC->Sections = NextSection;
  }

  while (DSC->Lines != NULL) {
    NextLine = DSC->Lines->Next;
    free (DSC->Lines->Line);
    free (DSC->Lines);
    DSC->Lines = NextLine;
  }

  while (DSC->FileName != NULL) {
    NextName = DSC->FileName->Next;
    free (DSC->FileName->FileName);
    free (DSC->FileName);
    DSC->FileName = NextName;
  }
}

SECTION *
DSCFileFindSection (
  DSC_FILE  *DSC,
  char      *Name
  )
{
  SECTION *Sect;

  //
  // Look through all the sections to find one with this name (case insensitive)
  //
  Sect = DSC->Sections;
  while (Sect != NULL) {
    if (_stricmp (Name, Sect->Name) == 0) {
      //
      // Position within file
      //
      DSC->CurrentLine = Sect->FirstLine->Next;
      return Sect;
    }

    Sect = Sect->Next;
  }

  return NULL;
}

int
DSCFileSavePosition (
  DSC_FILE *DSC
  )
{
  //
  // Advance to next slot
  //
  DSC->SavedPositionIndex++;
  if (DSC->SavedPositionIndex >= MAX_SAVES) {
    DSC->SavedPositionIndex--;
    Error (NULL, 0, 0, "APP ERROR", "max nesting of saved section file positions exceeded");
    return STATUS_ERROR;
  }

  DSC->SavedPosition[DSC->SavedPositionIndex] = DSC->CurrentLine;
  return STATUS_SUCCESS;
}

int
DSCFileRestorePosition (
  DSC_FILE *DSC
  )
{
  if (DSC->SavedPositionIndex < 0) {
    Error (NULL, 0, 0, "APP ERROR", "underflow of saved positions in section file");
    return STATUS_ERROR;
  }

  DSC->CurrentLine = DSC->SavedPosition[DSC->SavedPositionIndex];
  DSC->SavedPositionIndex--;
  return STATUS_SUCCESS;
}
