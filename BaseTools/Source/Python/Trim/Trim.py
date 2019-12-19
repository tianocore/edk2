## @file
# Trim files preprocessed by compiler
#
# Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
import Common.LongFilePathOs as os
import sys
import re
from io import BytesIO
import codecs
from optparse import OptionParser
from optparse import make_option
from Common.BuildToolError import *
from Common.Misc import *
from Common.DataType import *
from Common.BuildVersion import gBUILD_VERSION
import Common.EdkLogger as EdkLogger
from Common.LongFilePathSupport import OpenLongFilePath as open

# Version and Copyright
__version_number__ = ("0.10" + " " + gBUILD_VERSION)
__version__ = "%prog Version " + __version_number__
__copyright__ = "Copyright (c) 2007-2018, Intel Corporation. All rights reserved."

## Regular expression for matching Line Control directive like "#line xxx"
gLineControlDirective = re.compile('^\s*#(?:line)?\s+([0-9]+)\s+"*([^"]*)"')
## Regular expression for matching "typedef struct"
gTypedefPattern = re.compile("^\s*typedef\s+struct(\s+\w+)?\s*[{]*$", re.MULTILINE)
## Regular expression for matching "#pragma pack"
gPragmaPattern = re.compile("^\s*#pragma\s+pack", re.MULTILINE)
## Regular expression for matching "typedef"
gTypedef_SinglePattern = re.compile("^\s*typedef", re.MULTILINE)
## Regular expression for matching "typedef struct, typedef union, struct, union"
gTypedef_MulPattern = re.compile("^\s*(typedef)?\s+(struct|union)(\s+\w+)?\s*[{]*$", re.MULTILINE)

#
# The following number pattern match will only match if following criteria is met:
# There is leading non-(alphanumeric or _) character, and no following alphanumeric or _
# as the pattern is greedily match, so it is ok for the gDecNumberPattern or gHexNumberPattern to grab the maximum match
#
## Regular expression for matching HEX number
gHexNumberPattern = re.compile("(?<=[^a-zA-Z0-9_])(0[xX])([0-9a-fA-F]+)(U(?=$|[^a-zA-Z0-9_]))?")
## Regular expression for matching decimal number with 'U' postfix
gDecNumberPattern = re.compile("(?<=[^a-zA-Z0-9_])([0-9]+)U(?=$|[^a-zA-Z0-9_])")
## Regular expression for matching constant with 'ULL' 'LL' postfix
gLongNumberPattern = re.compile("(?<=[^a-zA-Z0-9_])(0[xX][0-9a-fA-F]+|[0-9]+)U?LL(?=$|[^a-zA-Z0-9_])")

## Regular expression for matching "Include ()" in asl file
gAslIncludePattern = re.compile("^(\s*)[iI]nclude\s*\(\"?([^\"\(\)]+)\"\)", re.MULTILINE)
## Regular expression for matching C style #include "XXX.asl" in asl file
gAslCIncludePattern = re.compile(r'^(\s*)#include\s*[<"]\s*([-\\/\w.]+)\s*([>"])', re.MULTILINE)
## Patterns used to convert EDK conventions to EDK2 ECP conventions

## Regular expression for finding header file inclusions
gIncludePattern = re.compile(r"^[ \t]*[%]?[ \t]*include(?:[ \t]*(?:\\(?:\r\n|\r|\n))*[ \t]*)*(?:\(?[\"<]?[ \t]*)([-\w.\\/() \t]+)(?:[ \t]*[\">]?\)?)", re.MULTILINE | re.UNICODE | re.IGNORECASE)


## file cache to avoid circular include in ASL file
gIncludedAslFile = []

