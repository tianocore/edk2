## @file
# This file is used to generate DEPEX file for module's dependency expression
#
# Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent

## Import Modules
#
import sys
import Common.LongFilePathOs as os
import re
import traceback
from Common.LongFilePathSupport import OpenLongFilePath as open
from io import BytesIO
from struct import pack
from Common.BuildToolError import *
from Common.Misc import SaveFileOnChange
from Common.Misc import GuidStructureStringToGuidString
from Common.Misc import GuidStructureByteArrayToGuidString
from Common.Misc import GuidStringToGuidStructureString
from Common import EdkLogger as EdkLogger
from Common.BuildVersion import gBUILD_VERSION
from Common.DataType import *

## Regular expression for matching "DEPENDENCY_START ... DEPENDENCY_END"
gStartClosePattern = re.compile(".*DEPENDENCY_START(.+)DEPENDENCY_END.*", re.S)

## Mapping between module type and EFI phase
gType2Phase = {
    SUP_MODULE_BASE              :   None,
    SUP_MODULE_SEC               :   "PEI",
    SUP_MODULE_PEI_CORE          :   "PEI",
    SUP_MODULE_PEIM              :   "PEI",
    SUP_MODULE_DXE_CORE          :   "DXE",
    SUP_MODULE_DXE_DRIVER        :   "DXE",
    SUP_MODULE_DXE_SMM_DRIVER    :   "DXE",
    SUP_MODULE_DXE_RUNTIME_DRIVER:   "DXE",
    SUP_MODULE_DXE_SAL_DRIVER    :   "DXE",
    SUP_MODULE_UEFI_DRIVER       :   "DXE",
    SUP_MODULE_UEFI_APPLICATION  :   "DXE",
    SUP_MODULE_SMM_CORE          :   "DXE",
    SUP_MODULE_MM_STANDALONE     :   "MM",
    SUP_MODULE_MM_CORE_STANDALONE :  "MM",
}

