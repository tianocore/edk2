## @file
# This file is used to parse a Module file of .PKG file
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
ModuleSurfaceAreaXml
'''
from xml.dom import minidom

from Library.StringUtils import ConvertNEToNOTEQ
from Library.StringUtils import ConvertNOTEQToNE
from Library.StringUtils import GetStringOfList
from Library.StringUtils import IsMatchArch
from Library.Xml.XmlRoutines import XmlElement
from Library.Xml.XmlRoutines import XmlAttribute
from Library.Xml.XmlRoutines import XmlNode
from Library.Xml.XmlRoutines import XmlList
from Library.Xml.XmlRoutines import CreateXmlElement
from Object.POM.CommonObject import GuidVersionObject
from Object.POM.ModuleObject import BootModeObject
from Object.POM.ModuleObject import DepexObject
from Object.POM.ModuleObject import ModuleObject
from Object.POM.ModuleObject import EventObject
from Object.POM.ModuleObject import HobObject
from Object.POM.ModuleObject import SourceFileObject
from Object.POM.ModuleObject import PackageDependencyObject
from Object.POM.ModuleObject import ExternObject
from Object.POM.ModuleObject import BinaryFileObject
from Object.POM.ModuleObject import AsBuiltObject
from Object.POM.ModuleObject import BinaryBuildFlagObject
from Xml.CommonXml import ClonedFromXml
from Xml.CommonXml import HeaderXml
from Xml.CommonXml import HelpTextXml
from Xml.CommonXml import CommonDefinesXml
from Xml.CommonXml import LibraryClassXml
from Xml.CommonXml import UserExtensionsXml
from Xml.CommonXml import MiscellaneousFileXml
from Xml.CommonXml import FilenameXml
from Xml.GuidProtocolPpiXml import GuidXml
from Xml.GuidProtocolPpiXml import ProtocolXml
from Xml.GuidProtocolPpiXml import PpiXml
from Xml.PcdXml import PcdEntryXml
from Xml.XmlParserMisc import GetHelpTextList
from Library import GlobalData
from Library.Misc import GetSplitValueList

##   BinaryFileXml
#
#    represent the following XML item
#
#    <BinaryFile>
#    <Filename
#    FileType=" FileType " {1}
#    SupArchList=" ArchListType " {0,1}
#    FeatureFlag=" FeatureFlagExpression " {0,1} >
#    xs:anyURI
#    </Filename> {1,}
#    <AsBuilt> ... </AsBuilt> {0,}
#    </BinaryFile> {1,}
#
class BinaryFileXml(object):
    def __init__(self):
        self.FileNames = []
        self.AsBuiltList = []
        self.PatchPcdValues = ''
        self.PcdExValues = ''
        self.LibraryInstances = ''
        self.BuildFlags = ''

    def FromXml(self, Item, Key):
        if self.FileNames:
            pass
        BinaryFile = BinaryFileObject()
        FilenameList = []
        SupArchList = ['COMMON']
        for SubItem in XmlList(Item, '%s/Filename' % Key):
            Axml = FilenameXml()
            Bxml = Axml.FromXml(SubItem, 'Filename')
            FilenameList.append(Bxml)
        BinaryFile.SetFileNameList(FilenameList)
        for FileName in FilenameList:
            if FileName.GetSupArchList():
                SupArchList = FileName.GetSupArchList()
        BinaryFile.SetSupArchList(SupArchList)
        if GlobalData.gIS_BINARY_INF:
            AsBuiltList = []
            for AsBuiltItem in XmlList(Item, '%s/AsBuilt' % Key):
                AsBuilt = AsBuiltObject()

                PatchPcdValueList = []
                for SubItem in XmlList(AsBuiltItem, 'AsBuilt/PatchPcdValue'):
                    Axml = PcdEntryXml()
                    Bxml = Axml.FromXml(SubItem, 'PatchPcdValue')
                    PatchPcdValueList.append(Bxml)
                AsBuilt.SetPatchPcdList(PatchPcdValueList)
                PcdExValueList = []
                for SubItem in XmlList(AsBuiltItem, 'AsBuilt/PcdExValue'):
                    Axml = PcdEntryXml()
                    Bxml = Axml.FromXml(SubItem, 'PcdExValue')
                    PcdExValueList.append(Bxml)
                AsBuilt.SetPcdExList(PcdExValueList)
                LibraryList = []
                for SubItem in XmlList(Item, '%s/AsBuilt/LibraryInstances/GUID' % Key):
                    GuidVerObj = GuidVersionObject()
                    GUID = XmlElement(SubItem, 'GUID')
                    Version = XmlAttribute(XmlNode(SubItem, 'GUID'), 'Version')
                    GuidVerObj.SetGuid(GUID)
                    GuidVerObj.SetVersion(Version)
                    LibraryList.append(GuidVerObj)
                if XmlList(Item, '%s/AsBuilt/LibraryInstances' % Key) and not LibraryList:
                    LibraryList = [None]
                AsBuilt.SetLibraryInstancesList(LibraryList)
                BuildFlagList = []
                for SubItem in XmlList(Item, '%s/AsBuilt/BuildFlags' % Key):
                    BuildFlag = BuildFlagXml()
                    BuildFlagList.append(BuildFlag.FromXml2(SubItem, 'BuildFlags'))
                AsBuilt.SetBuildFlagsList(BuildFlagList)
                AsBuiltList.append(AsBuilt)
            BinaryFile.SetAsBuiltList(AsBuiltList)
        return BinaryFile

    def ToXml(self, BinaryFile, Key):
        if self.FileNames:
            pass
        NodeList = []
        FilenameList = BinaryFile.GetFileNameList()
        SupportArch = None
        for Filename in FilenameList:
            Tmp = FilenameXml()
            NodeList.append(Tmp.ToXml(Filename, 'Filename'))
            SupportArch = Filename.SupArchList

        AsBuildList = BinaryFile.GetAsBuiltList()
        PatchPcdValueList = AsBuildList.GetPatchPcdList()
        PcdExList = AsBuildList.GetPcdExList()
        LibGuidVerList = AsBuildList.GetLibraryInstancesList()
        BuildFlagList = AsBuildList.GetBuildFlagsList()

        AsBuiltNodeList = []

        for Pcd in PatchPcdValueList:
            if IsMatchArch(Pcd.SupArchList, SupportArch):
                Tmp = PcdEntryXml()
                AsBuiltNodeList.append(Tmp.ToXml4(Pcd, 'PatchPcdValue'))

        for Pcd in PcdExList:
            if IsMatchArch(Pcd.SupArchList, SupportArch):
                Tmp = PcdEntryXml()
                AsBuiltNodeList.append(Tmp.ToXml4(Pcd, 'PcdExValue'))

        GuiVerElemList = []
        for LibGuidVer in LibGuidVerList:
            if LibGuidVer.GetLibGuid() and IsMatchArch(LibGuidVer.GetSupArchList(), SupportArch):
                GuiVerElem = \
                CreateXmlElement('GUID', LibGuidVer.GetLibGuid(), [], [['Version', LibGuidVer.GetLibVersion()]])
                GuiVerElemList.append(GuiVerElem)
        if len(GuiVerElemList) > 0:
            LibGuidVerElem = CreateXmlElement('LibraryInstances', '', GuiVerElemList, [])
            AsBuiltNodeList.append(LibGuidVerElem)

        for BuildFlag in BuildFlagList:
            if IsMatchArch(BuildFlag.GetSupArchList(), SupportArch):
                for Item in BuildFlag.GetAsBuildList():
                    Tmp = BuildFlagXml()
                    Elem = CreateXmlElement('BuildFlags', ''.join(Item), [], [])
                    AsBuiltNodeList.append(Elem)

        if len(AsBuiltNodeList) > 0:
            Element = CreateXmlElement('AsBuilt', '', AsBuiltNodeList, [])
            NodeList.append(Element)

        Root = CreateXmlElement('%s' % Key, '', NodeList, [])

        return Root

    def __str__(self):
        Str = "BinaryFiles:"
        for Item in self.FileNames:
            Str = Str + '\n\t' + str(Item)
        for Item in self.PatchPcdValues:
            Str = Str + '\n\t' + str(Item)
        for Item in self.PcdExValues:
            Str = Str + '\n\t' + str(Item)
        for Item in self.LibraryInstances:
            Str = Str + '\n\t' + str(Item)
        for Item in self.BuildFlags:
            Str = Str + '\n\t' + str(Item)
        return Str

##
# PackageXml
#
class PackageXml(object):
    def __init__(self):
        self.Description = ''
        self.Guid = ''
        self.Version = ''
        self.CommonDefines = CommonDefinesXml()

    def FromXml(self, Item, Key):
        self.Description = XmlElement(Item, '%s/Description' % Key)
        self.Guid = XmlElement(Item, '%s/GUID' % Key)
        self.Version = XmlAttribute(XmlNode(Item, '%s/GUID' % Key), 'Version')
        self.CommonDefines.FromXml(XmlNode(Item, '%s' % Key), Key)

        PackageDependency = PackageDependencyObject()
        PackageDependency.SetPackage(self.Description)
        PackageDependency.SetGuid(self.Guid)
        PackageDependency.SetVersion(self.Version)
        PackageDependency.SetFeatureFlag(ConvertNOTEQToNE(self.CommonDefines.FeatureFlag))
        PackageDependency.SetSupArchList(self.CommonDefines.SupArchList)

        return PackageDependency

    def ToXml(self, PackageDependency, Key):
        if self.Guid:
            pass
        AttributeList = [['SupArchList', GetStringOfList(PackageDependency.GetSupArchList())],
                         ['FeatureFlag', ConvertNEToNOTEQ(PackageDependency.GetFeatureFlag())], ]
        Element1 = CreateXmlElement('GUID', PackageDependency.GetGuid(), [],
                                    [['Version', PackageDependency.GetVersion()]])
        NodeList = [['Description', PackageDependency.GetPackage()], Element1, ]
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)

        return Root

    def __str__(self):
        Str = "Description = %s Guid = %s Version = %s %s" \
              % (self.Description, self.Guid, self.Version, self.CommonDefines)
        return Str
##
# ExternXml
#
class ExternXml(object):
    def __init__(self):
        self.CommonDefines = CommonDefinesXml()
        self.EntryPoint = ''
        self.UnloadImage = ''
        self.Constructor = ''
        self.Destructor = ''
        self.SupModList = ''
        self.SupArchList = ''
        self.HelpText = []

    def FromXml(self, Item, Key):
        self.CommonDefines.FromXml(Item, Key)
        self.EntryPoint = XmlElement(Item, '%s/EntryPoint' % Key)
        self.UnloadImage = XmlElement(Item, '%s/UnloadImage' % Key)
        self.Constructor = XmlElement(Item, '%s/Constructor' % Key)
        self.Destructor = XmlElement(Item, '%s/Destructor' % Key)

        Extern = ExternObject()
        Extern.SetEntryPoint(self.EntryPoint)
        Extern.SetUnloadImage(self.UnloadImage)
        Extern.SetConstructor(self.Constructor)
        Extern.SetDestructor(self.Destructor)
        if self.CommonDefines.SupModList:
            Extern.SetSupModList(self.CommonDefines.SupModList)
        if self.CommonDefines.SupArchList:
            Extern.SetSupArchList(self.CommonDefines.SupArchList)
        return Extern

    def ToXml(self, Extern, Key):
        if self.HelpText:
            pass

        NodeList = []
        if Extern.GetEntryPoint():
            NodeList.append(['EntryPoint', Extern.GetEntryPoint()])
        if Extern.GetUnloadImage():
            NodeList.append(['UnloadImage', Extern.GetUnloadImage()])
        if Extern.GetConstructor():
            NodeList.append(['Constructor', Extern.GetConstructor()])
        if Extern.GetDestructor():
            NodeList.append(['Destructor', Extern.GetDestructor()])
        Root = CreateXmlElement('%s' % Key, '', NodeList, [])

        return Root

    def __str__(self):
        Str = "EntryPoint = %s UnloadImage = %s Constructor = %s Destructor = %s %s" \
              % (self.EntryPoint, self.UnloadImage, self.Constructor, self.Destructor, self.CommonDefines)
        for Item in self.HelpText:
            Str = Str + '\n\t' + str(Item)
        return Str
##
# DepexXml
#
class DepexXml(object):
    def __init__(self):
        self.CommonDefines = CommonDefinesXml()
        self.Expression = None
        self.HelpText = []

    def FromXml(self, Item, Key):
        if not Item:
            return None
        self.CommonDefines.FromXml(Item, Key)
        self.Expression = XmlElement(Item, '%s/Expression' % Key)
        for HelpTextItem in XmlList(Item, '%s/HelpText' % Key):
            HelpTextObj = HelpTextXml()
            HelpTextObj.FromXml(HelpTextItem, '%s/HelpText' % Key)
            self.HelpText.append(HelpTextObj)

        Depex = DepexObject()
        Depex.SetDepex(self.Expression)
        Depex.SetModuleType(self.CommonDefines.SupModList)
        Depex.SetSupArchList(self.CommonDefines.SupArchList)
        Depex.SetFeatureFlag(self.CommonDefines.FeatureFlag)
        Depex.SetHelpTextList(GetHelpTextList(self.HelpText))

        return Depex

    def ToXml(self, Depex, Key):
        if self.HelpText:
            pass
        AttributeList = [['SupArchList', GetStringOfList(Depex.GetSupArchList())],
                         ['SupModList', Depex.GetModuleType()]]
        NodeList = [['Expression', Depex.GetDepex()]]
        if Depex.GetHelpText():
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Depex.GetHelpText(), 'HelpText'))

        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)
        return Root

    def __str__(self):
        Str = "Expression = %s" % (self.Expression)
        for Item in self.HelpText:
            Str = Str + '\n\t' + str(Item)
        return Str

##
# BootModeXml
#
class BootModeXml(object):
    def __init__(self):
        self.SupportedBootModes = ''
        self.CommonDefines = CommonDefinesXml()
        self.HelpText = []

    def FromXml(self, Item, Key):
        self.SupportedBootModes = \
        XmlElement(Item, '%s/SupportedBootModes' % Key)
        self.CommonDefines.FromXml(Item, Key)
        for HelpTextItem in XmlList(Item, '%s/HelpText' % Key):
            HelpTextObj = HelpTextXml()
            HelpTextObj.FromXml(HelpTextItem, '%s/HelpText' % Key)
            self.HelpText.append(HelpTextObj)

        BootMode = BootModeObject()
        BootMode.SetSupportedBootModes(self.SupportedBootModes)
        BootMode.SetUsage(self.CommonDefines.Usage)
        BootMode.SetHelpTextList(GetHelpTextList(self.HelpText))

        return BootMode

    def ToXml(self, BootMode, Key):
        if self.HelpText:
            pass
        AttributeList = [['Usage', BootMode.GetUsage()], ]
        NodeList = [['SupportedBootModes', BootMode.GetSupportedBootModes()]]
        for Item in BootMode.GetHelpTextList():
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item, 'HelpText'))
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)

        return Root

    def __str__(self):
        Str = "SupportedBootModes = %s %s" % (self.SupportedBootModes, self.CommonDefines)
        for Item in self.HelpText:
            Str = Str + '\n\t' + str(Item)
        return Str
##
# EventXml
#
class EventXml(object):
    def __init__(self):
        self.EventType = ''
        self.Name = ''
        self.CommonDefines = CommonDefinesXml()
        self.HelpText = []

    def FromXml(self, Item, Key):
        self.EventType = XmlAttribute(XmlNode(Item, '%s' % Key), 'EventType')
        self.Name = XmlElement(Item, '%s' % Key)
        self.CommonDefines.FromXml(Item, Key)
        for HelpTextItem in XmlList(Item, '%s/HelpText' % Key):
            HelpTextObj = HelpTextXml()
            HelpTextObj.FromXml(HelpTextItem, '%s/HelpText' % Key)
            self.HelpText.append(HelpTextObj)

        Event = EventObject()
        Event.SetEventType(self.EventType)
        Event.SetUsage(self.CommonDefines.Usage)
        Event.SetHelpTextList(GetHelpTextList(self.HelpText))

        return Event

    def ToXml(self, Event, Key):
        if self.HelpText:
            pass
        AttributeList = [['EventType', Event.GetEventType()],
                         ['Usage', Event.GetUsage()],
                        ]
        NodeList = []
        for Item in Event.GetHelpTextList():
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item, 'HelpText'))
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)

        return Root

    def __str__(self):
        Str = "EventType = %s %s" % (self.EventType, self.CommonDefines)
        for Item in self.HelpText:
            Str = Str + '\n\t' + str(Item)
        return Str
##
# HobXml
#
class HobXml(object):
    def __init__(self):
        self.HobType = ''
        self.Name = ''
        self.CommonDefines = CommonDefinesXml()
        self.HelpText = []

    def FromXml(self, Item, Key):
        self.HobType = XmlAttribute(XmlNode(Item, '%s' % Key), 'HobType')
        self.Name = XmlElement(Item, '%s' % Key)
        self.CommonDefines.FromXml(Item, Key)
        for HelpTextItem in XmlList(Item, '%s/HelpText' % Key):
            HelpTextObj = HelpTextXml()
            HelpTextObj.FromXml(HelpTextItem, '%s/HelpText' % Key)
            self.HelpText.append(HelpTextObj)

        Hob = HobObject()
        Hob.SetHobType(self.HobType)
        Hob.SetSupArchList(self.CommonDefines.SupArchList)
        Hob.SetUsage(self.CommonDefines.Usage)
        Hob.SetHelpTextList(GetHelpTextList(self.HelpText))

        return Hob

    def ToXml(self, Hob, Key):
        if self.Name:
            pass
        AttributeList = [['HobType', Hob.GetHobType()],
                         ['Usage', Hob.GetUsage()],
                         ['SupArchList', GetStringOfList(Hob.GetSupArchList())], ]
        NodeList = []
        for Item in Hob.GetHelpTextList():
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item, 'HelpText'))
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)

        return Root

    def __str__(self):
        Str = "HobType = %s %s" % (self.HobType, self.CommonDefines)
        for Item in self.HelpText:
            Str = Str + '\n\t' + str(Item)
        return Str

##
# SourceFileXml
#
class SourceFileXml(object):
    def __init__(self):
        self.SourceFile = ''
        self.ToolChainFamily = ''
        self.FileType = ''
        self.CommonDefines = CommonDefinesXml()

    def FromXml(self, Item, Key):
        self.ToolChainFamily = XmlAttribute(Item, 'Family')
        self.SourceFile = XmlElement(Item, 'Filename')
        self.CommonDefines.FromXml(Item, Key)

        self.CommonDefines.FeatureFlag = ConvertNOTEQToNE(self.CommonDefines.FeatureFlag)

        SourceFile = SourceFileObject()
        SourceFile.SetSourceFile(self.SourceFile)
        SourceFile.SetFamily(self.ToolChainFamily)
        SourceFile.SetSupArchList(self.CommonDefines.SupArchList)
        SourceFile.SetFeatureFlag(self.CommonDefines.FeatureFlag)

        return SourceFile

    def ToXml(self, SourceFile, Key):
        if self.SourceFile:
            pass
        FeatureFlag = ConvertNEToNOTEQ(SourceFile.GetFeatureFlag())
        AttributeList = [['SupArchList', GetStringOfList(SourceFile.GetSupArchList())],
                         ['Family', SourceFile.GetFamily()],
                         ['FeatureFlag', FeatureFlag], ]
        Root = CreateXmlElement('%s' % Key, SourceFile.GetSourceFile(), [], AttributeList)
        return Root

##
# ModulePropertyXml
#
class ModulePropertyXml(object):
    def __init__(self):
        self.CommonDefines = CommonDefinesXml()
        self.ModuleType = ''
        self.Path = ''
        self.PcdIsDriver = ''
        self.UefiSpecificationVersion = ''
        self.PiSpecificationVersion = ''
        self.SpecificationList = []
        self.SpecificationVersion = ''
        self.BootModes = []
        self.Events = []
        self.HOBs = []

    def FromXml(self, Item, Key, Header=None):
        self.CommonDefines.FromXml(Item, Key)
        self.ModuleType = XmlElement(Item, '%s/ModuleType' % Key)
        self.Path = XmlElement(Item, '%s/Path' % Key)
        self.PcdIsDriver = XmlElement(Item, '%s/PcdIsDriver' % Key)
        self.UefiSpecificationVersion = XmlElement(Item, '%s/UefiSpecificationVersion' % Key)
        self.PiSpecificationVersion = XmlElement(Item, '%s/PiSpecificationVersion' % Key)
        for SubItem in XmlList(Item, '%s/Specification' % Key):
            Specification = XmlElement(SubItem, '/Specification')
            Version = XmlAttribute(XmlNode(SubItem, '/Specification'), 'Version')
            self.SpecificationList.append((Specification, Version))
        for SubItem in XmlList(Item, '%s/BootMode' % Key):
            Axml = BootModeXml()
            BootMode = Axml.FromXml(SubItem, 'BootMode')
            self.BootModes.append(BootMode)
        for SubItem in XmlList(Item, '%s/Event' % Key):
            Axml = EventXml()
            Event = Axml.FromXml(SubItem, 'Event')
            self.Events.append(Event)
        for SubItem in XmlList(Item, '%s/HOB' % Key):
            Axml = HobXml()
            Hob = Axml.FromXml(SubItem, 'HOB')
            self.HOBs.append(Hob)

        if Header is None:
            Header = ModuleObject()

        Header.SetModuleType(self.ModuleType)
        Header.SetSupArchList(self.CommonDefines.SupArchList)
        Header.SetModulePath(self.Path)

        Header.SetPcdIsDriver(self.PcdIsDriver)
        Header.SetUefiSpecificationVersion(self.UefiSpecificationVersion)
        Header.SetPiSpecificationVersion(self.PiSpecificationVersion)
        Header.SetSpecList(self.SpecificationList)

        return Header, self.BootModes, self.Events, self.HOBs


    def ToXml(self, Header, BootModes, Events, Hobs, Key):
        if self.ModuleType:
            pass
        AttributeList = [['SupArchList', GetStringOfList(Header.GetSupArchList())], ]

        NodeList = [['ModuleType', Header.GetModuleType()],
                    ['Path', Header.GetModulePath()],
                    ['PcdIsDriver', Header.GetPcdIsDriver()],
                    ['UefiSpecificationVersion', Header.GetUefiSpecificationVersion()],
                    ['PiSpecificationVersion', Header.GetPiSpecificationVersion()],
                   ]
        for Item in Header.GetSpecList():
            Spec, Version = Item
            SpecElem = CreateXmlElement('Specification', Spec, [], [['Version', Version]])
            NodeList.append(SpecElem)

        for Item in BootModes:
            Tmp = BootModeXml()
            NodeList.append(Tmp.ToXml(Item, 'BootMode'))
        for Item in Events:
            Tmp = EventXml()
            NodeList.append(Tmp.ToXml(Item, 'Event'))
        for Item in Hobs:
            Tmp = HobXml()
            NodeList.append(Tmp.ToXml(Item, 'HOB'))
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)

        return Root

    def __str__(self):
        Str = "ModuleType = %s Path = %s PcdIsDriver = %s UefiSpecificationVersion = %s PiSpecificationVersion = %s \
               Specification = %s SpecificationVersion = %s %s" % \
        (self.ModuleType, self.Path, self.PcdIsDriver, \
         self.UefiSpecificationVersion, self.PiSpecificationVersion, \
         self.SpecificationList, self.SpecificationVersion, self.CommonDefines)
        for Item in self.BootModes:
            Str = Str + '\n\t' + str(Item)
        for Item in self.Events:
            Str = Str + '\n\t' + str(Item)
        for Item in self.HOBs:
            Str = Str + '\n\t' + str(Item)
        return Str

##
# ModuleXml
#
class ModuleSurfaceAreaXml(object):
    def __init__(self, Package=''):
        self.Module = None
        #
        # indicate the package that this module resides in
        #
        self.Package = Package

    def FromXml2(self, Item, Module):
        if self.Module:
            pass
        #
        # PeiDepex
        #
        PeiDepexList = []
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/PeiDepex'):
            Tmp = DepexXml()
            Depex = Tmp.FromXml(XmlNode(SubItem, 'PeiDepex'), 'PeiDepex')
            PeiDepexList.append(Depex)
        Module.SetPeiDepex(PeiDepexList)

        #
        # DxeDepex
        #
        DxeDepexList = []
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/DxeDepex'):
            Tmp = DepexXml()
            Depex = Tmp.FromXml(XmlNode(SubItem, 'DxeDepex'), 'DxeDepex')
            DxeDepexList.append(Depex)
        Module.SetDxeDepex(DxeDepexList)

        #
        # SmmDepex
        #
        SmmDepexList = []
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/SmmDepex'):
            Tmp = DepexXml()
            Depex = Tmp.FromXml(XmlNode(SubItem, 'SmmDepex'), 'SmmDepex')
            SmmDepexList.append(Depex)
        Module.SetSmmDepex(SmmDepexList)

        #
        # MiscellaneousFile
        Tmp = MiscellaneousFileXml()
        MiscFileList = Tmp.FromXml(XmlNode(Item, '/ModuleSurfaceArea/MiscellaneousFiles'), 'MiscellaneousFiles')
        if MiscFileList:
            Module.SetMiscFileList([MiscFileList])
        else:
            Module.SetMiscFileList([])

        #
        # UserExtensions
        #
        for Item in XmlList(Item, '/ModuleSurfaceArea/UserExtensions'):
            Tmp = UserExtensionsXml()
            UserExtension = Tmp.FromXml(Item, 'UserExtensions')
            Module.SetUserExtensionList(Module.GetUserExtensionList() + [UserExtension])

        return Module

    def FromXml(self, Item, Key, IsStandAlongModule=False):
        IsBinaryModule = XmlAttribute(Item, 'BinaryModule')
        #
        # Header
        #
        Tmp = HeaderXml()
        Module = Tmp.FromXml(XmlNode(Item, '/%s/Header' % Key), 'Header', True, IsStandAlongModule)
        Module.SetBinaryModule(IsBinaryModule)

        if IsBinaryModule:
            GlobalData.gIS_BINARY_INF = True

        #
        # ModuleProperties
        #
        Tmp = ModulePropertyXml()
        (Module, BootModes, Events, HOBs) = \
        Tmp.FromXml(XmlNode(Item, '/ModuleSurfaceArea/ModuleProperties'), 'ModuleProperties', Module)
        Module.SetBootModeList(BootModes)
        Module.SetEventList(Events)
        Module.SetHobList(HOBs)
        #
        # ClonedFrom
        #
        Tmp = ClonedFromXml()
        ClonedFrom = Tmp.FromXml(XmlNode(Item, '/ModuleSurfaceArea/ClonedFrom'), 'ClonedFrom')
        if ClonedFrom:
            Module.SetClonedFrom(ClonedFrom)

        #
        # LibraryClass
        #
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/LibraryClassDefinitions/LibraryClass'):
            Tmp = LibraryClassXml()
            LibraryClass = Tmp.FromXml(SubItem, 'LibraryClass')
            Module.SetLibraryClassList(Module.GetLibraryClassList() + [LibraryClass])

        if XmlList(Item, '/ModuleSurfaceArea/LibraryClassDefinitions') and \
           not XmlList(Item, '/ModuleSurfaceArea/LibraryClassDefinitions/LibraryClass'):
            Module.SetLibraryClassList([None])

        #
        # SourceFiles
        #
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/SourceFiles/Filename'):
            Tmp = SourceFileXml()
            SourceFile = Tmp.FromXml(SubItem, 'Filename')
            Module.SetSourceFileList(Module.GetSourceFileList() + [SourceFile])

        if XmlList(Item, '/ModuleSurfaceArea/SourceFiles') and \
           not XmlList(Item, '/ModuleSurfaceArea/SourceFiles/Filename') :
            Module.SetSourceFileList([None])

        #
        # BinaryFile
        #
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/BinaryFiles/BinaryFile'):
            Tmp = BinaryFileXml()
            BinaryFile = Tmp.FromXml(SubItem, 'BinaryFile')
            Module.SetBinaryFileList(Module.GetBinaryFileList() + [BinaryFile])

        if XmlList(Item, '/ModuleSurfaceArea/BinaryFiles') and \
           not XmlList(Item, '/ModuleSurfaceArea/BinaryFiles/BinaryFile') :
            Module.SetBinaryFileList([None])
        #
        # PackageDependencies
        #
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/PackageDependencies/Package'):
            Tmp = PackageXml()
            PackageDependency = Tmp.FromXml(SubItem, 'Package')
            Module.SetPackageDependencyList(Module.GetPackageDependencyList() + [PackageDependency])

        if XmlList(Item, '/ModuleSurfaceArea/PackageDependencies') and \
           not XmlList(Item, '/ModuleSurfaceArea/PackageDependencies/Package'):
            Module.SetPackageDependencyList([None])

        #
        # Guid
        #
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/Guids/GuidCName'):
            Tmp = GuidXml('Module')
            GuidProtocolPpi = Tmp.FromXml(SubItem, 'GuidCName')
            Module.SetGuidList(Module.GetGuidList() + [GuidProtocolPpi])

        if XmlList(Item, '/ModuleSurfaceArea/Guids') and not XmlList(Item, '/ModuleSurfaceArea/Guids/GuidCName'):
            Module.SetGuidList([None])

        #
        # Protocol
        #
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/Protocols/Protocol'):
            Tmp = ProtocolXml('Module')
            GuidProtocolPpi = Tmp.FromXml(SubItem, 'Protocol')
            Module.SetProtocolList(Module.GetProtocolList() + [GuidProtocolPpi])

        if XmlList(Item, '/ModuleSurfaceArea/Protocols') and not XmlList(Item, '/ModuleSurfaceArea/Protocols/Protocol'):
            Module.SetProtocolList([None])

        #
        # Ppi
        #
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/PPIs/Ppi'):
            Tmp = PpiXml('Module')
            GuidProtocolPpi = Tmp.FromXml(SubItem, 'Ppi')
            Module.SetPpiList(Module.GetPpiList() + [GuidProtocolPpi])

        if XmlList(Item, '/ModuleSurfaceArea/PPIs') and not XmlList(Item, '/ModuleSurfaceArea/PPIs/Ppi'):
            Module.SetPpiList([None])

        #
        # Extern
        #
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/Externs/Extern'):
            Tmp = ExternXml()
            Extern = Tmp.FromXml(SubItem, 'Extern')
            Module.SetExternList(Module.GetExternList() + [Extern])

        if XmlList(Item, '/ModuleSurfaceArea/Externs') and not XmlList(Item, '/ModuleSurfaceArea/Externs/Extern'):
            Module.SetExternList([None])

        if not Module.GetBinaryModule():
            #
            # PcdCoded
            #
            for SubItem in XmlList(Item, '/ModuleSurfaceArea/PcdCoded/PcdEntry'):
                Tmp = PcdEntryXml()
                PcdEntry = Tmp.FromXml3(SubItem, 'PcdEntry')
                Module.SetPcdList(Module.GetPcdList() + [PcdEntry])

            if XmlList(Item, '/ModuleSurfaceArea/PcdCoded') and \
                not XmlList(Item, '/ModuleSurfaceArea/PcdCoded/PcdEntry'):
                Module.SetPcdList([None])

        Module = self.FromXml2(Item, Module)
        #
        # return the module object
        #
        self.Module = Module
        return self.Module

    def ToXml(self, Module):
        if self.Package:
            pass
        #
        # Create root node of module surface area
        #
        DomModule = minidom.Document().createElement('ModuleSurfaceArea')
        if Module.GetBinaryModule():
            DomModule.setAttribute('BinaryModule', 'true')

        #
        # Header
        #
        Tmp = HeaderXml()
        DomModule.appendChild(Tmp.ToXml(Module, 'Header'))
        #
        # ModuleProperties
        #
        Tmp = ModulePropertyXml()
        DomModule.appendChild(Tmp.ToXml(Module, Module.GetBootModeList(), Module.GetEventList(), Module.GetHobList(), \
                                        'ModuleProperties'))
        #
        # ClonedFrom
        #
        Tmp = ClonedFromXml()
        if Module.GetClonedFrom():
            DomModule.appendChild(Tmp.ToXml(Module.GetClonedFrom(), 'ClonedFrom'))
        #
        # LibraryClass
        #
        LibraryClassNode = CreateXmlElement('LibraryClassDefinitions', '', [], [])
        for LibraryClass in Module.GetLibraryClassList():
            Tmp = LibraryClassXml()
            LibraryClassNode.appendChild(Tmp.ToXml2(LibraryClass, 'LibraryClass'))
        DomModule.appendChild(LibraryClassNode)
        #
        # SourceFile
        #
        SourceFileNode = CreateXmlElement('SourceFiles', '', [], [])
        for SourceFile in Module.GetSourceFileList():
            Tmp = SourceFileXml()
            SourceFileNode.appendChild(Tmp.ToXml(SourceFile, 'Filename'))
        DomModule.appendChild(SourceFileNode)
        #
        # BinaryFile
        #
        BinaryFileNode = CreateXmlElement('BinaryFiles', '', [], [])
        for BinaryFile in Module.GetBinaryFileList():
            Tmp = BinaryFileXml()
            BinaryFileNode.appendChild(Tmp.ToXml(BinaryFile, 'BinaryFile'))
        DomModule.appendChild(BinaryFileNode)
        #
        # PackageDependencies
        #
        PackageDependencyNode = CreateXmlElement('PackageDependencies', '', [], [])
        for PackageDependency in Module.GetPackageDependencyList():
            Tmp = PackageXml()
            PackageDependencyNode.appendChild(Tmp.ToXml(PackageDependency, 'Package'))
        DomModule.appendChild(PackageDependencyNode)

        #
        # Guid
        #
        GuidProtocolPpiNode = CreateXmlElement('Guids', '', [], [])
        for GuidProtocolPpi in Module.GetGuidList():
            Tmp = GuidXml('Module')
            GuidProtocolPpiNode.appendChild(Tmp.ToXml(GuidProtocolPpi, 'GuidCName'))
        DomModule.appendChild(GuidProtocolPpiNode)

        #
        # Protocol
        #
        GuidProtocolPpiNode = CreateXmlElement('Protocols', '', [], [])
        for GuidProtocolPpi in Module.GetProtocolList():
            Tmp = ProtocolXml('Module')
            GuidProtocolPpiNode.appendChild(Tmp.ToXml(GuidProtocolPpi, 'Protocol'))
        DomModule.appendChild(GuidProtocolPpiNode)

        #
        # Ppi
        #
        GuidProtocolPpiNode = CreateXmlElement('PPIs', '', [], [])
        for GuidProtocolPpi in Module.GetPpiList():
            Tmp = PpiXml('Module')
            GuidProtocolPpiNode.appendChild(Tmp.ToXml(GuidProtocolPpi, 'Ppi'))
        DomModule.appendChild(GuidProtocolPpiNode)
        #
        # Extern
        #
        ExternNode = CreateXmlElement('Externs', '', [], [])
        for Extern in Module.GetExternList():
            Tmp = ExternXml()
            ExternNode.appendChild(Tmp.ToXml(Extern, 'Extern'))
        DomModule.appendChild(ExternNode)
        #
        # PcdCoded
        #
        PcdEntryNode = CreateXmlElement('PcdCoded', '', [], [])
        for PcdEntry in Module.GetPcdList():
            Tmp = PcdEntryXml()
            PcdEntryNode.appendChild(Tmp.ToXml3(PcdEntry, 'PcdEntry'))
        DomModule.appendChild(PcdEntryNode)

        #
        # PeiDepex
        #
        if Module.GetPeiDepex():
            for Item in Module.GetPeiDepex():
                Tmp = DepexXml()
                DomModule.appendChild(Tmp.ToXml(Item, 'PeiDepex'))

        #
        # DxeDepex
        #
        if Module.GetDxeDepex():
            for Item in Module.GetDxeDepex():
                Tmp = DepexXml()
                DomModule.appendChild(Tmp.ToXml(Item, 'DxeDepex'))

        #
        # SmmDepex
        #
        if Module.GetSmmDepex():
            for Item in Module.GetSmmDepex():
                Tmp = DepexXml()
                DomModule.appendChild(Tmp.ToXml(Item, 'SmmDepex'))

        #
        # MiscellaneousFile
        #
        if Module.GetMiscFileList():
            Tmp = MiscellaneousFileXml()
            DomModule.appendChild(Tmp.ToXml(Module.GetMiscFileList()[0], 'MiscellaneousFiles'))
        #
        # UserExtensions
        #
        if Module.GetUserExtensionList():
            for UserExtension in Module.GetUserExtensionList():
                Tmp = UserExtensionsXml()
                DomModule.appendChild(Tmp.ToXml(UserExtension, 'UserExtensions'))

        return DomModule

##
# BuildFlagXml used to generate BuildFlag for <AsBuilt>
#
class BuildFlagXml(object):
    def __init__(self):
        self.Target = ''
        self.TagName = ''
        self.Family = ''
        self.AsBuiltFlags = ''

    def FromXml(self, Item, Key):
        self.Target = XmlElement(Item, '%s/Target' % Key)
        self.TagName = XmlElement(Item, '%s/TagName' % Key)
        self.Family = XmlElement(Item, '%s/Family' % Key)

        BuildFlag = BinaryBuildFlagObject()

        BuildFlag.SetTarget(self.Target)
        BuildFlag.SetTagName(self.TagName)
        BuildFlag.SetFamily(self.Family)

        return BuildFlag

    #
    # For AsBuild INF usage
    #
    def FromXml2(self, Item, Key):
        self.AsBuiltFlags = XmlElement(Item, '%s' % Key)

        LineList = GetSplitValueList(self.AsBuiltFlags, '\n')
        ReturnLine = ''
        Count = 0
        for Line in LineList:
            if Count == 0:
                ReturnLine = "# " + Line
            else:
                ReturnLine = ReturnLine + '\n' + '# ' + Line
            Count += 1

        BuildFlag = BinaryBuildFlagObject()
        BuildFlag.SetAsBuiltOptionFlags(ReturnLine)

        return BuildFlag

    def ToXml(self, BuildFlag, Key):
        if self.Target:
            pass
        AttributeList = []
        NodeList = []
        NodeList.append(['BuildFlags', BuildFlag])

        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)
        return Root
