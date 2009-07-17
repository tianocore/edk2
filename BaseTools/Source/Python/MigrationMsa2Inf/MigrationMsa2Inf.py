#!/usr/bin/env python
#
#
# Copyright (c) 2007, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
# 
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


""" This program converts EDK II MSA files into EDK II Extended INF format files """

import os, re, sys, fnmatch, xml.dom.minidom
from optparse import OptionParser
from AutoGenExterns import *
from Common.XmlRoutines import  *             # XmlParseFile, XmlElement, XmlAttribute, XmlList, XmlElementData, XmlNode
from Common.EdkIIWorkspace import *

versionNumber = "0.9"
__version__ = "%prog Version " + versionNumber
__copyright__ = "Copyright (c) 2007, Intel Corporation  All rights reserved."

commonHeaderFilename = "CommonHeader.h"
entryPointFilename   = "EntryPoint.c"

AutoGenLibraryMapping = {
    "HiiLib":"HiiLibFramework",
    "EdkIfrSupportLib":"IfrSupportLibFramework",
    "EdkScsiLib":"ScsiLib",
    "EdkUsbLib":"UsbLib",
    "EdkFvbServiceLib":"FvbServiceLib",
    "EdkGraphicsLib":"GraphicsLib"
    }
    
def myOptionParser():
    """ Argument Parser """
    usage = "%prog [options] -f input_filename"
    parser = OptionParser(usage=usage,description=__copyright__,version="%prog " + str(versionNumber))
    parser.add_option("-f", "--file", dest="filename", help="Name of MSA file to convert")
    parser.add_option("-o", "--output", dest="outfile", help="Specific Name of the INF file to create, otherwise it is the MSA filename with the extension repalced.")
    parser.add_option("-a", "--auto", action="store_true", dest="autowrite", default=False, help="Automatically create output files and write the INF file")
    parser.add_option("-i", "--interactive", action="store_true", dest="interactive", default=False, help="Set Interactive mode, user must approve each change.")
    parser.add_option("-q", "--quiet", action="store_const", const=0, dest="verbose", help="Do not print any messages, just return either 0 for succes or 1 for failure")
    parser.add_option("-v", "--verbose", action="count", dest="verbose", help="Do not print any messages, just return either 0 for succes or 1 for failure")
    parser.add_option("-d", "--debug", action="store_true", dest="debug", default=False, help="Enable printing of debug messages.")
    parser.add_option("-c", "--convert", action="store_true", dest="convert", default=False, help="Convert package: OldMdePkg->MdePkg EdkModulePkg->MdeModulePkg.")
    parser.add_option("-e", "--event", action="store_true", dest="event", default=False, help="Enable handling of Exit Boot Services & Virtual Address Changed Event")
    parser.add_option("-m", "--manual", action="store_true", dest="manual", default=False, help="Generate CommonHeader.txt, user picks up & copy it to a module common header")
    parser.add_option("-w", "--workspace", dest="workspace", default=str(os.environ.get('WORKSPACE')), help="Specify workspace directory.")
    (options, args) = parser.parse_args(sys.argv[1:])

    return options,args


def openDatabase(f):
    """ Parse XML in the FrameworkDatabase.db file pointed to by f """
    if (options.debug and options.verbose > 1):
        print "Opening the database file:", f
    if os.path.exists(f):
        fdb = XmlParseFile(f)
    else:
        return "None"
    return fdb

def openSpd(s):
    """ Parse XML in the SPD file pointed to by s """
    if (options.debug and options.verbose > 1):
        print "Opening the SPD file:", s
    if os.path.exists(s):
        spd = XmlParseFile(s)
    else:
        return "None"
    return spd

def openMsa(m):
    """ Parse XML in the MSA file pointed to by m """
    if (options.debug and options.verbose > 1):
        print "Opening the MSA file:", m
    if os.path.exists(m):
        msa = XmlParseFile(m)
    else:
        return "None"
    return msa

def AddGuid(ArchList, CName, Usage):
    """ Add a GUID to the Architecture array that the GUID is valid for. """
    if "IA32" in ArchList:
        GuidCNameIa32.insert(0, str("  %-45s # %s" % (CName, Usage)))
    if "X64" in ArchList:
        GuidCNameX64.insert(0, str("  %-45s # %s" % (CName, Usage)))
    if "IPF" in ArchList:
        GuidCNameIPF.insert(0, str("  %-45s # %s" % (CName, Usage)))
    if "EBC" in ArchList:
        GuidCNameEBC.insert(0, str("  %-45s # %s" % (CName, Usage)))
    if "ALL" in ArchList:
        GuidCName.insert(0, str("  %-45s # %s" % (CName, Usage)))


def removeDups(CN, ListName):
    """ Remove Duplicate Entries from the Guid List passed in """
    for Entry in ListName[:]:
        if " " + CN + " " in Entry:
            if (options.verbose > 1):
                print "Removing C Name %s Entry from Guids List." % (CN)
            ListName.remove(Entry)

def chkArch(Archs):
    """ Process the supported architectures passed in to combine if possible """
    Archs = Archs.upper()
    if (("IA32" in Archs) & ("X64" in Archs) & ("IPF" in Archs) & ("EBC" in Archs)):
        Archs = "ALL"
    if (len(Archs) == 0):
        Archs = "ALL"
    return Archs

def saveSourceFile(moduleDir, sourceFilename, sourceFileContents):
    newFilename = os.path.join(moduleDir, sourceFilename)
    
    try:
        f = open(newFilename, "w+")
        f.write(sourceFileContents)
        f.close()
    except:
        print "IO error in saving %s" % sourceFilename
        
    return sourceFilename

def openSourceFile(moduleDir, sourceFilename):
    newFilename = os.path.join(moduleDir, sourceFilename)
    sourceFileContents = ""
    try:
        f = open(newFilename, "r")
        sourceFileContents = f.read()
        f.close()
    except:
        print "IO error in opening %s" % sourceFilename
        
    return sourceFileContents

def MatchOption(eline, ToolChainFamily, Targets, Archs, ToolCode, Value):
    IDs = eline.split("_")
    
    if len(IDs) < 5:
        return []
    
    MatchedTargets = []
    if (Targets[0] == "*") or IDs[0] in Targets:
        MatchedTargets.append(IDs[0])
    elif IDs[0] == "*":
        MatchedTargets = Targets

    MatchedArchs = []
    if Archs[0] == "*" or IDs[2] in Archs:
        MatchedArchs.append(IDs[2])
    elif IDs[2] == "*":
        MatchedArchs = Archs

    if IDs[3] != ToolCode and IDs[3] != "*":
        return []
    
    result = []
    for arch in MatchedArchs:
        for target in MatchedTargets:
            line = "%s:%s_%s_%s_%s_FLAGS = %s" % (ToolChainFamily, target, IDs[1], arch, ToolCode, Value)
            result.append(line)

    return result
        
