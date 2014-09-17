## @file
# Contains several utilitities shared by migration tools.
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
import re
import EdkLogger
from optparse import OptionParser
from Common.BuildToolError import *
from XmlRoutines import *
from CommonDataClass.CommonClass import *
from Common.LongFilePathSupport import OpenLongFilePath as open

## Set all fields of CommonClass object.
#
# Set all attributes of CommonClass object from XML Dom object of XmlCommon.
#
# @param  Common     The destine CommonClass object.
# @param  XmlCommon  The source XML Dom object.
#
def SetCommon(Common, XmlCommon):
    XmlTag = "Usage"
    Common.Usage = XmlAttribute(XmlCommon, XmlTag).split()

    XmlTag = "FeatureFlag"
    Common.FeatureFlag = XmlAttribute(XmlCommon, XmlTag)
    
    XmlTag = "SupArchList"
    Common.SupArchList = XmlAttribute(XmlCommon, XmlTag).split()
    
    XmlTag = XmlNodeName(XmlCommon) + "/" + "HelpText"
    Common.HelpText = XmlElement(XmlCommon, XmlTag)


## Set some fields of CommonHeaderClass object.
#
# Set Name, Guid, FileName and FullPath fields of CommonHeaderClass object from
# XML Dom object of XmlCommonHeader, NameTag and FileName.
#
# @param  CommonHeader       The destine CommonClass object.
# @param  XmlCommonHeader    The source XML Dom object.
# @param  NameTag            The name tag in XML Dom object.
# @param  FileName           The file name of the XML file.
#
def SetIdentification(CommonHeader, XmlCommonHeader, NameTag, FileName):
    XmlParentTag = XmlNodeName(XmlCommonHeader)
    
    XmlTag = XmlParentTag + "/" + NameTag
    CommonHeader.Name = XmlElement(XmlCommonHeader, XmlTag)

    XmlTag = XmlParentTag + "/" + "GuidValue"
    CommonHeader.Guid = XmlElement(XmlCommonHeader, XmlTag)

    XmlTag = XmlParentTag + "/" + "Version"
    CommonHeader.Version = XmlElement(XmlCommonHeader, XmlTag)

    CommonHeader.FileName = os.path.basename(FileName)
    CommonHeader.FullPath = os.path.abspath(FileName)


## Regular expression to match specification and value.
mReSpecification = re.compile(r"(?P<Specification>\w+)\s+(?P<Value>\w*)")

## Add specification to specification dictionary.
#
# Abstract specification name, value pair from Specification String and add them
# to specification dictionary.
#
# @param  SpecificationDict   The destine Specification dictionary.
# @param  SpecificationString The source Specification String from which the
#                             specification name and value pair is abstracted.
#
def AddToSpecificationDict(SpecificationDict, SpecificationString):
    """Abstract specification name, value pair from Specification String"""
    for SpecificationMatch in mReSpecification.finditer(SpecificationString):
        Specification = SpecificationMatch.group("Specification")
        Value = SpecificationMatch.group("Value")
        SpecificationDict[Specification] = Value

## Set all fields of CommonHeaderClass object.
#
# Set all attributes of CommonHeaderClass object from XML Dom object of
# XmlCommonHeader, NameTag and FileName.
#
# @param  CommonHeader       The destine CommonClass object.
# @param  XmlCommonHeader    The source XML Dom object.
# @param  NameTag            The name tag in XML Dom object.
# @param  FileName           The file name of the XML file.
#
def SetCommonHeader(CommonHeader, XmlCommonHeader):
    """Set all attributes of CommonHeaderClass object from XmlCommonHeader"""
    XmlParent = XmlNodeName(XmlCommonHeader)
    
    XmlTag = XmlParent + "/" + "Abstract"
    CommonHeader.Abstract = XmlElement(XmlCommonHeader, XmlTag)

    XmlTag = XmlParent + "/" + "Description"
    CommonHeader.Description = XmlElement(XmlCommonHeader, XmlTag)

    XmlTag = XmlParent + "/" + "Copyright"
    CommonHeader.Copyright = XmlElement(XmlCommonHeader, XmlTag)

    XmlTag = XmlParent + "/" + "License"
    CommonHeader.License = XmlElement(XmlCommonHeader, XmlTag)

    XmlTag = XmlParent + "/" + "Specification"
    Specification = XmlElement(XmlCommonHeader, XmlTag)

    AddToSpecificationDict(CommonHeader.Specification, Specification)

    XmlTag = XmlParent + "/" + "ModuleType"
    CommonHeader.ModuleType = XmlElement(XmlCommonHeader, XmlTag)


