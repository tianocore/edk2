## @file InfPomAlignmentMisc.py
# This file contained the routines for InfPomAlignment
#
# Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available 
# under the terms and conditions of the BSD License which accompanies this 
# distribution. The full text of the license may be found at 
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

'''
InfPomAlignmentMisc
'''

##
# Import modules
#
import Logger.Log as Logger
from Library import DataType as DT
from Library.Misc import ConvertArchList
from Object.POM.ModuleObject import BinaryFileObject
from Object.POM import CommonObject

## GenModuleHeaderUserExt
#
#
def GenModuleHeaderUserExt(DefineObj, ArchString):
    DefinesDictNew = {}
    EdkReleaseVersion = DefineObj.GetEdkReleaseVersion()
    Shadow = DefineObj.GetShadow()
    DpxSource = DefineObj.GetDpxSource()
    PciVendorId = DefineObj.GetPciVendorId()
    PciDeviceId = DefineObj.GetPciDeviceId()
    PciClassCode = DefineObj.GetPciClassCode()
    PciRevision = DefineObj.GetPciRevision()
    PciCompress = DefineObj.GetPciCompress()
    CustomMakefile = DefineObj.GetCustomMakefile()
    UefiHiiResourceSection = DefineObj.GetUefiHiiResourceSection()

    if EdkReleaseVersion != None:
        Name = DT.TAB_INF_DEFINES_EDK_RELEASE_VERSION
        Value = EdkReleaseVersion.GetValue()
        Statement = _GenInfDefineStateMent(EdkReleaseVersion.Comments.GetHeaderComments(),
                                           Name,
                                           Value,
                                           EdkReleaseVersion.Comments.GetTailComments())
        DefinesDictNew[Statement] = ArchString

    if Shadow != None:
        Name = DT.TAB_INF_DEFINES_SHADOW
        Value = Shadow.GetValue()
        Statement = _GenInfDefineStateMent(Shadow.Comments.GetHeaderComments(),
                                           Name,
                                           Value,
                                           Shadow.Comments.GetTailComments())
        DefinesDictNew[Statement] = ArchString

    if DpxSource != None:
        Name = DT.TAB_INF_DEFINES_DPX_SOURCE
        for DpxSourceItem in DpxSource:
            Value = DpxSourceItem[0]
            Statement = _GenInfDefineStateMent(DpxSourceItem[1].GetHeaderComments(),
                                               Name,
                                               Value,
                                               DpxSourceItem[1].GetTailComments())
            DefinesDictNew[Statement] = ArchString

    if PciVendorId != None:
        Name = DT.TAB_INF_DEFINES_PCI_VENDOR_ID
        Value = PciVendorId.GetValue()
        Statement = _GenInfDefineStateMent(PciVendorId.Comments.GetHeaderComments(),
                                           Name,
                                           Value,
                                           PciVendorId.Comments.GetTailComments())
        DefinesDictNew[Statement] = ArchString

    if PciDeviceId != None:
        Name = DT.TAB_INF_DEFINES_PCI_DEVICE_ID
        Value = PciDeviceId.GetValue()
        Statement = _GenInfDefineStateMent(PciDeviceId.Comments.GetHeaderComments(),
                                           Name,
                                           Value,
                                           PciDeviceId.Comments.GetTailComments())
        DefinesDictNew[Statement] = ArchString

    if PciClassCode != None:
        Name = DT.TAB_INF_DEFINES_PCI_CLASS_CODE
        Value = PciClassCode.GetValue()
        Statement = _GenInfDefineStateMent(PciClassCode.Comments.GetHeaderComments(),
                                           Name,
                                           Value,
                                           PciClassCode.Comments.GetTailComments())
        DefinesDictNew[Statement] = ArchString

    if PciRevision != None:
        Name = DT.TAB_INF_DEFINES_PCI_REVISION
        Value = PciRevision.GetValue()
        Statement = _GenInfDefineStateMent(PciRevision.Comments.GetHeaderComments(),
                                           Name,
                                           Value,
                                           PciRevision.Comments.GetTailComments())
        DefinesDictNew[Statement] = ArchString

    if PciCompress != None:
        Name = DT.TAB_INF_DEFINES_PCI_COMPRESS
        Value = PciCompress.GetValue()
        Statement = _GenInfDefineStateMent(PciCompress.Comments.GetHeaderComments(),
                                           Name,
                                           Value,
                                           PciCompress.Comments.GetTailComments())
        DefinesDictNew[Statement] = ArchString

    if len(CustomMakefile) >= 1:
        for CustomMakefileItem in CustomMakefile:
            Name = DT.TAB_INF_DEFINES_CUSTOM_MAKEFILE
            #
            # Not with Feature Flag Expression
            #
            if len(CustomMakefileItem) == 3:
                if CustomMakefileItem[0] != '':
                    Value = CustomMakefileItem[0] + ' | ' + CustomMakefileItem[1]
                else:
                    Value = CustomMakefileItem[1]

                Comments = CustomMakefileItem[2]
                Statement = _GenInfDefineStateMent(Comments.GetHeaderComments(),
                                                   Name,
                                                   Value,
                                                   Comments.GetTailComments())

            DefinesDictNew[Statement] = ArchString

    if UefiHiiResourceSection != None:
        Name = DT.TAB_INF_DEFINES_UEFI_HII_RESOURCE_SECTION
        Value = UefiHiiResourceSection.GetValue()
        HeaderComment = UefiHiiResourceSection.Comments.GetHeaderComments()
        TailComment = UefiHiiResourceSection.Comments.GetTailComments()
        Statement = _GenInfDefineStateMent(HeaderComment,
                                           Name,
                                           Value,
                                           TailComment)
        DefinesDictNew[Statement] = ""

    return DefinesDictNew


