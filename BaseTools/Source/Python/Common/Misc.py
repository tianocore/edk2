## @file
# Common routines used by all tools
#
# Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import os
import sys
import string
import thread
import threading
import time
import re
import cPickle
import array
from UserDict import IterableUserDict
from UserList import UserList

from Common import EdkLogger as EdkLogger
from Common import GlobalData as GlobalData
from DataType import *
from BuildToolError import *

## Regular expression used to find out place holders in string template
gPlaceholderPattern = re.compile("\$\{([^$()\s]+)\}", re.MULTILINE|re.UNICODE)

## Dictionary used to store file time stamp for quick re-access
gFileTimeStampCache = {}    # {file path : file time stamp}

## Dictionary used to store dependencies of files
gDependencyDatabase = {}    # arch : {file path : [dependent files list]}

## callback routine for processing variable option
#
# This function can be used to process variable number of option values. The
# typical usage of it is specify architecure list on command line.
# (e.g. <tool> -a IA32 X64 IPF)
#
# @param  Option        Standard callback function parameter
# @param  OptionString  Standard callback function parameter
# @param  Value         Standard callback function parameter
# @param  Parser        Standard callback function parameter
#
# @retval
#
def ProcessVariableArgument(Option, OptionString, Value, Parser):
    assert Value is None
    Value = []
    RawArgs = Parser.rargs
    while RawArgs:
        Arg = RawArgs[0]
        if (Arg[:2] == "--" and len(Arg) > 2) or \
           (Arg[:1] == "-" and len(Arg) > 1 and Arg[1] != "-"):
            break
        Value.append(Arg)
        del RawArgs[0]
    setattr(Parser.values, Option.dest, Value)

## Convert GUID string in xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx style to C structure style
#
#   @param      Guid    The GUID string
#
#   @retval     string  The GUID string in C structure style
#
def GuidStringToGuidStructureString(Guid):
    GuidList = Guid.split('-')
    Result = '{'
    for Index in range(0,3,1):
        Result = Result + '0x' + GuidList[Index] + ', '
    Result = Result + '{0x' + GuidList[3][0:2] + ', 0x' + GuidList[3][2:4]
    for Index in range(0,12,2):
        Result = Result + ', 0x' + GuidList[4][Index:Index+2]
    Result += '}}'
    return Result

## Convert GUID structure in byte array to xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
#
#   @param      GuidValue   The GUID value in byte array
#
#   @retval     string      The GUID value in xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx format
#
def GuidStructureByteArrayToGuidString(GuidValue):
    guidValueString = GuidValue.lower().replace("{", "").replace("}", "").replace(" ", "").replace(";", "")
    guidValueList = guidValueString.split(",")
    if len(guidValueList) != 16:
        return ''
        #EdkLogger.error(None, None, "Invalid GUID value string %s" % GuidValue)
    try:
        return "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x" % (
                int(guidValueList[3], 16),
                int(guidValueList[2], 16),
                int(guidValueList[1], 16),
                int(guidValueList[0], 16),
                int(guidValueList[5], 16),
                int(guidValueList[4], 16),
                int(guidValueList[7], 16),
                int(guidValueList[6], 16),
                int(guidValueList[8], 16),
                int(guidValueList[9], 16),
                int(guidValueList[10], 16),
                int(guidValueList[11], 16),
                int(guidValueList[12], 16),
                int(guidValueList[13], 16),
                int(guidValueList[14], 16),
                int(guidValueList[15], 16)
                )
    except:
        return ''

## Convert GUID string in C structure style to xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
#
#   @param      GuidValue   The GUID value in C structure format
#
#   @retval     string      The GUID value in xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx format
#
def GuidStructureStringToGuidString(GuidValue):
    guidValueString = GuidValue.lower().replace("{", "").replace("}", "").replace(" ", "").replace(";", "")
    guidValueList = guidValueString.split(",")
    if len(guidValueList) != 11:
        return ''
        #EdkLogger.error(None, None, "Invalid GUID value string %s" % GuidValue)
    try:
        return "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x" % (
                int(guidValueList[0], 16),
                int(guidValueList[1], 16),
                int(guidValueList[2], 16),
                int(guidValueList[3], 16),
                int(guidValueList[4], 16),
                int(guidValueList[5], 16),
                int(guidValueList[6], 16),
                int(guidValueList[7], 16),
                int(guidValueList[8], 16),
                int(guidValueList[9], 16),
                int(guidValueList[10], 16)
                )
    except:
        return ''

## Convert GUID string in C structure style to xxxxxxxx_xxxx_xxxx_xxxx_xxxxxxxxxxxx
#
#   @param      GuidValue   The GUID value in C structure format
#
#   @retval     string      The GUID value in xxxxxxxx_xxxx_xxxx_xxxx_xxxxxxxxxxxx format
#
def GuidStructureStringToGuidValueName(GuidValue):
    guidValueString = GuidValue.lower().replace("{", "").replace("}", "").replace(" ", "")
    guidValueList = guidValueString.split(",")
    if len(guidValueList) != 11:
        EdkLogger.error(None, FORMAT_INVALID, "Invalid GUID value string [%s]" % GuidValue)
    return "%08x_%04x_%04x_%02x%02x_%02x%02x%02x%02x%02x%02x" % (
            int(guidValueList[0], 16),
            int(guidValueList[1], 16),
            int(guidValueList[2], 16),
            int(guidValueList[3], 16),
            int(guidValueList[4], 16),
            int(guidValueList[5], 16),
            int(guidValueList[6], 16),
            int(guidValueList[7], 16),
            int(guidValueList[8], 16),
            int(guidValueList[9], 16),
            int(guidValueList[10], 16)
            )

## Create directories
#
#   @param      Directory   The directory name
#
def CreateDirectory(Directory):
    if Directory == None or Directory.strip() == "":
        return True
    try:
        if not os.access(Directory, os.F_OK):
            os.makedirs(Directory)
    except:
        return False
    return True

## Remove directories, including files and sub-directories in it
#
#   @param      Directory   The directory name
#
def RemoveDirectory(Directory, Recursively=False):
    if Directory == None or Directory.strip() == "" or not os.path.exists(Directory):
        return
    if Recursively:
        CurrentDirectory = os.getcwd()
        os.chdir(Directory)
        for File in os.listdir("."):
            if os.path.isdir(File):
                RemoveDirectory(File, Recursively)
            else:
                os.remove(File)
        os.chdir(CurrentDirectory)
    os.rmdir(Directory)

