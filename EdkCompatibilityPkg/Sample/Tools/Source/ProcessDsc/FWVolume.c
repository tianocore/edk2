/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FWVolume.c

Abstract:

  This module contains functionality to keep track of files destined for
  multiple firmware volues. It saves them up, and when told to, dumps the
  file names out to some files used as input to other utilities that
  actually generate the FVs.

--*/

#include <windows.h>                        // for max_path definition
#include <stdio.h>
#include <string.h>
#include <stdlib.h>                         // for malloc()
#include "Common.h"
#include "DSCFile.h"
#include "FWVolume.h"

#define FV_INF_DIR          "FV_INF_DIR"    // symbol for where we create the FV INF file
#define FV_FILENAME         "FV_FILENAME"   // symbol for the current FV.INF filename
#define EFI_BASE_ADDRESS    "EFI_BASE_ADDRESS"
#define DEFAULT_FV_INF_DIR  "FV"            // default dir for where we create the FV INF file
#define DEFAULT_FV_DIR      "$(BUILD_DIR)"  // where the FV file comes from

typedef struct {
  char  *ComponentType;
  char  *Extension;
} COMP_TYPE_EXTENSION;

//
// Use a linked list of these to keep track of all the FV names used
//
typedef struct _FV_LIST {
  struct _FV_LIST *Next;
  char            FVFileName[MAX_PATH];
  char            BaseAddress[MAX_LINE_LEN];
  SMART_FILE      *FVFilePtr;
  SMART_FILE      *AprioriFilePtr;
  char            *Processor;
  int             ComponentsInstance; // highest [components.n] section with a file for this FV
} FV_LIST;

//
// Use a linked list of these to keep track of all FFS files built. When
// we're done, we turn the info into the FV INF files used to build the
// firmware volumes.
//
typedef struct _FILE_LIST {
  struct _FILE_LIST *Next;
  char              *FileName;
  char              *BaseFileName;
  char              *FVs;               // from FV=x,y,z
  char              *BaseName;          // only needed for duplicate basename check
  char              *Processor;         // only needed for duplicate basename check
  char              Apriori[100];       // of format "FVRecovery:1,FVMain:2" from APRIORI define
  char              *Guid;              // guid string
  int               ComponentsInstance; // which [components.n] section it's in
} FILE_LIST;

typedef struct _LINKED_LIST {
  struct _LINKED_LIST *Next;
  void                *Data;
} LINKED_LIST;

static FILE_LIST                  *mFileList;
static FILE_LIST                  *mLastFile;
static char                       *mXRefFileName  = NULL;
static FV_LIST                    *mNonFfsFVList  = NULL;

//
// Whenever an FV name is referenced, then add it to our list of known
// FV's using these.
//
static FV_LIST                    *mFVList      = NULL;
static FV_LIST                    *mFVListLast  = NULL;

//
// We use this list so that from a given component type, we can determine
// the name of the file on disk. For example, if we're given a file's
// guid and base name, and we know it's a "bs_driver", then we can look
// up "bs_driver" in this array and know that the file (after it's built)
// name is GUID-BASENAME.DXE
//
static const COMP_TYPE_EXTENSION  mCompTypeExtension[] = {
  {
    "bs_driver",
    ".dxe"
  },
  {
    "rt_driver",
    ".dxe"
  },
  {
    "sal_rt_driver",
    ".dxe"
  },
  {
    "security_core",
    ".sec"
  },
  {
    "pei_core",
    ".pei"
  },
  {
    "pic_peim",
    ".pei"
  },
  {
    "pe32_peim",
    ".pei"
  },
  {
    "relocatable_peim",
    ".pei"
  },
  {
    "binary",
    ".ffs"
  },
  {
    "application",
    ".app"
  },
  {
    "file",
    ".ffs"
  },
  {
    "fvimagefile",
    ".fvi"
  },
  {
    "rawfile",
    ".raw"
  },
  {
    "apriori",
    ".ffs"
  },
  {
    "combined_peim_driver",
    ".pei"
  },
  {
    NULL,
    NULL
  }
};

static
void
CFVFreeFileList (
  VOID
  );

static
char                              *
UpperCaseString (
  char *Str
  );

static
BOOLEAN
InSameFv (
  char  *FVs1,
  char  *FVs2
);

static
void
AddFirmwareVolumes (
  char          *FVs,
  int           ComponentsInstance
  );

static
BOOLEAN
OrderInFvList (
  char    *FvList,
  char    *FvName,
  int     *Order
  );

int
GetBaseAddress (
  char *Name,
  char *BaseAddress
  )
{
  char  *Start;
  char  *Cptr;
  char  CSave;
  char  *Value;

  Start = Name;
  while (*Name && isspace (*Name)) {
    Name++;
  }

  if (!*Name) {
    return STATUS_ERROR;
  }
  //
  // Find the end of the name. Either space or a '='.
  //
  for (Value = Name; *Value && !isspace (*Value) && (*Value != '='); Value++)
    ;
  if (!*Value) {
    return STATUS_ERROR;
  }
  //
  // Look for the '='
  //
  Cptr = Value;
  while (*Value && (*Value != '=')) {
    Value++;
  }

  if (!*Value) {
    return STATUS_ERROR;
  }
  //
  // Now truncate the name
  //
  CSave = *Cptr;
  *Cptr = 0;
  if (_stricmp (Name, EFI_BASE_ADDRESS) != 0) {
    return STATUS_ERROR;
  }

  *Cptr = CSave;
  //
  // Skip over the = and then any spaces
  //
  Value++;
  while (*Value && isspace (*Value)) {
    Value++;
  }
  //
  // Find end of string, checking for quoted string
  //
  if (*Value == '\"') {
    Value++;
    for (Cptr = Value; *Cptr && *Cptr != '\"'; Cptr++)
      ;
  } else {
    for (Cptr = Value; *Cptr && !isspace (*Cptr); Cptr++)
      ;
  }
  //
  // Null terminate the value string
  //
  CSave = *Cptr;
  *Cptr = 0;
  strcpy (BaseAddress, Value);
  *Cptr = CSave;

  return STATUS_SUCCESS;
}