## Trim preprocessed source code
#
# Remove extra content made by preprocessor. The preprocessor must enable the
# line number generation option when preprocessing.
#
# @param  Source    File to be trimmed
# @param  Target    File to store the trimmed content
# @param  Convert   If True, convert standard HEX format to MASM format
#
def TrimPreprocessedFile(Source, Target, ConvertHex, TrimLong):
    CreateDirectory(os.path.dirname(Target))
    try:
        with open(Source, "r") as File:
            Lines = File.readlines()
    except IOError:
        EdkLogger.error("Trim", FILE_OPEN_FAILURE, ExtraData=Source)
    except:
        EdkLogger.error("Trim", AUTOGEN_ERROR, "TrimPreprocessedFile: Error while processing file", File=Source)

    PreprocessedFile = ""
    InjectedFile = ""
    LineIndexOfOriginalFile = None
    NewLines = []
    LineControlDirectiveFound = False
    for Index in range(len(Lines)):
        Line = Lines[Index]
        #
        # Find out the name of files injected by preprocessor from the lines
        # with Line Control directive
        #
        MatchList = gLineControlDirective.findall(Line)
        if MatchList != []:
            MatchList = MatchList[0]
            if len(MatchList) == 2:
                LineNumber = int(MatchList[0], 0)
                InjectedFile = MatchList[1]
                InjectedFile = os.path.normpath(InjectedFile)
                InjectedFile = os.path.normcase(InjectedFile)
                # The first injected file must be the preprocessed file itself
                if PreprocessedFile == "":
                    PreprocessedFile = InjectedFile
            LineControlDirectiveFound = True
            continue
        elif PreprocessedFile == "" or InjectedFile != PreprocessedFile:
            continue

        if LineIndexOfOriginalFile is None:
            #
            # Any non-empty lines must be from original preprocessed file.
            # And this must be the first one.
            #
            LineIndexOfOriginalFile = Index
            EdkLogger.verbose("Found original file content starting from line %d"
                              % (LineIndexOfOriginalFile + 1))

        if TrimLong:
            Line = gLongNumberPattern.sub(r"\1", Line)
        # convert HEX number format if indicated
        if ConvertHex:
            Line = gHexNumberPattern.sub(r"0\2h", Line)
        else:
            Line = gHexNumberPattern.sub(r"\1\2", Line)

        # convert Decimal number format
        Line = gDecNumberPattern.sub(r"\1", Line)

        if LineNumber is not None:
            EdkLogger.verbose("Got line directive: line=%d" % LineNumber)
            # in case preprocessor removed some lines, like blank or comment lines
            if LineNumber <= len(NewLines):
                # possible?
                NewLines[LineNumber - 1] = Line
            else:
                if LineNumber > (len(NewLines) + 1):
                    for LineIndex in range(len(NewLines), LineNumber-1):
                        NewLines.append(TAB_LINE_BREAK)
                NewLines.append(Line)
            LineNumber = None
            EdkLogger.verbose("Now we have lines: %d" % len(NewLines))
        else:
            NewLines.append(Line)

    # in case there's no line directive or linemarker found
    if (not LineControlDirectiveFound) and NewLines == []:
        MulPatternFlag = False
        SinglePatternFlag = False
        Brace = 0
        for Index in range(len(Lines)):
            Line = Lines[Index]
            if MulPatternFlag == False and gTypedef_MulPattern.search(Line) is None:
                if SinglePatternFlag == False and gTypedef_SinglePattern.search(Line) is None:
                    # remove "#pragram pack" directive
                    if gPragmaPattern.search(Line) is None:
                        NewLines.append(Line)
                    continue
                elif SinglePatternFlag == False:
                    SinglePatternFlag = True
                if Line.find(";") >= 0:
                    SinglePatternFlag = False
            elif MulPatternFlag == False:
                # found "typedef struct, typedef union, union, struct", keep its position and set a flag
                MulPatternFlag = True

            # match { and } to find the end of typedef definition
            if Line.find("{") >= 0:
                Brace += 1
            elif Line.find("}") >= 0:
                Brace -= 1

            # "typedef struct, typedef union, union, struct" must end with a ";"
            if Brace == 0 and Line.find(";") >= 0:
                MulPatternFlag = False

    # save to file
    try:
        with open(Target, 'w') as File:
            File.writelines(NewLines)
    except:
        EdkLogger.error("Trim", FILE_OPEN_FAILURE, ExtraData=Target)

