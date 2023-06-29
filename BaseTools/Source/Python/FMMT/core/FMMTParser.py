## @file
# This file is used to define the interface of Bios Parser.
#
# Copyright (c) 2021-, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
from FirmwareStorageFormat.Common import *
from core.BinaryFactoryProduct import ParserEntry
from core.BiosTreeNode import *
from core.BiosTree import *
from core.GuidTools import *
from utils.FmmtLogger import FmmtLogger as logger

class FMMTParser:
    def __init__(self, name: str, TYPE: str) -> None:
        self.WholeFvTree = BIOSTREE(name)
        self.WholeFvTree.type = TYPE
        self.FinalData = b''
        self.BinaryInfo = []

    ## Parser the nodes in WholeTree.
    def ParserFromRoot(self, WholeFvTree=None, whole_data: bytes=b'', Reloffset: int=0) -> None:
        if WholeFvTree.type == ROOT_TREE or WholeFvTree.type == ROOT_FV_TREE or WholeFvTree.type == ROOT_ELF_TREE:
            ParserEntry().DataParser(self.WholeFvTree, whole_data, Reloffset)
        else:
            ParserEntry().DataParser(WholeFvTree, whole_data, Reloffset)
        for Child in WholeFvTree.Child:
            self.ParserFromRoot(Child, "")

    ## Encapuslation all the data in tree into self.FinalData
    def Encapsulation(self, rootTree, CompressStatus: bool) -> None:
        # If current node is Root node, skip it.
        if rootTree.type == ROOT_TREE or rootTree.type == ROOT_FV_TREE or rootTree.type == ROOT_FFS_TREE or rootTree.type == ROOT_SECTION_TREE:
            logger.debug('Encapsulated successfully!')
        # If current node do not have Header, just add Data.
        elif rootTree.type == BINARY_DATA or rootTree.type == FFS_FREE_SPACE:
            self.FinalData += rootTree.Data.Data
            rootTree.Child = []
        # If current node do not have Child and ExtHeader, just add its Header and Data.
        elif rootTree.type == DATA_FV_TREE or rootTree.type == FFS_PAD:
            self.FinalData += struct2stream(rootTree.Data.Header) + rootTree.Data.Data + rootTree.Data.PadData
            if rootTree.isFinalChild():
                ParTree = rootTree.Parent
                if ParTree.type != 'ROOT':
                    self.FinalData += ParTree.Data.PadData
            rootTree.Child = []
        # If current node is not Section node and may have Child and ExtHeader, add its Header,ExtHeader. If do not have Child, add its Data.
        elif rootTree.type == FV_TREE or rootTree.type == FFS_TREE or rootTree.type == SEC_FV_TREE:
            if rootTree.HasChild():
                self.FinalData += struct2stream(rootTree.Data.Header)
            else:
                self.FinalData += struct2stream(rootTree.Data.Header) + rootTree.Data.Data + rootTree.Data.PadData
                if rootTree.isFinalChild():
                    ParTree = rootTree.Parent
                    if ParTree.type != 'ROOT':
                        self.FinalData += ParTree.Data.PadData
        # If current node is Section, need to consider its ExtHeader, Child and Compressed Status.
        elif rootTree.type == SECTION_TREE:
            # Not compressed section
            if rootTree.Data.OriData == b'' or (rootTree.Data.OriData != b'' and CompressStatus):
                if rootTree.HasChild():
                    if rootTree.Data.ExtHeader:
                        self.FinalData += struct2stream(rootTree.Data.Header) + struct2stream(rootTree.Data.ExtHeader)
                    else:
                        self.FinalData += struct2stream(rootTree.Data.Header)
                else:
                    Data = rootTree.Data.Data
                    if rootTree.Data.ExtHeader:
                        self.FinalData += struct2stream(rootTree.Data.Header) + struct2stream(rootTree.Data.ExtHeader) + Data + rootTree.Data.PadData
                    else:
                        self.FinalData += struct2stream(rootTree.Data.Header) + Data + rootTree.Data.PadData
                    if rootTree.isFinalChild():
                        ParTree = rootTree.Parent
                        self.FinalData += ParTree.Data.PadData
            # If compressed section
            else:
                Data = rootTree.Data.OriData
                rootTree.Child = []
                if rootTree.Data.ExtHeader:
                    self.FinalData += struct2stream(rootTree.Data.Header) + struct2stream(rootTree.Data.ExtHeader) + Data + rootTree.Data.PadData
                else:
                    self.FinalData += struct2stream(rootTree.Data.Header) + Data + rootTree.Data.PadData
                if rootTree.isFinalChild():
                    ParTree = rootTree.Parent
                    self.FinalData += ParTree.Data.PadData
        for Child in rootTree.Child:
            self.Encapsulation(Child, CompressStatus)