int
CFVAddFVFile (
  char  *Name,
  char  *ComponentType,
  char  *FVs,
  int   ComponentsInstance,
  char  *FFSExt,
  char  *Processor,
  char  *Apriori,
  char  *BaseName,
  char  *Guid
  )
/*++

Routine Description:

  Add a file to the list of files in one or more firmware volumes.

Arguments:

  Name          - $(FILE_GUID)-$(BASE_NAME), or filename
  ComponentType - type of component being added. Required so we know the
                  resultant file name after it has been built
  FVs           - string of commma-separated FVs that the given file is
                  to be added to. For example, FVs="FV0001,FV0002"
  FFSExt        - FFS filename extension of the file after it has been built.
                  This is passed in to us in case we don't know the default
                  filename extension based on the component type.
  Processor     - the target processor which the FV is being built for
  Apriori       - pointer to the definition of APRIORI. For example APRIORI="FvRecovery:1,FvMain:4"

Returns:

  STATUS_SUCCESS if successful

--*/
{
  FILE_LIST *Ptr;
  char      FileName[MAX_PATH];
  char      Str[MAX_PATH];
  int       i;
  char      *Sym;

  // If they provided a filename extension for this type of file, then use it.
  // If they did not provide a filename extension, search our list for a
  // matching component type and use the extension appropriate for this
  // component type.
  //
  if (FFSExt == NULL) {
    //
    // They didn't give us a filename extension. Figure it out from the
    // component type.
    //
    for (i = 0; mCompTypeExtension[i].ComponentType != NULL; i++) {
      if (_stricmp (ComponentType, mCompTypeExtension[i].ComponentType) == 0) {
        FFSExt = mCompTypeExtension[i].Extension;
        break;
      }
    }
    //
    // If we don't know the file extension, then error out. Just means
    // the need to define "FFS_EXT = raw" in the component INF file.
    //
    if (mCompTypeExtension[i].ComponentType == NULL) {
      Error (
        NULL,
        0,
        0,
        ComponentType,
        "unknown component type - must define FFS_EXT for built filename extension in component INF file"
        );
      return STATUS_ERROR;
    }
  }
  //
  // We now have all the parts to the FFS filename. Prepend the path to it if
  // it's not a full pathname.
  // See if they overrode the default base directory for the FV files.
  //
  if (!IsAbsolutePath (Name)) {
    Sym = GetSymbolValue (FV_DIR);
    if (Sym == NULL) {
      Sym = DEFAULT_FV_DIR;
    }
    //
    // Create the file path. Something like $(BUILD_DIR)\$(PROCESSOR)\$(GUID)-$(BASE_NAME).ext
    // If the extension is non-zero length, then make sure there's a dot in it.
    //
    if ((strlen (FFSExt) > 0) && (FFSExt[0] != '.')) {
      sprintf (Str, "%s\\%s\\%s.%s", Sym, Processor, Name, FFSExt);
    } else {
      sprintf (Str, "%s\\%s\\%s%s", Sym, Processor, Name, FFSExt);
    }

    ExpandSymbols (Str, FileName, sizeof (FileName), EXPANDMODE_NO_UNDEFS);
  } else {
    strcpy (FileName, Name);
  }
  //
  // Traverse the list of files we have so far and make sure we don't have
  // any duplicate basenames. If the base name and processor match, then we'll
  // have build issues, so don't allow it. We also don't allow the same file GUID
  // in the same FV which will cause boot time error if we allow this.
  //
  Ptr = mFileList;
  while (Ptr != NULL) {
    if ((Ptr->BaseName != NULL) && (BaseName != NULL) && (_stricmp (BaseName, Ptr->BaseName) == 0)) {
      if ((Ptr->Processor != NULL) && (Processor != NULL) && (_stricmp (Processor, Ptr->Processor) == 0)) {
        Error (NULL, 0, 0, BaseName, "duplicate base name specified");
        return STATUS_ERROR;
      }
    }
    
    if ((Ptr->Guid != NULL) && (Guid != NULL) && (_stricmp (Guid, Ptr->Guid) == 0)) {
      if ((Ptr->FVs != NULL) && (FVs != NULL) && (InSameFv (FVs, Ptr->FVs))) {
        Error (NULL, 0, 0, Guid, "duplicate Guid specified in the same FV for %s and %s", 
               (Ptr->BaseName==NULL)?"Unknown":Ptr->BaseName, 
               (BaseName==NULL)?"Unknown":BaseName);
        return STATUS_ERROR;
      }
    }

    Ptr = Ptr->Next;
  }
  //
  // Allocate a new structure so we can add this file to the list of
  // files.
  //
  Ptr = (FILE_LIST *) malloc (sizeof (FILE_LIST));
  if (Ptr == NULL) {
    Error (NULL, 0, 0, NULL, "failed to allocate memory");
    return STATUS_ERROR;
  }

  memset ((char *) Ptr, 0, sizeof (FILE_LIST));
  Ptr->FileName = (char *) malloc (strlen (FileName) + 1);
  if (Ptr->FileName == NULL) {
    Error (NULL, 0, 0, NULL, "failed to allocate memory");
    return STATUS_ERROR;
  }

  strcpy (Ptr->FileName, FileName);
  Ptr->ComponentsInstance = ComponentsInstance;
  //
  // Allocate memory to save the FV list if it's going into an FV.
  //
  if ((FVs != NULL) && (FVs[0] != 0)) {
    Ptr->FVs = (char *) malloc (strlen (FVs) + 1);
    if (Ptr->FVs == NULL) {
      Error (NULL, 0, 0, NULL, "failed to allocate memory");
      return STATUS_ERROR;
    }

    strcpy (Ptr->FVs, FVs);
  }

  Ptr->BaseFileName = (char *) malloc (strlen (Name) + 1);
  if (Ptr->BaseFileName == NULL) {
    Error (NULL, 0, 0, NULL, "failed to allocate memory");
    return STATUS_ERROR;
  }

  strcpy (Ptr->BaseFileName, Name);
  //
  // Allocate memory for the basename if they gave us one. May not have one
  // if the user is simply adding pre-existing binary files to the image.
  //
  if (BaseName != NULL) {
    Ptr->BaseName = (char *) malloc (strlen (BaseName) + 1);
    if (Ptr->BaseName == NULL) {
      Error (NULL, 0, 0, NULL, "failed to allocate memory");
      return STATUS_ERROR;
    }

    strcpy (Ptr->BaseName, BaseName);
  }
  //
  // Allocate memory for the processor name
  //
  if (Processor != NULL) {
    Ptr->Processor = (char *) malloc (strlen (Processor) + 1);
    if (Ptr->Processor == NULL) {
      Error (NULL, 0, 0, NULL, "failed to allocate memory");
      return STATUS_ERROR;
    }

    strcpy (Ptr->Processor, Processor);
  }
  //
  // Allocate memory for the guid name
  //
  if (Guid != NULL) {
    Ptr->Guid = (char *) malloc (strlen (Guid) + 1);
    if (Ptr->Guid == NULL) {
      Error (NULL, 0, 0, NULL, "failed to allocate memory");
      return STATUS_ERROR;
    }

    strcpy (Ptr->Guid, Guid);
  }
  //
  // If non-null apriori symbol, then save the apriori list for this file
  //
  if (Apriori != NULL) {
    strcpy (Ptr->Apriori, Apriori);
  }

  if (mFileList == NULL) {
    mFileList = Ptr;
  } else {
    mLastFile->Next = Ptr;
  }

  mLastFile = Ptr;
  //
  // Add these firmware volumes to the list of known firmware
  // volume names.
  //
  AddFirmwareVolumes (FVs, ComponentsInstance);

  return STATUS_SUCCESS;
}

