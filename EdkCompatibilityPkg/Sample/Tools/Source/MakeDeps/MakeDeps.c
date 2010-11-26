/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  MakeDeps.c  

Abstract:

  Recursively scan source files to find include files and emit them to 
  create dependency lists.

--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "Tiano.h"
#include "EfiUtilityMsgs.h"

//
// Structure to maintain a linked list of strings
//
typedef struct _STRING_LIST {
  struct _STRING_LIST *Next;
  char                *Str;
} STRING_LIST;

#define UTILITY_NAME      "MakeDeps"
#define UTILITY_VERSION   "v1.0"

#define MAX_LINE_LEN      2048
#define MAX_PATH          2048
#define START_NEST_DEPTH  1
#define MAX_NEST_DEPTH    1000  // just in case we get in an endless loop.
//
// Define the relative paths used by the special #include macros
//
#define PROTOCOL_DIR_PATH       "Protocol\\"
#define GUID_DIR_PATH           "Guid\\"
#define ARCH_PROTOCOL_DIR_PATH  "ArchProtocol\\"
#define PPI_PROTOCOL_DIR_PATH   "Ppi\\"

//
// Use this structure to keep track of all the special #include forms
//
typedef struct {
  INT8  *IncludeMacroName;
  INT8  *PathName;
} INCLUDE_MACRO_CONVERSION;

//
// This data is used to convert #include macros like:
//    #include EFI_PROTOCOL_DEFINITION(xxx)
// into
//    #include Protocol/xxx/xxx.h
//
static const INCLUDE_MACRO_CONVERSION mMacroConversion[] = {
  "EFI_PROTOCOL_DEFINITION",
  PROTOCOL_DIR_PATH,
  "EFI_GUID_DEFINITION",
  GUID_DIR_PATH,
  "EFI_ARCH_PROTOCOL_DEFINITION",
  ARCH_PROTOCOL_DIR_PATH,
  "EFI_PROTOCOL_PRODUCER",
  PROTOCOL_DIR_PATH,
  "EFI_PROTOCOL_CONSUMER",
  PROTOCOL_DIR_PATH,
  "EFI_PROTOCOL_DEPENDENCY",
  PROTOCOL_DIR_PATH,
  "EFI_ARCH_PROTOCOL_PRODUCER",
  ARCH_PROTOCOL_DIR_PATH,
  "EFI_ARCH_PROTOCOL_CONSUMER",
  ARCH_PROTOCOL_DIR_PATH,
  "EFI_ARCH_PROTOCOL_DEPENDENCY",
  ARCH_PROTOCOL_DIR_PATH,
  "EFI_PPI_DEFINITION",
  PPI_PROTOCOL_DIR_PATH,
  "EFI_PPI_PRODUCER",
  PPI_PROTOCOL_DIR_PATH,
  "EFI_PPI_CONSUMER",
  PPI_PROTOCOL_DIR_PATH,
  "EFI_PPI_DEPENDENCY",
  PPI_PROTOCOL_DIR_PATH,
  NULL,
  NULL
};

typedef struct _SYMBOL {
  struct _SYMBOL  *Next;
  INT8            *Name;
  INT8            *Value;
} SYMBOL;

typedef enum {
  SearchCurrentDir,
  SearchIncludePaths,
  SearchAllPaths,
} FILE_SEARCH_TYPE;

//
// Here's all our globals. We need a linked list of include paths, a linked
// list of source files, a linked list of subdirectories (appended to each
// include path when searching), and flags to keep track of command-line options.
//
static struct {
  STRING_LIST *IncludePaths;            // all include paths to search
  STRING_LIST *ParentPaths;             // all parent paths to search
  STRING_LIST *SourceFiles;             // all source files to parse
  STRING_LIST *SubDirs;                 // appended to each include path when searching
  SYMBOL      *SymbolTable;             // for replacement strings
  FILE        *OutFptr;                 // output dependencies to this file
  BOOLEAN     Verbose;                  // for more detailed output
  BOOLEAN     IgnoreNotFound;           // no warnings if files not found
  BOOLEAN     QuietMode;                // -q - don't print missing file warnings
  BOOLEAN     NoSystem;                 // don't process #include <system> files
  BOOLEAN     NeverFail;                // always return success
  BOOLEAN     NoDupes;                  // to not list duplicate dependency files (for timing purposes)
  BOOLEAN     UseSumDeps;               // use summary dependency files if found
  BOOLEAN     IsAsm;                    // The SourceFiles are assembler files
  BOOLEAN     IsCl;                     // The SourceFiles are the output of cl with /showIncludes
  INT8        TargetFileName[MAX_PATH]; // target object filename
  INT8        SumDepsPath[MAX_PATH];    // path to summary files
  INT8        TmpFileName[MAX_PATH];    // temp file name for output file
  INT8        *OutFileName;             // -o option
} mGlobals;

static
STATUS
ProcessFile (
  INT8              *TargetFileName,
  INT8              *FileName,
  UINT32            NestDepth,
  STRING_LIST       *ProcessedFiles,
  FILE_SEARCH_TYPE  FileSearchType
  );

static
STATUS
ProcessClOutput (
  INT8            *TargetFileName,
  INT8            *FileName,
  STRING_LIST     *ProcessedFiles
  );

static
FILE  *
FindFile (
  INT8              *FileName,
  UINT32            FileNameLen,
  FILE_SEARCH_TYPE  FileSearchType
  );

static
void
PrintDependency (
  INT8    *Target,
  INT8    *DependentFile
  );

