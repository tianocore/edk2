## @file
# This file is used to define class objects of [Defines] section for INF file.
# It will consumed by InfParser
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent

'''
InfDefineObject
'''

import os
import re

from Logger import StringTable as ST
from Logger import ToolError
from Library import GlobalData
from Library import DataType as DT
from Library.StringUtils import GetSplitValueList
from Library.Misc import CheckGuidRegFormat
from Library.Misc import Sdict
from Library.Misc import ConvPathFromAbsToRel
from Library.Misc import ValidateUNIFilePath
from Library.ExpressionValidate import IsValidFeatureFlagExp
from Library.ParserValidate import IsValidWord
from Library.ParserValidate import IsValidInfMoudleType
from Library.ParserValidate import IsValidHex
from Library.ParserValidate import IsValidHexVersion
from Library.ParserValidate import IsValidDecVersion
from Library.ParserValidate import IsValidCVariableName
from Library.ParserValidate import IsValidBoolType
from Library.ParserValidate import IsValidPath
from Library.ParserValidate import IsValidFamily
from Library.ParserValidate import IsValidIdentifier
from Library.ParserValidate import IsValidDecVersionVal
from Object.Parser.InfCommonObject import InfLineCommentObject
from Object.Parser.InfCommonObject import CurrentLine
from Object.Parser.InfCommonObject import InfSectionCommonDef
from Object.Parser.InfMisc import ErrorInInf
from Object.Parser.InfDefineCommonObject import InfDefineLibraryItem
from Object.Parser.InfDefineCommonObject import InfDefineEntryPointItem
from Object.Parser.InfDefineCommonObject import InfDefineUnloadImageItem
from Object.Parser.InfDefineCommonObject import InfDefineConstructorItem
from Object.Parser.InfDefineCommonObject import InfDefineDestructorItem

class InfDefSectionOptionRomInfo():
    def __init__(self):
        self.PciVendorId                = None
        self.PciDeviceId                = None
        self.PciClassCode               = None
        self.PciRevision                = None
        self.PciCompress                = None
        self.CurrentLine                = ['', -1, '']
    def SetPciVendorId(self, PciVendorId, Comments):
        #
        # Value has been set before.
        #
        if self.PciVendorId is not None:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_MORE_THAN_ONE_FOUND%(DT.TAB_INF_DEFINES_PCI_VENDOR_ID),
                       LineInfo=self.CurrentLine)
            return False
        #
        # The PciVendorId should be hex string.
        #
        if (IsValidHex(PciVendorId)):
            self.PciVendorId = InfDefMember()
            self.PciVendorId.SetValue(PciVendorId)
            self.PciVendorId.Comments = Comments
            return True
        else:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(PciVendorId),
                       LineInfo=self.CurrentLine)
            return False

    def GetPciVendorId(self):
        return self.PciVendorId

    def SetPciDeviceId(self, PciDeviceId, Comments):
        #
        # Value has been set before.
        #
        if self.PciDeviceId is not None:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_MORE_THAN_ONE_FOUND%(DT.TAB_INF_DEFINES_PCI_DEVICE_ID),
                       LineInfo=self.CurrentLine)
            return False
        #
        # The PciDeviceId should be hex string.
        #
        if (IsValidHex(PciDeviceId)):
            self.PciDeviceId = InfDefMember()
            self.PciDeviceId.SetValue(PciDeviceId)
            self.PciDeviceId.Comments = Comments
            return True
        else:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(PciDeviceId),
                       LineInfo=self.CurrentLine)
            return False

    def GetPciDeviceId(self):
        return self.PciDeviceId

    def SetPciClassCode(self, PciClassCode, Comments):
        #
        # Value has been set before.
        #
        if self.PciClassCode is not None:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_MORE_THAN_ONE_FOUND%(DT.TAB_INF_DEFINES_PCI_CLASS_CODE),
                       LineInfo=self.CurrentLine)
            return False
        #
        # The PciClassCode should be 4 bytes hex string.
        #
        if (IsValidHex(PciClassCode)):
            self.PciClassCode = InfDefMember()
            self.PciClassCode.SetValue(PciClassCode)
            self.PciClassCode.Comments = Comments
            return True
        else:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%\
                       (PciClassCode),
                       LineInfo=self.CurrentLine)
            return False

    def GetPciClassCode(self):
        return self.PciClassCode

    def SetPciRevision(self, PciRevision, Comments):
        #
        # Value has been set before.
        #
        if self.PciRevision is not None:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_MORE_THAN_ONE_FOUND%(DT.TAB_INF_DEFINES_PCI_REVISION),
                       LineInfo=self.CurrentLine)
            return False
        #
        # The PciRevision should be 4 bytes hex string.
        #
        if (IsValidHex(PciRevision)):
            self.PciRevision = InfDefMember()
            self.PciRevision.SetValue(PciRevision)
            self.PciRevision.Comments = Comments
            return True
        else:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(PciRevision),
                       LineInfo=self.CurrentLine)
            return False

    def GetPciRevision(self):
        return self.PciRevision

    def SetPciCompress(self, PciCompress, Comments):
        #
        # Value has been set before.
        #
        if self.PciCompress is not None:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_MORE_THAN_ONE_FOUND%(DT.TAB_INF_DEFINES_PCI_COMPRESS),
                       LineInfo=self.CurrentLine)
            return False

        #
        # The PciCompress should be 'TRUE' or 'FALSE'.
        #
        if (PciCompress == 'TRUE' or PciCompress == 'FALSE'):
            self.PciCompress = InfDefMember()
            self.PciCompress.SetValue(PciCompress)
            self.PciCompress.Comments = Comments
            return True
        else:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(PciCompress),
                       LineInfo=self.CurrentLine)
            return False
    def GetPciCompress(self):
        return self.PciCompress
