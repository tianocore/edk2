## @file
# This file is used to parse meta files
#
# Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
# (C) Copyright 2015-2018 Hewlett Packard Enterprise Development LP<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
from CommonDataClass.DataClass import *
from Parser.MetaFileTable import MetaFileStorage
from build_objects.dsc import *
from Common.DataType import *
from Common.StringUtils import NormPath,GetSplitValueList
from Common.BuildToolError import *
from Common.Misc import PathClass
import Common.GlobalData as GlobalData
import uuid
from Parser.MetaFileParser import DscParser
from Common.Misc import ProcessDuplicatedInf

_PCD_TYPE_STRING_ = {
        MODEL_PCD_FIXED_AT_BUILD        :   TAB_PCDS_FIXED_AT_BUILD,
        MODEL_PCD_PATCHABLE_IN_MODULE   :   TAB_PCDS_PATCHABLE_IN_MODULE,
        MODEL_PCD_FEATURE_FLAG          :   TAB_PCDS_FEATURE_FLAG,
        MODEL_PCD_DYNAMIC               :   TAB_PCDS_DYNAMIC,
        MODEL_PCD_DYNAMIC_DEFAULT       :   TAB_PCDS_DYNAMIC,
        MODEL_PCD_DYNAMIC_HII           :   TAB_PCDS_DYNAMIC_HII,
        MODEL_PCD_DYNAMIC_VPD           :   TAB_PCDS_DYNAMIC_VPD,
        MODEL_PCD_DYNAMIC_EX            :   TAB_PCDS_DYNAMIC_EX,
        MODEL_PCD_DYNAMIC_EX_DEFAULT    :   TAB_PCDS_DYNAMIC_EX,
        MODEL_PCD_DYNAMIC_EX_HII        :   TAB_PCDS_DYNAMIC_EX_HII,
        MODEL_PCD_DYNAMIC_EX_VPD        :   TAB_PCDS_DYNAMIC_EX_VPD,
    }

