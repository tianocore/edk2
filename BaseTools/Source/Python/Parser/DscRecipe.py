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
from Common.Misc import tdict
import uuid
from Parser.MetaFileParser import DscParser
from Common.Parsing import IsValidWord
from random import sample
import string
import re
from Common.Expression import ValueExpressionEx
from CommonDataClass.Exceptions import BadExpression, EvaluationException
from Common.Misc import CheckPcdDatum
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

SkuIdPattern = re.compile(r'^[a-zA-Z_][a-zA-Z0-9_]*$')
## regular expressions for finding decimal and hex numbers
Pattern = re.compile('^[1-9]\d*|0$')
HexPattern = re.compile(r'0[xX][0-9a-fA-F]+$')
StructPattern = re.compile(r'[_a-zA-Z][0-9A-Za-z_]*$')

def AnalyzePcdExpression(Setting):
    RanStr = ''.join(sample(string.ascii_letters + string.digits, 8))
    Setting = Setting.replace('\\\\', RanStr).strip()
    # There might be escaped quote in a string: \", \\\" , \', \\\'
    Data = Setting
    # There might be '|' in string and in ( ... | ... ), replace it with '-'
    NewStr = ''
    InSingleQuoteStr = False
    InDoubleQuoteStr = False
    Pair = 0
    for Index, ch in enumerate(Data):
        if ch == '"' and not InSingleQuoteStr:
            if Data[Index - 1] != '\\':
                InDoubleQuoteStr = not InDoubleQuoteStr
        elif ch == "'" and not InDoubleQuoteStr:
            if Data[Index - 1] != '\\':
                InSingleQuoteStr = not InSingleQuoteStr
        elif ch == '(' and not (InSingleQuoteStr or InDoubleQuoteStr):
            Pair += 1
        elif ch == ')' and not (InSingleQuoteStr or InDoubleQuoteStr):
            Pair -= 1

        if (Pair > 0 or InSingleQuoteStr or InDoubleQuoteStr) and ch == TAB_VALUE_SPLIT:
            NewStr += '-'
        else:
            NewStr += ch
    FieldList = []
    StartPos = 0
    while True:
        Pos = NewStr.find(TAB_VALUE_SPLIT, StartPos)
        if Pos < 0:
            FieldList.append(Setting[StartPos:].strip())
            break
        FieldList.append(Setting[StartPos:Pos].strip())
        StartPos = Pos + 1
    for i, ch in enumerate(FieldList):
        if RanStr in ch:
            FieldList[i] = ch.replace(RanStr,'\\\\')
    return FieldList

