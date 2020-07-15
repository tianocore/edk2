## @file
# This file is used to define each component of Target.txt file
#
# Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import print_function
from __future__ import absolute_import

import Common.GlobalData as GlobalData
import Common.LongFilePathOs as os
from . import EdkLogger
from . import DataType
from .BuildToolError import *

from Common.LongFilePathSupport import OpenLongFilePath as open
from Common.MultipleWorkspace import MultipleWorkspace as mws

gDefaultTargetTxtFile = "target.txt"

## TargetTxtClassObject
#
# This class defined content used in file target.txt
#
# @param object:             Inherited from object class
# @param Filename:           Input value for full path of target.txt
#
# @var TargetTxtDictionary:  To store keys and values defined in target.txt
#
class TargetTxtClassObject(object):
    def __init__(self, Filename = None):
        self.TargetTxtDictionary = {
            DataType.TAB_TAT_DEFINES_ACTIVE_PLATFORM                            : '',
            DataType.TAB_TAT_DEFINES_ACTIVE_MODULE                              : '',
            DataType.TAB_TAT_DEFINES_TOOL_CHAIN_CONF                            : '',
            DataType.TAB_TAT_DEFINES_MAX_CONCURRENT_THREAD_NUMBER               : '',
            DataType.TAB_TAT_DEFINES_TARGET                                     : [],
            DataType.TAB_TAT_DEFINES_TOOL_CHAIN_TAG                             : [],
            DataType.TAB_TAT_DEFINES_TARGET_ARCH                                : [],
            DataType.TAB_TAT_DEFINES_BUILD_RULE_CONF                            : '',
        }
        self.ConfDirectoryPath = ""
        if Filename is not None:
            self.LoadTargetTxtFile(Filename)

    ## LoadTargetTxtFile
    #
    # Load target.txt file and parse it, return a set structure to store keys and values
    #
    # @param Filename:  Input value for full path of target.txt
    #
    # @retval set() A set structure to store keys and values
    # @retval 1     Error happenes in parsing
    #
    def LoadTargetTxtFile(self, Filename):
        if os.path.exists(Filename) and os.path.isfile(Filename):
             return self.ConvertTextFileToDict(Filename, '#', '=')
        else:
            EdkLogger.error("Target.txt Parser", FILE_NOT_FOUND, ExtraData=Filename)
            return 1

    ## ConvertTextFileToDict
    #
    # Convert a text file to a dictionary of (name:value) pairs.
    # The data is saved to self.TargetTxtDictionary
    #
    # @param FileName:             Text filename
    # @param CommentCharacter:     Comment char, be used to ignore comment content
    # @param KeySplitCharacter:    Key split char, between key name and key value. Key1 = Value1, '=' is the key split char
    #
    # @retval 0 Convert successfully
    # @retval 1 Open file failed
    #
    def ConvertTextFileToDict(self, FileName, CommentCharacter, KeySplitCharacter):
        F = None
        try:
            F = open(FileName, 'r')
            self.ConfDirectoryPath = os.path.dirname(FileName)
        except:
            EdkLogger.error("build", FILE_OPEN_FAILURE, ExtraData=FileName)
            if F is not None:
                F.close()

        for Line in F:
            Line = Line.strip()
            if Line.startswith(CommentCharacter) or Line == '':
                continue

            LineList = Line.split(KeySplitCharacter, 1)
            Key = LineList[0].strip()
            if len(LineList) == 2:
                Value = LineList[1].strip()
            else:
                Value = ""

            if Key in [DataType.TAB_TAT_DEFINES_ACTIVE_PLATFORM, DataType.TAB_TAT_DEFINES_TOOL_CHAIN_CONF, \
                       DataType.TAB_TAT_DEFINES_ACTIVE_MODULE, DataType.TAB_TAT_DEFINES_BUILD_RULE_CONF]:
                self.TargetTxtDictionary[Key] = Value.replace('\\', '/')
                if Key == DataType.TAB_TAT_DEFINES_TOOL_CHAIN_CONF and self.TargetTxtDictionary[Key]:
                    if self.TargetTxtDictionary[Key].startswith("Conf/"):
                        Tools_Def = os.path.join(self.ConfDirectoryPath, self.TargetTxtDictionary[Key].strip())
                        if not os.path.exists(Tools_Def) or not os.path.isfile(Tools_Def):
                            # If Conf/Conf does not exist, try just the Conf/ directory
                            Tools_Def = os.path.join(self.ConfDirectoryPath, self.TargetTxtDictionary[Key].replace("Conf/", "", 1).strip())
                    else:
                        # The File pointed to by TOOL_CHAIN_CONF is not in a Conf/ directory
                        Tools_Def = os.path.join(self.ConfDirectoryPath, self.TargetTxtDictionary[Key].strip())
                    self.TargetTxtDictionary[Key] = Tools_Def
                if Key == DataType.TAB_TAT_DEFINES_BUILD_RULE_CONF and self.TargetTxtDictionary[Key]:
                    if self.TargetTxtDictionary[Key].startswith("Conf/"):
                        Build_Rule = os.path.join(self.ConfDirectoryPath, self.TargetTxtDictionary[Key].strip())
                        if not os.path.exists(Build_Rule) or not os.path.isfile(Build_Rule):
                            # If Conf/Conf does not exist, try just the Conf/ directory
                            Build_Rule = os.path.join(self.ConfDirectoryPath, self.TargetTxtDictionary[Key].replace("Conf/", "", 1).strip())
                    else:
                        # The File pointed to by BUILD_RULE_CONF is not in a Conf/ directory
                        Build_Rule = os.path.join(self.ConfDirectoryPath, self.TargetTxtDictionary[Key].strip())
                    self.TargetTxtDictionary[Key] = Build_Rule
            elif Key in [DataType.TAB_TAT_DEFINES_TARGET, DataType.TAB_TAT_DEFINES_TARGET_ARCH, \
                         DataType.TAB_TAT_DEFINES_TOOL_CHAIN_TAG]:
                self.TargetTxtDictionary[Key] = Value.split()
            elif Key == DataType.TAB_TAT_DEFINES_MAX_CONCURRENT_THREAD_NUMBER:
                try:
                    V = int(Value, 0)
                except:
                    EdkLogger.error("build", FORMAT_INVALID, "Invalid number of [%s]: %s." % (Key, Value),
                                    File=FileName)
                self.TargetTxtDictionary[Key] = Value
            #elif Key not in GlobalData.gGlobalDefines:
            #    GlobalData.gGlobalDefines[Key] = Value

        F.close()
        return 0

