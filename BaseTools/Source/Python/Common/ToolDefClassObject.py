## @file
# This file is used to define each component of tools_def.txt file
#
# Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
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
import re
import EdkLogger

from Dictionary import *
from BuildToolError import *
from TargetTxtClassObject import *

##
# Static variables used for pattern
#
gMacroRefPattern = re.compile('(DEF\([^\(\)]+\))')
gEnvRefPattern = re.compile('(ENV\([^\(\)]+\))')
gMacroDefPattern = re.compile("DEFINE\s+([^\s]+)")
gDefaultToolsDefFile = "Conf/tools_def.txt"

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
    def __init__(self, FileName = None):
        self.ToolsDefTxtDictionary = {}
        self.MacroDictionary = {}
        for Env in os.environ:
            self.MacroDictionary["ENV(%s)" % Env] = os.environ[Env]

        if FileName != None:
            self.LoadToolDefFile(FileName)

    ## LoadToolDefFile
    #
    # Load target.txt file and parse it, return a set structure to store keys and values
    #
    # @param Filename:  Input value for full path of tools_def.txt
    #
    def LoadToolDefFile(self, FileName):
        FileContent = []
        if os.path.isfile(FileName):
            try:
                F = open(FileName,'r')
                FileContent = F.readlines()
            except:
                EdkLogger.error("tools_def.txt parser", FILE_OPEN_FAILURE, ExtraData=FileName)
        else:
            EdkLogger.error("tools_def.txt parser", FILE_NOT_FOUND, ExtraData=FileName)

        self.ToolsDefTxtDatabase = {
            TAB_TOD_DEFINES_TARGET          :   [],
            TAB_TOD_DEFINES_TOOL_CHAIN_TAG  :   [],
            TAB_TOD_DEFINES_TARGET_ARCH     :   [],
            TAB_TOD_DEFINES_COMMAND_TYPE    :   []
        }

        for Index in range(len(FileContent)):
            Line = FileContent[Index].strip()
            if Line == "" or Line[0] == '#':
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
            elif List[4] == '*':
                EdkLogger.verbose("Line %d: '*' is not allowed in last field: %s" % ((Index + 1), Name))
                continue
            else:
                self.ToolsDefTxtDictionary[Name] = Value
                if List[0] != '*':
                    self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TARGET] += [List[0]]
                if List[1] != '*':
                    self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TOOL_CHAIN_TAG] += [List[1]]
                if List[2] != '*':
                    self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TARGET_ARCH] += [List[2]]
                if List[3] != '*':
                    self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_COMMAND_TYPE] += [List[3]]
                if List[4] == TAB_TOD_DEFINES_FAMILY and List[2] == '*' and List[3] == '*':
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
                if List[4] == TAB_TOD_DEFINES_BUILDRULEFAMILY and List[2] == '*' and List[3] == '*':
                    if TAB_TOD_DEFINES_BUILDRULEFAMILY not in self.ToolsDefTxtDatabase \
                       or List[1] not in self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_FAMILY]:
                        EdkLogger.verbose("Line %d: The family is not specified, but BuildRuleFamily is specified for the tool chain: %s" % ((Index + 1), Name))
                    self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_BUILDRULEFAMILY][List[1]] = Value

        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TARGET] = list(set(self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TARGET]))
        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TOOL_CHAIN_TAG] = list(set(self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TOOL_CHAIN_TAG]))
        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TARGET_ARCH] = list(set(self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TARGET_ARCH]))
        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_COMMAND_TYPE] = list(set(self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_COMMAND_TYPE]))

        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TARGET].sort()
        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TOOL_CHAIN_TAG].sort()
        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TARGET_ARCH].sort()
        self.ToolsDefTxtDatabase[TAB_TOD_DEFINES_COMMAND_TYPE].sort()

        KeyList = [TAB_TOD_DEFINES_TARGET, TAB_TOD_DEFINES_TOOL_CHAIN_TAG, TAB_TOD_DEFINES_TARGET_ARCH, TAB_TOD_DEFINES_COMMAND_TYPE]
        for Index in range(3,-1,-1):
            for Key in dict(self.ToolsDefTxtDictionary):
                List = Key.split('_')
                if List[Index] == '*':
                    for String in self.ToolsDefTxtDatabase[KeyList[Index]]:
                        List[Index] = String
                        NewKey = '%s_%s_%s_%s_%s' % tuple(List)
                        if NewKey not in self.ToolsDefTxtDictionary:
                            self.ToolsDefTxtDictionary[NewKey] = self.ToolsDefTxtDictionary[Key]
                        continue
                    del self.ToolsDefTxtDictionary[Key]
                elif List[Index] not in self.ToolsDefTxtDatabase[KeyList[Index]]:
                    del self.ToolsDefTxtDictionary[Key]

    ## ExpandMacros
    #
    # Replace defined macros with real value
    #
    # @param Value:   The string with unreplaced macros
    #
    # @retval Value:  The string which has been replaced with real value
    #
    def ExpandMacros(self, Value):
        EnvReference = gEnvRefPattern.findall(Value)
        for Ref in EnvReference:
            if Ref not in self.MacroDictionary:
                Value = Value.replace(Ref, "")
            else:
                Value = Value.replace(Ref, self.MacroDictionary[Ref])
 

        MacroReference = gMacroRefPattern.findall(Value)
        for Ref in MacroReference:
            if Ref not in self.MacroDictionary:
                return False, Ref
            Value = Value.replace(Ref, self.MacroDictionary[Ref])

        return True, Value

## ToolDefDict
#
# Load tools_def.txt in input workspace dir
#
# @param WorkSpace:  Workspace dir
#
# @retval ToolDef An instance of ToolDefClassObject() with loaded tools_def.txt
#
def ToolDefDict(WorkSpace):
    Target = TargetTxtDict(WorkSpace)
    ToolDef = ToolDefClassObject()
    if DataType.TAB_TAT_DEFINES_TOOL_CHAIN_CONF in Target.TargetTxtDictionary:
        gDefaultToolsDefFile = Target.TargetTxtDictionary[DataType.TAB_TAT_DEFINES_TOOL_CHAIN_CONF]
    ToolDef.LoadToolDefFile(os.path.normpath(os.path.join(WorkSpace, gDefaultToolsDefFile)))
    return ToolDef

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    ToolDef = ToolDefDict(os.getenv("WORKSPACE"))
    pass