## Convert dependency expression string into EFI internal representation
#
#   DependencyExpression class is used to parse dependency expression string and
# convert it into its binary form.
#
class DependencyExpression:

    ArchProtocols = {
                        '665e3ff6-46cc-11d4-9a38-0090273fc14d',     #   'gEfiBdsArchProtocolGuid'
                        '26baccb1-6f42-11d4-bce7-0080c73c8881',     #   'gEfiCpuArchProtocolGuid'
                        '26baccb2-6f42-11d4-bce7-0080c73c8881',     #   'gEfiMetronomeArchProtocolGuid'
                        '1da97072-bddc-4b30-99f1-72a0b56fff2a',     #   'gEfiMonotonicCounterArchProtocolGuid'
                        '27cfac87-46cc-11d4-9a38-0090273fc14d',     #   'gEfiRealTimeClockArchProtocolGuid'
                        '27cfac88-46cc-11d4-9a38-0090273fc14d',     #   'gEfiResetArchProtocolGuid'
                        'b7dfb4e1-052f-449f-87be-9818fc91b733',     #   'gEfiRuntimeArchProtocolGuid'
                        'a46423e3-4617-49f1-b9ff-d1bfa9115839',     #   'gEfiSecurityArchProtocolGuid'
                        '26baccb3-6f42-11d4-bce7-0080c73c8881',     #   'gEfiTimerArchProtocolGuid'
                        '6441f818-6362-4e44-b570-7dba31dd2453',     #   'gEfiVariableWriteArchProtocolGuid'
                        '1e5668e2-8481-11d4-bcf1-0080c73c8881',     #   'gEfiVariableArchProtocolGuid'
                        '665e3ff5-46cc-11d4-9a38-0090273fc14d'      #   'gEfiWatchdogTimerArchProtocolGuid'
                    }

    OpcodePriority = {
        DEPEX_OPCODE_AND   :   1,
        DEPEX_OPCODE_OR    :   1,
        DEPEX_OPCODE_NOT   :   2,
    }

    Opcode = {
        "PEI"   : {
            DEPEX_OPCODE_PUSH  :   0x02,
            DEPEX_OPCODE_AND   :   0x03,
            DEPEX_OPCODE_OR    :   0x04,
            DEPEX_OPCODE_NOT   :   0x05,
            DEPEX_OPCODE_TRUE  :   0x06,
            DEPEX_OPCODE_FALSE :   0x07,
            DEPEX_OPCODE_END   :   0x08
        },

        "DXE"   : {
            DEPEX_OPCODE_BEFORE:   0x00,
            DEPEX_OPCODE_AFTER :   0x01,
            DEPEX_OPCODE_PUSH  :   0x02,
            DEPEX_OPCODE_AND   :   0x03,
            DEPEX_OPCODE_OR    :   0x04,
            DEPEX_OPCODE_NOT   :   0x05,
            DEPEX_OPCODE_TRUE  :   0x06,
            DEPEX_OPCODE_FALSE :   0x07,
            DEPEX_OPCODE_END   :   0x08,
            DEPEX_OPCODE_SOR   :   0x09
        },

        "MM"   : {
            DEPEX_OPCODE_BEFORE:   0x00,
            DEPEX_OPCODE_AFTER :   0x01,
            DEPEX_OPCODE_PUSH  :   0x02,
            DEPEX_OPCODE_AND   :   0x03,
            DEPEX_OPCODE_OR    :   0x04,
            DEPEX_OPCODE_NOT   :   0x05,
            DEPEX_OPCODE_TRUE  :   0x06,
            DEPEX_OPCODE_FALSE :   0x07,
            DEPEX_OPCODE_END   :   0x08,
            DEPEX_OPCODE_SOR   :   0x09
        }
    }

    # all supported op codes and operands
    SupportedOpcode = [DEPEX_OPCODE_BEFORE, DEPEX_OPCODE_AFTER, DEPEX_OPCODE_PUSH, DEPEX_OPCODE_AND, DEPEX_OPCODE_OR, DEPEX_OPCODE_NOT, DEPEX_OPCODE_END, DEPEX_OPCODE_SOR]
    SupportedOperand = [DEPEX_OPCODE_TRUE, DEPEX_OPCODE_FALSE]

    OpcodeWithSingleOperand = [DEPEX_OPCODE_NOT, DEPEX_OPCODE_BEFORE, DEPEX_OPCODE_AFTER]
    OpcodeWithTwoOperand = [DEPEX_OPCODE_AND, DEPEX_OPCODE_OR]

    # op code that should not be the last one
    NonEndingOpcode = [DEPEX_OPCODE_AND, DEPEX_OPCODE_OR, DEPEX_OPCODE_NOT, DEPEX_OPCODE_SOR]
    # op code must not present at the same time
    ExclusiveOpcode = [DEPEX_OPCODE_BEFORE, DEPEX_OPCODE_AFTER]
    # op code that should be the first one if it presents
    AboveAllOpcode = [DEPEX_OPCODE_SOR, DEPEX_OPCODE_BEFORE, DEPEX_OPCODE_AFTER]

    #
    # open and close brace must be taken as individual tokens
    #
    TokenPattern = re.compile("(\(|\)|\{[^{}]+\{?[^{}]+\}?[ ]*\}|\w+)")

    ## Constructor
    #
    #   @param  Expression  The list or string of dependency expression
    #   @param  ModuleType  The type of the module using the dependency expression
    #
    def __init__(self, Expression, ModuleType, Optimize=False):
        self.ModuleType = ModuleType
        self.Phase = gType2Phase[ModuleType]
        if isinstance(Expression, type([])):
            self.ExpressionString = " ".join(Expression)
            self.TokenList = Expression
        else:
            self.ExpressionString = Expression
            self.GetExpressionTokenList()

        self.PostfixNotation = []
        self.OpcodeList = []

        self.GetPostfixNotation()
        self.ValidateOpcode()

        EdkLogger.debug(EdkLogger.DEBUG_8, repr(self))
        if Optimize:
            self.Optimize()
            EdkLogger.debug(EdkLogger.DEBUG_8, "\n    Optimized: " + repr(self))

    def __str__(self):
        return " ".join(self.TokenList)

    def __repr__(self):
        WellForm = ''
        for Token in self.PostfixNotation:
            if Token in self.SupportedOpcode:
                WellForm += "\n    " + Token
            else:
                WellForm += ' ' + Token
        return WellForm

    ## Split the expression string into token list
    def GetExpressionTokenList(self):
        self.TokenList = self.TokenPattern.findall(self.ExpressionString)

    ## Convert token list into postfix notation
    def GetPostfixNotation(self):
        Stack = []
        LastToken = ''
        for Token in self.TokenList:
            if Token == "(":
                if LastToken not in self.SupportedOpcode + ['(', '', None]:
                    EdkLogger.error("GenDepex", PARSER_ERROR, "Invalid dependency expression: missing operator before open parentheses",
                                    ExtraData="Near %s" % LastToken)
                Stack.append(Token)
            elif Token == ")":
                if '(' not in Stack:
                    EdkLogger.error("GenDepex", PARSER_ERROR, "Invalid dependency expression: mismatched parentheses",
                                    ExtraData=str(self))
                elif LastToken in self.SupportedOpcode + ['', None]:
                    EdkLogger.error("GenDepex", PARSER_ERROR, "Invalid dependency expression: missing operand before close parentheses",
                                    ExtraData="Near %s" % LastToken)
                while len(Stack) > 0:
                    if Stack[-1] == '(':
                        Stack.pop()
                        break
                    self.PostfixNotation.append(Stack.pop())
            elif Token in self.OpcodePriority:
                if Token == DEPEX_OPCODE_NOT:
                    if LastToken not in self.SupportedOpcode + ['(', '', None]:
                        EdkLogger.error("GenDepex", PARSER_ERROR, "Invalid dependency expression: missing operator before NOT",
                                        ExtraData="Near %s" % LastToken)
                elif LastToken in self.SupportedOpcode + ['(', '', None]:
                        EdkLogger.error("GenDepex", PARSER_ERROR, "Invalid dependency expression: missing operand before " + Token,
                                        ExtraData="Near %s" % LastToken)

                while len(Stack) > 0:
                    if Stack[-1] == "(" or self.OpcodePriority[Token] >= self.OpcodePriority[Stack[-1]]:
                        break
                    self.PostfixNotation.append(Stack.pop())
                Stack.append(Token)
                self.OpcodeList.append(Token)
            else:
                if Token not in self.SupportedOpcode:
                    # not OP, take it as GUID
                    if LastToken not in self.SupportedOpcode + ['(', '', None]:
                        EdkLogger.error("GenDepex", PARSER_ERROR, "Invalid dependency expression: missing operator before %s" % Token,
                                        ExtraData="Near %s" % LastToken)
                    if len(self.OpcodeList) == 0 or self.OpcodeList[-1] not in self.ExclusiveOpcode:
                        if Token not in self.SupportedOperand:
                            self.PostfixNotation.append(DEPEX_OPCODE_PUSH)
                # check if OP is valid in this phase
                elif Token in self.Opcode[self.Phase]:
                    if Token == DEPEX_OPCODE_END:
                        break
                    self.OpcodeList.append(Token)
                else:
                    EdkLogger.error("GenDepex", PARSER_ERROR,
                                    "Opcode=%s doesn't supported in %s stage " % (Token, self.Phase),
                                    ExtraData=str(self))
                self.PostfixNotation.append(Token)
            LastToken = Token

        # there should not be parentheses in Stack
        if '(' in Stack or ')' in Stack:
            EdkLogger.error("GenDepex", PARSER_ERROR, "Invalid dependency expression: mismatched parentheses",
                            ExtraData=str(self))
        while len(Stack) > 0:
            self.PostfixNotation.append(Stack.pop())
        if self.PostfixNotation[-1] != DEPEX_OPCODE_END:
            self.PostfixNotation.append(DEPEX_OPCODE_END)

    ## Validate the dependency expression
    def ValidateOpcode(self):
        for Op in self.AboveAllOpcode:
            if Op in self.PostfixNotation:
                if Op != self.PostfixNotation[0]:
                    EdkLogger.error("GenDepex", PARSER_ERROR, "%s should be the first opcode in the expression" % Op,
                                    ExtraData=str(self))
                if len(self.PostfixNotation) < 3:
                    EdkLogger.error("GenDepex", PARSER_ERROR, "Missing operand for %s" % Op,
                                    ExtraData=str(self))
        for Op in self.ExclusiveOpcode:
            if Op in self.OpcodeList:
                if len(self.OpcodeList) > 1:
                    EdkLogger.error("GenDepex", PARSER_ERROR, "%s should be the only opcode in the expression" % Op,
                                    ExtraData=str(self))
                if len(self.PostfixNotation) < 3:
                    EdkLogger.error("GenDepex", PARSER_ERROR, "Missing operand for %s" % Op,
                                    ExtraData=str(self))
        if self.TokenList[-1] != DEPEX_OPCODE_END and self.TokenList[-1] in self.NonEndingOpcode:
            EdkLogger.error("GenDepex", PARSER_ERROR, "Extra %s at the end of the dependency expression" % self.TokenList[-1],
                            ExtraData=str(self))
        if self.TokenList[-1] == DEPEX_OPCODE_END and self.TokenList[-2] in self.NonEndingOpcode:
            EdkLogger.error("GenDepex", PARSER_ERROR, "Extra %s at the end of the dependency expression" % self.TokenList[-2],
                            ExtraData=str(self))
        if DEPEX_OPCODE_END in self.TokenList and DEPEX_OPCODE_END != self.TokenList[-1]:
            EdkLogger.error("GenDepex", PARSER_ERROR, "Extra expressions after END",
                            ExtraData=str(self))

    ## Simply optimize the dependency expression by removing duplicated operands
    def Optimize(self):
        OpcodeSet = set(self.OpcodeList)
        # if there are isn't one in the set, return
        if len(OpcodeSet) != 1:
          return
        Op = OpcodeSet.pop()
        #if Op isn't either OR or AND, return
        if Op not in [DEPEX_OPCODE_AND, DEPEX_OPCODE_OR]:
            return
        NewOperand = []
        AllOperand = set()
        for Token in self.PostfixNotation:
            if Token in self.SupportedOpcode or Token in NewOperand:
                continue
            AllOperand.add(Token)
            if Token == DEPEX_OPCODE_TRUE:
                if Op == DEPEX_OPCODE_AND:
                    continue
                else:
                    NewOperand.append(Token)
                    break
            elif Token == DEPEX_OPCODE_FALSE:
                if Op == DEPEX_OPCODE_OR:
                    continue
                else:
                    NewOperand.append(Token)
                    break
            NewOperand.append(Token)

        # don't generate depex if only TRUE operand left
        if self.ModuleType == SUP_MODULE_PEIM and len(NewOperand) == 1 and NewOperand[0] == DEPEX_OPCODE_TRUE:
            self.PostfixNotation = []
            return

        # don't generate depex if all operands are architecture protocols
        if self.ModuleType in [SUP_MODULE_UEFI_DRIVER, SUP_MODULE_DXE_DRIVER, SUP_MODULE_DXE_RUNTIME_DRIVER, SUP_MODULE_DXE_SAL_DRIVER, SUP_MODULE_DXE_SMM_DRIVER, SUP_MODULE_MM_STANDALONE] and \
           Op == DEPEX_OPCODE_AND and \
           self.ArchProtocols == set(GuidStructureStringToGuidString(Guid) for Guid in AllOperand):
            self.PostfixNotation = []
            return

        if len(NewOperand) == 0:
            self.TokenList = list(AllOperand)
        else:
            self.TokenList = []
            while True:
                self.TokenList.append(NewOperand.pop(0))
                if NewOperand == []:
                    break
                self.TokenList.append(Op)
        self.PostfixNotation = []
        self.GetPostfixNotation()


    ## Convert a GUID value in C structure format into its binary form
    #
    #   @param  Guid    The GUID value in C structure format
    #
    #   @retval array   The byte array representing the GUID value
    #
    def GetGuidValue(self, Guid):
        GuidValueString = Guid.replace("{", "").replace("}", "").replace(" ", "")
        GuidValueList = GuidValueString.split(",")
        if len(GuidValueList) != 11 and len(GuidValueList) == 16:
            GuidValueString = GuidStringToGuidStructureString(GuidStructureByteArrayToGuidString(Guid))
            GuidValueString = GuidValueString.replace("{", "").replace("}", "").replace(" ", "")
            GuidValueList = GuidValueString.split(",")
        if len(GuidValueList) != 11:
            EdkLogger.error("GenDepex", PARSER_ERROR, "Invalid GUID value string or opcode: %s" % Guid)
        return pack("1I2H8B", *(int(value, 16) for value in GuidValueList))

    ## Save the binary form of dependency expression in file
    #
    #   @param  File    The path of file. If None is given, put the data on console
    #
    #   @retval True    If the file doesn't exist or file is changed
    #   @retval False   If file exists and is not changed.
    #
    def Generate(self, File=None):
        Buffer = BytesIO()
        if len(self.PostfixNotation) == 0:
            return False

        for Item in self.PostfixNotation:
            if Item in self.Opcode[self.Phase]:
                Buffer.write(pack("B", self.Opcode[self.Phase][Item]))
            elif Item in self.SupportedOpcode:
                EdkLogger.error("GenDepex", FORMAT_INVALID,
                                "Opcode [%s] is not expected in %s phase" % (Item, self.Phase),
                                ExtraData=self.ExpressionString)
            else:
                Buffer.write(self.GetGuidValue(Item))

        FilePath = ""
        FileChangeFlag = True
        if File is None:
            sys.stdout.write(Buffer.getvalue())
            FilePath = "STDOUT"
        else:
            FileChangeFlag = SaveFileOnChange(File, Buffer.getvalue(), True)

        Buffer.close()
        return FileChangeFlag