def main():

    AutoGenSource = ""
    AutoGenHeader = ""
    AutoGenDeclaration = ""
    AutoGenModuleFolder = None

    workspace = ""

    if (options.workspace == None):
        print "ERROR: E0000: WORKSPACE not defined.\n  Please set the WORKSPACE environment variable to the location of the EDK II install directory."
        sys.exit(1)
    else:
        workspace = options.workspace
        if (options.debug):
            print "Using Workspace:", workspace

    try:
        options.verbose +=1
    except:
        options.verbose = 1
        pass


    FdbPath = os.path.join(workspace, "Conf")
    FdbPath = os.path.join(FdbPath, "FrameworkDatabase.db")
    if os.path.exists(FdbPath):
        FdbFile = FdbPath
    else:
        print "ERROR: E0001: WORKSPACE does not contain the FrameworkDatabase File.\n  Please run EdkSetup from the EDK II install directory.\n"
        sys.exit(1)

    Fdb = openDatabase(FdbFile)
    if (Fdb == 'None'):
        print "ERROR: E0002 Could not open the Framework Database file:", FdbFile
        sys.exit(1)

    if (options.debug):
        print "FrameworkDataBase.db file:", FdbFile

    #
    InitializeAutoGen(workspace, Fdb)

    if (options.filename):
        filename = options.filename
        if ((options.verbose > 1) | (options.autowrite)):
            print "Filename:", filename
    else:
        print "ERROR: E0001 - You must specify an input filename"
        sys.exit(1)

    if (options.outfile):
        outputFile = options.outfile
    else:
       outputFile = filename.replace('.msa', '.inf')

    if ((options.verbose > 2) or (options.debug)):
        print "Output Filename:", outputFile

    Msa = openMsa(filename)
    if (Msa == 'None'):
        ## Maybe developer think WORKSPACE macro is the root directory of file name
        ## So we will try to add WORKSPACE path into filename
        MsaFileName = ""
        MsaFileName = os.path.join(workspace, filename)
        Msa = openMsa(MsaFileName)
        if (Msa == 'None'):
            print "ERROR: E0002: Could not open the file:", filename
            sys.exit(1)

    AutoGenModuleFolder = os.path.dirname(filename)
        
    MsaHeader = "/ModuleSurfaceArea/MsaHeader/"
    MsaDefs = "/ModuleSurfaceArea/ModuleDefinitions/"
    BaseName = str(XmlElement(Msa, MsaDefs + "OutputFileBasename")).strip()

    if (len(BaseName) < 1):
      BaseName = str(XmlElement(Msa, MsaHeader + "BaseName")).strip()
      BaseName = re.sub(' ', '_', BaseName)

    GuidValue = str(XmlElement(Msa, MsaHeader + "GuidValue")).strip()
    VerString = str(XmlElement(Msa, MsaHeader + "Version")).strip()
    ModType = str(XmlElement(Msa, MsaHeader + "ModuleType")).strip()
    CopyRight = str(XmlElement(Msa, MsaHeader + "Copyright")).strip()
    Abstract = str(XmlElement(Msa, MsaHeader + "Abstract")).strip()
    Description = str(XmlElement(Msa, MsaHeader + "Description")).strip().replace("  ", " ").replace("  ", " ").replace("  ", " ")
    if not CopyRight.find("2007"):
        CopyRight = CopyRight.replace("2006", "2007")
    License = str(XmlElement(Msa, MsaHeader + "License")).strip().replace("  ", " ")
    MsaDefs = "/ModuleSurfaceArea/ModuleDefinitions/"
    BinModule = ""
    try:
        BinModule = str(XmlElement(Msa, MsaDefs + "BinaryModule")).strip()
    except:
        pass

    SupportedArchitectures = ""
    try:
        SupportedArchitectures = str(XmlElement(Msa, MsaDefs + "SupportedArchitectures")).strip()
    except:
        pass

    DefinesComments = []
    if (len(SupportedArchitectures) > 0):
        DefinesComments.insert(0, "\n#\n# The following information is for reference only and not required by the build tools.\n#\n")
        DefinesComments.append("#  VALID_ARCHITECTURES           = " + SupportedArchitectures + "\n")
        DefinesComments.append("#\n")

    MsaExtern = "/ModuleSurfaceArea/Externs/"
    PcdIsDriver = ""
    try:
        PcdIsDriver = str(XmlElement(Msa, MsaExtern + "PcdIsDriver")).strip()
    except:
        pass

    SpecList = []
    List = []
    try:
        List = XmlList(Msa, MsaExtern + "Specification")
    except:
        pass

    if (len(List) > 0):
        for spec in List[:]:
            SpecList.insert(0, str(XmlElementData(spec)).strip())

    DriverModules = []
    LibraryModules = []
    Externlist = []
    Flag = (DefinesComments == [])

    # Data structure to insert autogen code
    AutoGenDriverModel = []
    AutoGenExitBootServices = []
    AutoGenVirtualAddressChanged = []
    AutoGenEntryPoint = ""
    AutoGenUnload = ""
    AutoGenGuid = []
    AutoGenLibClass = []
    AutoGenPackage = []
    AutoGenSourceFiles = []
    OldEntryPoint = ""
    OldUnload = ""
    
    try:
        Externlist = XmlList(Msa, MsaExtern + "Extern")
    except:
        pass

    if (len(Externlist) > 0):
        if (options.debug and options.verbose > 2):
            print "In Extern Parsing Routine"
        for extern in Externlist:
            EntryPoint = ""
            Unload = ""
            DBinding = ""
            CompName = ""
            Diag = ""
            Config = ""
            Constr = ""
            Destr = ""
            CallBack = ""
            lFlag = False
            AutoGenDriverModelItem = []
            try:
                EntryPoint = str(XmlElementData(extern.getElementsByTagName("ModuleEntryPoint")[0])).strip()
                AutoGenEntryPoint = EntryPoint
                #DriverModules.append("  %-30s = %s\n" % ("ENTRY_POINT" , EntryPoint))
            except:
                pass

            try:
                Unload = str(XmlElementData(extern.getElementsByTagName("ModuleUnloadImage")[0])).strip()
                AutoGenUnload = Unload
                DriverModules.append("  %-30s = %s\n" % ("UNLOAD_IMAGE", Unload))
            except:
                pass

            try:
                DBinding = str(XmlElementData(extern.getElementsByTagName("DriverBinding")[0])).strip()
                AutoGenDriverModelItem.append("&" + DBinding)
                DefinesComments.append("#  %-29s =  %-45s\n" % ("DRIVER_BINDING", DBinding))
                lFlag = True
            except:
                pass

            try:
                CompName = str(XmlElementData(extern.getElementsByTagName("ComponentName")[0])).strip()
                AutoGenDriverModelItem.append("&" + CompName)
                DefinesComments.append("#  %-29s =  %-45s\n" % ("COMPONENT_NAME", CompName))
                lFlag = True
            except:
                if lFlag:
                    AutoGenDriverModelItem.append("NULL")
                pass

            try:
                Config = str(XmlElementData(extern.getElementsByTagName("DriverConfig")[0])).strip()
                AutoGenDriverModelItem.append("&" + Config)
                DefinesComments.append("#  %-29s =  %-45s\n" % ("DRIVER_CONFIG", Config))
                lFlag = True
            except:
                if lFlag:
                    AutoGenDriverModelItem.append("NULL")
                pass

            try:
                Diag = str(XmlElementData(extern.getElementsByTagName("DriverDiag")[0])).strip()
                AutoGenDriverModelItem.append("&" + Diag)
                DefinesComments.append("#  %-29s =  %-45s\n" % ("DRIVER_DIAG", Diag))
                lFlag = True
            except:
                if lFlag:
                    AutoGenDriverModelItem.append("NULL")
                pass

            if len(AutoGenDriverModelItem) > 0:
                AutoGenDriverModel.append(AutoGenDriverModelItem)
                
            try:
                Constr = str(XmlElementData(extern.getElementsByTagName("Constructor")[0])).strip()
                LibraryModules.append("  %-30s = %s\n" % ("CONSTRUCTOR", Constr))
            except:
                pass

            try:
                Destr = str(XmlElementData(extern.getElementsByTagName("Destructor")[0])).strip()
                LibraryModules.append("  %-30s = %s\n" % ("DESTRUCTOR", Destr))
            except:
                pass
    
            try:
                CallBack = str(XmlElement(extern, "Extern/SetVirtualAddressMapCallBack")).strip()
                if CallBack != "":
                    AutoGenVirtualAddressChanged.append(CallBack)
                    DefinesComments.append("#  %-29s =  %-45s\n" % ("VIRTUAL_ADDRESS_MAP_CALLBACK", CallBack))
                    lFlag = True
            except:

                pass

            try:
                CallBack = str(XmlElement(extern, "Extern/ExitBootServicesCallBack")).strip()
                if CallBack != "":
                    AutoGenExitBootServices.append(CallBack)
                    DefinesComments.append("#  %-29s =  %-45s\n" % ("EXIT_BOOT_SERVICES_CALLBACK", CallBack))
                    lFlag = True
            except:
                pass

       
    Flag = False

    """ Get the Module's custom build options """
    MBOlines = []
    MBO = "/ModuleSurfaceArea/ModuleBuildOptions/Options/Option"
    mboList = []
    try:
        mboList = XmlList(Msa, MBO)
    except:
        pass

    if (len(mboList) > 0):
        for Option in mboList:
            Targets = []
            Archs = []

            bt = ""
            try:
                bt = str(Option.getAttribute("BuildTargets"))
            except:
                pass

            if (len(bt) > 0):
                if (re.findall(" ", bt) > 0):
                    Targets = bt.split()
                else:
                    Targets.insert(0, bt)
            else:
                Targets.insert(0, "*")

            if (options.debug and options.verbose > 2):
                print "Targets", len(Targets), Targets

            pro = ""
            try:
                pro = Option.getAttribute("SupArchList")
                if (re.findall(" ", pro) > 0):
                    Archs = pro.split()
                elif (re.findall(",", pro) > 0):
                    Archs = pro.split(",")
            except:
                pass

            if (len(pro) == 0):
                Archs.insert(0, "*")

            if (options.debug and options.verbose > 2):
                print "Archs", len(Archs), Archs

            ToolCode = ""
            try:
                ToolCode = str(Option.getAttribute("ToolCode"))
            except:
                pass

            if (len(ToolCode) == 0):
                ToolCode="*"

            value = ""
            try:
                value = str(XmlElementData(Option))
            except:
                pass
            Tags = []
            TagName = ""
            try:
                TagName = str(Option.getAttribute("TagName"))
            except:
                pass

            if (len(TagName) > 0) :
                if (options.debug and options.verbose > 2):
                    print "TagName was defined:", TagName
                Tags.insert(0, TagName)
            else:
                if (options.debug and options.verbose > 2):
                    print "TagName was NOT defined!"
                TagName = "*"
                Tags.insert(0, "*")

            Family = ""
            try:
                Family = str(Option.getAttribute("ToolChainFamily")).strip()
            except:
                pass

            if (len(Family) > 0):
                if (options.debug):
                    print "Searching tools_def.txt for Tool Tags that belong to:", Family, "family"
                TCF = []
                tdFile = ""
                tdPath = os.path.join(workspace, "Tools")
                tdPath = os.path.join(tdPath, "Conf")
                tdPath = os.path.join(tdPath, "tools_def.txt")
                tdPath = tdPath.replace("\\", "/")
                if os.path.exists(tdPath):
                    tdFile = tdPath
                else:
                    tdPath = os.path.join(workspace, "Conf")
                    tdPath = os.path.join(tdPath, "tools_def.txt")
                    if os.path.exists(tdPath):
                        tdFile = tdPath
                    else:
                        print "ERROR: E0001: WORKSPACE does not contain the tools_def.txt File.\n  Please run EdkSetup from the EDK II install directory.\n"
                        sys.exit(1)

                if (options.debug and options.verbose > 2):
                    print "Opening:", tdFile

                TagNameList = []
                tools_def = open(tdFile, "r")
                for tdline in tools_def:
                    if "# " in tdline:
                        continue
                    if "FAMILY" in tdline:
                        if (options.debug and options.verbose > 2):
                            print "Testing for FAMILY:", Family, "in the line:", tdline.strip()
                        if Family in tdline:
                            enter = tdline.split("=")[0]
                            if (options.debug and options.verbose > 2):
                                print "Adding TNL:", tdline
                            TagNameList.insert(0, enter)
                tools_def.close()

                if (options.debug and options.verbose > 2):
                    print "TagNameList:", TagNameList

                olinesSet = {}
                for eline in TagNameList:
                    if "# " in eline:
                        continue
                    if (options.debug and options.verbose > 2):
                        print "ToolsDef entry:", eline
                        
                    olines = MatchOption(eline, Family, Targets, Archs, ToolCode, value)
                    for oline in olines:
                        olinesSet[oline] = 1
                        
                for oline in olinesSet:
                    if (options.debug and options.verbose > 2):
                        print "Adding:", str(oline)
                    MBOlines.insert(0, oline)
            else:
                for targ in Targets:
                    for arch in Archs:
                        oline = "  %s_%s_%s_%s_FLAGS = %s" % (targ, Tags[0], arch, str(ToolCode), str(Value))
                        if (options.debug and options.verbose > 2):
                            print "Adding:", str(oline)
                        MBOlines.insert(0, oline)
           



            for tag in Tags:
                for targ in Targets:
                    for arch in Archs:
                        oline = "  " + str(targ) + "_" + str(tag) + "_" + str(arch) + "_" + str(ToolCode) + "_FLAGS  = " + str(value)
                        if (options.debug and options.verbose > 2):
                            print "Adding:", str(oline)
                        #MBOlines.insert(0, oline)
  

    """ Get the Library Class information """
    MsaLcDefs = "/ModuleSurfaceArea/LibraryClassDefinitions/LibraryClass"
    LcDefList = []
    try:
        LcDefList = XmlList(Msa, MsaLcDefs)
    except:
        pass

    IamLibrary = []
    LibClassList = []
    LibClassListIa32 = []
    LibClassListX64 = []
    LibClassListIpf = []
    LibClassListEbc = []

    
    if (len(LcDefList) > 0):
        for Lc in LcDefList:
            lcKeyword = ""
            try:
                lcKeyword = str(XmlElementData(Lc.getElementsByTagName("Keyword")[0]))
            except:
                raise SyntaxError, "The MSA is not correctly formed, a Library Class Keyword Element is required"

            lcUsage = ""
            try:
                lcUsage = str(XmlAttribute(Lc, "Usage"))
            except:
                raise SyntaxError, "The MSA is not correctly formed, a Usage Attribute is required for all Library Class Elements"

            Archs = ""
            try:
                Archs = str(XmlAttribute(Lc, "SupArchList"))
            except:
                pass

            Archs = chkArch(Archs)

            if (options.debug and options.verbose > 2):
                print "Attr: ",  lcUsage, lcKeyword, Archs

            if (options.convert):
                lcKeyword = AutoGenLibraryMapping.get(lcKeyword, lcKeyword)
                
            if re.findall("PRODUCED", lcUsage, re.IGNORECASE):
                try:
                    lcSupModList = ""

                    try:
                        lcSupModList = str(XmlAttribute(Lc, "SupModuleList"))
                    except:
                        lcSupModList = ""
                    pass

                    lcLine = lcKeyword
                    AutoGenLibClass.append(lcKeyword)
                    if len(lcSupModList) > 0:
                        lcLine = lcLine + "|" + lcSupModList
                    IamLibrary.insert(0, lcLine)
                except:
                    pass
            elif lcKeyword != "UefiDriverModelLib":
                AutoGenLibClass.append(lcKeyword)
                # This section handles the library classes that are CONSUMED
                if "IA32" in Archs:
                    LibClassListIa32.insert(0, lcKeyword)
                if "X64" in Archs:
                    LibClassListX64.insert(0, lcKeyword)
                if "IPF" in Archs:
                    LibClassListIpf.insert(0, lcKeyword)
                if "EBC" in Archs:
                    LibClassListEbc.insert(0, lcKeyword)
                if "ALL" in Archs:
                    LibClassList.insert(0, lcKeyword)
        if len(AutoGenDriverModel) > 0 and "UefiLib" not in LibClassList:
            AutoGenLibClass.append("UefiLib")
            LibClassList.insert(0, "UefiLib")
    
    AutoGenDxsFiles = []
    """ Get the Source File list """
    SrcFilenames = []
    SrcFilenamesIa32 = []
    SrcFilenamesX64 = []
    SrcFilenamesIpf = []
    SrcFilenamesEbc = []
    SrcFiles = "/ModuleSurfaceArea/SourceFiles/Filename"
    SrcList = []
    try:
        SrcList = XmlList(Msa, SrcFiles)
    except:
        pass

    if (len(SrcList) > 0):
        for fn in SrcList:
            file = ""
            Archs = ""

            try:
                Archs = fn.getAttribute("SupArchList")
            except:
                pass

            Archs = chkArch(Archs)

            try:
                file = str(XmlElementData(fn))
            except:
                pass

            if file.endswith(".dxs"):
                AutoGenDxsFiles.append((file, Archs))
            else:
                AutoGenSourceFiles.append(file)
                if "IA32" in Archs:
                    SrcFilenamesIa32.insert(0, file)
                if "X64" in Archs:
                    SrcFilenamesX64.insert(0, file)
                if "IPF" in Archs:
                    SrcFilenamesIpf.insert(0, file)
                if "EBC" in Archs:
                    SrcFilenamesEbc.insert(0, file)
                if "ALL" in Archs:
                    SrcFilenames.insert(0, file)

    """ Package Dependency section """
    DbPkgList = "/FrameworkDatabase/PackageList/Filename"
    WorkspacePkgs = []
    try:
        WorkspacePkgs = XmlList(Fdb, DbPkgList)
    except:
        print "Could not tet the package data from the database"
        sys.exit(1)

    PkgDb = []
    HeaderLocations = []

    if (options.debug and options.verbose > 1):
        print "Found %s packages in the WORKSPACE" % (len(WorkspacePkgs))

    Dirs = []
    GuidDecls = []
    if (len(WorkspacePkgs) > 0):
        SpdHeader = "/PackageSurfaceArea/SpdHeader/"
        for Pkg in WorkspacePkgs[:]:
            PackageGuid = ""
            PackageVersion = ""
            file = ""
            try:
                file = str(XmlElementData(Pkg))
            except:
                pass

            if (options.debug and options.verbose > 2):
                print "PKG:", file

            if fnmatch.fnmatch(file, "*.dec"):
                print "parsing " + os.path.join(workspace, file)
                PackageGuid = ""
                PackageVersion = ""
                try:
                    Lines = open(os.path.join(workspace, file)).readlines()
                except:
                    print "Could not parse the Package file:", file
                    sys.exit(1)
                    
                for Line in Lines:
                    Line = Line.split("#")[0]
                    Items = Line.split("=")
                    if len(Items) != 2:
                        continue

                    Key = Items[0].strip().upper()
                    if Key == "PACKAGE_GUID":
                        PackageGuid = Items[1].strip()
                    if Key == "PACKAGE_VERSION":
                        PackageVersion = Items[1].strip()

            else:
                Spd = openSpd(os.path.join(workspace, file))
                if (Spd == 'None'):
                    print "Could not parse the Package file:", file
                    sys.exit(1)

                path = os.path.split(file)[0]
                file = file.replace(".nspd", ".dec")
                file = file.replace(".spd", ".dec")

                try:
                    PackageGuid = str(XmlElement(Spd, SpdHeader + "GuidValue"))
                except:
                    pass

                try:
                    PackageVersion = str(XmlElement(Spd, SpdHeader + "Version"))
                except:
                    pass

            file = file + "|" + PackageGuid + "|" + PackageVersion
            PkgDb.insert(0, file)

            GuidEntries = []
            try:
                GuidEntries = XmlList(Spd, "/PackageSurfaceArea/GuidDeclarations/Entry")
            except:
                pass

            if (len(GuidEntries) > 0):
                for Entry in GuidEntries[:]:
                    try:
                        GuidDecls.append(str(XmlElementData(Entry.getElementsByTagName("C_Name")[0])).strip())
                    except:
                        pass

       
            pHdrs = []
            try:
                pHdrs = XmlList(Spd, "/PackageSurfaceArea/PackageHeaders/IncludePkgHeader")
            except:
                pass

            if (len(pHdrs) > 0):
                for Hdr in pHdrs[:]:
                    try:
                        ModTypeList = str(Hdr.getAttribute("ModuleType"))
                        if (ModType in ModTypeList):
                            HeaderName= str(XmlElementData(Hdr))[0]
                            Dirs.insert(0, os.path.join(packagepath,str(os.path.split(HeaderName))))
                    except:
                        pass

            # Get the Guid:Header from the Packages
            SpdLcDec = "/PackageSurfaceArea/LibraryClassDeclarations/LibraryClass"
            lcList = []
            try:
                lcList = XmlList(Spd, SpdLcDec)
            except:
                pass

            if (len(lcList) > 0):
                for Lc in lcList[:]:
                    Name = ""
                    try:
                        Name = Lc.getAttribute("Name")
                    except:
                        pass

                    Header = ""
                    try:
                        Header = XmlElementData(Lc.getElementsByTagName("IncludeHeader")[0])
                    except:
                        pass

                    if ((len(Name) > 0) and (len(Header) > 0)):
                        line = Name + "|" + os.path.join(path, Header)
                        if (options.debug and options.verbose > 2):
                            print "Adding:", line
                        HeaderLocations.insert(0, line)

            ishList = []
            try:
                IndStdHeaders = "/PackageSurfaceArea/IndustryStdIncludes/IndustryStdHeader"
                ishList = XmlList(Spd, IndStdHeaders)
            except:
                pass

            if (len(ishList) > 0):
                for Lc in ishList[:]:
                    Name = ""
                    try:
                        Name = str(Lc.getAttribute("Name")).strip()
                    except:
                        pass

                    Header = ""
                    try:
                        Header = str(XmlElementData(Lc.getElementsByTagName("IncludeHeader")[0])).strip()
                    except:
                        pass

                    if ((len(Name) > 0) and (len(Header) > 0)):
                        line = Name + "|" + os.path.join(path, Header)
                        HeaderLocations.insert(0, str(line))

    PkgList = []
    PkgListIa32 = []
    PkgListX64 = []
    PkgListIpf = []
    PkgListEbc = []
    Pkgs = "/ModuleSurfaceArea/PackageDependencies/Package"
    pkgL = []
    try:
       pkgL = XmlList(Msa, Pkgs)
    except:
       pass
   

    gUnknownPkgGuid = {}
    if (len(pkgL) > 0):
        if (options.debug and options.verbose > 1):
            print "Found %s packages in the module" % (len(pkgL))
        for pkg in pkgL[:]:
            Archs = ""
            pkgGuid = ""
            pkgVer = ""
            
            FindPkgGuid = False
            try:
                Archs = pkg.getAttribute("SupArchList")
            except:
                pass

            Archs = chkArch(Archs)

            try:
                pkgGuid = pkg.getAttribute("PackageGuid")
            except:
                pass

            if options.convert:
                if pkgGuid.lower() == "5e0e9358-46b6-4ae2-8218-4ab8b9bbdcec":
                    pkgGuid = "1E73767F-8F52-4603-AEB4-F29B510B6766"
                if pkgGuid.lower() == "68169ab0-d41b-4009-9060-292c253ac43d":
                    pkgGuid = "BA0D78D6-2CAF-414b-BD4D-B6762A894288"
            AutoGenPackage.append(pkgGuid)
            try:
                pkgVer = pkg.getAttribute("PackageVersion")
            except:
                pass

            for PkgEntry in PkgDb[:]:
                if pkgGuid in PkgEntry:
                    if len(pkgVer) > 0:
                        if pkgVer in PkgEntry:
                            FindPkgGuid = True
                            if "IA32" in Archs:
                                PkgListIa32.insert(0, PkgEntry.split("|")[0])
                            if "X64" in Archs:
                                PkgListX64.insert(0, PkgEntry.split("|")[0])
                            if "IPF" in Archs:
                                PkgListIpf.insert(0, PkgEntry.split("|")[0])
                            if "EBC" in Archs:
                                PkgListEbc.insert(0, PkgEntry.split("|")[0])
                            if "ALL" in Archs:
                                PkgList.insert(0, PkgEntry.split("|")[0])
                    else:
                        FindPkgGuid = True
                        if "IA32" in Archs:
                            PkgListIa32.insert(0, PkgEntry.split("|")[0])
                        if "X64" in Archs:
                            PkgListX64.insert(0, PkgEntry.split("|")[0])
                        if "IPF" in Archs:
                            PkgListIpf.insert(0, PkgEntry.split("|")[0])
                        if "EBC" in Archs:
                            PkgListEbc.insert(0, PkgEntry.split("|")[0])
                        if "ALL" in Archs:
                            PkgList.insert(0, PkgEntry.split("|")[0])

            if not FindPkgGuid:
                gUnknownPkgGuid[str(pkgGuid)] = 1

    for UnknownPkgGuid in gUnknownPkgGuid:
        print "Cannot resolve package dependency Guid:", UnknownPkgGuid

    PkgList.reverse()
    PkgListIa32.reverse()
    PkgListX64.reverse()
    PkgListIpf.reverse()
    PkgListEbc.reverse()
    if (options.debug):
        print "Package List:", PkgList


    
    """ Setup the Global GuidCName arrays that will hold data from various MSA locations """
    global GuidCName
    global GuidCNameIa32
    global GuidCNameX64
    global GuidCNameIPF
    global GuidCNameEBC
    GuidCName = []
    GuidCNameIa32 = []
    GuidCNameX64 = []
    GuidCNameIPF = []
    GuidCNameEBC = []

    """ Check for the GUIDs Element """
    Guids = "/ModuleSurfaceArea/Guids/GuidCNames"
    GuidList = []
    try:
        GuidList = XmlList(Msa, Guids)
    except:
        pass

    if (len(GuidList) > 0):
        for Guid in GuidList:
            Archs = ""
            Usage = ""
            CName = ""

            try:
                Archs = Guid.getAttribute("SupArchList")
            except:
                pass

            Archs = chkArch(Archs)

            try:
                Usage = Guid.getAttribute("Usage")
            except:
                pass

            try:
                CName = str(XmlElementData(Guid.getElementsByTagName("GuidCName")[0]))
                if CName in GuidDecls:
                    if (options.debug and options.verbose > 1):
                        print "Guids Adding Guid CName: %-45s # %s Archs: %s" % (CName, Usage, Archs)
                    AddGuid(Archs, CName, Usage)
                    AutoGenGuid.append(CName)
                else:
                    raise AssertionError, "Guid %s defined in %s is not declared in any package (.dec) file!" % (CName, filename)
            except:
                pass

    if (options.debug and options.verbose > 2):
        print "Guid C Name List:", GuidCName

    """ Check for Events """
    Guids = "/ModuleSurfaceArea/Events/CreateEvents/EventTypes"
    GuidList = []
    try:
        GuidList = XmlList(Msa, Guids)
    except:
        pass

    if (len(GuidList) > 0):
        for Guid in GuidList:
            Archs = ""
            Usage = ""
            CName = ""

            try:
                Archs = Guid.getAttribute("SupArchList")
            except:
                pass

            Archs = chkArch(Archs)

            try:
                Usage = Guid.getAttribute("Usage")
                Type = str(XmlElementData(Guid.getElementsByTagName("EventType")[0]))
                Usage += "  Create Event: " + Type
            except:
                pass

            try:
                CName = str(Guid.getAttribute("EventGuidCName"))
                if CName in GuidDecls:
                    if (options.debug and options.verbose > 1):
                        print "CreateEvent Adding Guid CName: %-45s # %s Archs: %s" % (CName, Usage, Archs)
                    AddGuid(Archs, CName, Usage)
                    AutoGenGuid.append(CName)
                else:
                    if (len(DefinesComments) == 0):
                        DefinesComments.insert(0, "\n#\n# The following information is for reference only and not required by the build tools.\n#\n")
                    DefinesComments.append("#  Create Event Guid C Name: " + CName + " Event Type: " + Type + "\n")
                    Flag = True
            except:
                pass

    if (Flag):
        DefinesComments.append("#\n")
        Flag = False

    Guids = "/ModuleSurfaceArea/Events/SignalEvents/EventTypes"
    GuidList = []
    try:
        GuidList = XmlList(Msa, Guids)
    except:
        pass

    if (len(GuidList) > 0):
        for Guid in GuidList:
            Archs = ""
            Usage = ""
            CName = ""

            try:
                Archs = Guid.getAttribute("SupArchList")
            except:
                pass

            Archs = chkArch(Archs)

            try:
                Usage = Guid.getAttribute("Usage")
                Type = str(XmlElementData(Guid.getElementsByTagName("EventType")[0]))
                Usage += "  Signal Event: " + Type
            except:
                pass

            try:
                CName = str(Guid.getAttribute("EventGuidCName"))
                if CName in GuidDecls:
                    if (options.debug and options.verbose > 1):
                        print "SignalEvent Adding Guid CName: %-45s # %s Archs: %s" % (CName, Usage, Archs)
                    AddGuid(Archs, CName, Usage)
                    AutoGenGuid.append(CName)
                else:
                    if (len(DefinesComments) == 0):
                        DefinesComments.insert(0, "\n#\n# The following information is for reference only and not required by the build tools.\n#\n")
                    DefinesComments.append("#  Signal Event Guid C Name: " + CName + " Event Type: " + Type + "\n")
                    Flag = True
            except:
                pass

    if (Flag):
        DefinesComments.append("#\n")
        Flag = False

    """ Check the HOB guids """
    Guids = "/ModuleSurfaceArea/Hobs/HobTypes"
    GuidList = []
    try:
        GuidList = XmlList(Msa, Guids)
    except:
        pass

    if (len(GuidList) > 0):
        for Guid in GuidList:
            Archs = ""
            Usage = ""
            CName = ""

            try:
                Archs = Guid.getAttribute("SupArchList")
            except:
                pass

            Archs = chkArch(Archs)

            try:
                Usage = Guid.getAttribute("Usage")
                Type = str(XmlElementData(Guid.getElementsByTagName("HobType")[0]))
                Usage += "  Hob: " + Type
            except:
                pass

            try:
                CName = str(Guid.getAttribute("HobGuidCName"))
                if CName in GuidDecls:
                    if (options.debug and options.verbose > 1):
                        print "Hob Adding Guid CName: %-45s # %s Archs: %s" % (CName, Usage, Archs)
                    AddGuid(Archs, CName, Usage)
                    AutoGenGuid.append(CName)
                else:
                    if (len(DefinesComments) == 0):
                        DefinesComments.insert(0, "\n#\n# The following information is for reference only and not required by the build tools.\n#\n")
                    DefinesComments.append("#  HOB Guid C Name: " + CName + " Hob Type: " + Type + "\n")
                    Flag = True
            except:
                if (len(DefinesComments) == 0):
                    DefinesComments.insert(0, "\n#\n# The following information is for reference only and not required by the build tools.\n#\n")
                DefinesComments.append("#  HOB: " + Type + "\n")
                Flag = True
                pass
      
    if (Flag):
        DefinesComments.append("#\n")
        Flag = False

    """ Check for the SystemTables Element """
    Guids = "/ModuleSurfaceArea/SystemTables/SystemTableCNames"
    GuidList = []
    try:
        GuidList = XmlList(Msa, Guids)
    except:
        pass

    if (len(GuidList) > 0):
        for Guid in GuidList:
            Archs = ""
            Usage = ""
            CName = ""

            try:
                Archs = Guid.getAttribute("SupArchList")
            except:
                pass

            Archs = chkArch(Archs)

            try:
                Usage = Guid.getAttribute("Usage")
                Usage += "  System Table"
            except:
                pass

            try:
                CName = str(XmlElementData(Guid.getElementsByTagName("SystemTableCName")[0]))
                if (options.debug and options.verbose > 1):
                    print "System Table Adding Guid CName: %-45s # %s Archs: %s" % (CName, Usage, Archs)
                AddGuid(Archs, CName, Usage)
                AutoGenGuid.append(CName)
            except:
                pass

    """ Check for the DataHubs Element """
    Guids = "/ModuleSurfaceArea/DataHubs/DataHubRecord"
    GuidList = []
    try:
        GuidList = XmlList(Msa, Guids)
    except:
        pass

    if (len(GuidList) > 0):
        for Guid in GuidList:
            Archs = ""
            Usage = ""
            CName = ""

            try:
                Archs = Guid.getAttribute("SupArchList")
            except:
                pass

            Archs = chkArch(Archs)

            try:
                Usage = Guid.getAttribute("Usage")
                Usage += "  Data Hub"
            except:
                pass

            try:
                CName = str(XmlElementData(Guid.getElementsByTagName("DataHubCName")[0]))
                if (options.debug and options.verbose > 1):
                    print "Data Hub Adding Guid CName: %-45s # %s Archs: %s" % (CName, Usage, Archs)
                AddGuid(Archs, CName, Usage)
                AutoGenGuid.append(CName)
            except:
                pass

    """ Check for the HiiPackages Element """
    Guids = "/ModuleSurfaceArea/HiiPackages/HiiPackage"
    GuidList = []
    try:
        GuidList = XmlList(Msa, Guids)
    except:
        pass

    if (len(GuidList) > 0):
        for Guid in GuidList:
            Archs = ""
            Usage = ""
            CName = ""

            try:
                Archs = Guid.getAttribute("SupArchList")
            except:
                pass

            Archs = chkArch(Archs)

            try:
                Usage = Guid.getAttribute("Usage")
                Usage += "  HII Formset"
            except:
                pass

            try:
                CName = str(XmlElementData(Guid.getElementsByTagName("HiiCName")[0]))
                if (options.debug and options.verbose > 1):
                    print "Hii Formset Adding Guid CName: %-45s # %s Archs: %s" % (CName, Usage, Archs)
                AddGuid(Archs, CName, Usage)
                AutoGenGuid.append(CName)
            except:
                pass
 
    """ Check for the Variables Element """
    Guids = "/ModuleSurfaceArea/Variables/Variable"
    GuidList = []
    try:
        GuidList = XmlList(Msa, Guids)
    except:
        pass

    if (len(GuidList) > 0):
        for Guid in GuidList:
            Archs = ""
            Usage = ""
            CName = ""
            VariableName = ""

            try:
                Archs = Guid.getAttribute("SupArchList")
            except:
                pass

            Archs = chkArch(Archs)

            try:
                Usage = Guid.getAttribute("Usage")
            except:
                pass

            try:
                VariableName = str(XmlElementData(Guid.getElementsByTagName("VariableName")[0]))
                CName = str(XmlElementData(Guid.getElementsByTagName("GuidC_Name")[0]))

                HexData = VariableName.strip().split()
                UniString = " L\""
                for dig in HexData[:]:
                    UniString += str(unichr(eval(dig)))
                UniString += "\""

                Usage += UniString
      
                if CName in set(GuidDecls):
                    removeDups(CName, GuidCName)
                    removeDups(CName, GuidCNameIa32)
                    removeDups(CName, GuidCNameX64)
                    removeDups(CName, GuidCNameIPF)
                    removeDups(CName, GuidCNameEBC)

                    if (options.debug):
                        print "Variable Adding Guid CName: %-45s # %s Archs: %s" % (CName, Usage, Archs)
                    AddGuid(Archs, CName, Usage)
                    AutoGenGuid.append(CName)
                else:
                    if (len(DefinesComments) == 0):
                        DefinesComments.insert(0, "\n#\n# The following information is for reference only and not required by the build tools.\n#\n")
                    DefinesComments.append("#  Variable Guid C Name: " + CName + " Variable Name:" + UniString + "\n")
                    Flag = True
            except:
                pass

    if (Flag):
        DefinesComments.append("#\n")
        Flag = False

    """ Check for the Protocol Element """
    Protocols = "/ModuleSurfaceArea/Protocols/Protocol"
    ProtocolList = []
    ProtocolCName = []
    ProtocolCNameIa32 = []
    ProtocolCNameX64 = []
    ProtocolCNameIPF = []
    ProtocolCNameEBC = []

    try:
        ProtocolList = XmlList(Msa, Protocols)
    except:
        pass

    if (len(ProtocolList) > 0):
        for Protocol in ProtocolList:
            Archs = ""
            Usage = ""
            CName = ""

            try:
                Archs = Protocol.getAttribute("SupArchList")
            except:
                pass

            Archs = chkArch(Archs)

            try:
                Usage = Protocol.getAttribute("Usage")
            except:
                pass

            try:
                CName = str(XmlElementData(Protocol.getElementsByTagName("ProtocolCName")[0]))
                AutoGenGuid.append(CName)
                removeDups(CName, GuidCName)
                removeDups(CName, GuidCNameIa32)
                removeDups(CName, GuidCNameX64)
                removeDups(CName, GuidCNameIPF)
                removeDups(CName, GuidCNameEBC)
      
                if (options.debug and options.verbose > 1):
                    print "Found %s - %s - %s " % (CName, Usage, str(len(Archs)))

                if "IA32" in Archs:
                    ProtocolCNameIa32.insert(0, str("  %-45s # PROTOCOL %s" % (CName, Usage)))
                if "X64" in Archs:
                    ProtocolCNameX64.insert(0, str("  %-45s # PROTOCOL %s" % (CName, Usage)))
                if "IPF" in Archs:
                    ProtocolCNameIPF.insert(0, str("  %-45s # PROTOCOL %s" % (CName, Usage)))
                if "EBC" in Archs:
                    ProtocolCNameEBC.insert(0, str("  %-45s # PROTOCOL %s" % (CName, Usage)))
                if "ALL" in Archs:
                    ProtocolCName.insert(0, str("  %-45s # PROTOCOL %s" % (CName, Usage)))
            except:
                pass


    Protocols = "/ModuleSurfaceArea/Protocols/ProtocolNotify"
    try:
        ProtocolList = XmlList(Msa, Protocols)
    except:
        pass

    if (len(ProtocolList) > 0):
        for Protocol in ProtocolList:
            Archs = ""
            Usage = ""
            CName = ""

            try:
                Archs = Protocol.getAttribute("SupArchList")
            except:
                pass

            Archs = chkArch(Archs)

            try:
                Usage = Protocol.getAttribute("Usage")
            except:
                pass

            try:
                CName = str(XmlElementData(Protocol.getElementsByTagName("ProtocolNotifyCName")[0]))
                AutoGenGuid.append(CName)
                removeDups(CName, GuidCName)
                removeDups(CName, GuidCNameIa32)
                removeDups(CName, GuidCNameX64)
                removeDups(CName, GuidCNameIPF)
                removeDups(CName, GuidCNameEBC)
      
                if "IA32" in Archs:
                    ProtocolCNameIa32.insert(0, "  %-45s # PROTOCOL_NOTIFY %s" % (CName, Usage))
                if "X64" in Archs:
                    ProtocolCNameX64.insert(0, "  %-45s # PROTOCOL_NOTIFY %s" % (CName, Usage))
                if "IPF" in Archs:
                    ProtocolCNameIPF.insert(0, "  %-45s # PROTOCOL_NOTIFY %s" % (CName, Usage))
                if "EBC" in Archs:
                    ProtocolCNameEBC.insert(0, "  %-45s # PROTOCOL_NOTIFY %s" % (CName, Usage))
                if "ALL" in Archs:
                    ProtocolCName.insert(0, "  %-45s # PROTOCOL_NOTIFY %s" % (CName, Usage))
            except:
                pass

    """ Check for the PPIs Element """
    PPIs = "/ModuleSurfaceArea/PPIs/Ppi"
    PPIsList = []
    PpiCName = []
    PpiCNameIa32 = []
    PpiCNameX64 = []
    PpiCNameIPF = []
    PpiCNameEBC = []

    try:
        PPIsList = XmlList(Msa, PPIs)
    except:
        pass

    if (len(PPIsList) > 0):
        for Ppi in PPIsList:
            Archs = ""
            Usage = ""
            CName = ""

            try:
                Archs = str(Ppi.getAttribute("SupArchList"))
            except:
                pass

            Archs = chkArch(Archs)

            try:
                Usage = str(Ppi.getAttribute("Usage"))
            except:
                pass

            try:
                CName = str(XmlElementData(Ppi.getElementsByTagName("PpiCName")[0])).strip()
                AutoGenGuid.append(CName)
                removeDups(CName, GuidCName)
                removeDups(CName, GuidCNameIa32)
                removeDups(CName, GuidCNameX64)
                removeDups(CName, GuidCNameIPF)
                removeDups(CName, GuidCNameEBC)
      
                if "IA32" in Archs:
                    PpiCNameIa32.insert(0, "  %-45s # PPI %s" % (CName, Usage))
                if "X64" in Archs:
                    PpiCNameX64.insert(0, "  %-45s # PPI %s" % (CName, Usage))
                if "IPF" in Archs:
                    PpiCNameIPF.insert(0, "  %-45s # PPI %s" % (CName, Usage))
                if "EBC" in Archs:
                    PpiCNameEBC.insert(0, "  %-45s # PPI %s" % (CName, Usage))
                if "ALL" in Archs:
                    PpiCName.insert(0, "  %-45s # PPI %s" % (CName, Usage))
            except:
                pass


    PPIs = "/ModuleSurfaceArea/PPIs/PpiNotify"
    try:
        PPIsList = XmlList(Msa, PPIs)
    except:
        pass 

    if (len(PPIsList) > 0):
        for Ppi in PPIsList:
            Archs = ""
            Usage = ""
            CName = ""

            try:
                Archs = Ppi.getAttribute("SupArchList")
            except:
                pass

            Archs = chkArch(Archs)

            try:
                Usage = Ppi.getAttribute("Usage")
            except:
                pass

            try:
                CName = str(XmlElementData(Ppi.getElementsByTagName("PpiNotifyCName")[0]))
                AutoGenGuid.append(CName)
                removeDups(CName, GuidCName)
                removeDups(CName, GuidCNameIa32)
                removeDups(CName, GuidCNameX64)
                removeDups(CName, GuidCNameIPF)
                removeDups(CName, GuidCNameEBC)
      
                if "IA32" in Archs:
                    PpiCNameIa32.insert(0, "  %-45s # PPI_NOTIFY %s" % (CName, Usage))
                if "X64" in Archs:
                    PpiCNameX64.insert(0,  "  %-45s # PPI_NOTIFY %s" % (CName, Usage))
                if "IPF" in Archs:
                    PpiCNameIPF.insert(0,  "  %-45s # PPI_NOTIFY %s" % (CName, Usage))
                if "EBC" in Archs:
                    PpiCNameEBC.insert(0,  "  %-45s # PPI_NOTIFY %s" % (CName, Usage))
                if "ALL" in Archs:
                    PpiCName.insert(0,     "  %-45s # PPI_NOTIFY %s" % (CName, Usage))
            except:
                pass


    """ Get the PCD entries now """
    PcdCoded = "/ModuleSurfaceArea/PcdCoded/PcdEntry"
    PcdList = []
    try:
        PcdList = XmlList(Msa, PcdCoded)
    except:
        pass

    (PcdFF, PcdFFIa32, PcdFFX64, PcdFFIpf, PcdFFEbc) = ([],[],[],[],[])
    (PcdFAB, PcdFABIa32, PcdFABX64, PcdFABIpf, PcdFABEbc) = ([],[],[],[],[])
    (PcdPIM, PcdPIMIa32, PcdPIMX64, PcdPIMIpf, PcdPIMEbc) = ([],[],[],[],[])
    (PcdDY, PcdDYIa32, PcdDYX64, PcdDYIpf, PcdDYEbc) = ([],[],[],[],[])
    (PcdDYE, PcdDYEIa32, PcdDYEX64, PcdDYEIpf, PcdDYEEbc) = ([],[],[],[],[])

    if (len(PcdList) > 0):
        for Pcd in PcdList:
            Archs = ""
            Usage = ""
            CName = ""
            DefVal = ""

            try:
                Archs = Pcd.getAttribute("SupArchList")
            except:
                pass

            Archs = chkArch(Archs)

            try:
                ItemType = Pcd.getAttribute("PcdItemType")
            except:
                pass

            try:
                CName = str(XmlElementData(Pcd.getElementsByTagName("C_Name")[0]))
            except:
                raise SyntaxError, "ERROR: MSA has a PCD with no Pcd C_Name defined"

            try:
                TSGC = str(XmlElementData(Pcd.getElementsByTagName("TokenSpaceGuidCName")[0]))
            except:
                pass

            try:
                DefVal = str(XmlElementData(Pcd.getElementsByTagName("DefaultValue")))
            except:
                pass

            if (len(DefVal) > 0):
                line = TSGC + "." + CName + "|" + DefVal
            else:
                line = TSGC + "." + CName

            if (ItemType == "FEATURE_FLAG"):
                if ("IA32" in Archs):
                    PcdFFIa32.insert(0, line)
                if ("IPF" in Archs):
                    PcdFFIpf.insert(0, line)
                if ("X64" in Archs):
                    PcdFFX64.insert(0, line)
                if ("EBC" in Archs):
                    PcdFFEbc.insert(0, line)
                if ("ALL" in Archs):
                    PcdFF.insert(0, line)
            elif (ItemType == "FIXED_AT_BUILD"):
                if ("IA32" in Archs):
                    PcdFABIa32.insert(0, line)
                if ("IPF" in Archs):
                    PcdFABIpf.insert(0, line)
                if ("X64" in Archs):
                    PcdFABX64.insert(0, line)
                if ("EBC" in Archs):
                    PcdFABEbc.insert(0, line)
                if ("ALL" in Archs):
                    PcdFAB.insert(0, line)
            elif (ItemType == "PATCHABLE_IN_MODULE"):
                if ("IA32" in Archs):
                    PcdPIMIa32.insert(0, line)
                if ("IPF" in Archs):
                    PcdPIMIpf.insert(0, line)
                if ("X64" in Archs):
                    PcdPIMX64.insert(0, line)
                if ("EBC" in Archs):
                    PcdPIMEbc.insert(0, line)
                if ("ALL" in Archs):
                    PcdFAB.insert(0, line)
            elif (ItemType == "DYNAMIC_EX"):
                if ("IA32" in Archs):
                    PcdDYEIa32.insert(0, line)
                if ("IPF" in Archs):
                    PcdDYEIpf.insert(0, line)
                if ("X64" in Archs):
                    PcdDYEX64.insert(0, line)
                if ("EBC" in Archs):
                    PcdDYEEbc.insert(0, line)
                if ("ALL" in Archs):
                    PcdDYE.insert(0, line)
            else:
                if ("IA32" in Archs):
                    PcdDYIa32.insert(0, line)
                if ("IPF" in Archs):
                    PcdDYIpf.insert(0, line)
                if ("X64" in Archs):
                    PcdDYX64.insert(0, line)
                if ("EBC" in Archs):
                    PcdDYEbc.insert(0, line)
                if ("ALL" in Archs):
                    PcdDY.insert(0, line)

    """ User Extensions Section """
    UEList = []
    UESectionList = []
    try:
        UESectionList = XmlList(Msa, "/ModuleSurfaceArea/UserExtensions")
    except:
        pass

    if (len(UESectionList) > 0):
        for UE in UESectionList[:]:
            UserId = ""
            Identifier = ""
            Value = ""

            try:
                UserId = str(UE.getAttribute("UserID"))
            except:
                raise SyntaxError, "ERROR: Malformed MSA, No UserID Specified in UserExtensions element"

            try:
                Identifier = str(UE.getAttribute("Identifier"))
            except:
                raise SyntaxError, "ERROR: Malformed MSA, No Identifier Specified in UserExtensions element"

            if (options.debug):
                print "FOUND A UE Element", UserId, Identifier

            try:
                Value = str(XmlElementData(UE))
            except:
                pass
      
            Entry = [UserId, Identifier, Value]
            UEList.insert(0, Entry)



    if (len(Externlist) > 0):
        AutoGenSource = ""
        AutoGenDefinitionSource = ""
        AutoGenEntryPointSource = ""
        AutoGenUnloadSource = ""
        if (len(AutoGenDriverModel) > 0):
            AutoGenCode = AddDriverBindingProtocolStatement(AutoGenDriverModel)
            AutoGenEntryPointSource += AutoGenCode[0]
            AutoGenUnloadSource += AutoGenCode[1]
            AutoGenDeclaration += AutoGenCode[3]
            

        if (len(AutoGenExitBootServices) > 0):
            print "[Warning] Please manually add Create Event statement for Exit Boot Service Event!"
            if options.event:
                AutoGenCode = AddBootServiceEventStatement(AutoGenExitBootServices)
                AutoGenEntryPointSource += AutoGenCode[0]
                AutoGenUnloadSource += AutoGenCode[1]
                AutoGenDefinitionSource += AutoGenCode[2]
                AutoGenDeclaration += AutoGenCode[3]

        if (len(AutoGenVirtualAddressChanged) > 0):
            print "[Warning] Please manually add Create Event statement for Virtual Address Change Event!"
            if options.event:
                AutoGenCode = AddVirtualAddressEventStatement(AutoGenVirtualAddressChanged)
                AutoGenEntryPointSource += AutoGenCode[0]
                AutoGenUnloadSource += AutoGenCode[1]
                AutoGenDefinitionSource += AutoGenCode[2]
                AutoGenDeclaration += AutoGenCode[3]
            
        if AutoGenEntryPointSource != "":
            OldEntryPoint = AutoGenEntryPoint
            AutoGenCode   = AddNewEntryPointContentsStatement(BaseName, AutoGenEntryPoint, AutoGenEntryPointSource)
            AutoGenEntryPoint = AutoGenCode[0]
            AutoGenEntryPointSource = AutoGenCode[1]
            AutoGenDeclaration += AutoGenCode[2]
                
            
        if AutoGenEntryPoint != "":    
            DriverModules.insert(0, "  %-30s = %s\n" % ("ENTRY_POINT" , AutoGenEntryPoint))
         
        AutoGenSource = AutoGenDefinitionSource + AutoGenEntryPointSource + AutoGenUnloadSource     
        
        if (lFlag):
            DefinesComments.append("#\n")

        if (Flag and len(DefinesComments) > 0):
            DefinesComments.insert(0, "\n#\n# The following information is for reference only and not required by the build tools.\n#\n")

        if (options.debug and options.verbose > 2):
            if (len(DriverModules) > 0):
                print DriverModules
            if (len(LibraryModules) > 0):
                print LibraryModules
            if (len(DefinesComments) > 0):
                print DefinesComments

    Depex = []
    DepexIa32 = []
    DepexX64 = []
    DepexIpf = []
    DepexEbc = []
    
    for DxsFile, Archs in AutoGenDxsFiles:
        fileContents = openSourceFile(AutoGenModuleFolder, DxsFile)
        Contents, Unresolved = TranslateDpxSection(fileContents)
        if Contents == "":
            print "[warning] Cannot read dxs expression"
        else:
            if (len(Unresolved) > 0):
                print "[warning] Guid Macro(s): %s cannot find corresponding cNames. Please resolve it in [depex] section in extened inf" % ",".join(Unresolved)
                
            if ("IA32" in Archs):
                DepexIa32.insert(0, Contents)
            if ("IPF" in Archs):
                DepexIpf.insert(0, Contents)
            if ("X64" in Archs):
                DepexX64.insert(0, Contents)
            if ("EBC" in Archs):
                DepexEbc.insert(0, Contents)
            if ("ALL" in Archs):
                Depex.insert(0, Contents)

    AutoGenSourceHeaderFormat = "/**@file\n  %s\n\n  %s\n  %s\n  %s\n**/\n\n%s"
    includeCommonHeaderFileStatement = "#include \"%s\"" % commonHeaderFilename

    AutoGenHeader += AddSystemIncludeStatement(ModType, AutoGenPackage)
    AutoGenHeader += AddGuidStatement(AutoGenGuid)
    AutoGenHeader += AddLibraryClassStatement(AutoGenLibClass)

    if options.manual:
        saveSourceFile(AutoGenModuleFolder, "CommonHeader.txt", AutoGenHeader)
    else:
 
        commonHeaderFilename2 = re.sub("(?=[^a-z])", "_", commonHeaderFilename)
        commonHeaderFilename2 = "_" + commonHeaderFilename2.replace(".", "").upper() + "_"
        briefDiscription = "Common header file shared by all source files."
        detailedDiscription = "This file includes package header files, library classes and protocol, PPI & GUID definitions.\n"
        AutoGenHeader += AutoGenDeclaration
        AutoGenHeader = "#ifndef %s\n#define %s\n\n\n%s\n#endif\n" % (commonHeaderFilename2, commonHeaderFilename2, AutoGenHeader)
        AutoGenHeader = AutoGenSourceHeaderFormat % (briefDiscription, detailedDiscription, CopyRight, License, AutoGenHeader)
        saveSourceFile(AutoGenModuleFolder, commonHeaderFilename, AutoGenHeader)
        SrcFilenames.append(commonHeaderFilename)

        for source in AutoGenSourceFiles:
            extension = os.path.splitext(source)[1]
            if extension == ".c":
                sourceContents = openSourceFile(AutoGenModuleFolder, source)
                sourceContents = AddCommonInclusionStatement(sourceContents, includeCommonHeaderFileStatement)
                saveSourceFile(AutoGenModuleFolder, source, sourceContents)
    

    if AutoGenSource != "":
        briefDiscription = "Entry Point Source file."
        detailedDiscription = "This file contains the user entry point \n"
        AutoGenSource = AutoGenSourceHeaderFormat % (briefDiscription, detailedDiscription, CopyRight, License, AutoGenSource)
        AutoGenSource = AddCommonInclusionStatement(AutoGenSource, includeCommonHeaderFileStatement)

        saveSourceFile(AutoGenModuleFolder, entryPointFilename, AutoGenSource)
        SrcFilenames.append(entryPointFilename)
    
    

      
    # DONE Getting data, now output it in INF format.
    Msa.unlink()
    Fdb.unlink()
    Output = []

    """ Print the converted data format """
    head =  "#/** @file\n"
    head += "# " + str(Abstract) + "\n#\n"
    head += "# " + str(Description).strip().replace("\n", "\n# ") + "\n"
    head += "# " + str(CopyRight) + "\n#\n"
    head += "#  " + str(License).replace("\n", "\n# ").replace("  ", " ").strip() + "\n#\n"
    head += "#\n#**/\n"
  
    Output.append(head)
    if (options.debug):
        print head

