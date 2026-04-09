## @file
# section base class
#
#  Copyright (c) 2007-2018, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
from CommonDataClass.FdfClass import SectionClassObject
from .GenFdsGlobalVariable import GenFdsGlobalVariable
import Common.LongFilePathOs as os, glob
from Common import EdkLogger
from Common.BuildToolError import *
from Common.DataType import *

## section base class
#
#
class Section (SectionClassObject):
    SectionType = {
        'RAW'       : 'EFI_SECTION_RAW',
        'FREEFORM'  : 'EFI_SECTION_FREEFORM_SUBTYPE_GUID',
        BINARY_FILE_TYPE_PE32      : 'EFI_SECTION_PE32',
        BINARY_FILE_TYPE_PIC       : 'EFI_SECTION_PIC',
        BINARY_FILE_TYPE_TE        : 'EFI_SECTION_TE',
        'FV_IMAGE'  : 'EFI_SECTION_FIRMWARE_VOLUME_IMAGE',
        'COMPAT16'  : 'EFI_SECTION_COMPATIBILITY16',
        BINARY_FILE_TYPE_DXE_DEPEX : 'EFI_SECTION_DXE_DEPEX',
        BINARY_FILE_TYPE_PEI_DEPEX : 'EFI_SECTION_PEI_DEPEX',
        'GUIDED'    : 'EFI_SECTION_GUID_DEFINED',
        'COMPRESS'  : 'EFI_SECTION_COMPRESSION',
        BINARY_FILE_TYPE_UI        : 'EFI_SECTION_USER_INTERFACE',
        BINARY_FILE_TYPE_SMM_DEPEX : 'EFI_SECTION_SMM_DEPEX'
    }

    BinFileType = {
        BINARY_FILE_TYPE_GUID          : '.guid',
        'ACPI'          : '.acpi',
        'ASL'           : '.asl' ,
        BINARY_FILE_TYPE_UEFI_APP      : '.app',
        BINARY_FILE_TYPE_LIB           : '.lib',
        BINARY_FILE_TYPE_PE32          : '.pe32',
        BINARY_FILE_TYPE_PIC           : '.pic',
        BINARY_FILE_TYPE_PEI_DEPEX     : '.depex',
        'SEC_PEI_DEPEX' : '.depex',
        BINARY_FILE_TYPE_TE            : '.te',
        BINARY_FILE_TYPE_UNI_VER       : '.ver',
        BINARY_FILE_TYPE_VER           : '.ver',
        BINARY_FILE_TYPE_UNI_UI        : '.ui',
        BINARY_FILE_TYPE_UI            : '.ui',
        BINARY_FILE_TYPE_BIN           : '.bin',
        'RAW'           : '.raw',
        'COMPAT16'      : '.comp16',
        BINARY_FILE_TYPE_FV            : '.fv'
    }

    SectFileType = {
        'SEC_GUID'      : '.sec' ,
        'SEC_PE32'      : '.sec' ,
        'SEC_PIC'       : '.sec',
        'SEC_TE'        : '.sec',
        'SEC_VER'       : '.sec',
        'SEC_UI'        : '.sec',
        'SEC_COMPAT16'  : '.sec',
        'SEC_BIN'       : '.sec'
    }

    ToolGuid = {
        '0xa31280ad-0x481e-0x41b6-0x95e8-0x127f-0x4c984779' : 'TianoCompress',
        '0xee4e5898-0x3914-0x4259-0x9d6e-0xdc7b-0xd79403cf' : 'LzmaCompress'
    }

    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        SectionClassObject.__init__(self)

    ## GenSection() method
    #
    #   virtual function
    #
    #   @param  self        The object pointer
    #   @param  OutputPath  Where to place output file
    #   @param  ModuleName  Which module this section belongs to
    #   @param  SecNum      Index of section
    #   @param  KeyStringList  Filter for inputs of section generation
    #   @param  FfsInf      FfsInfStatement object that contains this section data
    #   @param  Dict        dictionary contains macro and its value
    #
    def GenSection(self, OutputPath, GuidName, SecNum, keyStringList, FfsInf = None, Dict = None):
        pass

    ## GetFileList() method
    #
    #   Generate compressed section
    #
    #   @param  self        The object pointer
    #   @param  FfsInf      FfsInfStatement object that contains file list
    #   @param  FileType    File type to get
    #   @param  FileExtension  File extension to get
    #   @param  Dict        dictionary contains macro and its value
    #   @retval tuple       (File list, boolean)
    #
    def GetFileList(FfsInf, FileType, FileExtension, Dict = None, IsMakefile=False, SectionType=None):
        IsSect = FileType in Section.SectFileType

        if FileExtension is not None:
            Suffix = FileExtension
        elif IsSect :
            Suffix = Section.SectionType.get(FileType)
        else:
            Suffix = Section.BinFileType.get(FileType)
        if FfsInf is None:
            EdkLogger.error("GenFds", GENFDS_ERROR, 'Inf File does not exist!')

        FileList = []
        if FileType is not None:
            for File in FfsInf.BinFileList:
                if File.Arch == TAB_ARCH_COMMON or FfsInf.CurrentArch == File.Arch:
                    if File.Type == FileType or (int(FfsInf.PiSpecVersion, 16) >= 0x0001000A \
                                                 and FileType == 'DXE_DPEX' and File.Type == BINARY_FILE_TYPE_SMM_DEPEX) \
                                                 or (FileType == BINARY_FILE_TYPE_TE and File.Type == BINARY_FILE_TYPE_PE32):
                        if TAB_STAR in FfsInf.TargetOverrideList or File.Target == TAB_STAR or File.Target in FfsInf.TargetOverrideList or FfsInf.TargetOverrideList == []:
                            FileList.append(FfsInf.PatchEfiFile(File.Path, File.Type))
                        else:
                            GenFdsGlobalVariable.InfLogger ("\nBuild Target \'%s\' of File %s is not in the Scope of %s specified by INF %s in FDF" %(File.Target, File.File, FfsInf.TargetOverrideList, FfsInf.InfFileName))
                    else:
                        GenFdsGlobalVariable.VerboseLogger ("\nFile Type \'%s\' of File %s in %s is not same with file type \'%s\' from Rule in FDF" %(File.Type, File.File, FfsInf.InfFileName, FileType))
                else:
                    GenFdsGlobalVariable.InfLogger ("\nCurrent ARCH \'%s\' of File %s is not in the Support Arch Scope of %s specified by INF %s in FDF" %(FfsInf.CurrentArch, File.File, File.Arch, FfsInf.InfFileName))

        elif FileType is None and SectionType == BINARY_FILE_TYPE_RAW:
            for File in FfsInf.BinFileList:
                if File.Ext == Suffix:
                    FileList.append(File.Path)

        if (not IsMakefile and Suffix is not None and os.path.exists(FfsInf.EfiOutputPath)) or (IsMakefile and Suffix is not None):
            if not FileList:
                SuffixMap = FfsInf.GetFinalTargetSuffixMap()
                if Suffix in SuffixMap:
                    FileList.extend(SuffixMap[Suffix])

        #Process the file lists is alphabetical for a same section type
        if len (FileList) > 1:
            FileList.sort()

        return FileList, IsSect
    GetFileList = staticmethod(GetFileList)
