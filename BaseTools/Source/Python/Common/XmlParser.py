## @file
# This file is used to parse a xml file of .PKG file
#
# Copyright (c) 2008, Intel Corporation
# All rights reserved. This program and the accompanying materials
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
from xml.dom import minidom
from XmlRoutines import *
from CommonDataClass.DistributionPackageClass import *
from CommonDataClass.PackageClass import *
from CommonDataClass.ModuleClass import *
from Common.String import GetStringOfList

#
# Get Help Text
#
def GetHelpTextList(HelpText):
    HelpTextList = []
    for HT in HelpText:
        HelpTextObj = HelpTextClass()
        HelpTextObj.Lang = HT.Lang
        HelpTextObj.String = HT.HelpText
        HelpTextList.append(HelpTextObj)
    return HelpTextList

# HeaderXml
class HeaderXml(object):
    def __init__(self):
        self.Name = ''
        self.BaseName = ''
        self.GUID = ''
        self.Version = ''
        self.Copyright = ''
        self.License = ''
        self.Abstract = ''
        self.Description = ''

    def FromXml(self, Item, Key):
        self.Name = XmlElement(Item, '%s/Name' % Key)
        self.BaseName = XmlAttribute(XmlNode(Item, '%s/Name' % Key), 'BaseName')
        self.GUID = XmlElement(Item, '%s/GUID' % Key)
        self.Version = XmlAttribute(XmlNode(Item, '%s/GUID' % Key), 'Version')
        self.Copyright = XmlElement(Item, '%s/Copyright' % Key)
        self.License = XmlElement(Item, '%s/License' % Key)
        self.Abstract = XmlElement(Item, '%s/Abstract' % Key)
        self.Description = XmlElement(Item, '%s/Description' % Key)
        
        ModuleHeader = ModuleHeaderClass()
        ModuleHeader.Name = self.Name
        ModuleHeader.BaseName = self.BaseName
        ModuleHeader.Guid = self.GUID
        ModuleHeader.Version = self.Version
        ModuleHeader.Copyright = self.Copyright
        ModuleHeader.License = self.License
        ModuleHeader.Abstract = self.Abstract
        ModuleHeader.Description = self.Description
        
        return ModuleHeader
        
    def ToXml(self, Header, Key):
        Element1 = CreateXmlElement('Name', Header.Name, [], [['BaseName', Header.BaseName]])
        Element2 = CreateXmlElement('GUID', Header.Guid, [], [['Version', Header.Version]])
        AttributeList = []
        NodeList = [Element1,
                    Element2,
                    ['Abstract', Header.Abstract],
                    ['Copyright', Header.Copyright],
                    ['License', Header.License],
                    ['Description', Header.Description],
                    ]
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)

        return Root

    def __str__(self):
        return "Name = %s BaseName = %s GUID = %s Version = %s Copyright = %s License = %s Abstract = %s Description = %s" \
               % (self.Name, self.BaseName, self.GUID, self.Version, self.Copyright, self.License, self.Abstract, self.Description)

# DistributionPackageHeaderXml
class DistributionPackageHeaderXml(object):
    def __init__(self):
        self.Header = HeaderXml()
        self.ReadOnly = False
        self.RePackage = True
        self.Vendor = ''
        self.Date = ''
        self.Signature = ''
        self.XmlSpecification = ''
    
    def FromXml(self, Item, Key):
        self.ReadOnly = XmlAttribute(XmlNode(Item, '%s' % Key), 'ReadOnly')
        self.RePackage = XmlAttribute(XmlNode(Item, '%s' % Key), 'RePackage')
        self.Vendor = XmlElement(Item, '%s/Vendor' % Key)
        self.Date = XmlElement(Item, '%s/Date' % Key)
        self.Signature = XmlElement(Item, '%s/Signature' % Key)
        self.XmlSpecification = XmlElement(Item, '%s/XmlSpecification' % Key)
        self.Header.FromXml(Item, Key)
        
        DistributionPackageHeader = DistributionPackageHeaderClass()
        DistributionPackageHeader.ReadOnly = self.ReadOnly
        DistributionPackageHeader.RePackage = self.RePackage
        DistributionPackageHeader.Name = self.Header.Name
        DistributionPackageHeader.BaseName = self.Header.BaseName
        DistributionPackageHeader.Guid = self.Header.GUID
        DistributionPackageHeader.Version = self.Header.Version
        DistributionPackageHeader.Vendor = self.Vendor
        DistributionPackageHeader.Date = self.Date
        DistributionPackageHeader.Copyright = self.Header.Copyright
        DistributionPackageHeader.License = self.Header.License
        DistributionPackageHeader.Abstract = self.Header.Abstract
        DistributionPackageHeader.Description = self.Header.Description
        DistributionPackageHeader.Signature = self.Signature
        DistributionPackageHeader.XmlSpecification = self.XmlSpecification
        
        return DistributionPackageHeader
    
    def ToXml(self, DistributionPackageHeader, Key):
        Element1 = CreateXmlElement('Name', DistributionPackageHeader.Name, [], [['BaseName', DistributionPackageHeader.BaseName]])
        Element2 = CreateXmlElement('GUID', DistributionPackageHeader.Guid, [], [['Version', DistributionPackageHeader.Version]])
        AttributeList = [['ReadOnly', str(DistributionPackageHeader.ReadOnly)], ['RePackage', str(DistributionPackageHeader.RePackage)]]
        NodeList = [Element1,
                    Element2,
                    ['Vendor', DistributionPackageHeader.Vendor],
                    ['Date', DistributionPackageHeader.Date],
                    ['Copyright', DistributionPackageHeader.Copyright],
                    ['License', DistributionPackageHeader.License],
                    ['Abstract', DistributionPackageHeader.Abstract],
                    ['Description', DistributionPackageHeader.Description],
                    ['Signature', DistributionPackageHeader.Signature],
                    ['XmlSpecification', DistributionPackageHeader.XmlSpecification],
                    ]
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)

        return Root
    
    def __str__(self):
        return "ReadOnly = %s RePackage = %s Vendor = %s Date = %s Signature = %s XmlSpecification = %s %s" \
               % (self.ReadOnly, self.RePackage, self.Vendor, self.Date, self.Signature, self.XmlSpecification, self.Header)

# PackageHeaderXml
class PackageHeaderXml(object):
    def __init__(self):
        self.Header = HeaderXml()
        self.PackagePath = ''
    
    def FromXml(self, Item, Key):
        self.PackagePath = XmlElement(Item, '%s/PackagePath' % Key)
        self.Header.FromXml(Item, Key)
        
        PackageHeader = PackageHeaderClass()
        PackageHeader.Name = self.Header.Name
        PackageHeader.BaseName = self.Header.BaseName
        PackageHeader.Guid = self.Header.GUID
        PackageHeader.Version = self.Header.Version
        PackageHeader.Copyright = self.Header.Copyright
        PackageHeader.License = self.Header.License
        PackageHeader.Abstract = self.Header.Abstract
        PackageHeader.Description = self.Header.Description
        PackageHeader.CombinePath = self.PackagePath

        return PackageHeader
    
    def ToXml(self, PackageHeader, Key):
        Element1 = CreateXmlElement('Name', PackageHeader.Name, [], [['BaseName', PackageHeader.BaseName]])
        Element2 = CreateXmlElement('GUID', PackageHeader.Guid, [], [['Version', PackageHeader.Version]])
        AttributeList = []
        NodeList = [Element1,
                    Element2,
                    ['Copyright', PackageHeader.Copyright],
                    ['License', PackageHeader.License],
                    ['Abstract', PackageHeader.Abstract],
                    ['Description', PackageHeader.Description],
                    ['PackagePath', PackageHeader.CombinePath],
                    ]
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)
        
        return Root
    
    def __str__(self):
        return "PackagePath = %s %s" \
               % (self.PackagePath, self.Header)

# ClonedFromXml
class ClonedFromXml(object):
    def __init__(self):
        self.GUID = ''
        self.Version = ''
    
    def FromXml(self, Item, Key):
        self.GUID = XmlElement(Item, '%s/GUID' % Key)
        self.Version = XmlAttribute(XmlNode(Item, '%s/GUID' % Key), 'Version')
        
        if self.GUID == '' and self.Version == '':
            return None
        
        ClonedFrom = ClonedRecordClass()
        ClonedFrom.PackageGuid = self.GUID
        ClonedFrom.PackageVersion = self.Version
        
        return ClonedFrom
    
    def ToXml(self, ClonedFrom, Key):
        Root = minidom.Document()
        Element1 = CreateXmlElement('GUID', ClonedFrom.PackageGuid, [], [['Version', ClonedFrom.PackageVersion]])
        AttributeList = []
        NodeList = [Element1]
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)
        
        return Root
    
    def __str__(self):
        return "GUID = %s Version = %s" % (self.GUID, self.Version)

# CommonDefinesXml
class CommonDefinesXml(object):
    def __init__(self):
        self.Usage = ''
        self.SupArchList = ''
        self.SupModList = ''
        self.FeatureFlag = ''
    
    def FromXml(self, Item, Key):
        self.Usage = XmlAttribute(Item, 'Usage')
        self.SupArchList = XmlAttribute(Item, 'SupArchList')
        self.SupModList = XmlAttribute(Item, 'SupModList')
        self.FeatureFlag = XmlAttribute(Item, 'FeatureFlag')
    
    def ToXml(self):
        pass
    
    def __str__(self):
        return "Usage = %s SupArchList = %s SupModList = %s FeatureFlag = %s" % (self.Usage, self.SupArchList, self.SupModList, self.FeatureFlag)