##    Defines = "\n" + "#"*80+ "\n#\n"
##    if (BinModule != "false"):
##        Defines += "# Defines Section - statements that will be processed to generate a binary image.\n"
##    else:
##        Defines += "# Defines Section - statements that will be processed to create a Makefile.\n"
##    Defines += "#\n" + "#"*80 + "\n"

    Defines  = "\n"
    Defines += "[Defines]\n"
    Defines += "  %-30s = %s\n" % ("INF_VERSION", "0x00010005")
    Defines += "  %-30s = %s\n" % ("BASE_NAME", BaseName)
    Defines += "  %-30s = %s\n" % ("FILE_GUID", GuidValue)
    Defines += "  %-30s = %s\n" % ("MODULE_TYPE", ModType)
    Defines += "  %-30s = %s\n" % ("VERSION_STRING", VerString)
  
    if (len(PcdIsDriver) > 0):
        Defines += "  %-30s = %s\n" %  ("PCD_DRIVER", PcdIsDriver)
  
    if (len(IamLibrary) > 0):
        lcstr = ""
        for lc in IamLibrary[:]:
            lcstr += lc + " "
            Defines += "  %-30s = %s" %  ("LIBRARY_CLASS", lcstr)
        Defines += "\n"
  
    if (len(SpecList) > 0):
        for spec in SpecList[:]:
            (specname, specval) = spec.split()
            Defines += "  %-30s = %s\n" %  (specname, specval)
        Defines += "\n"
  
    if (len(DriverModules) > 0):
        for line in DriverModules[:]:
            Defines += line

    if (len(LibraryModules) > 0):
        for line in LibraryModules[:]:
            Defines += line
     
    if (len(DefinesComments) > 0):
        for line in DefinesComments[:]:
            Defines += line
  
    Output.append(Defines)
  
    if (options.debug):
        print Defines
  
    if (BinModule != "false"):
        """ Binary Module, so sources are really binaries. """
