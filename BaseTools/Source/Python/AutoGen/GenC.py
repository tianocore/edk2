## @file
# Routines for generating AutoGen.h and AutoGen.c
#
# Copyright (c) 2007 - 2015, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

## Import Modules
#
import string

from Common import EdkLogger

from Common.BuildToolError import *
from Common.DataType import *
from Common.Misc import *
from Common.String import StringToArray
from StrGather import *
from GenPcdDb import CreatePcdDatabaseCode

## PCD type string
gItemTypeStringDatabase  = {
    TAB_PCDS_FEATURE_FLAG       :   'FixedAtBuild',
    TAB_PCDS_FIXED_AT_BUILD     :   'FixedAtBuild',
    TAB_PCDS_PATCHABLE_IN_MODULE:   'BinaryPatch',
    TAB_PCDS_DYNAMIC            :   '',
    TAB_PCDS_DYNAMIC_DEFAULT    :   '',
    TAB_PCDS_DYNAMIC_VPD        :   '',
    TAB_PCDS_DYNAMIC_HII        :   '',
    TAB_PCDS_DYNAMIC_EX         :   '',
    TAB_PCDS_DYNAMIC_EX_DEFAULT :   '',
    TAB_PCDS_DYNAMIC_EX_VPD     :   '',
    TAB_PCDS_DYNAMIC_EX_HII     :   '',
}

## Dynamic PCD types
gDynamicPcd = [TAB_PCDS_DYNAMIC, TAB_PCDS_DYNAMIC_DEFAULT, TAB_PCDS_DYNAMIC_VPD, TAB_PCDS_DYNAMIC_HII]

## Dynamic-ex PCD types
gDynamicExPcd = [TAB_PCDS_DYNAMIC_EX, TAB_PCDS_DYNAMIC_EX_DEFAULT, TAB_PCDS_DYNAMIC_EX_VPD, TAB_PCDS_DYNAMIC_EX_HII]

## Datum size
gDatumSizeStringDatabase = {'UINT8':'8','UINT16':'16','UINT32':'32','UINT64':'64','BOOLEAN':'BOOLEAN','VOID*':'8'}
gDatumSizeStringDatabaseH = {'UINT8':'8','UINT16':'16','UINT32':'32','UINT64':'64','BOOLEAN':'BOOL','VOID*':'PTR'}
gDatumSizeStringDatabaseLib = {'UINT8':'8','UINT16':'16','UINT32':'32','UINT64':'64','BOOLEAN':'Bool','VOID*':'Ptr'}

## AutoGen File Header Templates
gAutoGenHeaderString = TemplateString("""\
/**
  DO NOT EDIT
  FILE auto-generated
  Module name:
    ${FileName}
  Abstract:       Auto-generated ${FileName} for building module or library.
**/
""")

gAutoGenHPrologueString = TemplateString("""
#ifndef _${File}_${Guid}
#define _${File}_${Guid}

""")

gAutoGenHCppPrologueString = """\
#ifdef __cplusplus
extern "C" {
#endif

"""

gAutoGenHEpilogueString = """

#ifdef __cplusplus
}
#endif

#endif
"""

## PEI Core Entry Point Templates
gPeiCoreEntryPointPrototype = TemplateString("""
${BEGIN}
VOID
EFIAPI
${Function} (
  IN CONST  EFI_SEC_PEI_HAND_OFF    *SecCoreData,
  IN CONST  EFI_PEI_PPI_DESCRIPTOR  *PpiList,
  IN VOID                           *Context
  );
${END}
""")

gPeiCoreEntryPointString = TemplateString("""
${BEGIN}
VOID
EFIAPI
ProcessModuleEntryPointList (
  IN CONST  EFI_SEC_PEI_HAND_OFF    *SecCoreData,
  IN CONST  EFI_PEI_PPI_DESCRIPTOR  *PpiList,
  IN VOID                           *Context
  )

{
  ${Function} (SecCoreData, PpiList, Context);
}
${END}
""")


## DXE Core Entry Point Templates
gDxeCoreEntryPointPrototype = TemplateString("""
${BEGIN}
VOID
EFIAPI
${Function} (
  IN VOID  *HobStart
  );
${END}
""")

gDxeCoreEntryPointString = TemplateString("""
${BEGIN}
VOID
EFIAPI
ProcessModuleEntryPointList (
  IN VOID  *HobStart
  )

{
  ${Function} (HobStart);
}
${END}
""")

## PEIM Entry Point Templates
gPeimEntryPointPrototype = TemplateString("""
${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  );
${END}
""")

gPeimEntryPointString = [
TemplateString("""
GLOBAL_REMOVE_IF_UNREFERENCED const UINT32 _gPeimRevision = ${PiSpecVersion};

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )

{
  return EFI_SUCCESS;
}
"""),
TemplateString("""
GLOBAL_REMOVE_IF_UNREFERENCED const UINT32 _gPeimRevision = ${PiSpecVersion};
${BEGIN}
EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )

{
  return ${Function} (FileHandle, PeiServices);
}
${END}
"""),
TemplateString("""
GLOBAL_REMOVE_IF_UNREFERENCED const UINT32 _gPeimRevision = ${PiSpecVersion};

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )

{
  EFI_STATUS  Status;
  EFI_STATUS  CombinedStatus;

  CombinedStatus = EFI_LOAD_ERROR;
${BEGIN}
  Status = ${Function} (FileHandle, PeiServices);
  if (!EFI_ERROR (Status) || EFI_ERROR (CombinedStatus)) {
    CombinedStatus = Status;
  }
${END}
  return CombinedStatus;
}
""")
]

## SMM_CORE Entry Point Templates
gSmmCoreEntryPointPrototype = TemplateString("""
${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );
${END}
""")

gSmmCoreEntryPointString = TemplateString("""
${BEGIN}
const UINT32 _gUefiDriverRevision = ${UefiSpecVersion};
const UINT32 _gDxeRevision = ${PiSpecVersion};

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  return ${Function} (ImageHandle, SystemTable);
}
${END}
""")

## DXE SMM Entry Point Templates
gDxeSmmEntryPointPrototype = TemplateString("""
${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
${END}
""")

gDxeSmmEntryPointString = [
TemplateString("""
const UINT32 _gUefiDriverRevision = ${UefiSpecVersion};
const UINT32 _gDxeRevision = ${PiSpecVersion};

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )

{
  return EFI_SUCCESS;
}
"""),
TemplateString("""
const UINT32 _gUefiDriverRevision = ${UefiSpecVersion};
const UINT32 _gDxeRevision = ${PiSpecVersion};

static BASE_LIBRARY_JUMP_BUFFER  mJumpContext;
static EFI_STATUS  mDriverEntryPointStatus;

VOID
EFIAPI
ExitDriver (
  IN EFI_STATUS  Status
  )
{
  if (!EFI_ERROR (Status) || EFI_ERROR (mDriverEntryPointStatus)) {
    mDriverEntryPointStatus = Status;
  }
  LongJump (&mJumpContext, (UINTN)-1);
  ASSERT (FALSE);
}

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  mDriverEntryPointStatus = EFI_LOAD_ERROR;

${BEGIN}
  if (SetJump (&mJumpContext) == 0) {
    ExitDriver (${Function} (ImageHandle, SystemTable));
    ASSERT (FALSE);
  }
${END}

  return mDriverEntryPointStatus;
}
""")
]

## UEFI Driver Entry Point Templates
gUefiDriverEntryPointPrototype = TemplateString("""
${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
${END}
""")

gUefiDriverEntryPointString = [
TemplateString("""
const UINT32 _gUefiDriverRevision = ${UefiSpecVersion};
const UINT32 _gDxeRevision = ${PiSpecVersion};

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
"""),
TemplateString("""
const UINT32 _gUefiDriverRevision = ${UefiSpecVersion};
const UINT32 _gDxeRevision = ${PiSpecVersion};

${BEGIN}
EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )

{
  return ${Function} (ImageHandle, SystemTable);
}
${END}
VOID
EFIAPI
ExitDriver (
  IN EFI_STATUS  Status
  )
{
  if (EFI_ERROR (Status)) {
    ProcessLibraryDestructorList (gImageHandle, gST);
  }
  gBS->Exit (gImageHandle, Status, 0, NULL);
}
"""),
TemplateString("""
const UINT32 _gUefiDriverRevision = ${UefiSpecVersion};
const UINT32 _gDxeRevision = ${PiSpecVersion};

static BASE_LIBRARY_JUMP_BUFFER  mJumpContext;
static EFI_STATUS  mDriverEntryPointStatus;

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  mDriverEntryPointStatus = EFI_LOAD_ERROR;
  ${BEGIN}
  if (SetJump (&mJumpContext) == 0) {
    ExitDriver (${Function} (ImageHandle, SystemTable));
    ASSERT (FALSE);
  }
  ${END}
  return mDriverEntryPointStatus;
}

VOID
EFIAPI
ExitDriver (
  IN EFI_STATUS  Status
  )
{
  if (!EFI_ERROR (Status) || EFI_ERROR (mDriverEntryPointStatus)) {
    mDriverEntryPointStatus = Status;
  }
  LongJump (&mJumpContext, (UINTN)-1);
  ASSERT (FALSE);
}
""")
]