# HelpTextXml
class HelpTextXml(object):
    def __init__(self):
        self.HelpText = ''
        self.Lang = ''
        
    def FromXml(self, Item, Key):
        self.HelpText = XmlElement(Item, 'HelpText')
        self.Lang = XmlAttribute(Item, 'Lang')
    
    def ToXml(self, HelpText, Key = 'HelpText'):
        return CreateXmlElement('%s' % Key, HelpText.String, [], [['Lang', HelpText.Lang]])

    def __str__(self):
        return "HelpText = %s Lang = %s" % (self.HelpText, self.Lang)

# LibraryClassXml
class LibraryClassXml(object):
    def __init__(self):
        self.Keyword = ''
        self.HeaderFile = ''
        self.RecommendedInstanceGuid = ''
        self.RecommendedInstanceVersion = ''
        self.CommonDefines = CommonDefinesXml()
        self.HelpText = []
        
    def FromXml(self, Item, Key):
        self.Keyword = XmlAttribute(XmlNode(Item, '%s' % Key), 'Keyword')
        if self.Keyword == '':
            self.Keyword = XmlElement(Item, '%s/Keyword' % Key)
        self.HeaderFile = XmlElement(Item, '%s/HeaderFile' % Key)
        self.RecommendedInstanceGuid = XmlElement(Item, '%s/RecommendedInstance/GUID' % Key)
        self.RecommendedInstanceVersion = XmlAttribute(XmlNode(Item, '%s/RecommendedInstance/GUID' % Key), 'Version')
        self.CommonDefines.FromXml(XmlNode(Item, '%s' % Key), Key)
        for HelpTextItem in XmlList(Item, '%s/HelpText' % Key):
            HelpTextObj = HelpTextXml()
            HelpTextObj.FromXml(HelpTextItem, '%s/HelpText' % Key)
            self.HelpText.append(HelpTextObj)
        
        LibraryClass = LibraryClassClass()
        LibraryClass.LibraryClass = self.Keyword
        LibraryClass.IncludeHeader = self.HeaderFile
        LibraryClass.SupArchList = self.CommonDefines.SupArchList
        LibraryClass.SupModuleList = self.CommonDefines.SupModList
        LibraryClass.RecommendedInstanceGuid = self.RecommendedInstanceGuid
        LibraryClass.RecommendedInstanceVersion = self.RecommendedInstanceVersion
        LibraryClass.HelpTextList = GetHelpTextList(self.HelpText)
        
        return LibraryClass
        
    def ToXml(self, LibraryClass, Key):
        Element1 = CreateXmlElement('GUID', LibraryClass.RecommendedInstanceGuid, [], [['Version', LibraryClass.RecommendedInstanceVersion]])
        Element2 = CreateXmlElement('RecommendedInstance', '', [Element1], [])
        AttributeList = [['Keyword', LibraryClass.LibraryClass], 
                         ['SupArchList', GetStringOfList(LibraryClass.SupArchList)],
                         ['SupModList', GetStringOfList(LibraryClass.SupModuleList)]
                        ]
        NodeList = [['HeaderFile', LibraryClass.IncludeHeader],
                    Element2
                    ]
        for Item in LibraryClass.HelpTextList:
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item))
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)

        return Root

    def __str__(self):
        Str = "Keyword = %s HeaderFile = %s RecommendedInstanceGuid = %s RecommendedInstanceVersion = %s %s" \
             % (self.Keyword, self.HeaderFile, self.RecommendedInstanceGuid, self.RecommendedInstanceVersion, \
                self.CommonDefines)
        for Item in self.HelpText:
            Str = Str + "\n\t" + str(Item)
        return Str

# IndustryStandardHeaderXml
class IndustryStandardHeaderXml(object):
    def __init__(self):
        self.HeaderFile = ''
        self.HelpText = []
        
    def FromXml(self, Item, Key):
        self.HeaderFile = XmlElement(Item, '%s/HeaderFile' % Key)
        for HelpTextItem in XmlList(Item, '%s/HelpText' % Key):
            HelpTextObj = HelpTextXml()
            HelpTextObj.FromXml(HelpTextItem, '%s/HelpText' % Key)
            self.HelpText.append(HelpTextObj)
        
        Include = IncludeClass()
        Include.FilePath = self.HeaderFile
        Include.HelpTextList = GetHelpTextList(self.HelpText)
        
        return Include

    def ToXml(self, IndustryStandardHeader, Key):
        AttributeList = []
        NodeList = [['HeaderFile', IndustryStandardHeader.FilePath]]
        for Item in IndustryStandardHeader.HelpTextList:
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item))
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)

        return Root

    def __str__(self):
        Str = "HeaderFile = %s" % (self.HeaderFile)
        for Item in self.HelpText:
            Str = Str + "\n\t" + str(Item)
        return Str

# PackageIncludeHeaderXml
class PackageIncludeHeaderXml(object):
    def __init__(self):
        self.HeaderFile = ''
        self.CommonDefines = CommonDefinesXml()
        self.HelpText = []
        
    def FromXml(self, Item, Key):
        self.HeaderFile = XmlElement(Item, '%s/HeaderFile' % Key)
        self.CommonDefines.FromXml(XmlNode(Item, '%s/HeaderFile' % Key), Key)
        for HelpTextItem in XmlList(Item, '%s/HelpText' % Key):
            HelpTextObj = HelpTextXml()
            HelpTextObj.FromXml(HelpTextItem, '%s/HelpText' % Key)
            self.HelpText.append(HelpTextObj)

        Include = IncludeClass()
        Include.FilePath = self.HeaderFile
        Include.SupArchList = self.CommonDefines.SupArchList
        Include.SupModuleList = self.CommonDefines.SupModList
        Include.HelpTextList = GetHelpTextList(self.HelpText)
            
        return Include

    def ToXml(self, PackageIncludeHeader, Key):
        AttributeList = [['SupArchList', PackageIncludeHeader.SupArchList],
                         ['SupModList', PackageIncludeHeader.SupModuleList]
                        ]
        NodeList = [['HeaderFile', PackageIncludeHeader.FilePath]]
        for Item in PackageIncludeHeader.HelpTextList:
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item))
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)
        
        return Root

    def __str__(self):
        Str = "HeaderFile = %s\n\t%s" % (self.HeaderFile, self.CommonDefines)
        for Item in self.HelpText:
            Str = Str + "\n\t" + str(Item)
        return Str

#GUID/Protocol/Ppi
class GuidProtocolPpiXml(object):
    def __init__(self):
        self.UiName = ''
        self.GuidTypes = ''
        self.Notify = ''
        self.CName = ''
        self.GuidValue = ''
        self.CommonDefines = CommonDefinesXml()
        self.HelpText = []
        
    def FromXml(self, Item, Key):
        self.UiName = XmlAttribute(XmlNode(Item, '%s' % Key), 'UiName')
        self.GuidTypes = XmlAttribute(XmlNode(Item, '%s' % Key), 'GuidTypes')
        self.GuidType = XmlAttribute(XmlNode(Item, '%s' % Key), 'GuidType')
        self.Notify = XmlAttribute(XmlNode(Item, '%s' % Key), 'Notify')
        self.CName = XmlElement(Item, '%s/CName' % Key)
        self.GuidValue = XmlElement(Item, '%s/GuidValue' % Key)
        self.VariableName = XmlElement(Item, '%s/VariableName' % Key)
        self.CommonDefines.FromXml(XmlNode(Item, '%s' % Key), Key)
        for HelpTextItem in XmlList(Item, '%s/HelpText' % Key):
            HelpTextObj = HelpTextXml()
            HelpTextObj.FromXml(HelpTextItem, '%s/HelpText' % Key)
            self.HelpText.append(HelpTextObj)
            
        GuidProtocolPpi = GuidProtocolPpiCommonClass()
        GuidProtocolPpi.Name = self.UiName
        GuidProtocolPpi.CName = self.CName
        GuidProtocolPpi.Guid = self.GuidValue
        GuidProtocolPpi.VariableName = self.VariableName
        GuidProtocolPpi.Notify = self.Notify
        GuidProtocolPpi.Usage = self.CommonDefines.Usage
        GuidProtocolPpi.FeatureFlag = self.CommonDefines.FeatureFlag
        GuidProtocolPpi.SupArchList = self.CommonDefines.SupArchList
        GuidProtocolPpi.SupModuleList = self.CommonDefines.SupModList
        GuidProtocolPpi.GuidTypeLists = self.GuidTypes
        GuidProtocolPpi.GuidTypeList = self.GuidType
        GuidProtocolPpi.HelpTextList = GetHelpTextList(self.HelpText)
        
        return GuidProtocolPpi

    def ToXml(self, GuidProtocolPpi, Key):
        AttributeList = [['Usage', GetStringOfList(GuidProtocolPpi.Usage)],
                         ['UiName', GuidProtocolPpi.Name],
                         ['GuidTypes', GetStringOfList(GuidProtocolPpi.GuidTypeLists)],
                         ['GuidType', GetStringOfList(GuidProtocolPpi.GuidTypeList)],
                         ['Notify', str(GuidProtocolPpi.Notify)],
                         ['SupArchList', GetStringOfList(GuidProtocolPpi.SupArchList)],
                         ['SupModList', GetStringOfList(GuidProtocolPpi.SupModuleList)],
                         ['FeatureFlag', GuidProtocolPpi.FeatureFlag]
                        ]
        NodeList = [['CName', GuidProtocolPpi.CName], 
                    ['GuidValue', GuidProtocolPpi.Guid],
                    ['VariableName', GuidProtocolPpi.VariableName]
                   ]
        for Item in GuidProtocolPpi.HelpTextList:
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item))
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)
        
        return Root

    def __str__(self):
        Str = "UiName = %s Notify = %s GuidTypes = %s CName = %s GuidValue = %s %s" \
             % (self.UiName, self.Notify, self.GuidTypes, self.CName, self.GuidValue, self.CommonDefines)
        for Item in self.HelpText:
            Str = Str + "\n\t" + str(Item)
        return Str