class DscRecipe(object):
    def __init__(self, file_path, arch, workspace, packages_path=None):
        self.dsc_file_path = PathClass(file_path,workspace)
        self.workspace = PathClass(workspace)
        self.packages_path = packages_path if packages_path else []
        self._Arch = arch
        self.dsc_parser = DscParser(
                            self.dsc_file_path,
                            MODEL_FILE_DSC,
                            self._Arch,
                            MetaFileStorage(self.dsc_file_path, MODEL_FILE_DSC)
                            )
        self.dsc_parser.DoPostProcess()
        self.dsc_recipe = dsc(self.dsc_file_path)
        self.parsed = False
        GlobalData.gGlobalDefines['WORKSPACE'] = workspace
        GlobalData.gGlobalDefines['TOOL_CHAIN_TAG'] = 'VS2015x86'
        GlobalData.gWorkspace = workspace
        self._MacroDict = None
        self._NullLibraryNumber = 0

    def get(self):
        if not self.parsed:
            self.init_recipe()
        return self.dsc_recipe

    def init_recipe(self):
        self.LoadDefines(self.dsc_recipe.defines)
        self.LoadComponents(self.dsc_recipe.components)

    ## Get current effective macros
    @property
    def _Macros(self):
        if self._MacroDict is None:
            self._MacroDict = {}
            self._MacroDict.update(GlobalData.gPlatformDefines)
            self._MacroDict.update(GlobalData.gGlobalDefines)
            self._MacroDict.update(GlobalData.gCommandLineDefines)
        return self._MacroDict

    def LoadDefines(self, target):
        def_set = target
        RecordList = self.dsc_parser[MODEL_META_DATA_HEADER, self._Arch]
        for Record in RecordList:
            Name = Record[1]
            # items defined _PROPERTY_ don't need additional processing
            # some special items in [Defines] section need special treatment
            if Name == TAB_DSC_DEFINES_OUTPUT_DIRECTORY:
                OutputDirectory = NormPath(Record[2], self._Macros)
                if ' ' in OutputDirectory:
                    EdkLogger.error("build", FORMAT_NOT_SUPPORTED, "No space is allowed in OUTPUT_DIRECTORY",
                                    File=self.MetaFile, Line=Record[-1],
                                    ExtraData=OutputDirectory)
                def_set.add(definition(Name,OutputDirectory,source_info=self.dsc_file_path))
            elif Name == TAB_DSC_DEFINES_FLASH_DEFINITION:
                FlashDefinition = PathClass(NormPath(Record[2], self._Macros), GlobalData.gWorkspace)
                ErrorCode, ErrorInfo = FlashDefinition.Validate('.fdf')
                if ErrorCode != 0:
                    EdkLogger.error('build', ErrorCode, File=self.MetaFile, Line=Record[-1],
                                    ExtraData=ErrorInfo)
                def_set.add(definition(Name,FlashDefinition,source_info=self.dsc_file_path))
            elif Name == TAB_DSC_PREBUILD:
                PrebuildValue = Record[2]
                if Record[2][0] == '"':
                    if Record[2][-1] != '"':
                        EdkLogger.error('build', FORMAT_INVALID, 'Missing double quotes in the end of %s statement.' % TAB_DSC_PREBUILD,
                                    File=self.MetaFile, Line=Record[-1])
                    PrebuildValue = Record[2][1:-1]
                def_set.add(definition(Name,PrebuildValue,source_info=self.dsc_file_path))
            elif Name == TAB_DSC_POSTBUILD:
                PostbuildValue = Record[2]
                if Record[2][0] == '"':
                    if Record[2][-1] != '"':
                        EdkLogger.error('build', FORMAT_INVALID, 'Missing double quotes in the end of %s statement.' % TAB_DSC_POSTBUILD,
                                    File=self.MetaFile, Line=Record[-1])
                    PostbuildValue = Record[2][1:-1]
                def_set.add(definition(Name,PostbuildValue,source_info=self.dsc_file_path))
            elif Name == TAB_DSC_DEFINES_SUPPORTED_ARCHITECTURES:
                def_set.add(definition(Name,GetSplitValueList(Record[2], TAB_VALUE_SPLIT),source_info=self.dsc_file_path))
            elif Name == TAB_DSC_DEFINES_BUILD_TARGETS:
                def_set.add(definition(Name,GetSplitValueList(Record[2]),source_info=self.dsc_file_path))
            elif Name == TAB_DSC_DEFINES_SKUID_IDENTIFIER:
                if GlobalData.gSKUID_CMD:
                    def_set.add(definition(Name,GlobalData.gSKUID_CMD,source_info=self.dsc_file_path))
                else:
                    def_set.add(definition(Name,Record[2],source_info=self.dsc_file_path))
            elif Name == TAB_DSC_DEFINES_PCD_INFO_GENERATION:
                def_set.add(definition(Name,Record[2],source_info=self.dsc_file_path))
            elif Name == TAB_DSC_DEFINES_PCD_VAR_CHECK_GENERATION:
                def_set.add(definition(Name,Record[2],source_info=self.dsc_file_path))
            elif Name == TAB_FIX_LOAD_TOP_MEMORY_ADDRESS:
                try:
                    def_set.add(definition(Name,int (Record[2], 0),source_info=self.dsc_file_path))
                except:
                    EdkLogger.error("build", PARAMETER_INVALID, "FIX_LOAD_TOP_MEMORY_ADDRESS %s is not valid dec or hex string" % (Record[2]))
            elif Name == TAB_DSC_DEFINES_RFC_LANGUAGES:
                if not Record[2] or Record[2][0] != '"' or Record[2][-1] != '"' or len(Record[2]) == 1:
                    EdkLogger.error('build', FORMAT_NOT_SUPPORTED, 'language code for RFC_LANGUAGES must have double quotes around it, for example: RFC_LANGUAGES = "en-us;zh-hans"',
                                    File=self.MetaFile, Line=Record[-1])
                LanguageCodes = Record[2][1:-1]
                if not LanguageCodes:
                    EdkLogger.error('build', FORMAT_NOT_SUPPORTED, 'one or more RFC4646 format language code must be provided for RFC_LANGUAGES statement',
                                    File=self.MetaFile, Line=Record[-1])
                LanguageList = GetSplitValueList(LanguageCodes, TAB_SEMI_COLON_SPLIT)
                # check whether there is empty entries in the list
                if None in LanguageList:
                    EdkLogger.error('build', FORMAT_NOT_SUPPORTED, 'one or more empty language code is in RFC_LANGUAGES statement',
                                    File=self.MetaFile, Line=Record[-1])
                def_set.add(definition(Name,LanguageList,source_info=self.dsc_file_path))
            elif Name == TAB_DSC_DEFINES_ISO_LANGUAGES:
                if not Record[2] or Record[2][0] != '"' or Record[2][-1] != '"' or len(Record[2]) == 1:
                    EdkLogger.error('build', FORMAT_NOT_SUPPORTED, 'language code for ISO_LANGUAGES must have double quotes around it, for example: ISO_LANGUAGES = "engchn"',
                                    File=self.MetaFile, Line=Record[-1])
                LanguageCodes = Record[2][1:-1]
                if not LanguageCodes:
                    EdkLogger.error('build', FORMAT_NOT_SUPPORTED, 'one or more ISO639-2 format language code must be provided for ISO_LANGUAGES statement',
                                    File=self.MetaFile, Line=Record[-1])
                if len(LanguageCodes) % 3:
                    EdkLogger.error('build', FORMAT_NOT_SUPPORTED, 'bad ISO639-2 format for ISO_LANGUAGES',
                                    File=self.MetaFile, Line=Record[-1])
                LanguageList = []
                for i in range(0, len(LanguageCodes), 3):
                    LanguageList.append(LanguageCodes[i:i + 3])
                def_set.add(definition(Name,LanguageList,source_info=self.dsc_file_path))
            elif Name == TAB_DSC_DEFINES_VPD_TOOL_GUID:
                #
                # try to convert GUID to a real UUID value to see whether the GUID is format
                # for VPD_TOOL_GUID is correct.
                #
                try:
                    uuid.UUID(Record[2])
                except:
                    EdkLogger.error("build", FORMAT_INVALID, "Invalid GUID format for VPD_TOOL_GUID", File=self.MetaFile)
                def_set.add(definition(Name,Record[2],source_info=self.dsc_file_path))
            else:
                def_set.add(definition(Name,Record[2],source_info=self.dsc_file_path))
        return def_set

    def _OverrideDuplicateModule(self):
        RecordList = self.dsc_parser[MODEL_META_DATA_COMPONENT, self._Arch]
        Macros = self._Macros
        Components = {}
        for Record in RecordList:
            ModuleId = Record[6]
            file_guid = self.dsc_parser[MODEL_META_DATA_HEADER, self._Arch, None, ModuleId]
            file_guid_str = file_guid[0][2] if file_guid else "NULL"
            ModuleFile = PathClass(NormPath(Record[0], Macros), GlobalData.gWorkspace, Arch=self._Arch)
            if self._Arch != TAB_ARCH_COMMON and (file_guid_str,str(ModuleFile)) in Components:
                self.dsc_parser.DisableOverrideComponent(Components[(file_guid_str,str(ModuleFile))])
            Components[(file_guid_str,str(ModuleFile))] = ModuleId
        self.dsc_parser._PostProcessed = False

    def _SourceInfo(self,LineNo):
        return ":".join((self.dsc_file_path.Path,str(LineNo)))

    def LoadComponents(self,target):
        if not isinstance(target, dsc_dict):
            return
        self._OverrideDuplicateModule()
        RecordList = self.dsc_parser[MODEL_META_DATA_COMPONENT, self._Arch]
        component_sec_type = dsc_section_type(self._Arch)
        Macros = self._Macros
        for Record in RecordList:
            ModuleFile = PathClass(NormPath(Record[0], Macros), GlobalData.gWorkspace, Arch=self._Arch)
            ModuleId = Record[6]
            LineNo = Record[7]

            # check the file validation
            ErrorCode, ErrorInfo = ModuleFile.Validate('.inf')
            if ErrorCode != 0:
                EdkLogger.error('build', ErrorCode, File=self.MetaFile, Line=LineNo,
                                ExtraData=ErrorInfo)

            Module = component(ModuleFile, self._SourceInfo(LineNo))

            # get module private library instance
            RecordList = self.dsc_parser[MODEL_EFI_LIBRARY_CLASS, self._Arch, None, ModuleId]
            lib_section_type = dsc_section_type(self._Arch)
            for Record in RecordList:
                LibraryClass = Record[0]
                LibraryPath = PathClass(NormPath(Record[1], Macros), GlobalData.gWorkspace, Arch=self._Arch)
                LineNo = Record[-1]

                # check the file validation
                ErrorCode, ErrorInfo = LibraryPath.Validate('.inf')
                if ErrorCode != 0:
                    EdkLogger.error('build', ErrorCode, File=self.MetaFile, Line=LineNo,
                                    ExtraData=ErrorInfo)

                if LibraryClass == '' or LibraryClass == 'NULL':
                    self._NullLibraryNumber += 1
                    LibraryClass = 'NULL%d' % self._NullLibraryNumber
                    EdkLogger.verbose("Found forced library for %s\n\t%s [%s]" % (ModuleFile, LibraryPath, LibraryClass))

                lib_class_obj = library_class(LibraryClass,LibraryPath,self._SourceInfo(LineNo))
                Module.library_classes.append(lib_class_obj)
                self.dsc_recipe.library_classes[lib_section_type].add(lib_class_obj)

            # get module private PCD setting
            for Type in [MODEL_PCD_FIXED_AT_BUILD, MODEL_PCD_PATCHABLE_IN_MODULE, \
                         MODEL_PCD_FEATURE_FLAG, MODEL_PCD_DYNAMIC, MODEL_PCD_DYNAMIC_EX]:
                TypeString = _PCD_TYPE_STRING_[Type]
                dsc_comp_pcd_sec_type = dsc_pcd_component_type(TypeString)
                RecordList = self.dsc_parser[Type, self._Arch, None, ModuleId]
                for TokenSpaceGuid, PcdCName, Setting, Dummy1, Dummy2, Dummy3, Dummy4, Dummy5 in RecordList:
                    TokenList = GetSplitValueList(Setting)
                    DefaultValue = TokenList[0]
                    # the format is PcdName| Value | VOID* | MaxDatumSize
                    if len(TokenList) > 2:
                        if TokenList[2]:
                            MaxDatumSize = int(TokenList[2],16) if TokenList[2].lower().startswith('0x') else int(TokenList[2])
                        else:
                            MaxDatumSize = 0
                    else:
                        MaxDatumSize = 0
                    try:
                        Module.pcds[dsc_comp_pcd_sec_type].add(pcd_typed(TokenSpaceGuid,PcdCName,DefaultValue,'',MaxDatumSize,self._SourceInfo(LineNo)))
                    except:
                        Module.pcds[dsc_comp_pcd_sec_type] = dsc_set()
                        Module.pcds[dsc_comp_pcd_sec_type].add(pcd_typed(TokenSpaceGuid,PcdCName,DefaultValue,'',MaxDatumSize,self._SourceInfo(LineNo)))

            # get module private build options
            RecordList = self.dsc_parser[MODEL_META_DATA_BUILD_OPTION, self._Arch, None, ModuleId]
            for ToolChainFamily, ToolChain, Option, Dummy1, Dummy2, Dummy3, Dummy4, Dummy5 in RecordList:
                if (ToolChainFamily, ToolChain) not in Module.build_options:
                    Module.build_options[ToolChainFamily, ToolChain] = Option
                else:
                    OptionString = Module.build_options[ToolChainFamily, ToolChain]
                    Module.build_options[ToolChainFamily, ToolChain] = OptionString + " " + Option

            RecordList = self.dsc_parser[MODEL_META_DATA_HEADER, self._Arch, None, ModuleId]
            if RecordList:
                if len(RecordList) != 1:
                    EdkLogger.error('build', OPTION_UNKNOWN, 'Only FILE_GUID can be listed in <Defines> section.',
                                    File=self.MetaFile, ExtraData=str(ModuleFile), Line=LineNo)
#                 ModuleFile = ProcessDuplicatedInf(ModuleFile, RecordList[0][2], GlobalData.gWorkspace)
#                 ModuleFile.Arch = self._Arch
                Module.defines.add(RecordList[0][2])

            target[component_sec_type].add(Module)
        return target

if __name__ == "__main__":
    dsc_reci = DscRecipe(r"OvmfPkg\OvmfPkgIa32.dsc", "IA32", r"C:\BobFeng\ToolDev\EDKIITrunk\BobEdk2\edk2",[r"C:\BobFeng\ToolDev\EDKIITrunk\BobEdk2\edk2"])
    dsc = dsc_reci.get()
    for item in dsc.defines:
        print(item)
    for comps in dsc.components.values():
        for comp in comps:
            print(comp)