void
CFVConstructor (
  VOID
  )
{
  mFileList = NULL;
  mLastFile = NULL;
}

void
CFVDestructor (
  VOID
  )
{
  CFVFreeFileList ();
  //
  // Free up our firmware volume list
  //
  while (mFVList != NULL) {
    mFVListLast = mFVList->Next;
    free (mFVList);
    mFVList = mFVListLast;
  }
}

static
void
CFVFreeFileList (
  VOID
  )
{
  FILE_LIST *Next;
  while (mFileList != NULL) {
    if (mFileList->FileName != NULL) {
      free (mFileList->FileName);
    }

    if (mFileList->FVs != NULL) {
      free (mFileList->FVs);
    }

    free (mFileList->BaseFileName);
    if (mFileList->BaseName != NULL) {
      free (mFileList->BaseName);
    }

    if (mFileList->Processor != NULL) {
      free (mFileList->Processor);
    }

    if (mFileList->Guid != NULL) {
      free (mFileList->Guid);
    }

    Next = mFileList->Next;
    free (mFileList);
    mFileList = Next;
  }

  mFileList = NULL;
}

int
CFVWriteInfFiles (
  DSC_FILE  *DSC,
  FILE      *MakeFptr
  )
/*++

Routine Description:

  After processing all components in a DSC file, create the firmware
  volume INF files. We actually do a lot more here.

  * Create the FVxxx.inf file that is used by GenFvImage
  * Create the Apriori files for each firmware volume that requires one
  * Create makefile.out macros for FVxxx_FILES = FVxxx_FILES AnotherFile
    so you can do incremental builds of firmware volumes.
  * For each FV, emit its build commands to makefile.out

Arguments:

  DSC       - pointer to a DSC_FILE object to extract info from
  MakeFptr  - pointer to the output makefile

Returns:

  0  if successful
  non-zero otherwise

--*/
{
  FILE_LIST *FileListPtr;
  FV_LIST   *FVList;
  FV_LIST   *LastFVList;
  FV_LIST   *FVPtr;
  SECTION   *Section;
  char      *StartCptr;
  char      *EndCptr;
  char      CSave;
  char      Str[MAX_PATH];
  char      Line[MAX_LINE_LEN];
  char      ExpandedLine[MAX_LINE_LEN];
  char      FVDir[MAX_PATH];
  FILE      *XRefFptr;
  int       AprioriCounter;
  int       AprioriCount;
  int       AprioriPosition;
  BOOLEAN   AprioriFound;
  int       ComponentsInstance;
  int       ComponentCount;

  //
  // Use this to keep track of all the firmware volume names
  //
  FVList      = NULL;
  LastFVList  = NULL;
  //
  // See if they specified a FV directory to dump the FV files out to. If not,
  // then use the default. Then create the output directory.
  //
  StartCptr = GetSymbolValue (FV_INF_DIR);
  if (StartCptr == NULL) {
    ExpandSymbols (DEFAULT_FV_INF_DIR, FVDir, sizeof (FVDir), EXPANDMODE_NO_UNDEFS);
  } else {
    strcpy (FVDir, StartCptr);
  }
  //
  // Make sure the fv directory path ends in /
  //
  CSave = FVDir[strlen (FVDir) - 1];
  if ((CSave != '\\') && (CSave != '/')) {
    strcat (FVDir, "\\");
  }
  //
  // Traverse the list of all files, determine which FV each is in, then
  // write out the file's name to the output FVxxx.inf file.
  //
  for (FileListPtr = mFileList; FileListPtr != NULL; FileListPtr = FileListPtr->Next) {
    //
    // Parse all the "FV1,FV2..." in the FVs
    //
    if (FileListPtr->FVs != NULL) {
      //
      // Process each fv this file is in
      //
      StartCptr = FileListPtr->FVs;
      while (*StartCptr) {
        EndCptr = StartCptr;
        while (*EndCptr && (*EndCptr != ',')) {
          EndCptr++;
        }

        CSave     = *EndCptr;
        *EndCptr  = 0;
        //
        // Ok, we have a fv name, now see if we've already opened
        // an fv output file of this name.
        //
        for (FVPtr = FVList; FVPtr != NULL; FVPtr = FVPtr->Next) {
          if (_stricmp (FVPtr->FVFileName, StartCptr) == 0) {
            break;
          }
        }
        //
        // If we didn't find one, then create a new one
        //
        if (FVPtr == NULL) {
          //
          // Create a new one, add it to the list
          //
          FVPtr = (FV_LIST *) malloc (sizeof (FV_LIST));
          if (FVPtr == NULL) {
            Error (NULL, 0, 0, NULL, "failed to allocate memory for FV");
            return STATUS_ERROR;
          }

          memset ((char *) FVPtr, 0, sizeof (FV_LIST));
          //
          // Add it to the end of our list
          //
          if (FVList == NULL) {
            FVList = FVPtr;
          } else {
            LastFVList->Next = FVPtr;
          }

          LastFVList = FVPtr;
          //
          // Save the FV name in the FileName pointer so we can compare
          // for any future FV names specified.
          //
          strcpy (FVPtr->FVFileName, StartCptr);

          //
          // Add a symbol for the FV filename
          //
          UpperCaseString (FVPtr->FVFileName);
          AddSymbol (FV_FILENAME, FVPtr->FVFileName, SYM_LOCAL | SYM_OVERWRITE);
          //
          // Now create the FVx.inf filename from the fv name and
          // default filename extension. Dump it in the FV directory
          // as well.
          //
          strcpy (Str, FVDir);
          strcat (Str, FVPtr->FVFileName);
          strcat (Str, ".inf");
          //
          // Create the directory path for our new fv.inf output file.
          //
          MakeFilePath (Str);
          if ((FVPtr->FVFilePtr = SmartOpen (Str)) == NULL) {
            Error (NULL, 0, 0, Str, "could not open FV output file");
            return STATUS_ERROR;
          }
          //
          // Now copy the [fv.$(FV).options] to the fv INF file
          //
          sprintf (Str, "fv.%s.options", StartCptr);
          Section = DSCFileFindSection (DSC, Str);
          if (Section != NULL) {
            SmartWrite (FVPtr->FVFilePtr, "[options]\n");
            while (DSCFileGetLine (DSC, Line, sizeof (Line)) != NULL) {
              ExpandSymbols (Line, ExpandedLine, sizeof (ExpandedLine), 0);
              SmartWrite (FVPtr->FVFilePtr, ExpandedLine);
              GetBaseAddress (ExpandedLine, FVPtr->BaseAddress);
            }
          } else {
            Error (NULL, 0, 0, Str, "could not find FV section in description file");
          }
          //
          // Copy the [fv.$(FV).attributes] to the fv INF file
          //
          sprintf (Str, "fv.%s.attributes", StartCptr);
          Section = DSCFileFindSection (DSC, Str);
          if (Section != NULL) {
            SmartWrite (FVPtr->FVFilePtr, "[attributes]\n");
            while (DSCFileGetLine (DSC, Line, sizeof (Line)) != NULL) {
              ExpandSymbols (Line, ExpandedLine, sizeof (ExpandedLine), 0);
              SmartWrite (FVPtr->FVFilePtr, ExpandedLine);
            }
          } else {
            Error (NULL, 0, 0, Str, "Could not find FV section in description file");
          }
          //
          // Start the files section
          //
          SmartWrite (FVPtr->FVFilePtr, "\n[files]\n");
        }
        //
        // Now write the FV filename to the FV.inf file. Prepend $(PROCESSOR) on
        // it.
        //
        sprintf (ExpandedLine, "EFI_FILE_NAME = %s\n", FileListPtr->FileName);
        SmartWrite (FVPtr->FVFilePtr, ExpandedLine);

        //
        // Next FV on the FV list
        //
        *EndCptr  = CSave;
        StartCptr = EndCptr;
        if (*StartCptr) {
          StartCptr++;
        }
      }
    }
  }
  //
  // Now we walk the list of firmware volumes and create the APRIORI list
  // file for it .
  //
  for (FVPtr = FVList; FVPtr != NULL; FVPtr = FVPtr->Next) {
    //
    // Run through all the files and count up how many are to be
    // added to the apriori list for this FV. Then when we're done
    // we'll make sure we processed them all. We do this in case they
    // skipped an apriori index for a given FV.
    //
    AprioriCount = 0;
    for (FileListPtr = mFileList; FileListPtr != NULL; FileListPtr = FileListPtr->Next) {
      if (OrderInFvList (FileListPtr->Apriori, FVPtr->FVFileName, &AprioriPosition)) {
        //
        // Emit an error if the index was 0, or they didn't give one.
        //
        if (AprioriPosition == 0) {
          Error (
            GetSymbolValue (DSC_FILENAME),
            1,
            0,
            "apriori indexes are 1-based",
            "component %s:APRIORI=%s",
            FileListPtr->BaseName,
            FileListPtr->Apriori
            );
        } else {
          AprioriCount++;
        }

      }
    }
    //
    // Now scan the files as we increment our apriori index
    //
    AprioriCounter = 0;
    do {
      AprioriFound = 0;
      AprioriCounter++;
      for (FileListPtr = mFileList; FileListPtr != NULL; FileListPtr = FileListPtr->Next) {
        //
        // If in the apriori list for this fv, print the name. Open the
        // file first if we have to.
        //
        if ((FileListPtr->Apriori[0] != 0) &&
            (OrderInFvList (FileListPtr->Apriori, FVPtr->FVFileName, &AprioriPosition))
            ) {
          if (AprioriPosition == AprioriCounter) {
            //
            // If we've already found one for this index, emit an error. Decrement the
            // count of how files we are to process so we don't emit another error for
            // a miscount below.
            //
            if (AprioriFound) {
              Error (
                GetSymbolValue (DSC_FILENAME),
                1,
                0,
                "duplicate apriori index found",
                "%s:%d",
                FVPtr->FVFileName,
                AprioriCounter
                );
              AprioriCount--;
            }

            AprioriFound = 1;
            //
            // Open the apriori output file if we haven't already
            //
            if (FVPtr->AprioriFilePtr == NULL) {
              strcpy (Str, FVDir);
              strcat (Str, FVPtr->FVFileName);
              strcat (Str, ".apr");
              if ((FVPtr->AprioriFilePtr = SmartOpen (Str)) == NULL) {
                Error (NULL, 0, 0, Str, "could not open output Apriori file for writing");
                return STATUS_ERROR;
              }
            }
            
            sprintf (ExpandedLine, "%s\n", FileListPtr->BaseFileName);
            SmartWrite (FVPtr->AprioriFilePtr, ExpandedLine);
          }
        }
      }
    } while (AprioriFound);
    //
    // See if they skipped an apriori position for this FV
    //
    if (AprioriCount != (AprioriCounter - 1)) {
      Error (
        GetSymbolValue (DSC_FILENAME),
        1,
        0,
        "apriori index skipped",
        "%s:%d",
        FVPtr->FVFileName,
        AprioriCounter
        );
    }
  }
  //
  // Traverse the list of all files again, and create a macro in the output makefile
  // that defines all the files in each fv. For example, for each FV file, create a line:
  // FV0001_FILES = $(FV_0001_FILES) xxxx-yyy.dxe.
  // This can then be used as a dependency in their makefile.
  // Also if they wanted us to dump a cross-reference, do that now.
  //
  if (mXRefFileName != NULL) {
    if ((XRefFptr = fopen (mXRefFileName, "w")) == NULL) {
      Message (
        0,
        "Failed to open cross-reference file '%s' for writing\n",
        mXRefFileName
        );
    }
  } else {
    XRefFptr = NULL;
  }

  for (FileListPtr = mFileList; FileListPtr != NULL; FileListPtr = FileListPtr->Next) {
    //
    // Parse all the "FV1,FV2..." in the FV field that came from FV=FVa,FVb,... on the
    // component line in the DSC file.
    //
    if (FileListPtr->FVs != NULL) {
      //
      // If generating a cross-reference file, dump the data
      //
      if (XRefFptr != NULL) {
        if ((FileListPtr->Guid != NULL) && (FileListPtr->BaseName != NULL) && (FileListPtr->Processor)) {
          fprintf (
            XRefFptr,
            "%s %s %s\n",
            FileListPtr->Guid,
            FileListPtr->BaseName,
            FileListPtr->Processor
            );
        }
      }
      //
      // Convert to uppercase since we're going to use the name as a macro variable name
      // in the makefile.
      //
      UpperCaseString (FileListPtr->FVs);
      //
      // Process each FV this file is in to write fvxxx_FILES = $(fvxxx_FILES) Guid-BaseName.ffs
      //
      StartCptr = FileListPtr->FVs;
      while (*StartCptr) {
        EndCptr = StartCptr;
        while (*EndCptr && (*EndCptr != ',')) {
          EndCptr++;
        }

        CSave     = *EndCptr;
        *EndCptr  = 0;
        fprintf (
          MakeFptr,
          "%s_FILES = $(%s_FILES) %s\n",
          StartCptr,
          StartCptr,
          FileListPtr->FileName
          );
        //
        // Next FV on the FV list
        //
        *EndCptr  = CSave;
        StartCptr = EndCptr;
        if (*StartCptr) {
          StartCptr++;
        }
      }
    }
  }

  fprintf (MakeFptr, "\n");

  //
  // Now go through the list of all NonFFS FVs they specified and search for
  // a [build.fv.$(FV)] or [build.fv] command and emit the commands to the
  // output makefile. Add them to the "fvs_0" target as well.
  //
  if (mNonFfsFVList != NULL) {
    fprintf (MakeFptr, "fvs_0 ::");
    FVPtr = mNonFfsFVList;
    while (FVPtr != NULL) {
      fprintf (MakeFptr, " %s%s.fv", FVDir, FVPtr->FVFileName);
      FVPtr = FVPtr->Next;
    }

    fprintf (MakeFptr, "\n\n");
    FVPtr = mNonFfsFVList;
    while (FVPtr != NULL) {
      //
      // Save the position in the file
      //
      DSCFileSavePosition (DSC);
      //
      // first try to find a build section specific for this fv.
      //
      sprintf (Str, "build.fv.%s", FVPtr->FVFileName);
      Section = DSCFileFindSection (DSC, Str);
      if (Section == NULL) {
        sprintf (Str, "build.fv");
        Section = DSCFileFindSection (DSC, Str);
      }

      if (Section == NULL) {
        Warning (
          NULL,
          0,
          0,
          NULL,
          "No [build.fv.%s] nor [%s] section found in description file for building %s",
          FVPtr->FVFileName,
          Str,
          FVPtr->FVFileName
          );
      } else {
        //
        // Add a symbol for the FV filename
        //
        UpperCaseString (FVPtr->FVFileName);
        AddSymbol (FV_FILENAME, FVPtr->FVFileName, SYM_LOCAL | SYM_OVERWRITE);
        AddSymbol (EFI_BASE_ADDRESS, FVPtr->BaseAddress, SYM_LOCAL | SYM_OVERWRITE);

        //
        // Now copy the build commands from the section to the makefile
        //
        while (DSCFileGetLine (DSC, Line, sizeof (Line)) != NULL) {
          ExpandSymbols (
            Line,
            ExpandedLine,
            sizeof (ExpandedLine),
            EXPANDMODE_NO_DESTDIR | EXPANDMODE_NO_SOURCEDIR
            );

          fprintf (MakeFptr, ExpandedLine);
        }
      }

      FVPtr = FVPtr->Next;
      DSCFileRestorePosition (DSC);
    }
  }

  //
  // Get the components count
  //
  ComponentCount = -1;
  for (FileListPtr = mFileList; FileListPtr != NULL; FileListPtr = FileListPtr->Next) {
    if (FileListPtr->ComponentsInstance > ComponentCount) {
      ComponentCount = FileListPtr->ComponentsInstance;
    }
  }
  ComponentCount++;

  //
  // Now print firmware volumes build targets fvs_0, fvs_1 etc.
  //
  for (ComponentsInstance = 0; ComponentsInstance < ComponentCount; ComponentsInstance++) {
    fprintf (MakeFptr, "fvs_%d ::", ComponentsInstance);
    for (FVPtr = mFVList; FVPtr != NULL; FVPtr = FVPtr->Next) {
      if (FVPtr->ComponentsInstance == ComponentsInstance) {
        fprintf (MakeFptr, " %s%s.fv", FVDir, FVPtr->FVFileName);
      }
    }
    fprintf (MakeFptr, "\n\n");
  }
      
  //
  // Create an "fvs" target that builds everything. It has to be a mix of 
  // components and FV's in order. For example:
  // fvs :: components_0 fvs_0 components_1 fvs_1
  //
  fprintf (MakeFptr, "fvs ::");
  for (ComponentsInstance = 0; ComponentsInstance < ComponentCount; ComponentsInstance++) {
    fprintf (MakeFptr, " components_%d fvs_%d", ComponentsInstance, ComponentsInstance);
  }
  fprintf (MakeFptr, "\n\n");

  //
  // Create a "components" target for build convenience. It should
  // look something like:
  // components : components_0 components_1...
  //
  if (ComponentCount > 0) {
    fprintf (MakeFptr, "components :");
    for (ComponentsInstance = 0; ComponentsInstance < ComponentCount; ComponentsInstance++) {
      fprintf (MakeFptr, " components_%d", ComponentsInstance);
    }

    fprintf (MakeFptr, "\n\n");
  }
  //
  // Now go through the list of all FV's defined and search for
  // a [build.fv.$(FV)] or [build.fv] command and emit the commands to the
  // output makefile.
  //
  FVPtr = mFVList;
  while (FVPtr != NULL) {
    if (FVPtr->FVFileName[0]) {
      //
      // Save the position in the file
      //
      DSCFileSavePosition (DSC);
      //
      // First try to find a build section specific for this FV.
      //
      sprintf (Str, "build.fv.%s", FVPtr->FVFileName);
      Section = DSCFileFindSection (DSC, Str);
      if (Section == NULL) {
        sprintf (Str, "build.fv");
        Section = DSCFileFindSection (DSC, Str);
      }

      if (Section == NULL) {
        Error (
          NULL,
          0,
          0,
          NULL,
          "no [build.fv.%s] nor [%s] section found in description file for building %s",
          FVPtr->FVFileName,
          Str,
          FVPtr->FVFileName
          );
      } else {
        //
        // Add a symbol for the FV filename
        //
        UpperCaseString (FVPtr->FVFileName);
        AddSymbol (FV_FILENAME, FVPtr->FVFileName, SYM_LOCAL | SYM_OVERWRITE);
        AddSymbol (EFI_BASE_ADDRESS, FVPtr->BaseAddress, SYM_LOCAL | SYM_OVERWRITE);

        //
        // Now copy the build commands from the section to the makefile
        //
        while (DSCFileGetLine (DSC, Line, sizeof (Line)) != NULL) {
          ExpandSymbols (
            Line,
            ExpandedLine,
            sizeof (ExpandedLine),
            EXPANDMODE_NO_DESTDIR | EXPANDMODE_NO_SOURCEDIR
            );
          fprintf (MakeFptr, ExpandedLine);
        }
      }

      DSCFileRestorePosition (DSC);
    }

    FVPtr = FVPtr->Next;
  }
  //
  // Close all the files and free up the memory
  //
  while (FVList != NULL) {
    FVPtr = FVList->Next;
    if (FVList->FVFilePtr != NULL) {
      SmartClose (FVList->FVFilePtr);
    }

    if (FVList->AprioriFilePtr != NULL) {
      SmartClose (FVList->AprioriFilePtr);
    }

    free (FVList);
    FVList = FVPtr;
  }

  while (mNonFfsFVList != NULL) {
    FVPtr = mNonFfsFVList->Next;
    free (mNonFfsFVList);
    mNonFfsFVList = FVPtr;
  }

  if (XRefFptr != NULL) {
    fclose (XRefFptr);
  }

  return STATUS_SUCCESS;
}