# PcdErrorXml
class PcdErrorXml(object):
    def __init__(self):
        self.ValidValueList = ''
        self.ValidValueListLang = ''
        self.ValidValueRange = ''
        self.Expression = ''
        self.ErrorNumber = ''
        self.ErrorMessage = []
        
    def FromXml(self, Item, Key):
        self.ValidValueList = XmlElement(Item, '%s/ValidValueList' % Key)
        self.ValidValueListLang = XmlAttribute(XmlNode(Item, '%s/ValidValueList' % Key), 'Lang')
        self.ValidValueRange = XmlElement(Item, '%s/ValidValueRange' % Key)
        self.Expression = XmlElement(Item, '%s/Expression' % Key)
        self.ErrorNumber = XmlElement(Item, '%s/ErrorNumber' % Key)
        for ErrMsg in XmlList(Item, '%s/ErrorMessage' % Key):
            ErrorMessageString = XmlElement(ErrMsg, 'ErrorMessage')
            ErrorMessageLang = XmlAttribute(XmlNode(ErrMsg, 'ErrorMessage'), 'Lang')
            self.ErrorMessage.append((ErrorMessageLang, ErrorMessageString))
        
        Error = PcdErrorClass()
        Error.ValidValueList = self.ValidValueList
        Error.ValidValueListLang = self.ValidValueListLang
        Error.ValidValueRange = self.ValidValueRange
        Error.Expression = self.Expression
        Error.ErrorNumber = self.ErrorNumber
        Error.ErrorMessage = self.ErrorMessage
        
        return Error

    def ToXml(self, PcdError, Key):
        AttributeList = []
        Element1 = CreateXmlElement('ValidValueList', PcdError.ValidValueList, [], [['Lang', PcdError.ValidValueListLang]])
        NodeList = [Element1,
                    ['ValidValueRange', PcdError.ValidValueRange], 
                    ['Expression', PcdError.Expression],
                    ['ErrorNumber', PcdError.ErrorNumber],
                   ]
        for Item in PcdError.ErrorMessage:
            Element = CreateXmlElement('ErrorMessage', Item[1], [], [['Lang', Item[0]]])
            NodeList.append(Element)
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)
        
        return Root

    def __str__(self):
        return "ValidValueList = %s ValidValueListLang = %s ValidValueRange = %s Expression = %s ErrorNumber = %s %s" \
               % (self.ValidValueList, self.ValidValueListLang, self.ValidValueRange, self.Expression, self.ErrorNumber, self.ErrorMessage)

# PcdEntryXml
class PcdEntryXml(object):
    def __init__(self):
        self.PcdItemType = ''
        self.PcdUsage = ''
        self.TokenSpaceGuidCName = ''
        self.TokenSpaceGuidValue = ''
        self.Token = ''
        self.CName = ''
        self.PcdCName = ''
        self.DatumType = ''
        self.ValidUsage = ''
        self.DefaultValue = ''
        self.MaxDatumSize = ''
        self.Value = ''
        self.Offset = ''
        self.CommonDefines = CommonDefinesXml()
        self.HelpText = []
        self.PcdError = []
        
    def FromXml(self, Item, Key):
        self.PcdItemType = XmlAttribute(XmlNode(Item, '%s' % Key), 'PcdItemType')
        self.PcdUsage = XmlAttribute(XmlNode(Item, '%s' % Key), 'PcdUsage')
        self.TokenSpaceGuidCName = XmlElement(Item, '%s/TokenSpaceGuidCName' % Key)
        self.TokenSpaceGuidValue = XmlElement(Item, '%s/TokenSpaceGuidValue' % Key)
        self.Token = XmlElement(Item, '%s/Token' % Key)
        self.CName = XmlElement(Item, '%s/CName' % Key)
        self.PcdCName = XmlElement(Item, '%s/PcdCName' % Key)
        self.DatumType = XmlElement(Item, '%s/DatumType' % Key)
        self.ValidUsage = XmlElement(Item, '%s/ValidUsage' % Key)
        self.DefaultValue = XmlElement(Item, '%s/DefaultValue' % Key)
        self.MaxDatumSize = XmlElement(Item, '%s/MaxDatumSize' % Key)
        self.Value = XmlElement(Item, '%s/Value' % Key)
        self.Offset = XmlElement(Item, '%s/Offset' % Key)
        self.CommonDefines.FromXml(XmlNode(Item, '%s' % Key), Key)
        for HelpTextItem in XmlList(Item, '%s/HelpText' % Key):
            HelpTextObj = HelpTextXml()
            HelpTextObj.FromXml(HelpTextItem, '%s/HelpText' % Key)
            self.HelpText.append(HelpTextObj)
        for PcdErrorItem in XmlList(Item, '%s/PcdError' % Key):
            PcdErrorObj = PcdErrorXml()
            PcdErrorObj.FromXml(PcdErrorItem, 'PcdError')
            self.PcdError.append(PcdErrorObj)
        
        PcdEntry = PcdClass()
        PcdEntry.SupArchList = self.CommonDefines.SupArchList
        PcdEntry.SupModuleList = self.CommonDefines.SupModList
        PcdEntry.TokenSpaceGuidCName = self.TokenSpaceGuidCName
        PcdEntry.TokenSpaceGuidValue = self.TokenSpaceGuidValue
        PcdEntry.Token = self.Token
        PcdEntry.CName = self.CName
        PcdEntry.PcdCName = self.PcdCName
        PcdEntry.DatumType = self.DatumType
        PcdEntry.ValidUsage = self.ValidUsage
        PcdEntry.PcdUsage = self.PcdUsage
        PcdEntry.Usage = self.CommonDefines.Usage
        PcdEntry.DefaultValue = self.DefaultValue
        PcdEntry.Value = self.Value
        PcdEntry.Offset = self.Offset
        PcdEntry.MaxDatumSize = self.MaxDatumSize
        PcdEntry.FeatureFlag = self.CommonDefines.FeatureFlag
        PcdEntry.PcdItemType = self.PcdItemType
        PcdEntry.HelpTextList = GetHelpTextList(self.HelpText)
        PcdEntry.PcdErrors = self.PcdError
        
        return PcdEntry

    def ToXml(self, PcdEntry, Key):
        AttributeList = [['SupArchList', GetStringOfList(PcdEntry.SupArchList)],
                         ['PcdUsage', PcdEntry.PcdUsage],
                         ['PcdItemType', PcdEntry.PcdItemType],
                         ['FeatureFlag', PcdEntry.FeatureFlag],
                         ['SupModList', GetStringOfList(PcdEntry.SupModuleList)]
                        ]
        NodeList = [['TokenSpaceGuidCName', PcdEntry.TokenSpaceGuidCName],
                    ['TokenSpaceGuidValue', PcdEntry.TokenSpaceGuidValue],
                    ['Token', PcdEntry.Token], 
                    ['CName', PcdEntry.CName],
                    ['PcdCName', PcdEntry.PcdCName],
                    ['DatumType', PcdEntry.DatumType],
                    ['ValidUsage', GetStringOfList(PcdEntry.ValidUsage)],
                    ['DefaultValue', PcdEntry.DefaultValue],
                    ['Value', PcdEntry.Value],
                    ['Offset', PcdEntry.Offset],
                    ['MaxDatumSize', PcdEntry.MaxDatumSize],
                   ]
        for Item in PcdEntry.HelpTextList:
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item, 'HelpText'))
        for Item in PcdEntry.PcdErrors:
            Tmp = PcdErrorXml()
            NodeList.append(Tmp.ToXml(Item, 'PcdError'))
        
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)
        
        return Root

    def __str__(self):
        Str = "PcdItemType = %s PcdUsage = %s TokenSpaceGuidCName = %s TokenSpaceGuidValue = %s Token = %s CName = %s PcdCName = %s DatumType = %s ValidUsage = %s DefaultValue = %s MaxDatumSize = %s Value = %s Offset = %s %s" \
             % (self.PcdItemType, self.PcdUsage, self.TokenSpaceGuidCName, self.TokenSpaceGuidValue, self.Token, self.CName, self.PcdCName, self.DatumType, self.ValidUsage, self.DefaultValue, self.MaxDatumSize, self.Value, self.Offset, self.CommonDefines)
        for Item in self.HelpText:
            Str = Str + "\n\t" + str(Item)
        for Item in self.PcdError:
            Str = Str + "\n\tPcdError:" + str(Item)
        return Str

# PcdCheckXml
class PcdCheckXml(object):
    def __init__(self):
        self.PcdCheck = ''
        
    def FromXml(self, Item, Key):
        self.PcdCheck = XmlElement(Item, 'PcdCheck')
        
        return self.PcdCheck

    def ToXml(self, PcdCheck, Key):
        Root = CreateXmlElement('%s' % Key, PcdCheck, [], [])
        return Root

    def __str__(self):
        return "PcdCheck = %s" % (self.PcdCheck)

