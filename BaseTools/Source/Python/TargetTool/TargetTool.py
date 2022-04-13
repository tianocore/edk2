## @file
# Target Tool Parser
#
#  Copyright (c) 2007 - 2021, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

from __future__ import print_function
import Common.LongFilePathOs as os
import sys
import traceback
from optparse import OptionParser

import Common.EdkLogger as EdkLogger
import Common.BuildToolError as BuildToolError
from Common.DataType import *
from Common.BuildVersion import gBUILD_VERSION
from Common.LongFilePathSupport import OpenLongFilePath as open
from Common.TargetTxtClassObject import gDefaultTargetTxtFile

# To Do 1.set clean, 2. add item, if the line is disabled.

class TargetTool():
    def __init__(self, opt, args):
        self.WorkSpace = os.path.normpath(os.getenv('WORKSPACE'))
        self.Opt       = opt
        self.Arg       = args[0]
        self.FileName  = os.path.normpath(os.path.join(self.WorkSpace, 'Conf', gDefaultTargetTxtFile))
        if os.path.isfile(self.FileName) == False:
            print("%s does not exist." % self.FileName)
            sys.exit(1)
        self.TargetTxtDictionary = {
            TAB_TAT_DEFINES_ACTIVE_PLATFORM                            : None,
            TAB_TAT_DEFINES_TOOL_CHAIN_CONF                            : None,
            TAB_TAT_DEFINES_MAX_CONCURRENT_THREAD_NUMBER               : None,
            TAB_TAT_DEFINES_TARGET                                     : None,
            TAB_TAT_DEFINES_TOOL_CHAIN_TAG                             : None,
            TAB_TAT_DEFINES_TARGET_ARCH                                : None,
            TAB_TAT_DEFINES_BUILD_RULE_CONF                            : None,
        }
        self.LoadTargetTxtFile(self.FileName)

    def LoadTargetTxtFile(self, filename):
        if os.path.exists(filename) and os.path.isfile(filename):
            return self.ConvertTextFileToDict(filename, '#', '=')
        else:
            raise ParseError('LoadTargetTxtFile() : No Target.txt file exists.')
            return 1

#
# Convert a text file to a dictionary
#
    def ConvertTextFileToDict(self, FileName, CommentCharacter, KeySplitCharacter):
        """Convert a text file to a dictionary of (name:value) pairs."""
        try:
            f = open(FileName, 'r')
            for Line in f:
                if Line.startswith(CommentCharacter) or Line.strip() == '':
                    continue
                LineList = Line.split(KeySplitCharacter, 1)
                if len(LineList) >= 2:
                    Key = LineList[0].strip()
                    if Key.startswith(CommentCharacter) == False and Key in self.TargetTxtDictionary:
                        if Key == TAB_TAT_DEFINES_ACTIVE_PLATFORM or Key == TAB_TAT_DEFINES_TOOL_CHAIN_CONF \
                          or Key == TAB_TAT_DEFINES_MAX_CONCURRENT_THREAD_NUMBER \
                          or Key == TAB_TAT_DEFINES_ACTIVE_MODULE:
                            self.TargetTxtDictionary[Key] = LineList[1].replace('\\', '/').strip()
                        elif Key == TAB_TAT_DEFINES_TARGET or Key == TAB_TAT_DEFINES_TARGET_ARCH \
                          or Key == TAB_TAT_DEFINES_TOOL_CHAIN_TAG or Key == TAB_TAT_DEFINES_BUILD_RULE_CONF:
                            self.TargetTxtDictionary[Key] = LineList[1].split()
            f.close()
            return 0
        except:
            last_type, last_value, last_tb = sys.exc_info()
            traceback.print_exception(last_type, last_value, last_tb)

    def Print(self):
        errMsg  = ''
        for Key in self.TargetTxtDictionary:
            if isinstance(self.TargetTxtDictionary[Key], type([])):
                print("%-30s = %s" % (Key, ''.join(elem + ' ' for elem in self.TargetTxtDictionary[Key])))
            elif self.TargetTxtDictionary[Key] is None:
                errMsg += "  Missing %s configuration information, please use TargetTool to set value!" % Key + os.linesep
            else:
                print("%-30s = %s" % (Key, self.TargetTxtDictionary[Key]))

        if errMsg != '':
            print(os.linesep + 'Warning:' + os.linesep + errMsg)

    def RWFile(self, CommentCharacter, KeySplitCharacter, Num):
        try:
            fr = open(self.FileName, 'r')
            fw = open(os.path.normpath(os.path.join(self.WorkSpace, 'Conf\\targetnew.txt')), 'w')

            existKeys = []
            for Line in fr:
                if Line.startswith(CommentCharacter) or Line.strip() == '':
                    fw.write(Line)
                else:
                    LineList = Line.split(KeySplitCharacter, 1)
                    if len(LineList) >= 2:
                        Key = LineList[0].strip()
                        if Key.startswith(CommentCharacter) == False and Key in self.TargetTxtDictionary:
                            if Key not in existKeys:
                                existKeys.append(Key)
                            else:
                                print("Warning: Found duplicate key item in original configuration files!")

                            if Num == 0:
                                Line = "%-30s = \n" % Key
                            else:
                                ret = GetConfigureKeyValue(self, Key)
                                if ret is not None:
                                    Line = ret
                            fw.write(Line)
            for key in self.TargetTxtDictionary:
                if key not in existKeys:
                    print("Warning: %s does not exist in original configuration file" % key)
                    Line = GetConfigureKeyValue(self, key)
                    if Line is None:
                        Line = "%-30s = " % key
                    fw.write(Line)

            fr.close()
            fw.close()
            os.remove(self.FileName)
            os.rename(os.path.normpath(os.path.join(self.WorkSpace, 'Conf\\targetnew.txt')), self.FileName)

        except:
            last_type, last_value, last_tb = sys.exc_info()
            traceback.print_exception(last_type, last_value, last_tb)

