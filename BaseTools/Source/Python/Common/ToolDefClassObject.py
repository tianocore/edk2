## @file
# This file is used to define each component of tools_def.txt file
#
# Copyright (c) 2007 - 2019, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
import Common.LongFilePathOs as os
import re
from . import EdkLogger

from .BuildToolError import *
from Common.TargetTxtClassObject import TargetTxtDict
from Common.LongFilePathSupport import OpenLongFilePath as open
from Common.Misc import PathClass
from Common.StringUtils import NormPath
import Common.GlobalData as GlobalData
from Common import GlobalData
from Common.MultipleWorkspace import MultipleWorkspace as mws
from .DataType import TAB_TOD_DEFINES_TARGET, TAB_TOD_DEFINES_TOOL_CHAIN_TAG,\
                     TAB_TOD_DEFINES_TARGET_ARCH, TAB_TOD_DEFINES_COMMAND_TYPE\
                     , TAB_TOD_DEFINES_FAMILY, TAB_TOD_DEFINES_BUILDRULEFAMILY,\
                     TAB_STAR, TAB_TAT_DEFINES_TOOL_CHAIN_CONF


##
# Static variables used for pattern
#
gMacroRefPattern = re.compile('(DEF\([^\(\)]+\))')
gEnvRefPattern = re.compile('(ENV\([^\(\)]+\))')
gMacroDefPattern = re.compile("DEFINE\s+([^\s]+)")
gDefaultToolsDefFile = "tools_def.txt"