## Check if given file is changed or not
#
#  This method is used to check if a file is changed or not between two build
#  actions. It makes use a cache to store files timestamp.
#
#   @param      File    The path of file
#
#   @retval     True    If the given file is changed, doesn't exist, or can't be
#                       found in timestamp cache
#   @retval     False   If the given file is changed
#
def IsChanged(File):
    if not os.path.exists(File):
        return True

    FileState = os.stat(File)
    TimeStamp = FileState[-2]

    if File in gFileTimeStampCache and TimeStamp == gFileTimeStampCache[File]:
        FileChanged = False
    else:
        FileChanged = True
        gFileTimeStampCache[File] = TimeStamp

    return FileChanged

## Store content in file
#
#  This method is used to save file only when its content is changed. This is
#  quite useful for "make" system to decide what will be re-built and what won't.
#
#   @param      File            The path of file
#   @param      Content         The new content of the file
#   @param      IsBinaryFile    The flag indicating if the file is binary file or not
#
#   @retval     True            If the file content is changed and the file is renewed
#   @retval     False           If the file content is the same
#
def SaveFileOnChange(File, Content, IsBinaryFile=True):
    if not IsBinaryFile:
        Content = Content.replace("\n", os.linesep)

    if os.path.exists(File):
        try:
            if Content == open(File, "rb").read():
                return False
        except:
            EdkLogger.error(None, FILE_OPEN_FAILURE, ExtraData=File)

    DirName = os.path.dirname(File)
    if not CreateDirectory(DirName):
        EdkLogger.error(None, FILE_CREATE_FAILURE, "Could not create directory %s" % DirName)
    else:
        if DirName == '':
            DirName = os.getcwd()
        if not os.access(DirName, os.W_OK):
            EdkLogger.error(None, PERMISSION_FAILURE, "Do not have write permission on directory %s" % DirName)

    try:
        if GlobalData.gIsWindows:
            try:
                from PyUtility import SaveFileToDisk
                if not SaveFileToDisk(File, Content):
                    EdkLogger.error(None, FILE_CREATE_FAILURE, ExtraData=File)
            except:
                Fd = open(File, "wb")
                Fd.write(Content)
                Fd.close()
        else:
            Fd = open(File, "wb")
            Fd.write(Content)
            Fd.close()
    except IOError, X:
        EdkLogger.error(None, FILE_CREATE_FAILURE, ExtraData='IOError %s'%X)

    return True

## Make a Python object persistent on file system
#
#   @param      Data    The object to be stored in file
#   @param      File    The path of file to store the object
#
def DataDump(Data, File):
    Fd = None
    try:
        Fd = open(File, 'wb')
        cPickle.dump(Data, Fd, cPickle.HIGHEST_PROTOCOL)
    except:
        EdkLogger.error("", FILE_OPEN_FAILURE, ExtraData=File, RaiseError=False)
    finally:
        if Fd != None:
            Fd.close()

## Restore a Python object from a file
#
#   @param      File    The path of file stored the object
#
#   @retval     object  A python object
#   @retval     None    If failure in file operation
#
def DataRestore(File):
    Data = None
    Fd = None
    try:
        Fd = open(File, 'rb')
        Data = cPickle.load(Fd)
    except Exception, e:
        EdkLogger.verbose("Failed to load [%s]\n\t%s" % (File, str(e)))
        Data = None
    finally:
        if Fd != None:
            Fd.close()
    return Data

## Retrieve and cache the real path name in file system
#
#   @param      Root    The root directory of path relative to
#
#   @retval     str     The path string if the path exists
#   @retval     None    If path doesn't exist
#
class DirCache:
    _CACHE_ = set()
    _UPPER_CACHE_ = {}

    def __init__(self, Root):
        self._Root = Root
        for F in os.listdir(Root):
            self._CACHE_.add(F)
            self._UPPER_CACHE_[F.upper()] = F

    # =[] operator
    def __getitem__(self, Path):
        Path = Path[len(os.path.commonprefix([Path, self._Root])):]
        if not Path:
            return self._Root
        if Path and Path[0] == os.path.sep:
            Path = Path[1:]
        if Path in self._CACHE_:
            return os.path.join(self._Root, Path)
        UpperPath = Path.upper()
        if UpperPath in self._UPPER_CACHE_:
            return os.path.join(self._Root, self._UPPER_CACHE_[UpperPath])

        IndexList = []
        LastSepIndex = -1
        SepIndex = Path.find(os.path.sep)
        while SepIndex > -1:
            Parent = UpperPath[:SepIndex]
            if Parent not in self._UPPER_CACHE_:
                break
            LastSepIndex = SepIndex
            SepIndex = Path.find(os.path.sep, LastSepIndex + 1)

        if LastSepIndex == -1:
            return None

        Cwd = os.getcwd()
        os.chdir(self._Root)
        SepIndex = LastSepIndex
        while SepIndex > -1:
            Parent = Path[:SepIndex]
            ParentKey = UpperPath[:SepIndex]
            if ParentKey not in self._UPPER_CACHE_:
                os.chdir(Cwd)
                return None

            if Parent in self._CACHE_:
                ParentDir = Parent
            else:
                ParentDir = self._UPPER_CACHE_[ParentKey]
            for F in os.listdir(ParentDir):
                Dir = os.path.join(ParentDir, F)
                self._CACHE_.add(Dir)
                self._UPPER_CACHE_[Dir.upper()] = Dir

            SepIndex = Path.find(os.path.sep, SepIndex + 1)

        os.chdir(Cwd)
        if Path in self._CACHE_:
            return os.path.join(self._Root, Path)
        elif UpperPath in self._UPPER_CACHE_:
            return os.path.join(self._Root, self._UPPER_CACHE_[UpperPath])
        return None

## Get all files of a directory
#
# @param Root:       Root dir
# @param SkipList :  The files need be skipped
#
# @retval  A list of all files
#
def GetFiles(Root, SkipList=None, FullPath = True):
    OriPath = Root
    FileList = []
    for Root, Dirs, Files in os.walk(Root):
        if SkipList:
            for Item in SkipList:
                if Item in Dirs:
                    Dirs.remove(Item)

        for File in Files:
            File = os.path.normpath(os.path.join(Root, File))
            if not FullPath:
                File = File[len(OriPath) + 1:]
            FileList.append(File)

    return FileList

