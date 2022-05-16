## @file
# This file is used to define the FMMT dependent external tool management class.
#
# Copyright (c) 2021-, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import glob
import logging
import os
import shutil
import sys
import tempfile
import uuid
from FirmwareStorageFormat.Common import *
from utils.FmmtLogger import FmmtLogger as logger
import subprocess

def ExecuteCommand(cmd: list) -> None:
    subprocess.run(cmd,stdout=subprocess.DEVNULL)

class GUIDTool:
    def __init__(self, guid: str, short_name: str, command: str) -> None:
        self.guid: str = guid
        self.short_name: str = short_name
        self.command: str = command
        self.ifexist: bool = False

    def pack(self, buffer: bytes) -> bytes:
        """
        compress file.
        """
        tool = self.command
        if tool:
            tmp = tempfile.mkdtemp(dir=os.environ.get('tmp'))
            ToolInputFile = os.path.join(tmp, "pack_uncompress_sec_file")
            ToolOuputFile = os.path.join(tmp, "pack_sec_file")
            try:
                file = open(ToolInputFile, "wb")
                file.write(buffer)
                file.close()
                command = [tool, '-e', '-o', ToolOuputFile,
                                  ToolInputFile]
                ExecuteCommand(command)
                buf = open(ToolOuputFile, "rb")
                res_buffer = buf.read()
            except Exception as msg:
                logger.error(msg)
                return ""
            else:
                buf.close()
                if os.path.exists(tmp):
                    shutil.rmtree(tmp)
                return res_buffer
        else:
            logger.error(
                "Error parsing section: EFI_SECTION_GUID_DEFINED cannot be parsed at this time.")
            logger.info("Its GUID is: %s" % self.guid)
            return ""


    def unpack(self, buffer: bytes) -> bytes:
        """
        buffer: remove common header
        uncompress file
        """
        tool = self.command
        if tool:
            tmp = tempfile.mkdtemp(dir=os.environ.get('tmp'))
            ToolInputFile = os.path.join(tmp, "unpack_sec_file")
            ToolOuputFile = os.path.join(tmp, "unpack_uncompress_sec_file")
            try:
                file = open(ToolInputFile, "wb")
                file.write(buffer)
                file.close()
                command = [tool, '-d', '-o', ToolOuputFile, ToolInputFile]
                ExecuteCommand(command)
                buf = open(ToolOuputFile, "rb")
                res_buffer = buf.read()
            except Exception as msg:
                logger.error(msg)
                return ""
            else:
                buf.close()
                if os.path.exists(tmp):
                    shutil.rmtree(tmp)
                return res_buffer
        else:
            logger.error("Error parsing section: EFI_SECTION_GUID_DEFINED cannot be parsed at this time.")
            logger.info("Its GUID is: %s" % self.guid)
            return ""

class GUIDTools:
    '''
    GUIDTools is responsible for reading FMMTConfig.ini, verify the tools and provide interfaces to access those tools.
    '''
    default_tools = {
        struct2stream(ModifyGuidFormat("a31280ad-481e-41b6-95e8-127f4c984779")): GUIDTool("a31280ad-481e-41b6-95e8-127f4c984779", "TIANO", "TianoCompress"),
        struct2stream(ModifyGuidFormat("ee4e5898-3914-4259-9d6e-dc7bd79403cf")): GUIDTool("ee4e5898-3914-4259-9d6e-dc7bd79403cf", "LZMA", "LzmaCompress"),
        struct2stream(ModifyGuidFormat("fc1bcdb0-7d31-49aa-936a-a4600d9dd083")): GUIDTool("fc1bcdb0-7d31-49aa-936a-a4600d9dd083", "CRC32", "GenCrc32"),
        struct2stream(ModifyGuidFormat("d42ae6bd-1352-4bfb-909a-ca72a6eae889")): GUIDTool("d42ae6bd-1352-4bfb-909a-ca72a6eae889", "LZMAF86", "LzmaF86Compress"),
        struct2stream(ModifyGuidFormat("3d532050-5cda-4fd0-879e-0f7f630d5afb")): GUIDTool("3d532050-5cda-4fd0-879e-0f7f630d5afb", "BROTLI", "BrotliCompress"),
    }

    def __init__(self, tooldef_file: str=None) -> None:
        self.dir = os.path.join(os.path.dirname(__file__), "..")
        self.tooldef_file = tooldef_file if tooldef_file else os.path.join(self.dir, "FmmtConf.ini")
        self.tooldef = dict()

    def SetConfigFile(self) -> None:
        if os.environ['FmmtConfPath']:
            self.tooldef_file = os.path.join(os.environ['FmmtConfPath'], 'FmmtConf.ini')
        else:
            PathList = os.environ['PATH']
            for CurrentPath in PathList:
                if os.path.exists(os.path.join(CurrentPath, 'FmmtConf.ini')):
                    self.tooldef_file = os.path.join(CurrentPath, 'FmmtConf.ini')
                    break

    def VerifyTools(self, guidtool) -> None:
        """
        Verify Tools and Update Tools path.
        """
        path_env = os.environ.get("PATH")
        path_env_list = path_env.split(os.pathsep)
        path_env_list.append(os.path.dirname(__file__))
        path_env_list = list(set(path_env_list))
        cmd = guidtool.command
        if os.path.isabs(cmd):
            if not os.path.exists(cmd):
                if guidtool not in self.default_tools:
                    logger.error("Tool Not found %s, which causes compress/uncompress process error." % cmd)
                    logger.error("Please goto edk2 repo in current console, run 'edksetup.bat rebuild' command, and try again.\n")
                else:
                    logger.error("Tool Not found %s, which causes compress/uncompress process error." % cmd)
            else:
                guidtool.ifexist = True
        else:
            for syspath in path_env_list:
                if glob.glob(os.path.join(syspath, cmd+"*")):
                    guidtool.ifexist = True
                    break
            else:
                if guidtool not in self.default_tools:
                    logger.error("Tool Not found %s, which causes compress/uncompress process error." % cmd)
                    logger.error("Please goto edk2 repo in current console, run 'edksetup.bat rebuild' command, and try again.\n")
                else:
                    logger.error("Tool Not found %s, which causes compress/uncompress process error." % cmd)

    def LoadingTools(self) -> None:
        self.SetConfigFile()
        if os.path.exists(self.tooldef_file):
            with open(self.tooldef_file, "r") as fd:
                config_data = fd.readlines()
            for line in config_data:
                try:
                    if not line.startswith("#"):
                        guid, short_name, command = line.split()
                        new_format_guid = struct2stream(ModifyGuidFormat(guid.strip()))
                        self.tooldef[new_format_guid] = GUIDTool(
                            guid.strip(), short_name.strip(), command.strip())
                except:
                    logger.error("GuidTool load error!")
                    continue
        else:
            self.tooldef.update(self.default_tools)

    def __getitem__(self, guid):
        if not self.tooldef:
            self.LoadingTools()
        guid_tool = self.tooldef.get(guid)
        if guid_tool:
            self.VerifyTools(guid_tool)
            return guid_tool
        else:
            logger.error("{} GuidTool is not defined!".format(guid))
            raise Exception("Process Failed: is not defined!")

guidtools = GUIDTools()