##        Sources = "\n" + "#"*80 + "\n#\n"
##        Sources += "# Binaries Section - list of binary files that are required for the build\n# to succeed.\n"
##        Sources += "#\n" + "#"*80 + "\n\n"
        Sources = "\n"
        if ModType == "UEFI_APPLICATION":
            FileType = "UEFI_APP"
            if options.verbose > 0:
                print "WARNING: Binary Module: %s is assuming UEFI_APPLICATION file type." % (options.filename)
        else:
            FileType = "FV"
            if options.verbose > 0:
                print "WARNING: Binary Module: %s is assuming FV file type." % (options.filename)
  
        if (len(SrcFilenames) > 0):
            Sources += "[Binaries.common]\n"
            for file in SrcFilenames[:]:
                file = file.replace("\\", "/")
                Sources += "  " + FileType + "|" + file + "\n"
            Sources += "\n"
  
        if (len(SrcFilenamesIa32) > 0):
            Sources += "[Binaries.Ia32]\n"
            for file in SrcFilenamesIa32[:]:
                file = file.replace("\\", "/")
                Sources += "  " + FileType + "|" + file + "\n"
            Sources += "\n"
  
        if (len(SrcFilenamesX64) > 0):
            Sources += "[Binaries.X64]\n"
            for file in SrcFilenamesX64[:]:
                file = file.replace("\\", "/")
                Sources += "  " + FileType + "|" + file + "\n"
            Sources += "\n"
  
        if (len(SrcFilenamesIpf) > 0):
            Sources += "[Binaries.IPF]\n"
            for file in SrcFilenamesIpf[:]:
                file = file.replace("\\", "/")
                Sources += "  " + FileType + "|" + file + "\n"
            Sources += "\n"
  
        if (len(SrcFilenamesEbc) > 0):
            Sources += "[Binaries.EBC]\n"
            for file in SrcFilenamesEbc[:]:
                file = file.replace("\\", "/")
                Sources += "  " + FileType + "|" + file + "\n"
            Sources += "\n"
  
        Output.append(Sources)
        if (options.debug):
            print Sources
    else: 
