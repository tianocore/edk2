## @file
# This file is used to define the Bios layout tree structure and related operations.
#
# Copyright (c) 2021-, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import collections
from FirmwareStorageFormat.Common import *
from utils.FmmtLogger import FmmtLogger as logger

ROOT_TREE = 'ROOT'
ROOT_FV_TREE = 'ROOT_FV_TREE'
ROOT_FFS_TREE = 'ROOT_FFS_TREE'
ROOT_SECTION_TREE = 'ROOT_SECTION_TREE'

FV_TREE = 'FV'
DATA_FV_TREE = 'DATA_FV'
FFS_TREE = 'FFS'
FFS_PAD = 'FFS_PAD'
FFS_FREE_SPACE = 'FFS_FREE_SPACE'
SECTION_TREE = 'SECTION'
SEC_FV_TREE = 'SEC_FV_IMAGE'
BINARY_DATA = 'BINARY'

RootType = [ROOT_TREE, ROOT_FV_TREE, ROOT_FFS_TREE, ROOT_SECTION_TREE]
FvType = [FV_TREE, SEC_FV_TREE]
FfsType = FFS_TREE
SecType = SECTION_TREE

class BIOSTREE:
    def __init__(self, NodeName: str) -> None:
        self.key = NodeName
        self.type = None
        self.Data = None
        self.Child = []
        self.Findlist = []
        self.Parent = None
        self.NextRel = None
        self.LastRel = None

    def HasChild(self) -> bool:
        if self.Child == []:
            return False
        else:
            return True

    def isFinalChild(self) -> bool:
        ParTree = self.Parent
        if ParTree:
            if ParTree.Child[-1] == self:
                return True
        return False

    # FvTree.insertChild()
    def insertChild(self, newNode, pos: int=None) -> None:
        if len(self.Child) == 0:
            self.Child.append(newNode)
        else:
            if not pos:
                LastTree = self.Child[-1]
                self.Child.append(newNode)
                LastTree.NextRel = newNode
                newNode.LastRel = LastTree
            else:
                newNode.NextRel = self.Child[pos-1].NextRel
                newNode.LastRel = self.Child[pos].LastRel
                self.Child[pos-1].NextRel = newNode
                self.Child[pos].LastRel = newNode
                self.Child.insert(pos, newNode)
        newNode.Parent = self

    # lastNode.insertRel(newNode)
    def insertRel(self, newNode) -> None:
        if self.Parent:
            parentTree = self.Parent
            new_index = parentTree.Child.index(self) + 1
            parentTree.Child.insert(new_index, newNode)
        self.NextRel = newNode
        newNode.LastRel = self

    def deleteNode(self, deletekey: str) -> None:
        FindStatus, DeleteTree = self.FindNode(deletekey)
        if FindStatus:
            parentTree = DeleteTree.Parent
            lastTree = DeleteTree.LastRel
            nextTree = DeleteTree.NextRel
            if parentTree:
                index = parentTree.Child.index(DeleteTree)
                del parentTree.Child[index]
            if lastTree and nextTree:
                lastTree.NextRel = nextTree
                nextTree.LastRel = lastTree
            elif lastTree:
                lastTree.NextRel = None
            elif nextTree:
                nextTree.LastRel = None
            return DeleteTree
        else:
            logger.error('Could not find the target tree')
            return None

    def FindNode(self, key: str, Findlist: list) -> None:
        if self.key == key or (self.Data and self.Data.Name == key) or (self.type == FFS_TREE and self.Data.UiName == key):
            Findlist.append(self)
        for item in self.Child:
            item.FindNode(key, Findlist)

    def GetTreePath(self):
        BiosTreePath = [self]
        while self.Parent:
            BiosTreePath.insert(0, self.Parent)
            self = self.Parent
        return BiosTreePath

    def parserTree(self, TargetDict: dict=None, Info: list=None, space: int=0, ParFvId="") -> None:
        Key = list(TargetDict.keys())[0]
        if TargetDict[Key]["Type"] in RootType:
            Info.append("Image File: {}".format(Key))
            Info.append("FilesNum: {}".format(TargetDict.get(Key).get('FilesNum')))
            Info.append("\n")
        elif TargetDict[Key]["Type"] in FvType:
            space += 2
            if TargetDict[Key]["Type"] == SEC_FV_TREE:
                Info.append("{}Child FV named {} of {}".format(space*" ", Key, ParFvId))
                space += 2
            else:
                Info.append("FvId: {}".format(Key))
                ParFvId = Key
            Info.append("{}FvNameGuid: {}".format(space*" ", TargetDict.get(Key).get('FvNameGuid')))
            Info.append("{}Attributes: {}".format(space*" ", TargetDict.get(Key).get('Attributes')))
            Info.append("{}Total Volume Size: {}".format(space*" ", TargetDict.get(Key).get('Size')))
            Info.append("{}Free Volume Size: {}".format(space*" ", TargetDict.get(Key).get('FreeSize')))
            Info.append("{}Volume Offset: {}".format(space*" ", TargetDict.get(Key).get('Offset')))
            Info.append("{}FilesNum: {}".format(space*" ", TargetDict.get(Key).get('FilesNum')))
        elif TargetDict[Key]["Type"] in FfsType:
            space += 2
            if TargetDict.get(Key).get('UiName') != "b''":
                Info.append("{}File: {} / {}".format(space*" ", Key, TargetDict.get(Key).get('UiName')))
            else:
                Info.append("{}File: {}".format(space*" ", Key))
        if "Files" in list(TargetDict[Key].keys()):
            for item in TargetDict[Key]["Files"]:
                self.parserTree(item, Info, space, ParFvId)

    def ExportTree(self,TreeInfo: dict=None) -> dict:
        if TreeInfo is None:
            TreeInfo =collections.OrderedDict()

        if self.type == ROOT_TREE or self.type == ROOT_FV_TREE or self.type == ROOT_FFS_TREE or self.type == ROOT_SECTION_TREE:
            key = str(self.key)
            TreeInfo[self.key] = collections.OrderedDict()
            TreeInfo[self.key]["Name"] = key
            TreeInfo[self.key]["Type"] = self.type
            TreeInfo[self.key]["FilesNum"] = len(self.Child)
        elif self.type == FV_TREE or  self.type == SEC_FV_TREE:
            key = str(self.Data.FvId)
            TreeInfo[key] = collections.OrderedDict()
            TreeInfo[key]["Name"] = key
            if self.Data.FvId != self.Data.Name:
                TreeInfo[key]["FvNameGuid"] = str(self.Data.Name)
            TreeInfo[key]["Type"] = self.type
            TreeInfo[key]["Attributes"] = hex(self.Data.Header.Attributes)
            TreeInfo[key]["Size"] = hex(self.Data.Header.FvLength)
            TreeInfo[key]["FreeSize"] = hex(self.Data.Free_Space)
            TreeInfo[key]["Offset"] = hex(self.Data.HOffset)
            TreeInfo[key]["FilesNum"] = len(self.Child)
        elif self.type == FFS_TREE:
            key = str(self.Data.Name)
            TreeInfo[key] = collections.OrderedDict()
            TreeInfo[key]["Name"] = key
            TreeInfo[key]["UiName"] = '{}'.format(self.Data.UiName)
            TreeInfo[key]["Version"] = '{}'.format(self.Data.Version)
            TreeInfo[key]["Type"] = self.type
            TreeInfo[key]["Size"] = hex(self.Data.Size)
            TreeInfo[key]["Offset"] = hex(self.Data.HOffset)
            TreeInfo[key]["FilesNum"] = len(self.Child)
        elif self.type == SECTION_TREE and self.Data.Type == 0x02:
            key = str(self.Data.Name)
            TreeInfo[key] = collections.OrderedDict()
            TreeInfo[key]["Name"] = key
            TreeInfo[key]["Type"] = self.type
            TreeInfo[key]["Size"] = hex(len(self.Data.OriData) + self.Data.HeaderLength)
            TreeInfo[key]["DecompressedSize"] = hex(self.Data.Size)
            TreeInfo[key]["Offset"] = hex(self.Data.HOffset)
            TreeInfo[key]["FilesNum"] = len(self.Child)
        elif self is not None:
            key = str(self.Data.Name)
            TreeInfo[key] = collections.OrderedDict()
            TreeInfo[key]["Name"] = key
            TreeInfo[key]["Type"] = self.type
            TreeInfo[key]["Size"] = hex(self.Data.Size)
            TreeInfo[key]["Offset"] = hex(self.Data.HOffset)
            TreeInfo[key]["FilesNum"] = len(self.Child)

        for item in self.Child:
            TreeInfo[key].setdefault('Files',[]).append( item.ExportTree())

        return TreeInfo