## Generate the define statement that will be put into userextension
#  Not support comments.
#
# @param HeaderComment: the original header comment (# not remvoed)
# @param Name: the definition keyword, should not be empty or none
# @param Value: the definition keyword value
# @param TailComment: the original Tail comment (# not remvoed)
#
# @return: the regenerated define statement
#
def _GenInfDefineStateMent(HeaderComment, Name, Value, TailComment):
    Logger.Debug(5, HeaderComment + TailComment)
    Statement = '%s = %s' % (Name, Value)

    return Statement

## GenBinaryData
#
#
def GenBinaryData(BinaryData, BinaryObj, BinariesDict, AsBuildIns, BinaryFileObjectList, SupArchList, BinaryModule):
    if BinaryModule:
        pass
    OriSupArchList = SupArchList
    for Item in BinaryData:
        ItemObj = BinaryObj[Item][0][0]
        if ItemObj.GetType() not in DT.BINARY_FILE_TYPE_UI_LIST + DT.BINARY_FILE_TYPE_VER_LIST:
            TagName = ItemObj.GetTagName()
            Family = ItemObj.GetFamily()
        else:
            TagName = ''
            Family = ''
        FFE = ItemObj.GetFeatureFlagExp()

        #
        # If have architecturie specified, then use the specified architecturie;
        # If the section tag does not have an architecture modifier or the modifier is "common" (case in-sensitive),
        # and the VALID_ARCHITECTURES comment exists, the list from the VALID_ARCHITECTURES comment 
        # can be used for the attribute.
        # If both not have VALID_ARCHITECTURE comment and no architecturie specified, then keep it empty.
        #        
        SupArchList = ConvertArchList(ItemObj.GetSupArchList())
        SupArchList.sort()
        if len(SupArchList) == 1 and SupArchList[0] == 'COMMON':
            if not (len(OriSupArchList) == 1 or OriSupArchList[0] == 'COMMON'):
                SupArchList = OriSupArchList
            else:
                SupArchList = ['COMMON']

        FileNameObj = CommonObject.FileNameObject()
        FileNameObj.SetFileType(ItemObj.GetType())
        FileNameObj.SetFilename(ItemObj.GetFileName())
        FileNameObj.SetFeatureFlag(FFE)
        FileNameObj.SetSupArchList(SupArchList)
        FileNameList = [FileNameObj]

        BinaryFile = BinaryFileObject()
        BinaryFile.SetFileNameList(FileNameList)
        BinaryFile.SetAsBuiltList(AsBuildIns)
        BinaryFileObjectList.append(BinaryFile)

        SupArchStr = ' '.join(SupArchList)
        Key = (ItemObj.GetFileName(), ItemObj.GetType(), FFE, SupArchStr)
        ValueItem = (ItemObj.GetTarget(), Family, TagName, '')
        if Key in BinariesDict:
            ValueList = BinariesDict[Key]
            ValueList.append(ValueItem)
            BinariesDict[Key] = ValueList
        else:
            BinariesDict[Key] = [ValueItem]

    return BinariesDict, AsBuildIns, BinaryFileObjectList