## TargetTxtDict
#
# Load target.txt in input Conf dir
#
# @param ConfDir:  Conf dir
#
# @retval Target An instance of TargetTxtClassObject() with loaded target.txt
#

class TargetTxtDict():

    def __new__(cls, *args, **kw):
        if not hasattr(cls, '_instance'):
            orig = super(TargetTxtDict, cls)
            cls._instance = orig.__new__(cls, *args, **kw)
        return cls._instance

    def __init__(self):
        if not hasattr(self, 'Target'):
            self.TxtTarget = None

    @property
    def Target(self):
        if not self.TxtTarget:
            self._GetTarget()
        return self.TxtTarget

    def _GetTarget(self):
        Target = TargetTxtClassObject()
        ConfDirectory = GlobalData.gCmdConfDir
        if ConfDirectory:
            # Get alternate Conf location, if it is absolute, then just use the absolute directory name
            ConfDirectoryPath = os.path.normpath(ConfDirectory)

            if not os.path.isabs(ConfDirectoryPath):
                # Since alternate directory name is not absolute, the alternate directory is located within the WORKSPACE
                # This also handles someone specifying the Conf directory in the workspace. Using --conf=Conf
                ConfDirectoryPath = mws.join(os.environ["WORKSPACE"], ConfDirectoryPath)
        else:
            if "CONF_PATH" in os.environ:
                ConfDirectoryPath = os.path.normcase(os.path.normpath(os.environ["CONF_PATH"]))
            else:
                # Get standard WORKSPACE/Conf use the absolute path to the WORKSPACE/Conf
                ConfDirectoryPath = mws.join(os.environ["WORKSPACE"], 'Conf')
        GlobalData.gConfDirectory = ConfDirectoryPath
        targettxt = os.path.normpath(os.path.join(ConfDirectoryPath, gDefaultTargetTxtFile))
        if os.path.exists(targettxt):
            Target.LoadTargetTxtFile(targettxt)
        self.TxtTarget = Target

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    pass
    Target = TargetTxtDict(os.getenv("WORKSPACE"))
    print(Target.TargetTxtDictionary[DataType.TAB_TAT_DEFINES_MAX_CONCURRENT_THREAD_NUMBER])
    print(Target.TargetTxtDictionary[DataType.TAB_TAT_DEFINES_TARGET])
    print(Target.TargetTxtDictionary)