# MiscellaneousFileXml
class MiscellaneousFileXml(object):
    def __init__(self):
        self.Header = HeaderXml()
        self.Files = []
    
    def FromXml(self, Item, Key):
        self.Header.FromXml(Item, Key)
        NewItem = XmlNode(Item, '%s/Header' % Key)
        self.Header.FromXml(NewItem, 'Header')
        
        for SubItem in XmlList(Item, '%s/Filename' % Key):
            Filename =  XmlElement(SubItem, '%s/Filename' % Key)
            Executable = XmlAttribute(XmlNode(SubItem, '%s/Filename' % Key), 'Executable')
            self.Files.append([Filename, Executable])
        
        MiscFile = MiscFileClass()
        MiscFile.Copyright = self.Header.Copyright
        MiscFile.License = self.Header.License
        MiscFile.Abstract = self.Header.Abstract
        MiscFile.Description = self.Header.Description
        for File in self.Files:
            FileObj = FileClass()
            FileObj.Filename = File[0]
            FileObj.Executable = File[1]
            MiscFile.Files.append(FileObj)
        
        return MiscFile

    def FromXml2(self, Item, Key):
        NewItem = XmlNode(Item, '%s/Header' % Key)
        self.Header.FromXml(NewItem, 'Header')
        
        for SubItem in XmlList(Item, '%s/Filename' % Key):
            Filename =  XmlElement(SubItem, '%s/Filename' % Key)
            Executable = XmlAttribute(XmlNode(SubItem, '%s/Filename' % Key), 'Executable')
            self.Files.append([Filename, Executable])
        
        MiscFile = MiscFileClass()
        MiscFile.Name = self.Header.Name
        MiscFile.Copyright = self.Header.Copyright
        MiscFile.License = self.Header.License
        MiscFile.Abstract = self.Header.Abstract
        MiscFile.Description = self.Header.Description
        for File in self.Files:
            FileObj = FileClass()
            FileObj.Filename = File[0]
            FileObj.Executable = File[1]
            MiscFile.Files.append(FileObj)
        
        return MiscFile


    def ToXml(self, MiscFile, Key):
        if MiscFile:
            NodeList = [['Copyright', MiscFile.Copyright],
                        ['License', MiscFile.License],
                        ['Abstract', MiscFile.Abstract],
                        ['Description', MiscFile.Description],
                       ]
            if MiscFile != None:
                for File in MiscFile.Files:
                    NodeList.append(CreateXmlElement('Filename', File.Filename, [], [['Executable', File.Executable]]))
                Root = CreateXmlElement('%s' % Key, '', NodeList, [])
            
                return Root
    
    def ToXml2(self, MiscFile, Key):
        if MiscFile:
            NodeList = [['Name', MiscFile.Name],
                        ['Copyright', MiscFile.Copyright],
                        ['License', MiscFile.License],
                        ['Abstract', MiscFile.Abstract],
                        ['Description', MiscFile.Description],
                       ]
            HeaderNode = CreateXmlElement('Header', '', NodeList, [])
            NodeList = [HeaderNode]
        
            for File in MiscFile.Files:
                NodeList.append(CreateXmlElement('Filename', File.Filename, [], [['Executable', File.Executable]]))
            Root = CreateXmlElement('%s' % Key, '', NodeList, [])
        
            return Root
    
    def __str__(self):
        Str = str(self.Header)
        for Item in self.Files:
            Str = Str + '\n\tFilename:' + str(Item)
        return Str    

# UserExtensionsXml
class UserExtensionsXml(object):
    def __init__(self):
        self.UserId = ''
        self.Identifier = ''
        self.Defines = []
        self.BuildOptions = []
    
    def FromXml(self, Item, Key):
        self.UserId = XmlAttribute(XmlNode(Item, '%s' % Key), 'UserId')
        self.Identifier = XmlAttribute(XmlNode(Item, '%s' % Key), 'Identifier')
        for SubItem in XmlList(Item, '%s/Define' % Key):
            self.Defines.append(XmlElement(SubItem, '%s/Define' % Key))
        for SubItem in XmlList(Item, '%s/BuildOption' % Key):
            self.BuildOptions.append(XmlElement(SubItem, '%s/BuildOption' % Key))
        
        UserExtension = UserExtensionsClass()
        UserExtension.UserID = self.UserId
        UserExtension.Identifier = self.Identifier
        UserExtension.Defines = self.Defines
        UserExtension.BuildOptions = self.BuildOptions
        
        return UserExtension

    def ToXml(self, UserExtension, Key):
        AttributeList = [['UserId', str(UserExtension.UserID)],
                         ['Identifier', str(UserExtension.Identifier)]
                        ]
        NodeList = []
        for Item in UserExtension.Defines:
            NodeList.append(['Define', Item])
        for Item in UserExtension.BuildOptions:
            NodeList.append(['BuildOption', Item])
        Root = CreateXmlElement('%s' % Key, UserExtension.Content, NodeList, AttributeList)
        
        return Root
    
    def __str__(self):
        Str = "UserId = %s Identifier = %s" % (self.UserId, self.Identifier)
        Str = Str + '\n\tDefines:' + str(self.Defines)
        Str = Str + '\n\tBuildOptions:' + str(self.BuildOptions)
        return Str

# BootModeXml
class BootModeXml(object):
    def __init__(self):
        self.SupportedBootModes = ''
        self.CommonDefines = CommonDefinesXml()
        self.HelpText = []
    
    def FromXml(self, Item, Key):
        self.SupportedBootModes = XmlElement(Item, '%s/SupportedBootModes' % Key)
        self.CommonDefines.FromXml(Item, Key)
        for HelpTextItem in XmlList(Item, '%s/HelpText' % Key):
            HelpTextObj = HelpTextXml()
            HelpTextObj.FromXml(HelpTextItem, '%s/HelpText' % Key)
            self.HelpText.append(HelpTextObj)
        
        BootMode = ModuleBootModeClass()
        BootMode.Name = self.SupportedBootModes
        BootMode.SupArchList = self.CommonDefines.SupArchList
        BootMode.Usage = self.CommonDefines.Usage
        BootMode.FeatureFlag = self.CommonDefines.FeatureFlag
        BootMode.HelpTextList = GetHelpTextList(self.HelpText)
        
        return BootMode

    def ToXml(self, BootMode, Key):
        AttributeList = [['Usage', BootMode.Usage],
                         ['SupArchList', GetStringOfList(BootMode.SupArchList)],
                         ['FeatureFlag', BootMode.FeatureFlag],
                        ]
        NodeList = [['SupportedBootModes', BootMode.Name]]
        for Item in BootMode.HelpTextList:
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item, 'HelpText'))
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)
        
        return Root
    
    def __str__(self):
        Str = "SupportedBootModes = %s %s" % (self.SupportedBootModes, self.CommonDefines)
        for Item in self.HelpText:
            Str = Str + '\n\t' + str(Item)
        return Str

# EventXml
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
        
        Event = ModuleEventClass()
        Event.Type = self.EventType
        Event.GuidCName = self.Name
        Event.SupArchList = self.CommonDefines.SupArchList
        Event.Usage = self.CommonDefines.Usage
        Event.FeatureFlag = self.CommonDefines.FeatureFlag
        Event.HelpTextList = GetHelpTextList(self.HelpText)
        
        return Event

    def ToXml(self, Event, Key):
        AttributeList = [['EventType', Event.Type],
                         ['Usage', Event.Usage],
                         ['SupArchList', GetStringOfList(Event.SupArchList)],
                         ['FeatureFlag', Event.FeatureFlag],
                        ]
        NodeList = []
        for Item in Event.HelpTextList:
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item, 'HelpText'))
        Root = CreateXmlElement('%s' % Key, Event.GuidCName, NodeList, AttributeList)
        
        return Root
    
    def __str__(self):
        Str = "EventType = %s %s" % (self.EventType, self.CommonDefines)
        for Item in self.HelpText:
            Str = Str + '\n\t' + str(Item)
        return Str

# HobXml
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
        
        Hob = ModuleHobClass()
        Hob.Type = self.HobType
        Hob.GuidCName = self.Name
        Hob.SupArchList = self.CommonDefines.SupArchList
        Hob.Usage = self.CommonDefines.Usage
        Hob.FeatureFlag = self.CommonDefines.FeatureFlag
        Hob.HelpTextList = GetHelpTextList(self.HelpText)
        
        return Hob

    def ToXml(self, Hob, Key):
        AttributeList = [['EventType', Hob.Type],
                         ['Usage', Hob.Usage],
                         ['SupArchList', GetStringOfList(Hob.SupArchList)],
                         ['FeatureFlag', Hob.FeatureFlag],
                        ]
        NodeList = []
        for Item in Hob.HelpTextList:
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item, 'HelpText'))
        Root = CreateXmlElement('%s' % Key, Hob.GuidCName, NodeList, AttributeList)
        
        return Root
    
    def __str__(self):
        Str = "HobType = %s %s" % (self.HobType, self.CommonDefines)
        for Item in self.HelpText:
            Str = Str + '\n\t' + str(Item)
        return Str