## Check if gvien file exists or not
#
#   @param      File    File name or path to be checked
#   @param      Dir     The directory the file is relative to
#
#   @retval     True    if file exists
#   @retval     False   if file doesn't exists
#
def ValidFile(File, Ext=None):
    if Ext != None:
        Dummy, FileExt = os.path.splitext(File)
        if FileExt.lower() != Ext.lower():
            return False
    if not os.path.exists(File):
        return False
    return True

def RealPath(File, Dir='', OverrideDir=''):
    NewFile = os.path.normpath(os.path.join(Dir, File))
    NewFile = GlobalData.gAllFiles[NewFile]
    if not NewFile and OverrideDir:
        NewFile = os.path.normpath(os.path.join(OverrideDir, File))
        NewFile = GlobalData.gAllFiles[NewFile]
    return NewFile

def RealPath2(File, Dir='', OverrideDir=''):
    if OverrideDir:
        NewFile = GlobalData.gAllFiles[os.path.normpath(os.path.join(OverrideDir, File))]
        if NewFile:
            if OverrideDir[-1] == os.path.sep:
                return NewFile[len(OverrideDir):], NewFile[0:len(OverrideDir)]
            else:
                return NewFile[len(OverrideDir)+1:], NewFile[0:len(OverrideDir)]
    if GlobalData.gAllFiles:
        NewFile = GlobalData.gAllFiles[os.path.normpath(os.path.join(Dir, File))]
    else:
        NewFile = os.path.normpath(os.path.join(Dir, File))
    if NewFile:
        if Dir:
            if Dir[-1] == os.path.sep:
                return NewFile[len(Dir):], NewFile[0:len(Dir)]
            else:
                return NewFile[len(Dir)+1:], NewFile[0:len(Dir)]
        else:
            return NewFile, ''

    return None, None

## Check if gvien file exists or not
#
#
def ValidFile2(AllFiles, File, Ext=None, Workspace='', EfiSource='', EdkSource='', Dir='.', OverrideDir=''):
    NewFile = File
    if Ext != None:
        Dummy, FileExt = os.path.splitext(File)
        if FileExt.lower() != Ext.lower():
            return False, File

    # Replace the Edk macros
    if OverrideDir != '' and OverrideDir != None:
        if OverrideDir.find('$(EFI_SOURCE)') > -1:
            OverrideDir = OverrideDir.replace('$(EFI_SOURCE)', EfiSource)
        if OverrideDir.find('$(EDK_SOURCE)') > -1:
            OverrideDir = OverrideDir.replace('$(EDK_SOURCE)', EdkSource)

    # Replace the default dir to current dir
    if Dir == '.':
        Dir = os.getcwd()
        Dir = Dir[len(Workspace)+1:]

    # First check if File has Edk definition itself
    if File.find('$(EFI_SOURCE)') > -1 or File.find('$(EDK_SOURCE)') > -1:
        NewFile = File.replace('$(EFI_SOURCE)', EfiSource)
        NewFile = NewFile.replace('$(EDK_SOURCE)', EdkSource)
        NewFile = AllFiles[os.path.normpath(NewFile)]
        if NewFile != None:
            return True, NewFile

    # Second check the path with override value
    if OverrideDir != '' and OverrideDir != None:
        NewFile = AllFiles[os.path.normpath(os.path.join(OverrideDir, File))]
        if NewFile != None:
            return True, NewFile

    # Last check the path with normal definitions
    File = os.path.join(Dir, File)
    NewFile = AllFiles[os.path.normpath(File)]
    if NewFile != None:
        return True, NewFile

    return False, File

## Check if gvien file exists or not
#
#
def ValidFile3(AllFiles, File, Workspace='', EfiSource='', EdkSource='', Dir='.', OverrideDir=''):
    # Replace the Edk macros
    if OverrideDir != '' and OverrideDir != None:
        if OverrideDir.find('$(EFI_SOURCE)') > -1:
            OverrideDir = OverrideDir.replace('$(EFI_SOURCE)', EfiSource)
        if OverrideDir.find('$(EDK_SOURCE)') > -1:
            OverrideDir = OverrideDir.replace('$(EDK_SOURCE)', EdkSource)

    # Replace the default dir to current dir
    # Dir is current module dir related to workspace
    if Dir == '.':
        Dir = os.getcwd()
        Dir = Dir[len(Workspace)+1:]

    NewFile = File
    RelaPath = AllFiles[os.path.normpath(Dir)]
    NewRelaPath = RelaPath

    while(True):
        # First check if File has Edk definition itself
        if File.find('$(EFI_SOURCE)') > -1 or File.find('$(EDK_SOURCE)') > -1:
            File = File.replace('$(EFI_SOURCE)', EfiSource)
            File = File.replace('$(EDK_SOURCE)', EdkSource)
            NewFile = AllFiles[os.path.normpath(File)]
            if NewFile != None:
                NewRelaPath = os.path.dirname(NewFile)
                File = os.path.basename(NewFile)
                #NewRelaPath = NewFile[:len(NewFile) - len(File.replace("..\\", '').replace("../", '')) - 1]
                break

        # Second check the path with override value
        if OverrideDir != '' and OverrideDir != None:
            NewFile = AllFiles[os.path.normpath(os.path.join(OverrideDir, File))]
            if NewFile != None:
                #NewRelaPath = os.path.dirname(NewFile)
                NewRelaPath = NewFile[:len(NewFile) - len(File.replace("..\\", '').replace("../", '')) - 1]
                break

        # Last check the path with normal definitions
        NewFile = AllFiles[os.path.normpath(os.path.join(Dir, File))]
        if NewFile != None:
            break

        # No file found
        break

    return NewRelaPath, RelaPath, File


def GetRelPath(Path1, Path2):
    FileName = os.path.basename(Path2)
    L1 = os.path.normpath(Path1).split(os.path.normpath('/'))
    L2 = os.path.normpath(Path2).split(os.path.normpath('/'))
    for Index in range(0, len(L1)):
        if L1[Index] != L2[Index]:
            FileName = '../' * (len(L1) - Index)
            for Index2 in range(Index, len(L2)):
                FileName = os.path.join(FileName, L2[Index2])
            break
    return os.path.normpath(FileName)