##
# INF [Define] section Object
#
class InfDefSection(InfDefSectionOptionRomInfo):
    def __init__(self):
        self.BaseName                   = None
        self.FileGuid                   = None
        self.ModuleType                 = None
        self.ModuleUniFileName          = None
        self.InfVersion                 = None
        self.EdkReleaseVersion          = None
        self.UefiSpecificationVersion   = None
        self.PiSpecificationVersion     = None
        self.LibraryClass               = []
        self.Package                    = None
        self.VersionString              = None
        self.PcdIsDriver                = None
        self.EntryPoint                 = []
        self.UnloadImages               = []
        self.Constructor                = []
        self.Destructor                 = []
        self.Shadow                     = None
        self.CustomMakefile             = []
        self.Specification              = []
        self.UefiHiiResourceSection     = None
        self.DpxSource                  = []
        self.CurrentLine                = ['', -1, '']
        InfDefSectionOptionRomInfo.__init__(self)

    ## SetHeadComment
    #
    # @param BaseName: BaseName
    #
    def SetBaseName(self, BaseName, Comments):
        #
        # Value has been set before.
        #
        if self.BaseName is not None:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_MORE_THAN_ONE_FOUND%(DT.TAB_INF_DEFINES_BASE_NAME),
                       LineInfo=self.CurrentLine)
            return False
        if not (BaseName == '' or BaseName is None):
            if IsValidWord(BaseName) and not BaseName.startswith("_"):
                self.BaseName = InfDefMember()
                self.BaseName.SetValue(BaseName)
                self.BaseName.Comments = Comments
                return True
            else:
                ErrorInInf(ST.ERR_INF_PARSER_DEFINE_NAME_INVALID%(BaseName),
                           LineInfo=self.CurrentLine)
                return False

    ## GetBaseName
    #
    def GetBaseName(self):
        return self.BaseName

    ## SetFileGuid
    #
    # @param FileGuid: FileGuid
    #
    def SetFileGuid(self, FileGuid, Comments):
        #
        # Value has been set before.
        #
        if self.FileGuid is not None:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_MORE_THAN_ONE_FOUND\
                       %(DT.TAB_INF_DEFINES_FILE_GUID),
                       LineInfo=self.CurrentLine)
            return False
        #
        # Do verification of GUID content/format
        #
        if (CheckGuidRegFormat(FileGuid)):
            self.FileGuid = InfDefMember()
            self.FileGuid.SetValue(FileGuid)
            self.FileGuid.Comments = Comments
            return True
        else:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_GUID_INVALID%(FileGuid),
                       LineInfo=self.CurrentLine)
            return False

    ## GetFileGuid
    #
    def GetFileGuid(self):
        return self.FileGuid

    ## SetModuleType
    #
    # @param ModuleType: ModuleType
    #
    def SetModuleType(self, ModuleType, Comments):
        #
        # Value has been set before.
        #
        if self.ModuleType is not None:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_MORE_THAN_ONE_FOUND\
                       %(DT.TAB_INF_DEFINES_MODULE_TYPE),
                       LineInfo=self.CurrentLine)
            return False
        #
        # Valid Module Type or not
        #
        if (IsValidInfMoudleType(ModuleType)):
            self.ModuleType = InfDefMember()
            self.ModuleType.SetValue(ModuleType)
            self.ModuleType.CurrentLine = CurrentLine()
            self.ModuleType.CurrentLine.SetLineNo(self.CurrentLine[1])
            self.ModuleType.CurrentLine.SetLineString(self.CurrentLine[2])
            self.ModuleType.CurrentLine.SetFileName(self.CurrentLine[0])
            self.ModuleType.Comments = Comments
            return True
        else:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_MODULETYPE_INVALID%\
                       (ModuleType),
                       LineInfo=self.CurrentLine)
            return False

    ## GetModuleType
    #
    def GetModuleType(self):
        return self.ModuleType

    ## SetModuleUniFileName
    #
    # @param ModuleUniFileName: ModuleUniFileName
    #
    def SetModuleUniFileName(self, ModuleUniFileName, Comments):
        if Comments:
            pass
        if self.ModuleUniFileName is not None:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_MORE_THAN_ONE_FOUND%(DT.TAB_INF_DEFINES_MODULE_UNI_FILE),
                       LineInfo=self.CurrentLine)
        self.ModuleUniFileName = ModuleUniFileName

    ## GetModuleType
    #
    def GetModuleUniFileName(self):
        return self.ModuleUniFileName

    ## SetInfVersion
    #
    # @param InfVersion: InfVersion
    #
    def SetInfVersion(self, InfVersion, Comments):
        #
        # Value has been set before.
        #
        if self.InfVersion is not None:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_MORE_THAN_ONE_FOUND\
                       %(DT.TAB_INF_DEFINES_INF_VERSION),
                       LineInfo=self.CurrentLine)
            return False
        #
        # The InfVersion should be 4 bytes hex string.
        #
        if (IsValidHex(InfVersion)):
            if (InfVersion < '0x00010005'):
                ErrorInInf(ST.ERR_INF_PARSER_NOT_SUPPORT_EDKI_INF,
                           ErrorCode=ToolError.EDK1_INF_ERROR,
                           LineInfo=self.CurrentLine)
        elif IsValidDecVersionVal(InfVersion):
            if (InfVersion < 65541):
                ErrorInInf(ST.ERR_INF_PARSER_NOT_SUPPORT_EDKI_INF,
                           ErrorCode=ToolError.EDK1_INF_ERROR,
                           LineInfo=self.CurrentLine)
        else:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(InfVersion),
                       LineInfo=self.CurrentLine)
            return False

        self.InfVersion = InfDefMember()
        self.InfVersion.SetValue(InfVersion)
        self.InfVersion.Comments = Comments
        return True

    ## GetInfVersion
    #
    def GetInfVersion(self):
        return self.InfVersion

    ## SetEdkReleaseVersion
    #
    # @param EdkReleaseVersion: EdkReleaseVersion
    #
    def SetEdkReleaseVersion(self, EdkReleaseVersion, Comments):
        #
        # Value has been set before.
        #
        if self.EdkReleaseVersion is not None:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_MORE_THAN_ONE_FOUND\
                       %(DT.TAB_INF_DEFINES_EDK_RELEASE_VERSION),
                       LineInfo=self.CurrentLine)
            return False
        #
        # The EdkReleaseVersion should be 4 bytes hex string.
        #
        if IsValidHexVersion(EdkReleaseVersion) or \
           IsValidDecVersionVal(EdkReleaseVersion):
            self.EdkReleaseVersion = InfDefMember()
            self.EdkReleaseVersion.SetValue(EdkReleaseVersion)
            self.EdkReleaseVersion.Comments = Comments
            return True
        else:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID\
                       %(EdkReleaseVersion),
                       LineInfo=self.CurrentLine)
            return False

    ## GetEdkReleaseVersion
    #
    def GetEdkReleaseVersion(self):
        return self.EdkReleaseVersion

    ## SetUefiSpecificationVersion
    #
    # @param UefiSpecificationVersion: UefiSpecificationVersion
    #
    def SetUefiSpecificationVersion(self, UefiSpecificationVersion, Comments):
        #
        # Value has been set before.
        #
        if self.UefiSpecificationVersion is not None:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_MORE_THAN_ONE_FOUND\
                       %(DT.TAB_INF_DEFINES_UEFI_SPECIFICATION_VERSION),
                       LineInfo=self.CurrentLine)
            return False
        #
        # The EdkReleaseVersion should be 4 bytes hex string.
        #
        if IsValidHexVersion(UefiSpecificationVersion) or \
           IsValidDecVersionVal(UefiSpecificationVersion):
            self.UefiSpecificationVersion = InfDefMember()
            self.UefiSpecificationVersion.SetValue(UefiSpecificationVersion)
            self.UefiSpecificationVersion.Comments = Comments
            return True
        else:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID\
                       %(UefiSpecificationVersion),
                       LineInfo=self.CurrentLine)
            return False

    ## GetUefiSpecificationVersion
    #
    def GetUefiSpecificationVersion(self):
        return self.UefiSpecificationVersion

    ## SetPiSpecificationVersion
    #
    # @param PiSpecificationVersion: PiSpecificationVersion
    #
    def SetPiSpecificationVersion(self, PiSpecificationVersion, Comments):
        #
        # Value has been set before.
        #
        if self.PiSpecificationVersion is not None:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_MORE_THAN_ONE_FOUND\
                       %(DT.TAB_INF_DEFINES_PI_SPECIFICATION_VERSION),
                       LineInfo=self.CurrentLine)
            return False
        #
        # The EdkReleaseVersion should be 4 bytes hex string.
        #
        if IsValidHexVersion(PiSpecificationVersion) or \
           IsValidDecVersionVal(PiSpecificationVersion):
            self.PiSpecificationVersion = InfDefMember()
            self.PiSpecificationVersion.SetValue(PiSpecificationVersion)
            self.PiSpecificationVersion.Comments = Comments
            return True
        else:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID\
                       %(PiSpecificationVersion),
                       LineInfo=self.CurrentLine)
            return False

    ## GetPiSpecificationVersion
    #
    def GetPiSpecificationVersion(self):
        return self.PiSpecificationVersion

    ## SetLibraryClass
    #
    # @param LibraryClass: LibraryClass
    #
    def SetLibraryClass(self, LibraryClass, Comments):
        ValueList = GetSplitValueList(LibraryClass)
        Name = ValueList[0]
        if IsValidWord(Name):
            InfDefineLibraryItemObj = InfDefineLibraryItem()
            InfDefineLibraryItemObj.SetLibraryName(Name)
            InfDefineLibraryItemObj.Comments = Comments
            if len(ValueList) == 2:
                Type = ValueList[1]
                TypeList = GetSplitValueList(Type, ' ')
                TypeList = [Type for Type in TypeList if Type != '']
                for Item in TypeList:
                    if Item not in DT.MODULE_LIST:
                        ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(Item),
                                   LineInfo=self.CurrentLine)
                        return False
                InfDefineLibraryItemObj.SetTypes(TypeList)
            self.LibraryClass.append(InfDefineLibraryItemObj)
        else:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(Name),
                       LineInfo=self.CurrentLine)
            return False

        return True

    def GetLibraryClass(self):
        return self.LibraryClass

    def SetVersionString(self, VersionString, Comments):
        #
        # Value has been set before.
        #
        if self.VersionString is not None:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_MORE_THAN_ONE_FOUND\
                       %(DT.TAB_INF_DEFINES_VERSION_STRING),
                       LineInfo=self.CurrentLine)
            return False
        if not IsValidDecVersion(VersionString):
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID\
                       %(VersionString),
                       LineInfo=self.CurrentLine)
        self.VersionString = InfDefMember()
        self.VersionString.SetValue(VersionString)
        self.VersionString.Comments = Comments
        return True


    def GetVersionString(self):
        return self.VersionString

    def SetPcdIsDriver(self, PcdIsDriver, Comments):
        #
        # Value has been set before.
        #
        if self.PcdIsDriver is not None:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_MORE_THAN_ONE_FOUND\
                       %(DT.TAB_INF_DEFINES_PCD_IS_DRIVER),
                       LineInfo=self.CurrentLine)
            return False
        if PcdIsDriver == 'PEI_PCD_DRIVER' or PcdIsDriver == 'DXE_PCD_DRIVER':
            self.PcdIsDriver = InfDefMember()
            self.PcdIsDriver.SetValue(PcdIsDriver)
            self.PcdIsDriver.Comments = Comments
            return True
        else:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(PcdIsDriver),
                       LineInfo=self.CurrentLine)
            return False

    def GetPcdIsDriver(self):
        return self.PcdIsDriver

    #
    # SetEntryPoint
    #
    def SetEntryPoint(self, EntryPoint, Comments):
        #
        # It can be a list
        #
        ValueList = []
        TokenList = GetSplitValueList(EntryPoint, DT.TAB_VALUE_SPLIT)
        ValueList[0:len(TokenList)] = TokenList
        InfDefineEntryPointItemObj = InfDefineEntryPointItem()
        if not IsValidCVariableName(ValueList[0]):
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%\
                       (ValueList[0]),
                       LineInfo=self.CurrentLine)
        InfDefineEntryPointItemObj.SetCName(ValueList[0])
        if len(ValueList) == 2:
            if ValueList[1].strip() == '':
                ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%\
                           (ValueList[1]),
                           LineInfo=self.CurrentLine)
            #
            # Validate FFE
            #
            FeatureFlagRtv = IsValidFeatureFlagExp(ValueList[1].strip())
            if not FeatureFlagRtv[0]:
                ErrorInInf(ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_SYNTAX_INVLID%\
                           (FeatureFlagRtv[1]),
                           LineInfo=self.CurrentLine)
            InfDefineEntryPointItemObj.SetFeatureFlagExp(ValueList[1])
        if len(ValueList) > 2:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(EntryPoint),
                       LineInfo=self.CurrentLine)
        InfDefineEntryPointItemObj.Comments = Comments
        self.EntryPoint.append(InfDefineEntryPointItemObj)

    def GetEntryPoint(self):
        return self.EntryPoint

    #
    # SetUnloadImages
    #
    def SetUnloadImages(self, UnloadImages, Comments):
        #
        # It can be a list
        #
        ValueList = []
        TokenList = GetSplitValueList(UnloadImages, DT.TAB_VALUE_SPLIT)
        ValueList[0:len(TokenList)] = TokenList
        InfDefineUnloadImageItemObj = InfDefineUnloadImageItem()
        if not IsValidCVariableName(ValueList[0]):
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(ValueList[0]),
                       LineInfo=self.CurrentLine)
        InfDefineUnloadImageItemObj.SetCName(ValueList[0])
        if len(ValueList) == 2:
            if ValueList[1].strip() == '':
                ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(ValueList[1]),
                           LineInfo=self.CurrentLine)
            #
            # Validate FFE
            #
            FeatureFlagRtv = IsValidFeatureFlagExp(ValueList[1].strip())
            if not FeatureFlagRtv[0]:
                ErrorInInf(ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_SYNTAX_INVLID%(FeatureFlagRtv[1]),
                           LineInfo=self.CurrentLine)
            InfDefineUnloadImageItemObj.SetFeatureFlagExp(ValueList[1])

        if len(ValueList) > 2:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(UnloadImages),
                       LineInfo=self.CurrentLine)
        InfDefineUnloadImageItemObj.Comments = Comments
        self.UnloadImages.append(InfDefineUnloadImageItemObj)

    def GetUnloadImages(self):
        return self.UnloadImages

    #
    # SetConstructor
    #
    def SetConstructor(self, Constructor, Comments):
        #
        # It can be a list
        #
        ValueList = []
        TokenList = GetSplitValueList(Constructor, DT.TAB_VALUE_SPLIT)
        ValueList[0:len(TokenList)] = TokenList
        InfDefineConstructorItemObj = InfDefineConstructorItem()
        if not IsValidCVariableName(ValueList[0]):
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(ValueList[0]),
                       LineInfo=self.CurrentLine)
        InfDefineConstructorItemObj.SetCName(ValueList[0])
        if len(ValueList) >= 2:
            ModList = GetSplitValueList(ValueList[1], ' ')
            if ValueList[1].strip() == '':
                ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(ValueList[1]),
                           LineInfo=self.CurrentLine)
            for ModItem in ModList:
                if ModItem not in DT.MODULE_LIST:
                    ErrorInInf(ST.ERR_INF_PARSER_DEFINE_MODULETYPE_INVALID%(ModItem),
                               LineInfo=self.CurrentLine)
            InfDefineConstructorItemObj.SetSupModList(ModList)
        if len(ValueList) == 3:
            if ValueList[2].strip() == '':
                ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(ValueList[2]),
                           LineInfo=self.CurrentLine)
            #
            # Validate FFE
            #
            FeatureFlagRtv = IsValidFeatureFlagExp(ValueList[2].strip())
            if not FeatureFlagRtv[0]:
                ErrorInInf(ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_SYNTAX_INVLID%(FeatureFlagRtv[2]),
                           LineInfo=self.CurrentLine)
            InfDefineConstructorItemObj.SetFeatureFlagExp(ValueList[2])

        if len(ValueList) > 3:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(Constructor),
                       LineInfo=self.CurrentLine)
        InfDefineConstructorItemObj.Comments = Comments
        self.Constructor.append(InfDefineConstructorItemObj)

    def GetConstructor(self):
        return self.Constructor

    #
    # SetDestructor
    #
    def SetDestructor(self, Destructor, Comments):
        #
        # It can be a list and only 1 set to TRUE
        #
        ValueList = []
        TokenList = GetSplitValueList(Destructor, DT.TAB_VALUE_SPLIT)
        ValueList[0:len(TokenList)] = TokenList
        InfDefineDestructorItemObj = InfDefineDestructorItem()
        if not IsValidCVariableName(ValueList[0]):
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(ValueList[0]),
                       LineInfo=self.CurrentLine)
        InfDefineDestructorItemObj.SetCName(ValueList[0])
        if len(ValueList) >= 2:
            ModList = GetSplitValueList(ValueList[1].strip(), ' ')
            if ValueList[1].strip() == '':
                ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(ValueList[1]),
                           LineInfo=self.CurrentLine)
            for ModItem in ModList:
                if ModItem not in DT.MODULE_LIST:
                    ErrorInInf(ST.ERR_INF_PARSER_DEFINE_MODULETYPE_INVALID%(ModItem),
                               LineInfo=self.CurrentLine)
            InfDefineDestructorItemObj.SetSupModList(ModList)
        if len(ValueList) == 3:
            if ValueList[2].strip() == '':
                ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(ValueList[2]),
                           LineInfo=self.CurrentLine)
            #
            # Validate FFE
            #
            FeatureFlagRtv = IsValidFeatureFlagExp(ValueList[2].strip())
            if not FeatureFlagRtv[0]:
                ErrorInInf(ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_SYNTAX_INVLID%(FeatureFlagRtv[1]),
                           LineInfo=self.CurrentLine)
            InfDefineDestructorItemObj.SetFeatureFlagExp(ValueList[2])

        if len(ValueList) > 3:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(Destructor),
                       LineInfo=self.CurrentLine)

        InfDefineDestructorItemObj.Comments = Comments
        self.Destructor.append(InfDefineDestructorItemObj)

    def GetDestructor(self):
        return self.Destructor

    def SetShadow(self, Shadow, Comments):
        #
        # Value has been set before.
        #
        if self.Shadow is not None:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_MORE_THAN_ONE_FOUND%(DT.TAB_INF_DEFINES_SHADOW),
                       LineInfo=self.CurrentLine)
            return False
        if (IsValidBoolType(Shadow)):
            self.Shadow = InfDefMember()
            self.Shadow.SetValue(Shadow)
            self.Shadow.Comments = Comments
            return True
        else:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(Shadow),
                       LineInfo=self.CurrentLine)
            return False
    def GetShadow(self):
        return self.Shadow

    #
    # <Family>               ::=  {"MSFT"} {"GCC"}
    # <CustomMake>           ::=  [<Family> "|"] <Filename>
    #
    def SetCustomMakefile(self, CustomMakefile, Comments):
        if not (CustomMakefile == '' or CustomMakefile is None):
            ValueList = GetSplitValueList(CustomMakefile)
            if len(ValueList) == 1:
                FileName = ValueList[0]
                Family = ''
            else:
                Family = ValueList[0]
                FileName = ValueList[1]
            Family = Family.strip()
            if Family != '':
                if not IsValidFamily(Family):
                    ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(Family),
                               LineInfo=self.CurrentLine)
                    return False
            #
            # The MakefileName specified file should exist
            #
            IsValidFileFlag = False
            ModulePath = os.path.split(self.CurrentLine[0])[0]
            if IsValidPath(FileName, ModulePath):
                IsValidFileFlag = True
            else:
                ErrorInInf(ST.ERR_INF_PARSER_FILE_NOT_EXIST_OR_NAME_INVALID%(FileName),
                           LineInfo=self.CurrentLine)
                return False
            if IsValidFileFlag:
                FileName = ConvPathFromAbsToRel(FileName, GlobalData.gINF_MODULE_DIR)
                self.CustomMakefile.append((Family, FileName, Comments))
                IsValidFileFlag = False
            return True
        else:
            return False

    def GetCustomMakefile(self):
        return self.CustomMakefile

    #
    # ["SPEC" <Spec> <EOL>]*{0,}
    # <Spec>                 ::=  <Word> "=" <VersionVal>
    # <VersionVal>           ::=  {<HexVersion>] {<DecVersion>}
    # <HexNumber>            ::=  "0x" [<HexDigit>]{1,}
    # <DecVersion>           ::=  (0-9){1,} ["." (0-9){1,2}]
    #
    def SetSpecification(self, Specification, Comments):
        #
        # Valid the value of Specification
        #
        __ValueList = []
        TokenList = GetSplitValueList(Specification, DT.TAB_EQUAL_SPLIT, 1)
        __ValueList[0:len(TokenList)] = TokenList
        if len(__ValueList) != 2:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_NO_NAME + ' Or ' + ST.ERR_INF_PARSER_DEFINE_ITEM_NO_VALUE,
                       LineInfo=self.CurrentLine)
        Name = __ValueList[0].strip()
        Version = __ValueList[1].strip()
        if IsValidIdentifier(Name):
            if IsValidDecVersion(Version):
                self.Specification.append((Name, Version, Comments))
                return True
            else:
                ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(Version),
                           LineInfo=self.CurrentLine)
                return False
        else:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(Name),
                       LineInfo=self.CurrentLine)
            return False
        return True

    def GetSpecification(self):
        return self.Specification

    #
    # [<UefiHiiResource> <EOL>]{0,1}
    # <UefiHiiResource>      ::=  "UEFI_HII_RESOURCE_SECTION" "=" <BoolType>
    #
    def SetUefiHiiResourceSection(self, UefiHiiResourceSection, Comments):
        #
        # Value has been set before.
        #
        if self.UefiHiiResourceSection is not None:
            ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_MORE_THAN_ONE_FOUND
                       %(DT.TAB_INF_DEFINES_UEFI_HII_RESOURCE_SECTION),
                       LineInfo=self.CurrentLine)
            return False
        if not (UefiHiiResourceSection == '' or UefiHiiResourceSection is None):
            if (IsValidBoolType(UefiHiiResourceSection)):
                self.UefiHiiResourceSection = InfDefMember()
                self.UefiHiiResourceSection.SetValue(UefiHiiResourceSection)
                self.UefiHiiResourceSection.Comments = Comments
                return True
            else:
                ErrorInInf(ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID%(UefiHiiResourceSection),
                           LineInfo=self.CurrentLine)
                return False
        else:
            return False

    def GetUefiHiiResourceSection(self):
        return self.UefiHiiResourceSection

    def SetDpxSource(self, DpxSource, Comments):
        #
        # The MakefileName specified file should exist
        #
        IsValidFileFlag = False
        ModulePath = os.path.split(self.CurrentLine[0])[0]
        if IsValidPath(DpxSource, ModulePath):
            IsValidFileFlag = True
        else:
            ErrorInInf(ST.ERR_INF_PARSER_FILE_NOT_EXIST_OR_NAME_INVALID%(DpxSource),
                       LineInfo=self.CurrentLine)
            return False
        if IsValidFileFlag:
            DpxSource = ConvPathFromAbsToRel(DpxSource,
                            GlobalData.gINF_MODULE_DIR)
            self.DpxSource.append((DpxSource, Comments))
            IsValidFileFlag = False
        return True

    def GetDpxSource(self):
        return self.DpxSource

