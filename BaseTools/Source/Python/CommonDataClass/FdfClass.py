## @file
# classes represent data in FDF
#
#  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

## FD data in FDF
#
#
class FDClassObject:
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        self.FdUiName = ''
        self.CreateFileName = None
        self.BaseAddress = None
        self.BaseAddressPcd = None
        self.Size = None
        self.SizePcd = None
        self.ErasePolarity = None
        # 3-tuple list (blockSize, numBlocks, pcd)
        self.BlockSizeList = []
        # DefineVarDict[var] = value
        self.DefineVarDict = {}
        # SetVarDict[var] = value
        self.SetVarDict = {}
        self.RegionList = []

## FFS data in FDF
#
#
class FfsClassObject:
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        self.NameGuid = None
        self.Fixed = False
        self.CheckSum = False
        self.Alignment = None
        self.SectionList = []

## FILE statement data in FDF
#
#
class FileStatementClassObject (FfsClassObject) :
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        FfsClassObject.__init__(self)
        self.FvFileType = None
        self.FileName = None
        self.KeyStringList = []
        self.FvName = None
        self.FdName = None
        self.DefineVarDict = {}
        self.KeepReloc = None

## INF statement data in FDF
#
#
class FfsInfStatementClassObject(FfsClassObject):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        FfsClassObject.__init__(self)
        self.Rule = None
        self.Version = None
        self.Ui = None
        self.InfFileName = None
        self.BuildNum = ''
        self.KeyStringList = []
        self.KeepReloc = None
        self.UseArch = None

## section data in FDF
#
#
class SectionClassObject:
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        self.Alignment = None

## Depex expression section in FDF
#
#
class DepexSectionClassObject (SectionClassObject):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        self.DepexType = None
        self.Expression = None
        self.ExpressionProcessed = False

## Compress section data in FDF
#
#
class CompressSectionClassObject (SectionClassObject) :
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        SectionClassObject.__init__(self)
        self.CompType = None
        self.SectionList = []

## Data section data in FDF
#
#
class DataSectionClassObject (SectionClassObject):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        SectionClassObject.__init__(self)
        self.SecType = None
        self.SectFileName = None
        self.SectionList = []
        self.KeepReloc = True

## Rule section data in FDF
#
#
class EfiSectionClassObject (SectionClassObject):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        SectionClassObject.__init__(self)
        self.SectionType = None
        self.Optional = False
        self.FileType = None
        self.StringData = None
        self.FileName = None
        self.FileExtension = None
        self.BuildNum = None
        self.KeepReloc = None

## FV image section data in FDF
#
#
class FvImageSectionClassObject (SectionClassObject):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        SectionClassObject.__init__(self)
        self.Fv = None
        self.FvName = None
        self.FvFileType = None
        self.FvFileName = None
        self.FvFileExtension = None
        self.FvAddr = None

## GUIDed section data in FDF
#
#
class GuidSectionClassObject (SectionClassObject) :
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        SectionClassObject.__init__(self)
        self.NameGuid = None
        self.SectionList = []
        self.SectionType = None
        self.ProcessRequired = False
        self.AuthStatusValid = False
        self.ExtraHeaderSize = -1
        self.FvAddr = []
        self.FvParentAddr = None
        self.IncludeFvSection = False

## SubType GUID section data in FDF
#
#
class SubTypeGuidSectionClassObject (SectionClassObject) :
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        SectionClassObject.__init__(self)
        self.SubTypeGuid = None

## UI section data in FDF
#
#
class UiSectionClassObject (SectionClassObject):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        SectionClassObject.__init__(self)
        self.StringData = None
        self.FileName = None

## Version section data in FDF
#
#
class VerSectionClassObject (SectionClassObject):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        SectionClassObject.__init__(self)
        self.BuildNum = None
        self.StringData = None
        self.FileName = None

## Rule data in FDF
#
#
class RuleClassObject :
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        self.Arch = None
        self.ModuleType = None    # For Module Type
        self.TemplateName = None
        self.NameGuid = None
        self.Fixed = False
        self.Alignment = None
        self.SectAlignment = None
        self.CheckSum = False
        self.FvFileType = None       # for Ffs File Type
        self.KeyStringList = []
        self.KeepReloc = None

## Complex rule data in FDF
#
#
class RuleComplexFileClassObject(RuleClassObject) :
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        RuleClassObject.__init__(self)
        self.SectionList = []

## Simple rule data in FDF
#
#
class RuleSimpleFileClassObject(RuleClassObject) :
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        RuleClassObject.__init__(self)
        self.FileName = None
        self.SectionType = ''
        self.FileExtension = None

## File extension rule data in FDF
#
#
class RuleFileExtensionClassObject(RuleClassObject):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        RuleClassObject.__init__(self)
        self.FileExtension = None

## Capsule data in FDF
#
#
class CapsuleClassObject :
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        self.SpecName = None
        self.UiCapsuleName = None
        self.CreateFile = None
        self.GroupIdNumber = None
        # DefineVarDict[var] = value
        self.DefineVarDict = {}
        # SetVarDict[var] = value
        self.SetVarDict = {}
        # TokensDict[var] = value
        self.TokensDict = {}
        self.CapsuleDataList = []
        self.FmpPayloadList = []

## OptionROM data in FDF
#
#
class OptionRomClassObject:
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        self.DriverName = None
        self.FfsList = []