## Get GUID value from given packages
#
#   @param      CName           The CName of the GUID
#   @param      PackageList     List of packages looking-up in
#
#   @retval     GuidValue   if the CName is found in any given package
#   @retval     None        if the CName is not found in all given packages
#
def GuidValue(CName, PackageList):
    for P in PackageList:
        if CName in P.Guids:
            return P.Guids[CName]
    return None

## Get Protocol value from given packages
#
#   @param      CName           The CName of the GUID
#   @param      PackageList     List of packages looking-up in
#
#   @retval     GuidValue   if the CName is found in any given package
#   @retval     None        if the CName is not found in all given packages
#
def ProtocolValue(CName, PackageList):
    for P in PackageList:
        if CName in P.Protocols:
            return P.Protocols[CName]
    return None

## Get PPI value from given packages
#
#   @param      CName           The CName of the GUID
#   @param      PackageList     List of packages looking-up in
#
#   @retval     GuidValue   if the CName is found in any given package
#   @retval     None        if the CName is not found in all given packages
#
def PpiValue(CName, PackageList):
    for P in PackageList:
        if CName in P.Ppis:
            return P.Ppis[CName]
    return None

## A string template class
#
#  This class implements a template for string replacement. A string template
#  looks like following
#
#       ${BEGIN} other_string ${placeholder_name} other_string ${END}
#
#  The string between ${BEGIN} and ${END} will be repeated as many times as the
#  length of "placeholder_name", which is a list passed through a dict. The
#  "placeholder_name" is the key name of the dict. The ${BEGIN} and ${END} can
#  be not used and, in this case, the "placeholder_name" must not a list and it
#  will just be replaced once.
#
class TemplateString(object):
    _REPEAT_START_FLAG = "BEGIN"
    _REPEAT_END_FLAG = "END"

    class Section(object):
        _LIST_TYPES = [type([]), type(set()), type((0,))]

        def __init__(self, TemplateSection, PlaceHolderList):
            self._Template = TemplateSection
            self._PlaceHolderList = []

            # Split the section into sub-sections according to the position of placeholders
            if PlaceHolderList:
                self._SubSectionList = []
                SubSectionStart = 0
                #
                # The placeholders passed in must be in the format of
                #
                #   PlaceHolderName, PlaceHolderStartPoint, PlaceHolderEndPoint
                #
                for PlaceHolder,Start,End in PlaceHolderList:
                    self._SubSectionList.append(TemplateSection[SubSectionStart:Start])
                    self._SubSectionList.append(TemplateSection[Start:End])
                    self._PlaceHolderList.append(PlaceHolder)
                    SubSectionStart = End
                if SubSectionStart < len(TemplateSection):
                    self._SubSectionList.append(TemplateSection[SubSectionStart:])
            else:
                self._SubSectionList = [TemplateSection]

        def __str__(self):
            return self._Template + " : " + str(self._PlaceHolderList)

        def Instantiate(self, PlaceHolderValues):
            RepeatTime = -1
            RepeatPlaceHolders = {}
            NonRepeatPlaceHolders = {}

            for PlaceHolder in self._PlaceHolderList:
                if PlaceHolder not in PlaceHolderValues:
                    continue
                Value = PlaceHolderValues[PlaceHolder]
                if type(Value) in self._LIST_TYPES:
                    if RepeatTime < 0:
                        RepeatTime = len(Value)
                    elif RepeatTime != len(Value):
                        EdkLogger.error(
                                    "TemplateString",
                                    PARAMETER_INVALID,
                                    "${%s} has different repeat time from others!" % PlaceHolder,
                                    ExtraData=str(self._Template)
                                    )
                    RepeatPlaceHolders["${%s}" % PlaceHolder] = Value
                else:
                    NonRepeatPlaceHolders["${%s}" % PlaceHolder] = Value

            if NonRepeatPlaceHolders:
                StringList = []
                for S in self._SubSectionList:
                    if S not in NonRepeatPlaceHolders:
                        StringList.append(S)
                    else:
                        StringList.append(str(NonRepeatPlaceHolders[S]))
            else:
                StringList = self._SubSectionList

            if RepeatPlaceHolders:
                TempStringList = []
                for Index in range(RepeatTime):
                    for S in StringList:
                        if S not in RepeatPlaceHolders:
                            TempStringList.append(S)
                        else:
                            TempStringList.append(str(RepeatPlaceHolders[S][Index]))
                StringList = TempStringList

            return "".join(StringList)

    ## Constructor
    def __init__(self, Template=None):
        self.String = ''
        self.IsBinary = False
        self._Template = Template
        self._TemplateSectionList = self._Parse(Template)

    ## str() operator
    #
    #   @retval     string  The string replaced
    #
    def __str__(self):
        return self.String

    ## Split the template string into fragments per the ${BEGIN} and ${END} flags
    #
    #   @retval     list    A list of TemplateString.Section objects
    #
    def _Parse(self, Template):
        SectionStart = 0
        SearchFrom = 0
        MatchEnd = 0
        PlaceHolderList = []
        TemplateSectionList = []
        while Template:
            MatchObj = gPlaceholderPattern.search(Template, SearchFrom)
            if not MatchObj:
                if MatchEnd <= len(Template):
                    TemplateSection = TemplateString.Section(Template[SectionStart:], PlaceHolderList)
                    TemplateSectionList.append(TemplateSection)
                break

            MatchString = MatchObj.group(1)
            MatchStart = MatchObj.start()
            MatchEnd = MatchObj.end()

            if MatchString == self._REPEAT_START_FLAG:
                if MatchStart > SectionStart:
                    TemplateSection = TemplateString.Section(Template[SectionStart:MatchStart], PlaceHolderList)
                    TemplateSectionList.append(TemplateSection)
                SectionStart = MatchEnd
                PlaceHolderList = []
            elif MatchString == self._REPEAT_END_FLAG:
                TemplateSection = TemplateString.Section(Template[SectionStart:MatchStart], PlaceHolderList)
                TemplateSectionList.append(TemplateSection)
                SectionStart = MatchEnd
                PlaceHolderList = []
            else:
                PlaceHolderList.append((MatchString, MatchStart - SectionStart, MatchEnd - SectionStart))
            SearchFrom = MatchEnd
        return TemplateSectionList

    ## Replace the string template with dictionary of placeholders and append it to previous one
    #
    #   @param      AppendString    The string template to append
    #   @param      Dictionary      The placeholder dictionaries
    #
    def Append(self, AppendString, Dictionary=None):
        if Dictionary:
            SectionList = self._Parse(AppendString)
            self.String += "".join([S.Instantiate(Dictionary) for S in SectionList])
        else:
            self.String += AppendString

    ## Replace the string template with dictionary of placeholders
    #
    #   @param      Dictionary      The placeholder dictionaries
    #
    #   @retval     str             The string replaced with placeholder values
    #
    def Replace(self, Dictionary=None):
        return "".join([S.Instantiate(Dictionary) for S in self._TemplateSectionList])