## Trim preprocessed VFR file
#
# Remove extra content made by preprocessor. The preprocessor doesn't need to
# enable line number generation option when preprocessing.
#
# @param  Source    File to be trimmed
# @param  Target    File to store the trimmed content
#
def TrimPreprocessedVfr(Source, Target):
    CreateDirectory(os.path.dirname(Target))

    try:
        with open(Source, "r") as File:
            Lines = File.readlines()
    except:
        EdkLogger.error("Trim", FILE_OPEN_FAILURE, ExtraData=Source)
    # read whole file

    FoundTypedef = False
    Brace = 0
    TypedefStart = 0
    TypedefEnd = 0
    for Index in range(len(Lines)):
        Line = Lines[Index]
        # don't trim the lines from "formset" definition to the end of file
        if Line.strip() == 'formset':
            break

        if FoundTypedef == False and (Line.find('#line') == 0 or Line.find('# ') == 0):
            # empty the line number directive if it's not aomong "typedef struct"
            Lines[Index] = "\n"
            continue

        if FoundTypedef == False and gTypedefPattern.search(Line) is None:
            # keep "#pragram pack" directive
            if gPragmaPattern.search(Line) is None:
                Lines[Index] = "\n"
            continue
        elif FoundTypedef == False:
            # found "typedef struct", keept its position and set a flag
            FoundTypedef = True
            TypedefStart = Index

        # match { and } to find the end of typedef definition
        if Line.find("{") >= 0:
            Brace += 1
        elif Line.find("}") >= 0:
            Brace -= 1

        # "typedef struct" must end with a ";"
        if Brace == 0 and Line.find(";") >= 0:
            FoundTypedef = False
            TypedefEnd = Index
            # keep all "typedef struct" except to GUID, EFI_PLABEL and PAL_CALL_RETURN
            if Line.strip("} ;\r\n") in [TAB_GUID, "EFI_PLABEL", "PAL_CALL_RETURN"]:
                for i in range(TypedefStart, TypedefEnd+1):
                    Lines[i] = "\n"

    # save all lines trimmed
    try:
        with open(Target, 'w') as File:
            File.writelines(Lines)
    except:
        EdkLogger.error("Trim", FILE_OPEN_FAILURE, ExtraData=Target)

## Read the content  ASL file, including ASL included, recursively
#
# @param  Source            File to be read
# @param  Indent            Spaces before the Include() statement
# @param  IncludePathList   The list of external include file
# @param  LocalSearchPath   If LocalSearchPath is specified, this path will be searched
#                           first for the included file; otherwise, only the path specified
#                           in the IncludePathList will be searched.
#
def DoInclude(Source, Indent='', IncludePathList=[], LocalSearchPath=None, IncludeFileList = None, filetype=None):
    NewFileContent = []
    if IncludeFileList is None:
        IncludeFileList = []
    try:
        #
        # Search LocalSearchPath first if it is specified.
        #
        if LocalSearchPath:
            SearchPathList = [LocalSearchPath] + IncludePathList
        else:
            SearchPathList = IncludePathList

        for IncludePath in SearchPathList:
            IncludeFile = os.path.join(IncludePath, Source)
            if os.path.isfile(IncludeFile):
                try:
                    with open(IncludeFile, "r") as File:
                        F = File.readlines()
                except:
                    with codecs.open(IncludeFile, "r", encoding='utf-8') as File:
                        F = File.readlines()
                break
        else:
            EdkLogger.error("Trim", "Failed to find include file %s" % Source)
    except:
        EdkLogger.error("Trim", FILE_OPEN_FAILURE, ExtraData=Source)


    # avoid A "include" B and B "include" A
    IncludeFile = os.path.abspath(os.path.normpath(IncludeFile))
    if IncludeFile in gIncludedAslFile:
        EdkLogger.warn("Trim", "Circular include",
                       ExtraData= "%s -> %s" % (" -> ".join(gIncludedAslFile), IncludeFile))
        return []
    gIncludedAslFile.append(IncludeFile)
    IncludeFileList.append(IncludeFile.strip())
    for Line in F:
        LocalSearchPath = None
        if filetype == "ASL":
            Result = gAslIncludePattern.findall(Line)
            if len(Result) == 0:
                Result = gAslCIncludePattern.findall(Line)
                if len(Result) == 0 or os.path.splitext(Result[0][1])[1].lower() not in [".asl", ".asi"]:
                    NewFileContent.append("%s%s" % (Indent, Line))
                    continue
                #
                # We should first search the local directory if current file are using pattern #include "XXX"
                #
                if Result[0][2] == '"':
                    LocalSearchPath = os.path.dirname(IncludeFile)
            CurrentIndent = Indent + Result[0][0]
            IncludedFile = Result[0][1]
            NewFileContent.extend(DoInclude(IncludedFile, CurrentIndent, IncludePathList, LocalSearchPath,IncludeFileList,filetype))
            NewFileContent.append("\n")
        elif filetype == "ASM":
            Result = gIncludePattern.findall(Line)
            if len(Result) == 0:
                NewFileContent.append("%s%s" % (Indent, Line))
                continue

            IncludedFile = Result[0]

            IncludedFile = IncludedFile.strip()
            IncludedFile = os.path.normpath(IncludedFile)
            NewFileContent.extend(DoInclude(IncludedFile, '', IncludePathList, LocalSearchPath,IncludeFileList,filetype))
            NewFileContent.append("\n")

    gIncludedAslFile.pop()

    return NewFileContent