static
void
ReplaceSymbols (
  INT8    *Str,
  UINT32  StrSize
  );

static
STATUS
ProcessArgs (
  int   Argc,
  char  *Argv[]
  );

static
void
Usage (
  VOID
  );

static
void
FreeLists (
  VOID
  );

int
main (
  int   Argc,
  char  *Argv[]
  )
/*++

Routine Description:

  Call the routine to parse the command-line options, then process each file
  to build dependencies.
  
Arguments:

  Argc - Standard C main() argc.
  Argv - Standard C main() argv.

Returns:

  0       if successful
  nonzero otherwise
  
--*/
{
  STRING_LIST *File;
  STRING_LIST ProcessedFiles;
  STRING_LIST *TempList;
  STATUS      Status;
  INT8        *Cptr;
  INT8        TargetFileName[MAX_PATH];

  SetUtilityName (UTILITY_NAME);
  //
  // Process the command-line arguments
  //
  Status = ProcessArgs (Argc, Argv);
  if (Status != STATUS_SUCCESS) {
    return STATUS_ERROR;
  }
  //
  // Go through the list of source files and process each.
  //
  memset (&ProcessedFiles, 0, sizeof (STRING_LIST));
  File = mGlobals.SourceFiles;
  while (File != NULL) {
    //
    // Clear out our list of processed files
    //
    TempList = ProcessedFiles.Next;
    while (ProcessedFiles.Next != NULL) {
      TempList = ProcessedFiles.Next->Next;
      free (ProcessedFiles.Next->Str);
      free (ProcessedFiles.Next);
      ProcessedFiles.Next = TempList;
    }
    //
    // Replace filename extension with ".obj" if they did not
    // specifically specify the target file
    //
    if (mGlobals.TargetFileName[0] == 0) {
      strcpy (TargetFileName, File->Str);
      //
      // Find the .extension
      //
      for (Cptr = TargetFileName + strlen (TargetFileName) - 1;
           (*Cptr != '\\') && (Cptr > TargetFileName) && (*Cptr != '.');
           Cptr--
          )
        ;
      if (Cptr == TargetFileName) {
        Error (NULL, 0, 0, File->Str, "could not locate extension in filename");
        goto Finish;
      }
      //
      // Tack on the ".obj"
      //
      strcpy (Cptr, ".obj");
    } else {
      //
      // Copy the target filename they specified
      //
      strcpy (TargetFileName, mGlobals.TargetFileName);
    }

    if (mGlobals.IsCl) {
      Status = ProcessClOutput (TargetFileName, File->Str, &ProcessedFiles);
    } else {
      Status = ProcessFile (TargetFileName, File->Str, START_NEST_DEPTH, 
                            &ProcessedFiles, SearchCurrentDir);
    }
    if (Status != STATUS_SUCCESS) {
      goto Finish;
    }

    File = File->Next;
  }

Finish:
  //
  // Free up memory
  //
  FreeLists ();
  //
  // Free up our processed files list
  //
  TempList = ProcessedFiles.Next;
  while (ProcessedFiles.Next != NULL) {
    TempList = ProcessedFiles.Next->Next;
    free (ProcessedFiles.Next->Str);
    free (ProcessedFiles.Next);
    ProcessedFiles.Next = TempList;
  }
  //
  // Close our temp output file
  //
  if ((mGlobals.OutFptr != stdout) && (mGlobals.OutFptr != NULL)) {
    fclose (mGlobals.OutFptr);
  }

  if (mGlobals.NeverFail) {
    return STATUS_SUCCESS;
  }

  if (mGlobals.OutFileName != NULL) {
    if (GetUtilityStatus () == STATUS_ERROR) {
      //
      // If any errors, then delete our temp output
      // Also try to delete target file to improve the incremental build
      //      
      remove (mGlobals.TmpFileName);
      remove (TargetFileName);
    } else {
      //
      // Otherwise, rename temp file to output file
      //
      remove (mGlobals.OutFileName);
      rename (mGlobals.TmpFileName, mGlobals.OutFileName);
    }
  }

  return GetUtilityStatus ();
}

static
STATUS
ProcessFile (
  INT8              *TargetFileName,
  INT8              *FileName,
  UINT32            NestDepth,
  STRING_LIST       *ProcessedFiles,
  FILE_SEARCH_TYPE  FileSearchType
  )
