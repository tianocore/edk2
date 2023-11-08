## @file
# The file is used to preprocess the source file.
#
# Copyright (c) 2022-, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import re
import Common.EdkLogger as EdkLogger
from antlr4 import *
from pathlib import Path
from Common.BuildToolError import *
from VfrCompiler.IfrCtypes import EFI_GUID
from VfrCompiler.IfrUtility import VfrVarDataTypeDB
from Common.LongFilePathSupport import LongFilePath
from VfrCompiler.IfrCommon import GUID_BUFFER_VALUE_LEN


class Options:
    def __init__(self):
        # open/close VfrCompiler
        self.LanuchVfrCompiler = False
        self.ModuleName = None
        self.Workspace = None
        self.VFRPP = None
        self.InputFileName = None
        self.BaseFileName = None
        self.IncludePaths = []
        self.OutputDirectory = None
        self.DebugDirectory = None
        self.CreateRecordListFile = True
        self.RecordListFileName = None
        self.CreateIfrPkgFile = True
        self.PkgOutputFileName = None
        self.COutputFileName = None
        self.SkipCPreprocessor = True
        self.CPreprocessorOptions = None
        self.CProcessedVfrFileName = None
        self.HasOverrideClassGuid = False
        self.OverrideClassGuid = None
        self.WarningAsError = False
        self.AutoDefault = False
        self.CheckDefault = False
        self.CreateYamlFile = True
        self.YamlFileName = None
        self.CreateJsonFile = True
        self.JsonFileName = None
        self.UniStrDefFileName = None
        self.YamlOutputFileName = None
        self.UniStrDisplayFile = None


class KV:
    def __init__(self, Key, Value) -> None:
        self.Key = Key
        self.Value = Value


class ValueDB:
    def __init__(self, PreVal, PostVal) -> None:
        self.PreVal = PreVal
        self.PostVal = PostVal