## AnalyzeDscPcd
#
#  Analyze DSC PCD value, since there is no data type info in DSC
#  This function is used to match functions (AnalyzePcdData) used for retrieving PCD value from database
#  1. Feature flag: TokenSpace.PcdCName|PcdValue
#  2. Fix and Patch:TokenSpace.PcdCName|PcdValue[|VOID*[|MaxSize]]
#  3. Dynamic default:
#     TokenSpace.PcdCName|PcdValue[|VOID*[|MaxSize]]
#     TokenSpace.PcdCName|PcdValue
#  4. Dynamic VPD:
#     TokenSpace.PcdCName|VpdOffset[|VpdValue]
#     TokenSpace.PcdCName|VpdOffset[|MaxSize[|VpdValue]]
#  5. Dynamic HII:
#     TokenSpace.PcdCName|HiiString|VariableGuid|VariableOffset[|HiiValue]
#  PCD value needs to be located in such kind of string, and the PCD value might be an expression in which
#    there might have "|" operator, also in string value.
#
#  @param Setting: String contain information described above with "TokenSpace.PcdCName|" stripped
#  @param PcdType: PCD type: feature, fixed, dynamic default VPD HII
#  @param DataType: The datum type of PCD: VOID*, UNIT, BOOL
#  @retval:
#    ValueList: A List contain fields described above
#    IsValid:   True if conforming EBNF, otherwise False
#    Index:     The index where PcdValue is in ValueList
#
def AnalyzeDscPcd(Setting, PcdType):
    FieldList = AnalyzePcdExpression(Setting)
    DataType = ''
    IsValid = True
    if PcdType in (MODEL_PCD_FIXED_AT_BUILD, MODEL_PCD_PATCHABLE_IN_MODULE, MODEL_PCD_DYNAMIC_DEFAULT, MODEL_PCD_DYNAMIC_EX_DEFAULT):
        Value = FieldList[0]
        Size = ''
        if len(FieldList) > 1 and FieldList[1]:
            DataType = FieldList[1]
            if FieldList[1] != TAB_VOID and StructPattern.match(FieldList[1]) is None:
                IsValid = False
        if len(FieldList) > 2:
            Size = FieldList[2]
        if Size:
            try:
                int(Size, 16) if Size.upper().startswith("0X") else int(Size)
            except:
                IsValid = False
                Size = -1
        return [str(Value), DataType, str(Size)], IsValid, 0
    elif PcdType == MODEL_PCD_FEATURE_FLAG:
        Value = FieldList[0]
        Size = ''
        return [Value, DataType, str(Size)], IsValid, 0
    elif PcdType in (MODEL_PCD_DYNAMIC_VPD, MODEL_PCD_DYNAMIC_EX_VPD):
        VpdOffset = FieldList[0]
        Value = Size = ''
        if Size:
            try:
                int(Size, 16) if Size.upper().startswith("0X") else int(Size)
            except:
                IsValid = False
                Size = -1
        return [VpdOffset, str(Size), Value], IsValid, 2
    elif PcdType in (MODEL_PCD_DYNAMIC_HII, MODEL_PCD_DYNAMIC_EX_HII):
        IsValid = (3 <= len(FieldList) <= 5)
        HiiString = FieldList[0]
        Guid = Offset = Value = Attribute = ''
        if len(FieldList) > 1:
            Guid = FieldList[1]
        if len(FieldList) > 2:
            Offset = FieldList[2]
        if len(FieldList) > 3:
            Value = FieldList[3]
        if len(FieldList) > 4:
            Attribute = FieldList[4]
        return [HiiString, Guid, Offset, Value, Attribute], IsValid, 3
    return [], False, 0

def to_method(func):
    def wrapper(*args, **kwargs):
        return func(*args, **kwargs)
    return wrapper
class cached_data(object):
    def __init__(self,function):
        self._function = function
        self._lock = False
        self._value = None
    def __call__(self,*args,**kwargs):
        if not self._lock:
            self._value = self._function(*args,**kwargs)
            self._lock = True
        return self._value