def GetConfigureKeyValue(self, Key):
    Line = None
    if Key == TAB_TAT_DEFINES_ACTIVE_PLATFORM and self.Opt.DSCFILE is not None:
        dscFullPath = os.path.join(self.WorkSpace, self.Opt.DSCFILE)
        if os.path.exists(dscFullPath):
            Line = "%-30s = %s\n" % (Key, self.Opt.DSCFILE)
        else:
            EdkLogger.error("TargetTool", BuildToolError.FILE_NOT_FOUND,
                            "DSC file %s does not exist!" % self.Opt.DSCFILE, RaiseError=False)
    elif Key == TAB_TAT_DEFINES_TOOL_CHAIN_CONF and self.Opt.TOOL_DEFINITION_FILE is not None:
        tooldefFullPath = os.path.join(self.WorkSpace, self.Opt.TOOL_DEFINITION_FILE)
        if os.path.exists(tooldefFullPath):
            Line = "%-30s = %s\n" % (Key, self.Opt.TOOL_DEFINITION_FILE)
        else:
            EdkLogger.error("TargetTool", BuildToolError.FILE_NOT_FOUND,
                            "Tooldef file %s does not exist!" % self.Opt.TOOL_DEFINITION_FILE, RaiseError=False)

    elif self.Opt.NUM >= 2:
        Line = "%-30s = %s\n" % (Key, 'Enable')
    elif self.Opt.NUM <= 1:
        Line = "%-30s = %s\n" % (Key, 'Disable')
    elif Key == TAB_TAT_DEFINES_MAX_CONCURRENT_THREAD_NUMBER and self.Opt.NUM is not None:
        Line = "%-30s = %s\n" % (Key, str(self.Opt.NUM))
    elif Key == TAB_TAT_DEFINES_TARGET and self.Opt.TARGET is not None:
        Line = "%-30s = %s\n" % (Key, ''.join(elem + ' ' for elem in self.Opt.TARGET))
    elif Key == TAB_TAT_DEFINES_TARGET_ARCH and self.Opt.TARGET_ARCH is not None:
        Line = "%-30s = %s\n" % (Key, ''.join(elem + ' ' for elem in self.Opt.TARGET_ARCH))
    elif Key == TAB_TAT_DEFINES_TOOL_CHAIN_TAG and self.Opt.TOOL_CHAIN_TAG is not None:
        Line = "%-30s = %s\n" % (Key, self.Opt.TOOL_CHAIN_TAG)
    elif Key == TAB_TAT_DEFINES_BUILD_RULE_CONF and self.Opt.BUILD_RULE_FILE is not None:
        buildruleFullPath = os.path.join(self.WorkSpace, self.Opt.BUILD_RULE_FILE)
        if os.path.exists(buildruleFullPath):
            Line = "%-30s = %s\n" % (Key, self.Opt.BUILD_RULE_FILE)
        else:
            EdkLogger.error("TagetTool", BuildToolError.FILE_NOT_FOUND,
                            "Build rule file %s does not exist!" % self.Opt.BUILD_RULE_FILE, RaiseError=False)
    return Line

VersionNumber = ("0.01" + " " + gBUILD_VERSION)
__version__ = "%prog Version " + VersionNumber
__copyright__ = "Copyright (c) 2007 - 2018, Intel Corporation  All rights reserved."
__usage__ = "%prog [options] {args} \
\nArgs:                                                  \
\n Clean  clean the all default configuration of target.txt. \
\n Print  print the all default configuration of target.txt. \
\n Set    replace the default configuration with expected value specified by option."