/*++

Routine Description:

  Given a source file name, open the file and parse all #include lines.
  
Arguments:

  TargetFileName - name of the usually .obj target
  FileName       - name of the file to process
  NestDepth      - how deep we're nested in includes
  ProcessedFiles - list of processed files.
  FileSearchType - search type for FileName

Returns:

  standard status.
  
--*/
{
  FILE        *Fptr;
  INT8        Line[MAX_LINE_LEN];
  INT8        *Cptr;
  INT8        *EndPtr;
  INT8        *SaveCptr;
  INT8        EndChar;
  INT8        FileNameCopy[MAX_PATH];
  INT8        MacroIncludeFileName[MAX_LINE_LEN];
  INT8        SumDepsFile[MAX_PATH];
  STATUS      Status;
  UINT32      Index;
  UINT32      LineNum;
  STRING_LIST *ListPtr;
  STRING_LIST ParentPath;

  Status  = STATUS_SUCCESS;
  Fptr    = NULL;
  //
  // Print the file being processed. Indent so you can tell the include nesting
  // depth.
  //
  if (mGlobals.Verbose) {
    fprintf (stdout, "%*cProcessing file '%s'\n", NestDepth * 2, ' ', FileName);
  }
  //
  // If we're using summary dependency files, and a matching .dep file is
  // found for this file, then just emit the summary dependency file as
  // a dependency and return.
  //
  if (mGlobals.UseSumDeps) {
    strcpy (SumDepsFile, mGlobals.SumDepsPath);
    strcat (SumDepsFile, FileName);
    for (Cptr = SumDepsFile + strlen (SumDepsFile) - 1;
         (*Cptr != '\\') && (Cptr > SumDepsFile) && (*Cptr != '.');
         Cptr--
        )
      ;
    if (*Cptr == '.') {
      strcpy (Cptr, ".dep");
    } else {
      strcat (SumDepsFile, ".dep");
    }
    //
    // See if the summary dep file exists. Could use _stat() function, but
    // it's less portable.
    //
    if ((Fptr = fopen (SumDepsFile, "r")) != NULL) {
      PrintDependency (TargetFileName, SumDepsFile);
      fclose (Fptr);
      return STATUS_SUCCESS;
    }
  }

  //
  // Make sure we didn't exceed our maximum nesting depth
  //
  if (NestDepth > MAX_NEST_DEPTH) {
    Error (NULL, 0, 0, FileName, "max nesting depth exceeded on file");
    goto Finish;
  }
  //
  // Make a local copy of the filename. Then we can manipulate it
  // if we have to.
  //
  strcpy (FileNameCopy, FileName);
  
  if (FileSearchType == SearchCurrentDir) {
    //
    // Try to open the source file locally
    //
    if ((Fptr = fopen (FileNameCopy, "r")) == NULL) {
      Error (NULL, 0, 0, FileNameCopy, "could not open source file");
      return STATUS_ERROR;
    }
  } else {
    //
    // Try to find it among the paths.
    //
    Fptr = FindFile (FileNameCopy, sizeof (FileNameCopy), FileSearchType);
    if (Fptr == NULL) {
      //
      // If this is not the top-level file, and the command-line argument
      // said to ignore missing files, then return ok
      //
      if (NestDepth != START_NEST_DEPTH) {
        if (mGlobals.IgnoreNotFound) {
          if (!mGlobals.QuietMode) {
            DebugMsg (NULL, 0, 0, FileNameCopy, "could not find file");
          }

          return STATUS_SUCCESS;
        } else {
          Error (NULL, 0, 0, FileNameCopy, "could not find file");
          return STATUS_ERROR;
        }
      } else {
        //
        // Top-level (first) file. Emit an error.
        //
        Error (NULL, 0, 0, FileNameCopy, "could not find file");
        return STATUS_ERROR;
      }
    }
  }

  //
  // If we're not doing duplicates, and we've already seen this filename,
  // then return
  //
  if (mGlobals.NoDupes) {
    for (ListPtr = ProcessedFiles->Next; ListPtr != NULL; ListPtr = ListPtr->Next) {
      if (_stricmp (FileNameCopy, ListPtr->Str) == 0) {
        break;
      }
    }
    //
    // If we found a match, we're done. If we didn't, create a new element
    // and add it to the list.
    //
    if (ListPtr != NULL) {
      //
      // Print a message if verbose mode
      //
      if (mGlobals.Verbose) {
        DebugMsg (NULL, 0, 0, FileNameCopy, "duplicate include -- not processed again");
      }
      fclose (Fptr);
      return STATUS_SUCCESS;
    }

    ListPtr       = malloc (sizeof (STRING_LIST));
    ListPtr->Str  = malloc (strlen (FileNameCopy) + 1);
    strcpy (ListPtr->Str, FileNameCopy);
    ListPtr->Next         = ProcessedFiles->Next;
    ProcessedFiles->Next  = ListPtr;
  }
    
  //
  // Print the dependency, with string substitution
  //
  PrintDependency (TargetFileName, FileNameCopy);
  
  //
  // Get the file path and push to ParentPaths
  //
  Cptr = FileNameCopy + strlen (FileNameCopy) - 1;
  for (; (Cptr > FileNameCopy) && (*Cptr != '\\') && (*Cptr != '/'); Cptr--);
  if ((*Cptr == '\\') || (*Cptr == '/')) {
    *(Cptr + 1) = 0;
  } else {
    strcpy (FileNameCopy, ".\\");
  }
  ParentPath.Next = mGlobals.ParentPaths;
  ParentPath.Str = FileNameCopy;
  mGlobals.ParentPaths = &ParentPath;
  
  //
  // Now read in lines and find all #include lines. Allow them to indent, and
  // to put spaces between the # and include.
  //
  LineNum = 0;
  while ((fgets (Line, sizeof (Line), Fptr) != NULL) && (Status == STATUS_SUCCESS)) {
    LineNum++;
    Cptr = Line;
    //
    // Skip preceeding spaces on the line
    //
    while (*Cptr && (isspace (*Cptr))) {
      Cptr++;
    }
    //
    // Check for # character, there is no # for asm
    //
    if ((*Cptr == '#') || (mGlobals.IsAsm)) {
      if (*Cptr == '#') {
        Cptr++;
      }
      
      //
      // Check for "include", case insensitive for asm
      //
      while (*Cptr && (isspace (*Cptr))) {
        Cptr++;
      }
      if (((!mGlobals.IsAsm) && (strncmp (Cptr, "include", 7) == 0)) || 
          (mGlobals.IsAsm && (_strnicmp (Cptr, "include", 7) == 0))) {
        //
        // Skip over "include" and move on to filename as "file" or <file> or file for asm
        //
        Cptr += 7;
        while (*Cptr && (isspace (*Cptr))) {
          Cptr++;
        }

        if (*Cptr == '<') {
          EndChar = '>';
        } else if (*Cptr == '"') {
          EndChar = '"';
        } else if (mGlobals.IsAsm) {
          //
          // Handle include file for asm
          // Set EndChar to null so we fall through on processing below.
          //
          EndChar = 0;
          
          //
          // Look for the end of include file name
          //
          EndPtr = Cptr;
          while (*EndPtr && (!isspace (*EndPtr))) {
            EndPtr++;
          }
      
          //
          // Null terminate the filename and try to process it.
          //
          *EndPtr = 0;
          Status  = ProcessFile (TargetFileName, Cptr, NestDepth + 1, 
                                 ProcessedFiles, SearchAllPaths);
        } else {
          //
          // Handle special #include MACRO_NAME(file)
          // Set EndChar to null so we fall through on processing below.
          //
          EndChar = 0;
          //
          // Look for all the special include macros and convert accordingly.
          //
          for (Index = 0; mMacroConversion[Index].IncludeMacroName != NULL; Index++) {
            //
            // Save the start of the string in case some macros are substrings
            // of others.
            //
            SaveCptr = Cptr;
            if (strncmp (
                  Cptr,
                  mMacroConversion[Index].IncludeMacroName,
                  strlen (mMacroConversion[Index].IncludeMacroName)
                  ) == 0) {
              //
              // Skip over the macro name
              //
              Cptr += strlen (mMacroConversion[Index].IncludeMacroName);
              //
              // Skip over open parenthesis, blank spaces, then find closing
              // parenthesis or blank space
              //
              while (*Cptr && (isspace (*Cptr))) {
                Cptr++;
              }

              if (*Cptr == '(') {
                Cptr++;
                while (*Cptr && (isspace (*Cptr))) {
                  Cptr++;
                }

                EndPtr = Cptr;
                while (*EndPtr && !isspace (*EndPtr) && (*EndPtr != ')')) {
                  EndPtr++;
                }

                *EndPtr = 0;
                //
                // Create the path
                //
                strcpy (MacroIncludeFileName, mMacroConversion[Index].PathName);
                strcat (MacroIncludeFileName, Cptr);
                strcat (MacroIncludeFileName, "\\");
                strcat (MacroIncludeFileName, Cptr);
                strcat (MacroIncludeFileName, ".h");
                //
                // Process immediately, then break out of the outside FOR loop.
                //
                Status = ProcessFile (TargetFileName, MacroIncludeFileName, NestDepth + 1, 
                                      ProcessedFiles, SearchAllPaths);
                break;
              }
            }
            //
            // Restore the start
            //
            Cptr = SaveCptr;
          }
          //
          // Don't recognize the include line? Ignore it. We assume that the
          // file compiles anyway.
          //
          if (mMacroConversion[Index].IncludeMacroName == NULL) {
            //
            // Warning (FileNameCopy, LineNum, 0, "could not parse line", NULL);
            // Status = STATUS_WARNING;
            //
          }
        }
        //
        // Process "normal" includes. If the endchar is 0, then the
        // file has already been processed. Otherwise look for the
        // endchar > or ", and process the include file.
        //
        if (EndChar != 0) {
          Cptr++;
          EndPtr = Cptr;
          while (*EndPtr && (*EndPtr != EndChar)) {
            EndPtr++;
          }

          if (*EndPtr == EndChar) {
            //
            // If we're processing it, do it
            //
            if (EndChar != '>') {
              //
              // Null terminate the filename and try to process it.
              //
              *EndPtr = 0;
              Status  = ProcessFile (TargetFileName, Cptr, NestDepth + 1, 
                                     ProcessedFiles, SearchAllPaths);
            } else if (!mGlobals.NoSystem) {
              //
              // Null terminate the filename and try to process it.
              //
              *EndPtr = 0;
              Status  = ProcessFile (TargetFileName, Cptr, NestDepth + 1, 
                                     ProcessedFiles, SearchIncludePaths);
            }
          } else {
            Warning (FileNameCopy, LineNum, 0, "malformed include", "missing closing %c", EndChar);
            Status = STATUS_WARNING;
            goto Finish;
          }
        }
      }
    }
  }
  //
  // Pop the file path from ParentPaths
  //
  mGlobals.ParentPaths = ParentPath.Next;  

Finish:
  //
  // Close open files and return status
  //
  if (Fptr != NULL) {
    fclose (Fptr);
  }

  return Status;
}