int
NonFFSFVWriteInfFiles (
  DSC_FILE  *DSC,
  char      *FileName
  )
/*++

Routine Description:

  Generate a Non FFS fv file. It can only some variables,
  or simply contains nothing except header.

Arguments:

  DSC       - pointer to a DSC_FILE object to extract info from
  FileName  - pointer to the fv file

Returns:

  STATUS_SUCCESS  if successful
  non-STATUS_SUCCESS  otherwise

--*/
{
  FV_LIST *FVPtr;
  SECTION *Section;
  char    *StartCptr;
  char    *EndCptr;
  char    CSave;
  char    Str[MAX_PATH];
  char    Line[MAX_LINE_LEN];
  char    ExpandedLine[MAX_LINE_LEN];
  char    FVDir[MAX_PATH];

  //
  // See if they specified a FV directory to dump the FV files out to. If not,
  // then use the default. Then create the output directory.
  //
  DSCFileSavePosition (DSC);
  StartCptr = GetSymbolValue (FV_INF_DIR);
  if (StartCptr == NULL) {
    ExpandSymbols (DEFAULT_FV_INF_DIR, FVDir, sizeof (FVDir), EXPANDMODE_NO_UNDEFS);
  } else {
    strcpy (FVDir, StartCptr);
  }

  //
  // Make sure the fv directory path ends in /
  //
  CSave = FVDir[strlen (FVDir) - 1];
  if ((CSave != '\\') && (CSave != '/')) {
    strcat (FVDir, "\\");
  }

  StartCptr = FileName;
  while (*StartCptr) {
    EndCptr = StartCptr;
    while (*EndCptr && (*EndCptr != ',')) {
      EndCptr++;
    }

    CSave     = *EndCptr;
    *EndCptr  = 0;
    //
    // Ok, we have a fv name, now see if we've already opened
    // an fv output file of this name.
    //
    for (FVPtr = mNonFfsFVList; FVPtr != NULL; FVPtr = FVPtr->Next) {
      if (_stricmp (FVPtr->FVFileName, StartCptr) == 0) {
        break;
      }
    }
    //
    // If there is already one with the same name, wrong
    //
    if (FVPtr != NULL) {
      DSCFileRestorePosition (DSC);
      return STATUS_ERROR;
    }
    //
    // Create a new one, add it to the list
    //
    FVPtr = (FV_LIST *) malloc (sizeof (FV_LIST));
    if (FVPtr == NULL) {
      Error (__FILE__, __LINE__, 0, "failed to allocate memory", NULL);
      DSCFileRestorePosition (DSC);
      return STATUS_ERROR;
    }

    memset ((char *) FVPtr, 0, sizeof (FV_LIST));
    FVPtr->Next   = mNonFfsFVList;
    mNonFfsFVList = FVPtr;
    //
    // Save the FV name in the FileName pointer so we can compare
    // for any future FV names specified.
    //
    strcpy (FVPtr->FVFileName, StartCptr);
    //
    // Add a symbol for the FV filename
    //
    UpperCaseString (FVPtr->FVFileName);
    AddSymbol (FV_FILENAME, FVPtr->FVFileName, SYM_LOCAL | SYM_OVERWRITE);

    //
    // Now create the FVx.inf filename from the fv name and
    // default filename extension. Dump it in the FV directory
    // as well.
    //
    strcpy (Str, FVDir);
    strcat (Str, FVPtr->FVFileName);
    strcat (Str, ".inf");
    //
    // Create the directory path for our new fv.inf output file.
    //
    MakeFilePath (Str);
    if ((FVPtr->FVFilePtr = SmartOpen (Str)) == NULL) {
      Error (NULL, 0, 0, Str, "could not open FV output file");
      DSCFileRestorePosition (DSC);
      return STATUS_ERROR;
    }
    //
    // Now copy the [fv.fvfile.options] to the fv file
    //
    sprintf (Str, "fv.%s.options", StartCptr);
    Section = DSCFileFindSection (DSC, Str);
    if (Section != NULL) {
      SmartWrite (FVPtr->FVFilePtr, "[options]\n");
      while (DSCFileGetLine (DSC, Line, sizeof (Line)) != NULL) {
        ExpandSymbols (Line, ExpandedLine, sizeof (ExpandedLine), 0);
        SmartWrite (FVPtr->FVFilePtr, ExpandedLine);
        GetBaseAddress (ExpandedLine, FVPtr->BaseAddress);
      }
    } else {
      Warning (NULL, 0, 0, NULL, "Could not find FV section '%s' in description file", Str);
    }
    //
    // Copy the [fv.fvfile.attributes] to the fv file
    //
    sprintf (Str, "fv.%s.attributes", StartCptr);
    Section = DSCFileFindSection (DSC, Str);
    if (Section != NULL) {
      SmartWrite (FVPtr->FVFilePtr, "[attributes]\n");
      while (DSCFileGetLine (DSC, Line, sizeof (Line)) != NULL) {
        ExpandSymbols (Line, ExpandedLine, sizeof (ExpandedLine), 0);
        SmartWrite (FVPtr->FVFilePtr, ExpandedLine);
      }
    } else {
      Warning (NULL, 0, 0, NULL, "Could not find FV section '%s' in description file", Str);
    }
    //
    // Copy the [fv.fvfile.components] to the fv file
    //
    sprintf (Str, "fv.%s.components", StartCptr);
    Section = DSCFileFindSection (DSC, Str);
    if (Section != NULL) {
      SmartWrite (FVPtr->FVFilePtr, "[components]\n");
      while (DSCFileGetLine (DSC, Line, sizeof (Line)) != NULL) {
        ExpandSymbols (Line, ExpandedLine, sizeof (ExpandedLine), 0);
        SmartWrite (FVPtr->FVFilePtr, ExpandedLine);
      }
    } else {
      //
      // An empty FV is allowed to contain nothing
      //
    }
    //
    // Close the file
    //
    SmartClose (FVPtr->FVFilePtr);
    //
    // Next FV in FileName
    //
    *EndCptr  = CSave;
    StartCptr = EndCptr;
    if (*StartCptr) {
      StartCptr++;
    }
  }

  DSCFileRestorePosition (DSC);
  return STATUS_SUCCESS;
}