# ModulePropertyXml
class ModulePropertyXml(object):
    def __init__(self):
        self.CommonDefines = CommonDefinesXml()
        self.ModuleType = ''
        self.Path = ''
        self.PcdIsDriver = ''
        self.UefiSpecificationVersion = ''
        self.PiSpecificationVersion = ''
        self.Specification = ''
        self.SpecificationVersion = ''
        self.BootModes = []
        self.Events = []
        self.HOBs = []
    
    def FromXml(self, Item, Key, Header = None):
        self.CommonDefines.FromXml(Item, Key)
        self.ModuleType = XmlElement(Item, '%s/ModuleType' % Key)
        self.Path = XmlElement(Item, '%s/Path' % Key)
        self.PcdIsDriver = XmlElement(Item, '%s/PcdIsDriver' % Key)
        self.UefiSpecificationVersion = XmlElement(Item, '%s/UefiSpecificationVersion' % Key)
        self.PiSpecificationVersion = XmlElement(Item, '%s/PiSpecificationVersion' % Key)
        self.Specification = XmlElement(Item, '%s/Specification' % Key)
        self.SpecificationVersion = XmlAttribute(XmlNode(Item, '%s/Specification' % Key), 'Version')
        for SubItem in XmlList(Item, '%s/BootMode' % Key):
            A = BootModeXml()
            BootMode = A.FromXml(SubItem, 'BootMode')
            self.BootModes.append(BootMode)
        for SubItem in XmlList(Item, '%s/Event' % Key):
            A = EventXml()
            Event = A.FromXml(SubItem, 'Event')
            self.Events.append(Event)
        for SubItem in XmlList(Item, '%s/HOB' % Key):
            A = HobXml()
            Hob = A.FromXml(SubItem, 'HOB')
            self.HOBs.append(Hob)
            
        if Header == None:
            Header = ModuleHeaderClass()
        
        Header.ModuleType = self.ModuleType
        Header.SupArchList = self.CommonDefines.SupArchList
        Header.SupModuleList = self.CommonDefines.SupModList
        Header.CombinePath = self.Path
        Header.PcdIsDriver = self.PcdIsDriver
        Header.UefiSpecificationVersion = self.UefiSpecificationVersion
        Header.PiSpecificationVersion = self.PiSpecificationVersion
        
        return Header, self.BootModes, self.Events, self.HOBs
        
    
    def ToXml(self, Header, BootModes, Events, Hobs, Key):
        AttributeList = [['SupArchList', GetStringOfList(Header.SupArchList)],
                         ['SupModList', GetStringOfList(Header.SupModuleList)],
                        ]
        NodeList = [['ModuleType', Header.ModuleType],
                    ['Path', Header.CombinePath],
                    ['PcdIsDriver', Header.PcdIsDriver],
                    ['UefiSpecificationVersion', Header.UefiSpecificationVersion],
                    ['PiSpecificationVersion', Header.PiSpecificationVersion],
                   ]
        for Item in BootModes:
            Tmp = BootModeXml()
            NodeList.append(Tmp.ToXml(Item, 'BootMode'))
        for Item in Events:
            Tmp = EventXml()
            NodeList.append(Tmp.ToXml(Item, 'Event'))
        for Item in Hobs:
            Tmp = HobXml()
            NodeList.append(Tmp.ToXml(Item, 'Hob'))
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)
        
        return Root
    
    def __str__(self):
        Str = "ModuleType = %s Path = %s PcdIsDriver = %s UefiSpecificationVersion = %s PiSpecificationVersion = %s Specification = %s SpecificationVersion = %s %s" \
                % (self.ModuleType, self.Path, self.PcdIsDriver, self.UefiSpecificationVersion, self.PiSpecificationVersion, \
                   self.Specification, self.SpecificationVersion, self.CommonDefines)
        for Item in self.BootModes:
            Str = Str + '\n\t' + str(Item)
        for Item in self.Events:
            Str = Str + '\n\t' + str(Item)
        for Item in self.HOBs:
            Str = Str + '\n\t' + str(Item)
        return Str

# SourceFileXml
class SourceFileXml(object):
    def __init__(self):
        self.SourceFile = ''
        self.ToolChainFamily = ''
        self.FileType = ''
        self.CommonDefines = CommonDefinesXml()

    def FromXml(self, Item, Key):
        self.ToolChainFamily = XmlAttribute(Item, 'Family')
        self.FileType = XmlAttribute(Item, 'FileType')
        self.SourceFile = XmlElement(Item, 'Filename')
        self.CommonDefines.FromXml(Item, Key)
        
        SourceFile = ModuleSourceFileClass()
        SourceFile.SourceFile = self.SourceFile
        SourceFile.FileType = self.FileType
        SourceFile.ToolChainFamily = self.ToolChainFamily
        SourceFile.SupArchList = self.CommonDefines.SupArchList
        SourceFile.FeatureFlag = self.CommonDefines.FeatureFlag
        
        return SourceFile
    
    def ToXml(self, SourceFile, Key):
        AttributeList = [['SupArchList', GetStringOfList(SourceFile.SupArchList)],
                         ['Family', SourceFile.ToolChainFamily],
                         ['FileType', SourceFile.FileType],
                         ['FeatureFlag', SourceFile.FeatureFlag],
                        ]
        Root = CreateXmlElement('%s' % Key, SourceFile.SourceFile, [], AttributeList)
        
        return Root

# FilenameXml
class FilenameXml(object):
    def __init__(self):
        self.OS = ''
        self.Family = ''
        self.FileType = ''
        self.Filename = ''
        self.Executable = ''
        self.CommonDefines = CommonDefinesXml()
    
    def FromXml(self, Item, Key):
        self.OS = XmlAttribute(Item, 'OS')
        self.Family = XmlAttribute(Item, 'Family')
        self.FileType = XmlAttribute(Item, 'FileType')
        self.Filename = XmlElement(Item, 'Filename')
        self.Executable = XmlElement(Item, 'Executable')
        self.CommonDefines.FromXml(Item, Key)
        
        Filename = FileClass()
        Filename.Family = self.Family
        Filename.FileType = self.FileType
        Filename.Filename = self.Filename
        Filename.Executable = self.Executable
        Filename.SupArchList = self.CommonDefines.SupArchList
        Filename.FeatureFlag = self.CommonDefines.FeatureFlag
        
        return Filename
    
    def ToXml(self, Filename, Key):
        AttributeList = [['SupArchList', GetStringOfList(Filename.SupArchList)],
                         ['Family', Filename.Family],
                         ['FileType', Filename.FileType],
                         ['Executable', Filename.Executable],
                         ['FeatureFlag', Filename.FeatureFlag],
                        ]
        NodeList = [['Filename', Filename.Filename],
                   ]
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)
        
        return Root
    
    def __str__(self):
        return "OS = %s Family = %s FileType = %s Filename = %s Executable = %s %s" \
             % (self.OS, self.Family, self.FileType, self.Filename, self.Executable, self.CommonDefines)
    
class BinaryFileXml(object):
    def __init__(self):
        self.Filenames = []
        self.PatchPcdValues = []
        self.PcdExValues = []
        self.LibraryInstances = []
        self.BuildFlags = []
    
    def FromXml(self, Item, Key):
        BinaryFile = ModuleBinaryFileClass()
        for SubItem in XmlList(Item, '%s/Filename' % Key):
            A = FilenameXml()
            B = A.FromXml(SubItem, 'Filename')
            BinaryFile.Filenames.append(B)
        for SubItem in XmlList(Item, '%s/AsBuilt/PatchPcdValue' % Key):
            A = PcdEntryXml()
            B = A.FromXml(SubItem, 'PatchPcdValue')
            BinaryFile.PatchPcdValues.append(B)
        for SubItem in XmlList(Item, '%s/AsBuilt/PcdExValue' % Key):
            A = PcdEntryXml()
            B = A.FromXml(SubItem, 'PcdExValue')
            BinaryFile.PatchPcdValues.append(B)
        for SubItem in XmlList(Item, '%s/AsBuilt/LibraryInstances/GUID' % Key):
            GUID = XmlElement(SubItem, 'GUID')
            Version = XmlAttribute(XmlNode(SubItem, 'GUID'), 'Version')
            BinaryFile.LibraryInstances.append([GUID, Version])
        for SubItem in XmlList(Item, '%s/AsBuilt/BuildFlags' % Key):
            BinaryFile.BuildFlags.append(XmlElement(SubItem, 'BuildFlags'))
        
        return BinaryFile
    
    def ToXml(self, BinaryFile, Key):
        NodeList = []
        for Item in BinaryFile.Filenames:
            Tmp = FilenameXml()
            NodeList.append(Tmp.ToXml(Item, 'Filename'))
        AsBuiltNodeList = []
        for Item in BinaryFile.PatchPcdValues:
            Tmp = PcdEntryXml()
            AsBuiltNodeList.append(Tmp.ToXml(Item, 'PatchPcdValue'))
        for Item in BinaryFile.PcdExValues:
            Tmp = PcdEntryXml()
            AsBuiltNodeList.append(Tmp.ToXml(Item, 'PcdExValue'))
        LibNodeList = []
        for Item in BinaryFile.LibraryInstances:
            LibNode = CreateXmlElement('GUID', Item[0], [], [['Version', Item[1]]])
            LibNodeList.append(LibNode)
        if LibNodeList:
            AsBuiltNodeList.append(CreateXmlElement('LibraryInstances', '', LibNodeList, []))
        for Item in BinaryFile.BuildFlags:
            AsBuiltNodeList.append(CreateXmlElement('BuildFlags', Item, [], []))
        Element = CreateXmlElement('AsBuilt', '', AsBuiltNodeList, [])
        NodeList.append(Element)
        
        Root = CreateXmlElement('%s' % Key, '', NodeList, [])

        return Root
        
    def __str__(self):
        Str = "BinaryFiles:"
        for Item in self.Filenames:
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

