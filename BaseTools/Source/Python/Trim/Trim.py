## @file
# Trim files preprocessed by compiler
#
# Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
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
import Common.LongFilePathOs as os
import sys
import re

from optparse import OptionParser
from optparse import make_option
from Common.BuildToolError import *
from Common.Misc import *
from Common.BuildVersion import gBUILD_VERSION
import Common.EdkLogger as EdkLogger
from Common.LongFilePathSupport import OpenLongFilePath as open

# Version and Copyright
__version_number__ = ("0.10" + " " + gBUILD_VERSION)
__version__ = "%prog Version " + __version_number__
__copyright__ = "Copyright (c) 2007-2010, Intel Corporation. All rights reserved."

## Regular expression for matching Line Control directive like "#line xxx"
gLineControlDirective = re.compile('^\s*#(?:line)?\s+([0-9]+)\s+"*([^"]*)"')
## Regular expression for matching "typedef struct"
gTypedefPattern = re.compile("^\s*typedef\s+struct(\s+\w+)?\s*[{]*$", re.MULTILINE)
## Regular expression for matching "#pragma pack"
gPragmaPattern = re.compile("^\s*#pragma\s+pack", re.MULTILINE)

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
gImportCodePatterns = [
    [
        re.compile('^(\s*)\(\*\*PeiServices\)\.PciCfg\s*=\s*([^;\s]+);', re.MULTILINE),
        '''\\1{
\\1  STATIC EFI_PEI_PPI_DESCRIPTOR gEcpPeiPciCfgPpiList = {
\\1    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
\\1    &gEcpPeiPciCfgPpiGuid,
\\1    \\2
\\1  };
\\1  (**PeiServices).InstallPpi (PeiServices, &gEcpPeiPciCfgPpiList);
\\1}'''
    ],

    [
        re.compile('^(\s*)\(\*PeiServices\)->PciCfg\s*=\s*([^;\s]+);', re.MULTILINE),
        '''\\1{
\\1  STATIC EFI_PEI_PPI_DESCRIPTOR gEcpPeiPciCfgPpiList = {
\\1    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
\\1    &gEcpPeiPciCfgPpiGuid,
\\1    \\2
\\1  };
\\1  (**PeiServices).InstallPpi (PeiServices, &gEcpPeiPciCfgPpiList);
\\1}'''
    ],

    [
        re.compile("(\s*).+->Modify[\s\n]*\(", re.MULTILINE),
        '\\1PeiLibPciCfgModify ('
    ],

    [
        re.compile("(\W*)gRT->ReportStatusCode[\s\n]*\(", re.MULTILINE),
        '\\1EfiLibReportStatusCode ('
    ],

    [
        re.compile('#include\s+EFI_GUID_DEFINITION\s*\(FirmwareFileSystem\)', re.MULTILINE),
        '#include EFI_GUID_DEFINITION (FirmwareFileSystem)\n#include EFI_GUID_DEFINITION (FirmwareFileSystem2)'
    ],

    [
        re.compile('gEfiFirmwareFileSystemGuid', re.MULTILINE),
        'gEfiFirmwareFileSystem2Guid'
    ],

    [
        re.compile('EFI_FVH_REVISION', re.MULTILINE),
        'EFI_FVH_PI_REVISION'
    ],

    [
        re.compile("(\s*)\S*CreateEvent\s*\([\s\n]*EFI_EVENT_SIGNAL_READY_TO_BOOT[^,]*,((?:[^;]+\n)+)(\s*\));", re.MULTILINE),
        '\\1EfiCreateEventReadyToBoot (\\2\\3;'
    ],

    [
        re.compile("(\s*)\S*CreateEvent\s*\([\s\n]*EFI_EVENT_SIGNAL_LEGACY_BOOT[^,]*,((?:[^;]+\n)+)(\s*\));", re.MULTILINE),
        '\\1EfiCreateEventLegacyBoot (\\2\\3;'
    ],
#    [
#        re.compile("(\W)(PEI_PCI_CFG_PPI)(\W)", re.MULTILINE),
#        '\\1ECP_\\2\\3'
#    ]
]

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
        f = open (Source, 'r')
    except:
        EdkLogger.error("Trim", FILE_OPEN_FAILURE, ExtraData=Source)

    # read whole file
    Lines = f.readlines()
    f.close()

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
                # The first injetcted file must be the preprocessed file itself
                if PreprocessedFile == "":
                    PreprocessedFile = InjectedFile
            LineControlDirectiveFound = True
            continue
        elif PreprocessedFile == "" or InjectedFile != PreprocessedFile:
            continue

        if LineIndexOfOriginalFile == None:
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

        if LineNumber != None:
            EdkLogger.verbose("Got line directive: line=%d" % LineNumber)
            # in case preprocessor removed some lines, like blank or comment lines
            if LineNumber <= len(NewLines):
                # possible?
                NewLines[LineNumber - 1] = Line
            else:
                if LineNumber > (len(NewLines) + 1):
                    for LineIndex in range(len(NewLines), LineNumber-1):
                        NewLines.append(os.linesep)
                NewLines.append(Line)
            LineNumber = None
            EdkLogger.verbose("Now we have lines: %d" % len(NewLines))
        else:
            NewLines.append(Line)

    # in case there's no line directive or linemarker found
    if (not LineControlDirectiveFound) and NewLines == []:
        NewLines = Lines

    # save to file
    try:
        f = open (Target, 'wb')
    except:
        EdkLogger.error("Trim", FILE_OPEN_FAILURE, ExtraData=Target)
    f.writelines(NewLines)
    f.close()

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
        f = open (Source,'r')
    except:
        EdkLogger.error("Trim", FILE_OPEN_FAILURE, ExtraData=Source)
    # read whole file
    Lines = f.readlines()
    f.close()

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

        if FoundTypedef == False and gTypedefPattern.search(Line) == None:
            # keep "#pragram pack" directive
            if gPragmaPattern.search(Line) == None:
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
            if Line.strip("} ;\r\n") in ["GUID", "EFI_PLABEL", "PAL_CALL_RETURN"]:
                for i in range(TypedefStart, TypedefEnd+1):
                    Lines[i] = "\n"

    # save all lines trimmed
    try:
        f = open (Target,'w')
    except:
        EdkLogger.error("Trim", FILE_OPEN_FAILURE, ExtraData=Target)
    f.writelines(Lines)
    f.close()