## Progress indicator class
#
#  This class makes use of thread to print progress on console.
#
class Progressor:
    # for avoiding deadloop
    _StopFlag = None
    _ProgressThread = None
    _CheckInterval = 0.25

    ## Constructor
    #
    #   @param      OpenMessage     The string printed before progress charaters
    #   @param      CloseMessage    The string printed after progress charaters
    #   @param      ProgressChar    The charater used to indicate the progress
    #   @param      Interval        The interval in seconds between two progress charaters
    #
    def __init__(self, OpenMessage="", CloseMessage="", ProgressChar='.', Interval=1.0):
        self.PromptMessage = OpenMessage
        self.CodaMessage = CloseMessage
        self.ProgressChar = ProgressChar
        self.Interval = Interval
        if Progressor._StopFlag == None:
            Progressor._StopFlag = threading.Event()

    ## Start to print progress charater
    #
    #   @param      OpenMessage     The string printed before progress charaters
    #
    def Start(self, OpenMessage=None):
        if OpenMessage != None:
            self.PromptMessage = OpenMessage
        Progressor._StopFlag.clear()
        if Progressor._ProgressThread == None:
            Progressor._ProgressThread = threading.Thread(target=self._ProgressThreadEntry)
            Progressor._ProgressThread.setDaemon(False)
            Progressor._ProgressThread.start()

    ## Stop printing progress charater
    #
    #   @param      CloseMessage    The string printed after progress charaters
    #
    def Stop(self, CloseMessage=None):
        OriginalCodaMessage = self.CodaMessage
        if CloseMessage != None:
            self.CodaMessage = CloseMessage
        self.Abort()
        self.CodaMessage = OriginalCodaMessage

    ## Thread entry method
    def _ProgressThreadEntry(self):
        sys.stdout.write(self.PromptMessage + " ")
        sys.stdout.flush()
        TimeUp = 0.0
        while not Progressor._StopFlag.isSet():
            if TimeUp <= 0.0:
                sys.stdout.write(self.ProgressChar)
                sys.stdout.flush()
                TimeUp = self.Interval
            time.sleep(self._CheckInterval)
            TimeUp -= self._CheckInterval
        sys.stdout.write(" " + self.CodaMessage + "\n")
        sys.stdout.flush()

    ## Abort the progress display
    @staticmethod
    def Abort():
        if Progressor._StopFlag != None:
            Progressor._StopFlag.set()
        if Progressor._ProgressThread != None:
            Progressor._ProgressThread.join()
            Progressor._ProgressThread = None

## A dict which can access its keys and/or values orderly
#
#  The class implements a new kind of dict which its keys or values can be
#  accessed in the order they are added into the dict. It guarantees the order
#  by making use of an internal list to keep a copy of keys.
#
class sdict(IterableUserDict):
    ## Constructor
    def __init__(self):
        IterableUserDict.__init__(self)
        self._key_list = []

    ## [] operator
    def __setitem__(self, key, value):
        if key not in self._key_list:
            self._key_list.append(key)
        IterableUserDict.__setitem__(self, key, value)

    ## del operator
    def __delitem__(self, key):
        self._key_list.remove(key)
        IterableUserDict.__delitem__(self, key)

    ## used in "for k in dict" loop to ensure the correct order
    def __iter__(self):
        return self.iterkeys()

    ## len() support
    def __len__(self):
        return len(self._key_list)

    ## "in" test support
    def __contains__(self, key):
        return key in self._key_list

    ## indexof support
    def index(self, key):
        return self._key_list.index(key)

    ## insert support
    def insert(self, key, newkey, newvalue, order):
        index = self._key_list.index(key)
        if order == 'BEFORE':
            self._key_list.insert(index, newkey)
            IterableUserDict.__setitem__(self, newkey, newvalue)
        elif order == 'AFTER':
            self._key_list.insert(index + 1, newkey)
            IterableUserDict.__setitem__(self, newkey, newvalue)

    ## append support
    def append(self, sdict):
        for key in sdict:
            if key not in self._key_list:
                self._key_list.append(key)
            IterableUserDict.__setitem__(self, key, sdict[key])

    def has_key(self, key):
        return key in self._key_list

    ## Empty the dict
    def clear(self):
        self._key_list = []
        IterableUserDict.clear(self)

    ## Return a copy of keys
    def keys(self):
        keys = []
        for key in self._key_list:
            keys.append(key)
        return keys

    ## Return a copy of values
    def values(self):
        values = []
        for key in self._key_list:
            values.append(self[key])
        return values

    ## Return a copy of (key, value) list
    def items(self):
        items = []
        for key in self._key_list:
            items.append((key, self[key]))
        return items

    ## Iteration support
    def iteritems(self):
        return iter(self.items())

    ## Keys interation support
    def iterkeys(self):
        return iter(self.keys())

    ## Values interation support
    def itervalues(self):
        return iter(self.values())

    ## Return value related to a key, and remove the (key, value) from the dict
    def pop(self, key, *dv):
        value = None
        if key in self._key_list:
            value = self[key]
            self.__delitem__(key)
        elif len(dv) != 0 :
            value = kv[0]
        return value

    ## Return (key, value) pair, and remove the (key, value) from the dict
    def popitem(self):
        key = self._key_list[-1]
        value = self[key]
        self.__delitem__(key)
        return key, value

    def update(self, dict=None, **kwargs):
        if dict != None:
            for k, v in dict.items():
                self[k] = v
        if len(kwargs):
            for k, v in kwargs.items():
                self[k] = v

