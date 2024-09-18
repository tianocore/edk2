## @file
# This file is used to parse a xml file of .PKG file
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
XmlParser
'''

##
# Import Modules
#
import re

from Library.Xml.XmlRoutines import XmlNode
from Library.Xml.XmlRoutines import CreateXmlElement
from Library.Xml.XmlRoutines import XmlList
from Library.Xml.XmlRoutines import XmlParseFile
from Core.DistributionPackageClass import DistributionPackageClass
from Object.POM.ModuleObject import DepexObject
from Library.ParserValidate import IsValidInfMoudleType
from Library.ParserValidate import IsValidInstallPath
from Library.Misc import IsEqualList
from Library.Misc import Sdict

from Logger.StringTable import ERR_XML_INVALID_VARIABLENAME
from Logger.StringTable import ERR_XML_INVALID_LIB_SUPMODLIST
from Logger.StringTable import ERR_XML_INVALID_EXTERN_SUPARCHLIST
from Logger.StringTable import ERR_XML_INVALID_EXTERN_SUPMODLIST
from Logger.StringTable import ERR_XML_INVALID_EXTERN_SUPMODLIST_NOT_LIB
from Logger.StringTable import ERR_FILE_NAME_INVALIDE
from Logger.ToolError import PARSER_ERROR
from Logger.ToolError import FORMAT_INVALID

from Xml.CommonXml import DistributionPackageHeaderXml
from Xml.CommonXml import MiscellaneousFileXml
from Xml.CommonXml import UserExtensionsXml
from Xml.XmlParserMisc import ConvertVariableName
from Xml.XmlParserMisc import IsRequiredItemListNull
from Xml.ModuleSurfaceAreaXml import ModuleSurfaceAreaXml
from Xml.PackageSurfaceAreaXml import PackageSurfaceAreaXml

import Logger.Log as Logger

##
# DistributionPackageXml
#
class DistributionPackageXml(object):
    def __init__(self):
        self.DistP = DistributionPackageClass()
        self.Pkg = ''

    ## ValidateDistributionPackage
    #
    # Check if any required item is missing in DistributionPackage
    #
    def ValidateDistributionPackage(self):
        XmlTreeLevel = ['DistributionPackage']
        if self.DistP:
            #
            # Check DistributionPackage -> DistributionHeader
            #
            XmlTreeLevel = ['DistributionPackage', '']
            CheckDict = {'DistributionHeader':self.DistP.Header }
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)

            if self.DistP.Header:
                DpHeader = self.DistP.Header
                XmlTreeLevel = ['DistributionPackage', 'DistributionHeader']
                CheckDict = Sdict()
                if DpHeader.GetAbstract():
                    DPAbstract = DpHeader.GetAbstract()[0][1]
                else:
                    DPAbstract = ''
                if DpHeader.GetCopyright():
                    DPCopyright = DpHeader.GetCopyright()[0][1]
                else:
                    DPCopyright = ''
                if DpHeader.GetLicense():
                    DPLicense = DpHeader.GetLicense()[0][1]
                else:
                    DPLicense = ''

                CheckDict['Name'] = DpHeader.GetName()
                CheckDict['GUID'] = DpHeader.GetGuid()
                CheckDict['Version'] = DpHeader.GetVersion()
                CheckDict['Copyright'] = DPCopyright
                CheckDict['License'] = DPLicense
                CheckDict['Abstract'] = DPAbstract
                CheckDict['Vendor'] = DpHeader.GetVendor()
                CheckDict['Date'] = DpHeader.GetDate()
                CheckDict['XmlSpecification'] = DpHeader.GetXmlSpecification()

                IsRequiredItemListNull(CheckDict, XmlTreeLevel)
            else:
                XmlTreeLevel = ['DistributionPackage', 'DistributionHeader']
                CheckDict = CheckDict = {'DistributionHeader': '', }
                IsRequiredItemListNull(CheckDict, XmlTreeLevel)

            #
            # Check Each Package
            #
            for Key in self.DistP.PackageSurfaceArea:
                ValidatePackageSurfaceArea(self.DistP.PackageSurfaceArea[Key])

            #
            # Check Each Module
            #
            for Key in self.DistP.ModuleSurfaceArea:
                ValidateMS(self.DistP.ModuleSurfaceArea[Key], ['DistributionPackage', 'ModuleSurfaceArea'])

            #
            # Check Each Tool
            #
            if self.DistP.Tools:
                XmlTreeLevel = ['DistributionPackage', 'Tools', 'Header']
                CheckDict = {'Name': self.DistP.Tools.GetName(), }
                IsRequiredItemListNull(CheckDict, XmlTreeLevel)

                if not self.DistP.Tools.GetFileList():
                    XmlTreeLevel = ['DistributionPackage', 'Tools']
                    CheckDict = {'FileName': None, }
                    IsRequiredItemListNull(CheckDict, XmlTreeLevel)
                for Item in self.DistP.Tools.GetFileList():
                    XmlTreeLevel = ['DistributionPackage', 'Tools']
                    CheckDict = {'FileName': Item.GetURI(), }
                    IsRequiredItemListNull(CheckDict, XmlTreeLevel)

            #
            # Check Each Misc File
            #
            if self.DistP.MiscellaneousFiles:
                XmlTreeLevel = ['DistributionPackage', 'MiscellaneousFiles', 'Header']
                CheckDict = {'Name': self.DistP.MiscellaneousFiles.GetName(), }
                IsRequiredItemListNull(CheckDict, XmlTreeLevel)

                if not self.DistP.MiscellaneousFiles.GetFileList():
                    XmlTreeLevel = ['DistributionPackage', 'MiscellaneousFiles']
                    CheckDict = {'FileName': None, }
                    IsRequiredItemListNull(CheckDict, XmlTreeLevel)
                for Item in self.DistP.MiscellaneousFiles.GetFileList():
                    XmlTreeLevel = ['DistributionPackage', 'MiscellaneousFiles']
                    CheckDict = {'FileName': Item.GetURI(), }
                    IsRequiredItemListNull(CheckDict, XmlTreeLevel)

            #
            # Check Each Distribution Level User Extension
            #
            for Item in self.DistP.UserExtensions:
                XmlTreeLevel = ['DistributionPackage', 'UserExtensions']
                CheckDict = {'UserId': Item.GetUserID(), }
                IsRequiredItemListNull(CheckDict, XmlTreeLevel)


    def FromXml(self, Filename=None):
        if Filename is not None:
            self.DistP = DistributionPackageClass()
            #
            # Load to XML
            #
            self.Pkg = XmlParseFile(Filename)

            #
            # Parse Header information
            #
            Tmp = DistributionPackageHeaderXml()
            DistributionPackageHeader = \
            Tmp.FromXml(XmlNode(self.Pkg, '/DistributionPackage/DistributionHeader'), 'DistributionHeader')
            self.DistP.Header = DistributionPackageHeader
            #
            # Parse each PackageSurfaceArea
            #
            for Item in XmlList(self.Pkg, '/DistributionPackage/PackageSurfaceArea'):
                Psa = PackageSurfaceAreaXml()
                Package = Psa.FromXml(Item, 'PackageSurfaceArea')
                self.DistP.PackageSurfaceArea[(Package.GetGuid(), \
                                               Package.GetVersion(), \
                                               Package.GetPackagePath())] = \
                                               Package
            #
            # Parse each ModuleSurfaceArea
            #
            for Item in XmlList(self.Pkg, '/DistributionPackage/ModuleSurfaceArea'):
                Msa = ModuleSurfaceAreaXml()
                Module = Msa.FromXml(Item, 'ModuleSurfaceArea', True)
                ModuleKey = (Module.GetGuid(), Module.GetVersion(), Module.GetName(), Module.GetModulePath())
                self.DistP.ModuleSurfaceArea[ModuleKey] = Module

            #
            # Parse Tools
            #
            Tmp = MiscellaneousFileXml()
            self.DistP.Tools = Tmp.FromXml2(XmlNode(self.Pkg, '/DistributionPackage/Tools'), 'Tools')

            #
            # Parse MiscFiles
            #
            Tmp = MiscellaneousFileXml()
            self.DistP.MiscellaneousFiles = \
            Tmp.FromXml2(XmlNode(self.Pkg, \
                                 '/DistributionPackage/MiscellaneousFiles'), \
                                 'MiscellaneousFiles')

            #
            # Parse UserExtensions
            #
            for Item in XmlList(self.Pkg, '/DistributionPackage/UserExtensions'):
                Tmp = UserExtensionsXml()
                self.DistP.UserExtensions.append(Tmp.FromXml2(Item, 'UserExtensions'))

            #
            # Check Required Items for XML
            #
            self.ValidateDistributionPackage()

            return self.DistP

    def ToXml(self, DistP):
        if self.DistP:
            pass
        if DistP is not None:
            #
            # Parse DistributionPackageHeader
            #
            Attrs = [['xmlns', 'http://www.uefi.org/2011/1.1'],
                     ['xmlns:xsi', 'http:/www.w3.org/2001/XMLSchema-instance'],
                     ]
            Root = CreateXmlElement('DistributionPackage', '', [], Attrs)

            Tmp = DistributionPackageHeaderXml()
            Root.appendChild(Tmp.ToXml(DistP.Header, 'DistributionHeader'))
            #
            # Parse each PackageSurfaceArea
            #
            for Package in DistP.PackageSurfaceArea.values():
                Psa = PackageSurfaceAreaXml()
                DomPackage = Psa.ToXml(Package)
                Root.appendChild(DomPackage)
            #
            # Parse each ModuleSurfaceArea
            #
            for Module in DistP.ModuleSurfaceArea.values():
                Msa = ModuleSurfaceAreaXml()
                DomModule = Msa.ToXml(Module)
                Root.appendChild(DomModule)
            #
            # Parse Tools
            #
            Tmp = MiscellaneousFileXml()
            ToolNode = Tmp.ToXml2(DistP.Tools, 'Tools')
            if ToolNode is not None:
                Root.appendChild(ToolNode)
            #
            # Parse MiscFiles
            #
            Tmp = MiscellaneousFileXml()
            MiscFileNode = Tmp.ToXml2(DistP.MiscellaneousFiles,
                                      'MiscellaneousFiles')
            if MiscFileNode is not None:
                Root.appendChild(MiscFileNode)

            XmlContent = Root.toprettyxml(indent='  ')


            #
            # Remove empty element
            #
            XmlContent = re.sub(r'[\s\r\n]*<[^<>=]*/>', '', XmlContent)

            #
            # Remove empty help text element
            #
            XmlContent = re.sub(r'[\s\r\n]*<HelpText Lang="en-US"/>', '',
                                XmlContent)

            #
            # Remove SupArchList="COMMON" or "common"
            #
            XmlContent = \
            re.sub(r'[\s\r\n]*SupArchList[\s\r\n]*=[\s\r\n]*"[\s\r\n]*COMMON'
            r'[\s\r\n]*"', '', XmlContent)
            XmlContent = \
            re.sub(r'[\s\r\n]*SupArchList[\s\r\n]*=[\s\r\n]*"[\s\r\n]*common'
            r'[\s\r\n]*"', '', XmlContent)
            #
            # Remove <SupArchList> COMMON </SupArchList>
            #
            XmlContent = \
            re.sub(r'[\s\r\n]*<SupArchList>[\s\r\n]*COMMON[\s\r\n]*'
            r'</SupArchList>[\s\r\n]*', '', XmlContent)

            #
            # Remove <SupArchList> common </SupArchList>
            #
            XmlContent = \
            re.sub(r'[\s\r\n]*<SupArchList>[\s\r\n]*'
            r'common[\s\r\n]*</SupArchList>[\s\r\n]*', '', XmlContent)

            #
            # Remove SupModList="COMMON" or "common"
            #
            XmlContent = \
            re.sub(r'[\s\r\n]*SupModList[\s\r\n]*=[\s\r\n]*"[\s\r\n]*COMMON'
            r'[\s\r\n]*"', '', XmlContent)
            XmlContent = \
            re.sub(r'[\s\r\n]*SupModList[\s\r\n]*=[\s\r\n]*"[\s\r\n]*common'
            r'[\s\r\n]*"', '', XmlContent)

            return XmlContent

        return ''

## ValidateMS
#
# Check if any required item is missing in ModuleSurfaceArea
#
# @param Module: The ModuleSurfaceArea to be checked
# @param XmlTreeLevel: The top level of Module
#
def ValidateMS(Module, TopXmlTreeLevel):
    ValidateMS1(Module, TopXmlTreeLevel)
    ValidateMS2(Module, TopXmlTreeLevel)
    ValidateMS3(Module, TopXmlTreeLevel)

## ValidateMS1
#
# Check if any required item is missing in ModuleSurfaceArea
#
# @param Module: The ModuleSurfaceArea to be checked
# @param XmlTreeLevel: The top level of Module
#
def ValidateMS1(Module, TopXmlTreeLevel):
    #
    # Check Guids -> GuidCName
    #
    XmlTreeLevel = TopXmlTreeLevel + ['Guids']
    for Item in Module.GetGuidList():
        if Item is None:
            CheckDict = {'GuidCName':''}
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    XmlTreeLevel = TopXmlTreeLevel + ['Guids', 'GuidCName']
    for Item in Module.GetGuidList():
        CheckDict = {'CName':Item.GetCName(),
                     'GuidType':Item.GetGuidTypeList(),
                     'Usage':Item.GetUsage()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

        if Item.GetVariableName():
            Result = ConvertVariableName(Item.GetVariableName())
            if Result is None:
                Msg = "->".join(Node for Node in XmlTreeLevel)
                ErrorMsg = ERR_XML_INVALID_VARIABLENAME % (Item.GetVariableName(), Item.GetCName(), Msg)
                Logger.Error('\nUPT', PARSER_ERROR, ErrorMsg, RaiseError=True)
            else:
                Item.SetVariableName(Result)

    #
    # Check Protocols -> Protocol
    #
    XmlTreeLevel = TopXmlTreeLevel + ['Protocols']
    for Item in Module.GetProtocolList():
        if Item is None:
            CheckDict = {'Protocol':''}
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    XmlTreeLevel = TopXmlTreeLevel + ['Protocols', 'Protocol']
    for Item in Module.GetProtocolList():
        CheckDict = {'CName':Item.GetCName(),
                     'Usage':Item.GetUsage()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check PPIs -> Ppi
    #
    XmlTreeLevel = TopXmlTreeLevel + ['PPIs']
    for Item in Module.GetPpiList():
        if Item is None:
            CheckDict = {'Ppi':''}
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    XmlTreeLevel = TopXmlTreeLevel + ['PPIs', 'Ppi']
    for Item in Module.GetPpiList():
        CheckDict = {'CName':Item.GetCName(),
                     'Usage':Item.GetUsage()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check PcdCoded -> Entry
    #
    XmlTreeLevel = TopXmlTreeLevel + ['PcdCoded']
    for Item in Module.GetPcdList():
        if Item is None:
            CheckDict = {'PcdEntry':''}
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    XmlTreeLevel = TopXmlTreeLevel + ['PcdCoded', 'PcdEntry']
    for Item in Module.GetPcdList():
        CheckDict = {'TokenSpaceGuidCname':Item.GetTokenSpaceGuidCName(),
                     'CName':Item.GetCName(),
                     'PcdUsage':Item.GetValidUsage(),
                     'PcdItemType':Item.GetItemType()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check Externs -> Extern
    #
    XmlTreeLevel = TopXmlTreeLevel + ['Externs']
    for Item in Module.GetExternList():
        if Item is None:
            CheckDict = {'Extern':''}
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # If SupArchList is used to identify different EntryPoint, UnloadImage, Constructor/Destructor elements and
    # that SupArchList does not match ModuleSurfaceArea.ModuleProperties:SupArchList, the tool must exit gracefully,
    # informing the user that the EDK II Build system does not support different EntryPoint, UnloadImage,
    # Constructor or Destructor elements based on Architecture type.  Two SupArchList attributes are considered
    # identical if it lists the same CPU architectures in any order.
    #
    for Item in Module.GetExternList():
        if len(Item.SupArchList) > 0:
            if not IsEqualList(Item.SupArchList, Module.SupArchList):
                Logger.Error('\nUPT',
                             PARSER_ERROR,
                             ERR_XML_INVALID_EXTERN_SUPARCHLIST % (str(Item.SupArchList), str(Module.SupArchList)),
                             RaiseError=True)

    #
    # Check DistributionPackage -> ModuleSurfaceArea -> UserExtensions
    #
    XmlTreeLevel = TopXmlTreeLevel + ['UserExtensions']
    for Item in Module.GetUserExtensionList():
        CheckDict = {'UserId':Item.GetUserID(), 'Identifier':Item.GetIdentifier()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check DistributionPackage -> PackageSurfaceArea -> MiscellaneousFiles -> Filename
    #
    XmlTreeLevel = TopXmlTreeLevel + ['MiscellaneousFiles']
    for Item in Module.GetMiscFileList():
        if not Item.GetFileList():
            CheckDict = {'Filename': '', }
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)
        for File in Item.GetFileList():
            CheckDict = {'Filename': File.GetURI(), }

## ValidateMS2
#
# Check if any required item is missing in ModuleSurfaceArea
#
# @param Module: The ModuleSurfaceArea to be checked
# @param XmlTreeLevel: The top level of Module
#
def ValidateMS2(Module, TopXmlTreeLevel):
    #
    # Check Header
    #
    XmlTreeLevel = TopXmlTreeLevel + ['Header']
    CheckDict = Sdict()
    CheckDict['Name'] = Module.GetName()
    CheckDict['BaseName'] = Module.GetBaseName()
    CheckDict['GUID'] = Module.GetGuid()
    CheckDict['Version'] = Module.GetVersion()
    IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check ModuleProperties
    #
    XmlTreeLevel = TopXmlTreeLevel + ['ModuleProperties']
    CheckDict = {'ModuleType':Module.GetModuleType(),
                 'Path':Module.GetModulePath()}
    IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    if not IsValidInstallPath(Module.GetModulePath()):
        Logger.Error("UPT", FORMAT_INVALID, ERR_FILE_NAME_INVALIDE % Module.GetModulePath())

    #
    # Check ModuleProperties->BootMode
    #
    XmlTreeLevel = TopXmlTreeLevel + ['ModuleProperties'] + ['BootMode']
    for Item in Module.GetBootModeList():
        CheckDict = {'Usage':Item.GetUsage(),
                     'SupportedBootModes':Item.GetSupportedBootModes()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check ModuleProperties->Event
    #
    XmlTreeLevel = TopXmlTreeLevel + ['ModuleProperties'] + ['Event']
    for Item in Module.GetEventList():
        CheckDict = {'Usage':Item.GetUsage(),
                     'EventType':Item.GetEventType()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check ModuleProperties->Hob
    #
    XmlTreeLevel = TopXmlTreeLevel + ['ModuleProperties'] + ['HOB']
    for Item in Module.GetHobList():
        CheckDict = {'Usage':Item.GetUsage(),
                     'HobType':Item.GetHobType()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # The UDP Specification supports the module type of UEFI_RUNTIME_DRIVER, which is not present in the EDK II INF
    # File Specification v. 1.23, so UPT must perform the following translation that include the generation of a
    # [Depex] section.
    #
    if Module.ModuleType == "UEFI_RUNTIME_DRIVER":
        Module.ModuleType = "DXE_RUNTIME_DRIVER"
        DxeObj = DepexObject()
        DxeObj.SetDepex("gEfiBdsArchProtocolGuid AND \ngEfiCpuArchProtocolGuid AND\n" + \
                        "gEfiMetronomeArchProtocolGuid AND \ngEfiMonotonicCounterArchProtocolGuid AND\n" + \
                        "gEfiRealTimeClockArchProtocolGuid AND \ngEfiResetArchProtocolGuid AND\n" + \
                        "gEfiRuntimeArchProtocolGuid AND \ngEfiSecurityArchProtocolGuid AND\n" + \
                        "gEfiTimerArchProtocolGuid AND \ngEfiVariableWriteArchProtocolGuid AND\n" + \
                        "gEfiVariableArchProtocolGuid AND \ngEfiWatchdogTimerArchProtocolGuid")
        DxeObj.SetModuleType(['DXE_RUNTIME_DRIVER'])
        Module.PeiDepex = []
        Module.DxeDepex = []
        Module.SmmDepex = []
        Module.DxeDepex.append(DxeObj)

    #
    # Check LibraryClassDefinitions -> LibraryClass
    #
    XmlTreeLevel = TopXmlTreeLevel + ['LibraryClassDefinitions']
    for Item in Module.GetLibraryClassList():
        if Item is None:
            CheckDict = {'LibraryClass':''}
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    XmlTreeLevel = TopXmlTreeLevel + ['LibraryClassDefinitions', 'LibraryClass']

    IsLibraryModule = False
    LibrarySupModList = []
    for Item in Module.GetLibraryClassList():
        CheckDict = {'Keyword':Item.GetLibraryClass(),
                     'Usage':Item.GetUsage()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)
        #
        # If the LibraryClass:SupModList is not "UNDEFINED" the LIBRARY_CLASS entry must have the list
        # appended using the format:
        # LIBRARY_CLASS = <ClassName> ["|" <Edk2ModuleTypeList>]
        #
        # Edk2ModuleTypeList ::= <ModuleType> [" " <ModuleType>]{0,}
        # <ModuleTypes>      ::= {"BASE"} {"SEC"} {"PEI_CORE"} {"PEIM"}
        #                       {"DXE_CORE"} {"DXE_DRIVER"} {"SMM_CORE"}
        #                       {"DXE_SMM_DRIVER"} {"DXE_RUNTIME_DRIVER"}
        #                       {"DXE_SAL_DRIVER"} {"UEFI_DRIVER"}
        #                       {"UEFI_APPLICATION"} {"USER_DEFINED"}
        #
        if len(Item.SupModuleList) > 0:
            for SupModule in Item.SupModuleList:
                if not IsValidInfMoudleType(SupModule):
                    Logger.Error('\nUPT',
                                 PARSER_ERROR,
                                 ERR_XML_INVALID_LIB_SUPMODLIST % (Item.LibraryClass, str(SupModule)),
                                 RaiseError=True)

        if Item.Usage == 'PRODUCES' or Item.Usage == 'SOMETIMES_PRODUCES':
            IsLibraryModule = True
            LibrarySupModList = Item.SupModuleList


    #
    # For Library modules (indicated by a LIBRARY_CLASS statement in the [Defines] section)
    # If the SupModList attribute of the CONSTRUCTOR or DESTRUCTOR element does not match the Supported Module
    # Types listed after "LIBRARY_CLASS = <Keyword> |", the tool should gracefully exit with an error message
    # stating that there is a conflict in the module types the CONSTRUCTOR/DESTRUCTOR is to be used with and
    # the Module types this Library supports.
    #
    if IsLibraryModule:
        for Item in Module.GetExternList():
            if Item.Constructor or Item.Destructor:
                if hasattr(Item, 'SupModList') and len(Item.SupModList) > 0 and \
                   not IsEqualList(Item.SupModList, LibrarySupModList):
                    Logger.Error('\nUPT',
                         PARSER_ERROR,
                         ERR_XML_INVALID_EXTERN_SUPMODLIST % (str(Item.SupModList), str(LibrarySupModList)),
                         RaiseError=True)

    #
    # If the module is not a library module, the MODULE_TYPE listed in the ModuleSurfaceArea.Header must match the
    # SupModList attribute.  If these conditions cannot be met, the tool must exit gracefully, informing the user
    # that the EDK II Build system does not currently support the features required by this Module.
    #
    if not IsLibraryModule:
        for Item in Module.GetExternList():
            if hasattr(Item, 'SupModList') and len(Item.SupModList) > 0 and \
               not IsEqualList(Item.SupModList, [Module.ModuleType]):
                Logger.Error('\nUPT',
                     PARSER_ERROR,
                     ERR_XML_INVALID_EXTERN_SUPMODLIST_NOT_LIB % (str(Module.ModuleType), str(Item.SupModList)),
                     RaiseError=True)
    #
    # Check SourceFiles
    #
    XmlTreeLevel = TopXmlTreeLevel + ['SourceFiles']
    for Item in Module.GetSourceFileList():
        if Item is None:
            CheckDict = {'Filename':''}
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    XmlTreeLevel = TopXmlTreeLevel + ['SourceFiles']
    for Item in Module.GetSourceFileList():
        CheckDict = {'Filename':Item.GetSourceFile()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    for ItemCount in range(len(Module.GetBinaryFileList())):
        Item = Module.GetBinaryFileList()[ItemCount]
        if Item and len(Item.FileNamList) > 0 and Item.FileNamList[0].FileType == 'FREEFORM':
            Item.FileNamList[0].FileType = 'SUBTYPE_GUID'
            Module.GetBinaryFileList()[ItemCount] = Item

## ValidateMS3
#
# Check if any required item is missing in ModuleSurfaceArea
#
# @param Module: The ModuleSurfaceArea to be checked
# @param XmlTreeLevel: The top level of Module
#
def ValidateMS3(Module, TopXmlTreeLevel):
    #
    # Check PackageDependencies -> Package
    #
    XmlTreeLevel = TopXmlTreeLevel + ['PackageDependencies']
    for Item in Module.GetPackageDependencyList():
        if Item is None:
            CheckDict = {'Package':''}
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    XmlTreeLevel = TopXmlTreeLevel + ['PackageDependencies', 'Package']
    for Item in Module.GetPackageDependencyList():
        CheckDict = {'GUID':Item.GetGuid()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check BinaryFiles -> BinaryFile
    #
    for Item in Module.GetBinaryFileList():
        if Item is None:
            XmlTreeLevel = TopXmlTreeLevel + ['BinaryFiles']
            CheckDict = {'BinaryFile':''}
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)
        if not Item.GetFileNameList():
            XmlTreeLevel = TopXmlTreeLevel + ['BinaryFiles', 'BinaryFile']
            CheckDict = {'Filename':''}
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)

        XmlTreeLevel = TopXmlTreeLevel + ['BinaryFiles', 'BinaryFile']
        for File in Item.GetFileNameList():
            CheckDict = {'Filename':File.GetFilename(),
                         'FileType':File.GetFileType()}
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)
        for AsBuilt in Item.GetAsBuiltList():
            #
            # Check LibInstance
            #
            if len(AsBuilt.LibraryInstancesList) == 1 and not AsBuilt.LibraryInstancesList[0]:
                CheckDict = {'GUID':''}
                XmlTreeLevel = TopXmlTreeLevel + ['BinaryFiles', 'BinaryFile', 'AsBuilt', 'LibraryInstances']
                IsRequiredItemListNull(CheckDict, XmlTreeLevel)

            for LibItem in AsBuilt.LibraryInstancesList:
                CheckDict = {'Guid':LibItem.Guid,
                             'Version':LibItem.Version}
                XmlTreeLevel = TopXmlTreeLevel + ['BinaryFiles', 'BinaryFile', 'AsBuilt', 'LibraryInstances']
                IsRequiredItemListNull(CheckDict, XmlTreeLevel)

            #
            # Check PatchPcd
            #
            for PatchPcdItem in AsBuilt.PatchPcdList:
                CheckDict = {'TokenSpaceGuidValue':PatchPcdItem.TokenSpaceGuidValue,
                             'PcdCName':PatchPcdItem.PcdCName,
                             'Token':PatchPcdItem.Token,
                             'DatumType':PatchPcdItem.DatumType,
                             'Value':PatchPcdItem.DefaultValue,
                             'Offset':PatchPcdItem.Offset}
                XmlTreeLevel = TopXmlTreeLevel + ['BinaryFiles', 'BinaryFile', 'AsBuilt', 'PatchPcdValue']
                IsRequiredItemListNull(CheckDict, XmlTreeLevel)
                #
                # Check PcdError
                #
                for PcdErrorItem in PatchPcdItem.PcdErrorsList:
                    CheckDict = {'ErrorNumber':PcdErrorItem.ErrorNumber}
                    XmlTreeLevel = TopXmlTreeLevel + ['BinaryFiles', 'BinaryFile', 'AsBuilt',
                                                      'PatchPcdValue', 'PcdError']
                    IsRequiredItemListNull(CheckDict, XmlTreeLevel)
            #
            # Check PcdEx
            #
            for PcdExItem in AsBuilt.PcdExValueList:
                CheckDict = {'TokenSpaceGuidValue':PcdExItem.TokenSpaceGuidValue,
                             'Token':PcdExItem.Token,
                             'DatumType':PcdExItem.DatumType}
                XmlTreeLevel = TopXmlTreeLevel + ['BinaryFiles', 'BinaryFile', 'AsBuilt', 'PcdExValue']
                IsRequiredItemListNull(CheckDict, XmlTreeLevel)
                #
                # Check PcdError
                #
                for PcdErrorItem in PcdExItem.PcdErrorsList:
                    CheckDict = {'ErrorNumber':PcdErrorItem.ErrorNumber}
                    XmlTreeLevel = TopXmlTreeLevel + ['BinaryFiles', 'BinaryFile', 'AsBuilt',
                                                      'PcdExValue', 'PcdError']
                    IsRequiredItemListNull(CheckDict, XmlTreeLevel)
    #
    # Check SmmDepex
    #
    XmlTreeLevel = TopXmlTreeLevel + ['SmmDepex']
    for Item in Module.GetSmmDepex():
        CheckDict = {'Expression':Item.GetDepex()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check PeiDepex
    #
    XmlTreeLevel = TopXmlTreeLevel + ['PeiDepex']
    for Item in Module.GetPeiDepex():
        CheckDict = {'Expression':Item.GetDepex()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check DxeDepex
    #
    XmlTreeLevel = TopXmlTreeLevel + ['DxeDepex']
    for Item in Module.GetDxeDepex():
        CheckDict = {'Expression':Item.GetDepex()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check <UserExtensions>
    #
    XmlTreeLevel = TopXmlTreeLevel + ['UserExtensions']
    for Item in Module.GetUserExtensionList():
        CheckDict = {'UserId':Item.GetUserID(), 'Identifier':Item.GetIdentifier()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

## ValidatePS1
#
# ValidatePS1
#
def ValidatePS1(Package):
    #
    # Check DistributionPackage -> PackageSurfaceArea -> Header
    #
    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'Header']
    CheckDict = Sdict()
    CheckDict['Name'] = Package.GetName()
    CheckDict['BaseName'] = Package.GetBaseName()
    CheckDict['GUID'] = Package.GetGuid()
    CheckDict['Version'] = Package.GetVersion()
    CheckDict['PackagePath'] = Package.GetPackagePath()

    IsRequiredItemListNull(CheckDict, XmlTreeLevel)
    if not IsValidInstallPath(Package.GetPackagePath()):
        Logger.Error("UPT", FORMAT_INVALID, ERR_FILE_NAME_INVALIDE % Package.GetPackagePath())

    #
    # Check DistributionPackage -> PackageSurfaceArea -> ClonedFrom
    #
    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'ClonedFrom']
    for Item in Package.GetClonedFromList():
        if Item is None:
            CheckDict = Sdict()
            CheckDict['GUID'] = ''
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)
        CheckDict = Sdict()
        CheckDict['GUID'] = Item.GetPackageGuid()
        CheckDict['Version'] = Item.GetPackageVersion()

        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check DistributionPackage -> PackageSurfaceArea -> LibraryClassDeclarations -> LibraryClass
    #
    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'LibraryClassDeclarations']
    for Item in Package.GetLibraryClassList():
        if Item is None:
            CheckDict = {'LibraryClass':''}
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'LibraryClassDeclarations', 'LibraryClass']
    for Item in Package.GetLibraryClassList():
        CheckDict = {'Keyword':Item.GetLibraryClass(),
                     'HeaderFile':Item.GetIncludeHeader()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check DistributionPackage -> PackageSurfaceArea -> IndustryStandardIncludes -> IndustryStandardHeader
    #
    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'IndustryStandardIncludes']
    for Item in Package.GetStandardIncludeFileList():
        if Item is None:
            CheckDict = {'IndustryStandardHeader':''}
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'IndustryStandardIncludes', 'IndustryStandardHeader']
    for Item in Package.GetStandardIncludeFileList():
        CheckDict = {'HeaderFile':Item.GetFilePath()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check DistributionPackage -> PackageSurfaceArea -> PackageIncludes -> PackageHeader
    #
    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'PackageIncludes']
    for Item in Package.GetPackageIncludeFileList():
        if Item is None:
            CheckDict = {'PackageHeader':''}
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'PackageIncludes', 'PackageHeader']
    for Item in Package.GetPackageIncludeFileList():
        CheckDict = {'HeaderFile':Item.GetFilePath()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

## ValidatePS2
#
# ValidatePS2
#
def ValidatePS2(Package):
    #
    # Check DistributionPackage -> PackageSurfaceArea -> Modules -> ModuleSurfaceArea
    #
    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'Modules', 'ModuleSurfaceArea']
    for Item in Package.GetModuleDict().values():
        ValidateMS(Item, XmlTreeLevel)

    #
    # Check DistributionPackage -> PackageSurfaceArea -> GuidDeclarations Entry
    #
    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'GuidDeclarations']
    for Item in Package.GetGuidList():
        if Item is None:
            CheckDict = {'Entry':''}
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'GuidDeclarations', 'Entry']
    for Item in Package.GetGuidList():
        CheckDict = {'CName':Item.GetCName(),
                     'GuidValue':Item.GetGuid()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check DistributionPackage -> PackageSurfaceArea -> ProtocolDeclarations -> Entry
    #
    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'ProtocolDeclarations']
    for Item in Package.GetProtocolList():
        if Item is None:
            CheckDict = {'Entry':''}
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'ProtocolDeclarations', 'Entry']
    for Item in Package.GetProtocolList():
        CheckDict = {'CName':Item.GetCName(),
                     'GuidValue':Item.GetGuid()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check DistributionPackage -> PackageSurfaceArea -> PpiDeclarations -> Entry
    #
    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'PpiDeclarations']
    for Item in Package.GetPpiList():
        if Item is None:
            CheckDict = {'Entry':''}
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'PpiDeclarations', 'Entry']
    for Item in Package.GetPpiList():
        CheckDict = {'CName':Item.GetCName(),
                     'GuidValue':Item.GetGuid()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check DistributionPackage -> PackageSurfaceArea -> PcdDeclarations -> Entry
    #
    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'PcdDeclarations']
    for Item in Package.GetPcdList():
        if Item is None:
            CheckDict = {'PcdEntry':''}
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'PcdDeclarations', 'PcdEntry']
    for Item in Package.GetPcdList():
        CheckDict = {'TokenSpaceGuidCname':Item.GetTokenSpaceGuidCName(),
                     'Token':Item.GetToken(),
                     'CName':Item.GetCName(),
                     'DatumType':Item.GetDatumType(),
                     'ValidUsage':Item.GetValidUsage(),
                     'DefaultValue':Item.GetDefaultValue()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check DistributionPackage -> PackageSurfaceArea -> UserExtensions
    #
    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'UserExtensions']
    for Item in Package.GetUserExtensionList():
        CheckDict = {'UserId':Item.GetUserID(), 'Identifier':Item.GetIdentifier()}
        IsRequiredItemListNull(CheckDict, XmlTreeLevel)

    #
    # Check DistributionPackage -> PackageSurfaceArea -> MiscellaneousFiles -> Filename
    #
    XmlTreeLevel = ['DistributionPackage', 'PackageSurfaceArea', 'MiscellaneousFiles']
    for Item in Package.GetMiscFileList():
        if not Item.GetFileList():
            CheckDict = {'Filename': '', }
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)
        for File in Item.GetFileList():
            CheckDict = {'Filename': File.GetURI(), }
            IsRequiredItemListNull(CheckDict, XmlTreeLevel)

## ValidatePackageSurfaceArea
#
# Check if any required item is missing in  PackageSurfaceArea
#
# @param Package: The PackageSurfaceArea to be checked
#
def ValidatePackageSurfaceArea(Package):
    ValidatePS1(Package)
    ValidatePS2(Package)