## Load a new Cloned Record class object.
#
# Read an input XML ClonedRecord DOM object and return an object of Cloned Record
# contained in the DOM object.
#
# @param  XmlCloned            A child XML DOM object in a Common XML DOM.
#
# @retvel ClonedRecord         A new Cloned Record object created by XmlCloned.
#
def LoadClonedRecord(XmlCloned):
    ClonedRecord = ClonedRecordClass()

    XmlTag = "Id"
    ClonedRecord.Id = int(XmlAttribute(XmlCloned, XmlTag))

    XmlTag = "FarGuid"
    ClonedRecord.FarGuid = XmlAttribute(XmlCloned, XmlTag)

    XmlTag = "Cloned/PackageGuid"
    ClonedRecord.PackageGuid = XmlElement(XmlCloned, XmlTag)
    
    XmlTag = "Cloned/PackageVersion"
    ClonedRecord.PackageVersion = XmlElement(XmlCloned, XmlTag)
    
    XmlTag = "Cloned/ModuleGuid"
    ClonedRecord.ModuleGuid = XmlElement(XmlCloned, XmlTag)
    
    XmlTag = "Cloned/ModuleVersion"
    ClonedRecord.ModuleVersion = XmlElement(XmlCloned, XmlTag)
    
    return ClonedRecord


## Load a new Guid/Protocol/Ppi common class object.
#
# Read an input XML Guid/Protocol/Ppi DOM object and return an object of
# Guid/Protocol/Ppi contained in the DOM object.
#
# @param  XmlGuidProtocolPpiCommon A child XML DOM object in a Common XML DOM.
#
# @retvel GuidProtocolPpiCommon    A new GuidProtocolPpiCommon class object
#                                  created by XmlGuidProtocolPpiCommon.
#
def LoadGuidProtocolPpiCommon(XmlGuidProtocolPpiCommon):
    GuidProtocolPpiCommon = GuidProtocolPpiCommonClass()
    
    XmlTag = "Name"
    GuidProtocolPpiCommon.Name = XmlAttribute(XmlGuidProtocolPpiCommon, XmlTag)

    XmlParent = XmlNodeName(XmlGuidProtocolPpiCommon)
    if XmlParent == "Entry":
        XmlTag = "%s/C_Name" % XmlParent
    elif XmlParent == "GuidCNames":
        XmlTag = "%s/GuidCName" % XmlParent
    else:
        XmlTag = "%s/%sCName" % (XmlParent, XmlParent)
        
    GuidProtocolPpiCommon.CName = XmlElement(XmlGuidProtocolPpiCommon, XmlTag)
    
    XmlTag = XmlParent + "/" + "GuidValue"
    GuidProtocolPpiCommon.Guid = XmlElement(XmlGuidProtocolPpiCommon, XmlTag)
    
    if XmlParent.endswith("Notify"):
        GuidProtocolPpiCommon.Notify = True

    XmlTag = "GuidTypeList"
    GuidTypes = XmlAttribute(XmlGuidProtocolPpiCommon, XmlTag)
    GuidProtocolPpiCommon.GuidTypeList = GuidTypes.split()
    
    XmlTag = "SupModuleList"
    SupModules = XmlAttribute(XmlGuidProtocolPpiCommon, XmlTag)
    GuidProtocolPpiCommon.SupModuleList = SupModules.split()

    SetCommon(GuidProtocolPpiCommon, XmlGuidProtocolPpiCommon)

    return GuidProtocolPpiCommon