static
void
AddFirmwareVolumes (
  char          *FVs,
  int           ComponentsInstance
  )
{
  FV_LIST *FvPtr;
  char    *StartPtr;
  char    *EndPtr;
  char    SaveChar;

  if ((FVs != NULL) && (FVs[0] != 0)) {
    //
    // Extract each FV name from the string. It's from the DSC file "FV=FvRecover,FvMain"
    //
    StartPtr = FVs;
    while (*StartPtr != 0) {
      EndPtr = StartPtr;
      while (*EndPtr && (*EndPtr != ',')) {
        EndPtr++;
      }

      SaveChar  = *EndPtr;
      *EndPtr   = 0;
      //
      // Look through our list of known firmware volumes and see if we've
      // already added it.
      //
      for (FvPtr = mFVList; FvPtr != NULL; FvPtr = FvPtr->Next) {
        if (_stricmp (FvPtr->FVFileName, StartPtr) == 0) {
          break;
        }
      }
      //
      // If we didn't find a match, then create a new one
      //
      if (FvPtr == NULL) {
        FvPtr = malloc (sizeof (FV_LIST));
        if (FvPtr == NULL) {
          Error (__FILE__, __LINE__, 0, "application error", "memory allocation failed");
          return ;
        }

        memset (FvPtr, 0, sizeof (FV_LIST));
        strcpy (FvPtr->FVFileName, StartPtr);
        if (mFVList == NULL) {
          mFVList = FvPtr;
        } else {
          mFVListLast->Next = FvPtr;
        }

        mFVListLast = FvPtr;
      }
      //
      // If this component's section number is higher than that of this
      // FV, then set the FV's to it.
      //
      if (FvPtr->ComponentsInstance < ComponentsInstance) {
        FvPtr->ComponentsInstance = ComponentsInstance;
      }
      //
      // If we found then end of the FVs in the string, then we're done.
      // Always restore the original string's contents.
      //
      if (SaveChar != 0) {
        *EndPtr   = SaveChar;
        StartPtr  = EndPtr + 1;
      } else {
        StartPtr = EndPtr;
      }
    }
  }
}