## UEFI Application Entry Point Templates
gUefiApplicationEntryPointPrototype = TemplateString("""
${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
${END}
""")

gUefiApplicationEntryPointString = [
TemplateString("""
const UINT32 _gUefiDriverRevision = ${UefiSpecVersion};

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
"""),
TemplateString("""
const UINT32 _gUefiDriverRevision = ${UefiSpecVersion};

${BEGIN}
EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )

{
  return ${Function} (ImageHandle, SystemTable);
}
${END}
VOID
EFIAPI
ExitDriver (
  IN EFI_STATUS  Status
  )
{
  if (EFI_ERROR (Status)) {
    ProcessLibraryDestructorList (gImageHandle, gST);
  }
  gBS->Exit (gImageHandle, Status, 0, NULL);
}
"""),
TemplateString("""
const UINT32 _gUefiDriverRevision = ${UefiSpecVersion};

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )

{
  ${BEGIN}
  if (SetJump (&mJumpContext) == 0) {
    ExitDriver (${Function} (ImageHandle, SystemTable));
    ASSERT (FALSE);
  }
  ${END}
  return mDriverEntryPointStatus;
}

static BASE_LIBRARY_JUMP_BUFFER  mJumpContext;
static EFI_STATUS  mDriverEntryPointStatus = EFI_LOAD_ERROR;

VOID
EFIAPI
ExitDriver (
  IN EFI_STATUS  Status
  )
{
  if (!EFI_ERROR (Status) || EFI_ERROR (mDriverEntryPointStatus)) {
    mDriverEntryPointStatus = Status;
  }
  LongJump (&mJumpContext, (UINTN)-1);
  ASSERT (FALSE);
}
""")
]

## UEFI Unload Image Templates
gUefiUnloadImagePrototype = TemplateString("""
${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN EFI_HANDLE        ImageHandle
  );
${END}
""")

gUefiUnloadImageString = [
TemplateString("""
GLOBAL_REMOVE_IF_UNREFERENCED const UINT8 _gDriverUnloadImageCount = ${Count};

EFI_STATUS
EFIAPI
ProcessModuleUnloadList (
  IN EFI_HANDLE        ImageHandle
  )
{
  return EFI_SUCCESS;
}
"""),
TemplateString("""
GLOBAL_REMOVE_IF_UNREFERENCED const UINT8 _gDriverUnloadImageCount = ${Count};

${BEGIN}
EFI_STATUS
EFIAPI
ProcessModuleUnloadList (
  IN EFI_HANDLE        ImageHandle
  )
{
  return ${Function} (ImageHandle);
}
${END}
"""),
TemplateString("""
GLOBAL_REMOVE_IF_UNREFERENCED const UINT8 _gDriverUnloadImageCount = ${Count};

EFI_STATUS
EFIAPI
ProcessModuleUnloadList (
  IN EFI_HANDLE        ImageHandle
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;
${BEGIN}
  if (EFI_ERROR (Status)) {
    ${Function} (ImageHandle);
  } else {
    Status = ${Function} (ImageHandle);
  }
${END}
  return Status;
}
""")
]

gLibraryStructorPrototype = {
'BASE'  : TemplateString("""${BEGIN}
RETURN_STATUS
EFIAPI
${Function} (
  VOID
  );${END}
"""),

'PEI'   : TemplateString("""${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN       EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES          **PeiServices
  );${END}
"""),

'DXE'   : TemplateString("""${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );${END}
"""),
}

gLibraryStructorCall = {
'BASE'  : TemplateString("""${BEGIN}
  Status = ${Function} ();
  ASSERT_EFI_ERROR (Status);${END}
"""),

'PEI'   : TemplateString("""${BEGIN}
  Status = ${Function} (FileHandle, PeiServices);
  ASSERT_EFI_ERROR (Status);${END}
"""),

'DXE'   : TemplateString("""${BEGIN}
  Status = ${Function} (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);${END}
"""),
}

## Library Constructor and Destructor Templates
gLibraryString = {
'BASE'  :   TemplateString("""
${BEGIN}${FunctionPrototype}${END}

VOID
EFIAPI
ProcessLibrary${Type}List (
  VOID
  )
{
${BEGIN}  EFI_STATUS  Status;
${FunctionCall}${END}
}
"""),

'PEI'   :   TemplateString("""
${BEGIN}${FunctionPrototype}${END}

VOID
EFIAPI
ProcessLibrary${Type}List (
  IN       EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
{
${BEGIN}  EFI_STATUS  Status;
${FunctionCall}${END}
}
"""),

'DXE'   :   TemplateString("""
${BEGIN}${FunctionPrototype}${END}

VOID
EFIAPI
ProcessLibrary${Type}List (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
${BEGIN}  EFI_STATUS  Status;
${FunctionCall}${END}
}
"""),
}

gBasicHeaderFile = "Base.h"

gModuleTypeHeaderFile = {
    "BASE"              :   [gBasicHeaderFile],
    "SEC"               :   ["PiPei.h", "Library/DebugLib.h"],
    "PEI_CORE"          :   ["PiPei.h", "Library/DebugLib.h", "Library/PeiCoreEntryPoint.h"],
    "PEIM"              :   ["PiPei.h", "Library/DebugLib.h", "Library/PeimEntryPoint.h"],
    "DXE_CORE"          :   ["PiDxe.h", "Library/DebugLib.h", "Library/DxeCoreEntryPoint.h"],
    "DXE_DRIVER"        :   ["PiDxe.h", "Library/BaseLib.h", "Library/DebugLib.h", "Library/UefiBootServicesTableLib.h", "Library/UefiDriverEntryPoint.h"],
    "DXE_SMM_DRIVER"    :   ["PiDxe.h", "Library/BaseLib.h", "Library/DebugLib.h", "Library/UefiBootServicesTableLib.h", "Library/UefiDriverEntryPoint.h"],
    "DXE_RUNTIME_DRIVER":   ["PiDxe.h", "Library/BaseLib.h", "Library/DebugLib.h", "Library/UefiBootServicesTableLib.h", "Library/UefiDriverEntryPoint.h"],
    "DXE_SAL_DRIVER"    :   ["PiDxe.h", "Library/BaseLib.h", "Library/DebugLib.h", "Library/UefiBootServicesTableLib.h", "Library/UefiDriverEntryPoint.h"],
    "UEFI_DRIVER"       :   ["Uefi.h",  "Library/BaseLib.h", "Library/DebugLib.h", "Library/UefiBootServicesTableLib.h", "Library/UefiDriverEntryPoint.h"],
    "UEFI_APPLICATION"  :   ["Uefi.h",  "Library/BaseLib.h", "Library/DebugLib.h", "Library/UefiBootServicesTableLib.h", "Library/UefiApplicationEntryPoint.h"],
    "SMM_CORE"          :   ["PiDxe.h", "Library/BaseLib.h", "Library/DebugLib.h", "Library/UefiDriverEntryPoint.h"],
    "USER_DEFINED"      :   [gBasicHeaderFile]
}

