#!/usr/bin/env python

# Copyright (c) 2007, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

"""Framework SurfaceArea Elemments"""
#
# TODO: FFS layout, Flash, FV, PCD
#
import os, sys, re, getopt, string, glob, xml.dom.minidom, pprint, time, copy, shelve, pickle
from XmlRoutines import *
import FrameworkElement
import BuildConfig

################################################################################
##
## Convert given list to a string in the format like: [a, b, c]
##
################################################################################
def ListString(lst):
    return "[%s]" % ",".join(lst)

class SurfaceAreaElement:
    """Base class for Surface Area XML element"""
    _ModuleTypes = ('BASE', 'SEC', 'PEI_CORE', 'PEIM', 'DXE_CORE', 'DXE_DRIVER',
                    'DXE_RUNTIME_DRIVER', 'DXE_SAL_DRIVER', 'DXE_SMM_DRIVER',
                    'TOOL', 'UEFI_DRIVER', 'UEFI_APPLICATION', 'USER_DEFINED')
    _GuidTypes = ('DATA_HUB_RECORD', 'EFI_EVENT', 'EFI_SYSTEM_CONFIGURATION_TABLE',
                  'EFI_VARIABLE', 'GUID', 'HII_PACKAGE_LIST', 'HOB', 'TOKEN_SPACE_GUID')
    _Archs = ('EBC', 'IA32', 'X64', 'IPF', 'ARM', 'PPC')
    _Usages = ('ALWAYS_CONSUMED', 'SOMETIMES_CONSUMED', 'ALWAYS_PRODUCED',
               'SOMETIMES_PRODUCED', 'TO_START', 'BY_START', 'PRIVATE')
    _FileTypes = {
                    ".c"    :   "CCode",
                    ".C"    :   "CCode",
                    ".cpp"  :   "CCode",
                    ".Cpp"  :   "CCode",
                    ".CPP"  :   "CCode",
                    ".h"    :   "CHeader",
                    ".H"    :   "CHeader",
                    ".asm"  :   "ASM",
                    ".Asm"  :   "Assembly",
                    ".ASM"  :   "Assembly",
                    ".s"    :   "IpfAssembly",
                    ".S"    :   "GccAssembly",
                    ".uni"  :   "UNI",
                    ".Uni"  :   "Unicode",
                    ".UNI"  :   "Unicode",
                    ".vfr"  :   "VFR",
                    ".Vfr"  :   "VFR",
                    ".VFR"  :   "VFR",
                    ".dxs"  :   "DPX",
                    ".Dxs"  :   "DPX",
                    ".DXS"  :   "DPX",
                    ".fv"   :   "FirmwareVolume",
                    ".Fv"   :   "FirmwareVolume",
                    ".FV"   :   "FirmwareVolume",
                    ".efi"  :   "EFI",
                    ".Efi"  :   "EFI",
                    ".EFI"  :   "EFI",
                    ".SEC"  :   "FFS",
                    ".PEI"  :   "FFS",
                    ".DXE"  :   "FFS",
                    ".APP"  :   "FFS",
                    ".FYI"  :   "FFS",
                    ".FFS"  :   "FFS",
                    ".bmp"  :   "BMP",
                    ".i"    :   "PPCode",
                    ".asl"  :   "ASL",
                    ".Asl"  :   "ASL",
                    ".ASL"  :   "ASL",
                 }
    _ToolMapping = {
                    "CCode"             :   "CC",
                    "CHeader"           :   "",
                    "ASM"               :   "ASM",
                    "Assembly"          :   "ASM",
                    "IpfAssembly"       :   "ASM",
                    "GccAssembly"       :   "ASM",
                    "UNI"               :   "",
                    "Unicode"           :   "",
                    "VFR"               :   "",
                    "DPX"               :   "",
                    "FirmwareVolume"    :   "",
                    "EFI"               :   "",
                    "FFS"               :   "",
                    "PPCode"            :   "PP",
                    "BMP"               :   "",
                   }

    _BuildableFileTypes = ("CCode", "ASM", "Assembly", "IpfAssembly", "GccAssembly", "UNI", "Unicode", "VFR", "DPX", "EFI")
    
    def __init__(self, workspace, owner=None, dom=None, parse=True, postprocess=True):
        self._Workspace = workspace
        
        if owner == None: self._Owner = ""
        else:             self._Owner = owner
            
        if dom == None: self._Root = ""
        else:           self._Root = dom
            
        self._Elements = {}
        
        if parse: self.Parse()
        if postprocess: self.Postprocess()
    
    def Parse(self):
        """Parse the XML element in DOM form"""
        pass
    
    def Postprocess(self):
        """Re-organize the original information form XML DOM into a format which can be used directly"""
        pass
    
    def GetArchList(self, dom):
        """Parse the SupArchList attribute. If not spcified, return all ARCH supported"""
        archs = XmlAttribute(dom, "SupArchList").split()
        if archs == []:
            if self._Owner.Archs != []:
                archs = self._Owner.Archs
            elif self._Workspace.ActiveArchs != []:
                archs = self._Workspace.ActiveArchs
            elif self._Workspace.ActivePlatform != "" and self._Workspace.ActivePlatform.Archs != []:
                archs = self._Workspace.ActivePlatform.Archs
            else:
                archs = self._Archs
        return archs
    
    def GetModuleTypeList(self, dom):
        """Parse the SupModuleList attribute. If not specified, return all supported module types"""
        moduleTypes = XmlAttribute(dom, "SupModuleList").split()
        if moduleTypes == []:
            moduleTypes = self._ModuleTypes
        return moduleTypes
    
    def GetGuidTypeList(self, dom):
        """Parse GuidTypeList attribute. Default to GUID if not specified"""
        guidTypes = XmlAttribute(dom, "GuidTypeList")
        if guidTypes == []:
            guidTypes = ["GUID"]
        return guidTypes

    def GetFeatureList(self, dom):
        """Parse FeatureFlag attribute"""
        return XmlAttribute(dom, "FeatureFlag").split()
    
    def GetToolchainTagList(self, dom):
        """Parse TagName attribute. Return all defined toolchains defined in tools_def.txt if not given"""
        toolchainTagString = XmlAttribute(dom, "TagName")
        if toolchainTagString == "":
            return self._Workspace.ToolConfig.Toolchains
        return toolchainTagString.split()
    
    def GetToolchainFamilyList(self, dom):
        """Parse ToolChainFamily attribute. Return all defined toolchain families in tools_def.txt if not given"""
        familyString = XmlAttribute(dom, "ToolChainFamily")
        if familyString != "":
            return familyString.split()
        return self._Workspace.ToolConfig.Families
    
    def GetTargetList(self, dom):
        """Parse BuildTargets attribute. Return all build targets defined in tools_def.txt if not given"""
        targetList = XmlAttribute(dom, "BuildTargets").split()
        if targetList == []:
            targetList = self._Workspace.ToolConfig.Targets
        return targetList
    
    def GetUsage(self, dom):
        """Parse Usage attribute. Default to ALWAYS_CONSUMED if not given"""
        usageString = XmlAttribute(dom, "Usage")
        if usageString == "":
            return "ALWAYS_CONSUMED"
        return usageString
    
    def GetBuildOptionList(self, dom):
        """Parse Options/Option element. Return a options dictionay with keys as (toolchain, target, arch, toolcode, attr)"""
        optionList = XmlList(dom, "/Options/Option")
        buildOptions = {}
        for option in optionList:
            targets = self.GetTargetList(option)
            toolchainFamilies = self.GetToolchainFamilyList(option)
            toolchainTags = self.GetToolchainTagList(option)
            toolcode = XmlAttribute(option, "ToolCode")
            archs = self.GetArchList(option)
            flag = XmlElementData(option)
            # print flag

            toolchains = []
            if toolchainTags != []:
                toolchains = toolchainTags
            elif toolchainFamilies != []:
                toolchains = toolchainFamilies
            else:
                raise Exception("No toolchain specified for a build option: " + self._Owner.Name)

            if targets == []: targets = self._Workspace.ActiveTargets
            if archs == []: archs = self._Workspace.ActiveArchs

            for toolchain in toolchains:
                for target in targets:
                    for arch in archs:
                        buildOptions[(toolchain, target, arch, toolcode, "FLAGS")] = flag
        return buildOptions

    def GetFvBindingList(self, dom):
        """Parse FvBinding element. If not specified, return NULL FV"""
        fvBindingList = XmlElementData(dom).split()
        if fvBindingList == []:
            fvBindingList = ["NULL"]
        return fvBindingList
    
    def IsBuildable(self, type):
        """Test if a file with the type can be built by a tool"""
        return type in self._BuildableFileTypes
    
    def GetToolCode(self, type):
        """Get the toolcode which must be used to build files with the type"""
        toolcode = ""
        if type in self._ToolMapping:
            toolcode = self._ToolMapping[type]
        return toolcode
        
    def GetBoolean(self, dom):
        """Transate true/false in string form to python's True/False value"""
        boolString = XmlElementData(dom).upper()
        if boolString == ""  or boolString == "FALSE"  or boolString == "NO":
            return False
        else:
            return True
        