static
BOOLEAN
OrderInFvList (
  char    *FvList,
  char    *FvName,
  int     *Order
  )
{
  //
  // Given FvList of format "FV_a,FV_b,FV_c" or "FV_a:1,FV_b:2" and
  // FvName of format "FV_c", determine if FvName is in FvList. If
  // FV_a:1 format, then return the value after the colon.
  //
  while (*FvList) {
    //
    // If it matches for the length of FvName...
    //
    if (_strnicmp (FvList, FvName, strlen (FvName)) == 0) {
      //
      // Then see if the match string in FvList is terminated at the
      // same length.
      //
      if ((FvList[strlen (FvName)] == ',') || (FvList[strlen (FvName)] == 0)) {
        *Order = 0;
        return TRUE;
      } else if (FvList[strlen (FvName)] == ':') {
        *Order = atoi (FvList + strlen (FvName) + 1);
        return TRUE;
      }
    }
    //
    // Skip to next FV in the comma-separated list
    //
    while ((*FvList != ',') && (*FvList != 0)) {
      FvList++;
    }
    //
    // Skip over comma
    //
    if (*FvList == ',') {
      FvList++;
    }
  }

  return FALSE;
}

static
char *
UpperCaseString (
  char *Str
  )
{
  char  *Cptr;

  for (Cptr = Str; *Cptr; Cptr++) {
    *Cptr = (char) toupper (*Cptr);
  }

  return Str;
}