## Trim ASL file
#
# Replace ASL include statement with the content the included file
#
# @param  Source          File to be trimmed
# @param  Target          File to store the trimmed content
# @param  IncludePathFile The file to log the external include path
#
def TrimAslFile(Source, Target, IncludePathFile,AslDeps = False):
    CreateDirectory(os.path.dirname(Target))

    SourceDir = os.path.dirname(Source)
    if SourceDir == '':
        SourceDir = '.'

    #
    # Add source directory as the first search directory
    #
    IncludePathList = [SourceDir]

    #
    # If additional include path file is specified, append them all
    # to the search directory list.
    #
    if IncludePathFile:
        try:
            LineNum = 0
            with open(IncludePathFile, 'r') as File:
                FileLines = File.readlines()
            for Line in FileLines:
                LineNum += 1
                if Line.startswith("/I") or Line.startswith ("-I"):
                    IncludePathList.append(Line[2:].strip())
                else:
                    EdkLogger.warn("Trim", "Invalid include line in include list file.", IncludePathFile, LineNum)
        except:
            EdkLogger.error("Trim", FILE_OPEN_FAILURE, ExtraData=IncludePathFile)
    AslIncludes = []
    Lines = DoInclude(Source, '', IncludePathList,IncludeFileList=AslIncludes,filetype='ASL')
    AslIncludes = [item for item in AslIncludes if item !=Source]
    if AslDeps and AslIncludes:
        SaveFileOnChange(os.path.join(os.path.dirname(Target),os.path.basename(Source))+".trim.deps", " \\\n".join([Source+":"] +AslIncludes),False)

    #
    # Undef MIN and MAX to avoid collision in ASL source code
    #
    Lines.insert(0, "#undef MIN\n#undef MAX\n")

    # save all lines trimmed
    try:
        with open(Target, 'w') as File:
            File.writelines(Lines)
    except:
        EdkLogger.error("Trim", FILE_OPEN_FAILURE, ExtraData=Target)

## Trim ASM file
#
# Output ASM include statement with the content the included file
#
# @param  Source          File to be trimmed
# @param  Target          File to store the trimmed content
# @param  IncludePathFile The file to log the external include path
#
def TrimAsmFile(Source, Target, IncludePathFile):
    CreateDirectory(os.path.dirname(Target))

    SourceDir = os.path.dirname(Source)
    if SourceDir == '':
        SourceDir = '.'

    #
    # Add source directory as the first search directory
    #
    IncludePathList = [SourceDir]
    #
    # If additional include path file is specified, append them all
    # to the search directory list.
    #
    if IncludePathFile:
        try:
            LineNum = 0
            with open(IncludePathFile, 'r') as File:
                FileLines = File.readlines()
            for Line in FileLines:
                LineNum += 1
                if Line.startswith("/I") or Line.startswith ("-I"):
                    IncludePathList.append(Line[2:].strip())
                else:
                    EdkLogger.warn("Trim", "Invalid include line in include list file.", IncludePathFile, LineNum)
        except:
            EdkLogger.error("Trim", FILE_OPEN_FAILURE, ExtraData=IncludePathFile)
    AsmIncludes = []
    Lines = DoInclude(Source, '', IncludePathList,IncludeFileList=AsmIncludes,filetype='ASM')
    AsmIncludes = [item for item in AsmIncludes if item != Source]
    if AsmIncludes:
        SaveFileOnChange(os.path.join(os.path.dirname(Target),os.path.basename(Source))+".trim.deps", " \\\n".join([Source+":"] +AsmIncludes),False)
    # save all lines trimmed
    try:
        with open(Target, 'w') as File:
            File.writelines(Lines)
    except:
        EdkLogger.error("Trim", FILE_OPEN_FAILURE, ExtraData=Target)