##        Sources = "\n" + "#"*80 + "\n#\n"
##        Sources += "# Sources Section - list of files that are required for the build to succeed.\n"
##        Sources += "#\n" + "#"*80 + "\n\n"
        Sources = "\n"
        if (len(SrcFilenames) > 0):
            Sources += "[Sources.common]\n"
            for file in SrcFilenames[:]:
                Sources += "  " + file + "\n"
            Sources += "\n"
  
        if (len(SrcFilenamesIa32) > 0):
            Sources += "[Sources.Ia32]\n"
            for file in SrcFilenamesIa32[:]:
                Sources += "  " + file + "\n"
            Sources += "\n"
  
        if (len(SrcFilenamesX64) > 0):
            Sources += "[Sources.X64]\n"
            for file in SrcFilenamesX64[:]:
                Sources += "  " + file + "\n"
            Sources += "\n"
  
        if (len(SrcFilenamesIpf) > 0):
            Sources += "[Sources.IPF]\n"
            for file in SrcFilenamesIpf[:]:
                Sources += "  " + file + "\n"
            Sources += "\n"
  
        if (len(SrcFilenamesEbc) > 0):
            Sources += "[Sources.EBC]\n"
            for file in SrcFilenamesEbc[:]:
                Sources += "  " + file + "\n"
            Sources += "\n"
  
        Output.append(Sources)
        if (options.debug):
            print Sources
  

    includeLine = ""
    if ((len(HeaderLocations) > 0) or (len(Dirs) > 0)):
        allLcs = set(LibClassList + LibClassListIa32 + LibClassListX64 + LibClassListIpf + LibClassListEbc + Dirs)
        Lines = []
        for line in HeaderLocations[:]:
            for Lc in allLcs:
                (keyword, header) = line.split("|")
                if Lc in keyword:
                    if (options.debug):
                        print "FOUND", Lc, "in", keyword, "header", header
                    path = "$(WORKSPACE)/" + os.path.split(header)[0]
                    Lines.insert(0, path.strip())
        Includes = ""