static
BOOLEAN
InSameFv (
  char  *FVs1,
  char  *FVs2
)
{
  char    *StartCptr1;
  char    *StartCptr2;
  char    *EndCptr1;
  char    *EndCptr2;
  char    CSave1;
  char    CSave2;
  
  //
  // Process each FV in first FV list
  //
  StartCptr1 = FVs1;
  while (*StartCptr1) {
    EndCptr1 = StartCptr1;
    while (*EndCptr1 && (*EndCptr1 != ',')) {
      EndCptr1++;
    }

    CSave1     = *EndCptr1;
    *EndCptr1  = 0;  
    
    if (*StartCptr1) {
      //
      // Process each FV in second FV list
      //
      StartCptr2 = FVs2;
      while (*StartCptr2) {
        EndCptr2 = StartCptr2;
        while (*EndCptr2 && (*EndCptr2 != ',')) {
          EndCptr2++;
        }
    
        CSave2     = *EndCptr2;
        *EndCptr2  = 0;  
        
        if (_stricmp (StartCptr1, StartCptr2) == 0) {
          *EndCptr1  = CSave1;
          *EndCptr2  = CSave2;
          return TRUE;
        }
        
        //
        // Next FV on the second FV list
        //
        *EndCptr2  = CSave2;
        StartCptr2 = EndCptr2;
        if (*StartCptr2) {
          StartCptr2++;
        }
      }    
    }
    
    //
    // Next FV on the first FV list
    //
    *EndCptr1  = CSave1;
    StartCptr1 = EndCptr1;
    if (*StartCptr1) {
      StartCptr1++;
    }
  }        
  
  return FALSE;
}

int
CFVSetXRefFileName (
  char    *FileName
  )
{
  mXRefFileName = FileName;
  return 0;
}