# PackageXml
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
        
        PackageDependency = ModulePackageDependencyClass()
        PackageDependency.FilePath = self.Description
        PackageDependency.PackageGuid = self.Guid
        PackageDependency.PackageVersion = self.Version
        PackageDependency.FeatureFlag = self.CommonDefines.FeatureFlag
        PackageDependency.SupArchList = self.CommonDefines.SupArchList
        
        return PackageDependency

    def ToXml(self, PackageDependency, Key):
        AttributeList = [['SupArchList', GetStringOfList(PackageDependency.SupArchList)],
                         ['FeatureFlag', PackageDependency.FeatureFlag],
                        ]
        Element1 = CreateXmlElement('GUID', PackageDependency.PackageGuid, [], [['Version', PackageDependency.PackageVersion]])
        NodeList = [['Description', PackageDependency.FilePath],
                    Element1,
                   ]

        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)
        
        return Root

    def __str__(self):
        Str = "Description = %s Guid = %s Version = %s %s" \
             % (self.Description, self.Guid, self.Version, self.CommonDefines)
        return Str

# ExternXml
class ExternXml(object):
    def __init__(self):
        self.CommonDefines = CommonDefinesXml()
        self.EntryPoint = ''
        self.UnloadImage = ''
        self.Constructor = ''
        self.Destructor = ''
        self.HelpText = []
        
    def FromXml(self, Item, Key):
        self.CommonDefines.FromXml(Item, Key)
        self.EntryPoint = XmlElement(Item, '%s/EntryPoint' % Key)
        self.UnloadImage = XmlElement(Item, '%s/UnloadImage' % Key)
        self.Constructor = XmlElement(Item, '%s/Constructor' % Key)
        self.Destructor = XmlElement(Item, '%s/Destructor' % Key)
        for HelpTextItem in XmlList(Item, '%s/HelpText' % Key):
            HelpTextObj = HelpTextXml()
            HelpTextObj.FromXml(HelpTextItem, '%s/HelpText' % Key)
            self.HelpText.append(HelpTextObj)
        
        Extern = ModuleExternClass()
        Extern.EntryPoint = self.EntryPoint
        Extern.UnloadImage = self.UnloadImage
        Extern.Constructor = self.Constructor
        Extern.Destructor = self.Destructor
        Extern.SupArchList = self.CommonDefines.SupArchList
        Extern.FeatureFlag = self.CommonDefines.FeatureFlag
        Extern.HelpTextList = GetHelpTextList(self.HelpText)
        
        return Extern
    
    def ToXml(self, Extern, Key):
        AttributeList = [['SupArchList', GetStringOfList(Extern.SupArchList)],
                         ['FeatureFlag', Extern.FeatureFlag],
                        ]
        NodeList = [['EntryPoint', Extern.EntryPoint],
                    ['UnloadImage', Extern.UnloadImage],
                    ['Constructor', Extern.Constructor],
                    ['Destructor', Extern.Destructor],
                   ]
        for Item in Extern.HelpTextList:
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item, 'HelpText'))

        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)
        
        return Root
    
    def __str__(self):    
        Str = "EntryPoint = %s UnloadImage = %s Constructor = %s Destructor = %s %s" \
             % (self.EntryPoint, self.UnloadImage, self.Constructor, self.Destructor, self.CommonDefines)
        for Item in self.HelpText:
            Str = Str + '\n\t' + str(Item)
        return Str
# DepexXml
class DepexXml(object):
    def __init__(self):
        self.Expression = ''
        #self.HelpText = HelpTextXml()
        self.HelpText = []
    
    def FromXml(self, Item, Key):
        self.Expression = XmlElement(Item, '%s/Expression' % Key)
        for HelpTextItem in XmlList(Item, '%s/HelpText' % Key):
            HelpTextObj = HelpTextXml()
            HelpTextObj.FromXml(HelpTextItem, '%s/HelpText' % Key)
            self.HelpText.append(HelpTextObj)
        
        Depex = ModuleDepexClass()
        Depex.Depex = self.Expression
        Depex.HelpTextList = GetHelpTextList(self.HelpText)

        return Depex
        
    def ToXml(self, Depex, Key):
        AttributeList = []
        NodeList = [['Expression', Depex.Depex],
                   ]
        for Item in Depex.HelpTextList:
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item, 'HelpText'))

        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)
        
        return Root
    
    def __str__(self):    
        Str = "Expression = %s" % (self.Expression)
        for Item in self.HelpText:
            Str = Str + '\n\t' + str(Item)
        return Str

# PackageSurfaceAreaXml
class PackageSurfaceAreaXml(object):
    def __init__(self):
        self.Package = None
    
    def FromXml(self, Item, Key):
        # Create a package object
        Package = PackageClass()
        
        # Header
        Tmp = PackageHeaderXml()
        PackageHeader = Tmp.FromXml(XmlNode(Item, '/PackageSurfaceArea/Header'), 'Header')
        Package.PackageHeader = PackageHeader
        
        # ClonedFrom
        Tmp = ClonedFromXml()
        ClonedFrom = Tmp.FromXml(XmlNode(Item, '/PackageSurfaceArea/ClonedFrom'), 'ClonedFrom')
        if ClonedFrom:
            Package.PackageHeader.ClonedFrom.append(ClonedFrom)
        
        # LibraryClass
        for SubItem in XmlList(Item, '/PackageSurfaceArea/LibraryClassDeclarations/LibraryClass'):
            Tmp = LibraryClassXml()
            LibraryClass = Tmp.FromXml(SubItem, 'LibraryClass')
            Package.LibraryClassDeclarations.append(LibraryClass)
        
        # IndustryStandardHeader
        for SubItem in XmlList(Item, '/PackageSurfaceArea/IndustryStandardIncludes/IndustryStandardHeader'):
            Tmp = IndustryStandardHeaderXml()
            Include = Tmp.FromXml(SubItem, 'IndustryStandardHeader')
            Package.IndustryStdHeaders.append(Include)

        # PackageHeader
        for SubItem in XmlList(Item, '/PackageSurfaceArea/PackageIncludes/PackageHeader'):
            Tmp = PackageIncludeHeaderXml()
            Include = Tmp.FromXml(SubItem, 'PackageHeader')
            Package.PackageIncludePkgHeaders.append(Include)
        
        # Guid
        for SubItem in XmlList(Item, '/PackageSurfaceArea/GuidDeclarations/Entry'):
            Tmp = GuidProtocolPpiXml()
            GuidProtocolPpi = Tmp.FromXml(SubItem, 'Entry')
            Package.GuidDeclarations.append(GuidProtocolPpi)
    
        # Protocol
        for SubItem in XmlList(Item, '/PackageSurfaceArea/ProtocolDeclarations/Entry'):
            Tmp = GuidProtocolPpiXml()
            GuidProtocolPpi = Tmp.FromXml(SubItem, 'Entry')
            Package.ProtocolDeclarations.append(GuidProtocolPpi)

        # Ppi
        for SubItem in XmlList(Item, '/PackageSurfaceArea/PpiDeclarations/Entry'):
            Tmp = GuidProtocolPpiXml()
            GuidProtocolPpi = Tmp.FromXml(SubItem, 'Entry')
            Package.PpiDeclarations.append(GuidProtocolPpi)
        
        # PcdEntry
        for SubItem in XmlList(Item, '/PackageSurfaceArea/PcdDeclarations/PcdEntry'):
            Tmp = PcdEntryXml()
            PcdEntry = Tmp.FromXml(SubItem, 'PcdEntry')
            Package.PcdDeclarations.append(PcdEntry)
        
        # PcdCheck
        for SubItem in XmlList(Item, '/PackageSurfaceArea/PcdRelationshipChecks/PcdCheck'):
            Tmp = PcdCheckXml()
            PcdCheck = Tmp.FromXml(SubItem, 'PcdCheck')
            Package.PcdChecks.append(PcdCheck)
        
        # MiscellaneousFile
        Tmp = MiscellaneousFileXml()
        Package.MiscFiles = Tmp.FromXml(XmlNode(Item, '/PackageSurfaceArea/MiscellaneousFiles'), 'MiscellaneousFiles')
       
        # UserExtensions
        Tmp = UserExtensionsXml()
        Package.UserExtensions = Tmp.FromXml(XmlNode(Item, '/PackageSurfaceArea/UserExtensions'), 'UserExtensions')
        
        # Modules
        for SubItem in XmlList(Item, '/PackageSurfaceArea/Modules/ModuleSurfaceArea'):
            Tmp = ModuleSurfaceAreaXml()
            Module = Tmp.FromXml(SubItem, 'ModuleSurfaceArea')
            Package.Modules[(Module.ModuleHeader.Guid, Module.ModuleHeader.Version, Module.ModuleHeader.CombinePath)] = Module
        
        self.Package = Package
        return self.Package

    def ToXml(self, Package):
        # Create PackageSurfaceArea node
        DomPackage = minidom.Document().createElement('PackageSurfaceArea')
        
        # Header
        Tmp = PackageHeaderXml()
        DomPackage.appendChild(Tmp.ToXml(Package.PackageHeader, 'Header'))
        
        # ClonedFrom
        Tmp = ClonedFromXml()
        if Package.PackageHeader.ClonedFrom != []:
            DomPackage.appendChild(Tmp.ToXml(Package.PackageHeader.ClonedFrom[0], 'ClonedFrom'))
        
        # LibraryClass
        LibraryClassNode = CreateXmlElement('LibraryClassDeclarations', '', [], [])
        for LibraryClass in Package.LibraryClassDeclarations:
            Tmp = LibraryClassXml()
            LibraryClassNode.appendChild(Tmp.ToXml(LibraryClass, 'LibraryClass'))
        DomPackage.appendChild(LibraryClassNode)
        
        # IndustryStandardHeader
        IndustryStandardHeaderNode = CreateXmlElement('IndustryStandardIncludes', '', [], [])
        for Include in Package.IndustryStdHeaders:
            Tmp = IndustryStandardHeaderXml()
            IndustryStandardHeaderNode.appendChild(Tmp.ToXml(Include, 'IndustryStandardHeader'))
        DomPackage.appendChild(IndustryStandardHeaderNode)
        
        # PackageHeader
        PackageIncludeHeaderNode = CreateXmlElement('PackageIncludes', '', [], [])
        for Include in Package.PackageIncludePkgHeaders:
            Tmp = PackageIncludeHeaderXml()
            PackageIncludeHeaderNode.appendChild(Tmp.ToXml(Include, 'PackageHeader'))
        DomPackage.appendChild(PackageIncludeHeaderNode)
        
        # Guid
        GuidProtocolPpiNode = CreateXmlElement('GuidDeclarations', '', [], [])
        for GuidProtocolPpi in Package.GuidDeclarations:
            Tmp = GuidProtocolPpiXml()
            GuidProtocolPpiNode.appendChild(Tmp.ToXml(GuidProtocolPpi, 'Entry'))
        DomPackage.appendChild(GuidProtocolPpiNode)
    
        # Protocol
        GuidProtocolPpiNode = CreateXmlElement('ProtocolDeclarations', '', [], [])
        for GuidProtocolPpi in Package.ProtocolDeclarations:
            Tmp = GuidProtocolPpiXml()
            GuidProtocolPpiNode.appendChild(Tmp.ToXml(GuidProtocolPpi, 'Entry'))
        DomPackage.appendChild(GuidProtocolPpiNode)

        # Ppi
        GuidProtocolPpiNode = CreateXmlElement('PpiDeclarations', '', [], [])
        for GuidProtocolPpi in Package.PpiDeclarations:
            Tmp = GuidProtocolPpiXml()
            GuidProtocolPpiNode.appendChild(Tmp.ToXml(GuidProtocolPpi, 'Entry'))
        DomPackage.appendChild(GuidProtocolPpiNode)
        
        # PcdEntry
        PcdEntryNode = CreateXmlElement('PcdDeclarations', '', [], [])
        for PcdEntry in Package.PcdDeclarations:
            Tmp = PcdEntryXml()
            PcdEntryNode.appendChild(Tmp.ToXml(PcdEntry, 'PcdEntry'))
        DomPackage.appendChild(PcdEntryNode)
        
        # PcdCheck
        PcdCheckNode = CreateXmlElement('PcdRelationshipChecks', '', [], [])
        for PcdCheck in Package.PcdChecks:
            Tmp = PcdCheckXml()
            PcdCheckNode.appendChild(Tmp.ToXml(PcdCheck, 'PcdCheck'))
        DomPackage.appendChild(PcdCheckNode)
        
        # MiscellaneousFile
        Tmp = MiscellaneousFileXml()
        DomPackage.appendChild(Tmp.ToXml(Package.MiscFiles, 'MiscellaneousFiles'))
       
        # UserExtensions
        Tmp = UserExtensionsXml()
        DomPackage.appendChild(Tmp.ToXml(Package.UserExtensions, 'UserExtensions'))
        
        # Modules
        ModuleNode = CreateXmlElement('Modules', '', [], [])
        for Module in Package.Modules.values():
            Tmp = ModuleSurfaceAreaXml()
            ModuleNode.appendChild(Tmp.ToXml(Module))
        DomPackage.appendChild(ModuleNode)
        
        return DomPackage