## Autogen internal worker macro to define DynamicEx PCD name includes both the TokenSpaceGuidName 
#  the TokenName and Guid comparison to avoid define name collisions.
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenH    The TemplateString object for header file
#
#
def DynExPcdTokenNumberMapping(Info, AutoGenH):
    ExTokenCNameList = []
    PcdExList        = []
    if Info.IsLibrary:
        PcdList = Info.LibraryPcdList
    else:
        PcdList = Info.ModulePcdList
    for Pcd in PcdList:
        if Pcd.Type in gDynamicExPcd:
            ExTokenCNameList.append(Pcd.TokenCName)
            PcdExList.append(Pcd)
    if len(ExTokenCNameList) == 0:
        return
    AutoGenH.Append('\n#define COMPAREGUID(Guid1, Guid2) (BOOLEAN)(*(CONST UINT64*)Guid1 == *(CONST UINT64*)Guid2 && *((CONST UINT64*)Guid1 + 1) == *((CONST UINT64*)Guid2 + 1))\n')
    # AutoGen for each PCD listed in a [PcdEx] section of a Module/Lib INF file.
    # Auto generate a macro for each TokenName that takes a Guid pointer as a parameter.  
    # Use the Guid pointer to see if it matches any of the token space GUIDs.
    TokenCNameList = []
    for TokenCName in ExTokenCNameList:
        if TokenCName in TokenCNameList:
            continue
        Index = 0
        Count = ExTokenCNameList.count(TokenCName)
        for Pcd in PcdExList:
            if Pcd.TokenCName == TokenCName:
                Index = Index + 1
                if Index == 1:
                    AutoGenH.Append('\n#define __PCD_%s_ADDR_CMP(GuidPtr)  (' % (Pcd.TokenCName))
                    AutoGenH.Append('\\\n  (GuidPtr == &%s) ? _PCD_TOKEN_%s_%s:' 
                                    % (Pcd.TokenSpaceGuidCName, Pcd.TokenSpaceGuidCName, Pcd.TokenCName))
                else:
                    AutoGenH.Append('\\\n  (GuidPtr == &%s) ? _PCD_TOKEN_%s_%s:' 
                                    % (Pcd.TokenSpaceGuidCName, Pcd.TokenSpaceGuidCName, Pcd.TokenCName))
                if Index == Count:
                    AutoGenH.Append('0 \\\n  )\n')
                TokenCNameList.append(TokenCName)
    
    TokenCNameList = []
    for TokenCName in ExTokenCNameList:
        if TokenCName in TokenCNameList:
            continue
        Index = 0
        Count = ExTokenCNameList.count(TokenCName)
        for Pcd in PcdExList:
            if Pcd.Type in gDynamicExPcd and Pcd.TokenCName == TokenCName:
                Index = Index + 1
                if Index == 1:
                    AutoGenH.Append('\n#define __PCD_%s_VAL_CMP(GuidPtr)  (' % (Pcd.TokenCName))
                    AutoGenH.Append('\\\n  COMPAREGUID (GuidPtr, &%s) ? _PCD_TOKEN_%s_%s:' 
                                    % (Pcd.TokenSpaceGuidCName, Pcd.TokenSpaceGuidCName, Pcd.TokenCName))
                else:
                    AutoGenH.Append('\\\n  COMPAREGUID (GuidPtr, &%s) ? _PCD_TOKEN_%s_%s:' 
                                    % (Pcd.TokenSpaceGuidCName, Pcd.TokenSpaceGuidCName, Pcd.TokenCName))
                if Index == Count:
                    AutoGenH.Append('0 \\\n  )\n')
                    # Autogen internal worker macro to compare GUIDs.  Guid1 is a pointer to a GUID.  
                    # Guid2 is a C name for a GUID. Compare pointers first because optimizing compiler
                    # can do this at build time on CONST GUID pointers and optimize away call to COMPAREGUID().
                    #  COMPAREGUID() will only be used if the Guid passed in is local to the module.
                    AutoGenH.Append('#define _PCD_TOKEN_EX_%s(GuidPtr)   __PCD_%s_ADDR_CMP(GuidPtr) ? __PCD_%s_ADDR_CMP(GuidPtr) : __PCD_%s_VAL_CMP(GuidPtr)  \n'
                                    % (Pcd.TokenCName, Pcd.TokenCName, Pcd.TokenCName, Pcd.TokenCName))
                TokenCNameList.append(TokenCName)