class DscRecipe(object):
    def __init__(self, file_path, arch, workspace, packages_path=None,build_target='', tool_chain_tag='', tool_family='', cmd_macro = None, cmd_pcd=None):
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
        GlobalData.gGlobalDefines['TOOL_CHAIN_TAG'] = tool_chain_tag
        GlobalData.gGlobalDefines['TARGET'] = build_target
        GlobalData.gGlobalDefines['ARCH'] = arch
        GlobalData.gGlobalDefines['FAMILY'] = tool_family
        GlobalData.gCommandLineDefines = cmd_macro if cmd_macro else {}
        GlobalData.BuildOptionPcd = cmd_pcd

        GlobalData.gWorkspace = workspace
        self._MacroDict = None
        self._NullLibraryNumber = 0

    def read(self):
        if not self.parsed:
            self.init_recipe()
            self.parsed = True
        return self.dsc_recipe
    def write(self, file_path):
        pass

    def init_recipe(self):
        self.ReadDefines()
        self.ReadComponents()
        self.ReadBuildOptions()
        self.ReadDefaultStore()
        self.ReadLibClasses()
        self.ReadSkus()
        self.ReadPcds()

    ## Get current effective macros
    @property
    def _Macros(self):
        if self._MacroDict is None:
            self._MacroDict = {}
            self._MacroDict.update(GlobalData.gPlatformDefines)
            self._MacroDict.update(GlobalData.gGlobalDefines)
            self._MacroDict.update(GlobalData.gCommandLineDefines)
        return self._MacroDict

    @to_method
    @cached_data
    def ReadDefines(self):
        def_set = self.dsc_recipe.defines
        RecordList = self.dsc_parser[MODEL_META_DATA_HEADER, self._Arch]
        for Record in RecordList:
            Name = Record[1]
            # items defined _PROPERTY_ don't need additional processing
            # some special items in [Defines] section need special treatment
            if Name == TAB_DSC_DEFINES_OUTPUT_DIRECTORY:
                OutputDirectory = NormPath(Record[2], self._Macros)
                if ' ' in OutputDirectory:
                    EdkLogger.error("build", FORMAT_NOT_SUPPORTED, "No space is allowed in OUTPUT_DIRECTORY",
                                    File=self.dsc_file_path, Line=Record[-1],
                                    ExtraData=OutputDirectory)
                def_set.add(definition(Name,OutputDirectory,source_info=self.dsc_file_path))
            elif Name == TAB_DSC_DEFINES_FLASH_DEFINITION:
                FlashDefinition = PathClass(NormPath(Record[2], self._Macros), GlobalData.gWorkspace)
                ErrorCode, ErrorInfo = FlashDefinition.Validate('.fdf')
                if ErrorCode != 0:
                    EdkLogger.error('build', ErrorCode, File=self.dsc_file_path, Line=Record[-1],
                                    ExtraData=ErrorInfo)
                def_set.add(definition(Name,FlashDefinition,source_info=self.dsc_file_path))
            elif Name == TAB_DSC_PREBUILD:
                PrebuildValue = Record[2]
                if Record[2][0] == '"':
                    if Record[2][-1] != '"':
                        EdkLogger.error('build', FORMAT_INVALID, 'Missing double quotes in the end of %s statement.' % TAB_DSC_PREBUILD,
                                    File=self.dsc_file_path, Line=Record[-1])
                    PrebuildValue = Record[2][1:-1]
                def_set.add(definition(Name,PrebuildValue,source_info=self.dsc_file_path))
            elif Name == TAB_DSC_POSTBUILD:
                PostbuildValue = Record[2]
                if Record[2][0] == '"':
                    if Record[2][-1] != '"':
                        EdkLogger.error('build', FORMAT_INVALID, 'Missing double quotes in the end of %s statement.' % TAB_DSC_POSTBUILD,
                                    File=self.dsc_file_path, Line=Record[-1])
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
                                    File=self.dsc_file_path, Line=Record[-1])
                LanguageCodes = Record[2][1:-1]
                if not LanguageCodes:
                    EdkLogger.error('build', FORMAT_NOT_SUPPORTED, 'one or more RFC4646 format language code must be provided for RFC_LANGUAGES statement',
                                    File=self.dsc_file_path, Line=Record[-1])
                LanguageList = GetSplitValueList(LanguageCodes, TAB_SEMI_COLON_SPLIT)
                # check whether there is empty entries in the list
                if None in LanguageList:
                    EdkLogger.error('build', FORMAT_NOT_SUPPORTED, 'one or more empty language code is in RFC_LANGUAGES statement',
                                    File=self.dsc_file_path, Line=Record[-1])
                def_set.add(definition(Name,LanguageList,source_info=self.dsc_file_path))
            elif Name == TAB_DSC_DEFINES_ISO_LANGUAGES:
                if not Record[2] or Record[2][0] != '"' or Record[2][-1] != '"' or len(Record[2]) == 1:
                    EdkLogger.error('build', FORMAT_NOT_SUPPORTED, 'language code for ISO_LANGUAGES must have double quotes around it, for example: ISO_LANGUAGES = "engchn"',
                                    File=self.dsc_file_path, Line=Record[-1])
                LanguageCodes = Record[2][1:-1]
                if not LanguageCodes:
                    EdkLogger.error('build', FORMAT_NOT_SUPPORTED, 'one or more ISO639-2 format language code must be provided for ISO_LANGUAGES statement',
                                    File=self.dsc_file_path, Line=Record[-1])
                if len(LanguageCodes) % 3:
                    EdkLogger.error('build', FORMAT_NOT_SUPPORTED, 'bad ISO639-2 format for ISO_LANGUAGES',
                                    File=self.dsc_file_path, Line=Record[-1])
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
                    EdkLogger.error("build", FORMAT_INVALID, "Invalid GUID format for VPD_TOOL_GUID", File=self.dsc_file_path)
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

    @to_method
    @cached_data
    def ReadComponents(self):
        target = self.dsc_recipe.components

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
                EdkLogger.error('build', ErrorCode, File=self.dsc_file_path, Line=LineNo,
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
                    EdkLogger.error('build', ErrorCode, File=self.dsc_file_path, Line=LineNo,
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
                                    File=self.dsc_file_path, ExtraData=str(ModuleFile), Line=LineNo)

                Module.defines.add(RecordList[0][2])

            target[component_sec_type].add(Module)
        return target

    @to_method
    @cached_data
    def ReadSkus(self):
        target = self.dsc_recipe.skus
        RecordList = self.dsc_parser[MODEL_EFI_SKU_ID, self._Arch]
        for Record in RecordList:
            if not Record[0]:
                EdkLogger.error('build', FORMAT_INVALID, 'No Sku ID number',
                                File=self.dsc_file_path, Line=Record[-1])
            if not Record[1]:
                EdkLogger.error('build', FORMAT_INVALID, 'No Sku ID name',
                                File=self.dsc_file_path, Line=Record[-1])
            if not Pattern.match(Record[0]) and not HexPattern.match(Record[0]):
                EdkLogger.error('build', FORMAT_INVALID, "The format of the Sku ID number is invalid. It only support Integer and HexNumber",
                                File=self.dsc_file_path, Line=Record[-1])
            if not SkuIdPattern.match(Record[1]) or (Record[2] and not SkuIdPattern.match(Record[2])):
                EdkLogger.error('build', FORMAT_INVALID, "The format of the Sku ID name is invalid. The correct format is '(a-zA-Z_)(a-zA-Z0-9_)*'",
                                File=self.dsc_file_path, Line=Record[-1])

            skuid = int(Record[0], 16) if Record[0].upper().startswith("0X") else int(Record[0])
            skuname = Record[1].upper()
            parentname = Record[2].upper() if Record[2] else "DEFAULT"
            target.add(sku_id(skuid, skuname, parentname, self._SourceInfo(Record[-1])))
        return target

    @to_method
    @cached_data
    def ReadDefaultStore(self):
        target = self.dsc_recipe.default_stores
        RecordList = self.dsc_parser[MODEL_EFI_DEFAULT_STORES, self._Arch]
        for Record in RecordList:
            if not Record[0]:
                EdkLogger.error('build', FORMAT_INVALID, 'No DefaultStores ID number',
                                File=self.dsc_file_path, Line=Record[-1])
            if not Record[1]:
                EdkLogger.error('build', FORMAT_INVALID, 'No DefaultStores ID name',
                                File=self.dsc_file_path, Line=Record[-1])
            if not Pattern.match(Record[0]) and not HexPattern.match(Record[0]):
                EdkLogger.error('build', FORMAT_INVALID, "The format of the DefaultStores ID number is invalid. It only support Integer and HexNumber",
                                File=self.dsc_file_path, Line=Record[-1])
            if not IsValidWord(Record[1]):
                EdkLogger.error('build', FORMAT_INVALID, "The format of the DefaultStores ID name is invalid. The correct format is '(a-zA-Z0-9_)(a-zA-Z0-9_-.)*'",
                                File=self.dsc_file_path, Line=Record[-1])

            target.add(default_store(int(Record[0], 16) if Record[0].upper().startswith("0X") else int(Record[0]), Record[1].upper(), self._SourceInfo(Record[-1])))

        return target

    @to_method
    @cached_data
    def ReadLibClasses(self):
        target = self.dsc_recipe.library_classes
        RecordList = self.dsc_parser[MODEL_EFI_LIBRARY_CLASS, self._Arch, None, -1]
        Macros = self._Macros
        LibraryClassSet = set()
        #
        # tdict is a special dict kind of type, used for selecting correct
        # library instance for given library class and module type
        #
        LibraryClassDict = tdict(True, 3)
        for Record in RecordList:
            LibraryClass, LibraryInstance, Dummy, Arch, ModuleType, Dummy, Dummy, LineNo = Record
            if LibraryClass == '' or LibraryClass == 'NULL':
                self._NullLibraryNumber += 1
                LibraryClass = 'NULL%d' % self._NullLibraryNumber
                EdkLogger.verbose("Found forced library for arch=%s\n\t%s [%s]" % (Arch, LibraryInstance, LibraryClass))
            LibraryClassSet.add(LibraryClass)
            LibraryInstance = PathClass(NormPath(LibraryInstance, Macros), GlobalData.gWorkspace, Arch=self._Arch)
            # check the file validation
            ErrorCode, ErrorInfo = LibraryInstance.Validate('.inf')
            if ErrorCode != 0:
                EdkLogger.error('build', ErrorCode, File=self.dsc_file_path, Line=LineNo,
                                ExtraData=ErrorInfo)

            if ModuleType != TAB_COMMON and ModuleType not in SUP_MODULE_LIST:
                EdkLogger.error('build', OPTION_UNKNOWN, "Unknown module type [%s]" % ModuleType,
                                File=self.dsc_file_path, ExtraData=LibraryInstance, Line=LineNo)
            LibraryClassDict[Arch, ModuleType, LibraryClass] = (LibraryInstance, LineNo)

        # resolve the specific library instance for each class and each module type

        for LibraryClass in LibraryClassSet:
            # try all possible module types
            for ModuleType in SUP_MODULE_LIST:
                LibClassData = LibraryClassDict[self._Arch, ModuleType, LibraryClass]
                if LibClassData is None:
                    continue
                LibraryInstance,LineNo = LibClassData
                lib_sec_obj = dsc_section_type(self._Arch, ModuleType)
                target[lib_sec_obj].add(library_class(LibraryClass, LibraryInstance, source_info=self._SourceInfo(LineNo)))

        return target

    @to_method
    @cached_data
    def ReadBuildOptions(self):
        target = self.dsc_recipe.build_options

        RecordList = self.dsc_parser[MODEL_META_DATA_BUILD_OPTION, self._Arch, EDKII_NAME]
        for ToolChainFamily, ToolChain, Option, Dummy1, Dummy2, Dummy3, Dummy4, LineNo in RecordList:
            #
            # Only flags can be appended
            #
            Target, Tag, Arch, Tool, Attr = ToolChain.split("_")
            build_option_sec_obj = dsc_buildoption_section_type(self._Arch, EDKII_NAME,Dummy3)
            if not ToolChain.endswith('_FLAGS') or Option.startswith('='):
                build_option_obj = build_option(Tool,Attr,Option,Target,Tag,Arch,ToolChainFamily, True, self._SourceInfo(LineNo))
                target[build_option_sec_obj].add(build_option_obj)
            else:
                build_option_obj = build_option(Tool,Attr,Option,Target,Tag,Arch,ToolChainFamily, False, self._SourceInfo(LineNo))
                target[build_option_sec_obj].add(build_option_obj)
        return target

    @to_method
    @cached_data
    def ReadPcds(self):
        target = self.dsc_recipe.pcds
        PcdTypeSet = (
            MODEL_PCD_FIXED_AT_BUILD,
            MODEL_PCD_PATCHABLE_IN_MODULE,
            MODEL_PCD_FEATURE_FLAG,
            MODEL_PCD_DYNAMIC_DEFAULT,
            MODEL_PCD_DYNAMIC_HII,
            MODEL_PCD_DYNAMIC_VPD,
            MODEL_PCD_DYNAMIC_EX_DEFAULT,
            MODEL_PCD_DYNAMIC_EX_HII,
            MODEL_PCD_DYNAMIC_EX_VPD)
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

        for PcdType in PcdTypeSet:
            RecordList = self.dsc_parser[PcdType, self._Arch]
            for TokenSpaceGuid, PcdCName, Setting, Arch, SkuName, DefaultStore, Dummy4, LineNo in RecordList:
                SkuName = SkuName.upper()
                SkuName = TAB_DEFAULT if SkuName == TAB_COMMON else SkuName
                if PcdType in (MODEL_PCD_DYNAMIC_HII, MODEL_PCD_DYNAMIC_EX_HII):
                    pcd_sec_obj = dsc_pcd_section_type(_PCD_TYPE_STRING_[PcdType], self._Arch, SkuName, DefaultStore)
                else:
                    pcd_sec_obj = dsc_pcd_section_type(_PCD_TYPE_STRING_[PcdType], self._Arch, SkuName)

                ValueList, IsValid, _ = AnalyzeDscPcd(Setting, PcdType)
                if IsValid:
                    if PcdType in (MODEL_PCD_DYNAMIC_VPD,MODEL_PCD_DYNAMIC_EX_VPD):
                        VpdOffset, MaxDatumSize, InitialValue = ValueList
                        MaxDatumSize = 0 if not MaxDatumSize else MaxDatumSize
                        pcd_obj = pcd_vpd(TokenSpaceGuid,PcdCName,InitialValue,VpdOffset,MaxDatumSize,self._SourceInfo(LineNo))
                    if PcdType in (MODEL_PCD_DYNAMIC_HII, MODEL_PCD_DYNAMIC_EX_HII):
                        VariableName, VariableGuid, VariableOffset, DefaultValue, VarAttribute = ValueList
                        pcd_obj = pcd_variable(TokenSpaceGuid,PcdCName,VariableName,VariableGuid,VariableOffset,DefaultValue,VarAttribute,self._SourceInfo(LineNo))
                    else:
                        PcdValue, DatumType, MaxDatumSize = ValueList
                        MaxDatumSize = 0 if not MaxDatumSize else MaxDatumSize
                        pcd_obj = pcd_typed(TokenSpaceGuid,PcdCName,PcdValue,DatumType,MaxDatumSize,self._SourceInfo(LineNo))
                    target[pcd_sec_obj].add(pcd_obj)
        return target

## Open Dsc File
#
#  Return a DSC handler
#
#   @param      dsc_path        The relative path of platform description file
#   @param      arch            Default Arch value for filtering sections
#   @param      workspace       Workspace path
#   @param      packagepath     A list of packages search path.
#   @param      build_target    Build target
#   @param      tool_chain_tag  Tool chain
#   @param      tool_family     Build tool family
#   @param      cmd_macro       A dict which store the macro and its value.
#   @param      cmd_pcd         A string list. e.g. pcdtoken.pcdname=value
#
def OpenDsc(dsc_path, arch, workspace, packagepath,build_target='', tool_chain_tag='', tool_family='', cmd_macro = None, cmd_pcd=None):
    return DscRecipe(dsc_path, arch, workspace, packagepath,build_target, tool_chain_tag, tool_family, cmd_macro, cmd_pcd)

if __name__ == "__main__":
    dsc_handle = OpenDsc(r"OvmfPkg\OvmfPkgIa32.dsc", "IA32", r"C:\BobFeng\ToolDev\EDKIITrunk\BobEdk2\edk2",[r"C:\BobFeng\ToolDev\EDKIITrunk\BobEdk2\edk2"])
    dsc = dsc_handle.read()
    for item in dsc.defines:
        print(item)
    for comps in dsc.components.values():
        for comp in comps:
            print(comp)
    for items in dsc.pcds.values():
        for pcd in items:
            print(pcd)
    for items in dsc.skus:
        print(items)

    for items in dsc.default_stores:
        print(items)
    for items in dsc.build_options.values():
        for bo in items:
            print(bo)

    for item in dsc_handle.ReadDefines():
        print(item)


