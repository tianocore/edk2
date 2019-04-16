/** @file

  VfrCompiler main class and main function.

Copyright (c) 2004 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "VfrCompiler.h"
#include "CommonLib.h"
#include "EfiUtilityMsgs.h"

PACKAGE_DATA  gCBuffer;
PACKAGE_DATA  gRBuffer;
CVfrStringDB  gCVfrStringDB;

VOID
CVfrCompiler::DebugError (
  IN CHAR8         *FileName,
  IN UINT32        LineNumber,
  IN UINT32        MessageCode,
  IN CONST CHAR8   *Text,
  IN CONST CHAR8   *MsgFmt,
  ...
  )
{
  va_list List;
  va_start (List, MsgFmt);
  PrintMessage ((CHAR8 *) "ERROR", FileName, LineNumber, MessageCode, (CHAR8 *) Text, (CHAR8 *) MsgFmt, List);
  va_end (List);
}

VOID
CVfrCompiler::SET_RUN_STATUS (
  IN COMPILER_RUN_STATUS Status
  )
{
  mRunStatus = Status;
}

BOOLEAN
CVfrCompiler::IS_RUN_STATUS (
  IN COMPILER_RUN_STATUS Status
  )
{
  return mRunStatus == Status;
}

VOID
CVfrCompiler::OptionInitialization (
  IN INT32      Argc,
  IN CHAR8      **Argv
  )
{
  INT32         Index;
  EFI_STATUS    Status;

  Status = EFI_SUCCESS;
  SetUtilityName ((CHAR8*) PROGRAM_NAME);

  mOptions.VfrFileName                   = NULL;
  mOptions.RecordListFile                = NULL;
  mOptions.CreateRecordListFile          = FALSE;
  mOptions.CreateIfrPkgFile              = FALSE;
  mOptions.PkgOutputFileName             = NULL;
  mOptions.COutputFileName               = NULL;
  mOptions.OutputDirectory               = NULL;
  mOptions.PreprocessorOutputFileName    = NULL;
  mOptions.VfrBaseFileName               = NULL;
  mOptions.IncludePaths                  = NULL;
  mOptions.SkipCPreprocessor             = TRUE;
  mOptions.CPreprocessorOptions          = NULL;
  mOptions.HasOverrideClassGuid          = FALSE;
  mOptions.WarningAsError                = FALSE;
  mOptions.AutoDefault                   = FALSE;
  mOptions.CheckDefault                  = FALSE;
  memset (&mOptions.OverrideClassGuid, 0, sizeof (EFI_GUID));

  if (Argc == 1) {
    Usage ();
    SET_RUN_STATUS (STATUS_DEAD);
    return;
  }

  for (Index = 1; (Index < Argc) && (Argv[Index][0] == '-'); Index++) {
    if ((stricmp(Argv[Index], "-h") == 0) || (stricmp(Argv[Index], "--help") == 0)) {
      Usage ();
      SET_RUN_STATUS (STATUS_DEAD);
      return;
    } else if (stricmp(Argv[Index], "--version") == 0) {
      Version ();
      SET_RUN_STATUS (STATUS_DEAD);
      return;
    } else if (stricmp(Argv[Index], "-l") == 0) {
      mOptions.CreateRecordListFile = TRUE;
      gCIfrRecordInfoDB.TurnOn ();
    } else if (stricmp(Argv[Index], "-i") == 0) {
      Index++;
      if ((Index >= Argc) || (Argv[Index][0] == '-')) {
        DebugError (NULL, 0, 1001, "Missing option", "-i missing path argument");
        goto Fail;
      }

      AppendIncludePath(Argv[Index]);
    } else if (stricmp(Argv[Index], "-o") == 0 || stricmp(Argv[Index], "--output-directory") == 0 || stricmp(Argv[Index], "-od") == 0) {
      Index++;
      if ((Index >= Argc) || (Argv[Index][0] == '-')) {
        DebugError (NULL, 0, 1001, "Missing option", "-o missing output directory name");
        goto Fail;
      }

      mOptions.OutputDirectory = (CHAR8 *) malloc (strlen (Argv[Index]) + strlen ("\\") + 1);
      if (mOptions.OutputDirectory == NULL) {
        DebugError (NULL, 0, 4001, "Resource: memory can't be allocated", NULL);
        goto Fail;
      }
      strcpy (mOptions.OutputDirectory, Argv[Index]);

      CHAR8 lastChar = mOptions.OutputDirectory[strlen(mOptions.OutputDirectory) - 1];
      if ((lastChar != '/') && (lastChar != '\\')) {
        if (strchr(mOptions.OutputDirectory, '/') != NULL) {
          strcat (mOptions.OutputDirectory, "/");
        } else {
          strcat (mOptions.OutputDirectory, "\\");
        }
      }
      DebugMsg (NULL, 0, 9, (CHAR8 *) "Output Directory", (CHAR8 *) "%s", mOptions.OutputDirectory);
    } else if (stricmp(Argv[Index], "-b") == 0 || stricmp(Argv[Index], "--create-ifr-package") == 0 || stricmp(Argv[Index], "-ibin") == 0) {
      mOptions.CreateIfrPkgFile = TRUE;
    } else if (stricmp(Argv[Index], "-n") == 0 || stricmp(Argv[Index], "--no-pre-processing") == 0 || stricmp(Argv[Index], "-nopp") == 0) {
      mOptions.SkipCPreprocessor = TRUE;
    } else if (stricmp(Argv[Index], "-f") == 0 || stricmp(Argv[Index], "--pre-processing-flag") == 0 || stricmp(Argv[Index], "-ppflag") == 0) {
      Index++;
      if ((Index >= Argc) || (Argv[Index][0] == '-')) {
        DebugError (NULL, 0, 1001, "Missing option", "-od - missing C-preprocessor argument");
        goto Fail;
      }

      AppendCPreprocessorOptions (Argv[Index]);
    } else if (stricmp(Argv[Index], "-s") == 0|| stricmp(Argv[Index], "--string-db") == 0) {
      Index++;
      if ((Index >= Argc) || (Argv[Index][0] == '-')) {
        DebugError (NULL, 0, 1001, "Missing option", "-s missing input string file name");
        goto Fail;
      }
      gCVfrStringDB.SetStringFileName(Argv[Index]);
      DebugMsg (NULL, 0, 9, (CHAR8 *) "Input string file path", (CHAR8 *) "%s", Argv[Index]);
    } else if ((stricmp (Argv[Index], "-g") == 0) || (stricmp (Argv[Index], "--guid") == 0)) {
      Index++;
      Status = StringToGuid (Argv[Index], &mOptions.OverrideClassGuid);
      if (EFI_ERROR (Status)) {
        DebugError (NULL, 0, 1000, "Invalid format:", "%s", Argv[Index]);
        goto Fail;
      }
      mOptions.HasOverrideClassGuid = TRUE;
    } else if (stricmp(Argv[Index], "-w") == 0 || stricmp(Argv[Index], "--warning-as-error") == 0) {
      mOptions.WarningAsError = TRUE;
    } else if (stricmp(Argv[Index], "-a") == 0 ||stricmp(Argv[Index], "--autodefault") == 0) {
      mOptions.AutoDefault = TRUE;
    } else if (stricmp(Argv[Index], "-d") == 0 ||stricmp(Argv[Index], "--checkdefault") == 0) {
      mOptions.CheckDefault = TRUE;
    } else {
      DebugError (NULL, 0, 1000, "Unknown option", "unrecognized option %s", Argv[Index]);
      goto Fail;
    }
  }

  if (Index != Argc - 1) {
    DebugError (NULL, 0, 1001, "Missing option", "VFR file name is not specified.");
    goto Fail;
  } else {
    mOptions.VfrFileName = (CHAR8 *) malloc (strlen (Argv[Index]) + 1);
    if (mOptions.VfrFileName == NULL) {
      DebugError (NULL, 0, 4001, "Resource: memory can't be allocated", NULL);
      goto Fail;
    }
    strcpy (mOptions.VfrFileName, Argv[Index]);

    if (mOptions.OutputDirectory == NULL) {
      mOptions.OutputDirectory = (CHAR8 *) malloc (1);
      if (mOptions.OutputDirectory == NULL) {
        DebugError (NULL, 0, 4001, "Resource: memory can't be allocated", NULL);
        goto Fail;
      }
      mOptions.OutputDirectory[0] = '\0';
    }
  }

  if (SetBaseFileName() != 0) {
    goto Fail;
  }
  if (SetPkgOutputFileName () != 0) {
    goto Fail;
  }
  if (SetCOutputFileName() != 0) {
    goto Fail;
  }
  if (SetPreprocessorOutputFileName () != 0) {
    goto Fail;
  }
  if (SetRecordListFileName () != 0) {
    goto Fail;
  }
  return;

Fail:
  SET_RUN_STATUS (STATUS_DEAD);

  mOptions.CreateRecordListFile          = FALSE;
  mOptions.CreateIfrPkgFile              = FALSE;

  if (mOptions.VfrFileName != NULL) {
    free (mOptions.VfrFileName);
    mOptions.VfrFileName                 = NULL;
  }
  if (mOptions.VfrBaseFileName != NULL) {
    free (mOptions.VfrBaseFileName);
    mOptions.VfrBaseFileName             = NULL;
  }
  if (mOptions.OutputDirectory != NULL) {
    free (mOptions.OutputDirectory);
    mOptions.OutputDirectory             = NULL;
  }
  if (mOptions.PkgOutputFileName != NULL) {
    free (mOptions.PkgOutputFileName);
    mOptions.PkgOutputFileName           = NULL;
  }
  if (mOptions.COutputFileName != NULL) {
    free (mOptions.COutputFileName);
    mOptions.COutputFileName             = NULL;
  }
  if (mOptions.PreprocessorOutputFileName != NULL) {
    free (mOptions.PreprocessorOutputFileName);
    mOptions.PreprocessorOutputFileName  = NULL;
  }
  if (mOptions.RecordListFile != NULL) {
    free (mOptions.RecordListFile);
    mOptions.RecordListFile              = NULL;
  }
  if (mOptions.IncludePaths != NULL) {
    delete mOptions.IncludePaths;
    mOptions.IncludePaths                = NULL;
  }
  if (mOptions.CPreprocessorOptions != NULL) {
    delete mOptions.CPreprocessorOptions;
    mOptions.CPreprocessorOptions        = NULL;
  }
}

VOID
CVfrCompiler::AppendIncludePath (
  IN CHAR8      *PathStr
  )
{
  UINT32  Len           = 0;
  CHAR8   *IncludePaths = NULL;

  Len = strlen (" -I ") + strlen (PathStr) + 1;
  if (mOptions.IncludePaths != NULL) {
    Len += strlen (mOptions.IncludePaths);
  }
  IncludePaths = new CHAR8[Len];
  if (IncludePaths == NULL) {
    DebugError (NULL, 0, 4001, "Resource: memory can't be allocated", NULL);
    return;
  }
  IncludePaths[0] = '\0';
  if (mOptions.IncludePaths != NULL) {
    strcat (IncludePaths, mOptions.IncludePaths);
  }
  strcat (IncludePaths, " -I ");
  strcat (IncludePaths, PathStr);
  if (mOptions.IncludePaths != NULL) {
    delete[] mOptions.IncludePaths;
  }
  mOptions.IncludePaths = IncludePaths;
}

VOID
CVfrCompiler::AppendCPreprocessorOptions (
  IN CHAR8      *Options
  )
{
  UINT32  Len           = 0;
  CHAR8   *Opt          = NULL;

  Len = strlen (Options) + strlen (" ") + 1;
  if (mOptions.CPreprocessorOptions != NULL) {
    Len += strlen (mOptions.CPreprocessorOptions);
  }
  Opt = new CHAR8[Len];
  if (Opt == NULL) {
    DebugError (NULL, 0, 4001, "Resource: memory can't be allocated", NULL);
    return;
  }
  Opt[0] = 0;
  if (mOptions.CPreprocessorOptions != NULL) {
    strcat (Opt, mOptions.CPreprocessorOptions);
  }
  strcat (Opt, " ");
  strcat (Opt, Options);
  if (mOptions.CPreprocessorOptions != NULL) {
    delete[] mOptions.CPreprocessorOptions;
  }
  mOptions.CPreprocessorOptions = Opt;
}

INT8
CVfrCompiler::SetBaseFileName (
  VOID
  )
{
  CHAR8         *pFileName, *pPath, *pExt;

  if (mOptions.VfrFileName == NULL) {
    return -1;
  }

  pFileName = mOptions.VfrFileName;
  while (
    ((pPath = strchr (pFileName, '\\')) != NULL) ||
    ((pPath = strchr (pFileName, '/')) != NULL)
    )
  {
    pFileName = pPath + 1;
  }

  if (pFileName == NULL) {
    return -1;
  }

  if ((pExt = strchr (pFileName, '.')) == NULL) {
    return -1;
  }

  *pExt = '\0';

  mOptions.VfrBaseFileName = (CHAR8 *) malloc (strlen (pFileName) + 1);
  if (mOptions.VfrBaseFileName == NULL) {
    *pExt = '.';
    return -1;
  }

  strcpy (mOptions.VfrBaseFileName, pFileName);
  *pExt = '.';

  return 0;
}

INT8
CVfrCompiler::SetPkgOutputFileName (
  VOID
  )
{
  INTN Length;

  if (mOptions.VfrBaseFileName == NULL) {
    return -1;
  }

  Length = strlen (mOptions.OutputDirectory) +
           strlen (mOptions.VfrBaseFileName) +
           strlen (VFR_PACKAGE_FILENAME_EXTENSION) +
           1;

  mOptions.PkgOutputFileName = (CHAR8 *) malloc (Length);
  if (mOptions.PkgOutputFileName == NULL) {
    return -1;
  }

  strcpy (mOptions.PkgOutputFileName, mOptions.OutputDirectory);
  strcat (mOptions.PkgOutputFileName, mOptions.VfrBaseFileName);
  strcat (mOptions.PkgOutputFileName, VFR_PACKAGE_FILENAME_EXTENSION);

  return 0;
}

INT8
CVfrCompiler::SetCOutputFileName (
  VOID
  )
{
  INTN Length;

  if (mOptions.VfrBaseFileName == NULL) {
    return -1;
  }

  Length = strlen (mOptions.OutputDirectory) +
           strlen (mOptions.VfrBaseFileName) +
           strlen (".c") +
           1;

  mOptions.COutputFileName = (CHAR8 *) malloc (Length);
  if (mOptions.COutputFileName == NULL) {
    return -1;
  }

  strcpy (mOptions.COutputFileName, mOptions.OutputDirectory);
  strcat (mOptions.COutputFileName, mOptions.VfrBaseFileName);
  strcat (mOptions.COutputFileName, ".c");

  return 0;
}

INT8
CVfrCompiler::SetPreprocessorOutputFileName (
  VOID
  )
{
  INTN Length;

  if (mOptions.VfrBaseFileName == NULL) {
    return -1;
  }

  Length = strlen (mOptions.OutputDirectory) +
           strlen (mOptions.VfrBaseFileName) +
           strlen (VFR_PREPROCESS_FILENAME_EXTENSION) +
           1;

  mOptions.PreprocessorOutputFileName = (CHAR8 *) malloc (Length);
  if (mOptions.PreprocessorOutputFileName == NULL) {
    return -1;
  }

  strcpy (mOptions.PreprocessorOutputFileName, mOptions.OutputDirectory);
  strcat (mOptions.PreprocessorOutputFileName, mOptions.VfrBaseFileName);
  strcat (mOptions.PreprocessorOutputFileName, VFR_PREPROCESS_FILENAME_EXTENSION);

  return 0;
}

INT8
CVfrCompiler::SetRecordListFileName (
  VOID
  )
{
  INTN Length;

  if (mOptions.VfrBaseFileName == NULL) {
    return -1;
  }

  Length = strlen (mOptions.OutputDirectory) +
           strlen (mOptions.VfrBaseFileName) +
           strlen (VFR_RECORDLIST_FILENAME_EXTENSION) +
           1;

  mOptions.RecordListFile = (CHAR8 *) malloc (Length);
  if (mOptions.RecordListFile == NULL) {
    return -1;
  }

  strcpy (mOptions.RecordListFile, mOptions.OutputDirectory);
  strcat (mOptions.RecordListFile, mOptions.VfrBaseFileName);
  strcat (mOptions.RecordListFile, VFR_RECORDLIST_FILENAME_EXTENSION);

  return 0;
}

CVfrCompiler::CVfrCompiler (
  IN INT32      Argc,
  IN CHAR8      **Argv
  )
{
  mPreProcessCmd = (CHAR8 *) PREPROCESSOR_COMMAND;
  mPreProcessOpt = (CHAR8 *) PREPROCESSOR_OPTIONS;

  SET_RUN_STATUS (STATUS_STARTED);

  OptionInitialization(Argc, Argv);

  if ((IS_RUN_STATUS(STATUS_FAILED)) || (IS_RUN_STATUS(STATUS_DEAD))) {
    return;
  }

  SET_RUN_STATUS(STATUS_INITIALIZED);
}

CVfrCompiler::~CVfrCompiler (
  VOID
  )
{
  if (mOptions.VfrFileName != NULL) {
    free (mOptions.VfrFileName);
    mOptions.VfrFileName = NULL;
  }

  if (mOptions.VfrBaseFileName != NULL) {
    free (mOptions.VfrBaseFileName);
    mOptions.VfrBaseFileName = NULL;
  }

  if (mOptions.OutputDirectory != NULL) {
    free (mOptions.OutputDirectory);
    mOptions.OutputDirectory = NULL;
  }

  if (mOptions.PkgOutputFileName != NULL) {
    free (mOptions.PkgOutputFileName);
    mOptions.PkgOutputFileName = NULL;
  }

  if (mOptions.COutputFileName != NULL) {
    free (mOptions.COutputFileName);
    mOptions.COutputFileName = NULL;
  }

  if (mOptions.PreprocessorOutputFileName != NULL) {
    free (mOptions.PreprocessorOutputFileName);
    mOptions.PreprocessorOutputFileName = NULL;
  }

  if (mOptions.RecordListFile != NULL) {
    free (mOptions.RecordListFile);
    mOptions.RecordListFile = NULL;
  }

  if (mOptions.IncludePaths != NULL) {
    delete[] mOptions.IncludePaths;
    mOptions.IncludePaths = NULL;
  }

  if (mOptions.CPreprocessorOptions != NULL) {
    delete[] mOptions.CPreprocessorOptions;
    mOptions.CPreprocessorOptions = NULL;
  }

  SET_RUN_STATUS(STATUS_DEAD);
}

VOID
CVfrCompiler::Usage (
  VOID
  )
{
  UINT32 Index;
  CONST  CHAR8 *Help[] = {
    " ",
    "VfrCompile version " VFR_COMPILER_VERSION "Build " __BUILD_VERSION,
    "Copyright (c) 2004-2016 Intel Corporation. All rights reserved.",
    " ",
    "Usage: VfrCompile [options] VfrFile",
    " ",
    "Options:",
    "  -h, --help     prints this help",
    "  --version      prints version info",
    "  -l             create an output IFR listing file",
    "  -o DIR, --output-directory DIR",
    "                 deposit all output files to directory OutputDir",
    "                 default is current directory",
    "  -b, --create-ifr-package",
    "                 create an IFR HII pack file",
    "  -n, --no-pre-processing",
    "                 do not preprocessing input file",
    "  -s, --string-db",
    "                 input uni string package file",
    "  -g, --guid",
    "                 override class guid input",
    "                 format is xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
    "  -w  --warning-as-error",
    "                 treat warning as an error",
    "  -a  --autodefaut    generate default value for question opcode if some default is missing",
    "  -d  --checkdefault  check the default information in a question opcode",
    NULL
    };
  for (Index = 0; Help[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Help[Index]);
  }
}

VOID
CVfrCompiler::Version (
  VOID
  )
{
  UINT32 Index;
  CONST  CHAR8 *Help[] = {
    "VfrCompile version " VFR_COMPILER_VERSION "Build " __BUILD_VERSION,
    NULL
    };
  for (Index = 0; Help[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Help[Index]);
  }
}

VOID
CVfrCompiler::PreProcess (
  VOID
  )
{
  FILE    *pVfrFile      = NULL;
  UINT32  CmdLen         = 0;
  CHAR8   *PreProcessCmd = NULL;

  if (!IS_RUN_STATUS(STATUS_INITIALIZED)) {
    goto Fail;
  }

  if (mOptions.SkipCPreprocessor == TRUE) {
    goto Out;
  }

  if ((pVfrFile = fopen (LongFilePath (mOptions.VfrFileName), "r")) == NULL) {
    DebugError (NULL, 0, 0001, "Error opening the input VFR file", "%s", mOptions.VfrFileName);
    goto Fail;
  }
  fclose (pVfrFile);

  CmdLen = strlen (mPreProcessCmd) + strlen (mPreProcessOpt) +
           strlen (mOptions.VfrFileName) + strlen (mOptions.PreprocessorOutputFileName);
  if (mOptions.CPreprocessorOptions != NULL) {
    CmdLen += strlen (mOptions.CPreprocessorOptions);
  }
  if (mOptions.IncludePaths != NULL) {
    CmdLen += strlen (mOptions.IncludePaths);
  }

  PreProcessCmd = new CHAR8[CmdLen + 10];
  if (PreProcessCmd == NULL) {
    DebugError (NULL, 0, 4001, "Resource: memory can't be allocated", NULL);
    goto Fail;
  }
  strcpy (PreProcessCmd, mPreProcessCmd), strcat (PreProcessCmd, " ");
  strcat (PreProcessCmd, mPreProcessOpt), strcat (PreProcessCmd, " ");
  if (mOptions.IncludePaths != NULL) {
    strcat (PreProcessCmd, mOptions.IncludePaths), strcat (PreProcessCmd, " ");
  }
  if (mOptions.CPreprocessorOptions != NULL) {
    strcat (PreProcessCmd, mOptions.CPreprocessorOptions), strcat (PreProcessCmd, " ");
  }
  strcat (PreProcessCmd, mOptions.VfrFileName), strcat (PreProcessCmd, " > ");
  strcat (PreProcessCmd, mOptions.PreprocessorOutputFileName);

  if (system (PreProcessCmd) != 0) {
    DebugError (NULL, 0, 0003, "Error parsing file", "failed to spawn C preprocessor on VFR file %s\n", PreProcessCmd);
    goto Fail;
  }

  delete[] PreProcessCmd;

Out:
  SET_RUN_STATUS (STATUS_PREPROCESSED);
  return;

Fail:
  if (!IS_RUN_STATUS(STATUS_DEAD)) {
    SET_RUN_STATUS (STATUS_FAILED);
  }
  delete[] PreProcessCmd;
}

extern UINT8 VfrParserStart (IN FILE *, IN INPUT_INFO_TO_SYNTAX *);

VOID
CVfrCompiler::Compile (
  VOID
  )
{
  FILE  *pInFile    = NULL;
  CHAR8 *InFileName = NULL;
  INPUT_INFO_TO_SYNTAX InputInfo;

  if (!IS_RUN_STATUS(STATUS_PREPROCESSED)) {
    goto Fail;
  }

  InFileName = (mOptions.SkipCPreprocessor == TRUE) ? mOptions.VfrFileName : mOptions.PreprocessorOutputFileName;

  gCVfrErrorHandle.SetInputFile (InFileName);
  gCVfrErrorHandle.SetWarningAsError(mOptions.WarningAsError);

  if ((pInFile = fopen (LongFilePath (InFileName), "r")) == NULL) {
    DebugError (NULL, 0, 0001, "Error opening the input file", "%s", InFileName);
    goto Fail;
  }

  if (mOptions.HasOverrideClassGuid) {
    InputInfo.OverrideClassGuid = &mOptions.OverrideClassGuid;
  } else {
    InputInfo.OverrideClassGuid = NULL;
  }

  if (VfrParserStart (pInFile, &InputInfo) != 0) {
    goto Fail;
  }

  fclose (pInFile);
  pInFile = NULL;

  if (gCFormPkg.HavePendingUnassigned () == TRUE) {
    gCFormPkg.PendingAssignPrintAll ();
    goto Fail;
  }

  SET_RUN_STATUS (STATUS_COMPILEED);
  return;

Fail:
  if (!IS_RUN_STATUS(STATUS_DEAD)) {
    DebugError (NULL, 0, 0003, "Error parsing", "compile error in file %s", InFileName);
    SET_RUN_STATUS (STATUS_FAILED);
  }
  if (pInFile != NULL) {
    fclose (pInFile);
  }
}

VOID
CVfrCompiler::AdjustBin (
  VOID
  )
{
  EFI_VFR_RETURN_CODE Status;

  if (!IS_RUN_STATUS(STATUS_COMPILEED)) {
    return;
  }

  if (gNeedAdjustOpcode) {
    //
    // When parsing the Vfr, has created some opcodes, now need to update the record info.
    //
    gCIfrRecordInfoDB.IfrUpdateRecordInfoForDynamicOpcode (FALSE);
  }

  //
  // Check whether need to check default info for question or auto add default for question.
  //
  if (mOptions.AutoDefault || mOptions.CheckDefault) {
    gCIfrRecordInfoDB.IfrCheckAddDefaultRecord (mOptions.AutoDefault, mOptions.CheckDefault);
  }

  //
  // Check Binary Code consistent between Form and IfrRecord
  //

  //
  // Get Package Data and IfrRecord Data
  //
  gCFormPkg.BuildPkg (gCBuffer);
  gCIfrRecordInfoDB.IfrRecordOutput (gRBuffer);

  //
  // Compare Form and Record data
  //
  if (gCBuffer.Buffer != NULL && gRBuffer.Buffer != NULL) {
    UINT32 Index;
    if (gCBuffer.Size != gRBuffer.Size) {
      DebugError (NULL, 0, 0001, "Error parsing vfr file", " %s. FormBinary Size 0x%X is not same to RecordBuffer Size 0x%X", mOptions.VfrFileName, gCBuffer.Size, gRBuffer.Size);
    }
    for (Index = 0; Index < gCBuffer.Size; Index ++) {
      if (gCBuffer.Buffer[Index] != gRBuffer.Buffer[Index]) {
        break;
      }
    }
    if (Index != gCBuffer.Size) {
      DebugError (NULL, 0, 0001, "Error parsing vfr file", " %s. the 0x%X byte is different between Form and Record", mOptions.VfrFileName, Index);
    }
    DebugMsg (NULL, 0, 9, (CHAR8 *) "IFR Buffer", (CHAR8 *) "Form Buffer same to Record Buffer and Size is 0x%X", Index);
  } else if (gCBuffer.Buffer == NULL && gRBuffer.Buffer == NULL) {
    //ok
  } else {
    DebugError (NULL, 0, 0001, "Error parsing vfr file", " %s.Buffer not allocated.", mOptions.VfrFileName);
  }

  return;
}

VOID
CVfrCompiler::GenBinary (
  VOID
  )
{
  FILE                    *pFile = NULL;

  if (!IS_RUN_STATUS(STATUS_COMPILEED)) {
    goto Fail;
  }

  if (mOptions.CreateIfrPkgFile == TRUE) {
    if ((pFile = fopen (LongFilePath (mOptions.PkgOutputFileName), "wb")) == NULL) {
      DebugError (NULL, 0, 0001, "Error opening file", "%s", mOptions.PkgOutputFileName);
      goto Fail;
    }
    if (gCFormPkg.BuildPkg (pFile, &gRBuffer) != VFR_RETURN_SUCCESS) {
      fclose (pFile);
      goto Fail;
    }
    fclose (pFile);
  }

  SET_RUN_STATUS (STATUS_GENBINARY);

  return;

Fail:
  if (!IS_RUN_STATUS(STATUS_DEAD)) {
    SET_RUN_STATUS (STATUS_FAILED);
  }
}

static const char *gSourceFileHeader[] = {
  "//",
  "//  DO NOT EDIT -- auto-generated file",
  "//",
  "//  This file is generated by the vfrcompiler utility",
  "//",
  NULL
};

VOID
CVfrCompiler::GenCFile (
  VOID
  )
{
  FILE                    *pFile;
  UINT32                  Index;

  if (!IS_RUN_STATUS(STATUS_GENBINARY)) {
    goto Fail;
  }

  if (!mOptions.CreateIfrPkgFile) {
    if ((pFile = fopen (LongFilePath (mOptions.COutputFileName), "w")) == NULL) {
      DebugError (NULL, 0, 0001, "Error opening output C file", "%s", mOptions.COutputFileName);
      goto Fail;
    }

    for (Index = 0; gSourceFileHeader[Index] != NULL; Index++) {
      fprintf (pFile, "%s\n", gSourceFileHeader[Index]);
    }

    if (gCFormPkg.GenCFile (mOptions.VfrBaseFileName, pFile, &gRBuffer) != VFR_RETURN_SUCCESS) {
      fclose (pFile);
      goto Fail;
    }
    fclose (pFile);
  }

  SET_RUN_STATUS (STATUS_FINISHED);
  return;

Fail:
  if (!IS_RUN_STATUS(STATUS_DEAD)) {
    SET_RUN_STATUS (STATUS_FAILED);
  }
}

VOID
CVfrCompiler::GenRecordListFile (
  VOID
  )
{
  CHAR8  *InFileName = NULL;
  FILE   *pInFile    = NULL;
  FILE   *pOutFile   = NULL;
  CHAR8  LineBuf[MAX_VFR_LINE_LEN];
  UINT32 LineNo;

  InFileName = (mOptions.SkipCPreprocessor == TRUE) ? mOptions.VfrFileName : mOptions.PreprocessorOutputFileName;

  if (mOptions.CreateRecordListFile == TRUE && InFileName != NULL && mOptions.RecordListFile != NULL) {
    if ((InFileName[0] == '\0') || (mOptions.RecordListFile[0] == '\0')) {
      return;
    }

    if ((pInFile = fopen (LongFilePath (InFileName), "r")) == NULL) {
      DebugError (NULL, 0, 0001, "Error opening the input VFR preprocessor output file", "%s", InFileName);
      return;
    }

    if ((pOutFile = fopen (LongFilePath (mOptions.RecordListFile), "w")) == NULL) {
      DebugError (NULL, 0, 0001, "Error opening the record list file", "%s", mOptions.RecordListFile);
      goto Err1;
    }

    fprintf (pOutFile, "//\n//  VFR compiler version " VFR_COMPILER_VERSION __BUILD_VERSION "\n//\n");
    LineNo = 0;
    while (!feof (pInFile)) {
      if (fgets (LineBuf, MAX_VFR_LINE_LEN, pInFile) != NULL) {
        fprintf (pOutFile, "%s", LineBuf);
        LineNo++;
        gCIfrRecordInfoDB.IfrRecordOutput (pOutFile, LineNo);
      }
    }

    fprintf (pOutFile, "\n//\n// All Opcode Record List \n//\n");
    gCIfrRecordInfoDB.IfrRecordOutput (pOutFile, 0);
    gCVfrVarDataTypeDB.Dump(pOutFile);

    fclose (pOutFile);
    fclose (pInFile);
  }

  return;

Err1:
  fclose (pInFile);
}

int
main (
  IN int             Argc,
  IN char            **Argv
  )
{
  COMPILER_RUN_STATUS  Status;

  SetPrintLevel(WARNING_LOG_LEVEL);
  CVfrCompiler         Compiler(Argc, Argv);

  Compiler.PreProcess();
  Compiler.Compile();
  Compiler.AdjustBin();
  Compiler.GenBinary();
  Compiler.GenCFile();
  Compiler.GenRecordListFile ();

  Status = Compiler.RunStatus ();
  if ((Status == STATUS_DEAD) || (Status == STATUS_FAILED)) {
    return 2;
  }

  if (gCBuffer.Buffer != NULL) {
    delete[] gCBuffer.Buffer;
  }

  if (gRBuffer.Buffer != NULL) {
    delete[] gRBuffer.Buffer;
  }

  return GetUtilityStatus ();
}