## Load a new Pcd class object.
#
# Read an input XML Pcd DOM object and return an object of Pcd
# contained in the DOM object.
#
# @param  XmlPcd               A child XML DOM object in a Common XML DOM.
#
# @retvel Pcd                  A new Pcd object created by XmlPcd.
#
def LoadPcd(XmlPcd):
    """Return a new PcdClass object equivalent to XmlPcd"""
    Pcd = PcdClass()

    XmlTag = "PcdEntry/C_Name"
    Pcd.CName = XmlElement(XmlPcd, XmlTag)

    XmlTag = "PcdEntry/Token"
    Pcd.Token = XmlElement(XmlPcd, XmlTag)

    XmlTag = "PcdEntry/TokenSpaceGuidCName"
    Pcd.TokenSpaceGuidCName = XmlElement(XmlPcd, XmlTag)

    XmlTag = "PcdEntry/DatumType"
    Pcd.DatumType = XmlElement(XmlPcd, XmlTag)

    XmlTag = "PcdEntry/MaxDatumSize"
    Pcd.MaxDatumSize = XmlElement(XmlPcd, XmlTag)

    XmlTag = "PcdEntry/DefaultValue"
    Pcd.DefaultValue = XmlElement(XmlPcd, XmlTag)

    XmlTag = "PcdItemType"
    Pcd.ItemType = XmlAttribute(XmlPcd, XmlTag)

    XmlTag = "PcdEntry/ValidUsage"
    Pcd.ValidUsage = XmlElement(XmlPcd, XmlTag).split()

    XmlTag = "SupModuleList"
    Pcd.SupModuleList = XmlAttribute(XmlPcd, XmlTag).split()

    SetCommon(Pcd, XmlPcd)

    return Pcd


## Load a new LibraryClass class object.
#
# Read an input XML LibraryClass DOM object and return an object of LibraryClass
# contained in the DOM object.
#
# @param  XmlLibraryClass    A child XML DOM object in a Common XML DOM.
#
# @retvel LibraryClass       A new LibraryClass object created by XmlLibraryClass.
#
def LoadLibraryClass(XmlLibraryClass):
    LibraryClass = LibraryClassClass()

    XmlTag = "LibraryClass/Keyword"
    LibraryClass.LibraryClass = XmlElement(XmlLibraryClass, XmlTag)
    if LibraryClass.LibraryClass == "":
        XmlTag = "Name"
        LibraryClass.LibraryClass = XmlAttribute(XmlLibraryClass, XmlTag)
    
    XmlTag = "LibraryClass/IncludeHeader"
    LibraryClass.IncludeHeader = XmlElement(XmlLibraryClass, XmlTag)
    
    XmlTag = "RecommendedInstanceVersion"
    RecommendedInstanceVersion = XmlAttribute(XmlLibraryClass, XmlTag)
    LibraryClass.RecommendedInstanceVersion = RecommendedInstanceVersion
    
    XmlTag = "RecommendedInstanceGuid"
    RecommendedInstanceGuid = XmlAttribute(XmlLibraryClass, XmlTag)
    LibraryClass.RecommendedInstanceGuid = RecommendedInstanceGuid
    
    XmlTag = "SupModuleList"
    SupModules = XmlAttribute(XmlLibraryClass, XmlTag)
    LibraryClass.SupModuleList = SupModules.split()
    
    SetCommon(LibraryClass, XmlLibraryClass)
    
    return LibraryClass


## Load a new Build Option class object.
#
# Read an input XML BuildOption DOM object and return an object of Build Option
# contained in the DOM object.
#
# @param  XmlBuildOption       A child XML DOM object in a Common XML DOM.
#
# @retvel BuildOption          A new Build Option object created by XmlBuildOption.
#
def LoadBuildOption(XmlBuildOption):
    """Return a new BuildOptionClass object equivalent to XmlBuildOption"""
    BuildOption = BuildOptionClass()
    
    BuildOption.Option = XmlElementData(XmlBuildOption)

    XmlTag = "BuildTargets"
    BuildOption.BuildTargetList = XmlAttribute(XmlBuildOption, XmlTag).split()
    
    XmlTag = "ToolChainFamily"
    BuildOption.ToolChainFamily = XmlAttribute(XmlBuildOption, XmlTag)
    
    XmlTag = "TagName"
    BuildOption.TagName = XmlAttribute(XmlBuildOption, XmlTag)
    
    XmlTag = "ToolCode"
    BuildOption.ToolCode = XmlAttribute(XmlBuildOption, XmlTag)
    
    XmlTag = "SupArchList"
    BuildOption.SupArchList = XmlAttribute(XmlBuildOption, XmlTag).split()
    
    return BuildOption


