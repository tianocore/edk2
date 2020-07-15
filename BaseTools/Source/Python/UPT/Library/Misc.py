## @file
# Common routines used by all tools
#
# Copyright (c) 2011 - 2019, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
Misc
'''

##
# Import Modules
#
import os.path
from os import access
from os import F_OK
from os import makedirs
from os import getcwd
from os import chdir
from os import listdir
from os import remove
from os import rmdir
from os import linesep
from os import walk
from os import environ
import re
from collections import OrderedDict as Sdict

import Logger.Log as Logger
from Logger import StringTable as ST
from Logger import ToolError
from Library import GlobalData
from Library.DataType import SUP_MODULE_LIST
from Library.DataType import END_OF_LINE
from Library.DataType import TAB_SPLIT
from Library.DataType import TAB_LANGUAGE_EN_US
from Library.DataType import TAB_LANGUAGE_EN
from Library.DataType import TAB_LANGUAGE_EN_X
from Library.DataType import TAB_UNI_FILE_SUFFIXS
from Library.StringUtils import GetSplitValueList
from Library.ParserValidate import IsValidHexVersion
from Library.ParserValidate import IsValidPath
from Object.POM.CommonObject import TextObject
from Core.FileHook import __FileHookOpen__
from Common.MultipleWorkspace import MultipleWorkspace as mws

## Convert GUID string in xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx style to C
# structure style
#
# @param      Guid:    The GUID string
#
def GuidStringToGuidStructureString(Guid):
    GuidList = Guid.split('-')
    Result = '{'
    for Index in range(0, 3, 1):
        Result = Result + '0x' + GuidList[Index] + ', '
    Result = Result + '{0x' + GuidList[3][0:2] + ', 0x' + GuidList[3][2:4]
    for Index in range(0, 12, 2):
        Result = Result + ', 0x' + GuidList[4][Index:Index + 2]
    Result += '}}'
    return Result

## Check whether GUID string is of format xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
#
# @param      GuidValue:   The GUID value
#
def CheckGuidRegFormat(GuidValue):
    ## Regular expression used to find out register format of GUID
    #
    RegFormatGuidPattern = re.compile("^\s*([0-9a-fA-F]){8}-"
                                       "([0-9a-fA-F]){4}-"
                                       "([0-9a-fA-F]){4}-"
                                       "([0-9a-fA-F]){4}-"
                                       "([0-9a-fA-F]){12}\s*$")

    if RegFormatGuidPattern.match(GuidValue):
        return True
    else:
        return False


## Convert GUID string in C structure style to
# xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
#
# @param      GuidValue:   The GUID value in C structure format
#
def GuidStructureStringToGuidString(GuidValue):
    GuidValueString = GuidValue.lower().replace("{", "").replace("}", "").\
    replace(" ", "").replace(";", "")
    GuidValueList = GuidValueString.split(",")
    if len(GuidValueList) != 11:
        return ''
    try:
        return "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x" % (
                int(GuidValueList[0], 16),
                int(GuidValueList[1], 16),
                int(GuidValueList[2], 16),
                int(GuidValueList[3], 16),
                int(GuidValueList[4], 16),
                int(GuidValueList[5], 16),
                int(GuidValueList[6], 16),
                int(GuidValueList[7], 16),
                int(GuidValueList[8], 16),
                int(GuidValueList[9], 16),
                int(GuidValueList[10], 16)
                )
    except BaseException:
        return ''

## Create directories
#
# @param      Directory:   The directory name
#
def CreateDirectory(Directory):
    if Directory is None or Directory.strip() == "":
        return True
    try:
        if not access(Directory, F_OK):
            makedirs(Directory)
    except BaseException:
        return False
    return True

## Remove directories, including files and sub-directories in it
#
# @param      Directory:   The directory name
#
def RemoveDirectory(Directory, Recursively=False):
    if Directory is None or Directory.strip() == "" or not \
    os.path.exists(Directory):
        return
    if Recursively:
        CurrentDirectory = getcwd()
        chdir(Directory)
        for File in listdir("."):
            if os.path.isdir(File):
                RemoveDirectory(File, Recursively)
            else:
                remove(File)
        chdir(CurrentDirectory)
    rmdir(Directory)

## Store content in file
#
# This method is used to save file only when its content is changed. This is
# quite useful for "make" system to decide what will be re-built and what
# won't.
#
# @param      File:            The path of file
# @param      Content:         The new content of the file
# @param      IsBinaryFile:    The flag indicating if the file is binary file
#                              or not
#
def SaveFileOnChange(File, Content, IsBinaryFile=True):
    if os.path.exists(File):
        if IsBinaryFile:
            try:
                if Content == __FileHookOpen__(File, "rb").read():
                    return False
            except BaseException:
                Logger.Error(None, ToolError.FILE_OPEN_FAILURE, ExtraData=File)
        else:
            try:
                if Content == __FileHookOpen__(File, "r").read():
                    return False
            except BaseException:
                Logger.Error(None, ToolError.FILE_OPEN_FAILURE, ExtraData=File)

    CreateDirectory(os.path.dirname(File))
    if IsBinaryFile:
        try:
            FileFd = __FileHookOpen__(File, "wb")
            FileFd.write(Content)
            FileFd.close()
        except BaseException:
            Logger.Error(None, ToolError.FILE_CREATE_FAILURE, ExtraData=File)
    else:
        try:
            FileFd = __FileHookOpen__(File, "w")
            FileFd.write(Content)
            FileFd.close()
        except BaseException:
            Logger.Error(None, ToolError.FILE_CREATE_FAILURE, ExtraData=File)

    return True

## Get all files of a directory
#
# @param Root:       Root dir
# @param SkipList :  The files need be skipped
#
def GetFiles(Root, SkipList=None, FullPath=True):
    OriPath = os.path.normpath(Root)
    FileList = []
    for Root, Dirs, Files in walk(Root):
        if SkipList:
            for Item in SkipList:
                if Item in Dirs:
                    Dirs.remove(Item)
                if Item in Files:
                    Files.remove(Item)
        for Dir in Dirs:
            if Dir.startswith('.'):
                Dirs.remove(Dir)

        for File in Files:
            if File.startswith('.'):
                continue
            File = os.path.normpath(os.path.join(Root, File))
            if not FullPath:
                File = File[len(OriPath) + 1:]
            FileList.append(File)

    return FileList

## Get all non-metadata files of a directory
#
# @param Root:       Root Dir
# @param SkipList :  List of path need be skipped
# @param FullPath:  True if the returned file should be full path
# @param PrefixPath: the path that need to be added to the files found
# @return: the list of files found
#
def GetNonMetaDataFiles(Root, SkipList, FullPath, PrefixPath):
    FileList = GetFiles(Root, SkipList, FullPath)
    NewFileList = []
    for File in FileList:
        ExtName = os.path.splitext(File)[1]
        #
        # skip '.dec', '.inf', '.dsc', '.fdf' files
        #
        if ExtName.lower() not in ['.dec', '.inf', '.dsc', '.fdf']:
            NewFileList.append(os.path.normpath(os.path.join(PrefixPath, File)))

    return NewFileList

## Check if given file exists or not
#
# @param      File:    File name or path to be checked
# @param      Dir:     The directory the file is relative to
#
def ValidFile(File, Ext=None):
    File = File.replace('\\', '/')
    if Ext is not None:
        FileExt = os.path.splitext(File)[1]
        if FileExt.lower() != Ext.lower():
            return False
    if not os.path.exists(File):
        return False
    return True

## RealPath
#
# @param      File:    File name or path to be checked
# @param      Dir:     The directory the file is relative to
# @param      OverrideDir:     The override directory
#
def RealPath(File, Dir='', OverrideDir=''):
    NewFile = os.path.normpath(os.path.join(Dir, File))
    NewFile = GlobalData.gALL_FILES[NewFile]
    if not NewFile and OverrideDir:
        NewFile = os.path.normpath(os.path.join(OverrideDir, File))
        NewFile = GlobalData.gALL_FILES[NewFile]
    return NewFile

## RealPath2
#
# @param      File:    File name or path to be checked
# @param      Dir:     The directory the file is relative to
# @param      OverrideDir:     The override directory
#
def RealPath2(File, Dir='', OverrideDir=''):
    if OverrideDir:
        NewFile = GlobalData.gALL_FILES[os.path.normpath(os.path.join\
                                                        (OverrideDir, File))]
        if NewFile:
            if OverrideDir[-1] == os.path.sep:
                return NewFile[len(OverrideDir):], NewFile[0:len(OverrideDir)]
            else:
                return NewFile[len(OverrideDir) + 1:], \
            NewFile[0:len(OverrideDir)]

    NewFile = GlobalData.gALL_FILES[os.path.normpath(os.path.join(Dir, File))]
    if NewFile:
        if Dir:
            if Dir[-1] == os.path.sep:
                return NewFile[len(Dir):], NewFile[0:len(Dir)]
            else:
                return NewFile[len(Dir) + 1:], NewFile[0:len(Dir)]
        else:
            return NewFile, ''

    return None, None

## CommonPath
#
# @param PathList: PathList
#
def CommonPath(PathList):
    Path1 = min(PathList).split(os.path.sep)
    Path2 = max(PathList).split(os.path.sep)
    for Index in range(min(len(Path1), len(Path2))):
        if Path1[Index] != Path2[Index]:
            return os.path.sep.join(Path1[:Index])
    return os.path.sep.join(Path1)

## PathClass
#
class PathClass(object):
    def __init__(self, File='', Root='', AlterRoot='', Type='', IsBinary=False,
                 Arch='COMMON', ToolChainFamily='', Target='', TagName='', \
                 ToolCode=''):
        self.Arch = Arch
        self.File = str(File)
        if os.path.isabs(self.File):
            self.Root = ''
            self.AlterRoot = ''
        else:
            self.Root = str(Root)
            self.AlterRoot = str(AlterRoot)

        #
        # Remove any '.' and '..' in path
        #
        if self.Root:
            self.Path = os.path.normpath(os.path.join(self.Root, self.File))
            self.Root = os.path.normpath(CommonPath([self.Root, self.Path]))
            #
            # eliminate the side-effect of 'C:'
            #
            if self.Root[-1] == ':':
                self.Root += os.path.sep
            #
            # file path should not start with path separator
            #
            if self.Root[-1] == os.path.sep:
                self.File = self.Path[len(self.Root):]
            else:
                self.File = self.Path[len(self.Root) + 1:]
        else:
            self.Path = os.path.normpath(self.File)

        self.SubDir, self.Name = os.path.split(self.File)
        self.BaseName, self.Ext = os.path.splitext(self.Name)

        if self.Root:
            if self.SubDir:
                self.Dir = os.path.join(self.Root, self.SubDir)
            else:
                self.Dir = self.Root
        else:
            self.Dir = self.SubDir

        if IsBinary:
            self.Type = Type
        else:
            self.Type = self.Ext.lower()

        self.IsBinary = IsBinary
        self.Target = Target
        self.TagName = TagName
        self.ToolCode = ToolCode
        self.ToolChainFamily = ToolChainFamily

        self._Key = None

    ## Convert the object of this class to a string
    #
    #  Convert member Path of the class to a string
    #
    def __str__(self):
        return self.Path

    ## Override __eq__ function
    #
    # Check whether PathClass are the same
    #
    def __eq__(self, Other):
        if isinstance(Other, type(self)):
            return self.Path == Other.Path
        else:
            return self.Path == str(Other)

    ## Override __hash__ function
    #
    # Use Path as key in hash table
    #
    def __hash__(self):
        return hash(self.Path)

    ## _GetFileKey
    #
    def _GetFileKey(self):
        if self._Key is None:
            self._Key = self.Path.upper()
        return self._Key
    ## Validate
    #
    def Validate(self, Type='', CaseSensitive=True):
        if GlobalData.gCASE_INSENSITIVE:
            CaseSensitive = False
        if Type and Type.lower() != self.Type:
            return ToolError.FILE_TYPE_MISMATCH, '%s (expect %s but got %s)' % \
        (self.File, Type, self.Type)

        RealFile, RealRoot = RealPath2(self.File, self.Root, self.AlterRoot)
        if not RealRoot and not RealFile:
            RealFile = self.File
            if self.AlterRoot:
                RealFile = os.path.join(self.AlterRoot, self.File)
            elif self.Root:
                RealFile = os.path.join(self.Root, self.File)
            return ToolError.FILE_NOT_FOUND, os.path.join(self.AlterRoot, RealFile)

        ErrorCode = 0
        ErrorInfo = ''
        if RealRoot != self.Root or RealFile != self.File:
            if CaseSensitive and (RealFile != self.File or \
                                  (RealRoot != self.Root and RealRoot != \
                                   self.AlterRoot)):
                ErrorCode = ToolError.FILE_CASE_MISMATCH
                ErrorInfo = self.File + '\n\t' + RealFile + \
                 " [in file system]"

            self.SubDir, self.Name = os.path.split(RealFile)
            self.BaseName, self.Ext = os.path.splitext(self.Name)
            if self.SubDir:
                self.Dir = os.path.join(RealRoot, self.SubDir)
            else:
                self.Dir = RealRoot
            self.File = RealFile
            self.Root = RealRoot
            self.Path = os.path.join(RealRoot, RealFile)
        return ErrorCode, ErrorInfo

    Key = property(_GetFileKey)

## Get current workspace
#
#  get WORKSPACE from environment variable if present,if not use current working directory as WORKSPACE
#
def GetWorkspace():
    #
    # check WORKSPACE
    #
    if "WORKSPACE" in environ:
        WorkspaceDir = os.path.normpath(environ["WORKSPACE"])
        if not os.path.exists(WorkspaceDir):
            Logger.Error("UPT",
                         ToolError.UPT_ENVIRON_MISSING_ERROR,
                         ST.ERR_WORKSPACE_NOTEXIST,
                         ExtraData="%s" % WorkspaceDir)
    else:
        WorkspaceDir = os.getcwd()

    if WorkspaceDir[-1] == ':':
        WorkspaceDir += os.sep

    PackagesPath = os.environ.get("PACKAGES_PATH")
    mws.setWs(WorkspaceDir, PackagesPath)

    return WorkspaceDir, mws.PACKAGES_PATH

## Get relative path
#
#  use full path and workspace to get relative path
#  the destination of this function is mainly to resolve the root path issue(like c: or c:\)
#
#  @param Fullpath: a string of fullpath
#  @param Workspace: a string of workspace
#
def GetRelativePath(Fullpath, Workspace):

    RelativePath = ''
    if Workspace.endswith(os.sep):
        RelativePath = Fullpath[Fullpath.upper().find(Workspace.upper())+len(Workspace):]
    else:
        RelativePath = Fullpath[Fullpath.upper().find(Workspace.upper())+len(Workspace)+1:]

    return RelativePath

## Check whether all module types are in list
#
# check whether all module types (SUP_MODULE_LIST) are in list
#
# @param ModuleList:  a list of ModuleType
#
def IsAllModuleList(ModuleList):
    NewModuleList = [Module.upper() for Module in ModuleList]
    for Module in SUP_MODULE_LIST:
        if Module not in NewModuleList:
            return False
    else:
        return True

## Dictionary that use comment(GenericComment, TailComment) as value,
# if a new comment which key already in the dic is inserted, then the
# comment will be merged.
# Key is (Statement, SupArch), when TailComment is added, it will ident
# according to Statement
#
class MergeCommentDict(dict):
    ## []= operator
    #
    def __setitem__(self, Key, CommentVal):
        GenericComment, TailComment = CommentVal
        if Key in self:
            OrigVal1, OrigVal2 = dict.__getitem__(self, Key)
            Statement = Key[0]
            dict.__setitem__(self, Key, (OrigVal1 + GenericComment, OrigVal2 \
                                         + len(Statement) * ' ' + TailComment))
        else:
            dict.__setitem__(self, Key, (GenericComment, TailComment))

    ## =[] operator
    #
    def __getitem__(self, Key):
        return dict.__getitem__(self, Key)


## GenDummyHelpTextObj
#
# @retval HelpTxt:   Generated dummy help text object
#
def GenDummyHelpTextObj():
    HelpTxt = TextObject()
    HelpTxt.SetLang(TAB_LANGUAGE_EN_US)
    HelpTxt.SetString(' ')
    return HelpTxt

## ConvertVersionToDecimal, the minor version should be within 0 - 99
# <HexVersion>          ::=  "0x" <Major> <Minor>
# <Major>               ::=  (a-fA-F0-9){4}
# <Minor>               ::=  (a-fA-F0-9){4}
# <DecVersion>          ::=  (0-65535) ["." (0-99)]
#
# @param StringIn:  The string contains version defined in INF file.
#                   It can be Decimal or Hex
#
def ConvertVersionToDecimal(StringIn):
    if IsValidHexVersion(StringIn):
        Value = int(StringIn, 16)
        Major = Value >> 16
        Minor = Value & 0xFFFF
        MinorStr = str(Minor)
        if len(MinorStr) == 1:
            MinorStr = '0' + MinorStr
        return str(Major) + '.' + MinorStr
    else:
        if StringIn.find(TAB_SPLIT) != -1:
            return StringIn
        elif StringIn:
            return StringIn + '.0'
        else:
            #
            # when StringIn is '', return it directly
            #
            return StringIn

## GetHelpStringByRemoveHashKey
#
# Remove hash key at the header of string and return the remain.
#
# @param String:  The string need to be processed.
#
def GetHelpStringByRemoveHashKey(String):
    ReturnString = ''
    PattenRemoveHashKey = re.compile(r"^[#+\s]+", re.DOTALL)
    String = String.strip()
    if String == '':
        return String

    LineList = GetSplitValueList(String, END_OF_LINE)
    for Line in LineList:
        ValueList = PattenRemoveHashKey.split(Line)
        if len(ValueList) == 1:
            ReturnString += ValueList[0] + END_OF_LINE
        else:
            ReturnString += ValueList[1] + END_OF_LINE

    if ReturnString.endswith('\n') and not ReturnString.endswith('\n\n') and ReturnString != '\n':
        ReturnString = ReturnString[:-1]

    return ReturnString

## ConvPathFromAbsToRel
#
# Get relative file path from absolute path.
#
# @param Path:  The string contain file absolute path.
# @param Root:  The string contain the parent path of Path in.
#
#
def ConvPathFromAbsToRel(Path, Root):
    Path = os.path.normpath(Path)
    Root = os.path.normpath(Root)
    FullPath = os.path.normpath(os.path.join(Root, Path))

    #
    # If Path is absolute path.
    # It should be in Root.
    #
    if os.path.isabs(Path):
        return FullPath[FullPath.find(Root) + len(Root) + 1:]

    else:
        return Path

## ConvertPath
#
# Convert special characters to '_', '\' to '/'
# return converted path: Test!1.inf -> Test_1.inf
#
# @param Path: Path to be converted
#
def ConvertPath(Path):
    RetPath = ''
    for Char in Path.strip():
        if Char.isalnum() or Char in '.-_/':
            RetPath = RetPath + Char
        elif Char == '\\':
            RetPath = RetPath + '/'
        else:
            RetPath = RetPath + '_'
    return RetPath

## ConvertSpec
#
# during install, convert the Spec string extract from UPD into INF allowable definition,
# the difference is period is allowed in the former (not the first letter) but not in the latter.
# return converted Spec string
#
# @param SpecStr: SpecStr to be converted
#
def ConvertSpec(SpecStr):
    RetStr = ''
    for Char in SpecStr:
        if Char.isalnum() or Char == '_':
            RetStr = RetStr + Char
        else:
            RetStr = RetStr + '_'

    return RetStr


## IsEqualList
#
# Judge two lists are identical(contain same item).
# The rule is elements in List A are in List B and elements in List B are in List A.
#
# @param ListA, ListB  Lists need to be judged.
#
# @return True  ListA and ListB are identical
# @return False ListA and ListB are different with each other
#
def IsEqualList(ListA, ListB):
    if ListA == ListB:
        return True

    for ItemA in ListA:
        if not ItemA in ListB:
            return False

    for ItemB in ListB:
        if not ItemB in ListA:
            return False

    return True

## ConvertArchList
#
# Convert item in ArchList if the start character is lower case.
# In UDP spec, Arch is only allowed as: [A-Z]([a-zA-Z0-9])*
#
# @param ArchList The ArchList need to be converted.
#
# @return NewList  The ArchList been converted.
#
def ConvertArchList(ArchList):
    NewArchList = []
    if not ArchList:
        return NewArchList

    if isinstance(ArchList, list):
        for Arch in ArchList:
            Arch = Arch.upper()
            NewArchList.append(Arch)
    elif isinstance(ArchList, str):
        ArchList = ArchList.upper()
        NewArchList.append(ArchList)

    return NewArchList

## ProcessLineExtender
#
# Process the LineExtender of Line in LineList.
# If one line ends with a line extender, then it will be combined together with next line.
#
# @param LineList The LineList need to be processed.
#
# @return NewList  The ArchList been processed.
#
def ProcessLineExtender(LineList):
    NewList = []
    Count = 0
    while Count < len(LineList):
        if LineList[Count].strip().endswith("\\") and Count + 1 < len(LineList):
            NewList.append(LineList[Count].strip()[:-2] + LineList[Count + 1])
            Count = Count + 1
        else:
            NewList.append(LineList[Count])

        Count = Count + 1

    return NewList

## ProcessEdkComment
#
# Process EDK style comment in LineList: c style /* */ comment or cpp style // comment
#
#
# @param LineList The LineList need to be processed.
#
# @return LineList  The LineList been processed.
# @return FirstPos  Where Edk comment is first found, -1 if not found
#
def ProcessEdkComment(LineList):
    FindEdkBlockComment = False
    Count = 0
    StartPos = -1
    EndPos = -1
    FirstPos = -1

    while(Count < len(LineList)):
        Line = LineList[Count].strip()
        if Line.startswith("/*"):
            #
            # handling c style comment
            #
            StartPos = Count
            while Count < len(LineList):
                Line = LineList[Count].strip()
                if Line.endswith("*/"):
                    if (Count == StartPos) and Line.strip() == '/*/':
                        Count = Count + 1
                        continue
                    EndPos = Count
                    FindEdkBlockComment = True
                    break
                Count = Count + 1

            if FindEdkBlockComment:
                if FirstPos == -1:
                    FirstPos = StartPos
                for Index in range(StartPos, EndPos+1):
                    LineList[Index] = ''
                FindEdkBlockComment = False
        elif Line.find("//") != -1 and not Line.startswith("#"):
            #
            # handling cpp style comment
            #
            LineList[Count] = Line.replace("//", '#')
            if FirstPos == -1:
                FirstPos = Count

        Count = Count + 1

    return LineList, FirstPos

## GetLibInstanceInfo
#
# Get the information from Library Instance INF file.
#
# @param string.  A string start with # and followed by INF file path
# @param WorkSpace. The WorkSpace directory used to combined with INF file path.
#
# @return GUID, Version
def GetLibInstanceInfo(String, WorkSpace, LineNo):

    FileGuidString = ""
    VerString = ""

    OriginalString = String
    String = String.strip()
    if not String:
        return None, None
    #
    # Remove "#" characters at the beginning
    #
    String = GetHelpStringByRemoveHashKey(String)
    String = String.strip()

    #
    # Validate file name exist.
    #
    FullFileName = os.path.normpath(os.path.realpath(os.path.join(WorkSpace, String)))
    if not (ValidFile(FullFileName)):
        Logger.Error("InfParser",
                     ToolError.FORMAT_INVALID,
                     ST.ERR_FILELIST_EXIST % (String),
                     File=GlobalData.gINF_MODULE_NAME,
                     Line=LineNo,
                     ExtraData=OriginalString)

    #
    # Validate file exist/format.
    #
    if IsValidPath(String, WorkSpace):
        IsValidFileFlag = True
    else:
        Logger.Error("InfParser",
                     ToolError.FORMAT_INVALID,
                     ST.ERR_INF_PARSER_FILE_NOT_EXIST_OR_NAME_INVALID % (String),
                     File=GlobalData.gINF_MODULE_NAME,
                     Line=LineNo,
                     ExtraData=OriginalString)
        return False
    if IsValidFileFlag:
        FileLinesList = []

        try:
            FInputfile = open(FullFileName, "r")
            try:
                FileLinesList = FInputfile.readlines()
            except BaseException:
                Logger.Error("InfParser",
                             ToolError.FILE_READ_FAILURE,
                             ST.ERR_FILE_OPEN_FAILURE,
                             File=FullFileName)
            finally:
                FInputfile.close()
        except BaseException:
            Logger.Error("InfParser",
                         ToolError.FILE_READ_FAILURE,
                         ST.ERR_FILE_OPEN_FAILURE,
                         File=FullFileName)

        ReFileGuidPattern = re.compile("^\s*FILE_GUID\s*=.*$")
        ReVerStringPattern = re.compile("^\s*VERSION_STRING\s*=.*$")

        FileLinesList = ProcessLineExtender(FileLinesList)

        for Line in FileLinesList:
            if ReFileGuidPattern.match(Line):
                FileGuidString = Line
            if ReVerStringPattern.match(Line):
                VerString = Line

        if FileGuidString:
            FileGuidString = GetSplitValueList(FileGuidString, '=', 1)[1]
        if VerString:
            VerString = GetSplitValueList(VerString, '=', 1)[1]

        return FileGuidString, VerString

## GetLocalValue
#
# Generate the local value for INF and DEC file. If Lang attribute not present, then use this value.
# If present, and there is no element without the Lang attribute, and one of the elements has the rfc1766 code is
# "en-x-tianocore", or "en-US" if "en-x-tianocore" was not found, or "en" if "en-US" was not found, or startswith 'en'
# if 'en' was not found, then use this value.
# If multiple entries of a tag exist which have the same language code, use the last entry.
#
# @param ValueList  A list need to be processed.
# @param UseFirstValue: True to use the first value, False to use the last value
#
# @return LocalValue
def GetLocalValue(ValueList, UseFirstValue=False):
    Value1 = ''
    Value2 = ''
    Value3 = ''
    Value4 = ''
    Value5 = ''
    for (Key, Value) in ValueList:
        if Key == TAB_LANGUAGE_EN_X:
            if UseFirstValue:
                if not Value1:
                    Value1 = Value
            else:
                Value1 = Value
        if Key == TAB_LANGUAGE_EN_US:
            if UseFirstValue:
                if not Value2:
                    Value2 = Value
            else:
                Value2 = Value
        if Key == TAB_LANGUAGE_EN:
            if UseFirstValue:
                if not Value3:
                    Value3 = Value
            else:
                Value3 = Value
        if Key.startswith(TAB_LANGUAGE_EN):
            if UseFirstValue:
                if not Value4:
                    Value4 = Value
            else:
                Value4 = Value
        if Key == '':
            if UseFirstValue:
                if not Value5:
                    Value5 = Value
            else:
                Value5 = Value

    if Value1:
        return Value1
    if Value2:
        return Value2
    if Value3:
        return Value3
    if Value4:
        return Value4
    if Value5:
        return Value5

    return ''


## GetCharIndexOutStr
#
# Get comment character index outside a string
#
# @param Line:              The string to be checked
# @param CommentCharacter:  Comment char, used to ignore comment content
#
# @retval Index
#
def GetCharIndexOutStr(CommentCharacter, Line):
    #
    # remove whitespace
    #
    Line = Line.strip()

    #
    # Check whether comment character is in a string
    #
    InString = False
    for Index in range(0, len(Line)):
        if Line[Index] == '"':
            InString = not InString
        elif Line[Index] == CommentCharacter and InString :
            pass
        elif Line[Index] == CommentCharacter and (Index +1) < len(Line) and Line[Index+1] == CommentCharacter \
            and not InString :
            return Index
    return -1

## ValidateUNIFilePath
#
# Check the UNI file path
#
# @param FilePath: The UNI file path
#
def ValidateUNIFilePath(Path):
    Suffix = Path[Path.rfind(TAB_SPLIT):]

    #
    # Check if the suffix is one of the '.uni', '.UNI', '.Uni'
    #
    if Suffix not in TAB_UNI_FILE_SUFFIXS:
        Logger.Error("Unicode File Parser",
                        ToolError.FORMAT_INVALID,
                        Message=ST.ERR_UNI_FILE_SUFFIX_WRONG,
                        ExtraData=Path)

    #
    # Check if '..' in the file name(without suffix)
    #
    if (TAB_SPLIT + TAB_SPLIT) in Path:
        Logger.Error("Unicode File Parser",
                        ToolError.FORMAT_INVALID,
                        Message=ST.ERR_UNI_FILE_NAME_INVALID,
                        ExtraData=Path)

    #
    # Check if the file name is valid according to the DEC and INF specification
    #
    Pattern = '[a-zA-Z0-9_][a-zA-Z0-9_\-\.]*'
    FileName = Path.replace(Suffix, '')
    InvalidCh = re.sub(Pattern, '', FileName)
    if InvalidCh:
        Logger.Error("Unicode File Parser",
                        ToolError.FORMAT_INVALID,
                        Message=ST.ERR_INF_PARSER_FILE_NOT_EXIST_OR_NAME_INVALID,
                        ExtraData=Path)