gParamCheck = []
def SingleCheckCallback(option, opt_str, value, parser):
    if option not in gParamCheck:
        setattr(parser.values, option.dest, value)
        gParamCheck.append(option)
    else:
        parser.error("Option %s only allows one instance in command line!" % option)

def RangeCheckCallback(option, opt_str, value, parser):
    if option not in gParamCheck:
        gParamCheck.append(option)
        if value < 1 or value > 8:
            parser.error("The count of multi-thread is not in valid range of 1 ~ 8.")
        else:
            setattr(parser.values, option.dest, value)
    else:
        parser.error("Option %s only allows one instance in command line!" % option)

def MyOptionParser():
    parser = OptionParser(version=__version__, prog="TargetTool.exe", usage=__usage__, description=__copyright__)
    parser.add_option("-a", "--arch", action="append", dest="TARGET_ARCH",
        help="ARCHS is one of list: IA32, X64, ARM, AARCH64 or EBC, which replaces target.txt's TARGET_ARCH definition. To specify more archs, please repeat this option. 0 will clear this setting in target.txt and can't combine with other value.")
    parser.add_option("-p", "--platform", action="callback", type="string", dest="DSCFILE", callback=SingleCheckCallback,
        help="Specify a DSC file, which replace target.txt's ACTIVE_PLATFORM definition. 0 will clear this setting in target.txt and can't combine with other value.")
    parser.add_option("-c", "--tooldef", action="callback", type="string", dest="TOOL_DEFINITION_FILE", callback=SingleCheckCallback,
        help="Specify the WORKSPACE relative path of tool_def.txt file, which replace target.txt's TOOL_CHAIN_CONF definition. 0 will clear this setting in target.txt and can't combine with other value.")
    parser.add_option("-t", "--target", action="append", type="choice", choices=['DEBUG', 'RELEASE', '0'], dest="TARGET",
        help="TARGET is one of list: DEBUG, RELEASE, which replaces target.txt's TARGET definition. To specify more TARGET, please repeat this option. 0 will clear this setting in target.txt and can't combine with other value.")
    parser.add_option("-n", "--tagname", action="callback", type="string", dest="TOOL_CHAIN_TAG", callback=SingleCheckCallback,
        help="Specify the Tool Chain Tagname, which replaces target.txt's TOOL_CHAIN_TAG definition. 0 will clear this setting in target.txt and can't combine with other value.")
    parser.add_option("-r", "--buildrule", action="callback", type="string", dest="BUILD_RULE_FILE", callback=SingleCheckCallback,
        help="Specify the build rule configure file, which replaces target.txt's BUILD_RULE_CONF definition. If not specified, the default value Conf/build_rule.txt will be set.")
    parser.add_option("-m", "--multithreadnum", action="callback", type="int", dest="NUM", callback=RangeCheckCallback,
        help="Specify the multi-thread number which replace target.txt's MAX_CONCURRENT_THREAD_NUMBER. If the value is less than 2, MULTIPLE_THREAD will be disabled. If the value is larger than 1, MULTIPLE_THREAD will be enabled.")
    (opt, args)=parser.parse_args()
    return (opt, args)

if __name__ == '__main__':
    EdkLogger.Initialize()
    EdkLogger.SetLevel(EdkLogger.QUIET)
    if os.getenv('WORKSPACE') is None:
        print("ERROR: WORKSPACE should be specified or edksetup script should be executed before run TargetTool")
        sys.exit(1)

    (opt, args) = MyOptionParser()
    if len(args) != 1 or (args[0].lower() != 'print' and args[0].lower() != 'clean' and args[0].lower() != 'set'):
        print("The number of args isn't 1 or the value of args is invalid.")
        sys.exit(1)
    if opt.NUM is not None and opt.NUM < 1:
        print("The MAX_CONCURRENT_THREAD_NUMBER must be larger than 0.")
        sys.exit(1)
    if opt.TARGET is not None and len(opt.TARGET) > 1:
        for elem in opt.TARGET:
            if elem == '0':
                print("0 will clear the TARGET setting in target.txt and can't combine with other value.")
                sys.exit(1)
    if opt.TARGET_ARCH is not None and len(opt.TARGET_ARCH) > 1:
        for elem in opt.TARGET_ARCH:
            if elem == '0':
                print("0 will clear the TARGET_ARCH setting in target.txt and can't combine with other value.")
                sys.exit(1)

    try:
        FileHandle = TargetTool(opt, args)
        if FileHandle.Arg.lower() == 'print':
            FileHandle.Print()
            sys.exit(0)
        elif FileHandle.Arg.lower() == 'clean':
            FileHandle.RWFile('#', '=', 0)
        else:
            FileHandle.RWFile('#', '=', 1)
    except Exception as e:
        last_type, last_value, last_tb = sys.exc_info()
        traceback.print_exception(last_type, last_value, last_tb)