## Dictionary with restricted keys
#
class rdict(dict):
    ## Constructor
    def __init__(self, KeyList):
        for Key in KeyList:
            dict.__setitem__(self, Key, "")

    ## []= operator
    def __setitem__(self, key, value):
        if key not in self:
            EdkLogger.error("RestrictedDict", ATTRIBUTE_SET_FAILURE, "Key [%s] is not allowed" % key,
                            ExtraData=", ".join(dict.keys(self)))
        dict.__setitem__(self, key, value)

    ## =[] operator
    def __getitem__(self, key):
        if key not in self:
            return ""
        return dict.__getitem__(self, key)

    ## del operator
    def __delitem__(self, key):
        EdkLogger.error("RestrictedDict", ATTRIBUTE_ACCESS_DENIED, ExtraData="del")

    ## Empty the dict
    def clear(self):
        for Key in self:
            self.__setitem__(Key, "")

    ## Return value related to a key, and remove the (key, value) from the dict
    def pop(self, key, *dv):
        EdkLogger.error("RestrictedDict", ATTRIBUTE_ACCESS_DENIED, ExtraData="pop")

    ## Return (key, value) pair, and remove the (key, value) from the dict
    def popitem(self):
        EdkLogger.error("RestrictedDict", ATTRIBUTE_ACCESS_DENIED, ExtraData="popitem")

## Dictionary using prioritized list as key
#
class tdict:
    _ListType = type([])
    _TupleType = type(())
    _Wildcard = 'COMMON'
    _ValidWildcardList = ['COMMON', 'DEFAULT', 'ALL', '*', 'PLATFORM']

    def __init__(self, _Single_=False, _Level_=2):
        self._Level_ = _Level_
        self.data = {}
        self._Single_ = _Single_

    # =[] operator
    def __getitem__(self, key):
        KeyType = type(key)
        RestKeys = None
        if KeyType == self._ListType or KeyType == self._TupleType:
            FirstKey = key[0]
            if len(key) > 1:
                RestKeys = key[1:]
            elif self._Level_ > 1:
                RestKeys = [self._Wildcard for i in range(0, self._Level_-1)]
        else:
            FirstKey = key
            if self._Level_ > 1:
                RestKeys = [self._Wildcard for i in range(0, self._Level_-1)]

        if FirstKey == None or str(FirstKey).upper() in self._ValidWildcardList:
            FirstKey = self._Wildcard

        if self._Single_:
            return self._GetSingleValue(FirstKey, RestKeys)
        else:
            return self._GetAllValues(FirstKey, RestKeys)

    def _GetSingleValue(self, FirstKey, RestKeys):
        Value = None
        #print "%s-%s" % (FirstKey, self._Level_) ,
        if self._Level_ > 1:
            if FirstKey == self._Wildcard:
                if FirstKey in self.data:
                    Value = self.data[FirstKey][RestKeys]
                if Value == None:
                    for Key in self.data:
                        Value = self.data[Key][RestKeys]
                        if Value != None: break
            else:
                if FirstKey in self.data:
                    Value = self.data[FirstKey][RestKeys]
                if Value == None and self._Wildcard in self.data:
                    #print "Value=None"
                    Value = self.data[self._Wildcard][RestKeys]
        else:
            if FirstKey == self._Wildcard:
                if FirstKey in self.data:
                    Value = self.data[FirstKey]
                if Value == None:
                    for Key in self.data:
                        Value = self.data[Key]
                        if Value != None: break
            else:
                if FirstKey in self.data:
                    Value = self.data[FirstKey]
                elif self._Wildcard in self.data:
                    Value = self.data[self._Wildcard]
        return Value

    def _GetAllValues(self, FirstKey, RestKeys):
        Value = []
        if self._Level_ > 1:
            if FirstKey == self._Wildcard:
                for Key in self.data:
                    Value += self.data[Key][RestKeys]
            else:
                if FirstKey in self.data:
                    Value += self.data[FirstKey][RestKeys]
                if self._Wildcard in self.data:
                    Value += self.data[self._Wildcard][RestKeys]
        else:
            if FirstKey == self._Wildcard:
                for Key in self.data:
                    Value.append(self.data[Key])
            else:
                if FirstKey in self.data:
                    Value.append(self.data[FirstKey])
                if self._Wildcard in self.data:
                    Value.append(self.data[self._Wildcard])
        return Value

    ## []= operator
    def __setitem__(self, key, value):
        KeyType = type(key)
        RestKeys = None
        if KeyType == self._ListType or KeyType == self._TupleType:
            FirstKey = key[0]
            if len(key) > 1:
                RestKeys = key[1:]
            else:
                RestKeys = [self._Wildcard for i in range(0, self._Level_-1)]
        else:
            FirstKey = key
            if self._Level_ > 1:
                RestKeys = [self._Wildcard for i in range(0, self._Level_-1)]

        if FirstKey in self._ValidWildcardList:
            FirstKey = self._Wildcard

        if FirstKey not in self.data and self._Level_ > 0:
            self.data[FirstKey] = tdict(self._Single_, self._Level_ - 1)

        if self._Level_ > 1:
            self.data[FirstKey][RestKeys] = value
        else:
            self.data[FirstKey] = value

    def SetGreedyMode(self):
        self._Single_ = False
        if self._Level_ > 1:
            for Key in self.data:
                self.data[Key].SetGreedyMode()

    def SetSingleMode(self):
        self._Single_ = True
        if self._Level_ > 1:
            for Key in self.data:
                self.data[Key].SetSingleMode()

    def GetKeys(self, KeyIndex=0):
        assert KeyIndex >= 0
        if KeyIndex == 0:
            return set(self.data.keys())
        else:
            keys = set()
            for Key in self.data:
                keys |= self.data[Key].GetKeys(KeyIndex - 1)
            return keys

## Boolean chain list
#
class Blist(UserList):
    def __init__(self, initlist=None):
        UserList.__init__(self, initlist)
    def __setitem__(self, i, item):
        if item not in [True, False]:
            if item == 0:
                item = False
            else:
                item = True
        self.data[i] = item
    def _GetResult(self):
        Value = True
        for item in self.data:
            Value &= item
        return Value
    Result = property(_GetResult)