## Load a new User Extensions class object.
#
# Read an input XML UserExtensions DOM object and return an object of User
# Extensions contained in the DOM object.
#
# @param  XmlUserExtensions    A child XML DOM object in a Common XML DOM.
#
# @retvel UserExtensions       A new User Extensions object created by
#                              XmlUserExtensions.
#
def LoadUserExtensions(XmlUserExtensions):
    UserExtensions = UserExtensionsClass()
    
    XmlTag = "UserID"
    UserExtensions.UserID = XmlAttribute(XmlUserExtensions, XmlTag)
    
    XmlTag = "Identifier"
    UserExtensions.Identifier = XmlAttribute(XmlUserExtensions, XmlTag)
    
    UserExtensions.Content = XmlElementData(XmlUserExtensions)
    
    return UserExtensions


## Store content to a text file object.
#
# Write some text file content to a text file object. The contents may echo
# in screen in a verbose way.
#
# @param  TextFile           The text file object.
# @param  Content            The string object to be written to a text file.
#
def StoreTextFile(TextFile, Content):
    EdkLogger.verbose(Content)
    TextFile.write(Content)


## Add item to a section.
#
# Add an Item with specific CPU architecture to section dictionary.
# The possible duplication is ensured to be removed.
#
# @param  Section            Section dictionary indexed by CPU architecture.
# @param  Arch               CPU architecture: Ia32, X64, Ipf, ARM, AARCH64, Ebc or Common.
# @param  Item               The Item to be added to section dictionary.
#
def AddToSection(Section, Arch, Item):
    SectionArch = Section.get(Arch, [])
    if Item not in SectionArch:
        SectionArch.append(Item)
        Section[Arch] = SectionArch


## Get section contents.
#
# Return the content of section named SectionName.
# the contents is based on Methods and ObjectLists.
#
# @param  SectionName        The name of the section.
# @param  Method             A function returning a string item of an object.
# @param  ObjectList         The list of object.
#
# @retval Section            The string content of a section.
#
def GetSection(SectionName, Method, ObjectList):
    SupportedArches = ["common", "Ia32", "X64", "Ipf", "Ebc", "ARM", "AARCH64"]
    SectionDict = {}
    for Object in ObjectList:
        Item = Method(Object)
        if Item == "":
            continue
        Item = "  %s" % Item
        Arches = Object.SupArchList
        if len(Arches) == 0:
            AddToSection(SectionDict, "common", Item)
        else:
            for Arch in SupportedArches:
                if Arch.upper() in Arches:
                    AddToSection(SectionDict, Arch, Item)

    Section = ""
    for Arch in SupportedArches:
        SectionArch = "\n".join(SectionDict.get(Arch, []))
        if SectionArch != "":
            Section += "[%s.%s]\n%s\n" % (SectionName, Arch, SectionArch)
            Section += "\n"
    if Section != "":
        Section += "\n"
    return Section


## Store file header to a text file.
#
# Write standard file header to a text file. The content includes copyright,
# abstract, description and license extracted from CommonHeader class object.
#
# @param  TextFile           The text file object.
# @param  CommonHeader       The source CommonHeader class object.
#
def StoreHeader(TextFile, CommonHeader):
    CopyRight = CommonHeader.Copyright
    Abstract = CommonHeader.Abstract
    Description = CommonHeader.Description
    License = CommonHeader.License

    Header =  "#/** @file\n#\n"
    Header += "# " + Abstract + "\n#\n"
    Header += "# " + Description.strip().replace("\n", "\n# ") + "\n"
    Header += "# " + CopyRight + "\n#\n"
    Header += "#  " + License.replace("\n", "\n# ").replace("  ", " ")
    Header += "\n#\n#**/\n\n"

    StoreTextFile(TextFile, Header)

## Store file header to a text file.
#
# Write Defines section to a text file. DefinesTupleList determines the content.
#
# @param  TextFile           The text file object.
# @param  DefinesTupleList   The list of (Tag, Value) to be added as one item.
#
def StoreDefinesSection(TextFile, DefinesTupleList):
    Section = "[Defines]\n"
    for DefineItem in DefinesTupleList:
        Section += "  %-30s = %s\n" % DefineItem

    Section += "\n\n"
    StoreTextFile(TextFile, Section)


## Return one User Extension section.
#
# Read the input UserExtentsions class object and return one section.
#
# @param  UserExtensions       An input UserExtensions class object.
#
# @retval UserExtensionSection A section representing UserExtensions object.
#
def GetUserExtensions(UserExtensions):
    UserId = UserExtensions.UserID
    Identifier = UserExtensions.Identifier
    Content = UserExtensions.Content

    return "[UserExtensions.%s.%s]\n  %s\n\n" % (UserId, Identifier, Content)