# ModuleXml
class ModuleSurfaceAreaXml(object):
    def __init__(self):
        self.Module = None
    
    def FromXml(self, Item, Key):
        # Create a package object
        Module = ModuleClass()
        
        # Header
        Tmp = HeaderXml()
        ModuleHeader = Tmp.FromXml(XmlNode(Item, '/ModuleSurfaceArea/Header'), 'Header')
        Module.ModuleHeader = ModuleHeader
        
        # ModuleProperties
        Tmp = ModulePropertyXml()
        (Header, BootModes, Events, HOBs) = Tmp.FromXml(XmlNode(Item, '/ModuleSurfaceArea/ModuleProperties'), 'ModuleProperties', ModuleHeader)
        Module.ModuleHeader = Header
        Module.BootModes = BootModes
        Module.Events = Events
        Module.Hobs = HOBs
        
        # ClonedFrom
        Tmp = ClonedFromXml()
        ClonedFrom = Tmp.FromXml(XmlNode(Item, '/ModuleSurfaceArea/ClonedFrom'), 'ClonedFrom')
        if ClonedFrom:
            Module.ModuleHeader.ClonedFrom.append(ClonedFrom)
        
        # LibraryClass
        #LibraryClassNode = CreateXmlElement('LibraryClassDefinitions', '', [], [])
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/LibraryClassDefinitions/LibraryClass'):
            Tmp = LibraryClassXml()
            LibraryClass = Tmp.FromXml(SubItem, 'LibraryClass')
            Module.LibraryClasses.append(LibraryClass)
        
        # SourceFile
        #SourceFileNode = CreateXmlElement('SourceFiles', '', [], [])
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/SourceFiles/Filename'):
            Tmp = SourceFileXml()
            SourceFile = Tmp.FromXml(SubItem, 'Filename')
            Module.Sources.append(SourceFile)
        
        # BinaryFile
        #BinaryFileNode = CreateXmlElement('BinaryFiles', '', [], [])
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/BinaryFiles/BinaryFile'):
            Tmp = BinaryFileXml()
            BinaryFile = Tmp.FromXml(SubItem, 'BinaryFile')
            Module.Binaries.append(BinaryFile)
        
        # PackageDependencies
        #PackageDependencyNode = CreateXmlElement('PackageDependencies', '', [], [])
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/PackageDependencies/Package'):
            Tmp = PackageXml()
            PackageDependency = Tmp.FromXml(SubItem, 'Package')
            Module.PackageDependencies.append(PackageDependency)

        # Guid
        #GuidProtocolPpiNode = CreateXmlElement('Guids', '', [], [])
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/Guids/GuidCName'):
            Tmp = GuidProtocolPpiXml()
            GuidProtocolPpi = Tmp.FromXml(SubItem, 'GuidCName')
            Module.Guids.append(GuidProtocolPpi)
    
        # Protocol
        #GuidProtocolPpiNode = CreateXmlElement('Protocols', '', [], [])
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/Protocols/Protocol'):
            Tmp = GuidProtocolPpiXml()
            GuidProtocolPpi = Tmp.FromXml(SubItem, 'Protocol')
            Module.Protocols.append(GuidProtocolPpi)

        # Ppi
        #GuidProtocolPpiNode = CreateXmlElement('PPIs', '', [], [])
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/PPIs/Ppi'):
            Tmp = GuidProtocolPpiXml()
            GuidProtocolPpi = Tmp.FromXml(SubItem, 'Ppi')
            Module.Ppis.append(GuidProtocolPpi)
        
        # Extern
        #ExternNode = CreateXmlElement('Externs', '', [], [])
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/Externs/Extern'):
            Tmp = ExternXml()
            Extern = Tmp.FromXml(SubItem, 'Extern')
            Module.Externs.append(Extern)
        
        # PcdCoded
        #PcdEntryNode = CreateXmlElement('PcdCoded', '', [], [])
        for SubItem in XmlList(Item, '/ModuleSurfaceArea/PcdCoded/PcdEntry'):
            Tmp = PcdEntryXml()
            PcdEntry = Tmp.FromXml(SubItem, 'PcdEntry')
            Module.PcdCodes.append(PcdEntry)

        # PeiDepex
        #DepexNode = CreateXmlElement('PeiDepex', '', [], [])
        Tmp = DepexXml()
        Module.PeiDepex = Tmp.FromXml(XmlNode(Item, '/ModuleSurfaceArea/PeiDepex'), 'PeiDepex')

        # DxeDepex
        #DepexNode = CreateXmlElement('DxeDepex', '', [], [])
        Tmp = DepexXml()
        Module.DxeDepex = Tmp.FromXml(XmlNode(Item, '/ModuleSurfaceArea/DxeDepex'), 'DxeDepex')
        
        # SmmDepex
        #DepexNode = CreateXmlElement('SmmDepex', '', [], [])
        Tmp = DepexXml()
        Module.SmmDepex = Tmp.FromXml(XmlNode(Item, '/ModuleSurfaceArea/DxeDepex'), 'SmmDepex')

        # MiscellaneousFile
        Tmp = MiscellaneousFileXml()
        Module.MiscFiles = Tmp.FromXml(XmlNode(Item, '/ModuleSurfaceArea/MiscellaneousFiles'), 'MiscellaneousFiles')
        
        # UserExtensions
        Tmp = UserExtensionsXml()
        Module.UserExtensions = Tmp.FromXml(XmlNode(Item, '/ModuleSurfaceArea/UserExtensions'), 'UserExtensions')
        
        # return the module object
        self.Module = Module
        return self.Module

    def ToXml(self, Module):
        # Create root node of module surface area
        DomModule = minidom.Document().createElement('ModuleSurfaceArea')

        # Header
        Tmp = HeaderXml()
        DomModule.appendChild(Tmp.ToXml(Module.ModuleHeader, 'Header'))
        
        # ModuleProperties
        Tmp = ModulePropertyXml()
        DomModule.appendChild(Tmp.ToXml(Module.ModuleHeader, Module.BootModes, Module.Events, Module.Hobs, 'ModuleProperties'))
        
        # ClonedFrom
        Tmp = ClonedFromXml()
        if Module.ModuleHeader.ClonedFrom != []:
            DomModule.appendChild(Tmp.ToXml(Module.ModuleHeader.ClonedFrom[0], 'ClonedFrom'))
        
        # LibraryClass
        LibraryClassNode = CreateXmlElement('LibraryClassDefinitions', '', [], [])
        for LibraryClass in Module.LibraryClasses:
            Tmp = LibraryClassXml()
            LibraryClassNode.appendChild(Tmp.ToXml(LibraryClass, 'LibraryClass'))
        DomModule.appendChild(LibraryClassNode)
        
        # SourceFile
        SourceFileNode = CreateXmlElement('SourceFiles', '', [], [])
        for SourceFile in Module.Sources:
            Tmp = SourceFileXml()
            SourceFileNode.appendChild(Tmp.ToXml(SourceFile, 'Filename'))
        DomModule.appendChild(SourceFileNode)
        
        # BinaryFile
        BinaryFileNode = CreateXmlElement('BinaryFiles', '', [], [])
        for BinaryFile in Module.Binaries:
            Tmp = BinaryFileXml()
            BinaryFileNode.appendChild(Tmp.ToXml(BinaryFile, 'BinaryFile'))
        DomModule.appendChild(BinaryFileNode)
        
        # PackageDependencies
        PackageDependencyNode = CreateXmlElement('PackageDependencies', '', [], [])
        for PackageDependency in Module.PackageDependencies:
            Tmp = PackageXml()
            PackageDependencyNode.appendChild(Tmp.ToXml(PackageDependency, 'Package'))
        DomModule.appendChild(PackageDependencyNode)

        # Guid
        GuidProtocolPpiNode = CreateXmlElement('Guids', '', [], [])
        for GuidProtocolPpi in Module.Guids:
            Tmp = GuidProtocolPpiXml()
            GuidProtocolPpiNode.appendChild(Tmp.ToXml(GuidProtocolPpi, 'GuidCName'))
        DomModule.appendChild(GuidProtocolPpiNode)
    
        # Protocol
        GuidProtocolPpiNode = CreateXmlElement('Protocols', '', [], [])
        for GuidProtocolPpi in Module.Protocols:
            Tmp = GuidProtocolPpiXml()
            GuidProtocolPpiNode.appendChild(Tmp.ToXml(GuidProtocolPpi, 'Protocol'))
        DomModule.appendChild(GuidProtocolPpiNode)

        # Ppi
        GuidProtocolPpiNode = CreateXmlElement('PPIs', '', [], [])
        for GuidProtocolPpi in Module.Ppis:
            Tmp = GuidProtocolPpiXml()
            GuidProtocolPpiNode.appendChild(Tmp.ToXml(GuidProtocolPpi, 'Ppi'))
        DomModule.appendChild(GuidProtocolPpiNode)
        
        # Extern
        ExternNode = CreateXmlElement('Externs', '', [], [])
        for Extern in Module.Externs:
            Tmp = ExternXml()
            ExternNode.appendChild(Tmp.ToXml(Extern, 'Extern'))
        DomModule.appendChild(ExternNode)
        
        # PcdCoded
        PcdEntryNode = CreateXmlElement('PcdCoded', '', [], [])
        for PcdEntry in Module.PcdCodes:
            Tmp = PcdEntryXml()
            PcdEntryNode.appendChild(Tmp.ToXml(PcdEntry, 'PcdEntry'))
        DomModule.appendChild(PcdEntryNode)

        # PeiDepex
        if Module.PeiDepex:
            DepexNode = CreateXmlElement('PeiDepex', '', [], [])
            Tmp = DepexXml()
            DomModule.appendChild(Tmp.ToXml(Module.PeiDepex, 'PeiDepex'))

        # DxeDepex
        if Module.DxeDepex:
            DepexNode = CreateXmlElement('DxeDepex', '', [], [])
            Tmp = DepexXml()
            DomModule.appendChild(Tmp.ToXml(Module.DxeDepex, 'DxeDepex'))
        
        # SmmDepex
        if Module.SmmDepex:
            DepexNode = CreateXmlElement('SmmDepex', '', [], [])
            Tmp = DepexXml()
            DomModule.appendChild(Tmp.ToXml(Module.SmmDepex, 'SmmDepex'))
        
        # MiscellaneousFile
        Tmp = MiscellaneousFileXml()
        DomModule.appendChild(Tmp.ToXml(Module.MiscFiles, 'MiscellaneousFiles'))
        
        # UserExtensions
        Tmp = UserExtensionsXml()
        DomModule.appendChild(Tmp.ToXml(Module.UserExtensions, 'UserExtensions'))
        
        return DomModule

