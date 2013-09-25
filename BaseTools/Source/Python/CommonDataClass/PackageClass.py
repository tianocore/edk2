## @file
# This file is used to define a class object to describe a package
#
# Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.    The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

##
# Import Modules
#
from CommonClass import *
from Common.Misc import sdict

## PackageHeaderClass
#
# This class defined header items used in Package file
# 
# @param IdentificationClass:  Inherited from IdentificationClass class
# @param CommonHeaderClass:    Inherited from CommonHeaderClass class
#
# @var DecSpecification:       To store value for DecSpecification
# @var ReadOnly:               To store value for ReadOnly
# @var RePackage:              To store value for RePackage
# @var ClonedFrom:             To store value for ClonedFrom, it is a set structure as
#                              [ ClonedRecordClass, ...]
#
class PackageHeaderClass(IdentificationClass, CommonHeaderClass):
    def __init__(self):
        IdentificationClass.__init__(self)
        CommonHeaderClass.__init__(self)
        self.DecSpecification = ''
        self.ReadOnly = False
        self.RePackage = False
        self.PackagePath = ''
        self.ClonedFrom = []

## PackageIndustryStdHeaderClass
#
# This class defined industry std header items used in Package file
# 
# @param CommonHeaderClass:  Inherited from CommonHeaderClass class
#
# @var Name:                 To store value for Name
# @var IncludeHeader:        To store value for IncludeHeader
#
class PackageIndustryStdHeaderClass(CommonClass):
    def __init__(self):
        self.Name = ''
        self.IncludeHeader = ''
        CommonClass.__init__(self)

## PackageIncludePkgHeaderClass
#
# This class defined include Pkg header items used in Package file
# 
# @param object:       Inherited from object class
#
# @var IncludeHeader:  To store value for IncludeHeader
# @var ModuleType:     To store value for ModuleType, it is a set structure as
#                      BASE | SEC | PEI_CORE | PEIM | DXE_CORE | DXE_DRIVER | DXE_RUNTIME_DRIVER | DXE_SAL_DRIVER | DXE_SMM_DRIVER | TOOL | UEFI_DRIVER | UEFI_APPLICATION | USER_DEFINED | SMM_CORE
#
class PackageIncludePkgHeaderClass(object):
    def __init__(self):
        self.IncludeHeader = ''
        self.ModuleType = []

## PackageClass
#
# This class defined a complete package item
# 
# @param object:                  Inherited from object class
#
# @var Header:                    To store value for Header, it is a structure as
#                                 {Arch : PackageHeaderClass}
# @var Includes:                  To store value for Includes, it is a list structure as
#                                 [ IncludeClass, ...]
# @var LibraryClassDeclarations:  To store value for LibraryClassDeclarations, it is a list structure as
#                                 [ LibraryClassClass, ...]
# @var IndustryStdHeaders:        To store value for IndustryStdHeaders, it is a list structure as
#                                 [ PackageIndustryStdHeader, ...]
# @var ModuleFiles:               To store value for ModuleFiles, it is a list structure as
#                                 [ '', '', ...] 
# @var PackageIncludePkgHeaders:  To store value for PackageIncludePkgHeaders, it is a list structure as
#                                 [ PackageIncludePkgHeader, ...]
# @var GuidDeclarations:          To store value for GuidDeclarations, it is a list structure as
#                                 [ GuidClass, ...]
# @var ProtocolDeclarations:      To store value for ProtocolDeclarations, it is a list structure as
#                                 [ ProtocolClass, ...]
# @var PpiDeclarations:           To store value for PpiDeclarations, it is a list structure as
#                                 [ PpiClass, ...]
# @var PcdDeclarations:           To store value for PcdDeclarations, it is a list structure as
#                                 [ PcdClass, ...]
# @var UserExtensions:            To store value for UserExtensions, it is a list structure as
#                                 [ UserExtensionsClass, ...]
#
class PackageClass(object):
    def __init__(self):
        self.PackageHeader = PackageHeaderClass()
        self.Header = {}
        self.Includes = []
        self.LibraryClassDeclarations = []
        self.IndustryStdHeaders = []
        self.ModuleFiles = []
        # {[Guid, Value, Path(relative to WORKSPACE)]: ModuleClassObj}
        self.Modules = sdict()
        self.PackageIncludePkgHeaders = []
        self.GuidDeclarations = []
        self.ProtocolDeclarations = []
        self.PpiDeclarations = []
        self.PcdDeclarations = []
        self.PcdChecks = []
        self.UserExtensions = UserExtensionsClass()
        self.MiscFiles = MiscFileClass()
        self.FileList = []

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    P = PackageClass()