def ParseConsoleLog(Filename):
    Opr = open(os.path.normpath(Filename), 'r')
    Opw = open(os.path.normpath(Filename + '.New'), 'w+')
    for Line in Opr.readlines():
        if Line.find('.efi') > -1:
            Line = Line[Line.rfind(' ') : Line.rfind('.efi')].strip()
            Opw.write('%s\n' % Line)

    Opr.close()
    Opw.close()

## AnalyzePcdData
#
#  Analyze the pcd Value, Datum type and TokenNumber.
#  Used to avoid split issue while the value string contain "|" character
#
#  @param[in] Setting:  A String contain value/datum type/token number information;
#  
#  @retval   ValueList: A List contain value, datum type and toke number. 
#
def AnalyzePcdData(Setting):   
    ValueList = ['', '', '']    
    
    ValueRe  = re.compile(r'^\s*L?\".*\|.*\"')
    PtrValue = ValueRe.findall(Setting)
    
    ValueUpdateFlag = False
    
    if len(PtrValue) >= 1:
        Setting = re.sub(ValueRe, '', Setting)
        ValueUpdateFlag = True   

    TokenList = Setting.split(TAB_VALUE_SPLIT)
    ValueList[0:len(TokenList)] = TokenList
    
    if ValueUpdateFlag:
        ValueList[0] = PtrValue[0]
        
    return ValueList   
 
## AnalyzeHiiPcdData
#
#  Analyze the pcd Value, variable name, variable Guid and variable offset.
#  Used to avoid split issue while the value string contain "|" character
#
#  @param[in] Setting:  A String contain VariableName, VariableGuid, VariableOffset, DefaultValue information;
#  
#  @retval   ValueList: A List contaian VariableName, VariableGuid, VariableOffset, DefaultValue. 
#
def AnalyzeHiiPcdData(Setting):   
    ValueList = ['', '', '', '']    
    
    ValueRe  = re.compile(r'^\s*L?\".*\|.*\"')
    PtrValue = ValueRe.findall(Setting)
    
    ValueUpdateFlag = False
    
    if len(PtrValue) >= 1:
        Setting = re.sub(ValueRe, '', Setting)
        ValueUpdateFlag = True   

    TokenList = Setting.split(TAB_VALUE_SPLIT)
    ValueList[0:len(TokenList)] = TokenList
    
    if ValueUpdateFlag:
        ValueList[0] = PtrValue[0]
        
    return ValueList     

## AnalyzeVpdPcdData
#
#  Analyze the vpd pcd Value, Datum type and TokenNumber.
#  Used to avoid split issue while the value string contain "|" character
#
#  @param[in] Setting:  A String contain value/datum type/token number information;
#  
#  @retval   ValueList: A List contain value, datum type and toke number. 
#
def AnalyzeVpdPcdData(Setting):   
    ValueList = ['', '', '']    
    
    ValueRe  = re.compile(r'\s*L?\".*\|.*\"\s*$')
    PtrValue = ValueRe.findall(Setting)
    
    ValueUpdateFlag = False
    
    if len(PtrValue) >= 1:
        Setting = re.sub(ValueRe, '', Setting)
        ValueUpdateFlag = True   

    TokenList = Setting.split(TAB_VALUE_SPLIT)
    ValueList[0:len(TokenList)] = TokenList
    
    if ValueUpdateFlag:
        ValueList[2] = PtrValue[0]
        
    return ValueList     

## check format of PCD value against its the datum type
#
# For PCD value setting
#
def CheckPcdDatum(Type, Value):
    if Type == "VOID*":
        if not (((Value.startswith('L"') or Value.startswith('"')) and Value.endswith('"'))
                or (Value.startswith('{') and Value.endswith('}'))
               ):
            return False, "Invalid value [%s] of type [%s]; must be in the form of {...} for array"\
                          ", or \"...\" for string, or L\"...\" for unicode string" % (Value, Type)
    elif Type == 'BOOLEAN':
        if Value not in ['TRUE', 'True', 'true', '0x1', '0x01', '1', 'FALSE', 'False', 'false', '0x0', '0x00', '0']:
            return False, "Invalid value [%s] of type [%s]; must be one of TRUE, True, true, 0x1, 0x01, 1"\
                          ", FALSE, False, false, 0x0, 0x00, 0" % (Value, Type)
    elif type(Value) == type(""):
        try:
            Value = long(Value, 0)
        except:
            return False, "Invalid value [%s] of type [%s];"\
                          " must be a hexadecimal, decimal or octal in C language format."\
                            % (Value, Type)

    return True, ""

## Split command line option string to list
#
# subprocess.Popen needs the args to be a sequence. Otherwise there's problem
# in non-windows platform to launch command
#
def SplitOption(OptionString):
    OptionList = []
    LastChar = " "
    OptionStart = 0
    QuotationMark = ""
    for Index in range(0, len(OptionString)):
        CurrentChar = OptionString[Index]
        if CurrentChar in ['"', "'"]:
            if QuotationMark == CurrentChar:
                QuotationMark = ""
            elif QuotationMark == "":
                QuotationMark = CurrentChar
            continue
        elif QuotationMark:
            continue

        if CurrentChar in ["/", "-"] and LastChar in [" ", "\t", "\r", "\n"]:
            if Index > OptionStart:
                OptionList.append(OptionString[OptionStart:Index-1])
            OptionStart = Index
        LastChar = CurrentChar
    OptionList.append(OptionString[OptionStart:])
    return OptionList

def CommonPath(PathList):
    P1 = min(PathList).split(os.path.sep)
    P2 = max(PathList).split(os.path.sep)
    for Index in xrange(min(len(P1), len(P2))):
        if P1[Index] != P2[Index]:
            return os.path.sep.join(P1[:Index])
    return os.path.sep.join(P1)