static
STATUS
ProcessClOutput (
  INT8            *TargetFileName,
  INT8            *FileName,
  STRING_LIST     *ProcessedFiles
  )
/*++

Routine Description:

  Given a source file name, open the file and parse all "Note: including file: xxx.h" lines.
  
Arguments:

  TargetFileName - name of the usually .obj target
  FileName       - name of the file to process
  ProcessedFiles - list of processed files.

Returns:

  standard status.
  
--*/
{
  FILE        *Fptr;
  INT8        Line[MAX_LINE_LEN];
  INT8        IncludeFileName[MAX_LINE_LEN];
  STRING_LIST *ListPtr;
  BOOLEAN     ClError;
  INT32       Ret;
  INT8        Char;

  if ((Fptr = fopen (FileName, "r")) == NULL) {
    Error (NULL, 0, 0, FileName, "could not open file for reading");
    return STATUS_ERROR;
  }
  if (fgets (Line, sizeof (Line), Fptr) != NULL) {
    //
    // First line is the source file name, print it
    //
    printf ("%s", Line);
  } else {
    //
    // No output from cl
    //
    fclose (Fptr);
    Error (NULL, 0, 0, NULL, "incorrect cl tool path may be used ");
    return STATUS_ERROR;
  }
  
  ClError = FALSE;
  while (fgets (Line, sizeof (Line), Fptr) != NULL) {
    Ret = sscanf (Line, "Note: including file: %s %c", IncludeFileName, &Char);
    if (Ret == 2) {
      //
      // There is space in include file name. It's VS header file. Ignore it.
      //
      continue;
    } else if ( Ret != 1) {
      //
      // Cl error info, print it
      // the tool will return error code to stop the nmake
      //
      ClError = TRUE;
      printf ("%s", Line);
      continue;
    }
    
    //
    // If we're not doing duplicates, and we've already seen this filename,
    // then continue
    //
    if (mGlobals.NoDupes) {
      for (ListPtr = ProcessedFiles->Next; ListPtr != NULL; ListPtr = ListPtr->Next) {
        if (_stricmp (IncludeFileName, ListPtr->Str) == 0) {
          break;
        }
      }
      //
      // If we found a match, we're done. If we didn't, create a new element
      // and add it to the list.
      //
      if (ListPtr != NULL) {
        //
        // Print a message if verbose mode
        //
        if (mGlobals.Verbose) {
          DebugMsg (NULL, 0, 0, IncludeFileName, "duplicate include -- not processed again");
        }
  
        continue;
      }
  
      ListPtr       = malloc (sizeof (STRING_LIST));
      ListPtr->Str  = malloc (strlen (IncludeFileName) + 1);
      strcpy (ListPtr->Str, IncludeFileName);
      ListPtr->Next         = ProcessedFiles->Next;
      ProcessedFiles->Next  = ListPtr;
    }
    
    PrintDependency (TargetFileName, IncludeFileName);
  }
  
  fclose (Fptr);
  
  if (ClError) {
    Error (NULL, 0, 0, NULL, "cl error");
    return STATUS_ERROR;
  } else {
    return STATUS_SUCCESS;
  }
}