class LibraryDeclaration(FrameworkElement.LibraryInterface, SurfaceAreaElement):
    def __init__(self, workspace, package, dom):
        FrameworkElement.LibraryInterface.__init__(self)
        self.Package = package
        SurfaceAreaElement.__init__(self, workspace, package, dom)

    def Parse(self):
        dom = self._Root
        self.Name = XmlAttribute(dom, "Name")
        self.Path = os.path.normpath(XmlElementData(XmlNode(dom, "/LibraryClass/IncludeHeader")))
        self.Dir  = os.path.dirname(self.Path)
        
        attribute = XmlAttribute(dom, "RecommendedInstanceGuid")
        if attribute is not '':
            self.FavoriteIntance = FrameworkElement.Module()
            self.FavoriteIntance.Guid = attribute

        attribute = XmlAttribute(dom, "RecommendedInstanceVersion")
        if attribute is not '':
            if self.FavoriteIntance == "":
                raise "No GUID for the recommened library instance"
            self.FavoriteIntance.Version = attribute

        self.Archs = self.GetArchList(dom)
        self.ModuleTypes = self.GetModuleTypeList(dom)

class LibraryClass(FrameworkElement.LibraryClass, SurfaceAreaElement):
    def __init__(self, workspace, module, dom):
        FrameworkElement.LibraryClass.__init__(self)
        SurfaceAreaElement.__init__(self, workspace, module, dom)

    def Parse(self):
        dom = self._Root
        
        self.Name = XmlElementData(XmlNode(dom, "/LibraryClass/Keyword"))
        self.Usage = self.GetUsage(dom)
        self.Features = self.GetFeatureList(dom)
        self.Archs  = self.GetArchList(dom)

        attribute = XmlAttribute(dom, "RecommendedInstanceGuid")
        if attribute is not '':
            self.FavoriteIntance = FrameworkElement.Module()
            self.FavoriteIntance.Guid = attribute

        attribute = XmlAttribute(dom, "RecommendedInstanceVersion")
        if attribute is not '':
            if self.FavoriteIntance == "":
                self.FavoriteIntance = FrameworkElement.Module()
            self.FavoriteIntance.Version = attribute

class SourceFile(FrameworkElement.SourceFile, SurfaceAreaElement):
    def __init__(self, workspace, module, dom):
        FrameworkElement.SourceFile.__init__(self)
        SurfaceAreaElement.__init__(self, workspace, module, dom)

    def Parse(self):
        dom = self._Root
        self.Path = os.path.normpath(XmlElementData(dom))
        self.Dir  = os.path.dirname(self.Path)
        self.Type = self.GetFileType()
        self.Toolchains = self.GetToolchainTagList(dom)
        self.Families = self.GetToolchainFamilyList(dom)
        self.Archs = self.GetArchList(dom)
        self.Features = self.GetFeatureList(dom)

    def GetFileType(self):
        type = XmlAttribute(self._Root, "ToolCode")
        if type == "":
            fileName = os.path.basename(self.Path)
            self.BaseName,self.Ext = os.path.splitext(fileName)
            if self.Ext in self._FileTypes:
                type = self._FileTypes[self.Ext]
            else:
                type = ""
        return type
            
class PackageDependency(FrameworkElement.PackageDependency, SurfaceAreaElement):
    def __init__(self, workspace, module, dom):
        FrameworkElement.PackageDependency.__init__(self)
        SurfaceAreaElement.__init__(self, workspace, module, dom)

    def Parse(self):
        dom = self._Root
        self.GuidValue = XmlAttribute(dom, "PackageGuid").upper()
        self.Version = XmlAttribute(dom, "PackageVersion")
        self.Archs = self.GetArchList(dom)
        self.Features = self.GetFeatureList(dom)

    def Postprocess(self):
        self.Package = self._Workspace.GetPackage(self.GuidValue, self.Version)
        if self.Package == "": raise "No package with GUID=" + self.GuidValue + "VERSION=" + self.Version

class Protocol(FrameworkElement.Protocol, SurfaceAreaElement):
    def __init__(self, workspace, module, dom):
        FrameworkElement.Protocol.__init__(self)
        SurfaceAreaElement.__init__(self, workspace, module, dom)

    def Parse(self):
        dom = self._Root
        self.CName = XmlElementData(XmlNode(dom, "/Protocol/ProtocolCName"))
        self.Usage = self.GetUsage(dom)
        self.Archs = self.GetArchList(dom)
        self.Features = self.GetFeatureList(dom)

    def Postprocess(self):
        for pd in self._Owner._Elements["PackageDependencies"]:
            if self.CName not in pd.Package.Protocols: continue
            self.GuidValue = pd.Package.Protocols[self.CName]

class ProtocolNotify(FrameworkElement.ProtocolNotify, SurfaceAreaElement):
    def __init__(self, workspace, module, dom):
        FrameworkElement.ProtocolNotify.__init__(self)
        SurfaceAreaElement.__init__(self, workspace, module, dom)

    def Parse(self):
        dom = self._Root
        
        self.CName = XmlElementData(XmlNode(dom, "/ProtocolNotify/ProtocolCName"))
        self.Usage = self.GetUsage(dom)
        self.Archs = self.GetArchList(dom)
        self.Features = self.GetFeatureList(dom)

    def Postprocess(self):
        for pd in self._Owner._Elements["PackageDependencies"]:
            if self.CName not in pd.Package.Protocols: continue
            self.GuidValue = pd.Package.Protocols[self.CName]

class Ppi(FrameworkElement.Ppi, SurfaceAreaElement):
    def __init__(self, workspace, module, dom):
        FrameworkElement.Ppi.__init__(self)
        SurfaceAreaElement.__init__(self, workspace, module, dom)

    def Parse(self):
        dom = self._Root
        self.CName = XmlElementData(XmlNode(dom, "/Ppi/PpiCName"))
        self.Usage = self.GetUsage(dom)
        self.Archs = self.GetArchList(dom)
        self.Features = self.GetFeatureList(dom)

    def Postprocess(self):
        for pd in self._Owner._Elements["PackageDependencies"]:
            if self.CName not in pd.Package.Ppis: continue
            self.GuidValue = pd.Package.Ppis[self.CName]

class PpiNotify(FrameworkElement.PpiNotify, SurfaceAreaElement):
    def __init__(self, workspace, module, dom):
        FrameworkElement.PpiNotify.__init__(self)
        SurfaceAreaElement.__init__(self, workspace, module, dom)

    def Parse(self):
        dom = self._Root
        self.CName = XmlElementData(XmlNode(dom, "/PpiNotify/PpiCName"))
        self.Usage = self.GetUsage(dom)
        self.Archs = self.GetArchList(dom)
        self.Features = self.GetFeatureList(dom)

    def Postprocess(self):
        for pd in self._Owner._Elements["PackageDependencies"]:
            if self.CName not in pd.Package.Ppis: continue
            self.GuidValue = pd.Package.Ppis[self.CName]