## ToolDefClassObject
#
# This class defined content used in file tools_def.txt
#
# @param object:               Inherited from object class
# @param Filename:             Input value for full path of tools_def.txt
#
# @var ToolsDefTxtDictionary:  To store keys and values defined in target.txt
# @var MacroDictionary:        To store keys and values defined in DEFINE statement
#
class ToolDefClassObject(object):
    def __init__(self, FileName=None):
        self.ToolsDefTxtDictionary = {}
        self.MacroDictionary = {}
        for Env in os.environ:
            self.MacroDictionary["ENV(%s)" % Env] = os.environ[Env]

        if FileName is not None:
            self.LoadToolDefFile(FileName)

    ## LoadToolDefFile
    #
    # Load target.txt file and parse it
    #
    # @param Filename:  Input value for full path of tools_def.txt
    #
    def LoadToolDefFile(self, FileName):
        # set multiple workspace
        PackagesPath = os.getenv("PACKAGES_PATH")
        mws.setWs(GlobalData.gWorkspace, PackagesPath)

        self.ToolsDefTxtDatabase = {
            TAB_TOD_DEFINES_TARGET          :   [],
            TAB_TOD_DEFINES_TOOL_CHAIN_TAG  :   [],
            TAB_TOD_DEFINES_TARGET_ARCH     :   [],
            TAB_TOD_DEFINES_COMMAND_TYPE    :   []
        }

        self.IncludeToolDefFile(FileName)

        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TARGET] = list(set(self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TARGET]))
        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TOOL_CHAIN_TAG] = list(set(self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TOOL_CHAIN_TAG]))
        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TARGET_ARCH] = list(set(self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TARGET_ARCH]))

        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_COMMAND_TYPE] = list(set(self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_COMMAND_TYPE]))

        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TARGET].sort()
        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TOOL_CHAIN_TAG].sort()
        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TARGET_ARCH].sort()
        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_COMMAND_TYPE].sort()

        KeyList = [TAB_TOD_DEFINES_TARGET, TAB_TOD_DEFINES_TOOL_CHAIN_TAG, TAB_TOD_DEFINES_TARGET_ARCH, TAB_TOD_DEFINES_COMMAND_TYPE]
        for Index in range(3, -1, -1):
            # make a copy of the keys to enumerate over to prevent issues when
            # adding/removing items from the original dict.
            for Key in list(self.ToolsDefTxtDictionary.keys()):
                List = Key.split('_')
                if List[Index] == TAB_STAR:
                    for String in self.ToolsDefTxtDatabase[KeyList[Index]]:
                        List[Index] = String
                        NewKey = '%s_%s_%s_%s_%s' % tuple(List)
                        if NewKey not in self.ToolsDefTxtDictionary:
                            self.ToolsDefTxtDictionary[NewKey] = self.ToolsDefTxtDictionary[Key]
                    del self.ToolsDefTxtDictionary[Key]
                elif List[Index] not in self.ToolsDefTxtDatabase[KeyList[Index]]:
                    del self.ToolsDefTxtDictionary[Key]


    ## IncludeToolDefFile
    #
    # Load target.txt file and parse it as if it's contents were inside the main file
    #
    # @param Filename:  Input value for full path of tools_def.txt
    #
    def IncludeToolDefFile(self, FileName):
        FileContent = []
        if os.path.isfile(FileName):
            try:
                F = open(FileName, 'r')
                FileContent = F.readlines()
            except:
                EdkLogger.error("tools_def.txt parser", FILE_OPEN_FAILURE, ExtraData=FileName)
        else:
            EdkLogger.error("tools_def.txt parser", FILE_NOT_FOUND, ExtraData=FileName)

        for Index in range(len(FileContent)):
            Line = FileContent[Index].strip()
            if Line == "" or Line[0] == '#':
                continue

            if Line.startswith("!include"):
                IncFile = Line[8:].strip()
                Done, IncFile = self.ExpandMacros(IncFile)
                if not Done:
                    EdkLogger.error("tools_def.txt parser", ATTRIBUTE_NOT_AVAILABLE,
                                    "Macro or Environment has not been defined",
                                ExtraData=IncFile[4:-1], File=FileName, Line=Index+1)
                IncFile = NormPath(IncFile)

                if not os.path.isabs(IncFile):
                    #
                    # try WORKSPACE
                    #
                    IncFileTmp = PathClass(IncFile, GlobalData.gWorkspace)
                    ErrorCode = IncFileTmp.Validate()[0]
                    if ErrorCode != 0:
                        #
                        # try PACKAGES_PATH
                        #
                        IncFileTmp = mws.join(GlobalData.gWorkspace, IncFile)
                        if not os.path.exists(IncFileTmp):
                            #
                            # try directory of current file
                            #
                            IncFileTmp = PathClass(IncFile, os.path.dirname(FileName))
                            ErrorCode = IncFileTmp.Validate()[0]
                            if ErrorCode != 0:
                                EdkLogger.error("tools_def.txt parser", FILE_NOT_FOUND, ExtraData=IncFile)

                    if isinstance(IncFileTmp, PathClass):
                        IncFile = IncFileTmp.Path
                    else:
                        IncFile = IncFileTmp

                self.IncludeToolDefFile(IncFile)
                continue

            NameValuePair = Line.split("=", 1)
            if len(NameValuePair) != 2:
                EdkLogger.warn("tools_def.txt parser", "Line %d: not correct assignment statement, skipped" % (Index + 1))
                continue

            Name = NameValuePair[0].strip()
            Value = NameValuePair[1].strip()

            if Name == "IDENTIFIER":
                EdkLogger.debug(EdkLogger.DEBUG_8, "Line %d: Found identifier statement, skipped: %s" % ((Index + 1), Value))
                continue

            MacroDefinition = gMacroDefPattern.findall(Name)
            if MacroDefinition != []:
                Done, Value = self.ExpandMacros(Value)
                if not Done:
                    EdkLogger.error("tools_def.txt parser", ATTRIBUTE_NOT_AVAILABLE,
                                    "Macro or Environment has not been defined",
                                ExtraData=Value[4:-1], File=FileName, Line=Index+1)

                MacroName = MacroDefinition[0].strip()
                self.MacroDictionary["DEF(%s)" % MacroName] = Value
                EdkLogger.debug(EdkLogger.DEBUG_8, "Line %d: Found macro: %s = %s" % ((Index + 1), MacroName, Value))
                continue

            Done, Value = self.ExpandMacros(Value)
            if not Done:
                EdkLogger.error("tools_def.txt parser", ATTRIBUTE_NOT_AVAILABLE,
                                "Macro or Environment has not been defined",
                                ExtraData=Value[4:-1], File=FileName, Line=Index+1)

            List = Name.split('_')
            if len(List) != 5:
                EdkLogger.verbose("Line %d: Not a valid name of definition: %s" % ((Index + 1), Name))
                continue
            elif List[4] == TAB_STAR:
                EdkLogger.verbose("Line %d: '*' is not allowed in last field: %s" % ((Index + 1), Name))
                continue
            else:
                self.ToolsDefTxtDictionary[Name] = Value
                if List[0] != TAB_STAR:
                    self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TARGET] += [List[0]]
                if List[1] != TAB_STAR:
                    self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TOOL_CHAIN_TAG] += [List[1]]
                if List[2] != TAB_STAR:
                    self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TARGET_ARCH] += [List[2]]
                if List[3] != TAB_STAR:
                    self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_COMMAND_TYPE] += [List[3]]
                if List[4] == TAB_TOD_DEFINES_FAMILY and List[2] == TAB_STAR and List[3] == TAB_STAR:
                    if TAB_TOD_DEFINES_FAMILY not in self.ToolsDefTxtDatabase:
                        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_FAMILY] = {}
                        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_FAMILY][List[1]] = Value
                        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_BUILDRULEFAMILY] = {}
                        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_BUILDRULEFAMILY][List[1]] = Value
                    elif List[1] not in self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_FAMILY]:
                        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_FAMILY][List[1]] = Value
                        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_BUILDRULEFAMILY][List[1]] = Value
                    elif self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_FAMILY][List[1]] != Value:
                        EdkLogger.verbose("Line %d: No override allowed for the family of a tool chain: %s" % ((Index + 1), Name))
                if List[4] == TAB_TOD_DEFINES_BUILDRULEFAMILY and List[2] == TAB_STAR and List[3] == TAB_STAR:
                    if TAB_TOD_DEFINES_BUILDRULEFAMILY not in self.ToolsDefTxtDatabase \
                       or List[1] not in self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_FAMILY]:
                        EdkLogger.verbose("Line %d: The family is not specified, but BuildRuleFamily is specified for the tool chain: %s" % ((Index + 1), Name))
                    self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_BUILDRULEFAMILY][List[1]] = Value

    ## ExpandMacros
    #
    # Replace defined macros with real value
    #
    # @param Value:   The string with unreplaced macros
    #
    # @retval Value:  The string which has been replaced with real value
    #
    def ExpandMacros(self, Value):
        # os.environ contains all environment variables uppercase on Windows which cause the key in the self.MacroDictionary is uppercase, but Ref may not
        EnvReference = gEnvRefPattern.findall(Value)
        for Ref in EnvReference:
            if Ref not in self.MacroDictionary and Ref.upper() not in self.MacroDictionary:
                Value = Value.replace(Ref, "")
            else:
                if Ref in self.MacroDictionary:
                    Value = Value.replace(Ref, self.MacroDictionary[Ref])
                else:
                    Value = Value.replace(Ref, self.MacroDictionary[Ref.upper()])
        MacroReference = gMacroRefPattern.findall(Value)
        for Ref in MacroReference:
            if Ref not in self.MacroDictionary:
                return False, Ref
            Value = Value.replace(Ref, self.MacroDictionary[Ref])

        return True, Value

## ToolDefDict
#
# Load tools_def.txt in input Conf dir
#
# @param ConfDir:  Conf dir
#
# @retval ToolDef An instance of ToolDefClassObject() with loaded tools_def.txt
#
def ToolDefDict(ConfDir):
    Target = TargetTxtDict(ConfDir)
    ToolDef = ToolDefClassObject()
    if TAB_TAT_DEFINES_TOOL_CHAIN_CONF in Target.TargetTxtDictionary:
        ToolsDefFile = Target.TargetTxtDictionary[TAB_TAT_DEFINES_TOOL_CHAIN_CONF]
        if ToolsDefFile:
            ToolDef.LoadToolDefFile(os.path.normpath(ToolsDefFile))
        else:
            ToolDef.LoadToolDefFile(os.path.normpath(os.path.join(ConfDir, gDefaultToolsDefFile)))
    else:
        ToolDef.LoadToolDefFile(os.path.normpath(os.path.join(ConfDir, gDefaultToolsDefFile)))
    return ToolDef

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    ToolDef = ToolDefDict(os.getenv("WORKSPACE"))
    pass