static
void
PrintDependency (
  INT8    *TargetFileName,
  INT8    *DependentFile
  )
/*++

Routine Description:

  Given a target (.obj) file name, and a dependent file name, do any string
  substitutions (per the command line options) on the file names, then
  print the dependency line of form:
  
  TargetFileName : DependentFile
  
Arguments:

  TargetFileName - build target file name
  DependentFile  - file on which TargetFileName depends

Returns:

  None
  
--*/
{
  INT8  Str[MAX_PATH];

  //
  // Go through the symbols and do replacements
  //
  strcpy (Str, TargetFileName);
  ReplaceSymbols (Str, sizeof (Str));
  fprintf (mGlobals.OutFptr, "%s : ", Str);
  strcpy (Str, DependentFile);
  ReplaceSymbols (Str, sizeof (Str));
  fprintf (mGlobals.OutFptr, "%s\n", Str);
  //
  // Add pseudo target to avoid incremental build failure when the file is deleted
  //
  fprintf (mGlobals.OutFptr, "%s : \n", Str);
}

static
void
ReplaceSymbols (
  INT8    *Str,
  UINT32  StrSize
  )
{
  SYMBOL  *Sym;
  INT8    StrCopy[MAX_LINE_LEN];
  INT8    *From;
  INT8    *To;
  BOOLEAN Replaced;

  //
  // Go through the entire string to look for replacement strings at
  // every position.
  //
  From  = Str;
  To    = StrCopy;
  while (*From) {
    //
    // Copy the character
    //
    *To       = *From;
    Replaced  = FALSE;
    //
    // Go through each symbol and try to find a string substitution
    //
    Sym = mGlobals.SymbolTable;
    while (Sym != NULL) {
      if (_strnicmp (From, Sym->Value, strlen (Sym->Value)) == 0) {
        //
        // Replace the string, then advance the pointers past the
        // replaced strings
        //
        strcpy (To, Sym->Name);
        To += strlen (Sym->Name);
        From += strlen (Sym->Value);
        Replaced = TRUE;
        //
        // Break from the while()
        //
        break;
      } else {
        Sym = Sym->Next;
      }
    }

    if (!Replaced) {
      From++;
      To++;
    }
  }
  //
  // Null terminate, and return it
  //
  *To = 0;
  if (strlen (StrCopy) < StrSize) {
    strcpy (Str, StrCopy);
  }
}
//
// Given a filename, try to find it along the include paths.
//
static
FILE *
FindFile (
  INT8              *FileName,
  UINT32            FileNameLen,
  FILE_SEARCH_TYPE  FileSearchType
  )
{
  FILE        *Fptr;
  STRING_LIST *List;
  STRING_LIST *SubDir;
  INT8        FullFileName[MAX_PATH * 2];

  //
  // Traverse the list of paths and try to find the file
  //
  if (FileSearchType == SearchAllPaths) {
    List = mGlobals.ParentPaths;
    while (List != NULL) {
      //
      // Put the path and filename together
      //
      if (strlen (List->Str) + strlen (FileName) + 1 > sizeof (FullFileName)) {
        Error (
          __FILE__,
          __LINE__,
          0,
          "application error",
          "cannot concatenate '%s' + '%s'",
          List->Str,
          FileName
          );
        return NULL;
      }
      //
      // Append the filename to this include path and try to open the file.
      //
      strcpy (FullFileName, List->Str);
      strcat (FullFileName, FileName);
      if ((Fptr = fopen (FullFileName, "r")) != NULL) {
        //
        // Return the file name
        //
        if (FileNameLen <= strlen (FullFileName)) {
          Error (__FILE__, __LINE__, 0, "application error", "internal path name of insufficient length");
          //
          // fprintf (stdout, "File length > %d: %s\n", FileNameLen, FullFileName);
          //
          return NULL;
        }
  
        strcpy (FileName, FullFileName);
        return Fptr;
      }
  
      List = List->Next;
    }    
  }
  
  List = mGlobals.IncludePaths;
  while (List != NULL) {
    //
    // Put the path and filename together
    //
    if (strlen (List->Str) + strlen (FileName) + 1 > sizeof (FullFileName)) {
      Error (
        __FILE__,
        __LINE__,
        0,
        "application error",
        "cannot concatenate '%s' + '%s'",
        List->Str,
        FileName
        );
      return NULL;
    }
    //
    // Append the filename to this include path and try to open the file.
    //
    strcpy (FullFileName, List->Str);
    strcat (FullFileName, FileName);
    if ((Fptr = fopen (FullFileName, "r")) != NULL) {
      //
      // Return the file name
      //
      if (FileNameLen <= strlen (FullFileName)) {
        Error (__FILE__, __LINE__, 0, "application error", "internal path name of insufficient length");
        //
        // fprintf (stdout, "File length > %d: %s\n", FileNameLen, FullFileName);
        //
        return NULL;
      }

      strcpy (FileName, FullFileName);
      return Fptr;
    }
    //
    // Didn't find it there. Now try this directory with every subdirectory
    // the user specified on the command line
    //
    for (SubDir = mGlobals.SubDirs; SubDir != NULL; SubDir = SubDir->Next) {
      strcpy (FullFileName, List->Str);
      strcat (FullFileName, SubDir->Str);
      strcat (FullFileName, FileName);
      if ((Fptr = fopen (FullFileName, "r")) != NULL) {
        //
        // Return the file name
        //
        if (FileNameLen <= strlen (FullFileName)) {
          Error (__FILE__, __LINE__, 0, "application error", "internal path name of insufficient length");
          return NULL;
        }

        strcpy (FileName, FullFileName);
        return Fptr;
      }
    }

    List = List->Next;
  }
  //
  // Not found
  //
  return NULL;
}
//
// Process the command-line arguments
//
static
STATUS
ProcessArgs (
  int   Argc,
  char  *Argv[]
  )
{
  STRING_LIST *NewList;
  STRING_LIST *LastIncludePath;
  STRING_LIST *LastSourceFile;
  SYMBOL      *Symbol;

  //
  // Clear our globals
  //
  memset ((char *) &mGlobals, 0, sizeof (mGlobals));
  mGlobals.NoDupes = TRUE;
  //
  // Skip program name
  //
  Argc--;
  Argv++;
  //
  // Initialize locals
  //
  LastIncludePath = NULL;
  LastSourceFile  = NULL;
  //
  // Process until no more args
  //
  while (Argc) {
    //
    // -i path    add include search path
    //
    if (_stricmp (Argv[0], "-i") == 0) {
      //
      // check for one more arg
      //
      if (Argc > 1) {
        //
        // Allocate memory for a new list element, fill it in, and
        // add it to our list of include paths. Always make sure it
        // has a "\" on the end of it.
        //
        NewList = malloc (sizeof (STRING_LIST));
        if (NewList == NULL) {
          Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
          return STATUS_ERROR;
        }

        NewList->Next = NULL;
        NewList->Str  = malloc (strlen (Argv[1]) + 2);
        if (NewList->Str == NULL) {
          free (NewList);
          Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
          return STATUS_ERROR;
        }

        strcpy (NewList->Str, Argv[1]);
        if (NewList->Str[strlen (NewList->Str) - 1] != '\\') {
          strcat (NewList->Str, "\\");
        }
        //
        // Add it to the end of the our list of include paths
        //
        if (mGlobals.IncludePaths == NULL) {
          mGlobals.IncludePaths = NewList;
        } else {
          LastIncludePath->Next = NewList;
        }

        LastIncludePath = NewList;
        //
        // fprintf (stdout, "Added path: %s\n", NewList->Str);
        //
      } else {
        Error (NULL, 0, 0, Argv[0], "option requires an include path");
        Usage ();
        return STATUS_ERROR;
      }

      Argc--;
      Argv++;
    } else if (_stricmp (Argv[0], "-f") == 0) {
      //
      // Check for one more arg
      //
      if (Argc > 1) {
        //
        // Allocate memory for a new list element, fill it in, and
        // add it to our list of source files.
        //
        NewList = malloc (sizeof (STRING_LIST));
        if (NewList == NULL) {
          Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
          return STATUS_ERROR;
        }

        NewList->Next = NULL;
        //
        // Allocate space to replace ".c" with ".obj", plus null termination
        //
        NewList->Str = malloc (strlen (Argv[1]) + 5);
        if (NewList->Str == NULL) {
          free (NewList);
          Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
          return STATUS_ERROR;
        }

        strcpy (NewList->Str, Argv[1]);
        if (mGlobals.SourceFiles == NULL) {
          mGlobals.SourceFiles = NewList;
        } else {
          LastSourceFile->Next = NewList;
        }

        LastSourceFile = NewList;
      } else {
        Error (NULL, 0, 0, Argv[0], "option requires a file name");
        Usage ();
        return STATUS_ERROR;
      }

      Argc--;
      Argv++;
    } else if (_stricmp (Argv[0], "-s") == 0) {
      //
      // -s subdir    add subdirectory subdir to list of subdirecties to scan.
      // Check for one more arg first.
      //
      if (Argc > 1) {
        //
        // Allocate memory for a new list element, fill it in, and
        // add it to our list of subdirectory include paths. Always
        // make sure it has a "\" on the end of it.
        //
        NewList = malloc (sizeof (STRING_LIST));
        if (NewList == NULL) {
          Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
          return STATUS_ERROR;
        }

        NewList->Str = malloc (strlen (Argv[1]) + 2);
        if (NewList->Str == NULL) {
          free (NewList);
          Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
          return STATUS_ERROR;
        }

        strcpy (NewList->Str, Argv[1]);
        if (NewList->Str[strlen (NewList->Str) - 1] != '\\') {
          strcat (NewList->Str, "\\");
        }

        NewList->Next     = mGlobals.SubDirs;
        mGlobals.SubDirs  = NewList;
      } else {
        Error (NULL, 0, 0, Argv[0], "option requires a subdirectory name");
        Usage ();
        return STATUS_ERROR;
      }

      Argc--;
      Argv++;
    } else if (_stricmp (Argv[0], "-sub") == 0) {
      //
      // -sub symname symvalue  to do string substitution in the output
      //
      if (Argc > 2) {
        //
        // Allocate memory for the symbol object
        //
        Symbol = malloc (sizeof (SYMBOL));
        if (Symbol == NULL) {
          Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
          return STATUS_ERROR;
        }
        //
        // Allocate memory for the symbol name and value, then save copies
        //
        Symbol->Name = malloc (strlen (Argv[1]) + 1);
        if (Symbol->Name == NULL) {
          free (Symbol);
          Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
          return STATUS_ERROR;
        }

        strcpy (Symbol->Name, Argv[1]);
        Symbol->Value = malloc (strlen (Argv[2]) + 1);
        if (Symbol->Value == NULL) {
          free (Symbol->Name);
          free (Symbol);
          Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
          return STATUS_ERROR;
        }

        strcpy (Symbol->Value, Argv[2]);
        //
        // Add it to the list
        //
        Symbol->Next          = mGlobals.SymbolTable;
        mGlobals.SymbolTable  = Symbol;
      } else {
        Error (NULL, 0, 0, Argv[0], "option requires a symbol name and value");
        Usage ();
        return STATUS_ERROR;
      }
      //
      // Skip over args
      //
      Argc -= 2;
      Argv += 2;
    } else if (_stricmp (Argv[0], "-nosystem") == 0) {
      mGlobals.NoSystem = TRUE;
    } else if (_stricmp (Argv[0], "-nodupes") == 0) {
      mGlobals.NoDupes = TRUE;
    } else if (_stricmp (Argv[0], "-nodups") == 0) {
      mGlobals.NoDupes = TRUE;
    } else if (_stricmp (Argv[0], "-target") == 0) {
      //
      // -target TargetFileName  - Target object file (only one allowed right
      // now) is TargetFileName rather than SourceFile.obj
      //
      if (Argc > 1) {
        strcpy (mGlobals.TargetFileName, Argv[1]);
      } else {
        Error (NULL, 0, 0, Argv[0], "option requires a target file name");
        Usage ();
        return STATUS_ERROR;
      }

      Argc--;
      Argv++;
    } else if (_stricmp (Argv[0], "-usesumdeps") == 0) {
      //
      // -usesumdeps Path - if we find an included file xxx.h, and file
      // Path/xxx.dep exists, list Path/xxx.dep as a dependency rather than
      // xxx.h and don't parse xxx.h. This allows you to create a dependency
      // file for a commonly included file, and have its dependency file updated
      // only if its included files are updated. Then anyone else including this
      // common include file can simply have a dependency on that file's .dep file
      // rather than on all the files included by it. Confusing enough?
      //
      mGlobals.UseSumDeps = 1;
      if (Argc > 1) {
        strcpy (mGlobals.SumDepsPath, Argv[1]);
        //
        // Add slash on end if not there
        //
        if (mGlobals.SumDepsPath[strlen (mGlobals.SumDepsPath) - 1] != '\\') {
          strcat (mGlobals.SumDepsPath, "\\");
        }
      } else {
        Error (NULL, 0, 0, Argv[0], "option requires path to summary dependency files");
        Usage ();
        return STATUS_ERROR;
      }

      Argc--;
      Argv++;

    } else if (_stricmp (Argv[0], "-o") == 0) {
      //
      // -o OutputFileName    - specify an output filename for dependency list
      // check for one more arg
      //
      if (Argc > 1) {
        mGlobals.OutFileName = Argv[1];
        //
        // Use temp file for output
        // This can avoid overwriting previous existed dep file when error 
        // ocurred in this tool
        //
        sprintf (mGlobals.TmpFileName, "%s2", mGlobals.OutFileName);
        //
        // Try to open the temp file
        //
        if ((mGlobals.OutFptr = fopen (mGlobals.TmpFileName, "w")) == NULL) {
          Error (NULL, 0, 0, mGlobals.TmpFileName, "could not open file for writing");
          return STATUS_ERROR;
        }
      } else {
        Error (NULL, 0, 0, Argv[0], "option requires output file name");
        Usage ();
        return STATUS_ERROR;
      }

      Argc--;
      Argv++;
    } else if (_stricmp (Argv[0], "-v") == 0) {
      mGlobals.Verbose = TRUE;
    } else if (_stricmp (Argv[0], "-neverfail") == 0) {
      mGlobals.NeverFail = TRUE;
    } else if (_stricmp (Argv[0], "-q") == 0) {
      mGlobals.QuietMode = TRUE;
    } else if (_stricmp (Argv[0], "-ignorenotfound") == 0) {
      mGlobals.IgnoreNotFound = TRUE;
    } else if (_stricmp (Argv[0], "-asm") == 0) {
      if (mGlobals.IsCl) {
        Error (NULL, 0, 0, Argv[0], "option conflict with -cl");
        return STATUS_ERROR;
      }      
      mGlobals.IsAsm = TRUE;
    } else if (_stricmp (Argv[0], "-cl") == 0) {
      if (mGlobals.IsAsm) {
        Error (NULL, 0, 0, Argv[0], "option conflict with -asm");
        return STATUS_ERROR;
      }
      mGlobals.IsCl = TRUE; 
    } else if ((_stricmp (Argv[0], "-h") == 0) || (strcmp (Argv[0], "-?") == 0)) {
      Usage ();
      return STATUS_ERROR;
    } else {
      Error (NULL, 0, 0, Argv[0], "unrecognized option");
      Usage ();
      return STATUS_ERROR;
    }

    Argc--;
    Argv++;
  }
  //
  // Had to specify at least one source file
  //
  if (mGlobals.SourceFiles == NULL) {
    Error (NULL, 0, 0, "must specify one source file name", NULL);
    Usage ();
    return STATUS_ERROR;
  }
  //
  // Assume output to stdout if not specified
  //
  if (mGlobals.OutFptr == NULL) {
    mGlobals.OutFptr = stdout;
  }

  return STATUS_SUCCESS;
}
//
// Free the global string lists we allocated memory for
//
static
void
FreeLists (
  VOID
  )
{
  STRING_LIST *Temp;
  SYMBOL      *NextSym;

  //
  // printf ("Free lists.....");
  //
  // Traverse the include paths, freeing each
  // printf ("freeing include paths\n");
  //
  while (mGlobals.IncludePaths != NULL) {
    Temp = mGlobals.IncludePaths->Next;
    //
    // printf ("Freeing include path string '%s' at 0x%X\n",
    //  mGlobals.IncludePaths->Str, (int)(mGlobals.IncludePaths->Str));
    //
    free (mGlobals.IncludePaths->Str);
    //
    // printf ("Freeing include path object at 0x%X\n", (int)(mGlobals.IncludePaths));
    //
    free (mGlobals.IncludePaths);
    mGlobals.IncludePaths = Temp;
  }
  //
  // Traverse the source files, freeing each
  //
  while (mGlobals.SourceFiles != NULL) {
    Temp = mGlobals.SourceFiles->Next;
    free (mGlobals.SourceFiles->Str);
    free (mGlobals.SourceFiles);
    mGlobals.SourceFiles = Temp;
  }
  //
  // Traverse the subdirectory list, freeing each
  //
  while (mGlobals.SubDirs != NULL) {
    Temp = mGlobals.SubDirs->Next;
    free (mGlobals.SubDirs->Str);
    free (mGlobals.SubDirs);
    mGlobals.SubDirs = Temp;
  }
  //
  // Free the symbol table
  //
  while (mGlobals.SymbolTable != NULL) {
    NextSym = mGlobals.SymbolTable->Next;
    free (mGlobals.SymbolTable->Name);
    free (mGlobals.SymbolTable->Value);
    mGlobals.SymbolTable = NextSym;
  }
  //
  // printf ("done\n");
  //
}