##        Includes = "\n" + "#"*80 + "\n#\n"
##        Includes += "# Includes Section - list of Include locations that are required for\n"
##        Includes += "#                    this module.\n"
##        Includes += "#\n" + "#"*80 + "\n\n"
##        Includes += "[Includes]\n"
##        includeLines = []
##        includeLines = set(Lines)
##        if (options.debug):
##            print "There are", len(includeLines), "entries"
##        for Line in includeLines:
##            Includes += "  " + str(Line).strip().replace("\\", "/") + "\n"
  
        Output.append(Includes)
        if (options.debug):
            print Includes
  
         
  
    if ((len(PkgList) + len(PkgListIa32) + len(PkgListX64) + len(PkgListIpf) + len(PkgListEbc)) > 0):
        """ We do this if and only if we have Package Dependencies """
##        PackageDepends = "\n" + "#"*80 + "\n#\n"
##        PackageDepends += "# Package Dependency Section - list of Package files that are required for\n"
##        PackageDepends += "#                              this module.\n"
##        PackageDepends += "#\n" + "#"*80 + "\n\n"
        PackageDepends = "\n"
        if (len(PkgList) > 0):
            PackageDepends += "[Packages]\n"
            for lc in PkgList[:]:
                lc = lc.replace("\\", "/")
                PackageDepends += "  " + lc + "\n"
            PackageDepends += "\n"
  
        if (len(PkgListIa32) > 0):
            PackageDepends += "[Packages.IA32]\n"
            for lc in PkgListIa32[:]:
                lc = lc.replace("\\", "/")
                PackageDepends += "  " + lc + "\n"
            PackageDepends += "\n"
  
        if (len(PkgListX64) > 0):
            PackageDepends += "[Packages.X64]\n"
            for lc in PkgListX64[:]:
                lc = lc.replace("\\", "/")
                PackageDepends += "  " + lc + "\n"
            PackageDepends += "\n"
  
        if (len(PkgListIpf) > 0):
            PackageDepends += "[Packages.IPF]\n"
            for lc in PkgListIpf[:]:
                lc = lc.replace("\\", "/")
                PackageDepends += "  " + lc + "\n"
            PackageDepends += "\n"
  
        if (len(PkgListEbc) > 0):
            PackageDepends += "[Packages.EBC]\n"
            for lc in PkgListEbc[:]:
                lc = lc.replace("\\", "/")
                PackageDepends += "  " + lc + "\n"
            PackageDepends += "\n"
  
        Output.append(PackageDepends)
        if (options.debug):
            print PackageDepends
  
    if ((len(LibClassList) + len(LibClassListIa32) + len(LibClassListX64) + len(LibClassListIpf) + len(LibClassListEbc)) > 0):