def GenerateVfrBinSec(ModuleName, DebugDir, OutputFile):
    VfrNameList = []
    if os.path.isdir(DebugDir):
        for CurrentDir, Dirs, Files in os.walk(DebugDir):
            for FileName in Files:
                Name, Ext = os.path.splitext(FileName)
                if Ext == '.c' and Name != 'AutoGen':
                    VfrNameList.append (Name + 'Bin')

    VfrNameList.append (ModuleName + 'Strings')

    EfiFileName = os.path.join(DebugDir, ModuleName + '.efi')
    MapFileName = os.path.join(DebugDir, ModuleName + '.map')
    VfrUniOffsetList = GetVariableOffset(MapFileName, EfiFileName, VfrNameList)

    if not VfrUniOffsetList:
        return

    try:
        fInputfile = open(OutputFile, "wb+")
    except:
        EdkLogger.error("Trim", FILE_OPEN_FAILURE, "File open failed for %s" %OutputFile, None)

    # Use a instance of BytesIO to cache data
    fStringIO = BytesIO()

    for Item in VfrUniOffsetList:
        if (Item[0].find("Strings") != -1):
            #
            # UNI offset in image.
            # GUID + Offset
            # { 0x8913c5e0, 0x33f6, 0x4d86, { 0x9b, 0xf1, 0x43, 0xef, 0x89, 0xfc, 0x6, 0x66 } }
            #
            UniGuid = b'\xe0\xc5\x13\x89\xf63\x86M\x9b\xf1C\xef\x89\xfc\x06f'
            fStringIO.write(UniGuid)
            UniValue = pack ('Q', int (Item[1], 16))
            fStringIO.write (UniValue)
        else:
            #
            # VFR binary offset in image.
            # GUID + Offset
            # { 0xd0bc7cb4, 0x6a47, 0x495f, { 0xaa, 0x11, 0x71, 0x7, 0x46, 0xda, 0x6, 0xa2 } };
            #
            VfrGuid = b'\xb4|\xbc\xd0Gj_I\xaa\x11q\x07F\xda\x06\xa2'
            fStringIO.write(VfrGuid)
            type (Item[1])
            VfrValue = pack ('Q', int (Item[1], 16))
            fStringIO.write (VfrValue)

    #
    # write data into file.
    #
    try :
        fInputfile.write (fStringIO.getvalue())
    except:
        EdkLogger.error("Trim", FILE_WRITE_FAILURE, "Write data to file %s failed, please check whether the file been locked or using by other applications." %OutputFile, None)

    fStringIO.close ()
    fInputfile.close ()


## Parse command line options
#
# Using standard Python module optparse to parse command line option of this tool.
#
# @retval Options   A optparse.Values object containing the parsed options
# @retval InputFile Path of file to be trimmed
#
def Options():
    OptionList = [
        make_option("-s", "--source-code", dest="FileType", const="SourceCode", action="store_const",
                          help="The input file is preprocessed source code, including C or assembly code"),
        make_option("-r", "--vfr-file", dest="FileType", const="Vfr", action="store_const",
                          help="The input file is preprocessed VFR file"),
        make_option("--Vfr-Uni-Offset", dest="FileType", const="VfrOffsetBin", action="store_const",
                          help="The input file is EFI image"),
        make_option("--asl-deps", dest="AslDeps", const="True", action="store_const",
                          help="Generate Asl dependent files."),
        make_option("-a", "--asl-file", dest="FileType", const="Asl", action="store_const",
                          help="The input file is ASL file"),
        make_option( "--asm-file", dest="FileType", const="Asm", action="store_const",
                          help="The input file is asm file"),
        make_option("-c", "--convert-hex", dest="ConvertHex", action="store_true",
                          help="Convert standard hex format (0xabcd) to MASM format (abcdh)"),

        make_option("-l", "--trim-long", dest="TrimLong", action="store_true",
                          help="Remove postfix of long number"),
        make_option("-i", "--include-path-file", dest="IncludePathFile",
                          help="The input file is include path list to search for ASL include file"),
        make_option("-o", "--output", dest="OutputFile",
                          help="File to store the trimmed content"),
        make_option("--ModuleName", dest="ModuleName", help="The module's BASE_NAME"),
        make_option("--DebugDir", dest="DebugDir",
                          help="Debug Output directory to store the output files"),
        make_option("-v", "--verbose", dest="LogLevel", action="store_const", const=EdkLogger.VERBOSE,
                          help="Run verbosely"),
        make_option("-d", "--debug", dest="LogLevel", type="int",
                          help="Run with debug information"),
        make_option("-q", "--quiet", dest="LogLevel", action="store_const", const=EdkLogger.QUIET,
                          help="Run quietly"),
        make_option("-?", action="help", help="show this help message and exit"),
    ]

    # use clearer usage to override default usage message
    UsageString = "%prog [-s|-r|-a|--Vfr-Uni-Offset] [-c] [-v|-d <debug_level>|-q] [-i <include_path_file>] [-o <output_file>] [--ModuleName <ModuleName>] [--DebugDir <DebugDir>] [<input_file>]"

    Parser = OptionParser(description=__copyright__, version=__version__, option_list=OptionList, usage=UsageString)
    Parser.set_defaults(FileType="Vfr")
    Parser.set_defaults(ConvertHex=False)
    Parser.set_defaults(LogLevel=EdkLogger.INFO)

    Options, Args = Parser.parse_args()

    # error check
    if Options.FileType == 'VfrOffsetBin':
        if len(Args) == 0:
            return Options, ''
        elif len(Args) > 1:
            EdkLogger.error("Trim", OPTION_NOT_SUPPORTED, ExtraData=Parser.get_usage())
    if len(Args) == 0:
        EdkLogger.error("Trim", OPTION_MISSING, ExtraData=Parser.get_usage())
    if len(Args) > 1:
        EdkLogger.error("Trim", OPTION_NOT_SUPPORTED, ExtraData=Parser.get_usage())

    InputFile = Args[0]
    return Options, InputFile