class Guid(FrameworkElement.Guid, SurfaceAreaElement):
    def __init__(self, workspace, module, dom):
        FrameworkElement.Guid.__init__(self)
        SurfaceAreaElement.__init__(self, workspace, module, dom)

    def Parse(self):
        dom = self._Root
        self.CName = XmlElementData(XmlNode(dom, "/GuidCNames/GuidCName"))
        self.Usage = self.GetUsage(dom)
        self.Archs = self.GetArchList(dom)
        self.Features = self.GetFeatureList(dom)

    def Postprocess(self):
        for pd in self._Owner._Elements["PackageDependencies"]:
            if self.CName not in pd.Package.Guids: continue
            self.GuidValue = pd.Package.Guids[self.CName]

class Extern(FrameworkElement.Extern, SurfaceAreaElement):
    def __init__(self, workspace, module, dom):
        FrameworkElement.Extern.__init__(self)
        SurfaceAreaElement.__init__(self, workspace, module, dom)

    def Parse(self):
        dom = self._Root
        self.Archs = self.GetArchList(dom)
        self.Features = self.GetFeatureList(dom)
        
        extern = XmlNode(dom, "/Extern/ModuleEntryPoint")
        if extern is not None and extern is not '':
            self.ModuleEntryPoints.append(XmlElementData(extern))
            
        extern = XmlNode(dom, "/Extern/ModuleUnloadImage")
        if extern is not None and extern is not '':
            self.ModuleUnloadImages.append(XmlElementData(extern))
            
        extern = XmlNode(dom, "/Extern/Constructor")
        if extern is not None and extern is not '':
            self.Constructors.append(XmlElementData(extern))
            
        extern = XmlNode(dom, "/Extern/Destructor")
        if extern is not None and extern is not '':
            self.Destructors.append(XmlElementData(extern))
            
        extern = XmlNode(dom, "/Extern/DriverBinding")
        if extern is not None and extern is not '':
            self.DriverBindings.append(XmlElementData(extern))
            
        extern = XmlNode(dom, "/Extern/ComponentName")
        if extern is not None and extern is not '':
            self.ComponentNames.append(XmlElementData(extern))
            
        extern = XmlNode(dom, "/Extern/DriverConfig")
        if extern is not None and extern is not '':
            self.DriverConfigs.append(XmlElementData(extern))
            
        extern = XmlNode(dom, "/Extern/DriverDiag")
        if extern is not None and extern is not '':
            self.DriverDiags.append(XmlElementData(extern))

        extern = XmlNode(dom, "/Extern/SetVirtualAddressMapCallBacks")
        if extern is not None and extern is not '':
            self.SetVirtualAddressMapCallBacks.append(XmlElementData(extern))

        extern = XmlNode(dom, "/Extern/ExitBootServicesCallBack")
        if extern is not None and extern is not '':
            self.ExitBootServicesCallBacks.append(XmlElementData(extern))

class IndustryStdHeader(FrameworkElement.IncludeFile, SurfaceAreaElement):
    def __init__(self, workspace, package, dom):
        FrameworkElement.IncludeFile.__init__(self)
        SurfaceAreaElement.__init__(self, workspace, package, dom)

    def Parse(self):
        dom = self._Root
        self.Path = os.path.normpath(XmlElementData(XmlNode(dom, "/IndustryStdHeader/IncludeHeader")))
        self.Dir  = os.path.dirname(self.Path)
        self.Archs = self.GetArchList(dom)
        self.ModuleTypes = self.GetModuleTypeList(dom)

class PackageHeader(FrameworkElement.IncludeFile, SurfaceAreaElement):
    def __init__(self, workspace, package, dom):
        FrameworkElement.IncludeFile.__init__(self)
        SurfaceAreaElement.__init__(self, workspace, package, dom)

    def Parse(self):
        dom = self._Root
        self.Path = os.path.normpath(XmlElementData(dom))
        self.Dir  = os.path.dirname(self.Path)
        self.ModuleType = XmlAttribute(dom, "ModuleType")

class GuidDeclaration(FrameworkElement.Guid, SurfaceAreaElement):
    def __init__(self, workspace, package, dom):
        FrameworkElement.Guid.__init__(self)
        SurfaceAreaElement.__init__(self, workspace, package, dom)

    def Parse(self):
        dom = self._Root
        self.CName = XmlElementData(XmlNode(dom, "/Entry/C_Name"))
        self.GuidValue = XmlElementData(XmlNode(dom, "/Entry/GuidValue")).upper()
        self.Name = XmlAttribute(dom, "Name")
        self.Types = self.GetGuidTypeList(dom)
        self.Archs = self.GetArchList(dom)
        self.ModuleTypes = self.GetModuleTypeList(dom)

    def Postprocess(self):
        pass
    
class ProtocolDeclaration(GuidDeclaration, SurfaceAreaElement):
    pass

class PpiDeclaration(GuidDeclaration, SurfaceAreaElement):
    pass

class PcdDeclaration(FrameworkElement.Pcd, SurfaceAreaElement):
    def __init__(self, workspace, package, dom):
        FrameworkElement.Pcd.__init__(self)
        SurfaceAreaElement.__init__(self, workspace, package, dom)

    def Parse(self):
        dom = self._Root
        self.Types      = XmlElementData(XmlNode(dom, "/PcdEntry/ValidUsage")).split()
        self.CName      = XmlElementData(XmlNode(dom, "/PcdEntry/C_Name"))
        self.Token      = XmlElementData(XmlNode(dom, "/PcdEntry/Token"))
        self.TokenSpace = XmlElementData(XmlNode(dom, "/PcdEntry/TokenSpaceGuidCName"))
        self.DatumType  = XmlElementData(XmlNode(dom, "/PcdEntry/DatumType"))
        self.Default    = XmlElementData(XmlNode(dom, "/PcdEntry/DefaultValue"))
        self.Archs      = self.GetArchList(dom)
        self.ModuleTypes= self.GetModuleTypeList(dom)

class LibraryInstance(FrameworkElement.PlatformModule, SurfaceAreaElement):
    def __init__(self, workspace, platformModule, dom):
        FrameworkElement.PlatformModule.__init__(self)
        SurfaceAreaElement.__init__(self, workspace, platformModule, dom)

    def Parse(self):
        dom = self._Root
        self.GuidValue = XmlAttribute(dom, "ModuleGuid").upper()
        self.Version = XmlAttribute(dom, "ModuleVersion")
        self._Elements["PackageGuid"] = XmlAttribute(dom, "PackageGuid").upper()
        self._Elements["PackageVersion"] = XmlAttribute(dom, "PackageVersion")
        
    def Postprocess(self):
        self.Module = self._Workspace.GetModule(self.GuidValue, self.Version,
                        self._Elements["PackageGuid"], self._Elements["PackageVersion"])
        self.Platform = self._Owner.Platform
        self.Archs = self._Owner.Archs
        self.Pcds = self._Owner.Pcds
        self.BuildType = "lib"

class PlatformModule(FrameworkElement.PlatformModule, SurfaceAreaElement):
    def __init__(self, workspace, platform, dom):
        FrameworkElement.PlatformModule.__init__(self)
        self.Platform = platform
        SurfaceAreaElement.__init__(self, workspace, platform, dom)

    def Parse(self):
        dom = self._Root
        self.GuidValue = XmlAttribute(dom, "ModuleGuid").upper()
        self.Version = XmlAttribute(dom, "ModuleVersion")
        self.Archs = self.GetArchList(dom)

        self._Elements["PackageGuid"] = XmlAttribute(dom, "PackageGuid").upper()
        self._Elements["PackageVersion"] = XmlAttribute(dom, "PackageVersion")

        libraryList = XmlList(dom, "/ModuleSA/Libraries/Instance")
        for lib in libraryList:
            self.Libraries.append(LibraryInstance(self._Workspace, self, lib))
            
        dom = XmlNode(dom, "/ModuleSA/ModuleSaBuildOptions")
        self.FvBindings = self.GetFvBindingList(XmlNode(dom, "/ModuleSaBuildOptions/FvBinding"))
        self.FfsLayouts = XmlElementData(XmlNode(dom, "/ModuleSaBuildOptions/FfsFormatKey")).split()
        self.BuildOptions = self.GetBuildOptionList(XmlNode(dom, "/ModuleSaBuildOptions/Options"))

    def Postprocess(self):
        self.Module = self._Workspace.GetModule(self.GuidValue, self.Version,
                        self._Elements["PackageGuid"], self._Elements["PackageVersion"])
        if self.Module == "":
            raise Exception("No module found: \n\t\tGUID=%s \n\t\tVERSION=%s \n\t\tPACKAGE_GUID=%s \n\t\tPACKAGE_VERSION=%s" % (
                  self.GuidValue, self.Version, self._Elements["PackageGuid"], self._Elements["PackageVersion"]))
    
