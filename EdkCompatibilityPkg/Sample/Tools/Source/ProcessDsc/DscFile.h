/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:  

  DscFile.h
  
Abstract:

  Defines and function prototypes for the ProcessDsc utility.
  
--*/

#ifndef _DSC_FILE_H_
#define _DSC_FILE_H_

typedef struct _SECTION_LINE {
  struct _SECTION_LINE  *Next;
  char                  *Line;
  char                  *FileName;
  UINT32                LineNum;
} SECTION_LINE;

//
// Use this structure to keep track of parsed file names. Then
// if we get a parse error we can figure out the file/line of
// the error and print a useful message.
//
typedef struct _DSC_FILE_NAME {
  struct _DSC_FILE_NAME *Next;
  char                  *FileName;
} DSC_FILE_NAME;

//
// We create a list of section names when we pre-parse a description file.
// Use this structure.
//
typedef struct _SECTION {
  struct _SECTION *Next;
  char            *Name;
  SECTION_LINE    *FirstLine;
} SECTION;

#define MAX_SAVES 4

typedef struct {
  SECTION_LINE  *SavedPosition[MAX_SAVES];
  int           SavedPositionIndex;
  SECTION       *Sections;
  SECTION_LINE  *Lines;
  SECTION       *LastSection;
  SECTION_LINE  *LastLine;
  SECTION_LINE  *CurrentLine;
  DSC_FILE_NAME *FileName;
  DSC_FILE_NAME *LastFileName;
} DSC_FILE;

//
// Function prototypes
//
int
DSCFileSetFile (
  DSC_FILE *DSC,
  char     *FileName
  );
SECTION *
DSCFileFindSection (
  DSC_FILE *DSC,
  char     *Name
  );
int
DSCFileSavePosition (
  DSC_FILE *DSC
  );
int
DSCFileRestorePosition (
  DSC_FILE *DSC
  );
char    *
DSCFileGetLine (
  DSC_FILE *DSC,
  char     *Line,
  int      LineLen
  );
int
DSCFileInit (
  DSC_FILE *DSC
  );
int
DSCFileDestroy (
  DSC_FILE *DSC
  );

#endif // ifndef _DSC_FILE_H_