## Read the content  ASL file, including ASL included, recursively
#
# @param  Source            File to be read
# @param  Indent            Spaces before the Include() statement
# @param  IncludePathList   The list of external include file
# @param  LocalSearchPath   If LocalSearchPath is specified, this path will be searched
#                           first for the included file; otherwise, only the path specified
#                           in the IncludePathList will be searched.
#
def DoInclude(Source, Indent='', IncludePathList=[], LocalSearchPath=None):
    NewFileContent = []

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
                F = open(IncludeFile, "r")
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
    
    for Line in F:
        LocalSearchPath = None
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
        NewFileContent.extend(DoInclude(IncludedFile, CurrentIndent, IncludePathList, LocalSearchPath))
        NewFileContent.append("\n")

    gIncludedAslFile.pop()
    F.close()

    return NewFileContent


## Trim ASL file
#
# Replace ASL include statement with the content the included file
#
# @param  Source          File to be trimmed
# @param  Target          File to store the trimmed content
# @param  IncludePathFile The file to log the external include path 
#
def TrimAslFile(Source, Target, IncludePathFile):
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
            for Line in open(IncludePathFile,'r'):
                LineNum += 1
                if Line.startswith("/I") or Line.startswith ("-I"):
                    IncludePathList.append(Line[2:].strip())
                else:
                    EdkLogger.warn("Trim", "Invalid include line in include list file.", IncludePathFile, LineNum)
        except:
            EdkLogger.error("Trim", FILE_OPEN_FAILURE, ExtraData=IncludePathFile)

    Lines = DoInclude(Source, '', IncludePathList)

    #
    # Undef MIN and MAX to avoid collision in ASL source code
    #
    Lines.insert(0, "#undef MIN\n#undef MAX\n")

    # save all lines trimmed
    try:
        f = open (Target,'w')
    except:
        EdkLogger.error("Trim", FILE_OPEN_FAILURE, ExtraData=Target)

    f.writelines(Lines)
    f.close()

## Trim EDK source code file(s)
#
#
# @param  Source    File or directory to be trimmed
# @param  Target    File or directory to store the trimmed content
#
def TrimEdkSources(Source, Target):
    if os.path.isdir(Source):
        for CurrentDir, Dirs, Files in os.walk(Source):
            if '.svn' in Dirs:
                Dirs.remove('.svn')
            elif "CVS" in Dirs:
                Dirs.remove("CVS")

            for FileName in Files:
                Dummy, Ext = os.path.splitext(FileName)
                if Ext.upper() not in ['.C', '.H']: continue
                if Target == None or Target == '':
                    TrimEdkSourceCode(
                        os.path.join(CurrentDir, FileName),
                        os.path.join(CurrentDir, FileName)
                        )
                else:
                    TrimEdkSourceCode(
                        os.path.join(CurrentDir, FileName),
                        os.path.join(Target, CurrentDir[len(Source)+1:], FileName)
                        )
    else:
        TrimEdkSourceCode(Source, Target)