gFUNCTION_MAPPING_FOR_DEFINE_SECTION = {
    #
    # Required Fields
    #
    DT.TAB_INF_DEFINES_BASE_NAME                   : InfDefSection.SetBaseName,
    DT.TAB_INF_DEFINES_FILE_GUID                   : InfDefSection.SetFileGuid,
    DT.TAB_INF_DEFINES_MODULE_TYPE                 : InfDefSection.SetModuleType,
    #
    # Required by EDKII style INF file
    #
    DT.TAB_INF_DEFINES_INF_VERSION                 : InfDefSection.SetInfVersion,
    #
    # Optional Fields
    #
    DT.TAB_INF_DEFINES_MODULE_UNI_FILE             : InfDefSection.SetModuleUniFileName,
    DT.TAB_INF_DEFINES_EDK_RELEASE_VERSION         : InfDefSection.SetEdkReleaseVersion,
    DT.TAB_INF_DEFINES_UEFI_SPECIFICATION_VERSION  : InfDefSection.SetUefiSpecificationVersion,
    DT.TAB_INF_DEFINES_PI_SPECIFICATION_VERSION    : InfDefSection.SetPiSpecificationVersion,
    DT.TAB_INF_DEFINES_LIBRARY_CLASS               : InfDefSection.SetLibraryClass,
    DT.TAB_INF_DEFINES_VERSION_STRING              : InfDefSection.SetVersionString,
    DT.TAB_INF_DEFINES_PCD_IS_DRIVER               : InfDefSection.SetPcdIsDriver,
    DT.TAB_INF_DEFINES_ENTRY_POINT                 : InfDefSection.SetEntryPoint,
    DT.TAB_INF_DEFINES_UNLOAD_IMAGE                : InfDefSection.SetUnloadImages,
    DT.TAB_INF_DEFINES_CONSTRUCTOR                 : InfDefSection.SetConstructor,
    DT.TAB_INF_DEFINES_DESTRUCTOR                  : InfDefSection.SetDestructor,
    DT.TAB_INF_DEFINES_SHADOW                      : InfDefSection.SetShadow,
    DT.TAB_INF_DEFINES_PCI_VENDOR_ID               : InfDefSection.SetPciVendorId,
    DT.TAB_INF_DEFINES_PCI_DEVICE_ID               : InfDefSection.SetPciDeviceId,
    DT.TAB_INF_DEFINES_PCI_CLASS_CODE              : InfDefSection.SetPciClassCode,
    DT.TAB_INF_DEFINES_PCI_REVISION                : InfDefSection.SetPciRevision,
    DT.TAB_INF_DEFINES_PCI_COMPRESS                : InfDefSection.SetPciCompress,
    DT.TAB_INF_DEFINES_CUSTOM_MAKEFILE             : InfDefSection.SetCustomMakefile,
    DT.TAB_INF_DEFINES_SPEC                        : InfDefSection.SetSpecification,
    DT.TAB_INF_DEFINES_UEFI_HII_RESOURCE_SECTION   : InfDefSection.SetUefiHiiResourceSection,
    DT.TAB_INF_DEFINES_DPX_SOURCE                  : InfDefSection.SetDpxSource
}