## Create code for module PCDs
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#   @param      Pcd         The PCD object
#
def CreateModulePcdCode(Info, AutoGenC, AutoGenH, Pcd):
    TokenSpaceGuidValue = Pcd.TokenSpaceGuidValue   #Info.GuidList[Pcd.TokenSpaceGuidCName]
    PcdTokenNumber = Info.PlatformInfo.PcdTokenNumber
    #
    # Write PCDs
    #
    PcdTokenName = '_PCD_TOKEN_' + Pcd.TokenCName
    if Pcd.Type in gDynamicExPcd:
        TokenNumber = int(Pcd.TokenValue, 0)
        # Add TokenSpaceGuidValue value to PcdTokenName to discriminate the DynamicEx PCDs with 
        # different Guids but same TokenCName
        PcdExTokenName = '_PCD_TOKEN_' + Pcd.TokenSpaceGuidCName + '_' + Pcd.TokenCName
        AutoGenH.Append('\n#define %s  %dU\n' % (PcdExTokenName, TokenNumber))
    else:
        if (Pcd.TokenCName, Pcd.TokenSpaceGuidCName) not in PcdTokenNumber:
            # If one of the Source built modules listed in the DSC is not listed in FDF modules, 
            # and the INF lists a PCD can only use the PcdsDynamic access method (it is only 
            # listed in the DEC file that declares the PCD as PcdsDynamic), then build tool will 
            # report warning message notify the PI that they are attempting to build a module 
            # that must be included in a flash image in order to be functional. These Dynamic PCD 
            # will not be added into the Database unless it is used by other modules that are 
            # included in the FDF file. 
            # In this case, just assign an invalid token number to make it pass build.
            if Pcd.Type in PCD_DYNAMIC_TYPE_LIST:
                TokenNumber = 0
            else:
                EdkLogger.error("build", AUTOGEN_ERROR,
                                "No generated token number for %s.%s\n" % (Pcd.TokenSpaceGuidCName, Pcd.TokenCName),
                                ExtraData="[%s]" % str(Info))
        else:
            TokenNumber = PcdTokenNumber[Pcd.TokenCName, Pcd.TokenSpaceGuidCName]
        AutoGenH.Append('\n#define %s  %dU\n' % (PcdTokenName, TokenNumber))

    EdkLogger.debug(EdkLogger.DEBUG_3, "Creating code for " + Pcd.TokenCName + "." + Pcd.TokenSpaceGuidCName)
    if Pcd.Type not in gItemTypeStringDatabase:
        EdkLogger.error("build", AUTOGEN_ERROR,
                        "Unknown PCD type [%s] of PCD %s.%s" % (Pcd.Type, Pcd.TokenSpaceGuidCName, Pcd.TokenCName),
                        ExtraData="[%s]" % str(Info))
    if Pcd.DatumType not in gDatumSizeStringDatabase:
        EdkLogger.error("build", AUTOGEN_ERROR,
                        "Unknown datum type [%s] of PCD %s.%s" % (Pcd.DatumType, Pcd.TokenSpaceGuidCName, Pcd.TokenCName),
                        ExtraData="[%s]" % str(Info))

    DatumSize = gDatumSizeStringDatabase[Pcd.DatumType]
    DatumSizeLib = gDatumSizeStringDatabaseLib[Pcd.DatumType]
    GetModeName = '_PCD_GET_MODE_' + gDatumSizeStringDatabaseH[Pcd.DatumType] + '_' + Pcd.TokenCName
    SetModeName = '_PCD_SET_MODE_' + gDatumSizeStringDatabaseH[Pcd.DatumType] + '_' + Pcd.TokenCName
    SetModeStatusName = '_PCD_SET_MODE_' + gDatumSizeStringDatabaseH[Pcd.DatumType] + '_S_' + Pcd.TokenCName

    PcdExCNameList  = []
    if Pcd.Type in gDynamicExPcd:
        if Info.IsLibrary:
            PcdList = Info.LibraryPcdList
        else:
            PcdList = Info.ModulePcdList
        for PcdModule in PcdList:
            if PcdModule.Type in gDynamicExPcd:
                PcdExCNameList.append(PcdModule.TokenCName)
        # Be compatible with the current code which using PcdToken and PcdGet/Set for DynamicEx Pcd.
        # If only PcdToken and PcdGet/Set used in all Pcds with different CName, it should succeed to build.
        # If PcdToken and PcdGet/Set used in the Pcds with different Guids but same CName, it should failed to build.
        if PcdExCNameList.count(Pcd.TokenCName) > 1:
            AutoGenH.Append('// Disabled the macros, as PcdToken and PcdGet/Set are not allowed in the case that more than one DynamicEx Pcds are different Guids but same CName.\n')
            AutoGenH.Append('// #define %s  %s\n' % (PcdTokenName, PcdExTokenName))
            AutoGenH.Append('// #define %s  LibPcdGetEx%s(&%s, %s)\n' % (GetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
            if Pcd.DatumType == 'VOID*':
                AutoGenH.Append('// #define %s(SizeOfBuffer, Buffer)  LibPcdSetEx%s(&%s, %s, (SizeOfBuffer), (Buffer))\n' % (SetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
                AutoGenH.Append('// #define %s(SizeOfBuffer, Buffer)  LibPcdSetEx%sS(&%s, %s, (SizeOfBuffer), (Buffer))\n' % (SetModeStatusName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
            else:
                AutoGenH.Append('// #define %s(Value)  LibPcdSetEx%s(&%s, %s, (Value))\n' % (SetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
                AutoGenH.Append('// #define %s(Value)  LibPcdSetEx%sS(&%s, %s, (Value))\n' % (SetModeStatusName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
        else:
            AutoGenH.Append('#define %s  %s\n' % (PcdTokenName, PcdExTokenName))
            AutoGenH.Append('#define %s  LibPcdGetEx%s(&%s, %s)\n' % (GetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
            if Pcd.DatumType == 'VOID*':
                AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPcdSetEx%s(&%s, %s, (SizeOfBuffer), (Buffer))\n' % (SetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
                AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPcdSetEx%sS(&%s, %s, (SizeOfBuffer), (Buffer))\n' % (SetModeStatusName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
            else:
                AutoGenH.Append('#define %s(Value)  LibPcdSetEx%s(&%s, %s, (Value))\n' % (SetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
                AutoGenH.Append('#define %s(Value)  LibPcdSetEx%sS(&%s, %s, (Value))\n' % (SetModeStatusName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
    elif Pcd.Type in gDynamicPcd:
        AutoGenH.Append('#define %s  LibPcdGet%s(%s)\n' % (GetModeName, DatumSizeLib, PcdTokenName))
        if Pcd.DatumType == 'VOID*':
            AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPcdSet%s(%s, (SizeOfBuffer), (Buffer))\n' %(SetModeName, DatumSizeLib, PcdTokenName))
            AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPcdSet%sS(%s, (SizeOfBuffer), (Buffer))\n' % (SetModeStatusName, DatumSizeLib, PcdTokenName))
        else:
            AutoGenH.Append('#define %s(Value)  LibPcdSet%s(%s, (Value))\n' % (SetModeName, DatumSizeLib, PcdTokenName))
            AutoGenH.Append('#define %s(Value)  LibPcdSet%sS(%s, (Value))\n' % (SetModeStatusName, DatumSizeLib, PcdTokenName))
    else:
        PcdVariableName = '_gPcd_' + gItemTypeStringDatabase[Pcd.Type] + '_' + Pcd.TokenCName
        Const = 'const'
        if Pcd.Type == TAB_PCDS_PATCHABLE_IN_MODULE:
            Const = ''
        Type = ''
        Array = ''
        Value = Pcd.DefaultValue
        Unicode = False
        ValueNumber = 0

        if Pcd.DatumType == 'BOOLEAN':
            BoolValue = Value.upper()
            if BoolValue == 'TRUE' or BoolValue == '1':
                Value = '1U'
            elif BoolValue == 'FALSE' or BoolValue == '0':
                Value = '0U'

        if Pcd.DatumType in ['UINT64', 'UINT32', 'UINT16', 'UINT8']:
            try:
                if Value.upper().startswith('0X'):
                    ValueNumber = int (Value, 16)
                else:
                    ValueNumber = int (Value)
            except:
                EdkLogger.error("build", AUTOGEN_ERROR,
                                "PCD value is not valid dec or hex number for datum type [%s] of PCD %s.%s" % (Pcd.DatumType, Pcd.TokenSpaceGuidCName, Pcd.TokenCName),
                                ExtraData="[%s]" % str(Info))
            if Pcd.DatumType == 'UINT64':
                if ValueNumber < 0:
                    EdkLogger.error("build", AUTOGEN_ERROR,
                                    "PCD can't be set to negative value for datum type [%s] of PCD %s.%s" % (Pcd.DatumType, Pcd.TokenSpaceGuidCName, Pcd.TokenCName),
                                    ExtraData="[%s]" % str(Info))
                elif ValueNumber >= 0x10000000000000000:
                    EdkLogger.error("build", AUTOGEN_ERROR,
                                    "Too large PCD value for datum type [%s] of PCD %s.%s" % (Pcd.DatumType, Pcd.TokenSpaceGuidCName, Pcd.TokenCName),
                                    ExtraData="[%s]" % str(Info))
                if not Value.endswith('ULL'):
                    Value += 'ULL'
            elif Pcd.DatumType == 'UINT32':
                if ValueNumber < 0:
                    EdkLogger.error("build", AUTOGEN_ERROR,
                                    "PCD can't be set to negative value for datum type [%s] of PCD %s.%s" % (Pcd.DatumType, Pcd.TokenSpaceGuidCName, Pcd.TokenCName),
                                    ExtraData="[%s]" % str(Info))
                elif ValueNumber >= 0x100000000:
                    EdkLogger.error("build", AUTOGEN_ERROR,
                                    "Too large PCD value for datum type [%s] of PCD %s.%s" % (Pcd.DatumType, Pcd.TokenSpaceGuidCName, Pcd.TokenCName),
                                    ExtraData="[%s]" % str(Info))
                if not Value.endswith('U'):
                    Value += 'U'
            elif Pcd.DatumType == 'UINT16':
                if ValueNumber < 0:
                    EdkLogger.error("build", AUTOGEN_ERROR,
                                    "PCD can't be set to negative value for datum type [%s] of PCD %s.%s" % (Pcd.DatumType, Pcd.TokenSpaceGuidCName, Pcd.TokenCName),
                                    ExtraData="[%s]" % str(Info))
                elif ValueNumber >= 0x10000:
                    EdkLogger.error("build", AUTOGEN_ERROR,
                                    "Too large PCD value for datum type [%s] of PCD %s.%s" % (Pcd.DatumType, Pcd.TokenSpaceGuidCName, Pcd.TokenCName),
                                    ExtraData="[%s]" % str(Info))
                if not Value.endswith('U'):
                    Value += 'U'                    
            elif Pcd.DatumType == 'UINT8':
                if ValueNumber < 0:
                    EdkLogger.error("build", AUTOGEN_ERROR,
                                    "PCD can't be set to negative value for datum type [%s] of PCD %s.%s" % (Pcd.DatumType, Pcd.TokenSpaceGuidCName, Pcd.TokenCName),
                                    ExtraData="[%s]" % str(Info))
                elif ValueNumber >= 0x100:
                    EdkLogger.error("build", AUTOGEN_ERROR,
                                    "Too large PCD value for datum type [%s] of PCD %s.%s" % (Pcd.DatumType, Pcd.TokenSpaceGuidCName, Pcd.TokenCName),
                                    ExtraData="[%s]" % str(Info))
                if not Value.endswith('U'):
                    Value += 'U'
        if Pcd.DatumType == 'VOID*':
            if Pcd.MaxDatumSize == None or Pcd.MaxDatumSize == '':
                EdkLogger.error("build", AUTOGEN_ERROR,
                                "Unknown [MaxDatumSize] of PCD [%s.%s]" % (Pcd.TokenSpaceGuidCName, Pcd.TokenCName),
                                ExtraData="[%s]" % str(Info))

            ArraySize = int(Pcd.MaxDatumSize, 0)
            if Value[0] == '{':
                Type = '(VOID *)'
            else:
                if Value[0] == 'L':
                    Unicode = True
                Value = Value.lstrip('L')   #.strip('"')
                Value = eval(Value)         # translate escape character
                NewValue = '{'
                for Index in range(0,len(Value)):
                    if Unicode:
                        NewValue = NewValue + str(ord(Value[Index]) % 0x10000) + ', '
                    else:
                        NewValue = NewValue + str(ord(Value[Index]) % 0x100) + ', '
                if Unicode:
                    ArraySize = ArraySize / 2;

                if ArraySize < (len(Value) + 1):
                    EdkLogger.error("build", AUTOGEN_ERROR,
                                    "The maximum size of VOID* type PCD '%s.%s' is less than its actual size occupied." % (Pcd.TokenSpaceGuidCName, Pcd.TokenCName),
                                    ExtraData="[%s]" % str(Info))
                Value = NewValue + '0 }'
            Array = '[%d]' % ArraySize
        #
        # skip casting for fixed at build since it breaks ARM assembly.
        # Long term we need PCD macros that work in assembly
        #
        elif Pcd.Type != TAB_PCDS_FIXED_AT_BUILD:
            Value = "((%s)%s)" % (Pcd.DatumType, Value)

        if Pcd.Type == TAB_PCDS_PATCHABLE_IN_MODULE:
            PcdValueName = '_PCD_PATCHABLE_VALUE_' + Pcd.TokenCName
        else:
            PcdValueName = '_PCD_VALUE_' + Pcd.TokenCName
            
        if Pcd.DatumType == 'VOID*':
            #
            # For unicode, UINT16 array will be generated, so the alignment of unicode is guaranteed.
            #
            if Unicode:
                AutoGenH.Append('#define _PCD_PATCHABLE_%s_SIZE %s\n' % (Pcd.TokenCName, Pcd.MaxDatumSize))
                AutoGenH.Append('#define %s  %s%s\n' %(PcdValueName, Type, PcdVariableName))
                AutoGenC.Append('GLOBAL_REMOVE_IF_UNREFERENCED %s UINT16 %s%s = %s;\n' % (Const, PcdVariableName, Array, Value))
                AutoGenH.Append('extern %s UINT16 %s%s;\n' %(Const, PcdVariableName, Array))
                AutoGenH.Append('#define %s  %s%s\n' %(GetModeName, Type, PcdVariableName))
            else:
                AutoGenH.Append('#define _PCD_PATCHABLE_%s_SIZE %s\n' % (Pcd.TokenCName, Pcd.MaxDatumSize))
                AutoGenH.Append('#define %s  %s%s\n' %(PcdValueName, Type, PcdVariableName))
                AutoGenC.Append('GLOBAL_REMOVE_IF_UNREFERENCED %s UINT8 %s%s = %s;\n' % (Const, PcdVariableName, Array, Value))
                AutoGenH.Append('extern %s UINT8 %s%s;\n' %(Const, PcdVariableName, Array))
                AutoGenH.Append('#define %s  %s%s\n' %(GetModeName, Type, PcdVariableName))
        elif Pcd.Type == TAB_PCDS_PATCHABLE_IN_MODULE:
            AutoGenH.Append('#define %s  %s\n' %(PcdValueName, Value))
            AutoGenC.Append('volatile %s %s %s = %s;\n' %(Const, Pcd.DatumType, PcdVariableName, PcdValueName))
            AutoGenH.Append('extern volatile %s  %s  %s%s;\n' % (Const, Pcd.DatumType, PcdVariableName, Array))
            AutoGenH.Append('#define %s  %s%s\n' % (GetModeName, Type, PcdVariableName))
        else:
            AutoGenH.Append('#define %s  %s\n' %(PcdValueName, Value))
            AutoGenC.Append('GLOBAL_REMOVE_IF_UNREFERENCED %s %s %s = %s;\n' %(Const, Pcd.DatumType, PcdVariableName, PcdValueName))
            AutoGenH.Append('extern %s  %s  %s%s;\n' % (Const, Pcd.DatumType, PcdVariableName, Array))
            AutoGenH.Append('#define %s  %s%s\n' % (GetModeName, Type, PcdVariableName))

        if Pcd.Type == TAB_PCDS_PATCHABLE_IN_MODULE:
            if Pcd.DatumType == 'VOID*':
                AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPatchPcdSetPtr(_gPcd_BinaryPatch_%s, (UINTN)_PCD_PATCHABLE_%s_SIZE, (SizeOfBuffer), (Buffer))\n' % (SetModeName, Pcd.TokenCName, Pcd.TokenCName))
                AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPatchPcdSetPtrS(_gPcd_BinaryPatch_%s, (UINTN)_PCD_PATCHABLE_%s_SIZE, (SizeOfBuffer), (Buffer))\n' % (SetModeStatusName, Pcd.TokenCName, Pcd.TokenCName))
            else:
                AutoGenH.Append('#define %s(Value)  (%s = (Value))\n' % (SetModeName, PcdVariableName))
                AutoGenH.Append('#define %s(Value)  ((%s = (Value)), RETURN_SUCCESS) \n' % (SetModeStatusName, PcdVariableName))
        else:
            AutoGenH.Append('//#define %s  ASSERT(FALSE)  // It is not allowed to set value for a FIXED_AT_BUILD PCD\n' % SetModeName)

## Create code for library module PCDs
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#   @param      Pcd         The PCD object
#
def CreateLibraryPcdCode(Info, AutoGenC, AutoGenH, Pcd):
    PcdTokenNumber = Info.PlatformInfo.PcdTokenNumber
    TokenSpaceGuidCName = Pcd.TokenSpaceGuidCName
    TokenCName  = Pcd.TokenCName
    PcdTokenName = '_PCD_TOKEN_' + TokenCName
    #
    # Write PCDs
    #
    if Pcd.Type in gDynamicExPcd:
        TokenNumber = int(Pcd.TokenValue, 0)
    else:
        if (Pcd.TokenCName, Pcd.TokenSpaceGuidCName) not in PcdTokenNumber:
            # If one of the Source built modules listed in the DSC is not listed in FDF modules, 
            # and the INF lists a PCD can only use the PcdsDynamic access method (it is only 
            # listed in the DEC file that declares the PCD as PcdsDynamic), then build tool will 
            # report warning message notify the PI that they are attempting to build a module 
            # that must be included in a flash image in order to be functional. These Dynamic PCD 
            # will not be added into the Database unless it is used by other modules that are 
            # included in the FDF file. 
            # In this case, just assign an invalid token number to make it pass build.
            if Pcd.Type in PCD_DYNAMIC_TYPE_LIST:
                TokenNumber = 0
            else:
                EdkLogger.error("build", AUTOGEN_ERROR,
                                "No generated token number for %s.%s\n" % (Pcd.TokenSpaceGuidCName, Pcd.TokenCName),
                                ExtraData="[%s]" % str(Info))
        else:
            TokenNumber = PcdTokenNumber[Pcd.TokenCName, Pcd.TokenSpaceGuidCName]

    if Pcd.Type not in gItemTypeStringDatabase:
        EdkLogger.error("build", AUTOGEN_ERROR,
                        "Unknown PCD type [%s] of PCD %s.%s" % (Pcd.Type, Pcd.TokenSpaceGuidCName, Pcd.TokenCName),
                        ExtraData="[%s]" % str(Info))
    if Pcd.DatumType not in gDatumSizeStringDatabase:
        EdkLogger.error("build", AUTOGEN_ERROR,
                        "Unknown datum type [%s] of PCD %s.%s" % (Pcd.DatumType, Pcd.TokenSpaceGuidCName, Pcd.TokenCName),
                        ExtraData="[%s]" % str(Info))

    DatumType   = Pcd.DatumType
    DatumSize   = gDatumSizeStringDatabaseH[DatumType]
    DatumSizeLib= gDatumSizeStringDatabaseLib[DatumType]
    GetModeName = '_PCD_GET_MODE_' + DatumSize + '_' + TokenCName
    SetModeName = '_PCD_SET_MODE_' + DatumSize + '_' + TokenCName
    SetModeStatusName = '_PCD_SET_MODE_' + DatumSize + '_S_' + TokenCName

    Type = ''
    Array = ''
    if Pcd.DatumType == 'VOID*':
        Type = '(VOID *)'
        Array = '[]'
    PcdItemType = Pcd.Type
    PcdExCNameList  = []
    if PcdItemType in gDynamicExPcd:
        PcdExTokenName = '_PCD_TOKEN_' + TokenSpaceGuidCName + '_' + Pcd.TokenCName
        AutoGenH.Append('\n#define %s  %dU\n' % (PcdExTokenName, TokenNumber))
        
        if Info.IsLibrary:
            PcdList = Info.LibraryPcdList
        else:
            PcdList = Info.ModulePcdList
        for PcdModule in PcdList:
            if PcdModule.Type in gDynamicExPcd:
                PcdExCNameList.append(PcdModule.TokenCName)
        # Be compatible with the current code which using PcdGet/Set for DynamicEx Pcd.
        # If only PcdGet/Set used in all Pcds with different CName, it should succeed to build.
        # If PcdGet/Set used in the Pcds with different Guids but same CName, it should failed to build.
        if PcdExCNameList.count(Pcd.TokenCName) > 1:
            AutoGenH.Append('// Disabled the macros, as PcdToken and PcdGet/Set are not allowed in the case that more than one DynamicEx Pcds are different Guids but same CName.\n')
            AutoGenH.Append('// #define %s  %s\n' % (PcdTokenName, PcdExTokenName))
            AutoGenH.Append('// #define %s  LibPcdGetEx%s(&%s, %s)\n' % (GetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
            if Pcd.DatumType == 'VOID*':
                AutoGenH.Append('// #define %s(SizeOfBuffer, Buffer)  LibPcdSetEx%s(&%s, %s, (SizeOfBuffer), (Buffer))\n' % (SetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
                AutoGenH.Append('// #define %s(SizeOfBuffer, Buffer)  LibPcdSetEx%sS(&%s, %s, (SizeOfBuffer), (Buffer))\n' % (SetModeStatusName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
            else:
                AutoGenH.Append('// #define %s(Value)  LibPcdSetEx%s(&%s, %s, (Value))\n' % (SetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
                AutoGenH.Append('// #define %s(Value)  LibPcdSetEx%sS(&%s, %s, (Value))\n' % (SetModeStatusName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
        else:
            AutoGenH.Append('#define %s  %s\n' % (PcdTokenName, PcdExTokenName))
            AutoGenH.Append('#define %s  LibPcdGetEx%s(&%s, %s)\n' % (GetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
            if Pcd.DatumType == 'VOID*':
                AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPcdSetEx%s(&%s, %s, (SizeOfBuffer), (Buffer))\n' % (SetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
                AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPcdSetEx%sS(&%s, %s, (SizeOfBuffer), (Buffer))\n' % (SetModeStatusName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
            else:
                AutoGenH.Append('#define %s(Value)  LibPcdSetEx%s(&%s, %s, (Value))\n' % (SetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
                AutoGenH.Append('#define %s(Value)  LibPcdSetEx%sS(&%s, %s, (Value))\n' % (SetModeStatusName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
    else:
        AutoGenH.Append('#define _PCD_TOKEN_%s  %dU\n' % (TokenCName, TokenNumber))
    if PcdItemType in gDynamicPcd:
        AutoGenH.Append('#define %s  LibPcdGet%s(%s)\n' % (GetModeName, DatumSizeLib, PcdTokenName))
        if DatumType == 'VOID*':
            AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPcdSet%s(%s, (SizeOfBuffer), (Buffer))\n' %(SetModeName, DatumSizeLib, PcdTokenName))
            AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPcdSet%sS(%s, (SizeOfBuffer), (Buffer))\n' % (SetModeStatusName, DatumSizeLib, PcdTokenName))
        else:
            AutoGenH.Append('#define %s(Value)  LibPcdSet%s(%s, (Value))\n' % (SetModeName, DatumSizeLib, PcdTokenName))
            AutoGenH.Append('#define %s(Value)  LibPcdSet%sS(%s, (Value))\n' % (SetModeStatusName, DatumSizeLib, PcdTokenName))
    if PcdItemType == TAB_PCDS_PATCHABLE_IN_MODULE:
        PcdVariableName = '_gPcd_' + gItemTypeStringDatabase[TAB_PCDS_PATCHABLE_IN_MODULE] + '_' + TokenCName
        AutoGenH.Append('extern volatile %s _gPcd_BinaryPatch_%s%s;\n' %(DatumType, TokenCName, Array) )
        AutoGenH.Append('#define %s  %s_gPcd_BinaryPatch_%s\n' %(GetModeName, Type, TokenCName))
        AutoGenH.Append('#define %s(Value)  (%s = (Value))\n' % (SetModeName, PcdVariableName))
        AutoGenH.Append('#define %s(Value)  ((%s = (Value)), RETURN_SUCCESS)\n' % (SetModeStatusName, PcdVariableName))
    if PcdItemType == TAB_PCDS_FIXED_AT_BUILD or PcdItemType == TAB_PCDS_FEATURE_FLAG:
        key = ".".join((Pcd.TokenSpaceGuidCName,Pcd.TokenCName))
        
        AutoGenH.Append('extern const %s _gPcd_FixedAtBuild_%s%s;\n' %(DatumType, TokenCName, Array))
        AutoGenH.Append('#define %s  %s_gPcd_FixedAtBuild_%s\n' %(GetModeName, Type, TokenCName))
        AutoGenH.Append('//#define %s  ASSERT(FALSE)  // It is not allowed to set value for a FIXED_AT_BUILD PCD\n' % SetModeName)
        
        if PcdItemType == TAB_PCDS_FIXED_AT_BUILD and key in Info.ConstPcd:
            AutoGenH.Append('#define _PCD_VALUE_%s %s\n' %(TokenCName, Pcd.DefaultValue))
        


## Create code for library constructor
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreateLibraryConstructorCode(Info, AutoGenC, AutoGenH):
    #
    # Library Constructors
    #
    ConstructorPrototypeString = TemplateString()
    ConstructorCallingString = TemplateString()
    if Info.IsLibrary:
        DependentLibraryList = [Info.Module]
    else:
        DependentLibraryList = Info.DependentLibraryList
    for Lib in DependentLibraryList:
        if len(Lib.ConstructorList) <= 0:
            continue
        Dict = {'Function':Lib.ConstructorList}
        if Lib.ModuleType in ['BASE', 'SEC']:
            ConstructorPrototypeString.Append(gLibraryStructorPrototype['BASE'].Replace(Dict))
            ConstructorCallingString.Append(gLibraryStructorCall['BASE'].Replace(Dict))
        elif Lib.ModuleType in ['PEI_CORE','PEIM']:
            ConstructorPrototypeString.Append(gLibraryStructorPrototype['PEI'].Replace(Dict))
            ConstructorCallingString.Append(gLibraryStructorCall['PEI'].Replace(Dict))
        elif Lib.ModuleType in ['DXE_CORE','DXE_DRIVER','DXE_SMM_DRIVER','DXE_RUNTIME_DRIVER',
                                'DXE_SAL_DRIVER','UEFI_DRIVER','UEFI_APPLICATION','SMM_CORE']:
            ConstructorPrototypeString.Append(gLibraryStructorPrototype['DXE'].Replace(Dict))
            ConstructorCallingString.Append(gLibraryStructorCall['DXE'].Replace(Dict))

    if str(ConstructorPrototypeString) == '':
        ConstructorPrototypeList = []
    else:
        ConstructorPrototypeList = [str(ConstructorPrototypeString)]
    if str(ConstructorCallingString) == '':
        ConstructorCallingList = []
    else:
        ConstructorCallingList = [str(ConstructorCallingString)]

    Dict = {
        'Type'              :   'Constructor',
        'FunctionPrototype' :   ConstructorPrototypeList,
        'FunctionCall'      :   ConstructorCallingList
    }
    if Info.IsLibrary:
        AutoGenH.Append("${BEGIN}${FunctionPrototype}${END}", Dict)
    else:
        if Info.ModuleType in ['BASE', 'SEC']:
            AutoGenC.Append(gLibraryString['BASE'].Replace(Dict))
        elif Info.ModuleType in ['PEI_CORE','PEIM']:
            AutoGenC.Append(gLibraryString['PEI'].Replace(Dict))
        elif Info.ModuleType in ['DXE_CORE','DXE_DRIVER','DXE_SMM_DRIVER','DXE_RUNTIME_DRIVER',
                                 'DXE_SAL_DRIVER','UEFI_DRIVER','UEFI_APPLICATION','SMM_CORE']:
            AutoGenC.Append(gLibraryString['DXE'].Replace(Dict))

## Create code for library destructor
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreateLibraryDestructorCode(Info, AutoGenC, AutoGenH):
    #
    # Library Destructors
    #
    DestructorPrototypeString = TemplateString()
    DestructorCallingString = TemplateString()
    if Info.IsLibrary:
        DependentLibraryList = [Info.Module]
    else:
        DependentLibraryList = Info.DependentLibraryList
    for Index in range(len(DependentLibraryList)-1, -1, -1):
        Lib = DependentLibraryList[Index]
        if len(Lib.DestructorList) <= 0:
            continue
        Dict = {'Function':Lib.DestructorList}
        if Lib.ModuleType in ['BASE', 'SEC']:
            DestructorPrototypeString.Append(gLibraryStructorPrototype['BASE'].Replace(Dict))
            DestructorCallingString.Append(gLibraryStructorCall['BASE'].Replace(Dict))
        elif Lib.ModuleType in ['PEI_CORE','PEIM']:
            DestructorPrototypeString.Append(gLibraryStructorPrototype['PEI'].Replace(Dict))
            DestructorCallingString.Append(gLibraryStructorCall['PEI'].Replace(Dict))
        elif Lib.ModuleType in ['DXE_CORE','DXE_DRIVER','DXE_SMM_DRIVER','DXE_RUNTIME_DRIVER',
                                'DXE_SAL_DRIVER','UEFI_DRIVER','UEFI_APPLICATION', 'SMM_CORE']:
            DestructorPrototypeString.Append(gLibraryStructorPrototype['DXE'].Replace(Dict))
            DestructorCallingString.Append(gLibraryStructorCall['DXE'].Replace(Dict))

    if str(DestructorPrototypeString) == '':
        DestructorPrototypeList = []
    else:
        DestructorPrototypeList = [str(DestructorPrototypeString)]
    if str(DestructorCallingString) == '':
        DestructorCallingList = []
    else:
        DestructorCallingList = [str(DestructorCallingString)]

    Dict = {
        'Type'              :   'Destructor',
        'FunctionPrototype' :   DestructorPrototypeList,
        'FunctionCall'      :   DestructorCallingList
    }
    if Info.IsLibrary:
        AutoGenH.Append("${BEGIN}${FunctionPrototype}${END}", Dict)
    else:
        if Info.ModuleType in ['BASE', 'SEC']:
            AutoGenC.Append(gLibraryString['BASE'].Replace(Dict))
        elif Info.ModuleType in ['PEI_CORE','PEIM']:
            AutoGenC.Append(gLibraryString['PEI'].Replace(Dict))
        elif Info.ModuleType in ['DXE_CORE','DXE_DRIVER','DXE_SMM_DRIVER','DXE_RUNTIME_DRIVER',
                                 'DXE_SAL_DRIVER','UEFI_DRIVER','UEFI_APPLICATION','SMM_CORE']:
            AutoGenC.Append(gLibraryString['DXE'].Replace(Dict))


## Create code for ModuleEntryPoint
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreateModuleEntryPointCode(Info, AutoGenC, AutoGenH):
    if Info.IsLibrary or Info.ModuleType in ['USER_DEFINED', 'SEC']:
        return
    #
    # Module Entry Points
    #
    NumEntryPoints = len(Info.Module.ModuleEntryPointList)
    if 'PI_SPECIFICATION_VERSION' in Info.Module.Specification:
        PiSpecVersion = Info.Module.Specification['PI_SPECIFICATION_VERSION']
    else:
        PiSpecVersion = '0x00000000'
    if 'UEFI_SPECIFICATION_VERSION' in Info.Module.Specification:
        UefiSpecVersion = Info.Module.Specification['UEFI_SPECIFICATION_VERSION']
    else:
        UefiSpecVersion = '0x00000000'
    Dict = {
        'Function'       :   Info.Module.ModuleEntryPointList,
        'PiSpecVersion'  :   PiSpecVersion + 'U',
        'UefiSpecVersion':   UefiSpecVersion + 'U'
    }

    if Info.ModuleType in ['PEI_CORE', 'DXE_CORE', 'SMM_CORE']:
        if Info.SourceFileList <> None and Info.SourceFileList <> []:
          if NumEntryPoints != 1:
              EdkLogger.error(
                  "build",
                  AUTOGEN_ERROR,
                  '%s must have exactly one entry point' % Info.ModuleType,
                  File=str(Info),
                  ExtraData= ", ".join(Info.Module.ModuleEntryPointList)
                  )
    if Info.ModuleType == 'PEI_CORE':
        AutoGenC.Append(gPeiCoreEntryPointString.Replace(Dict))
        AutoGenH.Append(gPeiCoreEntryPointPrototype.Replace(Dict))
    elif Info.ModuleType == 'DXE_CORE':
        AutoGenC.Append(gDxeCoreEntryPointString.Replace(Dict))
        AutoGenH.Append(gDxeCoreEntryPointPrototype.Replace(Dict))
    elif Info.ModuleType == 'SMM_CORE':
        AutoGenC.Append(gSmmCoreEntryPointString.Replace(Dict))
        AutoGenH.Append(gSmmCoreEntryPointPrototype.Replace(Dict))
    elif Info.ModuleType == 'PEIM':
        if NumEntryPoints < 2:
            AutoGenC.Append(gPeimEntryPointString[NumEntryPoints].Replace(Dict))
        else:
            AutoGenC.Append(gPeimEntryPointString[2].Replace(Dict))
        AutoGenH.Append(gPeimEntryPointPrototype.Replace(Dict))
    elif Info.ModuleType in ['DXE_RUNTIME_DRIVER','DXE_DRIVER','DXE_SAL_DRIVER','UEFI_DRIVER']:
        if NumEntryPoints < 2:
            AutoGenC.Append(gUefiDriverEntryPointString[NumEntryPoints].Replace(Dict))
        else:
            AutoGenC.Append(gUefiDriverEntryPointString[2].Replace(Dict))
        AutoGenH.Append(gUefiDriverEntryPointPrototype.Replace(Dict))
    elif Info.ModuleType == 'DXE_SMM_DRIVER':
        if NumEntryPoints == 0:
            AutoGenC.Append(gDxeSmmEntryPointString[0].Replace(Dict))
        else:
            AutoGenC.Append(gDxeSmmEntryPointString[1].Replace(Dict))
        AutoGenH.Append(gDxeSmmEntryPointPrototype.Replace(Dict))    
    elif Info.ModuleType == 'UEFI_APPLICATION':
        if NumEntryPoints < 2:
            AutoGenC.Append(gUefiApplicationEntryPointString[NumEntryPoints].Replace(Dict))
        else:
            AutoGenC.Append(gUefiApplicationEntryPointString[2].Replace(Dict))
        AutoGenH.Append(gUefiApplicationEntryPointPrototype.Replace(Dict))

## Create code for ModuleUnloadImage
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreateModuleUnloadImageCode(Info, AutoGenC, AutoGenH):
    if Info.IsLibrary or Info.ModuleType in ['USER_DEFINED', 'SEC']:
        return
    #
    # Unload Image Handlers
    #
    NumUnloadImage = len(Info.Module.ModuleUnloadImageList)
    Dict = {'Count':str(NumUnloadImage) + 'U', 'Function':Info.Module.ModuleUnloadImageList}
    if NumUnloadImage < 2:
        AutoGenC.Append(gUefiUnloadImageString[NumUnloadImage].Replace(Dict))
    else:
        AutoGenC.Append(gUefiUnloadImageString[2].Replace(Dict))
    AutoGenH.Append(gUefiUnloadImagePrototype.Replace(Dict))

## Create code for GUID
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreateGuidDefinitionCode(Info, AutoGenC, AutoGenH):
    if Info.IsLibrary:
        return

    if Info.ModuleType in ["USER_DEFINED", "BASE"]:
        GuidType = "GUID"
    else:
        GuidType = "EFI_GUID"

    if Info.GuidList:
        AutoGenC.Append("\n// Guids\n")
        AutoGenH.Append("\n// Guids\n")
    #
    # GUIDs
    #
    for Key in Info.GuidList:
        AutoGenC.Append('GLOBAL_REMOVE_IF_UNREFERENCED %s %s = %s;\n' % (GuidType, Key, Info.GuidList[Key]))
        AutoGenH.Append('extern %s %s;\n' % (GuidType, Key))

## Create code for protocol
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreateProtocolDefinitionCode(Info, AutoGenC, AutoGenH):
    if Info.IsLibrary:
        return

    if Info.ModuleType in ["USER_DEFINED", "BASE"]:
        GuidType = "GUID"
    else:
        GuidType = "EFI_GUID"

    if Info.ProtocolList:
        AutoGenC.Append("\n// Protocols\n")
        AutoGenH.Append("\n// Protocols\n")
    #
    # Protocol GUIDs
    #
    for Key in Info.ProtocolList:
        AutoGenC.Append('GLOBAL_REMOVE_IF_UNREFERENCED %s %s = %s;\n' % (GuidType, Key, Info.ProtocolList[Key]))
        AutoGenH.Append('extern %s %s;\n' % (GuidType, Key))

## Create code for PPI
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreatePpiDefinitionCode(Info, AutoGenC, AutoGenH):
    if Info.IsLibrary:
        return

    if Info.ModuleType in ["USER_DEFINED", "BASE"]:
        GuidType = "GUID"
    else:
        GuidType = "EFI_GUID"

    if Info.PpiList:
        AutoGenC.Append("\n// PPIs\n")
        AutoGenH.Append("\n// PPIs\n")
    #
    # PPI GUIDs
    #
    for Key in Info.PpiList:
        AutoGenC.Append('GLOBAL_REMOVE_IF_UNREFERENCED %s %s = %s;\n' % (GuidType, Key, Info.PpiList[Key]))
        AutoGenH.Append('extern %s %s;\n' % (GuidType, Key))

## Create code for PCD
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreatePcdCode(Info, AutoGenC, AutoGenH):

    # Collect Token Space GUIDs used by DynamicEc PCDs
    TokenSpaceList = []
    for Pcd in Info.ModulePcdList:
        if Pcd.Type in gDynamicExPcd and Pcd.TokenSpaceGuidCName not in TokenSpaceList:
            TokenSpaceList += [Pcd.TokenSpaceGuidCName]
            
    # Add extern declarations to AutoGen.h if one or more Token Space GUIDs were found
    if TokenSpaceList <> []:            
        AutoGenH.Append("\n// Definition of PCD Token Space GUIDs used in this module\n\n")
        if Info.ModuleType in ["USER_DEFINED", "BASE"]:
            GuidType = "GUID"
        else:
            GuidType = "EFI_GUID"              
        for Item in TokenSpaceList:
            AutoGenH.Append('extern %s %s;\n' % (GuidType, Item))

    if Info.IsLibrary:
        if Info.ModulePcdList:
            AutoGenH.Append("\n// PCD definitions\n")
        for Pcd in Info.ModulePcdList:
            CreateLibraryPcdCode(Info, AutoGenC, AutoGenH, Pcd)
        DynExPcdTokenNumberMapping (Info, AutoGenH)
    else:
        if Info.ModulePcdList:
            AutoGenH.Append("\n// Definition of PCDs used in this module\n")
            AutoGenC.Append("\n// Definition of PCDs used in this module\n")
        for Pcd in Info.ModulePcdList:
            CreateModulePcdCode(Info, AutoGenC, AutoGenH, Pcd)
        DynExPcdTokenNumberMapping (Info, AutoGenH)
        if Info.LibraryPcdList:
            AutoGenH.Append("\n// Definition of PCDs used in libraries is in AutoGen.c\n")
            AutoGenC.Append("\n// Definition of PCDs used in libraries\n")
        for Pcd in Info.LibraryPcdList:
            CreateModulePcdCode(Info, AutoGenC, AutoGenC, Pcd)
    CreatePcdDatabaseCode(Info, AutoGenC, AutoGenH)

## Create code for unicode string definition
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#   @param      UniGenCFlag     UniString is generated into AutoGen C file when it is set to True
#   @param      UniGenBinBuffer Buffer to store uni string package data
#
def CreateUnicodeStringCode(Info, AutoGenC, AutoGenH, UniGenCFlag, UniGenBinBuffer):
    WorkingDir = os.getcwd()
    os.chdir(Info.WorkspaceDir)

    IncList = [Info.MetaFile.Dir]
    # Get all files under [Sources] section in inf file for EDK-II module
    EDK2Module = True
    SrcList = [F for F in Info.SourceFileList]
    if Info.AutoGenVersion < 0x00010005:
        EDK2Module = False
        # Get all files under the module directory for EDK-I module
        Cwd = os.getcwd()
        os.chdir(Info.MetaFile.Dir)
        for Root, Dirs, Files in os.walk("."):
            if 'CVS' in Dirs:
                Dirs.remove('CVS')
            if '.svn' in Dirs:
                Dirs.remove('.svn')
            for File in Files:
                File = PathClass(os.path.join(Root, File), Info.MetaFile.Dir)
                if File in SrcList:
                    continue
                SrcList.append(File)
        os.chdir(Cwd)

    if 'BUILD' in Info.BuildOption and Info.BuildOption['BUILD']['FLAGS'].find('-c') > -1:
        CompatibleMode = True
    else:
        CompatibleMode = False

    #
    # -s is a temporary option dedicated for building .UNI files with ISO 639-2 language codes of EDK Shell in EDK2
    #
    if 'BUILD' in Info.BuildOption and Info.BuildOption['BUILD']['FLAGS'].find('-s') > -1:
        if CompatibleMode:
            EdkLogger.error("build", AUTOGEN_ERROR,
                            "-c and -s build options should be used exclusively",
                            ExtraData="[%s]" % str(Info))
        ShellMode = True
    else:
        ShellMode = False

    #RFC4646 is only for EDKII modules and ISO639-2 for EDK modules
    if EDK2Module:
        FilterInfo = [EDK2Module] + [Info.PlatformInfo.Platform.RFCLanguages]
    else:
        FilterInfo = [EDK2Module] + [Info.PlatformInfo.Platform.ISOLanguages]
    Header, Code = GetStringFiles(Info.UnicodeFileList, SrcList, IncList, Info.IncludePathList, ['.uni', '.inf'], Info.Name, CompatibleMode, ShellMode, UniGenCFlag, UniGenBinBuffer, FilterInfo)
    if CompatibleMode or UniGenCFlag:
        AutoGenC.Append("\n//\n//Unicode String Pack Definition\n//\n")
        AutoGenC.Append(Code)
        AutoGenC.Append("\n")
    AutoGenH.Append("\n//\n//Unicode String ID\n//\n")
    AutoGenH.Append(Header)
    if CompatibleMode or UniGenCFlag:
        AutoGenH.Append("\n#define STRING_ARRAY_NAME %sStrings\n" % Info.Name)
    os.chdir(WorkingDir)

## Create common code
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreateHeaderCode(Info, AutoGenC, AutoGenH):
    # file header
    AutoGenH.Append(gAutoGenHeaderString.Replace({'FileName':'AutoGen.h'}))
    # header file Prologue
    AutoGenH.Append(gAutoGenHPrologueString.Replace({'File':'AUTOGENH','Guid':Info.Guid.replace('-','_')}))
    AutoGenH.Append(gAutoGenHCppPrologueString)
    if Info.AutoGenVersion >= 0x00010005:
        # header files includes
        AutoGenH.Append("#include <%s>\n" % gBasicHeaderFile)
        if Info.ModuleType in gModuleTypeHeaderFile \
           and gModuleTypeHeaderFile[Info.ModuleType][0] != gBasicHeaderFile:
            AutoGenH.Append("#include <%s>\n" % gModuleTypeHeaderFile[Info.ModuleType][0])
        #
        # if either PcdLib in [LibraryClasses] sections or there exist Pcd section, add PcdLib.h 
        # As if modules only uses FixedPcd, then PcdLib is not needed in [LibraryClasses] section.
        #
        if 'PcdLib' in Info.Module.LibraryClasses or Info.Module.Pcds:
            AutoGenH.Append("#include <Library/PcdLib.h>\n")

        AutoGenH.Append('\nextern GUID  gEfiCallerIdGuid;')
        AutoGenH.Append('\nextern CHAR8 *gEfiCallerBaseName;\n\n')

        if Info.IsLibrary:
            return

        AutoGenH.Append("#define EFI_CALLER_ID_GUID \\\n  %s\n" % GuidStringToGuidStructureString(Info.Guid))

    if Info.IsLibrary:
        return
    # C file header
    AutoGenC.Append(gAutoGenHeaderString.Replace({'FileName':'AutoGen.c'}))
    if Info.AutoGenVersion >= 0x00010005:
        # C file header files includes
        if Info.ModuleType in gModuleTypeHeaderFile:
            for Inc in gModuleTypeHeaderFile[Info.ModuleType]:
                AutoGenC.Append("#include <%s>\n" % Inc)
        else:
            AutoGenC.Append("#include <%s>\n" % gBasicHeaderFile)

        #
        # Publish the CallerId Guid
        #
        AutoGenC.Append('\nGLOBAL_REMOVE_IF_UNREFERENCED GUID gEfiCallerIdGuid = %s;\n' % GuidStringToGuidStructureString(Info.Guid))
        AutoGenC.Append('\nGLOBAL_REMOVE_IF_UNREFERENCED CHAR8 *gEfiCallerBaseName = "%s";\n' % Info.Name)

## Create common code for header file
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreateFooterCode(Info, AutoGenC, AutoGenH):
    AutoGenH.Append(gAutoGenHEpilogueString)

## Create code for a module
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#   @param      UniGenCFlag     UniString is generated into AutoGen C file when it is set to True
#   @param      UniGenBinBuffer Buffer to store uni string package data
#
def CreateCode(Info, AutoGenC, AutoGenH, StringH, UniGenCFlag, UniGenBinBuffer):
    CreateHeaderCode(Info, AutoGenC, AutoGenH)

    if Info.AutoGenVersion >= 0x00010005:
        CreateGuidDefinitionCode(Info, AutoGenC, AutoGenH)
        CreateProtocolDefinitionCode(Info, AutoGenC, AutoGenH)
        CreatePpiDefinitionCode(Info, AutoGenC, AutoGenH)
        CreatePcdCode(Info, AutoGenC, AutoGenH)
        CreateLibraryConstructorCode(Info, AutoGenC, AutoGenH)
        CreateLibraryDestructorCode(Info, AutoGenC, AutoGenH)
        CreateModuleEntryPointCode(Info, AutoGenC, AutoGenH)
        CreateModuleUnloadImageCode(Info, AutoGenC, AutoGenH)

    if Info.UnicodeFileList:
        FileName = "%sStrDefs.h" % Info.Name
        StringH.Append(gAutoGenHeaderString.Replace({'FileName':FileName}))
        StringH.Append(gAutoGenHPrologueString.Replace({'File':'STRDEFS', 'Guid':Info.Guid.replace('-','_')}))
        CreateUnicodeStringCode(Info, AutoGenC, StringH, UniGenCFlag, UniGenBinBuffer)

        GuidMacros = []
        for Guid in Info.Module.Guids:
            if Guid in Info.Module.GetGuidsUsedByPcd():
                continue
            GuidMacros.append('#define %s %s' % (Guid, Info.Module.Guids[Guid]))
        for Guid, Value in Info.Module.Protocols.items() + Info.Module.Ppis.items():
            GuidMacros.append('#define %s %s' % (Guid, Value))
        if GuidMacros:
            StringH.Append('\n#ifdef VFRCOMPILE\n%s\n#endif\n' % '\n'.join(GuidMacros))

        StringH.Append("\n#endif\n")
        AutoGenH.Append('#include "%s"\n' % FileName)

    CreateFooterCode(Info, AutoGenC, AutoGenH)

    # no generation of AutoGen.c for Edk modules without unicode file
    if Info.AutoGenVersion < 0x00010005 and len(Info.UnicodeFileList) == 0:
        AutoGenC.String = ''

## Create the code file
#
#   @param      FilePath     The path of code file
#   @param      Content      The content of code file
#   @param      IsBinaryFile The flag indicating if the file is binary file or not
#
#   @retval     True        If file content is changed or file doesn't exist
#   @retval     False       If the file exists and the content is not changed
#
def Generate(FilePath, Content, IsBinaryFile):
    return SaveFileOnChange(FilePath, Content, IsBinaryFile)