class PreProcessDB:
    def __init__(self, Options: Options) -> None:
        self.Options = Options
        self.VfrVarDataTypeDB = VfrVarDataTypeDB()
        self.Preprocessed = False

    def Preprocess(self):
        self.HeaderFiles = self._ExtractHeaderFiles()
        # Read Uni string token/id definitions in StrDef.h file
        self.UniDict = self._GetUniDicts()
        # Read definitions in vfr file
        self.VfrDict = self._GetVfrDicts()
        self.Preprocessed = True

    def TransValue(self, Value):
        if type(Value) == EFI_GUID:
            return Value
        else:
            StrValue = str(Value)
            if self._IsDigit(StrValue):
                return self._ToDigit(StrValue)
            else:
                GuidList = re.findall(r"0x[0-9a-fA-F]+", StrValue)
                GuidList = [int(num, 16) for num in GuidList]
                Guid = EFI_GUID()
                Guid.from_list(GuidList)
                return Guid

    def RevertValue(self, Value) -> str:
        if type(Value) == EFI_GUID:
            return Value.to_string()
        elif ("0x" in Value) or ("0X" in Value):
            StrValue = hex(Value)
        else:
            StrValue = str(Value)
        return StrValue

    def DisplayValue(self, Value, Flag=False):
        if type(Value) == EFI_GUID:
            return Value.to_string()
        StrValue = str(Value)
        if self._IsDigit(StrValue):
            if Flag:
                return f"STRING_TOKEN({StrValue})"
            return int(StrValue, 0)
        return StrValue

    def GetKey(self, Value):
        if type(Value) == EFI_GUID:
            Value = Value.to_string()
            if Value in self.UniDict:
                return self.UniDict[Value]
            if Value in self.VfrDict:
                return self.VfrDict[Value]
        else:
            Value = "0x%04x" % Value
            Value = Value[:2] + Value[2:].upper()
            if Value in self.UniDict:
                return self.UniDict[Value]
        return Value

    def _ExtractHeaderFiles(self):
        FileName = self.Options.InputFileName
        HeaderFiles = []
        try:
            with open(LongFilePath(FileName), mode="r") as fFile:
                lines = fFile.readlines()
            for line in lines:
                if "#include" in line:
                    if line.find("<") != -1:
                        HeaderFile = line[line.find("<") + 1 : line.find(">")]
                        HeaderFiles.append(HeaderFile)
                    elif line.find('"') != -1:
                        l = line.find('"') + 1
                        r = l + line[l:].find('"')
                        HeaderFile = line[l:r]
                        HeaderFiles.append(HeaderFile)
        except Exception:
            EdkLogger.error(
                "VfrCompiler", FILE_PARSE_FAILURE, "File parse failed for %s" % FileName, None
            )
        return HeaderFiles

    def _GetUniDicts(self):
        if self.Options.UniStrDefFileName is None:
            self.Options.UniStrDefFileName = str(
                Path(self.Options.DebugDirectory) / f"{self.Options.ModuleName}StrDefs.h"
            )
        # find UniFile
        FileName = self.Options.UniStrDefFileName
        UniDict = {}
        self._ParseDefines(FileName, UniDict)
        return UniDict

    def _GetVfrDicts(self):
        VfrDict = {}
        if self.Options.LanuchVfrCompiler:
            FileName = self.Options.InputFileName
            self._ParseDefines(FileName, VfrDict, True)
        return VfrDict

    def _IsDigit(self, String):
        return String.isdigit() or (
            String.startswith(("0x", "0X"))
            and all(char in "0123456789ABCDEFabcdef" for char in String[2:])
        )

    def _ToDigit(self, String):
        if String.startswith(("0x", "0X")):
            return int(String, 16)
        return int(String)

    def _ParseDefines(self, FileName, Dict, IsVfrDef=False):
        Pattern = r"#define\s+(\w+)\s+(.*?)(?:(?<!\\)\n|$)"
        with open(FileName) as File:
            Content = File.read()

        Matches = re.findall(Pattern, Content, re.DOTALL)
        for Match in Matches:
            Key = Match[0]
            Value = re.sub(r"\s+", " ", Match[1].replace("\\\n", "").strip())
            SubDefineMatches = re.findall(
                r"#define\s+(\w+)\s+(.*?)(?:(?<!\\)\n|$)", Value, re.DOTALL
            )
            if SubDefineMatches:
                for SubMatch in SubDefineMatches:
                    SubKey = SubMatch[0]
                    SubValue = re.sub(r"\s+", " ", SubMatch[1].replace("\\\n", "").strip())
                    if SubValue.find("//") != -1:
                        SubValue = SubValue.split("//")[0].strip()

                    if SubValue.find("{") != -1:
                        GuidList = re.findall(r"0x[0-9a-fA-F]+", SubValue)
                        GuidList = [int(num, 16) for num in GuidList]
                        SubValue = EFI_GUID()
                        if len(GuidList) == GUID_BUFFER_VALUE_LEN:
                            SubValue.from_list(GuidList)

                    if self.Options.LanuchVfrCompiler:
                        # GUID is unique, to transfer GUID Parsed Value -> GUID Defined Key.
                        if IsVfrDef:
                            # Save def info for yaml generation
                            Dict[SubKey] = SubValue
                            # tranfer value to key for yaml generation
                            if type(SubValue) == EFI_GUID:
                                Dict[SubValue.to_string()] = SubKey
                        else:
                            SubValue = (
                                str(SubValue)
                                if type(SubValue) != EFI_GUID
                                else SubValue.to_string()
                            )
                            Dict[SubValue] = SubKey
            else:
                if Value.find("//") != -1:
                    Value = Value.split("//")[0].strip()
                if Value.find("{") != -1:
                    GuidList = re.findall(r"0x[0-9a-fA-F]+", Value)
                    GuidList = [int(num, 16) for num in GuidList]
                    Value = EFI_GUID()
                    if len(GuidList) == GUID_BUFFER_VALUE_LEN:
                        Value.from_list(GuidList)

                if self.Options.LanuchVfrCompiler:
                    # GUID is unique, to transfer GUID Parsed Value -> GUID Defined Key.
                    if IsVfrDef:
                        Dict[Key] = Value
                        if type(Value) == EFI_GUID:
                            Dict[Value.to_string()] = Key
                    else:
                        Value = str(Value) if type(Value) != EFI_GUID else Value.to_string()
                        Dict[Value] = Key

    def _FindIncludeHeaderFile(self, IncludePaths, File):
        FileList = []
        Name = File.split("/")[-1]
        for Start in IncludePaths:
            Matches = list(Path(Start).rglob(Name))
            for MatchFile in Matches:
                FileList.append(str(MatchFile))
        return list(set(FileList))