##        LibraryClasses = "\n" + "#"*80 + "\n#\n"
##        LibraryClasses += "# Library Class Section - list of Library Classes that are required for\n"
##        LibraryClasses += "#                         this module.\n"
##        LibraryClasses += "#\n" + "#"*80 + "\n\n"
  
        LibraryClasses = "\n"
        if (len(LibClassList) > 0):
            LibraryClasses += "[LibraryClasses]\n"
            for lc in LibClassList[:]:
              LibraryClasses += "  " + lc + "\n"
            LibraryClasses += "\n"
  
        if (len(LibClassListIa32) > 0):
            LibraryClasses += "[LibraryClasses.IA32]\n"
            for lc in LibClassListIa32[:]:
                LibraryClasses += "  " + lc + "\n"
            LibraryClasses += "\n"
  
        if (len(LibClassListX64) > 0):
            LibraryClasses += "[LibraryClasses.X64]\n"
            for lc in LibClassListX64[:]:
                LibraryClasses += "  " + lc + "\n"
            LibraryClasses += "\n"
  
        if (len(LibClassListIpf) > 0):
            LibraryClasses += "[LibraryClasses.IPF]\n"
            for lc in LibClassListIpf[:]:
                LibraryClasses += "  " + lc + "\n"
            LibraryClasses += "\n"
  
        if (len(LibClassListEbc) > 0):
            LibraryClasses += "[LibraryClasses.EBC]\n"
            for lc in LibClassListEbc[:]:
                LibraryClasses += "  " + lc + "\n"
            LibraryClasses += "\n"
  
        Output.append(LibraryClasses)
        if (options.debug):
            print LibraryClasses
  
    # Print the Guids sections
    if (len(GuidCName) + len(GuidCNameIa32) + len(GuidCNameIPF) + len(GuidCNameX64) + len(GuidCNameEBC)) > 0:
##        GuidSection = "\n" + "#"*80 + "\n#\n"
##        GuidSection += "# Guid C Name Section - list of Guids that this module uses or produces.\n"
##        GuidSection += "#\n" + "#"*80 + "\n\n"
        GuidSection = "\n"
        if (len(GuidCName) > 0):
            GuidSection += "[Guids]\n"
            for Guid in GuidCName[:]:
                GuidSection += Guid + "\n"
            GuidSection += "\n"
  
        if (len(GuidCNameIa32) > 0):
            GuidSection += "[Guids.IA32]\n"
            for Guid in GuidCNameIa32[:]:
                GuidSection += Guid + "\n"
            GuidSection += "\n"
  
        if (len(GuidCNameX64) > 0):
            GuidSection += "[Guids.X64]\n"
            for Guid in GuidCNameX64[:]:
                GuidSection += Guid + "\n"
            GuidSection += "\n"
  
        if (len(GuidCNameIPF) > 0):
            GuidSection += "[Guids.IPF]\n"
            for Guid in GuidCNameIPF[:]:
                GuidSection += Guid + "\n"
            GuidSection += "\n"
  
        if (len(GuidCNameEBC) > 0):
            GuidSection += "[Guids.EBC]\n"
            for Guid in GuidCNameEBC[:]:
                GuidSection += Guid + "\n"
            GuidSection += "\n"
  
        Output.append(GuidSection)
        if (options.debug and options.verbose > 1):
            print GuidSection
  
    # Print the Protocol sections
    if (len(ProtocolCName) + len(ProtocolCNameIa32) + len(ProtocolCNameIPF) + len(ProtocolCNameX64) + len(ProtocolCNameEBC)) > 0:
##        ProtocolsSection = "\n" + "#"*80 + "\n#\n"
##        ProtocolsSection += "# Protocol C Name Section - list of Protocol and Protocol Notify C Names\n"
##        ProtocolsSection += "#                           that this module uses or produces.\n"
##        ProtocolsSection += "#\n" + "#"*80 + "\n\n"
  
        ProtocolsSection = "\n"
        if (len(ProtocolCName) > 0):
            ProtocolsSection += "[Protocols]\n"
            for Guid in ProtocolCName[:]:
                ProtocolsSection += Guid + "\n"
            ProtocolsSection += "\n"
  
        if (len(ProtocolCNameIa32) > 0):
            ProtocolsSection += "[Protocols.IA32]\n"
            for Guid in ProtocolCNameIa32[:]:
                ProtocolsSection += Guid + "\n"
            ProtocolsSection += "\n"
  
        if (len(ProtocolCNameX64) > 0):
            ProtocolsSection += "[Protocols.X64]\n"
            for Guid in ProtocolCNameX64[:]:
                ProtocolsSection += Guid + "\n"
            ProtocolsSection += "\n"
  
        if (len(ProtocolCNameIPF) > 0):
            ProtocolsSection += "[Protocols.IPF]\n"
            for Guid in ProtocolCNameIPF[:]:
                ProtocolsSection += Guid + "\n"
            ProtocolsSection += "\n"
  
        if (len(ProtocolCNameEBC) > 0):
            ProtocolsSection += "[Protocols.EBC]\n"
            for Guid in ProtocolCNameEBC[:]:
                ProtocolsSection += Guid + "\n"
            ProtocolsSection += "\n"
  
        Output.append(ProtocolsSection)
        if (options.debug):
          print ProtocolsSection
  
    # Print the PPI sections
    if (len(PpiCName) + len(PpiCNameIa32) + len(PpiCNameIPF) + len(PpiCNameX64) + len(PpiCNameEBC)) > 0:
##        PpiSection = "\n" + "#"*80 + "\n#\n"
##        PpiSection += "# PPI C Name Section - list of PPI and PPI Notify C Names that this module\n"
##        PpiSection += "#                      uses or produces.\n"
##        PpiSection += "#\n" + "#"*80 + "\n\n"
  
        PpiSection = "\n"
        if (len(PpiCName) > 0):
            PpiSection += "[Ppis]\n"
            for Guid in PpiCName[:]:
                PpiSection += Guid + "\n"
            PpiSection += "\n"
  
        if (len(PpiCNameIa32) > 0):
            PpiSection += "[Ppis.IA32]\n"
            for Guid in PpiCNameIa32[:]:
                PpiSection += Guid + "\n"
            PpiSection += "\n"
  
        if (len(PpiCNameX64) > 0):
            PpiSection += "[Ppis.X64]\n"
            for Guid in PpiCNameX64[:]:
                PpiSection += Guid + "\n"
            PpiSection += "\n"
  
        if (len(PpiCNameIPF) > 0):
            PpiSection += "[Ppis.IPF]\n"
            for Guid in PpiCNameIPF[:]:
                PpiSection += Guid + "\n"
            PpiSection += "\n"
  
        if (len(PpiCNameEBC) > 0):
            PpiSection += "[Ppis.EBC]\n"
            for Guid in PpiCNameEBC[:]:
                PpiSection += Guid + "\n"
            PpiSection += "\n"
  
        Output.append(PpiSection)
        if (options.debug):
            print PpiSection
  
    # Print the PCD sections
    if ((len(PcdFF)+len(PcdFFIa32)+len(PcdFFX64)+len(PcdFFIpf)+len(PcdFFEbc)) > 0):
