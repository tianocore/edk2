/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  VfrCompiler.h

Abstract:

--*/

#ifndef _VFRCOMPILER_H_
#define _VFRCOMPILER_H_

#include "Tiano.h"
#include "EfiTypes.h"
#include "EfiVfr.h"
#include "VfrFormPkg.h"
#include "VfrUtilityLib.h"

#define UTILITY_NAME                        "VfrCompile"
#define UTILITY_VERSION                     "v1.1"

//
// This is how we invoke the C preprocessor on the VFR source file
// to resolve #defines, #includes, etc. To make C source files
// shareable between VFR and drivers, define VFRCOMPILE so that
// #ifdefs can be used in shared .h files.
//
#define PREPROCESSOR_COMMAND                "cl "
#define PREPROCESSOR_OPTIONS                "/nologo /E /TC /DVFRCOMPILE "

//
// Specify the filename extensions for the files we generate.
//
#define VFR_PREPROCESS_FILENAME_EXTENSION   ".i"
#define VFR_PACKAGE_FILENAME_EXTENSION      ".hpk"
#define VFR_RECORDLIST_FILENAME_EXTENSION   ".lst"

typedef struct {
  INT8    VfrFileName[MAX_PATH];
  INT8    RecordListFile[MAX_PATH];
  INT8    PkgOutputFileName[MAX_PATH];
  INT8    COutputFileName[MAX_PATH];
  bool    CreateRecordListFile;
  bool    CreateIfrPkgFile;
  INT8    OutputDirectory[MAX_PATH];
  INT8    PreprocessorOutputFileName[MAX_PATH];
  INT8    VfrBaseFileName[MAX_PATH];  // name of input VFR file with no path or extension
  INT8    *IncludePaths;
  bool    SkipCPreprocessor;
  INT8    *CPreprocessorOptions;
} OPTIONS;

typedef enum {
  STATUS_INITIALIZED = 1,
  STATUS_PREPROCESSED,
  STATUS_COMPILEED,
  STATUS_GENBINARY,
  STATUS_FINISHED,
  STATUS_FAILED,
  STATUS_DEAD,
} COMPILER_RUN_STATUS;

class CVfrCompiler {
private:
  COMPILER_RUN_STATUS  mRunStatus;
  OPTIONS              mOptions;
  INT8                 *mPreProcessCmd;
  INT8                 *mPreProcessOpt;

  VOID    OptionInitialization (IN INT32 , IN INT8 **);
  VOID    AppendIncludePath (IN INT8 *);
  VOID    AppendCPreprocessorOptions (IN INT8 *);
  INT8    SetBaseFileName (VOID);
  INT8    SetPkgOutputFileName (VOID);
  INT8    SetCOutputFileName(VOID);
  INT8    SetPreprocessorOutputFileName (VOID);
  INT8    SetRecordListFileName (VOID);

  VOID    SET_RUN_STATUS (IN COMPILER_RUN_STATUS);
  BOOLEAN IS_RUN_STATUS (IN COMPILER_RUN_STATUS);

public:
  COMPILER_RUN_STATUS RunStatus (VOID) {
    return mRunStatus;
  }

public:
  CVfrCompiler (IN INT32 , IN INT8 **);
  ~CVfrCompiler ();

  VOID                Usage (VOID);

  VOID                PreProcess (VOID);
  VOID                Compile (VOID);
  VOID                GenBinary (VOID);
  VOID                GenCFile (VOID);
  VOID                GenRecordListFile (VOID);
};

#endif