versionNumber = ("0.04" + " " + gBUILD_VERSION)
__version__ = "%prog Version " + versionNumber
__copyright__ = "Copyright (c) 2007-2018, Intel Corporation  All rights reserved."
__usage__ = "%prog [options] [dependency_expression_file]"

## Parse command line options
#
#   @retval OptionParser
#
def GetOptions():
    from optparse import OptionParser

    Parser = OptionParser(description=__copyright__, version=__version__, usage=__usage__)

    Parser.add_option("-o", "--output", dest="OutputFile", default=None, metavar="FILE",
                      help="Specify the name of depex file to be generated")
    Parser.add_option("-t", "--module-type", dest="ModuleType", default=None,
                      help="The type of module for which the dependency expression serves")
    Parser.add_option("-e", "--dependency-expression", dest="Expression", default="",
                      help="The string of dependency expression. If this option presents, the input file will be ignored.")
    Parser.add_option("-m", "--optimize", dest="Optimize", default=False, action="store_true",
                      help="Do some simple optimization on the expression.")
    Parser.add_option("-v", "--verbose", dest="verbose", default=False, action="store_true",
                      help="build with verbose information")
    Parser.add_option("-d", "--debug", action="store", type="int", help="Enable debug messages at specified level.")
    Parser.add_option("-q", "--quiet", dest="quiet", default=False, action="store_true",
                      help="build with little information")

    return Parser.parse_args()


