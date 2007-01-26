#!/usr/bin/env python

# Copyright (c) 2007, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

"""Framework Element Classes

TODO
"""

################################################################################
##
## Element class: base class for representing framework elements
##
################################################################################
class Element:
    def __init__(self, **kwargs):
        """(Name, GuidValue, Version, Path, Dir, Archs, Usage, Features, Signature)"""
        ## The path and directory where the information of the element comes from
        self.Path       = "."
        self.Dir        = "."

        ## Element name, guid and version
        self.Name       = ""
        self.GuidValue  = ""
        self.Version    = ""

        ## The element supported architecture
        self.Archs      = []
        
        ## Indiciate how the element is used
        self.Usage      = "ALWAYS_CONSUMED"
        
        ## Indicate what kind of features this element is bound
        self.Features   = []
        
        ## Element signature, used to check the its integerity
        self.Signature  = 0

    def __str__(self):
        return self.Name + "-" + self.Version + " " + " [" + self.GuidValue + "]"

    def __eq__(self, other):
        if not isinstance(other, Element):
            return False
        
        if self.GuidValue != other.GuidValue:
            return False

        if self.Version != other.Version:
            return False

        return True

    def __hash__(self):
        return hash(self.GuidValue + self.Version)

################################################################################
##
## ToolOption: build tool option
## 
################################################################################
class ToolOption(Element):
    
    def __init__(self, **kwargs):
        """Prefix, Value, Tool, Toolchains, Families, Targets"""
        Element.__init__(self)

        self.Prefix     = "/"
        self.Value      = ""
        self.Tool       = ""
        self.Toolchains = ""
        self.Families   = "MSFT"
        self.Targets    = "DEBUG"

################################################################################
##
## BuildTool: build tool
## 
################################################################################
class BuildTool(Element):
    def __init__(self, **kwargs):
        """Options, Toolchains, Families, Targets"""
        Element.__init__(self)

        self.Options    = []
        self.Toolchains = []
        self.Families   = []
        self.Targets    = []

################################################################################
##
## SourceFile: files in a module for build
## 
################################################################################
class SourceFile(Element):
    def __init__(self, **kwargs):
        """BaseName, Ext, Type, Toolchains, Families, Targets"""
        Element.__init__(self)

        self.BaseName   = ""
        self.Ext        = ""
        self.Type       = ""
        self.Toolchains = []
        self.Families   = []
        self.Targets    = []

################################################################################
## 
## IncludeFile: header file
## 
################################################################################
class IncludeFile(SourceFile):
    def __init__(self, **kwargs):
        """Type, Package, ModuleType"""
        SourceFile.__init__(self)

        self.Type       = "HeaderFile"
        self.Package    = ""
        self.ModuleType = ""

################################################################################
## 
## IncludePath:
## 
################################################################################
class IncludePath(IncludeFile):
    pass

################################################################################
##
## LibraryInterface: library class interface
## 
################################################################################
class LibraryInterface(Element):
    def __init__(self, **kwargs):
        """Package, FavoriteInstance, Instances, ModuleTypes, Consumers"""
        Element.__init__(self)

        self.Package            = ""
        self.FavoriteInstance   = ""
        self.Instances          = []
        self.ModuleTypes        = []
        self.Consumers          = []

    def __eq__(self, other):
        if not isinstance(other, LibraryInterface):
            return False
        return self.Name == other.Name
               
    def __hash__(self):
        return hash(self.Name)
    
################################################################################
##
## LibraryClass: library class
##
################################################################################
class LibraryClass(Element):
    def __init__(self, **kwargs):
        """
        ()
        """
        Element.__init__(self)

        self.Interface          = ""
        self.FavoriteInstance   = ""

    def __eq__(self, other):
        if not isinstance(other, LibraryClass):
            return False
        return self.Name == other.Name

    def __hash__(self):
        return hash(self.Name)

################################################################################
##
##  Guid:
## 
################################################################################
class Guid(Element):
    def __init__(self, **kwargs):
        """CName, Types, ModuleTypes"""
        Element.__init__(self)

        self.CName      = ""
        self.Types      = []
        self.ModuleTypes= []

################################################################################
##
##  Protocol:
## 
################################################################################
class Protocol(Guid):
    pass

################################################################################
##
##  ProtocolNotify:
## 
################################################################################
class ProtocolNotify(Protocol):
    pass