## InfDefMember
#
#
class InfDefMember():
    def __init__(self, Name='', Value=''):
        self.Comments = InfLineCommentObject()
        self.Name  = Name
        self.Value = Value
        self.CurrentLine = CurrentLine()
    def GetName(self):
        return self.Name
    def SetName(self, Name):
        self.Name = Name
    def GetValue(self):
        return self.Value
    def SetValue(self, Value):
        self.Value = Value

## InfDefObject
#
#
class InfDefObject(InfSectionCommonDef):
    def __init__(self):
        self.Defines = Sdict()
        InfSectionCommonDef.__init__(self)
    def SetDefines(self, DefineContent, Arch = None):
        #
        # Validate Arch
        #
        HasFoundInfVersionFalg = False
        LineInfo = ['', -1, '']
        ArchListString = ' '.join(Arch)
        #
        # Parse Define items.
        #
        for InfDefMemberObj in DefineContent:
            ProcessFunc = None
            Name = InfDefMemberObj.GetName()
            Value = InfDefMemberObj.GetValue()
            if Name == DT.TAB_INF_DEFINES_MODULE_UNI_FILE:
                ValidateUNIFilePath(Value)
                Value = os.path.join(os.path.dirname(InfDefMemberObj.CurrentLine.FileName), Value)
                if not os.path.isfile(Value) or not os.path.exists(Value):
                    LineInfo[0] = InfDefMemberObj.CurrentLine.GetFileName()
                    LineInfo[1] = InfDefMemberObj.CurrentLine.GetLineNo()
                    LineInfo[2] = InfDefMemberObj.CurrentLine.GetLineString()
                    ErrorInInf(ST.ERR_INF_PARSER_FILE_NOT_EXIST_OR_NAME_INVALID%(Name),
                                   LineInfo=LineInfo)
            InfLineCommentObj = InfLineCommentObject()
            InfLineCommentObj.SetHeaderComments(InfDefMemberObj.Comments.GetHeaderComments())
            InfLineCommentObj.SetTailComments(InfDefMemberObj.Comments.GetTailComments())
            if Name == 'COMPONENT_TYPE':
                ErrorInInf(ST.ERR_INF_PARSER_NOT_SUPPORT_EDKI_INF,
                           ErrorCode=ToolError.EDK1_INF_ERROR,
                           RaiseError=True)
            if Name == DT.TAB_INF_DEFINES_INF_VERSION:
                HasFoundInfVersionFalg = True
            if not (Name == '' or Name is None):
                #
                # Process "SPEC" Keyword definition.
                #
                ReName = re.compile(r"SPEC ", re.DOTALL)
                if ReName.match(Name):
                    SpecValue = Name[Name.find("SPEC") + len("SPEC"):].strip()
                    Name = "SPEC"
                    Value = SpecValue + " = " + Value
                if ArchListString in self.Defines:
                    DefineList = self.Defines[ArchListString]
                    LineInfo[0] = InfDefMemberObj.CurrentLine.GetFileName()
                    LineInfo[1] = InfDefMemberObj.CurrentLine.GetLineNo()
                    LineInfo[2] = InfDefMemberObj.CurrentLine.GetLineString()
                    DefineList.CurrentLine = LineInfo
                    #
                    # Found the process function from mapping table.
                    #
                    if Name not in gFUNCTION_MAPPING_FOR_DEFINE_SECTION.keys():
                        ErrorInInf(ST.ERR_INF_PARSER_DEFINE_SECTION_KEYWORD_INVALID%(Name),
                                   LineInfo=LineInfo)
                    else:
                        ProcessFunc = gFUNCTION_MAPPING_FOR_DEFINE_SECTION[Name]
                    if (ProcessFunc is not None):
                        ProcessFunc(DefineList, Value, InfLineCommentObj)
                    self.Defines[ArchListString] = DefineList
                else:
                    DefineList = InfDefSection()
                    LineInfo[0] = InfDefMemberObj.CurrentLine.GetFileName()
                    LineInfo[1] = InfDefMemberObj.CurrentLine.GetLineNo()
                    LineInfo[2] = InfDefMemberObj.CurrentLine.GetLineString()
                    DefineList.CurrentLine = LineInfo
                    #
                    # Found the process function from mapping table.
                    #
                    if Name not in gFUNCTION_MAPPING_FOR_DEFINE_SECTION.keys():
                        ErrorInInf(ST.ERR_INF_PARSER_DEFINE_SECTION_KEYWORD_INVALID%(Name),
                                   LineInfo=LineInfo)
                    #
                    # Found the process function from mapping table.
                    #
                    else:
                        ProcessFunc = gFUNCTION_MAPPING_FOR_DEFINE_SECTION[Name]
                    if (ProcessFunc is not None):
                        ProcessFunc(DefineList, Value, InfLineCommentObj)
                    self.Defines[ArchListString] = DefineList
        #
        # After set, check whether INF_VERSION defined.
        #
        if not HasFoundInfVersionFalg:
            ErrorInInf(ST.ERR_INF_PARSER_NOT_SUPPORT_EDKI_INF,
                       ErrorCode=ToolError.EDK1_INF_ERROR,
                       RaiseError=True)
        return True

    def GetDefines(self):
        return self.Defines