# DistributionPackageXml
class DistributionPackageXml(object):
    def __init__(self):
        self.Dp = DistributionPackageClass()

    def FromXml(self, Filename = None):
        if Filename != None:
            self.Dp = DistributionPackageClass()
            
            # Load to XML
            self.Pkg = XmlParseFile(Filename)

            # Parse Header information
            Tmp = DistributionPackageHeaderXml()
            DistributionPackageHeader = Tmp.FromXml(XmlNode(self.Pkg, '/DistributionPackage/DistributionHeader'), 'DistributionHeader')
            self.Dp.Header = DistributionPackageHeader
            
            # Parse each PackageSurfaceArea
            for Item in XmlList(self.Pkg, '/DistributionPackage/PackageSurfaceArea'):
                Psa = PackageSurfaceAreaXml()
                Package = Psa.FromXml(Item, 'PackageSurfaceArea')
                self.Dp.PackageSurfaceArea[(Package.PackageHeader.Guid, Package.PackageHeader.Version, Package.PackageHeader.CombinePath)] = Package
            
            # Parse each ModuleSurfaceArea
            for Item in XmlList(self.Pkg, '/DistributionPackage/ModuleSurfaceArea'):
                Msa = ModuleSurfaceAreaXml()
                Module = Msa.FromXml(Item, 'ModuleSurfaceArea')
                self.Dp.ModuleSurfaceArea[(Module.ModuleHeader.Guid, Module.ModuleHeader.Version, Module.ModuleHeader.CombinePath)] = Module
                
            # Parse Tools
            Tmp = MiscellaneousFileXml()
            self.Dp.Tools = Tmp.FromXml2(XmlNode(self.Pkg, '/DistributionPackage/Tools'), 'Tools')
            
            # Parse MiscFiles
            Tmp = MiscellaneousFileXml()
            self.Dp.MiscellaneousFiles = Tmp.FromXml2(XmlNode(self.Pkg, '/DistributionPackage/MiscellaneousFiles'), 'MiscellaneousFiles')
            
            return self.Dp
            
    def ToXml(self, Dp):
        if Dp != None:
            # Parse DistributionPackageHeader
            Attrs = [['xmlns', 'http://www.uefi.org/2008/2.1'], 
                     ['xmlns:xsi', 'http:/www.w3.org/2001/XMLSchema-instance'],
                     ]
            Root = CreateXmlElement('DistributionPackage', '', [], Attrs)
            
            Tmp = DistributionPackageHeaderXml()
            Root.appendChild(Tmp.ToXml(Dp.Header, 'DistributionHeader'))
            
            # Parse each PackageSurfaceArea
            for Package in Dp.PackageSurfaceArea.values():
                Psa = PackageSurfaceAreaXml()
                DomPackage = Psa.ToXml(Package)
                Root.appendChild(DomPackage)
            
            # Parse each ModuleSurfaceArea
            for Module in Dp.ModuleSurfaceArea.values():
                Msa = ModuleSurfaceAreaXml()
                DomModule = Msa.ToXml(Module)
                Root.appendChild(DomModule)
                
            # Parse Tools
            Tmp = MiscellaneousFileXml()
            #Tools = Tmp.FromXml2(XmlNode(self.Pkg, '/DistributionPackage/Tools'), 'Tools')
            Root.appendChild(Tmp.ToXml2(Dp.Tools, 'Tools'))
            
            # Parse MiscFiles
            Tmp = MiscellaneousFileXml()
            #Tools = Tmp.FromXml2(XmlNode(self.Pkg, '/DistributionPackage/MiscellaneousFiles'), 'MiscellaneousFiles')
            Root.appendChild(Tmp.ToXml2(Dp.MiscellaneousFiles, 'MiscellaneousFiles'))
            
            return Root.toprettyxml(indent = '  ')
        
        return ''

if __name__ == '__main__':
    M = DistributionPackageXml()
    M.FromXml('C:\Test.xml')
    print M.ToXml(M.Dp)
    