## Trim one EDK source code file
#
# Do following replacement:
#
#   (**PeiServices\).PciCfg = <*>;
#   =>  {
#         STATIC EFI_PEI_PPI_DESCRIPTOR gEcpPeiPciCfgPpiList = {
#         (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
#         &gEcpPeiPciCfgPpiGuid,
#         <*>
#       };
#       (**PeiServices).InstallPpi (PeiServices, &gEcpPeiPciCfgPpiList);
#
#   <*>Modify(<*>)
#   =>  PeiLibPciCfgModify (<*>)
#
#   gRT->ReportStatusCode (<*>)
#   => EfiLibReportStatusCode (<*>)
#
#   #include <LoadFile\.h>
#   =>  #include <FvLoadFile.h>
#
#   CreateEvent (EFI_EVENT_SIGNAL_READY_TO_BOOT, <*>)
#   => EfiCreateEventReadyToBoot (<*>)
#
#   CreateEvent (EFI_EVENT_SIGNAL_LEGACY_BOOT, <*>)
#   =>  EfiCreateEventLegacyBoot (<*>)
#
# @param  Source    File to be trimmed
# @param  Target    File to store the trimmed content
#
def TrimEdkSourceCode(Source, Target):
    EdkLogger.verbose("\t%s -> %s" % (Source, Target))
    CreateDirectory(os.path.dirname(Target))

    try:
        f = open (Source,'rb')
    except:
        EdkLogger.error("Trim", FILE_OPEN_FAILURE, ExtraData=Source)
    # read whole file
    Lines = f.read()
    f.close()

    NewLines = None
    for Re,Repl in gImportCodePatterns:
        if NewLines == None:
            NewLines = Re.sub(Repl, Lines)
        else:
            NewLines = Re.sub(Repl, NewLines)

    # save all lines if trimmed
    if Source == Target and NewLines == Lines:
        return

    try:
        f = open (Target,'wb')
    except:
        EdkLogger.error("Trim", FILE_OPEN_FAILURE, ExtraData=Target)
    f.write(NewLines)
    f.close()


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
        make_option("-a", "--asl-file", dest="FileType", const="Asl", action="store_const",
                          help="The input file is ASL file"),
        make_option("-8", "--Edk-source-code", dest="FileType", const="EdkSourceCode", action="store_const",
                          help="The input file is source code for Edk to be trimmed for ECP"),

        make_option("-c", "--convert-hex", dest="ConvertHex", action="store_true",
                          help="Convert standard hex format (0xabcd) to MASM format (abcdh)"),

        make_option("-l", "--trim-long", dest="TrimLong", action="store_true",
                          help="Remove postfix of long number"),
        make_option("-i", "--include-path-file", dest="IncludePathFile",
                          help="The input file is include path list to search for ASL include file"),
        make_option("-o", "--output", dest="OutputFile",
                          help="File to store the trimmed content"),
        make_option("-v", "--verbose", dest="LogLevel", action="store_const", const=EdkLogger.VERBOSE,
                          help="Run verbosely"),
        make_option("-d", "--debug", dest="LogLevel", type="int",
                          help="Run with debug information"),
        make_option("-q", "--quiet", dest="LogLevel", action="store_const", const=EdkLogger.QUIET,
                          help="Run quietly"),
        make_option("-?", action="help", help="show this help message and exit"),
    ]

    # use clearer usage to override default usage message
    UsageString = "%prog [-s|-r|-a] [-c] [-v|-d <debug_level>|-q] [-i <include_path_file>] [-o <output_file>] <input_file>"

    Parser = OptionParser(description=__copyright__, version=__version__, option_list=OptionList, usage=UsageString)
    Parser.set_defaults(FileType="Vfr")
    Parser.set_defaults(ConvertHex=False)
    Parser.set_defaults(LogLevel=EdkLogger.INFO)

    Options, Args = Parser.parse_args()

    # error check
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
    except FatalError, X:
        return 1
    
    try:
        if CommandOptions.FileType == "Vfr":
            if CommandOptions.OutputFile == None:
                CommandOptions.OutputFile = os.path.splitext(InputFile)[0] + '.iii'
            TrimPreprocessedVfr(InputFile, CommandOptions.OutputFile)
        elif CommandOptions.FileType == "Asl":
            if CommandOptions.OutputFile == None:
                CommandOptions.OutputFile = os.path.splitext(InputFile)[0] + '.iii'
            TrimAslFile(InputFile, CommandOptions.OutputFile, CommandOptions.IncludePathFile)
        elif CommandOptions.FileType == "EdkSourceCode":
            TrimEdkSources(InputFile, CommandOptions.OutputFile)
        else :
            if CommandOptions.OutputFile == None:
                CommandOptions.OutputFile = os.path.splitext(InputFile)[0] + '.iii'
            TrimPreprocessedFile(InputFile, CommandOptions.OutputFile, CommandOptions.ConvertHex, CommandOptions.TrimLong)
    except FatalError, X:
        import platform
        import traceback
        if CommandOptions != None and CommandOptions.LogLevel <= EdkLogger.DEBUG_9:
            EdkLogger.quiet("(Python %s on %s) " % (platform.python_version(), sys.platform) + traceback.format_exc())
        return 1
    except:
        import traceback
        import platform
        EdkLogger.error(
                    "\nTrim",
                    CODE_ERROR,
                    "Unknown fatal error when trimming [%s]" % InputFile,
                    ExtraData="\n(Please send email to edk2-devel@lists.sourceforge.net for help, attaching following call stack trace!)\n",
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