##        FeatureFlagSection = "\n" + "#"*80 + "\n#\n"
##        FeatureFlagSection += "# Pcd FEATURE_FLAG - list of PCDs that this module is coded for.\n"
##        FeatureFlagSection += "#\n" + "#"*80 + "\n\n"
  
        FeatureFlagSection = "\n"
        if (len(PcdFF) > 0):
            FeatureFlagSection += "[FeaturePcd.common]\n"
            for Entry in PcdFF[:]:
                FeatureFlagSection += "  " + Entry + "\n"
            FeatureFlagSection += "\n"
        if (len(PcdFFIa32) > 0):
            FeatureFlagSection += "[FeaturePcd.IA32]\n"
            for Entry in PcdFFIa32[:]:
                FeatureFlagSection += "  " + Entry + "\n"
            FeatureFlagSection += "\n"
        if (len(PcdFFX64) > 0):
            FeatureFlagSection += "[FeaturePcd.X64]\n"
            for Entry in PcdFFX64[:]:
                FeatureFlagSection += "  " + Entry + "\n"
            FeatureFlagSection += "\n"
        if (len(PcdFFIpf) > 0):
            FeatureFlagSection += "[PcdsFeatureFlag.IPF]\n"
            for Entry in PcdFFIpf[:]:
                FeatureFlagSection += "  " + Entry + "\n"
            FeatureFlagSection += "\n"
        if (len(PcdFFEbc) > 0):
            FeatureFlagSection += "[FeaturePcd.EBC]\n"
            for Entry in PcdFFEbc[:]:
                FeatureFlagSection += "  " + Entry + "\n"
            FeatureFlagSection += "\n"
  
        Output.append(FeatureFlagSection)
        if (options.debug):
          print FeatureFlagSection
  
    if ((len(PcdFAB)+len(PcdFABIa32)+len(PcdFABX64)+len(PcdFABIpf)+len(PcdFABEbc)) > 0):
##        FixedAtBuildSection = "\n" + "#"*80 + "\n#\n"
##        FixedAtBuildSection += "# Pcd FIXED_AT_BUILD - list of PCDs that this module is coded for.\n"
##        FixedAtBuildSection += "#\n" + "#"*80 + "\n\n"
  
        FixedAtBuildSection = "\n"
        if (len(PcdFAB) > 0):
            FixedAtBuildSection += "[FixedPcd.common]\n"
            for Entry in PcdFAB[:]:
                FixedAtBuildSection += "  " + Entry + "\n"
            FixedAtBuildSection += "\n"
        if (len(PcdFABIa32) > 0):
            FixedAtBuildSection += "[FixedPcd.IA32]\n"
            for Entry in PcdFABIa32[:]:
                FixedAtBuildSection += "  " + Entry + "\n"
            FixedAtBuildSection += "\n"
        if (len(PcdFABX64) > 0):
            FixedAtBuildSection += "[FixedPcd.X64]\n"
            for Entry in PcdFABX64[:]:
                FixedAtBuildSection += "  " + Entry + "\n"
            FixedAtBuildSection += "\n"
        if (len(PcdFABIpf) > 0):
            FixedAtBuildSection += "[FixedPcd.IPF]\n"
            for Entry in PcdFABIpf[:]:
                FixedAtBuildSection += "  " + Entry + "\n"
            FixedAtBuildSection += "\n"
        if (len(PcdFABEbc) > 0):
            FixedAtBuildSection += "[FixedPcd.EBC]\n"
            for Entry in PcdFABEbc[:]:
                FixedAtBuildSection += "  " + Entry + "\n"
            FixedAtBuildSection += "\n"
  
        Output.append(FixedAtBuildSection)
        if (options.debug):
            print FixedAtBuildSection
  
    if ((len(PcdPIM)+len(PcdPIMIa32)+len(PcdPIMX64)+len(PcdPIMIpf)+len(PcdPIMEbc)) > 0):
##        PatchableInModuleSection = "\n" + "#"*80 + "\n#\n"
##        PatchableInModuleSection += "# Pcd PATCHABLE_IN_MODULE - list of PCDs that this module is coded for.\n"
##        PatchableInModuleSection += "#\n" + "#"*80 + "\n\n"
    
        PatchableInModuleSection = "\n"
        if (len(PcdPIM) > 0):
            PatchableInModuleSection += "[PatchPcd.common]\n"
            for Entry in PcdPIM[:]:
                PatchableInModuleSection += "  " + Entry + "\n"
            PatchableInModuleSection += "\n"
        if (len(PcdPIMIa32) > 0):
            PatchableInModuleSection += "[PatchPcd.IA32]\n"
            for Entry in PcdPIMIa32[:]:
                PatchableInModuleSection += "  " + Entry + "\n"
            PatchableInModuleSection += "\n"
        if (len(PcdPIMX64) > 0):
            PatchableInModuleSection += "[PatchPcd.X64]\n"
            for Entry in PcdPIMX64[:]:
                PatchableInModuleSection += "  " + Entry + "\n"
            PatchableInModuleSection += "\n"
        if (len(PcdPIMIpf) > 0):
            PatchableInModuleSection += "[PatchPcd.IPF]\n"
            for Entry in PcdPIMIpf[:]:
                PatchableInModuleSection += "  " + Entry + "\n"
            PatchableInModuleSection += "\n"
        if (len(PcdPIMEbc) > 0):
            PatchableInModuleSection += "[PatchPcd.EBC]\n"
            for Entry in PcdPIMEbc[:]:
                PatchableInModuleSection += "  " + Entry + "\n"
            PatchableInModuleSection += "\n"
  
        Output.append(PatchableInModuleSection)
        if (options.debug):
            print PatchableInModuleSection
  
    if ((len(PcdDYE)+len(PcdDYEIa32)+len(PcdDYEX64)+len(PcdDYEIpf)+len(PcdDYEEbc)) > 0):
##        DynamicExSection = "\n" + "#"*80 + "\n#\n"
##        DynamicExSection += "# Pcd DYNAMIC_EX - list of PCDs that this module is coded for.\n"
##        DynamicExSection += "#\n" + "#"*80 + "\n\n"
  
        DynamicExSection = "\n"
        if (len(PcdDYE) > 0):
            DynamicExSection += "[PcdEx.common]\n"
            for Entry in PcdDYE[:]:
                DynamicExSection += "  " + Entry + "\n"
            DynamicExSection += "\n"
        if (len(PcdDYEIa32) > 0):
            DynamicExSection += "[PcdEx.IA32]\n"
            for Entry in PcdDYEIa32[:]:
                DynamicExSection += "  " + Entry + "\n"
            DynamicExSection += "\n"
        if (len(PcdDYEX64) > 0):
            DynamicExSection += "[PcdEx.X64]\n"
            for Entry in PcdDYEX64[:]:
                DynamicExSection += "  " + Entry + "\n"
            DynamicExSection += "\n"
        if (len(PcdDYEIpf) > 0):
            DynamicExSection += "[PcdEx.IPF]\n"
            for Entry in PcdDYEIpf[:]:
                DynamicExSection += "  " + Entry + "\n"
            DynamicExSection += "\n"
        if (len(PcdDYEEbc) > 0):
            DynamicExSection += "[PcdEx.EBC]\n"
            for Entry in PcdDYEEbc[:]:
                DynamicExSection += "  " + Entry + "\n"
            DynamicExSection += "\n"
    
        Output.append(DynamicExSection)
        if (options.debug):
            print DynamicExSection
  
    if ((len(PcdDY)+len(PcdDYIa32)+len(PcdDYX64)+len(PcdDYIpf)+len(PcdDYEbc)) > 0):
##        DynamicSection = "\n" + "#"*80 + "\n#\n"
##        DynamicSection += "# Pcd DYNAMIC - list of PCDs that this module is coded for.\n"
##        DynamicSection += "#\n" + "#"*80 + "\n\n"
      
        DynamicSection = "\n"
        if (len(PcdDY) > 0):
            DynamicSection += "[Pcd.common]\n"
            for Entry in PcdDY[:]:
                DynamicSection += "  " + Entry + "\n"
            DynamicSection += "\n"
        if (len(PcdDYIa32) > 0):
            DynamicSection += "[Pcd.IA32]\n"
            for Entry in PcdDYIa32[:]:
                DynamicSection += "  " + Entry + "\n"
            DynamicSection += "\n"
        if (len(PcdDYX64) > 0):
            DynamicSection += "[Pcd.X64]\n"
            for Entry in PcdDYX64[:]:
                DynamicSection += "  " + Entry + "\n"
            DynamicSection += "\n"
        if (len(PcdDYIpf) > 0):
            DynamicSection += "[Pcd.IPF]\n"
            for Entry in PcdDYIpf[:]:
                DynamicSection += "  " + Entry + "\n"
            DynamicSection += "\n"
        if (len(PcdDYEbc) > 0):
            DynamicSection += "[Pcd.EBC]\n"
            for Entry in PcdDYEbc[:]:
                DynamicSection += "  " + Entry + "\n"
            DynamicSection += "\n"
    
        Output.append(DynamicSection)
        if (options.debug):
            print DynamicSection
  
    if ((len(Depex) + len(DepexIa32) + len(DepexX64) + len(DepexIpf) + len(DepexEbc)) > 0):
        """ We do this if and only if we have Package Dependencies """
##        Dpx = "\n" + "#"*80 + "\n#\n"
##        Dpx += "# Dependency Expression Section - list of Dependency expressions that are required for\n"
##        Dpx += "#                              this module.\n"
##        Dpx += "#\n" + "#"*80 + "\n\n"
        Dpx = "\n"
        if (len(Depex) > 0):
            Dpx += "[Depex]\n"
            for lc in Depex[:]:
                Dpx += "  " + lc + "\n"
            Dpx += "\n"

        if (len(DepexIa32) > 0):
            Dpx += "[Depex.IA32]\n"
            for lc in DepexIa32[:]:
                Dpx += "  " + lc + "\n"
            Dpx += "\n"

        if (len(DepexX64) > 0):
            Dpx += "[Depex.X64]\n"
            for lc in DepexX64[:]:
                Dpx += "  " + lc + "\n"
            Dpx += "\n"

        if (len(DepexIpf) > 0):
            Dpx += "[Depex.IPF]\n"
            for lc in DepexIpf[:]:
                Dpx += "  " + lc + "\n"
            Dpx += "\n"

        if (len(DepexEbc) > 0):
            Dpx += "[Depex.EBC]\n"
            for lc in DepexEbc[:]:
                Dpx += "  " + lc + "\n"
            Dpx += "\n"

        Output.append(Dpx)
        if (options.debug):
            print Dpx
            
    if (len(MBOlines) > 0):
        BuildSection = ""
##        BuildSection = "\n" + "#"*80 + "\n#\n"
##        BuildSection += "# Build Options - list of custom build options for this module.\n"
##        BuildSection += "#\n" + "#"*80 + "\n\n"
        BuildSection += "\n[BuildOptions]\n"
        for mbo in MBOlines:
            tool, targs = mbo.split("=",2)
            BuildSection += "  %-40s = %s\n" %  (tool.strip(), targs.strip())

        Output.append(BuildSection)
        if (options.debug):
            print BuildSection
  

    if (len(UEList) > 0):
        UserExtensionSection = ""
        for UE in UEList[:]:
            UserExtensionSection += "[UserExtensions." + UE[0] + '."' + UE[1] + '"]\n'
            if (len(UE[2]) > 0):
                UserExtensionSection += '"' + UE[2] + '"\n'
            else:
                UserExtensionSection += "\n"
  
        Output.append(UserExtensionSection)
        if (options.debug):
            print UserExtensionSection

    print "write file", outputFile
    if (options.autowrite):
        fo = open(outputFile, "w")
        for Section in Output[:]:
            fo.writelines(Section)
            if (options.verbose > 1):
                print Section
        fo.close()
    elif (options.outfile):
        fo = open(outputFile, "w")
        for Section in Output[:]:
            fo.writelines(Section)
        fo.close()
    else:
        for Section in Output[:]:
            print Section

  
if __name__ == '__main__':

    global options
    global args
    options,args = myOptionParser()
    
    main()
    sys.exit(0)
  