class PathClass(object):
    def __init__(self, File='', Root='', AlterRoot='', Type='', IsBinary=False,
                 Arch='COMMON', ToolChainFamily='', Target='', TagName='', ToolCode=''):
        self.Arch = Arch
        self.File = str(File)
        if os.path.isabs(self.File):
            self.Root = ''
            self.AlterRoot = ''
        else:
            self.Root = str(Root)
            self.AlterRoot = str(AlterRoot)

        # Remove any '.' and '..' in path
        if self.Root:
            self.Path = os.path.normpath(os.path.join(self.Root, self.File))
            self.Root = os.path.normpath(CommonPath([self.Root, self.Path]))
            # eliminate the side-effect of 'C:'
            if self.Root[-1] == ':':
                self.Root += os.path.sep
            # file path should not start with path separator
            if self.Root[-1] == os.path.sep:
                self.File = self.Path[len(self.Root):]
            else:
                self.File = self.Path[len(self.Root)+1:]
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
    #  @retval string Formatted String
    #
    def __str__(self):
        return self.Path

    ## Override __eq__ function
    #
    # Check whether PathClass are the same
    #
    # @retval False The two PathClass are different
    # @retval True  The two PathClass are the same
    #
    def __eq__(self, Other):
        if type(Other) == type(self):
            return self.Path == Other.Path
        else:
            return self.Path == str(Other)

    ## Override __cmp__ function
    #
    # Customize the comparsion operation of two PathClass
    #
    # @retval 0     The two PathClass are different
    # @retval -1    The first PathClass is less than the second PathClass
    # @retval 1     The first PathClass is Bigger than the second PathClass
    def __cmp__(self, Other):
        if type(Other) == type(self):
            OtherKey = Other.Path
        else:
            OtherKey = str(Other)
            
        SelfKey = self.Path
        if SelfKey == OtherKey:
            return 0
        elif SelfKey > OtherKey:
            return 1
        else:
            return -1

    ## Override __hash__ function
    #
    # Use Path as key in hash table
    #
    # @retval string Key for hash table
    #
    def __hash__(self):
        return hash(self.Path)

    def _GetFileKey(self):
        if self._Key == None:
            self._Key = self.Path.upper()   # + self.ToolChainFamily + self.TagName + self.ToolCode + self.Target
        return self._Key

    def _GetTimeStamp(self):
        return os.stat(self.Path)[8]

    def Validate(self, Type='', CaseSensitive=True):
        if GlobalData.gCaseInsensitive:
            CaseSensitive = False
        if Type and Type.lower() != self.Type:
            return FILE_TYPE_MISMATCH, '%s (expect %s but got %s)' % (self.File, Type, self.Type)

        RealFile, RealRoot = RealPath2(self.File, self.Root, self.AlterRoot)
        if not RealRoot and not RealFile:
            RealFile = self.File
            if self.AlterRoot:
                RealFile = os.path.join(self.AlterRoot, self.File)
            elif self.Root:
                RealFile = os.path.join(self.Root, self.File)
            return FILE_NOT_FOUND, os.path.join(self.AlterRoot, RealFile)

        ErrorCode = 0
        ErrorInfo = ''
        if RealRoot != self.Root or RealFile != self.File:
            if CaseSensitive and (RealFile != self.File or (RealRoot != self.Root and RealRoot != self.AlterRoot)):
                ErrorCode = FILE_CASE_MISMATCH
                ErrorInfo = self.File + '\n\t' + RealFile + " [in file system]"

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
    TimeStamp = property(_GetTimeStamp)

## Parse PE image to get the required PE informaion.
#
class PeImageClass():
    ## Constructor
    #
    #   @param  File FilePath of PeImage
    #
    def __init__(self, PeFile):
        self.FileName   = PeFile
        self.IsValid    = False
        self.Size       = 0
        self.EntryPoint = 0
        self.SectionAlignment  = 0
        self.SectionHeaderList = []
        self.ErrorInfo = ''
        try:
            PeObject = open(PeFile, 'rb')
        except:
            self.ErrorInfo = self.FileName + ' can not be found\n'
            return
        # Read DOS header
        ByteArray = array.array('B')
        ByteArray.fromfile(PeObject, 0x3E)
        ByteList = ByteArray.tolist()
        # DOS signature should be 'MZ'
        if self._ByteListToStr (ByteList[0x0:0x2]) != 'MZ':
            self.ErrorInfo = self.FileName + ' has no valid DOS signature MZ'
            return

        # Read 4 byte PE Signature
        PeOffset = self._ByteListToInt(ByteList[0x3C:0x3E])
        PeObject.seek(PeOffset)
        ByteArray = array.array('B')
        ByteArray.fromfile(PeObject, 4)
        # PE signature should be 'PE\0\0'
        if ByteArray.tostring() != 'PE\0\0':
            self.ErrorInfo = self.FileName + ' has no valid PE signature PE00'
            return

        # Read PE file header
        ByteArray = array.array('B')
        ByteArray.fromfile(PeObject, 0x14)
        ByteList = ByteArray.tolist()
        SecNumber = self._ByteListToInt(ByteList[0x2:0x4])
        if SecNumber == 0:
            self.ErrorInfo = self.FileName + ' has no section header'
            return

        # Read PE optional header
        OptionalHeaderSize = self._ByteListToInt(ByteArray[0x10:0x12])
        ByteArray = array.array('B')
        ByteArray.fromfile(PeObject, OptionalHeaderSize)
        ByteList = ByteArray.tolist()
        self.EntryPoint       = self._ByteListToInt(ByteList[0x10:0x14])
        self.SectionAlignment = self._ByteListToInt(ByteList[0x20:0x24])
        self.Size             = self._ByteListToInt(ByteList[0x38:0x3C])

        # Read each Section Header
        for Index in range(SecNumber):
            ByteArray = array.array('B')
            ByteArray.fromfile(PeObject, 0x28)
            ByteList = ByteArray.tolist()
            SecName  = self._ByteListToStr(ByteList[0:8])
            SecVirtualSize = self._ByteListToInt(ByteList[8:12])
            SecRawAddress  = self._ByteListToInt(ByteList[20:24])
            SecVirtualAddress = self._ByteListToInt(ByteList[12:16])
            self.SectionHeaderList.append((SecName, SecVirtualAddress, SecRawAddress, SecVirtualSize))
        self.IsValid = True
        PeObject.close()

    def _ByteListToStr(self, ByteList):
        String = ''
        for index in range(len(ByteList)):
            if ByteList[index] == 0: 
                break
            String += chr(ByteList[index])
        return String

    def _ByteListToInt(self, ByteList):
        Value = 0
        for index in range(len(ByteList) - 1, -1, -1):
            Value = (Value << 8) | int(ByteList[index])
        return Value
        
##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    pass