static
void
Usage (
  VOID
  )
/*++

Routine Description:

  Print usage information for this utility.
  
Arguments:

  None.

Returns:

  Nothing.
  
--*/
{
  int         Index;
  const char  *Str[] = {
    UTILITY_NAME" "UTILITY_VERSION" - Intel Make Dependencies Utility",
    "  Copyright (C), 2004 - 2008 Intel Corporation",
    
#if ( defined(UTILITY_BUILD) && defined(UTILITY_VENDOR) )
    "  Built from "UTILITY_BUILD", project of "UTILITY_VENDOR,
#endif
    "",
    "Usage:",
    "  "UTILITY_NAME" [OPTION]...",
    "Options:",
    "  -h or -?         for this help information",
    "  -f SourceFile    add SourceFile to list of files to scan",
    "  -i IncludePath   add IncludePath to list of search paths",
    "  -o OutputFile    write output dependencies to OutputFile",
    "  -s SubDir        for each IncludePath, also search IncludePath\\SubDir",
    "  -v               for verbose output",
    "  -ignorenotfound  don't warn for files not found",
    "  -target Target   for single SourceFile, target is Target, not SourceFile.obj",
    "  -q               quiet mode to not report files not found if ignored",
    "  -sub sym str     replace all occurrances of 'str' with 'sym' in the output",
    "  -nosystem        not process system <include> files",
    "  -neverfail       always return a success return code",
    //
    //    "  -nodupes         keep track of include files, don't rescan duplicates",
    //
    "  -usesumdeps path use summary dependency files in 'path' directory.",
    "  -asm             The SourceFiles are assembler files",
    "  -cl              The SourceFiles are the output of cl with /showIncludes",
    NULL
  };
  for (Index = 0; Str[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Str[Index]);
  }
}