################################################################################
##
##  Ppi:
## 
################################################################################
class Ppi(Guid):
    pass

################################################################################
##
##  PpiNotify:
## 
################################################################################
class PpiNotify(Ppi):
    pass

################################################################################
##
##  Extern:
## 
################################################################################
class Extern(Element):
    def __init__(self, **kwargs):
        """Specifications, ModuleEntryPoints, ModuleUnloadImages, Constructors, Destructors, DriverBindings, ComponentNames, DriverConfigs, DriverDiags, SetVirtualAddressMapCallBacks, ExitBootServicesCallBacks"""
        Element.__init__(self)

        self.Specifications                 = []
        self.ModuleEntryPoints              = []
        self.ModuleUnloadImages             = []
        self.Constructors                   = []
        self.Destructors                    = []
        self.DriverBindings                 = []
        self.ComponentNames                 = []
        self.DriverConfigs                  = []
        self.DriverDiags                    = []
        self.SetVirtualAddressMapCallBacks  = []
        self.ExitBootServicesCallBacks      = []

################################################################################
##
##  Sku:
## 
################################################################################
class Sku(Element):
    def __init__(self, **kwargs):
        """Id, Value"""
        Element.__init__(self)

        self.Id     = 0
        self.Value  = ""

################################################################################
##
##  Pcd:
## 
################################################################################
class Pcd(Element):
    def __init__(self, **kwargs):
        """Types, CName, Value, Token, TokenSpace, DatumType, Sku, ModuleTypes"""
        Element.__init__(self)

        self.Types      = []
        self.CName      = ""
        self.Value      = ""
        self.Token      = ""
        self.TokenSpace = ""
        self.DatumType  = ""
        self.Default    = ""
        self.Sku        = ""
        self.ModuleTypes= []

################################################################################
##
## Module: framework module
## 
################################################################################
class Module(Element):
    def __init__(self, **kwargs):
        """Workspace, Package, Type, BaseName, FileBaseName, IsLibrary, PcdIsDriver,
           NeedsFlashMap_h, SourceFiles, IncludePaths, IncludeFiles, NonProcessedFiles,
           LibraryClasses, Guids, Protocols, Ppis, Externs, Pcds,
           UserExtensions, BuildOptions, Environment
        """
        Element.__init__(self)

        self.Workspace              = ""
        self.Package                = ""
        self.Type                   = ""
        self.BaseName               = ""
        self.FileBaseName           = ""
        self.IsLibrary              = False
        self.IsBinary               = False
        self.PcdIsDriver            = False
        self.NeedsFlashMap_h        = False
        self.SourceFiles            = {}    # arch -> {file type -> [source file list]}
        self.IncludePaths           = {}    # arch -> [path list]
        self.IncludeFiles           = {}
        
        self.NonProcessedFiles      = []
        self.LibraryClasses         = {}    # arch -> [library class list]
        
        self.Guids                  = {}    # arch -> [guid object list]
        self.Protocols              = {}    # arch -> []
        self.Ppis                   = {}    # arch -> []
        
        self.Externs                = {}    # arch -> []
        self.Pcds                   = {}    # arch -> []
        
        self.UserExtensions = {}    # identifier -> ...
        self.BuildOptions   = {}    # (toolchain, target, arch, toolcode, "FLAGS") -> flag
        self.Environment    = {}
        
    def __eq__(self, other):
        if not isinstance(other, Module):
            return False

        if self.GuidValue != other.GuidValue:
            return False

        if self.Version != other.Version:
            return False

        if self.Package != other.Package:
            return False
        
        return True

    def __hash__(self):
        return hash(self.GuidValue + self.Version + hash(self.Package))

################################################################################
##
##  PlatformModule: module for build
##
################################################################################
class PlatformModule(Element):
    def __init__(self, **kwargs):
        """Workspace, Platform, Module, Libraries, Pcds, FfsLayouts, FvBindings
           BuildOptions, BuildType, BuildFile, BuildPath, Sections, FfsFile, Environment
        """
        Element.__init__(self)
        self.Workspace      = ""
        self.Platform       = ""
        self.Module         = ""
        self.Libraries      = []
        self.Pcds           = []
        self.FfsLayouts     = []
        self.FvBindings     = []
        self.BuildOptions   = {}

        self.BuildType      = "efi"    ## efi or lib
        self.BuildFile      = "build.xml"
        self.BuildPath      = "${MODULE_BUILD_DIR}"
        self.Sections       = []
        self.FfsFile        = ""
        
        self.Environment    = {}    # name/value pairs
        
    def __eq__(self, other):
        if not isinstance(other, PlatformModule):
            if not isinstance(other, Module):
                return False
            elif self.Module != other:
                return False
        elif self.Module != other.Module:
            return False

        return True

    def __hash__(self):
        return hash(self.Module)