## Entrance method
#
# @retval 0     Tool was successful
# @retval 1     Tool failed
#
def Main():
    EdkLogger.Initialize()
    Option, Input = GetOptions()

    # Set log level
    if Option.quiet:
        EdkLogger.SetLevel(EdkLogger.QUIET)
    elif Option.verbose:
        EdkLogger.SetLevel(EdkLogger.VERBOSE)
    elif Option.debug is not None:
        EdkLogger.SetLevel(Option.debug + 1)
    else:
        EdkLogger.SetLevel(EdkLogger.INFO)

    try:
        if Option.ModuleType is None or Option.ModuleType not in gType2Phase:
            EdkLogger.error("GenDepex", OPTION_MISSING, "Module type is not specified or supported")

        DxsFile = ''
        if len(Input) > 0 and Option.Expression == "":
            DxsFile = Input[0]
            DxsString = open(DxsFile, 'r').read().replace("\n", " ").replace("\r", " ")
            DxsString = gStartClosePattern.sub("\\1", DxsString)
        elif Option.Expression != "":
            if Option.Expression[0] == '"':
                DxsString = Option.Expression[1:-1]
            else:
                DxsString = Option.Expression
        else:
            EdkLogger.error("GenDepex", OPTION_MISSING, "No expression string or file given")

        Dpx = DependencyExpression(DxsString, Option.ModuleType, Option.Optimize)
        if Option.OutputFile is not None:
            FileChangeFlag = Dpx.Generate(Option.OutputFile)
            if not FileChangeFlag and DxsFile:
                #
                # Touch the output file if its time stamp is older than the original
                # DXS file to avoid re-invoke this tool for the dependency check in build rule.
                #
                if os.stat(DxsFile)[8] > os.stat(Option.OutputFile)[8]:
                    os.utime(Option.OutputFile, None)
        else:
            Dpx.Generate()
    except BaseException as X:
        EdkLogger.quiet("")
        if Option is not None and Option.debug is not None:
            EdkLogger.quiet(traceback.format_exc())
        else:
            EdkLogger.quiet(str(X))
        return 1

    return 0

if __name__ == '__main__':
    sys.exit(Main())