## Entrance method
#
# This method mainly dispatch specific methods per the command line options.
# If no error found, return zero value so the caller of this tool can know
# if it's executed successfully or not.
#
# @retval 0     Tool was successful
# @retval 1     Tool failed
#
def Main():
    try:
        EdkLogger.Initialize()
        CommandOptions, InputFile = Options()
        if CommandOptions.LogLevel < EdkLogger.DEBUG_9:
            EdkLogger.SetLevel(CommandOptions.LogLevel + 1)
        else:
            EdkLogger.SetLevel(CommandOptions.LogLevel)
    except FatalError as X:
        return 1

    try:
        if CommandOptions.FileType == "Vfr":
            if CommandOptions.OutputFile is None:
                CommandOptions.OutputFile = os.path.splitext(InputFile)[0] + '.iii'
            TrimPreprocessedVfr(InputFile, CommandOptions.OutputFile)
        elif CommandOptions.FileType == "Asl":
            if CommandOptions.OutputFile is None:
                CommandOptions.OutputFile = os.path.splitext(InputFile)[0] + '.iii'
            TrimAslFile(InputFile, CommandOptions.OutputFile, CommandOptions.IncludePathFile,CommandOptions.AslDeps)
        elif CommandOptions.FileType == "VfrOffsetBin":
            GenerateVfrBinSec(CommandOptions.ModuleName, CommandOptions.DebugDir, CommandOptions.OutputFile)
        elif CommandOptions.FileType == "Asm":
            TrimAsmFile(InputFile, CommandOptions.OutputFile, CommandOptions.IncludePathFile)
        else :
            if CommandOptions.OutputFile is None:
                CommandOptions.OutputFile = os.path.splitext(InputFile)[0] + '.iii'
            TrimPreprocessedFile(InputFile, CommandOptions.OutputFile, CommandOptions.ConvertHex, CommandOptions.TrimLong)
    except FatalError as X:
        import platform
        import traceback
        if CommandOptions is not None and CommandOptions.LogLevel <= EdkLogger.DEBUG_9:
            EdkLogger.quiet("(Python %s on %s) " % (platform.python_version(), sys.platform) + traceback.format_exc())
        return 1
    except:
        import traceback
        import platform
        EdkLogger.error(
                    "\nTrim",
                    CODE_ERROR,
                    "Unknown fatal error when trimming [%s]" % InputFile,
                    ExtraData="\n(Please send email to %s for help, attaching following call stack trace!)\n" % MSG_EDKII_MAIL_ADDR,
                    RaiseError=False
                    )
        EdkLogger.quiet("(Python %s on %s) " % (platform.python_version(), sys.platform) + traceback.format_exc())
        return 1

    return 0

if __name__ == '__main__':
    r = Main()
    ## 0-127 is a safe return range, and 1 is a standard default error
    if r < 0 or r > 127: r = 1
    sys.exit(r)

