/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiVfr.h

Abstract:

  Defines and prototypes for the EFI internal forms representation
  setup protocol and drivers
  
--*/

#ifndef _EFI_VFR_H_
#define _EFI_VFR_H_

#include "Tiano.h"
#include "EfiInternalFormRepresentation.h"
#include <string.h>

//
// This number should be incremented with each change to the VFR compiler.
// We write the version to the output list file for debug purposes.
//
#define UTILITY_VERSION  "v1.9"
#define UTILITY_NAME     "VfrCompile"

//
// Maximum file path for filenames
//
#define MAX_PATH        255
#define MAX_QUEUE_COUNT 255
#define MAX_LINE_LEN    1024

//
// We parse C-style structure definitions which can then be referenced
// in VFR statements.
// We need to define an internal structure that can be used to
// track the fields in a structure definition, and another structure
// to keep track of the structure name and subfields.
//
typedef struct _STRUCT_FIELD_DEFINITION {
  struct _STRUCT_FIELD_DEFINITION *Next;
  int                             DataSize;
  int                             Offset;     // from the start of the structure
  int                             ArrayLength;
  char                            IsArray;
  char                            *Name;
} STRUCT_FIELD_DEFINITION;

typedef struct _STRUCT_DEFINITION {
  struct _STRUCT_DEFINITION *Next;
  int                       Size;
  int                       LineNum;          // line number where the structure was defined
  int                       IsNonNV;          // if this is the non-NV data structure definition
  int                       Referenced;       // if it's referenced anywhere in the VFR
  int                       VarStoreIdValid;  // found a 'varstore' statement for it in the VFR
  unsigned short            VarStoreId;       // key from a varstore IFR statement
  int                       VarStoreLineNum;  // line number where VARSTORE was defined
  char                      *Name;
  STRUCT_FIELD_DEFINITION   *Field;
  STRUCT_FIELD_DEFINITION   *LastField;
} STRUCT_DEFINITION;

//
// For the IdEqValList variable list of UINT16's, keep track of them using
// a linked list until we know how many there are.
// We also use a linked list of these to keep track of labels used in
// the VFR script so we can catch duplicates.
// We'll also use it to keep track of defined varstore id's so we can
// detect duplicate definitions.
//
typedef struct _UINT16_LIST {
  struct _UINT16_LIST *Next;
  UINT16              Value;
  UINT32              LineNum;
} UINT16_LIST;

typedef struct _GOTO_REFERENCE {
  struct _GOTO_REFERENCE  *Next;
  UINT32                  RefLineNum; // line number of source file where referenced
  UINT16                  Value;
} GOTO_REFERENCE;

typedef struct _FORM_ID_VALUE {
  struct _FORM_ID_VALUE *Next;
  UINT32                LineNum;
  UINT16                Value;
} FORM_ID_VALUE;

//
// We keep track in the parser of all "#line 4 "x.y"" strings so we
// can cross-reference the line numbers in the preprocessor output .i file
// to the original input files.
//
typedef struct _PARSER_LINE_DEFINITION {
  struct _PARSER_LINE_DEFINITION  *Next;
  UINT32                          HashLineNum;  // from the #line stmt
  UINT32                          TokenLineNum; // line number in the .i file
  INT8                            *FileName;    // from the #line stmt
} PARSER_LINE_DEFINITION;

extern PARSER_LINE_DEFINITION *gLineDefinition;
extern PARSER_LINE_DEFINITION *gLastLineDefinition;

extern
char                          *
ConvertLineNumber (
  UINT32 *LineNum
  )
/*++

Routine Description:
  Given the line number in the preprocessor-output file, use the line number
  information we've saved to determine the source file name and line number
  where the code originally came from. This is required for error reporting.

Arguments:
  LineNum - the line number in the preprocessor-output file.

Returns:
  Returns a pointer to the source file name. Also returns the line number 
  in the provided LineNum argument

--*/
;

typedef struct _IFR_BYTE {
  struct _IFR_BYTE  *Next;
  UINT32            LineNum;
  UINT8             OpcodeByte;
  UINT8             KeyByte;
} IFR_BYTE;

typedef struct {
  INT8  VfrFileName[MAX_PATH];
  INT8  VfrListFileName[MAX_PATH];
  INT8  CreateListFile;
  INT8  CreateIfrBinFile;
  INT8  IfrOutputFileName[MAX_PATH];
  INT8  OutputDirectory[MAX_PATH];
  INT8  PreprocessorOutputFileName[MAX_PATH];
  INT8  VfrBaseFileName[MAX_PATH];  // name of input VFR file with no path or extension
  INT8  *IncludePaths;
  INT8  *CPreprocessorOptions;
} OPTIONS;

extern OPTIONS  gOptions;

VOID
WriteStandardFileHeader (
  FILE *OutFptr
  )
/*++

Routine Description:
  This function is invoked to emit a standard header to an
  output text file.
  
Arguments:
  OutFptr - file to write the header to

Returns:
  None

--*/
;

#endif // #ifndef _EFI_VFR_H_