##    def SetupEnvironment(self):
##        self.Environment    = {
##            "ARCH"                  :   "",
##            "MODULE_BUILD_TARGET"   :   "",
##            "SINGLE_MODULE_BUILD"   :   "",
##            "PLATFORM_PREBUILD"     :   "",
##            "PLATFORM_POSTBUILD"    :   "",
##            "LIBS"                  :   "",
##            "SOURCE_FILES"          :   "",
##            "ENTRYPOINT"            :   "_ModuleEntryPoint",
##        }    # name/value pairs
##        self.Environment["MODULE_BUILD_TARGET"] = "platform_module_build"

class ModuleSurfaceArea(FrameworkElement.Module, SurfaceAreaElement):
    def __init__(self, workspace, package, path):
        FrameworkElement.Module.__init__(self)

        self.Path = os.path.normpath(path)
        self.Dir  = os.path.dirname(self.Path)
        self.FileBaseName,_ext = os.path.splitext(os.path.basename(self.Path))
        self.Package = package
        SurfaceAreaElement.__init__(self, workspace, package)

    def _MsaHeader(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
        self.Name = XmlElementData(XmlNode(dom, "/MsaHeader/ModuleName"))
        self.Type = XmlElementData(XmlNode(dom, "/MsaHeader/ModuleType"))
        self.GuidValue = XmlElementData(XmlNode(dom, "/MsaHeader/GuidValue")).upper()
        self.Version = XmlElementData(XmlNode(dom, "/MsaHeader/Version"))
        
    def _ModuleDefinitions(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
        self.Archs = XmlElementData(XmlNode(dom, "/ModuleDefinitions/SupportedArchitectures")).split()
        self.IsBinary = self.GetBoolean(XmlNode(dom, "/ModuleDefinitions/BinaryModule"))
        self.BaseName = XmlElementData(XmlNode(dom, "/ModuleDefinitions/OutputFileBasename"))
        
    def _LibraryClassDefinitions(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
        lcList = []
        for lc in XmlList(dom, "/LibraryClassDefinitions/LibraryClass"):
            lcList.append(LibraryClass(self._Workspace, self, lc))
        self._Elements["LibraryClassDefinitions"] = lcList

    def _SourceFiles(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
        srcList = []
        for f in XmlList(dom, "/SourceFiles/Filename"):
            srcList.append(SourceFile(self._Workspace, self, f))
        self._Elements["SourceFiles"] = srcList

    def _NonProcessedFiles(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
        for f in XmlList(dom, "/NonProcessedFiles/Filename"):
            self.NonProcessedFiles.append(SourceFile(self._Workspace, self, f))

    def _PackageDependencies(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
        pdList = []
        for pkg in XmlList(dom, "/PackageDependencies/Package"):
            pdList.append(PackageDependency(self._Workspace, self, pkg))
        self._Elements["PackageDependencies"] = pdList

    def _Protocols(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
    
        protocolList = []
        for p in XmlList(dom, "/Protocols/Protocol"):
            protocolList.append(Protocol(self._Workspace, self, p))
        for p in XmlList(dom, "/Protocols/ProtocolNotify"):
            protocolList.append(ProtocolNotify(self._Workspace, self, p))
            
        self._Elements["Protocols"] = protocolList

    def _Ppis(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
    
        ppiList = []
        for p in XmlList(dom, "/PPIs/Ppi"):
            ppiList.append(Ppi(self._Workspace, self, p))
        for p in XmlList(dom, "/PPIs/PpiNotify"):
            ppiList.append(PpiNotify(self._Workspace, self, p))
            
        self._Elements["PPIs"] = ppiList

    def _Guids(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
        guidList = []
        for g in XmlList(dom, "/Guids/GuidCNames"):
            guidList.append(Guid(self._Workspace, self, g))
        self._Elements["Guids"] = guidList

    def _Externs(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
        self.PcdIsDriver = self.GetBoolean(XmlNode(dom, "/Externs/PcdIsDriver"))
        self.NeedsFlashMap_h = self.GetBoolean(XmlNode(dom, "/Externs/TianoR8FlashMap_h"))

        externList = []
        specs = FrameworkElement.Extern()
        specs.Archs = self._Archs
        externList.append(specs)
        for spec in XmlList(dom, "/Externs/Specification"):
            specs.Specifications.append(XmlElementData(spec))
        for ext in XmlList(dom, "/Externs/Extern"):
            externList.append(Extern(self._Workspace, self, ext))
        self._Elements["Externs"] = externList

    def _ModuleBuildOptions(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
        self.BuildOptions = self.GetBuildOptionList(XmlNode(dom, "/ModuleBuildOptions/Options"))

    def _UserExtensions(self, xpath):
        domList = XmlList(self._Root, xpath)
        if domList == []: return
        for extension in domList:
            userId = XmlAttribute(extension, "UserID")
            identifier = XmlAttribute(extension, "Identifier")
            if userId == '' or identifier == '':
                raise Exception("No UserId or Identifier specified")
            if userId != "TianoCore": continue
            if identifier not in self.UserExtensions:
                self.UserExtensions[identifier] = []

            contentList = self.UserExtensions[identifier]
            for node in extension.childNodes:
                #print node.nodeType
                contentList.append(node.cloneNode(True))

    def Parse(self):
        fileFullPath = self._Workspace.SubPath(os.path.dirname(self.Package.Path), self.Path)
        self._Root = xml.dom.minidom.parse(fileFullPath)
        assert self._Root.documentElement.tagName == "ModuleSurfaceArea"

        # print "  Parsing...",self.Path
        self._MsaHeader("/ModuleSurfaceArea/MsaHeader")
        self._ModuleDefinitions("/ModuleSurfaceArea/ModuleDefinitions")
        self._PackageDependencies("/ModuleSurfaceArea/PackageDependencies")
        self._LibraryClassDefinitions("/ModuleSurfaceArea/LibraryClassDefinitions")
        self._SourceFiles("/ModuleSurfaceArea/SourceFiles")
        self._NonProcessedFiles("/ModuleSurfaceArea/NonProcessedFiles")
        self._Protocols("/ModuleSurfaceArea/Protocols")
        self._Ppis("/ModuleSurfaceArea/Ppis")
        self._Guids("/ModuleSurfaceArea/Guids")
        self._Externs("/ModuleSurfaceArea/Externs")
        self._ModuleBuildOptions("/ModuleSurfaceArea/ModuleBuildOptions")
        self._UserExtensions("/ModuleSurfaceArea/UserExtensions")

    def Postprocess(self):
        # resolve package dependency
        if self._Elements.has_key("PackageDependencies"):
            for pd in self._Elements["PackageDependencies"]:
                package = pd.Package
                if self.Type not in package.PackageIncludes:
                    print "! Module type %s is not supported in the package %s" % (self.Type, package.Name)

                for arch in pd.Archs:
                    if arch not in self.IncludePaths:
                        self.IncludePaths[arch] = []
                    self.IncludePaths[arch].append(package.SubPath("Include"))
                    self.IncludePaths[arch].append(package.SubPath("Include", arch.capitalize()))

                    if arch not in self.IncludeFiles:
                        self.IncludeFiles[arch] = []
                    if self.Type in package.PackageIncludes:
                        for path in package.PackageIncludes[self.Type]:
                            self.IncludeFiles[arch].append(package.SubPath(path))

        # resolve library class
        if self._Elements.has_key("LibraryClassDefinitions"):
            for lc in self._Elements["LibraryClassDefinitions"]:
                lc.Interface = self.GetLibraryInterface(lc.Name)
                if "ALWAYS_PRODUCED" in lc.Usage:
                    self.IsLibrary = True
                    lc.Interface.Instances.append(self)
                else:
                    lc.Interface.Consumers.append(self)

                for arch in lc.Archs:
                    if arch not in self.LibraryClasses:
                        self.LibraryClasses[arch] = []
                    self.LibraryClasses[arch].append(lc)
            
        # expand source files
        if self._Elements.has_key("SourceFiles"):
            for src in self._Elements["SourceFiles"]:
                for arch in src.Archs:
                    if arch not in self.SourceFiles:
                        self.SourceFiles[arch] = {}
                    if src.Type not in self.SourceFiles[arch]:
                        self.SourceFiles[arch][src.Type] = []
                    self.SourceFiles[arch][src.Type].append(src)
                
        # expand guids
        if self._Elements.has_key("Guids"):
            for guid in self._Elements["Guids"]:
                for arch in guid.Archs:
                    if arch not in self.Guids:
                        self.Guids[arch] = []
                    self.Guids[arch].append(guid)
                
        # expand protocol
        if self._Elements.has_key("Protocols"):
            for protocol in self._Elements["Protocols"]:
                for arch in protocol.Archs:
                    if arch not in self.Protocols:
                        self.Protocols[arch] = []
                    self.Protocols[arch].append(protocol)

        # expand ppi
        if self._Elements.has_key("PPIs"):
            for ppi in self._Elements["PPIs"]:
                for arch in ppi.Archs:
                    if arch not in self.Ppis:
                        self.Ppis[arch] = []
                    self.Ppis[arch].append(ppi)
                
        # expand extern
        if self._Elements.has_key("Externs"):
            for extern in self._Elements["Externs"]:
                for arch in extern.Archs:
                    if arch not in self.Externs:
                        self.Externs[arch] = []
                    self.Externs[arch].append(extern)
                    
    def GetLibraryInterface(self, name):
        if name in self.Package.LibraryInterfaces:
            return self.Package.LibraryInterfaces[name]
        for pd in self._Elements["PackageDependencies"]:
            if name in pd.Package.LibraryInterfaces:
                return pd.Package.LibraryInterfaces[name]
        return ""
##    def SetupEnvironment(self):
##        self.Environment["MODULE"] = self.Name
##        self.Environment["MODULE_GUID"] = self.GuidValue
##        self.Environment["MODULE_VERSION"] = self.Version
##        self.Environment["MODULE_TYPE"] = self.Type
##        self.Environment["MODULE_FILE_BASE_NAME"] = os.path.basename(self.Path).split(".")[0]
##        self.Environment["MODULE_RELATIVE_DIR"] = os.path.dirname(self.Path)
##        self.Environment["BASE_NAME"] = self.OutputName

class Workspace(FrameworkElement.Workspace, SurfaceAreaElement):
    _Db = "Tools/Conf/FrameworkDatabase.db"
    _Target = "Tools/Conf/Target.txt"
    _PlatformBuildPath = "Tools/Conf/platform_build_path.txt"
    _ModuleBuildPath = "Tools/Conf/module_build_path.txt"
    
    def __init__(self, path, fpdList=None, msaList=None):
        FrameworkElement.Workspace.__init__(self)
        SurfaceAreaElement.__init__(self, self, None, None, False, False)
        self.Path = os.path.normpath(path)
        self.Dir  = os.path.dirname(self.Path)
        self._Elements["PlatformList"] = fpdList
        self._Elements["ModuleList"] = msaList
        self.Parse()
        self.Postprocess()

    def _FdbHeader(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
        self.Name = XmlElementData(XmlNode(dom, "/FdbHeader/DatabaseName"))
        self.GuidValue = XmlElementData(XmlNode(dom, "/FdbHeader/GuidValue")).upper()
        self.Version = XmlElementData(XmlNode(dom, "/FdbHeader/Version"))

    def _PackageList(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
    
        fileList = XmlList(dom, "/PackageList/Filename")
        packages = []
        for f in fileList:
            packages.append(os.path.normpath(XmlElementData(f)))
        self._Elements["PackageList"] = packages
            
    def _PlatformList(self, xpath):
        if len(self._Elements["PlatformList"]) > 0:
            return
        
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
    
        fileList = XmlList(dom, "/PlatformList/Filename")
        platforms = []
        for f in fileList:
            platforms.append(os.path.normpath(XmlElementData(f)))
        self._Elements["PlatformList"] = platforms

    def _FarList(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
    
        fileList = XmlList(dom, "/FarList/Filename")
        fars = []
        for f in fileList:
            fars.append(os.path.normpath(XmlElementData(f)))
        self._Elements["FarList"] = fars

    def ParseWorkspaceDatabase(self):
        # parse frameworkdatabase.db
        self._Root = xml.dom.minidom.parse(self.SubPath(self._Db))
        assert self._Root.documentElement.tagName == "FrameworkDatabase"

        self._FdbHeader("/FrameworkDatabase/FdbHeader")
        self._PackageList("/FrameworkDatabase/PackageList")
        self._PlatformList("/FrameworkDatabase/PlatformList")
        self._FarList("/FrameworkDatabase/FarList")

    def ParseConfig(self):
        # parse target.txt
        self.ParseTargetConfig()
        # parse tools_def.txt
        self.ParseToolConfig()
        # parse platform/module_build_path.txt
        
        # active toolchain
        # print self.TargetConfig
        self.ActiveToolchain = self.TargetConfig["TOOL_CHAIN_TAG"]
        if self.ActiveToolchain not in self.ToolConfig.Toolchains:
            raise "Not supported tool chain tag %s" % self.ActiveToolchain

        # active toolchain family
        self.ActiveFamilies = []
        for key in self.ToolConfig:
            if self.ActiveToolchain in key and  "FAMILY" in key:
                family = self.ToolConfig[key]
                if family not in self.ActiveFamilies:
                    self.ActiveFamilies.append(family)


    def ParsePackage(self, packagePaths=None):
        if packagePaths == None:
            return
        
        for packagePath in packagePaths:
            self.Packages.append(PackageSurfaceArea(self, packagePath))
    
    def ParsePlatform(self, platformPaths=None):
        # Only one active platform is allowed
        activePlatformPath = ""
        if self.TargetConfig["ACTIVE_PLATFORM"] == "":
            if platformPaths != None and len(platformPaths) == 1:
                activePlatformPath = platformPaths[0]
            else:
                raise Exception("No active platform specified or implied!")
        else:
            activePlatformPath = os.path.normpath(self.TargetConfig["ACTIVE_PLATFORM"])

        self.ActivePlatform = PlatformSurfaceArea(self, activePlatformPath)
        self.Platforms.append(self.ActivePlatform)
        
    def ParseTargetConfig(self):
        self.TargetConfig = BuildConfig.TargetConfig(self.SubPath(self._Target))
        # print self.TargetConfig

    def ParseToolConfig(self):
        self.ToolConfig = BuildConfig.ToolConfig(self.SubPath(self.TargetConfig["TOOL_CHAIN_CONF"]))

    def GetModule(self, guid, version, packageGuid, packageVersion):
        moduleGuidIndex = self.ModuleXref["GUID"]
        if guid not in moduleGuidIndex:
            print "! No module has GUID=" + guid
            return ""

        moduleVersionList = moduleGuidIndex[guid]
        # print moduleVersionList
        moduleList = []
        module = ""
        if version != "":
            if version in moduleVersionList:
                moduleList = moduleVersionList[version]
            else:
                return ""
        else:
            ## no version given, return the first one
            version = "0.0"
            for ver in moduleVersionList:
                if ver > version: version = ver
            moduleList = moduleVersionList[version]

        if packageGuid == "":
            ## if no package GUID given, just return the latest one
            version = "0.0"
            for m in moduleList:
                if m.Package.Version > version:
                    version = m.Package.Version
                    module = m
        else:
            version = "0.0"
            for m in moduleList:
                if m.Package.GuidValue != packageGuid: continue
                if packageVersion == "":
                    ## if no version given, just return the latest
                    if m.Package.Version > version:
                        version = m.Package.Version
                        module = m
                elif packageVersion == m.Package.Version:
                    module = m
                    break;

        return module

    def GetModuleByPath(self, path):
        ownerPackage = ""
        ownerPackageFullPath = ""
        for package in self.Packages:
            ownerPackageFullPath = self.SubPath(package.Path)
            if path.startswith(packageFullPath): break

        if ownerPackage == "":
            return ""
        
        for module in ownerPackage.Modules:
            moduleFullPath = os.path.join(ownerPackageFullPath, module.Path)
            if moduleFullPath == path:
                return module
            
        return ""
            
    def GetPackage(self, guid, version):
        packageGuidIndex = self.PackageXref["GUID"]
        if guid not in packageGuidIndex:
            # raise Exception("No package has GUID=" + guid)
            return ""
        
        packageList = packageGuidIndex[guid]
        package = ""
        if version != "":
            if version in packageList:
                package = packageList[version]
        else:
            ## no version given, return the latest one
            version = "0.0"
            for ver in packageList:
                if ver > version: version = ver
            package = packageList[version]

        return package

    def GetPlatform(self, guid, version):
        pass
    
    def GetPlatformByPath(self, path):
        for platform in self.Platforms:
            platformFullPath = self.SubPath(platform.Path)
            if platformFullPath == path:
                return platform
        return ""

    def GetLibraryInterface(self, name, package):
        if name not in self.LibraryInterfaceXref["NAME"]:
            return ""
        liList = self.LibraryInterfaceXref["NAME"][name]
        for li in liList:
            if li.Package == package:
                return li
        return ""
    
    def SubPath(self, *relativePathList):
        return os.path.normpath(os.path.join(self.Path, *relativePathList))
        
    def SetupCrossRef(self):
        ##
        ## setup platform cross reference as nest-dict
        ##      guid -> {version -> platform}
        ##
        ##        platformList = self.Platforms
        ##        for p in platformList:
        ##            guid = p.GuidValue
        ##            version = p.Version
        ##            if guid not in self.PlatformIndex:
        ##                self.PlatformIndex[guid] = {}
        ##            if version in self.PlatformIndex[guid]:
        ##                raise Exception("Duplicate platform")
        ##            self.PlatformIndex[guid][version] = p

        ##
        ## setup package cross reference as nest-dict
        ##      guid -> {version -> package}
        ##      name -> [package list]
        ##      path -> package
        ##
        packageList = self.Packages
        for p in packageList:
            guid = p.GuidValue
            version = p.Version
            packageGuidIndex = self.PackageXref["GUID"]
            if guid not in packageGuidIndex:
                packageGuidIndex[guid] = {}
            if version in packageGuidIndex[guid]:
                raise Exception("Duplicate package: %s-%s [%s]" % p.Name, version, guid)
            packageGuidIndex[guid][version] = p
            
            packageNameIndex = self.PackageXref["NAME"]
            name = p.Name
            if name not in packageNameIndex:
                packageNameIndex[name] = []
            packageNameIndex[name].append(p)
            
            packagePathIndex = self.PackageXref["PATH"]
            path = p.Path
            if path in packagePathIndex:
                raise Exception("Duplicate package: %s %s" % p.Name, p.Path)
            packagePathIndex[path] = p.Path

            ##
            ## setup library class cross reference as
            ##      library class name -> library class object
            ##
            for lcname in p.LibraryInterfaces:
                if lcname not in self.LibraryInterfaceXref["NAME"]:
                    # raise Exception("Duplicate library class: %s in package %s" % (lcname, name))
                    self.LibraryInterfaceXref["NAME"][lcname] = []
                lcInterface = p.LibraryInterfaces[lcname]
                self.LibraryInterfaceXref["NAME"][lcname].append(lcInterface)
                
                lcHeader = p.SubPath(lcInterface.Path)
                if lcHeader not in self.LibraryInterfaceXref["PATH"]:
                    # raise Exception("Duplicate library class interface: %s in package %s" % (lcInterface, name))
                    self.LibraryInterfaceXref["PATH"][lcHeader] = []
                self.LibraryInterfaceXref["PATH"][lcHeader].append(lcInterface)

        ##
        ## setup package cross reference as nest-dict
        ##  guid -> {version -> [module list]}
        ##  name -> [module list]
        ##  path -> module
        for p in packageList:
            p.ParseMsaFile()
            
            moduleList = p.Modules
            for m in moduleList:
                name = m.Name
                path = m.Path
                guid = m.GuidValue
                version = m.Version
                moduleGuidIndex = self.ModuleXref["GUID"]
                if guid not in moduleGuidIndex:
                    moduleGuidIndex[guid] = {}
                else:
                    print "! Duplicate module GUID found:", guid, p.SubPath(path)
                    dm = moduleGuidIndex[guid].values()[0][0]
                    print "                              ", dm.GuidValue,\
                                                    dm.Package.SubPath(dm.Path)

                if version not in moduleGuidIndex[guid]:
                    moduleGuidIndex[guid][version] = []
                if m in moduleGuidIndex[guid][version]:
                    raise Exception("Duplicate modules in the same package: %s-%s [%s]" % (name, version, guid))
                moduleGuidIndex[guid][version].append(m)
                
                modulePathIndex = self.ModuleXref["PATH"]
                path = p.SubPath(m.Path)
                if path in modulePathIndex:
                    raise Exception("Duplicate modules in the same package: %s %s" % (name, path))
                modulePathIndex[path] = m
                
                moduleNameIndex = self.ModuleXref["NAME"]
                if name not in moduleNameIndex:
                    moduleNameIndex[name] = []
                moduleNameIndex[name].append(m)

    def GetToolDef(self, toolchain, target, arch, toolcode, attr):
        return self.ToolConfig[(toolchain, target, arch, toolcode, attr)]
    
    def Parse(self):
        self.ParseConfig()
        self.ParseWorkspaceDatabase()

    def SetupBuild(self):
        # active archs
        self.ActiveArchs = self.TargetConfig["TARGET_ARCH"].split()
        if self.ActiveArchs == []:
            self.ActiveArchs = self.ActivePlatform.Archs

        # active targets
        self.ActiveTargets = self.TargetConfig["TARGET"].split()
        if self.ActiveTargets == []:
            self.ActiveTargets = self.ActivePlatform.Targets


        # active modules
        for msa in self._Elements["ModuleList"]:
            module = self.GetModuleByPath(msa)
            if module == "":
                raise Exception(msa + " is not in any package!")
            self.ActiveModules.append(module)
            self.IndividualModuleBuild = True
        if self.TargetConfig["MULTIPLE_THREAD"].upper() == "ENABLE":
            self.MultiThreadBuild = True
            if "MAX_CONCURRENT_THREAD_NUMBER" in self.TargetConfig:
                self.ThreadCount = self.TargetConfig["MAX_CONCURRENT_THREAD_NUMBER"]
        else:
            self.ThreadCount = "1"

    def Postprocess(self):
        self.ParsePackage(self._Elements["PackageList"])
        self.SetupCrossRef()
        self.ParsePlatform(self._Elements["PlatformList"])
        self.SetupBuild()

##    def SetupEnvironment(self):
##        config = BuildConfig.Config(self.SubPath(self._PlatformBuildPath))
##        for name in config:
##            self.Environment[name] = config[name]
##
##        config = BuildConfig.Config(self.SubPath(self._ModuleBuildPath))
##        for name in config:
##            self.Environment[name] = config[name]
##
##        multiThread = self.TargetConfig["MULTIPLE_THREAD"].upper()
##        threadNumber = self.TargetConfig["MAX_CONCURRENT_THREAD_NUMBER"]
##        if multiThread == "" or multiThread == "FALSE":
##            self.Environment["MULTIPLE_THREAD"] = False
##            self.Environment["MAX_CONCURRENT_THREAD_NUMBER"] = 1
##        else:
##            self.Environment["MULTIPLE_THREAD"] = True
##            if threadNumber != "":
##                self.Environment["MAX_CONCURRENT_THREAD_NUMBER"] = threadNumber
##            else:
##                self.Environment["MAX_CONCURRENT_THREAD_NUMBER"] = 2

class PackageSurfaceArea(FrameworkElement.Package, SurfaceAreaElement):
    def __init__(self, workspace, path):
        FrameworkElement.Package.__init__(self)
        
        self.Path = os.path.normpath(path)
        self.Dir  = os.path.dirname(self.Path)
        SurfaceAreaElement.__init__(self, workspace, workspace, None, True, True)
        
    def _SpdHeader(self, xpath):
        dom = XmlNode(self._Root, xpath)
        self.Name = XmlElementData(XmlNode(dom, "/SpdHeader/PackageName"))
        self.GuidValue = XmlElementData(XmlNode(dom, "/SpdHeader/GuidValue")).upper()
        self.Version = XmlElementData(XmlNode(dom, "/SpdHeader/Version"))

    def _PackageDefinitions(self, xpath):
        dom = XmlNode(self._Root, xpath)
        self.ReadOnly = XmlElementData(XmlNode(dom, "/PackageDefinitions/ReadOnly"))
        self.Repackage = XmlElementData(XmlNode(dom, "/PackageDefinitions/RePackage"))

    def _LibraryClassDeclarations(self, xpath):
        dom = XmlNode(self._Root, xpath)
        lcdList = XmlList(dom, "/LibraryClassDeclarations/LibraryClass")
        lcds = []
        for lc in lcdList:
            lcds.append(LibraryDeclaration(self._Workspace, self, lc))
        self._Elements["LibraryClassDeclarations"] = lcds
        
    def _IndustryStdIncludes(self, xpath):
        dom = XmlNode(self._Root, xpath)
        headerList = XmlList(dom, "/IndustryStdIncludes/IndustryStdHeader")
        headers = []
        for h in headerList:
            headers.append(IndustryStdHeader(self._Workspace, self, h))
        self._Elements["IndustryStdIncludes"] = headers
        
    def _MsaFiles(self, xpath):
        dom = XmlNode(self._Root, xpath)
        msaFileList = XmlList(dom, "/MsaFiles/Filename")
        msaFiles = []
        for msa in msaFileList:
            filePath = os.path.normpath(XmlElementData(msa))
            msaFiles.append(filePath)
        self._Elements["MsaFiles"] = msaFiles

    def _PackageHeaders(self, xpath):
        dom = XmlNode(self._Root, xpath)
        headerList = XmlList(dom, "/PackageHeaders/IncludePkgHeader")
        headers = []
        for h in headerList:
            headers.append(PackageHeader(self._Workspace, self, h))
        self._Elements["PackageHeaders"] = headers

    def _GuidDeclarations(self, xpath):
        dom = XmlNode(self._Root, xpath)
        guidList = XmlList(dom, "/GuidDeclarations/Entry")
        guids = []
        for guid in guidList:
            guids.append(GuidDeclaration(self._Workspace, self, guid))
        self._Elements["GuidDeclarations"] = guids
            
    def _ProtocolDeclarations(self, xpath):
        dom = XmlNode(self._Root, xpath)
        protocolList = XmlList(dom, "/ProtocolDeclarations/Entry")
        protocols = []
        for p in protocolList:
            protocols.append(ProtocolDeclaration(self._Workspace, self, p))
        self._Elements["ProtocolDeclarations"] = protocols

    def _PpiDeclarations(self, xpath):
        dom = XmlNode(self._Root, xpath)
        ppiList = XmlList(dom, "/PpiDeclarations/Entry")
        ppis = []
        for p in ppiList:
            ppis.append(PpiDeclaration(self._Workspace, self, p))
        self._Elements["PpiDeclarations"] = ppis

    def _PcdDeclarations(self, xpath):
        dom = XmlNode(self._Root, xpath)
        pcdList = XmlList(dom, "/PcdDeclarations/PcdEntry")
        pcds = []
        for p in pcdList:
            pcds.append(PcdDeclaration(self._Workspace, self, p))
        self._Elements["PcdDeclarations"] = pcds

    def SubPath(self, *relativePathList):
        return os.path.normpath(os.path.join(self.Dir, *relativePathList))

    def Parse(self):
        self._Root = xml.dom.minidom.parse(self._Workspace.SubPath(self.Path))
        assert self._Root.documentElement.tagName == "PackageSurfaceArea"

        # print "Parsing...",self.Path
        self._SpdHeader("/PackageSurfaceArea/SpdHeader")
        self._PackageDefinitions("/PackageSurfaceArea/PackageDefinitions")
        self._LibraryClassDeclarations("/PackageSurfaceArea/LibraryClassDeclarations")
        self._IndustryStdIncludes("/PackageSurfaceArea/IndustryStdIncludes")
        self._MsaFiles("/PackageSurfaceArea/MsaFiles")
        self._PackageHeaders("/PackageSurfaceArea/PackageHeaders")
        self._GuidDeclarations("/PackageSurfaceArea/GuidDeclarations")
        self._ProtocolDeclarations("/PackageSurfaceArea/ProtocolDeclarations")
        self._PpiDeclarations("/PackageSurfaceArea/PpiDeclarations")
        self._PcdDeclarations("/PackageSurfaceArea/PcdDeclarations")
        
    def Postprocess(self):
        # setup guid, protocol, ppi
        for guid in self._Elements["GuidDeclarations"]:
            if guid.CName in self.Guids:
                print "! Duplicate GUID CName (%s) in package %s" % (guid.CName, self.Path)
            self.Guids[guid.CName] = guid
        
        for protocol in self._Elements["ProtocolDeclarations"]:
            if protocol.CName in self.Protocols:
                print "! Duplicate Protocol CName (%s) in package %s" % (protocol.CName, self.Path)
            self.Protocols[protocol.CName] = protocol

        for ppi in self._Elements["PpiDeclarations"]:
            if ppi.CName in self.Ppis:
                print "! Duplicate PPI CName (%s) in package (%s)" % (ppi.CName, self.Path)
            self.Ppis[ppi.CName] = ppi
            
        # package header
        for inc in self._Elements["PackageHeaders"]:
            if inc.ModuleType not in self.PackageIncludes:
                self.PackageIncludes[inc.ModuleType] = []
            self.PackageIncludes[inc.ModuleType].append(inc.Path)
                
        # library class
        for lcd in self._Elements["LibraryClassDeclarations"]:
            if lcd.Name in self.LibraryInterfaces:
                raise "Duplicate library class: " + lcd.Name
            self.LibraryInterfaces[lcd.Name] = lcd
        
        # parse mas files
        # self.ParseMsaFile()
        # resolve RecommendedInstance

    def ParseMsaFile(self):
        for msaFilePath in self._Elements["MsaFiles"]:
            self.Modules.append(ModuleSurfaceArea(self._Workspace, self, msaFilePath))

class PlatformSurfaceArea(FrameworkElement.Platform, SurfaceAreaElement):
    def __init__(self, workspace, path):
        FrameworkElement.Platform.__init__(self)

        self.Path = os.path.normpath(path)
        self.Dir  = os.path.dirname(self.Path)
        SurfaceAreaElement.__init__(self, workspace)
        
    def _PlatformHeader(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
        self.Name = XmlElementData(XmlNode(dom, "/PlatformHeader/PlatformName"))
        self.GuidValue = XmlElementData(XmlNode(dom, "/PlatformHeader/GuidValue")).upper()
        self.Version = XmlElementData(XmlNode(dom, "/PlatformHeader/Version"))

    def _PlatformDefinitions(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
        self.Archs = XmlElementData(XmlNode(dom, "/PlatformDefinitions/SupportedArchitectures")).split()
        if self.Archs == []:
            raise Exception("No ARCH specified in platform " + self.Path)
        self.Targets = XmlElementData(XmlNode(dom, "/PlatformDefinitions/BuildTargets")).split()
        self.OutputPath = os.path.normpath(XmlElementData(XmlNode(dom, "/PlatformDefinitions/OutputDirectory")))

    def _Flash(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return

    def _FrameworkModules(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
        moduleList = XmlList(dom, "/FrameworkModules/ModuleSA")
        modules = []
        for m in moduleList:
            modules.append(PlatformModule(self._Workspace, self, m))
        self._Elements["FrameworkModules"] = modules

    def _DynamicPcdBuildDefinitions(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return

    def _BuildOptions(self, xpath):
        dom = XmlNode(self._Root, xpath)
        if dom == '': return
        self.BuildOptions = self.GetBuildOptionList(XmlNode(dom, "/BuildOptions/Options"))
        # print self.BuildOptions

    def _UserExtensions(self, xpath):
        domList = XmlList(self._Root, xpath)
        if domList == []: return
        for extension in domList:
            userId = XmlAttribute(extension, "UserID")
            identifier = XmlAttribute(extension, "Identifier")
            
            if userId == '' or identifier == '':
                raise Exception("No UserId or Identifier specified")
            if userId != "TianoCore": continue
            if identifier not in self.UserExtensions:
                self.UserExtensions[identifier] = []
                
            contentList = self.UserExtensions[identifier]
            for node in extension.childNodes:
                # print node.nodeType
                contentList.append(node.cloneNode(True))

    def Parse(self):
        self._Root = xml.dom.minidom.parse(self._Workspace.SubPath(self.Path))
        assert self._Root.documentElement.tagName == "PlatformSurfaceArea"

        self._PlatformHeader("/PlatformSurfaceArea/PlatformHeader")
        self._PlatformDefinitions("/PlatformSurfaceArea/PlatformDefinitions")
        self._Flash("/PlatformSurfaceArea/Flash")
        self._FrameworkModules("/PlatformSurfaceArea/FrameworkModules")
        self._DynamicPcdBuildDefinitions("/PlatformSurfaceArea/DynamicPcdBuildDefinitions")
        self._BuildOptions("/PlatformSurfaceArea/BuildOptions")
        self._UserExtensions("/PlatformSurfaceArea/UserExtensions")

    def Postprocess(self):
        # summarize all library modules for build
        for module in self._Elements["FrameworkModules"]:
            for arch in module.Archs:
                if arch not in self.Modules:
                    self.Modules[arch] = []
                self.Modules[arch].append(module)

                if arch not in self.Libraries:
                    self.Libraries[arch] = []
                for li in module.Libraries:
                    if li in self.Libraries[arch]: continue
                    self.Libraries[arch].append(li)

                # FV
            for fvName in module.FvBindings:
                if fvName not in self.Fvs:
                    self.Fvs[fvName] = []
                self.Fvs[fvName].append(module)
        # build options
        # user extension
    
##    def SetupEnvironment(self):
##        self.Environment["PLATFORM"] = self.Name
##        self.Environment["PLATFORM_GUID"] = self.GuidValue
##        self.Environment["PLATFORM_VERSION"] = self.Version
##        self.Environment["PLATFORM_RELATIVE_DIR"] = self.Path
##        self.Environment["PLATFORM_OUTPUT_DIR"] = self.OutputPath

def PrintWorkspace(ws):
    print "\nPlatforms:\n"
    for guid in ws.PlatformXref["GUID"]:
        for ver in ws.PlatformXref["GUID"][guid]:
            platform = ws.PlatformXref["GUID"][guid][ver]
            print "  %s %s-%s" % (guid, platform.Name, ver)
            for pm in platform.Modules:
                print "     %-40s %-10s <%s-%s>" % (pm.Module.Name+"-"+pm.Module.Version,
                ListString(pm.Archs), pm.Module.Package.Name,
                pm.Module.Package.Version)
                for li in pm.Libraries:
                    print "         %-47s <%s-%s>" % (li.Module.Name+"-"+li.Module.Version,
                        li.Module.Package.Name, li.Module.Package.Version)
            print ""
            
    print "\nPackages:\n"
    for guid in ws.PackageXref["GUID"]:
        for ver in ws.PackageXref["GUID"][guid]:
            print "  %s %s-%s" % (guid, ws.PackageXref["GUID"][guid][ver].Name, ver)

    print "\nModules:\n"
    for guid in ws.ModuleXref["GUID"]:
        for ver in ws.ModuleXref["GUID"][guid]:
            for module in ws.ModuleXref["GUID"][guid][ver]:
                print "  %s %-40s [%s-%s]" % (guid, module.Name+"-"+ver, module.Package.Name, module.Package.Version)
                print "      Depending on packages:"
                for arch in module.IncludePaths:
                    print "         ", arch, ":"
                    for path in module.IncludePaths[arch]:
                        print "          ", path
                print "\n"

                for arch in module.IncludeFiles:
                    print "         ", arch, ":"
                    for path in module.IncludeFiles[arch]:
                        print "          ", path
                print "\n"
                
                print "      Source files:"
                for arch in module.SourceFiles:
                    print "         ", arch, ":"
                    for type in module.SourceFiles[arch]:
                        for src in module.SourceFiles[arch][type]:
                            print "            %-40s (%s)" % (src.Path, src.Type)
                print "\n"
    print "\nLibrary Classes:"
    for name in ws.LibraryInterfaceXref["NAME"]:
        lcList = ws.LibraryInterfaceXref["NAME"][name]
        for lc in lcList:
            pkgPath = os.path.dirname(lc.Package.Path)
            print "\n  [%s] <%s>" % (lc.Name, pkgPath + os.path.sep + lc.Path)

            print "    Produced By:"
            for li in lc.Instances:
                print "      %-40s <%s>" % (li.Name+"-"+li.Version, li.Package.SubPath(li.Path))

            print "    Consumed By:"
            for li in lc.Consumers:
                print "      %-40s <%s>" % (li.Name+"-"+li.Version, li.Package.SubPath(li.Path))

    print "\nActive Platform:"
    for arch in ws.ActivePlatform.Libraries:
        print "  Library Instances (%s) (%d libraries)" % (arch , len(ws.ActivePlatform.Libraries[arch]))
        for li in ws.ActivePlatform.Libraries[arch]:
            print "    %s-%s (%s-%s)" % (li.Module.Name, li.Module.Version,
                li.Module.Package.Name, li.Module.Package.Version)

    for arch in ws.ActivePlatform.Modules:
        print "  Driver Modules (%s) (%d modules)" % (arch, len(ws.ActivePlatform.Modules[arch]))
        for m in ws.ActivePlatform.Modules[arch]:
            print "    %s-%s (%s-%s)" % (m.Module.Name, m.Module.Version,
                m.Module.Package.Name, m.Module.Package.Version)

    for fv in ws.ActivePlatform.Fvs:
        print
        print "  Firmware Volume (%s) (%d modules)" % (fv, len(ws.ActivePlatform.Fvs[fv]))
        for m in ws.ActivePlatform.Fvs[fv]:
            print "    %s-%s (%s-%s)" % (m.Module.Name, m.Module.Version,
                m.Module.Package.Name, m.Module.Package.Version)

# for test
if __name__ == "__main__":
    # os.environ["WORKSPACE"]
    workspacePath = os.getenv("WORKSPACE", os.getcwd())
    saFile = ""
    if len(sys.argv) <= 1:
        saFile = os.path.join(workspacePath, "Tools/Conf/FrameworkDatabase.db")
    else:
        saFile = sys.argv[1]

    print "Parsing ... %s\n" % saFile

    startTime = time.clock()
    sa = Workspace(workspacePath, [], [])
    # sa = PackageSurfaceArea(saFile)
    # sa = PlatformSurfaceArea(saFile)
    # sa = ModuleSurfaceArea(saFile)
    # print sa
    
    PrintWorkspace(sa)
    print "\n[Finished in %fs]" % (time.clock() - startTime)