## Regular expression to match an equation.
mReEquation = re.compile(r"\s*(\S+)\s*=\s*(\S*)\s*")

## Return a value tuple matching information in a text fle.
#
# Parse the text file and return a value tuple corresponding to an input tag
# tuple. In case of any error, an tuple of empty strings is returned.
#
# @param  FileName           The file name of the text file.
# @param  TagTuple           A tuple of tags as the key to the value.
#
# @param  ValueTupe          The returned tuple corresponding to the tag tuple.
#
def GetTextFileInfo(FileName, TagTuple):
    ValueTuple = [""] * len(TagTuple)
    try:
        for Line in open(FileName):
            Line = Line.split("#", 1)[0]
            MatchEquation = mReEquation.match(Line)
            if MatchEquation:
                Tag = MatchEquation.group(1).upper()
                Value = MatchEquation.group(2)
                for Index in range(len(TagTuple)):
                    if TagTuple[Index] == Tag:
                        ValueTuple[Index] = Value
    except:
        EdkLogger.info("IO Error in reading file %s" % FileName)
        
    return ValueTuple


## Return a value tuple matching information in an XML fle.
#
# Parse the XML file and return a value tuple corresponding to an input tag
# tuple. In case of any error, an tuple of empty strings is returned.
#
# @param  FileName           The file name of the XML file.
# @param  TagTuple           A tuple of tags as the key to the value.
#
# @param  ValueTupe          The returned tuple corresponding to the tag tuple.
#
def GetXmlFileInfo(FileName, TagTuple):
    XmlDom = XmlParseFile(FileName)
    return tuple([XmlElement(XmlDom, XmlTag) for XmlTag in TagTuple])


## Parse migration command line options
#
# Use standard Python module optparse to parse command line option of this tool.
#
# @param  Source             The source file type.
# @param  Destinate          The destinate file type.
#
# @retval Options            A optparse object containing the parsed options.
# @retval InputFile          Path of an source file to be migrated.
#
def MigrationOptionParser(Source, Destinate, ToolName, VersionNumber = 1.0):
    # use clearer usage to override default usage message
    UsageString = "%s [-a] [-v|-q] [-o <output_file>] <input_file>" % ToolName
    Version = "%s Version %.2f" % (ToolName, VersionNumber)
    Copyright = "Copyright (c) 2007, Intel Corporation. All rights reserved."
    
    Parser = OptionParser(description=Copyright, version=Version, usage=UsageString)
    Parser.add_option("-o", "--output", dest="OutputFile", help="The name of the %s file to be created." % Destinate)
    Parser.add_option("-a", "--auto", dest="AutoWrite", action="store_true", default=False, help="Automatically create the %s file using the name of the %s file and replacing file extension" % (Source, Destinate))
    Parser.add_option("-q", "--quiet", action="store_true", type=None, help="Disable all messages except FATAL ERRORS.")
    Parser.add_option("-v", "--verbose", action="store_true", type=None, help="Turn on verbose output with informational messages printed.")

    Options, Args = Parser.parse_args()

    # Set logging level
    if Options.verbose:
        EdkLogger.setLevel(EdkLogger.VERBOSE)
    elif Options.quiet:
        EdkLogger.setLevel(EdkLogger.QUIET)
    else:
        EdkLogger.setLevel(EdkLogger.INFO)
        
    # error check
    if len(Args) == 0:
        raise MigrationError(PARAMETER_MISSING, name="Input file", usage=Parser.get_usage())
    if len(Args) > 1:
        raise MigrationError(PARAMETER_INVALID, name="Too many input files", usage=Parser.get_usage())

    InputFile = Args[0]
    if not os.path.exists(InputFile):
        raise MigrationError(FILE_NOT_FOUND, name=InputFile)

    if Options.OutputFile:
        if Options.AutoWrite:
            raise MigrationError(OPTION_CONFLICT, arg1="-o", arg2="-a", usage=Parser.get_usage())
    else:
        if Options.AutoWrite:
            Options.OutputFile = os.path.splitext(InputFile)[0] + "." + Destinate.lower()
        else:
            raise MigrationError(OPTION_MISSING, name="-o", usage=Parser.get_usage())

    return Options, InputFile

# This acts like the main() function for the script, unless it is 'import'ed
# into another script.
if __name__ == '__main__':
    pass
