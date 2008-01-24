#include "stdio.h"
#include "string.h"
#include "process.h"
#include "VfrCompiler.h"


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
  IN INT8       **Argv
  )
{
  INT32         Index;

  mOptions.VfrFileName[0]                = '\0';
  mOptions.RecordListFile[0]             = '\0';
  mOptions.CreateRecordListFile          = FALSE;
  mOptions.CreateIfrPkgFile              = FALSE;
  mOptions.PkgOutputFileName[0]          = '\0';
  mOptions.COutputFileName[0]            = '\0';
  mOptions.OutputDirectory[0]            = '\0';
  mOptions.PreprocessorOutputFileName[0] = '\0';
  mOptions.VfrBaseFileName[0]            = '\0';
  mOptions.IncludePaths                  = NULL;
  mOptions.CPreprocessorOptions          = NULL;

  for (Index = 1; (Index < Argc) && (Argv[Index][0] == '-'); Index++) {
    if ((_stricmp(Argv[Index], "-?") == 0) || (_stricmp(Argv[Index], "-h") == 0)) {
      Usage ();
      SET_RUN_STATUS (STATUS_DEAD);
      return;
    } else if (_stricmp(Argv[Index], "-l") == 0) {
      mOptions.CreateRecordListFile = TRUE;
      gCIfrRecordInfoDB.TurnOn ();
    } else if (_stricmp(Argv[Index], "-i") == 0) {
      Index++;
      if ((Index >= Argc) || (Argv[Index][0] == '-')) {
        printf ("%s -i - missing path argument\n", PROGRAM_NAME);
        goto Fail;
      }

      AppendIncludePath(Argv[Index]);
    } else if (_stricmp(Argv[Index], "-od") == 0) {
      Index++;
      if ((Index >= Argc) || (Argv[Index][0] == '-')) {
        printf ("%s -od - missing output directory name\n", PROGRAM_NAME);
        goto Fail;
      }
      strcpy (mOptions.OutputDirectory, Argv[Index]);
    } else if (_stricmp(Argv[Index], "-ibin") == 0) {
      mOptions.CreateIfrPkgFile = TRUE;
    } else if (_stricmp(Argv[Index], "-nostrings") == 0) {
    } else if (_stricmp(Argv[Index], "-ppflag") == 0) {
      Index++;
      if ((Index >= Argc) || (Argv[Index][0] == '-')) {
        printf ("%s -od - missing C-preprocessor argument\n", PROGRAM_NAME);
        goto Fail;
      }

      AppendCPreprocessorOptions (Argv[Index]);
    } else {
      printf ("%s unrecognized option %s\n", PROGRAM_NAME, Argv[Index]);
      Usage ();
      goto Fail;
    }
  }

  if (Index != Argc - 1) {
    printf ("%s must specify VFR file name", PROGRAM_NAME);
    Usage ();
    goto Fail;
  } else {
    strcpy (mOptions.VfrFileName, Argv[Index]);
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
  SET_RUN_STATUS (STATUS_FAILED);

  mOptions.VfrFileName[0]                = '\0';
  mOptions.RecordListFile[0]             = '\0';
  mOptions.CreateRecordListFile          = FALSE;
  mOptions.CreateIfrPkgFile              = FALSE;
  mOptions.PkgOutputFileName[0]          = '\0';
  mOptions.COutputFileName[0]            = '\0';
  mOptions.OutputDirectory[0]            = '\0';
  mOptions.PreprocessorOutputFileName[0] = '\0';
  mOptions.VfrBaseFileName[0]            = '\0';
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
  IN INT8       *PathStr
  )
{
  UINT32  Len           = 0;
  INT8    *IncludePaths = NULL;

  Len = strlen (" -I ") + strlen (PathStr) + 1;
  if (mOptions.IncludePaths != NULL) {
    Len += strlen (mOptions.IncludePaths);
  }
  IncludePaths = new INT8[Len];
  if (IncludePaths == NULL) {
    printf ("%s memory allocation failure\n", PROGRAM_NAME);
    return;
  }
  IncludePaths[0] = '\0';
  if (mOptions.IncludePaths != NULL) {
    strcat (IncludePaths, mOptions.IncludePaths);
  }
  strcat (IncludePaths, " -I ");
  strcat (IncludePaths, PathStr);
  if (mOptions.IncludePaths != NULL) {
    delete mOptions.IncludePaths;
  }
  mOptions.IncludePaths = IncludePaths;
}

VOID
CVfrCompiler::AppendCPreprocessorOptions (
  IN INT8       *Options
  )
{
  UINT32  Len           = 0;
  INT8    *Opt          = NULL;

  Len = strlen (Options) + strlen (" ") + 1;
  if (mOptions.CPreprocessorOptions != NULL) {
    Len += strlen (mOptions.CPreprocessorOptions);
  }
  Opt = new INT8[Len];
  if (Opt == NULL) {
    printf ("%s memory allocation failure\n", PROGRAM_NAME);
    return;
  }
  Opt[0] = 0;
  if (mOptions.CPreprocessorOptions != NULL) {
    strcat (Opt, mOptions.CPreprocessorOptions);
  }
  strcat (Opt, " ");
  strcat (Opt, Options);
  if (mOptions.CPreprocessorOptions != NULL) {
    delete mOptions.CPreprocessorOptions;
  }
  mOptions.CPreprocessorOptions = Opt;
}

INT8
CVfrCompiler::SetBaseFileName (
  VOID
  )
{
  INT8          *pFileName, *pPath, *pExt;

  if (mOptions.VfrFileName[0] == '\0') {
    return -1;
  }

  pFileName = mOptions.VfrFileName;
  while ((pPath = strchr (pFileName, '\\')) != NULL) {
    pFileName = pPath + 1;
  }

  if (pFileName == NULL) {
    return -1;
  }

  if ((pExt = strchr (pFileName, '.')) == NULL) {
    return -1;
  }

  strncpy (mOptions.VfrBaseFileName, pFileName, pExt - pFileName);
  mOptions.VfrBaseFileName[pExt - pFileName] = '\0';

  return 0;
}

INT8
CVfrCompiler::SetPkgOutputFileName (
  VOID
  )
{
  if (mOptions.VfrBaseFileName[0] == '\0') {
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
  if (mOptions.VfrBaseFileName[0] == '\0') {
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
  if (mOptions.VfrBaseFileName[0] == '\0') {
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
  if (mOptions.VfrBaseFileName[0] == '\0') {
    return -1;
  }

  strcpy (mOptions.RecordListFile, mOptions.OutputDirectory);
  strcat (mOptions.RecordListFile, mOptions.VfrBaseFileName);
  strcat (mOptions.RecordListFile, VFR_RECORDLIST_FILENAME_EXTENSION);

  return 0;
}

CVfrCompiler::CVfrCompiler (
  IN INT32      Argc, 
  IN INT8       **Argv
  )
{
  mPreProcessCmd = PREPROCESSOR_COMMAND;
  mPreProcessOpt = PREPROCESSOR_OPTIONS;

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
  if (mOptions.IncludePaths != NULL) {
    delete mOptions.IncludePaths;
    mOptions.IncludePaths = NULL;
  }

  if (mOptions.CPreprocessorOptions != NULL) {
    delete mOptions.CPreprocessorOptions;
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
  CONST  INT8 *Help[] = {
    " ", 
    "VfrCompile version " VFR_COMPILER_VERSION,
    " ",
    "  Usage: VfrCompile {options} [VfrFile]",
    " ",
    "    where options include:",
    "      -? or -h       prints this help",
    "      -l             create an output IFR listing file",
    "      -i IncPath     add IncPath to the search path for VFR included files",
    "      -od OutputDir  deposit all output files to directory OutputDir (default=cwd)",
    "      -ibin          create an IFR HII pack file"
    "      -ppflag        C-preprocessor argument",
    "    where parameters include:",
    "      VfrFile        name of the input VFR script file",
    " ",
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
  INT8    *PreProcessCmd = NULL;

  if (!IS_RUN_STATUS(STATUS_INITIALIZED)) {
    goto Fail;
  }

  if ((pVfrFile = fopen (mOptions.VfrFileName, "r")) == NULL) {
    printf ("%s could not open input VFR file - %s\n", PROGRAM_NAME, mOptions.VfrFileName);
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

  PreProcessCmd = new INT8[CmdLen + 10];
  if (PreProcessCmd == NULL) {
    printf ("%s could not allocate memory\n", PROGRAM_NAME);
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
    printf ("%s failed to spawn C preprocessor on VFR file \n\t - %s\n", PROGRAM_NAME, PreProcessCmd);
    goto Fail;
  }

  delete PreProcessCmd;
  SET_RUN_STATUS (STATUS_PREPROCESSED);
  return;

Fail:
  if (!IS_RUN_STATUS(STATUS_DEAD)) {
    SET_RUN_STATUS (STATUS_FAILED);
  }
  delete PreProcessCmd;
}

extern UINT8 VfrParserStart (IN FILE *);

VOID
CVfrCompiler::Compile (
  VOID
  )
{
  FILE *VfrFile = NULL;

  if (!IS_RUN_STATUS(STATUS_PREPROCESSED)) {
    goto Fail;
  }

  if ((VfrFile = fopen (mOptions.PreprocessorOutputFileName, "r")) == NULL) {
    printf ("%s failed to open input VFR preprocessor output file - %s\n", PROGRAM_NAME, mOptions.PreprocessorOutputFileName);
    goto Fail;
  }

  if (VfrParserStart (VfrFile) != 0) {
    goto Fail;
  }

  fclose (VfrFile);

  if (gCFormPkg.HavePendingUnassigned () == TRUE) {
    gCFormPkg.PendingAssignPrintAll ();
    goto Fail;
  }

  SET_RUN_STATUS (STATUS_COMPILEED);
  return;

Fail:
  if (!IS_RUN_STATUS(STATUS_DEAD)) {
    printf ("%s compile error!\n", PROGRAM_NAME);
    SET_RUN_STATUS (STATUS_FAILED);
  }
  if (VfrFile != NULL) {
    fclose (VfrFile);
  }
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
    if ((pFile = fopen (mOptions.PkgOutputFileName, "wb")) == NULL) {
      printf ("can not open PkgFileName\n", mOptions.PkgOutputFileName);
      goto Fail;
    }
    if (gCFormPkg.BuildPkg (pFile) != VFR_RETURN_SUCCESS) {
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

  if ((pFile = fopen (mOptions.COutputFileName, "w")) == NULL) {
    printf ("failed to open output C file - %s\n", mOptions.COutputFileName);
    goto Fail;
  }

  for (Index = 0; gSourceFileHeader[Index] != NULL; Index++) {
    fprintf (pFile, "%s\n", gSourceFileHeader[Index]);
  }

  gCVfrBufferConfig.OutputCFile (pFile, mOptions.VfrBaseFileName);

  if (gCFormPkg.GenCFile (mOptions.VfrBaseFileName, pFile) != VFR_RETURN_SUCCESS) {
    fclose (pFile);
    goto Fail;
  }
  fclose (pFile);

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
  FILE   *pInFile  = NULL;
  FILE   *pOutFile = NULL;
  INT8   LineBuf[MAX_LINE_LEN];
  UINT32 LineNo;

  if (mOptions.CreateRecordListFile == TRUE) {
    if ((mOptions.PreprocessorOutputFileName[0] == '\0') || (mOptions.RecordListFile[0] == '\0')) {
      return;
    }

    if ((pInFile = fopen (mOptions.PreprocessorOutputFileName, "r")) == NULL) {
      printf ("%s failed to open input VFR preprocessor output file - %s\n", PROGRAM_NAME, mOptions.PreprocessorOutputFileName);
      return;
    }

    if ((pOutFile = fopen (mOptions.RecordListFile, "w")) == NULL) {
      printf ("%s failed to open record list file for writing - %s\n", PROGRAM_NAME, mOptions.RecordListFile);
      goto Err1;
    }

    fprintf (pOutFile, "//\n//  VFR compiler version " VFR_COMPILER_VERSION "\n//\n");
    LineNo = 0;
    while (!feof (pInFile)) {
      if (fgets (LineBuf, MAX_LINE_LEN, pInFile) != NULL) {
        fprintf (pOutFile, "%s", LineBuf);
        LineNo++;
        gCIfrRecordInfoDB.IfrRecordOutput (pOutFile, LineNo);
      }
    }

    fclose (pOutFile);
    fclose (pInFile);
  }

  return;

Err1:
  fclose (pInFile);
}

INT32
main (
  IN INT32             Argc, 
  IN INT8              **Argv
  )
{
  COMPILER_RUN_STATUS  Status;
  CVfrCompiler         Compiler(Argc, Argv);

  Compiler.PreProcess();
  Compiler.Compile();
  Compiler.GenBinary();
  Compiler.GenCFile();
  Compiler.GenRecordListFile ();

  Status = Compiler.RunStatus ();
  if ((Status == STATUS_DEAD) || (Status == STATUS_FAILED)) {
    return 2;
  }

  return 0;
}