################################################################################
##
## Package: framework package
##
################################################################################
class Package(Element):
    def __init__(self, **kwargs):
        """Workspace, ReadOnly, Repackage, Modules, PackageIncludes, StandardIncludes,
           LibraryInterfaces, Guids, Protocols, Ppis, Pcds, Environment
        """
        Element.__init__(self)

        self.Workspace          = ""
        self.ReadOnly           = True
        self.Repackage          = True
        self.Modules            = []
        self.PackageIncludes    = {}    # module type -> [include file list]
        self.StandardIncludes   = []
        self.LibraryInterfaces  = {}    # class name -> include file
        self.Guids              = {}    # cname -> guid object
        self.Protocols          = {}    # cname -> protocol object
        self.Ppis               = {}    # cname -> ppi object
        self.Pcds               = {}    # token -> pcd object

        self.Environment        = {}

################################################################################
##
## PackageDependency:
##
################################################################################
class PackageDependency(Element):
    def __init__(self, **kwargs):
        """Package"""
        Element.__init__(self)

        self.Package = ""

################################################################################
##
## Platform:
## 
################################################################################
class Platform(Element):
    def __init__(self, **kwargs):
        """Targets, OutputPath, Images, Modules, DynamicPcds, Fvs, Libraries
           BuildFile, BuildPath, BuildOptions, UserExtensions
        """
        Element.__init__(self)
        
        self.Targets        = []
        self.OutputPath     = ""
        self.Images         = []
        self.Modules        = {}    # arch -> [module list]
        self.DynamicPcds    = []
        self.FfsLayouts     = {}
        self.Fvs            = {}
        
        self.Libraries      = {}    # arch -> [library instance]
        self.BuildFile      = "build.xml"
        self.BuildPath      = "${PLATFORM_BUILD_DIR}"
        self.BuildOptions   = {}
        self.UserExtensions = {}
        
        self.Environment    = {}    # name/value pairs

################################################################################
##
## Workspace:
##
################################################################################
class Workspace(Element):
    def __init__(self, **kwargs):
        """Packages, Platforms, Fars, Modules, PlatformIndex, PackageIndex"""
        Element.__init__(self)

        self.Packages               = []
        self.Platforms              = []
        self.Fars                   = []
        self.Modules                = []

        ## "GUID" : {guid : {version : platform}}
        ## "PATH" : {path : platform}
        ## "NAME" : {name : [platform]}
        self.PlatformXref           = {
                                        "GUID"  : {},
                                        "PATH"  : {},
                                        "NAME"  : {},
                                      }
        ## "GUID" : {guid : {version : package}}
        ## "PATH" : {path : package}
        ## "NAME" : {name : package}
        self.PackageXref            = {
                                        "GUID"  : {},
                                        "PATH"  : {},
                                        "NAME"  : {},
                                      }
        ## "GUID" : {guid : {version : [module]}}
        ## "PATH" : {path : module}
        ## "NAME" : {name : module}
        self.ModuleXref             = {
                                        "GUID"  : {},
                                        "PATH"  : {},
                                        "NAME"  : {},
                                      }
        ## "NAME" : {name : [library interface]}
        ## "PATH" : {path : library interface}
        self.LibraryInterfaceXref   = {
                                        "PATH"  : {},
                                        "NAME"  : {},
                                      }
        ## TODO
        self.FarIndex               = {}

        # from target.txt
        self.TargetConfig           = {}
        # from tools_def.txt
        self.ToolConfig             = {}

        self.ActivePlatform         = ""
        self.ActiveToolchain        = ""
        self.ActiveFamilies         = []
        self.ActiveModules          = []
        self.ActiveTargets          = []
        self.ActiveArchs            = []

        self.IndividualModuleBuild  = False
        self.MultiThreadBuild       = False
        self.ThreadCount            = "2"

        self.Environment            = {}

################################################################################
##
## For test:
## 
################################################################################
if __name__ == "__main__":
    pass
